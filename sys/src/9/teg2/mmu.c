/*
 * arm arch v7-a mpcore mmu, specifically for cortex-a9
 *
 * We set ttbr0 such that page tables can be looked-up in l1 cache by the mmu,
 * thus we don't need to flush them to memory (but we do have to be careful
 * when updating the live page tables).  Beware that when an MMU is enabled,
 * at least on the cortex-a9, the MMU will prefetch ravenously from its page
 * tables, even if all other prefetching has been disabled, so it's vital to
 * invalidate TLB entries corresponding to a modified PTE before the MMU tries
 * to use it.  See setpte().
 *
 * erratum 782773: updating a pte might cause an erroneous translation fault.
 * workaround: writeback and invalidate the pte's cache line before overwriting
 * a pte, with interrupts disabled.  arm think that this is only a concern
 * during start-up and shut-down, when we use double-mapping, but the pages
 * of a running process are doubled-mapped, accessible through the zero and
 * KZERO segments.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

#define L1X(va)		FEXT((va), 20, 12)
#define L2X(va)		FEXT((va), 12, 8)

/*
 * on the trimslice, the top of 1GB ram can't be addressible, as high
 * virtual memory (0xffff0000) contains high vectors.  We moved USTKTOP
 * down another MB to utterly avoid KADDR(USTKTOP) mapping to high
 * exception vectors, particularly before mmuinit sets up L2 page tables
 * for the high vectors.  USTKTOP is thus (0x40000000 - HIVECTGAP), which
 * in kernel virtual space is (0x100000000ull - HIVECTGAP), but we need
 * the whole user virtual address space (1GB) to be unmapped initially
 * in a new process, thus the choice of L1hi.
 */
enum {
	Sectionsz	= MB,
	L1lo		= UZERO/Sectionsz,	/* L1X(UZERO) */
	/* one Section pte past user space */
//	L1hi = (USTKTOP+Sectionsz-1)/Sectionsz, /* L1X(USTKTOP+Sectionsz-1) */
	L1hi		= DRAMSIZE/Sectionsz,
	/* part of an L2 page actually used by us */
	L2used		= (Sectionsz / BY2PG) * sizeof(PTE),

	Debug		= 0,
	/*
	 * Erratum782773 costs about 4% of cpu time.
	 * So far we're okay without it.
	 */
	Erratum782773	= 1,
};

#define ISHOLE(type)	((type) == 0)

typedef struct Range Range;
struct Range {
	uintptr	startva;
	uvlong	endva;
	uintptr	startpa;
	uvlong	endpa;
	ulong	attrs;
	int	type;			/* L1 Section or Coarse? */
};

static uchar *l2pages = KADDR(L2POOLBASE);

static void mmul1empty(void);

static char *
typename(int type)
{
	static char numb[20];

	switch(type) {
	case Coarse:
		return "4KB-page table(s)";
	case Section:
		return "1MB section(s)";
	default:
		snprint(numb, sizeof numb, "type %d", type);
		return numb;
	}
}

static void
prl1range(Range *rp)
{
	int attrs;

	print("l1 maps va (%#8.8lux-%#llux) -> ", rp->startva, rp->endva-1);
	if (rp->startva == rp->startpa)
		print("identity-mapped");
	else
		print("pa %#8.8lux", rp->startpa);
	print(" attrs ");
	attrs = rp->attrs;
	if (attrs) {
		if (attrs & Cached)
			print("C");
		if (attrs & Buffered)
			print("B");
		if (attrs & L1sharable)
			print("S1");
		if (attrs & L1wralloc)
			print("A1");
	} else
		print("\"\"");
	print(" %s\n", typename(rp->type));
	delay(100);
	rp->endva = 0;
}

static void
l2dump(Range *rp, PTE pte)
{
	USED(rp, pte);
}

