//#include "/usr/ehg/e.h"
#include "nat.h"
#include "block.h"

// borrowed from /sys/src/cmd/ip/ppp/block.c

enum
{
	PAD	= 128,
	NLIST	= (1<<5),
	BPOW	= 10,
	Npc=	64,		//  allocation tracing
};

typedef struct Barena Barena;
struct Barena
{
	QLock;
	Block*	list[NLIST];
};
static Barena area;

#define ADEBUG if(0)

typedef struct Arefent Arefent;
struct Arefent
{
	uint	pc;
	uint	n;
};
typedef struct Aref Aref;
struct  Aref
{
	Arefent	tab[Npc];
	QLock;
};
Aref arefblock;


//  keeping track of allocations

static ulong
callerpc(void *a)
{
	return ((ulong*)a)[-1];
}

static void
aref(Aref *ap, ulong pc)
{
	Arefent *a, *e;

	e = &ap->tab[Npc-1];
	qlock(ap);
	for(a = ap->tab; a < e; a++)
		if(a->pc == pc || a->pc == 0)
			break;
	a->pc = pc;
	a->n++;
	qunlock(ap);
}

static void
aunref(Aref *ap, ulong pc)
{
	Arefent *a, *e;

	e = &ap->tab[Npc-1];
	qlock(ap);
	for(a = ap->tab; a < e; a++)
		if(a->pc == pc || a->pc == 0)
			break;
	a->n--;
	qunlock(ap);
}


void*
emalloc(int size)
{
	void *m;
	if((m = malloc(size)) == nil)
		sysfatal("no mem");
	return m;
}

Block*
allocb(int len)
{
	int sz;
	Block *bp, **l;

	len += PAD;
	sz = (len>>BPOW)&(NLIST-1);
	
	qlock(&area);
	l = &area.list[sz];
	for(bp = *l; bp; bp = bp->flist) {
		if(bp->bsz >= len) {
			*l = bp->flist;
			qunlock(&area);

			bp->next = nil;
			bp->list = nil;
			bp->flist = nil;
			bp->base = (uchar*)bp+sizeof(Block);
			bp->rp = bp->base+PAD;
			bp->wp = bp->rp;
			bp->lim  = bp->base+bp->bsz;
			ADEBUG {
				bp->pc = callerpc(&len);
				aref(&arefblock, bp->pc);
			}
			return bp;
		}
		l = &bp->flist;
	}

	qunlock(&area);

	bp = emalloc(sizeof(Block)+len);
	//bp = malloc(sizeof(Block)+len);
	bp->bsz  = len;
	bp->base = (uchar*)bp+sizeof(Block);
	bp->rp = bp->base+PAD;
	bp->wp = bp->rp;
	bp->lim  = bp->base+len;
	ADEBUG {
		bp->pc = callerpc(&len);
		aref(&arefblock, bp->pc);
	}
	return bp;
}

void
freeb(Block *bp)
{
	int sz;
	Block **l, *next;

	qlock(&area);
	while(bp) {
		sz = (bp->bsz>>BPOW)&(NLIST-1);

		l = &area.list[sz];
		bp->flist = *l;
		*l = bp;

		next = bp->next;

		// to catch use after free
		bp->rp = (uchar*)0xdeadbabe;
		bp->wp = (uchar*)0xdeadbabe;
		bp->next = (Block*)0xdeadbabe;
		bp->list = (Block*)0xdeadbabe;

		ADEBUG aunref(&arefblock, bp->pc);

		bp = next;
	}
	qunlock(&area);
}


int
blen(Block *bp)
{
	int len;

	len = 0;
	while(bp) {
		len += BLEN(bp);
		bp = bp->next;
	}
	return len;
}


Block*
concat(Block *bp)
{
	int len;
	Block *nb, *f;

	nb = allocb(blen(bp));
	for(f = bp; f; f = f->next) {
		len = BLEN(f);
		memmove(nb->wp, f->rp, len);
		nb->wp += len;
	}
	freeb(bp);
	return nb;
}

Block *
pullup(Block *bp, int n)
{
	int i;
	Block *nbp;

	// this will usually be true
	if(BLEN(bp) >= n)
		return bp;

	// if not enough room in the first block,
	// add another to the front of the list.
	if(bp->lim - bp->rp < n){
		nbp = allocb(n);
		nbp->next = bp;
		bp = nbp;
	}

	// copy uchars from the trailing blocks into the first
	n -= BLEN(bp);
	while(nbp = bp->next){
		i = BLEN(nbp);
		if(i >= n) {
			memmove(bp->wp, nbp->rp, n);
			bp->wp += n;
			nbp->rp += n;
			return bp;
		}
		else {
			memmove(bp->wp, nbp->rp, i);
			bp->wp += i;
			bp->next = nbp->next;
			nbp->next = nil;
			freeb(nbp);
			n -= i;
		}
	}
	freeb(bp);
	return nil;
}

Block*
padb(Block *bp, int n)
{
	Block *nbp;

	if(bp->rp-bp->base >= n) {
		bp->rp -= n;
		return bp;
	} else {
		nbp = allocb(n);
		nbp->wp = nbp->lim;
		nbp->rp = nbp->wp - n;
		nbp->next = bp;
		return nbp;
	}
} 


Block *
btrim(Block *bp, int offset, int len)
{
	ulong l;
	Block *nb, *startb;

	if(blen(bp) < offset+len) {
		freeb(bp);
		return nil;
	}

	while((l = BLEN(bp)) < offset) {
		offset -= l;
		nb = bp->next;
		bp->next = nil;
		freeb(bp);
		bp = nb;
	}

	startb = bp;
	bp->rp += offset;

	while((l = BLEN(bp)) < len) {
		len -= l;
		bp = bp->next;
	}

	bp->wp -= (BLEN(bp) - len);

	if(bp->next) {
		freeb(bp->next);
		bp->next = nil;
	}

	return startb;
}

Block*
copyb(Block *bp, int count)
{
	int l;
	Block *nb, *head, **p;

	p = &head;
	head = nil;
	while(bp != nil && count != 0) {
		l = BLEN(bp);
		if(count < l)
			l = count;
		nb = allocb(l);
		memmove(nb->wp, bp->rp, l);
		nb->wp += l;
		count -= l;
		*p = nb;
		p = &nb->next;
		bp = bp->next;
	}
	if(count) {
		nb = allocb(count);
		memset(nb->wp, 0, count);
		nb->wp += count;
		*p = nb;
	}
	if(blen(head) == 0)
		fprint(2, "copyb: zero length\n");

	return head;
}

int
pullb(Block **bph, int count)
{
	Block *bp;
	int n, uchars;

	uchars = 0;
	if(bph == nil)
		return 0;

	while(*bph != nil && count != 0) {
		bp = *bph;
		n = BLEN(bp);
		if(count < n)
			n = count;
		uchars += n;
		count -= n;
		bp->rp += n;
		if(BLEN(bp) == 0) {
			*bph = bp->next;
			bp->next = nil;
			freeb(bp);
		}
	}
	return uchars;
}
