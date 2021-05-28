/*
 * Xilinx XPS LL Temac Ethernet v1.00b + XPS LL Local Link FIFOs v1.00a driver.
 *
 * It uses the Central DMA controller.
 * There are two interfaces per Temac controller.
 * Each Temac interface is connected to a pair of FIFOs (tx and rx).
 * Half-duplex is not supported by hardware.
 *
 * NB: The LL FIFO requires that we copy the data for a single packet
 * into the FIFO, then write the frp->tlr register before writing any
 * more data into the FIFO.  It forbids copying the data for multiple
 * packets into the FIFO and then writing frp->tlr with the length of each.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../ip/ip.h"		/* to declare ethermedium */

#include "ethermii.h"
#include "../port/netif.h"

#include "etherif.h"
#include "io.h"

enum {				/* directly-addressible registers' bits */
	/* raf register */
	Htrst	= 1<<0,		/* hard temac reset (both ifcs) */
	Mcstrej	= 1<<1,		/* reject received multicast dest addr */
	Bcstrej	= 1<<2,		/* reject received broadcast dest addr */

	/* is, ip, ie register */
	Hardacscmplt = 1<<0,	/* hard register access complete */
	Autoneg	= 1<<1,		/* auto-negotiation complete */
	Rxcmplt	= 1<<2,		/* receive complete */
	Rxrject	= 1<<3,		/* receive frame rejected (unknown m'cast?) */
	Rxfifoovr = 1<<4,	/* receive fifo overrun */
	Txcmplt	= 1<<5,		/* transmit complete */
	Rxdcmlock = 1<<6,	/* receive DCM lock (ready for use) */

	/* ctl register */
	Wen	= 1<<15,	/* write instead of read */

	/* ctl register address codes; select other registers */
	Rcw0	= 0x200,	/* receive configuration: pause addr low */
	Rcw1	= 0x240,	/* ": misc bits + pause addr high */
	Tc	= 0x280,	/* tx config */
	Fcc	= 0x2c0,	/* flow control */
	Emmc	= 0x300,	/* ethernet mac mode config */
	Phyc	= 0x320,	/* rgmii/sgmii config */
	Mc	= 0x340,	/* mgmt config */
	Uaw0	= 0x380,	/* unicast addr word 0 (low-order) */
	Uaw1	= 0x384,	/* unicast addr word 1 (high-order) */
	Maw0	= 0x388,	/* multicast addr word 0 (low) */
	Maw1	= 0x38c,	/* multicast addr word 1 (high + more) */
	Afm	= 0x390,	/* addr filter mode */
	Tis	= 0x3a0,	/* intr status */
	Tie	= 0x3a4,	/* intr enable */
	Miimwd	= 0x3b0,	/* mii mgmt write data */
	Miimai	= 0x3b4,	/* mii mgmt access initiate */

	/* rdy register */
	Fabrrr	= 1<<0,		/* fabric read ready */
	Miimrr	= 1<<1,		/* mii mgmt read ready */
	Miimwr	= 1<<2,		/* mii mgmt write ready */
	Afrr	= 1<<3,		/* addr filter read ready */
	Afwr	= 1<<4,		/* addr filter write ready */
	Cfgrr	= 1<<5,		/* config reg read ready */
	Cfgwr	= 1<<6,		/* config reg write ready */
	Hardacsrdy = 1<<16,	/* hard reg access ready */
};
enum {				/* indirectly-addressible registers' bits */
	/* Rcw1 register */
	Rst	= 1<<31,	/* reset */
	Jum	= 1<<30,	/* jumbo frame enable */
	Fcs	= 1<<29,	/* in-band fcs enable */
	Rx	= 1<<28,	/* rx enable */
	Vlan	= 1<<27,	/* vlan frame enable */
	Hd	= 1<<26,	/* half-duplex mode (must be 0) */
	Ltdis	= 1<<25,	/* length/type field valid check disable */

	/* Tc register.  same as Rcw1 but Rx->Tx, Ltdis->Ifg */
	Tx	= Rx,		/* tx enable */
	Ifg	= Ltdis,	/* inter-frame gap adjustment enable */

	/* Fcc register */
	Fctx	= 1<<30,	/* tx flow control enable */
	Fcrx	= 1<<29,	/* rx flow control enable */

