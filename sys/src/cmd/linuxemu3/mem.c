#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Range Range;
typedef struct Area Area;
typedef struct Filemap Filemap;
typedef struct Futex Futex;
typedef struct Seg Seg;
typedef struct Space Space;

/* keep in order, lowest base address first */
enum {
	SEGDATA,
	SEGPRIVATE,
	SEGSHARED,
	SEGSTACK,
	SEGMAX,
};

static char *segname[SEGMAX] = { "data", "private", "shared", "stack" };

struct Range
{
	ulong	base;
	ulong	top;
};

struct Filemap
{
	Range	addr;

	Filemap	*next;

	char		*path;
	ulong	offset;
	int		mode;
	Ufile		*file;

	Ref;
};

struct Futex
{
	ulong	*addr;

	Futex	*next;
	Futex	**link;

	Ref;
	Uwaitq;
};

struct Area
{
	Range	addr;

	Area 	*next;		/* next higher area */
	Area 	*prev;		/* previous lower area */
	Seg		*seg;			/* segment we belong to */

	int		prot;

	Filemap  	*filemap;
	Futex	*futex;
};

struct Seg
{
	Ref;
	QLock;

	Range	addr;
	ulong	limit;			/* maximum address this segment can grow */

	Area 	*areas;		/* orderd by address */

	int		type;			/* SEGDATA, SEGSHARED, SEGPRIVATE, SEGSTACK */

	Area		*freearea;
	Filemap	*freefilemap;
	Futex	*freefutex;
};

struct Space
{
	Ref;
	QLock;

	ulong	brk;
	Seg		*seg[SEGMAX];
};


void*
kmalloc(int size)
{
	void *p;

	p = malloc(size);
	if(p == nil)
		panic("kmalloc: out of memory");
	setmalloctag(p, getcallerpc(&size));
	return p;
}
void*
krealloc(void *ptr, int size)
{
	void *p;

	p = realloc(ptr, size);
	if(size > 0){
		if(p == nil)
			panic("krealloc: out of memory");
		setmalloctag(p, getcallerpc(&ptr));
	}
	return p;
}

void*
kmallocz(int size, int zero)
{
	void *p;

	p = mallocz(size, zero);
	if(p == nil)
		panic("kmallocz: out of memory");
	setmalloctag(p, getcallerpc(&size));
	return p;
}

char*
kstrdup(char *s)
{
	char *p;
	int n;

	n = strlen(s);
	p = kmalloc(n+1);
	memmove(p, s, n);
	p[n] = 0;
	setmalloctag(p, getcallerpc(&s));
	return p;
}

char*
ksmprint(char *fmt, ...)
{
	va_list args;
	char *p;
	int n;

	n = 4096;
	p = kmalloc(n);
	va_start(args, fmt);
	n = vsnprint(p, n, fmt, args);
	va_end(args);
	if((p = realloc(p, n+1)) == nil)
		panic("ksmprint: out of memory");
	setmalloctag(p, getcallerpc(&fmt));
	return p;
}

ulong
pagealign(ulong addr)
{
	ulong m;

	m = PAGESIZE-1;
	return (addr + m) & ~m;
}

static void
syncarea(Area *a, Range r)
{
	if(a->filemap == nil)
		return;
	if(a->filemap->file == nil)
		return;
	if((a->prot & PROT_WRITE) == 0)
		return;

	if(r.base < a->addr.base)
		r.base = a->addr.base;
	if(r.top > a->addr.top)
		r.top = a->addr.top;
	if(r.base < a->filemap->addr.base)
		r.base = a->filemap->addr.base;
	if(r.top > a->filemap->addr.top)
		r.top = a->filemap->addr.top;
	pwritefile(a->filemap->file, (void*)r.base, r.top - r.base,
		(r.base - a->filemap->addr.base) + a->filemap->offset);
}

static void
linkarea(Seg *seg, Area *a)
{
	Area *p;

	a->next = nil;
	a->prev = nil;
	a->seg = seg;

	for(p = seg->areas; p && p->next; p=p->next)
		if(p->addr.base > a->addr.base)
			break;
	if(p != nil){
		if(p->addr.base > a->addr.base){
			a->next = p;
			if(a->prev = p->prev)
				a->prev->next = a;
			p->prev = a;
		} else {
			a->prev = p;
			p->next = a;
		}
	}
	if(a->prev == nil)
		seg->areas = a;
}

