/*
 * marvell kirkwood ethernet (88e1116) driver
 * (as found in the sheevaplug & openrd).
 * from /public/doc/marvell/88f61xx.kirkwood.pdf
 * and  /public/doc/marvell/88e1116.pdf.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"
#include "ethermii.h"
#include "../ip/ip.h"

#define	MIIDBG	if(0)iprint

enum {
	Gbe0regs	= PHYSIO + 0x72000,
	Gbe1regs	= PHYSIO + 0x76000,

	Nrx		= 512,
	Ntx		= 512,
	Nrxblks		= 1024,
	Rxblklen	= 2+1522,  /* ifc. supplies first 2 bytes as padding */

	Maxrxintrsec	= 20*1000,	/* max. rx intrs. / sec */
	Etherstuck	= 90,	/* must send or receive a packet in this many sec.s */

	Descralign	= 16,
	Bufalign	= 8,

	Pass		= 1,		/* accept packets */

	Qno		= 0,		/* do everything on queue zero */
};

typedef struct Ctlr Ctlr;
typedef struct Gbereg Gbereg;
typedef struct Mibstats Mibstats;
typedef struct Rx Rx;
typedef struct Tx Tx;

static struct {
	Lock;
	Block	*head;
} freeblocks;

/* hardware receive buffer descriptor */
struct Rx {
	ulong	cs;
	ulong	countsize;	/* bytes, buffer size */
	ulong	buf;		/* phys. addr. of packet buffer */
	ulong	next;		/* phys. addr. of next Rx */
};

/* hardware transmit buffer descriptor */
struct Tx {
	ulong	cs;
	ulong	countchk;	/* bytes, checksum */
	ulong	buf;		/* phys. addr. of packet buffer */
	ulong	next;		/* phys. addr. of next Tx */
};

/* fixed by hw; part of Gberegs */
struct Mibstats {
	union {
		uvlong	rxby;		/* good bytes rcv'd */
		struct {
			ulong rxbylo;
			ulong rxbyhi;
		};
	};
	ulong	badrxby;		/* bad bytes rcv'd */
	ulong	mactxerr;		/* tx err pkts */
	ulong	rxpkt;			/* good pkts rcv'd */
	ulong	badrxpkt;		/* bad pkts rcv'd */
	ulong	rxbcastpkt;		/* b'cast pkts rcv'd */
	ulong	rxmcastpkt;		/* m'cast pkts rcv'd */

	ulong	rx64;			/* pkts <= 64 bytes */
	ulong	rx65_127;		/* pkts 65—127 bytes */
	ulong	rx128_255;		/* pkts 128—255 bytes */
	ulong	rx256_511;		/* pkts 256—511 bytes */
	ulong	rx512_1023;		/* pkts 512—1023 bytes */
	ulong	rx1024_max;		/* pkts >= 1024 bytes */

	union {
		uvlong	txby;		/* good bytes sent */
		struct {
			ulong txbylo;
			ulong txbyhi;
		};
	};
	ulong	txpkt;			/* good pkts sent */
	/* half-duplex: pkts dropped due to excessive collisions */
	ulong	txcollpktdrop;
	ulong	txmcastpkt;		/* m'cast pkts sent */
	ulong	txbcastpkt;		/* b'cast pkts sent */

	ulong	badmacctlpkts;		/* bad mac ctl pkts */
	ulong	txflctl;		/* flow-control pkts sent */
	ulong	rxflctl;		/* good flow-control pkts rcv'd */
	ulong	badrxflctl;		/* bad flow-control pkts rcv'd */

	ulong	rxundersized;		/* runts */
	ulong	rxfrags;		/* fragments rcv'd */
	ulong	rxtoobig;		/* oversized pkts rcv'd */
	ulong	rxjabber;		/* jabber pkts rcv'd */
	ulong	rxerr;			/* rx error events */
	ulong	crcerr;			/* crc error events */
	ulong	collisions;		/* collision events */
	ulong	latecoll;		/* late collisions */
};

struct Ctlr {
	Lock;
	Gbereg	*reg;

	Lock	initlock;
	int	init;

	Rx	*rx;		/* receive descriptors */
	Block	*rxb[Nrx];	/* blocks belonging to the descriptors */
	int	rxhead;		/* descr ethernet will write to next */
	int	rxtail;		/* next descr that might need a buffer */
	Rendez	rrendez;	/* interrupt wakes up read process */
	int	haveinput;

	Tx	*tx;
	Block	*txb[Ntx];
	int	txhead;		/* next descr we can use for new packet */
	int	txtail;		/* next descr to reclaim on tx complete */

	Mii	*mii;
	int	port;

	/* stats */
	ulong	intrs;
	ulong	newintrs;
	ulong	txunderrun;
	ulong	txringfull;
	ulong	rxdiscard;
	ulong	rxoverrun;
	ulong	nofirstlast;

	Mibstats;
};

#define	Rxqon(q)	(1<<(q))
#define	Txqon(q)	(1<<(q))

enum {
	/* sdma config, sdc */
	Burst1		= 0,
	Burst2,
	Burst4,
	Burst8,
	Burst16,
	SDCrifb		= 1<<0,		/* rx intr on pkt boundaries */
#define SDCrxburst(v)	((v)<<1)
	SDCrxnobyteswap	= 1<<4,
	SDCtxnobyteswap	= 1<<5,
	SDCswap64byte	= 1<<6,
#define SDCtxburst(v)	((v)<<22)
	/* rx intr ipg (inter packet gap) */
#define SDCipgintrx(v)	((((v)>>15) & 1)<<25) | (((v) & MASK(15))<<7)

	/* portcfg */
	PCFGupromisc		= 1<<0,	/* unicast promiscuous mode */
#define Rxqdefault(q)	((q)<<1)
#define Rxqarp(q)	((q)<<4)
	PCFGbcrejectnoiparp	= 1<<7,
	PCFGbcrejectip		= 1<<8,
	PCFGbcrejectarp		= 1<<9,
	PCFGamnotxes		= 1<<12, /* auto mode, no summary update on tx */
	PCFGtcpq	= 1<<14,
	PCFGudpq	= 1<<15,
#define	Rxqtcp(q)	((q)<<16)
#define	Rxqudp(q)	((q)<<19)
#define	Rxqbpdu(q)	((q)<<22)
	PCFGrxcs	= 1<<25,	/* rx tcp checksum mode with header */

	/* portcfgx */
	PCFGXspanq	= 1<<1,
	PCFGXcrcoff	= 1<<2,		/* no ethernet crc */

