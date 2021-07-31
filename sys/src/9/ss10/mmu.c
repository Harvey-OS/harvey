#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

/*
 * software description of an MMU context
 */
struct Ctx
{
	Ctx	*prev;	/* less recently used */
	Ctx	*next;	/* more recently used */
	Proc	*proc;	/* process that owns this context */
	ushort	index;	/* which context this is */
};

ulong		*ioptes;	/* IO MMU's table (shared by all processors) */
struct Iommu	*iommu;

/* offset of x into the three page table levels in a context */
#define AOFF(x) (((ulong)x)>>24)
#define BOFF(x)	((((ulong)x)>>18)&(64-1))
#define COFF(x)	((((ulong)x)>>12)&(64-1))
#define	ISPTAB(x) ((((ulong)x)&3) == PTPVALID)
#define KPN(va) (PADDR(va)>>4)

#define	NIOPTE	(DMASEGSIZE/BY2PG)

static void
append(Ctx *list, Ctx *entry)
{
	/* remove from list */
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;

	/* chain back in */
	entry->prev = list->prev;
	entry->next = list;
	list->prev->next = entry;
	list->prev = entry;
}

static void
putctx(Ctx *c)
{
	putrmmu(CXR, c->index);
	m->cctx = c;
}

static void
clearctx(Ctx *c)
{
	m->tlbfault++;
	if(c->proc->root) {	/* otherwise context was empty */
		m->tlbpurge++;
		putctx(c);
		/* must switch to valid root pointer before flush, so won't reload from old context */
		m->contexts[c->index] = m->contexts[0];
		flushtlbctx();
	} else if(m->contexts[c->index])
		panic("clearctx context[%d]=%lux\n", c->index, m->contexts[c->index]);
	putctx(m->ctxlru);

	/* now safe to clear old root pointer */
	m->contexts[c->index] = 0;

	/* detach from process */
	c->proc->ctx = 0;
	c->proc = 0;
}

/*
 *  get a context for a process.
 */
static Ctx*
allocctx(Proc *proc)
{
	Ctx *c;

	c = m->ctxlru->prev;	/* least recently used */
	if(c==m->ctxlru || c->index==0)
		panic("allocctx");
	if(c->proc)
		clearctx(c);
	if(m->contexts[c->index])
		panic("allocctx contexts[%d]=#%lux", c->index, m->contexts[c->index]);

	c->proc = proc;
	c->proc->ctx = c;
	append(m->ctxlru->next, c);
	return c;
}

/*
 * Called splhi, not in Running state
 */
void
mapstack(Proc *p)
{
	Page *pg, **l;
	Ctx *c;

	if(p->upage->va != (USERADDR|(p->pid&0xFFFF)) && p->pid != 0)
		panic("mapstack %d 0x%lux 0x%lux", p->pid, p->upage->pa, p->upage->va);

	if(p->newtlb) {
		c = p->ctx;
		if(c)
			clearctx(c);
		/*
		 *  bin the current page tables
		 */
		p->root = 0;
		p->mmuptr = 0;
		l = &p->mmufree;
		for(pg = p->mmufree; pg; pg = pg->next)
			l = &pg->next;
		*l = p->mmuused;
		p->mmuused = 0;

		p->newtlb = 0;
	}

	*m->upt = PPN(p->upage->pa) | PTEVALID | PTEKERNEL | PTEWRITE|PTECACHE;
	u = (User*)USERADDR;

	if(!p->kp) {
		c = p->ctx;
		if(c == 0) {
			c = allocctx(p);
			if(p->root)
				m->contexts[c->index] = p->root;
		}
		if(c == m->ctxlru)
			panic("mapstack p->ctx == ctx0");
		if(c->proc != p || p->ctx != c)
			panic("mapstack c->proc != p");
		if(p->root)
			putctx(c);
		else
			putctx(m->ctxlru);
	} else
		putctx(m->ctxlru);	/* kernel processes share context 0 */
	flushtlbpage(USERADDR);
}

/*
 *  allocate kernel page map and enter one mapping.  Return
 *  address of the mapping.
 */