/* dump level 1 page table at virtual addr l1 */
void
mmudump(PTE *l1)
{
	int i, type, attrs;
	uintptr pa;
	uvlong va;
	PTE pte;
	Range rng;

	/* dump first level of ptes */
	print("cpu%d l1 pt @ %#p:\n", m->machno, PADDR(l1));
	memset(&rng, 0, sizeof rng);
	for (va = i = 0; i < 4096; i++, va += Sectionsz) {
		pte = l1[i];
		type = pte & (Section|Coarse);
		if (type == Section)
			pa = pte & ~(Sectionsz - 1);
		else
			pa = pte & ~(KB - 1);
		attrs = 0;
		if (!ISHOLE(type) && type == Section)
			attrs = pte & L1ptedramattrs;

		/* if a range is open but this pte isn't part, close & open */
		if (!ISHOLE(type) &&
		    (pa != rng.endpa || type != rng.type || attrs != rng.attrs))
			if (rng.endva != 0) {	/* range is open? close it */
				prl1range(&rng);
				rng.type = 0;
				rng.attrs = 0;
			}

		if (ISHOLE(type)) {		/* end of any open range? */
			if (rng.endva != 0)	/* range is open? close it */
				prl1range(&rng);
		} else {			/* continuation or new range */
			if (rng.endva == 0) {	/* no open range? start one */
				rng.startva = va;
				rng.startpa = pa;
				rng.type = type;
				rng.attrs = attrs;
			}
			rng.endva = va + Sectionsz; /* continue the open range */
			rng.endpa = pa + Sectionsz;
		}
		if (type == Coarse)
			l2dump(&rng, pte);
	}
	if (rng.endva != 0)			/* close any open range */
		prl1range(&rng);
	print("\n");
}

/*
 * set ptes with all the errata covered.
 * va corresponds to pte, val will be stored in it.
 * n is the number of ptes to fill and incr the increment for va.
 *
 * we are likely updating the live L1 page table, so keep the PTE
 * invalidation close to the PTE update, and don't let interrupts in between.
 */
static void
setptes(PTE *pte, uintptr va, ulong val, int n, int incr)
{
	int s;

	s = splhi();
	va = PPN(va);	/* revisit if we use more than 1K of an l2 page */
	while (n-- > 0) {
		if (Erratum782773)
			cachedwbinvse(pte, sizeof *pte);
		*pte++ = val;
		mmuinvalidateaddr(va);	/* clear out the current tlb entry */
		va += incr;
	}
	splx(s);
}

static void
setpte(PTE *pte, uintptr va, ulong val)
{
	int s;

	s = splhi();
	if (Erratum782773)
		cachedwbinvse(pte, sizeof *pte);
	*pte = val;
	mmuinvalidateaddr(PPN(va));
	splx(s);
}

void
mmul1copy(void *dest, void *src)
{
	int s;

	s = splhi();
	if (Erratum782773)
		cachedwbinvse(dest, L1SIZE);
	memmove(dest, src, L1SIZE);
	splx(s);
}

/*
 * fill contiguous L1 PTEs with MMU off.
 */
static void
fillptes(PTE *start, PTE *end, ulong val, ulong pa)
{
	PTE *pte;

	for (pte = start; pte < end; pte++) {
		*pte = val | pa;
		pa += Sectionsz;
	}
}

/*
 * map `mbs' megabytes (sections) from virt to phys, uncached & unprefetchable.
 * device registers are sharable, except the private memory region:
 * 2 4K pages, at 0x50040000 on the tegra2.
 */
static void
mmuiomap(PTE *l1, uintptr virt, uintptr phys, int mbs)
{
	uint off;

	phys &= ~(Sectionsz-1);
	virt &= ~(Sectionsz-1);
	for (off = 0; mbs-- > 0; off += Sectionsz)
		setpte(&l1[L1X(virt + off)], virt + off,
			(phys + off) | Dom0 | L1AP(Krw) | Section |
			L1sharable | Noexecsect);
	mmuinvalidate();
}

/* identity map `mbs' megabytes from phys */
void
mmuidmap(PTE *l1, uintptr phys, int mbs)
{
	mmuiomap(l1, phys, phys, mbs);
}

