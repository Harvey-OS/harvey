#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"

#define ALIGNED(p, a)	(!(((uintptr)(p)) & ((a)-1)))

#define PDMAP		(0xffffffffff800000ull)
#define PDPX(v)		PTLX((v), 2)
#define PDX(v)		PTLX((v), 1)
#define PTX(v)		PTLX((v), 0)

#define VMAP		(0xffffffffe0000000ull)
#define VMAPSZ		(256*MiB)

#define KSEG1PML4	(0xffff000000000000ull\
			|(PTLX(KSEG1, 3)<<(((3)*PTSHFT)+PGSHFT))\
			|(PTLX(KSEG1, 3)<<(((2)*PTSHFT)+PGSHFT))\
			|(PTLX(KSEG1, 3)<<(((1)*PTSHFT)+PGSHFT))\
			|(PTLX(KSEG1, 3)<<(((0)*PTSHFT)+PGSHFT)))

#define KSEG1PTP(va, l)	((0xffff000000000000ull\
			|(KSEG1PML4<<((3-(l))*PTSHFT))\
			|(((va) & 0xffffffffffffull)>>(((l)+1)*PTSHFT))\
			& ~0xfffull))

static Lock vmaplock;
static Page mach0pml4;

void
mmuflushtlb(u64int)
{
	if(m->pml4->daddr){
		memset(UINT2PTR(m->pml4->va), 0, m->pml4->daddr*sizeof(PTE));
		m->pml4->daddr = 0;
	}
	cr3put(m->pml4->pa);
}

