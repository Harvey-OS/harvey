#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 *  WARNING:	Even though all MMU data is in the mach structure or
 *		pointed to by it, this code will not work on a multi-processor
 *		without modification.
 */
	
/*
 *  doubly linked list for LRU algorithm
 */
struct List
{
	List *prev;		/* less recently used */
	List *next;		/* more recently used */
};

/*
 *  a Sparc context description
 */
struct Ctx
{
	List;	/* MUST BE FIRST IN STRUCTURE!! */

	Proc	*proc;	/* process that owns this context */
	Pmeg	*pmeg;	/* list of pmeg's used by this context */
	ushort	index;	/* which context this is */
};

/*
 *  a Sparc PMEG description
 */
struct Pmeg
{
	List;	/* MUST BE FIRST IN STRUCTURE!! */

	ulong	lo;	/* low water mark of used entries */
	ulong	hi;	/* high water mark of used entries */
	ushort	index;	/* which Pmeg this is */

	ulong	virt;	/* virtual address this Pmeg represents */
	Ctx	*ctx;	/* context Pmeg belongs to */
	Pmeg	*cnext;	/* next pmeg used in same context */
};

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

static Pmeg*	allocpmeg(ulong);
static void	freepmeg(Pmeg*, int);
static Ctx*	allocctx(Proc*);
static void	freectx(Ctx*, int);
static void	putpme(ulong, ulong, int);
static void	mklast(List**, List*);
static void	mkfirst(List**, List*);
static void	remove(List**, List*);
static void	add(List**, List*);
static void	flushpage(ulong);
static void	putctx(Ctx*);

/*
 *  move to end of use list
 */
static void
mklast(List **list, List *entry)
{
	List *first;

	first = *list;
	if(entry == first){
		*list = first->next;
		return;	
	}

	/* remove from list */
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;

	/* chain back in */
	entry->prev = first->prev;
	entry->next = first;
	first->prev->next = entry;
	first->prev = entry;
}

/*
 *  move to front of use list
 */
static void
mkfirst(List **list, List *entry)
{
	List *first;

	first = *list;
	if(entry == first)
		return;		/* already at front */

	/* unchain */
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;

	/* chain back in */
	entry->prev = first->prev;
	entry->next = first;
	first->prev->next = entry;
	first->prev = entry;

	*list = entry;
}

/*
 *  add to list
 */
static void
add(List **list, List *entry)
{
	List *first;

	first = *list;
	if(first == 0){
		entry->prev = entry;
		entry->next = entry;
	} else {
		entry->prev = first->prev;
		entry->next = first;
		first->prev->next = entry;
		first->prev = entry;
	}
	*list = entry;
}

/*
 *  add a pmeg to the current context for the given address
 */
static Pmeg*
allocpmeg(ulong virt)
{
	Pmeg *p;

	virt = virt & ~(BY2SEGM-1);
	for(;;){
		p = (Pmeg*)m->plist;
		if(p->ctx == 0)
			break;
		m->needpmeg = 1;
		sched();
		splhi();
	}

	/* add to current context */
	p->ctx = m->cctx;
	p->cnext = m->cctx->pmeg;
	m->cctx->pmeg = p;
	p->virt = virt;
	p->lo = virt + BY2PG;
	p->hi = virt;
	putsegspace(p->virt, p->index);
	mklast(&m->plist, p);
	return p;
}

static void
freepmeg(Pmeg *p, int flush)
{
	ulong x;

	/* invalidate any used PTE's */
	for(x = p->lo; x <= p->hi; x += BY2PG)
		putpme(x, INVALIDPTE, flush);

	/* invalidate segment pointer */
	putsegspace(p->virt, INVALIDPMEG);

	p->hi = 0;
	p->lo = 0;
	p->cnext = 0;
	p->ctx = 0;
	p->virt = 0;
	mkfirst(&m->plist, p);
}

/*
 *  get a context for a process.
 */
static Ctx*
allocctx(Proc *proc)
{
	Ctx *c;

	c = (Ctx*)m->clist;
	putctx(c);
	if(c->proc)
		freectx(c, 1);
	if(c->pmeg)
		panic("allocctx c->pmeg %lux\n", c->pmeg);

	c->proc = proc;
	c->proc->ctxonmach[m->machno] = c;
	mklast(&m->clist, c);
	return c;
}

