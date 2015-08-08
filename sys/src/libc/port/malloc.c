/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <pool.h>
#include <tos.h>

static void* sbrkalloc(uint32_t);
static int sbrkmerge(void*, void*);
static void plock(Pool*);
static void punlock(Pool*);
static void pprint(Pool*, char*, ...);
static void ppanic(Pool*, char*, ...);

typedef struct Private Private;
struct Private {
	Lock lk;
	int pid;
	int printfd; /* gets debugging output if set */
};

Private sbrkmempriv;

static Pool sbrkmem = {
    .name = "sbrkmem",
    .maxsize = (3840UL - 1) * 1024 * 1024, /* up to ~0xf0000000 */
    .minarena = 4 * 1024,
    .quantum = 32,
    .alloc = sbrkalloc,
    .merge = sbrkmerge,
    .flags = 0,

    .lock = plock,
    .unlock = punlock,
    .print = pprint,
    .panic = ppanic,
    .private = &sbrkmempriv,
};
Pool* mainmem = &sbrkmem;
Pool* imagmem = &sbrkmem;

/*
 * we do minimal bookkeeping so we can tell pool
 * whether two blocks are adjacent and thus mergeable.
 */
static void*
sbrkalloc(uint32_t n)
{
	uint32_t* x;

	n += 2 * sizeof(uint32_t); /* two longs for us */
	x = sbrk(n);
	if(x == (void*)-1)
		return nil;
	x[0] = (n + 7) & ~7; /* sbrk rounds size up to mult. of 8 */
	x[1] = 0xDeadBeef;
	return x + 2;
}

static int
sbrkmerge(void* x, void* y)
{
	uint32_t* lx, *ly;

	lx = x;
	if(lx[-1] != 0xDeadBeef)
		abort();

	if((uint8_t*)lx + lx[-2] == (uint8_t*)y) {
		ly = y;
		lx[-2] += ly[-2];
		return 1;
	}
	return 0;
}

static void
plock(Pool* p)
{
	Private* pv;
	pv = p->private;
	lock(&pv->lk);
	if(pv->pid != 0)
		abort();
	pv->pid = _tos->pid;
}

static void
punlock(Pool* p)
{
	Private* pv;
	pv = p->private;
	if(pv->pid != _tos->pid)
		abort();
	pv->pid = 0;
	unlock(&pv->lk);
}

static int
checkenv(void)
{
	int n, fd;
	char buf[20];
	fd = open("/env/MALLOCFD", OREAD);
	if(fd < 0)
		return -1;
	if((n = read(fd, buf, sizeof buf)) < 0) {
		close(fd);
		return -1;
	}
	if(n >= sizeof buf)
		n = sizeof(buf) - 1;
	buf[n] = 0;
	n = atoi(buf);
	if(n == 0)
		n = -1;
	return n;
}

static void
pprint(Pool* p, char* fmt, ...)
{
	va_list v;
	Private* pv;

	pv = p->private;
	if(pv->printfd == 0)
		pv->printfd = checkenv();

	if(pv->printfd <= 0)
		pv->printfd = 2;

	va_start(v, fmt);
	vfprint(pv->printfd, fmt, v);
	va_end(v);
}

static char panicbuf[256];
static void
ppanic(Pool* p, char* fmt, ...)
{
	va_list v;
	int n;
	char* msg;
	Private* pv;

	pv = p->private;
	assert(canlock(&pv->lk) == 0);

	if(pv->printfd == 0)
		pv->printfd = checkenv();
	if(pv->printfd <= 0)
		pv->printfd = 2;

	msg = panicbuf;
	va_start(v, fmt);
	n = vseprint(msg, msg + sizeof panicbuf, fmt, v) - msg;
	write(2, "panic: ", 7);
	write(2, msg, n);
	write(2, "\n", 1);
	if(pv->printfd != 2) {
		write(pv->printfd, "panic: ", 7);
		write(pv->printfd, msg, n);
		write(pv->printfd, "\n", 1);
	}
	va_end(v);
	//	unlock(&pv->lk);
	abort();
}

/* - everything from here down should be the same in libc, libdebugmalloc, and
 * the kernel - */
/* - except the code for malloc(), which alternately doesn't clear or does. - */

/*
 * Npadlong is the number of 32-bit longs to leave at the beginning of
 * each allocated buffer for our own bookkeeping.  We return to the callers
 * a pointer that points immediately after our bookkeeping area.  Incoming
 *pointers
 * must be decremented by that much, and outgoing pointers incremented.
 * The malloc tag is stored at MallocOffset from the beginning of the block,
 * and the realloc tag at ReallocOffset.  The offsets are from the true
 *beginning
 * of the block, not the beginning the caller sees.
 *
 * The extra if(Npadlong != 0) in various places is a hint for the compiler to
 * compile out function calls that would otherwise be no-ops.
 */

/* tracing */
enum { Npadlong = 2, MallocOffset = 0, ReallocOffset = 1 };

void*
malloc(size_t size)
{
	void* v;

	v = poolalloc(mainmem, size + Npadlong * sizeof(uintptr_t));
	if(Npadlong && v != nil) {
		v = (uintptr_t*)v + Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void*
mallocz(uint32_t size, int clr)
{
	void* v;

	v = poolalloc(mainmem, size + Npadlong * sizeof(uintptr_t));
	if(Npadlong && v != nil) {
		v = (uintptr_t*)v + Npadlong;
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
	void* v;

	v = poolallocalign(mainmem, size + Npadlong * sizeof(uintptr_t), align,
	                   offset - Npadlong * sizeof(uintptr_t), span);
	if(Npadlong && v != nil) {
		v = (uintptr_t*)v + Npadlong;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void
free(void* v)
{
	if(v != nil)
		poolfree(mainmem, (uintptr_t*)v - Npadlong);
}

void*
realloc(void* v, size_t size)
{
	void* nv;

	if(size == 0) {
		free(v);
		return nil;
	}

	if(v)
		v = (uintptr_t*)v - Npadlong;
	size += Npadlong * sizeof(uintptr_t);

	if((nv = poolrealloc(mainmem, v, size))) {
		nv = (uintptr_t*)nv + Npadlong;
		setrealloctag(nv, getcallerpc(&v));
		if(v == nil)
			setmalloctag(nv, getcallerpc(&v));
	}
	return nv;
}

uint32_t
msize(void* v)
{
	return poolmsize(mainmem, (uintptr_t*)v - Npadlong) -
	       Npadlong * sizeof(uintptr_t);
}

void*
calloc(uint32_t n, size_t szelem)
{
	void* v;
	if((v = mallocz(n * szelem, 1)))
		setmalloctag(v, getcallerpc(&n));
	return v;
}

void
setmalloctag(void* v, uintptr_t pc)
{
	uintptr_t* u;
	if(Npadlong <= MallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong + MallocOffset] = pc;
}

void
setrealloctag(void* v, uintptr_t pc)
{
	uintptr_t* u;
	// USED(v, pc);
	if(Npadlong <= ReallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong + ReallocOffset] = pc;
}

uintptr_t
getmalloctag(void* v)
{
	USED(v);
	if(Npadlong <= MallocOffset)
		return ~0;
	return ((uintptr_t*)v)[-Npadlong + MallocOffset];
}

uintptr_t
getrealloctag(void* v)
{
	USED(v);
	if(Npadlong <= ReallocOffset)
		return ((uintptr_t*)v)[-Npadlong + ReallocOffset];
	return ~0;
}

void*
malloctopoolblock(void* v)
{
	if(v == nil)
		return nil;

	return &((uintptr_t*)v)[-Npadlong];
}
