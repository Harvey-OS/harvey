#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

void
delay(int l)
{
	ulong i, j;

	j = m->delayloop;
	while(l-- > 0)
		for(i=0; i < j; i++)
			;
}

void
microdelay(int l)
{
	ulong i;

	l *= m->delayloop;
	l /= 1000;
	if(l <= 0)
		l = 1;
	for(i = 0; i < l; i++)
		;
}

enum {
	Timebase = 1,	/* system clock cycles per time base cycle */

	Wp17=	0<<30,	/* watchdog period (2^x clocks) */
	Wp21=	1<<30,
	Wp25=	2<<30,
	Wp29=	3<<30,
	Wrnone=	0<<28,	/* no watchdog reset */
	Wrcore=	1<<28,	/* core reset */
	Wrchip=	2<<28,	/* chip reset */
	Wrsys=	3<<28,	/* system reset */
	Wie=		1<<27,	/* watchdog interrupt enable */
	Pie=		1<<26,	/* enable PIT interrupt */
	Fit9=		0<<24,	/* fit period (2^x clocks) */
	Fit13=	1<<24,
	Fit17=	2<<24,
	Fit21=	3<<24,
	Fie=		1<<23,	/* fit interrupt enable */
	Are=		1<<22,	/* auto reload enable */

	/* dcr */
	Boot=	0x0F1,
	Epctl=	0x0F3,
	Pllmr0=	0x0F0,
	Pllmr1=	0x0F4,
	Ucr=		0x0F5,
};

void	(*archclocktick)(void);	/* set by arch*.c if used */

void
clockinit(void)
{
	long x;

	assert(m->cpuhz != 0);
	m->delayloop = m->cpuhz/1000;	/* initial estimate */
	do {
		x = gettbl();
		delay(10);
		x = gettbl() - x;
	} while(x < 0);
	assert(x != 0);

	/*
	 *  fix count
	 */
	m->delayloop = ((vlong)m->delayloop*(10*(vlong)m->clockgen/1000))/(x*Timebase);
	if((int)m->delayloop <= 0)
		m->delayloop = 20000;

	x = (m->clockgen/Timebase)/HZ;
//iprint("initial PIT %ld\n", x);
	assert(x != 0);
	putpit(x);
	puttsr(~0);
	/* not sure if we want Wrnone or Wrcore */
//	puttcr(Pie|Are|Wie|Wrcore|Wp29);
	puttcr(Pie|Are|Wrnone);
}

void
clockintr(Ureg *ureg)
{
	/* PIT was set to reload automatically */
	puttsr(~0);
	m->fastclock++;
	timerintr(ureg, 0);
	if(archclocktick != nil)
		archclocktick();
}

uvlong
fastticks(uvlong *hz)
{
	if(hz)
		*hz = HZ;
	return m->fastclock;
}

ulong
µs(void)
{
	return fastticks2us(m->fastclock);
}

void
timerset(uvlong)
{
}

ulong
perfticks(void)
{
	return (ulong)fastticks(nil);
}

long
lcycles(void)
{
	return perfticks();
}
