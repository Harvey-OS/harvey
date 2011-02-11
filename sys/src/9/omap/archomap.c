/*
 * omap3530 SoC (e.g. beagleboard) architecture-specific stuff
 *
 * errata: usb port 3 cannot operate in ulpi mode, only serial or
 * ulpi tll mode
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
#include "usbehci.h"

#define FREQSEL(x) ((x) << 4)

typedef struct Cm Cm;
typedef struct Cntrl Cntrl;
typedef struct Gen Gen;
typedef struct Gpio Gpio;
typedef struct L3agent L3agent;
typedef struct L3protreg L3protreg;
typedef struct L3regs L3regs;
typedef struct Prm Prm;
typedef struct Usbotg Usbotg;
typedef struct Usbtll Usbtll;

/* omap3 non-standard usb stuff */
struct Usbotg {
	uchar	faddr;
	uchar	power;
	ushort	intrtx;
	ushort	intrrx;
	ushort	intrtxe;
	ushort	intrrxe;
	uchar	intrusb;
	uchar	intrusbe;
	ushort	frame;
	uchar	index;
	uchar	testmode;

	/* indexed registers follow; ignore for now */
	uchar	_pad0[0x400 - 0x10];

	ulong	otgrev;
	ulong	otgsyscfg;
	ulong	otgsyssts;
	ulong	otgifcsel;	/* interface selection */
	uchar	_pad1[0x414 - 0x410];
	ulong	otgforcestdby;
};

enum {
	/* power bits */
	Hsen		= 1<<5,		/* high-speed enable */

	/* testmode bits */
	Forcehost	= 1<<7,		/* force host (vs peripheral) mode */
	Forcehs		= 1<<4,		/* force high-speed at reset */

	/* otgsyscfg bits */
	Midle		= 1<<12,	/* no standby mode */
	Sidle		= 1<<3,		/* no idle mode */
//	Softreset	= 1<<1,

	/* otgsyssts bits, per sysstatus */
};

struct Usbtll {
	ulong	revision;	/* ro */
	uchar	_pad0[0x10-0x4];
	ulong	sysconfig;
	ulong	sysstatus;	/* ro */

	ulong	irqstatus;
	ulong	irqenable;
};

enum {
	/* sysconfig bits */
	Softreset	= 1<<1,

	/* sysstatus bits */
	Resetdone	= 1<<0,
	/* only in uhh->sysstatus */
	Ehci_resetdone	= 1<<2,
	Ohci_resetdone	= 1<<1,
};

/*
 * an array of these structs is preceded by error_log at 0x20, control,
 * error_clear_single, error_clear_multi.  first struct is at offset 0x48.
 */
struct L3protreg {		/* hw: an L3 protection region */
	uvlong	req_info_perm;
	uvlong	read_perm;
	uvlong	write_perm;
	uvlong	addr_match;	/* ro? write this one last, then flush */
};

// TODO: set these permission bits (e.g., for usb)?
enum {
	Permusbhost	= 1<<9,
	Permusbotg	= 1<<4,
	Permsysdma	= 1<<3,
	Permmpu		= 1<<1,
};

struct L3agent {		/* hw registers */
	uchar	_pad0[0x20];
	uvlong	ctl;
	uvlong	sts;
	uchar	_pad1[0x58 - 0x30];
	uvlong	errlog;
	uvlong	errlogaddr;
};

struct L3regs {
	L3protreg *base;		/* base of array */
	int	upper;			/* index maximum */
	char	*name;
};
L3regs l3regs[] = {
	(L3protreg *)(PHYSL3GPMCPM+0x48), 7, "gpmc",	/* known to be first */
	(L3protreg *)(PHYSL3PMRT+0x48), 1, "rt",	/* l3 config */
	(L3protreg *)(PHYSL3OCTRAM+0x48), 7, "ocm ram",
	(L3protreg *)(PHYSL3OCTROM+0x48), 1, "ocm rom",
	(L3protreg *)(PHYSL3MAD2D+0x48), 7, "mad2d",	/* die-to-die */
	(L3protreg *)(PHYSL3IVA+0x48), 3, "iva2.2",	/* a/v */
};

/*
 * PRM_CLKSEL (0x48306d40) low 3 bits are system clock speed, assuming
 * 	units of MHz: 0 = 12, 1 = 13, 2 = 19.2, 3 = 26, 4 = 38.4, 5 = 16.8
 */

struct Cm {				/* clock management */
	ulong	fclken;			/* ``functional'' clock enable */
	ulong	fclken2;
	ulong	fclken3;
	uchar	_pad0[0x10 - 0xc];

	ulong	iclken;			/* ``interface'' clock enable */
	ulong	iclken2;
	ulong	iclken3;
	uchar	_pad1[0x20 - 0x1c];

	ulong	idlest;			/* idle status */
	ulong	idlest2;
	ulong	idlest3;
	uchar	_pad2[0x30 - 0x2c];

	ulong	autoidle;
	ulong	autoidle2;
	ulong	autoidle3;
	uchar	_pad3[0x40 - 0x3c];

	union {
		ulong	clksel[5];
		struct unused {
			ulong	sleepdep;
			ulong	clkstctrl;
			ulong	clkstst;
		};
		uchar	_pad4[0x70 - 0x40];
	};
	ulong	clkoutctrl;
};

