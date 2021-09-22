/*
 * early physical memory allocation & mapping
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "riscv.h"

//#undef DBG
//#define DBG(...) print(__VA_ARGS__)

static Lock asmlock;
static Asm asmarray[64] = {
	{ 0, ~(uintptr)0, AsmNONE, nil, },
};
static int asmindex = 1;
Asm* asmlist = &asmarray[0];
static Asm* asmfreelist;
static char* asmtypes[] = {
	[AsmNONE]		"none",
	[AsmMEMORY]		"mem",
	[AsmRESERVED]		"rsrvd",
	[AsmDEV]		"device",
};

void
asmdump(void)
{
	Asm* asm;

	print("asm: index %d:\n", asmindex);
	for(asm = asmlist; asm != nil; asm = asm->next)
		print(" %#P %#P %d (%P)\n",
			asm->addr, asm->addr+asm->size, asm->type, asm->size);
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
		if(asmindex >= nelem(asmarray)) {
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

	DBG("asmfree:  %#P @ %#P, type %d\n", size, addr, type);
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
		DBG("asmfree: overlap %#P @ %#P, type %d\n", size, addr, type);
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

		DBG("asmfree: asmfreelist %#P @ %#P, type %d\n", size, addr, type);
		unlock(&asmlock);
		return 0;
	}

	if(np != nil && np->type == type && addr+size == np->addr){
		np->addr -= size;
		np->size += size;

		DBG("asmfree: insert 1 %#P @ %#P, type %d\n", size, addr, type);
		unlock(&asmlock);
		return 0;
	}

	if((pp = asmnew(addr, size, type)) == nil){
		unlock(&asmlock);
		DBG("asmfree: losing %#P @ %#P, type %d\n", size, addr, type);
		return -1;
	}
	*ppp = pp;
	pp->next = np;

	DBG("asmfree: insert 2 %#P @ %#P, type %d\n", size, addr, type);
	unlock(&asmlock);
	return 0;
}

uintmem
asmalloc(uintmem addr, uintmem size, int type, uintmem align)
{
	uintmem a, o;
	Asm *asm, *pp;

	DBG("asmalloc: %#P @ %#P, type %d\n", size, addr, type);
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
		if(asm->addr + asm->size - a < size)
			continue;

		o = asm->addr;
		asm->addr = a + size;
		asm->size -= a - o + size;
		if(asm->size == 0){
			if(pp != nil)
				pp->next = asm->next;
			asm->next = asmfreelist;
			asmfreelist = asm;
		}

		unlock(&asmlock);
		if(o != a)
			asmfree(o, a - o, type);
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
	sys->pmend = sys->pmstart;	/* start here, increment below */
	/* carve out kernel AS */
	asmalloc(sys->pmbase, sys->pmstart - sys->pmbase, AsmNONE, 0);
}

/*
 * Notes:
 * asmmapinit and asmmodinit were called from multiboot on x86;
 * subject to change; the numerology here is probably suspect.
 * Multiboot defines the alignment of modules as 4096.
 */
void
asmmapinit(u64int addr, u64int size, int type)
{
	uintptr endaddr;

	DBG("asmmapinit %#P %#P %s\n", (uintmem)addr, (uintmem)size,
		(uint)type < nelem(asmtypes)? asmtypes[type]: "gok");

	switch(type){
	default:
		asminsert(addr, size, type);
		break;
	case AsmMEMORY:
		/*
		 * Adjust things for the peculiarities of this architecture.
		 * Sys->pmend is the largest physical memory address found,
		 * there may be gaps between it and sys->pmstart, the range
		 * and how much of it is occupied, might need to be known
		 * for setting up allocators later.
		 */
		if(addr < sys->pmbase) {
			print("asmmapinit: addr %#p below ram\n", (uintptr)addr);
			break;
		}
		if(addr+size < sys->pmstart)
			break;
		if(addr < sys->pmstart){
			size -= sys->pmstart - addr;
			addr = sys->pmstart;
		}

		endaddr = addr + size;
		asminsert(addr, size, type);
		if(endaddr > sys->pmend)
			sys->pmend = endaddr;		/* extend end */
		sys->pmoccupied += size;
		if (sys->pmoccupied > sys->pmend)	/* sanity cap */
			sys->pmoccupied = sys->pmend;
		break;
	}
}