static ulong*
putkmmu(ulong virt, ulong phys, int level)
{
	ulong *a, *b, *c;

	a = &PPT(m->contexts[0])[AOFF(virt)];
	if(level > 1) {
		if(*a == 0){
			b = (ulong*)xspanalloc(64*sizeof(ulong), 64*sizeof(ulong), 0);
			*a = KPN(b) | PTPVALID;
		} else {
			if(!ISPTAB(*a))
				panic("putkmmu virt=%lux *a=%lux", virt, *a);
			b = PPT(*a);
		}
		b = &b[BOFF(virt)];
		if(level > 2) {
			if(*b == 0){
				c = (ulong*)xspanalloc(64*sizeof(ulong), 64*sizeof(ulong), 0);
				*b = KPN(c) | PTPVALID;
			} else {
				if(!ISPTAB(*b))
					panic("putkmmu virt=%lux *b=%lux", virt, *b);
				c = PPT(*b);
			}
			c = &c[COFF(virt)];
			*c = phys;
			return c;
		} else {
			*b = phys;
			return b;
		}
	} else {
		*a = phys;
		return a;
	}
}

void
mmuinit(void)
{
	int i, n;
	ulong *a;
	
	m->contexts = (ulong*)xspanalloc(conf.ncontext*sizeof(ulong), conf.ctxalign*sizeof(ulong), 0);

	/*
	 * context 0 will have the prototype level 1 entries
	 */
	a = (ulong*)xspanalloc(256*sizeof(ulong), 256*sizeof(ulong), 0);

	m->contexts[0] = KPN(a) | PTPVALID;

	/*
	 * map all memory to KZERO
	 * with KZERO at 0xE0000000, can't map an eighth bank--conflicts with IO devices 
	 */
	n = 7 * 64 * MB / BY2PG;
	for(i=0; i<(256*1024/BY2PG); i++)	/* pages to first segment boundary */
		putkmmu(KZERO|(i*BY2PG), PPN(i*BY2PG)|PTEKERNEL|PTEWRITE|PTEVALID|PTECACHE, 3);
	for(; i<(16*MB)/BY2PG; i += 64)	/* segments to first 16Mb boundary */
		putkmmu(KZERO|(i*BY2PG), PPN(i*BY2PG)|PTEKERNEL|PTEWRITE|PTEVALID|PTECACHE, 2);
	for(; i<n; i += 64*64)	/* 16 Mbyte regions to end */
		putkmmu(KZERO|(i*BY2PG), PPN(i*BY2PG)|PTEKERNEL|PTEWRITE|PTEVALID|PTECACHE, 1);

	/*
	 * allocate page table pages for IO mapping
	 */
	n = IOSEGSIZE/BY2PG;
	for(i=0; i<n; i++)
		putkmmu(IOSEGBASE+(i*BY2PG), 0, 3);

	/*
	 * create a page table entry for the User area
	 */
	m->upt = putkmmu(USERADDR, 0, 3);

	/*
	 * load kernel context
	 */

	putrmmu(CTPR, PADDR(m->contexts)>>4);
	putrmmu(CXR, 0);
	flushtlb();

	ioptes = (ulong*)xspanalloc(NIOPTE*sizeof(ulong), NIOPTE*sizeof(ulong), 0);

	/* 
	 * Bypass the IOMMU
	 */
	iommu = kmappas(IOSPACE, IOMMU, PTEIO|PTENOCACHE);
	iommu->base = PADDR(ioptes)>>4;
	iommu->ctl = (DMARANGE<<2)|0;	
	iommu->tlbflush = 0; /* Flush all old IO TLB entries */

	/*
	 * build the LRU structures
	 */
	m->ctxlru = (Ctx*)xalloc(conf.ncontext*sizeof(*m->ctxlru));
	for(i=0; i<conf.ncontext; i++) {
		Ctx *p;
		p = &m->ctxlru[i];
		p->prev = p->next = p;
		p->index = i;
		append(m->ctxlru, p);
	}
	m->cctx = m->ctxlru;
}


