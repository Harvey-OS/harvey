/*
 * initialise a risc-v RV64G system enough to call main
 * with the identity map and upper addresses mapped, in supervisor mode.
 * this is traditionally done entirely in assembler, typically called l.s.
 * do not use print nor iprint here; libc/fmt learns function addresses,
 * which need to be high addresses.
 *
 * all cpus may be executing this code simultaneously.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"
#include "ureg.h"
#include "riscv.h"
#include "start.h"

#define XLEN sizeof(uintptr)
#define Dbprs if (Debug) prsnolock

enum {
	Debug = 0,
};

uintptr r10[MACHMAX] = { 1 };		/* init w anything to avoid bss */

/* verify availability of our paging mode.  it's safe (only) in machine mode. */
static void
bestpaging(void)
{
	putsatp(Svnone);
#ifdef SV48
	pagingmode = Sv48;
#endif
	putsatp(pagingmode | (PHYSMEM/PGSZ));
	if (getsatp() != (pagingmode | (PHYSMEM/PGSZ)))
		panic("bestpaging: chosen paging mode not available");
	putsatp(Svnone);
}

/*
 * do machine-mode set up so we can leave machine mode.
 * set up for supervisor mode, needed for paging in kernel.
 * mostly punting faults to supervisor mode, preparing for clock
 * interrupts, and disabling paging.  mret will switch us to supervisor mode.
 *
 * needs m set.
 */
void
mach_to_super(void)
{
	/* no machine mode intrs yet */
	putmsts(getmsts() & ~Mie);
	putmie(getmie() & ~Machie);
	putmip(getmip() & ~Machie);

	/* set up normal delegations */
	putmcounteren(~0ull);		/* expose timers to super */
	putmideleg(~0ull);		/* punt intrs to super */
	/* punt except.s to super except env call from super */
	putmedeleg(~0ull & ~(1<<Envcallsup));
	mideleg = getmideleg();		/* how much of it stuck? */

	/* machine-mode set up: splhi, le, no traps, set prev mode to super */
	putmsts((getmsts() &
		~(Sbe64|Mbe64|Uxl|Sxl|Mpp|Ube|Tvm|Tw|Sie|Mie|Spie|Mpie)) |
		Mxlen64<<Uxlshft|Mxlen64<<Sxlshft|Mppsuper|Spp|Sum|Mxr);

	bestpaging();
	misa = getmisa();		/* export csr to C */
	putmie(getmie() | Superie|Machie); /* S+M intrs on */

	/* set up machine traps */
	putmtvec(ensurelow((uintptr)mtrap));
	putmscratch(ensurelow((uintptr)m));

	/* switch to super mode & enable mach intrs. */
	putmsts((getmsts() & ~(Spie|Sie)) | Mpie);
	mret();
}

static void
superintroff(void)
{
	putsie(getsie() & ~Superie);		/* no super mode intrs yet */
	putsip(getsip() & ~Superie);
	putsts(getsts() & ~(uvlong)Sie);
	WRCLTIMECMP(clintaddr(), VMASK(63));	/* no clock intrs */
	coherence();
}

static void
setptes(PTE *ptep, PTE ptebits, int n, int lvl)
{
	for (; n > 0; n--) {
		*ptep++ = ptebits;
		ptebits += (PGLSZ(lvl)>>PGSHFT) << PTESHFT;
	}
}

/* return a pointer to an initial top-level page table */
static PTE *
mkinitpgtbl(void)
{
	int lvl, nptes;
	PTE ptebits;
	PTE *ptp;
	static char toppt[2*PGSZ];	/* one more for page alignment */

	/*
	 * allocate temporary top-level page-aligned page table
	 * for 3-level page tables (Sv39).  in top-level pt for 4 levels,
	 * each PTE covers 256GB.
	 */
	ptp = (PTE*)ensurelow(((uintptr)toppt + PGSZ) & ~((uintptr)PGSZ-1));
	memset(ptp, 0, PTSZ);

	/* -1 for zero-origin level numbers */
	lvl = pagingmode2levels(pagingmode) - 1;
	nptes = 1;			/* enough for larger modes */
	if (pagingmode == Sv39)
		/*
		 * populate 1st 255GB of both maps (255 lvl 2 PTEs); leave last
		 * GB free for VMAP in upper->lower map at least.
		 */
		nptes = 255;

	/* populate id map */
	ptebits = ((0>>PGSHFT)<<PTESHFT)|PteR|PteW|PteX|Pteleafvalid;
	setptes(ptp, ptebits, nptes, lvl);

	/* populate upper -> lower map */
	setptes(&ptp[PTLX(KSEG0, lvl)], ptebits | PteG, nptes, lvl);
	return ptp;
}

static void
newmach(Mach *m, int cpu)
{
	m->machno = cpu;
	m->hartid = r10[cpu];
	putsscratch((uintptr)m);	/* ready for traps now */
}

static void
enablepaging(Sys *lowsys)		/* with id map & upper->lower map */
{
	PTE *ptp;

	ptp = mkinitpgtbl();
	lowsys->satp = pagingmode | ((uintptr)ptp / PGSZ);
/**/	putsatp(lowsys->satp);
}