	/* Emmc register */
	Linkspeed = 3<<30,	/* field */
	Ls1000	= 2<<30,	/* Gb */
	Ls100	= 1<<30,	/* 100Mb */
	Ls10	= 0<<30,	/* 10Mb */
	Rgmii	= 1<<29,	/* rgmii mode enable */
	Sgmii	= 1<<28,	/* sgmii mode enable */
	Gpcs	= 1<<27,	/* 1000base-x mode enable */
	Hostifen= 1<<26,	/* host interface enable */
	Tx16	= 1<<25,	/* tx 16-bit (vs 8-bit) data ifc enable (0) */
	Rx16	= 1<<24,	/* rx 16-bit (vs 8-bit) data ifc enable (0) */

	/* Phyc register.  sgmii link speed is Emmc's Linkspeed. */
	Rgmiills = 3<<2,	/* field */
	Rls1000	= 2<<2,		/* Gb */
	Rls100	= 1<<2,		/* 100Mb */
	Rls10	= 0<<2,		/* 10Mb */
	Rgmiihd	= 1<<1,		/* half-duplex */
	Rgmiilink = 1<<0,	/* rgmii link (is up) */

	/* Mc register */
	Mdioen	= 1<<6,		/* mdio (mii mgmt) enable */

	/* Maw1 register */
	Rnw	= 1<<23,	/* multicast addr table reg read (vs write) */
	Addr	= 3<<16,	/* field */

	/* Afm register */
	Pm	= 1<<31,	/* promiscuous mode */

	/* Tis, Tie register (*rst->*en) */
	Fabrrst	= 1<<0,		/* fabric read intr sts (read done) */
	Miimrst	= 1<<1,		/* mii mgmt read intr sts (read done) */
	Miimwst	= 1<<2,		/* mii mgmt write intr sts (write done) */
	Afrst	= 1<<3,		/* addr filter read intr sts (read done) */
	Afwst	= 1<<4,		/* addr filter write intr sts (write done) */
	Cfgrst	= 1<<5,		/* config read intr sts (read done) */
	Cfgwst	= 1<<6,		/* config write intr sts (write done) */
};

enum {
	/* tunable parameters */
	Defmbps	= 1000,		/* default Mb/s */
	Defls	= Ls1000,	/* must match Defmbps */
};

enum {				/* fifo's registers' bits */
	/* isr, ier registers (*?e -> *ee) */
	Rpure	= 1<<31,	/* rx packet underrun read error */
	Rpore	= 1<<30,	/* rx packet overrun read error */
	Rpue	= 1<<29,	/* rx packet underrun error */
	Tpoe	= 1<<28,	/* tx packet overrun error */
	FifoTc	= 1<<27,	/* tx complete */
	Rc	= 1<<26,	/* rx complete */
	Tse	= 1<<25,	/* tx size error */
	Trc	= 1<<24,	/* tx reset complete */
	Rrc	= 1<<23,	/* rx reset complete */

	Errors	= Rpure | Rpore | Rpue | Tpoe | Tse,
};
enum {				/* fifo constants */
	Chan0,			/* dedicated to input */
	Chan1,			/* dedicated to output */

	Reset	= 0xa5,		/* magic [tr]dfr & llr value */

	/* dmacr; copied from dma.c */
	Sinc	= 1<<31,	/* source increment */
	Dinc	= 1<<30,	/* dest increment */

	/* field masks */
	Bytecnt	= (1<<11) - 1,
	Wordcnt	= (1<<9) - 1,
};

typedef struct Temacctlr Temacctlr;
typedef struct Temacregs Temacregs;
typedef struct Llfiforegs Llfiforegs;

struct Temacregs {
	ulong	raf;		/* reset & addr filter */
	ulong	tpf;		/* tx pause frame */
	ulong	ifgp;		/* tx inter-frame gap adjustment */
	ulong	is;		/* intr status */
	ulong	ip;		/* intr pending */
	ulong	ie;		/* intr enable */
	ulong	pad[2];

	ulong	msw;		/* msw data; shared by ifcs */
	ulong	lsw;		/* lsw data; shared */
	ulong	ctl;		/* control; shared */
	ulong	rdy;		/* ready status */
	ulong	pad2[4];
};
struct Temacctlr {		/* software state */
	Temacregs *regs;
	Lock;
	ushort	active;

