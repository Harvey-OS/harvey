#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*
 * 440 mmu
 *
 * l.s has set some global TLB entries for the kernel at
 * the top of the tlb (NTLB-1 down), partly to account for
 * the way the firmware sets things up on some platforms (eg, 440).
 * The first entry not used by the kernel (eg, by kmapphys) is m->utlbhi.
 * User tlbs are assigned indices from 0 to m->utlbhi, round-robin.
 * m->utlbnext is the next to use.  Kernel tlbs are added at utlbhi, which then moves.
 *
 * In this version, the kernel TLB entries are in zone 0,
 * and user pages are in zone 1.  The kernel entries are also PID 0 (global)
 * so they are usable as the PID changes (0=no user; non-zero=user process mapped).
  */

enum {
	Minutlb=	NTLB/2,	/* at least this many for user process */
};

#define	DEBUG	0

Softtlb	softtlb[MAXMACH][STLBSIZE];

static int	newtlbpid(Proc*);
static void	purgetlb(int);
static void putstlb(int, u32int, u32int, u32int);

static int
lookstlb(int pid, u32int va)
{
	int i;

	pid <<= 2;
	i = ((va>>12)^pid)&(STLBSIZE-1);
	if(m->stlb[i].hi == (va|pid))
		return i;

	return -1;
}

void
tlbdump(char *s)
{
	int i, p;
	u32int hi, lo, mi, stid;

	p = pidget();
	iprint("tlb[%s] pid %d lastpid %d utlbnext %d\n",
		s, p, m->lastpid, m->utlbnext);
	for(i=0; i<NTLB; i++){
		hi = tlbrehi(i);
		if(hi & TLBVALID){
			stid = stidget();
			lo = tlbrelo(i);
			mi = tlbremd(i);
			iprint("%.2d %8.8ux %8.8ux %8.8ux %3d %2d\n",
				i, hi, mi, lo, stid, lookstlb(i, TLBEPN(hi)));
		}
		if(i == m->utlbhi)
			iprint("-----\n");
	}
	stidput(p);	/* tlbrehi changes STID */
}

void
alltlbdump(void)
{
	Mreg s;
	Proc *p;
	int i, machno;

	s = splhi();
	machno = m->machno;
	iprint("cpu%d pidproc:\n", machno);
	for(i=0; i<nelem(m->pidproc); i++){
		p = m->pidproc[i];
		if(p != nil && (p->mmupid != i || p == up))
			iprint("%d -> %ld [%d]\n", i, p->pid, p->mmupid);
	}
	tlbdump("final");
	splx(s);
}

/*
 * l.s provides a set of tlb entries for the kernel, allocating
 * them starting at the highest index down.  user-level entries
 * will be allocated from index 0 up.
 */
void
mmuinit(void)
{
	int i;

	pidput(0);
	stidput(0);
	m->stlb = softtlb[m->machno];
	if(DEBUG)
		tlbdump("init0");

	m->lastpid = 0;
	m->utlbhi = 0;
	for(i = 0; i < NTLB; i++){
		if(tlbrehi(i) & TLBVALID)
			break;
		m->utlbhi = i;
		stidput(0);
		tlbwrx(i, 0, 0, 0);
	}
	pidput(0);
	stidput(0);
	if(DEBUG)
		tlbdump("init1");
}

void
flushmmu(void)
{
	Mreg s;

	s = splhi();
	up->newtlb = 1;
	mmuswitch(up);
	splx(s);
}

/*
 * called with splhi
 */
void
mmuswitch(Proc *p)
{
	int pid;

	if(p->newtlb){
		p->mmupid = 0;
		p->newtlb = 0;
	}
	pid = p->mmupid;
	if(pid == 0 && !p->kp)
		pid = newtlbpid(p);
	pidput(pid);
	stidput(pid);
}

void
mmurelease(Proc* p)
{
	p->mmupid = 0;
	pidput(0);
	stidput(0);
}

