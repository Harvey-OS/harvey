/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * malloc
 *
 *	Uses Quickfit (see SIGPLAN Notices October 1988)
 *	with allocator from Kernighan & Ritchie
 *
 * This is a placeholder.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

typedef double Align;
typedef union Header Header;
typedef struct Qlist Qlist;

union Header {
	struct {
		Header*	next;
		uint	size;
	} s;
	Align	al;
};

struct Qlist {
	Lock	lk;
	Header*	first;

	uint	nalloc;
};

enum {
	Unitsz		= sizeof(Header),	/* 16 bytes on amd64 */
};

#define	NUNITS(n)	(HOWMANY(n, Unitsz) + 1)
#define	NQUICK		((512/Unitsz)+1)	/* 33 on amd64 */

static	Qlist	quicklist[NQUICK+1];
static	Header	misclist;
static	Header	*rover;
static	unsigned tailsize;
static	unsigned tailnunits;
static	Header	*tailbase;
static	Header	*tailptr;
static	Header	checkval;
static	int	morecore(unsigned);

enum
{
	QSmalign = 0,
	QSmalignquick,
	QSmalignrover,
	QSmalignfront,
	QSmalignback,
	QSmaligntail,
	QSmalignnottail,
	QSmalloc,
	QSmallocrover,
	QSmalloctail,
	QSfree,
	QSfreetail,
	QSfreequick,
	QSfreenext,
	QSfreeprev,
	QSmax
};

static	void	qfreeinternal(void*);
static	int	qstats[QSmax];
static	char*	qstatstr[QSmax] = {
[QSmalign]		"malign",
[QSmalignquick]	"malignquick",
[QSmalignrover]	"malignrover",
[QSmalignfront]	"malignfront",
[QSmalignback]	"malignback",
[QSmaligntail]	"maligntail",
[QSmalignnottail]	"malignnottail",
[QSmalloc]		"malloc",
[QSmallocrover]	"mallocrover",
[QSmalloctail]	"malloctail",
[QSfree]		"free",
[QSfreetail]	"freetail",
[QSfreequick]	"freequick",
[QSfreenext]	"freenext",
[QSfreeprev]	"freeprev",
};

static	Lock		mainlock;

#define	MLOCK		ilock(&(&mainlock)->lock)
#define	MUNLOCK		iunlock(&(&mainlock)->lock)
#define QLOCK(l)	ilock(&l->lock)
#define QUNLOCK(l)	iunlock(&l->lock)

#define	tailalloc(p, n)	((p)=tailptr, tailsize -= (n), tailptr+=(n),\
			 (p)->s.size=(n), (p)->s.next = &checkval)

#define ISPOWEROF2(x)	(/*((x) != 0) && */!((x) & ((x)-1)))
#define ALIGNHDR(h, a)	(Header*)((((uintptr)(h))+((a)-1)) & ~((a)-1))


/*
 * Experiment: per-core quick lists.
 * change quicklist to be
 * static	Qlist	quicklist[MACHMAX][NQUICK+1];
 * and define QLIST to be quicklist[machp()->machno]
 *
 * using quicklist[machp()->machno] runs out of memory soon.
 * using quicklist[machp()->machno%4] yields times worse than using quicklist!
 */
#define QLIST	quicklist

