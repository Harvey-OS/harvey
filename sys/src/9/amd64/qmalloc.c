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
#include <pool.h>

static void* sbrkalloc(uint32_t);
static int   sbrkmerge(void*, void*);
static void  klock(Pool*);
static void  kunlock(Pool*);
static void  kprint(Pool* p, char* c, ...) {}
static void  kpanic(Pool* p, char* c, ...) {}

typedef struct Private Private;
struct Private {
        Lock            lk;
        int             printfd;        /* gets debugging output if set */
};

Private sbrkmempriv;

static Pool sbrkmem = {
        .name=          "sbrkmem",
        .maxsize=       (3840UL-1)*1024*1024,   /* up to ~0xf0000000 */
        .minarena=      4*1024,
        .quantum=       64,
        .alloc=         sbrkalloc,
        .merge=         sbrkmerge,
        .flags=         0,

        .lock=          klock,
        .unlock=        kunlock,
        .print=         kprint,
        .panic=         kpanic,
        .private=       &sbrkmempriv,
};

Pool *kernelmem = &sbrkmem;

enum {
        Npadlong      = 2,
        MallocOffset  = 0,
        ReallocOffset = 1
};

#define	NUNITS(n)	(HOWMANY(n, Npadlong) + 1)

static	unsigned tailsize;
static	unsigned tailnunits;
static	void	*tailbase;
static	void	*tailptr;
static	int	morecore(unsigned);

enum
{
	QSmalign = 0,
	QSmalloc,
	QSfree,
	QSmallocz,
	QSmallocalign,
	QSsmalloc,
	QSremalloc,
	QSmax
};

static	int	qstats[QSmax];
static	char*	qstatstr[QSmax] = {
[QSmalign] = "malign",
[QSmalloc] = "malloc",
[QSfree] = "free",
[QSmallocz] = "mallocz",
[QSmallocalign] = "mallocalign",
[QSsmalloc] = "smalloc",
[QSremalloc] = "remalloc",
};

#define ISPOWEROF2(x)	(/*((x) != 0) && */!((x) & ((x)-1)))
#define ALIGNHDR(h, a)	(Header*)((((uintptr)(h))+((a)-1)) & ~((a)-1))

static void*
qmallocalign(usize size, uintptr_t align, int32_t offset, usize span)
{
        void *v;

	qstats[QSmalign]++;
        v = poolallocalign(kernelmem, size+Npadlong*sizeof(uintptr_t), align,
                           offset-Npadlong*sizeof(uintptr_t), span);
        if(Npadlong && v != nil){
                v = (uintptr_t*)v+Npadlong;
                setmalloctag(v, getcallerpc());
        }
        return v;
}

static void*
qmalloc(usize size)
{
        void *v;

	if(size == 0)
		return nil;

	qstats[QSmalloc]++;
        v = poolalloc(kernelmem, size+Npadlong*sizeof(uintptr_t));
        if(Npadlong && v != nil) {
                v = (uintptr_t*)v+Npadlong;
                setmalloctag(v, getcallerpc());
        }
        return v;
}

uint32_t
msize(void* v)
{
	return poolmsize(kernelmem, (uintptr_t*)v-Npadlong)-Npadlong*sizeof(uintptr_t);
}

static void
mallocreadfmt(char* s, char* e)
{
	char *p;
	int i;

	p = seprint(s, e,
		"%llud memory\n"
		"%d pagesize\n"
		"%llud kernel\n",
		(uint64_t)conf.npage*PGSZ,
		PGSZ,
		(uint64_t)conf.npage-conf.upages);

	p = seprint(p, e, "total allocated %lud, %ud remaining\n",
		(tailptr-tailbase)*Npadlong, tailnunits*Npadlong);

	for(i = 0; i < nelem(qstats); i++){
		if(qstats[i] == 0)
			continue;
		p = seprint(p, e, "%s %ud\n", qstatstr[i], qstats[i]);
	}
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
	int i;

	print("total allocated %lud, %ud remaining\n",
		(tailptr-tailbase)*Npadlong, tailnunits*Npadlong);

	for(i = 0; i < nelem(qstats); i++){
		if(qstats[i] == 0)
			continue;
		print("%s %ud\n", qstatstr[i], qstats[i]);
	}
}

void
free(void* v)
{
	qstats[QSfree]++;
        if(v != nil)
                poolfree(kernelmem, (uintptr_t*)v-Npadlong);
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

	qstats[QSmallocz]++;
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
	qstats[QSmallocalign]++;
	if((v = qmallocalign(nbytes, align, offset, span)) != nil)
		memset(v, 0, nbytes);

	return v;
}

void*
smalloc(uint32_t size)
{
	Proc *up = externup();
	void *v;

	qstats[QSsmalloc]++;
	while((v = malloc(size)) == nil)
		tsleep(&up->sleep, return0, 0, 100);
	memset(v, 0, size);

	return v;
}

void*
realloc(void* v, uint32_t size)
{
        void *nv;

	qstats[QSremalloc]++;
        if(size == 0){
                free(v);
                return nil;
        }

        if(v)
                v = (uintptr_t*)v-Npadlong;
        size += Npadlong*sizeof(uintptr_t);

        if((nv = poolrealloc(kernelmem, v, size))){
                nv = (uintptr_t*)nv+Npadlong;
                if(v == nil)
                        setmalloctag(nv, getcallerpc());
        }
        return nv;
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

/*
 *  * we do minimal bookkeeping so we can tell pool
 *   * whether two blocks are adjacent and thus mergeable.
 *    */
static void*
sbrkalloc(uint32_t n)
{
        uint32_t *x;

        n += 2*sizeof(uint32_t);        /* two longs for us */
	n = (n+7)&~7;
	if (tailsize < n){
		int nunits = NUNITS(n);
		if(morecore(nunits) == 0){
			return nil;
		}
	}
        x = tailptr;
	tailsize -= n;
	tailptr += n;
        x[0] = n;
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

static void
klock(Pool *p)
{
        Private *pv;
        pv = p->private;
        ilock(&pv->lk);
}

static void
kunlock(Pool *p)
{
        Private *pv;
        pv = p->private;
        iunlock(&pv->lk);
}

