/*
 * bcm283[56] timers
 *	System timers run at 1MHz (timers 1 and 2 are used by GPU)
 *	ARM timer usually runs at 250MHz (may be slower in low power modes)
 *	Cycle counter runs at 700MHz (unless overclocked)
 *    All are free-running up-counters
 *  Cortex-a7 has local generic timers per cpu (which we run at 1MHz)
 *
 * Use system timer 3 (64 bits) for hzclock interrupts and fastticks
 *   For smp on bcm2836, use local generic timer for interrupts on cpu1-3
 * Use ARM timer (32 bits) for perfticks
 * Use ARM timer to force immediate interrupt
 * Use cycle counter for cycles()
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "arm.h"

enum {
	SYSTIMERS	= VIRTIO+0x3000,
	ARMTIMER	= VIRTIO+0xB400,

	Localctl	= 0x00,
	Prescaler	= 0x08,
	Localintpending	= 0x60,

	SystimerFreq	= 1*Mhz,
	MaxPeriod	= SystimerFreq / HZ,
	MinPeriod	= 10,

};

typedef struct Systimers Systimers;
typedef struct Armtimer Armtimer;

struct Systimers {
	u32int	cs;
	u32int	clo;
	u32int	chi;
	u32int	c0;
	u32int	c1;
	u32int	c2;
	u32int	c3;
};

struct Armtimer {
	u32int	load;
	u32int	val;
	u32int	ctl;
	u32int	irqack;
	u32int	irq;
	u32int	maskedirq;
	u32int	reload;
	u32int	predivider;
	u32int	count;
};

enum {
	CntPrescaleShift= 16,	/* freq is sys_clk/(prescale+1) */
	CntPrescaleMask	= 0xFF,
	CntEnable	= 1<<9,
	TmrDbgHalt	= 1<<8,
	TmrEnable	= 1<<7,
	TmrIntEnable	= 1<<5,
	TmrPrescale1	= 0x00<<2,
	TmrPrescale16	= 0x01<<2,
	TmrPrescale256	= 0x02<<2,
	CntWidth16	= 0<<1,
	CntWidth32	= 1<<1,

	/* generic timer (cortex-a7) */
	Enable	= 1<<0,
	Imask	= 1<<1,
	Istatus = 1<<2,
};

static void
clockintr(Ureg *ureg, void *)
{
	Systimers *tn;

	if(m->machno != 0)
		panic("cpu%d: unexpected system timer interrupt", m->machno);
	tn = (Systimers*)SYSTIMERS;
	/* dismiss interrupt */
	tn->c3 = tn->clo - 1;
	tn->cs = 1<<3;
	timerintr(ureg, 0);
}

static void
localclockintr(Ureg *ureg, void *)
{
	if(m->machno == 0)
		panic("cpu0: Unexpected local generic timer interrupt");
	cpwrtimerphysctl(Imask|Enable);
	timerintr(ureg, 0);
}

void
clockshutdown(void)
{
	Armtimer *tm;

	tm = (Armtimer*)ARMTIMER;
	tm->ctl = 0;
	if(cpuserver)
		wdogfeed();
	else
		wdogoff();
}

void
clockinit(void)
{
	Systimers *tn;
	Armtimer *tm;
	u32int t0, t1, tstart, tend;

	if(((cprdfeat1() >> 16) & 0xF) != 0) {
		/* generic timer supported */
		if(m->machno == 0){
			/* input clock is 19.2MHz or 54MHz crystal */
			*(ulong*)(ARMLOCAL + Localctl) = 0;
			/* divide by (2^31/Prescaler) for 1Mhz */
			*(ulong*)(ARMLOCAL + Prescaler) = (((uvlong)SystimerFreq<<31)/soc.oscfreq)&~1UL;
		}
		cpwrtimerphysctl(Imask);
	}

	tn = (Systimers*)SYSTIMERS;
	tstart = tn->clo;
	do{
		t0 = lcycles();
	}while(tn->clo == tstart);
	tend = tstart + 10000;
	do{
		t1 = lcycles();
	}while(tn->clo != tend);
	t1 -= t0;
	m->cpuhz = 100 * t1;
	m->cpumhz = (m->cpuhz + Mhz/2 - 1) / Mhz;
	m->cyclefreq = m->cpuhz;
	if(m->machno == 0){
		tn->c3 = tn->clo - 1;
		tm = (Armtimer*)ARMTIMER;
		tm->load = 0;
		tm->ctl = TmrPrescale1|CntEnable|CntWidth32;
		intrenable(IRQtimer3, clockintr, nil, 0, "clock");
	}else
		intrenable(IRQcntpns, localclockintr, nil, 0, "clock");
}

void
timerset(uvlong next)
{
	Systimers *tn;
	uvlong now;
	long period;

	now = fastticks(nil);
	period = next - now;
	if(period < MinPeriod)
		period = MinPeriod;
	else if(period > MaxPeriod)
		period = MaxPeriod;
	if(m->machno > 0){
		cpwrtimerphysval(period);
		cpwrtimerphysctl(Enable);
	}else{
		tn = (Systimers*)SYSTIMERS;
		tn->c3 = tn->clo + period;
	}
}

uvlong
fastticks(uvlong *hz)
{
	Systimers *tn;
	ulong lo, hi;
	uvlong now;

	if(hz)
		*hz = SystimerFreq;
	tn = (Systimers*)SYSTIMERS;
	do{
		hi = tn->chi;
		lo = tn->clo;
	}while(tn->chi != hi);
	now = (uvlong)hi<<32 | lo;
	return now;
}

ulong
perfticks(void)
{
	Armtimer *tm;

	tm = (Armtimer*)ARMTIMER;
	return tm->count;
}

void
armtimerset(int n)
{
	Armtimer *tm;

	tm = (Armtimer*)ARMTIMER;
	if(n > 0){
		tm->ctl |= TmrEnable|TmrIntEnable;
		tm->load = n;
	}else{
		tm->load = 0;
		tm->ctl &= ~(TmrEnable|TmrIntEnable);
		tm->irq = 1;
	}
}

ulong
µs(void)
{
	if(SystimerFreq != 1*Mhz)
		return fastticks2us(fastticks(nil));
	return ((Systimers*)SYSTIMERS)->clo;
}

void
microdelay(int n)
{
	Systimers *tn;
	u32int now, diff;

	diff = n + 1;
	tn = (Systimers*)SYSTIMERS;
	now = tn->clo;
	while(tn->clo - now < diff)
		;
}

void
delay(int n)
{
	while(--n >= 0)
		microdelay(1000);
}