static Area *
duparea(Area *a)
{
	Area *r;

	if(r = a->seg->freearea){
		a->seg->freearea = r->next;
	} else {
		r = kmalloc(sizeof(Area));
	}
	r->addr = a->addr;
	r->next = nil;
	r->prev = nil;
	r->seg = nil;
	r->prot = a->prot;
	if(r->filemap = a->filemap)
		incref(r->filemap);
	r->futex = nil;
	return r;
}

static void
freearea(Area *a)
{
	Filemap *f;
	Futex *x;
	Seg *seg;

	seg = a->seg;
	if(f = a->filemap){
		syncarea(a, a->addr);
		a->filemap = nil;
		if(!decref(f)){
			free(f->path);
			putfile(f->file);
			f->next = seg->freefilemap;
			seg->freefilemap = f;
		}
	}
	while(x = a->futex){
		if(a->futex = x->next)
			x->next->link = &a->futex;
		x->link = nil;
		x->next = nil;
		wakeq(x, MAXPROC);
	}
	if(a->prev == nil){
		if(seg->areas = a->next)
			a->next->prev = nil;
	} else {
		if(a->prev->next = a->next)
			a->next->prev = a->prev;
	}

	a->next = seg->freearea;
	seg->freearea = a;
}

static Seg *
allocseg(int type, Range addr, ulong limit, int attr, char *class)
{
	Seg *seg;

	if(class){
		trace("allocseg(): segattach %s segment %lux-%lux", segname[type], addr.base, addr.top);
		if(segattach(attr, class, (void*)addr.base, addr.top - addr.base) != (void*)addr.base)
			panic("allocseg: segattach %s segment: %r", segname[type]);
	}

	seg = kmallocz(sizeof(Seg), 1);
	seg->addr = addr;
	seg->limit = limit;
	seg->type = type;
	seg->ref = 1;

	return seg;
}

static Seg *
dupseg(Seg *old, int copy)
{
	Seg *new;
	Area *a, *p, *x;

	if(old == nil)
		return nil;
	if(!copy){
		incref(old);
		return old;
	}
	new = allocseg(old->type, old->addr, old->limit, 0, nil);
	p = nil;
	for(a=old->areas; a; a=a->next){
		x = duparea(a);
		x->seg = new;
		if(x->prev = p){
			p->next = x;
		} else {
			new->areas = x;
		}
		p = x;
	}

	return new;
}

static Space *
getspace(Space *old, int copy)
{	
	Space *new;
	Seg *seg;
	int t;

	if(!copy){
		incref(old);
		return old;
	}

	new = kmallocz(sizeof(Space), 1);
	new->ref = 1;

	qlock(old);
	for(t=0; t<SEGMAX; t++){
		if(seg = old->seg[t]){
			qlock(seg);
			new->seg[t] = dupseg(seg, t != SEGSHARED);
			qunlock(seg);
		}
	}
	new->brk = old->brk;
	qunlock(old);

	return new;
}

static void
putspace(Space *space)
{
	Seg *seg;
	int t;
	Area *a;
	Filemap *f;
	Futex *x;
	void *addr;

	if(decref(space))
		return;
	for(t=0; t<SEGMAX; t++){
		if(seg = space->seg[t]){
			addr = (void*)seg->addr.base;
			if(!decref(seg)){
				qlock(seg);
				/* mark all areas as free */
				while(a = seg->areas)
					freearea(a);

				/* clear the free lists */
				while(a = seg->freearea){
					seg->freearea = a->next;
					free(a);
				}
				while(f = seg->freefilemap){
					seg->freefilemap = f->next;
					free(f);
				}
				while(x = seg->freefutex){
					seg->freefutex = x->next;
					free(x);
				}
				free(seg);
			}
			if(segdetach(addr) < 0)
				panic("putspace: segdetach %s segment: %r", segname[t]);
		}
	}
	free(space);
}

static int
canmerge(Area *a, Area *b)
{
	return a->filemap==nil && 
		a->futex==nil &&
		b->filemap==nil &&
		b->futex==nil &&
		a->prot == b->prot;
}

static void
mergearea(Area *a)
{
	if(a->prev && a->prev->addr.top == a->addr.base && canmerge(a->prev, a)){
		a->addr.base = a->prev->addr.base;
		freearea(a->prev);
	}
	if(a->next && a->next->addr.base == a->addr.top && canmerge(a->next, a)){
		a->addr.top = a->next->addr.top;
		freearea(a->next);
	}
}

