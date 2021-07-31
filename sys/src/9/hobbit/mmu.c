#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "machine.h"

static ulong *upte;

#define segnum			daddr

typedef struct {
	Lock;
	Page	*free;
	int	current;
	int	cache;
	ulong	hits;
	ulong	misses;
} Alloc;

static Alloc stballoc;
static Alloc ptballoc;

static Page*
mmucalloc(Alloc *alloc, void (*init)(Page*))
{
	Page *pg;
	int s;

	lock(alloc);
	if(alloc->free == 0){
		alloc->misses++;
		unlock(alloc);
		s = spllo();
		pg = newpage(0, 0, 0);
		pg->va = VA(kmap(pg));
		(*init)(pg);
		splx(s);
		return pg;
	}
	pg = alloc->free;
	alloc->free = pg->next;
	alloc->current--;
	alloc->hits++;
	unlock(alloc);
	return pg;
}

static void
mmucfree(Alloc *alloc, Page *pg)
{
	if(alloc->current >= alloc->cache){
		simpleputpage(pg);
		return;
	}
	lock(alloc);
	pg->next = alloc->free;
	alloc->free = pg;
	alloc->current++;
	unlock(alloc);
}

void
stbinit(Page *pg)
{
	memmove((void*)pg->va, (void*)KSTBADDR, BY2PG);
}

void
ptbinit(Page *pg)
{
	memset((void*)pg->va, 0, BY2PG);
}

static void
ptbuinit(ulong *ptb)
{
	ptb[0] = 0x7F<<4;
	ptb[1] = 0;
}

static void
ptbuupdate(ulong *ptb, ulong pgnum)
{
	int index;

	pgnum /= 8;
	index = ((ptb[0]>>4) & 0x7F);
	if(pgnum < index)
		ptb[0] = (ptb[0] & ~(0x7F<<4))|(pgnum<<4);
	index = ((ptb[1]>>4) & 0x7F);
	if(pgnum > index)
		ptb[1] = (ptb[1] & ~(0x7F<<4))|(pgnum<<4);
}

static void
ptbuclear(ulong *ptb)
{
	int first, last, n;

memset(ptb, 0, BY2PG);
return;
	first = ((ptb[0]>>4) & 0x7F);
	last = ((ptb[1]>>4) & 0x7F);
	n = last - first;
	if(n >= 0)
		memset(ptb+(first*8), 0, (n+1)*8*BY2WD);
}

/*
 * Map a physical I/O address one-to-one with its virtual address.
 * Must be called before process scheduling starts.
 * Could be more flexible, assumes segment alignment.
 */
int
mmuiomap(ulong ioa, ulong npages)
{
	ulong *stb = (ulong*)KSTBADDR;

	if((ioa & SEGOFFSET) || stb[SEGNUM(ioa)])
		return -1;
	stb[SEGNUM(ioa)] = NPSTE(ioa, npages, STE_W|STE_V);
	mmuflushpte(ioa);
	return 0;
}

void
mmuinit(void)
{
	ulong *stb;

	stb = (ulong*)KSTBADDR;
	upte = xspanalloc(BY2PG, BY2PG, 0);
	stb[SEGNUM(USERADDR)] = STE(PADDR((ulong)upte), STE_C|STE_V);
	upte += PGNUM(USERADDR);
	mmusetstb(PADDR(KSTBADDR));

	stballoc.cache = 5;
	ptballoc.cache = 10;
}

ulong *
mmuwalk(Proc *p, ulong va, int create)
{
	ulong *table;
	Page *pg;

	table = &((ulong*)p->mmustb->va)[SEGNUM(va)];
	if(*table == 0){
		if(create == 0)
			return 0;
		if(p->mmufree){
			pg = p->mmufree;
			p->mmufree = pg->next;
		}
		else
			pg = mmucalloc(&ptballoc, ptbinit);
		pg->next = p->mmuused;
		p->mmuused = pg;
		pg->segnum = SEGNUM(va);
		ptbuinit((ulong*)pg->va);
		*table = STE(pg->pa, STE_C|STE_V);
	}
	table = (ulong*)KADDR(PPN(*table));
	va = PGNUM(va);
	if(create)
		ptbuupdate(table, va);
	return &table[va];
}

void
putmmu(ulong va, ulong pa, Page *pg)
{
	Proc *p;
	ulong *entry;
	char *ctl;
	int s;

	pa |= PTE_R|PTE_C|PTE_U;
	ctl = &pg->cachectl[m->machno]; 
	if(*ctl == PG_TXTFLUSH){
		flushcpucache();
		if(pa & PTE_W)
			pa &= ~PTE_C;

		*ctl = PG_NOFLUSH;
	}
	p = u->p;
	s = splhi();
	if(p->mmustb == 0){
		p->mmustb = mmucalloc(&stballoc, stbinit);
		mmusetstb(p->mmustb->pa);
	}
	if((entry = mmuwalk(p, va, 1)) == 0)
		panic("putmmu #%lux\n", va);
	*entry = (*entry & (0x7F<<4))|pa;
	mmuflushpte(va);
	splx(s);
}

static void
mmustbflush(Proc *p)
{
	Page *pg, **lpg;
	ulong *stb;

	if(p->mmustb){
		stb = (ulong*)p->mmustb->va;
		lpg = &p->mmuused;
		for(pg = *lpg; pg; pg = pg->next){
			stb[pg->segnum] = 0;
			ptbuclear((ulong*)pg->va);
			lpg = &pg->next;
		}
		*lpg = p->mmufree;
		p->mmufree = p->mmuused;
		p->mmuused = 0;
	}
}

void
mmurelease(Proc *p)
{
	Page *pg, *next;

	mmusetstb(PADDR(KSTBADDR));
	mmustbflush(p);
	for(pg = p->mmufree; pg; pg = next){
		next = pg->next;
		mmucfree(&ptballoc, pg);
	}
	p->mmufree = 0;
	if(p->mmustb)
		mmucfree(&stballoc, p->mmustb);
	p->mmustb = 0;
}

void
flushmmu(void)
{
	int s;

	s = splhi();
	if(u){
		u->p->newtlb = 1;
		mapstack(u->p);
	}
	else {
		mmusetstb(PADDR(KSTBADDR));
		flushcpucache();
	}
	splx(s);
}

void
mapstack(Proc *p)
{
	ulong stbpa;

	if(p->newtlb){
		mmustbflush(p);
		flushcpucache();
		p->newtlb = 0;
	}
	if(p->mmustb)
		stbpa = p->mmustb->pa;
	else
		stbpa = PADDR(KSTBADDR);
	*upte = PTE(p->upage->pa, PTE_W|PTE_V);
	mmusetstb(stbpa);
	u = (User*)USERADDR;
}

void
invalidateu(void)
{
	*upte = PTE(0, 0);
	mmuflushpte(USERADDR);
}
