#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

ulong	putmmus;

ulong	*A;		/* top level page table */
ulong	*upt;		/* pointer to User page table entry */

/* offset of x into the three page table levels */
#define AOFF(x) (((ulong)x)>>25)
#define BOFF(x)	((((ulong)x)>>18)&(128-1))
#define COFF(x)	((((ulong)x)>>13)&(32-1))

struct{
	ulong	addr;
	ulong	baddr;
	Lock;
}kmapalloc = {
	KIO,
	KIOB
};

/*
 * Called splhi, not in Running state
 */
void
mapstack(Proc *p)
{
	Page *pg, **l;

	if(p->upage->va != (USERADDR|(p->pid&0xFFFF)) && p->pid != 0)
		panic("mapstack %d 0x%lux 0x%lux", p->pid, p->upage->pa, p->upage->va);

	if(p->newtlb) {
		flushcpucache();
		flushatc();

		/*
		 *  bin the current user level page tables
		 */
		p->mmuptr = 0;
		l = &p->mmufree;
		for(pg = p->mmufree; pg; pg = pg->next)
			l = &pg->next;
		*l = p->mmuused;
		p->mmuused = 0;
		p->mmuA[0] = 0;
		p->mmuA[1] = 0;

		p->newtlb = 0;
	}

	A[0] = p->mmuA[0];
	A[1] = p->mmuA[1];
	*upt = PPN(p->upage->pa) | PTEVALID | PTEKERNEL;
	u = (User*)USERADDR;

	if(p != m->lproc){
		flushatc();
		m->lproc = p;
	}
}

/*
 *  allocate kernel page map and enter one mapping.  Return
 *  address of the mapping.
 */
static ulong*
putkmmu(ulong virt, ulong phys)
{
	ulong *b, *c;

	b = (ulong*)(A[AOFF(virt)] & PTEAMASK);
	if(b == 0){
		b = (ulong*)xspanalloc(128*sizeof(ulong), 128*sizeof(ulong), 0);
		A[AOFF(virt)] = ((ulong)b) | DESCVALID;
	}
	c = (ulong*)(b[BOFF(virt)] & PTEBMASK);
	if(c == 0){
		c = (ulong*)xspanalloc(32*sizeof(ulong), 32*sizeof(ulong), 0);
		b[BOFF(virt)] = ((ulong)c) | DESCVALID;
	}
	c = &c[COFF(virt)];
	*c = phys;
	return c;
}

void
dumpmmu(char *tag)
{
	int ia, ib, ic;
	ulong *b, *c;

	print("%s A = %lux\n", tag, A);
	for(ia = 0; ia < 128; ia++)
		if(A[ia]){
			print("A[%d] = %lux\n", ia, A[ia]);
			b = (ulong*)(A[ia] & PTEAMASK);
			for(ib = 0; ib < 128; ib++)
				if(b[ib]){
					print("\tB[%d] = %lux\n", ib, b[ib]);
					c = (ulong*)(b[ib] & PTEBMASK);
					for(ic = 0; ic < 32; ic += 4)
						print("\t\%lux %lux %lux %lux\n",
						  c[ic], c[ic+1], c[ic+2], c[ic+3]);
				}
		}
	delay(10000);
}

void
mmuinit(void)
{
	ulong l;

	/*
	 * l.s already set up ITT0 and DTT0 to point to physical memory
	 * at address KZERO = 0x04000000 = 64MB.
	 * Every process and the kernel shares the same A-level entry set.
	 */
	A = (ulong*)xspanalloc(128*sizeof(ulong), 128*sizeof(ulong), 0);

	/*
	 * Set up user and kernel root pointers; set TC to 8K pages, enabled
	 */
	initmmureg(A);

	/*
	 *  Initialize the I/O space.  After this the IO() macros
	 *  in "io.h" will allow access to the I/O registers.
	 */
	for(l=IOMIN; l<IOMAX; l+=BY2PG)
		kmappa(l);

	/*
	 *  Initialize the byte I/O space.  After this the BIO() macros
	 *  in "io.h" will allow access to the byte I/O registers.
	 */
	for(l=IOBMIN; l<IOBMAX; l+=BY2PG)
		kmappba(l);

	/*
	 *  Create a page table entry for the User area
	 */
	upt = putkmmu(USERADDR, 0);
}


