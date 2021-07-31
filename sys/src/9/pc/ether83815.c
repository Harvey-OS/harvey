/*
 * National Semiconductor DP83815
 *
 * Supports only internal PHY and has been tested on:
 *	Netgear FA311TX (using Netgear DS108 10/100 hub)
 * To do:
 *	check Ethernet address;
 *	test autonegotiation on 10 Mbit, and 100 Mbit full duplex;
 *	external PHY via MII (should be common code for MII);
 *	thresholds;
 *	ring sizing;
 *	physical link changes/disconnect;
 *	push initialisation back to attach.
 *
 * C H Forsyth, forsyth@vitanuova.com, 18th June 2001.
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

#define DEBUG		(0)
#define debug		if(DEBUG)print

enum {
	Nrde		= 64,
	Ntde		= 64,
};

#define Rbsz		ROUNDUP(sizeof(Etherpkt)+4, 4)

typedef struct Des {
	ulong	next;
	int	cmdsts;
	ulong	addr;
	Block*	bp;
} Des;

enum {	/* cmdsts */
	Own	= 1<<31,	/* set by data producer to hand to consumer */
	More	= 1<<30,	/* more of packet in next descriptor */
	Intr	= 1<<29,	/* interrupt when device is done with it */
	Supcrc	= 1<<28,	/* suppress crc on transmit */
	Inccrc	= 1<<28,	/* crc included on receive (always) */
	Ok	= 1<<27,	/* packet ok */
	Size	= 0xFFF,	/* packet size in bytes */

	/* transmit */
	Txa	= 1<<26,	/* transmission aborted */
	Tfu	= 1<<25,	/* transmit fifo underrun */
	Crs	= 1<<24,	/* carrier sense lost */
	Td	= 1<<23,	/* transmission deferred */
	Ed	= 1<<22,	/* excessive deferral */
	Owc	= 1<<21,	/* out of window collision */
	Ec	= 1<<20,	/* excessive collisions */
	/* 19-16 collision count */

	/* receive */
	Rxa	= 1<<26,	/* receive aborted (same as Rxo) */
	Rxo	= 1<<25,	/* receive overrun */
	Dest	= 3<<23,	/* destination class */
	  Drej=	0<<23,		/* packet was rejected */
	  Duni=	1<<23,		/* unicast */
	  Dmulti=	2<<23,		/* multicast */
	  Dbroad=	3<<23,		/* broadcast */
	Long = 1<<22,		/* too long packet received */
	Runt =  1<<21,		/* packet less than 64 bytes */
	Ise =	1<<20,		/* invalid symbol */
	Crce =	1<<19,		/* invalid crc */
	Fae =	1<<18,		/* frame alignment error */
	Lbp =	1<<17,		/* loopback packet */
	Col =	1<<16,		/* collision during receive */
};

enum {				/* PCI vendor & device IDs */
	Nat83815	= (0x0020<<16)|0x100B,
	SiS = 	0x1039,
	SiS900 =	(0x0900<<16)|SiS,
	SiS7016 =	(0x7016<<16)|SiS,

	SiS630bridge	= 0x0008,

	/* SiS 900 PCI revision codes */
	SiSrev630s =	0x81,
	SiSrev630e =	0x82,
	SiSrev630ea1 =	0x83,

	SiSeenodeaddr =	8,		/* short addr of SiS eeprom mac addr */
	SiS630eenodeaddr =	9,	/* likewise for the 630 */
	Nseenodeaddr =	6,		/* " for NS eeprom */
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;			/* (pcidev->did<<16)|pcidev->vid */

	ushort	srom[0xB+1];
	uchar	sromea[Eaddrlen];	/* MAC address */

	uchar	fd;			/* option or auto negotiation */

	int	mbps;

	Lock	lock;

	Des*	rdr;			/* receive descriptor ring */
	int	nrdr;			/* size of rdr */
	int	rdrx;			/* index into rdr */

	Lock	tlock;
	Des*	tdr;			/* transmit descriptor ring */
	int	ntdr;			/* size of tdr */
	int	tdrh;			/* host index into tdr */
	int	tdri;			/* interface index into tdr */
	int	ntq;			/* descriptors active */
	int	ntqmax;

	ulong	rxa;			/* receive statistics */
	ulong	rxo;
	ulong	rlong;
	ulong	runt;
	ulong	ise;
	ulong	crce;
	ulong	fae;
	ulong	lbp;
	ulong	col;
	ulong	rxsovr;
	ulong	rxorn;

	ulong	txa;			/* transmit statistics */
	ulong	tfu;
	ulong	crs;
	ulong	td;
	ulong	ed;
	ulong	owc;
	ulong	ec;
	ulong	txurn;

