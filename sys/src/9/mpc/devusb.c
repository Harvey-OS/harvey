#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

typedef struct USBparam USBparam;
struct USBparam {
	ushort	epptr[4];	/**/
	ulong	rstate;
	ulong	rptr;
	ushort	frame_n;	/**/
	ushort	rbcnt;
	ulong	rtemp;
};

typedef struct EPparam EPparam;
struct EPparam {
	ushort	rbase;
	ushort	tbase;
	uchar	rfcr;
	uchar	tfcr;
	ushort	mrblr;
	ushort	rbptr;
	ushort	tbptr;

	ulong	tstate;
	ulong	tptr;
	ushort	tcrc;
	ushort	tbcnt;
	ulong	res[2];
};

enum {
	Nrdre		= 8,	/* receive descriptor ring entries */
	Ntdre		= 8,	/* transmit descriptor ring entries */

	Rbsize		= 8+3,		/* ring buffer size (+3 for PID+CRC16) */
	Bufsize		= (Rbsize+7)&~7,	/* aligned */

	Nendpt		= 4,
};

#define	MkPID(x) (((~x&0xF)<<4)|(x&0xF))
enum {
	TokIN = MkPID(9),
	TokOUT = MkPID(1),
	TokSOF = MkPID(5),
	TokSETUP = MkPID(0xD),
	TokDATA0 = MkPID(3),
	TokDATA1 = MkPID(0xB),
	TokACK = MkPID(2),
	TokNAK = MkPID(0xA),
	TokPRE = MkPID(0xC),
};

enum {
	BDData0=		1<<7,
	BDData1=		1<<6,	/* DATA1 if set, DATA0 otherwise */
	BDrxerr=		0x3F,	/* various error flags */

	BDtxcrc=		1<<10,
	BDcnf=		1<<9,	/* transmit confirmation */
	/* BDData0, BDData1 */
	BDNak=		1<<4,	/* nak received */
	BDStall=		1<<3,	/* stall received */
	BDTmo=		1<<2,	/* timeout */
	BDUrun=		1<<1,	/* underrun */

	/* usmod */
	EN=		1<<0,	/* enable USB */
	HOST=	1<<1,	/* host mode */
	TEST=	1<<2,	/* test mode */
	RESUME=	1<<6,	/* generate resume condition */
	LSS=		1<<7,	/* low-speed signalling */

	/* usber */
	Freset=	1<<9,
	Fidle=	1<<8,
	Ftxe3=	1<<7,
	Ftxe2=	1<<6,
	Ftxe1=	1<<5,
	Ftxe0=	1<<4,
	Ftxe=	Ftxe0|Ftxe1|Ftxe2|Ftxe3,
	Fsof=	1<<3,
	Fbsy=	1<<2,
	Ftxb=	1<<1,
	Frxb=	1<<0,

	/* uscom */
	FifoFill=		1<<7,
	FifoFlush=	1<<6,

	/* USB commands (or'd with USBCmd) */
	StopTxEndPt	= 1<<4,
	RestartTxEndPt	= 2<<4,

	SOFmask=	(1<<11)-1,
};

#define	USBABITS	(SIBIT(14)|SIBIT(15))
#define	USBRXCBIT	(SIBIT(10)|SIBIT(11))
#define	USBTXCBIT	(SIBIT(6)|SIBIT(7))

/*
 * software structures
 */
typedef struct Ctlr Ctlr;
typedef struct Endpt Endpt;

struct Endpt {
	int	x;	/* index in Ctlr.pts */
	int	dev;	/* remote device (host endpt only) */
	int	endpt;	/* remote endpt (host endpt only) */
	uchar	addr[2];	/* crc5<<11 | endpt<<7 | dev */
	int	rtog;		/* DATA0 or DATA1 */
	int	xtog;	/* DATA0 or DATA1 */
	int	rxintr;
	Rendez	ir;
	EPparam*	ep;
	Ring;

	Queue*	oq;
	Queue*	iq;
	Ctlr*	ctlr;
};

struct Ctlr {
	Lock;
	int	init;
	USB*	usb;
	USBparam *usbp;
	Endpt	pts[4];
};

static	Ctlr	usbctlr[1];

static	void	dumpusb(Block*, char*);
static	void	interrupt(Ureg*, void*);
static	void	setupep(int, EPparam*);
static	void	usbtest(void);

/*
 * test driver for USB, host mode
 */
