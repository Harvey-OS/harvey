#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

/*
 *  to avoid mmu and cash flushing, we use the pid register in the MMU
 *  to map all user addresses.  Although there are 64 possible pids, we
 *  can only use 31 because there are only 32 protection domains and we
 *  need one for the kernel.  Pid i is thus associated with domain i.
 *  Domain 0 is used for the kernel.
 */

/* real protection bits */
enum
{
	/* level 1 descriptor bits */
	L1TypeMask=	(3<<0),
	L1Invalid=	(0<<0),
	L1PageTable=	(1<<0),
	L1Section=	(2<<0),
	L1Cached=	(1<<3),
	L1Buffered=	(1<<2),
	L1DomShift=	5,
	L1Domain0=	(0<<L1DomShift),
	L1KernelRO=	(0x0<<10),
	L1KernelRW=	(0x1<<10),
	L1UserRO=	(0x2<<10),
	L1UserRW=	(0x3<<10),
	L1SectBaseMask=	(0xFFF<<20),
	L1PTBaseMask=	(0x3FFFFF<<10),
	
	/* level 2 descriptor bits */
	L2TypeMask=	(3<<0),
	L2SmallPage=	(2<<0),
	L2LargePage=	(1<<0),
	L2Cached=	(1<<3),
	L2Buffered=	(1<<2),
	L2KernelRW=	(0x55<<4),
	L2UserRO=	(0xAA<<4),
	L2UserRW=	(0xFF<<4),
	L2PageBaseMask=	(0xFFFFF<<12),

	/* domain values */
	Dnoaccess=	0,
	Dclient=	1,
	Dmanager=	3,
};

ulong *l1table;
static int mmuinited;

/*
 *  We map all of memory, flash, and the zeros area with sections.
 *  Special use space is mapped on the fly with regmap.
 */
void
mmuinit(void)
{
	ulong a, o;
	ulong *t;

	/* get a prototype level 1 page */
	l1table = xspanalloc(16*1024, 16*1024, 0);
	memset(l1table, 0, 16*1024);

	/* map low mem (I really don't know why I have to do this -- presotto) */
	for(o = 0; o < 1*OneMeg; o += OneMeg)
		l1table[(0+o)>>20] = L1Section | L1KernelRW| L1Domain0 
			| L1Cached | L1Buffered
			| ((0+o)&L1SectBaseMask);

	/* map DRAM */
	for(o = 0; o < DRAMTOP-DRAMZERO; o += OneMeg)
		l1table[(DRAMZERO+o)>>20] = L1Section | L1KernelRW| L1Domain0 
			| L1Cached | L1Buffered
			| ((PHYSDRAM0+o)&L1SectBaseMask);

	/* uncached DRAM */
	for(o = 0; o < UCDRAMTOP-UCDRAMZERO; o += OneMeg)
		l1table[(UCDRAMZERO+o)>>20] = L1Section | L1KernelRW| L1Domain0 
			| ((PHYSDRAM0+o)&L1SectBaseMask);

	/* map zeros area */
	for(o = 0; o < NULLTOP-NULLZERO; o += OneMeg)
		l1table[(NULLZERO+o)>>20] = L1Section | L1KernelRW | L1Domain0
			| L1Cached | L1Buffered
			| ((PHYSNULL0+o)&L1SectBaseMask);

	/* map flash */
	for(o = 0; o < FLASHTOP-FLASHZERO; o += OneMeg)
		l1table[(FLASHZERO+o)>>20] = L1Section | L1KernelRW | L1Domain0
			| ((PHYSFLASH0+o)&L1SectBaseMask);

	/* map peripheral control module regs */
	mapspecial(0x80000000, OneMeg);

	/* map system control module regs */
	mapspecial(0x90000000, OneMeg);

	/*
	 *  double map start of ram to exception vectors
	 */
	a = EVECTORS;
	t = xspanalloc(BY2PG, 1024, 0);
	memset(t, 0, BY2PG);
	l1table[a>>20] = L1PageTable | L1Domain0 | (((ulong)t) & L1PTBaseMask);
	t[(a&0xfffff)>>PGSHIFT] = L2SmallPage | L2KernelRW | (PHYSDRAM0 & L2PageBaseMask);

	mmurestart();

	mmuinited = 1;
}