struct Prm {				/* power & reset management */
	uchar	_pad[0x50];
	ulong	rstctrl;
};

struct Gpio {
	ulong	_pad0[4];
	ulong	sysconfig;
	ulong	sysstatus;

	ulong	irqsts1;		/* for mpu */
	ulong	irqen1;
	ulong	wkupen;
	ulong	_pad1;
	ulong	irqsts2;		/* for iva */
	ulong	irqen2;

	ulong	ctrl;

	ulong	oe;
	ulong	datain;
	ulong	dataout;

	ulong	lvldet0;
	ulong	lvldet1;
	ulong	risingdet;
	ulong	fallingdet;

	/* rest are uninteresting */
	ulong	deben;			/* debouncing enable */
	ulong	debtime;
	ulong	_pad2[2];

	ulong	clrirqen1;
	ulong	setirqen1;
	ulong	_pad3[2];

	ulong	clrirqen2;
	ulong	setirqen2;
	ulong	_pad4[2];

	ulong	clrwkupen;
	ulong	setwkupen;
	ulong	_pad5[2];

	ulong	clrdataout;
	ulong	setdataout;
};

enum {
	/* clock enable & idle status bits */
	Wkusimocp	= 1 << 9,	/* SIM card: uses 120MHz clock */
	Wkwdt2		= 1 << 5,	/* wdt2 clock enable bit for wakeup */
	Wkgpio1		= 1 << 3,	/* gpio1 " */
	Wkgpt1		= 1 << 0,	/* gpt1 " */

	Dssl3l4		= 1 << 0,	/* dss l3, l4 i clks */
	Dsstv		= 1 << 2,	/* dss tv f clock */
	Dss2		= 1 << 1,	/* dss clock 2 */
	Dss1		= 1 << 0,	/* dss clock 1 */

	Pergpio6	= 1 << 17,
	Pergpio5	= 1 << 16,
	Pergpio4	= 1 << 15,
	Pergpio3	= 1 << 14,
	Pergpio2	= 1 << 13,
	Perwdt3		= 1 << 12,	/* wdt3 clock enable bit for periphs */
	Peruart3	= 1 << 11,	/* console uart */
	Pergpt9		= 1 << 10,
	Pergpt8		= 1 << 9,
	Pergpt7		= 1 << 8,
	Pergpt6		= 1 << 7,
	Pergpt5		= 1 << 6,
	Pergpt4		= 1 << 5,
	Pergpt3		= 1 << 4,
	Pergpt2		= 1 << 3,	/* gpt2 clock enable bit for periphs */

	Perenable	= Pergpio6 | Pergpio5 | Perwdt3 | Pergpt2 | Peruart3,

	Usbhost2	= 1 << 1,	/* 120MHz clock enable */
	Usbhost1	= 1 << 0,	/* 48MHz clock enable */
	Usbhost		= Usbhost1,	/* iclock enable */
	Usbhostidle	= 1 << 1,
	Usbhoststdby	= 1 << 0,

	Coreusbhsotg	= 1 << 4,	/* usb hs otg enable bit */
	Core3usbtll	= 1 << 2,	/* usb tll enable bit */

	/* core->idlest bits */
	Coreusbhsotgidle = 1 << 5,
	Coreusbhsotgstdby= 1 << 4,

	Dplllock	= 7,

	/* mpu->idlest2 bits */
	Dplllocked	= 1,
	Dpllbypassed	= 0,

	/* wkup->idlest bits */
	Gpio1idle	= 1 << 3,

	/* dss->idlest bits */
	Dssidle		= 1 << 1,

	Gpio1vidmagic	= 1<<24 | 1<<8 | 1<<5,	/* gpio 1 pins for video */
};
enum {
	Rstgs		= 1 << 1,	/* global sw. reset */

	/* fp control regs.  most are read-only */
	Fpsid		= 0,
	Fpscr,				/* rw */
	Mvfr1		= 6,
	Mvfr0,
	Fpexc,				/* rw */
};

/* see ether9221.c for explanation */
enum {
	Ethergpio	= 176,
	Etherchanbit	= 1 << (Ethergpio % 32),
};

/*
 * these shift values are for the Cortex-A8 L1 cache (A=2, L=6) and
 * the Cortex-A8 L2 cache (A=3, L=6).
 * A = log2(# of ways), L = log2(bytes per cache line).
 * see armv7 arch ref p. 1403.
 *
 * #define L1WAYSH 30
 * #define L1SETSH 6
 * #define L2WAYSH 29
 * #define L2SETSH 6
 */
enum {
	/*
	 * cache capabilities.  write-back vs write-through is controlled
	 * by the Buffered bit in PTEs.
	 */
	Cawt	= 1 << 31,
	Cawb	= 1 << 30,
	Cara	= 1 << 29,
	Cawa	= 1 << 28,
};

struct Gen {
	ulong	padconf_off;
	ulong	devconf0;
	uchar	_pad0[0x68 - 8];
	ulong	devconf1;
};

struct Cntrl {
	ulong	_pad0;
	ulong	id;
	ulong	_pad1;
	ulong	skuid;
};


static char *
devidstr(ulong)
{
	return "ARM Cortex-A8";
}

void
archomaplink(void)
{
}

