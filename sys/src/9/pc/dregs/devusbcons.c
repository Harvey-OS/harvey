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

/*
 * USB debug port support for Net20DC Ajays debug cable.
 * Still WIP.
 *
 * ehci must avoid reset of the ctlr whose capio is at
 * ehcidebugcapio, if not nil.
 *
 * Note that using this requires being able to use vmap() to map ehci
 * registers for I/O. This means that MMU code must be initialized.
 *
 * CAUTION: The debug device has two ports but they are NOT
 * the same. If you put the "USB 2.0 debug cable" text in front of you
 * the port to the right is the one to attach to the debug port. The port
 *  on the left is the one to attach to the host used for debugging.
 * Thigs are worse. Power for the "debug cable" is taken only from the
 * port attached to the target machine.
 * This means the device will malfunction unless you do the plug/unplug
 * start serial dance in the wrong order.
 *
 * For the machine I tried, I plug the debugged port and cold start the
 * machine.
 * Then I do this each time I want to start again:
 *	- unplug the debugging port
 *	- warm reboot the debugged machine
 *	- wait until bootfile prompt (device has its power)
 *	- plug the debugging port
 *	- start usb/serail
 *	- con /dev/eiaUX/data
 *
 * If the debug device seems to be stuck I unplug the debugged port
 * to remove power from the device and make it restart.
 *
 * This is so clumsy that the bug might be ours, but I wouldn't bet.
 *
 * The usb/serial driver talking to us must be sure that maxpkt is
 * set to 8 bytes. Otherwise, the debug device hangs. It's picky this cable.
 *
 * There's also some problem in the usb/serial kludge for con'ing this:
 * we miss some messages. However, this could be due to the debugging
 * machine being too slow.
 */

typedef struct Ctlr Ctlr;

enum
{
	Pdebugport	= 0x0A,		/* ehci debug port PCI cap. */

	Ddebug		= 0x0A,		/* debug descriptor type */
	Ddebuglen	= 4,		/* debug descriptor length */
	Debugmode	= 6,		/* debug mode feature */
	Daddr		= 127,		/* debug device address */

	Tokin		= 0x69,
	Tokout		= 0xE1,
	Toksetup	= 0x2D,
	Ack		= 0xD2,
	Nak		= 0x5A,
	Nyet		= 0x96,
	Stall		= 0x1E,

	Data0		= 0xC3 << Pspidshift,
	Data1		= 0x4B << Pspidshift,

	Read		= 0,		/* mode for ucio() */
	Write		= 1,
};

struct Ctlr
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
	Queue*	iq;		/* input queue */
	Queue*	oq;		/* output queue */
	int	consid;		/* console id */
};

#undef dprint
#define dprint	if(debug)iprint

static int debug = 1;
static Ctlr ctlr;		/* one console at most */

static void
ehcirun(Ctlr *ctlr, int on)
{
	int i;
	Eopio *opio;

	dprint("usbcons: %s", on ? "starting" : "halting");
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
		dprint("usbcons: %s cmd timed out\n", on ? "run" : "halt");
	dprint(" sts %#ulx\n", opio->sts);
}

static int
rpid(ulong csw)
{
	return (csw >> Prpidshift) & Prpidmask;
}

static int
spid(ulong csw)
{
	return (csw >> Pspidshift) & Pspidmask;
}

/*
 * Perform I/O.
 * This returns -1 upon errors but -2 upon naks after (re)trying.
 * The caller could probably retry even more upon naks.
 */