static void
freectx(Ctx *c, int flush)
{
	Pmeg *p;

	putctx(c);
	if(c->proc->ctxonmach[m->machno] != c)
		panic("freectx %lux %lux\n", c->proc->ctxonmach[m->machno], c);

	/* give back mappings */
	while(p = c->pmeg){
		if(p->ctx != c)
			panic("freectx: %lux/%lux\n", p->ctx, c);
		c->pmeg = p->cnext;
		freepmeg(p, flush);
	}

	/* flush u-> */
	flushpage(USERADDR);

	/* detach from process */
	c->proc->ctxonmach[m->machno] = 0;
	c->proc = 0;
	mkfirst(&m->clist, c);
}

static void
putctx(Ctx *c)
{
	putcontext(c->index);
	m->cctx = c;
}

static void
flushpage(ulong virt)
{
	ulong a, evirt;

	a = virt;
	evirt = virt+BY2PG;
	do
		a = flushpg(a);
	while(a < evirt);
}


/*
 *  stuff an entry into a pmeg
 */
static void
putpme(ulong virt, ulong phys, int flush)
{
	virt = virt & ~(BY2PG-1);
	if(flush)
		flushpage(virt);
	putpmegspace(virt, phys);
}

/*
 *  put a mapping into current context
 */
void
putmmu(ulong virt, ulong phys, Page *pg)
{
	ulong x, v;
	Pmeg *p;
	int s;

	USED(pg);
	s = splhi();
	v = virt & ~(BY2SEGM-1);
	x = getsegspace(v);
	if(x == INVALIDPMEG)
		p = allocpmeg(v);
	else {
		p = &m->pmeg[x - m->pfirst];
		if(x != p->index)
			panic("putmmu %d/%d\n", x, p->index);
	}
	if(virt < p->lo)
		p->lo = virt;
	if(virt > p->hi)
		p->hi = virt;
	
	putpme(virt, phys, 1);
	splx(s);
}

/*
 * Initialize cache by clearing the valid bit
 * (along with the others) in all cache entries
 */
void
cacheinit(void)
{
	int i;

	for(i=0; i<conf.vacsize; i+=conf.vaclinesize)
		putsysspace(CACHETAGS+i, 0);

	/*
	 * Turn cache on
	 */
	putenab(getenab()|ENABCACHE);
}

/*
 *  set up initial mappings and the LRU lists for contexts and pmegs
 */
