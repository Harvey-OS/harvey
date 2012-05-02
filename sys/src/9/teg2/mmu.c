/*
 * arm arch v7 mmu
 *
 * we initially thought that we needn't flush the l2 cache since external
 * devices needn't see page tables.  sadly, reality does not agree with
 * the manuals.
 *
 * we use l1 and l2 cache ops here because they are empirically needed.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "arm.h"

#define L1X(va)		FEXT((va), 20, 12)
#define L2X(va)		FEXT((va), 12, 8)

enum {
	Debug		= 0,

	L1lo		= UZERO/MiB,		/* L1X(UZERO)? */
#ifdef SMALL_ARM				/* well under 1GB of RAM? */
	L1hi		= (USTKTOP+MiB-1)/MiB,	/* L1X(USTKTOP+MiB-1)? */
#else
	/*
	 * on trimslice, top of 1GB ram can't be addressible, as high
	 * virtual memory (0xfff.....) contains high vectors.  We
	 * moved USTKTOP down another MB to utterly avoid KADDR(stack_base)
	 * mapping to high exception vectors.  USTKTOP is thus
	 * (0x40000000 - 64*KiB - MiB), which in kernel virtual space is
	 * (0x100000000ull - 64*KiB - MiB), but we need the whole user
	 * virtual address space to be unmapped in a new process.
	 */
	L1hi		= DRAMSIZE/MiB,
#endif
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

	iprint("l1 maps va (%#8.8lux-%#llux) -> ", rp->startva, rp->endva-1);
	if (rp->startva == rp->startpa)
		iprint("identity-mapped");
	else
		iprint("pa %#8.8lux", rp->startpa);
	iprint(" attrs ");
	attrs = rp->attrs;
	if (attrs) {
		if (attrs & Cached)
			iprint("C");
		if (attrs & Buffered)
			iprint("B");
		if (attrs & L1sharable)
			iprint("S1");
		if (attrs & L1wralloc)
			iprint("A1");
	} else
		iprint("\"\"");
	iprint(" %s\n", typename(rp->type));
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
	iprint("cpu%d l1 pt @ %#p:\n", m->machno, PADDR(l1));
	memset(&rng, 0, sizeof rng);
	for (va = i = 0; i < 4096; i++, va += MB) {
		pte = l1[i];
		type = pte & (Section|Coarse);
		if (type == Section)
			pa = pte & ~(MB - 1);
		else
			pa = pte & ~(KiB - 1);
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
			rng.endva = va + MB;	/* continue the open range */
			rng.endpa = pa + MB;
		}
		if (type == Coarse)
			l2dump(&rng, pte);
	}
	if (rng.endva != 0)			/* close any open range */
		prl1range(&rng);
	iprint("\n");
}

/*
 * map `mbs' megabytes from virt to phys, uncached.
 * device registers are sharable, except the private memory region:
 * 2 4K pages, at 0x50040000 on the tegra2.
 */
void
mmumap(uintptr virt, uintptr phys, int mbs)
{
	uint off;
	PTE *l1;

	phys &= ~(MB-1);
	virt &= ~(MB-1);
	l1 = KADDR(ttbget());
	for (off = 0; mbs-- > 0; off += MB)
		l1[L1X(virt + off)] = (phys + off) | Dom0 | L1AP(Krw) |
			Section | L1sharable;
	allcache->wbse(l1, L1SIZE);
	mmuinvalidate();
}

/* identity map `mbs' megabytes from phys */
void
mmuidmap(uintptr phys, int mbs)
{
	mmumap(phys, phys, mbs);
}

PTE *
newl2page(void)
{
	PTE *p;

	if ((uintptr)l2pages >= HVECTORS - BY2PG)
		panic("l2pages");
	p = (PTE *)l2pages;
	l2pages += BY2PG;
	return p;
}

/*
 * replace an L1 section pte with an L2 page table and an L1 coarse pte,
 * with the same attributes as the original pte and covering the same
 * region of memory.
 */
