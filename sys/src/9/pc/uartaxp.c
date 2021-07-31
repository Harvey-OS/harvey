/*
 * Avanstar Xp pci uart driver
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#include "uartaxp.i"

typedef struct Cc Cc;
typedef struct Ccb Ccb;
typedef struct Ctlr Ctlr;
typedef struct Gcb Gcb;

/*
 * Global Control Block.
 * Service Request fields must be accessed using XCHG.
 */
struct Gcb {
	u16int	gcw;				/* Global Command Word */
	u16int	gsw;				/* Global Status Word */
	u16int	gsr;				/* Global Service Request */
	u16int	abs;				/* Available Buffer Space */
	u16int	bt;				/* Board Type */
	u16int	cpv;				/* Control Program Version */
	u16int	ccbn;				/* Ccb count */
	u16int	ccboff;				/* Ccb offset */
	u16int	ccbsz;				/* Ccb size */
	u16int	gcw2;				/* Global Command Word 2 */
	u16int	gsw2;				/* Global Status Word 2 */
	u16int	esr;				/* Error Service Request */
	u16int	isr;				/* Input Service Request */
	u16int	osr;				/* Output Service Request */
	u16int	msr;				/* Modem Service Request */
	u16int	csr;				/* Command Service Request */
};

/*
 * Channel Control Block.
 */
struct Ccb {
	u16int	br;				/* Baud Rate */
	u16int	df;				/* Data Format */
	u16int	lp;				/* Line Protocol */
	u16int	ibs;				/* Input Buffer Size */
	u16int	obs;				/* Output Buffer Size */
	u16int 	ibtr;				/* Ib Trigger Rate */
	u16int	oblw;				/* Ob Low Watermark */
	u8int	ixon[2];			/* IXON characters */
	u16int	ibhw;				/* Ib High Watermark */
	u16int	iblw;				/* Ib Low Watermark */
	u16int	cc;				/* Channel Command */
	u16int	cs;				/* Channel Status */
	u16int	ibsa;				/* Ib Start Addr */
	u16int 	ibea;				/* Ib Ending Addr */
	u16int	obsa;				/* Ob Start Addr */
	u16int 	obea;				/* Ob Ending Addr */
	u16int	ibwp;				/* Ib write pointer (RO) */
	u16int	ibrp;				/* Ib read pointer (R/W) */
	u16int	obwp;				/* Ob write pointer (R/W) */
	u16int	obrp;				/* Ob read pointer (RO) */
	u16int	ces;				/* Communication Error Status */
	u16int	bcp;				/* Bad Character Pointer */
	u16int	mc;				/* Modem Control */
	u16int	ms;				/* Modem Status */
	u16int	bs;				/* Blocking Status */
	u16int	crf;				/* Character Received Flag */
	u8int	ixoff[2];			/* IXOFF characters */
	u16int	cs2;				/* Channel Status 2 */
	u8int	sec[2];				/* Strip/Error Characters */
};

enum {						/* br */
	Br76800		= 0xFF00,
	Br115200	= 0xFF01,
};

enum {						/* df */
	Db5		= 0x0000,		/* Data Bits - 5 bits/byte */
	Db6		= 0x0001,		/*	6 bits/byte */
	Db7		= 0x0002,		/*	7 bits/byte */
	Db8		= 0x0003,		/*	8 bits/byte */
	DbMASK		= 0x0003,
	Sb1		= 0x0000,		/* 1 Stop Bit */
	Sb2		= 0x0004,		/* 2 Stop Bit */
	SbMASK		= 0x0004,
	Np		= 0x0000,		/* No Parity */
	Op		= 0x0008,		/* Odd Parity */
	Ep		= 0x0010,		/* Even Parity */
	Mp		= 0x0020,		/* Mark Parity */
	Sp		= 0x0030,		/* Space Parity */
	PMASK		= 0x0038,
	Cmn		= 0x0000,		/* Channel Mode Normal */
	Cme		= 0x0040,		/* CM Echo */
	Cmll		= 0x0080,		/* CM Local Loopback */
	Cmrl		= 0x00C0,		/* CM Remote Loopback */
};