static int
findhole(Seg *seg, Range *r, int fixed)
{
	Range h;
	Area *a;
	ulong m;
	ulong z;
	ulong hz;

	z = r->top - r->base;
	m = ~0;
	h.base = seg->addr.base;
	a = seg->areas;
	for(;;) {
		if((h.top = a ? a->addr.base : seg->addr.top) > h.base) {
			if(fixed){
				if(h.base > r->base)
					break;
				if((r->base >= h.base) && (r->top <= h.top))
					goto found;
			} else {
				hz = h.top - h.base;
				if((hz >= z) && (hz < m)) {
					r->base = h.top - z;
					r->top = h.top;
					if((m = hz) == z)
						goto found;
				}
			}
		}
		if(a == nil)
			break;
		h.base = a->addr.top;
		a = a->next;
	}
	if(!fixed && (m != ~0))
		goto found;
	return 0;

found:
	return 1;
}

/* wake up all futexes in range and unlink from area */
static void
wakefutexarea(Area *a, Range addr)
{
	Futex *fu, *x;

	for(fu = a->futex; fu; fu = x){
		x = fu->next;
		if((ulong)fu->addr >= addr.base && (ulong)fu->addr < addr.top){
			if(*fu->link = x)
				x->link = fu->link;
			fu->link = nil;
			fu->next = nil;

			trace("wakefutexarea: fu=%p addr=%p", fu, fu->addr);
			wakeq(fu, MAXPROC);
		}
	}
}

static void
makehole(Seg *seg, Range r)
{
	Area *a, *b, *x;
	Range f;

	for(a = seg->areas; a; a = x){
		x = a->next;

		if(a->addr.top <= r.base)
			continue;
		if(a->addr.base >= r.top)
			break;

		f = r;
		if(f.base < a->addr.base)
			f.base = a->addr.base;
		if(f.top > a->addr.top)
			f.top = a->addr.top;

		wakefutexarea(a, f);
		if(f.base == a->addr.base){
			if(f.top == a->addr.top){
				freearea(a);
			} else {
				a->addr.base = f.top;
			}
		} else if(f.top == a->addr.top){
			a->addr.top = f.base;
		} else {
			b = duparea(a);
			b->addr.base = f.top;

			a->addr.top = f.base;
			linkarea(seg, b);
		}

		if(segfree((void*)f.base, f.top - f.base) < 0)
			panic("makehole: segfree %s segment: %r", segname[seg->type]);
	}
}

static Seg*
addr2seg(Space *space, ulong addr)
{
	Seg *seg;
	int t;

	for(t=0; t<SEGMAX; t++){
		if((seg = space->seg[t]) == nil)
			continue;
		qlock(seg);
		if((addr >= seg->addr.base) && (addr < seg->addr.top))
			return seg;
		qunlock(seg);
	}

	return nil;
}

static Area*
addr2area(Seg *seg, ulong addr)
{
	Area *a;

	for(a=seg->areas; a; a=a->next)
		if((addr >= a->addr.base) && (addr < a->addr.top))
			return a;
	return nil;
}

int
okaddr(void *ptr, int len, int write)
{
	ulong addr;
	Space *space;
	Seg *seg;
	Area *a;
	int ok;

	ok = 0;
	addr = (ulong)ptr;
	if(addr < PAGESIZE)
		goto out;
	if(space = current->mem){
		qlock(space);
		if(seg = addr2seg(space, addr)){
			while(a = addr2area(seg, addr)){
				if(write){
					if((a->prot & PROT_WRITE) == 0)
						break;
				} else {
					if((a->prot & PROT_READ) == 0)
						break;
				}
				if((ulong)ptr + len <= a->addr.top){
					ok = 1;
					break;
				}
				addr = a->addr.top;
			}
			qunlock(seg);
		}
		qunlock(space);
	}
out:
	trace("okaddr(%lux-%lux, %d) -> %d", addr, addr+len, write, ok);
	return ok;
}

static void
unmapspace(Space *space, Range r)
{
	Seg *seg;
	int t;

	for(t=0; t<SEGMAX; t++){
		if((seg = space->seg[t]) == nil)
			continue;
		qlock(seg);
		if(seg->addr.base >= r.top){
			qunlock(seg);
			break;
		}
		if(seg->addr.top > r.base)
			makehole(seg, r);
		qunlock(seg);
	}
}