void
mmurestart(void) {
	/* set up the domain register to cause all domains to obey pte access bits */

	putdac(Dclient);

	/* point to map */
	putttb((ulong)l1table);

	/* enable mmu */
	wbflush();
	mmuinvalidate();
	mmuenable();
	cacheflush();
}

/*
 *  map on request
 */
static void*
_map(ulong pa, int len, ulong zero, ulong top, ulong l1prop, ulong l2prop)
{
	ulong *t;
	ulong va, i, base, end, off, entry;
	int large;
	ulong* rv;

	rv = nil;
	large = len >= 128*1024;
	if(large){
		base = pa & ~(OneMeg-1);
		end = (pa+len-1) & ~(OneMeg-1);
	} else {
		base = pa & ~(BY2PG-1);
		end = (pa+len-1) & ~(BY2PG-1);
	}
	off = pa - base;

	for(va = zero; va < top && base <= end; va += OneMeg){
		switch(l1table[va>>20] & L1TypeMask){
		default:
			/* found unused entry on level 1 table */
			if(large){
				if(rv == nil)
					rv = (ulong*)(va+off);
				l1table[va>>20] = L1Section | l1prop | L1Domain0 |
							(base & L1SectBaseMask);
				base += OneMeg;
				continue;
			} else {

				/* create an L2 page table and keep going */
				t = xspanalloc(BY2PG, 1024, 0);
				memset(t, 0, BY2PG);
				l1table[va>>20] = L1PageTable | L1Domain0 |
							(((ulong)t) & L1PTBaseMask);
			}
			break;
		case L1Section:
			/* if it's already mapped in a one meg area, don't remap */
			entry = l1table[va>>20];
			i = entry & L1SectBaseMask;
			if(pa >= i && (pa+len) <= i + OneMeg)
			if((entry & ~L1SectBaseMask) == (L1Section | l1prop | L1Domain0))
				return (void*)(va + (pa & (OneMeg-1)));
				
			continue;
		case L1PageTable:
			if(large)
				continue;
			break;
		}

		/* here if we're using page maps instead of sections */
		t = (ulong*)(l1table[va>>20] & L1PTBaseMask);
		for(i = 0; i < OneMeg && base <= end; i += BY2PG){
			entry = t[i>>PGSHIFT];

			/* found unused entry on level 2 table */
			if((entry & L2TypeMask) != L2SmallPage){
				if(rv == nil)
					rv = (ulong*)(va+i+off);
				t[i>>PGSHIFT] = L2SmallPage | l2prop | 
						(base & L2PageBaseMask);
				base += BY2PG;
				continue;
			}
		}
	}

	/* didn't fit */
	if(base <= end)
		return nil;
	cacheflush();

	return rv;
}

/* map in i/o registers */
void*
mapspecial(ulong pa, int len)
{
	return _map(pa, len, REGZERO, REGTOP, L1KernelRW, L2KernelRW);
}

/* map add on memory */
void*
mapmem(ulong pa, int len, int cached)
{
	ulong l1, l2;

	if(cached){
		l1 = L1KernelRW|L1Cached|L1Buffered;
		l2 = L2KernelRW|L2Cached|L2Buffered;
	} else {
		l1 = L1KernelRW;
		l2 = L2KernelRW;
	}
	return _map(pa, len, EMEMZERO, EMEMTOP, l1, l2);
}

/* map a virtual address to a physical one */
ulong
mmu_paddr(ulong va)
{
	ulong entry;
	ulong *t;

	entry = l1table[va>>20];
	switch(entry & L1TypeMask){
	case L1Section:
		return (entry & L1SectBaseMask) | (va & (OneMeg-1));
	case L1PageTable:
		t = (ulong*)(entry & L1PTBaseMask);
		va &= OneMeg-1;
		entry = t[va>>PGSHIFT];
		switch(entry & L1TypeMask){
		case L2SmallPage:
			return (entry & L2PageBaseMask) | (va & (BY2PG-1));
		}
	}
	return 0;
}

