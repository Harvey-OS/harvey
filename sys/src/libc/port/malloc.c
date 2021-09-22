#include <u.h>
#include <libc.h>
#include <pool.h>
#include <tos.h>

static void*		sbrkalloc(uintptr);
static int		sbrkmerge(void*, void*);
static void		plock(Pool*);
static void		punlock(Pool*);
static void		pprint(Pool*, char*, ...);
static void		ppanic(Pool*, char*, ...);

typedef struct Private Private;
struct Private {
	Lock		lk;
	int		pid;
	int		printfd;	/* gets debugging output if set */
};

Private sbrkmempriv;

static Pool sbrkmem = {
	.name=		"sbrkmem",
	/* see allocinit() for 64-bit overrides */
	.maxsize=	(3840UL-1)*1024ul*1024ul,	/* up to ~0xf0000000 */
	.quantum=	8 * sizeof(void *),
	.minarena=	4*1024,
	.alloc=		sbrkalloc,
	.merge=		sbrkmerge,
	.flags=		0,

	.lock=		plock,
	.unlock=	punlock,
	.print=		pprint,
	.panic=		ppanic,
	.private=	&sbrkmempriv,
};
Pool *mainmem = &sbrkmem;
Pool *imagmem = &sbrkmem;

/*
 * we do minimal bookkeeping so we can tell pool
 * whether two blocks are adjacent and thus mergeable.
 */
static void*
sbrkalloc(uintptr n)
{
	uintptr *x;

	n += 2*sizeof(uintptr);	/* two ptrs for us */
	/* sbrk takes a ulong, so use brk */
	x = sbrk(0);
	if (brk((char *)x + n) < 0)
		return nil;
	x[0] = (n+7)&~7;	/* sbrk rounds size up to mult. of 8 */
	x[1] = 0xDeadBeef;
	return x+2;
}