int
ispow2(uvlong ul)
{
	/* see Hacker's Delight if this isn't obvious */
	return (ul & (ul - 1)) == 0;
}

/*
 * return exponent of smallest power of 2 ≥ n
 */
int
log2(ulong n)
{
	int i;

	i = 31 - clz(n);
	if (n == 0 || !ispow2(n))
		i++;
	return i;
}

void
archconfinit(void)
{
	char *p;
	ulong mhz;

	assert(m != nil);
	m->cpuhz = 500 * Mhz;			/* beagle speed */
	p = getconf("*cpumhz");
	if (p) {
		mhz = atoi(p) * Mhz;
		if (mhz >= 100*Mhz && mhz <= 3000UL*Mhz)
			m->cpuhz = mhz;
	}
	m->delayloop = m->cpuhz/2000;		/* initial estimate */
}

static void
prperm(uvlong perm)
{
	if (perm == MASK(16))
		print("all");
	else
		print("%#llux", perm);
}

static void
prl3region(L3protreg *pr, int r)
{
	int level, size, addrspace;
	uvlong am, base;

	if (r == 0)
		am = 0;
	else
		am = pr->addr_match;
	size = (am >> 3) & MASK(5);
	if (r > 0 && size == 0)			/* disabled? */
		return;

	print("  %d: perms req ", r);
	prperm(pr->req_info_perm);
	if (pr->read_perm == pr->write_perm && pr->read_perm == MASK(16))
		print(" rw all");
	else {
		print(" read ");
		prperm(pr->read_perm);
		print(" write ");
		prperm(pr->write_perm);
	}
	if (r == 0)
		print(", all addrs level 0");
	else {
		size = 1 << size;		/* 2^size */
		level = (am >> 9) & 1;
		if (r == 1)
			level = 3;
		else
			level++;
		addrspace = am & 7;
		base = am & ~MASK(10);
		print(", base %#llux size %dKB level %d addrspace %d",
			base, size, level, addrspace);
	}
	print("\n");
	delay(100);
}


/*
 * dump the l3 interconnect firewall settings by protection region.
 * mpu, sys dma and both usbs (0x21a) should be set in all read & write
 * permission registers.
 */
static void
dumpl3pr(void)
{
	int r;
	L3regs *reg;
	L3protreg *pr;

	for (reg = l3regs; reg < l3regs + nelem(l3regs); reg++) {
		print("%#p (%s) enabled l3 regions:\n", reg->base, reg->name);
		for (r = 0; r <= reg->upper; r++)
			prl3region(reg->base + r, r);
	}
if (0) {				// TODO
	/* touch up gpmc perms */
	reg = l3regs;			/* first entry is gpmc */
	for (r = 0; r <= reg->upper; r++) {
		pr = reg->base + r;
		// TODO
	}
	print("%#p (%s) modified l3 regions:\n", reg->base, reg->name);
	for (r = 0; r <= reg->upper; r++)
		prl3region(reg->base + r, r);
}
}

static void
p16(uchar *p, ulong v)
{
	*p++ = v>>8;
	*p   = v;
}

static void
p32(uchar *p, ulong v)
{
	*p++ = v>>24;
	*p++ = v>>16;
	*p++ = v>>8;
	*p   = v;
}

int
archether(unsigned ctlrno, Ether *ether)
{
	switch(ctlrno) {
	case 0:
		/* there's no built-in ether on the beagle but igepv2 has 1 */
		ether->type = "9221";
		ether->ctlrno = ctlrno;
		ether->irq = 34;
		ether->nopt = 0;
		ether->mbps = 100;
		return 1;
	}
	return -1;
}

/*
 * turn on all the necessary clocks on the SoC.
 *
 * a ``functional'' clock drives a device; an ``interface'' clock drives
 * its communication with the rest of the system.  so the interface
 * clock must be enabled to reach the device's registers.
 *
 * dplls: 1 mpu, 2 iva2, 3 core, 4 per, 5 per2.
 */

