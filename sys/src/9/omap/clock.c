/*
 * omap3530 clocks
 *
 * timers count up to zero.
 *
 * the source clock signals for the timers are sometimes selectable.  for
 * WDTIMER[23] and GPTIMER12, it's always the 32kHz clock.  for the
 * others, it can be the 32kHz clock or the system clock.  we use only
 * WDTIMER2 and GPTIMER[12], and configure GPTIMER[12] in archomap.c to
 * use the 32kHZ clock.  WDTIMER1 is not accessible to us on GP
 * (general-purpose) omaps.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

enum {
	Debug		= 0,

	Tn0		= PHYSTIMER1,
	Tn1		= PHYSTIMER2,

	/* irq 36 is watchdog timer module 3 overflow */
	Tn0irq		= 37,			/* base IRQ for all timers */

	Freebase	= 1,			/* base of free-running timer */

	/*
	 * clock is 32K (32,768) Hz, so one tick is 30.517µs,
	 * so 327.68 ticks is 10ms, 32.768 ticks is 1ms.
	 */
	Clockfreqbase	= 32 * 1024,		/* base rate in Hz */
	Tcycles		= Clockfreqbase / HZ,	/* cycles per clock tick */

	MinPeriod	= (Tcycles / 100 < 2? 2: Tcycles / 100),
	MaxPeriod	= Tcycles,

	Dogtimeout	= 20 * Clockfreqbase,	/* was 4 s.; must be ≤ 21 s. */
};

enum {
	/* ticpcfg bits */
	Noidle		= 1<<3,
	Softreset	= 1<<1,

	/* tistat bits */
	Resetdone	= 1<<0,

	/* tisr/tier bits */
	Ovf_it		= 1<<1,		/* gp: overflow intr */
	Mat_it		= 1<<0,		/* gp: match intr */
	Wdovf_it	= 1<<0,		/* wdog: overflow intr */

	/* tclr bits */
	Ar		= 1<<1,		/* gp only: autoreload mode overflow */
	St		= 1<<0,		/* gp only: start the timer */
};

/* omap35x timer registers */
typedef struct Timerregs Timerregs;
struct Timerregs {
	/* common to all timers, gp and watchdog */
	uchar	pad0[0x10];
	ulong	ticpcfg;
	ulong	tistat;		/* ro: low bit: reset done */
	ulong	tisr;
	ulong	tier;
	ulong	twer;
	ulong	tclr;
	ulong	tcrr;		/* counter: cycles to zero */
	ulong	tldr;
	ulong	ttgr;		/* trigger */
	ulong	twps;		/* ro: write posted pending */

	/* gp timers only, unused by us */
	ulong	tmar;		/* value to compare with counter */
	ulong	tcar1;		/* ro */
	ulong	tsicr;
	ulong	tcar2;		/* ro */
	union {
		ulong	tpir;	/* gp: 1 ms tick generation: +ve */
		ulong	wspr;	/* wdog: start/stop control */
	};
	ulong	tnir;		/* 1 ms tick generation: -ve */
	ulong	tcvr;		/* 1 ms tick generation: next counter value */
	ulong	tocr;		/* intr mask for n ticks */
	ulong	towr;
};

static int ticks; /* for sanity checking; m->ticks doesn't always get called */
static Lock clklck;

static ulong	rdcycles(void), rdbaseticks(void);

/* write a watchdog timer's start/stop register */
static void
wdogwrss(Timerregs *tn, ulong val)
{
	while (tn->twps & (1 << 4))	/* pending write to start/stop reg? */
		;
	tn->wspr = val;
	coherence();
	while (tn->twps & (1 << 4))	/* pending write to start/stop reg? */
		;
}

static void
resetwait(Timerregs *tn)
{
	long bound;

	for (bound = 400*Mhz; !(tn->tistat & Resetdone) && bound > 0; bound--)
		;
	if (bound <= 0)
		iprint("clock reset didn't complete\n");
}


static void
wdogoff(Timerregs *tn)
{
	resetwait(tn);

	wdogwrss(tn, 0xaaaa);		/* magic off sequence */
	wdogwrss(tn, 0x5555);

	tn->tldr = 1;
	coherence();
	tn->tcrr = 1;			/* paranoia */
	coherence();
}

