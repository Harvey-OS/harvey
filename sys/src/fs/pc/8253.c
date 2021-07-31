#include "all.h"
#include "mem.h"
#include "ureg.h"
#include "io.h"

/*
 *  8253 timer
 */
enum
{
	T0cntr	= 0x40,		/* counter ports */
	T1cntr	= 0x41,		/* ... */
	T2cntr	= 0x42,		/* ... */
	Tmode	= 0x43,		/* mode port */

	/* commands */
	Latch0	= 0x00,		/* latch counter 0's value */
	Load0	= 0x30,		/* load counter 0 with 2 bytes */

	/* modes */
	Square	= 0x36,		/* periodic square wave */
	Trigger	= 0x30,		/* interrupt on terminal count */

	Freq	= 1193182,	/* Real clock frequency */
};

static uvlong cpufreq	= 66000000;
static uvlong cpuhz;
static int cpumhz	= 66;
static int loopconst	= 100;
/*static*/ int cpuidax, cpuiddx;
static char cpuidid[16];
static int havetsc;

static void
clockintr(Ureg *ur, void *v)
{
	USED(v);
	clock(0, ur->pc);
}

#define STEPPING(x)	((x)&0xf)
#define MODEL(x)	(((x)>>4)&0xf)
#define FAMILY(x)	(((x)>>8)&0xf)

enum
{
	/* flags */
	CpuidFPU	= 0x001,	/* on-chip floating point unit */
	CpuidMCE	= 0x080,	/* machine check exception */
	CpuidCX8	= 0x100,	/* CMPXCHG8B instruction */
};

typedef struct
{
	int family;
	int model;
	int aalcycles;
	char *name;
} X86type;

static X86type x86intel[] =
{
	{ 4,	0,	22,	"486DX", },	/* known chips */
	{ 4,	1,	22,	"486DX50", },
	{ 4,	2,	22,	"486SX", },
	{ 4,	3,	22,	"486DX2", },
	{ 4,	4,	22,	"486SL", },
	{ 4,	5,	22,	"486SX2", },
	{ 4,	7,	22,	"DX2WB", },	/* P24D */
	{ 4,	8,	22,	"DX4", },	/* P24C */
	{ 4,	9,	22,	"DX4WB", },	/* P24CT */
	{ 5,	0,	23,	"P5", },
	{ 5,	1,	23,	"P5", },
	{ 5,	2,	23,	"P54C", },
	{ 5,	3,	23,	"P24T", },
	{ 5,	4,	23,	"P55C MMX", },
	{ 5,	7,	23,	"P54C VRT", },
	{ 6,	1,	16,	"PentiumPro", },/* trial and error */
	{ 6,	3,	16,	"PentiumII", },
	{ 6,	5,	16,	"PentiumII/Xeon", },
	{ 6,	6,	16,	"Celeron", },
	{ 6,	7,	16,	"PentiumIII/Xeon", },
	{ 6,	8,	16,	"PentiumIII/Xeon", },
	{ 6,	0xB,	16,	"PentiumIII/Xeon", },
	{ 0xF,	1,	16,	"P4", },	/* P4 */
	{ 0xF,	2,	16,	"PentiumIV/Xeon", },

	{ 3,	-1,	32,	"386", },	/* family defaults */
	{ 4,	-1,	22,	"486", },
	{ 5,	-1,	23,	"P5", },
	{ 6,	-1,	16,	"P6", },
	{ 0xF,	-1,	16,	"P4", },	/* P4 */

	{ -1,	-1,	16,	"unknown", },	/* total default */
};

/*
 * The AMD processors all implement the CPUID instruction.
 * The later ones also return the processor name via functions
 * 0x80000002, 0x80000003 and 0x80000004 in registers AX, BX, CX
 * and DX:
 *	K5	"AMD-K5(tm) Processor"
 *	K6	"AMD-K6tm w/ multimedia extensions"
 *	K6 3D	"AMD-K6(tm) 3D processor"
 *	K6 3D+	?
 */
static X86type x86amd[] =
{
	{ 5,	0,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	1,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	2,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	3,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	6,	11,	"AMD-K6", },	/* trial and error */
	{ 5,	7,	11,	"AMD-K6", },	/* trial and error */
	{ 5,	8,	11,	"AMD-K6-2", },	/* trial and error */
	{ 5,	9,	11,	"AMD-K6-III", },/* trial and error */

	{ 6,	1,	11,	"AMD-Athlon", },/* trial and error */
	{ 6,	2,	11,	"AMD-Athlon", },/* trial and error */

	{ 4,	-1,	22,	"Am486", },	/* guesswork */
	{ 5,	-1,	23,	"AMD-K5/K6", },	/* guesswork */
	{ 6,	-1,	11,	"AMD-Athlon", },/* guesswork */
	{ 0xF,	-1,	11,	"AMD64", },	/* guesswork */

	{ -1,	-1,	23,	"unknown", },	/* total default */
};

/*
 * WinChip 240MHz
 */
static X86type x86winchip[] =
{
	{5,	4,	23,	"Winchip",},	/* guesswork */
	{6,	7,	23,	"Via C3 Samuel 2 or Ezra",},
	{6,	8,	23,	"Via C3 Ezra-T",},
	{ -1,	-1,	23,	"unknown", },	/* total default */
};

/*
 * SiS 55x
 */
