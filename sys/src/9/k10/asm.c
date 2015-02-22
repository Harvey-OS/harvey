/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * To do:
 *	find a purpose for this...
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"

/*
 * Address Space Map.
 * Low duty cycle.
 */
typedef struct Asm Asm;
typedef struct Asm {
	uintmem	addr;
	uintmem	size;
	int	type;
	int	location;
	Asm*	next;
} Asm;

enum {
	AsmNONE		= 0,
	AsmMEMORY	= 1,
	AsmRESERVED	= 2,
	AsmACPIRECLAIM	= 3,
	AsmACPINVS	= 4,

	AsmDEV		= 5,
};

static Lock asmlock;
static Asm asmarray[64] = {
	{ 0, ~0, AsmNONE, nil, },
};
static int asmindex = 1;
static Asm* asmlist = &asmarray[0];
static Asm* asmfreelist;

/*static*/ void
asmdump(void)
{
	Asm* asm;

	print("asm: index %d:\n", asmindex);
	for(asm = asmlist; asm != nil; asm = asm->next){
		print(" %#P %#P %d (%P)\n",
			asm->addr, asm->addr+asm->size,
			asm->type, asm->size);
	}
}

static Asm*
asmnew(uintmem addr, uintmem size, int type)
{
	Asm * asm;

	if(asmfreelist != nil){
		asm = asmfreelist;
		asmfreelist = asm->next;
		asm->next = nil;
	}
	else{
		if(asmindex >= nelem(asmarray))
			return nil;
		asm = &asmarray[asmindex++];
	}
	asm->addr = addr;
	asm->size = size;
	asm->type = type;

	return asm;
}

int
asmfree(uintmem addr, uintmem size, int type)
{
	Asm *np, *pp, **ppp;

	DBG("asmfree: %#P@%#P, type %d\n", size, addr, type);
	if(size == 0)
		return 0;

	lock(&asmlock);

	/*
	 * Find either a map entry with an address greater
	 * than that being returned, or the end of the map.
	 */
	pp = nil;
	ppp = &asmlist;
	for(np = *ppp; np != nil && np->addr <= addr; np = np->next){
		pp = np;
		ppp = &np->next;
	}

	if((pp != nil && pp->addr+pp->size > addr)
	|| (np != nil && addr+size > np->addr)){
		unlock(&asmlock);
		DBG("asmfree: overlap %#Px@%#P, type %d\n", size, addr, type);
		return -1;
	}

	if(pp != nil && pp->type == type && pp->addr+pp->size == addr){
		pp->size += size;
		if(np != nil && np->type == type && addr+size == np->addr){
			pp->size += np->size;
			pp->next = np->next;

			np->next = asmfreelist;
			asmfreelist = np;
		}

		unlock(&asmlock);
		return 0;
	}

	if(np != nil && np->type == type && addr+size == np->addr){
		np->addr -= size;
		np->size += size;

		unlock(&asmlock);
		return 0;
	}

	if((pp = asmnew(addr, size, type)) == nil){
		unlock(&asmlock);
		DBG("asmfree: losing %#P@%#P, type %d\n", size, addr, type);
		return -1;
	}
	*ppp = pp;
	pp->next = np;

	unlock(&asmlock);

	return 0;
}

