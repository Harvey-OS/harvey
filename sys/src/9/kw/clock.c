/*
 * kirkwood clocks
 *
 * timers count down to zero.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ureg.h"

enum {
	Tcycles		= CLOCKFREQ / HZ,	/* cycles per clock tick */
	Dogperiod	= 15 * CLOCKFREQ, /* at most 21 s.; must fit in ulong */
	MaxPeriod	= Tcycles,
	MinPeriod	= MaxPeriod / 100,

	/* timer ctl bits */
	Tmr0enable	= 1<<0,
	Tmr0reload	= 1<<1,	/* at 0 count, load timer0 from reload0 */
	Tmr1enable	= 1<<2,
	Tmr1reload	= 1<<3,	/* at 0 count, load timer1 from reload1 */
	TmrWDenable	= 1<<4,
	TmrWDreload	= 1<<5,
};

typedef struct TimerReg TimerReg;
struct TimerReg
{
	ulong	ctl;
	ulong	pad[3];
	ulong	reload0;
	ulong	timer0;			/* cycles until zero */
	ulong	reload1;
	ulong	timer1;			/* cycles until zero */
	ulong	reloadwd;
	ulong	timerwd;
};

static int ticks; /* for sanity checking; m->ticks doesn't always get updated */

static void
clockintr(Ureg *ureg, void *arg)
{
	TimerReg *tmr = arg;
	static int nesting;

	tmr->timerwd = Dogperiod;		/* reassure the watchdog */
	ticks++;
	coherence();

	if (nesting == 0) {	/* if the clock interrupted itself, bail out */
		++nesting;
		timerintr(ureg, 0);
		--nesting;
	}

	intrclear(Irqbridge, IRQcputimer0);
}

/* stop clock interrupts and disable the watchdog timer */
void
clockshutdown(void)
{
	TimerReg *tmr = (TimerReg *)soc.clock;

	tmr->ctl = 0;
	coherence();
}

void
clockinit(void)
{
	int i, s;
	CpucsReg *cpu = (CpucsReg *)soc.cpu;
	TimerReg *tmr = (TimerReg *)soc.clock;

	clockshutdown();

	/*
	 * verify sanity of timer0
	 */

	intrenable(Irqbridge, IRQcputimer0, clockintr, tmr, "clock0");
	s = spllo();			/* risky */
	/* take any deferred clock (& other) interrupts here */
	splx(s);

	/* adjust m->bootdelay, used by delay()? */
	m->ticks = ticks = 0;
	m->fastclock = 0;

	tmr->timer0 = 1;
	tmr->ctl = Tmr0enable;		/* just once */
	coherence();

	s = spllo();			/* risky */
	for (i = 0; i < 10 && ticks == 0; i++) {
		delay(1);
		coherence();
	}
	splx(s);
	if (ticks == 0) {
		serialputc('?');
		if (tmr->timer0 == 0)
			panic("clock not interrupting");
		else if (tmr->timer0 == tmr->reload0)
			panic("clock not ticking");
		else
			panic("clock running very slowly");
	}

	/*
	 * configure all timers
	 */
	clockshutdown();
	tmr->reload0 = tmr->timer0 = Tcycles;	/* tick clock */
	tmr->reload1 = tmr->timer1 = ~0;	/* cycle clock */
	tmr->timerwd = Dogperiod;		/* watch dog timer */
	coherence();
	tmr->ctl = Tmr0enable | Tmr0reload | Tmr1enable | Tmr1reload |
		TmrWDenable;
	cpu->rstout |= RstoutWatchdog;
	coherence();
}

void
timerset(Tval next)
{
	int offset;
	TimerReg *tmr = (TimerReg *)soc.clock;

	offset = next - fastticks(nil);
	if(offset < MinPeriod)
		offset = MinPeriod;
	else if(offset > MaxPeriod)
		offset = MaxPeriod;
	tmr->timer0 = offset;
	coherence();
}

uvlong
fastticks(uvlong *hz)
{
	uvlong now;
	int s;

	if(hz)
		*hz = CLOCKFREQ;
	s = splhi();
	/* zero low ulong of fastclock */
	now = (m->fastclock & ~(uvlong)~0ul) | perfticks();
	if(now < m->fastclock)		/* low bits must have wrapped */
		now += 1ll << 32;
	m->fastclock = now;
	splx(s);
	return now;
}

ulong
perfticks(void)
{
	TimerReg *tmr = (TimerReg *)soc.clock;

	return ~tmr->timer1;
}

long
lcycles(void)
{
	return perfticks();
}

ulong
µs(void)
{
	return fastticks2us(fastticks(nil));
}

void
microdelay(int l)
{
	int i;

	l *= m->delayloop;
	l /= 1000;
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