static int
ucio(Edbgio *dbgio, int pid, void *data, int len, int iswrite, int tmout)
{
	static char *wstr[] = {"rd", "wr" };
	int i;
	uchar *b;
	int ntries;
	int once;
	int ppid;

	b = data;
	if(len > 8)
		panic("usbcons ucio bug. len > 0");
	if(len > 0 && data == nil)
		panic("usbcons ucio bug. len but not data");
	ntries = 1000;
	once = 1;
Again:
	dbgio->csw = (dbgio->csw & ~Clen) | len;
	dbgio->pid = pid;
	if(once && debug > 1){
		once = 0;
		dprint("usbcons: %s: csw %#ulx pid %#ux [%d] ",
			wstr[iswrite], dbgio->csw, pid, len);
		if(iswrite){
			for(i = 0; i < len; i++)
				dprint("%02ux ", b[i]);
		}
	}
	memset(dbgio->data, 0, sizeof(dbgio->data));
	if(iswrite){
		if(len > 0)
			memmove(dbgio->data, data, len);
		dbgio->csw |= Cwrite;
	}else
		dbgio->csw &= ~Cwrite;
	dbgio->csw |= Cgo;
	for(i = 0; (dbgio->csw&Cdone) == 0; i++)
		if(tmout != 0 && i > 100000)
			return -1;
	dbgio->csw |= Cdone;	/* acknowledge */

	if((dbgio->csw & Cfailed) != 0){
		dprint(" err csw %#ulx\n", dbgio->csw);
		return -1;
	}

	ppid = rpid(dbgio->pid);
	if((ppid == Nak || ppid == Nyet) && --ntries > 0){
		microdelay(10);
		goto Again;
	}
	if(ntries == 0)
		dprint(" naks");
	if(ppid != Ack && ppid != spid(dbgio->pid)){
		dprint(" bad pid %#x\n", ppid);
		len = -1;
		if(ppid == Nak)
			len = -2;
	}
	if(iswrite == 0 && len > 0){
		if((dbgio->csw & Clen) < len)
			len = dbgio->csw & Clen;
		if(len > 0)
			memmove(data, dbgio->data, len);
	}
	if(debug > 1){
		dprint("-> [%d] ", len);
		if(iswrite == 0)
			for(i = 0; i < len; i++)
				dprint("%02ux ", b[i]);
		dprint(" csw %#ulx\n", dbgio->csw);
	}

	return len;
}

/*
 * BUG: This is not a generic usb cmd tool.
 * If you call this be sure it works for the type of call you are making.
 * This is just for what this driver uses. Compare this with the
 * general purpose ehci control transfer dance.
 */
static int
uccmd(Edbgio *dbgio, int type, int req, int val, int idx, void *data, int cnt)
{
	uchar buf[Rsetuplen];
	int r;

	assert(cnt >= 0 && cnt <= nelem(dbgio->data));
	dprint("usbcons: cmd t %#x r %#x v %#x i %#x c %d\n",
		type, req, val, idx, cnt);
	buf[Rtype] = type;
	buf[Rreq] = req;
	PUT2(buf+Rvalue, val);
	PUT2(buf+Rindex, idx);
	PUT2(buf+Rcount, cnt);
	if(ucio(dbgio, Data0|Toksetup, buf, Rsetuplen, Write, 100) < 0)
		return -1;

	if((type&Rd2h) == 0 && cnt > 0)
		panic("out debug command with data");

	r = ucio(dbgio, Data1|Tokin, data, cnt, Read, 100);
	if((type&Rd2h) != 0)
		ucio(dbgio, Data1|Tokout, nil, 0, Write, 100);
	return r;
}

static ulong
uctoggle(Edbgio *dbgio)
{
	return (dbgio->pid ^ Ptoggle) & Ptogglemask;
}

static int
ucread(Ctlr *ctlr, void *data, int cnt)
{
	Edbgio *dbgio;
	int r;

	ilock(ctlr);
	dbgio = ctlr->dbgio;
	dbgio->addr = (Daddr << Adevshift) | (ctlr->epin << Aepshift);
	r = ucio(dbgio, uctoggle(dbgio)|Tokin, data, cnt, Read, 10);
	if(r < 0)
		uctoggle(dbgio);	/* leave toggle as it was */
	iunlock(ctlr);
	return r;
}

static int
ucwrite(Ctlr *ctlr, void *data, int cnt)
{
	Edbgio *dbgio;
	int r;

	ilock(ctlr);
	dbgio = ctlr->dbgio;
	dbgio->addr = (Daddr << Adevshift) | (ctlr->epout << Aepshift);
	r = ucio(dbgio, uctoggle(dbgio)|Tokout, data, cnt, Write, 10);
	if(r < 0)
		uctoggle(dbgio);	/* leave toggle as it was */
	iunlock(ctlr);
	return r;
}

/*
 * Set the device address to 127 and enable it.
 * The device might have the address hardwired to 127.
 * We try to set it up in any case but ignore most errors.
 */