uintmem
asmalloc(uintmem addr, uintmem size, int type, int align)
{
	uintmem a, o;
	Asm *asm, *pp;

	DBG("asmalloc: %#P@%#P, type %d\n", size, addr, type);
	lock(&asmlock);
	for(pp = nil, asm = asmlist; asm != nil; pp = asm, asm = asm->next){
		if(asm->type != type)
			continue;
		a = asm->addr;

		if(addr != 0){
			/*
			 * A specific address range has been given:
			 *   if the current map entry is greater then
			 *   the address is not in the map;
			 *   if the current map entry does not overlap
			 *   the beginning of the requested range then
			 *   continue on to the next map entry;
			 *   if the current map entry does not entirely
			 *   contain the requested range then the range
			 *   is not in the map.
			 * The comparisons are strange to prevent
			 * overflow.
			 */
			if(a > addr)
				break;
			if(asm->size < addr - a)
				continue;
			if(addr - a > asm->size - size)
				break;
			a = addr;
		}

		if(align > 0)
			a = ((a+align-1)/align)*align;
		if(asm->addr+asm->size-a < size)
			continue;

		o = asm->addr;
		asm->addr = a+size;
		asm->size -= a-o+size;
		if(asm->size == 0){
			if(pp != nil)
				pp->next = asm->next;
			asm->next = asmfreelist;
			asmfreelist = asm;
		}

		unlock(&asmlock);
		if(o != a)
			asmfree(o, a-o, type);
		return a;
	}
	unlock(&asmlock);

	return 0;
}

static void
asminsert(uintmem addr, uintmem size, int type)
{
	if(type == AsmNONE || asmalloc(addr, size, AsmNONE, 0) == 0)
		return;
	if(asmfree(addr, size, type) == 0)
		return;
	asmfree(addr, size, 0);
}

void
asminit(void)
{
	sys->pmstart = ROUNDUP(PADDR(end), PGSZ);
	sys->pmend = sys->pmstart;
	asmalloc(0, sys->pmstart, AsmNONE, 0);
}

/*
 * Notes:
 * asmmapinit and asmmodinit called from multiboot;
 * subject to change; the numerology here is probably suspect.
 * Multiboot defines the alignment of modules as 4096.
 */
void
asmmapinit(uintmem addr, uintmem size, int type)
{
	switch(type){
	default:
		asminsert(addr, size, type);
		break;
	case AsmMEMORY:
		/*
		 * Adjust things for the peculiarities of this
		 * architecture.
		 * Sys->pmend is the largest physical memory address found,
		 * there may be gaps between it and sys->pmstart, the range
		 * and how much of it is occupied, might need to be known
		 * for setting up allocators later.
		 */
		if(addr < 1*MiB || addr+size < sys->pmstart)
			break;
		if(addr < sys->pmstart){
			size -= sys->pmstart - addr;
			addr = sys->pmstart;
		}
		asminsert(addr, size, type);
		sys->pmoccupied += size;
		if(addr+size > sys->pmend)
			sys->pmend = addr+size;
		break;
	}
}

void
asmmodinit(uint32_t start, uint32_t end, char* s)
{
	DBG("asmmodinit: %#ux -> %#ux: <%s> %#ux\n",
		start, end, s, ROUNDUP(end, 4096));

	if(start < sys->pmstart)
		return;
	end = ROUNDUP(end, 4096);
	if(end > sys->pmstart){
		asmalloc(sys->pmstart, end-sys->pmstart, AsmNONE, 0);
		sys->pmstart = end;
	}
}

static int npg[4];

void*
asmbootalloc(usize size)
{
	uintptr_t va;

	assert(sys->vmunused+size <= sys->vmunmapped);
	va = sys->vmunused;
	sys->vmunused += size;
	memset(UINT2PTR(va), 0, size);
	return UINT2PTR(va);
}

static PTE
asmwalkalloc(usize size)
{
	uintmem pa;

	assert(size == PTSZ && sys->vmunused+size <= sys->vmunmapped);

	if(!ALIGNED(sys->vmunused, PTSZ)){
		DBG("asmwalkalloc: %ulld wasted\n",
			ROUNDUP(sys->vmunused, PTSZ) - sys->vmunused);
		sys->vmunused = ROUNDUP(sys->vmunused, PTSZ);
	}
	if((pa = mmuphysaddr(sys->vmunused)) != ~0)
		sys->vmunused += size;

	return pa;
}

// still needed so iallocb gets initialised correctly. needs to go.
#define ConfCrap

