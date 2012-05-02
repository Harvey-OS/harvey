/*
 * tegra 2 SoC clocks; excludes cortex-a timers.
 *
 * SoC provides these shared clocks:
 * 4 29-bit count-down `timers' @ 1MHz,
 * 1 32-bit count-up time-stamp counter @ 1MHz,
 * and a real-time clock @ 32KHz.
 * the tegra watchdog (tegra 2 ref man §5.4.1) is tied to timers, not rtc.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

typedef struct Shrdtmr Shrdtmr;
typedef struct µscnt µscnt;

/* tegra2 shared-intr timer registers */
struct Shrdtmr {		/* 29-bit count-down timer (4); unused */
	ulong	trigger;
	ulong	prescnt;
};

enum {
	/* trigger bits */
	Enable =	1u<<31,
	Periodintr =	1<<30,
	Countmask =	MASK(29),

	/* prescnt bits */
	Intrclr =	1<<30,
	/* Countmask is ro */
};

struct µscnt {		/* tegra2 shared 32-bit count-up µs counter (1) */
	ulong	cntr;
	/*
	 * oscillator clock fraction - 1; initially 0xb (11) from u-boot
	 * for 12MHz periphclk.
	 */
	ulong	cfg;
	uchar	_pad0[0x3c - 0x8];
	ulong	freeze;
};

enum {
	/* cfg bits */
	Dividendshift =	8,
	Dividendmask =	MASK(8),
	Divisorshift =	0,
	Divisormask =	MASK(8),
};

void
tegclockintr(void)
{
	int junk;
	Shrdtmr *tmr;

	/* appease the tegra dog */
	tmr = (Shrdtmr *)soc.tmr[0];
	junk = tmr->trigger;
	USED(junk);
}

/*
 * if on cpu0, shutdown the shared tegra2 watchdog timer.
 */
void
tegclockshutdown(void)
{
	Shrdtmr *tmr;

	if (m->machno == 0) {
		tmr = (Shrdtmr *)soc.tmr[0];
		tmr->prescnt = tmr->trigger = 0;
		coherence();
	}
}

void
tegwdogintr(Ureg *, void *v)
{
	int junk;
	Shrdtmr *tmr;

	tmr = (Shrdtmr *)v;
	tmr->prescnt |= Intrclr;
	coherence();
	/* the lousy documentation says we also have to read trigger */
	junk = tmr->trigger;
	USED(junk);
}

/* start tegra2 shared watch dog */
void
tegclock0init(void)
{
	Shrdtmr *tmr;

	tmr = (Shrdtmr *)soc.tmr[0];
	irqenable(Tn0irq, tegwdogintr, tmr, "tegra watchdog");

	/*
	 * tegra watchdog only fires on the second missed interrupt, thus /2.
	 */
	tmr->trigger = (Dogsectimeout * Mhz / 2 - 1) | Periodintr | Enable;
	coherence();
}

/*
 * µscnt is a freerunning timer (cycle counter); it needs no
 * initialisation, wraps and does not dispatch interrupts.
 */
void
tegclockinit(void)
{
	ulong old;
	µscnt *µs = (µscnt *)soc.µs;

	/* verify µs counter sanity */
	assert(µs->cfg == 0xb);			/* set by u-boot */
	old = µs->cntr;
	delay(1);
	assert(old != µs->cntr);
}

ulong
perfticks(void)			/* MHz rate, assumed by timing loops */
{
	ulong v;

	/* keep it non-zero to prevent m->fastclock ever going to zero. */
	v = ((µscnt *)soc.µs)->cntr;
	return v == 0? 1: v;
}
