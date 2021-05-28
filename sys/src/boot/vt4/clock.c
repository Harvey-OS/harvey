/* virtex[45] ppc4xx clock */
#include "include.h"

uvlong clockintrs;

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
};

void
prcpuid(void)
{
	ulong pvr;
	static char xilinx[] = "Xilinx ";
	static char ppc[] = "PowerPC";

	pvr = getpvr();
	m->cputype = pvr >> 16;
	print("cpu%d: %ldMHz %#lux (", m->machno, m->cpuhz/1000000, pvr);
	switch (m->cputype) {
	case 0x1291: case 0x4011: case 0x41f1: case 0x5091: case 0x5121:
		print("%s 405", ppc);
		break;
	case 0x2001:			/* 200 is Xilinx, 1 is ppc405 */
		print(xilinx);
		switch (pvr & ~0xfff) {
		case 0x20010000:
			print("Virtex-II Pro %s 405", ppc);
			break;
		case 0x20011000:
			print("Virtex 4 FX %s 405D5X2", ppc);
			break;
		default:
			print("%s 405", ppc);
			break;
		}
		break;
	case 0x7ff2:
		print(xilinx);
		if ((pvr & ~0xf) == 0x7ff21910)
			print("Virtex 5 FXT %s 440X5", ppc);
		else
			print("%s 440", ppc);
		break;
	default:
		print(ppc);
		break;
	}
	print(")\n");
}

void
clockinit(void)
{
	int s;
	long x;
	vlong now;

	s = splhi();
	m->clockgen = m->cpuhz = myhz;
	m->delayloop = m->cpuhz/1000;		/* initial estimate */
	do {
		x = gettbl();
		delay(10);
		x = gettbl() - x;
	} while(x < 0);

	/*
	 *  fix count
	 */
	assert(x != 0);
	m->delayloop = ((vlong)m->delayloop * (10*(vlong)m->clockgen/1000)) /
		(x*Timebase);
	if((int)m->delayloop <= 0)
		m->delayloop = 20000;

	x = (m->clockgen/Timebase)/HZ;
// print("initial PIT %ld\n", x);
	putpit(x);
	puttsr(~0);
	puttcr(Pie|Are);
	coherence();
	splx(s);

	now = m->fastclock;
	x = 50000000UL;
	while (now == m->fastclock && x-- > 0)
		;
	if (now == m->fastclock)
		print("clock is NOT ticking\n");
}

void
clockintr(Ureg *ureg)
{
	/* PIT was set to reload automatically */
	puttsr(~0);
	m->fastclock++;
	dcflush(PTR2UINT(&m->fastclock), sizeof m->fastclock); /* seems needed */
	clockintrs++;
	dcflush(PTR2UINT(&clockintrs), sizeof clockintrs);  /* seems needed */
	timerintr(ureg);
}
