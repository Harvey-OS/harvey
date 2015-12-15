/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

	if(new->pgszi < 0)
		new->pgszi = getpgszi(2*MiB);	/* 2M pages by default */
	if(new->pgszi < 0)
		panic("addphysseg");
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
uintptr_t
ibrk(uintptr_t addr, int seg)
{
	Proc *up = externup();
	Segment *s, *ns;
	uintptr_t newtop, rtop;
	int32_t newsize;
	int i, mapsize;
	Pte **map;
	uintmem pgsz;

	s = up->seg[seg];
	if(s == 0)
		error(Ebadarg);

	if(addr == 0)
		return s->top;

	qlock(&s->lk);
	if(waserror()) {
		qunlock(&s->lk);
		nexterror();
	}

	/* We may start with the bss overlapping the data */
	if(addr < s->base)
		addr = s->base;

	pgsz = sys->pgsz[s->pgszi];
	newtop = ROUNDUP(addr, pgsz);
	newsize = (newtop-s->base)/pgsz;
	if(newtop < s->top) {
		mfreeseg(s, newtop, (s->top-newtop)/pgsz);
		s->top = newtop;
		s->size = newsize;
		poperror();
		qunlock(&s->lk);
		mmuflush();
		return newtop;
	}
	if(newsize > (SEGMAPSIZE*s->ptepertab))
		error(Enovmem);

	for(i = 0; i < NSEG; i++) {
		ns = up->seg[i];
		if(ns == 0 || ns == s)
			continue;
		if(newtop >= ns->base && newtop < ns->top)
			error(Esoverlap);
	}

	/*
	 * TODO(aki): reintroduce 1G page support:
	 * brk the bss up to the 1G boundary, and create
	 * a segment placed at that boundary, using 1G pages if it can.
	 * This is both back compatible, transparent, and permits using 1G pages.
	 */
	rtop = newtop;


	mapsize = HOWMANY(newsize, s->ptepertab);
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

	return rtop;
}