enum {						/* lp */
	Ixon		= 0x0001,		/* Obey IXON/IXOFF */
	Ixany		= 0x0002,		/* Any character retarts Tx */
	Ixgen		= 0x0004,		/* Generate IXON/IXOFF  */
	Cts		= 0x0008,		/* CTS controls Tx */
	Dtr		= 0x0010,		/* Rx controls DTR */
	½d		= 0x0020,		/* RTS off during Tx */
	Rts		= 0x0040,		/* generate RTS */
	Emcs		= 0x0080,		/* Enable Modem Control */
	Ecs		= 0x1000,		/* Enable Character Stripping */
	Eia422		= 0x2000,		/* EIA422 */
};

enum {						/* cc */
	Ccu		= 0x0001,		/* Configure Channel and UART */
	Cco		= 0x0002,		/* Configure Channel Only */
	Fib		= 0x0004,		/* Flush Input Buffer */
	Fob		= 0x0008,		/* Flush Output Buffer */
	Er		= 0x0010,		/* Enable Receiver */
	Dr		= 0x0020,		/* Disable Receiver */
	Et		= 0x0040,		/* Enable Transmitter */
	Dt		= 0x0080,		/* Disable Transmitter */
};

enum {						/* ces */
	Oe		= 0x0001,		/* Overrun Error */
	Pe		= 0x0002,		/* Parity Error */
	Fe		= 0x0004,		/* Framing Error */
	Br		= 0x0008,		/* Break Received */
};

enum {						/* mc */
	Adtr		= 0x0001,		/* Assert DTR */
	Arts		= 0x0002,		/* Assert RTS */
	Ab		= 0x0010,		/* Assert BREAK */
};

enum {						/* ms */
	Scts		= 0x0001,		/* Status od CTS */
	Sdsr		= 0x0002,		/* Status of DSR */
	Sri		= 0x0004,		/* Status of RI */
	Sdcd		= 0x0008,		/* Status of DCD */
};

enum {						/* bs */
	Rd		= 0x0001,		/* Receiver Disabled */
	Td		= 0x0002,		/* Transmitter Disabled */
	Tbxoff		= 0x0004,		/* Tx Blocked by XOFF */
	Tbcts		= 0x0008,		/* Tx Blocked by CTS */
	Rbxoff		= 0x0010,		/* Rx Blocked by XOFF */
	Rbrts		= 0x0020,		/* Rx Blocked by RTS */
};

enum {						/* Local Configuration */
	Range		= 0x00,
	Remap		= 0x04,
	Region		= 0x18,
	Mb0		= 0x40,			/* Mailbox 0 */
	Ldb		= 0x60,			/* PCI to Local Doorbell */
	Pdb		= 0x64,			/* Local to PCI Doorbell */
	Ics		= 0x68,			/* Interrupt Control/Status */
	Mcc		= 0x6C,			/* Misc. Command and Control */
};

enum {						/* Mb0 */
	Edcc		= 1,			/* exec. downloaded code cmd */
	Aic		= 0x10,			/* adapter init'zed correctly */
	Cpr		= 1ul << 31,		/* control program ready */
};

enum {						/* Mcc */
	Rcr		= 1ul << 29,		/* reload config. reg.s */
	Asr		= 1ul << 30,		/* pci adapter sw reset */
	Lis		= 1ul << 31,		/* local init status */
};

typedef struct Cc Cc;
typedef struct Ccb Ccb;
typedef struct Ctlr Ctlr;

/*
 * Channel Control, one per uart.
 * Devuart communicates via the PhysUart functions with
 * a Uart* argument. Uart.regs is filled in by this driver
 * to point to a Cc, and Cc.ctlr points to the Axp board
 * controller.
 */
struct Cc {
	int	uartno;
	Ccb*	ccb;
	Ctlr*	ctlr;

	Rendez;

	Uart;
};

typedef struct Ctlr {
	char*	name;
	Pcidev*	pcidev;
	int	ctlrno;
	Ctlr*	next;

	u32int*	reg;
	uchar*	mem;
	Gcb*	gcb;

	int	im;		/* interrupt mask */
	Cc	cc[16];
} Ctlr;

