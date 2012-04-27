#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * Compute nanosecond epoch time from the fastest ticking clock
 * on the system.  Converting the time to nanoseconds requires
 * the following formula
 *
 *	t = (((1000000000<<31)/f)*ticks)>>31
 *
 *  where
 *
 *	'f'		is the clock frequency
 *	'ticks'		are clock ticks
 *
 *  to avoid too much calculation in todget(), we calculate
 *
 *	mult = (1000000000<<32)/f
 *
 *  each time f is set.  f is normally set by a user level
 *  program writing to /dev/fastclock.  mul64fract will then
 *  take that fractional multiplier and a 64 bit integer and
 *  return the resulting integer product.
 *
 *  We assume that the cpu's of a multiprocessor are synchronized.
 *  This assumption needs to be questioned with each new architecture.
 */

/* frequency of the tod clock */
#define TODFREQ		1000000000ULL
#define MicroFREQ	1000000ULL

struct {
	int	init;		/* true if initialized */
	ulong	cnt;
	Lock;
	uvlong	multiplier;	/* ns = off + (multiplier*ticks)>>31 */
	uvlong	divider;	/* ticks = (divider*(ns-off))>>31 */
	uvlong	umultiplier;	/* µs = (µmultiplier*ticks)>>31 */
	uvlong	udivider;	/* ticks = (µdivider*µs)>>31 */
	vlong	hz;		/* frequency of fast clock */
	vlong	last;		/* last reading of fast clock */
	vlong	off;		/* offset from epoch to last */
	vlong	lasttime;	/* last return value from todget */
	vlong	delta;	/* add 'delta' each slow clock tick from sstart to send */
	ulong	sstart;		/* ... */
	ulong	send;		/* ... */
} tod;

static void todfix(void);

void
todinit(void)
{
	if(tod.init)
		return;
	ilock(&tod);
	tod.init = 1;			/* prevent reentry via fastticks */
	tod.last = fastticks((uvlong *)&tod.hz);
	iunlock(&tod);
	todsetfreq(tod.hz);
	addclock0link(todfix, 100);
}

/*
 *  calculate multiplier
 */
void
todsetfreq(vlong f)
{
	if (f <= 0)
		panic("todsetfreq: freq %lld <= 0", f);
	ilock(&tod);
	tod.hz = f;

	/* calculate multiplier for time conversion */
	tod.multiplier = mk64fract(TODFREQ, f);
	tod.divider = mk64fract(f, TODFREQ) + 1;
	tod.umultiplier = mk64fract(MicroFREQ, f);
	tod.udivider = mk64fract(f, MicroFREQ) + 1;
	iunlock(&tod);
}

/*
 *  Set the time of day struct
 */
void
todset(vlong t, vlong delta, int n)
{
	if(!tod.init)
		todinit();

	ilock(&tod);
	if(t >= 0){
		tod.off = t;
		tod.last = fastticks(nil);
		tod.lasttime = 0;
		tod.delta = 0;
		tod.sstart = tod.send;
	} else {
		if(n <= 0)
			n = 1;
		n *= HZ;
		if(delta < 0 && n > -delta)
			n = -delta;
		if(delta > 0 && n > delta)
			n = delta;
		if (n == 0) {
			iprint("todset: n == 0, delta == %lld\n", delta);
			delta = 0;
		} else
			delta /= n;
		tod.sstart = MACHP(0)->ticks;
		tod.send = tod.sstart + n;
		tod.delta = delta;
	}
	iunlock(&tod);
}

/*
 *  get time of day
 */
vlong
todget(vlong *ticksp)
{
	uvlong x;
	vlong ticks, diff;
	ulong t;

	if(!tod.init)
		todinit();

	/*
	 * we don't want time to pass twixt the measuring of fastticks
	 * and grabbing tod.last.  Also none of the vlongs are atomic so
	 * we have to look at them inside the lock.
	 */
	ilock(&tod);
	tod.cnt++;
	ticks = fastticks(nil);

	/* add in correction */
	if(tod.sstart != tod.send){
		t = MACHP(0)->ticks;
		if(t >= tod.send)
			t = tod.send;
		tod.off = tod.off + tod.delta*(t - tod.sstart);
		tod.sstart = t;
	}

	/* convert to epoch */
	diff = ticks - tod.last;
	if(diff < 0)
		diff = 0;
	mul64fract(&x, diff, tod.multiplier);
	x += tod.off;

	/* time can't go backwards */
	if(x < tod.lasttime)
		x = tod.lasttime;
	else
		tod.lasttime = x;

	iunlock(&tod);

	if(ticksp != nil)
		*ticksp = ticks;

	return x;
}

/*
 *  convert time of day to ticks
 */
uvlong
tod2fastticks(vlong ns)
{
	uvlong x;

	ilock(&tod);
	mul64fract(&x, ns-tod.off, tod.divider);
	x += tod.last;
	iunlock(&tod);
	return x;
}

/*
 *  called regularly to avoid calculation overflows
 */
static void
todfix(void)
{
	vlong ticks, diff;
	uvlong x;

	ticks = fastticks(nil);

	diff = ticks - tod.last;
	if(diff > tod.hz){
		ilock(&tod);

		/* convert to epoch */
		mul64fract(&x, diff, tod.multiplier);
if(x > 30000000000ULL) iprint("todfix %llud\n", x);
		x += tod.off;

		/* protect against overflows */
		tod.last = ticks;
		tod.off = x;

		iunlock(&tod);
	}
}

long
seconds(void)
{
	return (vlong)todget(nil) / TODFREQ;
}

uvlong
fastticks2us(uvlong ticks)
{
	uvlong res;

	if(!tod.init)
		todinit();
	mul64fract(&res, ticks, tod.umultiplier);
	return res;
}

uvlong
us2fastticks(uvlong us)
{
	uvlong res;

	if(!tod.init)
		todinit();
	mul64fract(&res, us, tod.udivider);
	return res;
}

/*
 *  convert milliseconds to fast ticks
 */
uvlong
ms2fastticks(ulong ms)
{
	if(!tod.init)
		todinit();
	return (tod.hz*ms)/1000ULL;
}

/*
 *  convert nanoseconds to fast ticks
 */
uvlong
ns2fastticks(uvlong ns)
{
	uvlong res;

	if(!tod.init)
		todinit();
	mul64fract(&res, ns, tod.divider);
	return res;
}

/*
 *  convert fast ticks to ns
 */
uvlong
fastticks2ns(uvlong ticks)
{
	uvlong res;

	if(!tod.init)
		todinit();
	mul64fract(&res, ticks, tod.multiplier);
	return res;
}

/*
 * Make a 64 bit fixed point number that has a decimal point
 * to the left of the low order 32 bits.  This is used with
 * mul64fract for converting twixt nanoseconds and fastticks.
 *
 *	multiplier = (to<<32)/from
 */
uvlong
mk64fract(uvlong to, uvlong from)
{
/*
	int shift;

	if(to == 0ULL)
		return 0ULL;

	shift = 0;
	while(shift < 32 && to < (1ULL<<(32+24))){
		to <<= 8;
		shift += 8;
	}
	while(shift < 32 && to < (1ULL<<(32+31))){
		to <<= 1;
		shift += 1;
	}

	return (to/from)<<(32-shift);
 */
	return (to<<32) / from;
}