	ulong	dperr;			/* system errors */
	ulong	rmabt;
	ulong	rtabt;
	ulong	sserr;
	ulong	rxsover;
} Ctlr;

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

enum {
	/* registers (could memory map) */
	Rcr=	0x00,		/* command register */
	  Rst=		1<<8,
	  Rxr=		1<<5,	/* receiver reset */
	  Txr=		1<<4,	/* transmitter reset */
	  Rxd=		1<<3,	/* receiver disable */
	  Rxe=		1<<2,	/* receiver enable */
	  Txd=		1<<1,	/* transmitter disable */
	  Txe=		1<<0,	/* transmitter enable */
	Rcfg=	0x04,		/* configuration */
	  Lnksts=	1<<31,	/* link good */
	  Speed100=	1<<30,	/* 100 Mb/s link */
	  Fdup=		1<<29,	/* full duplex */
	  Pol=		1<<28,	/* polarity reversal (10baseT) */
	  Aneg_dn=	1<<27,	/* autonegotiation done */
	  Pint_acen=	1<<17,	/* PHY interrupt auto clear enable */
	  Pause_adv=	1<<16,	/* advertise pause during auto neg */
	  Paneg_ena=	1<<13,	/* auto negotiation enable */
	  Paneg_all=	7<<13,	/* auto negotiation enable 10/100 half & full */
	  Ext_phy=	1<<12,	/* enable MII for external PHY */
	  Phy_rst=	1<<10,	/* reset internal PHY */
	  Phy_dis=	1<<9,	/* disable internal PHY (eg, low power) */
	  Req_alg=	1<<7,	/* PCI bus request: set means less aggressive */
	  Sb=		1<<6,	/* single slot back-off not random */
	  Pow=		1<<5,	/* out of window timer selection */
	  Exd=		1<<4,	/* disable excessive deferral timer */
	  Pesel=	1<<3,	/* parity error algorithm selection */
	  Brom_dis=	1<<2,	/* disable boot rom interface */
	  Bem=		1<<0,	/* big-endian mode */
	Rmear=	0x08,		/* eeprom access */
	  Mdc=		1<<6,	/* MII mangement check */
	  Mddir=	1<<5,	/* MII management direction */
	  Mdio=		1<<4,	/* MII mangement data */
	  Eesel=	1<<3,	/* EEPROM chip select */
	  Eeclk=	1<<2,	/* EEPROM clock */
	  Eedo=		1<<1,	/* EEPROM data out (from chip) */
	  Eedi=		1<<0,	/* EEPROM data in (to chip) */
	Rptscr=	0x0C,		/* pci test control */
	Risr=	0x10,		/* interrupt status */
	  Txrcmp=	1<<25,	/* transmit reset complete */
	  Rxrcmp=	1<<24,	/* receiver reset complete */
	  Dperr=	1<<23,	/* detected parity error */
	  Sserr=	1<<22,	/* signalled system error */
	  Rmabt=	1<<21,	/* received master abort */
	  Rtabt=	1<<20,	/* received target abort */
	  Rxsovr=	1<<16,	/* RX status FIFO overrun */
	  Hiberr=	1<<15,	/* high bits error set (OR of 25-16) */
	  Phy=		1<<14,	/* PHY interrupt */
	  Pme=		1<<13,	/* power management event (wake online) */
	  Swi=		1<<12,	/* software interrupt */
	  Mib=		1<<11,	/* MIB service */
	  Txurn=	1<<10,	/* TX underrun */
	  Txidle=	1<<9,	/* TX idle */
	  Txerr=	1<<8,	/* TX packet error */
	  Txdesc=	1<<7,	/* TX descriptor (with Intr bit done) */
	  Txok=		1<<6,	/* TX ok */
	  Rxorn=	1<<5,	/* RX overrun */
	  Rxidle=	1<<4,	/* RX idle */
	  Rxearly=	1<<3,	/* RX early threshold */
	  Rxerr=	1<<2,	/* RX packet error */
	  Rxdesc=	1<<1,	/* RX descriptor (with Intr bit done) */
	  Rxok=		1<<0,	/* RX ok */
	Rimr=	0x14,		/* interrupt mask */
	Rier=	0x18,		/* interrupt enable */
	  Ie=		1<<0,	/* interrupt enable */
	Rtxdp=	0x20,		/* transmit descriptor pointer */
	Rtxcfg=	0x24,		/* transmit configuration */
	  Csi=		1<<31,	/* carrier sense ignore (needed for full duplex) */
	  Hbi=		1<<30,	/* heartbeat ignore (needed for full duplex) */
	  Atp=		1<<28,	/* automatic padding of runt packets */
	  Mxdma=	7<<20,	/* maximum dma transfer field */
	  Mxdma32=	4<<20,	/* 4x32-bit words (32 bytes) */
	  Mxdma64=	5<<20,	/* 8x32-bit words (64 bytes) */
	  Flth=		0x3F<<8,/* Tx fill threshold, units of 32 bytes (must be > Mxdma) */
	  Drth=		0x3F<<0,/* Tx drain threshold (units of 32 bytes) */
	  Flth128=	4<<8,	/* fill at 128 bytes */
	  Drth512=	16<<0,	/* drain at 512 bytes */
	Rrxdp=	0x30,		/* receive descriptor pointer */
	Rrxcfg=	0x34,		/* receive configuration */
	  Atx=		1<<28,	/* accept transmit packets (needed for full duplex) */
	  Rdrth=	0x1F<<1,/* Rx drain threshold (units of 32 bytes) */
	  Rdrth64=	2<<1,	/* drain at 64 bytes */
	Rccsr=	0x3C,		/* CLKRUN control/status */
	  Pmests=	1<<15,	/* PME status */
	Rwcsr=	0x40,		/* wake on lan control/status */
	Rpcr=	0x44,		/* pause control/status */
	Rrfcr=	0x48,		/* receive filter/match control */
	  Rfen=		1<<31,	/* receive filter enable */
	  Aab=		1<<30,	/* accept all broadcast */
	  Aam=		1<<29,	/* accept all multicast */
	  Aau=		1<<28,	/* accept all unicast */
	  Apm=		1<<27,	/* accept on perfect match */
	  Apat=		0xF<<23,/* accept on pattern match */
	  Aarp=		1<<22,	/* accept ARP */
	  Mhen=		1<<21,	/* multicast hash enable */
	  Uhen=		1<<20,	/* unicast hash enable */
	  Ulm=		1<<19,	/* U/L bit mask */
				/* bits 0-9 are rfaddr */
	Rrfdr=	0x4C,		/* receive filter/match data */
	Rbrar=	0x50,		/* boot rom address */
	Rbrdr=	0x54,		/* boot rom data */
	Rsrr=	0x58,		/* silicon revision */
	Rmibc=	0x5C,		/* MIB control */
				/* 60-78 MIB data */

