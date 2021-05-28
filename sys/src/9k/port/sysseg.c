#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

Segment* (*_globalsegattach)(Proc*, char*);

static Lock physseglock;

int
addphysseg(Physseg* new)
{
	Physseg *ps;

	/*
	 * Check not already entered and there is room
	 * for a new entry and the terminating null entry.
	 */
	lock(&physseglock);
	for(ps = physseg; ps->name; ps++){
		if(strcmp(ps->name, new->name) == 0){
			unlock(&physseglock);
			return -1;
		}
	}
	if(ps-physseg >= nphysseg-2){
		unlock(&physseglock);
		return -1;
	}

	*ps = *new;
	unlock(&physseglock);

	return 0;
}

int
isphysseg(char *name)
{
	int rv;
	Physseg *ps;

	lock(&physseglock);
	rv = 0;
	for(ps = physseg; ps->name; ps++){
		if(strcmp(ps->name, name) == 0){
			rv = 1;
			break;
		}
	}
	unlock(&physseglock);
	return rv;
}

/* Needs to be non-static for BGP support */
uintptr
ibrk(uintptr addr, int seg)
{
	Segment *s, *ns;
	uintptr newtop, pgsize;
	usize newsize;
	int i, mapsize;
	Pte **map;

	s = up->seg[seg];
	if(s == nil)
		error(Ebadarg);

	if(addr == 0)
		return s->top;

	qlock(&s->lk);
	if(waserror()){
		qunlock(&s->lk);
		nexterror();
	}

	/* We may start with the bss overlapping the data */
	if(addr < s->base) {
		if(seg != BSEG || up->seg[DSEG] == nil || addr < up->seg[DSEG]->base)
			error(Enovmem);
		addr = s->base;
	}

	pgsize = segpgsize(s);
	newtop = ROUNDUP(addr, pgsize);
	newsize = (newtop-s->base)/pgsize;
	if(newtop < s->top) {
		/*
		 * do not shrink a segment shared with other procs, as the
		 * to-be-freed address space may have been passed to the kernel
		 * already by another proc and is past the validaddr stage.
		 */
		if(s->ref > 1)
			error(Einuse);
		mfreeseg(s, newtop, s->top);
		s->top = newtop;
		poperror();
		s->size = newsize;
		qunlock(&s->lk);
		mmuflush();
		return newtop;
	}

	for(i = 0; i < NSEG; i++) {
		ns = up->seg[i];
		if(ns == nil || ns == s)
			continue;
		if(newtop > ns->base && s->base < ns->top)
			error(Esoverlap);
	}

	mapsize = HOWMANY(newsize, PTEPERTAB);
	if(mapsize > SEGMAPSIZE)
		error(Enovmem);
	if(mapsize > s->mapsize){
		map = smalloc(mapsize*sizeof(Pte*));
		memmove(map, s->map, s->mapsize*sizeof(Pte*));
		if(s->map != s->ssegmap)
			free(s->map);
		s->map = map;
		s->mapsize = mapsize;
	}

	s->top = newtop;
	s->size = newsize;

	poperror();
	qunlock(&s->lk);

	return newtop;
}

void
syssegbrk(Ar0* ar0, va_list list)
{
	int i;
	uintptr addr;
	Segment *s;

	/*
	 * int segbrk(void*, void*);
	 * should be
	 * void* segbrk(void* saddr, void* addr);
	 */
	addr = PTR2UINT(va_arg(list, void*));
	for(i = 0; i < NSEG; i++) {
		s = up->seg[i];
		if(s == nil || addr < s->base || addr >= s->top)
			continue;
		switch(s->type&SG_TYPE) {
		case SG_TEXT:
		case SG_DATA:
		case SG_STACK:
			error(Ebadarg);
		default:
			addr = PTR2UINT(va_arg(list, void*));
			ar0->v = UINT2PTR(ibrk(addr, i));
			return;
		}
	}
	error(Ebadarg);
}

void
sysbrk_(Ar0* ar0, va_list list)
{
	uintptr addr;

	/*
	 * int brk(void*);
	 *
	 * Deprecated; should be for backwards compatibility only.
	 */
	addr = PTR2UINT(va_arg(list, void*));

	ibrk(addr, BSEG);

	ar0->i = 0;
}