void
usbreset(void)
{
	IMM *io;
	USB *usb;
	USBparam *usbp;
	EPparam *ep;
	int brg, i;

	brg = brgalloc();
	if(brg < 0){
		print("usb: no baud rate generator is free\n");
		return;
	}

	io = m->iomem;
	usb = (USB*)((ulong)m->iomem+0xA00);
	usbp = (USBparam*)KADDR(SCC1P);

	/* select port pins */
	io->padir &= ~USBABITS;
	io->papar |= USBABITS;
	io->pcpar = (io->pcpar & ~USBRXCBIT) | USBTXCBIT;
	io->pcdir = (io->pcdir & ~USBRXCBIT) | USBTXCBIT;
	io->pcso |= USBRXCBIT;
	eieio();

	ep = cpmalloc(Nendpt*sizeof(*ep), 32);
	if(ep == nil){
		print("can't allocate USB\n");
		return;
	}
print("ep=#%.8ux\n", PADDR(ep));

	usbp->frame_n = 0;
	usbp->rstate = 0;
	for(i=0; i<Nendpt; i++){
		usb->uscom = FifoFlush|i;
		usb->usep[i] = 0;
		usbp->epptr[i] = PADDR(ep+i) & 0xffff;
		ep[i].rbase = 0;
		ep[i].tbase = 0;
		ep[i].rfcr = 0x10;
		ep[i].tfcr = 0x10;
		ep[i].mrblr = Bufsize;
		ep[i].rbptr = 0;
		ep[i].tbptr = 0;
		ep[i].tstate = 0;
		ep[i].tptr = 0;
	}

	usbctlr->usb = usb;
	usbctlr->usbp = usbp;

	usb->usmod = 0;
	if(1)
		usb->usmod = HOST;
	if(0)
		usb->usmod |= LSS;
	if(1)
		usb->usmod |= TEST|HOST;

	/* set up baud rate generator for appropriate speed */
	if(usb->usmod & LSS){
		print("USB: low speed\n");
		io->brgc[brg] = baudgen(1000000, 1) | BaudEnable;
	}else
		io->brgc[brg] = BaudEnable;
	eieio();
	io->sicr = (io->sicr & ~(7<<3)) | (brg<<3);	/* set R1CS */
	eieio();

	usb->usep[0] = (0<<12) | (2<<8) | (1<<5);	/* EPN=0, TM=bulk, multi frame */
	setupep(0, ep);
	if(usb->usmod & TEST){
		print("USB: loop back mode\n");
		usb->usep[1] = (1<<12) | (0<<8);	/* EPN=1, TM=control*/
		setupep(1, ep+1);
	}

	m->bcsr[4] &= ~(DisableUSB|DisableUSBVcc);
	if(usb->usmod & LSS)
		m->bcsr[4] &= ~USBFullSpeed;
	else
		m->bcsr[4] |= USBFullSpeed;
	if(usb->usmod & HOST && !(usb->usmod & TEST))
		usb->usadr = 0x7f;	/* unused in host mode, but just in case */
	else
		usb->usadr = 0;
	usb->usber = ~0;	/* clear events */
	intrenable(CPICvec+0x1E, interrupt, usbctlr, BUSUNKNOWN);
	usb->usbmr = ~0 & ~Fidle;	/* enable all events except idle */
	eieio();
	delay(12);
	usb->usmod |= EN;
}

static void
setupep(int n, EPparam *ep)
{
	Endpt *e;

	e = &usbctlr->pts[n];
	e->x = n;
	e->ctlr = usbctlr;
	e->xtog = TokDATA0;
	if(e->oq == nil)
		e->oq = qopen(8*1024, 1, 0, 0);
	if(e->iq == nil)
		e->iq = qopen(8*1024, 1, 0, 0);
	e->ep = ep;
	if(ioringinit(e, Nrdre, Ntdre, Bufsize) < 0)
		panic("usbreset");
	ep->rbase = PADDR(e->rdr);
	ep->rbptr = ep->rbase;
	ep->tbase = PADDR(e->tdr);
	ep->tbptr = ep->tbase;
	eieio();
}

static void
epprod(Ctlr *ctlr, int epn)
{
	ctlr->usb->uscom = FifoFill | (epn&3);
	eieio();
}

