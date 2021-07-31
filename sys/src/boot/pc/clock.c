#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

/*
 *  8253 timer
 */
enum
{
	T0cntr=	0x40,		/* counter ports */
	T1cntr=	0x41,		/* ... */
	T2cntr=	0x42,		/* ... */
	Tmode=	0x43,		/* mode port */

	/* commands */
	Latch0=	0x00,		/* latch counter 0's value */
	Load0=	0x30,		/* load counter 0 with 2 bytes */

	/* modes */
	Square=	0x36,		/* perioic square wave */

	Freq=	1193182,	/* Real clock frequency */
};

static int cpufreq = 66000000;
static int cpumhz = 66;
static int cputype = 486;
static int loopconst = 100;

/*
 *  delay for l milliseconds more or less.  delayloop is set by
 *  clockinit() to match the actual CPU speed.
 */
void
delay(int l)
{
	aamloop(l*loopconst);
}

static void
clock(Ureg*, void*)
{
	m->ticks++;
	checkalarms();
}

void
clockinit(void)
{
	ulong x, y;	/* change in counter */
	ulong cycles, loops;

	switch(cputype = x86()){
	case 386:
		loops = 10000;
		cycles = 32;
		break;
	case 486:
		loops = 10000;
		cycles = 22;
		break;
	default:
		loops = 30000;
		cycles = 23;
		break;
	}

	/*
	 *  set vector for clock interrupts
	 */
	setvec(Clockvec, clock, 0);

	/*
	 *  set clock for 1/HZ seconds
	 */
	outb(Tmode, Load0|Square);
	outb(T0cntr, (Freq/HZ));	/* low byte */
	outb(T0cntr, (Freq/HZ)>>8);	/* high byte */


	/*
	 *  measure time for the loop
	 *
	 *			MOVL	loops,CX
	 *	aaml1:	 	AAM
	 *			LOOP	aaml1
	 *
	 *  the time for the loop should be independent from external
	 *  cache's and memory system since it fits in the execution
	 *  prefetch buffer.
	 *
	 */
	outb(Tmode, Latch0);
	x = inb(T0cntr);
	x |= inb(T0cntr)<<8;
	aamloop(loops);
	outb(Tmode, Latch0);
	y = inb(T0cntr);
	y |= inb(T0cntr)<<8;
	x -= y;

	/*
	 *  counter  goes at twice the frequency, once per transition,
	 *  i.e., twice per square wave
	 */
	x >>= 1;

	/*
 	 *  figure out clock frequency and a loop multiplier for delay().
	 */
	cpufreq = loops*((cycles*Freq)/x);
	loopconst = (cpufreq/1000)/cycles;	/* AAM+LOOP's for 1 ms */

	/*
	 *  add in possible .1% error and convert to MHz
	 */
	cpumhz = (cpufreq + cpufreq/1000)/1000000;
}