	/* port serial control0, psc0 */
	PSC0porton		= 1<<0,
	PSC0forcelinkup		= 1<<1,
	PSC0an_dplxoff		= 1<<2,	/* an_ = auto. negotiate */
	PSC0an_flctloff		= 1<<3,
	PSC0an_pauseadv		= 1<<4,
	PSC0nofrclinkdown	= 1<<10,
	PSC0an_spdoff		= 1<<13,
	PSC0dteadv		= 1<<14,

	/* max. input pkt size */
#define PSC0mru(v)	((v)<<17)
	PSC0mrumask	= PSC0mru(MASK(3)),
	PSC0mru1518	= 0,		/* 1500+2* 6(addrs) +2 + 4(crc) */
	PSC0mru1522,			/* 1518 + 4(vlan tags) */
	PSC0mru1552,			/* `baby giant' */
	PSC0mru9022,			/* `jumbo' */
	PSC0mru9192,			/* bigger jumbo */
	PSC0mru9700,			/* still bigger jumbo */

	PSC0fd_frc		= 1<<21,	/* force full duplex */
	PSC0flctlfrc		= 1<<22,
	PSC0gmiispd_gbfrc	= 1<<23,
	PSC0miispdfrc100mbps	= 1<<24,

	/* port status 0, ps0 */
	PS0linkup	= 1<<1,
	PS0fd		= 1<<2,			/* full duplex */
	PS0flctl	= 1<<3,
	PS0gmii_gb	= 1<<4,
	PS0mii100mbps	= 1<<5,
	PS0txbusy	= 1<<7,
	PS0txfifoempty	= 1<<10,
	PS0rxfifo1empty	= 1<<11,
	PS0rxfifo2empty	= 1<<12,

	/* port serial control 1, psc1 */
	PSC1loopback	= 1<<1,
	PSC1mii		= 0<<2,
	PSC1rgmii	= 1<<3,			/* enable RGMII */
	PSC1portreset	= 1<<4,
	PSC1clockbypass	= 1<<5,
	PSC1iban	= 1<<6,
	PSC1iban_bypass	= 1<<7,
	PSC1iban_restart= 1<<8,
	PSC1_gbonly	= 1<<11,
	PSC1encolonbp	= 1<<15, /* "collision during back-pressure mib counting" */
	PSC1coldomlimmask= MASK(6)<<16,
#define PSC1coldomlim(v) (((v) & MASK(6))<<16)
	PSC1miiallowoddpreamble	= 1<<22,

	/* port status 1, ps1 */
	PS1rxpause	= 1<<0,
	PS1txpause	= 1<<1,
	PS1pressure	= 1<<2,
	PS1syncfail10ms	= 1<<3,
	PS1an_done	= 1<<4,
	PS1inbandan_bypassed	= 1<<5,
	PS1serdesplllocked	= 1<<6,
	PS1syncok	= 1<<7,
	PS1nosquelch	= 1<<8,

	/* irq */
	Irx		= 1<<0,
	Iextend		= 1<<1,
#define Irxbufferq(q)	(1<<((q)+2))
	Irxerr		= 1<<10,
#define Irxerrq(q)	(1<<((q)+11))
#define Itxendq(q)	(1<<((q)+19))
	Isum		= 1<<31,

	/* irq extended, irqe */
#define	IEtxbufferq(q)	(1<<((q)+0))
#define	IEtxerrq(q)	(1<<((q)+8))
	IEphystschg	= 1<<16,
	IEptp		= 1<<17,
	IErxoverrun	= 1<<18,
	IEtxunderrun	= 1<<19,
	IElinkchg	= 1<<20,
	IEintaddrerr	= 1<<23,
	IEprbserr	= 1<<25,
	IEsum		= 1<<31,

	/* tx fifo urgent threshold (tx interrupt coalescing), pxtfut */
#define TFUTipginttx(v)	(((v) & MASK(16))<<4);

	/* minimal frame size, mfs */
	MFS40by	= 10<<2,
	MFS44by	= 11<<2,
	MFS48by	= 12<<2,
	MFS52by	= 13<<2,
	MFS56by	= 14<<2,
	MFS60by	= 15<<2,
	MFS64by	= 16<<2,

	/* receive descriptor */
#define Bufsize(v)	((v)<<3)

	/* receive descriptor status */
	RCSmacerr	= 1<<0,
	RCSmacmask	= 3<<1,
	RCSmacce	= 0<<1,
	RCSmacor	= 1<<1,
	RCSmacmf	= 2<<1,
	RCSl4chkshift	= 3,
	RCSl4chkmask	= MASK(16),
	RCSvlan		= 1<<17,
	RCSbpdu		= 1<<18,
	RCSl4mask	= 3<<21,
	RCSl4tcp4	= 0<<21,
	RCSl4udp4	= 1<<21,
	RCSl4other	= 2<<21,
	RCSl4rsvd	= 3<<21,
	RCSl2ev2	= 1<<23,
	RCSl3ip4	= 1<<24,
	RCSip4headok	= 1<<25,
	RCSlast		= 1<<26,
	RCSfirst	= 1<<27,
	RCSunknownaddr	= 1<<28,
	RCSenableintr	= 1<<29,
	RCSl4chkok	= 1<<30,
	RCSdmaown	= 1<<31,

	/* transmit descriptor status */
	TCSmacerr	= 1<<0,
	TCSmacmask	= 3<<1,
	TCSmaclc	= 0<<1,
	TCSmacur	= 1<<1,
	TCSmacrl	= 2<<1,
	TCSllc		= 1<<9,
	TCSl4chkmode	= 1<<10,
	TCSipv4hdlenshift= 11,
	TCSvlan		= 1<<15,
	TCSl4type	= 1<<16,
	TCSgl4chk	= 1<<17,
	TCSgip4chk	= 1<<18,
	TCSpadding	= 1<<19,
	TCSlast		= 1<<20,
	TCSfirst	= 1<<21,
	TCSenableintr	= 1<<23,
	TCSautomode	= 1<<30,
	TCSdmaown	= 1<<31,
};

enum {
	/* SMI regs */
	PhysmiTimeout	= 10000,	/* what units? in ms. */
	Physmidataoff	= 0,		/* Data */
	Physmidatamask	= 0xffff<<Physmidataoff,

	Physmiaddroff 	= 16,		/* PHY device addr */
	Physmiaddrmask	= 0x1f << Physmiaddroff,

	Physmiop	= 26,
	Physmiopmask	= 3<<Physmiop,
	PhysmiopWr	= 0<<Physmiop,
	PhysmiopRd	= 1<<Physmiop,

	PhysmiReadok	= 1<<27,
	PhysmiBusy	= 1<<28,

	SmiRegaddroff	= 21,		/* PHY device register addr */
	SmiRegaddrmask	= 0x1f << SmiRegaddroff,
};

