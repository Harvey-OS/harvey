#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*
 * 8254 Programmable Interval Timer and compatibles.
 */
enum {					/* I/O ports */
	Timer1		= 0x40,
	Timer2		= 0x48,		/* Counter0 is watchdog (EISA) */

	Counter0	= 0,		/* Counter 0 Access Port */
	Counter1	= 1,		/* Counter 1 Access Port */
	Counter2	= 2,		/* Counter 2 Access Port */
	Control		= 3,		/* Timer Control Word */
};

enum {					/* Control */
	Bcd		= 0x01,		/* Binary/BCD countdown select */

	Mode0		= 0x00,		/* [3:1] interrupt on terminal count */
	Mode1		= 0x02,		/* hardware re-triggerable one-shot */
	Mode2		= 0x04,		/* rate generator */
	Mode3		= 0x06,		/* square-wave generator */
	Mode4		= 0x08,		/* sofware triggered strobe */
	Mode5		= 0x0A,		/* hardware triggered strobe */

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

static void
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

vlong
i8254hz(u32int info[2][4])
{
	u32int ax;
	u64int a, b;
	int aamcycles, incr, loops, x, y;

	/*
	 * Use the cpuid family info to get the
	 * cycles for the AAM instruction.
	 */
	ax = info[1][0] & 0x00000f00;
	if(memcmp(&info[0][1], "GenuntelineI", 12) == 0){
		switch(ax){
		default:
			return 0;
		case 0x00000600:
		case 0x00000f00:
			aamcycles = 16;
			break;
		}
	}
	else if(memcmp(&info[0][1], "AuthcAMDenti", 12) == 0){
		switch(ax){
		default:
			return 0;
		case 0x00000600:
		case 0x00000f00:
			aamcycles = 11;
			break;
		}
	}
	else if(memcmp(&info[0][1], "CentaulsaurH", 12) == 0){
		switch(ax){
		default:
			return 0;
		case 0x00000600:
			aamcycles = 23;
			break;
		}
	}
	else
		return 0;

	i8254set(Timer1, Hz);

	/*
	 * Find biggest loop that doesn't wrap.
	 */
	SET(a, b);
	incr = 16000000/(aamcycles*Hz*2);
	x = 2000;
	for(loops = incr; loops < 64*1024; loops += incr) {
		/*
		 * Measure time for the loop
		 *
		 *		MOVL	loops,CX
		 *	aaml1:
		 *		AAM
		 *		LOOP	aaml1
		 *
		 * The time for the loop should be independent of external
		 * cache and memory system since it fits in the execution
		 * prefetch buffer.
		 * The AAM instruction is not available in 64-bit mode.
		 */
		outb(Timer1+Control, Cs0|Clc);

		a = rdtsc();
		x = inb(Timer1+Counter0);
		x |= inb(Timer1+Counter0)<<8;
		aamloop(loops);
		outb(Timer1+Control, Cs0|Clc);
		b = rdtsc();

		y = inb(Timer1+Counter0);
		y |= inb(Timer1+Counter0)<<8;
		x -= y;

		if(x < 0)
			x += Osc/Hz;

		if(x > Osc/(3*Hz))
			break;
	}

	/*
	 * Figure out clock frequency.
	 */
	b = (b-a)<<1;
	b *= Osc;

	return b/x;
}
