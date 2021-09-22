/*
 * nvidia tegra 2 architecture-specific stuff:
 * clocks, reset, scu, power, etc.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

#include "../port/netif.h"
#include "etherif.h"
#include "../port/flashif.h"

#include "../port/usb.h"
#include "../port/portusbehci.h"
#include "usbehci.h"

enum {
	/* hardware limits imposed by register contents or layouts */
	Maxcpus		= 4,
	Maxflowcpus	= 2,

	Debug	= 0,
	/* ms to wait for 8169 to signal cpu1 that L1 pt is stable */
	L1wait	= 15*1000,
	Cpuwait	= 10*1000,
};

typedef struct Clkrst Clkrst;
typedef struct Flow Flow;
typedef struct Isolated Isolated;
typedef struct Scu Scu;
typedef struct Power Power;

struct Clkrst {
	ulong	rstsrc;
	ulong	rstdevl;
	ulong	rstdevh;
	ulong	rstdevu;

	ulong	clkoutl;
	ulong	clkouth;
	ulong	clkoutu;

	uchar	_pad0[0x24-0x1c];
	ulong	supcclkdiv;		/* super cclk divider (cpu) */
	ulong	_pad1;
	ulong	supsclkdiv;		/* super sclk divider (avp) */

	uchar	_pad4[0x4c-0x30];
	ulong	clkcpu;

	uchar	_pad1[0xe0-0x50];
	ulong	pllxbase;		/* pllx controls CPU clock speed */
	ulong	pllxmisc;
	ulong	pllebase;		/* plle is dedicated to pcie */
	ulong	pllemisc;

	uchar	_pad2[0x340-0xf0];
	/* these don't seem to read back reliably from the other cpu */
	ulong	cpuset;
	ulong	cpuclr;
};

enum {
	/* rstsrc bits */
	Wdcpurst =	1<<0,
	Wdcoprst =	1<<1,
	Wdsysrst =	1<<2,
	Wdsel =		1<<4,		/* tmr1 or tmr2? */
	Wdena =		1<<5,

	/* devl bits */
	Sysreset =	1<<2,

	/* devh bits */
	Apbdmarst=	1<<2,
	Ahbdmarst=	1<<1,

	/* clkcpu bits */
	Cpu1stop =	1<<9,
	Cpu0stop =	1<<8,

	/* cpu(set|clr) bits */
	Cpupresetdbg =	1<<30,
	Cpuscureset =	1<<29,
	Cpuperiphreset=	1<<28,
	Cpu1dbgreset =	1<<13,
	Cpu0dbgreset =	1<<12,
	Cpu1wdreset =	1<<9,
	Cpu0wdreset =	1<<8,
	Cpu1dereset =	1<<5,
	Cpu0dereset =	1<<4,
	Cpu1reset =	1<<1,
	Cpu0reset =	1<<0,
};

struct Power {
	ulong	ctl;			/* mainly for rtc clock signals */
	ulong	secregdis;
	ulong	swrst;

	ulong	wakevmask;
	ulong	waklvl;
	ulong	waksts;
	ulong	swwaksts;

	ulong	dpdpadsovr;		/* deep power down pads override */
	ulong	dpdsample;
	ulong	dpden;

	ulong	gatetimroff;
	ulong	gatetimron;
	ulong	toggle;
	ulong	unclamp;
	ulong	gatests;		/* ro */

	ulong	goodtmr;
	ulong	blinktmr;

	ulong	noiopwr;
	ulong	detect;
	ulong	detlatch;

	ulong	scratch[24];
	ulong	secscratch[6];

	ulong	cpupwrgoodtmr;
	ulong	cpupwrofftmr;

	ulong	pgmask[2];

	ulong	autowaklvl;
	ulong	autowaklvlmask;
	ulong	wakdelay;