/*
 *  Mark the mmu and tlb as inconsistent and call mapstack to fix it up.
 */
void
flushmmu(void)
{
	int s;

	s = splhi();
	if(u){
		u->p->newtlb = 1;
		mapstack(u->p);
	} else {
		flushcpucache();
		flushatc();
	}
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

	p->mmuA[0] = 0;
	A[0] = 0;
	p->mmuA[1] = 0;
	A[1] = 0;
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
	ulong *b, *c;

	if(u==0)
		panic("putmmu");
	p = u->p;

	if(tlbvirt >= KZERO){
		pprint("process too large %lux; killed\n", tlbvirt);
		pexit("Suicide", 0);
	}

	splhi();
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

	/*
	 *  if no B level page tables, allocate 2 (for up to 64 meg).
	 */
	b = (ulong*)(A[AOFF(tlbvirt)] & PTEAMASK);
	if(b == 0){
		b = (ulong*)p->mmuptr;
		memset((uchar*)b, 0, 2*128*sizeof(ulong));
		p->mmuA[0] = p->mmuptr | DESCVALID;
		A[0] = p->mmuA[0];
		p->mmuptr += 128*sizeof(ulong);
		p->mmuA[1] = p->mmuptr | DESCVALID;
		A[1] = p->mmuA[1];
		p->mmuptr += 128*sizeof(ulong);
	}

	/*
	 *  if the C level page table is missing, allocate it.
	 */
	c = (ulong*)(b[BOFF(tlbvirt)] & PTEBMASK);
	if(c == 0){
		memset((uchar*)p->mmuptr, 0, 32*sizeof(ulong));
		c = (ulong*)p->mmuptr;
		b[BOFF(tlbvirt)] = p->mmuptr | DESCVALID;
		p->mmuptr += 32*sizeof(ulong);
		if(p->mmuptr >= p->mmuused->pa + BY2PG)
			p->mmuptr = 0;
	}

	s = splhi();
	c[COFF(tlbvirt)] = tlbphys;
	flushatcpage(tlbvirt);
	splx(s);
}

/*
 * Entry point known to libgnot.  bitblt has just compiled into
 * stack; make sure instruction cache sees it.
 */
void
gflushcpucache(void)
{
	int l;

	if(u && USERADDR<=(ulong)&l && (ulong)&l<USERADDR+BY2PG){
		/*
		 * stack is virtual and is in one page
		 */
		flushpage(u->p->upage->pa);
	}else{
		/*
		 * stack is physical and may straddle a page
		 */
		flushpage((ulong)&l);
		flushpage(((ulong)&l)+BY2PG);
	}
}

int
kernaddr(ulong x)
{
	if(x < (KZERO-BY2PG))
		return 0;
	if(x >= KIOB && x < kmapalloc.baddr)
		return 1;
	if(x >= KIO && x < kmapalloc.addr)
		return 1;
	if((x&~(BY2PG-1)) == USERADDR)
		return 1;
	if(x >= KZERO && x < (ulong)end)
		return 1;
	if(x >= conf.base0 && x < conf.npage0)
		return 1;
	if(x >= conf.base1 && x < conf.npage1)
		return 1;
	return 0;
}

ulong
kmappba(ulong pa)
{
	ulong k;

	lock(&kmapalloc);
	k = kmapalloc.baddr;
	if(k >= KIOBTOP)
		panic("kmappab");
	kmapalloc.baddr += BY2PG;
	putkmmu(k, pa|PTEUNCACHED|PTEKERNEL);
	unlock(&kmapalloc);
	return k;
}

ulong
kmappa(ulong pa)
{
	ulong k;

	lock(&kmapalloc);
	k = kmapalloc.addr;
	if(k >= KIOTOP)
		panic("kmappa");
	kmapalloc.addr += BY2PG;
	putkmmu(k, pa|PTEUNCACHED|PTEKERNEL);
	unlock(&kmapalloc);
	return k;
}

void
invalidateu(void)
{
	*upt = 0;
	flushatcpage(USERADDR);
}
