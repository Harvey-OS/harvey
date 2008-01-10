/*
	Via Rhine driver, written for VT6102.
	Uses the ethermii to control PHY.

	Currently always copies on both, tx and rx.
	rx side could be copy-free, and tx-side might be made
	(almost) copy-free by using (possibly) two descriptors (if it allows
	arbitrary tx lengths, which it should..): first for alignment and
	second for rest of the frame. Rx-part should be worth doing.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

typedef struct QLock { int r; } QLock;
#define qlock(i)	while(0)
#define qunlock(i)	while(0)
#define iprint		print

#include "etherif.h"
#include "ethermii.h"

enum {
	Ntxd = 4,
	Nrxd = 4,
	Nwait = 50,
	BIGSTR = 8192,
};

typedef struct Desc Desc;
typedef struct Ctlr Ctlr;

struct Desc {
	ulong	stat;
	ulong	size;
	ulong	addr;
	ulong	next;
	char	*buf;
	ulong	pad[3];
};

struct Ctlr {
	Pcidev	*pci;
	int	attached;
	int	txused;
	int	txhead;
	int	txtail;
	int	rxtail;
	ulong	port;

	Mii	mii;

	Desc	*txd;		/* wants to be aligned on 16-byte boundary */
	Desc	*rxd;

	QLock	attachlck;
	Lock	tlock;
};

#define ior8(c, r)	(inb((c)->port+(r)))
#define iow8(c, r, b)	(outb((c)->port+(r), (int)(b)))
#define ior16(c, r)	(ins((c)->port+(r)))
#define ior32(c, r)	(inl((c)->port+(r)))
#define iow16(c, r, w)	(outs((c)->port+(r), (ushort)(w)))
#define iow32(c, r, l)	(outl((c)->port+(r), (ulong)(l)))

/* names used everywhere else */
#define csr8r ior8
#define csr8w iow8
#define csr16r ior16
#define csr16w iow16
#define csr32r ior32
#define csr32w iow32

enum Regs {
	Eaddr		= 0x0,
	Rcr		= 0x6,
	Tcr		= 0x7,
	Cr		= 0x8,
	Isr		= 0xc,
	Imr		= 0xe,
	McastAddr	= 0x10,
	RxdAddr		= 0x18,
	TxdAddr		= 0x1C,
	Bcr0		= 0x6E,		/* Bus Control */
	Bcr1		= 0x6F,
	RhineMiiPhy	= 0x6C,
	RhineMiiSr	= 0x6D,
	RhineMiiCr	= 0x70,
	RhineMiiAddr	= 0x71,
	RhineMiiData	= 0x72,
	Eecsr		= 0x74,
	ConfigB		= 0x79,
	ConfigD		= 0x7B,
	MiscCr		= 0x80,
	HwSticky	= 0x83,
	MiscIsr		= 0x84,
	MiscImr		= 0x86,
	WolCrSet	= 0xA0,
	WolCfgSet	= 0xA1,
	WolCgSet	= 0xA3,
	WolCrClr	= 0xA4,
	PwrCfgClr	= 0xA5,
	WolCgClr	= 0xA7,
};

enum {					/* Rcr */
	Sep		= 0x01,		/* Accept Error Packets */
	Ar		= 0x02,		/* Accept Small Packets */
	Am		= 0x04,		/* Accept Multicast */
	Ab		= 0x08,		/* Accept Broadcast */
	RxBcast		= Ab,
	Prom		= 0x10,		/* Accept Physical Address Packets */
	RxProm		= Prom,
	RrftMASK	= 0xE0,		/* Receive FIFO Threshold */
	RrftSHIFT	= 5,
	Rrft64		= 0<<RrftSHIFT,
	Rrft32		= 1<<RrftSHIFT,
	Rrft128		= 2<<RrftSHIFT,
	Rrft256		= 3<<RrftSHIFT,
	Rrft512		= 4<<RrftSHIFT,
	Rrft768		= 5<<RrftSHIFT,
	Rrft1024	= 6<<RrftSHIFT,
	RrftSAF		= 7<<RrftSHIFT,
};

