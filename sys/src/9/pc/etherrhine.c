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
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

#include "ethermii.h"

typedef struct Desc Desc;
typedef struct Ctlr Ctlr;

enum {
	Ntxd = 16,
	Nrxd = 64,
	Nwait = 50,
	Ntxstats = 9,
	Nrxstats = 8,
	BIGSTR = 8192,
};

struct Desc {
	ulong stat;
	ulong size;
	ulong addr;
	ulong next;
	char *buf;
	ulong pad[3];
};

struct Ctlr {
	Pcidev *pci;
	int attached;
	int txused;
	int txhead;
	int txtail;
	int rxtail;
	ulong port;

	Mii mii;

	ulong txstats[Ntxstats];
	ulong rxstats[Nrxstats];

	Desc *txd;	/* wants to be aligned on 16-byte boundary */
	Desc *rxd;

	QLock attachlck;
	Lock lock;
};

#define ior8(c, r)	(inb((c)->port+(r)))
#define ior16(c, r)	(ins((c)->port+(r)))
#define ior32(c, r)	(inl((c)->port+(r)))
#define iow8(c, r, b)	(outb((c)->port+(r), (int)(b)))
#define iow16(c, r, w)	(outs((c)->port+(r), (ushort)(w)))
#define iow32(c, r, l)	(outl((c)->port+(r), (ulong)(l)))

enum Regs {
	Eaddr = 0x0,
	Rcr = 0x6,
	Tcr = 0x7,
	Cr = 0x8,
	Isr = 0xc,
	Imr = 0xe,
	McastAddr = 0x10,
	RxdAddr = 0x18,
	TxdAddr = 0x1C,
	Bcr = 0x6e,
	RhineMiiPhy = 0x6C,
	RhineMiiSr = 0x6D,
	RhineMiiCr = 0x70,
	RhineMiiAddr = 0x71,
	RhineMiiData = 0x72,
	Eecsr = 0x74,
	ConfigB = 0x79,
	ConfigD = 0x7B,
	MiscCr = 0x80,
	HwSticky = 0x83,
	MiscIsr = 0x84,
	MiscImr = 0x86,
	WolCrSet = 0xA0,
	WolCfgSet = 0xA1,
	WolCgSet = 0xA3,
	WolCrClr = 0xA4,
	PwrCfgClr = 0xA5,
	WolCgClr = 0xA7,
};

enum Rcrbits {
	RxErrX = 1<<0,
	RxSmall = 1<<1,
	RxMcast = 1<<2,
	RxBcast = 1<<3,
	RxProm = 1<<4,
	RxFifo64 = 0<<5, RxFifo32 = 1<<5, RxFifo128 = 2<<5, RxFifo256 = 3<<5,
	RxFifo512 = 4<<5, RxFifo768 = 5<<5, RxFifo1024 = 6<<5,
	RxFifoStoreForward = 7<<5,
};

enum Tcrbits {
	TxLoopback0 = 1<<1,
	TxLoopback1 = 1<<2,
	TxBackoff = 1<<3,
	TxFifo128 = 0<<5, TxFifo256 = 1<<5, TxFifo512 = 2<<5, TxFifo1024 = 3<<5,
	TxFifoStoreForward = 7<<5,
};

enum Crbits {
	Init = 1<<0,
	Start = 1<<1,
	Stop = 1<<2,
	RxOn = 1<<3,
	TxOn = 1<<4,
	Tdmd = 1<<5,
	Rdmd = 1<<6,
	EarlyRx = 1<<8,
	Reserved0 = 1<<9,
	FullDuplex = 1<<10,
	NoAutoPoll = 1<<11,
	Reserved1 = 1<<12,
	Tdmd1 = 1<<13,
	Rdmd1 = 1<<14,
	Reset = 1<<15,
};

enum Isrbits {
	RxOk = 1<<0,
	TxOk = 1<<1,
	RxErr = 1<<2,
	TxErr = 1<<3,
	TxBufUdf = 1<<4,
	RxBufLinkErr = 1<<5,
	BusErr = 1<<6,
	CrcOvf = 1<<7,
	EarlyRxInt = 1<<8,
	TxFifoUdf = 1<<9,
	RxFifoOvf = 1<<10,
	TxPktRace = 1<<11,
	NoRxbuf = 1<<12,
	TxCollision = 1<<13,
	PortCh = 1<<14,
	GPInt = 1<<15
};