	/* PHY registers */
	Rbmcr=	0x80,		/* basic mode configuration */
	  Reset=	1<<15,
	  Sel100=	1<<13,	/* select 100Mb/sec if no auto neg */
	  Anena=	1<<12,	/* auto negotiation enable */
	  Anrestart=	1<<9,	/* restart auto negotiation */
	  Selfdx=	1<<8,	/* select full duplex if no auto neg */
	Rbmsr=	0x84,		/* basic mode status */
	  Ancomp=	1<<5,	/* autonegotiation complete */
	Rphyidr1= 0x88,
	Rphyidr2= 0x8C,
	Ranar=	0x90,		/* autonegotiation advertisement */
	Ranlpar= 0x94,		/* autonegotiation link partner ability */
	Raner=	0x98,		/* autonegotiation expansion */
	Rannptr= 0x9C,		/* autonegotiation next page TX */
	Rphysts= 0xC0,		/* PHY status */
	Rmicr=	0xC4,		/* MII control */
	  Inten=	1<<1,	/* PHY interrupt enable */
	Rmisr=	0xC8,		/* MII status */
	Rfcscr=	0xD0,		/* false carrier sense counter */
	Rrecr=	0xD4,		/* receive error counter */
	Rpcsr=	0xD8,		/* 100Mb config/status */
	Rphycr=	0xE4,		/* PHY control */
	Rtbscr=	0xE8,		/* 10BaseT status/control */
};

/*
 * eeprom addresses
 * 	7 to 9 (16 bit words): mac address, shifted and reversed
 */

#define csr32r(c, r)	(inl((c)->port+(r)))
#define csr32w(c, r, l)	(outl((c)->port+(r), (ulong)(l)))
#define csr16r(c, r)	(ins((c)->port+(r)))
#define csr16w(c, r, l)	(outs((c)->port+(r), (ulong)(l)))

static void
dumpcregs(Ctlr *ctlr)
{
	int i;

	for(i=0; i<=0x5C; i+=4)
		print("%2.2ux %8.8lux\n", i, csr32r(ctlr, i));
}

static void
promiscuous(void* arg, int on)
{
	Ctlr *ctlr;
	ulong w;

	ctlr = ((Ether*)arg)->ctlr;
	ilock(&ctlr->lock);
	w = csr32r(ctlr, Rrfcr);
	if(on != ((w&Aau)!=0)){
		csr32w(ctlr, Rrfcr, w & ~Rfen);
		csr32w(ctlr, Rrfcr, Rfen | (w ^ Aau));
	}
	iunlock(&ctlr->lock);
}

