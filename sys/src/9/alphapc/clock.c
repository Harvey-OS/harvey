#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"axp.h"
#include	"ureg.h"

void
clockinit(void)
{
}

uvlong
cycletimer(void)
{
	ulong pcc;
	vlong delta;

	pcc = rpcc(nil) & 0xFFFFFFFF;
	if(m->cpuhz == 0){
		/*
		 * pcclast is needed to detect wraparound of
		 * the cycle timer which is only 32-bits.
		 * m->cpuhz is set from the info passed from
		 * the firmware.
		 * This could be in clockinit if can
		 * guarantee no wraparound between then and now.
		 *
		 * All the clock stuff needs work.
		 */
		m->cpuhz = hwrpb->cfreq;
		m->pcclast = pcc;
	}
	delta = pcc - m->pcclast;
	if(delta < 0)
		delta += 0x100000000LL;
	m->pcclast = pcc;
	m->fastclock += delta;

	return MACHP(0)->fastclock;
}

uvlong
fastticks(uvlong* hz)
{
	uvlong ticks;
	int x;

	x = splhi();
	ticks = cycletimer();
	splx(x);

	if(hz)
		*hz = m->cpuhz;

	return ticks;
}

ulong
µs(void)
{
	return fastticks2us(cycletimer());
}

/*  
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
ulong
perfticks(void)
{
	return rpcc(nil);
}

void
timerset(Tval)
{
}

void
microdelay(int us)
{
	uvlong eot;

	eot = fastticks(nil) + (m->cpuhz/1000000)*us;
	while(fastticks(nil) < eot)
		;
}

void
delay(int millisecs)
{
	microdelay(millisecs*1000);
}

void
clock(Ureg *ureg)
{
	static int count;

	cycletimer();

	/* HZ == 100, timer == 1024Hz.  error < 1ms */
	count += 100;
	if (count < 1024)
		return;
	count -= 1024;

	timerintr(ureg, 0);
}
