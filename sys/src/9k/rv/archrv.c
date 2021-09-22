/*
 * risc-v rv64gc dependencies
 * initially for microchip polarfire icicle, beagle v and hifive unmatched.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "riscv.h"

enum {
	Debug	= 0,
	Reservedways = 1,	/* we need one way not enabled for cache */
};

typedef struct L2cache L2cache;
struct L2cache {
	union {
		uchar _0_[0x200];
		struct {
			/* le bytes: banks, ways, lg sets, lg cb bytes (ro) */
			ulong	config;
			ulong	_1_;
			ulong	wayenable; /* low byte: ways enabled - 1 */
		};
	};
	uvlong	flush64;	/* write phys addr of cache line to flush here */
};

typedef struct Pagingmode Pagingmode;
struct Pagingmode {
	uchar	bits;
	uchar	levels;
	uvlong	satpmode;
};

uvlong pagingmode = Sv39;		/* minimum required on rv64 */

extern int nrdy;			/* from proc.c */

void
l2init(void)
{
	int enabways, maxways;
	L2cache *l2c = (L2cache *)soc.l2cache;

	if (l2c == nil)
		return;
	enabways = (l2c->wayenable & MASK(8)) + 1;
	maxways = (l2c->config>>8) & MASK(8);
	if (enabways < maxways - Reservedways) {
		print("%d of %d l2 cache ways enabled at entry; setting to %d\n",
			enabways, maxways, maxways - Reservedways);
		coherence();
		/* index of last way enabled for cache */
		l2c->wayenable = maxways - Reservedways - 1;
		coherence();
	}
}

/*
 * flush l2 cache from virtual address `vaddr' for `len' bytes.
 * "flush" means "write back the cache line if dirty, then invalidate it".
 * not needed on all systems.  only pre-release beagle v needs it.
 */
void
l2cacheflush(void *vaddr, uintptr len)
{
	uintptr addr, last;
	L2cache *l2c = (L2cache *)soc.l2cache;

	coherence();
	if (l2c == nil)
		return;
	addr = PADDR(vaddr);
	last = (addr + len + CACHELINESZ-1) & ~((uintptr)CACHELINESZ-1);
	addr &= ~((uintptr)CACHELINESZ-1);	/* start of first cache line */
	for (; addr < last; addr += CACHELINESZ) {
		l2c->flush64 = addr;
		coherence();
	}
}

static long cpumax;

static void
setcpumax(void)
{
	cpumax = m->cpuhz / (HZ*32);
	if (cpumax < 1)
		cpumax = 1;
}

/*
 * wait for *vp to change from val, or for an interrupt.
 * currently on risc-v, we can only wait for an interrupt.
 * see the privileged ISA spec for WFI; it's a bit subtle.
 * note that pause, halt, ainc, adec, and the spl* functions contain fences.
 */
int
waitfor(int *vp, int val)
{
	long max;
	Mreg ie;
	Mpl pl;
	void (*idler)(void);

	/* arrange that any interrupt will just resume WFI or PAUSE */
	pl = splhi();
	ie = getsie();
	putsie(ie | Superie);
	/* maybe sbi is trapping WFI, so try it the sbi way */
	if (0 && havesbihsm)			/* TODO */
		sbihartsuspend();
	else {
		/* limit pause instructions, mostly */
		if (cpumax == 0 && m->cpuhz)
			setcpumax();
		max = cpumax;
		idler = idlepause? pause: halt;
		while (*vp == val && max-- > 0 && (getsip() & Superie) == 0)
			/* no intrs pending, clock tick scheduled */
			idler();
	}
	putsie(ie);
	splx(pl);
	return *vp;
}

/* interface to the port code */
void
portmonitor(void *)
{
}

/*
 * only if mwait can be interrupted will we be able to
 * detect lock loops at high PL.
 *
 * this may seem to be a form of optimizing the idle loop, but mwait
 * (when available) should consume less power and fewer instructions
 * on many-core systems with more lock contention (e.g., NPROC=24 mk).
 */
void
portmwait(void)
{
}

