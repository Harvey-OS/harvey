/*
 * Realtek RTL8110/8168/8169 Gigabit Ethernet Controllers.
 * There are some magic register values used which are not described in
 * any datasheet or driver but seem to be necessary.
 * There are slight differences between the chips in the series so some
 * tweaks may be needed.
 *
 * we use l1 and l2 cache ops; data must reach ram for dma.
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

typedef struct Ctlr Ctlr;
typedef struct D D;			/* Transmit/Receive Descriptor */
typedef struct Dtcc Dtcc;

enum {
	Debug = 0,  /* beware: > 1 interferes with correct operation */
};

enum {					/* registers */
	Idr0		= 0x00,		/* MAC address */
	Mar0		= 0x08,		/* Multicast address */
	Dtccr		= 0x10,		/* Dump Tally Counter Command */
	Tnpds		= 0x20,		/* Transmit Normal Priority Descriptors */
	Thpds		= 0x28,		/* Transmit High Priority Descriptors */
	Flash		= 0x30,		/* Flash Memory Read/Write */
	Erbcr		= 0x34,		/* Early Receive Byte Count */
	Ersr		= 0x36,		/* Early Receive Status */
	Cr		= 0x37,		/* Command Register */
	Tppoll		= 0x38,		/* Transmit Priority Polling */
	Imr		= 0x3C,		/* Interrupt Mask */
	Isr		= 0x3E,		/* Interrupt Status */
	Tcr		= 0x40,		/* Transmit Configuration */
	Rcr		= 0x44,		/* Receive Configuration */
	Tctr		= 0x48,		/* Timer Count */
	Mpc		= 0x4C,		/* Missed Packet Counter */
	Cr9346		= 0x50,		/* 9346 Command Register */
	Config0		= 0x51,		/* Configuration Register 0 */
	Config1		= 0x52,		/* Configuration Register 1 */
	Config2		= 0x53,		/* Configuration Register 2 */
	Config3		= 0x54,		/* Configuration Register 3 */
	Config4		= 0x55,		/* Configuration Register 4 */
	Config5		= 0x56,		/* Configuration Register 5 */
	Timerint	= 0x58,		/* Timer Interrupt */
	Mulint		= 0x5C,		/* Multiple Interrupt Select */
	Phyar		= 0x60,		/* PHY Access */
	Tbicsr0		= 0x64,		/* TBI Control and Status */
	Tbianar		= 0x68,		/* TBI Auto-Negotiation Advertisment */
	Tbilpar		= 0x6A,		/* TBI Auto-Negotiation Link Partner */
	Phystatus	= 0x6C,		/* PHY Status */

	Rms		= 0xDA,		/* Receive Packet Maximum Size */
	Cplusc		= 0xE0,		/* C+ Command */
	Coal		= 0xE2,		/* Interrupt Mitigation (Coalesce) */
	Rdsar		= 0xE4,		/* Receive Descriptor Start Address */
	Etx		= 0xEC,		/* 8169: Early Tx Threshold; 32-byte units */
	Mtps		= 0xEC,		/* 8168: Maximum Transmit Packet Size */
};

enum {					/* Dtccr */
	Cmd		= 0x00000008,	/* Command */
};

enum {					/* Cr */
	Te		= 0x04,		/* Transmitter Enable */
	Re		= 0x08,		/* Receiver Enable */
	Rst		= 0x10,		/* Software Reset */
};

enum {					/* Tppoll */
	Fswint		= 0x01,		/* Forced Software Interrupt */
	Npq		= 0x40,		/* Normal Priority Queue polling */
	Hpq		= 0x80,		/* High Priority Queue polling */
};

enum {					/* Imr/Isr */
	Rok		= 0x0001,	/* Receive OK */
	Rer		= 0x0002,	/* Receive Error */
	Tok		= 0x0004,	/* Transmit OK */
	Ter		= 0x0008,	/* Transmit Error */
	Rdu		= 0x0010,	/* Receive Descriptor Unavailable */
	Punlc		= 0x0020,	/* Packet Underrun or Link Change */
	Fovw		= 0x0040,	/* Receive FIFO Overflow */
	Tdu		= 0x0080,	/* Transmit Descriptor Unavailable */
	Swint		= 0x0100,	/* Software Interrupt */
	Timeout		= 0x4000,	/* Timer */
	Serr		= 0x8000,	/* System Error */
};

enum {					/* Tcr */
	MtxdmaSHIFT	= 8,		/* Max. DMA Burst Size */
	MtxdmaMASK	= 0x00000700,
	Mtxdmaunlimited	= 0x00000700,
	Acrc		= 0x00010000,	/* Append CRC (not) */
	Lbk0		= 0x00020000,	/* Loopback Test 0 */
	Lbk1		= 0x00040000,	/* Loopback Test 1 */
	Ifg2		= 0x00080000,	/* Interframe Gap 2 */
	HwveridSHIFT	= 23,		/* Hardware Version ID */
	HwveridMASK	= 0x7C800000,
	Macv01		= 0x00000000,	/* RTL8169 */
	Macv02		= 0x00800000,	/* RTL8169S/8110S */
	Macv03		= 0x04000000,	/* RTL8169S/8110S */
	Macv04		= 0x10000000,	/* RTL8169SB/8110SB */
	Macv05		= 0x18000000,	/* RTL8169SC/8110SC */
	Macv07		= 0x24800000,	/* RTL8102e */
//	Macv8103e	= 0x24C00000,
	Macv25		= 0x28000000,	/* RTL8168D */
//	Macv8168dp	= 0x28800000,
//	Macv8168e	= 0x2C000000,
	Macv11		= 0x30000000,	/* RTL8168B/8111B */
	Macv14		= 0x30800000,	/* RTL8100E */
	Macv13		= 0x34000000,	/* RTL8101E */
	Macv07a		= 0x34800000,	/* RTL8102e */
	Macv12		= 0x38000000,	/* RTL8169B/8111B */
//	Macv8168spin3	= 0x38400000,
	Macv15		= 0x38800000,	/* RTL8100E */
	Macv12a		= 0x3c000000,	/* RTL8169C/8111C */
//	Macv19		= 0x3c000000,	/* dup Macv12a: RTL8111c-gr */
//	Macv8168cspin2	= 0x3c400000,
//	Macv8168cp	= 0x3c800000,
//	Macv8139	= 0x60000000,
//	Macv8139a	= 0x70000000,
//	Macv8139ag	= 0x70800000,
//	Macv8139b	= 0x78000000,
//	Macv8130	= 0x7C000000,
//	Macv8139c	= 0x74000000,
//	Macv8139d	= 0x74400000,
//	Macv8139cplus	= 0x74800000,
//	Macv8101	= 0x74c00000,
//	Macv8100	= 0x78800000,
//	Macv8169_8110sbl= 0x7cc00000,
//	Macv8169_8110sce= 0x98000000,
	Ifg0		= 0x01000000,	/* Interframe Gap 0 */
	Ifg1		= 0x02000000,	/* Interframe Gap 1 */
};

