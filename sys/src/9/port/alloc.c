/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	<pool.h>

static void*	sbrkalloc(uint32_t);
static int		sbrkmerge(void*, void*);
static void poolprint(Pool*, char*, ...);
static void ppanic(Pool*, char*, ...);
static void plock(Pool*);
static void punlock(Pool*);

extern	void	abort(void);
extern	void*	sbrk(uint32_t);

typedef struct Private	Private;
struct Private {
	Lock		lk;
	char*		end;
	char		msg[256];	/* a rock for messages to be printed at unlock */
};

static Private pmainpriv;
static Pool pmainmem = {
	.name=		"Main",
	.maxsize=	4*1024*1024,
	.minarena=	128*1024,
	.quantum=	32,
	.alloc=		sbrkalloc,
	.merge=		sbrkmerge,
	.flags=		/*POOL_TOLERANCE|POOL_ANTAGONISM|POOL_PARANOIA|*/0,

	.lock=		plock,
	.unlock=	punlock,
	.print=		poolprint,
	.panic=		ppanic,

	.private=	&pmainpriv,
};

static Private pimagpriv;
static Pool pimagmem = {
	.name=		"Image",
	.maxsize=	16*1024*1024,
	.minarena=	2*1024*1024,
	.quantum=	32,
	.alloc=		sbrkalloc,
	.merge=		sbrkmerge,
	.flags=		0,

	.lock=		plock,
	.unlock=	punlock,
	.print=		poolprint,
	.panic=		ppanic,

	.private=	&pimagpriv,
};

Pool*	mainmem = &pmainmem;
Pool*	imagmem = &pimagmem;

/*
 * we do minimal bookkeeping so we can tell pool
 * whether two blocks are adjacent and thus mergeable.
 */
static void*
sbrkalloc(uint32_t n)
{
	uint32_t *x;

	n += 2*sizeof(uint32_t);	/* two longs for us */
	x = sbrk(n);
	if(x == (void*)-1)
		return nil;
	x[0] = (n+7)&~7;	/* sbrk rounds size up to mult. of 8 */
	x[1] = 0xDeadBeef;
	return x+2;
}

static int
sbrkmerge(void *x, void *y)
{
	uint32_t *lx, *ly;

	lx = x;
	if(lx[-1] != 0xDeadBeef)
		abort();

	if((uint8_t*)lx+lx[-2] == (uint8_t*)y) {
		ly = y;
		lx[-2] += ly[-2];
		return 1;
	}
	return 0;
}

/*
 * because we can't print while we're holding the locks,
 * we have the save the message and print it once we let go.
 */
static void
poolprint(Pool *p, char *fmt, ...)
{
	va_list v;
	Private *pv;

	pv = p->private;
	va_start(v, fmt);
	pv->end = vseprint(pv->end, &pv->msg[sizeof pv->msg], fmt, v);
	va_end(v);
}

static void
ppanic(Pool *p, char *fmt, ...)
{
	va_list v;
	Private *pv;
	char msg[sizeof pv->msg];

	pv = p->private;
	va_start(v, fmt);
	vseprint(pv->end, &pv->msg[sizeof pv->msg], fmt, v);
	va_end(v);
	memmove(msg, pv->msg, sizeof msg);
	iunlock(&pv->lk);
	panic("%s", msg);
}

static void
plock(Pool *p)
{
	Private *pv;

	pv = p->private;
	ilock(&pv->lk);
	pv->lk._pc = getcallerpc(&p);
	pv->end = pv->msg;
}

static void
punlock(Pool *p)
{
	Private *pv;
	char msg[sizeof pv->msg];

	pv = p->private;
	if(pv->end == pv->msg){
		iunlock(&pv->lk);
		return;
	}

	memmove(msg, pv->msg, sizeof msg);
	pv->end = pv->msg;
	iunlock(&pv->lk);
	iprint("%.*s", sizeof pv->msg, msg);
}