/* crude allocation of l2 pages.  only used during mmu initialization. */
static PTE *
newl2page(void)
{
	PTE *p;

	if ((uintptr)l2pages >= HVECTORS - BY2PG)
		panic("newl2page");
	p = (PTE *)l2pages;
	l2pages += BY2PG;
	return p;
}

/* adjust mmul1lo and mmul1hi to include (new) pte mmul1[x] */
static void
mmul1adj(uint x)
{
	if(x+1 - m->mmul1lo < m->mmul1hi - x)
		m->mmul1lo = x+1;	/* add to text+data ptes */
	else
		m->mmul1hi = x;		/* add to stack ptes */
}

/*
 * replace an L1 section pte with an L2 page table and an L1 coarse pte,
 * with the same attributes as the original pte and covering the same
 * region of memory.  beware that we may be changing ptes that cover
 * the code or data that we are using while executing this.
 */
static void
expand(uintptr va)
{
	int x, s;
	uintptr tva, pa;
	PTE oldpte;
	PTE *l1, *l2;

	va &= ~(Sectionsz-1);
	x = L1X(va);
	l1 = &m->mmul1[x];		/* l1 pte to replace */
	oldpte = *l1;
	if (oldpte == Fault || (oldpte & (Coarse|Section)) != Section)
		return;			/* make idempotent */

	/* wasteful - l2 pages only have 256 entries - fix */

	/* fill new l2 page with l2 ptes with equiv of L1 attrs; copy AP bits */
	x = Small | oldpte & (Cached|Buffered) | (oldpte & (1<<15 | 3<<10)) >> 6;
	if (oldpte & L1sharable)
		x |= L2sharable;
	if (oldpte & L1wralloc)
		x |= L2wralloc;
	if (oldpte & Noexecsect)
		x |= Noexecsmall;
	pa = oldpte & ~(Sectionsz - 1);
	/*
	 * it may be very early, before any memory allocators are
	 * configured, so do a crude allocation from the top of memory.
	 */
	l2 = newl2page();
	s = splhi();
	for(tva = va; tva < va + Sectionsz; tva += BY2PG, pa += BY2PG)
		setpte(&l2[L2X(tva)], tva, PPN(pa) | x);

	/* write new l1 entry, for l2 page, back into l1 descriptors */
	setpte(l1, va, PPN(PADDR(l2))|Dom0|Coarse);
	splx(s);

	if ((*l1 & (Coarse|Section)) != Coarse)
		panic("expand %#p", va);
}

void
mmusetnewl1(Mach *mm, PTE *l1)
{
	int s;

	s = splhi();
	mm->mmul1 = l1;
	mm->mmul1lo = L1lo+1;	/* first pte after text+data */
	mm->mmul1hi = L1hi-1;	/* lowest stack pte */
	splx(s);
}

/* brute force: zero all the user-space ptes */
void
mmuzerouser(void)
{
	int s;

	s = splhi();
	setptes(&m->mmul1[L1lo], L1lo*Sectionsz, Fault, L1hi - L1lo, Sectionsz);
	m->mmul1lo = L1lo+1;
	m->mmul1hi = L1hi-1;
	splx(s);
}

/* l1 is base of my l1 descriptor table */
static PTE *
l2pteaddr(PTE *l1, uintptr va)
{
	uintptr l2pa;
	PTE pte;
	PTE *l2;

	expand(va);
	pte = l1[L1X(va)];
	if ((pte & (Coarse|Section)) != Coarse)
		panic("l2pteaddr l1 pte %#8.8ux @ %#p not Coarse",
			pte, &l1[L1X(va)]);
	l2pa = pte & ~(KB - 1);
	l2 = (PTE *)KADDR(l2pa);
	if (Erratum782773)
		cachedwbinvse(&l2[L2X(va)], sizeof(PTE));
	return &l2[L2X(va)];
}

/*
 * set up common L2 mappings, allocate more L2 page tables as needed.
 */