enum {					/* Rcr */
	Aap		= 0x00000001,	/* Accept All Packets */
	Apm		= 0x00000002,	/* Accept Physical Match */
	Am		= 0x00000004,	/* Accept Multicast */
	Ab		= 0x00000008,	/* Accept Broadcast */
	Ar		= 0x00000010,	/* Accept Runt */
	Aer		= 0x00000020,	/* Accept Error */
	Sel9356		= 0x00000040,	/* 9356 EEPROM used */
	MrxdmaSHIFT	= 8,		/* Max. DMA Burst Size */
	MrxdmaMASK	= 0x00000700,
	Mrxdmaunlimited	= 0x00000700,
	RxfthSHIFT	= 13,		/* Receive Buffer Length */
	RxfthMASK	= 0x0000E000,
	Rxfth256	= 0x00008000,
	Rxfthnone	= 0x0000E000,
	Rer8		= 0x00010000,	/* Accept Error Packets > 8 bytes */
	MulERINT	= 0x01000000,	/* Multiple Early Interrupt Select */
};

enum {					/* Cr9346 */
	Eedo		= 0x01,		/* */
	Eedi		= 0x02,		/* */
	Eesk		= 0x04,		/* */
	Eecs		= 0x08,		/* */
	Eem0		= 0x40,		/* Operating Mode */
	Eem1		= 0x80,
};

enum {					/* Phyar */
	DataMASK	= 0x0000FFFF,	/* 16-bit GMII/MII Register Data */
	DataSHIFT	= 0,
	RegaddrMASK	= 0x001F0000,	/* 5-bit GMII/MII Register Address */
	RegaddrSHIFT	= 16,
	Flag		= 0x80000000,	/* */
};

enum {					/* Phystatus */
	Fd		= 0x01,		/* Full Duplex */
	Linksts		= 0x02,		/* Link Status */
	Speed10		= 0x04,		/* */
	Speed100	= 0x08,		/* */
	Speed1000	= 0x10,		/* */
	Rxflow		= 0x20,		/* */
	Txflow		= 0x40,		/* */
	Entbi		= 0x80,		/* */
};

enum {					/* Cplusc */
	Init1		= 0x0001,	/* 8168 */
	Mulrw		= 0x0008,	/* PCI Multiple R/W Enable */
	Dac		= 0x0010,	/* PCI Dual Address Cycle Enable */
	Rxchksum	= 0x0020,	/* Receive Checksum Offload Enable */
	Rxvlan		= 0x0040,	/* Receive VLAN De-tagging Enable */
	Pktcntoff	= 0x0080,	/* 8168, 8101 */
	Endian		= 0x0200,	/* Endian Mode */
};

struct D {
	u32int	control;
	u32int	vlan;
	u32int	addrlo;
	u32int	addrhi;
};

enum {					/* Transmit Descriptor control */
	TxflMASK	= 0x0000FFFF,	/* Transmit Frame Length */
	TxflSHIFT	= 0,
	Tcps		= 0x00010000,	/* TCP Checksum Offload */
	Udpcs		= 0x00020000,	/* UDP Checksum Offload */
	Ipcs		= 0x00040000,	/* IP Checksum Offload */
	Lgsen		= 0x08000000,	/* TSO; WARNING: contains lark's vomit */
};

enum {					/* Receive Descriptor control */
	RxflMASK	= 0x00001FFF,	/* Receive Frame Length */
	Tcpf		= 0x00004000,	/* TCP Checksum Failure */
	Udpf		= 0x00008000,	/* UDP Checksum Failure */
	Ipf		= 0x00010000,	/* IP Checksum Failure */
	Pid0		= 0x00020000,	/* Protocol ID0 */
	Pid1		= 0x00040000,	/* Protocol ID1 */
	Crce		= 0x00080000,	/* CRC Error */
	Runt		= 0x00100000,	/* Runt Packet */
	Res		= 0x00200000,	/* Receive Error Summary */
	Rwt		= 0x00400000,	/* Receive Watchdog Timer Expired */
	Fovf		= 0x00800000,	/* FIFO Overflow */
	Bovf		= 0x01000000,	/* Buffer Overflow */
	Bar		= 0x02000000,	/* Broadcast Address Received */
	Pam		= 0x04000000,	/* Physical Address Matched */
	Mar		= 0x08000000,	/* Multicast Address Received */
};

enum {					/* General Descriptor control */
	Ls		= 0x10000000,	/* Last Segment Descriptor */
	Fs		= 0x20000000,	/* First Segment Descriptor */
	Eor		= 0x40000000,	/* End of Descriptor Ring */
	Own		= 0x80000000,	/* Ownership: belongs to hw */
};

/*
 */
enum {					/* Ring sizes  (<= 1024) */
	Ntd		= 1024,		/* Transmit Ring */
	/* at 1Gb/s, it only takes 12 ms. to fill a 1024-buffer ring */
	Nrd		= 1024,		/* Receive Ring */
	Nrb		= 4096,

	Mtu		= ETHERMAXTU,
	Mps		= ROUNDUP(ETHERMAXTU+4, 128),
//	Mps		= Mtu + 8 + 14,	/* if(mtu>ETHERMAXTU) */
};

struct Dtcc {
	u64int	txok;
	u64int	rxok;
	u64int	txer;
	u32int	rxer;
	u16int	misspkt;
	u16int	fae;
	u32int	tx1col;
	u32int	txmcol;
	u64int	rxokph;
	u64int	rxokbrd;
	u32int	rxokmu;
	u16int	txabt;
	u16int	txundrn;
};

enum {						/* Variants */
	Rtl8100e	= (0x8136<<16)|0x10EC,	/* RTL810[01]E: pci -e */
	Rtl8169c	= (0x0116<<16)|0x16EC,	/* RTL8169C+ (USR997902) */
	Rtl8169sc	= (0x8167<<16)|0x10EC,	/* RTL8169SC */
	Rtl8168b	= (0x8168<<16)|0x10EC,	/* RTL8168B: pci-e */
	Rtl8169		= (0x8169<<16)|0x10EC,	/* RTL8169 */
	/*
	 * trimslice is 10ec/8168 (8168b) Macv25 (8168D) but
	 * compulab says 8111dl.
	 *	oui 0x732 (aaeon) phyno 1, macv = 0x28000000 phyv = 0x0002
	 */
};

struct Ctlr {
	void*	nic;
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	Ether*	ether;			/* point back */
	int	active;

	QLock	alock;			/* attach */
	Lock	ilock;			/* init */
	int	init;			/*  */

	int	pciv;			/*  */
	int	macv;			/* MAC version */
	int	phyv;			/* PHY version */
	int	pcie;			/* flag: pci-express device? */

	uvlong	mchash;			/* multicast hash */

