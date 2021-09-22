/*
 * USB EHCI debug port as console. WIP. Untested.
 *
 * ehci must avoid reset of the ctlr whose capio is at
 * ehcidebugcapio, if not nil.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include	"../port/usb.h"
#include "../port/portusbehci.h"
#include "usbehci.h"

typedef struct Dbctlr Dbctlr;

enum
{
	Pdebugport	= 0x0A,		/* ehci debug port PCI cap. */

	Ddebug		= 0x0A,		/* debug descriptor type */
	Debugmode	= 6,		/* debug mode feature */
	Daddr		= 127,		/* debug device address */

	Tokin		= 0x69 << Ptokshift,
	Tokout		= 0xE1 << Ptokshift,
	Toksetup	= 0x2D << Ptokshift,
	Data0		= 0xC3 << Pspidshift,
	Data1		= 0x4B << Pspidshift,
};

struct Dbctlr
{
	Lock;

	Pcidev*	pcidev;
	Ecapio*	capio;		/* Capability i/o regs */
	Eopio*	opio;		/* Operational i/o regs */
	Edbgio*	dbgio;		/* Debug port i/o regs */

	/* software */
	int	port;		/* port number */
	int	epin;		/* input endpoint address */
	int	epout;		/* output endpoint address */

	int	sc;		/* start char in buf */
	int	ec;		/* end char in buf */
	char	buf[8];		/* read buffer */
	Queue*	iq;
	Queue*	oq;
};

Ecapio* ehcidebugcapio;

static int debug;
static Dbctlr ctlr;		/* one console at most */

static void
dbehcirun(Dbctlr *ctlr, int on)
{
	int i;
	Eopio *opio;

	dprint("usbcons: %s\n", on ? "starting" : "halting");
	opio = ctlr->opio;
	if(on)
		opio->cmd |= Crun;
	else
		opio->cmd = Cstop;
	for(i = 0; i < 100; i++)
		if(on == 0 && (opio->sts & Shalted) != 0)
			break;
		else if(on != 0 && (opio->sts & Shalted) == 0)
			break;
		else
			delay(1);
	if(i == 100)
		print("usbcons: %s cmd timed out\n", on ? "run" : "halt");
	dprint("usbcons: sts %#ulx\n", opio->sts);
}

static int
ucio(Edbgio *dbgio, int pid, void *data, int len, int iswrite)
{
	static char *wstr[] = {"read", "write" };
	int i;
	char *b;

	b = data;
	if(len > 8)
		panic("usbcons ucio bug");
	if(debug > 1){
		dprint("usbcons: %s: pid %#ux [%d] ", wstr[iswrite], pid, len);
		if(iswrite){
			for(i = 0; i < len; i++)
				dprint("%02ux ", b[i]);
		}
	}
	dbgio->csw = (dbgio->csw & ~Clen) | len;
	dbgio->pid = pid;
	if(iswrite){
		if(len > 0)
			memmove(dbgio->data, data, len);
		dbgio->csw |= Cwrite;
	}else
		dbgio->csw &= ~Cwrite;

	dbgio->csw |= Cgo;
	for(i = 0; (dbgio->csw&Cdone) == 0; i++){
		if(i > 10000)
			delay(1);
		if((i % 10000) == 9999)
			dprint("usbcons: waiting for io\n");
	}
	dbgio->csw |= Cdone;	/* acknowledge */

	if((dbgio->csw & Cfailed) != 0){
		len = -1;
		dprint("usbcons: transaction error csw %#ulx\n", dbgio->csw);
	}else
		if(iswrite == 0 && len > 0){
			len = dbgio->csw & Clen;
			if(len > 0)
				memmove(data, dbgio->data, len);
		}
	/*
	 * BUG: if the received pid is not the sent pid
	 * that's a nak. we should probably report that so that
	 * the caller may retry or abort.
	 */
	if(debug > 1){
		dprint("-> [%d] ", len);
		if(iswrite == 0){
			for(i = 0; i < len; i++)
				dprint("%02ux ", b[i]);
		}
		dprint(" csw %#ulx\n", dbgio->csw);
	}

	return len;
}

static int
uccmd(Edbgio *dbgio, int type, int req, int val, int idx, void *data, int cnt)
{
	uchar buf[Rsetuplen];

	assert(cnt >= 0 && cnt <= nelem(dbgio->data));
	dprint("usbcons: cmd t %#x r %#x v %#x i %#x c %d\n",
		type, req, val, idx, cnt);
	buf[Rtype] = type;
	buf[Rreq] = req;
	PUT2(buf+Rvalue, val);
	PUT2(buf+Rindex, idx);
	PUT2(buf+Rcount, cnt);
	if(ucio(dbgio, Data0|Toksetup, buf, Rsetuplen, 1) < 0)
		return -1;
	if((type&Rd2h) == 0 && cnt > 0)
		panic("out debug command with data");
	return ucio(dbgio, Data1|Tokin, data, cnt, 0);
}