	/* tx state, for Block being sent */
	int	txdma;
	Block	*tbp;		/* non-nil if dma to fifo in progress */

	/* rx state, for packet being read */
	int	rxdma;
	long	rlf;		/* read from frp->rlf iff non-negative */
	ulong	fifoier;
	Block	*rbp;		/* non-nil if dma from fifo in progress */
};

struct Llfiforegs {
	ulong	isr;		/* intr status */
	ulong	ier;		/* intr enable */

	ulong	tdfr;		/* tx data fifo reset */
	ulong	tdfv;		/* tx data fifo vacancy (words free) */
	ulong	tdfd;		/* tx data fifo write port */
	ulong	tlf;		/* tx length fifo */

	ulong	rdfr;		/* rx data fifo reset */
	ulong	rdfo;		/* rx data fifo occupancy */
	ulong	rdfd;		/* rx data fifo read port */
	ulong	rlf;		/* rx length fifo */

	ulong	llr;		/* locallink reset */
};

typedef struct {
	ulong	bit;
	char	*msg;
} Error;

extern int dmaready;		/* flag: ready for general use? */
extern uchar mymac[Eaddrlen];

static Error errs[] = {
	Rpure,	"rx pkt underrun read",
	Rpore,	"rx pkt overrun read",
	Rpue,	"rx pkt underrun",
	Tpoe,	"tx pkt overrun",
	Tse,	"tx size",
};

static Ether *ethers[1];	/* only first ether is connected to a fifo */
static Llfiforegs *frp = (Llfiforegs *)Llfifo;

int	fifointr(ulong bit);

static	void	fiforeset(Ether *ether);
static	int	dmatxstart(Ether *);

static void
getready(Temacregs *trp)
{
	while ((trp->rdy & Hardacsrdy) == 0)
		;
}

static ulong
rdindir(Temacregs *trp, unsigned code)
{
	ulong val;

	getready(trp);
	trp->ctl = code;
	barriers();

	getready(trp);
	val = trp->lsw;
	return val;
}

static int
wrindir(Temacregs *trp, unsigned code, ulong val)
{
	getready(trp);
	trp->lsw = val;
	barriers();
	trp->ctl = Wen | code;
	barriers();

	getready(trp);
	return 0;
}

/*
 * we're only interested in temac errors; completion interrupts come
 * from the fifo.
 */
static int
temacintr(ulong bit)
{
	int e, forme, sts;
	Ether *ether;
	Temacctlr *ctlr;
	Temacregs *trp;

	forme = 0;
	for (e = 0; e < nelem(ethers); e++) {
		ether = ethers[e];
		if (ether == nil)
			continue;
		ether->interrupts++;
		ctlr = ether->ctlr;
		trp = ctlr->regs;
		while ((sts = trp->is) & (Autoneg | Rxfifoovr)) {
			forme = 1;
			if (sts & Rxfifoovr) {
				iprint("temac: receive fifo overrun.  ");
				whackether(ether);
			}
			if (sts & Autoneg) {
				iprint("temac: autoneg done\n");
				trp->is = Autoneg;  /* extinguish intr source */
				barriers();
			}
		}
	}
	if (forme)
		intrack(bit);
	return forme;
}

static void
resetsw(Ether *ether)			/* reset software state */
{
	Temacctlr *ctlr;

	ctlr = ether->ctlr;

	delay(20);		/* let any dma in progress complete */
	ctlr->active = ctlr->txdma = ctlr->rxdma = 0;
	ctlr->rlf = -1;
	ctlr->rbp = ctlr->tbp = nil;
	barriers();
}

static void
reset(Ether *ether)
{
	Temacctlr *ctlr;
	Temacregs *trp;

	ctlr = ether->ctlr;
	trp = ctlr->regs;
	assert(trp);

	trp->raf = Htrst;	/* resets both ether interfaces */
	barriers();
	while (trp->raf & Htrst)
		;

	fiforeset(ether);
	barriers();

	resetsw(ether);
}

static void
attach(Ether *)
{
}

static void
closed(Ether *ether)
{
	Temacctlr *ctlr;

	ctlr = ether->ctlr;
	if(ctlr->active){
		reset(ether);
		ctlr->active = 0;
	}
}

static void
temacshutdown(Ether* ether)
{
	reset(ether);		/* stop dma at the very least */
}

static int promon, multion;