struct Gbereg {
	ulong	phy;			/* PHY address */
	ulong	smi;			/* serial mgmt. interface */
	ulong	euda;			/* ether default address */
	ulong	eudid;			/* ether default id */
	uchar	_pad0[0x80-0x10];

	ulong	euirq;			/* interrupt cause */
	ulong	euirqmask;		/* interrupt mask */
	uchar	_pad1[0x94-0x88];

	ulong	euea;			/* error address */
	ulong	euiae;			/* internal error address */
	uchar	_pad2[0xb0-0x9c];

	ulong	euc;			/* control */
	uchar	_pad3[0x200-0xb4];

	struct {
		ulong	base;		/* window base */
		ulong	size;		/* window size */
	} base[6];
	uchar	_pad4[0x280-0x230];

	ulong	harr[4];		/* high address remap */
	ulong	bare;			/* base address enable */
	ulong	epap;			/* port access protect */
	uchar	_pad5[0x400-0x298];

	ulong	portcfg;		/* port configuration */
	ulong	portcfgx;		/* port config. extend */
	ulong	mii;			/* mii serial parameters */
	ulong	_pad6;
	ulong	evlane;			/* vlan ether type */
	ulong	macal;			/* mac address low */
	ulong	macah;			/* mac address high */
	ulong	sdc;			/* sdma config. */
	ulong	dscp[7];		/* ip diff. serv. code point -> pri */
	ulong	psc0;			/* port serial control 0 */
	ulong	vpt2p;			/* vlan priority tag -> pri */
	ulong	ps0;			/* ether port status 0 */
	ulong	tqc;			/* transmit queue command */
	ulong	psc1;			/* port serial control 1 */
	ulong	ps1;			/* ether port status 1 */
	ulong	mvhdr;			/* marvell header */
	ulong	_pad8[2];

	/* interrupts */
	ulong	irq;			/* interrupt cause; some rw0c bits */
	ulong	irqe;			/* " " extended; some rw0c bits */
	ulong	irqmask;		/* interrupt mask (actually enable) */
	ulong	irqemask;		/* " " extended */

	ulong	_pad9;
	ulong	pxtfut;			/* port tx fifo urgent threshold */
	ulong	_pad10;
	ulong	pxmfs;			/* port rx minimum frame size */
	ulong	_pad11;

	/*
	 * # of input frames discarded by addr filtering or lack of resources;
	 * zeroed upon read.
	 */
	ulong	pxdfc;			/* port rx discard frame counter */
	ulong	pxofc;			/* port overrun frame counter */
	ulong	_pad12[2];
	ulong	piae;			/* port internal address error */
	uchar	_pad13[0x4bc-0x498];
	ulong	etherprio;		/* ether type priority */
	uchar	_pad14[0x4dc-0x4c0];
	ulong	tqfpc;			/* tx queue fixed priority config. */
	ulong	pttbrc;			/* port tx token-bucket rate config. */
	ulong	tqc1;			/* tx queue command 1 */
	ulong	pmtu;			/* port maximum transmit unit */
	ulong	pmtbs;			/* port maximum token bucket size */
	uchar	_pad15[0x600-0x4f0];

	struct {
		ulong	_pad[3];
		ulong	r;		/* phys. addr.: cur. rx desc. ptrs */
	} crdp[8];
	ulong	rqc;			/* rx queue command */
	ulong	tcsdp;			/* phys. addr.: cur. tx desc. ptr */
	uchar	_pad16[0x6c0-0x688];

	ulong	tcqdp[8];		/* phys. addr.: cur. tx q. desc. ptr */
	uchar	_pad17[0x700-0x6e0];

	struct {
		ulong	tbctr;		/* queue tx token-bucket counter */
		ulong	tbcfg;		/* tx queue token-bucket config. */
		ulong	acfg;		/* tx queue arbiter config. */
		ulong	_pad;
	} tq[8];
	ulong	pttbc;			/* port tx token-bucket counter */
	uchar	_pad18[0x7a8-0x784];

	ulong	ipg2;			/* tx queue ipg */
	ulong	_pad19[3];
	ulong	ipg3;
	ulong	_pad20;
	ulong	htlp;			/* high token in low packet */
	ulong	htap;			/* high token in async packet */
	ulong	ltap;			/* low token in async packet */
	ulong	_pad21;
	ulong	ts;			/* tx speed */
	uchar	_pad22[0x1000-0x7d4];

	/* mac mib counters: statistics */
	Mibstats;
	uchar	_pad23[0x1400-0x1080];

	/* multicast filtering; each byte: Qno<<1 | Pass */
	ulong	dfsmt[64];	/* dest addr filter special m'cast table */
	ulong	dfomt[64];	/* dest addr filter other m'cast table */
	/* unicast filtering */
	ulong	dfut[4];		/* dest addr filter unicast table */
};

static void getmibstats(Ctlr *);

static void
rxfreeb(Block *b)
{
	/* freeb(b) will have previously decremented b->ref to 0; raise to 1 */
	_xinc(&b->ref);
//iprint("fr %ld ", b->ref);
	b->wp = b->rp =
		(uchar*)((uintptr)(b->lim - Rxblklen) & ~(Bufalign - 1));
	assert(((uintptr)b->rp & (Bufalign - 1)) == 0);
	b->free = rxfreeb;

	ilock(&freeblocks);
	b->next = freeblocks.head;
	freeblocks.head = b;
	iunlock(&freeblocks);
}

static Block *
rxallocb(void)
{
	Block *b;

	ilock(&freeblocks);
	b = freeblocks.head;
	if(b != nil) {
		freeblocks.head = b->next;
		b->next = nil;
		b->free = rxfreeb;
	}
	iunlock(&freeblocks);
	return b;
}

static void
rxkick(Ctlr *ctlr)
{
	Gbereg *reg = ctlr->reg;

	if (reg->crdp[Qno].r == 0)
		reg->crdp[Qno].r = PADDR(&ctlr->rx[ctlr->rxhead]);
	if ((reg->rqc & 0xff) == 0)		/* all queues are stopped? */
		reg->rqc = Rxqon(Qno);		/* restart */
}

static void
txkick(Ctlr *ctlr)
{
	Gbereg *reg = ctlr->reg;

	if (reg->tcqdp[Qno] == 0)
		reg->tcqdp[Qno] = PADDR(&ctlr->tx[ctlr->txhead]);
	if ((reg->tqc & 0xff) == 0)		/* all q's stopped? */
		reg->tqc = Txqon(Qno);		/* restart */
}