/* map a physical address to a virtual one */
ulong
findva(ulong pa, ulong zero, ulong top)
{
	int i;
	ulong entry, va;
	ulong start, end;
	ulong *t;

	for(va = zero; va < top; va += OneMeg){
		/* search the L1 entry */
		entry = l1table[va>>20];
		switch(entry & L1TypeMask){
		default:
			return 0;	/* no holes */
		case L1Section:
			start = entry & L1SectBaseMask;
			end = start + OneMeg;
			if(pa >= start && pa < end)
				return va | (pa & (OneMeg-1));
			continue;
		case L1PageTable:
			break;
		}

		/* search the L2 entry */
		t = (ulong*)(l1table[va>>20] & L1PTBaseMask);
		for(i = 0; i < OneMeg; i += BY2PG){
			entry = t[i>>PGSHIFT];

			/* found unused entry on level 2 table */
			if((entry & L2TypeMask) != L2SmallPage)
				break;

			start = entry & L2PageBaseMask;
			end = start + BY2PG;
			if(pa >= start && pa < end)
				return va | (BY2PG*i) | (pa & (BY2PG-1));
		}
	}
	return 0;
}
ulong
mmu_kaddr(ulong pa)
{
	ulong va;

	/* try the easy stuff first (the first case is true most of the time) */
	if(pa >= PHYSDRAM0 && pa <= PHYSDRAM0+(DRAMTOP-DRAMZERO))
		return DRAMZERO+(pa-PHYSDRAM0);
	if(pa >= PHYSFLASH0 && pa <= PHYSFLASH0+(FLASHTOP-FLASHZERO))
		return FLASHZERO+(pa-PHYSFLASH0);
	if(pa >= PHYSNULL0 && pa <= PHYSNULL0+(NULLTOP-NULLZERO))
		return NULLZERO+(pa-PHYSNULL0);

	if(!mmuinited)
		return 0;	/* this shouldn't happen */

	/* walk the map for the special regs and extended memory */
	va = findva(pa, EMEMZERO, EMEMTOP);
	if(va != 0)
		return va;
	return findva(pa, REGZERO, REGTOP);
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 */
ulong
cankaddr(ulong pa)
{
	/*
	 * Is this enough?
	 * We'll find out if anyone still has one
	 * of these...
	 */
	if(pa >= PHYSDRAM0 && pa <= PHYSDRAM0+(DRAMTOP-DRAMZERO))
		return PHYSDRAM0+(DRAMTOP-DRAMZERO) - pa;
	return 0;
}

/*
 *  table to map fault.c bits to physical bits
 */
static ulong mmubits[16] =
{
	[PTEVALID]				L2SmallPage|L2Cached|L2Buffered|L2UserRO,
	[PTEVALID|PTEWRITE]			L2SmallPage|L2Cached|L2Buffered|L2UserRW,
	[PTEVALID|PTEUNCACHED]			L2SmallPage|L2UserRO,
	[PTEVALID|PTEUNCACHED|PTEWRITE]		L2SmallPage|L2UserRW,

	[PTEKERNEL|PTEVALID]			L2SmallPage|L2Cached|L2Buffered|L2KernelRW,
	[PTEKERNEL|PTEVALID|PTEWRITE]		L2SmallPage|L2Cached|L2Buffered|L2KernelRW,
	[PTEKERNEL|PTEVALID|PTEUNCACHED]		L2SmallPage|L2KernelRW,
	[PTEKERNEL|PTEVALID|PTEUNCACHED|PTEWRITE]	L2SmallPage|L2KernelRW,
};

/*
 *  add an entry to the current map
 */
void
putmmu(ulong va, ulong pa, Page *pg)
{
	Page *l2pg;
	ulong *t, *l1p, *l2p;
	int s;

	s = splhi();

	/* clear out the current entry */
	mmuinvalidateaddr(va);

	l2pg = up->l1page[va>>20];
	if(l2pg == nil){
		l2pg = up->mmufree;
		if(l2pg != nil){
			up->mmufree = l2pg->next;
		} else {
			l2pg = auxpage();
			if(l2pg == nil)
				pexit("out of memory", 1);
		}
		l2pg->va = VA(kmap(l2pg));
		up->l1page[va>>20] = l2pg;
		memset((uchar*)(l2pg->va), 0, BY2PG);
	}

	/* always point L1 entry to L2 page, can't hurt */
	l1p = &l1table[va>>20];
	*l1p = L1PageTable | L1Domain0 | (l2pg->pa & L1PTBaseMask);
	up->l1table[va>>20] = *l1p;
	t = (ulong*)l2pg->va;

	/* set L2 entry */
	l2p = &t[(va & (OneMeg-1))>>PGSHIFT];
	*l2p = mmubits[pa & (PTEKERNEL|PTEVALID|PTEUNCACHED|PTEWRITE)]
		| (pa & ~(PTEKERNEL|PTEVALID|PTEUNCACHED|PTEWRITE));

	/*  write back dirty entries - we need this because the pio() in
	 *  fault.c is writing via a different virt addr and won't clean
	 *  its changes out of the dcache.  Page coloring doesn't work
	 *  on this mmu because the virtual cache is set associative
	 *  rather than direct mapped.
	 */
	cachewb();
	if(pg->cachectl[0] == PG_TXTFLUSH){
		/* pio() sets PG_TXTFLUSH whenever a text page has been written */
		icacheinvalidate();
		pg->cachectl[0] = PG_NOFLUSH;
	}

	splx(s);
}

/*
 *  free up all page tables for this proc
 */
void
mmuptefree(Proc *p)
{
	Page *pg;
	int i;

	for(i = 0; i < Nmeg; i++){
		pg = p->l1page[i];
		if(pg == nil)
			continue;
		p->l1page[i] = nil;
		pg->next = p->mmufree;
		p->mmufree = pg;
	}
	memset(p->l1table, 0, sizeof(p->l1table));
}

/*
 *  this is called with palloc locked so the pagechainhead is kosher
 */
void
mmurelease(Proc* p)
{
	Page *pg, *next;

	/* write back dirty cache entries before changing map */
	cacheflush();

	mmuptefree(p);

	for(pg = p->mmufree; pg; pg = next){
		next = pg->next;
		if(--pg->ref)
			panic("mmurelease: pg->ref %d\n", pg->ref);
		pagechainhead(pg);
	}
	if(p->mmufree && palloc.r.p)
		wakeup(&palloc.r);
	p->mmufree = nil;

	memset(l1table, 0, sizeof(p->l1table));
	cachewbregion((ulong)l1table, sizeof(p->l1table));
}

void
mmuswitch(Proc *p)
{
	if(m->mmupid == p->pid && p->newtlb == 0)
		return;
	m->mmupid = p->pid;

	/* write back dirty cache entries and invalidate all cache entries */
	cacheflush();

	if(p->newtlb){
		mmuptefree(p);
		p->newtlb = 0;
	}

	/* move in new map */
	memmove(l1table, p->l1table, sizeof(p->l1table));

	/* make sure map is in memory */
	cachewbregion((ulong)l1table, sizeof(p->l1table));

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
peekmmu(ulong va)
{
	ulong e, d;

	e = l1table[va>>20];
	switch(e & L1TypeMask){
	default:
		iprint("l1: %#p[%#lux] = %#lux invalid\n", l1table, va>>20, e);
		break;
	case L1PageTable:
		iprint("l1: %#p[%#lux] = %#lux pt\n", l1table, va>>20, e);
		va &= OneMeg-1;
		va >>= PGSHIFT;
		e &= L1PTBaseMask;
		d = ((ulong*)e)[va];
		iprint("l2: %#lux[%#lux] = %#lux\n", e, va, d);
		break;
	case L1Section:
		iprint("l1: %#p[%#lux] = %#lux section\n", l1table, va>>20, e);
		break;
	}
}

void
checkmmu(ulong, ulong)
{
}

void
countpagerefs(ulong*, int)
{
}
