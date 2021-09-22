/*
 * memory management.  specifically, virtual to physical address mappings.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"

#define ALIGNED(p, a)	(!(((uintptr)(p)) & ((a)-1)))

/* see mem.h */
#define PDPX(v)		PTLX((v), 2)
#define PDX(v)		PTLX((v), 1)
#define PTX(v)		PTLX((v), 0)

enum {
	PDMAP =		VZERO - 8*MB,
	VMAPSZ =	256*MB,
	VMAP =		VZERO - 512*MB,
	/*
	 * holds entire 48-bit addr space but could probably be as small
	 * as ~512GB (max. page table tree size).
	 */
	PML4BASE =	VZERO - 256*TB,
	/* KSEG1PML4 works out to 0xffffff7fbfdfe000 */
	KSEG1PML4 =	PML4BASE
			|(PTLX(KSEG1, 3)<<(3*PTSHFT+PGSHFT))
			|(PTLX(KSEG1, 3)<<(2*PTSHFT+PGSHFT))
			|(PTLX(KSEG1, 3)<<(1*PTSHFT+PGSHFT))
			|(PTLX(KSEG1, 3)<<(0*PTSHFT+PGSHFT)),
};

#define KSEG1PTP(va, l)	(PML4BASE\
			|(KSEG1PML4 << ((Npglvls-1 - (l))*PTSHFT))\
			|(((va) & ~PML4BASE) >> (((l)+1)*PTSHFT)) &\
				~((uintptr)PTSZ-1))

static Lock vmaplock;
static Page mach0pml4;

/*
 * max kernel memory use within KSEG0, multiple of 2 or 4 MB.
 * stay below phys 0xa0000000.  used in mmu.c and map.c.
 */
uintptr kernmem = KSEG0SIZE + VMAP;		/* stay below VMAP */

void
mmuflushtlb(u64int)
{
	Page *pml4;

	pml4 = m->pml4;
	if(pml4->daddr){
		memset(UINT2PTR(pml4->va), 0, pml4->daddr*sizeof(PTE));
		pml4->daddr = 0;
	}
	cr3put(pml4->pa);
}

void
mmuflush(void)
{
	Mpl s;

	s = splhi();
	up->newtlb = 1;
	mmuswitch(up);
	splx(s);
}

static void
mmuptpfree(Proc* proc, int release)
{
	int l;
	PTE *pte;
	Page **last, *page;

	/*
	 * To do here:
	 *	coalesce the clean and release functionality
	 *	(it's either one or the other, and no need for
	 *	wakeup in mmurelease as not using the palloc pool);
	 *	0-based levels, not 1-based, for consistency;
	 *	fix memset level for 2MB pages;
	 *	use a dedicated datastructure rather than Page?
	 */
	for(l = 1; l < Npglvls; l++){
		last = &proc->mmuptp[l];
		if(*last == nil)
			continue;
		for(page = *last; page != nil; page = page->next){
			if(!release){
				if(l == 1)
					memset(UINT2PTR(page->va), 0, PTSZ);
				pte = UINT2PTR(page->prev->va);
				pte[page->daddr] = 0;
			}
			last = &page->next;
		}
		*last = proc->mmuptp[0];
		proc->mmuptp[0] = proc->mmuptp[l];
		proc->mmuptp[l] = nil;
	}

	m->pml4->daddr = 0;
}

static Page*
mmuptpalloc(void)
{
	void* va;
	Page *page;

	/*
	 * Do not really need a whole Page structure,
	 * but it makes testing this out a lot easier.
	 * Could keep a cache and free excess.
	 */
	if((page = malloc(sizeof(Page))) == nil){
		print("mmuptpalloc Page\n");
		return nil;
	}
	if((va = mallocalign(PTSZ, PTSZ, 0, 0)) == nil){
		print("mmuptpalloc va\n");
		free(page);
		return nil;
	}

	page->va = PTR2UINT(va);
	page->pa = PADDR(va);
	page->ref = 1;
	return page;
}