static void*
qmallocalign(usize nbytes, uintptr_t align, int32_t offset, usize span)
{
	Qlist *qlist;
	uintptr_t aligned;
	Header **pp, *p, *q, *r;
	uint naligned, nunits, n;

	if(nbytes == 0 || offset != 0 || span != 0)
		return nil;

	if(!ISPOWEROF2(align) || align < sizeof(Header))
		return nil;

	qstats[QSmalign]++;
	nunits = NUNITS(nbytes);
	if(nunits <= NQUICK){
		/*
		 * Look for a conveniently aligned block
		 * on one of the quicklists.
		 */
		qlist = &QLIST[nunits];
		QLOCK(&qlist->lk);
		pp = &qlist->first;
		for(p = *pp; p != nil; p = p->s.next){
			if(ALIGNED(p+1, align)){
				*pp = p->s.next;
				p->s.next = &checkval;
				QUNLOCK(&qlist->lk);
				qstats[QSmalignquick]++;
				return p+1;
			}
			pp = &p->s.next;
		}
		QUNLOCK(&qlist->lk);
	}

	MLOCK;
	if(nunits > tailsize) {
		/* hard way */
		if((q = rover) != nil){
			do {
				p = q->s.next;
				if(p->s.size < nunits)
					continue;
				aligned = ALIGNED(p+1, align);
				naligned = NUNITS(align)-1;
				if(!aligned && p->s.size < nunits+naligned)
					continue;

				/*
				 * This block is big enough, remove it
				 * from the list.
				 */
				q->s.next = p->s.next;
				rover = q;
				qstats[QSmalignrover]++;

				/*
				 * Free any runt in front of the alignment.
				 */
				if(!aligned){
					r = p;
					p = ALIGNHDR(p+1, align) - 1;
					n = p - r;
					p->s.size = r->s.size - n;

					r->s.size = n;
					r->s.next = &checkval;
					qfreeinternal(r+1);
					qstats[QSmalignfront]++;
				}

				/*
				 * Free any residue after the aligned block.
				 */
				if(p->s.size > nunits){
					r = p+nunits;
					r->s.size = p->s.size - nunits;
					r->s.next = &checkval;
					qstats[QSmalignback]++;
					qfreeinternal(r+1);

					p->s.size = nunits;
				}

				p->s.next = &checkval;
				MUNLOCK;
				return p+1;
			} while((q = p) != rover);
		}
		if((n = morecore(nunits)) == 0){
			MUNLOCK;
			return nil;
		}
		tailsize += n;
	}

	q = ALIGNHDR(tailptr+1, align);
	if(q == tailptr+1){
		tailalloc(p, nunits);
		qstats[QSmaligntail]++;
	}
	else{
		naligned = NUNITS(align)-1;
		if(tailsize < nunits+naligned){
			/*
			 * There are at least nunits,
			 * get enough for alignment.
			 */
			if((n = morecore(naligned)) == 0){
				MUNLOCK;
				return nil;
			}
			tailsize += n;
		}
		/*
		 * Save the residue before the aligned allocation
		 * and free it after the tail pointer has been bumped
		 * for the main allocation.
		 */
		n = q-tailptr - 1;
		tailalloc(r, n);
		tailalloc(p, nunits);
		qstats[QSmalignnottail]++;
		qfreeinternal(r+1);
	}
	MUNLOCK;

	return p+1;
}

static void*
qmalloc(usize nbytes)
{
	Qlist *qlist;
	Header *p, *q;
	uint nunits, n;

///* FIXME: (ignore for now)
	if(nbytes == 0)
		return nil;
//*/

	qstats[QSmalloc]++;
	nunits = NUNITS(nbytes);
	if(nunits <= NQUICK){
		qlist = &QLIST[nunits];
		QLOCK(&qlist->lk);
		if((p = qlist->first) != nil){
			qlist->first = p->s.next;
			qlist->nalloc++;
			QUNLOCK(&qlist->lk);
			p->s.next = &checkval;
			return p+1;
		}
		QUNLOCK(&qlist->lk);
	}

	MLOCK;
	if(nunits > tailsize) {
		/* hard way */
		if((q = rover) != nil){
			do {
				p = q->s.next;
				if(p->s.size >= nunits) {
					if(p->s.size > nunits) {
						p->s.size -= nunits;
						p += p->s.size;
						p->s.size = nunits;
					} else
						q->s.next = p->s.next;
					p->s.next = &checkval;
					rover = q;
					qstats[QSmallocrover]++;
					MUNLOCK;
					return p+1;
				}
			} while((q = p) != rover);
		}
		if((n = morecore(nunits)) == 0){
			MUNLOCK;
			return nil;
		}
		tailsize += n;
	}
	qstats[QSmalloctail]++;
	tailalloc(p, nunits);
	MUNLOCK;

	return p+1;
}

