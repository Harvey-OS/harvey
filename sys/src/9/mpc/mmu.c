#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#define TLBINVLAID	KZERO

void
mmuinit(void)
{
	int i;

	print("mmuinit\n");
	for(i=0; i<STLBSIZE; i++)
		m->stb[i].virt = TLBINVLAID;
}

void
flushmmu(void)
{
	int x;

if(0)print("flushmmu(%ld)\n", up->pid);
	x = splhi();
	up->newtlb = 1;
	mmuswitch(up);
	splx(x);
}

/*
 * called with splhi
 */
void
mmuswitch(Proc *p)
{
	int tp;

if(0)print("mmuswitch()\n");
	if(p->newtlb) {
		memset(p->pidonmach, 0, sizeof p->pidonmach);
		p->newtlb = 0;
	}
	tp = p->pidonmach[m->machno];
	putcasid(tp);
}

void
mmurelease(Proc* p)
{
if(0)print("mmurelease(%ld)\n", p->pid);
	memset(p->pidonmach, 0, sizeof p->pidonmach);
}

void
purgetlb(int pid)
{
	int i, mno;
	Proc *sp, **pidproc;
	Softtlb *entry, *etab;

if(0)print("purgetlb: pid = %d\n", pid);
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
			entry->virt = TLBINVLAID;

	/*
	 * clean up the hardware
	 */
	tlbflushall();
}

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

	if(h[i]) {
		i = m->purgepid+1;
		if(i >= NTLBPID)
			i = 1;
		 m->purgepid = i;
		purgetlb(i);
	}

	if(h[i] != 0)
		panic("newtlb");

	m->pidproc[i] = p;
	p->pidonmach[m->machno] = i;
	m->lastpid = i;
if(0)print("newtlbpid: pid=%ld = tlbpid = %d\n", p->pid, i);
	return i;
}

void
putmmu(ulong va, ulong pa, Page *pg)
{
	char *ctl;
	int tp;
	ulong h;

	qlock(&m->stlblk);

	tp = up->pidonmach[m->machno];
	if(tp == 0) {
		tp = newtlbpid(up);
		putcasid(tp);
	}

	h = ((va>>12)^(va>>24)^(tp<<8)) & 0xfff;
	m->stb[h].virt = va|tp;
	m->stb[h].phys = pa;
	tlbflush(va);

	qunlock(&m->stlblk);

	ctl = &pg->cachectl[m->machno];
if(0)print("putmmu tp=%d h=%ld va=%lux pa=%lux ctl=%x\n", tp, h,va, pa, *ctl);
	switch(*ctl) {
	default:
		panic("putmmu: %d\n", *ctl);
		break;
	case PG_NOFLUSH:
		break;
	case PG_TXTFLUSH:
		dcflush((void*)pg->va, BY2PG);
		icflush((void*)pg->va, BY2PG);
		*ctl = PG_NOFLUSH;
		break;
	case PG_NEWCOL:
print("PG_NEWCOL!!\n");
		*ctl = PG_NOFLUSH;
		break;
	}
}