void
mmuswitch(Proc* proc)
{
	PTE *pte;
	Page *page,*pml4;

	if(proc->newtlb){
		mmuptpfree(proc, 0);
		proc->newtlb = 0;
	}

	pml4 = m->pml4;
	if(pml4->daddr){
		memset(UINT2PTR(pml4->va), 0, pml4->daddr*sizeof(PTE));
		pml4->daddr = 0;
	}

	pte = UINT2PTR(pml4->va);
	for(page = proc->mmuptp[3]; page != nil; page = page->next){
		pte[page->daddr] = PPN(page->pa)|PteU|PteRW|PteP;
		if(page->daddr >= pml4->daddr)
			pml4->daddr = page->daddr+1;
		page->prev = pml4;
	}

	tssrsp0(STACKALIGN(PTR2UINT(proc->kstack+KSTACK)));
	cr3put(pml4->pa);
//	idlewake();			/* other runnable procs? */
}

void
mmurelease(Proc* proc)
{
	Page *page, *next;

	/*
	 * See comments in mmuptpfree above.
	 */
	mmuptpfree(proc, 1);

	for(page = proc->mmuptp[0]; page != nil; page = next){
		next = page->next;
		if(--page->ref)
			panic("mmurelease: page->ref %d", page->ref);
		free(UINT2PTR(page->va));
		free(page);
	}
	if(proc->mmuptp[0] && palloc.r.p)
		wakeup(&palloc.r);
	proc->mmuptp[0] = nil;

	tssrsp0(STACKALIGN(m->stack+MACHSTKSZ));
	cr3put(m->pml4->pa);
}

static PTE*
mmuptpget(uintptr va, int level)
{
	return (PTE*)KSEG1PTP(va, level);
}

void
mmuput(uintptr va, uintmem pa, Page*)
{
	Mpl pl;
	int l, x;
	PTE *pte, *ptp;
	Page *page, *prev;

	pte = nil;
	pl = splhi();
	prev = m->pml4;
	for(l = Npglvls-1; l >= 0; l--){
		ptp = mmuptpget(va, l);
		x = PTLX(va, l);
		pte = &ptp[x];
		for(page = up->mmuptp[l]; page != nil; page = page->next)
			if(page->prev == prev && page->daddr == x)
				break;
		if(page == nil){
			if(up->mmuptp[0] == 0)
				page = mmuptpalloc();
			else {
				page = up->mmuptp[0];
				up->mmuptp[0] = page->next;
			}
			page->daddr = x;
			page->next = up->mmuptp[l];
			up->mmuptp[l] = page;
			page->prev = prev;
			*pte = PPN(page->pa)|PteU|PteRW|PteP;
			if(l == Npglvls-1 && x >= m->pml4->daddr)
				m->pml4->daddr = x+1;
		}
		prev = page;
	}

	*pte = pa|PteU;
	if(pa & PteRW)
		*pte |= PteNX;
	splx(pl);

	invlpg(va);			/* only if old entry valid? */
}

static PTE
pdeget(uintptr va)
{
	PTE *pdp;

	if(va < KZERO)
		panic("pdeget(%#p)", va);

	pdp = (PTE*)(PDMAP + PDX(PDMAP)*PTSZ);
	DBG("pdeget: va %#p PDX(PDMAP) %llud pdp %#p "
		"PDX(va) %llud &pdp[PDX(va)] %#p\n",
		va, PDX(PDMAP), pdp, PDX(va), &pdp[PDX(va)]);
	DBG("pdeget: pdp[PDX(va)] %#p\n", pdp[PDX(va)]);
	if (pdp[PDX(va)] == 0) {
		print("pdeget: pdp[PDX(%#p)] == 0!\n", va);
		delay(5000);
	}
	return pdp[PDX(va)];
}

/*
 * Add kernel mappings for va -> pa for a section of size bytes.
 * Called only after the va range is known to be unoccupied.
 */