#define csr32r(c, r)	(*((c)->reg+((r)/4)))
#define csr32w(c, r, v)	(*((c)->reg+((r)/4)) = (v))

static Ctlr* axpctlrhead;
static Ctlr* axpctlrtail;

extern PhysUart axpphysuart;

static int
axpccdone(void* ccb)
{
	return !((Ccb*)ccb)->cc;	/* hw sets ccb->cc to zero */
}

static void
axpcc(Cc* cc, int cmd)
{
	Ccb *ccb;
	int timeo;
	u16int cs;

	ccb = cc->ccb;
	ccb->cc = cmd;

	if(!cc->ctlr->im)
		for(timeo = 0; timeo < 1000000; timeo++){
			if(!ccb->cc)
				break;
			microdelay(1);
		}
	else
		tsleep(cc, axpccdone, ccb, 1000);

	cs = ccb->cs;
	if(ccb->cc || cs){
		print("%s: cmd %#ux didn't terminate: %#ux %#ux\n",
			cc->name, cmd, ccb->cc, cs);
		if(cc->ctlr->im)
			error(Eio);
	}
}

static long
axpstatus(Uart* uart, void* buf, long n, long offset)
{
	char *p;
	Ccb *ccb;
	u16int bs, fstat, ms;

	ccb = ((Cc*)(uart->regs))->ccb;

	p = malloc(READSTR);
	bs = ccb->bs;
	fstat = ccb->df;
	ms = ccb->ms;

	snprint(p, READSTR,
		"b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d) "
		"berr(%d) serr(%d)%s%s%s%s\n",

		uart->baud,
		uart->hup_dcd,
		ms & Sdsr,
		uart->hup_dsr,
		(fstat & DbMASK) + 5,
		0,
		(fstat & PMASK) ? ((fstat & Ep) == Ep? 'e': 'o'): 'n',
		(bs & Rbrts) ? 1 : 0,
		(fstat & Sb2) ? 2 : 1,
		0,

		uart->dev,
		uart->type,
		uart->ferr,
		uart->oerr,
		uart->berr,
		uart->serr,
		(ms & Scts) ? " cts"  : "",
		(ms & Sdsr) ? " dsr"  : "",
		(ms & Sdcd) ? " dcd"  : "",
		(ms & Sri) ? " ring" : ""
	);
	n = readstr(offset, buf, n, p);
	free(p);

	return n;
}

static void
axpfifo(Uart*, int)
{
}

static void
axpdtr(Uart* uart, int on)
{
	Ccb *ccb;
	u16int mc;

	ccb = ((Cc*)(uart->regs))->ccb;

	mc = ccb->mc;
	if(on)
		mc |= Adtr;
	else
		mc &= ~Adtr;
	ccb->mc = mc;
}

/*
 * can be called from uartstageinput() during an input interrupt,
 * with uart->rlock ilocked or the uart qlocked, sometimes both.
 */
static void
axprts(Uart* uart, int on)
{
	Ccb *ccb;
	u16int mc;

	ccb = ((Cc*)(uart->regs))->ccb;

	mc = ccb->mc;
	if(on)
		mc |= Arts;
	else
		mc &= ~Arts;
	ccb->mc = mc;
}

static void
axpmodemctl(Uart* uart, int on)
{
	Ccb *ccb;
	u16int lp;

	ccb = ((Cc*)(uart->regs))->ccb;

	ilock(&uart->tlock);
	lp = ccb->lp;
	if(on){
		lp |= Cts|Rts;
		lp &= ~Emcs;
		uart->cts = ccb->ms & Scts;
	}
	else{
		lp &= ~(Cts|Rts);
		lp |= Emcs;
		uart->cts = 1;
	}
	uart->modem = on;
	iunlock(&uart->tlock);

	ccb->lp = lp;
	axpcc(uart->regs, Ccu);
}