static void
configmpu(void)
{
	ulong clk, mhz, nmhz, maxmhz;
	Cm *mpu = (Cm *)PHYSSCMMPU;
	Cntrl *id = (Cntrl *)PHYSCNTRL;

	if ((id->skuid & MASK(4)) == 8)
		maxmhz = 720;
	else
		maxmhz = 600;
	iprint("cpu capable of %ldMHz operation", maxmhz);

	clk = mpu->clksel[0];
	mhz = (clk >> 8) & MASK(11);		/* configured speed */
//	iprint("\tfclk src %ld; dpll1 mult %ld (MHz) div %ld",
//		(clk >> 19) & MASK(3), mhz, clk & MASK(7));
	iprint("; at %ldMHz", mhz);
	nmhz = m->cpuhz / Mhz;			/* nominal speed */
	if (mhz == nmhz) {
		iprint("\n");
		return;
	}

	mhz = nmhz;
	if (mhz > maxmhz) {
		mhz = maxmhz;
		iprint("; limiting operation to %ldMHz", mhz);
	}

	/* disable dpll1 lock mode; put into low-power bypass mode */
	mpu->fclken2 = mpu->fclken2 & ~MASK(3) | 5;
	coherence();
	while (mpu->idlest2 != Dpllbypassed)
		;

	/*
	 * there's a dance to change processor speed,
	 * prescribed in spruf98d §4.7.6.9.
	 */

	/* just change multiplier; leave divider alone at 12 (meaning 13?) */
	mpu->clksel[0] = clk & ~(MASK(11) << 8) | mhz << 8;
	coherence();

	/* set output divider (M2) in clksel[1]: leave at 1 */

	/*
	 * u-boot calls us with just freqsel 3 (~1MHz) & dpll1 lock mode.
	 */
	/* set FREQSEL */
	mpu->fclken2 = mpu->fclken2 & ~FREQSEL(MASK(4)) | FREQSEL(3);
	coherence();

	/* set ramp-up delay to `fast' */
	mpu->fclken2 = mpu->fclken2 & ~(MASK(2) << 8) | 3 << 8;
	coherence();

	/* set auto-recalibration (off) */
	mpu->fclken2 &= ~(1 << 3);
	coherence();

	/* disable auto-idle: ? */
	/* unmask clock intr: later */

	/* enable dpll lock mode */
	mpu->fclken2 |= Dplllock;
	coherence();
	while (mpu->idlest2 != Dplllocked)
		;
	delay(200);			/* allow time for speed to ramp up */

	if (((mpu->clksel[0] >> 8) & MASK(11)) != mhz)
		panic("mpu clock speed change didn't stick");
	iprint("; now at %ldMHz\n", mhz);
}

static void
configpll(void)
{
	int i;
	Cm *pll = (Cm *)PHYSSCMPLL;

	pll->clkoutctrl |= 1 << 7;	/* enable sys_clkout2 */
	coherence();
	delay(10);

	/*
	 * u-boot calls us with just freqsel 3 (~1MHz) & lock mode
	 * for both dplls (3 & 4).  ensure that.
	 */
	if ((pll->idlest & 3) != 3) {
		/* put dpll[34] into low-power bypass mode */
		pll->fclken = pll->fclken & ~(MASK(3) << 16 | MASK(3)) |
			1 << 16 | 5;
		coherence();
		while (pll->idlest & 3)  /* wait for both to bypass or stop */
			;

		pll->fclken =  (FREQSEL(3) | Dplllock) << 16 |
				FREQSEL(3) | Dplllock;
		coherence();
		while ((pll->idlest & 3) != 3)	/* wait for both to lock */
			;
	}

	/*
	 * u-boot calls us with just freqsel 1 (default but undefined)
	 * & stop mode for dpll5.  try to lock it at 120MHz.
	 */
	if (!(pll->idlest2 & Dplllocked)) {
		/* force dpll5 into low-power bypass mode */
		pll->fclken2 = 3 << 8 | FREQSEL(1) | 1;
		coherence();
		for (i = 0; pll->idlest2 & Dplllocked && i < 20; i++)
			delay(50);
		if (i >= 20)
			iprint(" [dpll5 failed to stop]");

		/*
		 * CORE_CLK is 26MHz.
		 */
		pll->clksel[4-1] = 120 << 8 | 12;	/* M=120, N=12+1 */
		/* M2 divisor: 120MHz clock is exactly the DPLL5 clock */
		pll->clksel[5-1] = 1;
		coherence();

		pll->fclken2 = 3 << 8 | FREQSEL(1) | Dplllock; /* def. freq */
		coherence();

		for (i = 0; !(pll->idlest2 & Dplllocked) && i < 20; i++)
			delay(50);
		if (i >= 20)
			iprint(" [dpll5 failed to lock]");
	}
	if (!(pll->idlest2 & (1<<1)))
		iprint(" [no 120MHz clock]");
	if (!(pll->idlest2 & (1<<3)))
		iprint(" [no dpll5 120MHz clock output]");
}

static void
configper(void)
{
	Cm *per = (Cm *)PHYSSCMPER;

	per->clksel[0] &= ~MASK(8);	/* select 32kHz clock for GPTIMER2-9 */

	per->iclken |= Perenable;
	coherence();
	per->fclken |= Perenable;
	coherence();
	while (per->idlest & Perenable)
		;

	per->autoidle = 0;
	coherence();
}

static void
configwkup(void)
{
	Cm *wkup = (Cm *)PHYSSCMWKUP;

	/* select 32kHz clock (not system clock) for GPTIMER1 */
	wkup->clksel[0] &= ~1;

	wkup->iclken |= Wkusimocp | Wkwdt2 | Wkgpt1;
	coherence();
	wkup->fclken |= Wkusimocp | Wkwdt2 | Wkgpt1;
	coherence();
	while (wkup->idlest & (Wkusimocp | Wkwdt2 | Wkgpt1))
		;
}

static void
configusb(void)
{
	int i;
	Cm *usb = (Cm *)PHYSSCMUSB;

	/*
	 * make the usb registers accessible without address faults,
	 * notably uhh, ochi & ehci.  tll seems to be separate & otg is okay.
	 */
	usb->iclken |= Usbhost;
	coherence();
	usb->fclken |= Usbhost1 | Usbhost2;	/* includes 120MHz clock */
	coherence();
	for (i = 0; usb->idlest & Usbhostidle && i < 20; i++)
		delay(50);
	if (i >= 20)
		iprint(" [usb inaccessible]");
}