static int
pdmap(uintmem pa, int attr, uintptr va, uintptr size)
{
	uintmem pae;
	uintptr pdx, pgsz;
	PTE *pd, *pde, *pt, *pte;

	pd = (PTE*)(PDMAP + PDX(PDMAP)*PTSZ);
	for(pae = pa + size; pa < pae; pa += pgsz){
		pdx = PDX(va);
		pde = &pd[pdx];

		/*
		 * Check if it can be mapped using a big page,
		 * i.e. is big enough and starts on a suitable boundary.
		 * Assume processor can do it.
		 */
		if(ALIGNED(pa, PGLSZ(1)) && ALIGNED(va, PGLSZ(1)) &&
		    (pae-pa) >= PGLSZ(1)){
			if(*pde != 0)
				panic("pdmap: *pde != 0 for pa %#p va %#p\n",
					pa, va);
			*pde = pa|attr|PtePS|PteP;
			pgsz = PGLSZ(1);
		}
		else{
			if(*pde == 0){
				/*
				 * Need a PTSZ physical allocator here.
				 * Because space will never be given back
				 * (see vunmap below), just malloc it so
				 * Ron can prove a point.
				*pde = pmalloc(PTSZ)|PteRW|PteP;
				 */
				void *alloc;

				alloc = mallocalign(PTSZ, PTSZ, 0, 0);
				if(alloc != nil){
					*pde = PADDR(alloc)|PteRW|PteP;
					memset((PTE*)(PDMAP+pdx*PTSZ), 0, PTSZ);

				}
			}
			if(*pde == 0)
				panic("pdmap: *pde == 0 for pa %#p va %#p\n",
					pa, va);

			pt = (PTE*)(PDMAP + pdx*PTSZ);
			pte = &pt[PTX(va)];
			assert(!(*pte & PteP));
			*pte = pa|attr|PteP;
			pgsz = PGLSZ(0);
		}
		va += pgsz;
	}

	return 0;
}

static int
findhole(PTE* a, int n, int count)
{
	int have, i;

	have = 0;
	for(i = 0; i < n; i++){
		if(a[i] == 0)
			have++;
		else
			have = 0;
		if(have >= count)
			return i+1 - have;
	}

	return -1;
}

/*
 * Look for free space in the vmap.
 */
static uintptr
vmapalloc(uintptr size)
{
	int i, n, o;
	uintptr pdsz, ptsz;
	PTE *pd, *pt;

	pd = (PTE*)(PDMAP + PDX(PDMAP)*PTSZ);
	pd += PDX(VMAP);
	pdsz = VMAPSZ/PGLSZ(1);

	/*
	 * Look directly in the PD entries if the size is
	 * larger than the range mapped by a single entry.
	 */
	if(size >= PGLSZ(1)){
		n = HOWMANY(size, PGLSZ(1));
		if((o = findhole(pd, pdsz, n)) != -1)
			return VMAP + o*PGLSZ(1);
		return 0;
	}

	/*
	 * Size is smaller than that mapped by a single PD entry.
	 * Look for an already mapped PT page that has room.
	 */
	n = HOWMANY(size, PGLSZ(0));
	ptsz = PGLSZ(0)/sizeof(PTE);
	for(i = 0; i < pdsz; i++){
		if(!(pd[i] & PteP) || (pd[i] & PtePS))
			continue;

		pt = (PTE*)(PDMAP + (PDX(VMAP) + i)*PTSZ);
		if((o = findhole(pt, ptsz, n)) != -1)
			return VMAP + i*PGLSZ(1) + o*PGLSZ(0);
	}

	/*
	 * Nothing suitable, start using a new PD entry.
	 */
	if((o = findhole(pd, pdsz, 1)) != -1)
		return VMAP + o*PGLSZ(1);

	return 0;
}

/* return uncached mapping for pa of size */
void*
vmap(uintmem pa, uintptr size)
{
	uintptr va, o, sz;

	DBG("vmap(%#P, %llud)\n", pa, (uvlong)size);

	if(m->machno != 0)
		panic("vmap");

	/*
	 * This is incomplete; the checks are not comprehensive enough.
	 * Sometimes the request is for an already-mapped piece
	 * of low memory, in which case just return a good value
	 * and hope that a corresponding vunmap of the address
	 * will have the same address.
	 * To do this properly will require keeping track of the
	 * mappings; perhaps something like kmap, but kmap probably
	 * can't be used early enough for some of the uses.
	 */
	if(pa+size <= MB)
		return KADDR(pa);
	if(pa < MB)
		return nil;

	/*
	 * Might be asking for less than a page.
	 * This should have a smaller granularity if
	 * the page size is large.
	 */
	o = pa & ((1<<PGSHFT)-1);
	pa -= o;
	sz = ROUNDUP(size+o, PGSZ);

	if(pa == 0){
		DBG("vmap(0, %llud) pc=%#p\n", (uvlong)size, getcallerpc(&pa));
		return nil;
	}
	ilock(&vmaplock);
	if((va = vmapalloc(sz)) == 0 || pdmap(pa, PtePCD|PteRW, va, sz) < 0){
		iunlock(&vmaplock);
		return nil;
	}
	iunlock(&vmaplock);

	DBG("vmap(%#P, %llud) => %#p\n", pa+o, (uvlong)size, va+o);

	return UINT2PTR(va + o);
}