static void
promiscuous(void* arg, int on)
{
	Ether *ether;
	Temacctlr *ctlr;

	ether = (Ether*)arg;
	ctlr = ether->ctlr;
	ilock(ctlr);
	promon = on;
	if(on) {
		ether->promisc = 1;
		wrindir(ctlr->regs, Afm, Pm);
	} else if (!multion) {
		ether->promisc = 0;
		wrindir(ctlr->regs, Afm, 0);
	}
	iunlock(ctlr);
}

static void
multicast(void* arg, uchar *addr, int on)
{
	Ether *ether;
	Temacctlr *ctlr;

	ether = (Ether*)arg;
	ctlr = ether->ctlr;
	multion = on;
	ilock(ctlr);
	/*
	 * could do better: set Maw[01] & Rnw since ipv6 needs multicast;
	 * see netmulti() in netif.c.
	 */
	USED(addr);
	if(on) {
		ether->promisc = 1;
		wrindir(ctlr->regs, Afm, Pm);		/* overkill */
	} else if (!promon) {
		ether->promisc = 0;
		wrindir(ctlr->regs, Afm, 0);
	}
	iunlock(ctlr);
}

/*
 * start next dma into tx fifo, if there's a pkt in the output queue,
 * room in the tx fifo, and the dma channel is idle.
 *
 * called for each new packet, but okay to send
 * any and all packets in output queue.
 */
void
temactransmit(Ether *ether)
{
	Temacctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(ctlr);
	dmatxstart(ether);
	iunlock(ctlr);
}

/*
 * allocate receive buffer space on cache-line boundaries
 */
Block*
clallocb(void)
{
	Block *bp;

	/* round up for sake of word-at-a-time dma */
	bp = iallocb(ROUNDUP(ETHERMAXTU, BY2WD) + DCACHELINESZ-1);
	if(bp == nil)
		return bp;
	dcflush(PTR2UINT(bp->base), BALLOC(bp));
	bp->wp = bp->rp = (uchar*)
		((PTR2UINT(bp->base) + DCACHELINESZ - 1) & ~(DCACHELINESZ-1));
	return bp;
}

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	char *p;
	int len;

	if(n == 0)
		return 0;

	p = malloc(READSTR);

	len = snprint(p, READSTR, "interrupts: %lud\n", ether->interrupts);
	len += snprint(p+len, READSTR-len, "dma rx intrs: %lud\n",
		ether->dmarxintr);
	len += snprint(p+len, READSTR-len, "dma tx intrs: %lud\n",
		ether->dmatxintr);
	len += snprint(p+len, READSTR-len, "broadcasts rcvd: %lud\n",
		ether->bcasts);
	len += snprint(p+len, READSTR-len, "multicasts rcvd: %lud\n",
		ether->mcasts);
	len += snprint(p+len, READSTR-len, "promiscuous: %lud\n",
		ether->promisc);
	len += snprint(p+len, READSTR-len, "dropped pkts: %lud\n",
		ether->pktsdropped);
	len += snprint(p+len, READSTR-len, "resets: %lud\n", ether->resets);
	snprint(p+len, READSTR-len, "misaligned buffers: %lud\n",
		ether->pktsmisaligned);

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
init(Ether *ether)
{
	int i;
	ulong ealo, eahi;
	uvlong ea;
	Temacctlr *ctlr;
	Temacregs *trp;
	static uchar pausemac[Eaddrlen] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x01 };

	trp = (Temacregs *)Temac + ether->ctlrno;
	ctlr = ether->ctlr;

	wrindir(trp, Mc, Mdioen | 29);	/* 29 is divisor; see p.47 of ds537 */
	delay(100);			/* guess */

	/*
	 * mac addr is stored little-endian in longs in Uaw[01].
	 * default address is rubbish.
	 */
	memmove(ether->ea, mymac, Eaddrlen);
	ea = 0;
	for (i = 0; i < Eaddrlen; i++)
		ea |= (uvlong)mymac[i] << (i * 8);
	wrindir(trp, Uaw0, (ulong)ea);
	wrindir(trp, Uaw1, (ulong)(ea >> 32));
	ealo = rdindir(trp, Uaw0);
	eahi = rdindir(trp, Uaw1) & 0xffff;
	if (ealo != (ulong)ea || eahi != (ulong)(ea >> 32))
		panic("temac mac address wouldn't set, got %lux %lux",
			eahi, ealo);

	/*
	 * admit broadcast packets too
	 */
	wrindir(trp, Maw0, ~0ul);
	wrindir(trp, Maw1, 0xffff);	/* write to mat reg 0 */

	wrindir(trp, Afm, 0);		/* not promiscuous */
	wrindir(trp, Tc, Tx);
	ether->promisc = 0;