static int
ucconfigdev(Ctlr *ctlr)
{
	uchar desc[8];
	Edbgio *dbgio;
	int r;

	dbgio = ctlr->dbgio;
	dprint("usbcons: setting up device address to %d\n", Daddr);

	dbgio->addr = (0 << Adevshift) | (0 << Aepshift);
	if(uccmd(dbgio, Rh2d|Rstd|Rdev, Rsetaddr, Daddr, 0, nil, 0) < 0)
		print("usbcons: debug device: can't set address to %d\n", Daddr);
	else
		dprint("usbcons: device address set to %d\n", Daddr);

	dbgio->addr = (Daddr << Adevshift) | (0 << Aepshift);

	dprint("usbcons: reading debug descriptor\n");
	r = uccmd(dbgio, Rd2h|Rstd|Rdev, Rgetdesc, Ddebug << 8, 0, desc, 4);
	if(r < Ddebuglen || desc[1] != Ddebug){
		print("usbcons: debug device: can't get debug descriptor\n");
		dbgio->csw &= ~(Cowner|Cbusy);
		return -1;
	}

	dprint("usbcons: setting up debug mode\n");
	if(uccmd(dbgio, Rh2d|Rstd|Rdev, Rsetfeature, Debugmode, 0, nil, 0) < 0){
		print("usbcons: debug device: can't set debug mode\n");
		dbgio->csw &= ~(Cowner|Cbusy);
		return -1;
	}
	ctlr->epin = desc[2] & ~0x80;	/* clear direction bit from ep. addr */;
	ctlr->epout = desc[3];
	print("#u/usb/ep%d.0: ehci debug port: in ep%d.%d out: ep%d.%d\n",
		Daddr, Daddr, ctlr->epin, Daddr, ctlr->epout);
	return 0;
}

/*
 * Ctlr already ilocked.
 */
static void
portreset(Ctlr *ctlr, int port)
{
	Eopio *opio;
	ulong s;
	int i;

	opio = ctlr->opio;
	s = opio->portsc[port-1];
	dprint("usbcons %#p port %d reset: sts %#ulx\n", ctlr->capio, port, s);
	s &= ~(Psenable|Psreset);
	opio->portsc[port-1] = s|Psreset;
	for(i = 0; i < 10; i++){
		delay(10);
		if((opio->portsc[port-1] & Psreset) == 0)
			break;
	}
	opio->portsc[port-1] &= ~Psreset;
	delay(50);
	dprint("usbcons %#p port %d after reset: sts %#ulx\n", ctlr->capio, port, s);
}

/*
 * Ctlr already ilocked.
 */
static void
portenable(Ctlr *ctlr, int port)
{
	Eopio *opio;
	ulong s;

	opio = ctlr->opio;
	s = opio->portsc[port-1];
	dprint("usbcons: port %d enable: sts %#ulx\n", port, s);
	if(s & (Psstatuschg | Pschange))
		opio->portsc[port-1] = s;
	opio->portsc[port-1] |= Psenable;
	delay(100);
	dprint("usbcons: port %d after enable: sts %#ulx\n", port, s);
}

static int
ucattachdev(Ctlr *ctlr)
{
	Eopio *opio;
	Edbgio *dbgio;
	int i;
	ulong s;
	int port;

	ilock(ctlr);
	if(ehcidebugcapio != nil){
		iunlock(ctlr);
		return -1;
	}

	/* reclaim port */
	dbgio = ctlr->dbgio;
	dbgio->csw |= Cowner;
	dbgio->csw &= ~(Cenable|Cbusy);

	opio = ctlr->opio;
	opio->cmd &= ~(Chcreset|Ciasync|Cpse|Case);
	opio->config = Callmine;	/* reclaim all ports */
	ehcirun(ctlr, 1);
	delay(100);
	ctlr->port = (ctlr->capio->parms >> Cdbgportshift) & Cdbgportmask;
	port = ctlr->port;
	if(port < 1 || port > (ctlr->capio->parms & Cnports)){
		print("usbcons: debug port out of range\n");
		dbgio->csw &= ~(Cowner|Cbusy);
		iunlock(ctlr);
		return -1;
	}
	dprint("usbcons: debug port: %d\n", port);

	opio->portsc[port-1] = Pspower;
	for(i = 0; i < 200; i++){
		s = opio->portsc[port-1];
		if(s & (Psstatuschg | Pschange)){
			opio->portsc[port-1] = s;
			dprint("usbcons: port sts %#ulx\n", s);
		}
		if(s & Pspresent){
			portreset(ctlr, port);
			break;
		}
		delay(1);
	}
	if((opio->portsc[port-1] & Pspresent) == 0 || i == 200){
		print("usbcons: no debug device (attached to another port?)\n");
		dbgio->csw &= ~(Cowner|Cbusy);
		iunlock(ctlr);
		return -1;
	}

	ehcidebugcapio = ctlr->capio;	/* this ehci must avoid reset */
	ehcidebugport = port;		/* and this port must not be available */


	dbgio->csw |= Cowner|Cenable|Cbusy;
	opio->portsc[port-1] &= ~Psenable;
	delay(100);
	ehcirun(ctlr, 0);

//	portenable(ctlr, port);
//	dbgio->csw |= Cowner|Cenable|Cbusy;

	delay(100);

	iunlock(ctlr);
	return 0;
}