void (*mwaitforlock)(Lock *, ulong) = portmwaitforlock;

Pagingmode pagingmodes[] = {
	64,	6,	Sv64,
	57,	5,	Sv57,
	48,	4,	Sv48,
	39,	3,	Sv39,
};

int
pagingmode2bits(uvlong pagingmode)
{
	Pagingmode *pm;

	for (pm = pagingmodes; pm < pagingmodes + nelem(pagingmodes); pm++)
		if (pm->satpmode == pagingmode)
			return pm->bits;
	return 0;
}

int
pagingmode2levels(uvlong pagingmode)
{
	Pagingmode *pm;

	for (pm = pagingmodes; pm < pagingmodes + nelem(pagingmodes); pm++)
		if (pm->satpmode == pagingmode)
			return pm->levels;
	return 0;
}

void
cpuidinit(void)
{
	print("%d-bit virtual addresses in use\n", pagingmode2bits(pagingmode));
}

/*
 * 32-bit little-endian versions of the macros, for tinyemu
 */

uvlong
rdcltime(Clint *clint)
{
	ulong *p;

	p = (ulong *)&clint->mtime;
	return p[0] | (uvlong)p[1] << 32;
}

uvlong
rdcltimecmp(Clint *clint)
{
	ulong *p;

	p = (ulong *)&clint->mtimecmp[m->hartid];
	return p[0] | (uvlong)p[1] << 32;
}

void
wrcltime(Clint *clint, uvlong v)
{
	ulong *p;

	p = (ulong *)&clint->mtime;
	p[1] = v>>32;
	p[0] = v;
}

void
wrcltimecmp(Clint *clint, uvlong v)
{
	ulong *p;

	p = (ulong *)&clint->mtimecmp[m->hartid];
	p[0] = ~0;
	p[1] = v>>32;
	p[0] = v;
}

static int mwords[1];
static int *mwaitwd = mwords;		/* make safe from the start */

void countipi(void);

void
prwficause(uvlong stclnt, uvlong clntnow)
{
	uvlong reason;

	iprint("cpu%d wfi woke: %,lld clints due to ",
		m->machno, clntnow - stclnt);
	reason = getsip();
	if (reason & Seie)
		iprint("ext. ");
	if (reason & Stie)
		iprint("clock ");
	if (reason & Ssie)
		iprint("ipi ");
	if ((reason & Superie) == 0)
		iprint("pause expired");
	else
		iprint("intr(s)");
	iprint("; sip = %#p\n", reason);
}

/*
 *  put the processor in the halt state if we've no processes to run.
 *  an interrupt will get us going again.
 *  called at spllo from proc.c, but also in infinite loops elsewhere.
 */
void
idlehands(void)
{
	int awaitipi;
	vlong clntdiff;
	uvlong stclnt, clntnow, clntnext;
	Mpl pl;
	Mreg pending;
	Clint *clnt;

	/*
	 * if there is work ready to do, just return.
	 */
	coherence();
	if (nrdy != 0 || active.exiting || active.rebooting)
		return;

	pl = splhi();
	clnt = clintaddr();
	stclnt = clntnow = RDCLTIME(clnt);
	clntnext = RDCLTIMECMP(clnt);
	clntdiff = clntnext - clntnow;
	if (clntnext <= clntnow + 2*sys->clintsperµs ||
	    clntdiff >= 2*timebase/HZ) {
		/*
		 * tick expired or is imminent: return and let it be serviced,
		 * or no current reasonable deadline (really shouldn't happen).
		 */
		splx(pl);
		return;
	}

	USED(stclnt);
	/* next tick is in the near future, but not imminent */
	if (Debug) {
		iprint("cpu%d wfi clints before intr %lld\n",
			m->machno, clntdiff);
		coherence();
		stclnt = RDCLTIME(clnt);	/* reset after printing */
	}

	/*
	 * ask for an ipi from another cpu if work becomes possibly available.
	 * if no other devices or processors interrupt, the clock will.
	 * waitfor is typically FENCE, WFI, which conserves power, thus heat.
	 * this works fine on the icicle but the unmatched and maybe the beagle
	 * act strangely.
	 */
	m->ipiwait = awaitipi = sys->nonline > 1;
	waitfor(mwaitwd, *mwaitwd);
	pending = getsip();

	if (Debug) {
		coherence();
		prwficause(stclnt, RDCLTIME(clnt));
	}

	/*
	 * if we popped out of wfi due to an ipi, clear it.  as a side-effect,
	 * if we waited in wfi, the ipi won't have been automatically counted
	 * for irqalloc since it wasn't serviced.
	 */
	if (awaitipi && amoswapw(&m->ipiwait, 0) == 0)
		countipi();
	clearipi();

	/* shouldn't return to pl lo trigger pending clock interrupt? */
//	if (pending & Stie)
//		trapclockintr(nil, clnt);	/* nil Ureg* is risky */
	USED(pending);
	splx(pl);
}