void
mmuinit(void)
{
	int c, i, j;
	ulong va, ktop, pme, kbot1, ktop1;
	int fp;		/* first free pmeg */
	Pmeg *p;
	Ctx *ctx;

	/*
	 *  mmuinit is entered with PMEG's 0-3 providing mapping
	 *  for virtual addresses KZERO<->KZERO+4*BY2SEGM to physical
	 *  0<->4*BY2SEGM
	 */
	compile();

	/*
	 *  map all of text image
	 */
	ktop = PGROUND((ulong)end);
	i = 0;
	for(c = 0; c < conf.ncontext; c++){
		i = 0;
		for(va = KZERO; va < ktop; va += BY2SEGM)
			putcxsegm(c, va, i++);
	}
	fp = i;

	/*
	 *  Make sure cache is turned on for program and data
	 */
	pme = PTEVALID|PTEWRITE|PTEKERNEL|PTEMAINMEM;
	i = 0;
	for(va = KZERO; va < ktop; va += BY2PG, i++)
		putpme(va, pme+i, 1);

	/*
	 *  invalidate rest of kernel's region 0's PMEG's
	 */
	va = ROUNDUP(ktop, BY2SEGM);
	for(; ktop < va; ktop += BY2PG)
		putpme(ktop, INVALIDPTE, 1);

	/*
	 *  Invalidate all entries in all other pmegs
	 */
	for(j = fp; j < conf.npmeg; j++){
		putsegspace(INVALIDSEGM, j);
		for(va = 0; va < BY2SEGM; va += BY2PG)
			putpmegspace(INVALIDSEGM+va, INVALIDPTE);
	}

	if(conf.base0 < conf.npage0){
		/*
		 *  map kernel region 0, this may overlap kernel text image
		 */
		ktop1 = PGROUND(conf.npage0);
		kbot1 = conf.base0 & ~(BY2SEGM - 1);
		if(kbot1 < ktop)
			kbot1 = ktop;
		for(c = 0; c < conf.ncontext; c++){
			i = fp;
			putcontext(c);
			for(va = kbot1; va < ktop1; va += BY2SEGM)
				putsegspace(va, i++);
		}
		fp = i;
	
		/*
		 *  Make sure cache is turned on for kernel region 0
		 */
		kbot1 = conf.base0 & ~(BY2PG - 1);
		pme = PTEVALID|PTEWRITE|PTEKERNEL|PTEMAINMEM;
		i = PPN(kbot1 & ~KZERO);
		for(va = kbot1; va < ktop1; va += BY2PG, i++)
			putpme(va, pme+i, 1);
		ktop = (ktop1+BY2SEGM - 1) & ~(BY2SEGM - 1);
	}

	if(conf.base1 < conf.npage1){
		/*
		 *  map kernel region 1, this may overlap kernel region 0's
		 *  PMEG's.
		 */
		ktop1 = PGROUND(conf.npage1);
		kbot1 = conf.base1 & ~(BY2SEGM - 1);
		if(kbot1 < ktop)
			kbot1 = ktop;
		for(c = 0; c < conf.ncontext; c++){
			i = fp;
			putcontext(c);
			for(va = kbot1; va < ktop1; va += BY2SEGM)
				putsegspace(va, i++);
		}
		fp = i;
	
		/*
		 *  Make sure cache is turned on for kernel region 1
		 */
		kbot1 = conf.base1 & ~(BY2PG - 1);
		pme = PTEVALID|PTEWRITE|PTEKERNEL|PTEMAINMEM;
		i = PPN(kbot1 & ~KZERO);
		for(va = kbot1; va < ktop1; va += BY2PG, i++)
			putpme(va, pme+i, 1);
		ktop = (ktop1+BY2SEGM - 1) & ~(BY2SEGM - 1);
	}

	/*
	 *  allocate pmegs for kmap'ing
	 */
	for(c = 0; c < conf.ncontext; c++){
		putcontext(c);
		i = fp;
		for(va = IOSEGM; va < IOSEGM+IOSEGSIZE; va += BY2SEGM)
			putsegspace(va, i++);
	}
	fp = i;

	/*
	 *  invalidate everything outside the kernel in every context
	 */
	for(c = 0; c < conf.ncontext; c++){
		putcontext(c);
		for(va = UZERO; va < (KZERO & VAMASK); va += BY2SEGM)
			putsegspace(va, INVALIDPMEG);

		for(va = ktop; va < IOSEGM; va += BY2SEGM)
			putsegspace(va, INVALIDPMEG);
	}

	/*
	 *  allocate MMU management data for this CPU
	 */
	m->pmeg = p = xalloc((INVALIDPMEG - fp) * sizeof(Pmeg));
	m->pfirst = fp;
	for(; fp < INVALIDPMEG; fp++, p++){
		p->index = fp;
		add(&m->plist, p);
	}
	m->cctx = ctx = xalloc(conf.ncontext * sizeof(Ctx));
	for(i = 0; i < conf.ncontext; i++, ctx++){
		ctx->index = i;
		add(&m->clist, ctx);
	}

	putctx(m->cctx);
}

/*
 *  give up our mmu context
 */
void
mmurelease(Proc *p)
{
	Ctx *c;

	c = p->ctxonmach[m->machno];
	if(c == 0)
		return;

	freectx(c, 1);
}

/*
 *  set up context for a process
 */