	Mii*	mii;

//	Lock	tlock;			/* transmit */
	Rendez	trendez;
	D*	td;			/* descriptor ring */
	Block**	tb;			/* transmit buffers */
	int	ntd;

	int	tdh;			/* head - producer index (host) */
	int	tdt;			/* tail - consumer index (NIC) */
	int	ntdfree;
	int	ntq;

	int	nrb;

//	Lock	rlock;			/* receive */
	Rendez	rrendez;
	D*	rd;			/* descriptor ring */
	Block**	rb;			/* receive buffers */
	int	nrd;

	int	rdh;			/* head - producer index (NIC) */
	int	rdt;			/* tail - consumer index (host) */
	int	nrdfree;

	Lock	reglock;
	int	tcr;			/* transmit configuration register */
	int	rcr;			/* receive configuration register */
	int	imr;
	int	isr;			/* sw copy for kprocs */

	QLock	slock;			/* statistics */
	Dtcc*	dtcc;
	uint	txdu;
	uint	tcpf;
	uint	udpf;
	uint	ipf;
	uint	fovf;
	uint	ierrs;
	uint	rer;
	uint	rdu;
	uint	punlc;
	uint	fovw;
	uint	mcast;
	uint	frag;			/* partial packets; rb was too small */
};

static Ctlr* rtl8169ctlrhead;
static Ctlr* rtl8169ctlrtail;

static Lock rblock;			/* free receive Blocks */
static Block* rbpool;

#define csr8r(c, r)	(*((uchar *) ((c)->nic)+(r)))
#define csr16r(c, r)	(*((u16int *)((c)->nic)+((r)/2)))
#define csr32p(c, r)	((u32int *)  ((c)->nic)+((r)/4))
#define csr32r(c, r)	(*csr32p(c, r))

#define csr8w(c, r, b)	(*((uchar *) ((c)->nic)+(r))     = (b), coherence())
#define csr16w(c, r, w)	(*((u16int *)((c)->nic)+((r)/2)) = (w), coherence())
#define csr32w(c, r, v)	(*csr32p(c, r) = (v), coherence())

static int
rtl8169miimir(Mii* mii, int pa, int ra)
{
	uint r;
	int timeo;
	Ctlr *ctlr;

	if(pa != 1)
		return -1;
	ctlr = mii->ctlr;
	r = (ra<<16) & RegaddrMASK;
	csr32w(ctlr, Phyar, r);
	delay(1);
	for(timeo = 0; timeo < 2000; timeo++){
		if((r = csr32r(ctlr, Phyar)) & Flag)
			break;
		microdelay(100);
	}
	if(!(r & Flag))
		return -1;

	return (r & DataMASK)>>DataSHIFT;
}

static int
rtl8169miimiw(Mii* mii, int pa, int ra, int data)
{
	uint r;
	int timeo;
	Ctlr *ctlr;

	if(pa != 1)
		return -1;
	ctlr = mii->ctlr;
	r = Flag|((ra<<16) & RegaddrMASK)|((data<<DataSHIFT) & DataMASK);
	csr32w(ctlr, Phyar, r);
	delay(1);
	for(timeo = 0; timeo < 2000; timeo++){
		if(!((r = csr32r(ctlr, Phyar)) & Flag))
			break;
		microdelay(100);
	}
	if(r & Flag)
		return -1;

	return 0;
}

