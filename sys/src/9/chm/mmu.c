#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

Lock	kmaplock;

KMap	ktlb[KTLBSIZE];

KMap *	kmapfree;

void
kmapinit(void)
{
	KMap *k, *klast;

	lock(&kmaplock);
	kmapfree = ktlb;
	klast = &ktlb[KTLBSIZE-1];
	for(k=ktlb; k<klast; k++)
		k->next = k+1;
	k->next = 0;
	unlock(&kmaplock);
}

/*
 *	Arrange that the KMap'd virtual address will hit the same
 *	primary cache line as pg->va.
 */

KMap *
kmap(Page *pg)
{
	KMap *k;
	int s;
	ulong virt;

	s = splhi();
	lock(&kmaplock);

	k = kmapfree;
	if(k == 0)
		panic("kmap");
	kmapfree = k->next;
	k->next = (void *)pg;	/* for debugging */

	virt = pg->va;
	if((virt & 0xffff0000) == USERADDR)
		virt = USERADDR;
	virt &= 7<<12;	/* bits 14..12 form the secondary-cache virtual index */
	virt |= KMAPADDR | ((k-ktlb)<<15);
	k->virt = virt;
	if(virt & BY2PG){
		k->phys0 = PTEXCLCOHER|PTEGLOBL;
		k->phys1 = PPN(pg->pa) | PTEXCLCOHER|PTEGLOBL|PTEWRITE|PTEVALID;
	}else{
		k->phys0 = PPN(pg->pa) | PTEXCLCOHER|PTEGLOBL|PTEWRITE|PTEVALID;
		k->phys1 = PTEXCLCOHER|PTEGLOBL;
	}

	puttlb((virt & ~BY2PG) | TLBPID(tlbvirt()), k->phys0, k->phys1);
/*print("kmap: pa=0x%ux, va=0x%ux, kv=0x%ux\n", pg->pa, pg->va, k->virt);/**/
	unlock(&kmaplock);
	splx(s);
	return k;
}

void
kunmap(KMap *k)
{
	int s;

	s = splhi();
	lock(&kmaplock);

	if(k == 0)
		panic("kunmap");
/*print("kunmap: kv=0x%ux\n", k->virt);/**/
	puttlb((k->virt & ~BY2PG) | TLBPID(tlbvirt()),
		k->phys0 & ~PTEVALID, k->phys1 & ~PTEVALID);

	k->virt = 0;
	k->phys0 = 0;
	k->phys1 = 0;
	k->next = kmapfree;
	kmapfree = k;

	unlock(&kmaplock);
	splx(s);
}

void
mmunewpage(Page *pg)
{
	KMap *k;

	k = kmap(pg);
	cdirtyexcl((void*)VA(k), BY2PG);
	kunmap(k);
}

/*
 * Called splhi, not in Running state
 */
void
mapstack(Proc *p)
{
	short tp;
	ulong tlbvirt, tlbphys;

	if(p->newtlb){
		memset(p->pidonmach, 0, sizeof p->pidonmach);
		p->newtlb = 0;
	}

	tp = p->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(p);
/*
	if(p->upage->va != (USERADDR|(p->pid&0xFFFF)) && p->pid != 0)
		panic("mapstack %d 0x%lux 0x%lux", p->pid, p->upage->pa, p->upage->va);
*/
	/*
	 * don't set m->pidhere[tp] because we're only writing entry 0.
	 * USERADDR is known to be an even-numbered page.
	 */
	tlbvirt = USERADDR | PTEPID(tp);
	tlbphys = PPN(p->upage->pa) | PTEXCLCOHER|PTEWRITE|PTEVALID| PTEGLOBL;
	puttlbx(0, tlbvirt, tlbphys, PTEXCLCOHER|PTEGLOBL);
	u = (User*)USERADDR;
}

void
mmurelease(Proc *p)
{
	memset(p->pidonmach, 0, sizeof p->pidonmach);
}

/*
 * Process must be non-interruptible
 */
int
newtlbpid(Proc *p)
{
	int i, s;
	Proc *sp;
	char *h;

	s = m->lastpid;
	if(s >= NTLBPID)
		s = 1;
	i = s;
	h = m->pidhere;
	do{
		i++;
		if(i >= NTLBPID)
			i = 1;
	}while(h[i] && i != s);

	if(i == s){
		i++;
		if(i >= NTLBPID)
			i = 1;
	}
	if(h[i])
		purgetlb(i);
	sp = m->pidproc[i];
	if(sp && sp->pidonmach[m->machno] == i)
		sp->pidonmach[m->machno] = 0;
	m->pidproc[i] = p;
	p->pidonmach[m->machno] = i;
	m->lastpid = i;
	return i;
}

