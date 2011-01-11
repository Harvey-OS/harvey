#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "arm.h"

#define L1X(va)		FEXT((va), 20, 12)
#define L2X(va)		FEXT((va), 12, 8)

enum {
	L1lo		= UZERO/MiB,		/* L1X(UZERO)? */
	L1hi		= (USTKTOP+MiB-1)/MiB,	/* L1X(USTKTOP+MiB-1)? */
};

#define ISHOLE(pte)	((pte) == 0)

/* dump level 1 page table at virtual addr l1 */
void
mmudump(PTE *l1)
{
	int i, type, rngtype;
	uintptr pa, startva, startpa;
	uvlong va, endva;
	PTE pte;

	iprint("\n");
	endva = startva = startpa = 0;
	rngtype = 0;
	/* dump first level of ptes */
	for (va = i = 0; i < 4096; i++) {
		pte = l1[i];
		pa = pte & ~(MB - 1);
		type = pte & (Fine|Section|Coarse);
		if (ISHOLE(pte)) {
			if (endva != 0) {	/* open range? close it */
				iprint("l1 maps va (%#lux-%#llux) -> pa %#lux type %#ux\n",
					startva, endva-1, startpa, rngtype);
				endva = 0;
			}
		} else {
			if (endva == 0) {	/* no open range? start one */
				startva = va;
				startpa = pa;
				rngtype = type;
			}
			endva = va + MB;	/* continue the open range */
		}
		va += MB;
	}
	if (endva != 0)			/* close an open range */
		iprint("l1 maps va (%#lux-%#llux) -> pa %#lux type %#ux\n",
			startva, endva-1, startpa, rngtype);
}

#ifdef CRYPTOSANDBOX
extern uchar sandbox[64*1024+BY2PG];
#endif

/* identity map `mbs' megabytes from phys */
void
mmuidmap(uintptr phys, int mbs)
{
	PTE *l1;
	uintptr pa, fpa;

	pa = ttbget();
	l1 = KADDR(pa);

	for (fpa = phys; mbs-- > 0; fpa += MiB)
		l1[L1X(fpa)] = fpa|Dom0|L1AP(Krw)|Section;
	coherence();

	mmuinvalidate();
	cacheuwbinv();
	l2cacheuwbinv();
}

void
mmuinit(void)
{
	PTE *l1, *l2;
	uintptr pa, i;

	pa = ttbget();
	l1 = KADDR(pa);

	/*
	 * map high vectors to start of dram, but only 4K, not 1MB.
	 */
	pa -= MACHSIZE+2*1024;
	l2 = KADDR(pa);
	memset(l2, 0, 1024);
	/* vectors step on u-boot, but so do page tables */
	l2[L2X(HVECTORS)] = PHYSDRAM|L2AP(Krw)|Small;
	l1[L1X(HVECTORS)] = pa|Dom0|Coarse;	/* vectors -> ttb-machsize-2k */

	/* double map vectors at virtual 0 so reset will see them */
	pa -= 1024;
	l2 = KADDR(pa);
	memset(l2, 0, 1024);
	l2[L2X(0)] = PHYSDRAM|L2AP(Krw)|Small;
	l1[L1X(0)] = pa|Dom0|Coarse;

	/*
	 * set up L2 ptes for PHYSIO (i/o registers), with smaller pages to
	 * enable user-mode access to a few devices.
	 */
	pa -= 1024;
	l2 = KADDR(pa);
	/* identity map by default */
	for (i = 0; i < 1024/4; i++)
		l2[L2X(VIRTIO + i*BY2PG)] = (PHYSIO + i*BY2PG)|L2AP(Krw)|Small;

#ifdef CRYPTOSANDBOX
	/*
	 * rest is to let rae experiment with the crypto hardware
	 */
	/* access to cycle counter */
	l2[L2X(soc.clock)] = soc.clock | L2AP(Urw)|Small;
	/* cesa registers; also visible in user space */
	for (i = 0; i < 16; i++)
		l2[L2X(soc.cesa + i*BY2PG)] = (soc.cesa + i*BY2PG) |
			L2AP(Urw)|Small;
	/* crypto sram; remapped to unused space and visible in user space */
	l2[L2X(PHYSIO + 0xa0000)] = PHYSCESASRAM | L2AP(Urw)|Small;
	/* 64k of scratch dram */
	for (i = 0; i < 16; i++)
		l2[L2X(PHYSIO + 0xb0000 + i*BY2PG)] =
			(PADDR((uintptr)sandbox & ~(BY2PG-1)) + i*BY2PG) |
			 L2AP(Urw) | Small;
#endif

	l1[L1X(VIRTIO)] = pa|Dom0|Coarse;
	coherence();

	mmuinvalidate();
	cacheuwbinv();
	l2cacheuwbinv();

	m->mmul1 = l1;
//	mmudump(l1);			/* DEBUG.  too early to print */
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
		l2 = &page->next;
	}
	*l2 = proc->mmul2cache;
	proc->mmul2cache = proc->mmul2;
	proc->mmul2 = nil;
}

