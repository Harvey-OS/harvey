#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"

#define TLBINVAL(x, pid)	puttlbx(x, KSEG0|((x)<<(PGSHIFT+1))|(pid), 0, 0, PGSZ)

enum {
	Debugswitch	= 0,
	Debughash	= 0,
};

void dumpstlb(void);

static ulong ktime[8];		/* only for first 8 cpus */

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

static int minfree = KPTESIZE;
static int lastfree;
static int tlbroff = TLBROFF;

void
kmapinit(void)
{
	KMap *k, *klast;

	lock(&kmaplock);
	kmapfree = kpte;
	klast = &kpte[KPTESIZE-1];
	for(k=kpte; k<klast; k++)
		k->next = k+1;
	k->next = 0;
	unlock(&kmaplock);

	m->ktlbnext = KTLBOFF;
}

void
kmapinval(void)
{
	int mno, i, curpid;
	KMap *k, *next;
	uchar *ktlbx;

	if(m->machno < nelem(ktime))
		ktime[m->machno] = MACHP(0)->ticks;
	if(m->kactive == 0)
		return;

	curpid = PTEPID(TLBPID(tlbvirt()));
	ktlbx = m->ktlbx;
	for(i = 0; i < NTLB; i++, ktlbx++){
		if(*ktlbx == 0)
			continue;
		TLBINVAL(i, curpid);
		*ktlbx = 0;
	}

	mno = m->machno;
	for(k = m->kactive; k; k = next) {
		next = k->konmach[mno];
		kunmap(k);
	}

	m->kactive = 0;
	m->ktlbnext = KTLBOFF;
}

static int
putktlb(KMap *k)
{
	int x;
	u64int virt, tlbent[4];

	virt = k->virt & ~BY2PG | TLBPID(tlbvirt());
	x = gettlbp(virt, tlbent);
	if (!m->paststartup)
		if (up) {			/* startup just ended? */
			tlbroff = KTLBOFF;
			setwired(tlbroff);
			m->paststartup = 1;
		} else if (x < 0) {		/* no such entry? use next */
			x = m->ktlbnext++;
			if(m->ktlbnext >= tlbroff)
				m->ktlbnext = KTLBOFF;
		}
	if (x < 0)		/* no entry for va? overwrite random one */
		x = puttlb(virt, k->phys0, k->phys1);
	else
		puttlbx(x, virt, k->phys0, k->phys1, PGSZ);
	m->ktlbx[x] = 1;
	return x;
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
	int s, printed = 0;
	uintptr pte, virt;
	KMap *k;

	s = splhi();
	lock(&kmaplock);

	if(kmapfree == 0) {
retry:
		unlock(&kmaplock);
		kmapinval();		/* try and free some */
		lock(&kmaplock);
		if(kmapfree == 0){
			unlock(&kmaplock);
			splx(s);
			if(printed++ == 0){
			/* using iprint here we get mixed up with other prints */
				print("%d KMAP RETRY %#p ktime %ld %ld %ld %ld %ld %ld %ld %ld\n",
					m->machno, getcallerpc(&pg),
					ktime[0], ktime[1], ktime[2], ktime[3],
					ktime[4], ktime[5], ktime[6], ktime[7]);
				delay(200);
			}
			splhi();
			lock(&kmaplock);
			goto retry;
		}
	}

	k = kmapfree;
	kmapfree = k->next;

	k->pg = pg;
	/*
	 * One for the allocation,
	 * One for kactive
	 */
	k->pc = getcallerpc(&pg);
	k->ref = 2;
	k->konmach[m->machno] = m->kactive;
	m->kactive = k;

	virt = pg->va;
	/* bits 13..12 form the cache virtual index */
	virt &= PIDX;
	virt |= KMAPADDR | ((k-kpte)<<KMAPSHIFT);

	k->virt = virt;
	pte = PPN(pg->pa)|PTECACHABILITY|PTEGLOBL|PTEWRITE|PTEVALID;
	if(virt & BY2PG) {
		k->phys0 = PTEGLOBL | PTECACHABILITY;
		k->phys1 = pte;
	}
	else {
		k->phys0 = pte;
		k->phys1 = PTEGLOBL | PTECACHABILITY;
	}

	putktlb(k);
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
//nfree();
		unlock(&kmaplock);
	}
	splx(s);
}

