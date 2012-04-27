/*
 * stuff specific to marvell's kirkwood architecture
 * as seen in the sheevaplug
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#include "../port/netif.h"
#include "etherif.h"
#include "../port/flashif.h"

#include "arm.h"

enum {
	L2writeback = 1,
	Debug = 0,
};

typedef struct GpioReg GpioReg;
struct GpioReg {
	ulong	dataout;
	ulong	dataoutena;
	ulong	blinkena;
	ulong	datainpol;
	ulong	datain;
	ulong	intrcause;
	ulong	intrmask;
	ulong	intrlevelmask;
};

typedef struct L2uncache L2uncache;
typedef struct L2win L2win;
struct L2uncache {
	struct L2win {
		ulong	base;	/* phys addr */
		ulong	size;
	} win[4];
};

enum {
	/* L2win->base bits */
	L2enable	= 1<<0,
};

typedef struct Dramctl Dramctl;
struct Dramctl {
	ulong	ctl;
	ulong	ddrctllo;
	struct {
		ulong	lo;
		ulong	hi;
	} time;
	ulong	addrctl;
	ulong	opagectl;
	ulong	oper;
	ulong	mode;
	ulong	extmode;
	ulong	ddrctlhi;
	ulong	ddr2timelo;
	ulong	operctl;
	struct {
		ulong	lo;
		ulong	hi;
	} mbusctl;
	ulong	mbustimeout;
	ulong	ddrtimehi;
	ulong	sdinitctl;
	ulong	extsdmode1;
	ulong	extsdmode2;
	struct {
		ulong	lo;
		ulong	hi;
	} odtctl;
	ulong	ddrodtctl;
	ulong	rbuffsel;

	ulong	accalib;
	ulong	dqcalib;
	ulong	dqscalib;
};

/* unused so far */
typedef struct SDramdReg SDramdReg;
struct SDramdReg {
	struct {
		ulong	base;
		ulong	size;
	} win[4];
};

typedef struct Addrmap Addrmap;
typedef struct Addrwin Addrwin;
struct Addrmap {
	struct Addrwin {
		ulong	ctl;		/* see Winenable in io.h */
		ulong	base;		/* virtual address */
		ulong	remaplo;	/* physical address sent to target */
		ulong	remaphi;	/* " */
	} win[8];
	ulong	dirba;		/* device internal reg's base addr.: PHYSIO */
};

Soc soc = {
	.cpu		= PHYSIO+0x20100,
	.devid		= PHYSIO+0x10034,
	.l2cache	= PHYSIO+0x20a00,  	/* uncachable addrs for L2 */
	.sdramc		= PHYSIO+0x01400,
//	.sdramd		= PHYSIO+0x01500,	/* unused */

	.iocfg		= PHYSIO+0x100e0,
	.addrmap	= PHYSIO+0x20000,	/* cpu address map */
	.intr		= PHYSIO+0x20200,
	.nand		= PHYSIO+0x10418,
	.cesa		= PHYSIO+0x30000,	/* crypto accelerator */
	.ehci		= PHYSIO+0x50000,
	.spi		= PHYSIO+0x10600,
 	.twsi		= PHYSIO+0x11000,

	.analog		= PHYSIO+0x1007c,
	.pci		= PHYSIO+0x40000,
	.pcibase	= PHYSIO+0x41800,

	.rtc		= PHYSIO+0x10300,
	.clock		= PHYSIO+0x20300,
//	.clockctl	= PHYSIO+0x1004c,	/* unused */

	.ether		= { PHYSIO+0x72000, PHYSIO+0x76000, },
	.sata		= { PHYSIO+0x80000,	/* sata config reg here */
			PHYSIO+0x82000,		/* edma config reg here */
			PHYSIO+0x84000,		/* edma config reg here */
			},
	.uart		= { PHYSIO+0x12000, PHYSIO+0x12100, },
	.gpio		= { PHYSIO+0x10100, PHYSIO+0x10140, },
};

