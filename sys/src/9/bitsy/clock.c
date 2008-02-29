#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"


enum {
	RTCREGS	=	0x90010000,	/* real time clock registers */
	RTSR_al	=	0x01,		/* alarm detected */
	RTSR_hz	=	0x02,		/* 1Hz tick */
	RTSR_ale=	0x04,		/* alarm interrupt enable */
	RTSR_hze=	0x08,		/* 1Hz tick enable */

	Never	=	0xffffffff,
};

typedef struct OSTimer
{
	ulong		osmr[4];	/* match registers */
	volatile ulong	oscr;		/* counter register */
	ulong		ossr;		/* status register */
	ulong		ower;	/* watchdog enable register */
	ulong		oier;		/* timer interrupt enable register */
} OSTimer;

typedef struct RTCregs 
{
	ulong	rtar;	/* alarm */
	ulong	rcnr;	/* count */
	ulong	rttr;	/* trim */
	ulong	dummy;	/* hole */
	ulong	rtsr;	/* status */
} RTCregs;

OSTimer *timerregs = (OSTimer*)OSTIMERREGS;
RTCregs *rtcregs = (RTCregs*)RTCREGS;
static int clockinited;

static void	clockintr(Ureg*, void*);
static void	rtcintr(Ureg*, void*);
static Tval	when;	/* scheduled time of next interrupt */

long	timeradjust;

enum
{
	Minfreq = ClockFreq/HZ,		/* At least one interrupt per HZ (50 ms) */
	Maxfreq = ClockFreq/10000,	/* At most one interrupt every 100 µs */
};

ulong
clockpower(int on)
{
	static ulong savedtime;

	if (on){
		timerregs->ossr |= 1<<0;
		timerregs->oier = 1<<0;
		timerregs->osmr[0] = timerregs->oscr + Minfreq;
		if (rtcregs->rttr == 0){
			rtcregs->rttr = 0x8000; // nominal frequency.
			rtcregs->rcnr = 0;
			rtcregs->rtar = 0xffffffff;
			rtcregs->rtsr |= RTSR_ale;
			rtcregs->rtsr |= RTSR_hze;
		}
		if (rtcregs->rcnr > savedtime)
			return rtcregs->rcnr - savedtime;
	} else
		savedtime = rtcregs->rcnr;
	clockinited = on;
	return 0L;
}

void
clockinit(void)
{
	ulong x;
	ulong id;

	/* map the clock registers */
	timerregs = mapspecial(OSTIMERREGS, sizeof(OSTimer));
	rtcregs   = mapspecial(RTCREGS, sizeof(RTCregs));

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

	/* enable RTC interrupts and alarms */
	intrenable(IRQ, IRQrtc, rtcintr, nil, "rtc");
	rtcregs->rttr = 0x8000; 	// make rcnr   1Hz
	rtcregs->rcnr = 0;		// reset counter
	rtcregs->rtsr |= RTSR_al;
	rtcregs->rtsr |= RTSR_ale;

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

ulong
µs(void)
{
	return fastticks2us(fastticks(nil));
}

void
timerset(Tval v)
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
rtcalarm(ulong secs)
{
	vlong t;

	if (t == 0){
		iprint("RTC alarm cancelled\n");
		rtcregs->rtsr &= ~RTSR_ale;
		rtcregs->rtar = 0xffffffff;
	} else {
		t = todget(nil);
		t = t / 1000000000ULL; // nsec to secs
		if (secs < t)
			return;
		secs -= t;
		iprint("RTC alarm set to %uld seconds from now\n", secs);
		rtcregs->rtar = rtcregs->rcnr + secs;
		rtcregs->rtsr|= RTSR_ale;
	}
}

static void
rtcintr(Ureg*, void*)
{
	/* reset interrupt */
	rtcregs->rtsr&= ~RTSR_ale;
	rtcregs->rtsr&= ~RTSR_al;

	rtcregs->rtar = 0;
	iprint("RTC alarm: %lud\n", rtcregs->rcnr);
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
microdelay(int µs)
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

/*  
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
ulong
perfticks(void)
{
	return timerregs->oscr;
}
