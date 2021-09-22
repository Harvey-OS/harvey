/*
 * simulate independent hardware watch-dog timer
 * using local cpu timers and NMIs, one watch-dog per system.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "mp.h"

typedef struct Wd Wd;
struct Wd {
	Lock;
	int	model;
	int	inuse;
	uint	ticks;
};

static Wd x86wd;

enum {
	P6		= 0,			/* Pentium Pro/II/III */
	P4		= 1,			/* P4 */
	K6		= 2,			/* Athlon */
	K8		= 3,			/* AMD64 */

	Twogigs		= 1ul << 31,

	Perfcnten	= 1<<22,		/* enable counters */
	Perfintren	= 1<<20,
	Perfos		= 1<<17,
	Perfuser	= 1<<16,
};

/*
 * return an interval in cycles of about a second, or as long as
 * will fit in 31 bits.
 */
static long
interval(void)
{
	if (m->cpuhz > Twogigs - 1)
		return Twogigs - 1;
	else
		return m->cpuhz;
}

static void
x86wdenable(void)
{
	Wd *wd;
	vlong r, t;
	int i, fam, model;
	ulong evntsel;

	wd = &x86wd;
	ilock(wd);
	if(wd->inuse){
		iunlock(wd);
		error(Einuse);
	}
	iunlock(wd);

	/*
	 * keep this process on cpu 0 so we always see the same timers
	 * and so that this will work even if all other cpus are shut down.
	 */
	runoncpu(0);

	/*
	 * Check the processor is capable of doing performance
	 * monitoring and that it has TSC, RDMSR/WRMSR and a local APIC.
	 */
	model = -1;
	fam = X86FAMILY(conf.cpuidax);
	if(conf.x86type == Amd){
		if(fam == 0x06)
			model = K6;
		else if(fam >= 0x0F)
			model = K8;	/* or K10 or Jaguar */
	}
	else if(conf.x86type == Intel){
		if(fam == 0x06)
			model = P6;
		else if(fam == 0x0F)
			model = P4;
	}
	if(model == -1 ||
	    (m->cpuiddx & (Cpuapic|Cpumsr|Tsc)) != (Cpuapic|Cpumsr|Tsc))
		error(Enodev);

	ilock(wd);
	if(wd->inuse){
		iunlock(wd);
		error(Einuse);
	}
	wd->model = model;
	wd->inuse = 1;
	wd->ticks = 0;

	/*
	 * See the IA-32 Intel Architecture Software
	 * Developer's Manual Volume 3: System Programming Guide,
	 * Chapter 15 (now 18, Performance Monitoring) and the AMD equivalent
	 * (bios & kernel guide; in family 10h, section 3.12)
	 * for what all this bit-whacking means.
	 */
	t = interval();
	switch(model){
	case P6:
		wrmsr(Msrpevsel0, 0);
		wrmsr(Msrpevsel1, 0);
		wrmsr(Msrpmc0, 0);
		wrmsr(Msrpmc1, 0);

		lapicnmienable();

		/* 0x79 is cpu clock unhalted event */
		evntsel = Perfintren|Perfos|Perfuser|0x79;
		wrmsr(Msrpmc0, -t);
		wrmsr(Msrpevsel0, Perfcnten|evntsel);	/* enable counters */
		break;
	case P4:
		rdmsr(Msrmiscen, &r);
		if(!(r & 0x80LL))
			return;

		for(i = 0; i < 18; i++)
			wrmsr(0x300+i, 0);		/* perfctr */
		for(i = 0; i < 18; i++)
			wrmsr(0x360+i, 0);		/* ccr */

		for(i = 0; i < 31; i++)
			wrmsr(0x3A0+i, 0);		/* escr */
		for(i = 0; i < 6; i++)
			wrmsr(0x3C0+i, 0);		/* escr */
		for(i = 0; i < 6; i++)
			wrmsr(0x3C8+i, 0);		/* escr */
		for(i = 0; i < 2; i++)
			wrmsr(0x3E0+i, 0);		/* escr */

		if(!(r & 0x1000LL)){
			for(i = 0; i < 2; i++)
				wrmsr(Msrpebsen+i, 0);
		}

		lapicnmienable();

		wrmsr(0x3B8, 0x7E00000CLL);		/* escr0 */
		r = 0x04FF8000ULL;
		wrmsr(0x36C, r);			/* cccr0 */
		wrmsr(0x30C, -t);
		wrmsr(0x36C, 0x1000LL|r);
		break;
	case K6:
	case K8:
		/*
		 * AMD64 Vol. 2 (System Programming).
		 * PerfEvtSel 0-3, PerfCtr 0-4.
		 */
		for(i = 0; i < 8; i++)
			wrmsr(AMsrpevsel0+i, 0);

		lapicnmienable();

		/* 0x76 is cpu clocks not halted event */
		evntsel = Perfintren|Perfos|Perfuser|0x76;
		wrmsr(AMsrpmc0, -t);
		wrmsr(AMsrpevsel0, Perfcnten|evntsel);	/* enable counters */
		break;
	}
	iunlock(wd);
}

static void
x86wddisable(void)
{
	Wd *wd;

	wd = &x86wd;
	ilock(wd);
	if(!wd->inuse){
		/*
		 * Can't error, called at boot by addwatchdog().
		 */
		iunlock(wd);
		return;
	}
	iunlock(wd);

	runoncpu(0);

	ilock(wd);
	lapicnmidisable();
	switch(wd->model){
	case P6:
		wrmsr(Msrpevsel0, 0);
		break;
	case P4:
		wrmsr(0x36C, 0);			/* cccr0 */
		wrmsr(0x3B8, 0);			/* escr0 */
		break;
	case K6:
	case K8:
		wrmsr(AMsrpevsel0, 0);
		break;
	}
	wd->inuse = 0;
	iunlock(wd);
}

static void
x86wdrestart(void)
{
	Wd *wd;
	vlong r, t;

	runoncpu(0);
	t = interval();

	wd = &x86wd;
	ilock(wd);
	switch(wd->model){
	case P6:
		wrmsr(Msrpmc0, -t);
		break;
	case P4:
		r = 0x04FF8000LL;
		wrmsr(0x36C, r);
		lapicnmienable();
		wrmsr(0x30C, -t);
		wrmsr(0x36C, 0x1000LL|r);
		break;
	case K6:
	case K8:
		wrmsr(AMsrpmc0, -t);
		break;
	}
	wd->ticks++;
	iunlock(wd);
}

void
x86wdstat(char* p, char* ep)
{
	Wd *wd;
	int inuse;
	uint ticks;

	wd = &x86wd;
	ilock(wd);
	inuse = wd->inuse;
	ticks = wd->ticks;
	iunlock(wd);

	if(inuse)
		seprint(p, ep, "enabled %ud restarts\n", ticks);
	else
		seprint(p, ep, "disabled %ud restarts\n", ticks);
}

Watchdog x86watchdog = {
	x86wdenable,
	x86wddisable,
	x86wdrestart,
	x86wdstat,
};

void
x86watchdoglink(void)
{
	addwatchdog(&x86watchdog);
}
