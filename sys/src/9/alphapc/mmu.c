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

static void
mmuptefree(Proc* proc)
{
	uvlong *lvl2;
	Page **last, *page;

	if(proc->mmutop && proc->mmuused){
		lvl2 = (uvlong*)proc->mmulvl2->va;
		last = &proc->mmuused;
		for(page = *last; page; page = page->next){
			lvl2[page->daddr] = 0;
			last = &page->next;
		}
		*last = proc->mmufree;
		proc->mmufree = proc->mmuused;
		proc->mmuused = 0;
	}
}

void
mmuswitch(Proc *proc)
{
	if(proc->newtlb){
		mmuptefree(proc);
		proc->newtlb = 0;
	}

	/* tell processor about new page table and flush cached entries */
	if(proc->mmutop == 0)
		setptb(origlvl1);
	else
		setptb(proc->mmutop->pa);
	tlbflush(-1, 0);
	icflush();
}

/* point to protoype page map */
void
mmupark(void)
{
	setptb(origlvl1);
	icflush();
}

/*
 *  give all page table pages back to the free pool.  This is called in sched()
 *  with palloc locked.
 */
void
mmurelease(Proc *proc)
{
	Page *page, *next;

	mmupark();
	mmuptefree(proc);
	proc->mmuused = 0;
	if(proc->mmutop) {
		proc->mmutop->next = proc->mmufree;
		proc->mmufree = proc->mmutop;
		proc->mmutop = 0;
	}
	if(proc->mmulvl2) {
		proc->mmulvl2->next = proc->mmufree;
		proc->mmufree = proc->mmulvl2;
		proc->mmulvl2 = 0;
	}
	for(page = proc->mmufree; page; page = next){
		next = page->next;
		if(--page->ref)
			panic("mmurelease: page->ref %d\n", page->ref);
		pagechainhead(page);
	}
	if(proc->mmufree && palloc.r.p)
		wakeup(&palloc.r);
	proc->mmufree = 0;
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

void*
vmap(ulong pa, int size)
{
	void *va;

	/*
	 * Viability hack. Only for PCI framebuffers.
	 */
	if(pa == 0)
		return 0;
	va = kmapv(((uvlong)0x88<<32LL)|pa, size);
	if(va == nil)
		return 0;
	return (void*)va;
}

void
vunmap(void*, int)
{
	print("vunmap: virtual mapping not freed\n");
}

void
mmudump(void)
{
	Page *top, *lvl2;

	iprint("ptbr %lux up %#p\n", (ulong)m->ptbr, up);
	if(up) {
		top = up->mmutop;
		if(top != nil)
			iprint("top %lux top[N-1] %llux\n", top->va, ((uvlong *)top->va)[PTE2PG-1]);
		lvl2 = up->mmulvl2;
		if(lvl2 != nil)
			iprint("lvl2 %lux\n", lvl2->va);
	}
}

ulong
upaalloc(int, int)
{
	return 0;
}

void
upafree(ulong, int)
{
}

void
checkmmu(ulong, ulong)
{
}

void
countpagerefs(ulong*, int)
{
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 */
ulong
cankaddr(ulong pa)
{
	ulong kzero;

	kzero = -KZERO;
	if(pa >= kzero)
		return 0;
	return kzero - pa;
}