	ulong	detval;
	ulong	ddr;
	ulong	usbdebdel;		/* usb de-bounce delay */
	ulong	usbao;
	ulong	cryptoop;
	ulong	pllpwb0ovr;
	ulong	scratch24[42-24+1];	/* guaranteed to survive deep sleep */
	ulong	boundoutmirr[3];
	ulong	sys33ven;
	ulong	boundoutmirracc;
	ulong	gate;
};

enum {
	/* toggle bits */
	Start	= 1<<8,
	/* partition ids */
	Partcpu	= 0,
	Partpcie= 3,			/* says the manual; correct? */
	Partl2	= 5,

	/* unclamp bits */
	Unccpu	= 1<<0,
	Uncpcie	= 1<<4,
	Uncl2	= 1<<5,
};

struct Scu {
	ulong	ctl;
	ulong	cfg;			/* ro */
	ulong	cpupwrsts;
	ulong	inval;

	uchar	_pad0[0x30-0x10];
	ulong	undoc;			/* erratum 764369 */

	uchar	_pad0[0x40-0x34];
	ulong	filtstart;
	ulong	filtend;

	uchar	_pad1[0x50-0x48];
	ulong	accctl;			/* initially 0 */
	ulong	nsaccctl;
};

enum {
	/* ctl bits */
	Scuenable =	1<<0,
	Filter =	1<<1,		/* address filtering enable */
	Scuparity =	1<<2,
	Specfill =	1<<3,		/* only for PL310 */
	/*
	 * force all requests from acp or cpus with AxCACHE=DV (Device) to be
	 * issued on axi master port M0 enable.
	 */
	Allport0 =	1<<4,
	Standby =	1<<5,
	Icstandby =	1<<6,		/* intr ctlr standby enable */

	/* cpupwrsts values */
	Pwrnormal =	0,
	Pwrreserved,
	Pwrdormant,
	Pwroff,
};

struct Flow {
	ulong	haltcpu0;
	ulong	haltcop;
	ulong	cpu0;
	ulong	cop;
	ulong	xrq;
	ulong	haltcpu1;
	ulong	cpu1;
};

enum {
	/* haltcpu* bits */
	Stop =	2<<29,

	/* cpu* bits */
	Event =			1<<14,	/* w1c */
	Waitwfebitsshift =	4,
	Waitwfebitsmask =	MASK(2),
	Eventenable =		1<<1,
	Cpuenable =		1<<0,
};

typedef uchar Cacheline[CACHELINESZ];

/* a word guaranteed to be in its own cache line */
struct Isolated {
	Cacheline c0;
	ulong	word;
	Cacheline c1;
};

/*
 * number of cpus available.  contrast with conf.nmach, which is number
 * of running cpus.
 */
int navailcpus;
static volatile Isolated l1ptstable;
static Lock shadlock;
static ulong shadcpuset = 1<<1;		/* shadow of clk->cpuset */

Soc soc = {
	.clkrst	= PHYSCLKRST,		/* clock & reset signals */
	.power	= 0x7000e400,
	.exceptvec = PHYSEVP,		/* undocumented magic */
	.sema	= 0x60001000,
	.l2cache= PHYSL2BAG,		/* pl310 bag on the side */
	.flow	= 0x60007000,		/* mostly unused flow controller */

	/* 4 unused non-gic interrupt controllers */
//	.intr	= { 0x60004000, 0x60004100, 0x60004200, 0x60004300, },

	/* private memory region; see `cortex-a series programmer's guide'. */
	.scu	= PHYSSCU,		/* also in cbar reg (periphbase) */
	/* could compute these from .scu in scuon() */
	.intr	= 0x50040100,		/* per-cpu interface */
	.glbtmr	= 0x50040200,
	.loctmr	= 0x50040600,
	.intrdist=0x50041000,

	.uart	= { 0x70006000, 0x70006040,
		    0x70006200, 0x70006300, 0x70006400, },

	.rtc	= 0x7000e000,
	.tmr	= { 0x60005000, 0x60005008, 0x60005050, 0x60005058, },
	.µs	= 0x60005010,

	.pci	= 0x80000000,
	.ether	= 0xa0024000,

	.nand	= 0x70008000,
	.nor	= 0x70009000,		/* also VIRTNOR */

	.ehci	= P2VAHB(0xc5000000),	/* 1st of 3 */
	.ide	= P2VAHB(0xc3000000),

	.gpio	= { 0x6000d000, 0x6000d080, 0x6000d100, 0x6000d180,
			    0x6000d200, 0x6000d280, 0x6000d300, },
	.spi	= { 0x7000d400, 0x7000d600, 0x7000d800, 0x7000da00, },
 	.twsi	= 0x7000c000,
	.mmc	= { P2VAHB(0xc8000000), P2VAHB(0xc8000200),
		    P2VAHB(0xc8000400), P2VAHB(0xc8000600), },
};