/*
 * wake some cpus in wfi state.  an interrupt will make one cpu break
 * out of wfi, but a qunlock probably should wake any idlers too.
 *
 * idlewake is called from the locking primitives, among others, so we
 * can't use them within it.  however, note that on riscv64, ainc and adec
 * use atomic memory operations, not locks.
 *
 * try to spread the load around fairly.  assume there's no point in
 * waking more cpus than currently-runnable processes.
 * only interrupt cpus advertising that they are waiting for an ipi.
 * currently only works for first 64 cpus.  by the time we have that
 * many cpus, we should have the SBI HSM functions available.
 */
void
idlewake(void)
{
	int nonline, hart, left, max, nrdym1;
	uint cpu, startcpu;
	uvlong hartbm;
	Mach *mp;
	Mpl pl;
	static uint currcpu;	/* atomically incremented; take its modulus */

	ainc(mwaitwd);
	nonline = sys->nonline;
	max = nonline - 1;
	if (max <= 0)
		return;

	pl = splhi();
	hartbm = 0;
	/* assume on-line cpus are 0 — nonline-1 */
	cpu = startcpu = currcpu % nonline;
	/*
	 * we can run one runnable process on this cpu, so wake up
	 * enough waiting cpus to run the others, if available.
	 * this is somewhat approximate as the other cpus may change
	 * state underfoot.
	 *
	 * Using MIN(nrdy-1, max) screws up locks for no obvious reason.
	 * max works, though with more overhead than previously.
	 */
	nrdym1 = nrdy - 1;
//	left = MIN(nrdym1, max);		/* fails */
	left = max;				/* works */
	while (left > 0 && max-- > 0) {
		if (cpu != m->machno && (mp = sys->machptr[cpu]) != nil &&
		    mp->online) {
			hart = mp->hartid;
			if (hart != m->hartid && (!hobbled || hart != 0) &&
			    amoswapw(&mp->ipiwait, 0)) {
				/* mp is awaiting an ipi (or other interrupt) */
				ipitohart(hart);
				left--;
			}
		}
		cpu = ainc(&currcpu) % nonline;
		if (cpu == startcpu)
			break;
		coherence();
	}
//	if (hartbm)
//		sbisendipi(&hartbm);
	USED(hartbm, nrdym1);
	splx(pl);
}

static ulong
prifexts(ulong isa, ulong exts, char *name)
{
	if ((isa & exts) == exts) {
		print(name);
		isa &= ~exts;
	}
	return isa;
}

#define EXT(let)	(1 << ((let) - 'A'))
#define EXTIMA		(EXT('I') | EXT('M') | EXT('A'))
#define EXTFD		(EXT('F') | EXT('D'))
#define EXTSU		(EXT('S') | EXT('U'))

uvlong cpuhz;

