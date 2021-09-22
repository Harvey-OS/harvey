#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "arm.h"

static KMap* kmapp(ulong pa);

#define L1X(va)		FEXT((va), 20, 12)
#define L2X(va)		FEXT((va), 12, 8)
#define L2AP(ap)	l2ap(ap)
#define L1ptedramattrs	soc.l1ptedramattrs
#define L2ptedramattrs	soc.l2ptedramattrs

enum {
	L1lo		= UZERO/MiB,		/* L1X(UZERO)? */
	L1hi		= (USTKTOP+MiB-1)/MiB,	/* L1X(USTKTOP+MiB-1)? */
	L2size		= 256*sizeof(PTE),
	KMAPADDR	= 0xFFF00000,
};

/*
 * Set up initial PTEs for cpu0 (called with mmu off)
 */
void
mmuinit(void *a)
{
	PTE *l1, *l2;
	uintptr pa, pe, va;

	l1 = (PTE*)a;
	l2 = (PTE*)PADDR(L2);

	/*
	 * map ram between KZERO and VIRTIO
	 */
	va = KZERO;
	pe = VIRTPCI - KZERO;
	if(pe > soc.dramsize)
		pe = soc.dramsize;
	for(pa = PHYSDRAM; pa < PHYSDRAM+pe; pa += MiB){
		l1[L1X(va)] = pa|Dom0|L1AP(Krw)|Section|L1ptedramattrs;
		va += MiB;
	}

	/*
	 * identity map first MB of ram so mmu can be enabled
	 */
	l1[L1X(PHYSDRAM)] = PHYSDRAM|Dom0|L1AP(Krw)|Section|L1ptedramattrs;

	/*
	 * map i/o registers 
	 */
	va = VIRTIO;
	for(pa = soc.physio; pa < soc.physio+IOSIZE; pa += MiB){
		l1[L1X(va)] = pa|Dom0|L1AP(Krw)|Section|L1noexec;
		va += MiB;
	}
	pa = soc.armlocal;
	if(pa)
		l1[L1X(va)] = pa|Dom0|L1AP(Krw)|Section|L1noexec;
	/*
	 * pi4 hack: ether and pcie are in segment 0xFD5xxxxx not 0xFE5xxxxx
	 *           gisb is in segment 0xFC4xxxxx not FE4xxxxx
	 */
	va = VIRTIO + 0x500000;
	pa = soc.physio - 0x1000000 + 0x500000;
	l1[L1X(va)] = pa|Dom0|L1AP(Krw)|Section|L1noexec;
	va = VIRTIO + 0x400000;
	pa = soc.physio - 0x2000000 + 0x400000;
	l1[L1X(va)] = pa|Dom0|L1AP(Krw)|Section|L1noexec;
	
	/*
	 * double map exception vectors near top of virtual memory
	 */
	va = HVECTORS;
	l1[L1X(va)] = (uintptr)l2|Dom0|Coarse;
	l2[L2X(va)] = PHYSDRAM|L2AP(Krw)|Small|L2ptedramattrs;
}

void
mmuinit1()
{
	PTE *l1, *l2;
	uintptr va;

	l1 = m->mmul1;

	/*
	 * undo identity map of first MB of ram
	 */
	l1[L1X(PHYSDRAM)] = 0;
	cachedwbtlb(&l1[L1X(PHYSDRAM)], sizeof(PTE));
	mmuinvalidateaddr(PHYSDRAM);

	/*
	 * make a local mapping for highest MB of virtual space
	 * containing kmap area and exception vectors
	 */
	if(m->machno == 0)
		m->kmapl2 = (PTE*)L2;
	else{
		va = HVECTORS;
		m->kmapl2 = l2 = mallocalign(L2size, L2size, 0, 0);
		l1[L1X(va)] = PADDR(l2)|Dom0|Coarse;
		l2[L2X(va)] = PHYSDRAM|L2AP(Krw)|Small|L2ptedramattrs;
		cachedwbtlb(&l1[L1X(va)], sizeof(PTE));
		mmuinvalidateaddr(va);
	}
}

static void
mmul2empty(Proc* proc, int clear)
{
	PTE *l1;
	Page **l2, *page;
	KMap *k;

	l1 = m->mmul1;
	l2 = &proc->mmul2;
	for(page = *l2; page != nil; page = page->next){
		if(clear){
			k = kmap(page);
			memset((void*)VA(k), 0, L2size);
			kunmap(k);
		}
		l1[page->daddr] = Fault;
		l2 = &page->next;
	}
	coherence();
	*l2 = proc->mmul2cache;
	proc->mmul2cache = proc->mmul2;
	proc->mmul2 = nil;
}