/*
 * sheeva/openrd u-boot leaves us with this address map:
 *
 * 0 targ 4 attr 0xe8 size 256MB addr 0x9::  remap addr 0x9::	pci mem
 * 1 targ 1 attr 0x2f size   8MB addr 0xf9:: remap addr 0xf9::	nand flash
 * 2 targ 4 attr 0xe0 size  16MB addr 0xf::  remap addr 0xc::	pci i/o
 * 3 targ 1 attr 0x1e size  16MB addr 0xf8:: remap addr 0x0	spi flash
 * 4 targ 1 attr 0x1d size  16MB addr 0xff::			boot rom
 * 5 targ 1 attr 0x1e size 128MB addr 0xe8::	disabled	spi flash
 * 6 targ 1 attr 0x1d size 128MB addr 0xf::	disabled	boot rom
 * 7 targ 3 attr 0x1  size  64K  addr 0xfb::			crypto sram
 *
 * dreamplug u-boot leaves us with this address map:
 *
 * 0 targ 4 attr 0xe8 size 256MB addr 0x9::  remap addr 0x9::	pci mem
 * 1 targ 4 attr 0xe0 size  64KB addr 0xc::  remap addr 0xc::	pci i/o
 * 2 targ 1 attr 0x2f size 128MB addr 0xd8:: remap addr 0xd8::	nand flash
 * 3 targ 1 attr 0x1e size 128MB addr 0xe8:: remap addr 0xe8::	spi flash
 * 4 targ 1 attr 0x1d size 128MB addr 0xf8::			boot rom
 * 5 targ 3 attr 0x1  size  64K  addr 0xc801::			crypto sram
 * 6 targ 0 attr 0x0  size  64K  addr 0xf::	disabled	ram?
 * 7 targ 0 attr 0x0  size  64K  addr 0xf8::	disabled	ram?
 */

#define WINTARG(ctl)	(((ctl) >> 4) & 017)
#define WINATTR(ctl)	(((ctl) >> 8) & 0377)
#define WIN64KSIZE(ctl)	(((ctl) >> 16) + 1)

static void
praddrwin(Addrwin *win, int i)
{
	ulong ctl, targ, attr, size64k;

	if (!Debug) {
		USED(win, i);
		return;
	}
	ctl = win->ctl;
	targ = WINTARG(ctl);
	attr = WINATTR(ctl);
	size64k = WIN64KSIZE(ctl);
	print("cpu addr map: %s window %d: targ %ld attr %#lux size %,ld addr %#lux",
		ctl & Winenable? "enabled": "disabled", i, targ, attr,
		size64k * 64*1024, win->base);
	if (i < 4)
		print(" remap addr %#llux", (uvlong)win->remaphi<<32 |
			win->remaplo);
	print("\n");
}

static void
fixaddrmap(void)
{
	int i;
	ulong ctl, targ, attr, size64k;
	Addrmap *map;
	Addrwin *win;

	map = (Addrmap *)soc.addrmap;
	for (i = 0; i < nelem(map->win); i++) {
		win = &map->win[i];
		ctl = win->ctl;
		targ = WINTARG(ctl);
		attr = WINATTR(ctl);
		size64k = WIN64KSIZE(ctl);

		USED(attr, size64k);
		if (targ == Targcesasram) {
			win->ctl |= Winenable;
			win->base = PHYSCESASRAM;
			coherence();
			praddrwin(win, i);
		}
	}
	if (map->dirba != PHYSIO)
		panic("dirba not %#ux", PHYSIO);
}

static void
praddrmap(void)
{
	int i;
	Addrmap *map;

	map = (Addrmap *)soc.addrmap;
	for (i = 0; i < nelem(map->win); i++)
		praddrwin(&map->win[i], i);
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
	if (!ispow2(n) || n == 0)
		i++;
	return i;
}