static void
configcore(void)
{
	Cm *core = (Cm *)PHYSSCMCORE;

	/*
	 * make the usb tll registers accessible.
	 */
	core->iclken  |= Coreusbhsotg;
	core->iclken3 |= Core3usbtll;
	coherence();
	core->fclken3 |= Core3usbtll;
	coherence();
	delay(100);
	while (core->idlest & Coreusbhsotgidle)
		;
	if (core->idlest3 & Core3usbtll)
		iprint(" [no usb tll]");
}

static void
configclks(void)
{
	int s;
	Gen *gen = (Gen *)PHYSSCMPCONF;

	delay(20);
	s = splhi();
	configmpu();		/* sets cpu clock rate, turns on dplls 1 & 2 */

	/*
	 * the main goal is to get enough clocks running, in the right order,
	 * so that usb has all the necessary clock signals.
	 */
	iprint("clocks:");
	iprint(" usb");
	configusb();		/* starts usb clocks & 120MHz clock */
	iprint(", pll");
	configpll();		/* starts dplls 3, 4 & 5 & 120MHz clock */
	iprint(", wakeup");
	configwkup();		/* starts timer clocks and usim clock */
	iprint(", per");
	configper();		/* starts timer & gpio (ether) clocks */
	iprint(", core");
	configcore();		/* starts usb tll */
	iprint("\n");

	gen->devconf0 |= 1 << 1 | 1 << 0;	/* dmareq[01] edge sensitive */
	/* make dmareq[2-6] edge sensitive */
	gen->devconf1 |= 1 << 23 | 1 << 22 | 1 << 21 | 1 << 8 | 1 << 7;
	coherence();
	splx(s);
	delay(20);
}

static void
resetwait(ulong *reg)
{
	long bound;

	for (bound = 400*Mhz; !(*reg & Resetdone) && bound > 0; bound--)
		;
	if (bound <= 0)
		iprint("archomap: Resetdone didn't come ready\n");
}

/*
 * gpio irq 1 goes to the mpu intr ctlr; irq 2 goes to the iva's.
 * this stuff is magic and without it, we won't get irq 34 interrupts
 * from the 9221 ethernet controller.
 */
static void
configgpio(void)
{
	Gpio *gpio = (Gpio *)PHYSGPIO6;

	gpio->sysconfig = Softreset;
	coherence();
	resetwait(&gpio->sysstatus);

	gpio->ctrl = 1<<1 | 0;	/* enable this gpio module, gating ratio 1 */
	gpio->oe |= Etherchanbit;	/* cfg ether pin as input */
	coherence();

	gpio->irqen1 = Etherchanbit;	/* channel # == pin # */
	gpio->irqen2 = 0;

	gpio->lvldet0 = Etherchanbit;	/* enable irq ass'n on low det'n */
	gpio->lvldet1 = 0;		/* disable irq ass'n on high det'n */
	gpio->risingdet = 0;		/* enable irq rising edge det'n */
	gpio->fallingdet = 0;		/* disable irq falling edge det'n */

	gpio->wkupen = 0;

	gpio->deben = 0;		/* no de-bouncing */
	gpio->debtime = 0;
	coherence();

	gpio->irqsts1 = ~0;		/* dismiss all outstanding intrs */
	gpio->irqsts2 = ~0;
	coherence();
}

void
configscreengpio(void)
{
	Cm *wkup = (Cm *)PHYSSCMWKUP;
	Gpio *gpio = (Gpio *)PHYSGPIO1;

	/* no clocksel needed */
	wkup->iclken |= Wkgpio1;
	coherence();
	wkup->fclken |= Wkgpio1;		/* turn gpio clock on */
	coherence();
	// wkup->autoidle |= Wkgpio1; 		/* set gpio clock on auto */
	wkup->autoidle = 0;
	coherence();
	while (wkup->idlest & Gpio1idle)
		;

	/*
	 * 0 bits in oe are output signals.
	 * enable output for gpio 1 (first gpio) video magic pins.
	 */
	gpio->oe &= ~Gpio1vidmagic;
	coherence();
	gpio->dataout |= Gpio1vidmagic;		/* set output pins to 1 */
	coherence();
	delay(50);
}

void
screenclockson(void)
{
	Cm *dss = (Cm *)PHYSSCMDSS;

	dss->iclken |= Dssl3l4;
	coherence();
	dss->fclken = Dsstv | Dss2 | Dss1;
	coherence();
	/* tv fclk is dpll4 clk; dpll4 m4 divide factor for dss1 fclk is 2 */
	dss->clksel[0] = 1<<12 | 2;
	coherence();
	delay(50);
	while (dss->idlest & Dssidle)
		;
}

void
gpioirqclr(void)
{
	Gpio *gpio = (Gpio *)PHYSGPIO6;

	gpio->irqsts1 = gpio->irqsts1;
	coherence();
}

static char *
l1iptype(uint type)
{
	static char *types[] = {
		"reserved",
		"asid-tagged VIVT",
		"VIPT",
		"PIPT",
	};

	if (type >= nelem(types) || types[type] == nil)
		return "GOK";
	return types[type];
}