static int
axpparity(Uart* uart, int parity)
{
	Ccb *ccb;
	u16int df;

	switch(parity){
	default:
		return -1;
	case 'e':
		parity = Ep;
		break;
	case 'o':
		parity = Op;
		break;
	case 'n':
		parity = Np;
		break;
	}

	ccb = ((Cc*)(uart->regs))->ccb;

	df = ccb->df & ~PMASK;
	ccb->df = df|parity;
	axpcc(uart->regs, Ccu);

	return 0;
}

static int
axpstop(Uart* uart, int stop)
{
	Ccb *ccb;
	u16int df;

	switch(stop){
	default:
		return -1;
	case 1:
		stop = Sb1;
		break;
	case 2:
		stop = Sb2;
		break;
	}

	ccb = ((Cc*)(uart->regs))->ccb;

	df = ccb->df & ~SbMASK;
	ccb->df = df|stop;
	axpcc(uart->regs, Ccu);

	return 0;
}

static int
axpbits(Uart* uart, int bits)
{
	Ccb *ccb;
	u16int df;

	bits -= 5;
	if(bits < 0 || bits > 3)
		return -1;

	ccb = ((Cc*)(uart->regs))->ccb;

	df = ccb->df & ~DbMASK;
	ccb->df = df|bits;
	axpcc(uart->regs, Ccu);

	return 0;
}

static int
axpbaud(Uart* uart, int baud)
{
	Ccb *ccb;
	int i, ibtr;

	/*
	 * Set baud rate (high rates are special - only 16 bits).
	 */
	if(baud <= 0)
		return -1;
	uart->baud = baud;

	ccb = ((Cc*)(uart->regs))->ccb;

	switch(baud){
	default:
		ccb->br = baud;
		break;
	case 76800:
		ccb->br = Br76800;
		break;
	case 115200:
		ccb->br = Br115200;
		break;
	}

	/*
	 * Set trigger level to about 50 per second.
	 */
	ibtr = baud/500;
	i = (ccb->ibea - ccb->ibsa)/2;
	if(ibtr > i)
		ibtr = i;
	ccb->ibtr = ibtr;
	axpcc(uart->regs, Ccu);

	return 0;
}

static void
axpbreak(Uart* uart, int ms)
{
	Ccb *ccb;
	u16int mc;

	/*
	 * Send a break.
	 */
	if(ms <= 0)
		ms = 200;

	ccb = ((Cc*)(uart->regs))->ccb;

	mc = ccb->mc;
	ccb->mc = Ab|mc;
	tsleep(&up->sleep, return0, 0, ms);
	ccb->mc = mc & ~Ab;
}

/* only called from interrupt service */
static void
axpmc(Cc* cc)
{
	int old;
	Ccb *ccb;
	u16int ms;

	ccb = cc->ccb;

	ms = ccb->ms;

	if(ms & Scts){
		ilock(&cc->tlock);
		old = cc->cts;
		cc->cts = ms & Scts;
		if(old == 0 && cc->cts)
			cc->ctsbackoff = 2;
		iunlock(&cc->tlock);
	}
	if(ms & Sdsr){
		old = ms & Sdsr;
		if(cc->hup_dsr && cc->dsr && !old)
			cc->dohup = 1;
		cc->dsr = old;
	}
	if(ms & Sdcd){
		old = ms & Sdcd;
		if(cc->hup_dcd && cc->dcd && !old)
			cc->dohup = 1;
		cc->dcd = old;
	}
}

/* called from uartkick() with uart->tlock ilocked */
static void
axpkick(Uart* uart)
{
	Cc *cc;
	Ccb *ccb;
	uchar *ep, *mem, *rp, *wp, *bp;

	if(uart->cts == 0 || uart->blocked)
		return;

	cc = uart->regs;
	ccb = cc->ccb;

	mem = (uchar*)cc->ctlr->gcb;
	bp = mem + ccb->obsa;
	rp = mem + ccb->obrp;
	wp = mem + ccb->obwp;
	ep = mem + ccb->obea;
	while(wp != rp-1 && (rp != bp || wp != ep)){
		/*
		 * if we've exhausted the uart's output buffer,
		 * ask for more from the output queue, and quit if there
		 * isn't any.
		 */
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		*wp++ = *(uart->op++);
		if(wp > ep)
			wp = bp;
		ccb->obwp = wp - mem;
	}
}

