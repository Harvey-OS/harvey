#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

/*
 *  tlb entry 0 is used only by mmuswitch() to set the current tlb pid.
 */

/*
 *  Kmap entries start at tlb index 1 and work their way up until
 *  kmapinval() removes them.  They then restart at 1.  As long as there
 *  are few kmap entries they will not pass TLBROFF (the WIRED tlb entry
 *  limit) and interfere with user tlb entries.
 */

/*
 *  All invalidations of the tlb are via indexed entries.  The virtual
 *  address used is always 'KZERO | (x<<(PGSHIFT+1) | currentpid' where
 *  'x' is the index into the tlb.  This ensures that the current pid doesn't
 *  change and that no two invalidated entries have matching virtual
 *  addresses just in case SGI/MIPS ever makes a chip that cares (as
 *  they keep threatening).  These entries should never be used in
 *  lookups since accesses to KZERO addresses don't go through the tlb.
 */

#define TLBINVAL(x, pid) puttlbx(x, KZERO|((x)<<(PGSHIFT+1))|(pid), 0, 0, PGSZ4K)

void
tlbinit(void)
{
	int i;

	for(i=0; i<NTLB; i++)
		TLBINVAL(i, 0);
}

Lock	kmaplock;
KMap	kpte[KPTESIZE];
KMap*	kmapfree;

void
kmapinit(void)
{
	KMap *k, *klast;

	kmapfree = kpte;
	klast = &kpte[KPTESIZE-1];
	for(k=kpte; k<klast; k++)
		k->next = k+1;
	k->next = 0;

	m->ktlbnext = TLBROFF;
}

/*
 *  Arrange that the KMap'd virtual address will hit the same
 *  primary cache line as pg->va by making bits 14...12 of the
 *  tag the same as virtual address.  These bits are the index
 *  into the primary cache and are checked whenever accessing
 *  the secondary cache through the primary.  Violation causes
 *  a VCE trap.
 */
KMap *
kmap(Page *pg)
{
	int s;
	KMap *k;
	ulong pte;
	ulong x, virt;

	s = splhi();
	lock(&kmaplock);

	if(kmapfree == 0) {
		unlock(&kmaplock);
		kmapinval();		/* try and free some */
		lock(&kmaplock);
		if(kmapfree == 0)
			panic("out of kmap");
	}

	k = kmapfree;
	kmapfree = k->next;

	k->pg = pg;
	/* One for the allocation,
	 * One for kactive
	 */
	k->ref = 2;
	k->konmach[m->machno] = m->kactive;
	m->kactive = k;

	virt = pg->va;
	/* bits 14..12 form the secondary-cache virtual index */
	virt &= PIDX;	
	virt |= KMAPADDR | ((k-kpte)<<KMAPSHIFT);

	k->virt = virt;
	pte = PPN(pg->pa)|PTECOHERXCLW|PTEGLOBL|PTEWRITE|PTEVALID;
	if(virt & BY2PG) {
		k->phys0 = PTEGLOBL|PTECOHERXCLW;
		k->phys1 = pte;
	}
	else {
		k->phys0 = pte;
		k->phys1 = PTEGLOBL|PTECOHERXCLW;
	}

	x = m->ktlbnext;
	k->tlbi[m->machno] = x;
	puttlbx(x, (virt&~BY2PG)|TLBPID(tlbvirt()), k->phys0, k->phys1, PGSZ4K);
	if(++x >= NTLB)
		m->ktlbnext = TLBROFF;
	else
		m->ktlbnext = x;

	unlock(&kmaplock);

	splx(s);
	return k;
}

void
kunmap(KMap *k)
{
	int s;

	s = splhi();
	if(decref(k) == 0) {
		k->virt = 0;
		k->phys0 = 0;
		k->phys1 = 0;
		k->pg = 0;

		lock(&kmaplock);
		k->next = kmapfree;
		kmapfree = k;
		unlock(&kmaplock);
	}
	splx(s);
}

void
kfault(Ureg *ur)
{
	int mno;
	KMap *k, *f;
	ulong x, index, virt, addr;

	mno = m->machno;
	addr = ur->badvaddr;
	index = (addr & ~KMAPADDR) >> KMAPSHIFT;

	if(index >= KPTESIZE)
		panic("kmapfault: %lux", addr);

	k = &kpte[index];
	if(k->virt == 0 || k->ref < 1)
		panic("kmapfault: unmapped %lux", addr);

	for(f = m->kactive; f; f = f->konmach[mno])
		if(f == k)
			break;

	if(f == 0) {
		incref(k);
		k->konmach[mno] = m->kactive;
		m->kactive = k;
	}

	x = m->ktlbnext;
	virt = (k->virt&~BY2PG)|TLBPID(tlbvirt());
	puttlbx(x, virt, k->phys0, k->phys1, PGSZ4K);
	k->tlbi[mno] = x;

	if(++x >= NTLB)
		m->ktlbnext = TLBROFF;
	else
		m->ktlbnext = x;
}

