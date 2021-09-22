/*
 * misc. use of 8080 I/O ports
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum {
	Ppcireset =	0xcf9,		/* pci reset i/o port */
};

void
pcicf9reset(void)
{
	int i;

	/*
	 * The reset register (Ppcireset) is usually in one of the bridge
	 * chips. The actual location and sequence could be extracted from
	 * ACPI but why bother, this is the end of the line anyway.
	 */
	i = inb(Ppcireset) & 6;			/* ICHx reset control */
	outb(Ppcireset, i|0x02);		/* SYS_RST */
	millidelay(1);
	outb(Ppcireset, i|0x06);		/* RST_CPU transition */
}

void
nmienable(void)
{
	int x;

	/*
	 * Hack: should be locked with NVRAM access.
	 */
	outb(NVRADDR, 0x80);		/* NMI latch clear */
	outb(NVRADDR, 0);

	x = inb(KBDCTLB) & 0x07;	/* Enable NMI */
	outb(KBDCTLB, 0x08|x);
	outb(KBDCTLB, x);
}

/*
 * 8254 Programmable Interval Timer and compatibles.
 */
enum {					/* I/O port offsets */
	Counter0	= 0,		/* Counter 0 Access Port */
	Control		= 3,		/* Timer Control Word */
};

enum {					/* Control */
	Mode3		= 0x06,		/* square-wave generator */

	Clc		= 0x00,		/* [5:4] Counter Latch Command */
	RWlsb		= 0x10,		/* R/W LSB */
	RWmsb		= 0x20,		/* R/W MSB */
	RW16		= 0x30,		/* R/W LSB then MSB */
	Cs0		= 0x00,		/* [7:6] Counter 0 Select */
	Cs1		= 0x40,		/* Counter 1 Select */
	Cs2		= 0x80,		/* Counter 2 Select */

	Rbc		= 0xC0,		/* Read-Back Command */
	RbCnt0		= 0x02,		/* Select Counter 0 */
	RbCnt1		= 0x04,		/* Select Counter 1 */
	RbCnt2		= 0x08,		/* Select Counter 2 */
	RbS		= 0x20,		/* Read-Back Status */
	RbC		= 0x10,		/* Read-Back Count */
	RbCS		= 0x00,		/* Read-Back Count and Status */

	RbNULL		= 0x40,		/* NULL-Count Flag */
	RbOUT		= 0x80,		/* OUT-pin */
};

enum {
	Osc		= 1193182,	/* 14.318180MHz/12 */
	Hz		= 82,		/* 2*41*14551 = 1193182 */
};

void
i8254set(int port, int hz)
{
	int counter, timeo;

	/*
	 * Initialise Counter0 to be the system clock if necessary,
	 * it's normally connected to IRQ0 on an interrupt controller.
	 * Use a periodic square wave (Mode3).
	 */
	counter = Osc/hz;
	outb(port+Control, Cs0|RW16|Mode3);
	outb(port+Counter0, counter);
	outb(port+Counter0, counter>>8);

	/*
	 * Wait until the counting register has been loaded
	 * into the counting element.
	 */
	for(timeo = 0; timeo < 100000; timeo++){
		outb(port+Control, Rbc|RbS|RbCnt0);
		if(!(inb(port+Counter0) & RbNULL))
			break;
	}
}

/* i8254 timing assist */
uvlong
sampletimer(int tmrport, int *cntp)
{
	int cnt;
	uvlong cyc;

	outb(tmrport+Control, Cs0|Clc);
	cyc = rdtsc();
	cnt = inb(tmrport+Counter0);
	*cntp = cnt | inb(tmrport+Counter0)<<8;
	return cyc;
}

