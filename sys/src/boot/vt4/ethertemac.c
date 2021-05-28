/*
 * Xilinx Temacs Ethernet driver.
 * It uses the Local Link FIFOs.
 * There are two interfaces per Temacs controller.
 * Half-duplex is not supported by hardware.
 */
#include "include.h"

enum {
	/* fixed by hardware */
	Nifcs = 2,

	/* tunable parameters; see below for more */
	Nrde = 64,
	Ntde = 4,
};
enum {				/* directly-addressible registers' bits */
	/* raf register */
	Htrst	= 1<<0,		/* hard temac reset (both ifcs) */
	Mcstrej	= 1<<1,		/* reject received multicast dest addr */
	Bcstrej	= 1<<2,		/* reject received broadcast dest addr */

	/* is, ip, ie register */
	Hardacscmplt = 1<<0,	/* hard register access complete */
	Autoneg	= 1<<1,		/* auto-negotiation complete */
	Rxcmplt	= 1<<2,		/* receive complete */
	Rxrject	= 1<<3,		/* receive frame rejected */
	Rxfifoovr = 1<<4,	/* receive fifo overrun */
	Txcmplt	= 1<<5,		/* transmit complete */
	Rxdcmlock = 1<<6,	/* receive DCM lock (ready for use) */
	Mgtrdy	= 1<<7,		/* mgt ready (new in 1.01b)

	/* ctl register */
	Wen	= 1<<15,	/* write instead of read */

	/* ctl register address codes; select other registers */
	Rcw0	= 0x200,	/* receive configuration */
	Rcw1	= 0x240,
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

typedef struct Temacsw Temacsw;
typedef struct Temacregs Temacregs;
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
struct Temacsw {
	Temacregs *regs;
};

extern uchar mymac[Eaddrlen];

static Ether *ethers[1];	/* only first ether is connected to a fifo */
static Lock shreglck;		/* protects shared registers */

static void	transmit(Ether *ether);

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

	ilock(&shreglck);
	getready(trp);
	trp->ctl = code;
	coherence();

	getready(trp);
	val = trp->lsw;
	iunlock(&shreglck);
	return val;
}

static int
wrindir(Temacregs *trp, unsigned code, ulong val)
{
	ilock(&shreglck);
	getready(trp);
	trp->lsw = val;
	coherence();
	trp->ctl = Wen | code;
	coherence();

	getready(trp);
	iunlock(&shreglck);
	return 0;
}

static int
interrupt(ulong bit)
{
	int e, r, sts;
	Ether *ether;
	Temacsw *ctlr;

	r = 0;
	for (e = 0; e < MaxEther; e++) {
		ether = ethers[e];
		if (ether == nil)
			continue;
		ctlr = ether->ctlr;
		sts = ctlr->regs->is;
		if (sts)
			r = 1;
		ctlr->regs->is = sts;	/* extinguish intr source */
		coherence();
		sts &= ~(Rxcmplt | Txcmplt | Rxdcmlock | Mgtrdy);
		if (sts)
			iprint("ethertemac: sts %#ux\n", sts);
	}
	if (r)
		intrack(bit);
	return r;
}

static void
reset(Ether *ether)
{
	Temacsw *ctlr;
	Temacregs *trp;

	ctlr = ether->ctlr;
	trp = ctlr->regs;
	trp->ie = 0;
	coherence();
	/* don't use raf to reset: that resets both interfaces */
	wrindir(trp, Tc,   Rst);
	while (rdindir(trp, Tc) & Rst)
		;
	wrindir(trp, Rcw1, Rst);
	while (rdindir(trp, Rcw1) & Rst)
		;
	llfiforeset();
}

static void
attach(Ether *)
{
}

static void
transmit(Ether *ether)
{
	RingBuf *tb;

	if (ether->tbusy)
		return;
	tb = &ether->tb[ether->ti];
	if (tb->owner != Interface)
		return;
	llfifotransmit(tb->pkt, tb->len);
	coherence();
	tb->owner = Host;
	coherence();
	ether->ti = NEXT(ether->ti, ether->ntb);
	coherence();
}

static void
detach(Ether *ether)
{
	reset(ether);
}

int
temacreset(Ether* ether)
{
	int i;
	ulong ealo, eahi;
	uvlong ea;
	Temacsw *ctlr;
	Temacregs *trp;

	if ((unsigned)ether->ctlrno >= nelem(ethers) || ethers[ether->ctlrno])
		return -1;		/* already probed & found */
	trp = (Temacregs *)Temac + ether->ctlrno;
	if (probeaddr((uintptr)trp) < 0)
		return -1;

	ethers[ether->ctlrno] = ether;
	ether->ctlr = ctlr = malloc(sizeof *ctlr);
	ctlr->regs = trp;

	/*
	 * Determine media.
	 */
	ether->mbps = Defmbps;
//	ether->mbps = media(ether, 1);

	/*
	 * Initialise descriptor rings, ethernet address.
	 */
	ether->nrb = Nrde;
	ether->ntb = Ntde;
	ether->rb = malloc(Nrde * sizeof(RingBuf));
	ether->tb = malloc(Ntde * sizeof(RingBuf));
	ether->port = Temac;

	reset(ether);
	delay(1);

	llfifoinit(ether);

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
	wrindir(trp, Emmc, Defls);

/*	intrenable(Inttemac, interrupt); /* done by ether.c */
	trp->ie = Rxrject | Rxfifoovr;	/* just errors */
	coherence();

	wrindir(trp, Tc,   Tx);
	wrindir(trp, Rcw1, Rx);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->detach = detach;

	return 0;
}