static int
rtl8169mii(Ctlr* ctlr)
{
	MiiPhy *phy;

	/*
	 * Link management.
	 */
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->mir = rtl8169miimir;
	ctlr->mii->miw = rtl8169miimiw;
	ctlr->mii->ctlr = ctlr;

	/*
	 * Get rev number out of Phyidr2 so can config properly.
	 * There's probably more special stuff for Macv0[234] needed here.
	 */
	ilock(&ctlr->reglock);
	ctlr->phyv = rtl8169miimir(ctlr->mii, 1, Phyidr2) & 0x0F;
	if(ctlr->macv == Macv02){
		csr8w(ctlr, 0x82, 1);				/* magic */
		rtl8169miimiw(ctlr->mii, 1, 0x0B, 0x0000);	/* magic */
	}

	if(mii(ctlr->mii, (1<<1)) == 0 || (phy = ctlr->mii->curphy) == nil){
		iunlock(&ctlr->reglock);
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	print("rtl8169: oui %#ux phyno %d, macv = %#8.8ux phyv = %#4.4ux\n",
		phy->oui, phy->phyno, ctlr->macv, ctlr->phyv);

	miiane(ctlr->mii, ~0, ~0, ~0);
	iunlock(&ctlr->reglock);

	return 0;
}

static Block*
rballoc(void)
{
	Block *bp;

	ilock(&rblock);
	if((bp = rbpool) != nil){
		rbpool = bp->next;
		bp->next = nil;
		_xinc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&rblock);
	return bp;
}

static void
rbfree(Block *bp)
{
	bp->wp = bp->rp = bp->lim - Mps;
 	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(&rblock);
	bp->next = rbpool;
	rbpool = bp;
	iunlock(&rblock);
}

static void
rtl8169promiscuous(void* arg, int on)
{
	Ether *edev;
	Ctlr * ctlr;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(&ctlr->ilock);
	ilock(&ctlr->reglock);

	if(on)
		ctlr->rcr |= Aap;
	else
		ctlr->rcr &= ~Aap;
	csr32w(ctlr, Rcr, ctlr->rcr);
	iunlock(&ctlr->reglock);
	iunlock(&ctlr->ilock);
}

enum {
	/* everyone else uses 0x04c11db7, but they both produce the same crc */
	Etherpolybe = 0x04c11db6,
	Bytemask = (1<<8) - 1,
};

static ulong
ethercrcbe(uchar *addr, long len)
{
	int i, j;
	ulong c, crc, carry;

	crc = ~0UL;
	for (i = 0; i < len; i++) {
		c = addr[i];
		for (j = 0; j < 8; j++) {
			carry = ((crc & (1UL << 31))? 1: 0) ^ (c & 1);
			crc <<= 1;
			c >>= 1;
			if (carry)
				crc = (crc ^ Etherpolybe) | carry;
		}
	}
	return crc;
}

static ulong
swabl(ulong l)
{
	return l>>24 | (l>>8) & (Bytemask<<8) |
		(l<<8) & (Bytemask<<16) | l<<24;
}

static void
rtl8169multicast(void* ether, uchar *eaddr, int add)
{
	Ether *edev;
	Ctlr *ctlr;

	if (!add)
		return;	/* ok to keep receiving on old mcast addrs */

	edev = ether;
	ctlr = edev->ctlr;
	ilock(&ctlr->ilock);
	ilock(&ctlr->reglock);

	ctlr->mchash |= 1ULL << (ethercrcbe(eaddr, Eaddrlen) >> 26);

	ctlr->rcr |= Am;
	csr32w(ctlr, Rcr, ctlr->rcr);

	/* pci-e variants reverse the order of the hash byte registers */
	if (ctlr->pcie) {
		csr32w(ctlr, Mar0,   swabl(ctlr->mchash>>32));
		csr32w(ctlr, Mar0+4, swabl(ctlr->mchash));
	} else {
		csr32w(ctlr, Mar0,   ctlr->mchash);
		csr32w(ctlr, Mar0+4, ctlr->mchash>>32);
	}

	iunlock(&ctlr->reglock);
	iunlock(&ctlr->ilock);
}

static long
rtl8169ifstat(Ether* edev, void* a, long n, ulong offset)
{
	char *p;
	Ctlr *ctlr;
	Dtcc *dtcc;
	int i, l, r, timeo;

	ctlr = edev->ctlr;
	qlock(&ctlr->slock);

	p = nil;
	if(waserror()){
		qunlock(&ctlr->slock);
		free(p);
		nexterror();
	}

	/* copy hw statistics into ctlr->dtcc */
	dtcc = ctlr->dtcc;
	allcache->invse(dtcc, sizeof *dtcc);
	ilock(&ctlr->reglock);
	csr32w(ctlr, Dtccr+4, 0);
	csr32w(ctlr, Dtccr, PCIWADDR(dtcc)|Cmd);	/* initiate dma? */
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(ctlr, Dtccr) & Cmd))
			break;
		delay(1);
	}
	iunlock(&ctlr->reglock);
	if(csr32r(ctlr, Dtccr) & Cmd)
		error(Eio);

	edev->oerrs = dtcc->txer;
	edev->crcs = dtcc->rxer;
	edev->frames = dtcc->fae;
	edev->buffs = dtcc->misspkt;
	edev->overflows = ctlr->txdu + ctlr->rdu;

	if(n == 0){
		qunlock(&ctlr->slock);
		poperror();
		return 0;
	}

	if((p = malloc(READSTR)) == nil)
		error(Enomem);

	l = snprint(p, READSTR, "TxOk: %llud\n", dtcc->txok);
	l += snprint(p+l, READSTR-l, "RxOk: %llud\n", dtcc->rxok);
	l += snprint(p+l, READSTR-l, "TxEr: %llud\n", dtcc->txer);
	l += snprint(p+l, READSTR-l, "RxEr: %ud\n", dtcc->rxer);
	l += snprint(p+l, READSTR-l, "MissPkt: %ud\n", dtcc->misspkt);
	l += snprint(p+l, READSTR-l, "FAE: %ud\n", dtcc->fae);
	l += snprint(p+l, READSTR-l, "Tx1Col: %ud\n", dtcc->tx1col);
	l += snprint(p+l, READSTR-l, "TxMCol: %ud\n", dtcc->txmcol);
	l += snprint(p+l, READSTR-l, "RxOkPh: %llud\n", dtcc->rxokph);
	l += snprint(p+l, READSTR-l, "RxOkBrd: %llud\n", dtcc->rxokbrd);
	l += snprint(p+l, READSTR-l, "RxOkMu: %ud\n", dtcc->rxokmu);
	l += snprint(p+l, READSTR-l, "TxAbt: %ud\n", dtcc->txabt);
	l += snprint(p+l, READSTR-l, "TxUndrn: %ud\n", dtcc->txundrn);

	l += snprint(p+l, READSTR-l, "txdu: %ud\n", ctlr->txdu);
	l += snprint(p+l, READSTR-l, "tcpf: %ud\n", ctlr->tcpf);
	l += snprint(p+l, READSTR-l, "udpf: %ud\n", ctlr->udpf);
	l += snprint(p+l, READSTR-l, "ipf: %ud\n", ctlr->ipf);
	l += snprint(p+l, READSTR-l, "fovf: %ud\n", ctlr->fovf);
	l += snprint(p+l, READSTR-l, "ierrs: %ud\n", ctlr->ierrs);
	l += snprint(p+l, READSTR-l, "rer: %ud\n", ctlr->rer);
	l += snprint(p+l, READSTR-l, "rdu: %ud\n", ctlr->rdu);
	l += snprint(p+l, READSTR-l, "punlc: %ud\n", ctlr->punlc);
	l += snprint(p+l, READSTR-l, "fovw: %ud\n", ctlr->fovw);

	l += snprint(p+l, READSTR-l, "tcr: %#8.8ux\n", ctlr->tcr);
	l += snprint(p+l, READSTR-l, "rcr: %#8.8ux\n", ctlr->rcr);
	l += snprint(p+l, READSTR-l, "multicast: %ud\n", ctlr->mcast);

	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		l += snprint(p+l, READSTR, "phy:   ");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				l += snprint(p+l, READSTR-l, "\n       ");
			r = miimir(ctlr->mii, i);
			l += snprint(p+l, READSTR-l, " %4.4ux", r);
		}
		snprint(p+l, READSTR-l, "\n");
	}

	n = readstr(offset, a, n, p);

	qunlock(&ctlr->slock);
	poperror();
	free(p);

	return n;
}

static void
rtl8169halt(Ctlr* ctlr)
{
	ilock(&ctlr->reglock);
	csr32w(ctlr, Timerint, 0);
	csr8w(ctlr, Cr, 0);
	csr16w(ctlr, Imr, 0);
	csr16w(ctlr, Isr, ~0);
	iunlock(&ctlr->reglock);
}

static int
rtl8169reset(Ctlr* ctlr)
{
	u32int r;
	int timeo;

	/*
	 * Soft reset the controller.
	 */
	ilock(&ctlr->reglock);
	csr8w(ctlr, Cr, Rst);
	for(r = timeo = 0; timeo < 1000; timeo++){
		r = csr8r(ctlr, Cr);
		if(!(r & Rst))
			break;
		delay(1);
	}
	iunlock(&ctlr->reglock);

	rtl8169halt(ctlr);

	if(r & Rst)
		return -1;
	return 0;
}

static void
rtl8169shutdown(Ether *ether)
{
	rtl8169reset(ether->ctlr);
}

static int
rtl8169replenish(Ether *edev)
{
	int rdt;
	Block *bp;
	Ctlr *ctlr;
	D *d;

	ctlr = edev->ctlr;
	if (ctlr->nrd == 0) {
		iprint("rtl8169replenish: not yet initialised\n");
		return -1;
	}
	rdt = ctlr->rdt;
	assert(ctlr->rb);
	assert(ctlr->rd);
	while(NEXT(rdt, ctlr->nrd) != ctlr->rdh){
		d = &ctlr->rd[rdt];
		if (d == nil)
			panic("rtl8169replenish: nil ctlr->rd[%d]", rdt);
		if (d->control & Own) {	/* ctlr owns it? shouldn't happen */
			iprint("replenish: descriptor owned by hw\n");
			break;
		}
		if(ctlr->rb[rdt] == nil){
			bp = rballoc();
			if(bp == nil){
				iprint("rtl8169: no available buffers\n");
				break;
			}
			ctlr->rb[rdt] = bp;
			d->addrhi = 0;
			coherence();
			d->addrlo = PCIWADDR(bp->rp);
			coherence();
		} else
			iprint("8169: replenish: rx overrun\n");
		d->control = (d->control & ~RxflMASK) | Mps | Own;
		coherence();

		rdt = NEXT(rdt, ctlr->nrd);
		ctlr->nrdfree++;
	}
	ctlr->rdt = rdt;
	coherence();
	return 0;
}