enum {					/* Tcr */
	Lb0		= 0x02,		/* Loopback Mode */
	Lb1		= 0x04,
	Ofset		= 0x08,		/* Back-off Priority Selection */
	RtsfMASK	= 0xE0,		/* Transmit FIFO Threshold */
	RtsfSHIFT	= 5,
	Rtsf128		= 0<<RtsfSHIFT,
	Rtsf256		= 1<<RtsfSHIFT,
	Rtsf512		= 2<<RtsfSHIFT,
	Rtsf1024	= 3<<RtsfSHIFT,
	RtsfSAF		= 7<<RtsfSHIFT,
};

enum Crbits {
	Init		= 1<<0,
	Start		= 1<<1,
	Stop		= 1<<2,
	RxOn		= 1<<3,
	TxOn		= 1<<4,
	Tdmd		= 1<<5,
	Rdmd		= 1<<6,
	EarlyRx		= 1<<8,
	Reserved0	= 1<<9,
	FullDuplex	= 1<<10,
	NoAutoPoll	= 1<<11,
	Reserved1	= 1<<12,
	Tdmd1		= 1<<13,
	Rdmd1		= 1<<14,
	Reset		= 1<<15,
};

enum Isrbits {
	RxOk		= 1<<0,
	TxOk		= 1<<1,
	RxErr		= 1<<2,
	TxErr		= 1<<3,
	TxBufUdf	= 1<<4,
	RxBufLinkErr	= 1<<5,
	BusErr		= 1<<6,
	CrcOvf		= 1<<7,
	EarlyRxInt	= 1<<8,
	TxFifoUdf	= 1<<9,
	RxFifoOvf	= 1<<10,
	TxPktRace	= 1<<11,
	NoRxbuf		= 1<<12,
	TxCollision	= 1<<13,
	PortCh		= 1<<14,
	GPInt		= 1<<15,
};

enum {					/* Bcr0 */
	DmaMASK		= 0x07,		/* DMA Length */
	DmaSHIFT	= 0,
	Dma32		= 0<<DmaSHIFT,
	Dma64		= 1<<DmaSHIFT,
	Dma128		= 2<<DmaSHIFT,
	Dma256		= 3<<DmaSHIFT,
	Dma512		= 4<<DmaSHIFT,
	Dma1024		= 5<<DmaSHIFT,
	DmaSAF		= 7<<DmaSHIFT,
	CrftMASK	= 0x38,		/* Rx FIFO Threshold */
	CrftSHIFT	= 3,
	Crft64		= 1<<CrftSHIFT,
	Crft128		= 2<<CrftSHIFT,
	Crft256		= 3<<CrftSHIFT,
	Crft512		= 4<<CrftSHIFT,
	Crft1024	= 5<<CrftSHIFT,
	CrftSAF		= 7<<CrftSHIFT,
	Extled		= 0x40,		/* Extra LED Support Control */
	Med2		= 0x80,		/* Medium Select Control */
};

enum {					/* Bcr1 */
	PotMASK		= 0x07,		/* Polling Timer Interval */
	PotSHIFT	= 0,
	CtftMASK	= 0x38,		/* Tx FIFO Threshold */
	CtftSHIFT	= 3,
	Ctft64		= 1<<CtftSHIFT,
	Ctft128		= 2<<CtftSHIFT,
	Ctft256		= 3<<CtftSHIFT,
	Ctft512		= 4<<CtftSHIFT,
	Ctft1024	= 5<<CtftSHIFT,
	CtftSAF		= 7<<CtftSHIFT,
};


enum Eecsrbits {
	EeAutoLoad	= 1<<5,
};

enum Descbits {
	OwnNic		= 1<<31,	/* stat */
	TxAbort		= 1<<8,		/* stat */
	TxError		= 1<<15,	/* stat */
	RxChainbuf	= 1<<10,	/* stat */
	RxChainStart	= 1<<9,		/* stat */
	RxChainEnd	= 1<<8,		/* stat */
	Chainbuf	= 1<<15,	/* size rx & tx*/
	TxDisableCrc	= 1<<16,	/* size */
	TxChainStart	= 1<<21,	/* size */
	TxChainEnd	= 1<<22,	/* size */
	TxInt		= 1<<23,	/* size */
};