static int missed;

int
cmpswap(long *addr, long old, long new)
{
	return cas((int *)addr, old, new);
}

void
dumpcpuclks(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	print("pllx base %#lux misc %#lux\n", clk->pllxbase, clk->pllxmisc);
	print("plle base %#lux misc %#lux\n", clk->pllebase, clk->pllemisc);
	print("super cclk divider %#lux\n", clk->supcclkdiv);
	print("super sclk divider %#lux\n", clk->supsclkdiv);
}

void
archtegralink(void)
{
}

/* convert AddrDevid register to a string in buf and return buf */
char *
cputype2name(char *buf, int size)
{
	ulong r;

	if (conf.cpurev == 0 && conf.cpupart == 0) {
		r = cpidget();			/* main id register */
		assert((r >> 24) == 'A');
		conf.cpupart = r & MASK(4);	/* minor revision */
		r >>= 20;
		r &= MASK(4);			/* major revision */
		conf.cpurev = r;
	}
	seprint(buf, buf + size, "Cortex-A9 r%dp%d", conf.cpurev, conf.cpupart);
	return buf;
}

enum {					/* undocumented diagnostic bits */
	Dmbisdsb	= 1<<4,		/* treat DMB as DSB */
	Nofastlookup	= 1<<6,
	Nointrmaint	= 1<<11,   /* make cp15 maint. ops. uninterruptible */
	Nodirevict	= 1<<21,	/* no direct eviction */
	Nowrallocwait	= 1<<22,
	Nordalloc	= 1<<23,
};

/*
 * apply cortex-a9 errata workarounds.
 * trimslice a9 is r1p0.
 */
void
errata(void)
{
	ulong reg, r, p;

	r = cpidget();			/* main id register */
	if ((r >> 24) != 'A')
		return;			/* non-arm cpu */
	conf.cpupart = p = r & MASK(4);	/* minor revision */
	r >>= 20;
	r &= MASK(4);			/* major revision */
	conf.cpurev = r;

	/*
	 * this is referred to as an `undocumented diagnostic register'
	 * in the errata.
	 */
	reg = cprdsc(0, CpDTLB, CpDTLBmisc, CpDTLBdiag);
	/* errata 742230, 794072: dmb might be buggy */
	reg |= Dmbisdsb;
	/* erratum 743622: faulty store buffer leads to corruption */
	/* erratum 742231: bad hazard handling in the scu leads to corruption */
	if (r == 2 && p <= 2)
		reg |= Nofastlookup | 1<<12;
	/* erratum 751472: interrupted ICIALLUIS may not complete */
	if (r == 1)
		reg |= Nointrmaint;
	/*
	 * erratum 761320: full cache line writes to same mem region from
	 * 2 cpus might deadlock the cpu.
	 */
	reg |= Nodirevict | Nowrallocwait;
	cpwrsc(0, CpDTLB, CpDTLBmisc, CpDTLBdiag, reg);

	putauxctl(CpACparity);
}

