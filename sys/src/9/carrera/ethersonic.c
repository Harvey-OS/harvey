#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"../port/netif.h"

#include "etherif.h"

/*
 * National Semiconductor DP83932
 * Systems-Oriented Network Interface Controller
 * (SONIC)
 */

#define SONICADDR	((Sonic*)Sonicbase)

#define RD(rn)		(delay(0), *(ulong*)((ulong)&SONICADDR->rn^4))
#define WR(rn, v)	(delay(0), *(ulong*)((ulong)&SONICADDR->rn^4) = (v))
#define ISquad(s)	if((ulong)s & 0x7) panic("sonic: Quad alignment");

typedef struct Pbuf Pbuf;
struct Pbuf
{
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];
	uchar	data[1500];
	uchar	crc[4];
};

typedef struct
{
	ulong	cr;		/* command */
	ulong	dcr;		/* data configuration */
	ulong	rcr;		/* receive control */
	ulong	tcr;		/* transmit control */
	ulong	imr;		/* interrupt mask */
	ulong	isr;		/* interrupt status */
	ulong	utda;		/* upper transmit descriptor address */
	ulong	ctda;		/* current transmit descriptor address */
	ulong	pad0x08[5];	/*  */
	ulong	urda;		/* upper receive descriptor address */
	ulong	crda;		/* current receive descriptor address */
	ulong	crba0;		/* DO NOT WRITE THESE */
	ulong	crba1;
	ulong	rbwc0;
	ulong	rbwc1;
	ulong	eobc;		/* end of buffer word count */
	ulong	urra;		/* upper receive resource address */
	ulong	rsa;		/* resource start address */
	ulong	rea;		/* resource end address */
	ulong	rrp;		/* resource read pointer */
	ulong	rwp;		/* resource write pointer */
	ulong	pad0x19[8];	/*  */
	ulong	cep;		/* CAM entry pointer */
	ulong	cap2;		/* CAM address port 2 */
	ulong	cap1;		/* CAM address port 1 */
	ulong	cap0;		/* CAM address port 0 */
	ulong	ce;		/* CAM enable */
	ulong	cdp;		/* CAM descriptor pointer */
	ulong	cdc;		/* CAM descriptor count */
	ulong	sr;		/* silicon revision */
	ulong	wt0;		/* watchdog timer 0 */
	ulong	wt1;		/* watchdog timer 1 */
	ulong	rsc;		/* receive sequence counter */
	ulong	crct;		/* CRC error tally */
	ulong	faet;		/* FAE tally */
	ulong	mpt;		/* missed packet tally */
	ulong	mdt;		/* maximum deferral timer */
	ulong	pad0x30[15];	/*  */
	ulong	dcr2;		/* data configuration 2 */
} Sonic;

enum
{
	Nrb		= 16,		/* receive buffers */
	Ntb		= 8,		/* transmit buffers */
};

enum
{
	Htx	= 0x0001,	/* halt transmission */
	Txp	= 0x0002,	/* transmit packet(s) */
	Rxdis	= 0x0004,	/* receiver disable */
	Rxen	= 0x0008,	/* receiver enable */
	Stp	= 0x0010,	/* stop timer */
	St	= 0x0020,	/* start timer */
	Rst	= 0x0080,	/* software reset */
	Rrra	= 0x0100,	/* read RRA */
	Lcam	= 0x0200,	/* load CAM */

	Dw32	= 0x0020,	/* data width select */
	Sterm	= 0x0400,	/* synchronous termination */
	Lbr	= 0x4000,	/* latched bus retry */
	Efm	= 0x0010,	/* Empty fill mode */
	W14tf	= 0x0003,	/* 14 Word transmit fifo */

	Prx	= 0x0001,	/* packet received ok */
	Fae	= 0x0004,	/* frame alignment error */
	Crc	= 0x0008,	/* CRC error */
	Lpkt	= 0x0040,	/* last packet in rba */
	Bc	= 0x0080,	/* broadcast packet received */
	Pro	= 0x1000,	/* physical promiscuous mode */
	Brd	= 0x2000,	/* accept broadcast packets */
	Rnt	= 0x4000,	/* accept runt packets */
	Err	= 0x8000,	/* accept packets with errors */

	Ptx	= 0x0001,	/* packet transmitted ok */
	Pintr	= 0x8000,	/* programmable interrupt */