static void
expand(uintptr va)
{
	int x;
	uintptr tva, pa;
	PTE oldpte;
	PTE *l1, *l2;

	va &= ~(MB-1);
	x = L1X(va);
	l1 = &m->mmul1[x];
	oldpte = *l1;
	if (oldpte == Fault || (oldpte & (Coarse|Section)) != Section)
		return;			/* make idempotent */

	/* wasteful - l2 pages only have 256 entries - fix */
	/*
	 * it may be very early, before any memory allocators are
	 * configured, so do a crude allocation from the top of memory.
	 */
	l2 = newl2page();
	memset(l2, 0, BY2PG);

	/* write new L1 l2 entry back into L1 descriptors */
	*l1 = PPN(PADDR(l2))|Dom0|Coarse;

	/* fill l2 page with l2 ptes with equiv attrs; copy AP bits */
	x = Small | oldpte & (Cached|Buffered) | (oldpte & (1<<15 | 3<<10)) >> 6;
	if (oldpte & L1sharable)
		x |= L2sharable;
	if (oldpte & L1wralloc)
		x |= L2wralloc;
	pa = oldpte & ~(MiB - 1);
	for(tva = va; tva < va + MiB; tva += BY2PG, pa += BY2PG)
		l2[L2X(tva)] = PPN(pa) | x;

	/* force l2 page to memory */
	allcache->wbse(l2, BY2PG);

	/* clear out the current entry */
	mmuinvalidateaddr(PPN(va));

	allcache->wbinvse(l1, sizeof *l1);
	if ((*l1 & (Coarse|Section)) != Coarse)
		panic("explode %#p", va);
}

/*
 * cpu0's l1 page table has likely changed since we copied it in
 * launchinit, notably to allocate uncached sections for ucalloc.
 * so copy it again from cpu0's.
 */
void
mmuninit(void)
{
	int s;
	PTE *l1, *newl1;

	s = splhi();
	l1 = m->mmul1;
	newl1 = mallocalign(L1SIZE, L1SIZE, 0, 0);
	assert(newl1);

	allcache->wbinvse((PTE *)L1, L1SIZE);	/* get cpu0's up-to-date copy */
	memmove(newl1, (PTE *)L1, L1SIZE);
	allcache->wbse(newl1, L1SIZE);

	mmuinvalidate();
	coherence();

	ttbput(PADDR(newl1));		/* switch */
	coherence();
	mmuinvalidate();
	coherence();
	m->mmul1 = newl1;
	coherence();

	mmul1empty();
	coherence();
	mmuinvalidate();
	coherence();

//	mmudump(m->mmul1);		/* DEBUG */
	splx(s);
	free(l1);
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
	l2pa = pte & ~(KiB - 1);
	l2 = (PTE *)KADDR(l2pa);
	return &l2[L2X(va)];
}

void
mmuinit(void)
{
	ulong va;
	uintptr pa;
	PTE *l1, *l2;

	if (m->machno != 0) {
		mmuninit();
		return;
	}

	pa = ttbget();
	l1 = KADDR(pa);

	/* identity map most of the io space */
	mmuidmap(PHYSIO, (PHYSIOEND - PHYSIO + MB - 1) / MB);
	/* move the rest to more convenient addresses */
	mmumap(VIRTNOR, PHYSNOR, 256);	/* 0x40000000 v -> 0xd0000000 p */
	mmumap(VIRTAHB, PHYSAHB, 256);	/* 0xb0000000 v -> 0xc0000000 p */

	/* map high vectors to start of dram, but only 4K, not 1MB */
	pa -= MACHSIZE+BY2PG;		/* page tables must be page aligned */
	l2 = KADDR(pa);
	memset(l2, 0, 1024);

	m->mmul1 = l1;		/* used by explode in l2pteaddr */

	/* map private mem region (8K at soc.scu) without sharable bits */
	va = soc.scu;
	*l2pteaddr(l1, va) &= ~L2sharable;
	va += BY2PG;
	*l2pteaddr(l1, va) &= ~L2sharable;

	/*
	 * below (and above!) the vectors in virtual space may be dram.
	 * populate the rest of l2 for the last MB.
	 */
	for (va = -MiB; va != 0; va += BY2PG)
		l2[L2X(va)] = PADDR(va) | L2AP(Krw) | Small | L2ptedramattrs;
	/* map high vectors page to 0; must match attributes of KZERO->0 map */
	l2[L2X(HVECTORS)] = PHYSDRAM | L2AP(Krw) | Small | L2ptedramattrs;
	coherence();
	l1[L1X(HVECTORS)] = pa | Dom0 | Coarse;	/* l1 -> ttb-machsize-4k */

	/* make kernel text unwritable */
	for(va = KTZERO; va < (ulong)etext; va += BY2PG)
		*l2pteaddr(l1, va) |= L2apro;

	allcache->wbinv();
	mmuinvalidate();

	m->mmul1 = l1;
	coherence();
	mmul1empty();
	coherence();
//	mmudump(l1);			/* DEBUG */
}