static void
txstart(Endpt *r)
{
	int len, flags;
	Block *b;
	BD *dre, *rdy;

	if(r->ctlr->init)
		return;
	rdy = nil;
	while(r->ntq < Ntdre-1){
		b = qget(r->oq);
		if(b == 0)
			break;

		dre = &r->tdr[r->tdrh];
		if(dre->status & BDReady)
			panic("txstart");
print("Tx%d: ", r->x); dumpusb(b, "TX");
	
		/*
		 * Give ownership of the descriptor to the chip, increment the
		 * software ring descriptor pointer and tell the chip to poll.
		 */
		flags = 0;
		switch(b->rp[0]){
		case TokDATA1:
			flags = BDData1|BDData0;
		case TokDATA0:
			flags |= BDtxcrc|BDcnf|BDReady;
			flags |= BDData0;
			/*b->rp++;	/* skip DATAn */
		}
		len = BLEN(b);
		dcflush(b->rp, len);
		if(r->txb[r->tdrh] != nil)
			panic("usb: txstart");
		r->txb[r->tdrh] = b;
		dre->addr = PADDR(b->rp);
		dre->length = len;
		eieio();
		dre->status = (dre->status & BDWrap) | BDInt|BDLast | flags;
		if(rdy){
			rdy->status |= BDReady;
			rdy = nil;
		}
		if((flags & BDReady) == 0)
			rdy = dre;
		eieio();
		r->ntq++;
		r->tdrh = NEXT(r->tdrh, Ntdre);
	}
	if(rdy)
		rdy->status |= BDReady;
	eieio();
}

static void
transmit(Endpt *r)
{
	ilock(r->ctlr);
	txstart(r);
	iunlock(r->ctlr);
}

static void
endptintr(Endpt *r, int events)
{
	int len, status;
	BD *dre;
	Block *b;

	if(events & Frxb || 1){
		dre = &r->rdr[r->rdrx];
		while(((status = dre->status) & BDEmpty) == 0){
			if(status & BDrxerr || (status & (BDFirst|BDLast)) != (BDFirst|BDLast)){
				print("usb rx%d: %4.4ux %d ", r->x, status, dre->length);
				{uchar *p;int i; p=KADDR(dre->addr); for(i=0;i<14&&i<dre->length; i++)print(" %.2ux", p[i]);print("\n");}
			}
			else{
				/*
				 * We have a packet. Read it into the next
				 * free ring buffer, if any.
				 */
				len = dre->length-2;	/* discard CRC */
				if(len >= 0 && (b = iallocb(len)) != 0){
					memmove(b->wp, KADDR(dre->addr), len);
					dcflush(KADDR(dre->addr), len);
					b->wp += len;
					// usbiq(ctlr, b);
					print("rx%d: ", r->x); dumpusb(b, "RX");
					freeb(b);
				}
			}

			/*
			 * Finished with this descriptor, reinitialise it,
			 * give it back to the chip, then on to the next...
			 */
			dre->length = 0;
			dre->status = (dre->status & BDWrap) | BDEmpty | BDInt;
			eieio();

			r->rdrx = NEXT(r->rdrx, Nrdre);
			dre = &r->rdr[r->rdrx];
			r->rxintr = 1;
			wakeup(&r->ir);
		}
	}

	/*
	 * Transmitter interrupt: handle anything queued for a free descriptor.
	 */
	if(events & (Ftxb|Ftxe0|Ftxe1)){
		lock(r->ctlr);
		while(r->ntq){
			dre = &r->tdr[r->tdri];
			if(dre->status & BDReady)
				break;
			print("usbtx%d=#%.4x %8.8lux\n", r->x, dre->status, dre->addr);
			/* TO DO: error counting */
			b = r->txb[r->tdri];
			if(b == nil)
				panic("usb/interrupt: bufp");
			r->txb[r->tdri] = nil;
			freeb(b);
			r->ntq--;
			r->tdri = NEXT(r->tdri, Ntdre);
		}
		txstart(r);
		unlock(r->ctlr);
		if(r->ntq)
			epprod(r->ctlr, r->x);
	}
}

static void
interrupt(Ureg*, void *arg)
{
	int events;
	Ctlr *ctlr;
	USB *usb;

	ctlr = arg;
	usb = ctlr->usb;
	events = usb->usber;
	eieio();
	usb->usber = events;
	print("USB#%x\n", events);

	if(events & Fsof)
		print("SOF #%ux\n", ctlr->usbp->frame_n&SOFmask);
	endptintr(&ctlr->pts[0], events);
	if(usb->usmod & TEST)
		endptintr(&ctlr->pts[1], events);
	if(events & Ftxe0)
		cpmop(USBCmd|RestartTxEndPt, USBID, 0<<1);
	if(events & Ftxe1)
		cpmop(USBCmd|RestartTxEndPt, USBID, 1<<1);
}

void
usbinit(void)
{
	usbreset();
usbtest();
}

static void
dumpusb(Block *b, char *msg)
{
	int i;

	print("%s: %8.8lux [%d]: ", msg, (ulong)b->rp, BLEN(b));
	for(i=0; i<BLEN(b) && i < 16; i++)
		print(" %.2x", b->rp[i]);
	print("\n");
}

