/*
 * cortex-a clocks; excludes tegra 2 SoC clocks
 *
 * cortex-a processors include private `global' and local timers
 * at soc.scu + 0x200 (global) and + 0x600 (local).
 * the global timer is a single count-up timer shared by all cores
 * but with per-cpu comparator and auto-increment registers.
 * a local count-down timer can be used as a watchdog.
 *
 * v7 arch provides a 32-bit count-up cycle counter (at about 1GHz in our case)
 * but it's unsuitable as our source of fastticks, because it stops advancing
 * when the cpu is suspended by WFI.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

enum {
	Debug		= 0,

	Basetickfreq	= Mhz,			/* soc.µs rate in Hz */
	/* the local timers seem to run at half the expected rate */
	Clockfreqbase	= 250*Mhz / 2,	/* private timer rate (PERIPHCLK/2) */
	Tcycles		= Clockfreqbase / HZ,	/* cycles per clock tick */

	MinPeriod	= Tcycles / 100,
	MaxPeriod	= Tcycles,

	Dogtimeout	= Dogsectimeout * Clockfreqbase,
};

typedef struct Ltimer Ltimer;
typedef struct Pglbtmr Pglbtmr;
typedef struct Ploctmr Ploctmr;

/*
 * cortex-a private-intr local timer registers.  all cpus see their
 * own local timers at the same base address.
 */
struct Ltimer {
	ulong	load;		/* new value + 1 */
	ulong	cnt;		/* counts down */
	ulong	ctl;
	ulong	isr;

	/* watchdog only */
	ulong	wdrst;
	ulong	wddis;		/* wo */

	ulong	_pad0[2];
};
struct Ploctmr {
	Ltimer	loc;
	Ltimer	wd;
};

enum {
	/* ctl bits */
	Tmrena	= 1<<0,		/* timer enabled */
	Wdogena = Tmrena,	/* watchdog enabled */
	Xreload	= 1<<1,		/* reload on intr; periodic interrupts */
	Tintena	= 1<<2,		/* enable irq 29 at cnt==0 (30 for watchdog) */
	Wdog	= 1<<3,		/* watchdog, not timer, mode */
	Xsclrshift = 8,
	Xsclrmask = MASK(8),

	/* isr bits */
	Xisrclk	= 1<<0,		/* write to clear */

	/* wdrst bits */
	Wdrst	= 1<<0,

	/* wddis values */
	Wdon	= 1,
	Wdoff1	= 0x12345678,	/* send these two to switch to timer mode */
	Wdoff2	= 0x87654321,
};

/* cortex-a private-intr globl timer registers */
struct Pglbtmr {
	ulong	cnt[2];		/* counts up; little-endian uvlong */
	ulong	ctl;
	ulong	isr;
	ulong	cmp[2];		/* little-endian uvlong */
	ulong	inc;
};

enum {
	/* unique ctl bits (otherwise see X* above) */
	Gcmp	= 1<<1,
//	Gtintena= 1<<2,		/* enable irq 27 */
	Gincr	= 1<<3,
};

/*
 * until 5[cl] inline vlong ops, avoid them where possible,
 * they are currently slow function calls.
 */
typedef union Vlong Vlong;
union Vlong {
	uvlong	uvl;
	struct {			/* little-endian */
		ulong	low;
		ulong	high;
	};
};

static int fired;
static int ticking[MAXMACH];

/* no lock is needed to update our local timer.  splhi keeps it tight. */
static void
setltimer(Ltimer *tn, ulong ticks)
{
	int s;

	assert(ticks <= Clockfreqbase);
	s = splhi();
	tn->load = ticks - 1;
	coherence();
	tn->ctl = Tmrena | Tintena | Xreload;
	coherence();
	splx(s);
}

