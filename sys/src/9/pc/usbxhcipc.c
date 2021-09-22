/*
 * PC-specific code for
 * USB eXtensible Host Controller Interface (XHCI) driver
 * Super-speed USB 3.x.
 *
 * currently just a stub to shut down any XHCI controllers found
 * so that they can't generate interrupts.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"../port/usb.h"
#include	"../port/portusbxhci.h"
#include	"usbxhci.h"

static Ctlr* ctlrs[Nhcis];
static int maxxhci = Nhcis;

static void
stop(Eopio *opio)
{
	opio->cmd &= ~Crun;
	coherence();
	delay(20);			/* 16 ms. max says xhci 1.1 spec */
}

static void
nointrs(Pcidev *p, ulong io)
{
	Ecapio *capio;
	Eopio *opio;

	capio = vmap(io, p->mem[0].size);
	if (capio == nil)
		panic("can't vmap usbxhci regs at %#p", io);
	opio = (Eopio*)((uintptr)capio + (capio->cap & 0xff));
	/* must stop before reset */
	stop(opio);
	opio->cmd |= Chcreset;		/* controller reset */
	coherence();
	delay(200);
	opio->cmd &= ~Inte;
	coherence();
	stop(opio);
	vunmap(capio, p->mem[0].size);
	pciclrbme(p);
}

static void
scanpci(void)
{
	static int already = 0;
	ulong io;
	Pcidev *p;

	if(already)
		return;
	already = 1;
	p = nil;
	while ((p = pcimatch(p, 0, 0)) != nil) {
		/*
		 * Find XHCI controllers (Programming Interface = 0x30).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb)
			continue;
		switch(p->ccrp){
		case 0x30:
			io = p->mem[0].bar & ~0x0f;
			break;
		default:
			continue;
		}
		if(io == 0){
			print("usbxhci: %ux/%ux: failed to map registers\n",
				p->vid, p->did);
			continue;
		}
		print("usbxhci: shutting off usb 3 ctlr %#ux/%#ux\n",
			p->vid, p->did);
		nointrs(p, io);
	}
}

static int
reset(Hci *)
{
	char *s;
	static Lock resetlck;

	s = getconf("*maxxhci");
	if (s != nil && s[0] >= '0' && s[0] <= '9')
		maxxhci = atoi(s);
	if(maxxhci == 0 || getconf("*nousbxhci"))
		return -1;
	ilock(&resetlck);
	scanpci();
	iunlock(&resetlck);
	return -1;
}

void
usbxhcilink(void)
{
	addhcitype("xhci", reset);
}