#ifdef unneeded		/* we have no actual dynamically-loaded modules */
void
asmmodinit(u32int start, u32int end, char* s)
{
	DBG("asmmodinit: %#ux -> %#ux: <%s> %#ux\n",
		start, end, s, ROUNDUP(end, 4096));

	if(start < sys->pmstart)
		return;
	end = ROUNDUP(end, 4096);
	if(end > sys->pmstart){
		asmalloc(sys->pmstart, end - sys->pmstart, AsmNONE, 0);
		sys->pmstart = end;
	}
}
#endif

PTE
asmwalkalloc(uintptr size)
{
	uintmem pa;

	assert(sys->vmunused+size <= sys->vmunmapped);

	if((pa = mmuphysaddr(sys->vmunused)) != ~(uintptr)0) {
		memset((void *)sys->vmunused, 0, size);
		sys->vmunused += size;
	}
	return pa;
}

static int npg[NPGSZ];
int dokseg2map;		/* config file option; off by default on riscv */

static int
walknewpte(uintptr va, uintmem mem, int lvl)
{
	int l;
	PTE ptebits;
	PTE *pte;

	/* NB: already covered by identity map on pf */
	pte = 0;
	if((l = mmuwalk(va, lvl, &pte, asmwalkalloc)) < 0)
		panic("meminit 3: kseg2map");
	if (PAGEPRESENT(*pte)) {
		DBG("meminit: pte @ %#p maps %#p -> %#p\n",
			pte, va, (uintptr)PADDRINPTE(*pte));
		return l;
	}

	/* *pte is unpopulated, so set it */
	ptebits = PTEPPN(mem) | va2gbit(va) | PteLeaf | Pteleafvalid;
	if(l > 0)
		ptebits |= PtePS;	/* super page leaf */
	DBG("writing pte %#p with %#p for %#p -> %#p\n", pte, ptebits, va, mem);
	mmuwrpte(pte, ptebits);
	return l;
}

/*
 * on riscv, don't actually change the mappings; physical memory is
 * identity-mapped.
 */
static void
kseg2map(Asm *asm)
{
	int i, l;
	uintmem hi, mem, nextmem;
	uintptr va;

//	va = KSEG2 + asm->addr - sys->pmbase;	/* approx. KADDR(asm->addr) */
	va = ROUNDUP(asm->addr, PGSZ);		/* skip to next page */

	DBG("kseg2 pa %#P end %#P %s (%P) start at va %#p\n",
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
			if (dokseg2map) {
				l = walknewpte(va, mem, i);
				USED(l);
			}
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
		if(asm->limit == asm->base || asm->type != AsmMEMORY ||
		    asm->limit < asm->base)
			continue;
		/*
		 * kernel memory is contiguous;
		 * there can be unmapped gaps, derived from the PC's e820
		 * map, for example, ACPI memory.
		 */
		if (ksize == 0 && asm->base >= sys->pmbase)
			ksize = asm->limit;

		np += (asm->limit - asm->base)/PGSZ;
	}
	if (ksize == 0) {
		ksize = sys->pmbase + kernmem;
		np = memtotal / PGSZ;
	}
	*ksizep = ksize;
	DBG("kernel stops at %#P %,llud\n", ksize, (uvlong)ksize);

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
			j, pm->base, pm->limit, (uvlong)(pm->limit - pm->base)/PGSZ);
		pm++;
		j++;
	}
}

/*
 * create the kernel page tables and populate palloc.mem[]
 * from asmlist.
 * Called from main(), this code should only be run
 * by the bootstrap processor.
 *
 * on riscv, we have no e820 memory map, memory just runs
 * from sys->pmbase to sys->pmbase + membanks[0].size
 * and then there may be other discontiguous banks of memory
 * allocated to user processes.
 */