static void wdogassure(void);

static void
wdogon(Timerregs *tn)
{
	static int beenhere;

	resetwait(tn);
	tn->tldr = -Dogtimeout;
	tn->tcrr = -Dogtimeout;
	coherence();
	wdogwrss(tn, 0xbbbb);		/* magic on sequence */
	wdogwrss(tn, 0x4444);		/* magic on sequence */

	if (!beenhere) {
		beenhere = 1;
		/* touching the dog is not quick, so do it infrequently */
		addclock0link(wdogassure, HZ);
	}
}

static void
wdogassure(void)		/* reset the watch dog's counter */
{
	Timerregs *tn;

	tn = (Timerregs *)PHYSWDOG;
	wdogoff(tn);

	tn->tcrr = -Dogtimeout;
	coherence();

	wdogon(tn);
}

static void
clockintr(Ureg* ureg, void *arg)
{
	Timerregs *tn;
	static int nesting;

	ticks++;
	coherence();

	if (nesting == 0) {	/* if the clock interrupted itself, bail out */
		++nesting;
		timerintr(ureg, 0);
		--nesting;
	}

	tn = arg;
	tn->tisr = Ovf_it;			/* dismiss the interrupt */
	coherence();
}

static void
clockreset(Timerregs *tn)
{
	if (probeaddr((uintptr)&tn->ticpcfg) < 0)
		panic("no clock at %#p", tn);
	tn->ticpcfg = Softreset | Noidle;
	coherence();
	resetwait(tn);
	tn->tier = tn->tclr = 0;
	coherence();
}

/* stop clock interrupts and disable the watchdog timer */
void
clockshutdown(void)
{
	clockreset((Timerregs *)PHYSWDT2);
	wdogoff((Timerregs *)PHYSWDT2);
	clockreset((Timerregs *)PHYSWDT3);
	wdogoff((Timerregs *)PHYSWDT3);

	clockreset((Timerregs *)Tn0);
	clockreset((Timerregs *)Tn1);
}

enum {
	Instrs		= 10*Mhz,
};

static long
issue1loop(void)
{
	register int i;
	long st;

	st = rdbaseticks();
	i = Instrs;
	do {
		--i; --i; --i; --i; --i;
		--i; --i; --i; --i;
	} while(--i >= 0);
	return rdbaseticks() - st;
}

static long
issue2loop(void)
{
	register int i, j;
	long st;

	st = rdbaseticks();
	i = Instrs / 2;
	j = 0;
	do {
		--i; --j; --i; --j;
		--i; --j; --i; --j;
		--j;
	} while(--i >= 0);
	return rdbaseticks() - st;
}

/* estimate instructions/s. using 32kHz clock */
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
	 * Instrs instructions took tcks ticks @ Clockfreqbase Hz.
	 */
	s = ((vlong)Clockfreqbase * Instrs) / tcks / 1000000;
	if (Debug)
		iprint("%ud mips (%s-issue)", s, lab);
	USED(s);
}

