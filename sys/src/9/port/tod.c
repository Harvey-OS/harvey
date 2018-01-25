/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

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
	uint32_t	cnt;
	Lock Lock;
	uint64_t	multiplier;	/* ns = off + (multiplier*ticks)>>31 */
	uint64_t	divider;	/* ticks = (divider*(ns-off))>>31 */
	uint64_t	umultiplier;	/* µs = (µmultiplier*ticks)>>31 */
	uint64_t	udivider;	/* ticks = (µdivider*µs)>>31 */
	int64_t	hz;		/* frequency of fast clock */
	int64_t	last;		/* last reading of fast clock */
	int64_t	off;		/* offset from epoch to last */
	int64_t	lasttime;	/* last return value from todget */
	int64_t	delta;	/* add 'delta' each slow clock tick from sstart to send */
	uint64_t	sstart;		/* ... */
	uint64_t	send;		/* ... */
} tod;

static void todfix(void);

void
todinit(void)
{
	if(tod.init)
		return;
	ilock(&tod.Lock);
	tod.last = fastticks((uint64_t *)&tod.hz);
	iunlock(&tod.Lock);
	todsetfreq(tod.hz);
	tod.init = 1;
	addclock0link(todfix, 100);
}

/*
 *  calculate multiplier
 */
void
todsetfreq(int64_t f)
{
	ilock(&tod.Lock);
	tod.hz = f;

	/* calculate multiplier for time conversion */
	tod.multiplier = mk64fract(TODFREQ, f);
	tod.divider = mk64fract(f, TODFREQ) + 1;
	tod.umultiplier = mk64fract(MicroFREQ, f);
	tod.udivider = mk64fract(f, MicroFREQ) + 1;
	iunlock(&tod.Lock);
}

/*
 *  Set the time of day struct
 */
void
todset(int64_t t, int64_t delta, int n)
{
	if(!tod.init)
		todinit();

	ilock(&tod.Lock);
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
		delta = delta/n;
		tod.sstart = sys->ticks;
		tod.send = tod.sstart + n;
		tod.delta = delta;
	}
	iunlock(&tod.Lock);
}

/*
 *  get time of day
 */
int64_t
todget(int64_t *ticksp)
{
	uint64_t x;
	int64_t ticks, diff;
	uint64_t t;

	if(!tod.init)
		todinit();

	/*
	 * we don't want time to pass twixt the measuring of fastticks
	 * and grabbing tod.last.  Also none of the int64_ts are atomic so
	 * we have to look at them inside the lock.
	 */
	ilock(&tod.Lock);
	tod.cnt++;
	ticks = fastticks(nil);

	/* add in correction */
	if(tod.sstart != tod.send){
		t = sys->ticks;
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

	iunlock(&tod.Lock);

	if(ticksp != nil)
		*ticksp = ticks;

	return x;
}

/*
 *  convert time of day to ticks
 */
uint64_t
tod2fastticks(int64_t ns)
{
	uint64_t x;

	ilock(&tod.Lock);
	mul64fract(&x, ns-tod.off, tod.divider);
	x += tod.last;
	iunlock(&tod.Lock);
	return x;
}

/*
 *  called regularly to avoid calculation overflows
 */
static void
todfix(void)
{
	int64_t ticks, diff;
	uint64_t x;

	ticks = fastticks(nil);

	diff = ticks - tod.last;
	if(diff > tod.hz){
		ilock(&tod.Lock);

		/* convert to epoch */
		mul64fract(&x, diff, tod.multiplier);
if(x > 30000000000ULL) print("todfix %llu\n", x);
		x += tod.off;

		/* protect against overflows */
		tod.last = ticks;
		tod.off = x;

		iunlock(&tod.Lock);
	}
}

int32_t
seconds(void)
{
	int64_t x;
	int i;

	x = todget(nil);
	x = x/TODFREQ;
	i = x;
	return i;
}

uint64_t
fastticks2us(uint64_t ticks)
{
	uint64_t res;

	if(!tod.init)
		todinit();
	mul64fract(&res, ticks, tod.umultiplier);
	return res;
}

uint64_t
us2fastticks(uint64_t us)
{
	uint64_t res;

	if(!tod.init)
		todinit();
	mul64fract(&res, us, tod.udivider);
	return res;
}

/*
 *  convert milliseconds to fast ticks
 */
uint64_t
ms2fastticks(uint32_t ms)
{
	if(!tod.init)
		todinit();
	return (tod.hz*ms)/1000ULL;
}

/*
 *  convert nanoseconds to fast ticks
 */
uint64_t
ns2fastticks(uint64_t ns)
{
	uint64_t res;

	if(!tod.init)
		todinit();
	mul64fract(&res, ns, tod.divider);
	return res;
}

/*
 *  convert fast ticks to ns
 */
uint64_t
fastticks2ns(uint64_t ticks)
{
	uint64_t res;

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
uint64_t
mk64fract(uint64_t to, uint64_t from)
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
