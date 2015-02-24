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

extern void* xalloc(uint32_t);
extern void xinit(void);
extern int xmerge(void*, void*);

static void poolprint(Pool*, char*, ...);
static void ppanic(Pool*, char*, ...);
static void plock(Pool*);
static void punlock(Pool*);

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
	.alloc=		xalloc,
	.merge=		xmerge,
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
	.alloc=		xalloc,
	.merge=		xmerge,
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

static char*
poolsummary(Pool* p, char* s, char* e)
{
	return seprint(s, e, "%s max %lud cur %lud free %lud alloc %lud\n",
		p->name, p->maxsize, p->cursize, p->curfree, p->curalloc);
}

void
mallocsummary(void)
{
	char buf[256], *p;

	p = poolsummary(mainmem, buf, buf+sizeof(buf));
	poolsummary(imagmem, p, buf+sizeof(buf));

	print(buf);
}

int32_t
mallocreadsummary(Chan* c, void *a, int32_t n, int32_t offset)
{
	char buf[256], *p;

	p = poolsummary(mainmem, buf, buf+sizeof(buf));
	poolsummary(imagmem, p, buf+sizeof(buf));

	return readstr(offset, a, n, buf);
}

void
mallocinit(void)
{
	xinit();
}

/* everything from here down should be the same in libc, libdebugmalloc, and the kernel */
/* - except the code for malloc(), which alternately doesn't clear or does. */
/* - except the code for smalloc(), which lives only in the kernel. */

/*
 * Npadlong is the number of 32-bit longs to leave at the beginning of
 * each allocated buffer for our own bookkeeping.  We return to the callers
 * a pointer that points immediately after our bookkeeping area.  Incoming pointers
 * must be decremented by that much, and outgoing pointers incremented.
 * The malloc tag is stored at MallocOffset from the beginning of the block,
 * and the realloc tag at ReallocOffset.  The offsets are from the true beginning
 * of the block, not the beginning the caller sees.
 *
 * The extra if(Npadlong != 0) in various places is a hint for the compiler to
 * compile out function calls that would otherwise be no-ops.
 */

/*	non tracing
 *
enum {
	Npadlong	= 0,
	MallocOffset = 0,
	ReallocOffset = 0,
};
 *
 */

/* tracing */
enum {
	Npadlong	= 2,
	MallocOffset = 0,
	ReallocOffset = 1
};


void*
smalloc(uint32_t size)
{
	void *v;

	for(;;) {
		v = poolalloc(mainmem, size+Npadlong*sizeof(uint32_t));
		if(v != nil)
			break;
		tsleep(&up->sleep, return0, 0, 100);
	}
	if(Npadlong){
		v = (uint32_t*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
	}
	memset(v, 0, size);
	return v;
}

void*
malloc(uint32_t size)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(uint32_t));
	if(v == nil)
		return nil;
	if(Npadlong){
		v = (uint32_t*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	memset(v, 0, size);
	return v;
}

void*
mallocz(uint32_t size, int clr)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(uint32_t));
	if(Npadlong && v != nil){
		v = (uint32_t*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void*
mallocalign(uint32_t size, uint32_t align, int32_t offset, uint32_t span)
{
	void *v;

	v = poolallocalign(mainmem, size+Npadlong*sizeof(uint32_t), align,
			   offset-Npadlong*sizeof(uint32_t), span);
	if(Npadlong && v != nil){
		v = (uint32_t*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void
free(void *v)
{
	if(v != nil)
		poolfree(mainmem, (uint32_t*)v-Npadlong);
}

void*
realloc(void *v, uint32_t size)
{
	void *nv;

	if(v != nil)
		v = (uint32_t*)v-Npadlong;
	if(Npadlong !=0 && size != 0)
		size += Npadlong*sizeof(uint32_t);

	if(nv = poolrealloc(mainmem, v, size)){
		nv = (uint32_t*)nv+Npadlong;
		setrealloctag(nv, getcallerpc(&v));
		if(v == nil)
			setmalloctag(nv, getcallerpc(&v));
	}
	return nv;
}

uint32_t
msize(void *v)
{
	return poolmsize(mainmem, (uint32_t*)v-Npadlong)-Npadlong*sizeof(uint32_t);
}

void
setmalloctag(void *v, uint32_t pc)
{
	uint32_t *u;
	USED(v); USED(pc);
	if(Npadlong <= MallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong+MallocOffset] = pc;
}

void
setrealloctag(void *v, uint32_t pc)
{
	uint32_t *u;
	USED(v); USED(pc);
	if(Npadlong <= ReallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong+ReallocOffset] = pc;
}

uint32_t
getmalloctag(void *v)
{
	USED(v);
	if(Npadlong <= MallocOffset)
		return ~0;
	return ((uint32_t*)v)[-Npadlong+MallocOffset];
}

uint32_t
getrealloctag(void *v)
{
	USED(v);
	if(Npadlong <= ReallocOffset)
		return ((uint32_t*)v)[-Npadlong+ReallocOffset];
	return ~0;
}
