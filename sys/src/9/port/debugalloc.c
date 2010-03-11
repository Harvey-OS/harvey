#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"pool.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#define left	u.s.bhl
#define right	u.s.bhr
#define fwd	u.s.bhf
#define prev	u.s.bhv
#define parent	u.s.bhp

typedef struct Bhdr	Bhdr;

struct Bhdr {
	ulong	magic;
	ulong	size;
};
enum {
	NOT_MAGIC = 0xdeadfa11,
};

struct Pool
{
	char*	name;
	ulong	maxsize;
	int	quanta;
	int	chunk;
	ulong	cursize;
	ulong	arenasize;
	ulong	hw;
	Lock	l;
	Bhdr*	root;
	Bhdr*	chain;
	int	nalloc;
	int	nfree;
	int	nbrk;
	int	lastfree;
	void	(*move)(void*, void*);
};

struct
{
	int	n;
	Pool	pool[MAXPOOL];
	Lock	l;
} table = {
	2,
	{
		{ "Main",	 4*1024*1024, 31,  128*1024 },
		{ "Image",	 16*1024*1024, 31, 2*1024*1024 },
	}
};

Pool*	mainmem = &table.pool[0];
Pool*	imagmem = &table.pool[1];

int	poolcompact(Pool*);

Bhdr*
poolchain(Pool *p)
{
	return p->chain;
}

void
pooldel(Pool *p, Bhdr *t)
{
	Bhdr *s, *f, *rp, *q;

	if(t->parent == nil && p->root != t) {
		t->prev->fwd = t->fwd;
		t->fwd->prev = t->prev;
		return;
	}

	if(t->fwd != t) {
		f = t->fwd;
		s = t->parent;
		f->parent = s;
		if(s == nil)
			p->root = f;
		else {
			if(s->left == t)
				s->left = f;
			else
				s->right = f;
		}

		rp = t->left;
		f->left = rp;
		if(rp != nil)
			rp->parent = f;
		rp = t->right;
		f->right = rp;
		if(rp != nil)
			rp->parent = f;

		t->prev->fwd = t->fwd;
		t->fwd->prev = t->prev;
		return;
	}

	if(t->left == nil)
		rp = t->right;
	else {
		if(t->right == nil)
			rp = t->left;
		else {
			f = t;
			rp = t->right;
			s = rp->left;
			while(s != nil) {
				f = rp;
				rp = s;
				s = rp->left;
			}
			if(f != t) {
				s = rp->right;
				f->left = s;
				if(s != nil)
					s->parent = f;
				s = t->right;
				rp->right = s;
				if(s != nil)
					s->parent = rp;
			}
			s = t->left;
			rp->left = s;
			s->parent = rp;
		}
	}
	q = t->parent;
	if(q == nil)
		p->root = rp;
	else {
		if(t == q->left)
			q->left = rp;
		else
			q->right = rp;
	}
	if(rp != nil)
		rp->parent = q;
}

void
pooladd(Pool *p, Bhdr *q)
{
	int size;
	Bhdr *tp, *t;

	q->magic = MAGIC_F;

	q->left = nil;
	q->right = nil;
	q->parent = nil;
	q->fwd = q;
	q->prev = q;

	t = p->root;
	if(t == nil) {
		p->root = q;
		return;
	}

	size = q->size;

	tp = nil;
	while(t != nil) {
		if(size == t->size) {
			q->fwd = t->fwd;
			q->fwd->prev = q;
			q->prev = t;
			t->fwd = q;
			return;
		}
		tp = t;
		if(size < t->size)
			t = t->left;
		else
			t = t->right;
	}

	q->parent = tp;
	if(size < tp->size)
		tp->left = q;
	else
		tp->right = q;
}

