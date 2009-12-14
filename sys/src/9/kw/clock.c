/*
 * kirkwood clock
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ureg.h"

enum {
	Tcycles = CLOCKFREQ / HZ,		/* cycles per clock tick */
};

static void
clockintr(Ureg *ureg, void*)
{
	TIMERREG->timerwd = CLOCKFREQ;		/* reassure the watchdog */
	m->fastclock++;
	coherence();
	timerintr(ureg, 0);
	intrclear(Irqbridge, IRQcputimer0);
}

void
clockinit(void)
{
	int s;
	long cyc;
	TimerReg *tmr = TIMERREG;

	tmr->ctl = 0;
	coherence();
	intrenable(Irqbridge, IRQcputimer0, clockintr, nil, "clock");

	s = spllo();			/* risky */
	/* take any deferred clock (& other) interrupts here */
	splx(s);

	/* adjust m->bootdelay, used by delay()? */
	m->ticks = 0;
	m->fastclock = 0;

	tmr->timer0 = Tcycles;
	tmr->ctl = Tmr0enable;		/* just once */
	coherence();

	s = spllo();			/* risky */
	/* one iteration seems to take about 40 ns. */
	for (cyc = Tcycles; cyc > 0 && m->fastclock == 0; cyc--)
		;
	splx(s);

	if (m->fastclock == 0) {
		serialputc('?');
		if (tmr->timer0 == 0)
			panic("clock not interrupting");
		else if (tmr->timer0 == tmr->reload0)
			panic("clock not ticking");
		else
			panic("clock running very slowly");
	}

	tmr->ctl = 0;
	coherence();
	tmr->timer0  = Tcycles;
	tmr->reload0 = Tcycles;
	tmr->timerwd = CLOCKFREQ;
	coherence();
	tmr->ctl = Tmr0enable | Tmr0periodic | TmrWDenable;
	CPUCSREG->rstout |= RstoutWatchdog;
	coherence();
}

void
timerset(uvlong next)
{
#ifdef FANCYTIMERS
	Tn *tn;
	Tval offset;

	ilock(&timers.tn1lock);
	tn = (Tn*)Tn1;
	tn->cr = Tm;

	offset = next + tn->cv;
	if(offset < timers.tn1minperiod)
		offset = timers.tn1minperiod;
	else if(offset > timers.tn1maxperiod)
		offset = timers.tn1maxperiod;

	tn->lc = offset;
	tn->cr = Tm|Te;
	iunlock(&timers.tn1lock);
#else
	USED(next);
#endif
}

/*
 * shift by 8 to provide enough resolution that dropping the tick rate
 * won't mess up TOD calculation and cause infrequent clock interrupts.
 */
uvlong
fastticks(uvlong *hz)
{
	if(hz)
		*hz = HZ << 8;
	return m->fastclock << 8;
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

ulong
perfticks(void)
{
//	return ((Tn*)Tn0)->cv;		// TODO: FANCYTIMERS
	return (ulong)fastticks(nil);
}