enum RhineMiiCrbits {
	Mdc	= 1<<0,
	Mdi	= 1<<1,
	Mdo	= 1<<2,
	Mdout	= 1<<3,
	Mdpm	= 1<<4,
	Wcmd	= 1<<5,
	Rcmd	= 1<<6,
	Mauto	= 1<<7,
};

static void
attach(Ether *edev)
{
	Ctlr *ctlr;
	Desc *txd, *rxd, *td, *rd;
	Mii *mi;
	MiiPhy *phy;
	int i, s;

	ctlr = edev->ctlr;
	qlock(&ctlr->attachlck);
	if (ctlr->attached == 0) {
		txd = ctlr->txd;
		rxd = ctlr->rxd;
		for (i = 0; i < Ntxd; ++i) {
			td = &txd[i];
			td->next = PCIWADDR(&txd[(i+1) % Ntxd]);
			td->buf = xspanalloc(sizeof(Etherpkt)+4, 4, 0);
			td->addr = PCIWADDR(td->buf);
			td->size = 0;
			coherence();
			td->stat = 0;
		}
		for (i = 0; i < Nrxd; ++i) {
			rd = &rxd[i];
			rd->next = PCIWADDR(&rxd[(i+1) % Nrxd]);
			rd->buf = xspanalloc(sizeof(Etherpkt)+4, 4, 0);
			rd->addr = PCIWADDR(rd->buf);
			rd->size = sizeof(Etherpkt)+4;
			coherence();
			rd->stat = OwnNic;
		}

		ctlr->txhead = ctlr->txtail = ctlr->rxtail = 0;
		mi = &ctlr->mii;
		miistatus(mi);
		phy = mi->curphy;
		s = splhi();
		iow32(ctlr, TxdAddr, PCIWADDR(&txd[0]));
		iow32(ctlr, RxdAddr, PCIWADDR(&rxd[0]));
		iow16(ctlr, Cr, (phy->fd? FullDuplex: 0) | NoAutoPoll | TxOn |
			RxOn | Start | Rdmd);
		iow16(ctlr, Isr, 0xFFFF);
		iow16(ctlr, Imr, 0xFFFF);
		iow8(ctlr, MiscIsr, 0xFF);
		iow8(ctlr, MiscImr, ~(3<<5));
		splx(s);
		ctlr->attached = 1;
	}
	qunlock(&ctlr->attachlck);
}

static void
txstart(Ether *edev)
{
	Ctlr *ctlr;
	Desc *txd, *td;
	int i, txused, n;
	RingBuf *tb;

	ctlr = edev->ctlr;
	txd = ctlr->txd;
	i = ctlr->txhead;
	n = 0;
	for (txused = ctlr->txused; txused < Ntxd; txused++) {
		tb = &edev->tb[edev->ti];
		if(tb->owner != Interface)
			break;

		td = &txd[i];
		memmove(td->buf, tb->pkt, tb->len);
		/* could reduce number of intrs here */
		td->size = tb->len | TxChainStart | TxChainEnd | TxInt;
		coherence();
		td->stat = OwnNic;
		i = (i + 1) % Ntxd;
		n++;

		tb->owner = Host;
		edev->ti = NEXT(edev->ti, edev->ntb);
	}
	if (n)
		iow16(ctlr, Cr, ior16(ctlr, Cr) | Tdmd);

	ctlr->txhead = i;
	ctlr->txused = txused;
}

static void
transmit(Ether *edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	ilock(&ctlr->tlock);
	txstart(edev);
	iunlock(&ctlr->tlock);
}

static void
txcomplete(Ether *edev)
{
	Ctlr *ctlr;
	Desc *txd, *td;
	int i, txused;
	ulong stat;

	ctlr = edev->ctlr;
 	txd = ctlr->txd;
	i = ctlr->txtail;
	for (txused = ctlr->txused; txused > 0; txused--) {
		td = &txd[i];
		stat = td->stat;
		if (stat & OwnNic)
			break;
		i = (i + 1) % Ntxd;
	}
	ctlr->txused = txused;
	ctlr->txtail = i;

	if (txused <= Ntxd/2)
		txstart(edev);
}