static void
qfreeinternal(void* ap)
{
	Qlist *qlist;
	Header *p, *q;
	uint nunits;

	if(ap == nil)
		return;
	qstats[QSfree]++;

	p = (Header*)ap - 1;
	if((nunits = p->s.size) == 0 || p->s.next != &checkval)
		panic("malloc: corrupt allocation arena\n");
	if(tailptr != nil && p+nunits == tailptr) {
		/* block before tail */
		tailptr = p;
		tailsize += nunits;
		qstats[QSfreetail]++;
		return;
	}
	if(nunits <= NQUICK) {
		qlist = &QLIST[nunits];
		QLOCK(&qlist->lk);
		p->s.next = qlist->first;
		qlist->first = p;
		QUNLOCK(&qlist->lk);
		qstats[QSfreequick]++;
		return;
	}
	if((q = rover) == nil) {
		q = &misclist;
		q->s.size = 0;
		q->s.next = q;
	}
	for(; !(p > q && p < q->s.next); q = q->s.next)
		if(q >= q->s.next && (p > q || p < q->s.next))
			break;
	if(p+p->s.size == q->s.next) {
		p->s.size += q->s.next->s.size;
		p->s.next = q->s.next->s.next;
		qstats[QSfreenext]++;
	} else
		p->s.next = q->s.next;
	if(q+q->s.size == p) {
		q->s.size += p->s.size;
		q->s.next = p->s.next;
		qstats[QSfreeprev]++;
	} else
		q->s.next = p;
	rover = q;
}

uint32_t
msize(void* ap)
{
	Header *p;
	uint nunits;

	if(ap == nil)
		return 0;

	p = (Header*)ap - 1;
	if((nunits = p->s.size) == 0 || p->s.next != &checkval)
		panic("malloc: corrupt allocation arena\n");

	return (nunits-1) * sizeof(Header);
}

static void
mallocreadfmt(char* s, char* e)
{
	char *p;
	Header *q;
	int i, n, t;
	Qlist *qlist;

	p = seprint(s, e,
		"%llud memory\n"
		"%d pagesize\n"
		"%llud kernel\n",
		(uint64_t)conf.npage*PGSZ,
		PGSZ,
		(uint64_t)conf.npage-conf.upages);

	t = 0;
	for(i = 0; i <= NQUICK; i++) {
		n = 0;
		qlist = &QLIST[i];
		QLOCK(&qlist->lk);
		for(q = qlist->first; q != nil; q = q->s.next){
//			if(q->s.size != i)
//				p = seprint(p, e, "q%d\t%#p\t%ud\n",
//					i, q, q->s.size);
			n++;
		}
		QUNLOCK(&qlist->lk);

//		if(n != 0)
//			p = seprint(p, e, "q%d %d\n", i, n);
		t += n * i*sizeof(Header);
	}
	p = seprint(p, e, "quick: %ud bytes total\n", t);

	MLOCK;
	if((q = rover) != nil){
		i = t = 0;
		do {
			t += q->s.size;
			i++;
//			p = seprint(p, e, "m%d\t%#p\n", q->s.size, q);
		} while((q = q->s.next) != rover);

		p = seprint(p, e, "rover: %d blocks %ud bytes total\n",
			i, t*sizeof(Header));
	}
	p = seprint(p, e, "total allocated %lud, %ud remaining\n",
		(tailptr-tailbase)*sizeof(Header), tailnunits*sizeof(Header));

	for(i = 0; i < nelem(qstats); i++){
		if(qstats[i] == 0)
			continue;
		p = seprint(p, e, "%s %ud\n", qstatstr[i], qstats[i]);
	}
	MUNLOCK;
}

int32_t
mallocreadsummary(Chan* c, void *a, int32_t n, int32_t offset)
{
	char *alloc;

	alloc = malloc(16*READSTR);
	mallocreadfmt(alloc, alloc+16*READSTR);
	n = readstr(offset, a, n, alloc);
	free(alloc);

	return n;
}

void
mallocsummary(void)
{
	Header *q;
	int i, n, t;
	Qlist *qlist;

	t = 0;
	for(i = 0; i <= NQUICK; i++) {
		n = 0;
		qlist = &QLIST[i];
		QLOCK(&qlist->lk);
		for(q = qlist->first; q != nil; q = q->s.next){
			if(q->s.size != i)
				DBG("q%d\t%#p\t%ud\n", i, q, q->s.size);
			n++;
		}
		QUNLOCK(&qlist->lk);

		t += n * i*sizeof(Header);
	}
	print("quick: %ud bytes total\n", t);

	MLOCK;
	if((q = rover) != nil){
		i = t = 0;
		do {
			t += q->s.size;
			i++;
		} while((q = q->s.next) != rover);
	}
	MUNLOCK;

	if(i != 0){
		print("rover: %d blocks %ud bytes total\n",
			i, t*sizeof(Header));
	}
	print("total allocated %lud, %ud remaining\n",
		(tailptr-tailbase)*sizeof(Header), tailnunits*sizeof(Header));

	for(i = 0; i < nelem(qstats); i++){
		if(qstats[i] == 0)
			continue;
		print("%s %ud\n", qstatstr[i], qstats[i]);
	}
}

