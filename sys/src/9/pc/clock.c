#include	"u.h"
#include	"../port/lib.h"
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
	Trigger= 0x30,		/* interrupt on terminal count */

	Freq=	1193182,	/* Real clock frequency */
};

static int cpufreq = 66000000;
static int cpumhz = 66;
static int loopconst = 100;
static int cpuidax, cpuiddx;

static void
clock(Ureg *ur, void *arg)
{
	Proc *p;
	int nrun = 0;

	USED(arg);

	m->ticks++;

	checkalarms();
	uartclock();
	hardclock();

	/*
	 *  process time accounting
	 */
	p = m->proc;
	if(p){
		nrun = 1;
		p->pc = ur->pc;
		if (p->state==Running)
			p->time[p->insyscall]++;
	}
	nrun = (nrdy+nrun)*1000;
	MACHP(0)->load = (MACHP(0)->load*19+nrun)/20;

	if(u && p && p->state==Running){
		/*
		 *  preemption
		 */
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if((ur->cs&0xffff) == UESEL){
			spllo();		/* in case we fault */
			(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);	/* profiling clock */
			splhi();
		}
	}

	/* last because it goes spllo() */
	mouseclock();
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

X86type x86type[] =
{
	/* from the cpuid instruction */
	{ 4,	0,	22,	"Intel486DX", },
	{ 4,	1,	22,	"Intel486DX", },
	{ 4,	2,	22,	"Intel486SX", },
	{ 4,	3,	22,	"Intel486DX2", },
	{ 4,	4,	22,	"Intel486DX2", },
	{ 4,	5,	22,	"Intel486SL", },
	{ 4,	8,	22,	"IntelDX4", },
	{ 5,	1,	23,	"Pentium510", },
	{ 5,	2,	23,	"Pentium735", },

	/* family defaults */
	{ 3,	-1,	32,	"Intel386", },
	{ 4,	-1,	22,	"Intel486", },
	{ 5,	-1,	23,	"Pentium", },

	/* total default */
	{ -1,	-1,	23,	"unknown", },
};

static X86type	*cputype;

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
	print("CPU is a %d MHz %s (cpuid: ax %lux dx %lux)\n",
		cpumhz, cputype->name, cpuidax, cpuiddx);
}

int
x86(void)
{
	return cputype->family;
}

void
clockinit(void)
{
	int x, y;	/* change in counter */
	int family, model, loops, incr;
	X86type *t;

	/*
	 *  set vector for clock interrupts
	 */
	setvec(Clockvec, clock, 0);

	/*
	 *  figure out what we are
	 */
	x86cpuid(&cpuidax, &cpuiddx);
	family = FAMILY(cpuidax);
	model = MODEL(cpuidax);
	for(t = x86type; t->name; t++)
		if((t->family == family && t->model == model)
		|| (t->family == family && t->model == -1)
		|| (t->family == -1))
			break;
	cputype = t;

	/*
	 *  set clock for 1/HZ seconds
	 */
	outb(Tmode, Load0|Square);
	outb(T0cntr, (Freq/HZ));	/* low byte */
	outb(T0cntr, (Freq/HZ)>>8);	/* high byte */

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
		x = inb(T0cntr);
		x |= inb(T0cntr)<<8;
		aamloop(loops);
		outb(Tmode, Latch0);
		y = inb(T0cntr);
		y |= inb(T0cntr)<<8;
		x -= y;
	
		if(x < 0)
			x += Freq/HZ;

		if(x > Freq/(3*HZ))
			break;
	}

	/*
	 *  counter  goes at twice the frequency, once per transition,
	 *  i.e., twice per square wave
	 */
	x >>= 1;

	/*
 	 *  figure out clock frequency and a loop multiplier for delay().
	 */
	cpufreq = loops*((t->aalcycles*Freq)/x);
	loopconst = (cpufreq/1000)/t->aalcycles;	/* AAM+LOOP's for 1 ms */

	/*
	 *  add in possible .2% error and convert to MHz
	 */
	cpumhz = (cpufreq + cpufreq/500)/1000000;
}