/* only called from interrupt service */
static void
axprecv(Cc* cc)
{
	Ccb *ccb;
	uchar *ep, *mem, *rp, *wp;

	ccb = cc->ccb;

	mem = (uchar*)cc->ctlr->gcb;
	rp = mem + ccb->ibrp;
	wp = mem + ccb->ibwp;
	ep = mem + ccb->ibea;

	while(rp != wp){
		uartrecv(cc, *rp++);		/* ilocks cc->tlock */
		if(rp > ep)
			rp = mem + ccb->ibsa;
		ccb->ibrp = rp - mem;
	}
}

static void
axpinterrupt(Ureg*, void* arg)
{
	int work;
	Cc *cc;
	Ctlr *ctlr;
	u32int ics;
	u16int r, sr;

	work = 0;
	ctlr = arg;
	ics = csr32r(ctlr, Ics);
	if(ics & 0x0810C000)
		print("%s: unexpected interrupt %#ux\n", ctlr->name, ics);
	if(!(ics & 0x00002000)) {
		/* we get a steady stream of these on consoles */
		// print("%s: non-doorbell interrupt\n", ctlr->name);
		ctlr->gcb->gcw2 = 0x0001;	/* set Gintack */
		return;
	}

//	while(work to do){
		cc = ctlr->cc;
		for(sr = xchgw(&ctlr->gcb->isr, 0); sr != 0; sr >>= 1){
			if(sr & 0x0001)
				work++, axprecv(cc);
			cc++;
		}
		cc = ctlr->cc;
		for(sr = xchgw(&ctlr->gcb->osr, 0); sr != 0; sr >>= 1){
			if(sr & 0x0001)
				work++, uartkick(&cc->Uart);
			cc++;
		}
		cc = ctlr->cc;
		for(sr = xchgw(&ctlr->gcb->csr, 0); sr != 0; sr >>= 1){
			if(sr & 0x0001)
				work++, wakeup(cc);
			cc++;
		}
		cc = ctlr->cc;
		for(sr = xchgw(&ctlr->gcb->msr, 0); sr != 0; sr >>= 1){
			if(sr & 0x0001)
				work++, axpmc(cc);
			cc++;
		}
		cc = ctlr->cc;
		for(sr = xchgw(&ctlr->gcb->esr, 0); sr != 0; sr >>= 1){
			if(sr & 0x0001){
				r = cc->ccb->ms;
				if(r & Oe)
					cc->oerr++;
				if(r & Pe)
					cc->perr++;
				if(r & Fe)
					cc->ferr++;
				if (r & (Oe|Pe|Fe))
					work++;
			}
			cc++;
		}
//	}
	/* only meaningful if we don't share the irq */
	if (0 && !work)
		print("%s: interrupt with no work\n", ctlr->name);
	csr32w(ctlr, Pdb, 1);		/* clear doorbell interrupt */
	ctlr->gcb->gcw2 = 0x0001;	/* set Gintack */
}

static void
axpdisable(Uart* uart)
{
	Cc *cc;
	u16int lp;
	Ctlr *ctlr;

	/*
 	 * Turn off DTR and RTS, disable interrupts.
	 */
	(*uart->phys->dtr)(uart, 0);
	(*uart->phys->rts)(uart, 0);

	cc = uart->regs;
	lp = cc->ccb->lp;
	cc->ccb->lp = Emcs|lp;
	axpcc(cc, Dt|Dr|Fob|Fib|Ccu);

	/*
	 * The Uart is qlocked.
	 */
	ctlr = cc->ctlr;
	ctlr->im &= ~(1<<cc->uartno);
	if(ctlr->im == 0)
		intrdisable(ctlr->pcidev->intl, axpinterrupt, ctlr,
			ctlr->pcidev->tbdf, ctlr->name);
}

