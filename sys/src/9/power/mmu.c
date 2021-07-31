#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

/*
 * Called splhi, not in Running state
 */
void
mapstack(Proc *p)
{
	short tp;
	ulong tlbvirt, tlbphys;

	if(p->newtlb) {
		memset(p->pidonmach, 0, sizeof p->pidonmach);
		p->newtlb = 0;
	}

	tp = p->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(p);

	tlbvirt = USERADDR | PTEPID(tp);
	tlbphys = p->upage->pa | PTEWRITE|PTEVALID|PTEGLOBL;
	puttlbx(0, tlbvirt, tlbphys);
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

	splhi();

	ctl = &pg->cachectl[m->machno]; 
	if(*ctl == PG_TXTFLUSH) {
		dcflush((void*)pg->pa, BY2PG);
		icflush((void*)pg->pa, BY2PG);
		*ctl = PG_NOFLUSH;
	}

	p = u->p;
	tp = p->pidonmach[m->machno];
	if(tp == 0)
		tp = newtlbpid(p);

	tlbvirt |= PTEPID(tp);
	putstlb(tlbvirt, tlbphys);
	puttlb(tlbvirt, tlbphys);
	m->pidhere[tp] = 1;
	spllo();
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
	 * garbage collect pids with no entries
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
			puttlbx(i, KZERO | PTEPID(i), 0);
}

void
flushmmu(void)
{
	splhi();
	u->p->newtlb = 1;
	mapstack(u->p);
	spllo();
}

void
clearmmucache(void)
{
}

void
invalidateu(void)
{
	puttlbx(0, KZERO | PTEPID(0), 0);
	putstlb(KZERO | PTEPID(0), 0);
}

void
putstlb(ulong tlbvirt, ulong tlbphys)
{
	Softtlb *entry;

	/* This hash function is also coded into utlbmiss in l.s */
	entry = &m->stb[((tlbvirt<<1) ^ (tlbvirt>>12)) & (STLBSIZE-1)];
	entry->phys = tlbphys;
	entry->virt = tlbvirt;
	if(tlbphys == 0)
		entry->virt = 0;
}
