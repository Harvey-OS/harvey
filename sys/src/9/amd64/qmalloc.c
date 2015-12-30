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
static	Header	*rover;
static	unsigned tailnunits;
static	Header	*tailbase;
static	Header	*tailptr;

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

#define	MLOCK		ilock(&mainlock)
#define	MUNLOCK		iunlock(&mainlock)
#define QLOCK(l)	ilock(l)
#define QUNLOCK(l)	iunlock(l)

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