void
dumpscustate(void)
{
	Scu *scu = (Scu *)soc.scu;

	print("cpu%d scu: accctl %#lux\n", m->machno, scu->accctl);
	print("cpu%d scu: smp cpu bit map %#lo for %ld cpus; ", m->machno,
		(scu->cfg >> 4) & MASK(4), (scu->cfg & MASK(2)) + 1);
	print("cpus' power %#lux\n", scu->cpupwrsts);
}

void
archconfinit(void)
{
	char *p;
	ulong hz;

	assert(m != nil);
	m->cpuhz = 1000 * Mhz;			/* trimslice speed */
	p = getconf("*cpumhz");
	if (p) {
		hz = atoi(p) * Mhz;
		if (hz >= 100*Mhz && hz <= 3600UL*Mhz)
			m->cpuhz = hz;
	}
	m->delayloop = m->cpuhz/2000;		/* initial estimate */
}

int
archether(unsigned ctlrno, Ether *ether)
{
	switch(ctlrno) {
	case 0:
		ether->type = "rtl8169";		/* pci-e ether */
		ether->ctlrno = ctlrno;
		ether->irq = Pcieirq;			/* non-msi pci-e intr */
		ether->nopt = 0;
		ether->mbps = 1000;
		return 1;
	}
	return -1;
}

/* stop cache coherency */
void
scuoff(void)
{
	Scu *scu;

	scu = (Scu *)soc.scu;
	if (!(scu->ctl & Scuenable))
		return;
	scu->ctl &= Filter; /* leave Filter alone, per arm's recommendation */
	coherence();
}

void
scuinval(void)
{
	Scu *scu;

	scu = (Scu *)soc.scu;
	if (scu->ctl & Scuenable)
		return;
	scu->inval = MASK(Maxcpus*4);
	coherence();
}

void
scuon(void)
{
	Scu *scu;

//	soc.scu = cprdsc(CpDTLBcbar1, CpDTLB, CpDTLBmisc, CpDTLBcbar2);
	scu = (Scu *)soc.scu;
	if (scu->ctl & Scuenable)
		return;
	/* erratum 764369: cache maint by mva may fail on inner shareable mem */
	scu->undoc |= 1;		/* no migratory bit: bit reads as 0 */
	/* leave Filter alone, per arm's recommendation */
	scu->ctl &= ~(Standby | Icstandby | Allport0);
	scu->ctl |= Scuenable | Scuparity | Specfill;
	coherence();
}

/* don't shut down the scu, but tell it that this cpu is shutting down. */
void
tellscushutdown(void)
{
	ulong sts, osts;
	Scu *scu = (Scu *)soc.scu;

	sts = osts = scu->cpupwrsts;
	sts &= ~(MASK(2)  << (m->machno*8));
	sts |= Pwrdormant << (m->machno*8);
	if (sts != osts) {
		scu->cpupwrsts = sts;
		coherence();
	}
}

void
tellscuup(void)
{
	ulong sts, osts;
	Scu *scu = (Scu *)soc.scu;

	sts = osts = scu->cpupwrsts;
	sts &= ~(MASK(2) << (m->machno*8));
	sts |= Pwrnormal << (m->machno*8);
	if (sts != osts) {
		scu->cpupwrsts = sts;
		coherence();
	}
}

int
getncpus(void)
{
	int n;
	char *p;
	Scu *scu;

	if (navailcpus == 0) {
		scu = (Scu *)soc.scu;
		navailcpus = (scu->cfg & MASK(2)) + 1;
		if (navailcpus > MAXMACH)
			navailcpus = MAXMACH;

		p = getconf("*ncpu");
		if (p && *p) {
			n = atoi(p);
			if (n > 0 && n < navailcpus)
				navailcpus = n;
		}
	}
	return navailcpus;
}

