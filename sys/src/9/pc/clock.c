#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

/*
 *  this is called both by the mp's clock interrupt and
 *  by the clockintr0 for the i8253
 */
void
clockintr(Ureg* ureg, void*)
{
	/* this has to get called every tick or the 8253 timer will overflow */
	fastticks(nil);

	if(m->flushmmu){
		if(up)
			flushmmu();
		m->flushmmu = 0;
	}

	portclock(ureg);
}

void
delay(int millisecs)
{
	millisecs *= m->loopconst;
	if(millisecs <= 0)
		millisecs = 1;
	aamloop(millisecs);
}

void
microdelay(int microsecs)
{
	microsecs *= m->loopconst;
	microsecs /= 1000;
	if(microsecs <= 0)
		microsecs = 1;
	aamloop(microsecs);
}

/*  
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
ulong
perfticks(void)
{
	uvlong x;

	if(!m->havetsc)
		return ticks;
	rdtsc(&x);
	return x;
}
