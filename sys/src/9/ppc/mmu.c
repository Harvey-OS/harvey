#include	<u.h>
#include	<ureg.h>
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

/*
 *	We have one page table per processor.
 *
 *	Different processes are distinguished via the VSID field in
 *	the segment registers.  As flushing the entire page table is an
 *	expensive operation, we implement an aging algorithm for
 *	mmu pids, with a background kproc to purge stale pids en mass.
 *
 *	This needs modifications to run on a multiprocessor.
 */

static ulong	ptabsize;			/* number of bytes in page table */
static ulong	ptabmask;		/* hash mask */

/*
 *	VSID is 24 bits.  3 are required to distinguish segments in user
 *	space (kernel space only uses the BATs).  pid 0 is reserved.
 *	The top 2 bits of the pid are used as a `color' for the background
 *	pid reclamation algorithm.
 */

enum {
	PIDBASE = 1,
	PIDBITS = 21,
	COLBITS = 2,
	PIDMAX = ((1<<PIDBITS)-1),
	COLMASK = ((1<<COLBITS)-1),
};

#define	VSID(pid, i)	(((pid)<<3)|i)
#define	PIDCOLOR(pid)	((pid)>>(PIDBITS-COLBITS))
#define	PTECOL(color)	PTE0(1, VSID(((color)<<(PIDBITS-COLBITS)), 0), 0, 0)

void
mmuinit(void)
{
	int lhash, mem, i;
	ulong memsize;

	memsize = conf.npage * BY2PG;
	if(ptabsize == 0) {
		/* heuristically size the hash table */
		lhash = 10;
		mem = (1<<23);
		while(mem < memsize) {
			lhash++;
			mem <<= 1;
		}
		ptabsize = (1<<(lhash+6));
		ptabmask = (1<<lhash)-1;
	}
	m->ptabbase = (ulong)xspanalloc(ptabsize, 0, ptabsize);
	/* set page table base address */
	putsdr1(PADDR(m->ptabbase) | (ptabmask>>10));
	m->mmupid = PIDBASE;
	m->sweepcolor = 0;
	m->trigcolor = COLMASK;

	for(i = 0; i < 16; i++)
		putsr(i<<28, 0);
}

static int
work(void*)
{
	return PIDCOLOR(m->mmupid) == m->trigcolor;
}

void
mmusweep(void*)
{
	Proc *p;
	int i, x, sweepcolor;
	ulong *ptab, *ptabend, ptecol;

	for(;;) {
		if(PIDCOLOR(m->mmupid) != m->trigcolor)
			sleep(&m->sweepr, work, nil);

		sweepcolor = m->sweepcolor;
		x = splhi();
		p = proctab(0);
		for(i = 0; i < conf.nproc; i++, p++)
			if(PIDCOLOR(p->mmupid) == sweepcolor)
				p->mmupid = 0;
		splx(x);

		ptab = (ulong*)m->ptabbase;
		ptabend = (ulong*)(m->ptabbase+ptabsize);
		ptecol = PTECOL(sweepcolor);
		while(ptab < ptabend) {
			if((*ptab & PTECOL(3)) == ptecol){
				*ptab = 0;
			}
			ptab += 2;
		}

		m->sweepcolor = (sweepcolor+1) & COLMASK;
		m->trigcolor = (m->trigcolor+1) & COLMASK;
	}
}

int
newmmupid(void)
{
	int pid, newcolor, i, x;
	Proc *p;

	pid = m->mmupid++;
	if(m->mmupid > PIDMAX){
		/* Used up all mmupids, start again from first.  Flush the tlb
		 * to delete any entries with old pids remaining, then reassign
		 * all pids.
		 */
		m->mmupid = PIDBASE;
		x = splhi();
		tlbflushall();
		p = proctab(0);
		for(i = 0; i < conf.nproc; i++, p++)
			p->mmupid = 0;
		splx(x);
		wakeup(&m->sweepr);
	}
	newcolor = PIDCOLOR(m->mmupid);
	if(newcolor != PIDCOLOR(pid)) {
		if(newcolor == m->sweepcolor) {
			/* desperation time.  can't block here.  punt to fault/putmmu */
			print("newmmupid: %uld: no free mmu pids\n", up->pid);
			if(m->mmupid == PIDBASE)
				m->mmupid = PIDMAX;
			else
				m->mmupid--;
			pid = 0;
		}
		else if(newcolor == m->trigcolor)
			wakeup(&m->sweepr);
	}
	up->mmupid = pid;
	return pid;
}

void
flushmmu(void)
{
	int x;

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
	int i, mp;
	ulong r;

	if(p->kp) {
		for(i = 0; i < 8; i++)
			putsr(i<<28, 0);
		return;
	}

	if(p->newtlb) {
		p->mmupid = 0;
		p->newtlb = 0;
	}
	mp = p->mmupid;
	if(mp == 0)
		mp = newmmupid();

	for(i = 0; i < 8; i++){
		r = VSID(mp, i)|BIT(1)|BIT(2);
		putsr(i<<28, r);
	}
}

void
mmurelease(Proc* p)
{
	p->mmupid = 0;
}

void
putmmu(ulong va, ulong pa, Page *pg)
{
	int mp;
	char *ctl;
	ulong *p, *ep, *q, pteg;
	ulong vsid, hash;
	ulong ptehi, x;
	static ulong pva;

	/*
	 *	If mmupid is 0, mmuswitch/newmmupid was unable to assign us
	 *	a pid, hence we faulted.  Keep calling sched() until the mmusweep
	 *	proc catches up, and we are able to get a pid.
	 */
	while((mp = up->mmupid) == 0)
		sched();

	vsid = VSID(mp, va>>28);
	hash = (vsid ^ ((va>>12)&0xffff)) & ptabmask;
	ptehi = PTE0(1, vsid, 0, va);
	pteg = m->ptabbase + BY2PTEG*hash;

	p = (ulong*)pteg;
	ep = (ulong*)(pteg+BY2PTEG);
	q = nil;

	while(p < ep) {
		x = p[0];
		if(x == ptehi) {
			q = p;
			break;
		}
		if(q == nil && (x & BIT(0)) == 0)
			q = p;
		p += 2;
	}
	if(q == nil) {
		q = (ulong*)(pteg+m->slotgen);
		m->slotgen = (m->slotgen + BY2PTE) & (BY2PTEG-1);
	}

	if (q[0] != ptehi || q[1] != pa){
		tlbflush(va);
		m->tlbpurge++;
	}
	q[0] = ptehi;
	q[1] = pa;

	ctl = &pg->cachectl[m->machno];
	switch(*ctl) {
	case PG_NEWCOL:
	default:
		panic("putmmu: %d\n", *ctl);
		break;
	case PG_TXTFLUSH:
		dcflush((void*)pg->va, BY2PG);
		icflush((void*)pg->va, BY2PG);
		*ctl = PG_NOFLUSH;
		break;
	case PG_NOFLUSH:
		break;
	}

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
	if(pa >= -KZERO)
		return 0;
	return -KZERO - pa;
}