void*
poolalloc(Pool *p, int size)
{
	Bhdr *q, *t;
	int alloc, ldr, ns, frag;

	if(size < 0 || size >= 1024*1024*1024)	/* for sanity and to avoid overflow */
		return nil;
	size = (size + BHDRSIZE + p->quanta) & ~(p->quanta);

	ilock(&p->l);
	p->nalloc++;

	t = p->root;
	q = nil;
	while(t) {
		if(t->size == size) {
			pooldel(p, t);
			t->magic = MAGIC_A;
			p->cursize += t->size;
			if(p->cursize > p->hw)
				p->hw = p->cursize;
			iunlock(&p->l);
			return B2D(t);
		}
		if(size < t->size) {
			q = t;
			t = t->left;
		}
		else
			t = t->right;
	}
	if(q != nil) {
		pooldel(p, q);
		q->magic = MAGIC_A;
		frag = q->size - size;
		if(frag < (size>>2)) {
			p->cursize += q->size;
			if(p->cursize > p->hw)
				p->hw = p->cursize;
			iunlock(&p->l);
			return B2D(q);
		}
		/* Split */
		ns = q->size - size;
		q->size = size;
		B2T(q)->hdr = q;
		t = B2NB(q);
		t->size = ns;
		B2T(t)->hdr = t;
		pooladd(p, t);
		p->cursize += q->size;
		if(p->cursize > p->hw)
			p->hw = p->cursize;
		iunlock(&p->l);
		return B2D(q);
	}

	ns = p->chunk;
	if(size > ns)
		ns = size;
	ldr = p->quanta+1;

	alloc = ns+ldr+sizeof(t->magic);
	p->arenasize += alloc;
	if(p->arenasize > p->maxsize) {
		p->arenasize -= alloc;

		if(poolcompact(p)) {
			iunlock(&p->l);
			return poolalloc(p, size);
		}

		iunlock(&p->l);
		print("%s arena too large: size %d cursize %lud arenasize %lud maxsize %lud\n",
			 p->name, size, p->cursize, p->arenasize, p->maxsize);
		return nil;
	}

	p->nbrk++;
	t = xalloc(alloc);
	if(t == nil) {
		iunlock(&p->l);
		return nil;
	}

	t->magic = MAGIC_E;		/* Make a leader */
	t->size = ldr;
	t->csize = ns+ldr;
	t->clink = p->chain;
	p->chain = t;
	B2T(t)->hdr = t;
	t = B2NB(t);

	t->magic = MAGIC_A;		/* Make the block we are going to return */
	t->size = size;
	B2T(t)->hdr = t;
	q = t;

	ns -= size;			/* Free the rest */
	if(ns > 0) {
		q = B2NB(t);
		q->size = ns;
		B2T(q)->hdr = q;
		pooladd(p, q);
	}
	B2NB(q)->magic = MAGIC_E;	/* Mark the end of the chunk */

	p->cursize += t->size;
	if(p->cursize > p->hw)
		p->hw = p->cursize;
	iunlock(&p->l);
	return B2D(t);
}

void
poolfree(Pool *p, void *v)
{
	Bhdr *b, *c;

	D2B(b, v);

	ilock(&p->l);
	p->nfree++;
	p->cursize -= b->size;

	c = B2NB(b);
	if(c->magic == MAGIC_F) {	/* Join forward */
		pooldel(p, c);
		c->magic = 0;
		b->size += c->size;
		B2T(b)->hdr = b;
	}

	c = B2PT(b)->hdr;
	if(c->magic == MAGIC_F) {	/* Join backward */
		pooldel(p, c);
		b->magic = 0;
		c->size += b->size;
		b = c;
		B2T(b)->hdr = b;
	}

	pooladd(p, b);
	iunlock(&p->l);
}

int
poolread(char *va, int count, ulong offset)
{
	Pool *p;
	int n, i, signed_off;

	n = 0;
	signed_off = offset;
	for(i = 0; i < table.n; i++) {
		p = &table.pool[i];
		n += snprint(va+n, count-n, "%11lud %11lud %11lud %11d %11d %11d %s\n",
			p->cursize,
			p->maxsize,
			p->hw,
			p->nalloc,
			p->nfree,
			p->nbrk,
			p->name);

		if(signed_off > 0) {
			signed_off -= n;
			if(signed_off < 0) {
				memmove(va, va+n+signed_off, -signed_off);
				n = -signed_off;
			}
			else
				n = 0;
		}

	}
	return n;
}

Lock pcxlock;
struct {
	ulong	n;
	ulong	pc;
} pcx[1024];

static void
remember(ulong pc, void *v)
{
	Bhdr *b;
	int i;

	if(v == nil)
		return;

	D2B(b, v);
	if((b->size>>5) != 2)
		return;

	ilock(&pcxlock);
	B2T(b)->pad = 0;
	for(i = 0; i < 1024; i++)
		if(pcx[i].pc == pc || pcx[i].pc == 0){
			pcx[i].pc = pc;
			pcx[i].n++;
			B2T(b)->pad = i;
			break;
		}
	iunlock(&pcxlock);
}

static void
forget(void *v)
{
	Bhdr *b;

	if(v == nil)
		return;

	D2B(b, v);
	if((b->size>>5) != 2)
		return;

	ilock(&pcxlock);
	pcx[B2T(b)->pad].n--;
	iunlock(&pcxlock);
}

void*
malloc(ulong size)
{
	void *v;

	v = poolalloc(mainmem, size);
remember(getcallerpc(&size), v);
	if(v != nil)
		memset(v, 0, size);
	return v;
}

void*
smalloc(ulong size)
{
	void *v;

	for(;;) {
		v = poolalloc(mainmem, size);
remember(getcallerpc(&size), v);
		if(v != nil)
			break;
		tsleep(&up->sleep, return0, 0, 100);
	}
	memset(v, 0, size);
	return v;
}