static void
rxreplenish(Ctlr *ctlr)
{
	Rx *r;
	Block *b;

	while(ctlr->rxb[ctlr->rxtail] == nil) {
		b = rxallocb();
		if(b == nil) {
			iprint("etherkw: rxreplenish out of buffers\n");
			break;
		}

		ctlr->rxb[ctlr->rxtail] = b;

		/* set up receive descriptor */
		r = &ctlr->rx[ctlr->rxtail];
		assert(((uintptr)r & (Descralign - 1)) == 0);
		r->countsize = Bufsize(Rxblklen);
		r->buf = PADDR(b->rp);
		cachedwbse(r, sizeof *r);

		/* and fire */
		r->cs = RCSdmaown | RCSenableintr;
		cachedwbse(&r->cs, BY2SE);

		ctlr->rxtail = NEXT(ctlr->rxtail, Nrx);
	}
	rxkick(ctlr);
}

static void
dump(uchar *bp, long max)
{
	if (max > 64)
		max = 64;
	for (; max > 0; max--, bp++)
		iprint("%02.2ux ", *bp);
	print("...\n");
}

static void
etheractive(Ether *ether)
{
	ether->starttime = TK2MS(MACHP(0)->ticks)/1000;
}

static void
ethercheck(Ether *ether)
{
	if (ether->starttime != 0 &&
	    TK2MS(MACHP(0)->ticks)/1000 - ether->starttime > Etherstuck)
		iprint("ethernet stuck\n");
}

static void
receive(Ether *ether)
{
	int i;
	ulong n;
	Block *b;
	Ctlr *ctlr = ether->ctlr;
	Rx *r;

	ethercheck(ether);
	for (i = Nrx-2; i > 0; i--) {
		r = &ctlr->rx[ctlr->rxhead];
		assert(((uintptr)r & (Descralign - 1)) == 0);
		cachedinvse(r, sizeof *r);
		if(r->cs & RCSdmaown)
			break;

		b = ctlr->rxb[ctlr->rxhead];
		if (b == nil)
			panic("etherkw: nil ctlr->rxb[ctlr->rxhead] "
				"in receive");
		ctlr->rxb[ctlr->rxhead] = nil;
		ctlr->rxhead = NEXT(ctlr->rxhead, Nrx);

		if((r->cs & (RCSfirst|RCSlast)) != (RCSfirst|RCSlast)) {
			ctlr->nofirstlast++;
			freeb(b);
			continue;
		}
		if(r->cs & RCSmacerr) {
			freeb(b);
			continue;
		}

		n = r->countsize >> 16;
		assert(n >= 2 && n < 2048);

		cachedinvse(b->rp, n);
		b->wp = b->rp + n;
		/*
		 * skip hardware padding to align ipv4 address in memory
		 * (mv-s104860-u0 §8.3.4.1)
		 */
		b->rp += 2;
		etheriq(ether, b, 1);
		etheractive(ether);
		if (i % (Nrx / 2) == 0)
			rxreplenish(ctlr);
	}
	rxreplenish(ctlr);
}

static void
txreplenish(Ether *ether)			/* free transmitted packets */
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	while(ctlr->txtail != ctlr->txhead) {
		cachedinvse(&ctlr->tx[ctlr->txtail].cs, BY2SE);
		if(ctlr->tx[ctlr->txtail].cs & TCSdmaown)
			break;
		if(ctlr->txb[ctlr->txtail] == nil)
			panic("no block for sent packet?!");
		freeb(ctlr->txb[ctlr->txtail]);
		ctlr->txb[ctlr->txtail] = nil;
		ctlr->txtail = NEXT(ctlr->txtail, Ntx);
		etheractive(ether);
	}
}

/*
 * transmit strategy: fill the output ring as far as possible,
 * perhaps leaving a few spare; kick off the output and take
 * an interrupt only when the transmit queue is empty.
 */
static void
transmit(Ether *ether)
{
	int i, kick, len;
	Block *b;
	Ctlr *ctlr = ether->ctlr;
	Gbereg *reg = ctlr->reg;
	Tx *t;

	ethercheck(ether);
	ilock(ctlr);
	txreplenish(ether);			/* reap old packets */

	/* queue new packets; don't use more than half the tx descs. */
	kick = 0;
	for (i = Ntx/2 - 2; i > 0; i--) {
		t = &ctlr->tx[ctlr->txhead];
		assert(((uintptr)t & (Descralign - 1)) == 0);
		cachedinvse(t, sizeof *t);
		if(t->cs & TCSdmaown) {		/* free descriptor? */
			ctlr->txringfull++;
			break;
		}

		b = qget(ether->oq);		/* outgoing packet? */
		if (b == nil)
			break;
		len = BLEN(b);
		if(len < ether->minmtu || len > ether->maxmtu) {
			freeb(b);
			continue;
		}
		ctlr->txb[ctlr->txhead] = b;

		/* set up the transmit descriptor */
		t->buf = PADDR(b->rp);
		t->countchk = len << 16;
		cachedwbse(t, sizeof *t);

		/* and fire */
		t->cs = TCSpadding | TCSfirst | TCSlast | TCSdmaown |
			TCSenableintr;
		cachedwbse(&t->cs, BY2SE);

		kick++;
		ctlr->txhead = NEXT(ctlr->txhead, Ntx);
	}
	if (kick) {
		txkick(ctlr);

		reg->irqmask  |= Itxendq(Qno);
		reg->irqemask |= IEtxerrq(Qno) | IEtxunderrun;
	}
	iunlock(ctlr);
}

static void
dumprxdescs(Ctlr *ctlr)
{
	int i;
	Gbereg *reg = ctlr->reg;

	iprint("\nrxhead %d rxtail %d; txcdp %#p rxcdp %#p\n",
		ctlr->rxhead, ctlr->rxtail, reg->tcqdp[Qno], reg->crdp[Qno].r);
	for (i = 0; i < Nrx; i++)
		iprint("rxb %d @ %#p: %#p\n", i, &ctlr->rxb[i], ctlr->rxb[i]);
	for (i = 0; i < Nrx; i++)
		iprint("rx %d @ %#p: cs %#lux countsize %lud buf %#lux next %#lux\n",
			i, &ctlr->rx[i], ctlr->rx[i].cs,
			ctlr->rx[i].countsize >> 3, ctlr->rx[i].buf,
			ctlr->rx[i].next);
	delay(1000);
}

static int
gotinput(void* ctlr)
{
	return ((Ctlr*)ctlr)->haveinput != 0;
}

/*
 * process any packets in the input ring.
 * also sum mib stats frequently to avoid the overflow
 * mentioned in the errata.
 */
static void
rcvproc(void* arg)
{
	Ctlr *ctlr;
	Ether *ether;

	ether = arg;
	ctlr = ether->ctlr;
	for(;;){
		tsleep(&ctlr->rrendez, gotinput, ctlr, 10*1000);
		ilock(ctlr);
		getmibstats(ctlr);
		if (ctlr->haveinput) {
			ctlr->haveinput = 0;
			iunlock(ctlr);
			receive(ether);
		} else
			iunlock(ctlr);
	}
}