void
cpuidprint(void)
{
	char name[64];

	cputype2name(name, sizeof name);
	delay(50);				/* let uart catch up */
	iprint("cpu%d: %lldMHz ARM %s %s-endian\n",
		m->machno, m->cpuhz / Mhz, name,
		getpsr() & PsrBigend? "big": "little");
}

static void
setcpusclkdiv(uchar m, uchar n)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	/* setting cpu super clk div to m/n */
	clk->supcclkdiv = 1<<31 | (m-1)<<8 | (n-1);
	coherence();
	delay(1);
}

void
clockson(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	clk->clkoutl = clk->clkouth = clk->clkoutu = ~0; /* enable all clocks */
	coherence();
	microdelay(20);
	clk->rstdevl = clk->rstdevh = clk->rstdevu = 0; /* deassert resets */
	coherence();
	microdelay(20);

	/* configure watchdog resets */
	clk->rstsrc = Wdcpurst | Wdcoprst | Wdsysrst | Wdena;
	/* again.  paranoia due to nxp erratum 4346. */
	coherence();
	clk->rstsrc = Wdcpurst | Wdcoprst | Wdsysrst | Wdena;
	coherence();

	/* setting cpu super clk div to full speed */
	clk->supcclkdiv = 0;
	coherence();
	delay(1);
}

void
clockenable(int cpu)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	/* start cpu's clock */
	clk->clkcpu &= ~(Cpu0stop << cpu);
	coherence();
}

void
clockdisable(int cpu)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	clk->clkcpu |= Cpu0stop << cpu;
	coherence();
}

/* tidy up before shutting down this cpu */
void
cpucleanup(void)
{
	// intrcpushutdown();		/* let clock intrs in for scheduling */
	if (m->machno != 0)
		watchdogoff();
	cachedwb();			/* flush our dirt */
	// smpcoheroff();
	if (m->machno == 0)
		allcacheswb();
}

/* idle forever */
void
cpuwfi(void)
{
	splhi();
	smpcoheroff();
	// tellscushutdown(); /* likely a bad idea; will stop clocks, etc.? */
	if (wfiloop)
		wfiloop();		/* low memory; no return */
	for (;;)
		wfi();
}

/* shut down arm7 avp coproc so it can't cause mischief. */
static void
flowstopavp(void)
{
	Flow *flow = (Flow *)soc.flow;

	flow->haltcop = Stop;
	coherence();
	flow->cop = 0;					/* no Cpuenable */
	coherence();
}

int
cpusinreset(void)
{
	return shadcpuset & MASK(MAXMACH);
}

int
iscpureset(uint cpu)
{
	return shadcpuset & (Cpu0reset << cpu);
}

static void
pushshadow(void)
{
	cachedwbse(&shadcpuset, sizeof shadcpuset);
	allcacheswbse(&shadcpuset, sizeof shadcpuset);
	l2pl310sync();		/* erratum 769419 */
}

static void
setshadcpu(ulong bits)
{
	if (m->printok)
		ilock(&shadlock);
	shadcpuset |= bits;
	if (m->printok)
		iunlock(&shadlock);
	pushshadow();
}

static void
clrshadcpu(ulong bits)
{
	if (m->printok)
		ilock(&shadlock);
	shadcpuset &= ~bits;
	if (m->printok)
		iunlock(&shadlock);
	pushshadow();
}

/* put cpu into reset.  it takes about 1 µs. to take effect. */
void
resetcpu(uint cpu)
{
	int i;
	ulong cpubit;
	Clkrst *clk = (Clkrst *)soc.clkrst;

	if (cpu == 0) {
		iprint("may not reset cpu%d\n", cpu);
		return;
	}
	cpubit = Cpu0reset << cpu;
	if (shadcpuset & cpubit)
		return;			/* already in reset */
	delay(2);
	if (cpu == m->machno)
		setshadcpu((Cpu0reset | Cpu0wdreset) << cpu);	/* optimism */
	for (i = 1000; !(clk->cpuset & cpubit) && i > 0; i--) {
		/* whack it again! */
		clk->cpuset = (Cpu0reset | Cpu0wdreset) << cpu;
		coherence();
		microdelay(2);
	}
	if (clk->cpuset & cpubit) {
		setshadcpu((Cpu0reset | Cpu0wdreset) << cpu);	/* optimism */
	} else
		iprint("bloody cpu%d NOT in reset\n", cpu);
	delay(2);
}