static void
ckrderrs(Ctlr *ctlr, Block *bp, ulong control)
{
	if(control & Fovf)
		ctlr->fovf++;
	if(control & Mar)
		ctlr->mcast++;

	switch(control & (Pid1|Pid0)){
	case Pid0:
		if(control & Tcpf){
			iprint("8169: bad tcp checksum\n");
			ctlr->tcpf++;
			break;
		}
		bp->flag |= Btcpck;
		break;
	case Pid1:
		if(control & Udpf){
			iprint("8169: bad udp checksum\n");
			ctlr->udpf++;
			break;
		}
		bp->flag |= Budpck;
		break;
	case Pid1|Pid0:
		if(control & Ipf){
			iprint("8169: bad ip checksum\n");
			ctlr->ipf++;
			break;
		}
		bp->flag |= Bipck;
		break;
	}
}

static void
badpkt(Ether *edev, int rdh, ulong control)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	/* Res is only valid if Fs is set */
	if(control & Res)
		iprint("8169: rcv error; d->control %#.8lux\n", control);
	else if (control == 0) {		/* buggered? */
		if (edev->link)
			iprint("8169: rcv: d->control==0 (wtf?)\n");
	} else {
		ctlr->frag++;
		iprint("8169: rcv'd frag; d->control %#.8lux\n", control);
	}
	if (ctlr->rb[rdh])
		freeb(ctlr->rb[rdh]);
}

void
qpkt(Ether *edev, int rdh, ulong control)
{
	int len;
	Block *bp;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	len = (control & RxflMASK) - 4;
	if ((uint)len > Mps)
		if (len < 0)
			panic("8169: received pkt non-existent");
		else if (len > Mps)
			panic("8169: received pkt too big");
	bp = ctlr->rb[rdh];
	bp->wp = bp->rp + len;
	bp->next = nil;

	allcache->invse(bp->rp, len);	/* clear any stale cached packet */
	ckrderrs(ctlr, bp, control);
	etheriq(edev, bp, 1);

	if(Debug > 1)
		iprint("R%d ", len);
}

static int
pktstoread(void* v)
{
	Ctlr *ctlr = v;

	return ctlr->isr & (Fovw|Rdu|Rer|Rok) &&
		!(ctlr->rd[ctlr->rdh].control & Own);
}

static void
rproc(void* arg)
{
	int rdh;
	ulong control;
	Ctlr *ctlr;
	D *rd;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		/* wait for next interrupt */
		ilock(&ctlr->reglock);
		ctlr->imr |= Fovw|Rdu|Rer|Rok;
		csr16w(ctlr, Imr, ctlr->imr);
		iunlock(&ctlr->reglock);

		sleep(&ctlr->rrendez, pktstoread, ctlr);

		/* clear saved isr bits */
		ilock(&ctlr->reglock);
		ctlr->isr &= ~(Fovw|Rdu|Rer|Rok);
		iunlock(&ctlr->reglock);

		rdh = ctlr->rdh;
		for (rd = &ctlr->rd[rdh]; !(rd->control & Own);
		     rd = &ctlr->rd[rdh]){
			control = rd->control;
			if((control & (Fs|Ls|Res)) == (Fs|Ls))
				qpkt(edev, rdh, control);
			else
				badpkt(edev, rdh, control);
			ctlr->rb[rdh] = nil;
			coherence();
			rd->control &= Eor;
			coherence();

			ctlr->nrdfree--;
			rdh = NEXT(rdh, ctlr->nrd);
			if(ctlr->nrdfree < ctlr->nrd/2) {
				/* replenish reads ctlr->rdh */
				ctlr->rdh = rdh;
				rtl8169replenish(edev);
				/* if replenish called restart, rdh is reset */
				rdh = ctlr->rdh;
			}
		}
		ctlr->rdh = rdh;
	}
}

static int
pktstosend(void* v)
{
	Ether *edev = v;
	Ctlr *ctlr = edev->ctlr;

	return ctlr->isr & (Ter|Tok) &&
		!(ctlr->td[ctlr->tdh].control & Own) && edev->link;
}

static void
tproc(void* arg)
{
	int x, len;
	Block *bp;
	Ctlr *ctlr;
	D *d;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		/* wait for next interrupt */
		ilock(&ctlr->reglock);
		ctlr->imr |= Ter|Tok;
		csr16w(ctlr, Imr, ctlr->imr);
		iunlock(&ctlr->reglock);

		sleep(&ctlr->trendez, pktstosend, edev);

		/* clear saved isr bits */
		ilock(&ctlr->reglock);
		ctlr->isr &= ~(Ter|Tok);
		iunlock(&ctlr->reglock);

		/* reclaim transmitted Blocks */
		for(x = ctlr->tdh; ctlr->ntq > 0; x = NEXT(x, ctlr->ntd)){
			d = &ctlr->td[x];
			if(d == nil || d->control & Own)
				break;

			/*
			 * Free it up.
			 * Need to clean the descriptor here? Not really.
			 * Simple freeb for now (no chain and freeblist).
			 * Use ntq count for now.
			 */
			freeb(ctlr->tb[x]);
			ctlr->tb[x] = nil;
			d->control &= Eor;
			coherence();

			ctlr->ntq--;
		}
		ctlr->tdh = x;

		if (ctlr->ntq > 0)
			csr8w(ctlr, Tppoll, Npq); /* kick xmiter to keep it going */
		/* copy as much of my output q as possible into output ring */
		x = ctlr->tdt;
		while(ctlr->ntq < (ctlr->ntd-1)){
			if((bp = qget(edev->oq)) == nil)
				break;

			/* make sure the whole packet is in ram */
			len = BLEN(bp);
			allcache->wbse(bp->rp, len);

			d = &ctlr->td[x];
			assert(d);
			assert(!(d->control & Own));
			d->addrhi = 0;
			d->addrlo = PCIWADDR(bp->rp);
			ctlr->tb[x] = bp;
			coherence();
			d->control = (d->control & ~TxflMASK) |
				Own | Fs | Ls | len;
			coherence();

			if(Debug > 1)
				iprint("T%d ", len);

			x = NEXT(x, ctlr->ntd);
			ctlr->ntq++;

			ctlr->tdt = x;
			coherence();
			csr8w(ctlr, Tppoll, Npq);	/* kick xmiter again */
		}
		if(x != ctlr->tdt){		/* added new packet(s)? */
			ctlr->tdt = x;
			coherence();
			csr8w(ctlr, Tppoll, Npq);
		}
		else if(ctlr->ntq >= (ctlr->ntd-1))
			ctlr->txdu++;
	}
}