	Rfo	= 0x0001,	/* receive fifo overrun */
	MpTally	= 0x0002,	/* missed packet tally counter rollover */
	FaeTally= 0x0004,	/* frame alignment error tally counter rollover */
	CrcTally= 0x0008,	/* Crc tally counter rollover */
	Rbae	= 0x0010,	/* receive buffer area exceeded */
	Rbe	= 0x0020,	/* receive buffer exhausted */
	Rde	= 0x0040,	/* receive descriptors exhausted */
	Txer	= 0x0100,	/* transmit error */
	Txdn	= 0x0200,	/* transmission done */
	Pktrx	= 0x0400,	/* packet received */
	Pint	= 0x0800,	/* programmed interrupt */
	Lcd	= 0x1000,	/* load CAM done */
	Hbl	= 0x2000,	/* CD heartbeat lost */
	Br	= 0x4000,	/* bus retry occurred */
	AllIntr	= 0x7771,	/* all of the above */

	Rxbuf	= sizeof(Pbuf)+4,
	Txbuf	= sizeof(Pbuf),
};

/*
 * Receive Resource Descriptor.
 */
typedef struct
{
	ushort	pad1;
	ushort		ptr1;		/* buffer pointer in the RRA */
	ushort  pad2;
	ushort		ptr0;
	ushort  pad3;
	ushort		wc1;		/* buffer word count in the RRA */
	ushort  pad4;
	ushort		wc0;
} RXrsc;

/*
 * Receive Packet Descriptor.
 */
typedef struct
{
	ushort	pad0;
		ushort	count;		/* packet byte count */
	ushort	pad1;
		ushort	status;		/* receive status */
	ushort	pad2;
		ushort	ptr1;		/* buffer pointer */
	ushort	pad3;
		ushort	ptr0;
	ushort  pad4;
		ushort	link;		/* descriptor link and EOL */
	ushort	pad5;
		ushort	seqno;		/*  */
	ulong	pad6;
	ushort  pad7;
		ushort	owner;		/* in use */
} RXpkt;

/*
 * Transmit Packet Descriptor.
 */
typedef struct
{
	ushort	pad1;
		ushort	config;		/*  */
	ushort	pad0;
		ushort	status;		/* transmit status */
	ushort	pad3;
		ushort	count;		/* fragment count */
	ushort	pad2;
		ushort	size;		/* byte count of entire packet */
	ushort	pad5;
		ushort	ptr1;
	ushort	pad4;
		ushort	ptr0;		/* packet pointer */
	ushort	pad7;
		ushort	link;		/* descriptor link */
	ushort	pad6;
		ushort	fsize;		/* fragment size */
} TXpkt;

enum{
	Eol		= 1,	/* end of list bit in descriptor link */
	Host		= 0,	/* descriptor belongs to host */
	Interface	= -1,	/* descriptor belongs to interface */
};

/*
 * CAM Descriptor
 */
typedef struct
{
	ushort	pad0;
		ushort	cap0;		/* CAM address port 0 */
	ushort	pad1;
		ushort	cep;		/* CAM entry pointer */
	ushort	pad2;
		ushort	cap2;		/* CAM address port 2 */
	ushort	pad3;
		ushort	cap1;		/* CAM address port 1 */
	ulong	pad4;
	ushort	pad5;
		ushort	ce;		/* CAM enable */
} Cam;

typedef struct Ctlr {
	Lock	lock;

	Lock	tlock;
	int	th;		/* first transmit buffer owned by host */	
	int	ti;		/* first transmit buffer owned by interface */
	int	ntq;

	int	rh;		/* first receive buffer owned by host */
	int	ri;		/* first receive buffer owned by interface */

	RXrsc*	rra;		/* receive resource area */
	RXpkt*	rda;		/* receive descriptor area */
	TXpkt*	tda;		/* transmit descriptor area */
	Cam*	cda;		/* CAM descriptor area */

	ulong*	rb[Nrb];	/* receive buffer area */
	ulong*	tb[Ntb];	/* transmit buffer area */
} Ctlr;

static Ether* etherxx;

#define LS16(addr)	(PADDR(addr) & 0xFFFF)
#define MS16(addr)	((PADDR(addr)>>16) & 0xFFFF)

static void
attach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;

	ilock(&ctlr->lock);
	if(!(RD(cr) & Rxen))
		WR(cr, Rxen);
	iunlock(&ctlr->lock);
}

static void
promiscuous(void* arg, int on)
{
	Ctlr *ctlr;
	ushort rcr;

	ctlr = ((Ether*)arg)->ctlr;

	ilock(&ctlr->lock);
	rcr = RD(rcr);
	if(on)
		rcr |= Pro;
	else
		rcr &= ~Pro;
	WR(rcr, rcr);
	iunlock(&ctlr->lock);
}