static void
ckstuck(int cpu, long myticks, long histicks)
{
	if (labs(histicks - myticks) > HZ) {
//		iprint("cpu%d: clock ticks %ld (vs myticks %ld cpu0 %ld); "
//			"apparently stopped\n",
//			cpu, histicks, myticks, MACHP(0)->ticks);
		if (!ticking[cpu])
			panic("cpu%d: clock not interrupting", cpu);
	}
}

static void
mpclocksanity(void)
{
	int cpu, mycpu;
	long myticks, histicks;

	if (conf.nmach <= 1 || active.exiting || navailcpus == 0)
		return;

	mycpu = m->machno;
	myticks = m->ticks;
	if (myticks == HZ)
		ticking[mycpu] = 1;

	if (myticks < 5*HZ)
		return;

	for (cpu = 0; cpu < navailcpus; cpu++) {
		if (cpu == mycpu)
			continue;
		histicks = MACHP(cpu)->ticks;
		if (myticks == 5*HZ || histicks > 1)
			ckstuck(cpu, myticks, histicks);
	}
}

static void
clockintr(Ureg* ureg, void *arg)
{
	Ltimer *wd, *tn;
	Ploctmr *lt;

	lt = (Ploctmr *)arg;
	tn = &lt->loc;
	tn->isr = Xisrclk;
	coherence();

	timerintr(ureg, 0);

#ifdef watchdog_not_bloody_useless
	/* appease the dogs */
	wd = &lt->wd;
	if (wd->cnt == 0 &&
	    (wd->ctl & (Wdog | Wdogena | Tintena)) == (Wdog | Wdogena))
		panic("cpu%d: zero watchdog count but no system reset",
			m->machno);
	wd->load = Dogtimeout - 1;
	coherence();
#endif
	SET(wd); USED(wd);
	tegclockintr();

	mpclocksanity();
}

void
clockprod(Ureg *ureg)
{
	Ltimer *tn;

	timerintr(ureg, 0);
	tegclockintr();
	if (m->machno != 0) {		/* cpu1 gets stuck */
		tn = &((Ploctmr *)soc.loctmr)->loc;
		setltimer(tn, Tcycles);
	}
}

static void
clockreset(Ltimer *tn)
{
	if (probeaddr((uintptr)tn) < 0)
		panic("no clock at %#p", tn);
	tn->ctl = 0;
	coherence();
}

void
watchdogoff(Ltimer *wd)
{
	wd->ctl &= ~Wdogena;
	coherence();
	wd->wddis = Wdoff1;
	coherence();
	wd->wddis = Wdoff2;
	coherence();
}

/* clear any pending watchdog intrs or causes */
void
wdogclrintr(Ltimer *wd)
{
#ifdef watchdog_not_bloody_useless
	wd->isr = Xisrclk;
	coherence();
	wd->wdrst = Wdrst;
	coherence();
#endif
	USED(wd);
}

/*
 * stop clock interrupts on this cpu and disable the local watchdog timer,
 * and, if on cpu0, shutdown the shared tegra2 watchdog timer.
 */
void
clockshutdown(void)
{
	Ploctmr *lt;

	lt = (Ploctmr *)soc.loctmr;
	clockreset(&lt->loc);
	watchdogoff(&lt->wd);

	tegclockshutdown();
}

enum {
	Instrs		= 10*Mhz,
};

/* we assume that perfticks are microseconds */
static long
issue1loop(void)
{
	register int i;
	long st;

	i = Instrs;
	st = perfticks();
	do {
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i; --i;
		--i; --i; --i; --i; --i; --i; --i; --i; --i;
	} while(--i >= 0);
	return perfticks() - st;
}

static long
issue2loop(void)
{
	register int i, j;
	long st;

	i = Instrs / 2;			/* j gets half the decrements */
	j = 0;
	st = perfticks();
	do {
		     --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;

		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
		--i; --j; --i; --j; --i; --j; --i; --j; --i; --j;
	} while(--i >= 0);
	return perfticks() - st;
}