static void
interrupt(Ureg *, void *arg)
{
	Ether *edev;
	Ctlr *ctlr;
	RingBuf *rb;
	ushort  isr, misr;
	ulong stat;
	Desc *rxd, *rd;
	int i, n, size;

	edev = (Ether*)arg;
	ctlr = edev->ctlr;
	iow16(ctlr, Imr, 0);
	isr = ior16(ctlr, Isr);
	iow16(ctlr, Isr, 0xFFFF);
	/* don't care about used defined intrs */
	misr = ior16(ctlr, MiscIsr) & ~(3<<5);

	if (isr & RxOk) {
		rxd = ctlr->rxd;
		i = ctlr->rxtail;

		n = 0;
		while ((rxd[i].stat & OwnNic) == 0) {
			rd = &rxd[i];
			stat = rd->stat;
			if (stat & 0xFF)
				iprint("rx: %lux\n", stat & 0xFF);
			size = ((rd->stat>>16) & (2048-1)) - 4;

			rb = &edev->rb[edev->ri];
			if(rb->owner == Interface){
				rb->owner = Host;
				rb->len = size;
				memmove(rb->pkt, rd->buf, size);
				edev->ri = NEXT(edev->ri, edev->nrb);
			}

			rd->size = sizeof(Etherpkt)+4;
			coherence();
			rd->stat = OwnNic;
			i = (i + 1) % Nrxd;
			n++;
		}
		if (n)
			iow16(ctlr, Cr, ior16(ctlr, Cr) | Rdmd);
		ctlr->rxtail = i;
		isr &= ~RxOk;
	}
	if (isr & TxOk) {
		txcomplete(edev);
		isr &= ~TxOk;
	}
	if (isr | misr)
		iprint("etherrhine: unhandled irq(s). isr:%x misr:%x\n",
			isr, misr);
	iow16(ctlr, Imr, 0xFFFF);
}

static int
miiread(Mii *mii, int phy, int reg)
{
	Ctlr *ctlr;
	int n;

	ctlr = mii->ctlr;

	n = Nwait;
	while (n-- && ior8(ctlr, RhineMiiCr) & (Rcmd | Wcmd))
		microdelay(1);
	if (n == Nwait)
		iprint("etherrhine: miiread: timeout\n");

	iow8(ctlr, RhineMiiCr, 0);
	iow8(ctlr, RhineMiiPhy, phy);
	iow8(ctlr, RhineMiiAddr, reg);
	iow8(ctlr, RhineMiiCr, Rcmd);

	n = Nwait;
	while (n-- && ior8(ctlr, RhineMiiCr) & Rcmd)
		microdelay(1);
	if (n == Nwait)
		iprint("etherrhine: miiread: timeout\n");

	return ior16(ctlr, RhineMiiData);
}

static int
miiwrite(Mii *mii, int phy, int reg, int data)
{
	int n;
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	n = Nwait;
	while (n-- && ior8(ctlr, RhineMiiCr) & (Rcmd | Wcmd))
		microdelay(1);
	if (n == Nwait)
		iprint("etherrhine: miiwrite: timeout\n");

	iow8(ctlr, RhineMiiCr, 0);
	iow8(ctlr, RhineMiiPhy, phy);
	iow8(ctlr, RhineMiiAddr, reg);
	iow16(ctlr, RhineMiiData, data);
	iow8(ctlr, RhineMiiCr, Wcmd);

	n = Nwait;
	while (n-- && ior8(ctlr, RhineMiiCr) & Wcmd)
		microdelay(1);
	if (n == Nwait)
		iprint("etherrhine: miiwrite: timeout\n");

	return 0;
}

