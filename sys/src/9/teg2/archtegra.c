/*
 * nvidia tegra 2 architecture-specific stuff
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
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
};

typedef struct Clkrst Clkrst;
typedef struct Diag Diag;
typedef struct Flow Flow;
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
	ulong	supcclkdiv;		/* super cclk divider */
	ulong	_pad1;
	ulong	supsclkdiv;		/* super sclk divider */

	uchar	_pad4[0x4c-0x30];
	ulong	clkcpu;

	uchar	_pad1[0xe0-0x50];
	ulong	pllxbase;		/* pllx controls CPU clock speed */
	ulong	pllxmisc;
	ulong	pllebase;		/* plle is dedicated to pcie */
	ulong	pllemisc;

	uchar	_pad2[0x340-0xf0];
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

	/* clkcpu bits */
	Cpu1stop =	1<<9,
	Cpu0stop =	1<<8,

	/* cpu* bits */
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
	ulong	usbdebdel;	/* usb de-bounce delay */
	ulong	usbao;
	ulong	cryptoop;
	ulong	pllpwb0ovr;
	ulong	scratch24[42-24+1];
	ulong	boundoutmirr[3];
	ulong	sys33ven;
	ulong	boundoutmirracc;
	ulong	gate;
};

enum {
	/* toggle bits */
	Start	= 1<<8,
	/* partition ids */
	Partpcie= 3,
	Partl2	= 4,
};

struct Scu {
	ulong	ctl;
	ulong	cfg;			/* ro */
	ulong	cpupwrsts;
	ulong	inval;

	uchar	_pad0[0x40-0x10];
	ulong	filtstart;
	ulong	filtend;

	uchar	_pad1[0x50-0x48];
	ulong	accctl;			/* initially 0 */
	ulong	nsaccctl;
};

