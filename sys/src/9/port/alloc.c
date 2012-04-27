#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	<pool.h>

static void poolprint(Pool*, char*, ...);
static void ppanic(Pool*, char*, ...);
static void plock(Pool*);
static void punlock(Pool*);

typedef struct Private	Private;
struct Private {
	Lock		lk;
	char		msg[256];	/* a rock for messages to be printed at unlock */
};

static Private pmainpriv;
static Pool pmainmem = {
	.name=	"Main",
	.maxsize=	4*1024*1024,
	.minarena=	128*1024,
	.quantum=	32,
	.alloc=	xalloc,
	.merge=	xmerge,
	.flags=	POOL_TOLERANCE,

	.lock=	plock,
	.unlock=	punlock,
	.print=	poolprint,
	.panic=	ppanic,

	.private=	&pmainpriv,
};

static Private pimagpriv;
static Pool pimagmem = {
	.name=	"Image",
	.maxsize=	16*1024*1024,
	.minarena=	2*1024*1024,
	.quantum=	32,
	.alloc=	xalloc,
	.merge=	xmerge,
	.flags=	0,

	.lock=	plock,
	.unlock=	punlock,
	.print=	poolprint,
	.panic=	ppanic,

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
	vseprint(pv->msg+strlen(pv->msg), pv->msg+sizeof pv->msg, fmt, v);
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
	vseprint(pv->msg+strlen(pv->msg), pv->msg+sizeof pv->msg, fmt, v);
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
	pv->lk.pc = getcallerpc(&p);
	pv->msg[0] = 0;
}

static void
punlock(Pool *p)
{
	Private *pv;
	char msg[sizeof pv->msg];

	pv = p->private;
	if(pv->msg[0] == 0){
		iunlock(&pv->lk);
		return;
	}

	memmove(msg, pv->msg, sizeof msg);
	iunlock(&pv->lk);
	iprint("%.*s", sizeof pv->msg, msg);
}

void
poolsummary(Pool *p)
{
	print("%s max %lud cur %lud free %lud alloc %lud\n", p->name,
		p->maxsize, p->cursize, p->curfree, p->curalloc);
}

void
mallocsummary(void)
{
	poolsummary(mainmem);
	poolsummary(imagmem);
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
smalloc(ulong size)
{
	void *v;

	for(;;) {
		v = poolalloc(mainmem, size+Npadlong*sizeof(ulong));
		if(v != nil)
			break;
		tsleep(&up->sleep, return0, 0, 100);
	}
	if(Npadlong){
		v = (ulong*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
	}
	memset(v, 0, size);
	return v;
}

void*
malloc(ulong size)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(ulong));
	if(v == nil)
		return nil;
	if(Npadlong){
		v = (ulong*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	memset(v, 0, size);
	return v;
}

void*
mallocz(ulong size, int clr)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(ulong));
	if(Npadlong && v != nil){
		v = (ulong*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void*
mallocalign(ulong size, ulong align, long offset, ulong span)
{
	void *v;

	v = poolallocalign(mainmem, size+Npadlong*sizeof(ulong), align, offset-Npadlong*sizeof(ulong), span);
	if(Npadlong && v != nil){
		v = (ulong*)v+Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	if(v)
		memset(v, 0, size);
	return v;
}

void
free(void *v)
{
	if(v != nil)
		poolfree(mainmem, (ulong*)v-Npadlong);
}

void*
realloc(void *v, ulong size)
{
	void *nv;

	if(v != nil)
		v = (ulong*)v-Npadlong;
	if(Npadlong !=0 && size != 0)
		size += Npadlong*sizeof(ulong);

	if(nv = poolrealloc(mainmem, v, size)){
		nv = (ulong*)nv+Npadlong;
		setrealloctag(nv, getcallerpc(&v));
		if(v == nil)
			setmalloctag(nv, getcallerpc(&v));
	}		
	return nv;
}

ulong
msize(void *v)
{
	return poolmsize(mainmem, (ulong*)v-Npadlong)-Npadlong*sizeof(ulong);
}

void*
calloc(ulong n, ulong szelem)
{
	void *v;
	if(v = mallocz(n*szelem, 1))
		setmalloctag(v, getcallerpc(&n));
	return v;
}

void
setmalloctag(void *v, ulong pc)
{
	ulong *u;
	USED(v, pc);
	if(Npadlong <= MallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong+MallocOffset] = pc;
}

void
setrealloctag(void *v, ulong pc)
{
	ulong *u;
	USED(v, pc);
	if(Npadlong <= ReallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong+ReallocOffset] = pc;
}

ulong
getmalloctag(void *v)
{
	USED(v);
	if(Npadlong <= MallocOffset)
		return ~0;
	return ((ulong*)v)[-Npadlong+MallocOffset];
}

ulong
getrealloctag(void *v)
{
	USED(v);
	if(Npadlong <= ReallocOffset)
		return ((ulong*)v)[-Npadlong+ReallocOffset];
	return ~0;
}
