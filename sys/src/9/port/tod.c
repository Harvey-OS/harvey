#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

// compute nanosecond epoch time from the fastest ticking clock
// on the system.  converting the time to nanoseconds requires
// the following formula
//
//	t = (((1000000000<<31)/f)*ticks)>>31
//
//  where
//
//	'f'		is the clock frequency
//	'ticks'		are clock ticks
//
//  to avoid too much calculation in todget(), we calculate
//
//	mult = (1000000000<<31)/f
//
//  each time f is set.  f is normally set by a user level
//  program writing to /dev/fastclock.
//
//  We assume that the cpu's of a multiprocessor are synchronized.
//  This assumption needs to be questioned with each new architecture.


// frequency of the tod clock
#define TODFREQ	1000000000ULL

struct {
	ulong	cnt;
	Lock;
	vlong	multiplier;	// t = off + (multiplier*ticks)>>31
	vlong	hz;		// frequency of fast clock
	vlong	last;		// last reading of fast clock
	vlong	off;		// offset from epoch to last
	vlong	lasttime;	// last return value from todget
	vlong	delta;	// add 'delta' each slow clock tick from sstart to send
	ulong	sstart;	// ...
	ulong	send;	// ...
} tod;

void
todinit(void)
{
	ilock(&tod);
	tod.last = fastticks((uvlong*)&tod.hz);
	iunlock(&tod);
	todsetfreq(tod.hz);
	addclock0link(todfix);
}

//
//  calculate multiplier
//
void
todsetfreq(vlong f)
{
	ilock(&tod);
	tod.hz = f;

	/* calculate multiplier for time conversion */
	tod.multiplier = (TODFREQ<<31)/f;

	iunlock(&tod);
}

//
//  Set the time of day struct
//
void
todset(vlong t, vlong delta, int n)
{
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
		delta = delta/n;
		tod.sstart = MACHP(0)->ticks;
		tod.send = tod.sstart + n;
		tod.delta = delta;
	}
	iunlock(&tod);
}

//
//  get time of day
//
vlong
todget(vlong *ticksp)
{
	uvlong x;
	vlong ticks, diff;
	ulong t;

	if(tod.hz == 0)
		ticks = fastticks((uvlong*)&tod.hz);
	else
		ticks = fastticks(nil);

	// since 64 bit loads are not atomic, we have to lock around them
	ilock(&tod);
	tod.cnt++;

	// add in correction
	if(tod.sstart != tod.send){
		t = MACHP(0)->ticks;
		if(t >= tod.send)
			t = tod.send;
		tod.off = tod.off + tod.delta*(t - tod.sstart);
		tod.sstart = t;
	}

	// convert to epoch
	diff = ticks - tod.last;
	x = (diff * tod.multiplier) >> 31;
	x = x + tod.off;

	// time can't go backwards
	if(x < tod.lasttime)
		x = tod.lasttime;
	else
		tod.lasttime = x;

	iunlock(&tod);

	if(ticksp != nil)
		*ticksp = ticks;

	return x;
}

//
//  called every clock tick to avoid calculation overflows
//
void
todfix(void)
{
	vlong ticks, diff;
	uvlong x;

	ticks = fastticks(nil);

	diff = ticks - tod.last;
	if(diff > tod.hz){
		ilock(&tod);
	
		// convert to epoch
		x = diff * tod.multiplier;
		x = x >> 31;
		x = x + tod.off;
	
		// protect against overflows
		tod.last = ticks;
		tod.off = x;
	
		iunlock(&tod);
	}
}

long
seconds(void)
{
	vlong x;
	int i;

	x = todget(nil);
	x = x/TODFREQ;
	i = x;
	return i;
}

//  convert milliseconds to fast ticks
//
uvlong
ms2fastticks(ulong ms)
{
	if(tod.hz == 0)
		fastticks((uvlong*)&tod.hz);
	return (tod.hz*ms)/1000ULL;
}
