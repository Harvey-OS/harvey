#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

/*
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
	DEBUG = 0,
	Minutlb=	NTLB/2,	/* at least this many for user process */
};

Softtlb	softtlb[MAXMACH][STLBSIZE];

static int	newtlbpid(Proc*);
static void	purgetlb(int);
static void putstlb(int, u32int, u32int);

/* clobbers SPR_PID */
static int
probehi(uintptr va)
{
	int i;

	for (i = 0; i < NTLBPID; i++) {
		putpid(i);
		if (tlbsxcc(va) >= 0)
			return i;
	}
	return -1;
}

void
tlbdump(char *s)
{
	int i, p, ppid;
	u32int hi, lo;

	p = getpid();
	iprint("tlb[%s] pid %d lastpid %d\n", s, p, m->lastpid);
	for(i=0; i<NTLB; i++){
		hi = tlbrehi(i);
		if(hi & TLBVALID){
			ppid = probehi(TLBEPN(hi));
			lo = tlbrelo(i);
			iprint("%.2d hi %#8.8ux pid %d lo %#8.8ux\n",
				i, hi, ppid, lo);
		}
		if(i == m->utlbhi)
			iprint("-----\n");
	}
	putpid(p);	/* tlbrehi changes PID */
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

	if(DEBUG)
		tlbdump("init0");
	m->lastpid = 0;
	m->utlbhi = 0;
	for(i = 0; i < NTLB; i++){
		if(tlbrehi(i) & TLBVALID)
			break;
		m->utlbhi = i;
		tlbwrx(i, 0, 0);
	}
	if(DEBUG)
		tlbdump("init1");
	putpid(0);

	m->stlb = softtlb[m->machno];
	m->pstlb = PADDR(m->stlb);

	/*
	 * set OCM mapping, assuming:
	 *	caches were invalidated earlier;
	 *	and we aren't currently using it
	 * must also set a tlb entry that validates the virtual address but
	 * the translation is not used (see p. 5-2)
	 */
/*
	putdcr(Isarc, OCMZERO);
	putdcr(Dsarc, OCMZERO);
	putdcr(Iscntl, Isen);
	putdcr(Iscntl, Dsen|Dof);
	tlbwrx(tlbx, OCMZERO|TLB4K|TLBVALID, OCMZERO|TLBZONE(0)|TLBWR|TLBEX|TLBI);
	tlbx--;
*/
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
	int tp;

	if(p->newtlb){
		p->mmupid = 0;
		p->newtlb = 0;
	}
	tp = p->mmupid;
	if(tp == 0 && !p->kp)
		tp = newtlbpid(p);
	putpid(tp);
}

void
mmurelease(Proc* p)
{
	p->mmupid = 0;
	putpid(0);
}

void
putmmu(uintptr va, uintptr pa, Page* page)
{
	int x, s, tp;
	char *ctl;
	u32int tlbhi, tlblo;

	if(va >= KZERO)
		panic("mmuput");

	tlbhi = TLBEPN(va) | TLB4K | TLBVALID;
	tlblo = TLBRPN(pa) | TLBEX | TLBZONE(1);	/* user page */
	if(pa & PTEWRITE)
		tlblo |= TLBWR;
	/* must not set TLBG on instruction pages */
	if(pa & PTEUNCACHED)
		tlblo |= TLBI;
	/* else use write-back cache; write-through would need TLBW set */

	s = splhi();
	tp = up->mmupid;
	if(tp == 0){
		if(up->kp)
			panic("mmuput kp");
		tp = newtlbpid(up);
		putpid(tp);
	}else if(getpid() != tp)
		panic("mmuput pid %#ux %#ux", tp, getpid());

	/* see if it's already there: note that tlbsx[cc] uses current PID */
	x = tlbsxcc(va);
	if(x < 0){
		if(m->utlbnext > m->utlbhi)
			m->utlbnext = 0;
		x = m->utlbnext++;
	}else if(x > m->utlbhi)		/* shouldn't touch kernel entries */
		panic("mmuput index va=%#p x=%d klo=%d", va, x, m->utlbhi);
	if(DEBUG)
		iprint("put %#p %#p-> %d: %#ux %8.8ux\n", va, pa, x, tlbhi, tlblo);
	barriers(); sync(); isync();
	tlbwrx(x, tlbhi, tlblo);
	putstlb(tp, TLBEPN(va), tlblo);
	barriers(); sync(); isync();
	/* verify that tlb entry was written into the tlb okay */
	if (tlbsxcc(va) < 0)
		panic("tlb entry for va %#lux written into but not received by tlb",
			va);
	splx(s);

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
//		cleancache();	/* expensive, but fortunately not needed here */
		*ctl = PG_NOFLUSH;
		break;
	}
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
	int i, p;
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
			entry->lo = 0;
		}

	/*
	 * clean up the hardware
	 */
	p = getpid();
	for(i = 0; i <= m->utlbhi; i++){
		hi = tlbrehi(i);
		if(hi & TLBVALID && pidproc[getpid()] == nil)
			tlbwrx(i, 0, 0);
	}
	putpid(p);
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
	for(i = 0; i < 8 && size < n; i++)
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
	int s, i, p;
	ulong size;

	if(va == 0)
		va = pa;	/* simplest is to use a 1-1 map */
	size = 1024;
	for(i = 0; i < 8 && size < nb; i++)
		size <<= 2;
	if(i >= 8)
		return 0;
	if(m->utlbhi <= Minutlb)
		panic("kmapphys");
	s = splhi();
	p = getpid();
	putpid(0);
	tlbwrx(m->utlbhi, va | (i<<7) | TLBVALID | le, pa | TLBZONE(0) | attr);
	m->utlbhi--;
	putpid(p);
	splx(s);
	if(DEBUG)
		tlbdump("kmapphys");

	return UINT2PTR(va);
}

/*
 * return an uncached alias for the memory at a
 * (unused)
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
 */

static void
putstlb(int pid, u32int va, u32int tlblo)
{
	Softtlb *entry;

	pid <<= 2;
	entry = &m->stlb[((va>>12)^pid)&(STLBSIZE-1)];
	entry->hi = va | pid;
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