static void
wus(ushort* a, ushort v)
{
	a[0] = v;
	a[-1] = v;
}

static void
swizzle(uchar* to, ulong *from, int nbytes)
{
	ulong *l, l0, l1;
	uchar *p;

	p = to;
	l = from;
	for(nbytes = ROUNDUP(nbytes, 8); nbytes; nbytes -= 8){
		l1 = *l++;
		l0 = *l++;
		p[0] = l0;
		p[1] = l0>>8;
		p[2] = l0>>16;
		p[3] = l0>>24;
		p[4] = l1;
		p[5] = l1>>8;
		p[6] = l1>>16;
		p[7] = l1>>24;
		p += 8;
	}
}

static void
swozzle(ulong* to, uchar *from, int nbytes)
{
	ulong *l;
	uchar *p;

	l = to;
	p = from;
	for(nbytes = ROUNDUP(nbytes, 8); nbytes; nbytes -= 8){
		*l++ = (p[7]<<24)|(p[6]<<16)|(p[5]<<8)|p[4];
		*l++ = (p[3]<<24)|(p[2]<<16)|(p[1]<<8)|p[0];
		p += 8;
	}
}

static void
txstart(Ether* ether)
{
	Ctlr *ctlr;
	Block *bp;
	ushort *s;
	TXpkt *txpkt;
	int len;

	ctlr = ether->ctlr;
	txpkt = &ctlr->tda[ctlr->th];
	while(txpkt->status == Host){
		bp = qget(ether->oq);
		if(bp == nil)
			break;

		len = BLEN(bp);
		swozzle(ctlr->tb[ctlr->th], bp->rp, len);

		txpkt->size = len;
		txpkt->fsize = len;
		wus(&txpkt->link, txpkt->link|Eol);
		txpkt->status = Interface;
		s = &ctlr->tda[PREV(ctlr->th, Ntb)].link;
		wus(s, *s & ~Eol);

		ctlr->th = NEXT(ctlr->th, Ntb);
		txpkt = &ctlr->tda[ctlr->th];
		WR(cr, Txp);

		freeb(bp);
	}
}

void
etherintr(void)
{
	Ether *ether;
	Ctlr *ctlr;
	ushort *s;
	ulong status;
	TXpkt *txpkt;
	RXpkt *rxpkt;
	int len;
	Block *bp;

	ether = etherxx;
	ctlr = ether->ctlr;

	for(status = (RD(isr) & AllIntr); status; status = (RD(isr) & AllIntr)){
		/*
		 * Warnings that something is atoe.
		 */
		if(status & Hbl){
			WR(isr, Hbl);
			status &= ~Hbl;
			print("sonic: cd heartbeat lost\n");
		}
		if(status & Br){
			WR(cr, Rst);
			print("sonic: bus retry occurred\n");
			(*(void(*)(void))0xA001C020)();
			status &= ~Br;
		}
	
		/*
		 * Transmission complete, for good or bad.
		 */
		if(status & (Txdn|Txer)){
			txpkt = &ctlr->tda[ctlr->ti];
			while(txpkt->status != Host){
				if(txpkt->status == Interface){
					WR(ctda, LS16(txpkt));
					WR(cr, Txp);
					break;
				}
	
				if((txpkt->status & Ptx) == 0)
					ether->oerrs++;
	
				txpkt->status = Host;
				ctlr->ti = NEXT(ctlr->ti, Ntb);
				txpkt = &ctlr->tda[ctlr->ti];
			}
			WR(isr, status & (Txdn|Txer));
			status &= ~(Txdn|Txer);

			lock(&ctlr->tlock);
			txstart(ether);
			unlock(&ctlr->tlock);
		}

		if((status & (Pktrx|Rde)) == 0)
			goto noinput;

		/*
		 * A packet arrived or we ran out of descriptors.
		 */
		rxpkt = &ctlr->rda[ctlr->rh];
		while(rxpkt->owner == Host){
			ether->inpackets++;
	
			/*
			 * If the packet was received OK, pass it up,
			 * otherwise log the error.
			 */
			if(rxpkt->status & Prx){
				/*
				 * Sonic delivers CRC as part of the packet count.
				 */
				len = (rxpkt->count & 0xFFFF)-4;
				if(bp = iallocb(ROUNDUP(len, 8))){
					swizzle(bp->rp, ctlr->rb[ctlr->rh], len);
					bp->wp += len;
					etheriq(ether, bp, 1);
				}
			}
			else if(rxpkt->status & Fae)
				ether->frames++;
			else if(rxpkt->status & Crc)
				ether->crcs++;
			else
				ether->buffs++;
			rxpkt->status = 0;

			/*
			 * Finished with this packet, it becomes the
			 * last free packet in the ring, so give it Eol,
			 * and take the Eol bit off the previous packet.
			 * Move the ring index on.
			 */
			wus(&rxpkt->link,  rxpkt->link|Eol);
			rxpkt->owner = Interface;
			s = &ctlr->rda[PREV(ctlr->rh, Nrb)].link;
			wus(s, *s & ~Eol);
			ctlr->rh = NEXT(ctlr->rh, Nrb);
	
			rxpkt = &ctlr->rda[ctlr->rh];
		}
		WR(isr, status & (Pktrx|Rde));
		status &= ~(Pktrx|Rde);

	noinput:
		/*
		 * We get a 'load CAM done' interrupt
		 * after initialisation. Ignore it.
		 */
		if(status & Lcd) {
			WR(isr, Lcd);
			status &= ~Lcd;
		}
	
		if(status & AllIntr) {
			WR(isr, status);
			print("sonic #%lux\n", status);
		}
	}
}

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->tlock);
	txstart(ether);
	iunlock(&ctlr->tlock);
}