static void
attach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->lock);
	if(0)
		dumpcregs(ctlr);
	csr32w(ctlr, Rcr, Rxe);
	iunlock(&ctlr->lock);
}

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *buf, *p;
	int i, l, len;

	ctlr = ether->ctlr;

	ether->crcs = ctlr->crce;
	ether->frames = ctlr->runt+ctlr->ise+ctlr->rlong+ctlr->fae;
	ether->buffs = ctlr->rxorn+ctlr->tfu;
	ether->overflows = ctlr->rxsovr;

	if(n == 0)
		return 0;

	p = malloc(READSTR);
	l = snprint(p, READSTR, "Rxa: %lud\n", ctlr->rxa);
	l += snprint(p+l, READSTR-l, "Rxo: %lud\n", ctlr->rxo);
	l += snprint(p+l, READSTR-l, "Rlong: %lud\n", ctlr->rlong);
	l += snprint(p+l, READSTR-l, "Runt: %lud\n", ctlr->runt);
	l += snprint(p+l, READSTR-l, "Ise: %lud\n", ctlr->ise);
	l += snprint(p+l, READSTR-l, "Fae: %lud\n", ctlr->fae);
	l += snprint(p+l, READSTR-l, "Lbp: %lud\n", ctlr->lbp);
	l += snprint(p+l, READSTR-l, "Tfu: %lud\n", ctlr->tfu);
	l += snprint(p+l, READSTR-l, "Txa: %lud\n", ctlr->txa);
	l += snprint(p+l, READSTR-l, "CRC Error: %lud\n", ctlr->crce);
	l += snprint(p+l, READSTR-l, "Collision Seen: %lud\n", ctlr->col);
	l += snprint(p+l, READSTR-l, "Frame Too Long: %lud\n", ctlr->rlong);
	l += snprint(p+l, READSTR-l, "Runt Frame: %lud\n", ctlr->runt);
	l += snprint(p+l, READSTR-l, "Rx Underflow Error: %lud\n", ctlr->rxorn);
	l += snprint(p+l, READSTR-l, "Tx Underrun: %lud\n", ctlr->txurn);
	l += snprint(p+l, READSTR-l, "Excessive Collisions: %lud\n", ctlr->ec);
	l += snprint(p+l, READSTR-l, "Late Collision: %lud\n", ctlr->owc);
	l += snprint(p+l, READSTR-l, "Loss of Carrier: %lud\n", ctlr->crs);
	l += snprint(p+l, READSTR-l, "Parity: %lud\n", ctlr->dperr);
	l += snprint(p+l, READSTR-l, "Aborts: %lud\n", ctlr->rmabt+ctlr->rtabt);
	l += snprint(p+l, READSTR-l, "RX Status overrun: %lud\n", ctlr->rxsover);
	snprint(p+l, READSTR-l, "ntqmax: %d\n", ctlr->ntqmax);
	ctlr->ntqmax = 0;
	buf = a;
	len = readstr(offset, buf, n, p);
	if(offset > l)
		offset -= l;
	else
		offset = 0;
	buf += len;
	n -= len;

	l = snprint(p, READSTR, "srom:");
	for(i = 0; i < nelem(ctlr->srom); i++){
		if(i && ((i & 0x0F) == 0))
			l += snprint(p+l, READSTR-l, "\n     ");
		l += snprint(p+l, READSTR-l, " %4.4uX", ctlr->srom[i]);
	}

	snprint(p+l, READSTR-l, "\n");
	len += readstr(offset, buf, n, p);
	free(p);

	return len;
}