static void
interrupt(Ureg*, void *arg)
{
	ulong irq, irqe, handled;
	Ether *ether = arg;
	Ctlr *ctlr = ether->ctlr;
	Gbereg *reg = ctlr->reg;

	handled = 0;
	irq = reg->irq;
	irqe = reg->irqe;
	reg->irq = 0;				/* extinguish intr causes */
	reg->irqe = 0;				/* " " " */
	ethercheck(ether);

	if(irq & Irxbufferq(Qno)) {
		/*
		 * letting a kproc process the input takes far less real time
		 * than doing it all at interrupt level.
		 */
		ctlr->haveinput = 1;
		wakeup(&ctlr->rrendez);
		handled++;
	} else
		rxkick(ctlr);

	if(irq & Itxendq(Qno)) {		/* transmit ring empty? */
		reg->irqmask  &= ~Itxendq(Qno);	/* prevent more interrupts */
		reg->irqemask &= ~(IEtxerrq(Qno) | IEtxunderrun);
		transmit(ether);
		handled++;
	}

	if(irqe & IEsum) {
		/*
		 * IElinkchg appears to only be set when unplugging.
		 * autonegotiation is likely not done yet, so linkup not valid,
		 * thus we note the link change here, and check for
		 * that and autonegotiation done below.
		 */
		if(irqe & IEphystschg) {
			ether->link = (reg->ps0 & PS0linkup) != 0;
			ether->linkchg = 1;
		}
		if(irqe & IEtxerrq(Qno))
			ether->oerrs++;
		if(irqe & IErxoverrun)
			ether->overflows++;
		if(irqe & IEtxunderrun)
			ctlr->txunderrun++;
		if(irqe & (IEphystschg | IEtxerrq(Qno) | IErxoverrun |
		    IEtxunderrun))
			handled++;
	}
	if (irq & Isum) {
		if (irq & Irxerrq(Qno)) {
			ether->buffs++;		/* approx. error */
			/* null descriptor pointer or descriptor owned by cpu */
			panic("etherkw: rx err on queue 0");
		}
		if (irq & Irxerr) {
			ether->buffs++;		/* approx. error */
			/* null descriptor pointer or descriptor owned by cpu */
			panic("etherkw: rx err");
		}
		if(irq & (Irxerr | Irxerrq(Qno)))
			handled++;
	}

	if(ether->linkchg && (reg->ps1 & PS1an_done)) {
		handled++;
		ether->link = (reg->ps0 & PS0linkup) != 0;
		ether->linkchg = 0;
	}
	ctlr->newintrs++;

	if (!handled) {
		irq  &= ~Isum;
		irqe &= ~IEtxbufferq(Qno);
		if (irq == 0 && irqe == 0) {
			/* seems to be triggered by continuous output */
			// iprint("etherkw: spurious interrupt\n");
		} else
			iprint("etherkw: interrupt cause unknown; "
				"irq %#lux irqe %#lux\n", irq, irqe);
	}
	intrclear(Irqlo, ether->irq);
}

void
promiscuous(void *arg, int on)
{
	Ether *ether = arg;
	Ctlr *ctlr = ether->ctlr;
	Gbereg *reg = ctlr->reg;

	ilock(ctlr);
	ether->prom = on;
	if(on)
		reg->portcfg |= PCFGupromisc;
	else
		reg->portcfg &= ~PCFGupromisc;
	iunlock(ctlr);
}

void
multicast(void *, uchar *, int)
{
	/* nothing to do; we always accept multicast */
}

static void quiesce(Gbereg *reg);

static void
shutdown(Ether *ether)
{
	Ctlr *ctlr = ether->ctlr;
	Gbereg *reg = ctlr->reg;

	ilock(ctlr);
	quiesce(reg);
	reg->tcqdp[Qno]  = 0;
	reg->crdp[Qno].r = 0;
	reg->psc0 = 0;			/* no PSC0porton */
	reg->psc1 |= PSC1portreset;
	iunlock(ctlr);
	coherence();
	delay(100);
}

enum {
	CMjumbo,
};

static Cmdtab ctlmsg[] = {
	CMjumbo,	"jumbo",	2,
};

long
ctl(Ether *e, void *p, long n)
{
	Cmdbuf *cb;
	Cmdtab *ct;
	Ctlr *ctlr = e->ctlr;
	Gbereg *reg = ctlr->reg;

	cb = parsecmd(p, n);
	if(waserror()) {
		free(cb);
		nexterror();
	}

	ct = lookupcmd(cb, ctlmsg, nelem(ctlmsg));
	switch(ct->index) {
	case CMjumbo:
		if(strcmp(cb->f[1], "on") == 0) {
			/* incoming packet queue doesn't expect jumbo frames */
			error("jumbo disabled");
			reg->psc0 = (reg->psc0 & ~PSC0mrumask) |
				PSC0mru(PSC0mru9022);
			e->maxmtu = 9022;
		} else if(strcmp(cb->f[1], "off") == 0) {
			reg->psc0 = (reg->psc0 & ~PSC0mrumask) |
				PSC0mru(PSC0mru1522);
			e->maxmtu = ETHERMAXTU;
		} else
			error(Ebadctl);
		break;
	default:
		error(Ebadctl);
		break;
	}
	free(cb);
	poperror();
	return n;
}

/*
 * phy/mii goo
 */

static int
smibusywait(Gbereg *reg, ulong waitbit)
{
	ulong timeout, smi_reg;

	timeout = PhysmiTimeout;
	/* wait till the SMI is not busy */
	do {
		/* read smi register */
		smi_reg = reg->smi;
		if (timeout-- == 0) {
			MIIDBG("SMI busy timeout\n");
			return -1;
		}
//		delay(1);
	} while (smi_reg & waitbit);
	return 0;
}

static int
miird(Mii *mii, int pa, int ra)
{
	ulong smi_reg, timeout;
	Ctlr *ctlr;
	Gbereg *reg;

	ctlr = (Ctlr*)mii->ctlr;
	reg = ctlr->reg;

	/* check params */
	if ((pa<<Physmiaddroff) & ~Physmiaddrmask ||
	    (ra<<SmiRegaddroff) & ~SmiRegaddrmask)
		return -1;

	smibusywait(reg, PhysmiBusy);

	/* fill the phy address and register offset and read opcode */
	reg->smi = pa << Physmiaddroff | ra << SmiRegaddroff | PhysmiopRd;
	coherence();

	/* wait til read value is ready */
//	if (smibusywait(reg, PhysmiReadok) < 0)
//		return -1;
	timeout = PhysmiTimeout;
	do {
		smi_reg = reg->smi;
		if (timeout-- == 0) {
			MIIDBG("SMI read-valid timeout\n");
			return -1;
		}
	} while (!(smi_reg & PhysmiReadok));

	/* Wait for the data to update in the SMI register */
	for (timeout = 0; timeout < PhysmiTimeout; timeout++)
		;
	return reg->smi & Physmidatamask;
}

