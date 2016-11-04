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

#include "encoding.h"
#include "mmu.h"

#undef DBGFLG
#define DBGFLG 32

/* this gets pretty messy. RV64 has *at least* two modes:
 * 4 level and 3 level page tables. And people wonder why
 * I like soft TLB so much. Anyway, for now, not sure
 * how to handle it.
 * Would be cool to work out a way to Do The Right Thing
 * without regard to page size, so that's what I'm going to
 * try to do.
 */
void msg(char *);
/*
 * To do:
 *	PteNX;
 *	mmukmapsync grot for >1 processor;
 *	mmuptcopy (PteSHARED trick?);
 *	calculate and map up to TMFM (conf crap);
 */

/* strike off 2M so it won't wrap to 0. Sleazy. */
#define TMFM		(2*GiB-2*MiB)		/* kernel memory */

#define PPN(x)		((x)&~(PGSZ-1))

#if 0
/* Print the page table structures to the console */
void print_page_table(void) {
	print_page_table_at((void *)(read_csr(sptbr) << RISCV_PGSHIFT), 0, 0);
}
#endif

void flush_tlb(void)
{
	asm volatile("sfence.vm");
}

size_t pte_ppn(uint64_t pte)
{
	return pte >> PTE_PPN_SHIFT;
}

uint64_t ptd_create(uintptr_t ppn)
{
	return (ppn << PTE_PPN_SHIFT) | PTE_V;
}

uint64_t pte_create(uintptr_t ppn, int prot, int user)
{
	uint64_t pte = (ppn << PTE_PPN_SHIFT) | PTE_R | PTE_V;
	if (prot & PTE_W)
		pte |= PTE_W;
	if (prot & PTE_X)
		pte |= PTE_X;
	if (user)
		pte |= PTE_U;
	return pte;
}

void
rootput(uintptr_t root)
{
	uintptr_t ptbr = root >> RISCV_PGSHIFT;
	write_csr(sptbr, ptbr);

}
void
mmuflushtlb(uint64_t u)
{

	machp()->tlbpurge++;
	if(machp()->MMU.pml4->daddr){
		memset(UINT2PTR(machp()->MMU.pml4->va), 0, machp()->MMU.pml4->daddr*sizeof(PTE));
		machp()->MMU.pml4->daddr = 0;
	}
	rootput((uintptr_t) machp()->MMU.pml4->pa);
}

void
mmuflush(void)
{
	Proc *up = externup();
	Mpl pl;

	pl = splhi();
	up->newtlb = 1;
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
	pte = UINT2PTR(KADDR(pa));
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
	print("pml4 %#llx\n", machp()->MMU.pml4->pa);
	if(0)dumpptepg(4, machp()->MMU.pml4->pa);
}

void
dumpmmuwalk(uint64_t addr)
{
	int l;
	PTE *pte, *pml4;

	pml4 = UINT2PTR(machp()->MMU.pml4->va);
	if((l = mmuwalk(pml4, addr, 3, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 2, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 1, &pte, nil)) >= 0)
		print("cpu%d: mmu l%d pte %#p = %llx\n", machp()->machno, l, pte, *pte);
	if((l = mmuwalk(pml4, addr, 0, &pte, nil)) >= 0)
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

	//tssrsp0(machp(), STACKALIGN(PTR2UINT(proc->kstack+KSTACK)));
	rootput((uintptr_t) machp()->MMU.pml4->pa);
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
	if(proc->MMU.mmuptp[0] && pga.rend.l.p)
		wakeup(&pga.rend);
	proc->MMU.mmuptp[0] = nil;

	panic("tssrsp0");
	//tssrsp0(machp(), STACKALIGN(machp()->stack+MACHSTKSZ));
	rootput(machp()->MMU.pml4->pa);
}