ulong
ckcpurst(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	if (!(shadcpuset & Cpu1reset)) {
		iprint("cpuset says cpu1 is running: %#lux\n", clk->cpuset);
		return 0;
	}
	return 1;
}

static void
stopcpuclock(uint cpu)
{
	delay(20);
	clockdisable(cpu);
	delay(10);
}

/*
 * we could be shutting down ourself (if cpu == m->machno), so take care.
 * this works best if up != nil (i.e., called from a process context).
 */
void
stopcpu(uint cpu)
{
	int s;

	if (cpu == 0) {
		print("stopcpu: may not stop cpu0\n");
		return;
	}
	assert(cpu < Maxcpus);
	s = splhi();
	machoff(cpu);
	if (iscpureset(cpu)) {
		splx(s);
		return;			/* already in reset */
	}

	/* if we're shutting ourself down, tidy up before reset */
	if (cpu == m->machno) {
		cpucleanup();
		/* leave coherence & mmu on so resetcpu can update shadcpuset */
	}
	resetcpu(cpu);
	if (cpu == m->machno)
		cpuwfi();	/* wait for the reset to actually happen */
	splx(s);
}

/* unclamp i/o via power ctlr */
void
unclamp(void)
{
	Power *pwr = (Power *)soc.power;

	pwr->unclamp |= Unccpu | Uncpcie | Uncl2;
	coherence();
	delay(4);		/* let i/o signals settle */
}

/* reset various peripherals and the interrupt controllers */
void
periphreset(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	clk->cpuclr = Cpuperiphreset;
	l2pl310sync();			/* erratum 769419 */
	delay(1);

	clk->cpuset = Cpuperiphreset;
	l2pl310sync();			/* erratum 769419 */
	delay(10);

	clk->cpuclr = Cpuperiphreset;
	l2pl310sync();			/* erratum 769419 */
	delay(200);
}

static void
unfreeze(uint cpu)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	if (cpu == 0) {
		print("unfreeze: may not unfreeze cpu0\n");
		return;
	}

	assert(cpu < Maxcpus);

	/* ensure cpu is in reset */
	resetcpu(cpu);

	clockenable(cpu);
	delay(10);

	unclamp();

	/* take cpu out of reset; should start it at _vrst */
	clrshadcpu((Cpu0reset|Cpu0wdreset|Cpu0dbgreset|Cpu0dereset) << cpu);
	clk->cpuclr = (Cpu0reset | Cpu0wdreset | Cpu0dbgreset | Cpu0dereset) <<
		cpu | Cpupresetdbg | Cpuscureset | Cpuperiphreset;
	coherence();
	delay(10);
}

/*
 * this is all a bit magic.  the soc.exceptvec register is effectively
 * undocumented.  we had to look at linux and experiment, alas.  this is the
 * sort of thing that should be standardised as part of the cortex mpcore spec.
 * even intel document their equivalent procedure.
 */
