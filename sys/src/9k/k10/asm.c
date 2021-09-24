/*
 * Address Space Maps.  early physical memory allocation & mapping
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

static Lock asmlock;
static Asm asmarray[64] = {
	{ 0, ~0ull, AsmNONE, nil, },
};
static int asmindex = 1;
Asm* asmlist = &asmarray[0];
static Asm* asmfreelist;
static char* asmtypes[] = {
	[AsmNONE]		"none",
	[AsmMEMORY]		"mem",
	[AsmRESERVED]		"res",
	[AsmACPIRECLAIM]	"acpirecl",
	[AsmACPINVS]		"acpinvs",
	[AsmDEV]		"dev",
};

void
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
		if(asmindex >= nelem(asmarray)){
			print("asmnew: asmarray full, discarding %#p\n", addr);
			return nil;
		}
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
		DBG("asmfree: overlap %#P@%#P, type %d\n", size, addr, type);
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
	asmfree(addr, size, AsmNONE);
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
	uintptr endaddr;

	DBG("asmmapinit %#P %#P %s\n", addr, size, asmtypes[type]);

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
		endaddr = addr + size;
		if(endaddr > sys->pmend)
			sys->pmend = endaddr;
		break;
	}
}

void
asmmodinit(u32int start, u32int end, char* s)
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

static PTE
asmwalkalloc(uintptr size)
{
	uintmem pa;

	assert(size == PTSZ && sys->vmunused+size <= sys->vmunmapped);

	if((pa = mmuphysaddr(sys->vmunused)) != ~0ull)
		sys->vmunused += size;

	return pa;
}

#include "amd64.h"

static int npg[NPGSZ];

static void
kseg2map(Asm *asm)
{
	PTE *pte;
	int i, l;
	uintmem hi, mem, nextmem;
	uintptr va;

	va = KSEG2 + asm->addr;			/* approx. KADDR(asm->addr) */
	DBG("kseg2 %#P %#P %s (%P) va %#p\n",
		asm->addr, asm->addr+asm->size,
		asmtypes[asm->type], asm->size, va);

	/* Convert a range into pages */
	hi = asm->addr + asm->size;
	for(mem = asm->addr; mem < hi; mem = nextmem){
		nextmem = (mem + PGLSZ(0)) & ~m->pgszmask[0];

		/* Try large pages first */
		for(i = m->npgsz - 1; i >= 0; i--){
			if((mem & m->pgszmask[i]) != 0 || mem + PGLSZ(i) > hi)
				continue;
			/*
			 * This page fits entirely within the range,
			 * mark it as usable.
			 */
			if((l = mmuwalk(va, i, &pte, asmwalkalloc)) < 0)
				panic("meminit 3: kseg2map");

			*pte = mem|PteRW|PteP;
			if(l > 0)
				*pte |= PtePS;

			nextmem = mem + PGLSZ(i);
			va += PGLSZ(i);
			npg[i]++;
			break;
		}
	}

	asm->base = ROUNDUP(asm->addr, PGSZ);
	asm->limit = ROUNDDN(hi, PGSZ);
	asm->kbase = (uintptr)KADDR(asm->base);
}

/*
 * return space needed for Page array, plus the number of pages it can
 * accomodate (via *npp) and kernel allocation space top (via *ksizep).
 */
static uintmem
pagesarraysize(uintmem *npp, uintmem *ksizep)
{
	Asm *asm;
	uintmem np, ksize;

	np = ksize = 0;
	for(asm = asmlist; asm != nil; asm = asm->next){
		if(asm->limit == asm->base || asm->type != AsmMEMORY)
			continue;
		/*
		 * kernel memory is contiguous from 1 MiB to the first gap;
		 * there can be unmapped gaps, derived from the PC's e820
		 * map, for example, ACPI memory.
		 */
		if (ksize == 0 && asm->base >= MiB && asm->base < 4ull*GiB &&
		    asm->limit <= 4ull*GiB)
			ksize = asm->limit;

		np += (asm->limit - asm->base)/PGSZ;
	}
	*ksizep = ksize;
	DBG("kernel stops at %#P %,llud\n", ksize, ksize);

	/*
	 * leave room in kernel for things other than Pages.
	 * if the machine has > ~8GiB, put Pages above 4GiB, rather than
	 * reducing np.
	 */
	if (ksize && np*sizeof(Page) > ksize*3/4 && sys->pmend < 7ull*GiB) {
		ksize = (ksize/PGSZ)*3/4;
		print("reduced np %,llud to %,llud to fit bottom 4GiB\n",
			np, ksize);
		np = ksize;
	}
	*npp = np;
	return ROUNDUP(np * sizeof(Page), PGSZ);
}