static void
txstart(Ether* ether)
{
	Ctlr *ctlr;
	Block *bp;
	Des *des;
	int started;

	ctlr = ether->ctlr;
	started = 0;
	while(ctlr->ntq < ctlr->ntdr-1){
		bp = qget(ether->oq);
		if(bp == nil)
			break;
		des = &ctlr->tdr[ctlr->tdrh];
		des->bp = bp;
		des->addr = PADDR(bp->rp);
		ctlr->ntq++;
		coherence();
		des->cmdsts = Own | BLEN(bp);
		ctlr->tdrh = NEXT(ctlr->tdrh, ctlr->ntdr);
		started = 1;
	}
	if(started){
		coherence();
		csr32w(ctlr, Rcr, Txe);	/* prompt */
	}

	if(ctlr->ntq > ctlr->ntqmax)
		ctlr->ntqmax = ctlr->ntq;
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
txrxcfg(Ctlr *ctlr, int txdrth)
{
	ulong rx, tx;

	rx = csr32r(ctlr, Rrxcfg);
	tx = csr32r(ctlr, Rtxcfg);
	if(ctlr->fd){
		rx |= Atx;
		tx |= Csi | Hbi;
	}else{
		rx &= ~Atx;
		tx &= ~(Csi | Hbi);
	}
	tx &= ~(Mxdma|Drth|Flth);
	tx |= Mxdma64 | Flth128 | txdrth;
	csr32w(ctlr, Rtxcfg, tx);
	rx &= ~(Mxdma|Rdrth);
	rx |= Mxdma64 | Rdrth64;
	csr32w(ctlr, Rrxcfg, rx);
}

static void
interrupt(Ureg*, void* arg)
{
	int len, status, cmdsts, n;
	Ctlr *ctlr;
	Ether *ether;
	Des *des;
	Block *bp;

	ether = arg;
	ctlr = ether->ctlr;

	while((status = csr32r(ctlr, Risr)) != 0){

		status &= ~(Pme|Mib);

		if(status & Hiberr){
			if(status & Rxsovr)
				ctlr->rxsover++;
			if(status & Sserr)
				ctlr->sserr++;
			if(status & Dperr)
				ctlr->dperr++;
			if(status & Rmabt)
				ctlr->rmabt++;
			if(status & Rtabt)
				ctlr->rtabt++;
			status &= ~(Hiberr|Txrcmp|Rxrcmp|Rxsovr|Dperr|Sserr|Rmabt|Rtabt);
		}

		/* update link state */
		if(status&Phy){
			status &= ~Phy;
			csr32r(ctlr, Rcfg);
			n = csr32r(ctlr, Rcfg);
//			iprint("83815 phy %x %x\n", n, n&Lnksts);
			ether->link = (n&Lnksts) != 0;
		}

		/*
		 * Received packets.
		 */
		if(status & (Rxdesc|Rxok|Rxerr|Rxearly|Rxorn)){
			des = &ctlr->rdr[ctlr->rdrx];
			while((cmdsts = des->cmdsts) & Own){
				if((cmdsts&Ok) == 0){
					if(cmdsts & Rxa)
						ctlr->rxa++;
					if(cmdsts & Rxo)
						ctlr->rxo++;
					if(cmdsts & Long)
						ctlr->rlong++;
					if(cmdsts & Runt)
						ctlr->runt++;
					if(cmdsts & Ise)
						ctlr->ise++;
					if(cmdsts & Crce)
						ctlr->crce++;
					if(cmdsts & Fae)
						ctlr->fae++;
					if(cmdsts & Lbp)
						ctlr->lbp++;
					if(cmdsts & Col)
						ctlr->col++;
				}
				else if(bp = iallocb(Rbsz)){
					len = (cmdsts&Size)-4;
					if(len <= 0){
						debug("ns83815: packet len %d <=0\n", len);
						freeb(des->bp);
					}else{
						des->bp->wp = des->bp->rp+len;
						etheriq(ether, des->bp, 1);
					}
					des->bp = bp;
					des->addr = PADDR(bp->rp);
					coherence();
				}else{
					debug("ns83815: interrupt: iallocb for input buffer failed\n");
					des->bp->next = 0;
				}

				des->cmdsts = Rbsz;
				coherence();

				ctlr->rdrx = NEXT(ctlr->rdrx, ctlr->nrdr);
				des = &ctlr->rdr[ctlr->rdrx];
			}
			status &= ~(Rxdesc|Rxok|Rxerr|Rxearly|Rxorn);
		}

		/*
		 * Check the transmit side:
		 *	check for Transmit Underflow and Adjust
		 *	the threshold upwards;
		 *	free any transmitted buffers and try to
		 *	top-up the ring.
		 */
		if(status & Txurn){
			ctlr->txurn++;
			ilock(&ctlr->lock);
			/* change threshold */
			iunlock(&ctlr->lock);
			status &= ~(Txurn);
		}

		ilock(&ctlr->tlock);
		while(ctlr->ntq){
			des = &ctlr->tdr[ctlr->tdri];
			cmdsts = des->cmdsts;
			if(cmdsts & Own)
				break;

			if((cmdsts & Ok) == 0){
				if(cmdsts & Txa)
					ctlr->txa++;
				if(cmdsts & Tfu)
					ctlr->tfu++;
				if(cmdsts & Td)
					ctlr->td++;
				if(cmdsts & Ed)
					ctlr->ed++;
				if(cmdsts & Owc)
					ctlr->owc++;
				if(cmdsts & Ec)
					ctlr->ec++;
				ether->oerrs++;
			}

			freeb(des->bp);
			des->bp = nil;
			des->cmdsts = 0;

			ctlr->ntq--;
			ctlr->tdri = NEXT(ctlr->tdri, ctlr->ntdr);
		}
		txstart(ether);
		iunlock(&ctlr->tlock);

		status &= ~(Txurn|Txidle|Txerr|Txdesc|Txok);

		/*
		 * Anything left not catered for?
		 */
		if(status)
			print("#l%d: status %8.8uX\n", ether->ctlrno, status);
	}
}

static void
ctlrinit(Ether* ether)
{
	Ctlr *ctlr;
	Des *des, *last;

	ctlr = ether->ctlr;

	/*
	 * Allocate suitable aligned descriptors
	 * for the transmit and receive rings;
	 * initialise the receive ring;
	 * initialise the transmit ring;
	 * unmask interrupts and start the transmit side.
	 */
	des = xspanalloc((ctlr->nrdr+ctlr->ntdr)*sizeof(Des), 32, 0);
	ctlr->tdr = des;
	ctlr->rdr = des+ctlr->ntdr;

	last = nil;
	for(des = ctlr->rdr; des < &ctlr->rdr[ctlr->nrdr]; des++){
		des->bp = iallocb(Rbsz);
		if(des->bp == nil)
			error(Enomem);
		des->cmdsts = Rbsz;
		des->addr = PADDR(des->bp->rp);
		if(last != nil)
			last->next = PADDR(des);
		last = des;
	}
	ctlr->rdr[ctlr->nrdr-1].next = PADDR(ctlr->rdr);
	ctlr->rdrx = 0;
	csr32w(ctlr, Rrxdp, PADDR(ctlr->rdr));

	last = nil;
	for(des = ctlr->tdr; des < &ctlr->tdr[ctlr->ntdr]; des++){
		des->cmdsts = 0;
		des->bp = nil;
		des->addr = ~0;
		if(last != nil)
			last->next = PADDR(des);
		last = des;
	}
	ctlr->tdr[ctlr->ntdr-1].next = PADDR(ctlr->tdr);
	ctlr->tdrh = 0;
	ctlr->tdri = 0;
	csr32w(ctlr, Rtxdp, PADDR(ctlr->tdr));

	txrxcfg(ctlr, Drth512);

	csr32w(ctlr, Rimr, Dperr|Sserr|Rmabt|Rtabt|Rxsovr|Hiberr|Txurn|Txerr|
		Txdesc|Txok|Rxorn|Rxerr|Rxdesc|Rxok);	/* Phy|Pme|Mib */
	csr32w(ctlr, Rmicr, Inten);	/* enable phy interrupts */
	csr32r(ctlr, Risr);		/* clear status */
	csr32w(ctlr, Rier, Ie);
}

static void
eeclk(Ctlr *ctlr, int clk)
{
	csr32w(ctlr, Rmear, Eesel | clk);
	microdelay(2);
}

static void
eeidle(Ctlr *ctlr)
{
	int i;

	eeclk(ctlr, 0);
	eeclk(ctlr, Eeclk);
	for(i=0; i<25; i++){
		eeclk(ctlr, 0);
		eeclk(ctlr, Eeclk);
	}
	eeclk(ctlr, 0);
	csr32w(ctlr, Rmear, 0);
	microdelay(2);
}

static int
eegetw(Ctlr *ctlr, int a)
{
	int d, i, w, v;

	eeidle(ctlr);
	eeclk(ctlr, 0);
	eeclk(ctlr, Eeclk);
	d = 0x180 | a;
	for(i=0x400; i; i>>=1){
		v = (d & i) ? Eedi : 0;
		eeclk(ctlr, v);
		eeclk(ctlr, Eeclk|v);
	}
	eeclk(ctlr, 0);

	w = 0;
	for(i=0x8000; i; i >>= 1){
		eeclk(ctlr, Eeclk);
		if(csr32r(ctlr, Rmear) & Eedo)
			w |= i;
		microdelay(2);
		eeclk(ctlr, 0);
	}
	eeidle(ctlr);
	return w;
}

static void
resetctlr(Ctlr *ctlr)
{
	int i;

	csr32w(ctlr, Rcr, Rst);
	for(i=0;; i++){
		if(i > 100)
			panic("ns83815: soft reset did not complete");
		microdelay(250);
		if((csr32r(ctlr, Rcr) & Rst) == 0)
			break;
		delay(1);
	}
}

static void
shutdown(Ether* ether)
{
	Ctlr *ctlr = ether->ctlr;

print("ether83815 shutting down\n");
	csr32w(ctlr, Rcr, Rxd|Txd);	/* disable transceiver */
	resetctlr(ctlr);
}

static void
softreset(Ctlr* ctlr, int resetphys)
{
	int i, w;

	/*
	 * Soft-reset the controller
	 */
	resetctlr(ctlr);
	csr32w(ctlr, Rccsr, Pmests);
	csr32w(ctlr, Rccsr, 0);
	csr32w(ctlr, Rcfg, csr32r(ctlr, Rcfg) | Pint_acen);

	if(resetphys){
		/*
		 * Soft-reset the PHY
		 */
		csr32w(ctlr, Rbmcr, Reset);
		for(i=0;; i++){
			if(i > 100)
				panic("ns83815: PHY soft reset time out");
			if((csr32r(ctlr, Rbmcr) & Reset) == 0)
				break;
			delay(1);
		}
	}

	/*
	 * Initialisation values, in sequence (see 4.4 Recommended Registers Configuration)
	 */
	csr16w(ctlr, 0xCC, 0x0001);	/* PGSEL */
	csr16w(ctlr, 0xE4, 0x189C);	/* PMCCSR */
	csr16w(ctlr, 0xFC, 0x0000);	/* TSTDAT */
	csr16w(ctlr, 0xF4, 0x5040);	/* DSPCFG */
	csr16w(ctlr, 0xF8, 0x008C);	/* SDCFG */

	/*
	 * Auto negotiate
	 */
	w = csr16r(ctlr, Rbmsr);	/* clear latched bits */
	debug("anar: %4.4ux\n", csr16r(ctlr, Ranar));
	csr16w(ctlr, Rbmcr, Anena);
	if(csr16r(ctlr, Ranar) == 0 || (csr32r(ctlr, Rcfg) & Aneg_dn) == 0){
		csr16w(ctlr, Rbmcr, Anena|Anrestart);
		for(i=0;; i++){
			if(i > 3000){
				print("ns83815: auto neg timed out\n");
				break;
			}
			if((w = csr16r(ctlr, Rbmsr)) & Ancomp)
				break;
			delay(1);
		}
		debug("%d ms\n", i);
		w &= 0xFFFF;
		debug("bmsr: %4.4ux\n", w);
	}
	USED(w);
	debug("anar: %4.4ux\n", csr16r(ctlr, Ranar));
	debug("anlpar: %4.4ux\n", csr16r(ctlr, Ranlpar));
	debug("aner: %4.4ux\n", csr16r(ctlr, Raner));
	debug("physts: %4.4ux\n", csr16r(ctlr, Rphysts));
	debug("tbscr: %4.4ux\n", csr16r(ctlr, Rtbscr));
}

static int
media(Ether* ether)
{
	Ctlr* ctlr;
	ulong cfg;

	ctlr = ether->ctlr;
	cfg = csr32r(ctlr, Rcfg);
	ctlr->fd = (cfg & Fdup) != 0;
	ether->link = (cfg&Lnksts) != 0;
	return (cfg&(Lnksts|Speed100)) == Lnksts? 10: 100;
}

static char* mediatable[9] = {
	"10BASE-T",				/* TP */
	"10BASE-2",				/* BNC */
	"10BASE-5",				/* AUI */
	"100BASE-TX",
	"10BASE-TFD",
	"100BASE-TXFD",
	"100BASE-T4",
	"100BASE-FX",
	"100BASE-FXFD",
};

static int
is630(ulong id, Pcidev *p)
{
	if(id == SiS900)
		switch (p->rid) {
		case SiSrev630s:
		case SiSrev630e:
	  	case SiSrev630ea1:
			return 1;
		}
	return 0;
}

enum {
	MagicReg = 0x48,
	MagicRegSz = 1,
	Magicrden = 0x40,	/* read enable, apparently */
	Paddr=		0x70,	/* address port */
	Pdata=		0x71,	/* data port */
};

/* rcmos() originally from LANL's SiS 900 driver's rcmos() */
static int
sisrdcmos(Ctlr *ctlr)
{
	int i;
	unsigned reg;
	ulong port;
	Pcidev *p;

	debug("ns83815: SiS 630 rev. %ux reading mac address from cmos\n", ctlr->pcidev->rid);
	p = pcimatch(nil, SiS, SiS630bridge);
	if(p == nil) {
		print("ns83815: no SiS 630 rev. %ux bridge for mac addr\n",
			ctlr->pcidev->rid);
		return 0;
	}
	port = p->mem[0].bar & ~0x01;
	debug("ns83815: SiS 630 rev. %ux reading mac addr from cmos via bridge at port 0x%lux\n", ctlr->pcidev->rid, port);

	reg = pcicfgr8(p, MagicReg);
	pcicfgw8(p, MagicReg, reg|Magicrden);

	for (i = 0; i < Eaddrlen; i++) {
		outb(port+Paddr, SiS630eenodeaddr + i);
		ctlr->sromea[i] = inb(port+Pdata);
	}

	pcicfgw8(p, MagicReg, reg & ~Magicrden);
	return 1;
}

/*
 * If this is a SiS 630E chipset with an embedded SiS 900 controller,
 * we have to read the MAC address from the APC CMOS RAM. - sez freebsd.
 * However, CMOS *is* NVRAM normally.  See devrtc.c:440, memory.c:88.
 */
static void
sissrom(Ctlr *ctlr)
{
	union {
		uchar	eaddr[Eaddrlen];
		ushort	alignment;
	} ee;
	int i, off = SiSeenodeaddr, cnt = sizeof ee.eaddr / sizeof(short);
	ushort *shp = (ushort *)ee.eaddr;

	if(!is630(ctlr->id, ctlr->pcidev) || !sisrdcmos(ctlr)) {
		for (i = 0; i < cnt; i++)
			*shp++ = eegetw(ctlr, off++);
		memmove(ctlr->sromea, ee.eaddr, sizeof ctlr->sromea);
	}
}

static void
nssrom(Ctlr* ctlr)
{
	int i, j;

	for(i = 0; i < nelem(ctlr->srom); i++)
		ctlr->srom[i] = eegetw(ctlr, i);

	/*
	 * the MAC address is reversed, straddling word boundaries
	 */
	j = Nseenodeaddr*16 + 15;
	for(i=0; i<48; i++){
		ctlr->sromea[i>>3] |= ((ctlr->srom[j>>4] >> (15-(j&0xF))) & 1) << (i&7);
		j++;
	}
}

static void
srom(Ctlr* ctlr)
{
	memset(ctlr->sromea, 0, sizeof(ctlr->sromea));
	switch (ctlr->id) {
	case SiS900:
	case SiS7016:
		sissrom(ctlr);
		break;
	case Nat83815:
		nssrom(ctlr);
		break;
	default:
		print("ns83815: srom: unknown id 0x%ux\n", ctlr->id);
		break;
	}
}

static void
scanpci83815(void)
{
	Ctlr *ctlr;
	Pcidev *p;
	ulong id;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != 0)
			continue;
		id = (p->did<<16)|p->vid;
		switch(id){
		default:
			continue;

		case Nat83815:
			break;
		case SiS900:
			break;
		}

		/*
		 * bar[0] is the I/O port register address and
		 * bar[1] is the memory-mapped register address.
		 */
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = p->mem[0].bar & ~0x01;
		ctlr->pcidev = p;
		ctlr->id = id;

		if(ioalloc(ctlr->port, p->mem[0].size, 0, "ns83815") < 0){
			print("ns83815: port 0x%uX in use\n", ctlr->port);
			free(ctlr);
			continue;
		}

		softreset(ctlr, 0);
		srom(ctlr);

		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;
	}
}