void
kfault(Ureg *ur)			/* called from trap() */
{
	ulong index;
	uintptr addr;
	KMap *k, *f;

	addr = ur->badvaddr;
	index = (addr & ~KSEGM) >> KMAPSHIFT;
	if(index >= KPTESIZE) {
		dumpregs(ur);
		dumptlb();
		dumpstlb();
		panic("kmapfault: %#p", addr);
	}

	k = &kpte[index];
	if(k->virt == 0) {
		dumptlb();
		dumpstlb();
		panic("kmapfault: unmapped %#p", addr);
	}

	for(f = m->kactive; f; f = f->konmach[m->machno])
		if(f == k)
			break;
	if(f == 0) {
		incref(k);
		k->konmach[m->machno] = m->kactive;
		m->kactive = k;
	}
	putktlb(k);
}

struct
{
	u64int	va;
	u64int	pl;
	u64int	ph;
} wired[NWTLB+1];		/* +1 to avoid zero size if NWTLB==0 */

void
machwire(void)
{
	int i;

//	if(m->machno == 0)
//		return;
	for(i = 0; i < NWTLB; i++)
		if(wired[i].va)
			puttlbx(i+WTLBOFF, wired[i].va, wired[i].pl,
				wired[i].ph, PGSZ);
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
mmuswitch(Proc *p)
{
	int tp;
	static char lasttext[32];

	if(Debugswitch && !p->kp){
		if(strncmp(lasttext, p->text, sizeof lasttext) != 0)
			iprint("[%s]", p->text);
		strncpy(lasttext, p->text, sizeof lasttext);
	}

	if(p->newtlb) {
		memset(p->pidonmach, 0, sizeof p->pidonmach);
		p->newtlb = 0;
	}
	tp = p->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(p);
	puttlbx(0, KSEG0|PTEPID(tp), 0, 0, PGSZ);
}

void
mmurelease(Proc *p)
{
	memset(p->pidonmach, 0, sizeof p->pidonmach);
}

void
putmmu(u64int tlbvirt, u64int tlbphys, Page *pg)
{
	short tp;
	char *ctl;
	Softtlb *entry;
	int s;

	s = splhi();
	tp = up->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(up);

	tlbvirt |= PTEPID(tp);
	if((tlbphys & PTEALGMASK) != PTEUNCACHED) {
		tlbphys &= ~PTEALGMASK;
		tlbphys |= PTECACHABILITY;
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
	for(i=tlbroff; i<NTLB; i++)
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

/* tlbvirt also has TLBPID() in its low byte as the asid */
Softtlb*
putstlb(u64int tlbvirt, u64int tlbphys)
{
	int odd;
	Softtlb *entry;

	/* identical calculation in l.s/utlbmiss */
	entry = &m->stb[stlbhash(tlbvirt)];
	odd = tlbvirt & BY2PG;		/* even/odd bit */
	tlbvirt &= ~BY2PG;		/* zero even/odd bit */
	if(entry->virt != (tlbvirt & TLBVIRTMASK)) {	/* not my entry? overwrite it */
		if(entry->virt != 0) {
			m->hashcoll++;
			if (Debughash)
				iprint("putstlb: hash collision: %#lx old virt "
					"%#p new virt %#p page %#llux\n",
					entry - m->stb, entry->virt, tlbvirt,
					tlbvirt >> (PGSHIFT+1));
		}
		entry->virt = tlbvirt & TLBVIRTMASK;
		entry->phys0 = 0;
		entry->phys1 = 0;
	}

	if(odd)
		entry->phys1 = tlbphys;
	else
		entry->phys0 = tlbphys;

	if(entry->phys0 == 0 && entry->phys1 == 0)
		entry->virt = 0;

	return entry;
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
uintptr
cankaddr(uintptr pa)
{
	if(pa >= KZERO || pa >= memsize)
		return 0;
	return memsize - pa;
}

/* print tlb entries for debug */
#define TLBPHYS(x)	((((x)&~0x3f)<<6) | ((x)&0x3f))	/* phys addr & flags */

void
dumptlb(void)
{
	Softtlb entry;
	int i;

	iprint("dump tlb\n");
	for(i=0; i<NTLB; i++) {
		gettlbx(i, &entry);
//		if(entry.phys0 != 0 || entry.phys1 != 0)
			iprint("tlb index %2d, virt %.16llux, phys0 %.16llux, phys1 %.16llux\n",
				i, entry.virt, TLBPHYS(entry.phys0), TLBPHYS(entry.phys1));
	}
}

void
dumpstlb(void)
{
	Softtlb *entry;
	int i;

	iprint("dump stlb\n");
	for(i=0; i<STLBSIZE; i++) {
		entry = &m->stb[i];
		if(entry->virt != 0)
			iprint("stlb index %2d, virt %.16llux, phys0 %.16llux, phys1 %.16llux\n",
				i, entry->virt, TLBPHYS(entry->phys0), TLBPHYS(entry->phys1));
	}
}