void
vunmap(void* v, uintptr size)
{
	uintptr va;

	DBG("vunmap(%#p, %llud)\n", v, (uvlong)size);

	if(m->machno != 0)
		panic("vunmap");

	/*
	 * See the comments above in vmap.
	 */
	va = PTR2UINT(v);
	if(va >= KZERO && va+size < KZERO+1ull*MB)
		return;

	/*
	 * Here will have to deal with releasing any
	 * resources used for the allocation (e.g. page table
	 * pages).
	 */
	DBG("vunmap(%#p, %llud)\n", v, (uvlong)size);
}

int
mmuwalk(uintptr va, int level, PTE** ret, u64int (*alloc)(uintptr))
{
//alloc and pa - uintmem or PTE or what?
	int l;
	Mpl pl;
	uintptr pa;
	PTE *pte, *ptp;

	DBG("mmuwalk%d: va %#p level %d\n", m->machno, va, level);
	pte = nil;
	pl = splhi();
	for(l = Npglvls-1; l >= 0; l--){
		ptp = mmuptpget(va, l);
		pte = &ptp[PTLX(va, l)];
		if(l == level)
			break;
		if(!(*pte & PteP)){
			if(alloc == nil)	/* normal case: no allocator */
				break;
			pa = alloc(PTSZ);
			if(pa == ~0) {
				splx(pl);
				return -1;
			}
			if(pa & (PTSZ-1))
				print("mmuwalk pa %#p unaligned\n", pa); /* DEBUG */
			*pte = pa|PteRW|PteP;
			if((ptp = mmuptpget(va, l-1)) == nil)
				panic("mmuwalk: mmuptpget(%#p, %d)", va, l-1);
			memset(ptp, 0, PTSZ);
		}
		else if(*pte & PtePS)
			break;
	}
	*ret = pte;
	splx(pl);

	return l;
}

u64int
mmuphysaddr(uintptr va)
{
	int l;
	PTE *pte;
	u64int mask, pa;

	/*
	 * Given a VA, find the PA.
	 * This is probably not the right interface,
	 * but will do as an experiment. Usual
	 * question, should va be void* or uintptr?
	 */
	l = mmuwalk(va, 0, &pte, nil);
	DBG("mmuphysaddr: va %#p l %d\n", va, l);
	if(l < 0)
		return ~0ull;

	mask = (1ull<<(((l)*PTSHFT)+PGSHFT))-1;
	pa = (*pte & ~mask) + (va & mask);

	DBG("mmuphysaddr: l %d va %#p pa %#llux\n", l, va, pa);

	return pa;
}

void
mmuinitap(void)
{
	uchar *p;
	Page *pml4;
	PTE *pte;

	assert(m->machno != 0);
	/*
	 * GAK: Has to go when each mach is using
	 * its own page table
	 *
	 * make a copy of the top-level pml4 page into sys->ptroot,
	 * and point our m->pml4 at it.
	 */
	p = UINT2PTR(m->stack);
	p += MACHSTKSZ;				/* p = sys->ptroot */
	memmove(p, UINT2PTR(mach0pml4.va), PTSZ);
	m->pml4 = pml4 = &m->pml4kludge;	/* private Page */
	pml4->va = PTR2UINT(p);
	pml4->pa = PADDR(p);
	pml4->daddr = mach0pml4.daddr;	/* # of user mappings in pml4 */
	if(pml4->daddr){
		memset(p, 0, pml4->daddr*sizeof(PTE));
		pml4->daddr = 0;
	}
	/* point to new pml4 in pml4 (for VPT) */
	pte = (PTE*)p;
	pte[PTLX(KSEG1PML4, Npglvls-1)] = pml4->pa|PteRW|PteP;

	wrmsr(Efer, rdmsr(Efer) | Nxe);
	cr3put(pml4->pa);
	DBG("mach%d: %#p pml4 %#p\n", m->machno, m, pml4);
}

