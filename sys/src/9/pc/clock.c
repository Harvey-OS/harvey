#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

void
clockintr(Ureg* ureg, void*)
{
	// this needs to be here for synchronizing mp clocks
	// see mp.c's squidboy()
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