static Area*
mapspace(Space *space, Range r, int flags, int prot, int *perr)
{
	Seg *seg;
	Area *a;
	Range f;
	int t;

	if(flags & MAP_PRIVATE){
		if(r.base >= space->seg[SEGSTACK]->addr.base){
			t = SEGSTACK;
		} else if(r.base >= space->seg[SEGDATA]->addr.base && 
			r.base < space->seg[SEGDATA]->limit){
			t = SEGDATA;
		} else {
			t = SEGPRIVATE;
		}
	} else {
		t = SEGSHARED;
	}

	if((seg = space->seg[t]) == nil)
		goto nomem;

	qlock(seg);
	if((r.base >= seg->addr.base) && (r.top <= seg->limit)){
		if(r.base >= seg->addr.top)
			goto addrok;

		f = r;
		if(f.top > seg->addr.top)
			f.top = seg->addr.top;
		if(findhole(seg, &f, 1))
			goto addrok;
		if(flags & MAP_FIXED){
			if(seg->type == SEGSHARED){
				trace("mapspace(): cant make hole %lux-%lux in shared segment",
					f.base, f.top);
				goto nomem;
			}
			makehole(seg, f);
			goto addrok;
		}		
	}

	if(flags & MAP_FIXED){
		trace("mapspace(): no free hole for fixed mapping %lux-%lux in %s segment", 
			r.base, r.top, segname[seg->type]);
		goto nomem;
	}

	if(findhole(seg, &r, 0))
		goto addrok;

	r.top -= r.base;
	r.base = seg->addr.top;
	r.top += r.base;

addrok:
	trace("mapspace(): addr %lux-%lux", r.base, r.top);

	if(r.top > seg->addr.top){
		if(r.top > seg->limit){
			trace("mapspace(): area top %lux over %s segment limit %lux",
				r.top, segname[seg->type], seg->limit);
			goto nomem;
		}
		trace("mapspace(): segbrk %s segment %lux-%lux -> %lux",
			segname[seg->type], seg->addr.base, seg->addr.top, r.top);
		if(segbrk((void*)seg->addr.base, (void*)r.top) == (void*)-1){
			trace("mapspace(): segbrk failed: %r");
			goto nomem;
		}
		seg->addr.top = r.top;
	}

	if(a = seg->freearea){
		seg->freearea = a->next;
	} else {
		a = kmalloc(sizeof(Area));
	}
	a->addr = r;
	a->prot = prot;
	a->filemap = nil;
	a->futex = nil;

	linkarea(seg, a);

	/* keep seg locked */
	return a;

nomem:
	if(seg != nil)
		qunlock(seg);
	if(perr) *perr = -ENOMEM;
	return nil;
}

static ulong
brkspace(Space *space, ulong bk)
{
	Seg *seg;
	Area *a;
	ulong old, new;
	Range r;

	if((seg = space->seg[SEGDATA]) == nil)
		goto out;

	qlock(seg);
	if(space->brk < seg->addr.base)
		space->brk = seg->addr.top;

	if(bk < seg->addr.base)
		goto out;

	old = pagealign(space->brk);
	new = pagealign(bk);

	if(old != new){
		if(bk < space->brk){
			r.base = new;
			r.top = old;
			qunlock(seg);
			seg = nil;

			unmapspace(space, r);
		} else {
			r.base = old;
			r.top = new;

			trace("brkspace(): new mapping %lux-%lux", r.base, r.top);
			for(a = addr2area(seg, old - PAGESIZE); a; a = a->next){
				if(a->addr.top <= r.base)
					continue;
				if(a->addr.base > r.top + PAGESIZE)
					break;

				trace("brkspace(): mapping %lux-%lux is in the way", a->addr.base, a->addr.top);
				goto out;
			}
			qunlock(seg);
			seg = nil;

			a = mapspace(space, r,
				MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED,
				PROT_READ|PROT_WRITE|PROT_EXEC, nil);

			if(a == nil)
				goto out;

			seg = a->seg;
			mergearea(a);
		}
	}

	if(space->brk != bk){
		trace("brkspace: set new brk %lux", bk);
		space->brk = bk;
	}

out:
	if(seg != nil)
		qunlock(seg);

	return space->brk;
}