static int
rtl8169init(Ether* edev)
{
	u32int r;
	Ctlr *ctlr;
	ushort cplusc;

	ctlr = edev->ctlr;
	ilock(&ctlr->ilock);
	rtl8169reset(ctlr);

	ilock(&ctlr->reglock);
	switch(ctlr->pciv){
	case Rtl8169sc:
		csr8w(ctlr, Cr, 0);
		break;
	case Rtl8168b:
	case Rtl8169c:
		/* 8168b manual says set c+ reg first, then command */
		csr16w(ctlr, Cplusc, 0x2000);		/* magic */
		csr8w(ctlr, Cr, 0);
		break;
	}

	/*
	 * MAC Address is not settable on some (all?) chips.
	 * Must put chip into config register write enable mode.
	 */
	csr8w(ctlr, Cr9346, Eem1|Eem0);

	/*
	 * Transmitter.
	 */
	memset(ctlr->td, 0, sizeof(D)*ctlr->ntd);
	ctlr->tdh = ctlr->tdt = 0;
	ctlr->ntq = 0;
	ctlr->td[ctlr->ntd-1].control = Eor;

	/*
	 * Receiver.
	 * Need to do something here about the multicast filter.
	 */
	memset(ctlr->rd, 0, sizeof(D)*ctlr->nrd);
	ctlr->nrdfree = ctlr->rdh = ctlr->rdt = 0;
	ctlr->rd[ctlr->nrd-1].control = Eor;

	rtl8169replenish(edev);

	switch(ctlr->pciv){
	default:
		ctlr->rcr = Rxfthnone|Mrxdmaunlimited|Ab|Apm;
		break;
	case Rtl8168b:
	case Rtl8169c:
		ctlr->rcr = Rxfthnone|6<<MrxdmaSHIFT|Ab|Apm; /* DMA max 1024 */
		break;
	}

	/*
	 * Setting Mulrw in Cplusc disables the Tx/Rx DMA burst
	 * settings in Tcr/Rcr; the (1<<14) is magic.
	 */
	cplusc = csr16r(ctlr, Cplusc) & ~(1<<14);
	switch(ctlr->pciv){
	case Rtl8168b:
	case Rtl8169c:
		cplusc |= Pktcntoff | Init1;
		break;
	}
	cplusc |= /*Rxchksum|*/Mulrw;
	switch(ctlr->macv){
	default:
		panic("ether8169: unknown macv %#08ux for vid %#ux did %#ux",
			ctlr->macv, ctlr->pcidev->vid, ctlr->pcidev->did);
	case Macv01:
		break;
	case Macv02:
	case Macv03:
		cplusc |= 1<<14;			/* magic */
		break;
	case Macv05:
		/*
		 * This is interpreted from clearly bogus code
		 * in the manufacturer-supplied driver, it could
		 * be wrong. Untested.
		 */
		r = csr8r(ctlr, Config2) & 0x07;
		if(r == 0x01)				/* 66MHz PCI */
			csr32w(ctlr, 0x7C, 0x0007FFFF);	/* magic */
		else
			csr32w(ctlr, 0x7C, 0x0007FF00);	/* magic */
		pciclrmwi(ctlr->pcidev);
		break;
	case Macv13:
		/*
		 * This is interpreted from clearly bogus code
		 * in the manufacturer-supplied driver, it could
		 * be wrong. Untested.
		 */
		pcicfgw8(ctlr->pcidev, 0x68, 0x00);	/* magic */
		pcicfgw8(ctlr->pcidev, 0x69, 0x08);	/* magic */
		break;
	case Macv04:
	case Macv07:
	case Macv07a:
	case Macv11:
	case Macv12:
	case Macv12a:
	case Macv14:
	case Macv15:
	case Macv25:
		break;
	}

	/*
	 * Enable receiver/transmitter.
	 * Need to do this first or some of the settings below
	 * won't take.
	 */
	switch(ctlr->pciv){
	default:
		csr8w(ctlr, Cr, Te|Re);
		csr32w(ctlr, Tcr, Ifg1|Ifg0|Mtxdmaunlimited);
		csr32w(ctlr, Rcr, ctlr->rcr);
		break;
	case Rtl8169sc:
	case Rtl8168b:
		break;
	}
	ctlr->mchash = 0;
	csr32w(ctlr, Mar0,   0);
	csr32w(ctlr, Mar0+4, 0);

	/*
	 * Interrupts.
	 * Disable Tdu for now, the transmit routine will tidy.
	 * Tdu means the NIC ran out of descriptors to send (i.e., the
	 * output ring is empty), so it doesn't really need to ever be on.
	 *
	 * The timer runs at the PCI(-E) clock frequency, 125MHz for PCI-E,
	 * presumably 66MHz for PCI.  Thus the units for PCI-E controllers
	 * (e.g., 8168) are 8ns, and only the buggy 8168 seems to need to use
	 * timeouts to keep from stalling.
	 */
	csr32w(ctlr, Tctr, 0);
	/* Tok makes the whole system run faster */
	ctlr->imr = Serr|Fovw|Punlc|Rdu|Ter|Tok|Rer|Rok;
	switch(ctlr->pciv){
	case Rtl8169sc:
	case Rtl8168b:
		/* alleged workaround for rx fifo overflow on 8168[bd] */
		ctlr->imr &= ~Rdu;
		break;
	}
	csr16w(ctlr, Imr, ctlr->imr);

	/*
	 * Clear missed-packet counter;
	 * clear early transmit threshold value;
	 * set the descriptor ring base addresses;
	 * set the maximum receive packet size;
	 * no early-receive interrupts.
	 *
	 * note: the maximum rx size is a filter.  the size of the buffer
	 * in the descriptor ring is still honored.  we will toss >Mtu
	 * packets because they've been fragmented into multiple
	 * rx buffers.
	 */
	csr32w(ctlr, Mpc, 0);
	if (ctlr->pcie)
		csr8w(ctlr, Mtps, Mps / 128);
	else
		csr8w(ctlr, Etx, 0x3f);		/* max; no early transmission */
	csr32w(ctlr, Tnpds+4, 0);
	csr32w(ctlr, Tnpds, PCIWADDR(ctlr->td));
	csr32w(ctlr, Rdsar+4, 0);
	csr32w(ctlr, Rdsar, PCIWADDR(ctlr->rd));
	csr16w(ctlr, Rms, 2048);		/* was Mps; see above comment */
	r = csr16r(ctlr, Mulint) & 0xF000;	/* no early rx interrupts */
	csr16w(ctlr, Mulint, r);
	csr16w(ctlr, Cplusc, cplusc);
	csr16w(ctlr, Coal, 0);

	/*
	 * Set configuration.
	 */
	switch(ctlr->pciv){
	case Rtl8169sc:
		csr8w(ctlr, Cr, Te|Re);
		csr32w(ctlr, Tcr, Ifg1|Ifg0|Mtxdmaunlimited);
		csr32w(ctlr, Rcr, ctlr->rcr);
		break;
	case Rtl8168b:
	case Rtl8169c:
		csr16w(ctlr, Cplusc, 0x2000);		/* magic */
		csr8w(ctlr, Cr, Te|Re);
		csr32w(ctlr, Tcr, Ifg1|Ifg0|6<<MtxdmaSHIFT); /* DMA max 1024 */
		csr32w(ctlr, Rcr, ctlr->rcr);
		break;
	}
	ctlr->tcr = csr32r(ctlr, Tcr);
	csr8w(ctlr, Cr9346, 0);

	iunlock(&ctlr->reglock);
	iunlock(&ctlr->ilock);

//	rtl8169mii(ctlr);

	return 0;
}