/* estimate instructions/s. */
static void
guessmips(long (*loop)(void), char *lab)
{
	int s;
	long tcks;

	do {
		s = splhi();
		tcks = loop();
		splx(s);
		if (tcks < 0)
			iprint("again...");
	} while (tcks < 0);
	/*
	 * Instrs instructions took tcks ticks @ Basetickfreq Hz.
	 * round the result.
	 */
	s = (((vlong)Basetickfreq * Instrs) / tcks + 500000) / 1000000;
	if (Debug)
		iprint("%ud mips (%s-issue)", s, lab);
	USED(s);
}

void
wdogintr(Ureg *, void *ltmr)
{
#ifdef watchdog_not_bloody_useless
	Ltimer *wd;

	wd = ltmr;
	fired++;
	wdogclrintr(wd);
#endif
	USED(ltmr);
}

static void
ckcounting(Ltimer *lt)
{
	ulong old;

	old = lt->cnt;
	if (old == lt->cnt)
		delay(1);
	if (old == lt->cnt)
		panic("cpu%d: watchdog timer not counting down", m->machno);
}

/* test fire with interrupt to see that it's working */
static void
ckwatchdog(Ltimer *wd)
{
#ifdef watchdog_not_bloody_useless
	int s;

	fired = 0;
	wd->load = Tcycles - 1;
	coherence();
	/* Tintena is supposed to be ignored in watchdog mode */
	wd->ctl |= Wdogena | Tintena;
	coherence();

	ckcounting(wd);

	s = spllo();
	delay(2 * 1000/HZ);
	splx(s);
	if (!fired)
		/* useless local watchdog */
		iprint("cpu%d: local watchdog failed to interrupt\n", m->machno);
	/* clean up */
	wd->ctl &= ~Wdogena;
	coherence();
#endif
	USED(wd);
}

static void
startwatchdog(void)
{
#ifdef watchdog_not_bloody_useless
	Ltimer *wd;
	Ploctmr *lt;

	lt = (Ploctmr *)soc.loctmr;
	wd = &lt->wd;
	watchdogoff(wd);
	wdogclrintr(wd);
	irqenable(Wdtmrirq, wdogintr, wd, "watchdog");

	ckwatchdog(wd);

	/* set up for normal use, causing reset */
	wd->ctl &= ~Tintena;			/* reset, don't interrupt */
	coherence();
	wd->ctl |= Wdog;
	coherence();
	wd->load = Dogtimeout - 1;
	coherence();
	wd->ctl |= Wdogena;
	coherence();

	ckcounting(wd);
#endif
}

static void
clock0init(Ltimer *tn)
{
	int s;
	ulong old, fticks;

	/*
	 * calibrate fastclock
	 */
	s = splhi();
	tn->load = ~0ul >> 1;
	coherence();
	tn->ctl = Tmrena;
	coherence();

	old = perfticks();
	fticks = tn->cnt;
	delay(1);
	fticks = abs(tn->cnt - fticks);
	old = perfticks() - old;
	splx(s);
	if (Debug)
		iprint("cpu%d: fastclock %ld/%ldµs = %ld fastticks/µs (MHz)\n",
			m->machno, fticks, old, (fticks + old/2 - 1) / old);
	USED(fticks, old);

	if (Debug)
		iprint("cpu%d: ", m->machno);
	guessmips(issue1loop, "single");
	if (Debug)
		iprint(", ");
	guessmips(issue2loop, "dual");
	if (Debug)
		iprint("\n");

	/*
	 * m->delayloop should be the number of delay loop iterations
	 * needed to consume 1 ms.  2 is instr'ns in the delay loop.
	 */
	m->delayloop = m->cpuhz / (1000 * 2);
//	iprint("cpu%d: m->delayloop = %lud\n", m->machno, m->delayloop);

	tegclock0init();
}

/*
 * the local timer is the interrupting timer and does not
 * participate in measuring time.  It is initially set to HZ.
 */