static ulong
remapspace(Space *space, ulong addr, ulong oldlen, ulong newlen, ulong newaddr, int flags)
{
	Area *a;
	Seg *seg;
	int move;
	Range r;

	if(pagealign(addr) != addr)
		return -EINVAL;

	oldlen = pagealign(oldlen);
	newlen = pagealign(newlen);

	if((addr + oldlen) < addr)
		return -EINVAL;
	if((addr + newlen) <= addr)
		return -EINVAL;

	move = 0;
	if(flags & MREMAP_FIXED){
		if(pagealign(newaddr) != newaddr)
			return -EINVAL;
		if((flags & MREMAP_MAYMOVE) == 0)
			return -EINVAL;
		if((newaddr <= addr) && ((newaddr+newlen)  > addr))
			return -EINVAL;
		if((addr <= newaddr) && ((addr+oldlen) > newaddr))
			return -EINVAL;
		move = (newaddr != addr);
	}

	if(newlen < oldlen){
		r.base = addr + newlen;
		r.top = addr + oldlen;

		unmapspace(space, r);

		oldlen = newlen;
	}

	if((newlen == oldlen) && !move)
		return addr;

	if((seg = addr2seg(space, addr)) == nil)
		return -EFAULT;

	if((a = addr2area(seg, addr)) == nil)
		goto fault;
	if(a->addr.top < (addr + oldlen))
		goto fault;

	if(move)
		goto domove;
	if((addr + oldlen) != a->addr.top)
		goto domove;
	if((addr + newlen) > seg->limit)
		goto domove;
	if(a->next != nil)
		if((addr + newlen) > a->next->addr.base)
			goto domove;

	if((addr + newlen) > seg->addr.top){
		trace("remapspace(): segbrk %s segment %lux-%lux -> %lux", 
			segname[seg->type], seg->addr.base, seg->addr.top, (addr + newlen));
		if(segbrk((void*)seg->addr.base, (void*)(addr + newlen)) == (void*)-1){
			trace("remapspace(): segbrk: %r");
			goto domove;
		}

		seg->addr.top = (addr + newlen);
	}
	a->addr.top = (addr + newlen);
	mergearea(a);
	qunlock(seg);

	return addr;

domove:
	trace("remapspace(): domove not implemented");
	if(seg != nil)
		qunlock(seg);
	return -ENOMEM;

fault:
	if(seg != nil)
		qunlock(seg);
	return -EFAULT;
}

static void
syncspace(Space *space, Range r)
{
	Seg *seg;
	Area *a;

	if(seg = addr2seg(space, r.base)){
		for(a = addr2area(seg, r.base); a; a=a->next){
			if(r.base >= a->addr.top)
				break;
			syncarea(a, r);
		}
		qunlock(seg);
	}
}