enum Bcrbits {
	Dma32 = 0<<0, Dma64 = 1<<0, Dma128 = 2<<0,
	Dma256 = 3<<0, Dma512 = 4<<0, Dma1024 = 5<<0,
	DmaStoreForward = 7<<0,
	DupRxFifo0 = 1<<3, DupRxFifo1 = 1<<4, DupRxFifo2 = 1<<5,
	ExtraLed = 1<<6,
	MediumSelect = 1<<7,
	PollTimer0 = 1<<8, PollTimer1 = 1<<9, PollTimer2 = 1<<10,
	DupTxFifo0 = 1<<11, DupTxFifo1 = 1<<12, DupTxFifo2 = 1<<13,
};

enum Eecsrbits {
	EeAutoLoad = 1<<5,
};

enum MiscCrbits {
	Timer0Enable= 1<<0,
	Timer0Suspend = 1<<1,
	HalfDuplexFlowControl = 1<<2,
	FullDuplexFlowControl = 1<<3,
	Timer1Enable = 1<<8,
	ForceSoftReset = 1<<14,
};

enum HwStickybits {
	StickyDS0 = 1<<0,
	StickyDS1 = 1<<1,
	WOLEna = 1<<2,
	WOLStat = 1<<3,
};

enum WolCgbits {
	PmeOvr = 1<<7,
};

enum Descbits {
	OwnNic = 1<<31,		/* stat */
	TxAbort = 1<<8,		/* stat */
	TxError = 1<<15,		/* stat */
	RxChainbuf = 1<<10,	/* stat */
	RxChainStart = 1<<9,	/* stat */
	RxChainEnd = 1<<8,		/* stat */
	Chainbuf = 1<<15,		/* size rx & tx*/
	TxDisableCrc = 1<<16,	/* size */
	TxChainStart = 1<<21,	/* size */
	TxChainEnd = 1<<22,	/* size */
	TxInt = 1<<23,			/* size */
};

enum ConfigDbits {
	BackoffOptional = 1<<0,
	BackoffAMD = 1<<1,
	BackoffDEC = 1<<2,
	BackoffRandom = 1<<3,
	PmccTestMode = 1<<4,
	PciReadlineCap = 1<<5,
	DiagMode = 1<<6,
	MmioEnable = 1<<7,
};

enum ConfigBbits {
	LatencyTimer = 1<<0,
	WriteWaitState = 1<<1,
	ReadWaitState = 1<<2,
	RxArbit = 1<<3,
	TxArbit = 1<<4,
	NoMemReadline = 1<<5,
	NoParity = 1<<6,
	NoTxQueuing = 1<<7,
};

enum RhineMiiCrbits {
	Mdc = 1<<0,
	Mdi = 1<<1,
	Mdo = 1<<2,
	Mdout = 1<<3,
	Mdpm = 1<<4,
	Wcmd = 1<<5,
	Rcmd = 1<<6,
	Mauto = 1<<7,
};

enum RhineMiiSrbits {
	Speed10M = 1<<0,
	LinkFail = 1<<1,
	PhyError = 1<<3,
	DefaultPhy = 1<<4,
	ResetPhy = 1<<7,
};

enum RhineMiiAddrbits {
	Mdone = 1<<5,
	Msrcen = 1<<6,
	Midle = 1<<7,
};

static char *
txstatnames[Ntxstats] = {
	"aborts (excess collisions)",
	"out of window collisions",
	"carrier sense losses",
	"fifo underflows",
	"invalid descriptor format or underflows",
	"system errors",
	"reserved",
	"transmit errors",
	"collisions",
};

static char *
rxstatnames[Nrxstats] = {
	"receiver errors",
	"crc errors",
	"frame alignment errors",
	"fifo overflows",
	"long packets",
	"run packets",
	"system errors",
	"buffer underflows",
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
		iow16(ctlr, Cr, (phy->fd ? FullDuplex : 0) | NoAutoPoll | TxOn | RxOn | Start | Rdmd);
		iow16(ctlr, Isr, 0xFFFF);
		iow16(ctlr, Imr, 0xFFFF);
		iow8(ctlr, MiscIsr, 0xFF);
		iow8(ctlr, MiscImr, ~(3<<5));
		splx(s);
	}
	ctlr->attached++;
	qunlock(&ctlr->attachlck);
}