static void
setmach(Sys *lowsys, int cpu)
{
	m = lowsys->machptr[cpu];
	if (m == nil)
		panic("setmach: nil sys->machptr[%d]", cpu);
	if (!bootmachmode)
		m = (Mach *)ensurelow((uintptr)m);
	newmach(m, cpu);
}

static void
alignchk(void)
{
	static int align = 0x123456;

	if (align != 0x123456)
		panic("data segment misaligned\n");
}

static void
zerosyssome(Sys *lowsys, uintptr stktop)
{
	char *aftreboot;

	/* zero machstk for usage measurements */
	memset((void *)ensurelow((uintptr)lowsys->machstk), 0, MACHSTKSZ);

	/* zero Sys from after Mach stack to below reboottramp */
	memset((void *)stktop, 0, (uintptr)lowsys->reboottramp - stktop);

	/* zero Sys from syspage after Reboot to below kmesg */
	aftreboot = (char *)&lowsys->Reboot + sizeof(Reboot);
	memset(aftreboot, 0, (char *)lowsys->sysextend - aftreboot);
}

/* return stack top */
static uintptr
setstkmach0(Sys *lowsys, int cpu)
{
	uintptr stktop;

	USED(cpu);
	Dbprs("cpu0...");
	alignchk();
	stktop = ensurelow((uintptr)&lowsys->machstk[MACHSTKSZ]);
	zerosyssome(lowsys, stktop);

	m = (Mach *)ensurelow((uintptr)&lowsys->mach);
	newmach(m, 0);
	return stktop;
}

/* return stack top */
static uintptr
secwait(Sys *lowsys, int cpu)
{
	Dbprs("cpu stalling...");
	/* wait for cpu0 to allocate secondaries' data structures */
	while (lowsys->secstall)	/* cleared by schedcpus, settrampargs from reboot */
		pause();
	/*
	 * by agreement, cpu0 has now installed at least a simple page
	 * table with identity and upper->lower maps, so we can just
	 * install it, but we have to be in supervisor mode for it to
	 * have effect.  iovmapsoc hasn't run yet.
	 */
	Dbprs("cpu paging...");
/**/	putsatp(lowsys->satp);		/* initial page table */
	/* we could be in machine mode, though, so maybe no paging */
	setmach(lowsys, cpu);
	m->waiting = 1;			/* notify schedcpus that we are up */
	Dbprs("cpu waiting...");
	while (!m->online)
		pause();
	/* iovmapsoc must have run on cpu0 by now. */
	return m->stack + MACHSTKSZ;
}

/*
 * a temporary stack (in initstks) must exist when we are called.
 * don't use any high addresses until we are definitely in supervisor mode.
 * main goals: establish a permanent stack, m, sys, [sm]tvec, [sm]scratch,
 * page table.
 */
void
begin(int cpu)
{
	uintptr stktop;
	Sys *lowsys;
	static int stallnext = 1;

	/*
	 * sys is a shared global ptr, always high, but our use here will be
	 * of its (low) physical address, lowsys.  it's at the top of the first
	 * memory bank.
	 */
	putstvec((void *)ensurelow((uintptr)strap));
	lowsys = (Sys *)(PHYSMEM + membanks[0].size - MB);	/* physical */
	Dbprs("lowsys...");
	lowsys->secstall |= stallnext;
	stallnext = 0;				/* harmless race here */
	coherence();
	Dbprs("stall set...");
	if (cpu == 0) {
		/* zero bss */
		Dbprs("bss...");
		memset((char *)ensurelow((uintptr)edata), 0, end - edata);
		coherence();
	}
	sys = (Sys *)KADDR((uintptr)lowsys);
//	putstvec((void *)ensurelow((uintptr)strap));
	Dbprs("introff...");
	superintroff();

	/* cpu0 sets up, others wait */
	stktop = (cpu == 0? setstkmach0: secwait)(lowsys, cpu);
	if (bootmachmode) {
		Dbprs("to super...");
		mach_to_super();
	}

	/*
	 * we're now definitely in supervisor mode.  secondary cpus have
	 * paging enabled and a minimal map.  cpu0 has paging disabled.
	 */
	Dbprs("[new cpu]\n");
	Dbprs("putsts...");
	/* splhi, le, no traps, rv64 user, super user access, exec readable */
	putsts((getsts() & ~(uvlong)(Uxl|Ube|Sie|Spie|Spp|Mxlenmask)) |
		Mxlen64<<Uxlshft|Sum|Mxr);

	if (cpu == 0) {
		Dbprs("page tbl...");
		enablepaging(lowsys);	/* make & use initial page table */
	}

	/*
	 * paging is now on with a minimal map, on any cpu.
	 * switch to upper addresses.
	 */
	Dbprs("stack...");
/**/	putsp(stktop);		/* new permanent stack; invalidates autos */
	Dbprs("high...");
	gohigh(0); /* calls jumphigh but may return to low saved pc on stack */
	jumphigh();	/* call asm: force PC in particular high before main */

	main(m->machno);
	notreached();
}
