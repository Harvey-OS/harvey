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
// #include "../port/flashif.h"

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
		ulong	base;
		ulong	size;
	} win[4];
};

enum {
	/* L2win->base bits */
	L2enable	= 1<<0,
};

typedef struct Addrmap Addrmap;
typedef struct Addrwin Addrwin;
struct Addrmap {
	struct Addrwin {
		ulong	ctl;
		ulong	base;
		ulong	remaplo;
		ulong	remaphi;
	} win[8];
	ulong	dirba;		/* device internal reg's base addr.: Regbase */
};

enum {
	/* Addrwin->ctl bits */
	Winenable	= 1<<0,
};

/*
 * u-boot leaves us with this address map:
 *
 * 0 targ 4 attr 0xe8 size 256MB addr 0x9::  remap addr 0x9::	pci mem
 * 1 targ 1 attr 0x2f size   8MB addr 0xf9:: remap addr 0xf9::	nand flash
 * 2 targ 4 attr 0xe0 size  16MB addr 0xf::  remap addr 0xc::	pci i/o
 * 3 targ 1 attr 0x1e size  16MB addr 0xf8:: remap addr 0x0	spi flash
 * 4 targ 1 attr 0x1d size  16MB addr 0xff::			boot rom
 * 5 targ 1 attr 0x1e size 128MB addr 0xe8::	disabled	spi flash
 * 6 targ 1 attr 0x1d size 128MB addr 0xf::	disabled	boot rom
 * 7 targ 3 attr 0x1  size  64K  addr 0xfb::			crypto
 */
#define WINTARG(ctl)	(((ctl) >> 4) & 017)
#define WINATTR(ctl)	(((ctl) >> 8) & 0377)
#define WIN64KSIZE(ctl)	(((ctl) >> 16) + 1)

enum {
	Targflash = 1,
	Attrspi	= 0x1e,
};

static void
addrmap(void)
{
	int i, sawspi;
	ulong ctl, targ, attr, size64k;
	Addrmap *map;
	Addrwin *win;

	sawspi = 0;
	map = (Addrmap *)AddrWin;
	for (i = 0; i < nelem(map->win); i++) {
		win = &map->win[i];
		ctl = win->ctl;
		targ = WINTARG(ctl);
		attr = WINATTR(ctl);
		size64k = WIN64KSIZE(ctl);
		if (targ == Targflash && attr == Attrspi &&
		    size64k == 128*MB/(64*1024)) {
			sawspi = 1;
			if (!(ctl & Winenable)) {
				win->ctl |= Winenable;
				coherence();
				iprint("address map: enabled window %d for spi:\n"
					"\ttarg %ld attr %#lux size %,ld addr %#lux",
					i, targ, attr, size64k * 64*1024,
					win->base);
				if (i < 4)
					iprint(" remap addr %#llux",
						(uvlong)win->remaphi<<32 |
						win->remaplo);
				iprint("\n");
			}
		}
	}
	if (!sawspi)
		panic("address map: no existing window for spi");
//	iprint("dirba %#lux\n", map->dirba);
}

void
l2cacheon(void)
{
	L2uncache *l2p;

	l1cachesoff();
	cacheuwbinv();

	/* disable caching of i/o registers */
	l2p = (L2uncache *)Addrl2cache;
	memset(l2p, 0, sizeof *l2p);
	/* l2: don't cache upper half of address space */
	l2p->win[0].base = 0x80000000 | L2enable;	/* 64K multiple */
	l2p->win[0].size = (32768-1) << 16;		/* 64K multiples */
	coherence();

	cacheuwbinv();

	/* writeback requires extra care */
	CPUCSREG->l2cfg |= L2on | L2ecc | L2writethru;
	coherence();

	cachedinv();

	l2cachecfgon();
	l1cacheson();

	print("l2 cache enabled write-through\n");
}

/* called late in main */
void
archconfinit(void)
{
	m->cpuhz = 1200*1000*1000;
	m->delayloop = m->cpuhz/6000;  /* only an initial estimate */
	addrmap();
}

void
archkwlink(void)
{
}

int
archether(int ctlno, Ether *ether)
{
	if(ctlno != 0)
		return -1;
	ether->type = "kirkwood";
	ether->port = ctlno;
//	ether->mbps = 1000;
	return 1;
}

/* LED/USB gpios */
enum
{
	SheevaOEValLow	= 1<<29,        /* USB_PWEN low */
	SheevaOEValHigh	= 1<<17,        /* LED pin high */
	SheevaOELow	= ~0,
	SheevaOEHigh	= ~0,
};

/* called early in main */
void
archreset(void)
{
	/* watchdog disabled */
	TIMERREG->ctl = 0;

	/* configure gpios */
	((GpioReg*)AddrGpio0)->dataout = SheevaOEValLow;
	((GpioReg*)AddrGpio0)->dataoutena = SheevaOELow;

	((GpioReg*)AddrGpio1)->dataout = SheevaOEValHigh;
	((GpioReg*)AddrGpio1)->dataoutena = SheevaOEHigh;
	coherence();

	CPUCSREG->l2cfg &= ~L2on;
	coherence();
}

void
archreboot(void)
{
	iprint("reset!\n");
	delay(100);

	CPUCSREG->rstout = RstoutSoft;
	CPUCSREG->softreset = ResetSystem;
	CPUCSREG->cpucsr = Reset;
	delay(500);

	splhi();
	iprint("waiting...");
	for(;;)
		idlehands();
}

long
lcycles(void)
{
	return 0;
}

void
archconsole(void)
{
//	uartconsole(0, "b115200");
//serialputs("uart0 console @ 115200\n", strlen("uart0 console @ 115200\n"));
}

#ifdef USE_FLASH
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
	f->type = "nand";
	f->addr = (void*)PHYSNAND;
	f->size = 0;	/* done by probe */
	f->width = 1;
	f->interleave = 0;
	return 0;
}
#endif