if (0) {
	ea = 0;
	for (i = 0; i < Eaddrlen; i++)
		ea |= (uvlong)pausemac[i] << (i * 8);
	wrindir(trp, Rcw0, (ulong)ea);
	wrindir(trp, Rcw1, (ulong)(ea >> 32) | Rx);
	wrindir(trp, Fcc, Fcrx);	/* honour pause frames sent to us */
} else
	wrindir(trp, Fcc, 0);		/* no flow control */

	wrindir(trp, Emmc, Defls);

	ctlr->active = 1;
	barriers();

	intrenable(Inttemac, temacintr, "lltemac");
	trp->ie = Autoneg | Rxfifoovr;
	barriers();

	fifoinit(ether);
}

int
temacreset(Ether* ether)
{
	Temacctlr *ctlr;
	Temacregs *trp;

	if ((unsigned)ether->ctlrno >= nelem(ethers) || ethers[ether->ctlrno])
		return -1;		/* already probed & found */
	trp = (Temacregs *)Temac + ether->ctlrno;

	/* too early to probe on virtex 4 at least; up must be set first */
//	if (probeaddr((uintptr)trp) < 0)
//		return -1;

	ethers[ether->ctlrno] = ether;
	ether->ctlr = ctlr = malloc(sizeof *ctlr);
	ctlr->regs = trp;
	ether->port = (uintptr)trp;
	trp->ie = 0;
	barriers();
	ether->interrupts = 0;		/* should be redundant */

	/* we can't tell, so assert what we hope for */
	ether->mbps = Defmbps;
	ether->fullduplex = ether->link = 1;

	/*
	 * whatever loaded this kernel (9load or another kernel)
	 * must have configured the ethernet in order to use it to load
	 * this kernel.  so try not to reset the hardware, which could
	 * force a seconds-long link negotiation.
	 */
	reset(ether);
//	resetsw(ether);
	init(ether);
	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->attach = attach;
	ether->closed = closed;
	ether->shutdown = temacshutdown;
	ether->transmit = temactransmit;
	ether->interrupt = temacintr;
	ether->ifstat = ifstat;

	ether->arg = ether;
	ether->promiscuous = promiscuous;
	ether->multicast = multicast;
	return 0;
}

void
etherlltemaclink(void)
{
	addethercard("lltemac", temacreset);
}

/*
 * Xilinx Local Link FIFOs for Temac, in pairs (rx and tx).
 */

/*
 * as of dma controller v2, keyhole operations are on ulongs,
 * but otherwise it's as if memmove were used.
 * addresses need not be word-aligned, though registers are.
 */
static void
fifocpy(void *vdest, void *vsrc, uint bytes, ulong flags)
{
	int words;
	ulong *dest, *src;
	uchar buf[ETHERMAXTU + 2*BY2WD];

	dest = vdest;
	src = vsrc;
	words = bytes / BY2WD;
	if (bytes % BY2WD != 0)
		words++;

	switch (flags & (Sinc | Dinc)) {
	case Sinc | Dinc:
		memmove(vdest, vsrc, bytes);
		break;
	case Sinc:				/* mem to register */
		memmove(buf, vsrc, bytes);	/* ensure src alignment */
		src = (ulong *)buf;
		while (words-- > 0)
			*dest = *src++;
		break;
	case Dinc:				/* register to mem */
		dest = (ulong *)buf;
		while (words-- > 0)
			*dest++ = *src;
		memmove(vdest, buf, bytes);	/* ensure dest alignment */
		break;
	case 0:				/* register-to-null or vice versa */
		while (words-- > 0)
			*dest = *src;
		break;
	}
}

/* returns true iff we whacked the ether */
static int
whackiferr(Ether *ether, int whack, int sts)
{
	Error *ep;

	/*
	 * these require a reset of the receive logic: Rpure, Rpore, Rpue.
	 * same for transmit logic: Tpoe, Tse.
	 */
	if (whack || sts & Errors) {
		iprint("fifo: errors:");
		for (ep = errs; ep < errs + nelem(errs); ep++)
			if (sts & ep->bit)
				iprint(" %s;", ep->msg);
		iprint("\n");
		whackether(ether);
		return 1;
	}
	return 0;
}