void
mmuaddl2maps(void)
{
	int s;
	ulong va;
	PTE *l1, *l2;

	l1 = m->mmul1;

	s = splhi();
#ifdef PREFETCH_HW_NOT_BROKEN
	/*
	 * make all cpus' l1 page tables no-execute to prevent wild and crazy
	 * speculative accesses; the page tables are contiguous.
	 * HOWEVER, the hardware hates this and generates read access faults
	 * when copying L1 page tables later.
	 */
	cachedwbinv();
	for(va = L1CPU0; va < L1CPU0 + MAXMACH*L1SIZE; va += BY2PG)
		*l2pteaddr(l1, va) |= Noexecsmall;
#endif
	/*
	 * scu was initially covered by PHYSIO identity map.
	 * map private mem region (8K at soc.scu) without sharable bits.
	 * this includes timers and the interrupt controller parts.
	 */
	va = soc.scu;
	*l2pteaddr(l1, va) &= ~L2sharable;
	*l2pteaddr(l1, va + BY2PG) &= ~L2sharable;

	/*
	 * map high vectors to start of dram, but only 4K at HVECTORS, not 1MB.
	 *
	 * below (and above!) the vectors in virtual space may be dram.
	 * populate the rest of l2 for the last MB.
	 */
	l2 = newl2page();
	for (va = -Sectionsz; va != 0; va += BY2PG)
		setpte(&l2[L2X(va)], va, PADDR(va) | L2AP(Krw) | Small |
			L2ptedramattrs);
	/* map high vectors page to 0; must match attributes of KZERO->0 map */
	setpte(&l2[L2X(HVECTORS)], HVECTORS, PHYSDRAM | L2AP(Krw) | Small |
		L2ptedramattrs);
	setpte(&l1[L1X(HVECTORS)], HVECTORS, PADDR(l2) | Dom0 | Coarse);

	/* make kernel text unwritable; these l2 page tables will be shared */
	cachedwb();		/* flush any modified kernel text pages */
	for(va = KTZERO; va < (ulong)etext; va += BY2PG)
		*l2pteaddr(l1, va) |= L2apro;
	splx(s);
}

/*
 * set up initial page tables for cpu0 before enabling mmu,
 * this in the zero segment.
 */
void
mmuinit0(void)
{
	int s;
	PTE *l1;

	l1 = (PTE *)DRAMADDR(L1CPU0);
	s = splhi();
	/*
	 * set up double map of PHYSDRAM, KZERO to PHYSDRAM for first few MBs,
	 * but only if KZERO and PHYSDRAM differ.  map v 0 -> p 0.
	 */
	if (PHYSDRAM != KZERO)
		fillptes(&l1[L1X(PHYSDRAM)],
			&l1[L1X(PHYSDRAM + DOUBLEMAPMBS*Sectionsz)],
			PTEDRAM, PHYSDRAM);
	/*
	 * fill in PTEs for memory at KZERO (v KZERO -> p 0).
	 * trimslice has 1 bank of 1GB at PHYSDRAM.
	 * Map the maximum.
	 */
	fillptes(&l1[L1X(KZERO)], &l1[L1X(KZERO + MAXMB*Sectionsz)], PTEDRAM,
		PHYSDRAM);

	/* identity-mapping PTEs for MMIO */
	fillptes(&l1[L1X(VIRTIO)], &l1[L1X(PHYSIOEND)], PTEIO, PHYSIO);

	mmuinvalidate();
	splx(s);
}

/*
 * redo double map of PHYSDRAM, KZERO in this cpu's ptes:
 * map v 0 -> p 0 so we can run after enabling mmu.
 * thus we are in the zero segment.
 * mmuinit will undo this double mapping later.
 */
void
mmudoublemap(void)
{
	PTE *l1;

	/* launchinit set m->mmul1 to a copy of cpu0's l1 page table */
	assert(m);
	assert(m->mmul1);
	l1 = (PTE *)DRAMADDR(m->mmul1);
	if (PHYSDRAM != KZERO)
		fillptes(&l1[L1X(PHYSDRAM)],
			 &l1[L1X(PHYSDRAM + DOUBLEMAPMBS*Sectionsz)],
			 PTEDRAM, PHYSDRAM);
}