void*
mapstack(int size)
{
	Space *space;
	ulong a;

	space = current->mem;
	a = space->seg[SEGSTACK]->addr.top;
	size = pagealign(size);
	a = sys_mmap(a - size, size, 
		PROT_READ|PROT_WRITE, 
		MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if(a == 0)
		return nil;

	return (void*)(a + size);
}

void
mapdata(ulong base)
{
	Space *space;
	Range r;
	ulong top;
	int t;

	space = current->mem;
	base = pagealign(base);
	top = space->seg[SEGSTACK]->addr.base - PAGESIZE;

	for(t=0; t<SEGMAX; t++){
		if(space->seg[t] == nil){
			switch(t){
			case SEGDATA:
				r.base = base;
				break;
			case SEGPRIVATE:
				r.base = base + 0x10000000;
				break;
			case SEGSHARED:
				r.base = top - 0x10000000;
				break;
			}
			r.top = r.base + PAGESIZE;
			space->seg[t] = allocseg(t, r, r.top, 0, (t == SEGSHARED) ? "shared" : "memory");
		}
		if(t > 0 && space->seg[t-1])
			space->seg[t-1]->limit = space->seg[t]->addr.base - PAGESIZE;
	}
}

/*
 * unmapuserspace is called from kprocfork to get rid of
 * the linux memory segments used by the calling process
 * before current is set to zero. we just segdetach() all that
 * segments but keep the data structures valid for the calling
 * (linux) process.
 */
void
unmapuserspace(void)
{
	Space *space;
	Seg *seg;
	int t;

	space = current->mem;
	qlock(space);
	for(t=0; t<SEGMAX; t++){
		if((seg = space->seg[t]) == nil)
			continue;
		if(segdetach((void*)seg->addr.base) < 0)
			panic("unmapuserspace: segdetach %s segment: %r", segname[seg->type]);
	}
	qunlock(space);
}

/* hack: 
 * we write segment out into a file, detach it and reattach
 * a new one and reading contents back. i'm surprised that
 * this even works seamless with the Plan9 Bss! :-)
 */
static void
convertseg(Range r, ulong attr, char *class)
{
	char name[64];
	ulong p;
	int n;
	int fd;
	ulong len;

	snprint(name, sizeof(name), "/tmp/seg%s%d", class, getpid());
	fd = create(name, ORDWR|ORCLOSE, 0600);
	if(fd < 0)
		panic("convertseg: cant create %s: %r", name);

	len = r.top - r.base;

	if(len > 0){
		n = write(fd, (void*)r.base, len);
		if(n != len)
			panic("convertseg: write: %r");
	}

	/* copy string to stack because its memory gets detached :-) */
	strncpy(name, class, sizeof(name));

	trace("detaching %lux-%lux", r.base, r.top);

	/* point of no return */
	if(segdetach((void*)r.base) < 0)
		panic("convertseg: segdetach: %r");
	if(segattach(attr, name, (void*)r.base, len) != (void*)r.base)
		*((int*)0) = 0;

	p = 0;
	while(p < len) {
		/*
		 * we use pread directly to avoid hitting profiling code until
		 * data segment is read back again. pread is unprofiled syscall
		 * stub.
		 */
		n = pread(fd, (void*)(r.base + p), len - p, (vlong)p);
		if(n <= 0)
			*((int*)0) = 0;
		p += n;
	}

	/* anything normal again */
	trace("segment %lux-%lux reattached as %s", r.base, r.top, class);

	close(fd);
}

void initmem(void)
{
	Space *space;
	Range r, x;
	char buf[80];
	int fd;
	int n;

	static int firsttime = 1;

	space = kmallocz(sizeof(Space), 1);
	space->ref = 1;

	snprint(buf, sizeof(buf), "/proc/%d/segment", getpid());
	if((fd = open(buf, OREAD)) < 0)
		panic("initspace: cant open %s: %r", buf);

	n = 10 + 9 + 9 + 4 + 1;
	x.base = x.top = 0;
	while(readn(fd, buf, n)==n){
		char *name;

		buf[8] = 0;
		buf[18] = 0;
		buf[28] = 0;
		buf[33] = 0;
	
		name = &buf[0];
		r.base = strtoul(&buf[9], nil, 16);
		r.top = strtoul(&buf[19], nil, 16);

		trace("initspace(): %s %lux-%lux", name, r.base, r.top);

		if(firsttime){
			/*
			 * convert Plan9 data+bss segments into shared segments so
			 * that the memory of emulator data structures gets shared across 
			 * all processes. This only happens if initspace() is called the first time.
			 */
			if(strstr(name, "Data")==name)
				convertseg(r, 0, "shared");
			if(strstr(name, "Bss")==name)
				convertseg(r, 0, "shared");
		}

		if(strstr(name, "Stack")==name){
			x.top = r.base - PAGESIZE;
			x.base = x.top - pagealign((MAXPROC / 4) * USTACK);

			if(!firsttime)
				break;
		}
	}
	close(fd);
	firsttime = 0;

	/* allocate the linux stack */
	space->seg[SEGSTACK] = allocseg(SEGSTACK, x, x.top, 0, "memory");

	current->mem = space;
}

void exitmem(void)
{
	Space *space;

	if(space = current->mem){
		current->mem = nil;
		putspace(space);
	}
}

void clonemem(Uproc *new, int copy)
{
	Space *space;

	if((space = current->mem) == nil){
		new->mem = nil;
		return;
	}
	new->mem = getspace(space, copy);
}

ulong procmemstat(Uproc *proc, ulong *pdat, ulong *plib, ulong *pshr, ulong *pstk, ulong *pexe)
{
	Space *space;
	ulong size, z;
	int i;

	if(pdat) *pdat = 0;
	if(plib) *plib = 0;
	if(pshr) *pshr = 0;
	if(pstk) *pstk = 0;
	if(pexe) *pexe = 0;

	if((space = proc->mem) == nil)
		return 0;

	size = 0;
	qlock(space);
	for(i=0; i<SEGMAX; i++){
		Area *a;
		Seg *seg;
		if((seg = space->seg[i]) == nil)
			continue;
		qlock(seg);
		for(a = seg->areas; a; a = a->next){
			z = a->addr.top - a->addr.base;
			switch(i){
			case SEGDATA:
				if(pdat)
					*pdat += z;
			case SEGPRIVATE:
				if(plib)
					*plib += z;
				break;
			case SEGSHARED:
				if(pshr)
					*pshr += z;
				break;
			case SEGSTACK:
				if(pstk)
					*pstk += z;
				break;
			}
			if(pexe && (a->prot & PROT_EXEC))
				*pexe += z;
			size += z;
		}
		qunlock(seg);
	}
	qunlock(space);

	return size;
}

struct linux_mmap_args {
 	ulong addr;
	int len;
	int prot;
	int flags;
	int fd;
	ulong offset;
};

ulong
sys_linux_mmap(void *a)
{
	struct linux_mmap_args *p = a;

	if(pagealign(p->offset) != p->offset)
		return -EINVAL;

	return sys_mmap(
		p->addr, 
		p->len,
		p->prot,
		p->flags,
		p->fd,
		p->offset / PAGESIZE);
}

ulong
sys_mmap(ulong addr, ulong len, int prot, int flags, int fd, ulong pgoff)
{
	Space *space;
	Seg *seg;
	Range r;
	ulong o;
	int e, n;
	Area *a;
	Filemap *f;
	Ufile *file;

	trace("sys_mmap(%lux, %lux, %d, %d, %d, %lux)", addr, len, prot, flags, fd, pgoff);

	if(pagealign(addr) != addr)
		return (ulong)-EINVAL;

	r.base = addr;
	r.top = addr + pagealign(len);
	if(r.top <= r.base)
		return (ulong)-EINVAL;

	file = nil;
	if((flags & MAP_ANONYMOUS)==0)
		if((file = fdgetfile(fd))==nil)
			return (ulong)-EBADF;

	space = current->mem;
	qlock(space);
	if((a = mapspace(space, r, flags, prot, &e)) == nil){
		qunlock(space);
		putfile(file);
		return (ulong)e;
	}

	seg = a->seg;
	r = a->addr;

	if(flags & MAP_ANONYMOUS){
		mergearea(a);
		qunlock(seg);
		qunlock(space);

		return r.base;
	}

	o = pgoff * PAGESIZE;

	if(f = seg->freefilemap)
		seg->freefilemap = f->next;
	if(f == nil)
		f = kmalloc(sizeof(Filemap));
	f->ref = 1;
	f->addr = r;
	f->next = nil;
	f->path = kstrdup(file->path);
	f->offset = o;
	if((f->mode = file->mode) != O_RDONLY){
		f->file = getfile(file);
	} else {
		f->file = nil;
	}
	a->filemap = f;
	qunlock(seg);
	qunlock(space);

	trace("map %s [%lux-%lux] at [%lux-%lux]", file->path, o, o + (r.top - r.base), r.base, r.top);

	addr = r.base;
	while(addr < r.top){
		n = preadfile(file, (void*)addr, r.top - addr, o);
		if(n == 0)
			break;
		if(n < 0){
			trace("read failed at offset %lux for address %lux failed: %r", o, addr);
			break;
		}
		addr += n;
		o += n;
	}

	putfile(file);

	return r.base;
}

int sys_munmap(ulong addr, ulong len)
{
	Space *space;
	Range r;

	trace("sys_munmap(%lux, %lux)", addr, len);

	if(pagealign(addr) != addr)
		return -EINVAL;
	r.base = addr;
	r.top = addr + pagealign(len);
	if(r.top <= r.base)
		return -EINVAL;

	space = current->mem;
	qlock(space);
	unmapspace(current->mem, r);
	qunlock(space);

	return 0;
}

ulong
sys_brk(ulong bk)
{
	Space *space;
	ulong a;

	trace("sys_brk(%lux)", bk);

	space = current->mem;
	qlock(space);
	a = brkspace(space, bk);
	qunlock(space);

	return a;
}

int sys_mprotect(ulong addr, ulong len, int prot)
{
	Space *space;
	Seg *seg;
	Area *a, *b;
	int err;

	trace("sys_mprotect(%lux, %lux, %lux)", addr, len, (ulong)prot);

	len = pagealign(len);
	if(pagealign(addr) != addr)
		return -EINVAL;
	if(len == 0)
		return -EINVAL;

	err = -ENOMEM;
	space = current->mem;
	qlock(space);
	if(seg = addr2seg(space, addr)){
		for(a = addr2area(seg, addr); a!=nil; a=a->next){
			if(addr + len <= a->addr.base)
				break;
			err = 0;
			if(a->prot == prot)
				continue;
			wakefutexarea(a, a->addr);
			if(a->addr.base < addr){
				b = duparea(a);
				a->addr.base = addr;
				b->addr.top = addr;
				linkarea(seg, b);
			}
			if(a->addr.top > addr + len){
				b = duparea(a);
				a->addr.top = addr + len;
				b->addr.base = addr + len;
				linkarea(seg, b);
			}
			trace("%lux-%lux %lux -> %lux", a->addr.base, a->addr.top, (ulong)a->prot, (long)prot);
			a->prot = prot;
		}
		qunlock(seg);
	}
	qunlock(space);

	return err;
}

int sys_msync(ulong addr, ulong len, int flags)
{
	Space *space;
	Range r;

	trace("sys_msync(%lux, %lux, %x)", addr, len, flags);

	if(pagealign(addr) != addr)
		return -EINVAL;
	r.base = addr;
	r.top = addr + pagealign(len);
	if(r.top <= r.base)
		return -EINVAL;

	space = current->mem;
	qlock(space);
	syncspace(space, r);
	qunlock(space);

	return 0;
}

ulong
sys_mremap(ulong addr, ulong oldlen, ulong newlen, int flags, ulong newaddr)
{
	Space *space;
	int r;

	trace("sys_mremap(%lux, %lux, %lux, %x, %lux)",
		addr, oldlen, newlen, flags, newaddr);

	space = current->mem;
	qlock(space);
	r = remapspace(space, addr, oldlen, newlen, newaddr, flags);
	qunlock(space);

	return r;
}

enum {
	FUTEX_WAIT,
	FUTEX_WAKE,
	FUTEX_FD,
	FUTEX_REQUEUE,
	FUTEX_CMP_REQUEUE,
};

int sys_futex(ulong *addr, int op, int val, void *ptime, ulong *addr2, int val3)
{
	Space *space;
	Seg *seg;
	Area *a;
	Futex *fu, *fu2;
	int err, val2;
	vlong timeout;

	trace("sys_futex(%p, %d, %d, %p, %p, %d)", addr, op, val, ptime, addr2, val3);

	seg = nil;
	err = -EFAULT;
	if((space = current->mem) == 0)
		goto out;

	qlock(space);
	if((seg = addr2seg(space, (ulong)addr)) == nil){
		qunlock(space);
		goto out;
	}
	qunlock(space);
	if((a = addr2area(seg, (ulong)addr)) == nil)
		goto out;
	for(fu = a->futex; fu; fu = fu->next)
		if(fu->addr == addr)
			break;

	switch(op){
	case FUTEX_WAIT:
		trace("sys_futex(): FUTEX_WAIT futex=%p addr=%p", fu, addr);

		if(fu == nil){
			if(fu = seg->freefutex){
				seg->freefutex = fu->next;
			} else {
				fu = kmallocz(sizeof(Futex), 1);
			}
			fu->ref = 1;
			fu->addr = addr;
			if(fu->next = a->futex)
				fu->next->link = &fu->next;
			fu->link = &a->futex;
			a->futex = fu;
		} else {
			incref(fu);
		}

		err = 0;
		timeout = 0;
		if(ptime != nil){
			struct linux_timespec *ts = ptime;
			vlong now;

			wakeme(1);
			now = nsec();
			if(current->restart->syscall){
				timeout = current->restart->futex.timeout;
			} else {
				timeout = now + (vlong)ts->tv_sec * 1000000000LL + ts->tv_nsec;
			}
			if(now < timeout){
				current->timeout = timeout;
				setalarm(timeout);
			} else {
				err = -ETIMEDOUT;
			}
		}
		if(err == 0){
			if(*addr != val){
				err = -EWOULDBLOCK;
			} else {
				err = sleepq(fu, seg, 1);
			}
		}
		if(ptime != nil){
			current->timeout = 0;
			wakeme(0);
		}
		if(err == -ERESTART)
			current->restart->futex.timeout = timeout;

		if(!decref(fu)){
			if(fu->link){
				if(*fu->link = fu->next)
					fu->next->link = fu->link;
				fu->link = nil;
				fu->next = nil;
			}
			fu->next = seg->freefutex;
			seg->freefutex = fu;
		}
		break;

	case FUTEX_WAKE:
		trace("sys_futex(): FUTEX_WAKE futex=%p addr=%p", fu, addr);
		err = fu ? wakeq(fu, val < 0 ? 0 : val) : 0;
		break;

	case FUTEX_CMP_REQUEUE:
		trace("sys_futex(): FUTEX_CMP_REQUEUE futex=%p addr=%p", fu, addr);
		if(*addr != val3){
			err = -EAGAIN;
			break;
	case FUTEX_REQUEUE:
			trace("sys_futex(): FUTEX_REQUEUE futex=%p addr=%p", fu, addr);
		}
		err = fu ? wakeq(fu, val < 0 ? 0 : val) : 0;
		if(err > 0){
			val2 = (int)ptime;

			/* BUG: fu2 has to be in the same segment as fu */
			if(a = addr2area(seg, (ulong)addr2)){
				for(fu2 = a->futex; fu2; fu2 = fu2->next){
					if(fu2->addr == addr2){
						err += requeue(fu, fu2, val2);
						break;
					}
				}
			}
		}
		break;

	default:
		err = -ENOSYS;
	}

out:
	if(seg)
		qunlock(seg);
	return err;
}