static void
dbgprpalloc(void)
{
	Asm* asm;
	int j;
	Pallocmem *pm;

	pm = palloc.mem;
	j = 0;
	for(asm = asmlist; asm != nil; asm = asm->next){
		if(asm->limit == asm->base || pm >= palloc.mem+nelem(palloc.mem))
			continue;
		DBG("asm pm%d: base %#P limit %#P npage %llud\n",
			j, pm->base, pm->limit, (pm->limit - pm->base)/PGSZ);
		pm++;
		j++;
	}
}

/*
 * create the kernel page tables and populate palloc.mem[].
 * Called from main(), this code should only be run
 * by the bootstrap processor.
 */
void
meminit(void)
{
	Asm* asm;
	PTE *pte;
	int l, first;
	uintmem mem, pa, pgmem, np, ksize, vmgap;
	Pallocmem *pm;

	/*
	 * do we need a map, like vmap, for best use of mapping kmem?
	 * - in fact, a rewritten pdmap could do the job, no?
	 * have to assume up to vmend is contiguous memory.
	 * can't mmuphysaddr(sys->vmunmapped) because...
	 *
	 * Assume already 2MiB aligned; currently this is done in mmuinit.
	 */
	assert(m->pgszlg2[1] == 21);
	assert(!((sys->vmunmapped|sys->vmend) & m->pgszmask[1]));

	/* adjust sys->vm things and asmalloc to compensate for map to kzero */
	if((pa = mmuphysaddr(sys->vmunused)) == ~0ull)
		panic("meminit: mmuphysaddr(%#p)", sys->vmunused);
	pa += sys->vmunmapped - sys->vmunused;

	first = 1;
	/* try with existing kernmem */
	vmgap = sys->vmend - sys->vmunused;	/* kernel's data after end */
	/*
	 * if asmalloc fails, it's probably due to reserved memory holes in the
	 * memory map below 2GiB, which is not playing fair.  reduce kernmem
	 * and try again.
	 */
	while ((mem = asmalloc(pa, vmgap, AsmMEMORY, 0)) != pa &&
	    kernmem >= 300*MiB) {
		if (first) {
			print("meminit: can't allocate address space of %#p "
				"bytes of memory at %#p\n"
				"\tfor currently-unmapped vm; reducing kernmem.\n",
				vmgap, pa);
			print("\tif your bios has a Max TOLUD setting, set "
				"it as high as possible.\n");
			first = 0;
		}
		kernmem -= 50*MiB;
		/* fix up vmend */
		sys->vmend = ROUNDUP(sys->vmstart + kernmem, PGLSZ(1));
		vmgap = sys->vmend - sys->vmunused; /* kernel's data after end */
	}
	if(mem != pa)
		panic("meminit: can't allocate address space of %#p bytes "
			"of memory at %#p for currently-unmapped vm", vmgap, pa);
	print("kernel alloc space = %,lld\n", kernmem);

	DBG("meminit: mem %#P\n", mem);

	/* map between vmunmapped and vmend to kzero */
	for(; sys->vmunmapped < sys->vmend; sys->vmunmapped += 2*MiB){
		l = mmuwalk(sys->vmunmapped, 1, &pte, asmwalkalloc);
		DBG("meminit: %#p l %d\n", sys->vmunmapped, l);
		USED(l);
		*pte = pa|PtePS|PteRW|PteP;
		pa += 2*MiB;
	}

	/* run through asmlist and map KSEG2 to ram. */
	for(asm = asmlist; asm != nil; asm = asm->next)
		if(asm->type == AsmMEMORY)
			kseg2map(asm);

	ialloclimit((sys->vmend - sys->vmstart)/2);	/* close enough */

	/* populate palloc.mem array from asmlist */
	pm = palloc.mem;
	for(asm = asmlist; asm != nil; asm = asm->next){
		if(asm->limit == asm->base)
			continue;
		if(pm >= palloc.mem+nelem(palloc.mem)){
			print("meminit: losing %#P pages\n",
				(asm->limit - asm->base)/PGSZ);
			continue;
		}
		pm->base = asm->base;
		pm->limit = asm->limit;
		pm++;
	}

	/* allocate Pages array from palloc.mem directly, if possible */
	pgmem = pagesarraysize(&np, &ksize);	/* reads asmlist */
	DBG("alloc(%,llud) for %,llud Pages\n", pgmem, np);
	mallocinit();
	allocpages(ksize, pgmem, 0);	/* may alter palloc.mem */

	dbgprpalloc();
}