void
clockinit(void)
{
	int i, s;
	Timerregs *tn;

	clockshutdown();

	/* turn cycle counter on */
	cpwrsc(0, CpCLD, CpCLDena, CpCLDenacyc, 1<<31);

	/* turn all counters on and clear the cycle counter */
	cpwrsc(0, CpCLD, CpCLDena, CpCLDenapmnc, 1<<2 | 1);

	/* let users read the cycle counter directly */
	cpwrsc(0, CpCLD, CpCLDena, CpCLDenapmnc, 1);

	ilock(&clklck);
	m->fastclock = 1;
	m->ticks = ticks = 0;

	/*
	 * T0 is a freerunning timer (cycle counter); it wraps,
	 * automatically reloads, and does not dispatch interrupts.
	 */
	tn = (Timerregs *)Tn0;
	tn->tcrr = Freebase;			/* count up to 0 */
	tn->tldr = Freebase;
	coherence();
	tn->tclr = Ar | St;
	iunlock(&clklck);

	/*
	 * T1 is the interrupting timer and does not participate
	 * in measuring time.  It is initially set to HZ.
	 */
	tn = (Timerregs *)Tn1;
	irqenable(Tn0irq+1, clockintr, tn, "clock");
	ilock(&clklck);
	tn->tcrr = -Tcycles;			/* approx.; count up to 0 */
	tn->tldr = -Tcycles;
	coherence();
	tn->tclr = Ar | St;
	coherence();
	tn->tier = Ovf_it;
	coherence();
	iunlock(&clklck);

	/*
	 * verify sanity of timer1
	 */
	s = spllo();			/* risky */
	for (i = 0; i < 5 && ticks == 0; i++) {
		delay(10);
		cachedwbinvse(&ticks, sizeof ticks);
	}
	splx(s);
	if (ticks == 0) {
		if (tn->tcrr == 0)
			panic("clock not interrupting");
		else if (tn->tcrr == tn->tldr)
			panic("clock not ticking at all");
#ifdef PARANOID
		else
			panic("clock running very slowly");
#endif
	}

	guessmips(issue1loop, "single");
	if (Debug)
		iprint(", ");
	guessmips(issue2loop, "dual");
	if (Debug)
		iprint("\n");

	/*
	 * m->delayloop should be the number of delay loop iterations
	 * needed to consume 1 ms.  2 is min. instructions in the delay loop.
	 */
	m->delayloop = m->cpuhz / (1000 * 2);
//	iprint("m->delayloop = %lud\n", m->delayloop);

	/*
	 *  desynchronize the processor clocks so that they all don't
	 *  try to resched at the same time.
	 */
	delay(m->machno*2);
}

void
watchdoginit(void)
{
	wdogassure();
}

ulong
µs(void)
{
	return fastticks2us(fastticks(nil));
}

void
timerset(Tval next)
{
	long offset;
	Timerregs *tn = (Timerregs *)Tn1;
	static Lock setlck;

	ilock(&setlck);
	offset = next - fastticks(nil);
	if(offset < MinPeriod)
		offset = MinPeriod;
	else if(offset > MaxPeriod)
		offset = MaxPeriod;
	tn->tcrr = -offset;
	coherence();
	iunlock(&setlck);
}

static ulong
rdcycles(void)
{
	ulong v;

	/* reads 32-bit cycle counter (counting up) */
	v = cprdsc(0, CpCLD, CpCLDcyc, 0);
	/* keep it positive; prevent m->fastclock ever going to 0 */
	return v == 0? 1: v;
}

static ulong
rdbaseticks(void)
{
	ulong v;

	v = ((Timerregs *)Tn0)->tcrr;		/* tcrr should be counting up */
	/* keep it positive; prevent m->fastclock ever going to 0 */
	return v == 0? 1: v;
}

ulong
perfticks(void)
{
	return rdcycles();
}

long
lcycles(void)
{
	return perfticks();
}

/*
 * until 5[cal] inline vlong ops, avoid them where possible,
 * they are currently slow function calls.
 */
typedef union Counter Counter;
union Counter {
	uvlong	uvl;
	struct {			/* little-endian */
		ulong	low;
		ulong	high;
	};
};

enum {
	Fastvlongops	= 0,
};

uvlong
fastticks(uvlong *hz)
{
	Counter now, sclnow;

	if(hz)
		*hz = m->cpuhz;
	ilock(&clklck);
	if (m->ticks > HZ/10 && m->fastclock == 0)
		panic("fastticks: zero m->fastclock; ticks %lud fastclock %#llux",
			m->ticks, m->fastclock);

	now.uvl = m->fastclock;
	now.low = rdcycles();
	if(now.uvl < m->fastclock)	/* low bits must have wrapped */
		now.high++;
	m->fastclock = now.uvl;
	coherence();

	sclnow.uvl = now.uvl;
	iunlock(&clklck);
	return sclnow.uvl;
}

void
microdelay(int l)
{
	int i;

	l = l * (vlong)m->delayloop / 1000;
	if(l <= 0)
		l = 1;
	for(i = 0; i < l; i++)
		;
}

void
delay(int l)
{
	ulong i, j;

	j = m->delayloop;
	while(l-- > 0)
		for(i=0; i < j; i++)
			;
}