void
cacheinfo(int level, Memcache *cp)
{
	ulong setsways;

	/* select cache level */
	cpwrsc(CpIDcssel, CpID, CpIDid, 0, (level - 1) << 1);

	setsways = cprdsc(CpIDcsize, CpID, CpIDid, 0);
	cp->l1ip = cprdsc(0, CpID, CpIDidct, CpIDct);
	cp->level = level;
	cp->nways = ((setsways >> 3)  & MASK(10)) + 1;
	cp->nsets = ((setsways >> 13) & MASK(15)) + 1;
	cp->log2linelen = (setsways & MASK(2)) + 2 + 2;
	cp->linelen = 1 << cp->log2linelen;
	cp->setsways = setsways;

	cp->setsh = cp->log2linelen;
	cp->waysh = 32 - log2(cp->nways);
}

static void
prcachecfg(void)
{
	int cache;
	Memcache mc;

	for (cache = 1; cache <= 2; cache++) {
		cacheinfo(cache, &mc);
		iprint("l%d: %d ways %d sets %d bytes/line",
			mc.level, mc.nways, mc.nsets, mc.linelen);
		if (mc.linelen != CACHELINESZ)
			iprint(" *should* be %d", CACHELINESZ);
		if (mc.setsways & Cawt)
			iprint("; can WT");
		if (mc.setsways & Cawb)
			iprint("; can WB");
#ifdef COMPULSIVE			/* both caches can do this */
		if (mc.setsways & Cara)
			iprint("; can read-allocate");
#endif
		if (mc.setsways & Cawa)
			iprint("; can write-allocate");
		if (cache == 1)
			iprint("; l1 I policy %s",
				l1iptype((mc.l1ip >> 14) & MASK(2)));
		iprint("\n");
	}
}

static char *
subarch(int impl, uint sa)
{
	static char *armarchs[] = {
		"VFPv1 (pre-armv7)",
		"VFPv2 (pre-armv7)",
		"VFPv3+ with common VFP subarch v2",
		"VFPv3+ with null subarch",
		"VFPv3+ with common VFP subarch v3",
	};

	if (impl != 'A' || sa >= nelem(armarchs))
		return "GOK";
	else
		return armarchs[sa];
}

/*
 * padconf bits in a short, 2 per long register
 *	15	wakeupevent
 *	14	wakeupenable
 *	13	offpulltypeselect
 *	12	offpulludenable
 *	11	offoutvalue
 *	10	offoutenable
 *	9	offenable
 *	8	inputenable
 *	4	pulltypeselect
 *	3	pulludenable
 *	2-0	muxmode
 *
 * see table 7-5 in §7.4.4.3 of spruf98d
 */

enum {
	/* pad config register bits */
	Inena	= 1 << 8,		/* input enable */
	Indis	= 0 << 8,		/* input disable */
	Ptup	= 1 << 4,		/* pull type up */
	Ptdown	= 0 << 4,		/* pull type down */
	Ptena	= 1 << 3,		/* pull type selection is active */
	Ptdis	= 0 << 3,		/* pull type selection is inactive */
	Muxmode	= MASK(3),

	/* pad config registers relevant to flash */
	GpmcA1		= 0x4800207A,
	GpmcA2		= 0x4800207C,
	GpmcA3		= 0x4800207E,
	GpmcA4		= 0x48002080,
	GpmcA5		= 0x48002082,
	GpmcA6		= 0x48002084,
	GpmcA7		= 0x48002086,
	GpmcA8		= 0x48002088,
	GpmcA9		= 0x4800208A,
	GpmcA10		= 0x4800208C,
	GpmcD0		= 0x4800208E,
	GpmcD1		= 0x48002090,
	GpmcD2		= 0x48002092,
	GpmcD3		= 0x48002094,
	GpmcD4		= 0x48002096,
	GpmcD5		= 0x48002098,
	GpmcD6		= 0x4800209A,
	GpmcD7		= 0x4800209C,
	GpmcD8		= 0x4800209E,
	GpmcD9		= 0x480020A0,
	GpmcD10		= 0x480020A2,
	GpmcD11		= 0x480020A4,
	GpmcD12		= 0x480020A6,
	GpmcD13		= 0x480020A8,
	GpmcD14		= 0x480020AA,
	GpmcD15		= 0x480020AC,
	GpmcNCS0	= 0x480020AE,
	GpmcNCS1	= 0x480020B0,
	GpmcNCS2	= 0x480020B2,
	GpmcNCS3	= 0x480020B4,
	GpmcNCS4	= 0x480020B6,
	GpmcNCS5	= 0x480020B8,
	GpmcNCS6	= 0x480020BA,
	GpmcNCS7	= 0x480020BC,
	GpmcCLK		= 0x480020BE,
	GpmcNADV_ALE	= 0x480020C0,
	GpmcNOE		= 0x480020C2,
	GpmcNWE		= 0x480020C4,
	GpmcNBE0_CLE	= 0x480020C6,
	GpmcNBE1	= 0x480020C8,
	GpmcNWP		= 0x480020CA,
	GpmcWAIT0	= 0x480020CC,
	GpmcWAIT1	= 0x480020CE,
	GpmcWAIT2	= 0x480020D0,
	GpmcWAIT3	= 0x480020D2,
};

/* set SCM pad config mux mode */
void
setmuxmode(ulong addr, int shorts, int mode)
{
	int omode;
	ushort *ptr;

	mode &= Muxmode;
	for (ptr = (ushort *)addr; shorts-- > 0; ptr++) {
		omode = *ptr & Muxmode;
		if (omode != mode)
			*ptr = *ptr & ~Muxmode | mode;
	}
	coherence();
}