static void
rtl8169attach(Ether* edev)
{
	int timeo, s, i;
	char name[KNAMELEN];
	Block *bp;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	s = splhi();
	qlock(&ctlr->alock);
	if(ctlr->init || waserror()) {
		qunlock(&ctlr->alock);
		splx(s);
		return;
	}
	ctlr->td = ucallocalign(sizeof(D)*Ntd, 256, 0);
	ctlr->tb = malloc(Ntd*sizeof(Block*));
	ctlr->ntd = Ntd;

	ctlr->rd = ucallocalign(sizeof(D)*Nrd, 256, 0);
	ctlr->rb = malloc(Nrd*sizeof(Block*));
	ctlr->nrd = Nrd;

	ctlr->dtcc = mallocalign(sizeof(Dtcc), 64, 0, 0);
	if(waserror()){
		free(ctlr->td);
		free(ctlr->tb);
		free(ctlr->rd);
		free(ctlr->rb);
		free(ctlr->dtcc);
		nexterror();
	}
	if(ctlr->td == nil || ctlr->tb == nil || ctlr->rd == nil ||
	   ctlr->rb == nil || ctlr->dtcc == nil)
		error(Enomem);

	/* allocate private receive-buffer pool */
	ctlr->nrb = Nrb;
	for(i = 0; i < Nrb; i++){
		if((bp = allocb(Mps)) == nil)
			error(Enomem);
		bp->free = rbfree;
		freeb(bp);
	}

	rtl8169init(edev);
	ctlr->init = 1;
	qunlock(&ctlr->alock);
	splx(s);
	poperror();				/* free */
	poperror();				/* qunlock */

	/* signal secondary cpus that l1 ptes are stable */
	l1ptstable.word = 1;
	allcache->wbse(&l1ptstable, sizeof l1ptstable);

	s = spllo();
	/* Don't wait long for link to be ready. */
	for(timeo = 0; timeo < 50 && miistatus(ctlr->mii) != 0; timeo++)
//		tsleep(&up->sleep, return0, 0, 100); /* fewer miistatus msgs */
		delay(100);

	while (!edev->link)
		tsleep(&up->sleep, return0, 0, 10);
	splx(s);

	snprint(name, KNAMELEN, "#l%drproc", edev->ctlrno);
	kproc(name, rproc, edev);

	snprint(name, KNAMELEN, "#l%dtproc", edev->ctlrno);
	kproc(name, tproc, edev);
}

/* call with ctlr->reglock held */
static void
rtl8169link(Ether* edev)
{
	uint r;
	int limit;
	Ctlr *ctlr;

	ctlr = edev->ctlr;

	if(!((r = csr8r(ctlr, Phystatus)) & Linksts)){
		if (edev->link) {
			edev->link = 0;
			csr8w(ctlr, Cr, Re);
			iprint("#l%d: link down\n", edev->ctlrno);
		}
		return;
	}
	if (edev->link == 0) {
		edev->link = 1;
		csr8w(ctlr, Cr, Te|Re);
		iprint("#l%d: link up\n", edev->ctlrno);
	}
	limit = 256*1024;
	if(r & Speed10){
		edev->mbps = 10;
		limit = 65*1024;
	} else if(r & Speed100)
		edev->mbps = 100;
	else if(r & Speed1000)
		edev->mbps = 1000;

	if(edev->oq != nil)
		qsetlimit(edev->oq, limit);
}

static void
rtl8169transmit(Ether* edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if (ctlr == nil || ctlr->ntd == 0) {
		iprint("rtl8169transmit: not yet initialised\n");
		return;
	}
	wakeup(&ctlr->trendez);
}

/*
 * the controller has lost its mind, so reset it.
 * call with ctlr->reglock held.
 */
static void
restart(Ether *edev, char *why)
{
	int i, s, del;
	Ctlr *ctlr;
	static int inrestart;
	static Lock rstrtlck;

	/* keep other cpus out */
	s = splhi();
	if (inrestart) {
		splx(s);
		return;
	}
	ilock(&rstrtlck);

	ctlr = edev->ctlr;
	if (ctlr == nil || !ctlr->init) {
		iunlock(&rstrtlck);
		splx(s);
		return;
	}

	if (Debug)
		iprint("#l%d: restart due to %s\n", edev->ctlrno, why);
	inrestart = 1;

	/* process any pkts in the rings */
	wakeup(&ctlr->rrendez);
	coherence();
	rtl8169transmit(edev);
	/* allow time to drain 1024-buffer ring */
	for (del = 0; del < 13 && ctlr->ntq > 0; del++)
		delay(1);			

	iunlock(&ctlr->reglock);
	rtl8169reset(ctlr);
	/* free any remaining unprocessed input buffers */
	for (i = 0; i < ctlr->nrd; i++) {
		freeb(ctlr->rb[i]);
		ctlr->rb[i] = nil;
	}
	rtl8169init(edev);
	ilock(&ctlr->reglock);

	rtl8169link(edev);
	rtl8169transmit(edev);		/* drain any output queue */
	wakeup(&ctlr->rrendez);

	inrestart = 0;

	iunlock(&rstrtlck);
	splx(s);
}

static ulong
rcvdiag(Ether *edev, ulong isr)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(!(isr & (Punlc|Rok)))
		ctlr->ierrs++;
	if(isr & Rer)
		ctlr->rer++;
	if(isr & Rdu)
		ctlr->rdu++;
	if(isr & Punlc)
		ctlr->punlc++;
	if(isr & Fovw)
		ctlr->fovw++;
	if (isr & (Fovw|Rdu|Rer)) {
		if (isr & ~(Tdu|Tok|Rok))		/* harmless */
			iprint("#l%d: isr %8.8#lux\n", edev->ctlrno, isr);
		restart(edev, "rcv error");
		isr = ~0;
	}
	return isr;
}