vlong
archhz(void)
{
	int bit;
	ulong isa;

	if(m->machno != 0)
		return sys->machptr[0]->cpuhz; /* cpus have to be ~identical */
	print("cpu%d: risc-v RV%d", m->machno, BI2BY*(int)sizeof(uintptr));
	if (bootmachmode) {
		isa = prifexts(misa, EXTIMA|EXTFD, "G");
		isa = prifexts(isa, EXTIMA, "IMA");
		isa = prifexts(isa, EXTFD, "FD");
		isa = prifexts(isa, EXT('C'), "C");
		isa &= MASK('z'+1-'a');		/* isolate extensions */
		for (bit = 0; bit < 'z'+1-'a'; bit++)
			if (isa & (1<<bit))
				print("%c", 'A' + bit);
		if ((misa & (EXTIMA|EXTFD)) != (EXTIMA|EXTFD))
			panic("cpu is not RV64G");
		if ((misa & EXT('C')) == 0)
			print(" no compression, many binaries won't run.");
		if ((misa & EXTSU) != EXTSU)
			panic("don't have super & user modes; hopeless.");
	} else
		print("G");				/* can't easily tell */
	print(" at %d MHz\n", (uint)(cpuhz / (1000*1000)));
	l2init();		/* meanwhile, other cpus are spinning */
	return cpuhz;
}

static void
addpgsz(int lg2)
{
	int npg;

	assert(m->npgsz < NPGSZ);
	npg = m->npgsz++;
	m->pgszlg2[npg] = lg2;
	m->pgszmask[npg] = VMASK(lg2);
}

int
archmmu(void)
{
	int lvl, lvls;

	if (m->npgsz)
		return m->npgsz;
	CTASSERT(PGSZ == 4*KB, PGSZ_4K);
	lvls = pagingmode2levels(pagingmode);
	for (lvl = 0; lvl < lvls; lvl++)
		addpgsz(PGLSHFT(lvl));
	return m->npgsz;
}

static int
fmtP(Fmt* f)
{
	uintmem pa;

	pa = va_arg(f->args, uintmem);

	if(f->flags & FmtSharp)
		return fmtprint(f, "%#16.16p", pa);

	return fmtprint(f, "%llud", pa);
}

static int
fmtL(Fmt* f)
{
	return fmtprint(f, "%#16.16p", va_arg(f->args, Mpl));
}

static int
fmtR(Fmt* f)
{
	return fmtprint(f, "%#16.16p", va_arg(f->args, Mreg));
}

void
archfmtinstall(void)
{
	/*
	 * Architecture-specific formatting. Not as neat as they
	 * could be (e.g. there's no defined type for a 'register':
	 *	L - Mpl, mach priority level
	 *	P - uintmem, physical address
	 *	R - register
	 * With a little effort these routines could be written
	 * in a fairly architecturally-independent manner, relying
	 * on the compiler to optimise-away impossible conditions,
	 * and/or by exploiting the innards of the fmt library.
	 */
	fmtinstall('P', fmtP);

	fmtinstall('L', fmtL);
	fmtinstall('R', fmtR);
}

/* delay for at least microsecs, avoiding the watchdog if necessary */
void
microdelay(vlong microsecs)
{
	uvlong t, now, nxtdog, mhz;

	if (microsecs <= 0)
		return;
	now = rdtsc();
	assert(m);
	mhz = m->cpumhz;
	if (mhz == 0)
		mhz = 2000;
	t = now + mhz * microsecs;
	/* when islo(), clock interrupts will restart the dog */
	if (watchdogon && watchdog && m->machno == 0 && !islo()) {
		nxtdog = 0;
		for (now = rdtsc(); now < t; now = rdtsc()) {
			if (now > nxtdog) {
				watchdog->restart();
				nxtdog = now + Wdogms*1000ll*mhz;
			}
			pause();
		}
	} else
		while (rdtsc() < t)
			pause();
}

void
millidelay(int millisecs)
{
	if (millisecs)
		microdelay(1000ll * millisecs);
}

int
clz(Clzuint n)			/* count leading zeroes */
{
	int cnt;
	uint hibits;

	if (n == 0)
		return Clzbits;
	cnt = 0;
	for (hibits = Clzbits/2; hibits > 0; hibits /= 2)
		if ((n & (VMASK(hibits) << (Clzbits - hibits))) == 0) {
			/* high bits are zero; count and toss them */
			n <<= hibits;
			cnt += hibits;
		}
	return cnt;
}