static int
miiwr(Mii *mii, int pa, int ra, int v)
{
	Ctlr *ctlr;
	Gbereg *reg;
	ulong smi_reg;

	ctlr = (Ctlr*)mii->ctlr;
	reg = ctlr->reg;

	/* check params */
	if (((pa<<Physmiaddroff) & ~Physmiaddrmask) ||
	    ((ra<<SmiRegaddroff) & ~SmiRegaddrmask))
		return -1;

	smibusywait(reg, PhysmiBusy);

	/* fill the phy address and register offset and read opcode */
	smi_reg = v << Physmidataoff | pa << Physmiaddroff | ra << SmiRegaddroff;
	reg->smi = smi_reg & ~PhysmiopRd;
	coherence();
	return 0;
}

static int
kirkwoodmii(Ether *ether)
{
	int i;
	Ctlr *ctlr;
	MiiPhy *phy;

	MIIDBG("mii\n");
	ctlr = ether->ctlr;
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->ctlr = ctlr;
	ctlr->mii->mir = miird;
	ctlr->mii->miw = miiwr;

	if(mii(ctlr->mii, ~0) == 0 || (phy = ctlr->mii->curphy) == nil){
		print("#l%d: etherkw: init mii failure\n", ether->ctlrno);
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}

	/* oui 005043 is marvell */
	MIIDBG("oui %#X phyno %d\n", phy->oui, phy->phyno);
	if(miistatus(ctlr->mii) < 0){
		miireset(ctlr->mii);
		MIIDBG("miireset\n");
		if(miiane(ctlr->mii, ~0, 0, ~0) < 0){
			iprint("miiane failed\n");
			return -1;
		}
		MIIDBG("miistatus\n");
		miistatus(ctlr->mii);
		if(miird(ctlr->mii, phy->phyno, Bmsr) & BmsrLs){
			for(i = 0; ; i++){
				if(i > 600){
					iprint("etherkw: autonegotiation failed\n");
					break;
				}
				if(miird(ctlr->mii, phy->phyno, Bmsr) & BmsrAnc)
					break;
				delay(10);
			}
			if(miistatus(ctlr->mii) < 0)
				iprint("miistatus failed\n");
		}else{
			iprint("etherkw: no link\n");
			phy->speed = 10;	/* simple default */
		}
	}

	ether->mbps = phy->speed;
//	iprint("#l%d: kirkwoodmii: fd %d speed %d tfc %d rfc %d\n",
//		ctlr->port, phy->fd, phy->speed, phy->tfc, phy->rfc);
	MIIDBG("mii done\n");
	return 0;
}

enum {						/* PHY register pages */
	Pagcopper,
	Pagfiber,
	Pagrgmii,
	Pagled,
	Pagrsvd1,
	Pagvct,
	Pagtest,
	Pagrsvd2,
	Pagfactest,
};

static void
miiregpage(Mii *mii, ulong dev, ulong page)
{
	miiwr(mii, dev, Eadr, page);
}

static int
miiphyinit(Mii *mii)
{
	ulong dev;
	Ctlr *ctlr;
	Gbereg *reg;

	ctlr = (Ctlr*)mii->ctlr;
	reg = ctlr->reg;
	dev = reg->phy;
	MIIDBG("phy dev addr %lux\n", dev);

	/* leds link & activity */
	miiregpage(mii, dev, Pagled);
	/* low 4 bits == 1: on - link, blink - activity, off - no link */
	miiwr(mii, dev, Scr, (miird(mii, dev, Scr) & ~0xf) | 1);

	miiregpage(mii, dev, Pagrgmii);
	miiwr(mii, dev, Scr, miird(mii, dev, Scr) | Rgmiipwrup);
	/* must now do a software reset, sez the manual */

	/* enable RGMII delay on Tx and Rx for CPU port */
	miiwr(mii, dev, Recr, miird(mii, dev, Recr) | Rxtiming | Rxtiming);

	miiregpage(mii, dev, Pagcopper);
	miiwr(mii, dev, Scr,
		(miird(mii, dev, Scr) & ~(Pwrdown|Endetect)) | Mdix);

	return 0;
}

/*
 * initialisation
 */

static void
quiesce(Gbereg *reg)
{
	ulong v;

	v = reg->tqc;
	if (v & 0xFF)
		reg->tqc = v << 8;		/* stop active channels */
	v = reg->rqc;
	if (v & 0xFF)
		reg->rqc = v << 8;		/* stop active channels */
	/* wait for all queues to stop */
	while (reg->tqc & 0xFF || reg->rqc & 0xFF)
		;
}

static void
portreset(Gbereg *reg)
{
	ulong i;

	quiesce(reg);
	reg->psc0 &= ~PSC0porton;		/* disable port */
	reg->psc1 &= ~(PSC1rgmii|PSC1portreset); /* set port & MII active */
	coherence();
	for (i = 0; i < 4000; i++)		/* magic delay */
		;
}

static void
p16(uchar *p, ulong v)
{
	*p++ = v>>8;
	*p   = v;
}

static void
p32(uchar *p, ulong v)
{
	*p++ = v>>24;
	*p++ = v>>16;
	*p++ = v>>8;
	*p   = v;
}

/*
 * set ether->ea from hw mac address,
 * configure unicast filtering to accept it.
 */
void
archetheraddr(Ether *ether, Gbereg *reg, int rxqno)
{
	ulong nibble, ucreg, tbloff, regoff;

	p32(ether->ea,   reg->macah);
	p16(ether->ea+4, reg->macal);

	/* accept frames on ea */
	nibble = ether->ea[5] & 0xf;
	tbloff = nibble / 4;
	regoff = nibble % 4;

	regoff *= 8;
	ucreg = reg->dfut[tbloff];
	ucreg &= 0xff << regoff;
	ucreg |= (rxqno << 1 | Pass) << regoff;
	reg->dfut[tbloff] = ucreg;

	/* accept all multicast too.  set up special & other tables. */
	memset(reg->dfsmt, Qno<<1 | Pass, sizeof reg->dfsmt);
	memset(reg->dfomt, Qno<<1 | Pass, sizeof reg->dfomt);
	coherence();
}

