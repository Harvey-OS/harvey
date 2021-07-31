#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"


/*
 * Plan 9 has two kernel allocators, the x... routines provide a first
 * fit hole allocator which should be used for permanent or large structures.
 * Routines are provided to allocate aligned memory which does not cross
 * arbitrary 2^n boundaries. A second allocator malloc, smalloc, free is
 * a 2n bucket allocator which steals from the x routines. This should
 * be used for small frequently used structures.
 */

#define	nil		((void*)0)
#define datoff		((ulong)((Xhdr*)0)->data)
#define bdatoff		((ulong)((Bucket*)0)->data)

enum
{
	Maxpow		= 18,
	CUTOFF		= 12,
	Nhole		= 128,
	Magichole	= 0xDeadBabe,
	Magic2n		= 0xFeedBeef,
	Spanlist	= 64,

	NTRACE		= 20,
};

typedef struct Hole Hole;
typedef struct Xalloc Xalloc;
typedef struct Xhdr Xhdr;
typedef struct Bucket Bucket;
typedef struct Arena Arena;

struct Hole
{
	ulong	addr;
	ulong	size;
	ulong	top;
	Hole	*link;
};

struct Xhdr
{
	ulong	size;
	ulong	magix;
	char	data[1];
};

struct Xalloc
{
	Lock;
	Hole	hole[Nhole];
	Hole	*flist;
	Hole	*table;
};

struct Bucket
{
	int	size;
	int	magic;
	Bucket	*next;
	ulong	pc;
	char	data[1];
};

struct Arena
{
	Lock;
	Bucket	*btab[Maxpow];
	int	nbuck[Maxpow];
	struct{
		ulong	pc;
		ulong	alloc;
		ulong	free;
	}	trace[NTRACE];
	QLock	rq;
	Rendez	r;
};

static Arena	arena;
static Xalloc	xlists;

void
xinit(void)
{
	Hole *h, *eh;
	int up, np0, np1;

	eh = &xlists.hole[Nhole-1];
	for(h = xlists.hole; h < eh; h++)
		h->link = h+1;

	xlists.flist = xlists.hole;

	up = conf.upages;
	np1 = up;
	if(np1 > conf.npage1)
		np1 = conf.npage1;

	palloc.p1 = conf.base1 + (conf.npage1 - np1)*BY2PG;
	conf.npage1 -= np1;
	xhole(conf.base1, conf.npage1*BY2PG);
	conf.npage1 = conf.base1+(conf.npage1*BY2PG);
	up -= np1;

	np0 = up;
	if(np0 > conf.npage0)
		np0 = conf.npage0;

	palloc.p0 = conf.base0 + (conf.npage0 - np0)*BY2PG;
	conf.npage0 -= np0;
	xhole(conf.base0, conf.npage0*BY2PG);
	conf.npage0 = conf.base0+(conf.npage0*BY2PG);

	palloc.np0 = np0;
	palloc.np1 = np1;
	/* Save the bounds of kernel alloc memory for kernel mmu mapping */
	conf.base0 = (ulong)KADDR(conf.base0);
	conf.base1 = (ulong)KADDR(conf.base1);
	conf.npage0 = (ulong)KADDR(conf.npage0);
	conf.npage1 = (ulong)KADDR(conf.npage1);
}

/*
 * NB. spanalloc memory will cause a panic if free'd
 */
void*
xspanalloc(ulong size, int align, ulong span)
{
	int i, j;
	ulong a, p, sinc;
	ulong ptr[Spanlist];

	sinc = size/8;
	span = ~(span-1);
	for(i = 0; i < Spanlist; i++) {
		p = (ulong)xalloc(size+align);
		if(p == 0)
			break;

		a = p+align;
		a &= ~(align-1);
		if((a&span) == ((a+size)&span)) {
			for(j = 0; j < i; j++)
				if(ptr[j])
					xfree((void*)ptr[j]);

			return (void*)a;
		}
		xfree((void*)p);
		ptr[i] = (ulong)xalloc(sinc);
	}
	USED(sinc);
	xsummary();
	panic("xspanalloc: %d %d %lux\n", size, align, span);	
	return 0;
}

void*
xalloc(ulong size)
{
	Xhdr *p;
	Hole *h, **l;

	size += BY2WD + sizeof(Xhdr);
	size &= ~(BY2WD-1);

	lock(&xlists);
	l = &xlists.table;
	for(h = *l; h; h = h->link) {
		if(h->size >= size) {
			p = (Xhdr*)h->addr;
			h->addr += size;
			h->size -= size;
			if(h->size == 0) {
				*l = h->link;
				h->link = xlists.flist;
				xlists.flist = h;
			}
			unlock(&xlists);
			p = KADDR(p);
			memset(p, 0, size);
			p->magix = Magichole;
			p->size = size;
			return p->data;
		}
		l = &h->link;
	}
	unlock(&xlists);
	return nil;
}

void
xfree(void *p)
{
	Xhdr *x;

	x = (Xhdr*)((ulong)p - datoff);
	if(x->magix != Magichole)
		panic("xfree");

	xhole(PADDR(x), x->size);
}