void
mmuinit(void)
{
	int l, lvl;
	PTE *pte, *pdp;
	Page *page;
	uintptr pml4;
	u64int o, pa, sz;

	archmmu();
	DBG("mach%d: %#p npgsz %d\n", m->machno, m, m->npgsz);
	if(m->machno != 0) {
		mmuinitap();
		return;
	}

	/* we are cpu0 */
	page = &mach0pml4;
	page->pa = cr3get();
	page->va = PTR2UINT(sys->pml4);

	m->pml4 = page;

	wrmsr(Efer, rdmsr(Efer) | Nxe);

	/*
	 * Set up the various kernel memory allocator limits:
	 * pmstart/pmend bound the unused physical memory;
	 * vmstart/vmend bound the total possible VM used by the kernel;
	 * vmunused is the highest virtual address currently mapped
	 * and used by the kernel;
	 * vmunmapped is the highest virtual address currently mapped
	 * by the kernel.
	 * Vmunused can be bumped up to vmunmapped before more
	 * physical memory needs to be allocated and mapped.
	 *
	 * This is set up here so meminit can map appropriately.
	 */
	o = sys->pmstart;
	sz = ROUNDUP(o, 4*MB) - o;	/* gap from kernel end to next 4MB */
	pa = asmalloc(0, sz, AsmMEMORY, 0);
	if(pa == 0)
		panic("mmuinit: asmalloc for gap after kernel failed");
	if(pa != o)			/* sanity check */
		panic("mmuinit: pa %#llux memstart %#llux", pa, o);
	sys->pmstart += sz;

	sys->vmstart = KSEG0;
	sys->vmunused = sys->vmstart + ROUNDUP(o, PGSZ); /* page after kernel */
	sys->vmunmapped = sys->vmstart + o + sz;	/* next 4MB */
	/* round up for meminit on vbox, else it may blow an assertion */
	/* end of kernel data */
	sys->vmend = ROUNDUP(sys->vmstart + kernmem, PGLSZ(1));

	DBG("mmuinit: kseg0: vmstart %#p vmunused %#p\n"
		"\tvmunmapped %#p vmend %#p\n",
		sys->vmstart, sys->vmunused, sys->vmunmapped, sys->vmend);
	DBG("\tKSEG1PML4 %#p\n", KSEG1PML4);

	/*
	 * Set up the map for PD entry access by inserting
	 * the relevant PDP entry into the PD. It's equivalent
	 * to PADDR(sys->pd2g)|PteRW|PteP.
	 *
	 * Change code that uses this to use the KSEG1PML4
	 * map below.
	 */
	pdp = sys->pd2g;	/* Sys; see l32p.s, dat.h; was sys->pd */
	pdp[PDX(PDMAP)] = sys->pdp[PDPX(PDMAP)] & ~(PteD|PteA);
	DBG("pdget(%#llux) = %llux, PADDR(%#p) = %#llux\n",
		PDMAP, pdeget(PDMAP), pdp, PADDR(pdp));
//	assert((pdeget(PDMAP) & ~(PteD|PteA)) == (PADDR(pdp)|PteRW|PteP));

	/*
	 * Set up the map for PTE access by inserting
	 * the relevant PML4 into itself.
	 * Note: outwith level 0, PteG is MBZ on AMD processors,
	 * is 'Reserved' on Intel processors, and the behaviour
	 * can be different.
	 */
	pml4 = cr3get();
	sys->pml4[PTLX(KSEG1PML4, Npglvls-1)] = pml4|PteRW|PteP;
	cr3put(m->pml4->pa);

	for (lvl = Npglvls-1; lvl >= 0; lvl--)
		if((l = mmuwalk(KZERO, lvl, &pte, nil)) >= 0) {
			DBG("l %d %#p %#llux\n", l, pte, *pte);
			USED(l);
		}

	mmuphysaddr(PTR2UINT(end));
}

/*
 * create an identity map for the first 1 GB.  the relevant page tables
 * may be currently set for a user process, rather than unused; just
 * overwrite them.
 */
void
mmuidentitymap(void)
{
	int i;
	Page *pml4;
	PTE *pml4pte;

	mmuflush();
	pml4 = m->pml4;
	pml4pte = (PTE *)pml4->va;
	pml4pte[PTLX(0, 3)]  = PADDR(sys->pdp)|PteRW|PteP;	/* 512GB */
	sys->pdp[PTLX(0, 2)] = PADDR(sys->pd )|PteRW|PteP;	/* 1GB */
	for (i = 0; i < PTSZ/sizeof(PTE); i++)
		sys->pd[PTLX(i*2*MB, 1)] = (i*2*MB)|PtePS|PteRW|PteP;
	coherence();
	cr3put(pml4->pa);
}
