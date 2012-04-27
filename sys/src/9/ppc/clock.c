#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"io.h"
#include	"fns.h"
#include	"ureg.h"

static	ulong	clkreload;

void
delayloopinit(void)
{
	ulong v;
	uvlong x;

	/* initial value for loopconst set in machinit */
	m->loopconst = 1000;
	v = getdec();
	delay(1000);
	v -= getdec();

	x = m->loopconst;
	x *= m->dechz;
	x /= v;
	m->loopconst = x;
}

void
clockinit(void)
{
	m->dechz = m->bushz/4;	/* true for all 604e */
	m->tbhz = m->dechz;	/* conjecture; manual doesn't say */

	delayloopinit();

	clkreload = m->dechz/HZ-1;
	putdec(clkreload);
}

void
clockintr(Ureg *)
{
	long v;

	v = -getdec();
	if(v > (clkreload >> 1)){
		if(v > clkreload)
			m->ticks += v/clkreload;
		v = 0;
	}
	putdec(clkreload-v);
}

void
delay(int l)
{
	ulong i, j;

	j = 0;
	if(m)
		j = m->loopconst;
	if(j == 0)
		j = 1096;
	while(l-- > 0)
		for(i=0; i < j; i++)
			;
}

void
microdelay(int l)
{
	ulong i;

	l *= m->loopconst;
	l += 500;
	l /= 1000;
	if(l <= 0)
		l = 1;
	for(i = 0; i < l; i++)
		;
}

ulong
perfticks(void)
{
	return (ulong)fastticks(nil);
}