void
mapstack(Proc *p)
{
	Ctx *c;
	ulong tlbphys;
	Pmeg *pm, *f, **l;

	if(p->upage->va != (USERADDR|(p->pid&0xFFFF)) && p->pid != 0)
		panic("mapstack %s %d %lux 0x%lux 0x%lux",
			p->text, p->pid, p->upage, p->upage->pa, p->upage->va);

	/* free a pmeg if a process needs one */
	if(m->needpmeg){
		pm = (Pmeg*)m->plist;
		if(c = pm->ctx){
			/* remove from context list */
			l = &c->pmeg;
			for(f = *l; f; f = f->cnext){
				if(f == pm){
					*l = f->cnext;
					break;
				}
				l = &f->cnext;
			}
			if(f != pm)
				panic("mapstack f != pm\n");
	
			/* flush cache & remove mappings */
			putctx(c);
			freepmeg(pm, 1);
		}
		m->needpmeg = 0;
	}

	/* give up our context if it is unclean */
	if(p->newtlb){
		c = p->ctxonmach[m->machno];
		if(c)
			freectx(c, 1);
		p->newtlb = 0;
	}

	/* set to this proc's context */
	c = p->ctxonmach[m->machno];
	if(c == 0)
		c = allocctx(p);
	else
		putctx(c);

	if(c->proc != p || p->ctxonmach[m->machno] != c)
		panic("mapstack c->proc != p\n");

	/* make sure there's a mapping for u-> */
	tlbphys = PPN(p->upage->pa)|PTEVALID|PTEWRITE|PTEKERNEL|PTEMAINMEM;
	putpmegspace(USERADDR, tlbphys);
	u = (User*)USERADDR;
}

void
flushmmu(void)
{
	Ctx *c;
	Pmeg *p;

	splhi();
	c = u->p->ctxonmach[m->machno];
	if(c == 0)
		panic("flushmmu");
	while(p = c->pmeg){
		c->pmeg = p->cnext;
		freepmeg(p, 1);
	}
	spllo();
}

void
invalidateu(void)
{
	putpme(USERADDR, INVALIDPTE, 1);
}

struct
{
	Lock;
	KMap	*free;
	KMap	arena[IOSEGSIZE/BY2PG];
	int inited;
}kmapalloc;

void
kmapinit(void)
{
	KMap *k;
	ulong va;

	kmapalloc.free = 0;
	k = kmapalloc.arena;
	for(va = IOSEGM; va < IOSEGM + IOSEGSIZE; va += BY2PG, k++){
		k->va = va;
		kunmap(k);
	}
	kmapalloc.inited = 1;
}

KMap*
kmappa(ulong pa, ulong flag)
{
	KMap *k;
	ulong s;

	lock(&kmapalloc);
	k = kmapalloc.free;
	if(k == 0){
		dumpstack();
		panic("kmap");
	}
	kmapalloc.free = k->next;
	unlock(&kmapalloc);
	k->pa = pa;
	s = splhi();
	putpme(k->va, PPN(pa)|PTEVALID|PTEKERNEL|PTEWRITE|flag, 1);
	splx(s);
	return k;
}


void
kunmap(KMap *k)
{
	ulong pa;

	pa = k->pa;
	k->pa = 0;
	lock(&kmapalloc);
	k->next = kmapalloc.free;
	kmapalloc.free = k;
	if(pa != k->pa)
		putpme(k->va, INVALIDPTE, 1);
	unlock(&kmapalloc);
}

ulong
kmapregion(ulong pa, ulong n, ulong flag)
{
	KMap *k;
	ulong va;
	int i, j;

	/*
	 * kmap's are initially in reverse order so rearrange them.
	 */
	i = (n+(BY2PG-1))/BY2PG;
	va = 0;
	for(j=i-1; j>=0; j--){
		k = kmappa(pa+j*BY2PG, flag);
		if(va && va != k->va+BY2PG)
			systemreset();
		va = k->va;
	}
	return va;
}

KMap*
kmap(Page *pg)
{
	ulong va;
	KMap *k;

	/*
	 * avoid an alias: if part of kernel memory, just return map
	 */
	va = pg->pa|KZERO;
	if((KZERO<=va && va<(ulong)end) ||
	   (conf.base0<=va && va<conf.npage0) ||
	   (conf.base1<=va && va<conf.npage1)){
		lock(&kmapalloc);
		k = kmapalloc.free;
		if(k == 0){
			dumpstack();
			panic("kmap");
		}
		kmapalloc.free = k->next;
		unlock(&kmapalloc);
		k->va = va;
		k->pa = va;
		return k;
	}

	/*
	 * Cache is virtual and a pain to deal with.
	 * Must avoid having the same entry in the cache twice, so
	 * must use NOCACHE or else extreme cleverness elsewhere.
	 */
	return kmappa(pg->pa, PTEMAINMEM|PTENOCACHE);
}
