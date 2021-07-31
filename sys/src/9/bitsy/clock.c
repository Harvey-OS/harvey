#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

typedef struct OSTimer
{
	ulong		osmr[4];	/* match registers */
	volatile ulong	oscr;		/* counter register */
	ulong		ossr;		/* status register */
	ulong		ower;	/* watchdog enable register */
	ulong		oier;		/* timer interrupt enable register */
} OSTimer;

OSTimer *timerregs = (OSTimer*)OSTIMERREGS;
static int clockinited;

static void		clockintr(Ureg*, void*);
static uvlong	when;	/* scheduled time of next interrupt */

long	timeradjust;

enum
{
	Minfreq = ClockFreq/HZ,		/* At least one interrupt per HZ (50 ms) */
	Maxfreq = ClockFreq/10000,	/* At most one interrupt every 100 µs */
};

void
clockpower(int on)
{

	if (on){
		timerregs->ossr |= 1<<0;
		timerregs->oier = 1<<0;
		timerregs->osmr[0] = timerregs->oscr + Minfreq;
	}
	clockinited = on;
}

void
clockinit(void)
{
	ulong x;
	ulong id;

	/* map the clock registers */
	timerregs = mapspecial(OSTIMERREGS, 32);

	/* enable interrupts on match register 0, turn off all others */
	timerregs->ossr |= 1<<0;
	intrenable(IRQ, IRQtimer0, clockintr, nil, "clock");
	timerregs->oier = 1<<0;

	/* figure out processor frequency */
	x = powerregs->ppcr & 0x1f;
	conf.hz = ClockFreq*(x*4+16);
	conf.mhz = (conf.hz+499999)/1000000;

	/* get processor type */
	id = getcpuid();

	print("%lud MHZ ARM, ver %lux/part %lux/step %lud\n", conf.mhz,
		(id>>16)&0xff, (id>>4)&0xfff, id&0xf);

	/* post interrupt 1/HZ secs from now */
	when = timerregs->oscr + Minfreq;
	timerregs->osmr[0] = when;

	timersinit();

	clockinited = 1;
}

/*  turn 32 bit counter into a 64 bit one.  since todfix calls
 *  us at least once a second and we overflow once every 1165
 *  seconds, we won't miss an overflow.
 */
uvlong
fastticks(uvlong *hz)
{
	static uvlong high;
	static ulong last;
	ulong x;

	if(hz != nil)
		*hz = ClockFreq;
	x = timerregs->oscr;
	if(x < last)
		high += 1LL<<32;
	last = x;
	return high+x;
}

void
timerset(uvlong v)
{
	ulong next, tics;	/* Must be unsigned! */
	static int count;

	next = v;

	/* post next interrupt: calculate # of tics from now */
	tics = next - timerregs->oscr - Maxfreq;
	if (tics > Minfreq){
		timeradjust++;
		next = timerregs->oscr + Maxfreq;
	}
	timerregs->osmr[0] = next;
}

static void
clockintr(Ureg *ureg, void*)
{
	/* reset previous interrupt */
	timerregs->ossr |= 1<<0;
	when += Minfreq;
	timerregs->osmr[0] = when;	/* insurance */

	timerintr(ureg, when);
}

void
delay(int ms)
{
	ulong start;
	int i;

	if(clockinited){
		while(ms-- > 0){
			start = timerregs->oscr;
			while(timerregs->oscr-start < ClockFreq/1000)
				;
		}
	} else {
		while(ms-- > 0){
			for(i = 0; i < 1000; i++)
				;
		}
	}
}

void
µdelay(ulong µs)
{
	ulong start;
	int i;

	µs++;
	if(clockinited){
		start = timerregs->oscr;
		while(timerregs->oscr - start < 1UL+(µs*ClockFreq)/1000000UL)
			;
	} else {
		while(µs-- > 0){
			for(i = 0; i < 10; i++)
				;
		}
	}
}

ulong
TK2MS(ulong ticks)
{
	uvlong t, hz;

	t = ticks;
	hz = HZ;
	t *= 1000L;
	t = t/hz;
	ticks = t;
	return ticks;
}