enum {
	/* ctl bits */
	Scuenable =	1<<0,
	Filter =	1<<1,
	Scuparity =	1<<2,
	Specfill =	1<<3,		/* only for PL310 */
	Allport0 =	1<<4,
	Standby =	1<<5,
	Icstandby =	1<<6,
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

struct Diag {
	Cacheline c0;
	Lock;
	long	cnt;
	long	sync;
	Cacheline c1;
};

extern ulong testmem;

/*
 * number of cpus available.  contrast with conf.nmach, which is number
 * of running cpus.
 */
int navailcpus;
Isolated l1ptstable;

Soc soc = {
	.clkrst	= 0x60006000,		/* clock & reset signals */
	.power	= 0x7000e400,
	.exceptvec = PHYSEVP,		/* undocumented magic */
	.sema	= 0x60001000,
	.l2cache= PHYSL2BAG,		/* pl310 bag on the side */
	.flow	= 0x60007000,

	/* 4 non-gic controllers */
//	.intr	= { 0x60004000, 0x60004100, 0x60004200, 0x60004300, },

	/* private memory region */
	.scu	= 0x50040000,
	/* we got this address from the `cortex-a series programmer's guide'. */
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

static volatile Diag diag;
static int missed;

void
dumpcpuclks(void)		/* run CPU at full speed */
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	iprint("pllx base %#lux misc %#lux\n", clk->pllxbase, clk->pllxmisc);
	iprint("plle base %#lux misc %#lux\n", clk->pllebase, clk->pllemisc);
	iprint("super cclk divider %#lux\n", clk->supcclkdiv);
	iprint("super sclk divider %#lux\n", clk->supsclkdiv);
}

static char *
devidstr(ulong)
{
	return "ARM Cortex-A9";
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

	r = cpidget();			/* main id register */
	assert((r >> 24) == 'A');
	seprint(buf, buf + size, "Cortex-A9 r%ldp%ld",
		(r >> 20) & MASK(4), r & MASK(4));
	return buf;
}

static void
errata(void)
{
	ulong reg, r, p;

	/* apply cortex-a9 errata workarounds */
	r = cpidget();			/* main id register */
	assert((r >> 24) == 'A');
	p = r & MASK(4);		/* minor revision */
	r >>= 20;
	r &= MASK(4);			/* major revision */

	/* this is an undocumented `diagnostic register' that linux knows */
	reg = cprdsc(0, CpDTLB, 0, 1);
	if (r < 2 || r == 2 && p <= 2)
		reg |= 1<<4;			/* 742230 */
	if (r == 2 && p <= 2)
		reg |= 1<<6 | 1<<12 | 1<<22;	/* 743622, 2×742231 */
	if (r < 3)
		reg |= 1<<11;			/* 751472 */
	cpwrsc(0, CpDTLB, 0, 1, reg);
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
	errata();
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
scuon(void)
{
	Scu *scu = (Scu *)soc.scu;

	if (scu->ctl & Scuenable)
		return;
	scu->inval = MASK(16);
	coherence();
	scu->ctl = Scuparity | Scuenable | Specfill;
	coherence();
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
clockson(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	/* enable all by clearing resets */
	clk->rstdevl = clk->rstdevh = clk->rstdevu = 0;
	coherence();
	clk->clkoutl = clk->clkouth = clk->clkoutu = ~0; /* enable all clocks */
	coherence();

	clk->rstsrc = Wdcpurst | Wdcoprst | Wdsysrst | Wdena;
	coherence();
}

/* we could be shutting down ourself (if cpu == m->machno), so take care. */
void
stopcpu(uint cpu)
{
	Flow *flow = (Flow *)soc.flow;
	Clkrst *clk = (Clkrst *)soc.clkrst;

	if (cpu == 0) {
		iprint("stopcpu: may not stop cpu0\n");
		return;
	}

	machoff(cpu);
	lock(&active);
	active.stopped |= 1 << cpu;
	unlock(&active);
	l1cache->wb();

	/* shut down arm7 avp coproc so it can't cause mischief. */
	/* could try watchdog without stopping avp. */
	flow->haltcop = Stop;
	coherence();
	flow->cop = 0;					/* no Cpuenable */
	coherence();
	delay(10);

	assert(cpu < Maxflowcpus);
	*(cpu == 0? &flow->haltcpu0: &flow->haltcpu1) = Stop;
	coherence();
	*(cpu == 0? &flow->cpu0: &flow->cpu1) = 0;	/* no Cpuenable */
	coherence();
	delay(10);

	/* cold reset */
	assert(cpu < Maxcpus);
	clk->cpuset = (Cpu0reset | Cpu0dbgreset | Cpu0dereset) << cpu;
	coherence();
	delay(1);

	l1cache->wb();
}

static void
synccpus(volatile long *cntp, int n)
{
	ainc(cntp);
	while (*cntp < n)
		;
	/* all cpus should now be here */
}

static void
pass1(int pass, volatile Diag *dp)
{
	int i;

	if(m->machno == 0)
		iprint(" %d", pass);
	for (i = 1000*1000; --i > 0; ) {
		ainc(&dp->cnt);
		adec(&dp->cnt);
	}

	synccpus(&dp->sync, navailcpus);
	/* all cpus are now here */

	ilock(dp);
	if(dp->cnt != 0)
		panic("cpu%d: diag: failed w count %ld", m->machno, dp->cnt);
	iunlock(dp);

	synccpus(&dp->sync, 2 * navailcpus);
	/* all cpus are now here */
	adec(&dp->sync);
	adec(&dp->sync);
}

/*
 * try to confirm coherence of l1 caches.
 * assume that all available cpus will be started.
 */
void
l1diag(void)
{
	int pass;
	volatile Diag *dp;

	if (!Debug)
		return;

	l1cache->wb();

	/*
	 * synchronise and print
	 */
	dp = &diag;
	ilock(dp);
	if (m->machno == 0)
		iprint("l1: waiting for %d cpus... ", navailcpus);
	iunlock(dp);

	synccpus(&dp->sync, navailcpus);

	ilock(dp);
	if (m->machno == 0)
		iprint("cache coherency pass");
	iunlock(dp);

	synccpus(&dp->sync, 2 * navailcpus);
	adec(&dp->sync);
	adec(&dp->sync);

	/*
	 * cpus contend
	 */
	for (pass = 0; pass < 3; pass++)
		pass1(pass, dp);

	/*
	 * synchronise and check sanity
	 */
	synccpus(&dp->sync, navailcpus);

	if(dp->sync < navailcpus || dp->sync >= 2 * navailcpus)
		panic("cpu%d: diag: failed w dp->sync %ld", m->machno,
			dp->sync);
	if(dp->cnt != 0)
		panic("cpu%d: diag: failed w dp->cnt %ld", m->machno,
			dp->cnt);

	ilock(dp);
	iprint(" cpu%d ok", m->machno);
	iunlock(dp);

	synccpus(&dp->sync, 2 * navailcpus);
	adec(&dp->sync);
	adec(&dp->sync);
	l1cache->wb();

	/*
	 * all done, print
	 */
	ilock(dp);
	if (m->machno == 0)
		iprint("\n");
	iunlock(dp);
}

static void
unfreeze(uint cpu)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;
	Flow *flow = (Flow *)soc.flow;

	assert(cpu < Maxcpus);

	clk->clkcpu &= ~(Cpu0stop << cpu);
	coherence();
	/* out of reset */
	clk->cpuclr = (Cpu0reset | Cpu0wdreset | Cpu0dbgreset | Cpu0dereset) <<
		cpu;
	coherence();

	assert(cpu < Maxflowcpus);
	*(cpu == 0? &flow->cpu0: &flow->cpu1) = 0;
	coherence();
	*(cpu == 0? &flow->haltcpu0: &flow->haltcpu1) = 0; /* normal operat'n */
	coherence();
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
	ulong *evp = (ulong *)soc.exceptvec;	/* magic */

	r = 0;
	if (getncpus() < 2 || cpu == m->machno ||
	    cpu >= MAXMACH || cpu >= navailcpus)
		return -1;

	oldvec = *evp;
	l1cache->wb();			/* start next cpu w same view of ram */
	*evp = rstaddr = PADDR(_vrst);	/* will start cpu executing at _vrst */
	coherence();
	l1cache->wb();
	unfreeze(cpu);

	for (i = 2000; i > 0 && *evp == rstaddr; i--)
		delay(1);
	if (i <= 0 || *evp != cpu) {
		iprint("cpu%d: didn't start!\n", cpu);
		stopcpu(cpu);		/* make sure it's stopped */
		r = -1;
	}
	*evp = oldvec;
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
		iprint("cpu%d: debug enable reg %#lux\n", m->machno, db);
}

ulong
smpon(void)
{
	ulong aux;

	/* cortex-a9 model-specific configuration */
	aux = getauxctl();
	putauxctl(aux | CpACsmp | CpACmaintbcast);
	return aux;
}

void
cortexa9cachecfg(void)
{
	/* cortex-a9 model-specific configuration */
	putauxctl(getauxctl() | CpACparity | CpAClwr0line | CpACl2pref);
}

/*
 * called on a cpu other than 0 from cpureset in l.s,
 * from _vrst in lexception.s.
 * mmu and l1 (and system-wide l2) caches and coherency (smpon) are on,
 * but interrupts are disabled.
 * our mmu is using an exact copy of cpu0's l1 page table
 * as it was after userinit ran.
 */
void
cpustart(void)
{
	int ms;
	ulong *evp;
	Power *pwr;

	up = nil;
	if (active.machs & (1<<m->machno)) {
		serialputc('?');
		serialputc('r');
		panic("cpu%d: resetting after start", m->machno);
	}
	assert(m->machno != 0);

	errata();
	cortexa9cachecfg();
	memdiag(&testmem);

	machinit();			/* bumps nmach, adds bit to machs */
	machoff(m->machno);		/* not ready to go yet */

	/* clock signals and scu are system-wide and already on */
	clockshutdown();		/* kill any watch-dog timer */

	trapinit();
	clockinit();			/* sets loop delay */
	timersinit();
	cpuidprint();

	/*
	 * notify cpu0 that we're up so it can proceed to l1diag.
	 */
	evp = (ulong *)soc.exceptvec;	/* magic */
	*evp = m->machno;
	coherence();

	l1diag();		/* contend with other cpus to verify sanity */

	/*
	 * pwr->noiopwr == 0
	 * pwr->detect == 0x1ff (default, all disabled)
	 */
	pwr = (Power *)soc.power;
	assert(pwr->gatests == MASK(7)); /* everything has power */

	/*
	 * 8169 has to initialise before we get past this, thus cpu0
	 * has to schedule processes first.
	 */
	if (Debug)
		iprint("cpu%d: waiting for 8169\n", m->machno);
	for (ms = 0; !l1ptstable.word && ms < 5000; ms += 10) {
		delay(10);
		cachedinvse(&l1ptstable.word, sizeof l1ptstable.word);
	}
	if (!l1ptstable.word)
		iprint("cpu%d: 8169 unreasonably slow; proceeding\n", m->machno);
	/* now safe to copy cpu0's l1 pt in mmuinit */

	mmuinit();			/* update our l1 pt from cpu0's */
	fpon();
	machon(m->machno);		/* now ready to go and be scheduled */

	if (Debug)
		iprint("cpu%d: scheding\n", m->machno);
	schedinit();
	panic("cpu%d: schedinit returned", m->machno);
}

/* mainly used to break out of wfi */
void
sgintr(Ureg *ureg, void *)
{
	iprint("cpu%d: got sgi\n", m->machno);
	/* try to prod cpu1 into life when it gets stuck */
	if (m->machno != 0)
		clockprod(ureg);
}

void
archreset(void)
{
	static int beenhere;

	if (beenhere)
		return;
	beenhere = 1;

	/* conservative temporary values until archconfinit runs */
	m->cpuhz = 1000 * Mhz;			/* trimslice speed */
	m->delayloop = m->cpuhz/2000;		/* initial estimate */

	prcachecfg();

	clockson();
	/* all partitions were powered up by u-boot, so needn't do anything */
	archconfinit();
//	resetusb();
	fpon();

	if (irqtooearly)
		panic("archreset: too early for irqenable");
	irqenable(Cpu0irq, sgintr, nil, "cpu0");
	irqenable(Cpu1irq, sgintr, nil, "cpu1");
	/* ... */
}

void
archreboot(void)
{
	Clkrst *clk = (Clkrst *)soc.clkrst;

	assert(m->machno == 0);
	iprint("archreboot: reset!\n");
	delay(20);

	clk->rstdevl |= Sysreset;
	coherence();
	delay(500);

	/* shouldn't get here */
	splhi();
	iprint("awaiting reset");
	for(;;) {
		delay(1000);
		print(".");
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
		iprint("address zero for %s\n", name);
		return;
	}
	if (probeaddr(addr) >= 0)
		return;
	missed++;
	if (firstmiss) {
		iprint("missing:");
		firstmiss = 0;
	} else
		iprint(",\n\t");
	iprint(" %s at %#lux", name, addr);
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
		iprint("\n");
	delay(10);
}

void
archflashwp(Flash*, int)
{
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