static void
mmul2empty(Proc* proc, int clear)
{
	PTE *l1;
	Page **l2, *page;

	l1 = m->mmul1;
	l2 = &proc->mmul2;
	for(page = *l2; page != nil; page = page->next){
		if(clear)
			memset(UINT2PTR(page->va), 0, BY2PG);
		l1[page->daddr] = Fault;
		allcache->wbse(l1, sizeof *l1);
		l2 = &page->next;
	}
	*l2 = proc->mmul2cache;
	proc->mmul2cache = proc->mmul2;
	proc->mmul2 = nil;
}

static void
mmul1empty(void)
{
#ifdef notdef
/* there's a bug in here */
	PTE *l1;

	/* clean out any user mappings still in l1 */
	if(m->mmul1lo > L1lo){
		if(m->mmul1lo == 1)
			m->mmul1[L1lo] = Fault;
		else
			memset(&m->mmul1[L1lo], 0, m->mmul1lo*sizeof(PTE));
		m->mmul1lo = L1lo;
	}
	if(m->mmul1hi < L1hi){
		l1 = &m->mmul1[m->mmul1hi];
		if((L1hi - m->mmul1hi) == 1)
			*l1 = Fault;
		else
			memset(l1, 0, (L1hi - m->mmul1hi)*sizeof(PTE));
		m->mmul1hi = L1hi;
	}
#else
	memset(&m->mmul1[L1lo], 0, (L1hi - L1lo)*sizeof(PTE));
#endif /* notdef */
	allcache->wbse(&m->mmul1[L1lo], (L1hi - L1lo)*sizeof(PTE));
}

void
mmuswitch(Proc* proc)
{
	int x;
	PTE *l1;
	Page *page;

	/* do kprocs get here and if so, do they need to? */
	if(m->mmupid == proc->pid && !proc->newtlb)
		return;
	m->mmupid = proc->pid;

	/* write back dirty and invalidate caches */
	l1cache->wbinv();

	if(proc->newtlb){
		mmul2empty(proc, 1);
		proc->newtlb = 0;
	}

	mmul1empty();

	/* move in new map */
	l1 = m->mmul1;
	for(page = proc->mmul2; page != nil; page = page->next){
		x = page->daddr;
		l1[x] = PPN(page->pa)|Dom0|Coarse;
		/* know here that L1lo < x < L1hi */
		if(x+1 - m->mmul1lo < m->mmul1hi - x)
			m->mmul1lo = x+1;
		else
			m->mmul1hi = x;
	}

	/* make sure map is in memory */
	/* could be smarter about how much? */
	allcache->wbse(&l1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));

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

	/* write back dirty and invalidate caches */
	l1cache->wbinv();

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

	/* make sure map is in memory */
	/* could be smarter about how much? */
	allcache->wbse(&m->mmul1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));

	/* lose any possible stale tlb entries */
	mmuinvalidate();
}

void
putmmu(uintptr va, uintptr pa, Page* page)
{
	int x;
	Page *pg;
	PTE *l1, *pte;

	x = L1X(va);
	l1 = &m->mmul1[x];
	if (Debug) {
		iprint("putmmu(%#p, %#p, %#p) ", va, pa, page->pa);
		iprint("mmul1 %#p l1 %#p *l1 %#ux x %d pid %ld\n",
			m->mmul1, l1, *l1, x, up->pid);
		if (*l1)
			panic("putmmu: old l1 pte non-zero; stuck?");
	}
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
			memset(UINT2PTR(pg->va), 0, BY2PG);
		}
		pg->daddr = x;
		pg->next = up->mmul2;
		up->mmul2 = pg;

		/* force l2 page to memory */
		allcache->wbse((void *)pg->va, BY2PG);

		*l1 = PPN(pg->pa)|Dom0|Coarse;
		allcache->wbse(l1, sizeof *l1);

		if (Debug)
			iprint("l1 %#p *l1 %#ux x %d pid %ld\n", l1, *l1, x, up->pid);

		if(x >= m->mmul1lo && x < m->mmul1hi){
			if(x+1 - m->mmul1lo < m->mmul1hi - x)
				m->mmul1lo = x+1;
			else
				m->mmul1hi = x;
		}
	}
	pte = UINT2PTR(KADDR(PPN(*l1)));
	if (Debug) {
		iprint("pte %#p index %ld was %#ux\n", pte, L2X(va), *(pte+L2X(va)));
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
	pte[L2X(va)] = PPN(pa)|x;
	allcache->wbse(&pte[L2X(va)], sizeof pte[0]);

	/* clear out the current entry */
	mmuinvalidateaddr(PPN(va));

	/*  write back dirty entries - we need this because the pio() in
	 *  fault.c is writing via a different virt addr and won't clean
	 *  its changes out of the dcache.  Page coloring doesn't work
	 *  on this mmu because the virtual cache is set associative
	 *  rather than direct mapped.
	 */
	l1cache->wb();

	if(page->cachectl[0] == PG_TXTFLUSH){
		/* pio() sets PG_TXTFLUSH whenever a text pg has been written */
		cacheiinv();
		page->cachectl[0] = PG_NOFLUSH;
	}
	if (Debug)
		iprint("putmmu %#p %#p %#p\n", va, pa, PPN(pa)|x);
}