void
rtl8169interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	u32int isr;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(&ctlr->reglock);

	while((isr = csr16r(ctlr, Isr)) != 0 && isr != 0xFFFF){
		ctlr->isr |= isr;		/* merge bits for [rt]proc */
		csr16w(ctlr, Isr, isr);		/* dismiss? */
		if((isr & ctlr->imr) == 0)
			break;
		if(isr & Fovw && ctlr->pciv == Rtl8168b) {
			/*
			 * Fovw means we got behind; relatively common on 8168.
			 * this is a big hammer, but it gets things going again.
			 */
			ctlr->fovw++;
			restart(edev, "rx fifo overrun");
			break;
		}
		if(isr & (Fovw|Punlc|Rdu|Rer|Rok)) {
			ctlr->imr &= ~(Fovw|Rdu|Rer|Rok);
			csr16w(ctlr, Imr, ctlr->imr);
			wakeup(&ctlr->rrendez);

			if (isr & (Fovw|Punlc|Rdu|Rer)) {
				isr = rcvdiag(edev, isr);
				if (isr == ~0)
					break;		/* restarted */
			}
			isr &= ~(Fovw|Rdu|Rer|Rok);
		}
		if(isr & (Ter|Tok)){
			ctlr->imr &= ~(Ter|Tok);
			csr16w(ctlr, Imr, ctlr->imr);
			wakeup(&ctlr->trendez);

			if (isr & Ter)
				iprint("xmit err; isr %8.8#ux\n", isr);
			isr &= ~(Ter|Tok);
		}

		if(isr & Punlc){
			rtl8169link(edev);
			isr &= ~Punlc;
		}

		/*
		 * Some of the reserved bits get set sometimes...
		 */
		if(isr & (Serr|Fovw|Punlc|Rdu|Ter|Tok|Rer|Rok))
			panic("rtl8169interrupt: imr %#4.4ux isr %#4.4ux",
				csr16r(ctlr, Imr), isr);
	}
	if (edev->link && ctlr->ntq > 0)
		csr8w(ctlr, Tppoll, Npq); /* kick xmiter to keep it going */
	iunlock(&ctlr->reglock);
	/*
	 * extinguish pci-e controller interrupt source.
	 * should be done more cleanly.
	 */
	if (ctlr->pcie)
		pcieintrdone();
}

int
vetmacv(Ctlr *ctlr, uint *macv)
{
	*macv = csr32r(ctlr, Tcr) & HwveridMASK;
	switch(*macv){
	default:
		return -1;
	case Macv01:
	case Macv02:
	case Macv03:
	case Macv04:
	case Macv05:
	case Macv07:
	case Macv07a:
	case Macv11:
	case Macv12:
	case Macv12a:
	case Macv13:
	case Macv14:
	case Macv15:
	case Macv25:
		break;
	}
	return 0;
}

static void
rtl8169pci(void)
{
	Pcidev *p;
	Ctlr *ctlr;
	int i, pcie;
	uint macv, bar;
	void *mem;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		pcie = 0;
		switch(i = ((p->did<<16)|p->vid)){
		default:
			continue;
		case Rtl8100e:			/* RTL810[01]E ? */
		case Rtl8168b:			/* RTL8168B */
			pcie = 1;
			break;
		case Rtl8169c:			/* RTL8169C */
		case Rtl8169sc:			/* RTL8169SC */
		case Rtl8169:			/* RTL8169 */
			break;
		case (0xC107<<16)|0x1259:	/* Corega CG-LAPCIGT */
			i = Rtl8169;
			break;
		}

		bar = p->mem[2].bar & ~0x0F;
		assert(bar != 0);
		assert(!(p->mem[2].bar & Barioaddr));
		if(0) iprint("rtl8169: %d-bit register accesses\n",
			((p->mem[2].bar >> Barwidthshift) & Barwidthmask) ==
			 Barwidth32? 32: 64);
		mem = (void *)bar;	/* don't need to vmap on trimslice */
		if(mem == 0){
			print("rtl8169: can't map %#ux\n", bar);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil)
			error(Enomem);
		ctlr->nic = mem;
		ctlr->port = bar;
		ctlr->pcidev = p;
		ctlr->pciv = i;
		ctlr->pcie = pcie;

		if(vetmacv(ctlr, &macv) == -1){
			free(ctlr);
			print("rtl8169: unknown mac %.4ux %.8ux\n", p->did, macv);
			continue;
		}

		if(pcigetpms(p) > 0){
			pcisetpms(p, 0);

			for(i = 0; i < 6; i++)
				pcicfgw32(p, PciBAR0+i*4, p->mem[i].bar);
			pcicfgw8(p, PciINTL, p->intl);
			pcicfgw8(p, PciLTR, p->ltr);
			pcicfgw8(p, PciCLS, p->cls);
			pcicfgw16(p, PciPCR, p->pcr);
		}

		if(rtl8169reset(ctlr)){
			free(ctlr);
			continue;
		}

		/*
		 * Extract the chip hardware version,
		 * needed to configure each properly.
		 */
		ctlr->macv = macv;

		rtl8169mii(ctlr);
		pcisetbme(p);

		if(rtl8169ctlrhead != nil)
			rtl8169ctlrtail->next = ctlr;
		else
			rtl8169ctlrhead = ctlr;
		rtl8169ctlrtail = ctlr;
	}
}

static int
rtl8169pnp(Ether* edev)
{
	u32int r;
	Ctlr *ctlr;
	uchar ea[Eaddrlen];
	static int once;

	if(once == 0){
		once = 1;
		rtl8169pci();
	}

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = rtl8169ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	ctlr->ether = edev;
	edev->port = ctlr->port;
//	edev->irq = ctlr->pcidev->intl;	/* incorrect on trimslice */
	edev->irq = Pcieirq;		/* trimslice: non-msi pci-e intr */
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->maxmtu = Mtu;

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the device and set in edev->ea.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, edev->ea, Eaddrlen) == 0){
		r = csr32r(ctlr, Idr0);
		edev->ea[0] = r;
		edev->ea[1] = r>>8;
		edev->ea[2] = r>>16;
		edev->ea[3] = r>>24;
		r = csr32r(ctlr, Idr0+4);
		edev->ea[4] = r;
		edev->ea[5] = r>>8;
	}

	edev->attach = rtl8169attach;
	edev->transmit = rtl8169transmit;
	edev->interrupt = rtl8169interrupt;
	edev->ifstat = rtl8169ifstat;

	edev->arg = edev;
	edev->promiscuous = rtl8169promiscuous;
	edev->multicast = rtl8169multicast;
	edev->shutdown = rtl8169shutdown;

	ilock(&ctlr->reglock);
	rtl8169link(edev);
	iunlock(&ctlr->reglock);
	return 0;
}

void
ether8169link(void)
{
	addethercard("rtl8169", rtl8169pnp);
}
