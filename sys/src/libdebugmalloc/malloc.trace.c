#include <u.h>
#include <libc.h>
#include <pool.h>

static void*	sbrkalloc(long);
static int		sbrkmerge(void*, void*);

static Pool sbrkmem = {
	.name=		"sbrkmem",
	.maxsize=		2*1024*1024*1024,
	.minarena=	4*1024,
	.quantum=	32,
	.alloc=		sbrkalloc,
	.merge=		sbrkmerge,
	.flags=		POOL_LOGGING|POOL_ANTAGONISM,
};

/* for tracing */
#define PAD 2
#define MALLOC -2
#define REALLOC -1

/* for not tracing */
/*
#define PAD 0
#define MALLOC 0
#define REALLOC 0
*/

void*
malloc(ulong size)
{
	ulong *l;
	void *v;

	v = poolalloc(mainmem, size+PAD*sizeof(ulong));
	if(PAD && v != nil) {
		v = l+PAD;
		tagwithpc(v, getcallerpc(&size));
		tagwithrpc(v, 0);
	}
	return v;
}

void*
mallocz(ulong size, int clr)
{
	void *v;

	v = poolalloc(mainmem, size+PAD*sizeof(ulong));
	if(PAD && v != nil){
		v = (ulong*)v+PAD;
		tagwithpc(v, getcallerpc(&size));
		tagwithrpc(v, 0);
	}
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void
free(void *v)
{
	if(v != nil)
		poolfree(mainmem, (ulong*)v-PAD);
}

void*
realloc(void *v, ulong size)
{
	void *nv;
	long nsize;

	if(size == 0) {
		free((ulong*)v-PAD);
		return nil;
	}
	if(v == nil){
		v = malloc(size+PAD*sizeof(ulong));
		if(PAD && v){
			v = (ulong*)v+PAD;
		tagwithrpc(v, getcallerpc(&v));
		return v;
	}
	nv = (ulong*)v-PAD;
	nsize = size+PAD*sizeof(ulong);
	if(v = poolrealloc(mainmem, nv, nsize))
		return (ulong*)v+PAD;
	return nil;
}

ulong
msize(void *v)
{
	return poolmsize(mainmem, (ulong*)v-PAD)-PAD*sizeof(ulong);
}

void*
calloc(ulong n, ulong szelem)
{
	return mallocz(n*szelem, 1);
}

void
tagwithpc(void *v, ulong pc)
{
	ulong *u;
	if(MALLOC == 0 || v == nil)
		return;
	u = v;
	u[MALLOC] = pc;
}

void
tagwithrpc(void *v, ulong pc)
{
	ulong *u;
	if(REALLOC == 0 || v == nil)
		return;
	u = v;
	u[REALLOC] = pc;
}

/*
 * we do minimal bookkeeping so we can tell arena
 * whether two blocks are adjacent and thus mergeable.
 */
static void*
sbrkalloc(ulong n)
{
	long *x;

	n += 8;	/* two longs for us */
	x = sbrk(n);
	if((int)x == -1)
		x = nil;
	x[0] = (n+7)&~7;	/* sbrk rounds size up to mult. of 8 */
	x[1] = 0xDeadBeef;
	return x+2;
}

static int
sbrkmerge(void *x, void *y)
{
	long *lx, *ly;

	lx = x;
	if(lx[-1] != 0xDeadBeef)
		abort();

	if((uchar*)lx+lx[-2] == (uchar*)y) {
		ly = y;
		lx[-2] += ly[-2];
		return 1;
	}
	return 0;
}
