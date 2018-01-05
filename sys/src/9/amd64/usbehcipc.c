/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

	ilock(&ctlr->l);
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
	iunlock(&ctlr->l);
}

static void
setdebug(Hci *hp, int d)
{
	ehcidebug = d;
}

static void
shutdown(Hci *hp)
{
	int i;
	Ctlr *ctlr;
	Eopio *opio;

	ctlr = hp->Hciimpl.aux;
	ilock(&ctlr->l);
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
	iunlock(&ctlr->l);
}

static void
scanpci(void)
{
	static int already = 0;
	int i;
	uint32_t io;
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
		//if(0 && p->vid == Vintel && p->did == 0x3b34) {
		//	print("usbehci: ignoring known bad ctlr %#x/%#x\n",
		//		p->vid, p->did);
		//	continue;
		//}
		if(io == 0){
			print("usbehci: %x %x: failed to map registers\n",
				p->vid, p->did);
			continue;
		}
		if(p->intl == 0xff || p->intl == 0) {
			print("usbehci: no irq assigned for port %#lx\n", io);
			continue;
		}
		dprint("usbehci: %#x %#x: port %#lx size %#x irq %d\n",
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

	}
}

static int
reset(Hci *hp)
{
	int i;
	//char *s;
	Ctlr *ctlr;
	Ecapio *capio;
	Pcidev *p;
	static Lock resetlck;

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
		if(hp->ISAConf.port == 0 || hp->ISAConf.port == (uintptr)ctlr->capio){
			ctlr->active = 1;
			break;
		}
	}
	iunlock(&resetlck);
	if(i >= Nhcis || ctlrs[i] == nil)
		return -1;

	p = ctlr->pcidev;
	hp->Hciimpl.aux = ctlr;
	hp->ISAConf.port = (uintptr)ctlr->capio;
	hp->ISAConf.irq = p->intl;
	hp->tbdf = p->tbdf;

	capio = ctlr->capio;
	hp->nports = capio->parms & Cnports;

	ddprint("echi: %s, ncc %lu npcc %lu\n",
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
	hp->Hciimpl.shutdown = shutdown;
	hp->Hciimpl.debug = setdebug;
	return 0;
}

void
usbehcilink(void)
{
	addhcitype("ehci", reset);
}
