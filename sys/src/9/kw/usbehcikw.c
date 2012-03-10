/*
 * Kirkwood-specific code for
 * USB Enhanced Host Controller Interface (EHCI) driver
 * High speed USB 2.0.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"../port/usb.h"
#include	"../port/portusbehci.h"
#include	"usbehci.h"
//#include	"uncached.h"

#define WINTARG(ctl)	(((ctl) >> 4) & 017)
#define WINATTR(ctl)	(((ctl) >> 8) & 0377)
#define WIN64KSIZE(ctl)	(((ctl) >> 16) + 1)

#define SIZETO64KSIZE(size) ((size) / (64*1024) - 1)

enum {
	Debug = 0,
};

typedef struct Kwusb Kwusb;
typedef struct Kwusbtt Kwusbtt;
typedef struct Usbwin Usbwin;

/* kirkwood usb transaction translator registers? (undocumented) */
struct Kwusbtt {		/* at soc.ehci */
	ulong	id;
	ulong	hwgeneral;
	ulong	hwhost;
	ulong	hwdevice;
	ulong	hwtxbuf;
	ulong	hwrxbuf;
	ulong	hwtttxbuf;
	ulong	hwttrxbuf;
};

/* kirkwood usb bridge & phy registers */
struct Kwusb {			/* at offset 0x300 from soc.ehci */
	ulong	bcs;		/* bridge ctl & sts */
	uchar	_pad0[0x310-0x304];

	ulong	bic;		/* bridge intr. cause */
	ulong	bim;		/* bridge intr. mask */
	ulong	_pad1;
	ulong	bea;		/* bridge error addr. */
	struct Usbwin {
		ulong	ctl;	/* see Winenable in io.h */
		ulong	base;
		ulong	_pad2[2];
	} win[4];
	ulong	phycfg;		/* phy config. */
	uchar	_pad3[0x400-0x364];

	ulong	pwrctl;		/* power control */
	uchar	_pad4[0x410-0x404];
	ulong	phypll;		/* phy pll control */
	uchar	_pad5[0x420-0x414];
	ulong	phytxctl;	/* phy transmit control */
	uchar	_pad6[0x430-0x424];
	ulong	phyrxctl;	/* phy receive control */
	uchar	_pad7[0x440-0x434];
	ulong	phyivref;	/* phy ivref control */
};

static Ctlr* ctlrs[Nhcis];

static void
addrmapdump(void)
{
	int i;
	ulong ctl, targ, attr, size64k;
	Kwusb *map;
	Usbwin *win;

	if (!Debug)
		return;
	map = (Kwusb *)(soc.ehci + 0x300);
	for (i = 0; i < nelem(map->win); i++) {
		win = &map->win[i];
		ctl = win->ctl;
		if (ctl & Winenable) {
			targ = WINTARG(ctl);
			attr = WINATTR(ctl);
			size64k = WIN64KSIZE(ctl);
			print("usbehci: address map window %d: "
				"targ %ld attr %#lux size %,ld addr %#lux\n",
				i, targ, attr, size64k * 64*1024, win->base);
		}
	}
}

/* assumes ctlr is ilocked */
static void
ctlrreset(Ctlr *ctlr)
{
	int i;
	Eopio *opio;

	opio = ctlr->opio;
	opio->cmd |= Chcreset;
	coherence();
	/* wait for it to come out of reset */
	for(i = 0; i < 100 && opio->cmd & Chcreset; i++)
		delay(1);
	if(i >= 100)
		print("ehci %#p controller reset timed out\n", ctlr->capio);
	/*
	 * Marvell errata FE-USB-340 workaround: 1 << 4 magic:
	 * disable streaming.  Magic 3 (usb host mode) from the linux driver
	 * makes it work.  Ick.
	 */
	opio->usbmode |= 1 << 4 | 3;
	coherence();
}

/*
 * configure window `win' as 256MB dram with attribute `attr' and
 * base address
 */
static void
setaddrwin(Kwusb *kw, int win, int attr, ulong base)
{
	kw->win[win].ctl = Winenable | Targdram << 4 | attr << 8 |
		SIZETO64KSIZE(256*MB) << 16;
	kw->win[win].base = base;
}

