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
};

enum {
	Osc		= 1193182,	/* 14.318180MHz/12 */
	Hz		= 82,		/* 2*41*14551 = 1193182 */
};

static int
mfcaamcycles(int ax, char *manuf, int cycs)
{
	switch(ax){
	case 0x600:		/* on intel, all other 686 */
	case 0xf00:		/* on intel, xeons */
		return cycs;
	default:
		print("i8254hz: unknown %s cpu family %#ux\n", manuf, ax);
		return 0;
	}
}

/*
 * Use the cpuid family info to get the
 * cycles for the AAM instruction.
 */
static int
getaamcycles(u32int info[2][4])
{
	uint ax;
	char *mfcid;

	ax = info[Procsig][Ax] & 0xf00;
	mfcid = (char *)&info[Highstdfunc][Bx];
	if(memcmp(mfcid, "GenuntelineI", 12) == 0)
		return mfcaamcycles(ax, "intel", 16);
	else if(memcmp(mfcid, "AuthcAMDenti", 12) == 0)
		return mfcaamcycles(ax, "amd", 11);
	else if(memcmp(mfcid, "CentaulsaurH", 12) == 0)
		return mfcaamcycles(ax, "via", 23);
	return 0;
}

vlong
i8254hz(u32int info[2][4])
{
	u64int a, b;
	int aamcycles, incr, loops, x, y, mhz;

	aamcycles = getaamcycles(info);
	if (aamcycles == 0)
		return 0;

	/*
	 * Find biggest loop that doesn't wrap.
	 */
	i8254set(Timer1, Hz);
	x = 2000;
	incr = 16000000 / (aamcycles*Hz*2); /* *2 for PAUSE/PAUSE/DECQ/JNZ? */
	a = b = loops = 0;
	while ((loops += incr) < 64*1024) {
		/*
		 * Measure time for a loop similar to
		 *
		 *		MOVL	loops,CX
		 *	aaml1:
		 *		AAM
		 *		LOOP	aaml1
		 *
		 * The loop time should be independent of external cache &
		 * memory system since it fits in the execution prefetch buffer.
		 */
		a = sampletimer(Timer1, &x);
		aamloop(loops);
		b = sampletimer(Timer1, &y);
		x -= y;
		if(x < 0)
			x += Osc/Hz;

		if(x > Osc/(3*Hz))
			break;
	}

	/*
	 * Figure out clock frequency.
	 */
	b = (b-a) * 2 * Osc;
	if (x)
		b /= x;

	mhz = (b + 500000) / 1000000;
	print(" %,ud MHz actual, by 8254 timer\n", mhz);
	return b;
}