void
free(void* ap)
{
	MLOCK;
	qfreeinternal(ap);
	MUNLOCK;
}

void*
malloc(uint32_t size)
{
	void* v;

	if((v = qmalloc(size)) != nil)
		memset(v, 0, size);

	return v;
}

void*
mallocz(uint32_t size, int clr)
{
	void *v;

	if((v = qmalloc(size)) != nil && clr)
		memset(v, 0, size);

	return v;
}

void*
mallocalign(uint32_t nbytes, uint32_t align, int32_t offset, uint32_t span)
{
	void *v;

	/*
	 * Should this memset or should it be left to the caller?
	 */
	if((v = qmallocalign(nbytes, align, offset, span)) != nil)
		memset(v, 0, nbytes);

	return v;
}

void*
smalloc(uint32_t size)
{
	Proc *up = externup();
	void *v;

	while((v = malloc(size)) == nil)
		tsleep(&up->sleep, return0, 0, 100);
	memset(v, 0, size);

	return v;
}

void*
realloc(void* ap, uint32_t size)
{
	void *v;
	Header *p;
	uint32_t osize;
	uint nunits, ounits;

	/*
	 * Easy stuff:
	 * free and return nil if size is 0
	 * (implementation-defined behaviour);
	 * behave like malloc if ap is nil;
	 * check for arena corruption;
	 * do nothing if units are the same.
	 */
	if(size == 0){
		MLOCK;
		qfreeinternal(ap);
		MUNLOCK;

		return nil;
	}
	if(ap == nil)
		return qmalloc(size);

	p = (Header*)ap - 1;
	if((ounits = p->s.size) == 0 || p->s.next != &checkval)
		panic("realloc: corrupt allocation arena\n");

	if((nunits = NUNITS(size)) == ounits)
		return ap;

	/*
	 * Slightly harder:
	 * if this allocation abuts the tail, try to just
	 * adjust the tailptr.
	 */
	MLOCK;
	if(tailptr != nil && p+ounits == tailptr){
		if(ounits > nunits){
			p->s.size = nunits;
			tailsize += ounits-nunits;
			MUNLOCK;
			return ap;
		}
		if(tailsize >= nunits-ounits){
			p->s.size = nunits;
			tailsize -= nunits-ounits;
			MUNLOCK;
			return ap;
		}
	}
	MUNLOCK;

	/*
	 * Worth doing if it's a small reduction?
	 * Do it anyway if <= NQUICK?
	if((ounits-nunits) < 2)
		return ap;
	 */

	/*
	 * Too hard (or can't be bothered):
	 * allocate, copy and free.
	 * What does the standard say for failure here?
	 */
	if((v = qmalloc(size)) != nil){
		osize = (ounits-1)*sizeof(Header);
		if(size < osize)
			osize = size;
		memmove(v, ap, osize);
		MLOCK;
		qfreeinternal(ap);
		MUNLOCK;
	}

	return v;
}

void
setmalloctag(void* v, uint32_t i)
{
}

void
mallocinit(void)
{
	if(tailptr != nil)
		return;

	tailbase = UINT2PTR(sys->vmunused);
	tailptr = tailbase;
	tailnunits = NUNITS(sys->vmend - sys->vmunused);
	print("base %#p ptr %#p nunits %ud\n", tailbase, tailptr, tailnunits);
}

static int
morecore(uint nunits)
{
	/*
	 * First (simple) cut.
	 * Pump it up when you don't really need it.
	 * Pump it up until you can feel it.
	 */
	if(nunits < NUNITS(128*KiB))
		nunits = NUNITS(128*KiB);
	if(nunits > tailnunits)
		nunits = tailnunits;
	tailnunits -= nunits;

	return nunits;
}