static int dmarecv(Ether *);

static void
fifointrset(Temacctlr *ctlr)
{
	frp->ier = ctlr->fifoier;
	barriers();
}

static void
enafifointr(Temacctlr *ctlr, int bits)
{
	ctlr->fifoier |= bits;
	fifointrset(ctlr);
}

static void
disfifointr(Temacctlr *ctlr, int bits)
{
	ctlr->fifoier &= ~bits;
	fifointrset(ctlr);
}

static long
getrdfo(void)
{
	ulong rdfo;

	rdfo = frp->rdfo;
	if (rdfo & ~Wordcnt) {
		iprint("fifo: impossible rdfo value\n");
		return -1;
	}
	return rdfo;
}

static void
dmarxdone(int)
{
	int whack, multi;
	long rdfo;
	Block *bp;
	Ether *ether;
	Etherpkt *pkt;
	Temacctlr *ctlr;

	ether = ethers[0];
	ctlr = ether->ctlr;

	ilock(ctlr);
	ether->dmarxintr++;
	bp = ctlr->rbp;
	ctlr->rbp = nil;			/* prevent accidents */
	if (ctlr->rxdma == 0 || bp == nil) {
		if (bp != nil)
			freeb(bp);
		if (ctlr->rxdma == 0)
			iprint("dmarxdone: no rx dma in progress\n");
		else
			iprint("dmarxdone: no block for rx dma just finished!\n");
		lightbitoff(Ledethinwait);
		enafifointr(ctlr, Rc);
		iunlock(ctlr);
		return;
	}
	ctlr->rxdma = 0;
	barriers();

	/*
	 * rx Step 2: packet is in Block, pass it upstream.
	 */
	/* could check for dma errors */
	pkt = (Etherpkt*)bp->rp;
	assert(pkt != nil);
	multi = pkt->d[0] & 1;
	if (multi)
		if(memcmp(pkt->d, ether->bcast, sizeof pkt->d) == 0)
			ether->bcasts++;
		else
			ether->mcasts++;

	etheriq(ether, bp, 1);
	lightbitoff(Ledethinwait);

	/*
	 * rx Step 3/0: if there's another packet in the rx fifo,
	 * start dma into a new Block, else reenable recv intrs.
	 */
	whack = 0;
	rdfo = getrdfo();
	if (rdfo < 0)
		whack = 1;		/* ether is buggered */
	else if (rdfo > 0)		/* more packets in rx fifo? */
		whack = dmarecv(ether);	/* if dma starts, disables Rc */
	else {
		if (frp->isr & Rc)
			wave('|');	/* isr Rc was set when fifo was empty */
		enafifointr(ctlr, Rc);
	}
	/* if this whacks the ctlr, all the intr enable bits will be set */
	whackiferr(ether, whack, frp->isr);
	iunlock(ctlr);
}

static void
discard(Ether *ether, unsigned ruplen)
{
	ulong null;

	/* discard the rx fifo's packet */
	fifocpy(&null, &frp->rdfd, ruplen, 0);
	ether->pktsdropped++;
}

/*
 * called when interrupt cause Rc is set (the rx fifo has at least one packet)
 * to begin dma to memory of the next packet in the input fifo.
 *
 * returns true iff the ether ctlr needs to be whacked.
 * may be called from interrupt routine; must be called with
 * interrupts masked.
 */