void
cacheinfo(int level, int kind, Memcache *cp)		/* l1 only */
{
	uint len, assoc, size;
	ulong setsways;

	/* get cache types & sizes (read-only reg) */
	setsways = cprdsc(0, CpID, CpIDidct, CpIDct);

	cp->level = level;
	cp->kind = kind;

	if ((setsways & (1<<24)) == 0)
		kind = Unified;
	if (kind != Icache)
		setsways >>= 12;

	assoc = (setsways >> 3) & MASK(3);
	cp->nways = 1 << assoc;
	size = (setsways >> 6) & MASK(4);
	cp->size  = 1 << (size + 9);
	len = setsways & MASK(2);
	cp->log2linelen = len + 3;
	cp->linelen = 1 << cp->log2linelen;
	cp->setsways = setsways;

	cp->nsets = 1 << (size + 6 - assoc - len);
	cp->setsh = cp->log2linelen;
	cp->waysh = 32 - log2(cp->nways);
}

static char *
wbtype(uint type)
{
	static char *types[] = {
		"write-through",
		"read data block",
		"reg 7 ops, no lock-down",
	[06]	"reg 7 ops, format A",
	[07]	"reg 7 ops, format B deprecated",
	[016]	"reg 7 ops, format C",
	[05]	"reg 7 ops, format D",
	};

	if (type >= nelem(types) || types[type] == nil)
		return "GOK";
	return types[type];
}

static void
prcache(Memcache *mcp)
{
	int type;
	char id;

	if (mcp->kind == Unified)
		id = 'U';
	else if (mcp->kind == Icache)
		id = 'I';
	else if (mcp->kind == Dcache)
		id = 'D';
	else
		id = '?';
	print("l%d %c: %d bytes, %d ways %d sets %d bytes/line",
		mcp->level, id, mcp->size, mcp->nways, mcp->nsets,
		mcp->linelen);
	if (mcp->linelen != CACHELINESZ)
		print(" *should* be %d", CACHELINESZ);
	type = (mcp->setsways >> 25) & MASK(4);
	if (type == 0)
		print("; write-through only");
	else
		print("; write-back type `%s' (%#o) possible",
			wbtype(type), type);
	if (mcp->setsways & (1<<11))
		print("; page table mapping restrictions apply");
	if (mcp->setsways & (1<<2))
		print("; M bit is set in cache type reg");
	print("\n");
}

static void
prcachecfg(void)
{
	Memcache mc;

	cacheinfo(1, Dcache, &mc);
	prcache(&mc);
	cacheinfo(1, Icache, &mc);
	prcache(&mc);
}

void
l2cacheon(void)
{
	ulong cfg;
	CpucsReg *cpu;
	L2uncache *l2p;

	cacheuwbinv();
	l2cacheuwbinv();
	l1cachesoff();			/* turns off L2 as a side effect */

	cpwrsc(CpDef, CpCLD, 0, 0, 0);  /* GL-CPU-100: set D cache lockdown reg. */

	/* marvell guideline GL-CPU-130 */
	cpu = (CpucsReg *)soc.cpu;
	cfg = cpu->cpucfg | L2exists | L2ecc | Cfgiprefetch | Cfgdprefetch;

	if (L2writeback)
		cfg &= ~L2writethru;	/* see PTE Cached & Buffered bits */
	else
		cfg |= L2writethru;
	cpu->l2cfg = cfg;
	coherence();			/* force l2 cache to pay attention */
	cpu->l2tm1 = cpu->l2tm0 = 0x66666666; /* marvell guideline GL-CPU-120 */
	coherence();

	cpwrsc(CpL2, CpTESTCFG, CpTCl2waylck, CpTCl2waylock, 0);

	cachedinv();
	l2cacheuinv();

	/* disable l2 caching of i/o registers */
	l2p = (L2uncache *)soc.l2cache;
	memset(l2p, 0, sizeof *l2p);
	/*
	 * l2: don't cache upper half of address space.
	 * the L2 cache is PIPT, so the addresses are physical.
	 */
	l2p->win[0].base = 0x80000000 | L2enable;	/* 64K multiple */
	l2p->win[0].size = (32*1024-1) << 16;		/* 64K multiples */
	coherence();

	l2cachecfgon();
	l1cacheson();			/* turns L2 on as a side effect */
	print("l2 cache: 256K or 512K: 4 ways, 32-byte lines, write-%s, sdram only\n",
		cpu->l2cfg & L2writethru? "through": "back");
}