static ulong
uctoggle(Edbgio *dbgio)
{
	return (dbgio->pid ^ Ptoggle) & Ptogglemask;
}

static int
ucread(Dbctlr *ctlr, void *data, int cnt)
{
	Edbgio *dbgio;
	int r;

	ilock(ctlr);
	dbgio = ctlr->dbgio;
	dbgio->addr = (Daddr << Adevshift) | (ctlr->epin << Aepshift);
	r = ucio(dbgio, uctoggle(dbgio)|Tokin, data, cnt, 0);
	iunlock(ctlr);
	return r;
}

static int
ucwrite(Dbctlr *ctlr, void *data, int cnt)
{
	Edbgio *dbgio;
	int r;

	ilock(ctlr);
	dbgio = ctlr->dbgio;
	dbgio->addr = (Daddr << Adevshift) | (ctlr->epout << Aepshift);
	r = ucio(dbgio, uctoggle(dbgio)|Tokout, data, cnt, 1);
	iunlock(ctlr);
	return r;
}

/*
 * Set the device address to 127 and enable it.
 */
static void
ucconfig(Dbctlr *ctlr)
{
	uchar desc[4];
	Edbgio *dbgio;
	int id;
	int r;

	dbgio = ctlr->dbgio;
	for(id = 0; id <= Devmax; id++){
		dbgio->addr = (id << Adevshift) | (0 << Aepshift);
		r = uccmd(dbgio, Rd2h|Rstd|Rdev, Rgetdesc, Ddebug << 8, 0,
			desc, sizeof(desc));
		if(r > 0)
			break;
	}
	if(id > Devmax){
		print("usbcons: debug device: can't get address\n");
		dbgio->csw &= ~(Cowner|Cbusy);
		return;
	}
	dprint("usbcons: device addr %d\n", id);
	if(id != Daddr)
		if(uccmd(dbgio, Rh2d|Rstd|Rdev, Rsetaddr, Daddr, 0, nil, 0) < 0){
			print("usbcons: debug device: can't set address\n");
			dbgio->csw &= ~(Cowner|Cbusy);
			return;
		}

	dbgio->addr = (Daddr << Adevshift) | (0 << Aepshift);
	if(uccmd(dbgio, Rh2d|Rstd|Rdev, Rsetfeature, Debugmode, 0, nil, 0) < 0){
		print("usbcons: debug device: can't set debug mode\n");
		dbgio->csw &= ~(Cowner|Cbusy);
	}
	/* from now on we are using these endpoints and ont the setup one */
	ctlr->epin = desc[2];
	ctlr->epout = desc[3];
}

static void
portreset(Dbctlr *ctlr, int port)
{
	Eopio *opio;
	ulong s;
	int i;

	opio = ctlr->opio;
	s = opio->portsc[port-1];
	dprint("usbcons %#p port %d reset; sts %#ulx\n", ctlr->capio, port, s);
	s &= ~(Psenable|Psreset);
	opio->portsc[port-1] = s|Psreset;
	for(i = 0; i < 10; i++){
		delay(10);
		if((opio->portsc[port-1] & Psreset) == 0)
			break;
	}
	opio->portsc[port-1] &= ~Psreset;
	delay(10);
}

static void
ucstart(Dbctlr *ctlr)
{
	Eopio *opio;
	Edbgio *dbgio;
	int i;
	ulong s;
	int port;

	ilock(ctlr);
	if(ehcidebugcapio != nil){
		iunlock(ctlr);
		return;
	}
	/* reclaim port */
	dbgio = ctlr->dbgio;
	dbgio->csw &= ~Cenable;
	dbgio->csw |= Cowner|Cbusy;

	opio = ctlr->opio;
	opio->cmd &= ~(Chcreset|Ciasync|Cpse|Case);
	opio->config = Callmine;	/* reclaim all ports */
	dbehcirun(ctlr, 1);

	ctlr->port = (ctlr->capio->parms >> Cdbgportshift) & Cdbgportmask;
	port = ctlr->port;

	for(i = 0; i < 200; i++){
		s = opio->portsc[port-1];
		if(s & (Psstatuschg | Pschange))
			opio->portsc[port-1] = s;
		if(s & Pspresent){
			portreset(ctlr, port);
			break;
		}
	}
	if((opio->portsc[port-1] & Pspresent) == 0 || i == 200){
		print("usbcons: no debug device\n");
		dbgio->csw &= ~(Cowner|Cbusy);
		iunlock(ctlr);
		return;
	}
	ehcidebugcapio = ctlr->capio;	/* ehci must avoid reset */
	dbgio->csw |= Cenable;
	opio->portsc[port-1] &= ~Psenable;
	iunlock(ctlr);

	ucconfig(ctlr);
}