/*
 * could generate a table, but since the result can be precomputed
 * per connection, it doesn't seem worthwhile.
 */
static ulong
crc5(ushort v)
{
	int i;
	ulong x, poly;

	x = 0x1F;
	poly = 0x14;
	for(i=0; i<11; i++, v>>= 1){
		if((x^v) & 1)
			x = ((x>>1)&0x0F)^poly;
		else
			x = ((x>>1)&0x0F);
	}
	return ~x&0x1F;
}

static void
setaddr(Endpt *e, int dev, int endpt)
{
	ushort v;

	e->dev = dev;
	e->endpt = endpt;
	v = (e->endpt<<7) | e->dev;
	v |= crc5(v) << 11;
	e->addr[0] = v;
	e->addr[1] = v>>8;
}

/*
 * Token packets (watch bit/byte order; it's LSB on the wire):
 *
 * 4 bit PID, 4 bit check
 *	TokIN, etc.
 * 7 bit device, 4 bit end point
 * 5 bit CRC5
 */
static void
sendtoken(Endpt *e, int pid)
{
	Block *b;

	b = allocb(3);
	b->wp[0] = pid;
	b->wp[1] = e->addr[0];
	b->wp[2] = e->addr[1];
	b->wp += 3;
	if(pid == TokDATA0 || pid == TokDATA1)
		e->xtog ^= TokDATA1^TokDATA0;
	else
		e->xtog = TokDATA0;
	qbwrite(e->oq, b);
	/* wait for reply? */
//	transmit(e);
}

static void
sendSOF(Endpt *e, int frame)
{
	Block *b;

	b = allocb(3);
	b->wp[0] = TokSOF;
	frame &= SOFmask;
	frame |= crc5(frame) << 11;
	b->wp[1] = frame;
	b->wp[2] = frame>>8;
	b->wp += 3;
	qbwrite(e->oq, b);
	transmit(e);
}

static void
sendack(Endpt *e, int pid)
{
	Block *b;

	b = allocb(1);
	b->wp[0] = pid;
	b->wp += 1;
	qbwrite(e->oq, b);
	/* wait for reply? */
	transmit(e);
}

static void
senddata(Endpt *e, void *buf, long n)
{
	Block *b;

	b = allocb(n+4);
	b->wp += 3;	/* skip 3 bytes for word alignment of data */
	b->rp = b->wp;
	b->wp[0] = e->xtog;
	memmove(b->wp+1, buf, n);
	b->wp += n+1;
	e->xtog ^= TokDATA1^TokDATA0;
	qbwrite(e->oq, b);
	/* wait for reply? */
	transmit(e);
	epprod(e->ctlr, e->x);
}

static int
usbinput(void *a)
{
	return ((Endpt*)a)->rxintr;
}

static void
usbtest(void)
{
	Ctlr *ctlr;
	Endpt *e, *e1;
	uchar msg[8];
	int epn, testing, i;

	ctlr = usbctlr;
	epn = 0;
	testing = ctlr->usb->usmod & TEST;
	if(testing)
		epn = 1;
	e = &ctlr->pts[0];
	e1 = &ctlr->pts[1];
	if((ctlr->usb->usmod & LSS) == 0)
		sendSOF(e, 0);
	memset(msg, 0, 8);
	msg[0] = 0;
	msg[1] = 8;	/* set configuration */
	msg[2] = 1;
	setaddr(e, 0, epn);
	sendtoken(e, TokSETUP);
	senddata(e, msg, 8);
	if(testing){
		for(i=0; i<8; i++)
			msg[i] = 0x40+i;
		senddata(e1, msg, 8);
		tsleep(&up->sleep, return0, 0, 3);
	}
	if(1){
		e->rxintr = 0;
		sendtoken(e, TokIN);
		transmit(e);
		epprod(ctlr, 0);
		tsleep(&e->ir, usbinput, e, 1000);
		if(!usbinput(e))
			print("RX: none\n");
	}
	sendtoken(e, TokSETUP);
	msg[0] = 0x23;
	msg[1] = 3;	/* set feature */
	msg[2] = 8;	/* port power */
	msg[3] = 0;
	msg[4] = 1;	/* port 1 */
	senddata(e, msg, 8);
	if(testing){
		for(i=0; i<8; i++)
			msg[i] = 0x60+i;
		senddata(e1, msg, 8);
		tsleep(&up->sleep, return0, 0, 3);
	}
	if(1){
		e->rxintr = 0;
		sendtoken(e, TokIN);
		transmit(e);
		epprod(ctlr, 0);
		tsleep(&e->ir, usbinput, e, 1000);
		if(!usbinput(e))
			print("RX: none\n");
	}
}
