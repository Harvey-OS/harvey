/*
 * allocate uncached memory
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include <pool.h>

typedef struct Private Private;
struct Private {
	Lock;
	char	msg[256];
	char*	cur;
};

static Private ucprivate;

static void
ucpoolpanic(Pool* p, char* fmt, ...)
{
	va_list v;
	Private *pv;
	char msg[sizeof pv->msg];

	pv = p->private;
	va_start(v, fmt);
	vseprint(pv->cur, &pv->msg[sizeof pv->msg], fmt, v);
	va_end(v);
	memmove(msg, pv->msg, sizeof msg);
	iunlock(pv);
	panic("%s", msg);
}

static void
ucpoolprint(Pool* p, char* fmt, ...)
{
	va_list v;
	Private *pv;

	pv = p->private;
	va_start(v, fmt);
	pv->cur = vseprint(pv->cur, &pv->msg[sizeof pv->msg], fmt, v);
	va_end(v);
}

static void
ucpoolunlock(Pool* p)
{
	Private *pv;
	char msg[sizeof pv->msg];

	pv = p->private;
	if(pv->cur == pv->msg){
		iunlock(pv);
		return;
	}

	memmove(msg, pv->msg, sizeof msg);
	pv->cur = pv->msg;
	iunlock(pv);

	iprint("%.*s", sizeof pv->msg, msg);
}

static void
ucpoollock(Pool* p)
{
	Private *pv;

	pv = p->private;
	ilock(pv);
	pv->pc = getcallerpc(&p);
	pv->cur = pv->msg;
}

static void*
ucarena(usize size)
{
	void *uv, *v;

	assert(size == 1*MiB);

	mainmem->maxsize += 1*MiB;
	if((v = mallocalign(1*MiB, 1*MiB, 0, 0)) == nil ||
	    (uv = mmuuncache(v, 1*MiB)) == nil){
		free(v);
		mainmem->maxsize -= 1*MiB;
		return nil;
	}
	return uv;
}

static Pool ucpool = {
	.name		= "Uncached",
	.maxsize	= 4*MiB,
	.minarena	= 1*MiB-32,
	.quantum	= 32,
	.alloc		= ucarena,
	.merge		= nil,
	.flags		= /*POOL_TOLERANCE|POOL_ANTAGONISM|POOL_PARANOIA|*/0,

	.lock		= ucpoollock,
	.unlock		= ucpoolunlock,
	.print		= ucpoolprint,
	.panic		= ucpoolpanic,

	.private	= &ucprivate,
};

void
ucfree(void* v)
{
	if(v == nil)
		return;
	poolfree(&ucpool, v);
}

void*
ucalloc(usize size)
{
	assert(size < ucpool.minarena-128);

	return poolallocalign(&ucpool, size, 32, 0, 0);
}

void*
ucallocalign(usize size, int align, int span)
{
	assert(size < ucpool.minarena-128);

	return poolallocalign(&ucpool, size, align, 0, span);
}