void
putmmu(ulong tlbvirt, ulong tlbphys, Page *pg)
{
	short tp;
	Proc *p;
	char *ctl;
	Softtlb *entry;
	int s;

	s = splhi();

	p = u->p;
/*	if(p->state != Running)
		panic("putmmu state %lux %lux %s\n", u, p, statename[p->state]);
*/
	tp = p->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(p);
	tlbvirt |= PTEPID(tp);
	if(!(tlbphys & PTEUNCACHED)){
		tlbphys &= ~PTEALGMASK;
		tlbphys |= PTEXCLCOHER;
	}
	entry = putstlb(tlbvirt, tlbphys);
	puttlb(entry->virt, entry->phys0, entry->phys1);
	m->pidhere[tp] = 1;

	ctl = &pg->cachectl[m->machno]; 
	if(*ctl == PG_TXTFLUSH) {
		icflush((void*)pg->va, BY2PG);
		*ctl = PG_NOFLUSH;
	}

	splx(s);
}

void
purgetlb(int pid)
{
	Softtlb *entry, *etab;
	char *pidhere;
	Proc *sp, **pidproc;
	int i, rpid, mno;
	char dead[NTLBPID];

	m->tlbpurge++;
	/*
	 * find all pid entries that are no longer used by processes
	 */
	mno = m->machno;
	pidproc = m->pidproc;
	memset(dead, 0, sizeof dead);
	for(i=1; i<NTLBPID; i++){
		sp = pidproc[i];
		if(!sp || sp->pidonmach[mno] != i){
			pidproc[i] = 0;
			dead[i] = 1;
		}
	}
	dead[pid] = 1;
	/*
	 * clean out all dead pids from the stlb;
	 * garbage collect any pids with no entries
	 */
	memset(m->pidhere, 0, sizeof m->pidhere);
	pidhere = m->pidhere;
	entry = m->stb;
	etab = &entry[STLBSIZE];
	for(; entry < etab; entry++){
		rpid = TLBPID(entry->virt);
		if(dead[rpid])
			entry->virt = 0;
		else
			pidhere[rpid] = 1;
	}
	/*
	 * clean up the hardware
	 */
	for(i=TLBROFF; i<NTLB; i++)
		if(!pidhere[TLBPID(gettlbvirt(i))])
			puttlbx(i, KZERO | PTEPID(i), 0, 0);
}

void
flushmmu(void)
{
	int s;

	s = splhi();
	/* easiest is to forget what pid we had.... */
	memset(u->p->pidonmach, 0, sizeof u->p->pidonmach);
	/* ....then get a new one by trying to map our stack */
	mapstack(u->p);
	splx(s);
}

void
clearmmucache(void)
{
}

void
invalidateu(void)
{
	puttlbx(0, USERADDR | TLBPID(tlbvirt()), 0, 0);
}

Softtlb *
putstlb(ulong tlbvirt, ulong tlbphys)
{
	Softtlb *entry;
	int k;

	k = tlbvirt & BY2PG;
	tlbvirt &= ~BY2PG;
	entry = &m->stb[(tlbvirt ^ (tlbvirt>>13)) & (STLBSIZE-1)];
	if(entry->virt != tlbvirt){
		entry->virt = tlbvirt;
		entry->phys0 = 0;
		entry->phys1 = 0;
	}
	if(k)
		entry->phys1 = tlbphys;
	else
		entry->phys0 = tlbphys;
	if(!(entry->phys0 || entry->phys1))
		entry->virt = 0;
	return entry;
}

#define	LINESIZE	128	/* secondary cache line size */

void *
pxspanalloc(ulong size, int align, ulong span)
{
	void *p;

	if(align < LINESIZE)
		align = LINESIZE;
	p = xspanalloc(size, align, span);
	cdirtyexcl(p, size);
	p = KADDR1(PADDR(p));
	memset(p, 0, size);
	return p;
}

void *
pxalloc(ulong size)
{
	return pxspanalloc(size, LINESIZE, 128*1024*1024);
}