static int
ucreset(Ctlr *ctlr)
{
	Eopio *opio;
	int i;

	dprint("ucreset\n");
	/*
	 * Turn off legacy mode.
	 */
	ehcirun(ctlr, 0);
	pcicfgw16(ctlr->pcidev, 0xc0, 0x2000);

	/* clear high 32 bits of address signals if it's 64 bits.
	 * This is probably not needed but it does not hurt.
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
	if(i == 100){
		print("usbcons: controller reset timed out\n");
		return -1;
	}
	dprint("ucreset done\n");
	return 0;
}

static int
ucinit(Pcidev *p, uint ptr)
{
	uintptr io;
	uint off;
	Ecapio *capio;

	io = p->mem[0].bar & ~0xF;
	if(io == 0){
		print("usbcons: failed to map registers\n");
		return -1;
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
		return -1;
	}
	print("usbcons: port %#p: ehci debug port\n", ctlr.dbgio);


	if(ucreset(&ctlr) < 0 || ucattachdev(&ctlr) < 0 || ucconfigdev(&ctlr) < 0)
		return -1;

	return 0;
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

/*
 * Put 8 chars at a time.
 * Ignore errors (this device seems to be flaky).
 * Some times (for some packets) the device keeps on
 * sending naks. This does not seem to depend on toggles or
 * pids or packet content. It's supposed we don't need to send
 * unstalls here. But it really looks like we do need then.
 * Time to use a sniffer to see what windows does
 * with the device??
 */
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
		ucwrite(&ctlr, s, nw);
		s += nw;
	}
}

static Lock lck;

static void
usbclock(void)
{
	char buf[80];
	int n;

	lock(&lck);
	usbputs(".", 1);
	if((n = qconsume(ctlr.oq, buf, sizeof(buf))) > 0)
		usbputs(buf, n);
	unlock(&lck);
//	while((n = ucread(&ctlr, buf, sizeof(buf))) > 0)
//		qproduce(ctlr.iq, buf, n);
}

/*
 * Queue interface
 * The debug port does not seem to
 * issue interrupts for us. We must poll for completion and
 * also to check for input.
 * By now this might suffice but it's probably wrong.
 */
static void
usbconsreset(void)
{
	if(ehcidebugcapio == nil)
		return;
return;
	ctlr.oq = qopen(8*1024, 0, nil, nil);
	if(ctlr.oq == nil)
		return;
	debug = 0;
	addconsdev(ctlr.oq, usbputs, ctlr.consid, 0);
	addclock0link(usbclock, 10);

	ctlr.iq = qopen(8*1024, 0, nil, nil);
	if(ctlr.iq == nil)
		return;
	addkbdq(ctlr.iq, -1);
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
		if(debug == 0)
			return;
	dprint("usb console...");
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
				dprint("found\n");
				if(ucinit(p, ptr) >= 0){
					ctlr.consid = addconsdev(nil, usbputs, -1, 0);
					print("1");
					print("2");
					print("3");
					print("4");
					print("5");
					print("6");
					print("7");
					print("Plan 9 usb console\n");
				}
				return;
			}
			ptr = pcicfgr8(p, ptr+1);
		}
	}
	dprint("not found\n");
}

/*
 * This driver does not really serve any file. However, we want to
 * setup the console at reset time.
 */

static Chan*
usbconsattach(char*)
{
	error(Eperm);
	return nil;
}

Dev usbconsdevtab = {
	L'↔',
	"usbcons",

	usbconsreset,
	devinit,
	devshutdown,
	usbconsattach,
	/* all others set to nil. Attachments are not allowed */
};