void*
mallocz(ulong size, int clr)
{
	void *v;

	v = poolalloc(mainmem, size);
remember(getcallerpc(&size), v);
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void
free(void *v)
{
	Bhdr *b;

	if(v != nil) {
forget(v);
		D2B(b, v);
		poolfree(mainmem, v);
	}
}

void*
realloc(void *v, ulong size)
{
	Bhdr *b;
	void *nv;
	int osize;

	if(v == nil)
		return malloc(size);

	D2B(b, v);

	osize = b->size - BHDRSIZE;
	if(osize >= size)
		return v;

	nv = poolalloc(mainmem, size);
remember(getcallerpc(&v), nv);
	if(nv != nil) {
		memmove(nv, v, osize);
		free(v);
	}
	return nv;
}

int
msize(void *v)
{
	Bhdr *b;

	D2B(b, v);
	return b->size - BHDRSIZE;
}

void*
calloc(ulong n, ulong szelem)
{
	return malloc(n*szelem);
}

/*
void
pooldump(Bhdr *b, int d, int c)
{
	Bhdr *t;

	if(b == nil)
		return;

	print("%.8lux %.8lux %.8lux %c %4d %d (f %.8lux p %.8lux)\n",
		b, b->left, b->right, c, d, b->size, b->fwd, b->prev);
	d++;
	for(t = b->fwd; t != b; t = t->fwd)
		print("\t%.8lux %.8lux %.8lux\n", t, t->prev, t->fwd);
	pooldump(b->left, d, 'l');
	pooldump(b->right, d, 'r');
}


void
poolshow(void)
{
	int i;

	for(i = 0; i < table.n; i++) {
		print("Arena: %s root=%.8lux\n", table.pool[i].name, table.pool[i].root);
		pooldump(table.pool[i].root, 0, 'R');
	}
}
*/

void
poolsummary(void)
{
	int i;

	for(i = 0; i < table.n; i++)
		print("Arena: %s cursize=%lud; maxsize=%lud\n",
			table.pool[i].name,
			table.pool[i].cursize,
			table.pool[i].maxsize);
}

/*
void
pooldump(Pool *p)
{
	Bhdr *b, *base, *limit, *ptr;

	b = p->chain;
	if(b == nil)
		return;
	base = b;
	ptr = b;
	limit = B2LIMIT(b);

	while(base != nil) {
		print("\tbase #%.8lux ptr #%.8lux", base, ptr);
		if(ptr->magic == MAGIC_A)
			print("\tA%.5d\n", ptr->size);
		else if(ptr->magic == MAGIC_E)
			print("\tE\tL#%.8lux\tS#%.8lux\n", ptr->clink, ptr->csize);
		else
			print("\tF%.5d\tL#%.8lux\tR#%.8lux\tF#%.8lux\tP#%.8lux\tT#%.8lux\n",
				ptr->size, ptr->left, ptr->right, ptr->fwd, ptr->prev, ptr->parent);
		ptr = B2NB(ptr);
		if(ptr >= limit) {
			print("link to #%.8lux\n", base->clink);
			base = base->clink;
			if(base == nil)
				break;
			ptr = base;
			limit = B2LIMIT(base);
		}
	}
	return;
}
*/

void
poolsetcompact(Pool *p, void (*move)(void*, void*))
{
	p->move = move;
}

void
poolsetparam(char *name, ulong maxsize, int quanta, int chunk)
{
	Pool *p;
	int i;

	for(i=0; i<table.n; i++){
		p = &table.pool[i];
		if(strcmp(name, p->name) == 0){
			if(maxsize)
				p->maxsize = maxsize;
			if(quanta)
				p->quanta = quanta;
			if(chunk)
				p->chunk = chunk;
			return;
		}
	}
}

int
poolcompact(Pool *pool)
{
	Bhdr *base, *limit, *ptr, *end, *next;
	int compacted, recov, nb;

	if(pool->move == nil || pool->lastfree == pool->nfree)
		return 0;

	pool->lastfree = pool->nfree;

	base = pool->chain;
	ptr = B2NB(base);	/* First Block in arena has clink */
	limit = B2LIMIT(base);
	compacted = 0;

	pool->root = nil;
	end = ptr;
	recov = 0;
	while(base != nil) {
		next = B2NB(ptr);
		if(ptr->magic == MAGIC_A) {
			if(ptr != end) {
				memmove(end, ptr, ptr->size);
				pool->move(B2D(ptr), B2D(end));
				recov = (uchar*)ptr - (uchar*)end;
				compacted = 1;
			}
			end = B2NB(end);
		}
		if(next >= limit) {
			nb = (uchar*)limit - (uchar*)end;
			//print("recovered %d bytes\n", recov);
			//print("%d bytes at end\n", nb);
			USED(recov);
			if(nb > 0){
				if(nb < pool->quanta+1)
					panic("poolcompact: leftover too small\n");
				end->size = nb;
				pooladd(pool, end);
			}
			base = base->clink;
			if(base == nil)
				break;
			ptr = B2NB(base);
			end = ptr;	/* could do better by copying between chains */
			limit = B2LIMIT(base);
			recov = 0;
		} else
			ptr = next;
	}

	return compacted;
}

int
recur(Bhdr *t)
{
	if(t == 0)
		return 1;
	if((uintptr)t < KZERO || (uintptr)t - KZERO > 0x10000000)
		return 0;
	return recur(t->right) && recur(t->left);
}