static int
sbrkmerge(void *x, void *y)
{
	uintptr *lx, *ly;

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

static void
plock(Pool *p)
{
	Private *pv;
	pv = p->private;
	lock(&pv->lk);
	if(pv->pid != 0)
		abort();
	pv->pid = _tos->pid;
}

static void
punlock(Pool *p)
{
	Private *pv;
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
		n = sizeof(buf)-1;
	buf[n] = 0;
	n = atoi(buf);
	if(n == 0)
		n = -1;
	return n;
}

static void
pprint(Pool *p, char *fmt, ...)
{
	va_list v;
	Private *pv;

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
ppanic(Pool *p, char *fmt, ...) 
{
	va_list v;
	int n;
	char *msg;
	Private *pv;

	pv = p->private;
	assert(canlock(&pv->lk)==0);

	if(pv->printfd == 0)
		pv->printfd = checkenv();
	if(pv->printfd <= 0)
		pv->printfd = 2;

	msg = panicbuf;
	va_start(v, fmt);
	n = vseprint(msg, msg+sizeof panicbuf, fmt, v) - msg;
	write(2, "panic: ", 7);
	write(2, msg, n);
	write(2, "\n", 1);
	if(pv->printfd != 2){
		write(pv->printfd, "panic: ", 7);
		write(pv->printfd, msg, n);
		write(pv->printfd, "\n", 1);
	}
	va_end(v);
//	unlock(&pv->lk);
	abort();
}

/* - everything from here down should be the same in libc, libdebugmalloc, and the kernel - */
/* - except the code for malloc(), which alternately doesn't clear or does. - */

/*
 * Npadptrs is the number of pointers to leave at the beginning of 
 * each allocated buffer for our own bookkeeping.  We return to the callers
 * a pointer that points immediately after our bookkeeping area.  Incoming pointers
 * must be decremented by that much, and outgoing pointers incremented.
 * The malloc tag is stored at MallocOffset from the beginning of the block,
 * and the realloc tag at ReallocOffset.  The offsets are from the true beginning
 * of the block, not the beginning the caller sees.
 *
 * The extra if(Npadptrs != 0) in various places is a hint for the compiler to
 * compile out function calls that would otherwise be no-ops.
 */

/*	non tracing
 *
enum {
	Npadptrs	= 0,
	MallocOffset	= 0,
	ReallocOffset	= 0,
};
 *
 */

/* tracing */
enum {
	Npadptrs	= 2,
	MallocOffset	= 0,
	ReallocOffset	= 1
};

enum {
	Npadbytes = Npadptrs * sizeof(void *),
};

static int firstalloc = 1;

static void
allocinit(void)
{
	if (!firstalloc)
		return;
	firstalloc = 0;
	plock(&sbrkmem);
	if (sizeof(void *) == sizeof(vlong)) {
		sbrkmem.maxsize = 128ll << 40;
		/* pool's MINBLOCKSIZE is 64 on 64-bit systems */
		sbrkmem.minblock = 64;
	}
	punlock(&sbrkmem);
	/*
	 * not sure why this helps, but it prevents poolfreel from
	 * dying later after large allocations, at least on 64-bit systems.
	 */
	malloc(8);
}

#define initialalloc()	if (firstalloc) allocinit(); else {}

void*
malloc(uintptr size)
{
	void *v;

	initialalloc();
	v = poolalloc(mainmem, size + Npadbytes);
	if(Npadptrs && v != nil) {
		v = (uintptr *)v + Npadptrs;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void*
mallocz(uintptr size, int clr)
{
	void *v;

	initialalloc();
	v = poolalloc(mainmem, size+Npadbytes);
	if(Npadptrs && v != nil){
		v = (uintptr *)v + Npadptrs;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void*
mallocalign(uintptr size, uintptr align, vlong offset, uintptr span)
{
	void *v;

	initialalloc();
	v = poolallocalign(mainmem, size+Npadbytes, align, offset-Npadbytes, span);
	if(Npadptrs && v != nil){
		v = (uintptr *)v + Npadptrs;
		setmalloctag(v, getcallerpc(&size));
		setrealloctag(v, 0);
	}
	return v;
}

void
free(void *v)
{
	if(v != nil)
		poolfree(mainmem, (uintptr *)v-Npadptrs);
}

void*
realloc(void *v, uintptr size)
{
	void *nv;

	initialalloc();
	if(size == 0){
		free(v);
		return nil;
	}

	if(v)
		v = (uintptr *)v - Npadptrs;
	size += Npadbytes;

	if(nv = poolrealloc(mainmem, v, size)){
		nv = (uintptr *)nv + Npadptrs;
		setrealloctag(nv, getcallerpc(&v));
		if(v == nil)
			setmalloctag(nv, getcallerpc(&v));
	}		
	return nv;
}

uintptr
msize(void *v)
{
	return poolmsize(mainmem, (uintptr*)v - Npadptrs) - Npadbytes;
}

void*
calloc(uintptr n, uintptr szelem)
{
	void *v;

	if(v = mallocz(n*szelem, 1))
		setmalloctag(v, getcallerpc(&n));
	return v;
}

void
setmalloctag(void *v, uintptr pc)
{
	if(Npadptrs <= MallocOffset || v == nil)
		return;
	((uintptr *)v)[-Npadptrs+MallocOffset] = pc;
}

void
setrealloctag(void *v, uintptr pc)
{
	if(Npadptrs <= ReallocOffset || v == nil)
		return;
	((uintptr *)v)[-Npadptrs+ReallocOffset] = pc;
}

uintptr
getmalloctag(void *v)
{
	if(Npadptrs <= MallocOffset)
		return ~0;
	return ((uintptr *)v)[-Npadptrs+MallocOffset];
}

uintptr
getrealloctag(void *v)
{
	if(Npadptrs <= ReallocOffset)
		return ~0;
	return ((uintptr *)v)[-Npadptrs+ReallocOffset];
}

void*
malloctopoolblock(void *v)
{
	if(v == nil)
		return nil;
	return &((uintptr *)v)[-Npadptrs];
}