void
mmuon(uintptr ttb)
{
	dacput(Client);		/* set the domain access control */
	pidput(0);
	ttbput(ttb);		/* set the translation table base */
	cacheuwbinv();
	mmuinvalidate();
	mmuenable();		/* into the virtual map */
}

/*
 * upon entry, the mmu is enabled and we have minimal page tables set up,
 * so we are updating the page tables in place.
 */
void
mmuinit(void)
{
	int s;
	PTE *l1;

	if (m->machno == 0)
		l1 = (PTE *)L1CPU0;
	else
		/* start with our existing minimal l1 table from launchinit */
		l1 = m->mmul1;

	s = splhi();
	if (m->machno != 0) {
		/*
		 * cpu0's l1 page table has likely changed since we copied it in
		 * launchinit, notably to allocate uncached sections for ucalloc
		 * used by ether, for example (ick), so copy it again to share
		 * its kernel mappings.
		 *
		 * NB: copying cpu0's l1 page table means that we share
		 * its l2 page tables(!).
		 */
		mmul1copy(l1, (char *)L1CPU0);
		mmuinvalidate();
	}
	/* m->mmul1 is used by expand in l2pteaddr */
	mmusetnewl1(m, l1);
	mmuzerouser();				/* no user proc yet */
	if (m->machno == 0) {
		/* identity map most of the io space in L1 table */
		mmuidmap(l1, PHYSIO, (PHYSIOEND - PHYSIO + Sectionsz - 1) /
			Sectionsz);

		/* move the rest to more convenient addresses */
		/* 0x40000000 v -> 0xd0000000 p */
		mmuiomap(l1, VIRTNOR, PHYSNOR, 256);
		/* 0xb0000000 v -> 0xc0000000 p */
		mmuiomap(l1, VIRTAHB, PHYSAHB, 256);

		/* map cpu0 high vec.s to start of dram, but only 4K, not 1MB */
		l2pages = KADDR(L2POOLBASE);
		mmuaddl2maps();
		hivecs();
	}

	cachedwbinv();
	mmuinvalidate();

	/* now that caches are on, include cacheability and shareability */
	ttbput(PADDR(l1) | TTBlow);
//	mmudump(l1);			/* DEBUG */
	splx(s);
}

static void
mmul2empty(Proc* proc, int clear)
{
	int s;
	PTE *l1;
	Page **l2, *page;

	l1 = m->mmul1;
	l2 = &proc->mmul2;
	for(page = *l2; page != nil; page = page->next){
		s = splhi();
		setpte(&l1[page->daddr], page->daddr * Sectionsz, Fault);
		if(clear)
			setptes(UINT2PTR(page->va), page->daddr * Sectionsz,
				Fault, L2used / sizeof *l1, BY2PG);
		splx(s);
		l2 = &page->next;
	}
	*l2 = proc->mmul2cache;
	proc->mmul2cache = proc->mmul2;
	proc->mmul2 = nil;
}

/*
 * clean out any user mappings still in l1 page table.
 */
static void
mmul1empty(void)
{
	int s;

	s = splhi();
	if(m->mmul1lo > L1lo) {			/* text+data ptes? */
		setptes(&m->mmul1[L1lo], L1lo * Sectionsz, Fault, m->mmul1lo,
			Sectionsz);
		m->mmul1lo = L1lo;
	}
	if(m->mmul1hi < L1hi){			/* stack ptes? */
		setptes(&m->mmul1[m->mmul1hi], m->mmul1hi * Sectionsz, Fault,
			L1hi - m->mmul1hi, Sectionsz);
		m->mmul1hi = L1hi - 1;
	}
	splx(s);
}

/*
 * switch page tables and tlbs on this cpu to a new process, `proc'.
 */