static void
reset(Ctlr* ctlr)
{
	int r, timeo;

	/*
	 * Soft reset the controller.
	 */
	csr16w(ctlr, Cr, Stop);
	csr16w(ctlr, Cr, Stop|Reset);
	for(timeo = 0; timeo < 10000; timeo++){
		if(!(csr16r(ctlr, Cr) & Reset))
			break;
		microdelay(1);
	}
	if(timeo >= 1000)
		return;

	/*
	 * Load the MAC address into the PAR[01]
	 * registers.
	 */
	r = csr8r(ctlr, Eecsr);
	csr8w(ctlr, Eecsr, EeAutoLoad|r);
	for(timeo = 0; timeo < 100; timeo++){
		if(!(csr8r(ctlr, Cr) & EeAutoLoad))
			break;
		microdelay(1);
	}
	if(timeo >= 100)
		return;

	/*
	 * Configure DMA and Rx/Tx thresholds.
	 * If the Rx/Tx threshold bits in Bcr[01] are 0 then
	 * the thresholds are determined by Rcr/Tcr.
	 */
	r = csr8r(ctlr, Bcr0) & ~(CrftMASK|DmaMASK);
	csr8w(ctlr, Bcr0, r|Crft64|Dma64);
	r = csr8r(ctlr, Bcr1) & ~CtftMASK;
	csr8w(ctlr, Bcr1, r|Ctft64);

	r = csr8r(ctlr, Rcr) & ~(RrftMASK|Prom|Ar|Sep);
	csr8w(ctlr, Rcr, r|Ab|Am);

	r = csr8r(ctlr, Tcr) & ~(RtsfMASK|Ofset|Lb1|Lb0);
	csr8w(ctlr, Tcr, r);
}

static void
detach(Ether* edev)
{
	reset(edev->ctlr);
}

static void
init(Ether *edev)
{
	Ctlr *ctlr;
	int i;

	ctlr = edev->ctlr;
	ilock(&ctlr->tlock);

	pcisetbme(ctlr->pci);
	reset(ctlr);

	iow8(ctlr, Eecsr, ior8(ctlr, Eecsr) | EeAutoLoad);
	for (i = 0; i < Nwait; ++i) {
		if ((ior8(ctlr, Eecsr) & EeAutoLoad) == 0)
			break;
		delay(5);
	}
	if (i >= Nwait)
		iprint("etherrhine: eeprom autoload timeout\n");

	for (i = 0; i < Eaddrlen; ++i)
		edev->ea[i] = ior8(ctlr, Eaddr + i);

	ctlr->mii.mir = miiread;
	ctlr->mii.miw = miiwrite;
	ctlr->mii.ctlr = ctlr;

	if(mii(&ctlr->mii, ~0) == 0 || ctlr->mii.curphy == nil){
		iunlock(&ctlr->tlock);
		iprint("etherrhine: init mii failure\n");
		return;
	}
	for (i = 0; i < NMiiPhy; ++i)
		if (ctlr->mii.phy[i])
			if (ctlr->mii.phy[i]->oui != 0xFFFFF)
				ctlr->mii.curphy = ctlr->mii.phy[i];
	miistatus(&ctlr->mii);

	iow16(ctlr, Imr, 0);
	iow16(ctlr, Cr, ior16(ctlr, Cr) | Stop);

	iunlock(&ctlr->tlock);
}

static Pcidev *
rhinematch(ulong)
{
	static int nrhines = 0;
	int nfound = 0;
	Pcidev *p = nil;

	while(p = pcimatch(p, 0x1106, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;
		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x3053<<16)|0x1106:	/* Rhine III vt6105m (Soekris) */
		case (0x3065<<16)|0x1106:	/* Rhine II */
		case (0x3106<<16)|0x1106:	/* Rhine III */
			if (++nfound > nrhines) {
				nrhines++;
				return p;
			}
			break;
		}
	}
	return p;
}

int
rhinepnp(Ether *edev)
{
	Pcidev *p;
	Ctlr *ctlr;
	ulong port;

	if (edev->attach)
		return 0;
	p = rhinematch(edev->port);
	if (p == nil)
		return -1;

	port = p->mem[0].bar & ~1;

	if ((ctlr = malloc(sizeof(Ctlr))) == nil) {
		print("etherrhine: couldn't allocate memory for ctlr\n");
		return -1;
	}
	memset(ctlr, 0, sizeof(Ctlr));
	ctlr->txd = xspanalloc(sizeof(Desc) * Ntxd, 16, 0);
	ctlr->rxd = xspanalloc(sizeof(Desc) * Nrxd, 16, 0);

	ctlr->pci = p;
	ctlr->port = port;

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = p->intl;
	edev->tbdf = p->tbdf;

	init(edev);

	edev->attach = attach;
	edev->transmit = transmit;
	edev->interrupt = interrupt;
	edev->detach = detach;

	return 0;
}

int
vt6102pnp(Ether *edev)
{
	return rhinepnp(edev);
}
