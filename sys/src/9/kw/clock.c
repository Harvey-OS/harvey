/*
 * kirkwood clocks
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ureg.h"

#define TIMERREG	((TimerReg *)AddrTimer)

enum {
	Tcycles = CLOCKFREQ / HZ,		/* cycles per clock tick */

	/* timer ctl bits */
	Tmr0enable	= 1<<0,
	Tmr0periodic	= 1<<1,
	Tmr1enable	= 1<<2,
	Tmr1periodic	= 1<<3,
	TmrWDenable	= 1<<4,
	TmrWDperiodic	= 1<<5,

	MaxPeriod	= Tcycles,
	MinPeriod	= MaxPeriod / 100,
};

typedef struct TimerReg TimerReg;
struct TimerReg
{
	ulong	ctl;
	ulong	pad[3];
	ulong	reload0;
	ulong	timer0;
	ulong	reload1;
	ulong	timer1;
	ulong	reloadwd;
	ulong	timerwd;
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

/* stop clock interrupts and disable the watchdog timer */
void
clockshutdown(void)
{
	TIMERREG->ctl = 0;
	coherence();
}

void
clockinit(void)
{
	int s;
	long cyc;
	TimerReg *tmr = TIMERREG;

	clockshutdown();
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

	clockshutdown();
	tmr->timer0  = Tcycles;
	tmr->timer1  = ~0;
	tmr->reload1 = ~0;
	tmr->timerwd = CLOCKFREQ;
	coherence();
	tmr->ctl = Tmr0enable | Tmr1enable | Tmr1periodic | TmrWDenable;
	CPUCSREG->rstout |= RstoutWatchdog;
	coherence();
}

void
timerset(Tval next)
{
	int offset;
	TimerReg *tmr = TIMERREG;

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
	now = (m->fastclock & ~(uvlong)~0ul) | ~TIMERREG->timer1;
	if(now < m->fastclock)
		now += 1ll << 32;
	m->fastclock = now;
	splx(s);
	return now;
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
	return ~TIMERREG->timer1;
}