/*
 *  Mark the mmu and tlb as inconsistent and call mapstack to fix it up.
 */
void
flushmmu(void)
{
	int s;

	s = splhi();
	u->p->newtlb = 1;
	mapstack(u->p);
	splx(s);
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
	Ctx *c;

	c = p->ctx;
	if(c) {
		clearctx(c);
		append(m->ctxlru, c);
	}
	p->root = 0;
	p->mmuptr = 0;

	/* give away page table pages */
	for(pg = p->mmufree; pg; pg = next){
		next = pg->next;
		simpleputpage(pg);
	}
	p->mmufree = 0;
	for(pg = p->mmuused; pg; pg = next){
		next = pg->next;
		simpleputpage(pg);
	}
	p->mmuused = 0;
}

/*
 *  fill in a user level mmu entry.
 *  allocate any page tables needed.
 */
void
putmmu(ulong tlbvirt, ulong tlbphys, Page *pg)
{
	int s;
	Proc *p;
	char *ctl;
	ulong *a, *b, *c;

	if(u==0)
		panic("putmmu");
	p = u->p;
	if(tlbvirt >= KZERO){
		pprint("process too large %lux; killed\n", tlbvirt);
		pexit("Suicide", 0);
	}

	s = splhi();
	/*
	 *  synchronize text and data cache for this page
	 */
	ctl = &pg->cachectl[m->machno]; 
	if(*ctl == PG_TXTFLUSH) {
		flushpage(pg->pa);
		*ctl = PG_NOFLUSH;
	}

	/*
	 *  Changing page tables must be atomic.  Since newpage() may context
	 *  switch, get any pages we need now.
	 */
	if(p->mmuptr == 0){
		if(p->mmufree == 0){
			spllo();
			pg = newpage(0, 0, 0);
			splhi();
		} else {
			pg = p->mmufree;
			p->mmufree = pg->next;
		}
		pg->next = p->mmuused;
		p->mmuused = pg;
		p->mmuptr = pg->pa;
	}

	if(p->kp || p->ctx == 0 || p->root && (p->ctx != m->cctx || m->cctx->index==0))
		panic("putmmu p->ctx");

	/*
	 * if the A level page table is missing, allocate it.
	 * initialise it with the kernel's level 1 mappings.
	 */
	if(p->root == 0) {
		a = (ulong*)KADDR(p->mmuptr);
		memmove(a, PPT(m->contexts[0]), 256*sizeof(ulong));
		p->mmuptr += 256*sizeof(ulong);
		p->root = KPN(a) | PTPVALID;
		m->contexts[p->ctx->index] = p->root;
		putctx(p->ctx);
	} else {
		if(!ISPTAB(p->root))
			panic("putmmu");
		a = PPT(p->root);
	}

	/*
	 *  if the B level page table is missing, allocate it.
	 */
	a = &a[AOFF(tlbvirt)];
	if(*a == 0){
		b = (ulong*)KADDR(p->mmuptr);
		memset(b, 0, 64*sizeof(ulong));
		*a = KPN(p->mmuptr) | PTPVALID;
		p->mmuptr += 64*sizeof(ulong);
		if(p->mmuptr >= p->mmuused->pa + BY2PG)
			p->mmuptr = 0;
	} else {
		if(!ISPTAB(*a))
			panic("putmmu");
		b = PPT(*a);
	}

	/*
	 *  if the C level page table is missing, allocate it.
	 */
	b = &b[BOFF(tlbvirt)];
	if(*b == 0){
		c = (ulong*)KADDR(p->mmuptr);
		memset(c, 0, 64*sizeof(ulong));
		*b = KPN(p->mmuptr) | PTPVALID;
		p->mmuptr += 64*sizeof(ulong);
		if(p->mmuptr >= p->mmuused->pa + BY2PG)
			p->mmuptr = 0;
	} else {
		if(!ISPTAB(*b))
			panic("putmmu2");
		c = PPT(*b);
	}

	if((tlbphys & PTEUNCACHED) == 0)
		tlbphys |= PTECACHE;
	c[COFF(tlbvirt)] = tlbphys;
	flushtlbpage(tlbvirt);
	splx(s);
}

