#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"/sys/src/boot/alphapc/conf.h"

static uvlong	origlvl1;	/* physical address */
static uvlong	klvl2;	/* physical, as created by boot loader */
static uchar	*nextio;	/* next virtual address to be allocated by kmapv */
extern Bootconf *bootconf;

#define LVL2OFF(v)	((((long)(v))>>(2*PGSHIFT-3))&(PTE2PG-1))
#define LVL3OFF(v)	((((long)(v))>>(PGSHIFT))&(PTE2PG-1))

static void
setptb(ulong pa)
{
	m->ptbr = (uvlong)pa>>PGSHIFT;
	swpctx(m);
}

void
mmuinit(void)
{
	uvlong *plvl2;

	/* set PCB to new one in mach structure before stomping on old one */
	m->usp = 0;
	m->fen = 1;
	m->ptbr = bootconf->pcb->ptbr;
	origlvl1 = (m->ptbr << PGSHIFT);
	setpcb(m);

	plvl2 = (uvlong*) (KZERO|origlvl1|(BY2PG-8));
	klvl2 = (*plvl2 >> 32)<<PGSHIFT;

	nextio = (uchar*) (KZERO|bootconf->maxphys);
}

/*
 * Called splhi, not in Running state
 */
void
mmuswitch(Proc *p)
{
	Page *pg;
	uvlong *lvl2;

	if(p->newtlb){
		/*
		 *  newtlb set means that they are inconsistent
		 *  with the segment.c data structures.
		 *
		 *  bin the current 3rd level page tables and
		 *  the pointers to them in the 2nd level page.
		 *  pg->daddr is used by putmmu to save the offset into
		 *  the 2nd level page.
		 */
		if(p->mmutop && p->mmuused){
			lvl2 = (uvlong*)p->mmulvl2->va;
			for(pg = p->mmuused; pg->next; pg = pg->next)
				lvl2[pg->daddr] = 0;
			lvl2[pg->daddr] = 0;
			pg->next = p->mmufree;
			p->mmufree = p->mmuused;
			p->mmuused = 0;
		}
		p->newtlb = 0;
	}

	/* tell processor about new page table and flush cached entries */
	if(p->mmutop == 0)
		setptb(origlvl1);
	else
		setptb(p->mmutop->pa);
	tlbflush(-1, 0);
	icflush();
}

/*
 *  give all page table pages back to the free pool.  This is called in sched()
 *  with palloc locked.
 */
void
mmurelease(Proc *p)
{
	Page *pg;
	Page *next;

	if(canlock(&palloc))
		panic("mmurelease");

	/* point to protoype page map */
	setptb(origlvl1);
	icflush();

	/* give away page table pages */
	for(pg = p->mmuused; pg; pg = next){
		next = pg->next;
		pg->next = p->mmufree;
		p->mmufree = pg;
	}
	p->mmuused = 0;
	if(p->mmutop) {
		p->mmutop->next = p->mmufree;
		p->mmufree = p->mmutop;
		p->mmutop = 0;
	}
	if(p->mmulvl2) {
		p->mmulvl2->next = p->mmufree;
		p->mmufree = p->mmulvl2;
		p->mmulvl2 = 0;
	}
	for(pg = p->mmufree; pg; pg = next){
		next = pg->next;
		if(--pg->ref)
			panic("mmurelease: pg->ref %d\n", pg->ref);
		pagechainhead(pg);
	}
	if(p->mmufree && palloc.r.p)
		wakeup(&palloc.r);
	p->mmufree = 0;
}

void
mmunewtop(void)
{
	Page *top, *lvl2;
	uvlong *ppte;

	top = newpage(1, 0, 0);
	top->va = VA(kmap(top));
	lvl2 = newpage(1, 0, 0);
	lvl2->va = VA(kmap(lvl2));

	ppte = (uvlong *)top->va;
	ppte[0] = PTEPFN(lvl2->pa) | PTEKVALID;
	ppte[PTE2PG-2] = PTEPFN(top->pa) | PTEKVALID;
	ppte[PTE2PG-1] = PTEPFN(klvl2) | PTEKVALID;

	up->mmutop = top;
	up->mmulvl2 = lvl2;
	setptb(top->pa);
	tlbflush(-1, 0);
	icflush();
}

void
putmmu(ulong va, ulong pa, Page *pg)
{
	int lvl2off;
	uvlong *lvl2, *pt;
	int s;

	if(up->mmutop == 0)
		mmunewtop();

	lvl2 = (uvlong*)up->mmulvl2->va;
	lvl2off = LVL2OFF(va);

	/*
	 *  if bottom level page table missing, allocate one 
	 *  and point the top level page at it.
	 */
	s = splhi();
	if(lvl2[lvl2off] == 0){
		if(up->mmufree == 0){
			spllo();
			pg = newpage(1, 0, 0);
			pg->va = VA(kmap(pg));
			splhi();
		} else {
			pg = up->mmufree;
			up->mmufree = pg->next;
			memset((void*)pg->va, 0, BY2PG);
		}
		lvl2[lvl2off] = PTEPFN(pg->pa) | PTEVALID;
		pg->daddr = lvl2off;
		pg->next = up->mmuused;
		up->mmuused = pg;
	}

	/*
	 *  put in new mmu entry
	 */
	pt = (uvlong*)(((lvl2[lvl2off] >> 32)<<PGSHIFT)|KZERO);
	pt[LVL3OFF(va)] = FIXPTE(pa);

	/* flush cached mmu entries */
	tlbflush(3, va);
	icflush();
	splx(s);
}

void *
kmapv(uvlong pa, int size)
{
	void *va, *new;
	int lvl2off, i, npage, offset;
	uvlong *lvl2, *pt;

	offset = pa&(BY2PG-1);
	npage = ((size+offset+BY2PG-1)>>PGSHIFT);

	va = nextio+offset;
	lvl2 = (uvlong*)(KZERO|klvl2);
	for (i = 0; i < npage; i++) {
		lvl2off = LVL2OFF(nextio);
		if (lvl2[lvl2off] == 0) {
			new = xspanalloc(BY2PG, BY2PG, 0);
			memset(new, 0, BY2PG);
			lvl2[lvl2off] = PTEPFN(PADDR(new)) | PTEKVALID | PTEASM;
		}
		pt = (uvlong*)(((lvl2[lvl2off] >> 32)<<PGSHIFT)|KZERO);
		pt[LVL3OFF(nextio)] = PTEPFN(pa) | PTEKVALID | PTEASM;
		nextio += BY2PG;
		pa += BY2PG;
	}
	return va;
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

ulong
upamalloc(ulong pa, int size, int align)
{
	void *va;

	/*
	 * Viability hack. Only for PCI framebuffers.
	 */
	if(pa == 0)
		return 0;
	USED(align);
	va = kmapv(((uvlong)0x88<<32LL)|pa, size);
	if(va == nil)
		return 0;
	return PADDR(va);
}

void
upafree(ulong, int)
{
	print("upafree: virtual mapping not freed\n");
}