/* multicast already on, don't need to do anything */
static void
multicast(void*, uchar*, int)
{
}

static int
reset(Ether* ether)
{
	Ctlr *ctlr;
	int i, x;
	ulong ctladdr;
	uchar ea[Eaddrlen];
	static int scandone;

	if(scandone == 0){
		scanpci83815();
		scandone = 1;
	}

	/*
	 * Any adapter matches if no ether->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(ether->port == 0 || ether->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	ether->ctlr = ctlr;
	ether->port = ctlr->port;
	ether->irq = ctlr->pcidev->intl;
	ether->tbdf = ctlr->pcidev->tbdf;

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to
	 * loading the station address in the hardware.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0)
		memmove(ether->ea, ctlr->sromea, Eaddrlen);
	for(i=0; i<Eaddrlen; i+=2){
		x = ether->ea[i] | (ether->ea[i+1]<<8);
		ctladdr = (ctlr->id == Nat83815? i: i<<15);
		csr32w(ctlr, Rrfcr, ctladdr);
		csr32w(ctlr, Rrfdr, x);
	}
	csr32w(ctlr, Rrfcr, Rfen|Apm|Aab|Aam);

	ether->mbps = media(ether);

	/*
	 * Look for a medium override in case there's no autonegotiation
	 * the autonegotiation fails.
	 */

	for(i = 0; i < ether->nopt; i++){
		if(cistrcmp(ether->opt[i], "FD") == 0){
			ctlr->fd = 1;
			continue;
		}
		for(x = 0; x < nelem(mediatable); x++){
			debug("compare <%s> <%s>\n", mediatable[x],
				ether->opt[i]);
			if(cistrcmp(mediatable[x], ether->opt[i]) == 0){
				if(x != 4 && x >= 3)
					ether->mbps = 100;
				else
					ether->mbps = 10;
				switch(x){
				default:
					ctlr->fd = 0;
					break;

				case 0x04:		/* 10BASE-TFD */
				case 0x05:		/* 100BASE-TXFD */
				case 0x08:		/* 100BASE-FXFD */
					ctlr->fd = 1;
					break;
				}
				break;
			}
		}
	}

	/*
	 * Initialise descriptor rings, ethernet address.
	 */
	ctlr->nrdr = Nrde;
	ctlr->ntdr = Ntde;
	pcisetbme(ctlr->pcidev);
	ctlrinit(ether);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;

	ether->arg = ether;
	ether->promiscuous = promiscuous;
	ether->multicast = multicast;
	ether->shutdown = shutdown;
	return 0;
}

void
ether83815link(void)
{
	addethercard("83815",  reset);
}
