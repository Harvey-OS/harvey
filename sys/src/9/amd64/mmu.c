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

#define PML4X(va) (PTLX(va, 3))
#define PML3X(va) (PTLX(va, 2))
#define PML2X(va) (PTLX(va, 1))
#define PML1X(va) (PTLX(va, 0))

#define PGSZHUGE (PGLSZ(2))
#define PGSZLARGE (PGLSZ(1))

#define PPNHUGE(x) ((x) & ~(PteNX | (PGSZHUGE - 1)))
#define PPNLARGE(x) ((x) & ~(PteNX | (PGSZLARGE - 1)))
#define PPN(x) ((x) & ~(PteNX | (PGSZ - 1)))

void
mmukflushtlb(void)
{
	cr3put(machp()->MMU.pml4->pa);
}

// This actually moves to the kernel page table, clearing the
// user portion.
void
mmuflushtlb(void)
{
	machp()->tlbpurge++;
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
mmuptpunmap(Proc *proc)
{
	Page *page, *next;

	memset(UINT2PTR(proc->MMU.root->va), 0, PTSZ / 2);
	for(next = nil, page = proc->MMU.root->next; page != nil; page = next){
		next = page->next;
		page->daddr = 0;
		memset(UINT2PTR(page->va), 0, PTSZ);
		page->next = proc->MMU.root->prev;
		proc->MMU.root->prev = page;
	}
	proc->MMU.root->next = nil;
}

static void
tabs(int n)
{
	int i;

	for(i = 0; i < n; i++)
		print("  ");
}

void
dumpptepg(int lvl, usize pa)
{
	PTE *pte;
	int tab, i;

	tab = 4 - lvl;
	pte = KADDR(pa);
	for(i = 0; i < PTSZ / sizeof(PTE); i++)
		if(pte[i] & PteP){
			tabs(tab);
			print("l%d %#p[%#05x]: %#llx\n", lvl, pa, i, pte[i]);

			/* skip kernel mappings */
			if((pte[i] & PteU) == 0){
				tabs(tab + 1);
				print("...kern...\n");
				continue;
			}
			if(lvl > 2)
				dumpptepg(lvl - 1, PPN(pte[i]));
		}
}

void
dumpmmu(Proc *p)
{
	int i;
	Page *pg;

	print("proc %#p pml4 %#P is pa %#llx\n", p, p->MMU.root->pa, p->MMU.root->pa);
	for(i = 3; i > 0; i--){
		print("page table pages at level %d:\n", i);
		for(pg = p->MMU.root->next; pg != nil; pg = pg->next){
			if(pg->daddr != i)
				continue;
			print("\tpg %#p = va %#llx pa %#llx"
			      " daddr %#lx next %#p prev %#p\n",
			      pg, pg->va, pg->pa, pg->daddr, pg->next, pg->prev);
		}
	}
	if(0)
		dumpptepg(4, machp()->MMU.pml4->pa);
}

void
dumpmmuwalk(const PTE *pml4, usize addr)
{
	int l;
	const PTE *pte;

	print("cpu%d: pml4 %#p\n", machp()->machno, pml4);
	if((l = mmuwalk(pml4, addr, 3, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 2, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 1, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 0, &pte)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
}

static void *
allocapage(void)
{
	void *p;

	if(1){
		// XXX: Something is blowing away page tables.
		// Reserve some space around them for whatever
		// is messing things up....
		const int npage = 3;
		char *pp = mallocalign(npage * PTSZ, PTSZ, 0, 0);
		assert(pp != nil);
		p = pp + (npage / 2) * PTSZ;
	}else{

		static alignas(4096) unsigned char alloc[16 * MiB];
		static usize offset = 0;

		if(offset >= sizeof(alloc))
			return nil;
		p = alloc + offset;
		offset += PTSZ;
	}

	return p;
}

static Page mmuptpfreelist;

static Page *
mmuptpalloc(void)
{
	void *va;
	Page *page;

	/*
	 * Do not really need a whole Page structure,
	 * but it makes testing this out a lot easier.
	 * Could keep a cache and free excess.
	 * Have to maintain any fiction for pexit?
	 */
	lock(&mmuptpfreelist.l);
	page = mmuptpfreelist.next;
	if(page != nil){
		mmuptpfreelist.next = page->next;
		mmuptpfreelist.ref--;
	}
	unlock(&mmuptpfreelist.l);

	if(page == nil){
		if((page = malloc(sizeof(Page))) == nil)
			panic("mmuptpalloc: Page alloc failed\n");
		//if((va = allocapage()) == nil)
		if((va = mallocalign(4096, 4096, 0, 0)) == nil)
			panic("mmuptpalloc: page table page alloc failed\n");
		page->va = PTR2UINT(va);
		page->pa = PADDR(va);
		page->ref = 0;
	}

	if(page->pa == 0)
		panic("mmuptpalloc: free page with pa == 0");
	if(page->ref++ != 0)
		panic("mmuptpalloc ref\n");
	page->prev = nil;
	page->next = nil;
	memset(UINT2PTR(page->va), 0, PTSZ);

	return page;
}

void
mmuswitch(Proc *proc)
{
	Mpl pl;

	//print("mmuswitch: proc = %#P\n", proc);
	pl = splhi();
	if(proc->newtlb){
		/*
 		 * NIX: We cannot clear our page tables if they are going to
		 * be used in the AC
		 */
		if(proc->ac == nil)
			mmuptpunmap(proc);
		proc->newtlb = 0;
	}

	tssrsp0(machp(), STACKALIGN(PTR2UINT(proc->kstack + KSTACK)));
	cr3put(proc->MMU.root->pa);
	splx(pl);
}

void
mmurelease(Proc *proc)
{
	Page *page, *next;
	int freed = 0;

	mmuptpunmap(proc);
	for(next = nil, page = proc->MMU.root->prev; page != nil; page = next){
		next = page->next;
		if(--page->ref)
			panic("mmurelease: page->ref %d\n", page->ref);
		lock(&mmuptpfreelist.l);
		page->prev = nil;
		page->next = mmuptpfreelist.next;
		mmuptpfreelist.next = page;
		mmuptpfreelist.ref++;
		unlock(&mmuptpfreelist.l);
		freed = 1;
	}

	lock(&mmuptpfreelist.l);
	if(--proc->MMU.root->ref)
		panic("mmurelease: proc->MMU.root->ref %d\n", proc->MMU.root->ref);
	proc->MMU.root->prev = nil;
	proc->MMU.root->next = mmuptpfreelist.next;
	mmuptpfreelist.next = proc->MMU.root;
	mmuptpfreelist.ref++;
	unlock(&mmuptpfreelist.l);

	if(freed && pga.rend.l.p)
		wakeup(&pga.rend);

	tssrsp0(machp(), STACKALIGN(machp()->stack + MACHSTKSZ));
	cr3put(machp()->MMU.pml4->pa);
}

static void
checkpte(const PTE *pml4, u64 ppn, void *a)
{
	int l;
	const PTE *pte;
	u64 addr;
	char buf[240], *s;

	addr = PTR2UINT(a);
	pte = 0;
	s = buf;
	*s = 0;
	if((l = mmuwalk(pml4, addr, 3, &pte)) < 0 || (*pte & PteP) == 0)
		goto Panic;
	s = seprint(buf, buf + sizeof buf,
		    "check3: l%d pte %#p = %llx\n",
		    l, pte, pte ? *pte : ~0);
	if((l = mmuwalk(pml4, addr, 2, &pte)) < 0 || (*pte & PteP) == 0)
		goto Panic;
	s = seprint(s, buf + sizeof buf,
		    "check2: l%d  pte %#p = %llx\n",
		    l, pte, pte ? *pte : ~0);
	if(*pte & PtePS)
		return;
	if((l = mmuwalk(pml4, addr, 1, &pte)) < 0 || (*pte & PteP) == 0)
		goto Panic;
	seprint(s, buf + sizeof buf,
		"check1: l%d  pte %#p = %llx\n",
		l, pte, pte ? *pte : ~0);
	return;

Panic:
	seprint(s, buf + sizeof buf,
		"checkpte: l%d addr %#p ppn %#llx kaddr %#p pte %#p = %llx",
		l, a, ppn, KADDR(ppn), pte, pte ? *pte : ~0);
	panic("%s", buf);
}

static u64
pteflags(u32 attr)
{
	u64 flags;

	flags = 0;
	if(attr & ~(PTEVALID | PTEWRITE | PTERONLY | PTEUSER | PTEUNCACHED | PTENOEXEC))
		panic("pteflags: wrong attr bits: %#x\n", attr);
	if(attr & PTEVALID)
		flags |= PteP;
	if(attr & PTEWRITE)
		flags |= PteRW;
	if(attr & PTEUSER)
		flags |= PteU;
	if(attr & PTEUNCACHED)
		flags |= PtePCD;
	if(attr & PTENOEXEC)
		flags |= PteNX;
	return flags;
}

static PTE
allocptpage(Proc *p, int level)
{
	Page *page;

	if(p->MMU.root->prev == nil)
		p->MMU.root->prev = mmuptpalloc();
	assert(p->MMU.root->prev != nil);
	page = p->MMU.root->prev;
	p->MMU.root->prev = page->next;
	page->daddr = level;
	page->next = p->MMU.root->next;
	p->MMU.root->next = page;

	return PPN(page->pa) | PteU | PteRW | PteP;
}

/*
 * pg->pgszi indicates the page size in machp()->pgsz[] used for the mapping.
 * For the user, it can be either 2*MiB or 1*GiB pages.
 * For 2*MiB pages, we use three levels, not four.
 * For 1*GiB pages, we use two levels.
 */
void
mmuput(usize va, Page *pg, u32 attr)
{
	Proc *up;
	int pgsz;
	PTE *pml4, *p4e, *p3e, *p2e, *p1e, *ptp;
	PTE entry;
	Mpl pl;

	DBG("cpu%d: up %#p mmuput %#p %#P %#x %d\n",
	    machp()->machno, up, va, pg->pa, attr, pgsz);

	pl = splhi();
	up = externup();
	if(up == nil)
		panic("mmuput: no process");
	if(pg->pa == 0)
		panic("mmuput: zero pa");
	if(va >= KZERO)
		panic("mmuput: kernel page\n");
	if(pg->pgszi < 0)
		panic("mmuput: page size index out of bounds (%d)\n", pg->pgszi);
	pgsz = sys->pgsz[pg->pgszi];
	if(pg->pa & (pgsz - 1))
		panic("mmumappage: pa offset non zero: %#llx\n", pg->pa);
	if(0x0000800000000000 <= va && va < KZERO)
		panic("mmumappage: va %#P is non-canonical", va);

	entry = pg->pa | pteflags(attr) | PteU;
	pml4 = UINT2PTR(up->MMU.root->va);
	p4e = &pml4[PML4X(va)];
	if(p4e == nil)
		panic("mmuput: PML4 is nil");
	if(*p4e == 0)
		*p4e = allocptpage(up, 3);
	ptp = KADDR(PPN(*p4e));
	p3e = &ptp[PML3X(va)];
	if(p3e == nil)
		panic("mmuput: PML3 is nil");
	if(pgsz == 1 * GiB){
		*p3e = entry | PtePS;
		splx(pl);
		DBG("cpu%d: up %#p new 1GiB pte %#p = %#llx\n",
		    machp()->machno, up, p3e, *p3e);
		return;
	}
	if(*p3e == 0)
		*p3e = allocptpage(up, 2);
	ptp = KADDR(PPN(*p3e));
	p2e = &ptp[PML2X(va)];
	if(p2e == nil)
		panic("mmuput: PML2 is nil");
	if(pgsz == 2 * MiB){
		*p2e = entry | PtePS;
		splx(pl);
		DBG("cpu%d: up %#p new 2MiB pte %#p = %#llx\n",
		    machp()->machno, up, p2e, *p2e);
		return;
	}
	if(*p2e == 0)
		*p2e = allocptpage(up, 1);
	ptp = KADDR(PPN(*p2e));
	p1e = &ptp[PML1X(va)];
	if(p1e == nil)
		panic("mmuput: PML1 is nil");
	*p1e = entry;
	invlpg(va); /* only if old entry valid? */
	splx(pl);

	DBG("cpu%d: up %#p new pte %#p = %#llx\n",
	    machp()->machno, up, p1e, *p1e);
	if(DBGFLG)
		checkpte(pml4, PPN(pg->pa), p1e);
}

static Lock vmaplock;

int
mmukmapsync(u64 va)
{
	USED(va);

	return 0;
}

static PTE *
mmukpmap4(PTE *pml4, usize va)
{
	PTE p4e = pml4[PML4X(va)];
	if((p4e & PteP) == 0)
		panic("mmukphysmap: PML4E for va %#P is missing", va);
	return KADDR(PPN(p4e));
}

static PTE *
mmukpmap3(PTE *pml3, u64 pa, usize va, PTE attr, usize size)
{
	PTE p3e = pml3[PML3X(va)];

	// Suitable for a huge page?
	if(ALIGNED(pa, PGSZHUGE) &&
	   ALIGNED(va, PGSZHUGE) &&
	   size >= PGSZHUGE &&
	   (p3e == 0 || (p3e & PtePS) == PtePS)){
		if((p3e & PteP) != 0 && PPNHUGE(p3e) != pa)
			panic("mmukphysmap: remapping kernel direct address at va %#P (old PML3E %#P, new %#P)",
			      va, p3e, pa | attr | PtePS | PteP);
		pml3[PML3X(va)] = pa | attr | PtePS | PteP;
		return nil;
	}else if((p3e & (PtePS | PteP)) == (PtePS | PteP)){
		PTE *pml2 = allocapage();
		if(pml2 == nil)
			panic("mmukphysmap: cannot allocate PML2 to splinter");
		PTE entry = p3e & ~(PteD | PteA);
		for(int i = 0; i < PTSZ / sizeof(PTE); i++)
			pml2[i] = entry + i * PGSZLARGE;
		p3e = PADDR(pml2) | PteRW | PteP;
		pml3[PML3X(va)] = p3e;
	}else if((p3e & PteP) == 0){
		PTE *pml2 = allocapage();
		if(pml2 == nil)
			panic("mmukphysmap: cannot allocate PML2");
		p3e = PADDR(pml2) | PteRW | PteP;
		pml3[PML3X(va)] = p3e;
	}

	return KADDR(PPN(p3e));
}

static PTE *
mmukpmap2(PTE *pml2, u64 pa, usize va, PTE attr, usize size)
{
	PTE p2e = pml2[PML2X(va)];

	// Suitable for a large page?
	if(ALIGNED(pa, PGSZLARGE) &&
	   ALIGNED(va, PGSZLARGE) &&
	   size >= PGSZLARGE &&
	   ((p2e & PteP) == 0 || (p2e & (PtePS | PteP)) == (PtePS | PteP))){
		if((p2e & PteP) != 0 && PPNLARGE(p2e) != pa)
			panic("mmukphysmap: remapping kernel direct address at va %#P (old PML2E %#P, new %#P)",
			      va, p2e, pa | attr | PtePS | PteP);
		pml2[PML2X(va)] = pa | attr | PtePS | PteP;
		return nil;
	}else if((p2e & (PtePS | PteP)) == (PtePS | PteP)){
		PTE *pml1 = allocapage();
		if(pml1 == nil)
			panic("mmukphysmap: cannot allocate PML1 to splinter");
		PTE entry = p2e & ~(PtePS | PteD | PteA);
		for(int i = 0; i < PTSZ / sizeof(PTE); i++)
			pml1[i] = entry + i * PGSZ;
		p2e = PADDR(pml1) | PteRW | PteP;
		pml2[PML2X(va)] = p2e;
	}else if((p2e & PteP) == 0){
		PTE *pml1 = allocapage();
		if(pml1 == nil)
			panic("mmukphysmap: cannot allocate PML1");
		p2e = PPN(PADDR(pml1)) | PteRW | PteP;
		pml2[PML2X(va)] = p2e;
	}

	return KADDR(PPN(p2e));
}

static void
mmukpmap1(PTE *pml1, u64 pa, usize va, PTE attr)
{
	PTE p1e = pml1[PML1X(va)];
	if((p1e & PteP) == PteP && PPN(p1e) != pa)
		panic("mmukphysmap: remapping kernel direct address at va %#P (pml1 %#P old %#P new %#P)",
		      va, pml1, p1e, pa | attr | PteP);
	pml1[PML1X(va)] = pa | attr | PteP;
}

/*
 * Add kernel mappings for pa -> va for a section of size bytes.
 * Called only after the va range is known to be unoccupied.
 */
void
mmukphysmap(PTE *pml4, u64 pa, PTE attr, usize size)
{
	usize pgsz = 0;
	Mpl pl;

	if(pa >= PHYSADDRSIZE || (pa + size) >= PHYSADDRSIZE)
		panic("mapping nonexistent physical address");

	pl = splhi();
	for(u64 pae = pa + size; pa < pae; size -= pgsz, pa += pgsz){
		usize va = (usize)KADDR(pa);
		invlpg(va);

		PTE *pml3 = mmukpmap4(pml4, va);
		assert(pml3 != nil);
		PTE *pml2 = mmukpmap3(pml3, pa, va, attr, size);
		if(pml2 == nil){
			pgsz = PGSZHUGE;
			continue;
		}
		PTE *pml1 = mmukpmap2(pml2, pa, va, attr, size);
		if(pml1 == nil){
			pgsz = PGSZLARGE;
			continue;
		}
		mmukpmap1(pml1, pa, va, attr);
		pgsz = PGSZ;
	}

	splx(pl);
}

/*
 * KZERO maps low memory.
 * Thus, almost everything in physical memory is already mapped, but
 * there are things that fall in the gap, mostly MMIO regions, where
 * in particular we would like to disable caching.
 * vmap() is required to access them.
 */
void *
vmap(usize pa, usize size)
{
	usize va;
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
	if(pa + size < 1ull * MiB)
		return KADDR(pa);
	if(pa < 1ull * MiB)
		return nil;

	/*
	 * Might be asking for less than a page.
	 * This should have a smaller granularity if
	 * the page size is large.
	 */
	o = pa % PGSZ;
	pa -= o;
	sz = ROUNDUP(size + o, PGSZ);

	if(pa == 0){
		print("vmap(0, %lu) pc=%#p\n", size, getcallerpc());
		return nil;
	}
	ilock(&vmaplock);
	va = (usize)KADDR(pa);
	mmukphysmap(sys->pml4, pa, PteNX | PtePCD | PteRW, sz);
	iunlock(&vmaplock);

	DBG("vmap(%#p, %lu) => %#p\n", pa + o, size, va + o);

	return UINT2PTR(va + o);
}

void
vunmap(void *v, usize size)
{
	usize va;

	DBG("vunmap(%#p, %lu)\n", v, size);

	if(machp()->machno != 0)
		DBG("vmap: machp()->machno != 0\n");

	/*
	 * See the comments above in vmap.
	 */
	va = PTR2UINT(v);
	if(va >= KZERO && va + size < KZERO + 1ull * MiB)
		return;

	/*
	 * Here will have to deal with releasing any
	 * resources used for the allocation (e.g. page table
	 * pages).
	 */
	DBG("vunmap(%#p, %lu)\n", v, size);
}

int
mmuwalk(const PTE *pml4, usize va, int level, const PTE **ret)
{
	Mpl pl;

	if(DBGFLG > 1)
		DBG("mmuwalk%d: va %#p level %d\n", machp()->machno, va, level);
	if(pml4 == nil)
		panic("mmuwalk with nil pml4");

	pl = splhi();
	const PTE *p4e = &pml4[PML4X(va)];
	*ret = p4e;
	if((*p4e & PteP) == 0){
		splx(pl);
		return -1;
	}
	if(level == 3){
		splx(pl);
		return 3;
	}

	const PTE *pml3 = KADDR(PPN(*p4e));
	const PTE *p3e = &pml3[PML3X(va)];
	*ret = p3e;
	if((*p3e & PteP) == 0){
		splx(pl);
		return -1;
	}
	if(level == 2 || (*p3e & PtePS) == PtePS){
		splx(pl);
		return 2;
	}

	const PTE *pml2 = KADDR(PPN(*p3e));
	const PTE *p2e = &pml2[PML2X(va)];
	*ret = p2e;
	if((*p2e & PteP) == 0){
		splx(pl);
		return -1;
	}
	if(level == 1 || (*p2e & PtePS) == PtePS){
		splx(pl);
		return 1;
	}
	const PTE *pml1 = KADDR(PPN(*p2e));
	const PTE *p1e = &pml1[PML1X(va)];
	*ret = p1e;
	if(level == 0 && (*p1e & PteP) == PteP){
		splx(pl);
		return 0;
	}
	splx(pl);
	return -1;
}

usize
mmuphysaddr(const PTE *pml4, usize va)
{
	int l;
	const PTE *pte;
	u64 mask, pa;

	/*
	 * Given a VA, find the PA.
	 * This is probably not the right interface,
	 * but will do as an experiment. Usual
	 * question, should va be void* or usize?
	 * cross: Since it is unknown whether a mapping
	 * for the virtual address exists in the given
	 * address space, usize is more appropriate.
	 */
	l = mmuwalk(pml4, va, 0, &pte);
	DBG("physaddr: va %#p l %d\n", va, l);
	if(l < 0)
		return ~0;

	mask = PGLSZ(l) - 1;
	pa = (*pte & ~(PteNX | mask)) + (va & mask);

	DBG("physaddr: l %d pte %#P va %#p pa %#llx\n", l, *pte, va, pa);

	return pa;
}

Page mach0pml4;

void
mmuinit(void)
{
	u8 *p;
	Page *page;
	u64 r;

	archmmu();
	DBG("mach%d: %#p pml4 %#p npgsz %d\n", machp()->machno, machp(), machp()->MMU.pml4, sys->npgsz);

	if(machp()->machno != 0){
		/* NIX: KLUDGE: Has to go when each mach is using
		 * its own page table
		 */
		p = UINT2PTR(machp()->stack);
		p += MACHSTKSZ;

		memmove(p + PTSZ / 2, &sys->pml4[PTSZ / (2 * sizeof(PTE))], PTSZ / 4 + PTSZ / 8);
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
	memmove(UINT2PTR(pg->va + PTSZ / 2), UINT2PTR(machp()->MMU.pml4->va + PTSZ / 2), PTSZ / 4 + PTSZ / 8);
	p->MMU.root = pg;
}