void
mmuflush(void)
{
	int s;

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
	 *	fix memset level for 2MiB pages;
	 *	use a dedicated datastructure rather than Page?
	 */
	for(l = 1; l < 4; l++){
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
	Page *page;
	uintmem pa;
	int color;

	/*
	 * Do not really need a whole Page structure,
	 * but it makes testing this out a lot easier.
	 * Could keep a cache and free excess.
	 */
	if((page = malloc(sizeof(Page))) == nil){
		print("mmuptpalloc Page\n");

		return nil;
	}
	color = NOCOLOR;
	if((pa = physalloc(PTSZ, &color, page)) == 0){
		print("mmuptpalloc pa\n");
		free(page);

		return nil;
	}

	page->va = PTR2UINT(KADDR(pa));
	page->pa = pa;
	page->ref = 1;
	page->color = color;
	memset(UINT2PTR(page->va), 0, PTSZ);

	return page;
}

void
mmuswitch(Proc* proc)
{
	PTE *pte;
	Page *page;

	if(proc->newtlb){
		mmuptpfree(proc, 0);
		proc->newtlb = 0;
	}

	if(m->pml4->daddr){
		memset(UINT2PTR(m->pml4->va), 0, m->pml4->daddr*sizeof(PTE));
		m->pml4->daddr = 0;
	}

	pte = UINT2PTR(m->pml4->va);
	for(page = proc->mmuptp[3]; page != nil; page = page->next){
		pte[page->daddr] = PPN(page->pa)|PteU|PteRW|PteP;
		if(page->daddr >= m->pml4->daddr)
			m->pml4->daddr = page->daddr+1;
		page->prev = m->pml4;
	}

	tssrsp0(STACKALIGN(PTR2UINT(proc->kstack+KSTACK)));
	cr3put(m->pml4->pa);
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
			panic("mmurelease: page->ref %d\n", page->ref);
		physfree(page->pa, PTSZ);
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
	for(l = 3; l >= 0; l--){
		ptp = mmuptpget(va, l);
		x = PTLX(va, l);
		pte = &ptp[x];
		for(page = up->mmuptp[l]; page != nil; page = page->next){
			if(page->prev == prev && page->daddr == x)
				break;
		}
		if(page == nil){
			if(up->mmuptp[0] == nil)
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
			if(l == 3 && x >= m->pml4->daddr)
				m->pml4->daddr = x+1;
		}
		prev = page;
	}

	*pte = pa|PteU;
//if(pa & PteRW)
//  *pte |= PteNX;
	splx(pl);

	invlpg(va);			/* only if old entry valid? */
}

static PTE
pdeget(uintptr va)
{
	PTE *pdp;

	if(va < 0xffffffffc0000000ull)
		panic("pdeget(%#p)", va);

	pdp = (PTE*)(PDMAP+PDX(PDMAP)*4096);

	return pdp[PDX(va)];
}

/*
 * Add kernel mappings for pa -> va for a section of size bytes.
 * Called only after the va range is known to be unoccupied.
 */
static int
pdmap(uintmem pa, int attr, uintptr va, usize size)
{
	uintmem pae;
	PTE *pd, *pde, *pt, *pte;
	uintmem pdpa;
	int pdx, pgsz, color;

	pd = (PTE*)(PDMAP+PDX(PDMAP)*4096);

	for(pae = pa + size; pa < pae; pa += pgsz){
		pdx = PDX(va);
		pde = &pd[pdx];

		/*
		 * Check if it can be mapped using a big page,
		 * i.e. is big enough and starts on a suitable boundary.
		 * Assume processor can do it.
		 */
		if(ALIGNED(pa, PGLSZ(1)) && ALIGNED(va, PGLSZ(1)) && (pae-pa) >= PGLSZ(1)){
			assert(*pde == 0);
			*pde = pa|attr|PtePS|PteP;
			pgsz = PGLSZ(1);
		}
		else{
			pt = (PTE*)(PDMAP+pdx*PTSZ);
			if(*pde == 0){
				color = NOCOLOR;
				pdpa = physalloc(PTSZ, &color, nil);
				if(pdpa == 0)
					panic("pdmap");
				*pde = pdpa|PteRW|PteP;
				memset(pt, 0, PTSZ);
			}

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
vmapalloc(usize size)
{
	int i, n, o;
	PTE *pd, *pt;
	int pdsz, ptsz;

	pd = (PTE*)(PDMAP+PDX(PDMAP)*4096);
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

		pt = (PTE*)(PDMAP+(PDX(VMAP)+i)*4096);
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

void*
vmap(uintmem pa, usize size)
{
	uintptr va;
	usize o, sz;

	DBG("vmap(%#P, %lud)\n", pa, size);

	if(m->machno != 0)
		panic("vmap");

	/*
	 * This is incomplete; the checks are not comprehensive
	 * enough.
	 * Sometimes the request is for an already-mapped piece
	 * of low memory, in which case just return a good value
	 * and hope that a corresponding vunmap of the address
	 * will have the same address.
	 * To do this properly will require keeping track of the
	 * mappings; perhaps something like kmap, but kmap probably
	 * can't be used early enough for some of the uses.
	 */
	if(pa+size < 1ull*MiB)
		return KADDR(pa);
	if(pa < 1ull*MiB)
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
		DBG("vmap(0, %lud) pc=%#p\n", size, getcallerpc(&pa));
		return nil;
	}
	ilock(&vmaplock);
	if((va = vmapalloc(sz)) == 0 || pdmap(pa, PtePCD|PteRW, va, sz) < 0){
		iunlock(&vmaplock);
		return nil;
	}
	iunlock(&vmaplock);

	DBG("vmap(%#P, %lud) => %#p\n", pa+o, size, va+o);

	return UINT2PTR(va + o);
}

void
vunmap(void* v, usize size)
{
	uintptr va;

	DBG("vunmap(%#p, %lud)\n", v, size);

	if(m->machno != 0)
		panic("vunmap");

	/*
	 * See the comments above in vmap.
	 */
	va = PTR2UINT(v);
	if(va >= KZERO && va+size < KZERO+1ull*MiB)
		return;

	/*
	 * Here will have to deal with releasing any
	 * resources used for the allocation (e.g. page table
	 * pages).
	 */
	DBG("vunmap(%#p, %lud)\n", v, size);
}

int
mmuwalk(uintptr va, int level, PTE** ret, u64int (*alloc)(usize))
{
//alloc and pa - uintmem or PTE or what?
	int l;
	Mpl pl;
	uintptr pa;
	PTE *pte, *ptp;

	DBG("mmuwalk%d: va %#p level %d\n", m->machno, va, level);
	pte = nil;
	pl = splhi();
	for(l = 3; l >= 0; l--){
		ptp = mmuptpget(va, l);
		pte = &ptp[PTLX(va, l)];
		if(l == level)
			break;
		if(!(*pte & PteP)){
			if(alloc == nil)
				break;
			pa = alloc(PTSZ);
			if(pa == ~0)
				return -1;
if(pa & 0xfffull) print("mmuwalk pa %#llux\n", pa);
			*pte = pa|PteRW|PteP;
			if((ptp = mmuptpget(va, l-1)) == nil)
				panic("mmuwalk: mmuptpget(%#p, %d)\n", va, l-1);
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
		return ~0;

	mask = (1ull<<(((l)*PTSHFT)+PGSHFT))-1;
	pa = (*pte & ~mask) + (va & mask);

	DBG("mmuphysaddr: l %d va %#p pa %#llux\n", l, va, pa);

	return pa;
}

void
mmuinit(void)
{
	int l;
	uchar *p;
	PTE *pte;
	Page *page;
	uintptr pml4;
	u64int o, pa, r, sz;

	archmmu();
	DBG("mach%d: %#p npgsz %d\n", m->machno, m, m->npgsz);
	if(m->machno != 0){
		/*
		 * GAK: Has to go when each mach is using
		 * its own page table
		 */
		p = UINT2PTR(m->stack);
		p += MACHSTKSZ;
		memmove(p, UINT2PTR(mach0pml4.va), PTSZ);
		m->pml4 = &m->pml4kludge;
		m->pml4->va = PTR2UINT(p);
		m->pml4->pa = PADDR(p);
		m->pml4->daddr = mach0pml4.daddr;	/* # of user mappings in pml4 */
		if(m->pml4->daddr){
			memset(p, 0, m->pml4->daddr*sizeof(PTE));
			m->pml4->daddr = 0;
		}
pte = (PTE*)p;
pte[PTLX(KSEG1PML4, 3)] = m->pml4->pa|PteRW|PteP;

		r = rdmsr(Efer);
		r |= Nxe;
		wrmsr(Efer, r);
		cr3put(m->pml4->pa);
		DBG("mach%d: %#p pml4 %#p\n", m->machno, m, m->pml4);
		return;
	}

	page = &mach0pml4;
	page->pa = cr3get();
	page->va = PTR2UINT(sys->pml4);

	m->pml4 = page;

	r = rdmsr(Efer);
	r |= Nxe;
	wrmsr(Efer, r);

	/*
	 * Set up the various kernel memory allocator limits:
	 * pmstart/pmend bound the unused physical memory;
	 * vmstart/vmend bound the total possible virtual memory
	 * used by the kernel;
	 * vmunused is the highest virtual address currently mapped
	 * and used by the kernel;
	 * vmunmapped is the highest virtual address currently
	 * mapped by the kernel.
	 * Vmunused can be bumped up to vmunmapped before more
	 * physical memory needs to be allocated and mapped.
	 *
	 * This is set up here so meminit can map appropriately.
	 */
	o = sys->pmstart;
	sz = ROUNDUP(o, 4*MiB) - o;
	pa = asmalloc(0, sz, 1, 0);
	if(pa != o)
		panic("mmuinit: pa %#llux memstart %#llux\n", pa, o);
	sys->pmstart += sz;

	sys->vmstart = KSEG0;
	sys->vmunused = sys->vmstart + ROUNDUP(o, 4*KiB);
	sys->vmunmapped = sys->vmstart + o + sz;
	sys->vmend = sys->vmstart + TMFM;

	print("mmuinit: vmstart %#p vmunused %#p vmunmapped %#p vmend %#p\n",
		sys->vmstart, sys->vmunused, sys->vmunmapped, sys->vmend);

	/*
	 * Set up the map for PD entry access by inserting
	 * the relevant PDP entry into the PD. It's equivalent
	 * to PADDR(sys->pd)|PteRW|PteP.
	 *
	 * Change code that uses this to use the KSEG1PML4
	 * map below.
	 */
	sys->pd[PDX(PDMAP)] = sys->pdp[PDPX(PDMAP)] & ~(PteD|PteA);
	print("sys->pd %#p %#p\n", sys->pd[PDX(PDMAP)], sys->pdp[PDPX(PDMAP)]);

	assert((pdeget(PDMAP) & ~(PteD|PteA)) == (PADDR(sys->pd)|PteRW|PteP));

	/*
	 * Set up the map for PTE access by inserting
	 * the relevant PML4 into itself.
	 * Note: outwith level 0, PteG is MBZ on AMD processors,
	 * is 'Reserved' on Intel processors, and the behaviour
	 * can be different.
	 */
	pml4 = cr3get();
	sys->pml4[PTLX(KSEG1PML4, 3)] = pml4|PteRW|PteP;
	cr3put(m->pml4->pa);

	if((l = mmuwalk(KZERO, 3, &pte, nil)) >= 0)
		print("l %d %#p %llux\n", l, pte, *pte);
	if((l = mmuwalk(KZERO, 2, &pte, nil)) >= 0)
		print("l %d %#p %llux\n", l, pte, *pte);
	if((l = mmuwalk(KZERO, 1, &pte, nil)) >= 0)
		print("l %d %#p %llux\n", l, pte, *pte);
	if((l = mmuwalk(KZERO, 0, &pte, nil)) >= 0)
		print("l %d %#p %llux\n", l, pte, *pte);

	mmuphysaddr(PTR2UINT(end));
}

void
mmucachectl(Page *p, uint why)
{
	if(!pagedout(p))
		memset(p->cachectl, why, sizeof(p->cachectl));
}

/*
 * Double-check the user MMU.
 * Error checking only.
 */
void
checkmmu(uintptr va, uintmem pa)
{
	uintmem mpa;

	mpa = mmuphysaddr(va);
	if(mpa != ~(uintmem)0 && mpa != pa)
		print("%d %s: va=%#p pa=%#P mmupa=%#P\n",
			up->pid, up->text, va, pa, mpa);
}
