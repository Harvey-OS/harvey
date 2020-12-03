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
 *	mmukmapsync grot for >1 processor;
 *	mmuptcopy (PteSHARED trick?);
 */

#define	PML4		((uintptr)0xFFFFFFFFFFFFF000ULL)

#define PML4E(va) ((PTE*)((((va)>>39)*sizeof(PTE))|(~0ULL<<12)))
#define PML3E(va) ((PTE*)((((va)>>30)*sizeof(PTE))|(~0ULL<<21)))
#define PML2E(va) ((PTE*)((((va)>>21)*sizeof(PTE))|(~0ULL<<30)))
#define PML1E(va) ((PTE*)((((va)>>12)*sizeof(PTE))|(~0ULL<<39)))

#define PPN(x)		((x)&~(PGSZ-1))

void
mmukflushtlb(void)
{
	cr3put(machp()->MMU.pml4->pa);
}

void
mmuflushtlb(void)
{

	machp()->tlbpurge++;
	// Remove this once per-proc mmuptp's are gone.
	memset(UINT2PTR(machp()->MMU.pml4->va), 0, machp()->MMU.pml4->daddr*sizeof(PTE));
	machp()->MMU.pml4->daddr = 0;
	cr3put(machp()->MMU.pml4->pa);
}

void
mmuflush(void)
{
	Proc *up = externup();
	Mpl pl;

	pl = splhi();
	up->newtlb = 1;
	//print("mmuflush: up = %#P\n", up);
	mmuswitch(up);
	splx(pl);
}