void*
mmuuncache(void* v, usize size)
{
	int x;
	PTE *pte;
	uintptr va;

	/*
	 * Simple helper for ucalloc().
	 * Uncache a Section, must already be
	 * valid in the MMU.
	 */
	va = PTR2UINT(v);
	assert(!(va & (1*MiB-1)) && size == 1*MiB);

	x = L1X(va);
	pte = &m->mmul1[x];
	if((*pte & (Section|Coarse)) != Section)
		return nil;
	*pte &= ~L1ptedramattrs;
	*pte |= L1sharable;
	mmuinvalidateaddr(va);
	allcache->wbse(pte, 4);

	return v;
}

uintptr
mmukmap(uintptr va, uintptr pa, usize size)
{
	int x;
	PTE *pte;

	/*
	 * Stub.
	 */
	assert(!(va & (1*MiB-1)) && !(pa & (1*MiB-1)) && size == 1*MiB);

	x = L1X(va);
	pte = &m->mmul1[x];
	if(*pte != Fault)
		return 0;
	*pte = pa|Dom0|L1AP(Krw)|Section;
	mmuinvalidateaddr(va);
	allcache->wbse(pte, 4);

	return va;
}

uintptr
mmukunmap(uintptr va, uintptr pa, usize size)
{
	int x;
	PTE *pte;

	/*
	 * Stub.
	 */
	assert(!(va & (1*MiB-1)) && !(pa & (1*MiB-1)) && size == 1*MiB);

	x = L1X(va);
	pte = &m->mmul1[x];
	if(*pte != (pa|Dom0|L1AP(Krw)|Section))
		return 0;
	*pte = Fault;
	mmuinvalidateaddr(va);
	allcache->wbse(pte, 4);

	return va;
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

/* from 386 */
void*
vmap(uintptr pa, usize size)
{
	uintptr pae, va;
	usize o, osize;

	/*
	 * XXX - replace with new vm stuff.
	 * Crock after crock - the first 4MB is mapped with 2MB pages
	 * so catch that and return good values because the current mmukmap
	 * will fail.
	 */
	if(pa+size < 4*MiB)
		return UINT2PTR(kseg0|pa);

	osize = size;
	o = pa & (BY2PG-1);
	pa -= o;
	size += o;
	size = ROUNDUP(size, BY2PG);

	va = kseg0|pa;
	pae = mmukmap(va, pa, size);
	if(pae == 0 || pae-size != pa)
		panic("vmap(%#p, %ld) called from %#p: mmukmap fails %#p",
			pa+o, osize, getcallerpc(&pa), pae);

	return UINT2PTR(va+o);
}

/* from 386 */
void
vunmap(void* v, usize size)
{
	/*
	 * XXX - replace with new vm stuff.
	 * Can't do this until do real vmap for all space that
	 * might be used, e.g. stuff below 1MB which is currently
	 * mapped automagically at boot but that isn't used (or
	 * at least shouldn't be used) by the kernel.
	upafree(PADDR(v), size);
	 */
	USED(v, size);
}

/*
 * Notes.
 * Everything is in domain 0;
 * domain 0 access bits in the DAC register are set
 * to Client, which means access is controlled by the
 * permission values set in the PTE.
 *
 * L1 access control for the kernel is set to 1 (RW,
 * no user mode access);
 * L2 access control for the kernel is set to 1 (ditto)
 * for all 4 AP sets;
 * L1 user mode access is never set;
 * L2 access control for user mode is set to either
 * 2 (RO) or 3 (RW) depending on whether text or data,
 * for all 4 AP sets.
 * (To get kernel RO set AP to 0 and S bit in control
 * register c1).
 * Coarse L1 page-tables are used. They have 256 entries
 * and so consume 1024 bytes per table.
 * Small L2 page-tables are used. They have 1024 entries
 * and so consume 4096 bytes per table.
 *
 * 4KiB. That's the size of 1) a page, 2) the
 * size allocated for an L2 page-table page (note only 1KiB
 * is needed per L2 page - to be dealt with later) and
 * 3) the size of the area in L1 needed to hold the PTEs
 * to map 1GiB of user space (0 -> 0x3fffffff, 1024 entries).
 */
