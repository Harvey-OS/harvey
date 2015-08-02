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

#include "amd64.h"

/*
 * To do:
 *	PteNX;
 *	mmukmapsync grot for >1 processor;
 *	replace vmap with newer version (no PDMAP);
 *	mmuptcopy (PteSHARED trick?);
 *	calculate and map up to TMFM (conf crap);
 */

#define TMFM		(64*MiB)		/* kernel memory */

#define PPN(x)		((x)&~(PGSZ-1))

void
mmuflushtlb(uint64_t u)
{
//	Proc *up = machp()->externup;

	machp()->tlbpurge++;
	if(machp()->pml4->daddr){
		memset(UINT2PTR(machp()->pml4->va), 0, machp()->pml4->daddr*sizeof(PTE));
		machp()->pml4->daddr = 0;
	}
	cr3put(machp()->pml4->pa);
}

void
mmuflush(void)
{
	Proc *up = machp()->externup;
	Mpl pl;

	pl = splhi();
	up->newtlb = 1;
	mmuswitch(up);
	splx(pl);
}

static void
mmuptpfree(Proc* proc, int clear)
{
//	Proc *up = machp()->externup;
	int l;
	PTE *pte;
	Page **last, *page;

	for(l = 1; l < 4; l++){
		last = &proc->mmuptp[l];
		if(*last == nil)
			continue;
		for(page = *last; page != nil; page = page->next){
//what is right here? 2 or 1?
			if(l <= 2 && clear)
				memset(UINT2PTR(page->va), 0, PTSZ);
			pte = UINT2PTR(page->prev->va);
			pte[page->daddr] = 0;
			last = &page->next;
		}
		*last = proc->mmuptp[0];
		proc->mmuptp[0] = proc->mmuptp[l];
		proc->mmuptp[l] = nil;
	}

	machp()->pml4->daddr = 0;
}

static void
tabs(int n)
{
	int i;

	for(i = 0; i < n; i++)
		print("  ");
}

void
dumpptepg(int lvl, uintptr_t pa)
{
	PTE *pte;
	int tab, i;

	tab = 4 - lvl;
	pte = UINT2PTR(KADDR(pa));
	for(i = 0; i < PTSZ/sizeof(PTE); i++)
		if(pte[i] & PteP){
			tabs(tab);
			print("l%d %#p[%#05x]: %#ullx\n", lvl, pa, i, pte[i]);

			/* skip kernel mappings */
			if((pte[i]&PteU) == 0){
				tabs(tab+1);
				print("...kern...\n");
				continue;
			}
			if(lvl > 2)
				dumpptepg(lvl-1, PPN(pte[i]));
		}
}

void
dumpmmu(Proc *p)
{
//	Proc *up = machp()->externup;
	int i;
	Page *pg;

	print("proc %#p\n", p);
	for(i = 3; i > 0; i--){
		print("mmuptp[%d]:\n", i);
		for(pg = p->mmuptp[i]; pg != nil; pg = pg->next)
			print("\tpg %#p = va %#ullx pa %#ullx"
				" daddr %#ulx next %#p prev %#p\n",
				pg, pg->va, pg->pa, pg->daddr, pg->next, pg->prev);
	}
	print("pml4 %#ullx\n", machp()->pml4->pa);
	if(0)dumpptepg(4, machp()->pml4->pa);
}