static uintptr
segattach(Proc* p, int attr, char* name, uintptr va, usize len)
{
	int sno;
	Segment *s, *os;
	Physseg *ps;
	uintptr pgsize;

	/* BUG: Only ok for now */
	if((va != 0 && va < UTZERO) || iskaddr(va))
		error("virtual address in kernel");

	vmemchr(name, 0, ~0);

	for(sno = 0; sno < NSEG; sno++)
		if(p->seg[sno] == nil && sno != ESEG)
			break;

	if(sno == NSEG)
		error("too many segments in process");

	/*
	 *  first look for a global segment with the
	 *  same name
	 */
	if(_globalsegattach != nil){
		s = (*_globalsegattach)(p, name);
		if(s != nil){
			p->seg[sno] = s;
			return s->base;
		}
	}

	for(ps = physseg; ps->name; ps++)
		if(strcmp(name, ps->name) == 0)
			goto found;

	error("segment not found");
found:
	pgsize = physsegpgsize(ps);
	len = ROUNDUP(len, pgsize);
	if(len == 0)
		error("length overflow");

	/*
	 * Find a hole in the address space.
	 * Starting at the lowest possible stack address - len,
	 * check for an overlapping segment, and repeat at the
	 * base of that segment - len until either a hole is found
	 * or the address space is exhausted.
	 */
//need check here to prevent mapping page 0?
	if(va == 0) {
		va = p->seg[SSEG]->base - len;
		for(;;) {
			os = isoverlap(p, va, len);
			if(os == nil)
				break;
			va = os->base;
			if(len > va)
				error("cannot fit segment at virtual address");
			va -= len;
		}
	}

	va = va&~(pgsize-1);
	if(isoverlap(p, va, len) != nil)
		error(Esoverlap);

	if((len/pgsize) > ps->size)
		error("len > segment size");

	attr &= ~SG_TYPE;		/* Turn off what is not allowed */
	attr |= ps->attr;		/* Copy in defaults */

	s = newseg(attr, va, va+len);
	s->pseg = ps;
	p->seg[sno] = s;

	return va;
}

void
syssegattach(Ar0* ar0, va_list list)
{
	int attr;
	char *name;
	uintptr va;
	usize len;

	/*
	 * long segattach(int, char*, void*, ulong);
	 * should be
	 * void* segattach(int, char*, void*, usize);
	 */
	attr = va_arg(list, int);
	name = va_arg(list, char*);
	va = PTR2UINT(va_arg(list, void*));
	len = va_arg(list, usize);

	ar0->v = UINT2PTR(segattach(up, attr, validaddr(name, 1, 0), va, len));
}

void
syssegdetach(Ar0* ar0, va_list list)
{
	int i;
	uintptr addr;
	Segment *s;

	/*
	 * int segdetach(void*);
	 */
	addr = PTR2UINT(va_arg(list, void*));

	qlock(&up->seglock);
	if(waserror()){
		qunlock(&up->seglock);
		nexterror();
	}

	s = 0;
	for(i = 0; i < NSEG; i++)
		if(s = up->seg[i]) {
			qlock(&s->lk);
			if((addr >= s->base && addr < s->top) ||
			   (s->top == s->base && addr == s->base))
				goto found;
			qunlock(&s->lk);
		}

	error(Ebadarg);

found:
	/*
	 * Can't detach the initial stack segment
	 * because the clock writes profiling info
	 * there.
	 */
	if(s == up->seg[SSEG]){
		qunlock(&s->lk);
		error(Ebadarg);
	}
	up->seg[i] = 0;
	qunlock(&s->lk);
	putseg(s);
	qunlock(&up->seglock);
	poperror();

	/* Ensure we flush any entries from the lost segment */
	mmuflush();

	ar0->i = 0;
}

void
syssegfree(Ar0* ar0, va_list list)
{
	Segment *s;
	uintptr from, to, pgsize;
	usize len;

	/*
	 * int segfree(void*, ulong);
	 * should be
	 * int segfree(void*, usize);
	 */
	from = PTR2UINT(va_arg(list, void*));
	s = seg(up, from, 1);
	if(s == nil)
		error(Ebadarg);
	len = va_arg(list, usize);
	pgsize = segpgsize(s);
	to = (from + len) & ~(pgsize-1);
	if(to < from || to > s->top){
		qunlock(&s->lk);
		error(Ebadarg);
	}
	from = ROUNDUP(from, pgsize);

	mfreeseg(s, from, to);
	qunlock(&s->lk);
	mmuflush();

	ar0->i = 0;
}

static void
pteflush(Pte *pte, int s, int e)
{
	int i;

	for(i = s; i < e; i++)
		mmucachectl(pte->pages[i], PG_TXTFLUSH);
}

void
syssegflush(Ar0* ar0, va_list list)
{
	Segment *s;
	uintptr addr, pgsize;
	Pte *pte;
	usize chunk, l, len, pe, ps;

	/*
	 * int segflush(void*, ulong);
	 * should be
	 * int segflush(void*, usize);
	 */
	addr = PTR2UINT(va_arg(list, void*));
	len = va_arg(list, usize);

	while(len > 0) {
		s = seg(up, addr, 1);
		if(s == nil)
			error(Ebadarg);

		s->flushme = 1;
		pgsize = segpgsize(s);
	more:
		l = len;
		if(addr+l > s->top)
			l = s->top - addr;

		ps = addr-s->base;
		pte = s->map[ps/s->ptemapmem];
		ps &= s->ptemapmem-1;
		pe = s->ptemapmem;
		if(pe-ps > l){
			pe = ps + l;
			pe = (pe+pgsize-1)&~(pgsize-1);
		}
		if(pe == ps) {
			qunlock(&s->lk);
			error(Ebadarg);
		}

		if(pte)
			pteflush(pte, ps/pgsize, pe/pgsize);

		chunk = pe-ps;
		len -= chunk;
		addr += chunk;

		if(len > 0 && addr < s->top)
			goto more;

		qunlock(&s->lk);
	}
	mmuflush();

	ar0->i = 0;
}