void
invalidateu(void)
{
	*m->upt = 0;
	flushtlbpage(USERADDR);
}

extern Label	catch;

void
cacheinit(void)
{
	/* Punt on ecache initialization.  If it's present, then it's been on since boot */

	dcflush();
	icflush();

	/* Enable snoop for FP self-modifying code */
	setpcr(getpcr()|ENABCACHE|ENABPRE|ENABSB|ENABSNOOP); 
}

typedef struct Mregion Mregion;
struct Mregion
{
	ulong	addr;
	long	size;
};

struct
{
	Mregion	io;
	Mregion	dma;
	Lock;
}kmapalloc = {
	{IOSEGBASE, IOSEGSIZE},
	{DMASEGBASE, DMASEGSIZE},
};

void
kmapinit(void)
{
}

KMap*
kmappas(uchar space, ulong pa, ulong flag)
{
	ulong k;

	lock(&kmapalloc);
	k = kmapalloc.io.addr;
	kmapalloc.io.addr += BY2PG;
	if((kmapalloc.io.size -= BY2PG) < 0)
		panic("kmappa");
	putkmmu(k, (space<<28)|PPN(pa)|PTEKERNEL|PTEWRITE|PTEVALID|flag, 3);
	flushtlbpage(k);
	unlock(&kmapalloc);
	return (KMap*)k;
}

ulong
kmapdma(ulong pa, ulong n)
{
	ulong va0, va;
	int i, j;

	lock(&kmapalloc);
	i = (n+(BY2PG-1))/BY2PG;
	va0 = kmapalloc.dma.addr;
	kmapalloc.dma.addr += i*BY2PG;
	if((kmapalloc.dma.size -= i*BY2PG) <= 0)
		panic("kmapdma");
	va = va0;
	for(j=0; j<i; j++) {
		putkmmu(va, PPN(pa)|PTEKERNEL|PTEVALID|PTEWRITE, 3);
		flushtlbpage(va);
		ioptes[(va>>PGSHIFT)&(NIOPTE-1)] = PPN(pa)|IOPTEVALID|IOPTEWRITE;
		va += BY2PG;
		pa += BY2PG;
	}
	unlock(&kmapalloc);
	return va0;
}

/*
 * map the frame buffer
 */
ulong
kmapsbus(int slot)
{
	int i, n;

	lock(&kmapalloc);
	n = FBSEGSIZE/BY2PG;
	for(i=0; i<n; i += 64*64) {
		putkmmu(FBSEGBASE+(i*BY2PG), 0xE0000000|PPN(SBUS(slot)+(i*BY2PG))|PTEKERNEL|PTEWRITE|PTEVALID, 1);
	}
	unlock(&kmapalloc);
	return FBSEGBASE;
}

void
dumpmmu(char *tag, int ctx)
{
	int ia, ib, ic, i;
	ulong *a, *b, *c;

	a = PPT(m->contexts[ctx]);
	print("%s (ctx %d) a = %lux\n", tag, ctx, a);
	if(a == 0)
		return;
	for(ia = 0; ia < AOFF(KZERO); ia++)
		if(a[ia]){
			print("A[#%x] = %lux\n", ia, a[ia]);
			if((a[ia]&3) == PTPVALID) {
				b = PPT(a[ia]);
				for(ib = 0; ib < 64; ib++)
					if(b[ib]){
						print("\tB[#%x] = %lux\n", ib, b[ib]);
						if((b[ib]&3) == PTPVALID) {
							c = PPT(b[ib]);
							for(ic = 0; ic < 64; ic += 16) {
								print("\t");
								for(i=ic; i<ic+16; i+=4)
									print(" %lux %lux %lux %lux",
								  		c[i], c[i+1], c[i+2], c[i+3]);
								print("\n");
							}
						}
					}
			}
		}
}

void
dumpiommu(void)
{
	int i;

	print("iommu state:\n");
	for (i = 0; i < NIOPTE; ++i)
		if (ioptes[i] != 0)
			print("%x: %lux\n", i, ioptes[i]);
}