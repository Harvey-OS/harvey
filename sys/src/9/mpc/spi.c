#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

typedef struct SPIparam SPIparam;
struct SPIparam {
	ushort	rbase;
	ushort	tbase;
	uchar	rfcr;
	uchar	tfcr;
	ushort	mrblr;
	ulong	rstate;
	ulong	rptr;
	ushort	rbptr;
	ushort	rcnt;
	ulong	rtmp;
	ulong	tstate;
	ulong	tptr;
	ushort	tbptr;
	ushort	tcnt;
	ulong	ttmp;
};

enum {
	Nrdre		= 2,	/* receive descriptor ring entries */
	Ntdre		= 2,	/* transmit descriptor ring entries */

	Rbsize		= 8,		/* ring buffer size (+3 for PID+CRC16) */
	Bufsize		= (Rbsize+7)&~7,	/* aligned */
};

enum {
	/* spi-specific BD flags */
	BDContin=	1<<9,	/* continuous mode */
	BDrxov=		1<<1,	/* overrun */
	BDtxun=		1<<1,	/* underflow */
	BDme=		1<<0,	/* multimaster error */
	BDrxerr=		BDrxov|BDme,
	BDtxerr=		BDtxun|BDme,

	/* spmod */
	MLoop=	1<<14,	/* loopback mode */
	MClockInv= 1<<13,	/* inactive state of SPICLK is high */
	MClockPhs= 1<<12,	/* SPCLK starts toggling at beginning of transfer */
	MDiv16=	1<<11,	/* use BRGCLK/16 as input to SPI baud rate */
	MRev=	1<<10,	/* normal operation */
	MMaster=	1<<9,
	MSlave=	0<<9,
	MEnable=	1<<8,
	/* LEN, PS fields */

	/* spie */
	MME =	1<<5,
	TXE =	1<<4,
	RES =	1<<3,
	BSY =	1<<2,
	TXB =	1<<1,
	RXB =	1<<0,
};

/*
 * software structures
 */
typedef struct Ctlr Ctlr;

struct Ctlr {
	QLock;
	int	init;
	SPI*	spi;
	SPIparam*	sp;

	int	rxintr;
	Rendez	ir;
	Ring;
};

static	Ctlr	spictlr[1];

static	void	interrupt(Ureg*, void*);

/*
 * test driver for SPI, host mode
 */
void
spireset(void)
{
	IMM *io;
	SPI *spi;
	SPIparam *sp;
	Ctlr *ctlr;

	io = m->iomem;
	sp = (SPIparam*)cpmparam(SPIP, sizeof(*sp));
	memset(sp, 0, sizeof(*sp));
	if(sp == nil){
		print("SPI: can't allocate new parameter memory\n");
		return;
	}

	spi = (SPI*)((ulong)m->iomem+0xAA0);
	ctlr = spictlr;

	/* step 1: select port pins */
	io->pbdir |= IBIT(30)|IBIT(29)|IBIT(28);
	io->pbpar |= IBIT(30)|IBIT(29)|IBIT(28);	/* SPICLK, SPIMOSI, SPIMISO */

	/* step 2: set CS pin - also disables SPISEL */
	io->pbpar &= ~(IBIT(31));
	io->pbdir |= IBIT(31);
	io->pbodr &= ~(IBIT(31));
	io->pbdat &= ~(IBIT(31));

	ctlr->spi = spi;
	ctlr->sp = sp;

	if(ioringinit(ctlr, Nrdre, Ntdre, Bufsize)<0)
		panic("spireset: ioringinit");

	/* step 3: configure rbase and tbase */
	sp->rbase = PADDR(ctlr->rdr);
	sp->tbase = PADDR(ctlr->tdr);

	/* step 4: do not issue InitRxTx command - due to microcode bug */

	/* step 5: */
	io->sdcr = 1;

	/* step 6: */
	sp->rfcr = 0x10;
	sp->tfcr = 0x10;

	/* step 7: */
	sp->mrblr = Bufsize;

	/* step 7.5: init other params because of bug in microcode */
	sp->rstate = 0;
	sp->rbptr = sp->rbase;
	sp->rcnt = 0;
	sp->tstate = 0;
	sp->tbptr = sp->tbase;
	sp->tcnt = 0;

	/* step 8-9: done by ioringinit */

	/* step 10: clear events */
	spi->spie = ~0;	

	/* step 11-12: enable interrupts  */
	intrenable(VectorCPIC+0x05, interrupt, spictlr, BUSUNKNOWN);
	spi->spim = ~0;

	/* step 13: enable in master mode, 8 bit chacters, slow clock */
	spi->spmode = MMaster|MRev|MDiv16|MEnable|(0x7<<4)|0xf;
print("spi->spmode = %ux\n", spi->spmode);
}

static void
interrupt(Ureg*, void *arg)
{
	int events;
	Ctlr *ctlr;
	SPI *spi;

	ctlr = arg;
	spi = ctlr->spi;
	events = spi->spie;
	spi->spie = events;
	print("SPI#%x\n", events);
}

void
spdump(SPIparam *sp)
{
	print("rbase = %ux\n", sp->rbase);
	print("tbase = %ux\n", sp->tbase);
	print("rfcr = %ux\n", sp->rfcr);
	print("tfcr = %ux\n", sp->tfcr);
	print("mrblr = %ux\n", sp->mrblr);
	print("rstate = %lux\n", sp->rstate);
	print("rptr = %lux\n", sp->rptr);
	print("rbptr = %ux\n", sp->rbptr);
	print("rcnt = %ux\n", sp->rcnt);
	print("rtmp = %lux\n", sp->rtmp);
	print("tstate = %lux\n", sp->tstate);
	print("tptr = %lux\n", sp->tptr);
	print("tbptr = %ux\n", sp->tbptr);
	print("tcnt = %ux\n", sp->tcnt);
	print("ttmp = %lux\n", sp->ttmp);
}