void
meminit(void)
{
	Asm* asm;
	PTE *pte;
	int l;
	uintmem mem, pa, pgmem, np, ksize, vmgap;
	Pallocmem *pm;

	/*
	 * do we need a map, like vmap, for best use of mapping kmem?
	 * - in fact, a rewritten pdmap could do the job, no?
	 * have to assume up to vmend is contiguous memory.
	 * can't mmuphysaddr(sys->vmunmapped) because...
	 *
	 * Assume already PGLSZ(1) aligned; currently this is done in mmuinit.
	 */
	assert(m->pgszlg2[1] == PTSHFT+PGSHFT);
	if (sys->vmunmapped & m->pgszmask[1])
		panic("sys->vmunmapped %#p not aligned as level 1 page",
			sys->vmunmapped);
	if (sys->vmend & m->pgszmask[1])
		panic("sys->vmend %#p not aligned as level 1 page", sys->vmend);

	/* adjust sys->vm things and asmalloc to compensate for map to kzero */
	if((pa = mmuphysaddr(sys->vmunused)) == ~(uintptr)0)
		panic("meminit: mmuphysaddr(%#p)", sys->vmunused);
	USED(pa);
	pa = sys->vmunmapped - KZERO;
	vmgap = sys->vmend - sys->vmunmapped; /* kernel's data after end */

	/*
	 * we asked for exactly the size of the gap and it failed on icicle.
	 * try to make progress anyway by requesting slightly less.
	 */
	mem = asmalloc(pa, vmgap - PGSZ, AsmMEMORY, 0);
	if(mem == pa)
		sys->vmend -= PGSZ;
	else
		panic("meminit: can't allocate address space of %,llud bytes "
			"at %#p for currently-unmapped vm", vmgap - PGSZ, pa);
	DBG("meminit: mem %#P\n", mem);

	DBG("map kzero...");
	/* map between vmunmapped and vmend to kzero space */
	for(; sys->vmunmapped < sys->vmend; sys->vmunmapped += PGLSZ(1)){
		pte = 0;
		l = mmuwalk(sys->vmunmapped, 1, &pte, asmwalkalloc);
		if (pte == 0)
			DBG("meminit: %#p l %d &pte %#p pte %#p\n",
				sys->vmunmapped, l, pte, *pte);
		USED(l);
		/* super page leaf */
		/* NB: covered by identity map on riscv; causes infinite traps */
//		mmuwrpte(pte, PTEPPN(pa)|PteLeaf|Pteleafvalid|PtePS|PteG);
		pa += PGLSZ(1);
	}

	DBG("map kseg2 to extra ram\n");
	/* run through asmlist and map KSEG2 to ram */
	for(asm = asmlist; asm != nil; asm = asm->next)
		if(asm->type == AsmMEMORY)
			kseg2map(asm);

	ialloclimit((sys->vmend - sys->vmstart)/2);	/* close enough */

	DBG("populate palloc.mem\n");
	/* populate palloc.mem array from asmlist */
	pm = palloc.mem;
	for(asm = asmlist; asm != nil; asm = asm->next){
		if(asm->limit == asm->base)
			continue;
		if(pm >= palloc.mem+nelem(palloc.mem)){
			print("meminit: losing %#P pages; palloc.mem too small\n",
				(asm->limit - asm->base)/PGSZ);
			continue;
		}
		DBG("asm base %#p lim %#p for palloc.mem[%lld]\n", asm->base,
		    asm->limit, pm - palloc.mem);
		pm->base = asm->base;
		pm->limit = asm->limit;
		pm++;
	}

	DBG("allocate Page array\n");
	/* allocate Page array from palloc.mem directly, if possible */
	pgmem = pagesarraysize(&np, &ksize);	/* reads asmlist */
	DBG("alloc(%,llud) for %,llud Pages\n", (uvlong)pgmem, (uvlong)np);
	mallocinit();
	DBG("allocpages\n");
	/* may alter palloc.mem */
	allocpages(ksize - sys->pmbase, pgmem, Couldmalloc);

	dbgprpalloc();
}