static X86type x86sis[] =
{
	{5,	0,	23,	"SiS 55x",},	/* guesswork */
	{ -1,	-1,	23,	"unknown", },	/* total default */
};

static X86type	*cputype;

static void
simplecycles(uvlong*x)
{
	*x = m->ticks;
}

void	(*cycles)(uvlong*) = simplecycles;
void	_cycles(uvlong*);	/* in l.s */

static void
nop(void)
{
}

void (*coherence)(void) = nop;

/*
 *  delay for l milliseconds more or less.  delayloop is set by
 *  clockinit() to match the actual CPU speed.
 */
void
delay(int l)
{
	l *= loopconst;
	if(l <= 0)
		l = 1;
	aamloop(l);
}

/*
 *  microsecond delay
 */
void
microdelay(int l)
{
	l *= loopconst;
	l /= 1000;
	if(l <= 0)
		l = 1;
	aamloop(l);
}

void
printcpufreq(void)
{
	int i;
	char buf[128];

	i = sprint(buf, "cpu%d: %dMHz ", 0, cpumhz);
	if(cpuidid[0])
		i += sprint(buf+i, "%s ", cpuidid);
	sprint(buf+i, "%s (cpuid: AX 0x%4.4ux DX 0x%4.4ux)\n",
		cputype->name, cpuidax, cpuiddx);
	print(buf);
}

void
clockinit(void)
{
	int x, y;	/* change in counter */
	int family, model, loops, incr;
	X86type *t;
	uvlong a, b;

	/*
	 *  set vector for clock interrupts
	 */
	setvec(Clockvec, clockintr, 0);

	/*
	 *  figure out what we are
	 */
	cpuid(cpuidid, &cpuidax, &cpuiddx);
	if(strncmp(cpuidid, "AuthenticAMD", 12) == 0)
		t = x86amd;
	else if(strncmp(cpuidid, "CentaurHauls", 12) == 0)
		t = x86winchip;
	else if(strncmp(cpuidid, "SiS SiS SiS ", 12) == 0)
		t = x86sis;
	else
		t = x86intel;
	family = FAMILY(cpuidax);
	model = MODEL(cpuidax);
	while(t->name){
		if((t->family == family && t->model == model)
		|| (t->family == family && t->model == -1)
		|| (t->family == -1))
			break;
		t++;
	}
	cputype = t;

	if(family >= 5)
		coherence = wbflush;

	/*
	 *  if there is one, set tsc to a known value
	 */
	if(cpuiddx & 0x10){
		havetsc = 1;
		cycles = _cycles;
		if(cpuiddx & 0x20)
			wrmsr(0x10, 0);
	}

	/*
	 *  set clock for 1/HZ seconds
	 */
	outb(Tmode, Load0|Square);
	outb(T0cntr, (Freq/HZ));	/* low byte */
	outb(T0cntr, (Freq/HZ)>>8);	/* high byte */

	/*
	 * Introduce a little delay to make sure the count is
	 * latched and the timer is counting down; with a fast
	 * enough processor this may not be the case.
	 * The i8254 (which this probably is) has a read-back
	 * command which can be used to make sure the counting
	 * register has been written into the counting element.
	 */
	x = (Freq/HZ);
	for(loops = 0; loops < 100000 && x >= (Freq/HZ); loops++){
		outb(Tmode, Latch0);
		x = inb(T0cntr);
		x |= inb(T0cntr)<<8;
	}

	/* find biggest loop that doesn't wrap */
	incr = 16000000/(t->aalcycles*HZ*2);
	x = 2000;
	for(loops = incr; loops < 64*1024; loops += incr) {

		/*
		 *  measure time for the loop
		 *
		 *			MOVL	loops,CX
		 *	aaml1:	 	AAM
		 *			LOOP	aaml1
		 *
		 *  the time for the loop should be independent of external
		 *  cache and memory system since it fits in the execution
		 *  prefetch buffer.
		 *
		 */
		outb(Tmode, Latch0);
		cycles(&a);
		x = inb(T0cntr);
		x |= inb(T0cntr)<<8;
		aamloop(loops);
		outb(Tmode, Latch0);
		cycles(&b);
		y = inb(T0cntr);
		y |= inb(T0cntr)<<8;
		x -= y;

		if(x < 0)
			x += Freq/HZ;

		if(x > Freq/(3*HZ))
			break;
	}

	/*
 	 *  figure out clock frequency and a loop multiplier for delay().
	 *  n.b. counter goes up by 2*Freq
	 */
	cpufreq = (vlong)loops*((t->aalcycles*2*Freq)/x);
	loopconst = (cpufreq/1000)/t->aalcycles;	/* AAM+LOOP's for 1 ms */
	if (0)
		print("loops %d x %d cpufreq %,lld loopconst %d\n", loops, x, cpufreq, loopconst);

	if(havetsc){
		/* counter goes up by 2*Freq */
		b = (b-a)<<1;
		b *= Freq;
		b /= x;

		/*
		 *  round to the nearest megahz
		 */
		cpumhz = (b+500000)/1000000L;
		cpuhz = b;
	} else {
		/*
		 *  add in possible 0.5% error and convert to MHz
		 */
		cpumhz = (cpufreq + cpufreq/200)/1000000;
		cpuhz = cpufreq;
	}
	if (0) {
		print("cpuhz %,lld cpumhz %d\n", cpuhz, cpumhz);
		delay(10*1000);
	}
}

void
clockreload(Timet n)
{
	USED(n);
}