static void
ehcireset(Ctlr *ctlr)
{
	int i, amp, txvdd;
	ulong v;
	Eopio *opio;
	Kwusb *kw;

	ilock(ctlr);
	dprint("ehci %#p reset\n", ctlr->capio);
	opio = ctlr->opio;

	kw = (Kwusb *)(soc.ehci + 0x300);
	kw->bic = 0;
	kw->bim = (1<<4) - 1;		/* enable all defined intrs */
	ctlrreset(ctlr);

	/*
	 * clear high 32 bits of address signals if it's 64 bits capable.
	 * This is probably not needed but it does not hurt and others do it.
	 */
	if((ctlr->capio->capparms & C64) != 0){
		dprint("ehci: 64 bits\n");
		opio->seg = 0;
	}

	/* requesting more interrupts per µframe may miss interrupts */
	opio->cmd &= ~Citcmask;
	opio->cmd |= 1 << Citcshift;		/* max of 1 intr. per 125 µs */
	switch(opio->cmd & Cflsmask){
	case Cfls1024:
		ctlr->nframes = 1024;
		break;
	case Cfls512:
		ctlr->nframes = 512;
		break;
	case Cfls256:
		ctlr->nframes = 256;
		break;
	default:
		panic("ehci: unknown fls %ld", opio->cmd & Cflsmask);
	}
	dprint("ehci: %d frames\n", ctlr->nframes);

	/*
	 * set up the USB address map (bridge address decoding)
	 */
	for (i = 0; i < nelem(kw->win); i++)
		kw->win[i].ctl = kw->win[i].base = 0;
	coherence();

	setaddrwin(kw, 0, Attrcs0, 0);
	setaddrwin(kw, 1, Attrcs1, 256*MB);
	coherence();

	if (Debug)
		if (kw->bcs & (1 << 4))
			print("usbehci: not swapping bytes\n");
		else
			print("usbehci: swapping bytes\n");
	addrmapdump();				/* verify sanity */

	kw->pwrctl |= 1 << 0 | 1 << 1;		/* Pu | PuPll */
	coherence();

	/*
	 * Marvell guideline GL-USB-160.
	 */
	kw->phypll |= 1 << 21;		/* VCOCAL_START: PLL calibration */
	coherence();
	microdelay(100);
	kw->phypll &= ~(1 << 21);

	v = kw->phytxctl & ~(017 << 27 | 7);	/* REG_EXT_FS_RCALL & AMP_2_0 */
	switch (m->socrev) {
	default:
		print("usbehci: bad 6281 soc rev %d\n", m->socrev);
		/* fall through */
	case Socreva0:
		amp = 4;
		txvdd = 1;
		break;
	case Socreva1:
		amp = 3;
		txvdd = 3;
		break;
	}
	/* REG_EXT_FS_RCALL_EN | REG_RCAL_START | AMP_2_0 */
	kw->phytxctl = v | 1 << 26 | 1 << 12 | amp;
	coherence();
	microdelay(100);
	kw->phytxctl &= ~(1 << 12);

	v = kw->phyrxctl & ~(3 << 2 | 017 << 4); /* LPF_COEF_1_0 & SQ_THRESH_3_0 */
	kw->phyrxctl = v | 1 << 2 | 8 << 4;

	v = kw->phyivref & ~(3 << 8);		/* TXVDD12 */
	kw->phyivref = v | txvdd << 8;
	coherence();

	ehcirun(ctlr, 0);
	ctlrreset(ctlr);

	iunlock(ctlr);
}

static void
setdebug(Hci*, int d)
{
	ehcidebug = d;
}

static void
shutdown(Hci *hp)
{
	Ctlr *ctlr;
	Eopio *opio;

	ctlr = hp->aux;
	ilock(ctlr);
	ctlrreset(ctlr);

	delay(100);
	ehcirun(ctlr, 0);

	opio = ctlr->opio;
	opio->frbase = 0;
	coherence();
	iunlock(ctlr);
}

static void
findehcis(void)		/* actually just use fixed addresses on sheeva */
{
	int i;
	Ctlr *ctlr;
	static int already = 0;

	if(already)
		return;
	already = 1;

	ctlr = malloc(sizeof(Ctlr));
	if (ctlr == nil)
		panic("ehci: out of memory");
	/* the sheeva's usb 2.0 otg uses a superset of the ehci registers */
	ctlr->capio = (Ecapio *)(soc.ehci + 0x100);
	ctlr->opio  = (Eopio *) (soc.ehci + 0x140);
	dprint("usbehci: port %#p\n", ctlr->capio);

	for(i = 0; i < Nhcis; i++)
		if(ctlrs[i] == nil){
			ctlrs[i] = ctlr;
			break;
		}
	if(i == Nhcis)
		print("ehci: bug: more than %d controllers\n", Nhcis);
}

static int
reset(Hci *hp)
{
	static Lock resetlck;
	int i;
	Ctlr *ctlr;
	Ecapio *capio;

	ilock(&resetlck);
	findehcis();

	/*
	 * Any adapter matches if no hp->port is supplied,
	 * otherwise the ports must match.
	 */
	ctlr = nil;
	for(i = 0; i < Nhcis && ctlrs[i] != nil; i++){
		ctlr = ctlrs[i];
		if(ctlr->active == 0)
		if(hp->port == 0 || hp->port == (uintptr)ctlr->capio){
			ctlr->active = 1;
			break;
		}
	}
	iunlock(&resetlck);
	if(ctlrs[i] == nil || i == Nhcis)
		return -1;

	hp->aux = ctlr;
	hp->port = (uintptr)ctlr->capio;
	hp->irq = IRQ0usb0;
	hp->tbdf = 0;

	capio = ctlr->capio;
	hp->nports = capio->parms & Cnports;

	ddprint("echi: %s, ncc %lud npcc %lud\n",
		capio->parms & 0x10000 ? "leds" : "no leds",
		(capio->parms >> 12) & 0xf, (capio->parms >> 8) & 0xf);
	ddprint("ehci: routing %s, %sport power ctl, %d ports\n",
		capio->parms & 0x40 ? "explicit" : "automatic",
		capio->parms & 0x10 ? "" : "no ", hp->nports);

	ehcireset(ctlr);
	ehcimeminit(ctlr);

	/*
	 * Linkage to the generic HCI driver.
	 */
	ehcilinkage(hp);
	hp->shutdown = shutdown;
	hp->debug = setdebug;
	return 0;
}

void
usbehcilink(void)
{
	addhcitype("ehci", reset);
}