static void
mmul1empty(void)
{
	PTE *l1;

	/* clean out any user mappings still in l1 */
	if(m->mmul1lo > 0){
		if(m->mmul1lo == 1)
			m->mmul1[L1lo] = Fault;
		else
			memset(&m->mmul1[L1lo], 0, m->mmul1lo*sizeof(PTE));
		m->mmul1lo = 0;
	}
	if(m->mmul1hi > 0){
		l1 = &m->mmul1[L1hi - m->mmul1hi];
		if(m->mmul1hi == 1)
			*l1 = Fault;
		else
			memset(l1, 0, m->mmul1hi*sizeof(PTE));
		m->mmul1hi = 0;
	}
	if(m->kmapl2 != nil)
		memset(m->kmapl2, 0, NKMAPS*sizeof(PTE));
}

void
mmuswitch(Proc* proc)
{
	int x;
	PTE *l1;
	Page *page;

	if(proc != nil && proc->newtlb){
		mmul2empty(proc, 1);
		proc->newtlb = 0;
	}

	mmul1empty();

	/* move in new map */
	l1 = m->mmul1;
	if(proc != nil){
	  for(page = proc->mmul2; page != nil; page = page->next){
		x = page->daddr;
		l1[x] = PPN(page->pa)|Dom0|Coarse;
		if(x >= L1lo + m->mmul1lo && x < L1hi - m->mmul1hi){
			if(x+1 - L1lo < L1hi - x)
				m->mmul1lo = x+1 - L1lo;
			else
				m->mmul1hi = L1hi - x;
		}
	  }
	  if(proc->nkmap)
		memmove(m->kmapl2, proc->kmaptab, sizeof(proc->kmaptab));
	}

	/* make sure map is in memory */
	/* could be smarter about how much? */
	cachedwbtlb(&l1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));
	if(proc != nil && proc->nkmap)
		cachedwbtlb(m->kmapl2, sizeof(proc->kmaptab));

	/* lose any possible stale tlb entries */
	mmuinvalidate();
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
	cachedwbtlb(&m->mmul1[L1X(UZERO)], (L1hi - L1lo)*sizeof(PTE));

	/* lose any possible stale tlb entries */
	mmuinvalidate();
}