void
spitest(void)
{
	BD *dre;
	Block *b;
	int len;
	Ctlr *ctlr;
	ulong status;

	ctlr = spictlr;

	dre = &ctlr->tdr[ctlr->tdrh];
	if(dre->status & BDReady)
		panic("spitest: txstart");

print("dre->status %ux %ux\n", dre[0].status, dre[1].status);	
	b = allocb(4);
	b->wp[0] = 0xc0;
	b->wp[1] = 0x00;
	b->wp[2] = 0x00;
	b->wp[3] = 0x00;
	b->wp += 4;
	len = BLEN(b);
print("len = %d\n", len);
	dcflush(b->rp, len);
	if(ctlr->txb[ctlr->tdrh] != nil)
		panic("scc/ether: txstart");
	ctlr->txb[ctlr->tdrh] = b;
	dre->addr = PADDR(b->rp);
print("addr = %lux\n", PADDR(b->rp));
	dre->length = len;
	dre->status = (dre->status & BDWrap) | BDReady|BDInt|BDLast;
	eieio();
spdump(ctlr->sp);
m->iomem->pbdat |= IBIT(31);
microdelay(1);
	ctlr->spi->spcom = 1<<7;	/* transmit now */
print("cpcom = %p\n", &ctlr->spi->spcom);
spdump(ctlr->sp);
	eieio();
	ctlr->ntq++;
	ctlr->tdrh = NEXT(ctlr->tdrh, Ntdre);
print("going into loop %ux %ux\n", dre->status, ctlr->spi->spcom);
	while(dre->status&BDReady) {
print("ctrl->spi->spim = %ux %ux\n", ctlr->spi->spie, dre->status);
spdump(ctlr->sp);
		delay(10000);
	}
delay(100);
	dre = &ctlr->rdr[ctlr->rdrx];
	status = dre->status;
	len = dre->length;
print("%d status = %lux len=%d\n", ctlr->rdrx, status, len);
	b = iallocb(len);
	memmove(b->wp, KADDR(dre->addr), len);
	b->wp += len;
print("%ux %ux %ux %ux\n", b->rp[0], b->rp[1], b->rp[2], b->rp[3]);
	dcflush(KADDR(dre->addr), len);
	dre->status = (status & BDWrap) | BDEmpty | BDInt;
	ctlr->rdrx = NEXT(ctlr->rdrx, Nrdre);
m->iomem->pbdat &= ~(IBIT(31));
microdelay(1);
}


void
spiinit(void)
{
	spireset();
	spitest();
}

#ifdef XXX

static void
epprod(Ctlr *ctlr, int epn)
{
	ctlr->spi->uscom = FifoFill | (epn&3);
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
dumpspi(b, "TX");
	
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
			b->rp++;	/* skip DATAn */
		}
		len = BLEN(b);
		dcflush(b->rp, len);
		if(r->txb[r->tdrh] != nil)
			panic("spi: txstart");
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
				print("spi rx: %4.4ux %d ", status, dre->length);
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
					// spiiq(ctlr, b);
					dumpspi(b, "RX");
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
			print("spitx=#%.4x %8.8lux\n", dre->status, dre->addr);
			/* TO DO: error counting */
			b = r->txb[r->tdri];
			if(b == nil)
				panic("spi/interrupt: bufp");
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
dumpspi(Block *b, char *msg)
{
	int i;

	print("%s: %8.8lux [%d]: ", msg, (ulong)b->rp, BLEN(b));
	for(i=0; i<BLEN(b) && i < 16; i++)
		print(" %.2x", b->rp[i]);
	print("\n");
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

static int
spiinput(void *a)
{
	return ((Ctlr*)a)->rxintr;
}

static void
spitest(void)
{
	Ctlr *ctlr;
	Endpt *e, *e1;
	uchar msg[8];
	int epn, testing, i;

	ctlr = spictlr;
	epn = 0;
	testing = ctlr->spi->usmod & TEST;
	if(testing)
		epn = 1;
	e = &ctlr->pts[0];
	e1 = &ctlr->pts[1];
	memset(msg, 0, 8);
	msg[0] = 0;
	msg[1] = 8;	/* set configuration */
	msg[2] = 1;
	setaddr(e, 0, epn);
	sendtoken(e, TokSETUP);
	senddata(e, msg, 8);
	if(!testing){
		for(i=0; i<8; i++)
			msg[i] = 0x40+i;
		senddata(e1, msg, 8);
		e->rxintr = 0;
		sendtoken(e, TokIN);
		transmit(e);
		epprod(ctlr, 0);
		tsleep(&e->ir, spiinput, e, 1000);
		if(!spiinput(e))
			print("RX: none\n");
	}
	sendtoken(e, TokSETUP);
	msg[0] = 0x23;
	msg[1] = 3;	/* set feature */
	msg[2] = 8;	/* port power */
	msg[3] = 0;
	msg[4] = 1;	/* port 1 */
	senddata(e, msg, 8);
	if(!testing){
		for(i=0; i<8; i++)
			msg[i] = 0x60+i;
		senddata(e1, msg, 8);
		e->rxintr = 0;
		sendtoken(e, TokIN);
		transmit(e);
		epprod(ctlr, 0);
		tsleep(&e->ir, spiinput, e, 1000);
		if(!spiinput(e))
			print("RX: none\n");
	}
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


#endif