int
startcpu(uint cpu)
{
	int i, r;
	ulong oldvec, rstaddr;
	volatile ulong *evp = (ulong *)soc.exceptvec;	/* magic */

	r = 0;
	if (getncpus() < 2 || cpu == m->machno ||
	    cpu >= MAXMACH || cpu >= navailcpus)
		return -1;

	stopcpu(cpu);			/* make sure it's stopped */
	oldvec = *evp;
	/* a cpu coming out of reset will start executing at _vrst */
	*evp = rstaddr = PADDR(_vrst);
	cachedwbinv();			/* start next cpu w same view of ram */

	/*
	 * since the cpu being started can't yet participate in L1 coherency,
	 * we manually flush or invalidate our caches.
	 */
	allcacheswbinvse(evp, sizeof *evp);
	unfreeze(cpu);

	for (i = Cpuwait/50; i > 0 && *evp == rstaddr; i--) {
		delay(50);
		cachedinvse(evp, sizeof *evp);
	}
	if (i <= 0 || *evp != cpu) {
		print("cpu%d: didn't start after %d s.!\n", cpu, Cpuwait/1000);
		stopcpu(cpu);		/* make sure it's stopped */
		r = -1;
	}
	*evp = oldvec;
	coherence();
	return r;
}

static void
cksecure(void)
{
	ulong db;
	extern ulong getdebug(void);

	if (getscr() & 1)
		panic("cpu%d: running non-secure", m->machno);
	db = getdebug();
	if (db)
		print("cpu%d: debug enable reg %#lux\n", m->machno, db);
}

enum {
	Cachecoherent	= CpACsmpcoher | CpACmaintbcast,
};

ulong
smpcoheroff(void)
{
	int s;
	ulong aux;

	s = splhi();
	aux = getauxctl();
	if (aux & Cachecoherent)
		/* cortex-a9 model-specific configuration */
		putauxctl(aux & ~Cachecoherent);
	splx(s);
	return aux;
}

/*
 * set SMP and FW bits in aux ctl.
 */
ulong
smpcoheron(void)
{
	int s;
	ulong aux;

	s = splhi();
	aux = getauxctl();
	if ((aux & Cachecoherent) != Cachecoherent)
		/* cortex-a9 model-specific configuration */
		putauxctl(aux | Cachecoherent);
	splx(s);
	return aux;
}

/* cortex-a9 model-specific cache configuration */
void
cortexa9cachecfg(void)
{
	ulong aux;

	/*
	 * the l2 cache must be enabled before setting CpAClwr0line.
	 *
	 * errata 751473, 719332: prefetcher can deadlock or corrupt.
	 * fix: clear CpACl?pref.
	 * ignore erratum 719331, which says it's okay.
	 */
	aux = getauxctl() | CpACparity | CpAClwr0line;
	if (conf.cpurev < 3)
		aux &= ~(CpACl1pref | CpACl2pref);
	putauxctl(aux);
}

/* signal secondary cpus that l1 ptes are stable */
void
signall1ptstable(void)
{
	l1ptstable.word = 1;
	coherence();
	cachedwbse(&l1ptstable.word, sizeof l1ptstable.word);
}

void
awaitstablel1pt(void)
{
	int ms;

	if (Debug)
		print("cpu%d: waiting for 8169\n", m->machno);
	for (ms = 0; !l1ptstable.word && ms < L1wait; ms += 50)
		delay(50);
	if (!l1ptstable.word)
		print("cpu%d: 8169 hasn't signaled L1 pt stable after %d ms; "
			"proceeding\n", m->machno, L1wait);
}

void
poweron(void)
{
	Power *pwr;

	/*
	 * pwr->noiopwr == 0, pwr->detect == 0x1ff (default, all disabled)
	 */
	unclamp();
	pwr = (Power *)soc.power;
	assert((pwr->gatests & (Unccpu | Uncpcie | Uncl2)) ==
		(Unccpu | Uncpcie | Uncl2));
	tellscuup();
	clockson();
}

int
vfyintrs(void)
{
	int s, oldticks, oldintr;

	oldintr = m->intr;
	oldticks = m->ticks;
	// iprint("cpu%d: spllo...", m->machno); delay(2);
	s = spllo();
	delay(200);			/* expect interrupts here */
	splx(s);
	// iprint("cpu%d back.\n", m->machno);
	if (m->intr == oldintr) {
		iprint("cpu%d: no interrupts; taking cpu offline\n",
			m->machno);
		return -1;
	}
	if (m->ticks == oldticks) {
		iprint("cpu%d: clock not interrupting; taking cpu offline\n",
			m->machno);
		return -1;
	}
	return 0;
}