static void
ctlrinit(Ether *ether)
{
	int i;
	Block *b;
	Ctlr *ctlr = ether->ctlr;
	Gbereg *reg = ctlr->reg;
	Rx *r;
	Tx *t;
	static char name[KNAMELEN];
	static Ctlr fakectlr;		/* bigger than 4K; keep off the stack */

	ilock(&freeblocks);
	for(i = 0; i < Nrxblks; i++) {
		b = iallocb(Rxblklen+Bufalign-1);
		if(b == nil) {
			iprint("etherkw: no memory for rx buffers\n");
			break;
		}
		assert(b->ref == 1);
		b->wp = b->rp = (uchar*)
			((uintptr)(b->lim - Rxblklen) & ~(Bufalign - 1));
		assert(((uintptr)b->rp & (Bufalign - 1)) == 0);
		b->free = rxfreeb;
		b->next = freeblocks.head;
		freeblocks.head = b;
	}
	iunlock(&freeblocks);

	ctlr->rx = xspanalloc(Nrx * sizeof(Rx), Descralign, 0);
	if(ctlr->rx == nil)
		panic("etherkw: no memory for rx ring");
	for(i = 0; i < Nrx; i++) {
		r = &ctlr->rx[i];
		assert(((uintptr)r & (Descralign - 1)) == 0);
		r->cs = 0;	/* not owned by hardware until r->buf is set */
		r->buf = 0;
		r->next = PADDR(&ctlr->rx[NEXT(i, Nrx)]);
		ctlr->rxb[i] = nil;
	}
	ctlr->rxtail = ctlr->rxhead = 0;
	cachedwb();
	rxreplenish(ctlr);

	ctlr->tx = xspanalloc(Ntx * sizeof(Tx), Descralign, 0);
	if(ctlr->tx == nil)
		panic("etherkw: no memory for tx ring");
	for(i = 0; i < Ntx; i++) {
		t = &ctlr->tx[i];
		assert(((uintptr)t & (Descralign - 1)) == 0);
		t->cs = 0;
		t->buf = 0;
		t->next = PADDR(&ctlr->tx[NEXT(i, Ntx)]);
		ctlr->txb[i] = nil;
	}
	ctlr->txtail = ctlr->txhead = 0;
	cachedwb();

	/* clear stats by reading them into fake ctlr */
	getmibstats(&fakectlr);

	reg->pxmfs = MFS64by;

	/*
	 * ipg's (inter packet gaps) for interrupt coalescing,
	 * values in units of 64 clock cycles.  A full-sized
	 * packet (1514 bytes) takes just over 12µs to transmit.
	 */
	if (CLOCKFREQ/(Maxrxintrsec*64) >= (1<<16))
		panic("rx coalescing value %d too big for short",
			CLOCKFREQ/(Maxrxintrsec*64));
	reg->sdc = SDCrifb | SDCrxburst(Burst16) | SDCtxburst(Burst16) |
		SDCrxnobyteswap | SDCtxnobyteswap |
		SDCipgintrx(CLOCKFREQ/(Maxrxintrsec*64));
	reg->pxtfut = 0;	/* TFUTipginttx(CLOCKFREQ/(Maxrxintrsec*64)) */

	/* allow just these interrupts */
	reg->irqmask = Irxbufferq(Qno) | Irxerr | Itxendq(Qno);
	reg->irqemask = IEtxerrq(Qno) | IEphystschg | IErxoverrun | IEtxunderrun;

	reg->irq = 0;
	reg->irqe = 0;
	reg->euirqmask = 0;
	reg->euirq = 0;

//	archetheraddr(ether, ctlr->reg, Qno);	/* 2nd location */

	reg->tcqdp[Qno]  = PADDR(&ctlr->tx[ctlr->txhead]);
	for (i = 1; i < nelem(reg->tcqdp); i++)
		reg->tcqdp[i] = 0;
	reg->crdp[Qno].r = PADDR(&ctlr->rx[ctlr->rxhead]);
	for (i = 1; i < nelem(reg->crdp); i++)
		reg->crdp[i].r = 0;

	reg->portcfg = Rxqdefault(Qno) | Rxqarp(Qno);
	reg->portcfgx = 0;

	reg->psc1 = PSC1rgmii | PSC1encolonbp | PSC1coldomlim(0x23);
	reg->psc0 = PSC0porton | PSC0an_flctloff |
		PSC0an_pauseadv | PSC0nofrclinkdown | PSC0mru(PSC0mru1522);

	ether->link = (reg->ps0 & PS0linkup) != 0;

	/* set ethernet MTU for leaky bucket mechanism to 0 (disabled) */
	reg->pmtu = 0;

	reg->rqc = Rxqon(Qno);
	coherence();
	etheractive(ether);

	snprint(name, sizeof name, "#l%drproc", ether->ctlrno);
	kproc(name, rcvproc, ether);
}

static void
attach(Ether* ether)
{
	Ctlr *ctlr = ether->ctlr;

	lock(&ctlr->initlock);
	if(ctlr->init == 0) {
		ctlrinit(ether);
		ctlr->init = 1;
	}
	unlock(&ctlr->initlock);
}

/*
 * statistics goo.
 * mib registers clear on read.
 */

static void
getmibstats(Ctlr *ctlr)
{
	Gbereg *reg = ctlr->reg;

	/*
	 * Marvell 88f6281 errata FE-ETH-120: high long of rxby and txby
	 * can't be read correctly, so read the low long frequently
	 * (every 30 seconds or less), thus avoiding overflow into high long.
	 */
	ctlr->rxby	+= reg->rxbylo;
	ctlr->txby	+= reg->txbylo;

	ctlr->badrxby	+= reg->badrxby;
	ctlr->mactxerr	+= reg->mactxerr;
	ctlr->rxpkt	+= reg->rxpkt;
	ctlr->badrxpkt	+= reg->badrxpkt;
	ctlr->rxbcastpkt+= reg->rxbcastpkt;
	ctlr->rxmcastpkt+= reg->rxmcastpkt;
	ctlr->rx64	+= reg->rx64;
	ctlr->rx65_127	+= reg->rx65_127;
	ctlr->rx128_255	+= reg->rx128_255;
	ctlr->rx256_511	+= reg->rx256_511;
	ctlr->rx512_1023+= reg->rx512_1023;
	ctlr->rx1024_max+= reg->rx1024_max;
	ctlr->txpkt	+= reg->txpkt;
	ctlr->txcollpktdrop+= reg->txcollpktdrop;
	ctlr->txmcastpkt+= reg->txmcastpkt;
	ctlr->txbcastpkt+= reg->txbcastpkt;
	ctlr->badmacctlpkts+= reg->badmacctlpkts;
	ctlr->txflctl	+= reg->txflctl;
	ctlr->rxflctl	+= reg->rxflctl;
	ctlr->badrxflctl+= reg->badrxflctl;
	ctlr->rxundersized+= reg->rxundersized;
	ctlr->rxfrags	+= reg->rxfrags;
	ctlr->rxtoobig	+= reg->rxtoobig;
	ctlr->rxjabber	+= reg->rxjabber;
	ctlr->rxerr	+= reg->rxerr;
	ctlr->crcerr	+= reg->crcerr;
	ctlr->collisions+= reg->collisions;
	ctlr->latecoll	+= reg->latecoll;
}