void
putmmu(uintptr va, uintptr pa, Page* page)
{
	int x, s;
	Page *pg;
	PTE *l1, *pte;
	KMap *k;

	/*
	 * disable interrupts to prevent flushmmu (called from hzclock)
	 * from clearing page tables while we are setting them
	 */
	s = splhi();
	x = L1X(va);
	l1 = &m->mmul1[x];
	if(*l1 == Fault){
		/* l2 pages only have 256 entries - wastes 3K per 1M of address space */
		if(up->mmul2cache == nil){
			spllo();
			pg = newpage(0, 0, 0);
			splhi();
			/* if newpage slept, we might be on a different cpu */
			l1 = &m->mmul1[x];
		}else{
			pg = up->mmul2cache;
			up->mmul2cache = pg->next;
		}
		pg->daddr = x;
		pg->next = up->mmul2;
		up->mmul2 = pg;

		/* force l2 page to memory */
		k = kmap(pg);
		memset((void*)VA(k), 0, L2size);
		cachedwbtlb((void*)VA(k), L2size);
		kunmap(k);

		*l1 = PPN(pg->pa)|Dom0|Coarse;
		cachedwbtlb(l1, sizeof *l1);

		if(x >= L1lo + m->mmul1lo && x < L1hi - m->mmul1hi){
			if(x+1 - L1lo < L1hi - x)
				m->mmul1lo = x+1 - L1lo;
			else
				m->mmul1hi = L1hi - x;
		}
	}
	k = kmapp(PPN(*l1));
	pte = (PTE*)VA(k);

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
	cachedwbtlb(&pte[L2X(va)], sizeof(PTE));
	kunmap(k);

	/* clear out the current entry */
	mmuinvalidateaddr(PPN(va));

	if(page->cachectl[m->machno] == PG_TXTFLUSH){
		/* pio() sets PG_TXTFLUSH whenever a text pg has been written */
		if(cankaddr(page->pa))
			cachedwbse((void*)(page->pa|KZERO), BY2PG);
		cacheiinvse((void*)page->va, BY2PG);
		page->cachectl[m->machno] = PG_NOFLUSH;
	}
	//checkmmu(va, PPN(pa));
	splx(s);
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
	*pte &= ~L1ptedramattrs;
	mmuinvalidateaddr(va);
	cachedwbinvse(pte, 4);

	return v;
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 */
uintptr
cankaddr(uintptr pa)
{
	if((pa - PHYSDRAM) < VIRTPCI-KZERO)
		return PHYSDRAM + VIRTPCI-KZERO - pa;
	return 0;
}

uintptr
mmukmap(uintptr va, uintptr pa, usize size)
{
	int o;
	usize n;
	PTE *pte, *pte0;

	assert((va & (MiB-1)) == 0);
	o = pa & (MiB-1);
	pa -= o;
	size += o;
	pte = pte0 = &m->mmul1[L1X(va)];
	for(n = 0; n < size; n += MiB)
		if(*pte++ != Fault)
			return 0;
	pte = pte0;
	for(n = 0; n < size; n += MiB){
		*pte++ = (pa+n)|Dom0|L1AP(Krw)|Section|L1noexec;
		mmuinvalidateaddr(va+n);
	}
	cachedwbtlb(pte0, (uintptr)pte - (uintptr)pte0);
	return va + o;
}

uintptr
mmukmapx(uintptr va, uvlong pa, usize size)
{
	int o;
	usize n;
	PTE ptex, *pte, *pte0;

	assert((va & (16*MiB-1)) == 0);
	assert(size <= 16*MiB);
	o = (int)pa & (16*MiB-1);
	pa -= o;
	ptex = FEXT(pa,24,8)<<24 | FEXT(pa,32,4)<<20 | FEXT(pa,36,4)<<5;
	pte = pte0 = &m->mmul1[L1X(va)];
	for(n = 0; n < 16*MiB; n += MiB)
		if(*pte++ != Fault)
			return 0;
	pte = pte0;
	for(n = 0; n < 16*MiB; n += MiB)
		*pte++ = ptex|L1AP(Krw)|Super|Section|L1noexec;
	mmuinvalidateaddr(va);
	cachedwbtlb(pte0, (uintptr)pte - (uintptr)pte0);
	return va + o;
}

void
checkmmu(uintptr va, uintptr pa)
{
	int x;
	PTE *l1, *pte;
	KMap *k;

	x = L1X(va);
	l1 = &m->mmul1[x];
	if(*l1 == Fault){
		iprint("checkmmu cpu%d va=%lux l1 %p=%ux\n", m->machno, va, l1, *l1);
		return;
	}
	k = kmapp(PPN(*l1));
	pte = (PTE*)VA(k);
	pte += L2X(va);
	if(pa == ~0 || (pa != 0 && PPN(*pte) != pa))
		iprint("checkmmu va=%lux pa=%lux l1 %p=%ux pte %p=%ux\n", va, pa, l1, *l1, pte, *pte);
	kunmap(k);
}

static KMap*
kmapp(ulong pa)
{
	int s, i;
	uintptr va;

	if(cankaddr(pa))
		return KADDR(pa);
	if(up == nil)
		panic("kmap without up %#p", getcallerpc(&pa));
	s = splhi();
	if(up->nkmap == NKMAPS)
		panic("kmap overflow %#p", getcallerpc(&pa));
	for(i = 0; i < NKMAPS; i++)
		if(up->kmaptab[i] == 0)
			break;
	if(i == NKMAPS)
		panic("can't happen");
	up->nkmap++;
	va = KMAPADDR + i*BY2PG;
	up->kmaptab[i] = pa | L2AP(Krw)|Small|L2ptedramattrs;
	m->kmapl2[i] = up->kmaptab[i];
	cachedwbtlb(&m->kmapl2[i], sizeof(PTE));
	mmuinvalidateaddr(va);
	splx(s);
	return (KMap*)va;
}

KMap*
kmap(Page *p)
{
	return kmapp(p->pa);
}

void
kunmap(KMap *k)
{
	int i;
	uintptr va;

	coherence();
	va = (uintptr)k;
	if(L1X(va) != L1X(KMAPADDR))
		return;
	/* wasteful: only needed for text pages aliased within data cache */
	cachedwbse((void*)PPN(va), BY2PG);
	i = L2X(va);
	up->kmaptab[i] = 0;
	m->kmapl2[i] = 0;
	up->nkmap--;
	cachedwbtlb(&m->kmapl2[i], sizeof(PTE));
	mmuinvalidateaddr(va);
}

/*
 * although needed by the pc port, this mapping can be trivial on our arm
 * systems, which have less memory.
 */
void*
vmap(uintptr pa, usize)
{
	return UINT2PTR(kseg0|pa);
}

void
vunmap(void* v, usize size)
{
	/*
	upafree(PADDR(v), size);
	 */
	USED(v, size);
}