static void
reset(Ether* ether)
{
	Ctlr *ctlr;
	int i;
	ushort lolen, hilen, loadr, hiadr;

	ctlr = ether->ctlr;

	/*
	 * Reset the SONIC, toggle the Rst bit.
	 * Set the data config register for synchronous termination
	 * and 32-bit data-path width.
	 * Setup the descriptor and buffer area.
	 */
	WR(cr, Rst);
	WR(dcr, 0x2423);	/* 5-19 Carrera manual */
	WR(cr, 0);

	/*
	 * Initialise the receive resource area (RRA) and
	 * the receive descriptor area (RDA).
	 *
	 * We use a simple scheme of one packet per descriptor.
	 * We achieve this by setting the EOBC register to be
	 * 2 (16-bit words) less than the buffer size;
	 * thus the size of the receive buffers must be sizeof(Pbuf)+4.
	 * Set up the receive descriptors as a ring.
	 */

	lolen = (Rxbuf/2) & 0xFFFF;
	hilen = ((Rxbuf/2)>>16) & 0xFFFF;

	for(i = 0; i < Nrb; i++) {
		wus(&ctlr->rra[i].wc0, lolen);
		wus(&ctlr->rra[i].wc1, hilen);

		ctlr->rda[i].link =  LS16(&ctlr->rda[NEXT(i, Nrb)]);
		ctlr->rda[i].owner = Interface;

		loadr = LS16(ctlr->rb[i]);
		wus(&ctlr->rra[i].ptr0, loadr);
		wus(&ctlr->rda[i].ptr0, loadr);

		hiadr = MS16(ctlr->rb[i]);
		wus(&ctlr->rra[i].ptr1, hiadr);
		wus(&ctlr->rda[i].ptr1, hiadr);
	}

	/*
	 * Check the important resources are QUAD aligned
	 */
	ISquad(ctlr->rra);
	ISquad(ctlr->rda);

	/*
	 * Terminate the receive descriptor ring
	 * and load the SONIC registers to describe the RDA.
	 */
	ctlr->rda[Nrb-1].link |= Eol;

	WR(crda, LS16(ctlr->rda));
	WR(urda, MS16(ctlr->rda));
	WR(eobc, Rxbuf/2 - 2);

	/*
	 * Load the SONIC registers to describe the RRA.
	 * We set the rwp to beyond the area delimited by rsa and
	 * rea. This means that since we've already allocated all
	 * the buffers, we'll never get a 'receive buffer area
	 * exhausted' interrupt and the rrp will just wrap round.
	 */
	WR(urra, MS16(&ctlr->rra[0]));
	WR(rsa, LS16(&ctlr->rra[0]));
	WR(rrp, LS16(&ctlr->rra[0]));
	WR(rea, LS16(&ctlr->rra[Nrb]));
	WR(rwp, LS16(&ctlr->rra[Nrb+1]));

	/*
	 * Initialise the transmit descriptor area (TDA).
	 * Each descriptor describes one packet, we make no use
	 * of having the packet in multiple fragments.
	 * The descriptors are linked in a ring; overlapping transmission
	 * with buffer queueing will cause some packets to
	 * go out back-to-back.
	 *
	 * Load the SONIC registers to describe the TDA.
	 */
	for(i = 0; i < Ntb; i++){
		ctlr->tda[i].status = Host;
		ctlr->tda[i].config = 0;
		ctlr->tda[i].count = 1;
		ctlr->tda[i].ptr0 = LS16(ctlr->tb[i]);
		ctlr->tda[i].ptr1 = MS16(ctlr->tb[i]);
		ctlr->tda[i].link = LS16(&ctlr->tda[NEXT(i, Ntb)]);
	}

	WR(ctda, LS16(&ctlr->tda[0]));
	WR(utda, MS16(&ctlr->tda[0]));

	/*
	 * Initialise the software receive and transmit
	 * ring indexes.
	 */
	ctlr->rh = 0;
	ctlr->ri = 0;
	ctlr->th = 0;
	ctlr->ti = 0;

	/*
	 * Initialise the CAM descriptor area (CDA).
	 * We only have one ethernet address to load,
	 * broadcast is defined by the SONIC as all 1s.
	 *
	 * Load the SONIC registers to describe the CDA.
	 */
	ctlr->cda->cep = 0;
	ctlr->cda->cap0 = (ether->addr[1]<<8)|ether->addr[0];
	ctlr->cda->cap1 = (ether->addr[3]<<8)|ether->addr[2];
	ctlr->cda->cap2 = (ether->addr[5]<<8)|ether->addr[4];
	ctlr->cda->ce = 1;

	WR(cdp, LS16(ctlr->cda));
	WR(cdc, 1);

	/*
	 * Load the Resource Descriptors and Cam contents
	 */
	WR(cr, Rrra);
	while(RD(cr) & Rrra)
		;

	WR(cr, Lcam);
	while(RD(cr) & Lcam)
		;

	/*
	 * Configure the receive control, transmit control
	 * and interrupt-mask registers.
	 * The SONIC is now initialised, but not enabled.
	 */
	WR(rcr, Brd);
	WR(tcr, 0);
	WR(imr, AllIntr);
}