/* called late in main */
void
archconfinit(void)
{
	m->cpuhz = Frequency;
	m->delayloop = m->cpuhz/2000; 	 /* initial estimate */
	fixaddrmap();
	if (Debug)
		praddrmap();
	prcachecfg();

	l2cacheon();
}

void
archkwlink(void)
{
}

int
archether(unsigned ctlno, Ether *ether)
{
	if(ctlno >= 2)
		return -1;
	ether->type = "88e1116";
	ether->port = ctlno;
//	ether->mbps = 1000;
	return 1;
}

/* LED/USB gpios */
enum {
	/*
	 * the bit assignments are MPP pin numbers from the last page of the
	 * sheevaplug 6.0.1 schematic.
	 */
	KWOEValHigh	= 1<<(49-32),	/* pin 49: LED pin */
	KWOEValLow	= 1<<29,	/* pin 29: USB_PWEN, pin 28: usb_pwerr */
	KWOELow		= ~0,
	KWOEHigh	= ~0,
};

/* called early in main */
void
archreset(void)
{
	ulong clocks;
	CpucsReg *cpu;
	Dramctl *dram;
	GpioReg *gpio;

	clockshutdown();		/* watchdog disabled */

	/* configure gpios */
	gpio = (GpioReg*)soc.gpio[0];
	gpio->dataout = KWOEValLow;
	coherence();
	gpio->dataoutena = KWOELow;

	gpio = (GpioReg*)soc.gpio[1];
	gpio->dataout = KWOEValHigh;
	coherence();
	gpio->dataoutena = KWOEHigh;
	coherence();

	cpu = (CpucsReg *)soc.cpu;
	cpu->mempm = 0;			/* turn everything on */
	coherence();

	clocks = MASK(10);
	clocks |= MASK(21) & ~MASK(14);
	clocks &= ~(1<<18 | 1<<1);	/* reserved bits */
	cpu->clockgate |= clocks;	/* enable all the clocks */
	cpu->l2cfg |= L2exists;		/* when L2exists is 0, the l2 ignores us */
	coherence();

	dram = (Dramctl *)soc.sdramc;
	dram->ddrctllo &= ~(1<<6);	/* marvell guideline GL-MEM-70 */

	*(ulong *)soc.analog = 0x68;	/* marvell guideline GL-MISC-40 */
	coherence();
}

void
archreboot(void)
{
	CpucsReg *cpu;

	iprint("reset!\n");
	delay(10);

	cpu = (CpucsReg *)soc.cpu;
	cpu->rstout = RstoutSoft;
	cpu->softreset = ResetSystem;
	coherence();
	cpu->cpucsr = Reset;
	coherence();
	delay(500);

	splhi();
	iprint("waiting...");
	for(;;)
		idlehands();
}

void
archconsole(void)
{
//	uartconsole(0, "b115200");
//serialputs("uart0 console @ 115200\n", strlen("uart0 console @ 115200\n"));
}

void
archflashwp(Flash*, int)
{
}

int	flashat(Flash *f, uintptr pa);

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
	f->type = "nand";
	if (flashat(f, PHYSNAND1))
		f->addr = (void*)PHYSNAND1;
	else if (flashat(f, PHYSNAND2))
		f->addr = (void*)PHYSNAND2;
	else
		f->addr = nil;
	f->size = 0;		/* done by probe */
	f->width = 1;
	f->interleave = 0;
	return 0;
}