static void
mmul1empty(void)
{
#ifdef notdef			/* there's a bug in here */
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

	/* write back dirty and invalidate l1 caches */
	cacheuwbinv();

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
	cachedwbse(&l1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));
	l2cacheuwbse(&l1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));

	/* lose any possible stale tlb entries */
	mmuinvalidate();

//	mmudump(l1);
	//print("mmuswitch l1lo %d l1hi %d %d\n",
	//	m->mmul1lo, m->mmul1hi, proc->kp);
//print("\n");
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

	/* write back dirty and invalidate l1 caches */
	cacheuwbinv();

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
	cachedwbse(&m->mmul1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));
	l2cacheuwbse(&m->mmul1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));

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
	//print("putmmu(%#p, %#p, %#p) ", va, pa, page->pa);
	//print("mmul1 %#p l1 %#p *l1 %#ux x %d pid %d\n",
	//	m->mmul1, l1, *l1, x, up->pid);
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
		cachedwbse((void *)pg->va, BY2PG);
		l2cacheuwbse((void *)pg->va, BY2PG);

		*l1 = PPN(pg->pa)|Dom0|Coarse;
		cachedwbse(l1, sizeof *l1);
		l2cacheuwbse(l1, sizeof *l1);
		//print("l1 %#p *l1 %#ux x %d pid %d\n", l1, *l1, x, up->pid);

		if(x >= m->mmul1lo && x < m->mmul1hi){
			if(x+1 - m->mmul1lo < m->mmul1hi - x)
				m->mmul1lo = x+1;
			else
				m->mmul1hi = x;
		}
	}
	pte = UINT2PTR(KADDR(PPN(*l1)));
	//print("pte %#p index %ld %#ux\n", pte, L2X(va), *(pte+L2X(va)));

	/* protection bits are
	 *	PTERONLY|PTEVALID;
	 *	PTEWRITE|PTEVALID;
	 *	PTEWRITE|PTEUNCACHED|PTEVALID;
	 */
	x = Small;
	if(!(pa & PTEUNCACHED))
		x |= Cached|Buffered;
	if(pa & PTEWRITE)
		x |= L2AP(Urw);
	else
		x |= L2AP(Uro);
	pte[L2X(va)] = PPN(pa)|x;
	cachedwbse(&pte[L2X(va)], sizeof pte[0]);
	l2cacheuwbse(&pte[L2X(va)], sizeof pte[0]);

	/* clear out the current entry */
	mmuinvalidateaddr(PPN(va));

	/*
	 *  write back dirty entries - we need this because pio() in
	 *  fault.c is writing via a different virt addr and won't clean
	 *  its changes out of the dcache.  Page coloring doesn't work
	 *  on this mmu because the l1 virtual cache is set associative
	 *  rather than direct mapped.
	 */
	cachedwbinv();
	if(page->cachectl[0] == PG_TXTFLUSH){
		/* pio() sets PG_TXTFLUSH whenever a text pg has been written */
		cacheiinv();
		page->cachectl[0] = PG_NOFLUSH;
	}
	//print("putmmu %#p %#p %#p\n", va, pa, PPN(pa)|x);
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
	if((*pte & (Fine|Section|Coarse)) != Section)
		return nil;
	*pte &= ~(Cached|Buffered);
	mmuinvalidateaddr(va);
	cachedwbse(pte, 4);
	l2cacheuwbse(pte, 4);

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
	cachedwbse(pte, 4);
	l2cacheuwbse(pte, 4);

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
	cachedwbse(pte, 4);
	l2cacheuwbse(pte, 4);

	return va;
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 */
uintptr
cankaddr(uintptr pa)
{
	if(pa < PHYSDRAM + 512*MiB)		/* assumes PHYSDRAM is 0 */
		return PHYSDRAM + 512*MiB - pa;
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