void
confirmup(void)
{
	ulong *evp;

	evp = (ulong *)soc.exceptvec;	/* magic */
	if (vfyintrs() < 0) {
		*evp = 0;		/* notify cpu0 that we failed. */
		coherence();
		stopcpu(m->machno);
		cpuwfi();		/* no return */
	}
	*evp = m->machno;  /* notify cpu0 that we're up so it can proceed */
	coherence();
}


/* mainly used to break out of wfi */
void
sgintr(Ureg *, void *vp)
{
	ulong *sgicntp = vp;

	++*sgicntp;
}

ulong sgicnt[MAXMACH];

void
sgienable(void)
{
	irqenable(Cpu0irq, sgintr, (void *)&sgicnt[0], "cpu0");
	irqenable(Cpu1irq, sgintr, (void *)&sgicnt[1], "cpu1");
	/* ... */
}

void
archreset(void)
{
	Power *pwr;
	static int beenhere;

	if (beenhere)
		return;
	beenhere = 1;

	/* conservative temporary values until archconfinit runs */
	m->cpuhz = 1000 * Mhz;			/* trimslice speed */
	m->delayloop = m->cpuhz/2000;		/* initial estimate */

	clockson();

	/*
	 * normally, all partitions are powered up by u-boot,
	 * so we needn't do anything.
	 */
	unclamp();
	pwr = (Power *)soc.power;
	assert((pwr->gatests & (Unccpu | Uncpcie | Uncl2)) ==
		(Unccpu | Uncpcie | Uncl2));

	flowstopavp();
	archconfinit();
	prcachecfg();
	fpon();				/* initialise */
	fpoff();			/* cause a fault on first use */
}

void
archreboot(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	assert(m->machno == 0);
	iprint("archreboot: reset!\n");
	delay(20);

	// smpcoheroff();
	// calling tellscushutdown() is likely a bad idea; will stop clocks, &c.
	scuoff();
	restartwatchdog();

	clk->rstdevl |= Sysreset;
	coherence();
	delay(100);

	/* shouldn't get here */
	splhi();
	iprint("awaiting reset");
	for(;;) {
		delay(1000);
		iprint(".");
	}
}

void
kbdinit(void)
{
}

static void
missing(ulong addr, char *name)
{
	static int firstmiss = 1;

	if (addr == 0) {
		print("address zero for %s\n", name);
		return;
	}
	if (probeaddr(addr) >= 0)
		return;
	missed++;
	if (firstmiss) {
		print("missing:");
		firstmiss = 0;
	} else
		print(",\n\t");
	print(" %s at %#lux", name, addr);
}

/* verify that all the necessary device registers are accessible */
void
chkmissing(void)
{
	delay(10);
	missing(KZERO, "dram");
	missing(soc.intr, "intr ctlr");
	missing(soc.intrdist, "intr distrib");
	missing(soc.tmr[0], "tegra timer1");
	missing(soc.uart[0], "console uart");
	missing(soc.pci, "pcie");
	missing(soc.ether, "ether8169");
	missing(soc.µs, "µs counter");
	if (missed)
		print("\n");
	delay(10);
}

/*
 * for ../port/devflash.c:/^flashreset
 * retrieve flash type, virtual base and length and return 0;
 * return -1 on error (no flash)
 */
int
archflashreset(int bank, Flash *f)
{
	if(bank != 0)
		return -1;
panic("archflashreset: rewrite for nor & nand flash on ts");
	/*
	 * this is set up for the igepv2 board.
	 */
	f->type = "onenand";
	f->addr = (void*)VIRTNOR;		/* mapped here by archreset */
	f->size = 0;				/* done by probe */
	f->width = 1;
	f->interleave = 0;
	return 0;
}