static void
ucreset(Dbctlr *ctlr)
{
	Eopio *opio;
	int i;

	/*
	 * Turn off legacy mode.
	 */
	dbehcirun(ctlr, 0);
	pcicfgw16(ctlr->pcidev, 0xc0, 0x2000);

	/* clear high 32 bits of address signals if it's 64 bits capable.
	 * This is probably not needed but it does not hurt and others do it.
	 */
	opio = ctlr->opio;
	if((ctlr->capio->capparms & C64) != 0){
		dprint("ehci: 64 bits\n");
		opio->seg = 0;
	}

	opio->cmd |= Chcreset;	/* controller reset */
	for(i = 0; i < 100; i++){
		if((opio->cmd & Chcreset) == 0)
			break;
		delay(1);
	}
	if(i == 100)
		print("usbcons: controller reset timed out\n");

}

static void
ucinit(Pcidev *p, uint ptr)
{
	uintptr io;
	uint off;
	Ecapio *capio;

	io = p->mem[0].bar & ~0xF;
	if(io == 0){
		print("usbcons: failed to map registers\n");
		return;
	}
	off = pcicfgr16(p, ptr+2) & 0xFFF;
	capio = ctlr.capio = vmap(io, p->mem[0].size);
	ctlr.opio = (Eopio*)((uintptr)capio + (capio->cap & 0xFF));
	ctlr.dbgio = (Edbgio*)((uintptr)capio + off);
	ctlr.pcidev = p;
	pcisetbme(p);
	pcisetpms(p, 0);

	if((ctlr.dbgio->csw & Cbusy) != 0){
		print("usbcons: debug port already in use\n");
		return;
	}
	print("usbcons: port %#p: ehci debug port\n", ctlr.dbgio);
	ucreset(&ctlr);
	ucstart(&ctlr);
}

static void
kickout(void *)
{
	char buf[8];
	int n;

	while((n = qconsume(ctlr.oq, buf, sizeof(buf))) > 0)
		ucwrite(&ctlr, buf, n);
}

static void
usbclock(void)
{
	uchar buf[8];
	int n;

	while((n = ucread(&ctlr, buf, sizeof(buf))) > 0)
		qproduce(ctlr.iq, buf, n);
}

/*
 * Polling interface.
 */
int
usbgetc(void)
{
	int nr;

	if(ehcidebugcapio == nil)
		return -1;
	if(ctlr.sc == ctlr.ec){
		ctlr.sc = ctlr.ec = 0;
		nr = ucread(&ctlr, ctlr.buf, sizeof(ctlr.buf));
		if(nr > 0)
			ctlr.ec += nr;
	}
	if(ctlr.sc < ctlr.ec)
		return ctlr.buf[ctlr.sc++];

	return -1;
}

void
usbputc(int c)
{
	char buf[1];

	if(ehcidebugcapio == nil)
		return;
	buf[0] = c;
	ucwrite(&ctlr, buf, 1);
}

void
usbputs(char *s, int n)
{
	int nw;

	if(ehcidebugcapio == nil)
		return;

	for(; n > 0; n -= nw){
		nw = n;
		if(nw > 8)
			nw = 8;
		nw = ucwrite(&ctlr, s, nw);
		if(nw <= 0)
			break;
		s += nw;
	}
}

/*
 * Queue interface
 * The debug port does not seem to
 * issue interrupts for us. We must poll for completion and
 * also to check for input.
 * By now this might suffice but it's probably wrong.
 */
void
usbconsoleinit(void)
{
	if(ehcidebugcapio == nil)
		return;

	ctlr.iq = qopen(8*1024, 0, nil, nil);
	ctlr.oq = qopen(8*1024, 0, kickout, nil);
	if(ctlr.iq == nil || ctlr.oq == nil){
		qfree(ctlr.iq);
		qfree(ctlr.oq);
		return;
	}
	kbdq = ctlr.iq;
	serialoq = ctlr.oq;
	addclock0link(usbclock, 50);
}

/*
 * Scan the bus and enable the first debug port found.
 * Perhaps we should search all for a ctlr specified by port
 * e.g., console=usb port=xxxx
 * or by some other means.
 */
void
usbconsole(void)
{
	static int already = 0;
	uint ptr;
	Pcidev *p;
	char *s;

	if(already++ != 0)
		return;
	if((s = getconf("console")) == nil || strncmp(s, "usb", 3) != 0)
		return;

	p = nil;
	while ((p = pcimatch(p, 0, 0)) != nil) {
		/*
		 * Find EHCI controllers (Programming Interface = 0x20).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb || p->ccrp != 0x20)
			continue;
		if((pcicfgr16(p, PciPSR) & 0x0010) == 0)
			continue;
		/*
		 * We have extended caps. search list for ehci debug port.
		 */
		ptr = pcicfgr32(p, 0x34);
		while(ptr >= 0x40 && (ptr & ~0xFC) == 0){
			if(pcicfgr8(p, ptr) == Pdebugport){
				ucinit(p, ptr);
				return;
			}
			ptr = pcicfgr8(p, ptr+1);
		}
	}
}