void
dumpmmuwalk(uint64_t addr)
{
//	Proc *up = machp()->externup;
	int l;
	PTE *pte, *pml4;

	pml4 = UINT2PTR(machp()->pml4->va);
	if((l = mmuwalk(pml4, addr, 3, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llux\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 2, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llux\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 1, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llux\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 0, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llux\n", machp()->machno, l, pte, *pte);
}

static Page mmuptpfreelist;

static Page*
mmuptpalloc(void)
{
	void* va;
	Page *page;

	/*
	 * Do not really need a whole Page structure,
	 * but it makes testing this out a lot easier.
	 * Could keep a cache and free excess.
	 * Have to maintain any fiction for pexit?
	 */
	lock(&mmuptpfreelist);
	if((page = mmuptpfreelist.next) != nil){
		mmuptpfreelist.next = page->next;
		mmuptpfreelist.ref--;
		unlock(&mmuptpfreelist);

		if(page->ref++ != 0)
			panic("mmuptpalloc ref\n");
		page->prev = page->next = nil;
		memset(UINT2PTR(page->va), 0, PTSZ);

		if(page->pa == 0)
			panic("mmuptpalloc: free page with pa == 0");
		return page;
	}
	unlock(&mmuptpfreelist);

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

	if(page->pa == 0)
		panic("mmuptpalloc: no pa");
	return page;
}

void
mmuswitch(Proc* proc)
{
//	Proc *up = machp()->externup;
	PTE *pte;
	Page *page;
	Mpl pl;

	pl = splhi();
	if(proc->newtlb){
		/*
 		 * NIX: We cannot clear our page tables if they are going to
		 * be used in the AC
		 */
		if(proc->ac == nil)
			mmuptpfree(proc, 1);
		proc->newtlb = 0;
	}

	if(machp()->pml4->daddr){
		memset(UINT2PTR(machp()->pml4->va), 0, machp()->pml4->daddr*sizeof(PTE));
		machp()->pml4->daddr = 0;
	}

	pte = UINT2PTR(machp()->pml4->va);
	for(page = proc->mmuptp[3]; page != nil; page = page->next){
		pte[page->daddr] = PPN(page->pa)|PteU|PteRW|PteP;
		if(page->daddr >= machp()->pml4->daddr)
			machp()->pml4->daddr = page->daddr+1;
		page->prev = machp()->pml4;
	}

	tssrsp0(machp(), STACKALIGN(PTR2UINT(proc->kstack+KSTACK)));
	cr3put(machp()->pml4->pa);
	splx(pl);
}

void
mmurelease(Proc* proc)
{
//	Proc *up = machp()->externup;
	Page *page, *next;

	mmuptpfree(proc, 0);

	for(page = proc->mmuptp[0]; page != nil; page = next){
		next = page->next;
		if(--page->ref)
			panic("mmurelease: page->ref %d\n", page->ref);
		lock(&mmuptpfreelist);
		page->next = mmuptpfreelist.next;
		mmuptpfreelist.next = page;
		mmuptpfreelist.ref++;
		page->prev = nil;
		unlock(&mmuptpfreelist);
	}
	if(proc->mmuptp[0] && pga.r.p)
		wakeup(&pga.r);
	proc->mmuptp[0] = nil;

	tssrsp0(machp(), STACKALIGN(machp()->stack+MACHSTKSZ));
	cr3put(machp()->pml4->pa);
}

static void
checkpte(uintmem ppn, void *a)
{
//	Proc *up = machp()->externup;
	int l;
	PTE *pte, *pml4;
	uint64_t addr;
	char buf[240], *s;

	addr = PTR2UINT(a);
	pml4 = UINT2PTR(machp()->pml4->va);
	pte = 0;
	s = buf;
	*s = 0;
	if((l = mmuwalk(pml4, addr, 3, &pte, nil)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	s = seprint(buf, buf+sizeof buf,
		"check3: l%d pte %#p = %llux\n",
		l, pte, pte?*pte:~0);
	if((l = mmuwalk(pml4, addr, 2, &pte, nil)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	s = seprint(s, buf+sizeof buf,
		"check2: l%d  pte %#p = %llux\n",
		l, pte, pte?*pte:~0);
	if(*pte&PtePS)
		return;
	if((l = mmuwalk(pml4, addr, 1, &pte, nil)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	seprint(s, buf+sizeof buf,
		"check1: l%d  pte %#p = %llux\n",
		l, pte, pte?*pte:~0);
	return;
Panic:

	seprint(s, buf+sizeof buf,
		"checkpte: l%d addr %#p ppn %#ullx kaddr %#p pte %#p = %llux",
		l, a, ppn, KADDR(ppn), pte, pte?*pte:~0);
	print("%s\n", buf);
	seprint(buf, buf+sizeof buf, "start %#ullx unused %#ullx"
		" unmap %#ullx end %#ullx\n",
		sys->vmstart, sys->vmunused, sys->vmunmapped, sys->vmend);
	panic("%s", buf);
}


static void
mmuptpcheck(Proc *proc)
{
//	Proc *up = machp()->externup;
	int lvl, npgs, i;
	Page *lp, *p, *pgs[16], *fp;
	uint idx[16];

	if(proc == nil)
		return;
	lp = machp()->pml4;
	for(lvl = 3; lvl >= 2; lvl--){
		npgs = 0;
		for(p = proc->mmuptp[lvl]; p != nil; p = p->next){
			for(fp = proc->mmuptp[0]; fp != nil; fp = fp->next)
				if(fp == p){
					dumpmmu(proc);
					panic("ptpcheck: using free page");
				}
			for(i = 0; i < npgs; i++){
				if(pgs[i] == p){
					dumpmmu(proc);
					panic("ptpcheck: dup page");
				}
				if(idx[i] == p->daddr){
					dumpmmu(proc);
					panic("ptcheck: dup daddr");
				}
			}
			if(npgs >= nelem(pgs))
				panic("ptpcheck: pgs is too small");
			idx[npgs] = p->daddr;
			pgs[npgs++] = p;
			if(lvl == 3 && p->prev != lp){
				dumpmmu(proc);
				panic("ptpcheck: wrong prev");
			}
		}

	}
	npgs = 0;
	for(fp = proc->mmuptp[0]; fp != nil; fp = fp->next){
		for(i = 0; i < npgs; i++)
			if(pgs[i] == fp)
				panic("ptpcheck: dup free page");
		pgs[npgs++] = fp;
	}
}

static uint
pteflags(uint attr)
{
	uint flags;

	flags = 0;
	if(attr & ~(PTEVALID|PTEWRITE|PTERONLY|PTEUSER|PTEUNCACHED))
		panic("mmuput: wrong attr bits: %#ux\n", attr);
	if(attr&PTEVALID)
		flags |= PteP;
	if(attr&PTEWRITE)
		flags |= PteRW;
	if(attr&PTEUSER)
		flags |= PteU;
	if(attr&PTEUNCACHED)
		flags |= PtePCD;
	return flags;
}

/*
 * pg->pgszi indicates the page size in machp()->pgsz[] used for the mapping.
 * For the user, it can be either 2*MiB or 1*GiB pages.
 * For 2*MiB pages, we use three levels, not four.
 * For 1*GiB pages, we use two levels.
 */
void
mmuput(uintptr_t va, Page *pg, uint attr)
{
	Proc *up = machp()->externup;
	int lvl, user, x, pgsz;
	PTE *pte;
	Page *page, *prev;
	Mpl pl;
	uintmem pa, ppn;
	char buf[80];

	ppn = 0;
	pa = pg->pa;
	if(pa == 0)
		panic("mmuput: zero pa");

	if(DBGFLG){
		snprint(buf, sizeof buf, "cpu%d: up %#p mmuput %#p %#P %#ux\n",
			machp()->machno, up, va, pa, attr);
		print("%s", buf);
	}
	assert(pg->pgszi >= 0);
	pgsz = machp()->pgsz[pg->pgszi];
	if(pa & (pgsz-1))
		panic("mmuput: pa offset non zero: %#ullx\n", pa);
	pa |= pteflags(attr);

	pl = splhi();
	if(DBGFLG)
		mmuptpcheck(up);
	user = (va < KZERO);
	x = PTLX(va, 3);

	pte = UINT2PTR(machp()->pml4->va);
	pte += x;
	prev = machp()->pml4;

	for(lvl = 3; lvl >= 0; lvl--){
		if(user){
			if(pgsz == 2*MiB && lvl == 1)	 /* use 2M */
				break;
			if(pgsz == 1ull*GiB && lvl == 2)	/* use 1G */
				break;
		}
		for(page = up->mmuptp[lvl]; page != nil; page = page->next)
			if(page->prev == prev && page->daddr == x){
				if(*pte == 0){
					print("mmu: jmk and nemo had fun\n");
					*pte = PPN(page->pa)|PteU|PteRW|PteP;
				}
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
			page->next = up->mmuptp[lvl];
			up->mmuptp[lvl] = page;
			page->prev = prev;
			*pte = PPN(page->pa)|PteU|PteRW|PteP;
			if(lvl == 3 && x >= machp()->pml4->daddr)
				machp()->pml4->daddr = x+1;
		}
		x = PTLX(va, lvl-1);

		ppn = PPN(*pte);
		if(ppn == 0)
			panic("mmuput: ppn=0 l%d pte %#p = %#P\n", lvl, pte, *pte);

		pte = UINT2PTR(KADDR(ppn));
		pte += x;
		prev = page;
	}

	if(DBGFLG)
		checkpte(ppn, pte);
	*pte = pa|PteU;

	if(user)
		switch(pgsz){
		case 2*MiB:
		case 1*GiB:
			*pte |= PtePS;
			break;
		default:
			panic("mmuput: user pages must be 2M or 1G");
		}
	splx(pl);

	if(DBGFLG){
		snprint(buf, sizeof buf, "cpu%d: up %#p new pte %#p = %#llux\n",
			machp()->machno, up, pte, pte?*pte:~0);
		print("%s", buf);
	}

	invlpg(va);			/* only if old entry valid? */
}

#if 0
static Lock mmukmaplock;
#endif
static Lock vmaplock;

#define PML4X(v)	PTLX((v), 3)
#define PDPX(v)		PTLX((v), 2)
#define PDX(v)		PTLX((v), 1)
#define PTX(v)		PTLX((v), 0)

int
mmukmapsync(uint64_t va)
{
	USED(va);

	return 0;
}

static PTE
pdeget(uintptr_t va)
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
pdmap(uintptr_t pa, int attr, uintptr_t va, usize size)
{
	uintptr_t pae;
	PTE *pd, *pde, *pt, *pte;
	int pdx, pgsz;
	Page *pg;

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
			if(*pde == 0){
				pg = mmuptpalloc();
				assert(pg != nil && pg->pa != 0);
				*pde = pg->pa|PteRW|PteP;
				memset((PTE*)(PDMAP+pdx*4096), 0, 4096);
			}
			assert(*pde != 0);

			pt = (PTE*)(PDMAP+pdx*4096);
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
static uintptr_t
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

/*
 * KSEG0 maps low memory.
 * KSEG2 maps almost all memory, but starting at an address determined
 * by the address space map (see asm.c).
 * Thus, almost everything in physical memory is already mapped, but
 * there are things that fall in the gap
 * (acpi tables, device memory-mapped registers, etc.)
 * for those things, we also want to disable caching.
 * vmap() is required to access them.
 */
void*
vmap(uintptr_t pa, usize size)
{
//	Proc *up = machp()->externup;
	uintptr_t va;
	usize o, sz;

	DBG("vmap(%#p, %lud) pc=%#p\n", pa, size, getcallerpc(&pa));

	if(machp()->machno != 0)
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
		print("vmap(0, %lud) pc=%#p\n", size, getcallerpc(&pa));
		return nil;
	}
	ilock(&vmaplock);
	if((va = vmapalloc(sz)) == 0 || pdmap(pa, PtePCD|PteRW, va, sz) < 0){
		iunlock(&vmaplock);
		return nil;
	}
	iunlock(&vmaplock);

	DBG("vmap(%#p, %lud) => %#p\n", pa+o, size, va+o);

	return UINT2PTR(va + o);
}

void
vunmap(void* v, usize size)
{
//	Proc *up = machp()->externup;
	uintptr_t va;

	DBG("vunmap(%#p, %lud)\n", v, size);

	if(machp()->machno != 0)
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
mmuwalk(PTE* pml4, uintptr_t va, int level, PTE** ret,
	uint64_t (*alloc)(usize))
{
//	Proc *up = machp()->externup;
	int l;
	uintmem pa;
	PTE *pte;

	Mpl pl;

	pl = splhi();
	if(DBGFLG > 1)
		DBG("mmuwalk%d: va %#p level %d\n", machp()->machno, va, level);
	pte = &pml4[PTLX(va, 3)];
	for(l = 3; l >= 0; l--){
		if(l == level)
			break;
		if(!(*pte & PteP)){
			if(alloc == nil)
				break;
			pa = alloc(PTSZ);
			if(pa == ~0)
				return -1;
			memset(UINT2PTR(KADDR(pa)), 0, PTSZ);
			*pte = pa|PteRW|PteP;
		}
		else if(*pte & PtePS)
			break;
		pte = UINT2PTR(KADDR(PPN(*pte)));
		pte += PTLX(va, l-1);
	}
	*ret = pte;
	splx(pl);
	return l;
}

uintmem
mmuphysaddr(uintptr_t va)
{
//	Proc *up = machp()->externup;
	int l;
	PTE *pte;
	uintmem mask, pa;

	/*
	 * Given a VA, find the PA.
	 * This is probably not the right interface,
	 * but will do as an experiment. Usual
	 * question, should va be void* or uintptr?
	 */
	l = mmuwalk(UINT2PTR(machp()->pml4->va), va, 0, &pte, nil);
	DBG("physaddr: va %#p l %d\n", va, l);
	if(l < 0)
		return ~0;

	mask = PGLSZ(l)-1;
	pa = (*pte & ~mask) + (va & mask);

	DBG("physaddr: l %d va %#p pa %#llux\n", l, va, pa);

	return pa;
}

Page mach0pml4;

void
mmuinit(void)
{
//	Proc *up = machp()->externup;
	uint8_t *p;
	Page *page;
	uint64_t o, pa, r, sz;

	archmmu();
	DBG("mach%d: %#p pml4 %#p npgsz %d\n", machp()->machno, machp(), machp()->pml4, machp()->npgsz);

	if(machp()->machno != 0){
		/* NIX: KLUDGE: Has to go when each mach is using
		 * its own page table
		 */
		p = UINT2PTR(machp()->stack);
		p += MACHSTKSZ;

		memmove(p, UINT2PTR(mach0pml4.va), PTSZ);
		machp()->pml4 = &machp()->pml4kludge;
		machp()->pml4->va = PTR2UINT(p);
		machp()->pml4->pa = PADDR(p);
		machp()->pml4->daddr = mach0pml4.daddr;	/* # of user mappings in pml4 */

		r = rdmsr(Efer);
		r |= Nxe;
		wrmsr(Efer, r);
		cr3put(machp()->pml4->pa);
		DBG("m %#p pml4 %#p\n", machp(), machp()->pml4);
		return;
	}

	page = &mach0pml4;
	page->pa = cr3get();
	page->va = PTR2UINT(KADDR(page->pa));

	machp()->pml4 = page;

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
	 */
	sys->pd[PDX(PDMAP)] = sys->pdp[PDPX(PDMAP)] & ~(PteD|PteA);
	print("sys->pd %#p %#p\n", sys->pd[PDX(PDMAP)], sys->pdp[PDPX(PDMAP)]);
	assert((pdeget(PDMAP) & ~(PteD|PteA)) == (PADDR(sys->pd)|PteRW|PteP));


	dumpmmuwalk(KZERO);

	mmuphysaddr(PTR2UINT(end));
}