void
xhole(ulong addr, ulong size)
{
	ulong top;
	Hole *h, *c, **l;

	if(size == 0)
		return;

	top = addr + size;
	lock(&xlists);
	l = &xlists.table;
	for(h = *l; h; h = h->link) {
		if(h->top == addr) {
			h->size += size;
			h->top = h->addr+h->size;
			c = h->link;
			if(c && h->top == c->addr) {
				h->top += c->size;
				h->size += c->size;
				h->link = c->link;
				c->link = xlists.flist;
				xlists.flist = c;
			}
			unlock(&xlists);
			return;
		}
		if(h->addr > addr)
			break;
		l = &h->link;
	}
	if(h && top == h->addr) {
		h->addr -= size;
		h->size += size;
		unlock(&xlists);
		return;
	}

	if(xlists.flist == nil) {
		unlock(&xlists);
		print("xfree: no free holes, leaked %d bytes\n", size);
		return;
	}

	h = xlists.flist;
	xlists.flist = h->link;
	h->addr = addr;
	h->top = top;
	h->size = size;
	h->link = *l;
	*l = h;
	unlock(&xlists);
}

void
alloctrace(void *p, ulong pc)
{
	Bucket *bp;
	int i;

	bp = (Bucket*)((ulong)p - bdatoff);
	if(bp->size != 13)
		return;
	bp->pc = pc;
	lock(&arena);
	for(i = 0; i < NTRACE; i++){
		if(arena.trace[i].pc == 0)
			arena.trace[i].pc = pc;
		if(arena.trace[i].pc == pc){
			arena.trace[i].alloc++;
			break;
		}
	}
	unlock(&arena);
}

void*
malloc(ulong size)
{
	ulong next;
	int pow, n;
	Bucket *bp, *nbp;

	for(pow = 3; pow < Maxpow; pow++)
		if(size <= (1<<pow))
			goto good;

	return nil;
good:
	/* Allocate off this list */
	lock(&arena);
	bp = arena.btab[pow];
	if(bp) {
		arena.btab[pow] = bp->next;
		if(bp->magic != 0 || bp->size != pow){
			unlock(&arena);
			panic("malloc bp %lux magic %lux size %d next %lux pow %d", bp,
				bp->magic, bp->size, bp->next, pow);
		}
		bp->magic = Magic2n;
		unlock(&arena);

		memset(bp->data, 0, size);
		bp->pc = getcallerpc(((uchar*)&size) - sizeof(size));
		return  bp->data;
	}
	unlock(&arena);

	size = sizeof(Bucket)+(1<<pow);
	size += 3;
	size &= ~3;

	if(pow < CUTOFF) {
		n = (CUTOFF-pow)+2;
		bp = xalloc(size*n);
		if(bp == nil)
			return nil;

		nbp = bp;
		lock(&arena);
		arena.nbuck[pow] += n;
		while(--n) {
			next = (ulong)nbp+size;
			nbp = (Bucket*)next;
			nbp->size = pow;
			nbp->next = arena.btab[pow];
			arena.btab[pow] = nbp;
		}
		unlock(&arena);
	}
	else {
		bp = xalloc(size);
		if(bp == nil)
			return nil;

		arena.nbuck[pow]++;
	}

	bp->size = pow;
	bp->magic = Magic2n;
	bp->pc = getcallerpc(((uchar*)&size) - sizeof(size));
	return bp->data;
}

void*
smalloc(ulong size)
{
	char *s;
	void *p;
	int attempt;
	Bucket *bp;

	for(attempt = 0; attempt < 1000; attempt++) {
		p = malloc(size);
		if(p != nil) {
			bp = (Bucket*)((ulong)p - bdatoff);
			bp->pc = getcallerpc(((uchar*)&size) - sizeof(size));
			return p;
		}
		s = u->p->psstate;
		u->p->psstate = "Malloc";
		qlock(&arena.rq);
		while(waserror())
			;
		sleep(&arena.r, return0, nil);
		poperror();
		qunlock(&arena.rq);
		u->p->psstate = s;
	}
	print("%s:%d: out of memory in smalloc %d\n", u->p->text, u->p->pid, size);
	u->p->state = Broken;
	u->p->psstate = "NoMem";
	for(;;)
		sched();
	return 0;
}

int
msize(void *ptr)
{
	Bucket *bp;

	bp = (Bucket*)((ulong)ptr - bdatoff);
	if(bp->magic != Magic2n)
		panic("msize");
	return 1<<bp->size;
}

void
free(void *ptr)
{
	ulong pc, n;
	Bucket *bp, **l;

	bp = (Bucket*)((ulong)ptr - bdatoff);
	l = &arena.btab[bp->size];

	lock(&arena);
	if(bp->magic != Magic2n)
		panic("free");
	bp->magic = 0;
	bp->next = *l;
	*l = bp;
	if(bp->size == 13) {
		pc = bp->pc;
		for(n = 0; n < NTRACE; n++){
			if(arena.trace[n].pc == pc){
				arena.trace[n].free++;
				break;
			}
		}
	}
	unlock(&arena);
	if(arena.r.p)
		wakeup(&arena.r);
}

void
xsummary(void)
{
	Hole *h;
	Bucket *k;
	int i, nfree, nused;

	i = 0;
	for(h = xlists.flist; h; h = h->link)
		i++;

	print("%d holes free\n", i);
	i = 0;
	for(h = xlists.table; h; h = h->link) {
		print("%.8lux %.8lux %d\n", h->addr, h->top, h->size);
		i += h->size;
	}
	print("%d bytes free\n", i);
	nused = 0;
	for(i = 3; i < Maxpow; i++) {
		if(arena.btab[i] == 0 && arena.nbuck[i] == 0)
			continue;
		nused += arena.nbuck[i]*(1<<i);
		nfree = 0;
		for(k = arena.btab[i]; k; k = k->next)
			nfree++;
		print("%8d %4d %4d\n", 1<<i, arena.nbuck[i], nfree);
	}
	for(i = 0; i < NTRACE && arena.trace[i].pc; i++)
		print("%lux %d %d\n", arena.trace[i].pc, arena.trace[i].alloc, arena.trace[i].free);
	print("%d bytes in pool\n", nused);
}