static int
dmarecv(Ether *ether)
{
	long rdfo;
	ulong len, ruplen;
	Block *bp;
	Temacctlr *ctlr;

	ctlr = ether->ctlr;
	if (ctlr->rxdma)
		return 0;  /* next rx dma interrupt should process this packet*/

	/* ignore frp->isr & Rc; just look at rx fifo occupancy */
	rdfo = getrdfo();
	if (rdfo < 0) {
		iprint("dmarecv: negative rdfo\n");
		return 1;		/* ether is buggered */
	}
	if (rdfo == 0)
		return 0;		/* no packets in the rx fifo */

	if (!(frp->isr & Rc))
		wave('@');  /* isr Rc wasn't set when fifo had stuff in it */

	/*
	 * We have at least one packet in the rx fifo.  Read the length
	 * of the first one, if not already known.
	 */
	if (ctlr->rlf >= 0)
		len = ctlr->rlf;	/* from a previous call */
	else {
		assert(frp != nil);
		/* read length word from rx fifo */
		len = frp->rlf;
		if (len & ~Bytecnt) {
			iprint("fifo: impossible rlf value\n");
			return 1;
		}
		if (len == 0) {
			iprint("fifo: rdfo %lud > 0 but rlf == 0\n", rdfo);
			return 1;
		}
		ctlr->rlf = len;	/* save in case dma is busy below */
	}

	ruplen = ROUNDUP(len, BY2WD);
	if (len > ETHERMAXTU) {
		iprint("fifo: jumbo pkt tossed\n");
		discard(ether, ruplen);
		return 0;
	}

	if (!dmaready)			/* too early, dma not really set up */
		return 0;
	bp = clallocb();
	if(bp == nil){
		iprint("fifo: no buffer for input pkt\n");
		discard(ether, ruplen);
		ether->soverflows++;
		return 0;
	}

	/*
	 * rx Step 1: dma from rx fifo into Block, turn off recv interrupts.
	 * wait for dmarxdone (interrupt) to pass the Block upstream.
	 */
	if (!dmastart(Chan0, bp->rp, &frp->rdfd, ruplen, Dinc, dmarxdone)) {
		/* should never get here */
		iprint("dmarecv: dmastart failed for Chan0\n");
		freeb(bp);
		enafifointr(ctlr, Rc);
		/* we'll try again next time we're called */
		return 0;
	}
	ctlr->rlf = -1;			/* we're committed now */
	lightbiton(Ledethinwait);
	bp->wp = bp->rp + len;
	assert(ctlr->rbp == nil);
	ctlr->rbp = bp;
	ctlr->rxdma = 1;
	barriers();
	/*
	 * we're waiting for dma and can't start dma of another
	 * incoming packet until the current dma is finished.
	 */
	disfifointr(ctlr, Rc);
	return 0;
}

void
whackether(Ether *ether)
{
	int s = splhi();

	iprint("resetting fifo+temac...");
	reset(ether);
	init(ether);
	iprint("\n");
	ether->resets++;
	splx(s);
}

/*
 * we've had trouble transmitting 60-byte packets and
 * 77-byte packets, so round up the length to make it even
 * and enforce a minimum of 64 bytes (ETHERMINTU+4).
 * rounding to a multiple of 4 (rather than 2) will make 1514
 * into 1516, which is a jumbo packet, so take care.
 * it's a bit sleazy, but this will just pick up a few junk
 * bytes beyond the packet for padding.
 */
static uint
padpktlen(uint len)
{
	len = ROUNDUP(len, BY2WD);
	if (len > ethermedium.maxtu)
		len = ethermedium.maxtu;
	return len;
}

int
fifointr(ulong bit)
{
	int r, whack, ic;
	ulong sts;
	Ether *ether;

	ether = ethers[0];
	if (ether == nil)
		return 0;			/* not for me */
	ether->interrupts++;
	r = 0;
	while ((sts = frp->isr) != 0) {
		ic = sts & (Rc | FifoTc);	/* interrupt causes */
		r |= ic;
		whack = 0;
		if (sts & Rc)
			whack = dmarecv(ether);
		else
			if (getrdfo() != 0)
				wave('~');  /* isr Rc off, yet fifo !empty */
		if (sts & FifoTc) {
			/*
			 * not much to do, really.  turn out the light and
			 * attempt another dma transfer anyway.
			 */
			lightbitoff(Ledethoutwait);
			temactransmit(ether);
		}
		frp->isr = ic;			/* extinguish intr sources */
		barriers();
		sts &= ~ic;
		if (sts)
			iprint("fifointr: sts %#lux\n", sts);
		r |= whackiferr(ether, whack, sts);
	}
	if (r) {
		intrack(bit);
		return 1;
	}
	return 0;
}