void
mmuswitch(Proc* proc)
{
	int x, s;
	PTE *l1;
	Page *page;

	/* do kprocs get here and if so, do they need to? */
	if(m->mmupid == proc->pid && !proc->newtlb)
		return;
	m->mmupid = proc->pid;

	/* write back dirty and invalidate caches with old map */
	cachedwbinv();  /* a big club, but we are changing user mapping */

	if(proc->newtlb){
		mmul2empty(proc, 1);
		proc->newtlb = 0;
	}

	mmul1empty();

	/* move in new map, generated from proc->mmul2 */
	l1 = m->mmul1;
	for(page = proc->mmul2; page != nil; page = page->next){
		x = page->daddr;
		s = splhi();
		setpte(&l1[x], page->va, PPN(page->pa)|Dom0|Coarse);

		/* know here that L1lo < x < L1hi */
		mmul1adj(x);
		splx(s);
	}

	/* lose any possible stale tlb entries */
	mmuinvalidate();

	//print("mmuswitch l1lo %d l1hi %d %d\n",
	//	m->mmul1lo, m->mmul1hi, proc->kp);

	wakewfi();		/* in case there's another runnable proc */
}

void
flushmmu(void)
{
	int s;

	s = splhi();
	up->newtlb = 1;
	mmuswitch(up);
	splx(s);
}

void
mmurelease(Proc* proc)
{
	Page *page, *next;

	/* write back dirty and invalidate caches with old map */
	cachedwbinv();  /* a big club, but we are changing user mapping */

	mmul2empty(proc, 0);
	for(page = proc->mmul2cache; page != nil; page = next){
		next = page->next;
		if(--page->ref)
			panic("mmurelease: page->ref %d", page->ref);
		pagechainhead(page);
	}
	if(proc->mmul2cache && palloc.r.p)
		wakeup(&palloc.r);
	proc->mmul2cache = nil;

	mmul1empty();

	/* lose any possible stale tlb entries */
	mmuinvalidate();
}

void
putmmu(uintptr va, uintptr pa, Page* page)
{
	int x, s;
	Page *pg;
	PTE *l1, *pte;

	x = L1X(va);
	l1 = &m->mmul1[x];
	if (Debug) {
		print("putmmu(%#p, %#p, %#p) ", va, pa, page->pa);
		print("mmul1 %#p l1 %#p *l1 %#ux x %d pid %ld\n",
			m->mmul1, l1, *l1, x, up->pid);
		if (*l1)
			panic("putmmu: old l1 pte non-zero; stuck?");
	}
	s = splhi();
	if(*l1 == Fault){
		/* wasteful - l2 pages only have 256 entries - fix */
		if(up->mmul2cache == nil){
			/* auxpg since we don't need much? memset if so */
			pg = newpage(1, 0, 0);
			pg->va = VA(kmap(pg));
		}
		else{
			pg = up->mmul2cache;
			up->mmul2cache = pg->next;
		}
		setptes(UINT2PTR(pg->va), x * Sectionsz, Fault,
			L2used/sizeof *l1, BY2PG);

		pg->daddr = x;
		pg->next = up->mmul2;
		up->mmul2 = pg;

		setpte(l1, va, PPN(pg->pa)|Dom0|Coarse);
		if (Debug)
			print("l1 %#p *l1 %#ux x %d pid %ld\n",
				l1, *l1, x, up->pid);

		if(x >= m->mmul1lo && x < m->mmul1hi)
			mmul1adj(x);
	}
	pte = UINT2PTR(KADDR(PPN(*l1)));
	if (Debug) {
		print("pte %#p index %ld was %#ux\n", pte, L2X(va), *(pte+L2X(va)));
		if (*(pte+L2X(va)))
			panic("putmmu: old l2 pte non-zero; stuck?");
	}

	/* protection bits are
	 *	PTERONLY|PTEVALID;
	 *	PTEWRITE|PTEVALID;
	 *	PTEWRITE|PTEUNCACHED|PTEVALID;
	 */
	x = Small;
	if(!(pa & PTEUNCACHED))
		x |= L2ptedramattrs;
	if(pa & PTEWRITE)
		x |= L2AP(Urw);
	else
		x |= L2AP(Uro);
	setpte(&pte[L2X(va)], va, PPN(pa)|x);

	/*  write back dirty entries - we need this because the pio() in
	 *  fault.c is writing via a different virt addr and won't clean
	 *  its changes out of the dcache.  Page coloring doesn't work
	 *  on this mmu because the virtual cache is set associative
	 *  rather than direct mapped.
	 */
	cachedwbinv();

	if(page->cachectl[0] == PG_TXTFLUSH){
		/* pio() sets PG_TXTFLUSH whenever a text pg has been written */
		cacheiinv();
		page->cachectl[0] = PG_NOFLUSH;
	}
	splx(s);
	if (Debug)
		print("putmmu %#p %#p %#p\n", va, pa, PPN(pa)|x);
}