static void
txstart(Ether *edev)
{
	Ctlr *ctlr;
	Desc *txd, *td;
	Block *b;
	int i, txused, n;
	ulong size;

	ctlr = edev->ctlr;

	txd = ctlr->txd;
	i = ctlr->txhead;
	txused = ctlr->txused;
	n = 0;
	while (txused < Ntxd) {
		if ((b = qget(edev->oq)) == nil)
			break;

		td = &txd[i];

		size = BLEN(b);
		memmove(td->buf, b->rp, size);
		freeb(b);
		td->size = size | TxChainStart | TxChainEnd | TxInt; /* could reduce number of ints here */
		coherence();
		td->stat = OwnNic;
		i = (i + 1) % Ntxd;
		txused++;
		n++;
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
	ilock(&ctlr->lock);
	txstart(edev);
	iunlock(&ctlr->lock);
}

static void
txcomplete(Ether *edev)
{
	Ctlr *ctlr;
	Desc *txd, *td;
	int i, txused, j;
	ulong stat;

	ctlr = edev->ctlr;
 	txd = ctlr->txd;
	txused = ctlr->txused;
	i = ctlr->txtail;
	while (txused > 0) {
		td = &txd[i];
		stat = td->stat;

		if (stat & OwnNic)
			break;

		ctlr->txstats[Ntxstats-1] += stat & 0xF;
		for (j = 0; j < Ntxstats-1; ++j)
			if (stat & (1<<(j+8)))
				ctlr->txstats[j]++;

		i = (i + 1) % Ntxd;
		txused--;
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
	ushort  isr, misr;
	ulong stat;
	Desc *rxd, *rd;
	int i, n, j;

	edev = (Ether*)arg;
	ctlr = edev->ctlr;
	iow16(ctlr, Imr, 0);
	isr = ior16(ctlr, Isr);
	iow16(ctlr, Isr, 0xFFFF);
	misr = ior16(ctlr, MiscIsr) & ~(3<<5); /* don't care about used defined ints */

	if (isr & RxOk) {
		Block *b;
		int size;
		rxd = ctlr->rxd;
		i = ctlr->rxtail;

		n = 0;
		while ((rxd[i].stat & OwnNic) == 0) {
			rd = &rxd[i];
			stat = rd->stat;
			for (j = 0; j < Nrxstats; ++j)
				if (stat & (1<<j))
					ctlr->rxstats[j]++;

			if (stat & 0xFF)
				iprint("rx: %lux\n", stat & 0xFF);

			size = ((rd->stat>>16) & 2047) - 4;
			b = iallocb(sizeof(Etherpkt));
			memmove(b->wp, rd->buf, size);
			b->wp += size;
			etheriq(edev, b, 1);
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
		iprint("etherrhine: unhandled irq(s). isr:%x misr:%x\n", isr, misr);

	iow16(ctlr, Imr, 0xFFFF);
}

static void
promiscuous(void *arg, int enable)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(&ctlr->lock);
	iow8(ctlr, Rcr, (ior8(ctlr, Rcr) & ~(RxProm|RxBcast)) |
		(enable ? RxProm : RxBcast));
	iunlock(&ctlr->lock);
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

	n = ior16(ctlr, RhineMiiData);

	return n;
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

/* multicast already on, don't need to do anything */
static void
multicast(void*, uchar*, int)
{
}

static void
shutdown(Ether *edev)
{
	int i;
	Ctlr *ctlr = edev->ctlr;

	ilock(&ctlr->lock);
	pcisetbme(ctlr->pci);

	iow16(ctlr, Cr, ior16(ctlr, Cr) | Stop);
	iow16(ctlr, Cr, ior16(ctlr, Cr) | Reset);

	for (i = 0; i < Nwait; ++i) {
		if ((ior16(ctlr, Cr) & Reset) == 0)
			break;
		delay(5);
	}
	if (i == Nwait)
		iprint("etherrhine: reset timeout\n");
	iunlock(&ctlr->lock);
}

static void
init(Ether *edev)
{
	Ctlr *ctlr;
	MiiPhy *phy;
	int i;

	shutdown(edev);

	ctlr = edev->ctlr;
	ilock(&ctlr->lock);
	iow8(ctlr, Eecsr, ior8(ctlr, Eecsr) | EeAutoLoad);
	for (i = 0; i < Nwait; ++i) {
		if ((ior8(ctlr, Eecsr) & EeAutoLoad) == 0)
			break;
		delay(5);
	}
	if (i == Nwait)
		iprint("etherrhine: eeprom autoload timeout\n");

	for (i = 0; i < Eaddrlen; ++i)
		edev->ea[i] = ior8(ctlr, Eaddr + i);

	ctlr->mii.mir = miiread;
	ctlr->mii.miw = miiwrite;
	ctlr->mii.ctlr = ctlr;

	if(mii(&ctlr->mii, ~0) == 0 || ctlr->mii.curphy == nil){
		iprint("etherrhine: init mii failure\n");
		return;
	}
	for (i = 0; i < NMiiPhy; ++i)
		if (ctlr->mii.phy[i])
			if (ctlr->mii.phy[i]->oui != 0xFFFFF)
				ctlr->mii.curphy = ctlr->mii.phy[i];

	miistatus(&ctlr->mii);
	phy = ctlr->mii.curphy;
	edev->mbps = phy->speed;

	iow16(ctlr, Imr, 0);
	iow16(ctlr, Cr, ior16(ctlr, Cr) | Stop);
	iow8(ctlr, Rcr, ior8(ctlr, Rcr) | RxMcast);

	iunlock(&ctlr->lock);
}

static Pcidev *
rhinematch(ulong)
{
	static int nrhines = 0;
	int nfound = 0;
	Pcidev *p = nil;

	while (p = pcimatch(p, 0x1106, 0))
		if (p->did == 0x3065)
			if (++nfound > nrhines) {
				nrhines++;
				break;
			}
	return p;
}
static long
ifstat(Ether* edev, void* a, long n, ulong offset)
{
	int l = 0, i;
	char *p;
	Ctlr *ctlr;
	ctlr = edev->ctlr;
	p = malloc(BIGSTR);

	for (i = 0; i < Ntxstats; ++i)
		if (txstatnames[i])
			l += snprint(p+l, BIGSTR - l, "tx: %s: %lud\n", txstatnames[i], ctlr->txstats[i]);

	for (i = 0; i < Nrxstats; ++i)
		if (rxstatnames[i])
			l += snprint(p+l, BIGSTR - l, "rx: %s: %lud\n", rxstatnames[i], ctlr->rxstats[i]);

/*
	for (i = 0; i < NMiiPhyr; ++i) {
		if ((i % 8) == 0)
			l += snprint(p + l, BIGSTR - l, "\nmii 0x%02x:", i);
		reg=miimir(&ctlr->mii, i);
		reg=miimir(&ctlr->mii, i);
		l += snprint(p + l, BIGSTR - l, " %4ux", reg);
	}

	for (i = 0; i < 0x100; i+=1) {
		if ((i % 16) == 0)
			l += snprint(p + l, BIGSTR - l, "\nreg 0x%02x:", i);
		else if ((i % 2) == 0)
			l += snprint(p + l, BIGSTR - l, " ");
		reg=ior8(ctlr, i);
		l += snprint(p + l, BIGSTR - l, "%02x", reg);
	}
	l += snprint(p + l, BIGSTR - l, " \n");
*/


	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static int
pnp(Ether *edev)
{
	Pcidev *p;
	Ctlr *ctlr;
	ulong port;
	ulong size;

	p = rhinematch(edev->port);
	if (p == nil)
		return -1;

	port = p->mem[0].bar & ~1;
	size = p->mem[0].size;
	if (ioalloc(port, size, 0, "rhine") < 0) {
		print("etherrhine: couldn't allocate port %lud\n", port);
		return -1;
	}

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

	edev->interrupt = interrupt;
	edev->arg = edev;

	edev->attach = attach;
	edev->transmit = transmit;
	edev->ifstat = ifstat;
	edev->promiscuous = promiscuous;
	edev->multicast = multicast;
	edev->shutdown = shutdown;
	return 0;
}

void
etherrhinelink(void)
{
	addethercard("rhine", pnp);
}