static void
fiforeset(Ether *ether)
{
	Temacctlr *ctlr;

	barriers();
	dma0init();	/* could be dma in progress, so shut that down */

	ctlr = ether->ctlr;
	ctlr->fifoier = 0;
	fifointrset(ctlr);

	/* should put a timeout on this and do a hard reset if it fails */
	frp->tdfr = Reset;	/* try it the graceful way first */
	frp->rdfr = Reset;
	barriers();
	while ((frp->isr & (Trc | Rrc)) != (Trc | Rrc))
		;
	frp->isr = Trc | Rrc;	/* reset the `reset done' bits */
	barriers();

if (0) {
	frp->llr = Reset;
	barriers();
	while ((frp->isr & (Trc | Rrc)) != (Trc | Rrc))
		;
}
}

void
fifoinit(Ether *ether)
{
	if (ethers[0] != nil)
		assert(ethers[0] == ether);
//	fiforeset(ether);
	frp->isr = frp->isr;		/* extinguish intr source */
	barriers();

	intrenable(Intllfifo, fifointr, "fifo");
	barriers();
	enafifointr(ether->ctlr, Rc | FifoTc | Rpure | Rpore | Rpue | Tpoe | Tse);
}

static void
tmoutreset(ulong bit)
{
	USED(bit);
	whackether(ethers[0]);
}

/*
 * tx Step 2: write frp->tlr, thus initiating ethernet transmission of
 * the Block just copied into the tx fifo, and free that Block.
 */
static void
dmatxdone(int)
{
	Block *bp;
	Ether *ether;
	Temacctlr *ctlr;

	ether = ethers[0];
	ctlr = ether->ctlr;

	ilock(ctlr);
	ether->dmatxintr++;
	ctlr->txdma = 0;

	/*
	 * start transmitting this packet from the output fifo.
	 * contrary to DS568 Table 5's description of TSE, it seems
	 * to be legal to write an odd value into tlf (unless the word
	 * `match' implies ±1), but it may be necessary to provide a
	 * padding byte in the fifo if you do.
	 */
	bp = ctlr->tbp;
	if (bp != nil) {
		ctlr->tbp = nil;
		frp->tlf = padpktlen(BLEN(bp));
		barriers();
		freeb(bp);
		ether->tbusy = 0;
	}

	dmatxstart(ether);		/* attempt another dma to tx fifo */
	iunlock(ctlr);
}

/*
 * if possible, start dma of the first packet of the output queue into
 * the tx fifo.
 *
 * must be called with ether->ctlr ilocked,
 * thus we cannot sleep nor qlock.
 *
 * output buffers are always misaligned, but that doesn't matter
 * as of v2 of the dma controller.
 */
static int
dmatxstart(Ether *ether)
{
	unsigned len, ruplen;
	Block *bp;
	Temacctlr *ctlr;

	if (ether == nil || ether->oq == nil || ether->tbusy)
		return 0;
	ctlr = ether->ctlr;
	if (ctlr->txdma)
		return 0;
	SET(len);
	while ((bp = qget(ether->oq)) != nil) {
		len = padpktlen(BLEN(bp));
		if (len > ETHERMAXTU) {
			iprint("fifo: dropping outgoing jumbo (%ud) pkt\n",
				len);
			freeb(bp);
		} else
			break;
	}
	if (bp == nil)
		return 0;
	if (!dmaready) {		/* too early? */
		iprint("dmatxstart: dma not ready\n");
		freeb(bp);
		disfifointr(ctlr, FifoTc);
		return 0;
	}

	if (len == 0)
		print("idiot sending zero-byte packet\n");
	ruplen = ROUNDUP(len, BY2WD);	/* must be multiple of 4 for dma */

	/*
	 * if there isn't enough room in the fifo for this
	 * packet, return and assume that the next transmit
	 * interrupt will resume transmission of it.
	 */
	if ((frp->tdfv & Wordcnt) < ruplen / BY2WD) {
		iprint("dmatxstart: no room in tx fifo\n");
		return 0;
	}

	/* tx Step 1: dma to tx fifo, wait for dma tx interrupt */
	lightbiton(Ledethoutwait);
	if (dmastart(Chan1, &frp->tdfd, bp->rp, ruplen, Sinc, dmatxdone)) {
		ether->tbusy = 1;
		ctlr->txdma = 1;
		barriers();
		/* Rc may be off if we got Rc intrs too early */
//		enafifointr(ctlr, FifoTc | Rc);
		ctlr->tbp = bp;			/* remember this block */
	} else
		iprint("dmatxstart: dmastart failed for Chan1\n");
	return 1;
}
