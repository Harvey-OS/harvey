#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include "ppp.h"

enum
{
	PAD	= 128,
	NLIST	= (1<<5),
	BPOW	= 10,
};

typedef struct Barena Barena;
struct Barena
{
	QLock;
	Block*	list[NLIST];
};
static Barena area;

#define ADEBUG if(0)


/*
 *  allocation tracing
 */
enum
{
	Npc=	64,
};
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

static ulong	callerpc(void*);
static void	aref(Aref*, ulong);
static void	aunref(Aref*, ulong);

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
			bp->rptr = bp->base+PAD;
			bp->wptr = bp->rptr;
			bp->lim  = bp->base+bp->bsz;
			bp->flow = nil;
			bp->flags= 0;
			ADEBUG {
				bp->pc = callerpc(&len);
				aref(&arefblock, bp->pc);
			}
			return bp;
		}
		l = &bp->flist;
	}

	qunlock(&area);

	bp = mallocz(sizeof(Block)+len, 1);

	bp->bsz  = len;
	bp->base = (uchar*)bp+sizeof(Block);
	bp->rptr = bp->base+PAD;
	bp->wptr = bp->rptr;
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

		/* to catch use after free */
		bp->rptr = (uchar*)0xdeadbabe;
		bp->wptr = (uchar*)0xdeadbabe;
		bp->next = (Block*)0xdeadbabe;
		bp->list = (Block*)0xdeadbabe;

		ADEBUG aunref(&arefblock, bp->pc);

		bp = next;
	}
	qunlock(&area);
}

/*
 *  concatenate a list of blocks into a single one and make sure
 *  the result is at least min uchars
 */
Block*
concat(Block *bp)
{
	int len;
	Block *nb, *f;

	nb = allocb(blen(bp));
	for(f = bp; f; f = f->next) {
		len = BLEN(f);
		memmove(nb->wptr, f->rptr, len);
		nb->wptr += len;
	}
	freeb(bp);
	return nb;
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

Block *
pullup(Block *bp, int n)
{
	int i;
	Block *nbp;

	/*
	 *  this should almost always be true, the rest it
	 *  just for to avoid every caller checking.
	 */
	if(BLEN(bp) >= n)
		return bp;

	/*
	 *  if not enough room in the first block,
	 *  add another to the front of the list.
	 */
	if(bp->lim - bp->rptr < n){
		nbp = allocb(n);
		nbp->next = bp;
		bp = nbp;
	}

	/*
	 *  copy uchars from the trailing blocks into the first
	 */
	n -= BLEN(bp);
	while(nbp = bp->next){
		i = BLEN(nbp);
		if(i >= n) {
			memmove(bp->wptr, nbp->rptr, n);
			bp->wptr += n;
			nbp->rptr += n;
			return bp;
		}
		else {
			memmove(bp->wptr, nbp->rptr, i);
			bp->wptr += i;
			bp->next = nbp->next;
			nbp->next = nil;
			freeb(nbp);
			n -= i;
		}
	}
	freeb(bp);
	return nil;
}

/*
 *  Pad a block to the front with n uchars.  This is used to add protocol
 *  headers to the front of blocks.
 */
Block*
padb(Block *bp, int n)
{
	Block *nbp;

	if(bp->rptr-bp->base >= n) {
		bp->rptr -= n;
		return bp;
	}
	else {
		/* fprint(2, "padb: required %d PAD %d\n", n, PAD) = malloc(sizeof(*required %d PAD %d\n", n, PAD))); */
		nbp = allocb(n);
		nbp->wptr = nbp->lim;
		nbp->rptr = nbp->wptr - n;
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
	bp->rptr += offset;

	while((l = BLEN(bp)) < len) {
		len -= l;
		bp = bp->next;
	}

	bp->wptr -= (BLEN(bp) - len);
	bp->flags |= S_DELIM;

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
		memmove(nb->wptr, bp->rptr, l);
		nb->wptr += l;
		count -= l;
		nb->flags = bp->flags;
		*p = nb;
		p = &nb->next;
		bp = bp->next;
	}
	if(count) {
		nb = allocb(count);
		memset(nb->wptr, 0, count);
		nb->wptr += count;
		nb->flags |= S_DELIM;
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
		bp->rptr += n;
		if(BLEN(bp) == 0) {
			*bph = bp->next;
			bp->next = nil;
			freeb(bp);
		}
	}
	return uchars;
}

/*
 *  handy routines for keeping track of allocations
 */
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