static int
isoksections(char *func, uintptr va, uintptr pa, usize size)
{
	if (va & (Sectionsz-1)) {
		print("%s: va %#p not a multiple of 1MB\n", func, va);
		return 0;
	}
	if (pa & (Sectionsz-1)) {
		print("%s: pa %#p not a multiple of 1MB\n", func, pa);
		return 0;
	}
	if (size != Sectionsz) {
		print("%s: size %lud != 1MB\n", func, size);
		return 0;
	}
	return 1;
}

void*
mmuuncache(void* v, usize size)
{
	int x, s;
	PTE *pte;
	uintptr va;

	/*
	 * Simple helper for ucalloc().
	 * Uncache a Section, must already be valid in the MMU.
	 */
	va = PTR2UINT(v);
	if (!isoksections("mmuuncache", va, 0, size))
		return nil;

	x = L1X(va);
	pte = &m->mmul1[x];
	if((*pte & (Section|Coarse)) != Section) {
		print("mmuuncache: v %#p not a Section\n", va);
		return nil;
	}

	s = splhi();
	if (Erratum782773)
		cachedwbinvse(pte, sizeof *pte);
	*pte &= ~L1ptedramattrs;
	*pte |= L1sharable;
	mmuinvalidateaddr(va);
	splx(s);

	return v;
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 */
uintptr
cankaddr(uintptr pa)
{
	if((PHYSDRAM == 0 || pa >= PHYSDRAM) && pa < PHYSDRAM+memsize)
		return PHYSDRAM+memsize - pa;
	return 0;
}

/*
 * although needed by the pc port, this mapping can be trivial on our arm
 * systems, which have less memory.
 */
void*
vmap(uintptr pa, usize)
{
	return UINT2PTR(KZERO|pa);
}

void
vunmap(void* v, usize size)
{
	/*
	upafree(PADDR(v), size);
	 */
	USED(v, size);
}

/*
 * Notes.
 * Everything is in domain 0; domain 0 access bits in the DAC register
 * are set to Client, which means access is controlled by the permission
 * values set in the PTE.
 *
 * L1 access control for the kernel is set to 1 (RW, no user mode access);
 * L2 access control for the kernel is set to 1 (ditto) for all 4 AP sets;
 * L1 user mode access is never set;
 * L2 access control for user mode is set to either
 * 2 (RO) or 3 (RW) depending on whether text or data, for all 4 AP sets.
 * (To get kernel RO set AP to 0 and S bit in control register c1).
 * Coarse L1 page-tables are used.
 * They have 256 entries and so consume 1024 bytes per table.
 * Small L2 page-tables are used.
 * They have 1024 entries and so consume 4096 bytes per table.
 *
 * 4KB.  That's the size of (1) a page, (2) the size allocated for an L2
 * page-table page (note only 1KB is needed per L2 page - to be dealt
 * with later) and (3) the size of the area in L1 needed to hold the PTEs
 * to map 1GB of user space (0 -> 0x3fffffff, 1024 entries).
 */