static void
setpadmodes(void)
{
	int off;

	/* set scm pad modes for usb; hasn't made any difference yet */
	setmuxmode(0x48002166, 7, 5);	/* hsusb3_tll* in mode 5; is mode 4 */
	setmuxmode(0x48002180, 1, 5);	/* hsusb3_tll_clk; is mode 4 */
	setmuxmode(0x48002184, 4, 5);	/* hsusb3_tll_data?; is mode 1 */
	setmuxmode(0x480021a2, 12, 0);	/* hsusb0 (console) in mode 0 */
	setmuxmode(0x480021d4, 6, 2);	/* hsusb2_tll* (ehci port 2) in mode 2 */
					/* mode 3 is hsusb2_data* */
	setmuxmode(0x480025d8, 18, 6);	/* hsusb[12]_tll*; mode 3 is */
					/* hsusb1_data*, hsusb2* */

	setmuxmode(0x480020e4, 2, 5);	/* uart3_rx_* in mode 5 */
	setmuxmode(0x4800219a, 4, 0);	/* uart3_* in mode 0 */
	/* uart3_* in mode 2; TODO: conflicts with hsusb0 */
	setmuxmode(0x480021aa, 4, 2);
	setmuxmode(0x48002240, 2, 3);	/* uart3_* in mode 3 */

	/*
	 * igep/gumstix only: mode 4 of 21d2 is gpio_176 (smsc9221 ether irq).
	 * see ether9221.c for more.
	 */
	*(ushort *)0x480021d2 = Inena | Ptup | Ptena | 4;

	/* magic from u-boot for flash */
	*(ushort *)GpmcA1	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA2	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA3	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA4	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA5	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA6	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA7	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA8	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA9	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcA10	= Indis | Ptup | Ptena | 0;

	*(ushort *)GpmcD0	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD1	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD2	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD3	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD4	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD5	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD6	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD7	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD8	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD9	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD10	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD11	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD12	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD13	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD14	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcD15	= Inena | Ptup | Ptena | 0;

	*(ushort *)GpmcNCS0	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcNCS1	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcNCS2	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcNCS3	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcNCS4	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcNCS5	= Indis | Ptup | Ptena | 0;
	*(ushort *)GpmcNCS6	= Indis | Ptup | Ptena | 0;

	*(ushort *)GpmcNOE	= Indis | Ptdown | Ptdis | 0;
	*(ushort *)GpmcNWE	= Indis | Ptdown | Ptdis | 0;

	*(ushort *)GpmcWAIT2	= Inena | Ptup | Ptena | 4; /* GPIO_64 -ETH_NRESET */
	*(ushort *)GpmcNCS7	= Inena | Ptup | Ptena | 1; /* SYS_nDMA_REQ3 */

	*(ushort *)GpmcCLK	= Indis | Ptdown | Ptdis | 0;

	*(ushort *)GpmcNBE1	= Inena | Ptdown | Ptdis | 0;

	*(ushort *)GpmcNADV_ALE	= Indis | Ptdown | Ptdis | 0;
	*(ushort *)GpmcNBE0_CLE	= Indis | Ptdown | Ptdis | 0;

	*(ushort *)GpmcNWP	= Inena | Ptdown | Ptdis | 0;

	*(ushort *)GpmcWAIT0	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcWAIT1	= Inena | Ptup | Ptena | 0;
	*(ushort *)GpmcWAIT3	= Inena | Ptup | Ptena | 0;

	/*
	 * magic from u-boot: set 0xe00 bits in gpmc_(nwe|noe|nadv_ale)
	 * to enable `off' mode for each.
	 */
	for (off = 0xc0; off <= 0xc4; off += sizeof(short))
		*((ushort *)(PHYSSCM + off)) |= 0xe00;
	coherence();
}

static char *
implement(uchar impl)
{
	if (impl == 'A')
		return "arm";
	else
		return "unknown";
}

static void
fpon(void)
{
	int gotfp, impl;
	ulong acc, scr;

	gotfp = 1 << CpFP | 1 << CpDFP;
	cpwrsc(0, CpCONTROL, 0, CpCPaccess, MASK(28));
	acc = cprdsc(0, CpCONTROL, 0, CpCPaccess);
	if ((acc & (MASK(2) << (2*CpFP))) == 0) {
		gotfp &= ~(1 << CpFP);
		print("fpon: no single FP coprocessor\n");
	}
	if ((acc & (MASK(2) << (2*CpDFP))) == 0) {
		gotfp &= ~(1 << CpDFP);
		print("fpon: no double FP coprocessor\n");
	}
	if (!gotfp) {
		print("fpon: no FP coprocessors\n");
		return;
	}

	/* enable fp.  must be first operation on the FPUs. */
	fpwr(Fpexc, fprd(Fpexc) | 1 << 30);

	scr = fprd(Fpsid);
	impl = scr >> 24;
	print("fp: %s arch %s", implement(impl),
		subarch(impl, (scr >> 16) & MASK(7)));

	scr = fprd(Fpscr);
	// TODO configure Fpscr further
	scr |= 1 << 9;					/* div-by-0 exception */
	scr &= ~(MASK(2) << 20 | MASK(3) << 16);	/* all ops are scalar */
	fpwr(Fpscr, scr);
	print("\n");
	/* we should now be able to execute VFP-style FP instr'ns natively */
}