static void
initbufs(Ctlr* ctlr)
{
	int i;
	uchar *mem, *base;

	/* Put the ethernet buffers in the same place
	 * as the bootrom
	 */
	mem = (void*)(KZERO|0x2000);
	base = mem;
	mem = CACHELINE(uchar, mem);

	/*
	 * Descriptors must be built in uncached space
	 */
	ctlr->rra = UNCACHED(RXrsc, mem);
	mem = QUAD(uchar, mem+Nrb*sizeof(RXrsc));

	ctlr->rda = UNCACHED(RXpkt, mem);
	mem = QUAD(uchar, mem+Nrb*sizeof(RXpkt));

	ctlr->tda = UNCACHED(TXpkt, mem);
	mem = QUAD(uchar, mem+Ntb*sizeof(TXpkt));

	ctlr->cda = UNCACHED(Cam, mem);

	mem = CACHELINE(uchar, mem+sizeof(Cam));
	for(i = 0; i < Nrb; i++) {
		ctlr->rb[i] = UNCACHED(ulong, mem);
		mem += sizeof(Pbuf)+4;
		mem = QUAD(uchar, mem);
	}
	for(i = 0; i < Ntb; i++) {
		ctlr->tb[i] = UNCACHED(ulong, mem);
		mem += sizeof(Pbuf);
		mem = QUAD(uchar, mem);
	}
	if(mem >= base+64*1024)
		panic("sonic init");
}

static int
sonicreset(Ether* ether)
{
	Ctlr *ctlr;

	if(ether->ctlrno != 0)
		return 1;

	/*
	 * Map the device registers and allocate
	 * memory for the receive/transmit rings.
	 * Set the physical ethernet address and
	 * prime the interrupt handler.
	 */
	ether->ctlr = malloc(sizeof(Ctlr));
	ctlr = ether->ctlr;
	ether->port = Sonicbase;

	initbufs(ctlr);
	enetaddr(ether->addr);

	reset(ether);

	/*
	 * Hack: etherintr is called directly from trap with no arguments.
	 * There can only be one sonic, so keep a note of the generic ether
	 * struct.
	 */
	etherxx = ether;

	return 0;
}

static Endev sonicdev = {
	"sonic",

	sonicreset,
	attach,
	transmit,
	nil,				/* interrupt - etherintr called directly from trap */
	nil,				/* ifstat */
	promiscuous,
	nil,				/* multicast */

	0, 0,				/* vid, did */
};

Endev* endev[] = {
	&sonicdev,
	nil,
};
