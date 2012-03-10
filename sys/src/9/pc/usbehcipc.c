/*
 * PC-specific code for
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

static Ctlr* ctlrs[Nhcis];
static int maxehci = Nhcis;

/* Isn't this cap list search in a helper function? */
static void
getehci(Ctlr* ctlr)
{
	int i, ptr, cap, sem;

	ptr = (ctlr->capio->capparms >> Ceecpshift) & Ceecpmask;
	for(; ptr != 0; ptr = pcicfgr8(ctlr->pcidev, ptr+1)){
		if(ptr < 0x40 || (ptr & ~0xFC))
			break;
		cap = pcicfgr8(ctlr->pcidev, ptr);
		if(cap != Clegacy)
			continue;
		sem = pcicfgr8(ctlr->pcidev, ptr+CLbiossem);
		if(sem == 0)
			continue;
		pcicfgw8(ctlr->pcidev, ptr+CLossem, 1);
		for(i = 0; i < 100; i++){
			if(pcicfgr8(ctlr->pcidev, ptr+CLbiossem) == 0)
				break;
			delay(10);
		}
		if(i == 100)
			dprint("ehci %#p: bios timed out\n", ctlr->capio);
		pcicfgw32(ctlr->pcidev, ptr+CLcontrol, 0);	/* no SMIs */
		ctlr->opio->config = 0;
		coherence();
		return;
	}
}

static void
ehcireset(Ctlr *ctlr)
{
	Eopio *opio;
	int i;

	ilock(ctlr);
	dprint("ehci %#p reset\n", ctlr->capio);
	opio = ctlr->opio;

	/*
	 * Turn off legacy mode. Some controllers won't
	 * interrupt us as expected otherwise.
	 */
	ehcirun(ctlr, 0);
	pcicfgw16(ctlr->pcidev, 0xc0, 0x2000);

	/*
	 * reclaim from bios
	 */
	getehci(ctlr);

	/* clear high 32 bits of address signals if it's 64 bits capable.
	 * This is probably not needed but it does not hurt and others do it.
	 */
	if((ctlr->capio->capparms & C64) != 0){
		dprint("ehci: 64 bits\n");
		opio->seg = 0;
		coherence();
	}

	if(ehcidebugcapio != ctlr->capio){
		opio->cmd |= Chcreset;	/* controller reset */
		coherence();
		for(i = 0; i < 100; i++){
			if((opio->cmd & Chcreset) == 0)
				break;
			delay(1);
		}
		if(i == 100)
			print("ehci %#p controller reset timed out\n", ctlr->capio);
	}

	/* requesting more interrupts per µframe may miss interrupts */
	opio->cmd &= ~Citcmask;
	opio->cmd |= 1 << Citcshift;		/* max of 1 intr. per 125 µs */
	coherence();
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
	int i;
	Ctlr *ctlr;
	Eopio *opio;

	ctlr = hp->aux;
	ilock(ctlr);
	opio = ctlr->opio;
	opio->cmd |= Chcreset;		/* controller reset */
	coherence();
	for(i = 0; i < 100; i++){
		if((opio->cmd & Chcreset) == 0)
			break;
		delay(1);
	}
	if(i >= 100)
		print("ehci %#p controller reset timed out\n", ctlr->capio);
	delay(100);
	ehcirun(ctlr, 0);
	opio->frbase = 0;
	iunlock(ctlr);
}

static void
scanpci(void)
{
	static int already = 0;
	int i;
	ulong io;
	Ctlr *ctlr;
	Pcidev *p;
	Ecapio *capio;

	if(already)
		return;
	already = 1;
	p = nil;
	while ((p = pcimatch(p, 0, 0)) != nil) {
		/*
		 * Find EHCI controllers (Programming Interface = 0x20).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb)
			continue;
		switch(p->ccrp){
		case 0x20:
			io = p->mem[0].bar & ~0x0f;
			break;
		default:
			continue;
		}
		if(0 && p->vid == Vintel && p->did == 0x3b34) {
			print("usbehci: ignoring known bad ctlr %#ux/%#ux\n",
				p->vid, p->did);
			continue;
		}
		if(io == 0){
			print("usbehci: %x %x: failed to map registers\n",
				p->vid, p->did);
			continue;
		}
		if(p->intl == 0xff || p->intl == 0) {
			print("usbehci: no irq assigned for port %#lux\n", io);
			continue;
		}
		dprint("usbehci: %#x %#x: port %#lux size %#x irq %d\n",
			p->vid, p->did, io, p->mem[0].size, p->intl);

		ctlr = malloc(sizeof(Ctlr));
		if (ctlr == nil)
			panic("usbehci: out of memory");
		ctlr->pcidev = p;
		capio = ctlr->capio = vmap(io, p->mem[0].size);
		ctlr->opio = (Eopio*)((uintptr)capio + (capio->cap & 0xff));
		pcisetbme(p);
		pcisetpms(p, 0);
		for(i = 0; i < Nhcis; i++)
			if(ctlrs[i] == nil){
				ctlrs[i] = ctlr;
				break;
			}
		if(i >= Nhcis)
			print("ehci: bug: more than %d controllers\n", Nhcis);

		/*
		 * currently, if we enable a second ehci controller on zt
		 * systems w x58m motherboard, we'll wedge solid after iunlock
		 * in init for the second one.
		 */
		if (i >= maxehci) {
			print("usbehci: ignoring controllers after first %d, "
				"at %#p\n", maxehci, io);
			ctlrs[i] = nil;
		}
	}
}

static int
reset(Hci *hp)
{
	int i;
	char *s;
	Ctlr *ctlr;
	Ecapio *capio;
	Pcidev *p;
	static Lock resetlck;

	s = getconf("*maxehci");
	if (s != nil && s[0] >= '0' && s[0] <= '9')
		maxehci = atoi(s);
	if(maxehci == 0 || getconf("*nousbehci"))
		return -1;

	ilock(&resetlck);
	scanpci();

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
	if(i >= Nhcis || ctlrs[i] == nil)
		return -1;

	p = ctlr->pcidev;
	hp->aux = ctlr;
	hp->port = (uintptr)ctlr->capio;
	hp->irq = p->intl;
	hp->tbdf = p->tbdf;

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