static void
checkpte(uintmem ppn, void *a)
{
	int l;
	PTE *pte, *pml4;
	uint64_t addr;
	char buf[240], *s;

	addr = PTR2UINT(a);
	pml4 = UINT2PTR(machp()->MMU.pml4->va);
	pte = 0;
	s = buf;
	*s = 0;
	if((l = mmuwalk(pml4, addr, 3, &pte, nil)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	s = seprint(buf, buf+sizeof buf,
		"check3: l%d pte %#p = %llx\n",
		l, pte, pte?*pte:~0);
	if((l = mmuwalk(pml4, addr, 2, &pte, nil)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	s = seprint(s, buf+sizeof buf,
		"check2: l%d  pte %#p = %llx\n",
		l, pte, pte?*pte:~0);
	if(*pte&PteFinal)
		return;
	if((l = mmuwalk(pml4, addr, 1, &pte, nil)) < 0 || (*pte&PteP) == 0)
		goto Panic;
	seprint(s, buf+sizeof buf,
		"check1: l%d  pte %#p = %llx\n",
		l, pte, pte?*pte:~0);
	return;
Panic:

	seprint(s, buf+sizeof buf,
		"checkpte: l%d addr %#p ppn %#llx kaddr %#p pte %#p = %llx",
		l, a, ppn, KADDR(ppn), pte, pte?*pte:~0);
	print("%s\n", buf);
	seprint(buf, buf+sizeof buf, "start %#llx unused %#llx"
		" unmap %#llx end %#llx\n",
		sys->vmstart, sys->vmunused, sys->vmunmapped, sys->vmend);
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
		panic("mmuput: wrong attr bits: %#x\n", attr);
	if(attr&PTEVALID)
		flags |= PteP;
	if(attr&PTEWRITE)
		flags |= PteRW;
	if(attr&PTEUSER)
		flags |= PteU;
	/* Can't do this -- what do we do?
	if(attr&PTEUNCACHED)
		flags |= PtePCD;
	*/
	if(attr&PTENOEXEC)
		flags &= ~PteX;
	return flags;
}

void
invlpg(uintptr_t _)
{
	panic("invlpage");
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
	assert(pg->pgszi >= 0);
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
			*pte |= attr & PteFinal | PteP;
			break;
		default:
			panic("mmuput: user pages must be 2M or 1G");
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

/* mmuwalk will walk the page tables as far as we ask (level)
 * or as far as possible (you might hit a tera/giga/mega PTE).
 * If it gets a valid PTE it will return it in ret; test for
 * validity by testing PetP. To see how far it got, check
 * the return value. */
int
mmuwalk(PTE* pml4, uintptr_t va, int level, PTE** ret,
	uint64_t (*alloc)(usize))
{
	int l;
	uintmem pa;
	PTE *pte;

	Mpl pl;
msg("mmuwalk\n");
	pl = splhi();
msg("post splihi\n");
	if(DBGFLG > 1) {
		print("mmuwalk%d: va %#p level %d\n", machp()->machno, va, level);
		print("PTLX(%p, 3) is 0x%x\n", va, PTLX(va,3));
		print("pml4 is %p\n", pml4);
	}
	pte = &pml4[PTLX(va, 3)];
	print("pte is %p\n", pte);
	print("*pte is %p\n", *pte);
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
		else if(*pte & PteFinal)
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
	int l;
	PTE *pte;
	uintmem mask, pa;

msg("mmyphysaddr\n");
	/*
	 * Given a VA, find the PA.
	 * This is probably not the right interface,
	 * but will do as an experiment. Usual
	 * question, should va be void* or uintptr?
	 */
	print("machp() %p \n", machp());
	print("mahcp()->MMU.pml4 %p\n", machp()->MMU.pml4);
	print("... va  %p\n", machp()->MMU.pml4->va);
	l = mmuwalk(UINT2PTR(machp()->MMU.pml4->va), va, 0, &pte, nil);
	print("physaddr: va %#p l %d\n", va, l);
	if(l < 0)
		return ~0;

	mask = PGLSZ(l)-1;
	pa = (*pte & ~mask) + (va & mask);

	print("physaddr: l %d va %#p pa %#llx\n", l, va, pa);

	return pa;
}

/* this is a hack, yes, but it will be necessary. */
static PTE fakepml4[512] __attribute((aligned(4096)));

/* to accomodate the weirdness of the rv64 modes, we're going to leave it as a 4
 * level PT, and fake up the PML4 with one entry when it's 3 levels. Later, we want
 * to be smarter, but a lot of our code is pretty wired to assume 4 level PT and I'm
 * not wanting to just rip it all out. */
void
mmuinit(void)
{
	uint8_t *p;
	uint64_t o, pa, sz, n;

	n = archmmu();
	print("%d page sizes\n", n);
	print("mach%d: %#p pml4 %#p npgsz %d\n", machp()->machno, machp(), machp()->MMU.pml4, sys->npgsz);

	if(machp()->machno != 0){
		/* NIX: KLUDGE: Has to go when each mach is using
		 * its own page table
		 */
		p = UINT2PTR(machp()->stack);
		p += MACHSTKSZ;
		panic("not yet");
#if 0
		memmove(p, UINT2PTR(mach0pml4.va), PTSZ);
		machp()->MMU.pml4 = &machp()->MMU.pml4kludge;
		machp()->MMU.pml4->va = PTR2UINT(p);
		machp()->MMU.pml4->pa = PADDR(p);
		machp()->MMU.pml4->daddr = mach0pml4.daddr;	/* # of user mappings in pml4 */

		rootput(machp()->MMU.pml4->pa);
		print("m %#p pml4 %#p\n", machp(), machp()->MMU.pml4);
#endif
		return;
	}
	
	machp()->MMU.pml4 = &sys->pml4;

	uintptr_t PhysicalRoot = read_csr(sptbr)<<12;
	PTE *root = KADDR(PhysicalRoot);
	PTE *KzeroPTE;
	/* As it happens, as this point, we don't know the number of page table levels.
	 * But a walk to "level 4" will work even if it's only 3, and we can use that
	 * information to know what to do. Further, KSEG0 is the last 2M so this will
	 * get us the last PTE on either an L3 or L2 pte page */
	int l;
	if((l = mmuwalk(root, KSEG0, 3, &KzeroPTE, nil)) < 0) {
		panic("Can't walk to PtePML2");
	}
	int PTLevels = (*KzeroPTE>>9)&3;
	switch(PTLevels) {
	default:
		panic("unsupported number of page table levels: %d", PTLevels);
		break;
	case 1:
		/* nothing to do, we're 4 levels */
		machp()->MMU.pml4->pa = PhysicalRoot;
		print("sptbr is 0x%x\n", PhysicalRoot);
		machp()->MMU.pml4->va = PTR2UINT(root);
		break;
	case 0:
		fakepml4[511] = ((PhysicalRoot>>12)<<10)|0xf;
		machp()->MMU.pml4->pa = PADDR(fakepml4);
		print("fakeroot is 0x%x\n", PADDR(fakepml4));
		machp()->MMU.pml4->va = (uintptr_t)fakepml4;
		/* fake up a level 4 page table. */
	}

	print("mach%d: %#p pml4 %#p npgsz %d\n", machp()->machno, machp(), machp()->MMU.pml4, sys->npgsz);

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
print("sys->pmstart is %p\n", o);
	sz = ROUNDUP(o, 4*MiB) - o;
print("Size is 0x%x\n", sz);
	pa = asmalloc(0, sz, 1, 0);
	if(pa != o)
		panic("mmuinit: pa %#llx memstart %#llx\n", pa, o);
	sys->pmstart += sz;

	sys->vmstart = KSEG0;
print("Going to set vmunused to %p + 0x%x\n", sys->vmstart, ROUNDUP(o, 4*KiB));
	/* more issues with arithmetic since physmem is at 80000000 */
	o &= 0x7fffffff;
	sys->vmunused = sys->vmstart + ROUNDUP(o, 4*KiB);
	sys->vmend = sys->vmstart + TMFM;

	// on amd64, this was set to just the end of the kernel, because
	// only that much was mapped, and also vmap required a lot of
	// free *address space* (not memory, *address space*) for the
	// vmap functions. vmap was a hack we intended to remove.
	// It's still there. But we can get rid of it on riscv.
	// There's lots more to do but at least vmap is gone,
	// as is the PDMAP hack, which was also supposed to
	// be temporary. 
	// TODO: We get much further now but still
	// die in meminit(). When that's fixed remove
	// this TODO.
	sys->vmunmapped = sys->vmend;

	print("mmuinit: vmstart %#p vmunused %#p vmunmapped %#p vmend %#p\n",
		sys->vmstart, sys->vmunused, sys->vmunmapped, sys->vmend);
	dumpmmuwalk(KZERO);

	mmuphysaddr(PTR2UINT(end));
}