void
syssegbrk(Ar0* ar0, ...)
{
	Proc *up = externup();
	int i;
	uintptr_t addr;
	Segment *s;
	va_list list;
	va_start(list, ar0);

	/*
	 * void* segbrk(void* saddr, void* addr);
	 */
	addr = PTR2UINT(va_arg(list, void*));
	if(addr == 0){
		for(i = 0; i < NSEG; i++){
			if(up->seg[i] != nil && (up->seg[i]->type&SG_TYPE) == SG_BSS){
				ar0->v = UINT2PTR(up->seg[i]->top);
				return;
			}
		}
		return;
	}
	for(i = 0; i < NSEG; i++) {
		s = up->seg[i];
		if(s == nil)
			continue;
		/* Ok to extend an empty segment */
		if(addr < s->base || addr > s->top)
			continue;
		if(addr == s->top && (s->base < s->top))
			continue;
		switch(s->type&SG_TYPE) {
		case SG_LOAD:
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
	va_end(list);
	error(Ebadarg);
}

void
sysbrk_(Ar0* ar0, ...)
{
	Proc *up = externup();
	uintptr_t addr;
	va_list list;
	va_start(list, ar0);
	int i;

	/*
	 * int brk(void*);
	 *
	 * Deprecated; should be for backwards compatibility only.
	 */
	addr = PTR2UINT(va_arg(list, void*));
	va_end(list);

	for(i = 0; i < NSEG; i++){
		if(up->seg[i] != nil && (up->seg[i]->type&SG_TYPE) == SG_BSS){
			ibrk(addr, i);
			ar0->i = 0;
			return;
		}
	}
	ar0->i = -1;
}

static uintptr_t
segattach(Proc* p, int attr, char* name, uintptr_t va, usize len)
{
	Proc *up = externup();
	int sno;
	Segment *s, *os;
	Physseg *ps;

	/* BUG: Only ok for now */
	if((va != 0 && va < UTZERO) || (va & KZERO) == KZERO)
		error("virtual address in kernel");

	vmemchr(name, 0, ~0);

	qlock(&p->seglock);
	if(waserror()){
		qunlock(&p->seglock);
		nexterror();
	}
	for(sno = 0; sno < NSEG; sno++)
		if(p->seg[sno] == nil)
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
			if(p == up && up->prepagemem)
				nixprepage(sno);
			va = s->base;
			goto clean_out;
		}
	}

	for(ps = physseg; ps->name != nil; ps++)
		if(strcmp(name, ps->name) == 0)
			break;
	if(ps->name == nil)
		error("segment not found");

	if(va == 0 && ps->gva != 0){
		va = ps->gva;
		if(len == 0)
			len = ps->size*BIGPGSZ;
	}

	if(len == 0)
		error("zero length");

	len = BIGPGROUND(len);
	if(len == 0)
		error("length overflow");

	/*
	 * Find a hole in the address space.
	 * Starting at the highest possible stack address - len,
	 * check for an overlapping segment, and repeat at the
	 * base of that segment - len until either a hole is found
	 * or the address space is exhausted.
	 */
	if(va == 0) {
		va = USTKTOP-len;
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

	va = va&~(BIGPGSZ-1);
	if(isoverlap(p, va, len) != nil)
		error(Esoverlap);

	if((len/BIGPGSZ) > ps->size)
		error("len > segment size");

	attr &= ~SG_TYPE;		/* Turn off what is not allowed */
	attr |= ps->attr;		/* Copy in defaults */

	s = newseg(attr, va, len/BIGPGSZ);
	s->pseg = ps;
	p->seg[sno] = s;

	if(p == up && up->prepagemem)
		nixprepage(sno);

clean_out:
	poperror();
	qunlock(&p->seglock);
	return va;
}

void
syssegattach(Ar0* ar0, ...)
{
	Proc *up = externup();
	int attr;
	char *name;
	uintptr_t va;
	usize len;
	va_list list;
	va_start(list, ar0);

	/*
	 * long segattach(int, char*, void*, uint32_t);
	 * should be
	 * void* segattach(int, char*, void*, usize);
	 */
	attr = va_arg(list, int);
	name = va_arg(list, char*);
	va = PTR2UINT(va_arg(list, void*));
	len = va_arg(list, usize);
	va_end(list);

	ar0->v = UINT2PTR(segattach(up, attr, validaddr(name, 1, 0), va, len));
}

void
syssegdetach(Ar0* ar0, ...)
{
	Proc *up = externup();
	int i;
	uintptr_t addr;
	Segment *s;
	va_list list;
	va_start(list, ar0);

	/*
	 * int segdetach(void*);
	 */
	addr = PTR2UINT(va_arg(list, void*));
	va_end(list);

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
	 * there. So let's not detach any stacks.
	 * TODO(aki): does it really?
	 */
	if((s->type & SG_TYPE) == SG_STACK){
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
syssegfree(Ar0* ar0, ...)
{
	Proc *up = externup();
	Segment *s;
	uintptr_t from, to;
	usize len;
	va_list list;
	va_start(list, ar0);

	/*
	 * int segfree(void*, uint32_t);
	 * should be
	 * int segfree(void*, usize);
	 */
	from = PTR2UINT(va_arg(list, void*));
	s = seg(up, from, 1);
	if(s == nil)
		error(Ebadarg);
	len = va_arg(list, usize);
	to = (from + len) & ~(BIGPGSZ-1);
	if(to < from || to > s->top){
		qunlock(&s->lk);
		error(Ebadarg);
	}
	from = BIGPGROUND(from);
	va_end(list);

	mfreeseg(s, from, (to - from) / BIGPGSZ);
	qunlock(&s->lk);
	mmuflush();

	ar0->i = 0;
}

static void
pteflush(Pte *pte, int s, int e)
{
	int i;
	Page *p;

	for(i = s; i < e; i++) {
		p = pte->pages[i];
		if(pagedout(p) == 0)
			memset(p->cachectl, PG_TXTFLUSH, sizeof(p->cachectl));
	}
}

void
syssegflush(Ar0* ar0, ...)
{
	Proc *up = externup();
	Segment *s;
	uintptr_t addr;
	Pte *pte;
	usize chunk, l, len, pe, ps;
	va_list list;
	va_start(list, ar0);

	/*
	 * int segflush(void*, uint32_t);
	 * should be
	 * int segflush(void*, usize);
	 */
	addr = PTR2UINT(va_arg(list, void*));
	len = va_arg(list, usize);
	va_end(list);

	while(len > 0) {
		s = seg(up, addr, 1);
		if(s == nil)
			error(Ebadarg);

		s->flushme = 1;
	more:
		l = len;
		if(addr+l > s->top)
			l = s->top - addr;

		ps = addr-s->base;
		pte = s->map[ps/PTEMAPMEM];
		ps &= PTEMAPMEM-1;
		pe = PTEMAPMEM;
		if(pe-ps > l){
			pe = ps + l;
			pe = (pe+BIGPGSZ-1)&~(BIGPGSZ-1);
		}
		if(pe == ps) {
			qunlock(&s->lk);
			error(Ebadarg);
		}

		if(pte)
			pteflush(pte, ps/BIGPGSZ, pe/BIGPGSZ);

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