void
clockinit(void)
{
	ulong old;
	Ltimer *tn;
	Ploctmr *lt;

	clockshutdown();

	/* turn my cycle counter on */
	cpwrsc(0, CpCLD, CpCLDena, CpCLDenacyc, 1<<31);

	/* turn all my counters on and clear my cycle counter */
	cpwrsc(0, CpCLD, CpCLDena, CpCLDenapmnc, 1<<2 | 1);

	/* let users read my cycle counter directly */
	cpwrsc(0, CpCLD, CpCLDuser, CpCLDenapmnc, 1);

	/* verify µs counter sanity */
	tegclockinit();

	lt = (Ploctmr *)soc.loctmr;
	tn = &lt->loc;
	if (m->machno == 0)
		irqenable(Loctmrirq, clockintr, lt, "clock");
	else
		intcunmask(Loctmrirq);

	/*
	 * verify sanity of local timer
	 */
	tn->load = Clockfreqbase / 1000;
	tn->isr = Xisrclk;
	coherence();
	tn->ctl = Tmrena;
	coherence();

	old = tn->cnt;
	delay(5);
	/* m->ticks won't be incremented here because timersinit hasn't run. */
	if (tn->cnt == old)
		panic("cpu%d: clock not ticking at all", m->machno);
	else if ((long)tn->cnt > 0)
		panic("cpu%d: clock ticking slowly", m->machno);

	if (m->machno == 0)
		clock0init(tn);

	/* if pci gets stuck, maybe one of the many watchdogs will nuke us. */
	startwatchdog();

	/*
	 *  desynchronize the processor clocks so that they all don't
	 *  try to resched at the same time.
	 */
	delay(m->machno*2);
	setltimer(tn, Tcycles);
}

/* our fastticks are at 1MHz (Basetickfreq), so the conversion is trivial. */
ulong
µs(void)
{
	return fastticks2us(fastticks(nil));
}

/* Tval is supposed to be in fastticks units. */
void
timerset(Tval next)
{
	int s;
	long offset;
	Ltimer *tn;

	tn = &((Ploctmr *)soc.loctmr)->loc;
	s = splhi();
	offset = fastticks2us(next - fastticks(nil));
	/* offset is now in µs (MHz); convert to Clockfreqbase Hz. */
	offset *= Clockfreqbase / Mhz;
	if(offset < MinPeriod)
		offset = MinPeriod;
	else if(offset > MaxPeriod)
		offset = MaxPeriod;

	setltimer(tn, offset);
	splx(s);
}

static ulong
cpucycles(void)	/* cpu clock rate, except when waiting for intr (unused) */
{
	ulong v;

	/* reads 32-bit cycle counter (counting up) */
//	v = cprdsc(0, CpCLD, CpCLDcyc, 0);
	v = getcyc();				/* fast asm */
	/* keep it non-negative; prevent m->fastclock ever going to 0 */
	return v == 0? 1: v;
}

long
lcycles(void)
{
	return perfticks();
}

uvlong
fastticks(uvlong *hz)
{
	int s;
	ulong newticks;
	Vlong *fcp;

	if(hz)
		*hz = Basetickfreq;

	fcp = (Vlong *)&m->fastclock;
	/* avoid reentry on interrupt or trap, to prevent recursion */
	s = splhi();
	newticks = perfticks();
	if(newticks < fcp->low)		/* low word must have wrapped */
		fcp->high++;
	fcp->low = newticks;
	splx(s);

	if (fcp->low == 0 && fcp->high == 0 && m->ticks > HZ/10)
		panic("fastticks: zero m->fastclock; ticks %lud fastclock %#llux",
			m->ticks, m->fastclock);
	return m->fastclock;
}

void
microdelay(int l)
{
	for (l = l * (vlong)m->delayloop / 1000; --l >= 0; )
		;
}

void
delay(int l)
{
	int i, d;

	d = m->delayloop;
	while(--l >= 0)
		for (i = d; --i >= 0; )
			;
}