long
ifstat(Ether *ether, void *a, long n, ulong off)
{
	Ctlr *ctlr = ether->ctlr;
	Gbereg *reg = ctlr->reg;
	char *buf, *p, *e;

	buf = p = malloc(READSTR);
	e = p + READSTR;

	ilock(ctlr);
	getmibstats(ctlr);

	ctlr->intrs += ctlr->newintrs;
	p = seprint(p, e, "interrupts: %lud\n", ctlr->intrs);
	p = seprint(p, e, "new interrupts: %lud\n", ctlr->newintrs);
	ctlr->newintrs = 0;
	p = seprint(p, e, "tx underrun: %lud\n", ctlr->txunderrun);
	p = seprint(p, e, "tx ring full: %lud\n", ctlr->txringfull);

	ctlr->rxdiscard += reg->pxdfc;
	ctlr->rxoverrun += reg->pxofc;
	p = seprint(p, e, "rx discarded frames: %lud\n", ctlr->rxdiscard);
	p = seprint(p, e, "rx overrun frames: %lud\n", ctlr->rxoverrun);
	p = seprint(p, e, "no first+last flag: %lud\n", ctlr->nofirstlast);

	p = seprint(p, e, "duplex: %s\n", (reg->ps0 & PS0fd)? "full": "half");
	p = seprint(p, e, "flow control: %s\n", (reg->ps0 & PS0flctl)? "on": "off");
	/* p = seprint(p, e, "speed: %d mbps\n", ); */

	p = seprint(p, e, "received bytes: %llud\n", ctlr->rxby);
	p = seprint(p, e, "bad received bytes: %lud\n", ctlr->badrxby);
	p = seprint(p, e, "internal mac transmit errors: %lud\n", ctlr->mactxerr);
	p = seprint(p, e, "total received frames: %lud\n", ctlr->rxpkt);
	p = seprint(p, e, "received broadcast frames: %lud\n", ctlr->rxbcastpkt);
	p = seprint(p, e, "received multicast frames: %lud\n", ctlr->rxmcastpkt);
	p = seprint(p, e, "bad received frames: %lud\n", ctlr->badrxpkt);
	p = seprint(p, e, "received frames 0-64: %lud\n", ctlr->rx64);
	p = seprint(p, e, "received frames 65-127: %lud\n", ctlr->rx65_127);
	p = seprint(p, e, "received frames 128-255: %lud\n", ctlr->rx128_255);
	p = seprint(p, e, "received frames 256-511: %lud\n", ctlr->rx256_511);
	p = seprint(p, e, "received frames 512-1023: %lud\n", ctlr->rx512_1023);
	p = seprint(p, e, "received frames 1024-max: %lud\n", ctlr->rx1024_max);
	p = seprint(p, e, "transmitted bytes: %llud\n", ctlr->txby);
	p = seprint(p, e, "total transmitted frames: %lud\n", ctlr->txpkt);
	p = seprint(p, e, "transmitted broadcast frames: %lud\n", ctlr->txbcastpkt);
	p = seprint(p, e, "transmitted multicast frames: %lud\n", ctlr->txmcastpkt);
	p = seprint(p, e, "transmit frames dropped by collision: %lud\n", ctlr->txcollpktdrop);
	p = seprint(p, e, "misaligned buffers: %lud\n", ether->pktsmisaligned);

	p = seprint(p, e, "bad mac control frames: %lud\n", ctlr->badmacctlpkts);
	p = seprint(p, e, "transmitted flow control messages: %lud\n", ctlr->txflctl);
	p = seprint(p, e, "received flow control messages: %lud\n", ctlr->rxflctl);
	p = seprint(p, e, "bad received flow control messages: %lud\n", ctlr->badrxflctl);
	p = seprint(p, e, "received undersized packets: %lud\n", ctlr->rxundersized);
	p = seprint(p, e, "received fragments: %lud\n", ctlr->rxfrags);
	p = seprint(p, e, "received oversized packets: %lud\n", ctlr->rxtoobig);
	p = seprint(p, e, "received jabber packets: %lud\n", ctlr->rxjabber);
	p = seprint(p, e, "mac receive errors: %lud\n", ctlr->rxerr);
	p = seprint(p, e, "crc errors: %lud\n", ctlr->crcerr);
	p = seprint(p, e, "collisions: %lud\n", ctlr->collisions);
	p = seprint(p, e, "late collisions: %lud\n", ctlr->latecoll);
	USED(p);
	iunlock(ctlr);

	n = readstr(off, a, n, buf);
	free(buf);
	return n;
}


static int
reset(Ether *ether)
{
	Ctlr *ctlr;
	static uchar zeroea[Eaddrlen];

	ether->ctlr = ctlr = malloc(sizeof *ctlr);
	switch(ether->ctlrno) {
	case 0:
		ctlr->reg = (Gbereg*)Gbe0regs;
		ether->irq = IRQ0gbe0sum;
		break;
	case 1:
		ctlr->reg = (Gbereg*)Gbe1regs;
		ether->irq = IRQ0gbe1sum;
		break;
	default:
		panic("etherkw: bad ether ctlr #%d", ether->ctlrno);
	}

//	*(ulong *)AddrIocfg0 |= 1 << 7 | 1 << 15;	/* io cfg 0: 1.8v gbe */
//	coherence();

	portreset(ctlr->reg);
	/* ensure that both interfaces are set to RGMII before calling mii */
	((Gbereg*)Gbe0regs)->psc1 |= PSC1rgmii;
	((Gbereg*)Gbe1regs)->psc1 |= PSC1rgmii;

	/* Set phy address of the port */
	ctlr->port = ether->ctlrno;
	ctlr->reg->phy = ether->ctlrno;
	coherence();
	ether->port = (uintptr)ctlr->reg;

	if(kirkwoodmii(ether) < 0){
		free(ctlr);
		ether->ctlr = nil;
		return -1;
	}
	miiphyinit(ctlr->mii);
	archetheraddr(ether, ctlr->reg, Qno);	/* original location */
	if (memcmp(ether->ea, zeroea, sizeof zeroea) == 0){
		free(ctlr);
		ether->ctlr = nil;
		return -1;			/* no rj45 for this ether */
	}
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;
	ether->shutdown = shutdown;
	ether->ctl = ctl;

	ether->arg = ether;
	ether->promiscuous = promiscuous;
	ether->multicast = multicast;

	return 0;
}

void
etherkwlink(void)
{
	addethercard("kirkwood", reset);
}