static void
axpenable(Uart* uart, int ie)
{
	Cc *cc;
	Ctlr *ctlr;
	u16int lp;

	cc = uart->regs;
	ctlr = cc->ctlr;

	/*
 	 * Enable interrupts and turn on DTR and RTS.
	 * Be careful if this is called to set up a polled serial line
	 * early on not to try to enable interrupts as interrupt-
	 * -enabling mechanisms might not be set up yet.
	 */
	if(ie){
		/*
		 * The Uart is qlocked.
		 */
		if(ctlr->im == 0){
			intrenable(ctlr->pcidev->intl, axpinterrupt, ctlr,
				ctlr->pcidev->tbdf, ctlr->name);
			csr32w(ctlr, Ics, 0x00031F00);
			csr32w(ctlr, Pdb, 1);
			ctlr->gcb->gcw2 = 1;
		}
		ctlr->im |= 1<<cc->uartno;
	}

	(*uart->phys->dtr)(uart, 1);
	(*uart->phys->rts)(uart, 1);

	/*
	 * Make sure we control RTS, DTR and break.
	 */
	lp = cc->ccb->lp;
	cc->ccb->lp = Emcs|lp;
	cc->ccb->oblw = 64;
	axpcc(cc, Et|Er|Ccu);
}

static void*
axpdealloc(Ctlr* ctlr)
{
	int i;

	for(i = 0; i < 16; i++){
		if(ctlr->cc[i].name != nil)
			free(ctlr->cc[i].name);
	}
	if(ctlr->reg != nil)
		vunmap(ctlr->reg, ctlr->pcidev->mem[0].size);
	if(ctlr->mem != nil)
		vunmap(ctlr->mem, ctlr->pcidev->mem[2].size);
	if(ctlr->name != nil)
		free(ctlr->name);
	free(ctlr);

	return nil;
}