static void
mmuptpfree(Proc* proc, int clear)
{
	int l;
	PTE *pte;
	Page **last, *page;

	for(l = 1; l < 4; l++){
		last = &proc->MMU.mmuptp[l];
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
		*last = proc->MMU.mmuptp[0];
		proc->MMU.mmuptp[0] = proc->MMU.mmuptp[l];
		proc->MMU.mmuptp[l] = nil;
	}

	machp()->MMU.pml4->daddr = 0;
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
	pte = KADDR(pa);
	for(i = 0; i < PTSZ/sizeof(PTE); i++)
		if(pte[i] & PteP){
			tabs(tab);
			print("l%d %#p[%#05x]: %#llx\n", lvl, pa, i, pte[i]);

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
	int i;
	Page *pg;

	print("proc %#p\n", p);
	for(i = 3; i > 0; i--){
		print("mmuptp[%d]:\n", i);
		for(pg = p->MMU.mmuptp[i]; pg != nil; pg = pg->next)
			print("\tpg %#p = va %#llx pa %#llx"
				" daddr %#lx next %#p prev %#p\n",
				pg, pg->va, pg->pa, pg->daddr, pg->next, pg->prev);
	}
	print("pml4 %#llx\n", *(uintmem*)PML4);
	if(0)dumpptepg(4, machp()->MMU.pml4->pa);
}

void
dumpmmuwalk(uint64_t addr)
{
	int l;
	PTE *pte;

	if((l = mmuwalk(addr, 3, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(addr, 2, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(addr, 1, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(addr, 0, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
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
	lock(&mmuptpfreelist.l);
	if((page = mmuptpfreelist.next) != nil){
		mmuptpfreelist.next = page->next;
		mmuptpfreelist.ref--;
		unlock(&mmuptpfreelist.l);

		if(page->ref++ != 0)
			panic("mmuptpalloc ref\n");
		page->prev = page->next = nil;
		memset(UINT2PTR(page->va), 0, PTSZ);

		if(page->pa == 0)
			panic("mmuptpalloc: free page with pa == 0");
		return page;
	}
	unlock(&mmuptpfreelist.l);

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
	PTE *pte;
	Page *page;
	Mpl pl;

	//print("mmuswitch: proc = %#P\n", proc);
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

	if(machp()->MMU.pml4->daddr){
		memset(UINT2PTR(machp()->MMU.pml4->va), 0, machp()->MMU.pml4->daddr*sizeof(PTE));
		machp()->MMU.pml4->daddr = 0;
	}

	pte = UINT2PTR(machp()->MMU.pml4->va);
	for(page = proc->MMU.mmuptp[3]; page != nil; page = page->next){
		pte[page->daddr] = PPN(page->pa)|PteU|PteRW|PteP;
		if(page->daddr >= machp()->MMU.pml4->daddr)
			machp()->MMU.pml4->daddr = page->daddr+1;
		page->prev = machp()->MMU.pml4;
	}

	tssrsp0(machp(), STACKALIGN(PTR2UINT(proc->kstack+KSTACK)));
	cr3put(machp()->MMU.pml4->pa);
	splx(pl);
}

void
mmurelease(Proc* proc)
{
	Page *page, *next;

	mmuptpfree(proc, 0);

	for(page = proc->MMU.mmuptp[0]; page != nil; page = next){
		next = page->next;
		if(--page->ref)
			panic("mmurelease: page->ref %d\n", page->ref);
		lock(&mmuptpfreelist.l);
		page->next = mmuptpfreelist.next;
		mmuptpfreelist.next = page;
		mmuptpfreelist.ref++;
		page->prev = nil;
		unlock(&mmuptpfreelist.l);
	}

	lock(&mmuptpfreelist.l);
	if(--proc->MMU.root->ref)
		panic("mmurelease: proc->MMU.root->ref %d\n", proc->MMU.root->ref);
	proc->MMU.root->prev = nil;
	proc->MMU.root->next = mmuptpfreelist.next;
	mmuptpfreelist.next = proc->MMU.root;
	mmuptpfreelist.ref++;
	unlock(&mmuptpfreelist.l);

	if(proc->MMU.mmuptp[0] && pga.rend.l.p)
		wakeup(&pga.rend);
	proc->MMU.mmuptp[0] = nil;

	tssrsp0(machp(), STACKALIGN(machp()->stack+MACHSTKSZ));
	cr3put(machp()->MMU.pml4->pa);
}

static void
checkpte(uintmem ppn, void *a)
{
	int l;
	PTE *pte;
	uint64_t addr;
	char buf[240], *s;

	addr = PTR2UINT(a);
	pte = 0;
	s = buf;
	*s = 0;
	if((l = mmuwalk(addr, 3, &pte)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	s = seprint(buf, buf+sizeof buf,
		"check3: l%d pte %#p = %llx\n",
		l, pte, pte?*pte:~0);
	if((l = mmuwalk(addr, 2, &pte)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	s = seprint(s, buf+sizeof buf,
		"check2: l%d  pte %#p = %llx\n",
		l, pte, pte?*pte:~0);
	if(*pte&PtePS)
		return;
	if((l = mmuwalk(addr, 1, &pte)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	seprint(s, buf+sizeof buf,
		"check1: l%d  pte %#p = %llx\n",
		l, pte, pte?*pte:~0);
	return;

Panic:
	seprint(s, buf+sizeof buf,
		"checkpte: l%d addr %#p ppn %#llx kaddr %#p pte %#p = %llx",
		l, a, ppn, KADDR(ppn), pte, pte?*pte:~0);
	panic("%s", buf);
}


static void
mmuptpcheck(Proc *proc)
{
	int lvl, npgs, i;
	Page *lp, *p, *pgs[16], *fp;
	uint idx[16];

	if(proc == nil)
		return;
	lp = machp()->MMU.pml4;
	for(lvl = 3; lvl >= 2; lvl--){
		npgs = 0;
		for(p = proc->MMU.mmuptp[lvl]; p != nil; p = p->next){
			for(fp = proc->MMU.mmuptp[0]; fp != nil; fp = fp->next)
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
	for(fp = proc->MMU.mmuptp[0]; fp != nil; fp = fp->next){
		for(i = 0; i < npgs; i++)
			if(pgs[i] == fp)
				panic("ptpcheck: dup free page");
		pgs[npgs++] = fp;
	}
}

static uintmem
pteflags(uint attr)
{
	uintmem flags;

	flags = 0;
	if(attr & ~(PTEVALID|PTEWRITE|PTERONLY|PTEUSER|PTEUNCACHED|PTENOEXEC))
		panic("pteflags: wrong attr bits: %#x\n", attr);
	if(attr&PTEVALID)
		flags |= PteP;
	if(attr&PTEWRITE)
		flags |= PteRW;
	if(attr&PTEUSER)
		flags |= PteU;
	if(attr&PTEUNCACHED)
		flags |= PtePCD;
	if(attr&PTENOEXEC)
		flags |= PteNX;
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
	Proc *up = externup();
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
		snprint(buf, sizeof buf, "cpu%d: up %#p mmuput %#p %#P %#x\n",
			machp()->machno, up, va, pa, attr);
		print("%s", buf);
	}
	if(pg->pgszi < 0)
		panic("mmuput: page size index out of bounds (%d)\n", pg->pgszi);
	pgsz = sys->pgsz[pg->pgszi];
	if(pa & (pgsz-1))
		panic("mmuput: pa offset non zero: %#llx\n", pa);
	pa |= pteflags(attr);

	pl = splhi();
	if(DBGFLG)
		mmuptpcheck(up);
	user = (va < KZERO);
	x = PTLX(va, 3);

	pte = UINT2PTR(machp()->MMU.pml4->va);
	pte += x;
	prev = machp()->MMU.pml4;

	for(lvl = 3; lvl >= 0; lvl--){
		if(user){
			if(pgsz == 2*MiB && lvl == 1)	 /* use 2M */
				break;
			if(pgsz == 1ull*GiB && lvl == 2)	/* use 1G */
				break;
		}
		for(page = up->MMU.mmuptp[lvl]; page != nil; page = page->next)
			if(page->prev == prev && page->daddr == x){
				if(*pte == 0){
					print("mmu: jmk and nemo had fun\n");
					*pte = PPN(page->pa)|PteU|PteRW|PteP;
				}
				break;
			}

		if(page == nil){
			if(up->MMU.mmuptp[0] == nil)
				page = mmuptpalloc();
			else {
				page = up->MMU.mmuptp[0];
				up->MMU.mmuptp[0] = page->next;
			}
			page->daddr = x;
			page->next = up->MMU.mmuptp[lvl];
			up->MMU.mmuptp[lvl] = page;
			page->prev = prev;
			*pte = PPN(page->pa)|PteU|PteRW|PteP;
			if(lvl == 3 && x >= machp()->MMU.pml4->daddr)
				machp()->MMU.pml4->daddr = x+1;
		}
		x = PTLX(va, lvl-1);

		ppn = PPN(*pte);
		if(ppn == 0)
			panic("mmuput: ppn=0 l%d pte %#p = %#P\n", lvl, pte, *pte);

		pte = KADDR(ppn);
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
			panic("mmuput: user pages must be 2M or 1G (pgsz:%d)", pgsz);
		}
	splx(pl);

	if(DBGFLG){
		snprint(buf, sizeof buf, "cpu%d: up %#p new pte %#p = %#llx\n",
			machp()->machno, up, pte, pte?*pte:~0);
		print("%s", buf);
	}

	invlpg(va);			/* only if old entry valid? */
}

#if 0
static Lock mmukmaplock;
#endif
static Lock vmaplock;

int
mmukmapsync(uint64_t va)
{
	USED(va);

	return 0;
}

/*
 * Add kernel mappings for pa -> va for a section of size bytes.
 * Called only after the va range is known to be unoccupied.
 */
uintptr
mmukphysmap(uintmem pa, int attr, usize size)
{
	uintptr_t pae;
	PTE *p4e, *p3e, *p2e, *p1e;
	usize pgsz = PGLSZ(1);
	Page *pg;
	uintptr va;

	if (pa >= PHYSADDRSIZE)
		panic("mapping nonexistent physical address");
	va = (uintptr)KADDR(pa);

	p4e = PML4E(va);
	if(p4e == nil || *p4e == 0)
		panic("mmukphysmap: PML4E for va %#P is missing", va);

	for(pae = pa + size; pa < pae; pa += pgsz){
		p3e = PML3E(va);
		if(p3e == nil)
			panic("mmukphysmap: PML3 for va %#P is missing", va);
		if(*p3e == 0){
			pg = mmuptpalloc();
			if(pg == nil || pg->pa == 0)
				panic("mmukphysmap: PML2 alloc failed for va %#P", va);
			*p3e = pg->pa|PteRW|PteP;
		}
		p2e = PML2E(va);
		if(p2e == nil)
			panic("mmukphysmap: PML2 missing for va %#P", va);

		/*
		 * Check if it can be mapped using a big page,
		 * i.e. is big enough and starts on a suitable boundary.
		 * Assume processor can do it.
		 */
		if(ALIGNED(pa, PGLSZ(1)) && ALIGNED(va, PGLSZ(1)) && (pae-pa) >= PGLSZ(1)){
			PTE entry = pa|attr|PtePS|PteP;
			if(*p2e != 0 && ((*p2e)&~(PGLSZ(1)-1)) != pa)
				panic("mmukphysmap: remapping kernel direct address at va %#P (old PMl2E %#P, new %#P)",
				    va, *p2e, entry);
			*p2e = entry;
			pgsz = PGLSZ(1);
		}else{
			if(*p2e == 0){
				pg = mmuptpalloc();
				if(pg == nil || pg->pa == 0)
					panic("mmukphysmap: PML1 alloc failed for va %#P", va);
				*p2e = pg->pa|PteRW|PteP;
			}
			p1e = PML1E(va);
			if(p1e == nil)
				panic("mmukphysmap: no PML1 for va %#P", va);
			PTE entry = pa|attr|PteP;
			if(*p1e != 0 && ((*p1e)&~(PGLSZ(0)-1)) != pa)
				panic("mmukphysmap: remapping kernel direct address at va %#P (old PMl2E %#P, new %#P)",
				    va, *p2e, entry);
			*p1e = entry;
			pgsz = PGLSZ(0);
		}
		va += pgsz;
	}

	return va;
}

/*
 * KZERO maps low memory.
 * Thus, almost everything in physical memory is already mapped, but
 * there are things that fall in the gap, mostly MMIO regions, where
 * in particular we would like to disable caching.
 * vmap() is required to access them.
 */
void*
vmap(uintptr_t pa, usize size)
{
	uintptr_t va;
	usize o, sz;

	DBG("vmap(%#p, %lu) pc=%#p\n", pa, size, getcallerpc());

	if(machp()->machno != 0 && DBGFLG)
		print("vmap: machp()->machno != 0\n");
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
		print("vmap(0, %lu) pc=%#p\n", size, getcallerpc());
		return nil;
	}
	ilock(&vmaplock);
	va = (uintptr)KADDR(pa);
	if(mmukphysmap(pa, PtePCD|PteRW, sz) < 0){
		iunlock(&vmaplock);
		return nil;
	}
	iunlock(&vmaplock);

	DBG("vmap(%#p, %lu) => %#p\n", pa+o, size, va+o);

	return UINT2PTR(va + o);
}

void
vunmap(void* v, usize size)
{
	uintptr_t va;

	DBG("vunmap(%#p, %lu)\n", v, size);

	if(machp()->machno != 0)
		DBG("vmap: machp()->machno != 0\n");

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
	DBG("vunmap(%#p, %lu)\n", v, size);
}

int
mmuwalk(uintptr_t va, int level, PTE** ret)
{
	PTE *pte;
	Mpl pl;

	pl = splhi();
	if(DBGFLG > 1)
		DBG("mmuwalk%d: va %#p level %d\n", machp()->machno, va, level);
	pte = (PTE *)PML4E(va);
	assert(pte != nil);
	if (level == 3 || !(*pte & PteP)){
		*ret = pte;
		splx(pl);
		return 3;
	}
	pte = (PTE *)PML3E(va);
	if (level == 2 || (!(*pte & PteP) || (*pte & PtePS))){
		*ret = pte;
		splx(pl);
		return 2;
	}
	pte = (PTE *)PML2E(va);
	if (level == 1 || (!(*pte & PteP) || (*pte & PtePS))){
		*ret = pte;
		splx(pl);
		return 1;
	}
	pte = (PTE *)PML1E(va);
	if (level == 0 || (*pte & PteP)){
		*ret = pte;
		splx(pl);
		return 1;
	}
	*ret = nil;
	splx(pl);

	return -1;
}

uintmem
mmuphysaddr(uintptr_t va)
{
	int l;
	PTE *pte;
	uintmem mask, pa;

	/*
	 * Given a VA, find the PA.
	 * This is probably not the right interface,
	 * but will do as an experiment. Usual
	 * question, should va be void* or uintptr?
	 */
	l = mmuwalk(va, 0, &pte);
	DBG("physaddr: va %#p l %d\n", va, l);
	if(l < 0)
		return ~0;

	mask = PGLSZ(l)-1;
	pa = (*pte & ~mask) + (va & mask);

	DBG("physaddr: l %d va %#p pa %#llx\n", l, va, pa);

	return pa;
}

Page mach0pml4;

void
mmuinit(void)
{
	uint8_t *p;
	PTE *rp;
	Page *page;
	uint64_t r;

	archmmu();
	DBG("mach%d: %#p pml4 %#p npgsz %d\n", machp()->machno, machp(), machp()->MMU.pml4, sys->npgsz);

	if(machp()->machno != 0){
		/* NIX: KLUDGE: Has to go when each mach is using
		 * its own page table
		 */
		p = UINT2PTR(machp()->stack);
		p += MACHSTKSZ;

		usize half = PTSZ/(2*sizeof(PTE));
		rp = (PTE*)p;
		memmove(&rp[half], &sys->pml4[half], PTSZ/4 + PTSZ/8);
		rp[PTSZ/sizeof(PTE)-1] = PADDR(p)|PteRW|PteP;
		machp()->MMU.pml4 = &machp()->MMU.pml4kludge;
		machp()->MMU.pml4->va = PTR2UINT(p);
		machp()->MMU.pml4->pa = PADDR(p);
		machp()->MMU.pml4->daddr = 0;

		r = rdmsr(Efer);
		r |= Nxe;
		wrmsr(Efer, r);
		cr3put(machp()->MMU.pml4->pa);
		DBG("m %#p pml4 %#p\n", machp(), machp()->MMU.pml4);
		return;
	}

	page = &mach0pml4;
	page->va = PTR2UINT(sys->pml4);
	page->pa = PADDR(sys->pml4);
	machp()->MMU.pml4 = page;
	rp = (PTE*)page->va;
	if(rp[PTSZ/sizeof(PTE)-1] != (page->pa|PteRW|PteP))
		panic("mmuinit: self-referential pointer for sys->pml4 wrong");
	cr3put(cr3get());

	r = rdmsr(Efer);
	r |= Nxe;
	wrmsr(Efer, r);

	print("mmuinit: KZERO %#p end %#p\n", KZERO, end);
}

void
mmuprocinit(Proc *p)
{
	Page *pg = mmuptpalloc();
	if(pg==nil)
		panic("mmuprocinit: cannot allocate page table for process");
	memmove(UINT2PTR(pg->va+PTSZ/2), UINT2PTR(PML4+PTSZ/2), PTSZ/4+PTSZ/8);
	PTE *ptes = UINT2PTR(pg->va);
	ptes[PTSZ/sizeof(PTE)-1] = pg->pa|PteRW|PteP;
	p->MMU.root = pg;
}