void
kmapinval()
{
	int mno, curpid;
	KMap *k, *next;

	if(m->kactive == 0)
		return;

	curpid = PTEPID(TLBPID(tlbvirt()));
	mno = m->machno;
	for(k = m->kactive; k; k = next) {
		next = k->konmach[mno];
		TLBINVAL(k->tlbi[mno], curpid);
		kunmap(k);
	}
	m->kactive = 0;
}

void
mmuswitch(Proc *p)
{
	int tp;

	if(p->newtlb) {
		memset(p->pidonmach, 0, sizeof p->pidonmach);
		p->newtlb = 0;
	}
	tp = p->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(p);

	puttlbx(0, KZERO|PTEPID(tp), 0, 0, PGSZ4K);
}

void
mmurelease(Proc *p)
{
	memset(p->pidonmach, 0, sizeof p->pidonmach);
}

/*
 * Process must be splhi
 */
int
newtlbpid(Proc *p)
{
	int i, s;
	Proc **h;

	i = m->lastpid;
	h = m->pidproc;
	for(s = 0; s < NTLBPID; s++) {
		i++;
		if(i >= NTLBPID)
			i = 1;
		if(h[i] == 0)
			break;
	}

	if(h[i])
		purgetlb(i);
	if(h[i] != 0)
		panic("newtlb");

	m->pidproc[i] = p;
	p->pidonmach[m->machno] = i;
	m->lastpid = i;

	return i;
}

void
putmmu(ulong tlbvirt, ulong tlbphys, Page *pg)
{
	int s;
	short tp;
	char *ctl;
	Softtlb *entry;

	s = splhi();
	tp = up->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(up);

	/*
	 * Fix up EISA memory address which are greater
	 * than 32 bits physical: 0x100000000
	 */
	if((tlbphys&0xffff0000) == PPN(Eisamphys)) {
		tlbphys &= ~PPN(Eisamphys);
		tlbphys |= 0x04000000;
	}
	tlbvirt |= PTEPID(tp);
	if((tlbphys & PTEALGMASK) != PTEUNCACHED) {
		tlbphys &= ~PTEALGMASK;
		tlbphys |= PTECOHERXCLW;
	}

	entry = putstlb(tlbvirt, tlbphys);
	puttlb(entry->virt, entry->phys0, entry->phys1);

	ctl = &pg->cachectl[m->machno];
	switch(*ctl) {
	case PG_TXTFLUSH:
		icflush((void*)pg->va, BY2PG);
		*ctl = PG_NOFLUSH;
		break;
	case PG_DATFLUSH:
		dcflush((void*)pg->va, BY2PG);
		*ctl = PG_NOFLUSH;
		break;
	case PG_NEWCOL:
		cleancache();		/* Too expensive */
		*ctl = PG_NOFLUSH;
		break;
	}
	splx(s);
}

void
purgetlb(int pid)
{
	int i, mno;
	Proc *sp, **pidproc;
	Softtlb *entry, *etab;

	m->tlbpurge++;

	/*
	 * find all pid entries that are no longer used by processes
	 */
	mno = m->machno;
	pidproc = m->pidproc;
	for(i=1; i<NTLBPID; i++) {
		sp = pidproc[i];
		if(sp && sp->pidonmach[mno] != i)
			pidproc[i] = 0;
	}

	/*
	 * shoot down the one we want
	 */
	sp = pidproc[pid];
	if(sp != 0)
		sp->pidonmach[mno] = 0;
	pidproc[pid] = 0;

	/*
	 * clean out all dead pids from the stlb;
	 */
	entry = m->stb;
	for(etab = &entry[STLBSIZE]; entry < etab; entry++)
		if(pidproc[TLBPID(entry->virt)] == 0)
			entry->virt = 0;

	/*
	 * clean up the hardware
	 */
	for(i=TLBROFF; i<NTLB; i++)
		if(pidproc[TLBPID(gettlbvirt(i))] == 0)
			TLBINVAL(i, pid);
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
clearmmucache(void)
{
}

Softtlb*
putstlb(ulong tlbvirt, ulong tlbphys)
{
	int k;
	Softtlb *entry;

	k = tlbvirt & BY2PG;
	tlbvirt &= ~BY2PG;

	entry = &m->stb[(tlbvirt ^ (tlbvirt>>13)) & (STLBSIZE-1)];
	if(entry->virt != tlbvirt) {
		entry->virt = tlbvirt;
		entry->phys0 = 0;
		entry->phys1 = 0;
	}

	if(k)
		entry->phys1 = tlbphys;
	else
		entry->phys0 = tlbphys;

	if(entry->phys0 == 0 && entry->phys1 == 0)
		entry->virt = 0;

	return entry;
}