static void
resetusb(void)
{
	int bound;
	Uhh *uhh;
	Usbotg *otg;
	Usbtll *tll;

	iprint("resetting usb: otg...");
	otg = (Usbotg *)PHYSUSBOTG;
	otg->otgsyscfg = Softreset;	/* see omap35x errata 3.1.1.144 */
	coherence();
	resetwait(&otg->otgsyssts);
	otg->otgsyscfg |= Sidle | Midle;
	coherence();

	iprint("uhh...");
	uhh = (Uhh *)PHYSUHH;
	uhh->sysconfig |= Softreset;
	coherence();
	resetwait(&uhh->sysstatus);
	for (bound = 400*Mhz; !(uhh->sysstatus & Resetdone) && bound > 0;
	    bound--)
		;
	uhh->sysconfig |= Sidle | Midle;

	/*
	 * using the TLL seems to be an optimisation when talking
	 * to another identical SoC, thus not very useful, so
	 * force PHY (ULPI) mode.
	 */
	/* this bit is normally off when we get here */
	uhh->hostconfig &= ~P1ulpi_bypass;
	coherence();
	if (uhh->hostconfig & P1ulpi_bypass)
		iprint("utmi (tll) mode...");	/* via tll */
	else
		/* external transceiver (phy), no tll */
		iprint("ulpi (phy) mode...");

	tll = (Usbtll *)PHYSUSBTLL;
	if (probeaddr(PHYSUSBTLL) >= 0) {
		iprint("tll...");
		tll->sysconfig |= Softreset;
		coherence();
		resetwait(&tll->sysstatus);
		tll->sysconfig |= Sidle;
		coherence();
	} else
		iprint("no tll...");
	iprint("\n");
}

/*
 * there are secure sdrc registers at 0x48002460
 * sdrc regs at PHYSSDRC; see spruf98c §1.2.8.2.
 * set or dump l4 prot regs at PHYSL4?
 */
void
archreset(void)
{
	static int beenhere;

	if (beenhere)
		return;
	beenhere = 1;

	/* conservative temporary values until archconfinit runs */
	m->cpuhz = 500 * Mhz;			/* beagle speed */
	m->delayloop = m->cpuhz/2000;		/* initial estimate */

//	dumpl3pr();
	prcachecfg();
	/* fight omap35x errata 2.0.1.104 */
	memset((void *)PHYSSWBOOTCFG, 0, 240);
	coherence();

	setpadmodes();
	configclks();			/* may change cpu speed */
	configgpio();

	archconfinit();

	resetusb();
	fpon();
}

void
archreboot(void)
{
	Prm *prm = (Prm *)PHYSPRMGLBL;

	iprint("archreboot: reset!\n");
	delay(20);

	prm->rstctrl |= Rstgs;
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

void
lastresortprint(char *buf, long bp)
{
	iprint("%.*s", (int)bp, buf);	/* nothing else seems to work */
}

static void
scmdump(ulong addr, int shorts)
{
	ushort reg;
	ushort *ptr;

	ptr = (ushort *)addr;
	print("scm regs:\n");
	while (shorts-- > 0) {
		reg = *ptr++;
		print("%#p: %#ux\tinputenable %d pulltypeselect %d "
			"pulludenable %d muxmode %d\n",
			ptr, reg, (reg>>8) & 1, (reg>>4) & 1, (reg>>3) & 1,
			reg & 7);
	}
}

char *cputype2name(char *buf, int size);

void
cpuidprint(void)
{
	char name[64];

	cputype2name(name, sizeof name);
	delay(250);				/* let uart catch up */
	iprint("cpu%d: %lldMHz ARM %s\n", m->machno, m->cpuhz / Mhz, name);
}

static void
missing(ulong addr, char *name)
{
	static int firstmiss = 1;

	if (probeaddr(addr) >= 0)
		return;
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
	delay(20);
	missing(PHYSSCM, "scm");
	missing(KZERO, "dram");
	missing(PHYSL3, "l3 config");
	missing(PHYSINTC, "intr ctlr");
	missing(PHYSTIMER1, "timer1");
	missing(PHYSCONS, "console uart2");
	missing(PHYSUART0, "uart0");
	missing(PHYSUART1, "uart1");
	missing(PHYSETHER, "smc9221");		/* not on beagle */
	missing(PHYSUSBOTG, "usb otg");
	missing(PHYSUHH, "usb uhh");
	missing(PHYSOHCI, "usb ohci");
	missing(PHYSEHCI, "usb ehci");
	missing(PHYSSDMA, "dma");
	missing(PHYSWDOG, "watchdog timer");
	missing(PHYSUSBTLL, "usb tll");
	iprint("\n");
	delay(20);
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
	/*
	 * this is set up for the igepv2 board.
	 * if the beagleboard ever works, we'll have to sort this out.
	 */
	f->type = "onenand";
	f->addr = (void*)PHYSNAND;		/* mapped here by archreset */
	f->size = 0;				/* done by probe */
	f->width = 1;
	f->interleave = 0;
	return 0;
}