static Uart*
axpalloc(int ctlrno, Pcidev* pcidev)
{
	Cc *cc;
	uchar *p;
	Ctlr *ctlr;
	void *addr;
	char name[64];
	u32int bar, r;
	int i, n, timeo;

	ctlr = malloc(sizeof(Ctlr));
	seprint(name, name+sizeof(name), "uartaxp%d", ctlrno);
	kstrdup(&ctlr->name, name);
	ctlr->pcidev = pcidev;
	ctlr->ctlrno = ctlrno;

	/*
	 * Access to runtime registers.
	 */
	bar = pcidev->mem[0].bar;
	if((addr = vmap(bar & ~0x0F, pcidev->mem[0].size)) == 0){
		print("%s: can't map registers at %#ux\n", ctlr->name, bar);
		return axpdealloc(ctlr);
	}
	ctlr->reg = addr;
	print("%s: port 0x%ux irq %d ", ctlr->name, bar, pcidev->intl);

	/*
	 * Local address space 0.
	 */
	bar = pcidev->mem[2].bar;
	if((addr = vmap(bar & ~0x0F, pcidev->mem[2].size)) == 0){
		print("%s: can't map memory at %#ux\n", ctlr->name, bar);
		return axpdealloc(ctlr);
	}
	ctlr->mem = addr;
	ctlr->gcb = (Gcb*)(ctlr->mem+0x10000);
	print("mem 0x%ux size %d: ", bar, pcidev->mem[2].size);

	/*
	 * Toggle the software reset and wait for
	 * the adapter local init status to indicate done.
	 *
	 * The two 'delay(100)'s below are important,
	 * without them the board seems to become confused
	 * (perhaps it needs some 'quiet time' because the
	 * timeout loops are not sufficient in themselves).
	 */
	r = csr32r(ctlr, Mcc);
	csr32w(ctlr, Mcc, r|Asr);
	microdelay(1);
	csr32w(ctlr, Mcc, r&~Asr);
	delay(100);

	for(timeo = 0; timeo < 100000; timeo++){
		if(csr32r(ctlr, Mcc) & Lis)
			break;
		microdelay(1);
	}
	if(!(csr32r(ctlr, Mcc) & Lis)){
		print("%s: couldn't reset\n", ctlr->name);
		return axpdealloc(ctlr);
	}
	print("downloading...");
	/*
	 * Copy the control programme to the card memory.
	 * The card's i960 control structures live at 0xD000.
	 */
	if(sizeof(uartaxpcp) > 0xD000){
		print("%s: control programme too big\n", ctlr->name);
		return axpdealloc(ctlr);
	}
	/* TODO: is this right for more than 1 card? devastar does the same */
	csr32w(ctlr, Remap, 0xA0000001);
	for(i = 0; i < sizeof(uartaxpcp); i++)
		ctlr->mem[i] = uartaxpcp[i];
	/*
	 * Execute downloaded code and wait for it
	 * to signal ready.
	 */
	csr32w(ctlr, Mb0, Edcc);
	delay(100);
	/* the manual says to wait for Cpr for 1 second */
	for(timeo = 0; timeo < 10000; timeo++){
		if(csr32r(ctlr, Mb0) & Cpr)
			break;
		microdelay(100);
	}
	if(!(csr32r(ctlr, Mb0) & Cpr)){
		print("control programme not ready; Mb0 %#ux\n",
			csr32r(ctlr, Mb0));
		print("%s: distribution panel not connected or card not fully seated?\n",
			ctlr->name);

		return axpdealloc(ctlr);
	}
	print("\n");

	n = ctlr->gcb->ccbn;
	if(ctlr->gcb->bt != 0x12 || n > 16){
		print("%s: wrong board type %#ux, %d channels\n",
			ctlr->name, ctlr->gcb->bt, ctlr->gcb->ccbn);
		return axpdealloc(ctlr);
	}

	p = ((uchar*)ctlr->gcb) + ctlr->gcb->ccboff;
	for(i = 0; i < n; i++){
		cc = &ctlr->cc[i];
		cc->ccb = (Ccb*)p;
		p += ctlr->gcb->ccbsz;
		cc->uartno = i;
		cc->ctlr = ctlr;

		cc->regs = cc;		/* actually Uart->regs */
		seprint(name, name+sizeof(name), "uartaxp%d%2.2d", ctlrno, i);
		kstrdup(&cc->name, name);
		cc->freq = 0;
		cc->bits = 8;
		cc->stop = 1;
		cc->parity = 'n';
		cc->baud = 9600;
		cc->phys = &axpphysuart;
		cc->console = 0;
		cc->special = 0;

		cc->next = &ctlr->cc[i+1];
	}
	ctlr->cc[n-1].next = nil;

	ctlr->next = nil;
	if(axpctlrhead != nil)
		axpctlrtail->next = ctlr;
	else
		axpctlrhead = ctlr;
	axpctlrtail = ctlr;

	return ctlr->cc;
}

static Uart*
axppnp(void)
{
	Pcidev *p;
	int ctlrno;
	Uart *head, *tail, *uart;

	/*
	 * Loop through all PCI devices looking for simple serial
	 * controllers (ccrb == 0x07) and configure the ones which
	 * are familiar.
	 */
	head = tail = nil;
	ctlrno = 0;
	for(p = pcimatch(nil, 0, 0); p != nil; p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x07)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x6001<<16)|0x114F:	/* AvanstarXp */
			if((uart = axpalloc(ctlrno, p)) == nil)
				continue;
			break;
		}

		if(head != nil)
			tail->next = uart;
		else
			head = uart;
		for(tail = uart; tail->next != nil; tail = tail->next)
			;
		ctlrno++;
	}

	return head;
}

PhysUart axpphysuart = {
	.name		= "AvanstarXp",
	.pnp		= axppnp,
	.enable		= axpenable,
	.disable	= axpdisable,
	.kick		= axpkick,
	.dobreak	= axpbreak,
	.baud		= axpbaud,
	.bits		= axpbits,
	.stop		= axpstop,
	.parity		= axpparity,
	.modemctl	= axpmodemctl,
	.rts		= axprts,
	.dtr		= axpdtr,
	.status		= axpstatus,
	.fifo		= axpfifo,
	.getc		= nil,
	.putc		= nil,
};