void
putmmu(uintptr va, uintptr pa, Page* page)
{
	Mreg s;
	int x, tp;
	char *ctl;
	u32int tlbhi, tlblo, tlbmi;

	if(va >= KZERO)
		panic("mmuput");

	tlbhi = TLBEPN(va) | TLB4K | TLBVALID;
	tlbmi = TLBRPN(pa);	/* TO DO: 36-bit physical memory */
	tlblo = TLBUR | TLBUX | TLBSR;	/* shouldn't need TLBSX */
	if(pa & PTEWRITE)
		tlblo |= TLBUW | TLBSW;
	if(pa & PTEUNCACHED)
		tlblo |= TLBI | TLBG;

	s = splhi();
	tp = up->mmupid;
	if(tp == 0){
		if(up->kp)
			panic("mmuput kp");
		tp = newtlbpid(up);
		pidput(tp);
		stidput(tp);
	}
	else if(pidget() != tp || stidget() != tp){
		iprint("mmuput: cpu%d pid up->mmupid=%d pidget=%d stidget=%d s=%#ux\n",
			m->machno, tp, pidget(), stidget(), s);
		alltlbdump();
		panic("mmuput: cpu%d pid %d %d s=%#ux",
			m->machno, tp, pidget(), s);
	}

	/* see if it's already there: note that tlbsx[cc] uses STID */
	x = tlbsxcc(va);
	if(x < 0){
		if(m->utlbnext > m->utlbhi)
			m->utlbnext = 0;
		x = m->utlbnext++;
	}else if(x > m->utlbhi)
		panic("mmuput index va=%#p x=%d klo=%d", va, x, m->utlbhi);	/* shouldn't touch kernel entries */
//	if(DEBUG)
//		iprint("put %#p %#p-> %d: %8.8ux %8.8ux %8.8ux\n", va, pa, x, tlbhi, tlbmi, tlblo);
	if(stidget() != tp)
		panic("mmuput stid:pid mismatch");
	stidput(tp);
	tlbwrx(x, tlbhi, tlbmi, tlblo);
	putstlb(tp, TLBEPN(va), tlbmi, tlblo);

	ctl = &page->cachectl[m->machno];
	switch(*ctl){
	case PG_TXTFLUSH:
		dcflush(page->va, BY2PG);
		icflush(page->va, BY2PG);
		*ctl = PG_NOFLUSH;
		break;
	case PG_DATFLUSH:
		dcflush(page->va, BY2PG);
		*ctl = PG_NOFLUSH;
		break;
	case PG_NEWCOL:
//		cleancache();		/* expensive, but fortunately not needed here */
		*ctl = PG_NOFLUSH;
		break;
	}
	splx(s);
}

/*
 * Process must be splhi
 */
static int
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
		if(h[i] == nil)
			break;
	}

	if(h[i] != nil){
		purgetlb(i);
		if(h[i] != nil)
			panic("newtlb");
	}

	m->pidproc[i] = p;
	p->mmupid = i;
	m->lastpid = i;

	return i;
}

static void
purgetlb(int pid)
{
	int i;
	Proc *sp, **pidproc;
	Softtlb *entry, *etab;
	u32int hi;

	m->tlbpurge++;

	/*
	 * find all pid entries that are no longer used by processes
	 */
	pidproc = m->pidproc;
	for(i=1; i<NTLBPID; i++) {
		sp = pidproc[i];
		if(sp != nil && sp->mmupid != i)
			pidproc[i] = nil;
	}

	/*
	 * shoot down the one we want
	 */
	sp = pidproc[pid];
	if(sp != nil)
		sp->mmupid = 0;
	pidproc[pid] = nil;

	/*
	 * clean out all dead pids from the stlb;
	 */
	entry = m->stlb;
	for(etab = &entry[STLBSIZE]; entry < etab; entry++)
		if(pidproc[(entry->hi>>2)&0xFF] == nil){
			entry->hi = 0;
			entry->mid = 0;
			entry->lo = 0;
		}

	/*
	 * clean up the hardware
	 */
	for(i = 0; i <= m->utlbhi; i++){
		hi = tlbrehi(i);
		if((hi & TLBVALID) && pidproc[stidget()] == nil)
			tlbwrx(i, 0, 0, 0);
	}
	stidput(pidget());
	mbar();
	barriers();
	iccci();
}

/*
 * return required size and alignment to map n bytes in a tlb entry
 */
ulong
mmumapsize(ulong n)
{
	ulong size;
	int i;

	size = 1024;
	for(i = 0; i < 10 && size < n; i++)
		size <<= 2;
	return size;
}

/*
 * map a physical addresses at pa to va, with the given attributes.
 * the virtual address must not be mapped already.
 * if va is nil, map it at pa in virtual space.
 */
void*
kmapphys(uintptr va, uintptr pa, ulong nb, ulong attr, ulong le)
{
	Mreg s;
	int i;
	ulong size;

	if(va == 0)
		va = pa;	/* simplest is to use a 1-1 map */
	size = 1024;
	for(i = 0; i < 10 && size < nb; i++)
		size <<= 2;
	if(i >= 10)
		return 0;
	if(i == 6 || i == 8)
		i++;	/* skip sizes not implemented by hardware */
	if(m->utlbhi <= Minutlb)
		panic("kmapphys");

	s = splhi();
	stidput(0);
	tlbwrx(m->utlbhi, va | (i<<4) | TLBVALID, pa, attr | le);
	m->utlbhi--;
	stidput(pidget());
	splx(s);

	if(DEBUG)
		tlbdump("kmapphys");

	return UINT2PTR(va);
}

/*
 * return an uncached alias for the memory at a
 */
void*
mmucacheinhib(void* a, ulong nb)
{
	uintptr pa;

	if(a == nil)
		return nil;
	dcflush(PTR2UINT(a), nb);
	pa = PADDR(a);
	return kmapphys(KSEG1|pa, pa, nb, TLBWR | TLBI | TLBG, 0);
}

static void
putstlb(int pid, u32int va, u32int tlbmid, u32int tlblo)
{
	Softtlb *entry;

	pid <<= 2;
	entry = &m->stlb[((va>>12)^pid)&(STLBSIZE-1)];
	entry->hi = va | pid;
	entry->mid = tlbmid;
	entry->lo = tlblo;
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 */
uintptr
cankaddr(uintptr pa)
{
	if( /* pa >= PHYSDRAM && */ pa < PHYSDRAM + 512*MiB)
		return PHYSDRAM + 512*MiB - pa;
	return 0;
}