void
asmmeminit(void)
{
	int i, l;
	Asm* asm;
	PTE *pte, *pml4;
	uintptr va;
	uintmem hi, lo, mem, nextmem, pa;
#ifdef ConfCrap
	int cx;
#endif /* ConfCrap */

	assert(!((sys->vmunmapped|sys->vmend) & m->pgszmask[1]));

	if((pa = mmuphysaddr(sys->vmunused)) == ~0)
		panic("asmmeminit 1");
	pa += sys->vmunmapped - sys->vmunused;
	mem = asmalloc(pa, sys->vmend - sys->vmunmapped, 1, 0);
	if(mem != pa)
		panic("asmmeminit 2");
	DBG("pa %#llux mem %#llux\n", pa, mem);

	/* assume already 2MiB aligned*/
	assert(ALIGNED(sys->vmunmapped, 2*MiB));
	pml4 = UINT2PTR(m->pml4->va);
	while(sys->vmunmapped < sys->vmend){
		l = mmuwalk(pml4, sys->vmunmapped, 1, &pte, asmwalkalloc);
		DBG("%#p l %d\n", sys->vmunmapped, l);
		*pte = pa|PtePS|PteRW|PteP;
		sys->vmunmapped += 2*MiB;
		pa += 2*MiB;
	}

#ifdef ConfCrap
	cx = 0;
#endif /* ConfCrap */
	for(asm = asmlist; asm != nil; asm = asm->next){
		if(asm->type != AsmMEMORY)
			continue;
		va = KSEG2+asm->addr;
		print("asm: addr %#P end %#P type %d size %P\n",
			asm->addr, asm->addr+asm->size,
			asm->type, asm->size);

		lo = asm->addr;
		hi = asm->addr+asm->size;
		/* Convert a range into pages */
		for(mem = lo; mem < hi; mem = nextmem){
			nextmem = (mem + PGLSZ(0)) & ~m->pgszmask[0];

			/* Try large pages first */
			for(i = m->npgsz - 1; i >= 0; i--){
				if((mem & m->pgszmask[i]) != 0)
					continue;
				if(mem + PGLSZ(i) > hi)
					continue;
				/* This page fits entirely within the range. */
				/* Mark it a usable */
				if((l = mmuwalk(pml4, va, i, &pte, asmwalkalloc)) < 0)
					panic("asmmeminit 3");

				*pte = mem|PteRW|PteP;
				if(l > 0)
					*pte |= PtePS;

				nextmem = mem + PGLSZ(i);
				va += PGLSZ(i);
				npg[i]++;

				break;
			}
		}

#ifdef ConfCrap
		/*
		 * Fill in conf crap.
		 */
		if(cx >= nelem(conf.mem))
			continue;
		lo = ROUNDUP(asm->addr, PGSZ);
//if(lo >= 600ull*MiB)
//    continue;
		conf.mem[cx].base = lo;
		hi = ROUNDDN(hi, PGSZ);
//if(hi > 600ull*MiB)
//  hi = 600*MiB;
		conf.mem[cx].npage = (hi - lo)/PGSZ;
		conf.npage += conf.mem[cx].npage;
		print("cm %d: addr %#llux npage %lud\n",
			cx, conf.mem[cx].base, conf.mem[cx].npage);
		cx++;
#endif /* ConfCrap */
	}
	print("%d %d %d\n", npg[0], npg[1], npg[2]);

#ifdef ConfCrap
	/*
	 * Fill in more conf crap.
	 * This is why I hate Plan 9.
	 */
	conf.upages = conf.npage;
	i = (sys->vmend - sys->vmstart)/PGSZ;		/* close enough */
	conf.ialloc = (i/2)*PGSZ;
	print("npage %llud upage %lud kpage %d\n",
		conf.npage, conf.upages, i);

#endif /* ConfCrap */
}

void
asmumeminit(void)
{
	Asm *asm;
	extern void physallocdump(void);

	for(asm = asmlist; asm != nil; asm = asm->next){
		if(asm->type != AsmMEMORY)
			continue;
		physinit(asm->addr, asm->size);
	}
	physallocdump();
}
