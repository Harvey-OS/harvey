/*
 * Realtek RTL8110/8168/8169 Gigabit Ethernet Controllers.
 * This driver uses memory-mapped registers, not PC I/O ports.
 *
 * There are some magic register values used which are not described in
 * any datasheet or driver but seem to be necessary.
 * There are slight differences between the chips in the series so some
 * tweaks may be needed.
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

/* backward compatibility with pc i/o ports */
#define csr8r(regs, r)	(*((uchar *) (regs)+(r)))
#define csr16r(regs, r)	(*((ushort *)(regs)+((r)/2)))
#define csr32p(regs, r)	((ulong *)   (regs)+((r)/4))
#define csr32r(regs, r)	(*csr32p(regs, r))

#define csr8w(regs, r, b)	(*((uchar *) (regs)+(r))     = (b))
#define csr16w(regs, r, w)	(*((ushort *)(regs)+((r)/2)) = (w))
#define csr32w(regs, r, v)	(*csr32p(regs, r) = (v))

typedef struct Ctlr Ctlr;
typedef struct Desc Desc;		/* Transmit/Receive Descriptor */
typedef struct Dtcc Dtcc;

enum {
	Debug = 0,	/* beware: > 1 interferes with correct operation */
};

enum {					/* descriptor ring sizes (<= 1024) */
	Ntd		= 64,		/* Transmit Ring */
	/* at 1Gb/s, it can take only 12 ms. to fill a 1024-buffer ring */
	Nrd		= 512,		/* Receive Ring */

	Nrb		= 1024,		/* > Nrd */

	Mps		= ETHERMAXTU+32, /* +32 slop for vlans, etc. */
};

enum {						/* Variants */
	Rtl8100e	= (0x8136<<16)|0x10EC,	/* RTL810[01]E: pci -e */
	Rtl8169c	= (0x0116<<16)|0x16EC,	/* RTL8169C+ (USR997902) */
	Rtl8169sc	= (0x8167<<16)|0x10EC,	/* RTL8169SC */
	Rtl8168b	= (0x8168<<16)|0x10EC,	/* RTL8168B: pci-e crock */
	Rtl8169		= (0x8169<<16)|0x10EC,	/* RTL8169 */
	/*
	 * trimslice is 10ec/8168 (8168b) Macv25 (8168D) but
	 * compulab says 8111dl.
	 *	oui 0x732 (aaeon) phyno 1, macv = 0x28000000 phyv = 0x0002
	 */
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
	/*
	 * the commented out devices may work, i just haven't verified
	 * their mac numbers against reality yet.
	 */
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
//	Macv19		= 0x3c000000,	/* dup of Macv12a: RTL8111c-gr */
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
	Speed10		= 0x04,		/* 10Mb/s link speed */
	Speed100	= 0x08,		/* 100Mb/s */
	Speed1000	= 0x10,		/* 1Gb/s */
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

struct Desc {
	ulong	control;
	ulong	vlan;
	ulong	addrlo;
	ulong	addrhi;
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

struct Dtcc {
	uvlong	txok;
	uvlong	rxok;
	uvlong	txer;
	ulong	rxer;
	ushort	misspkt;
	ushort	fae;
	ulong	tx1col;
	ulong	txmcol;
	uvlong	rxokph;
	uvlong	rxokbrd;
	ulong	rxokmu;
	ushort	txabt;
	ushort	txundrn;
};

struct Ctlr {
	void*	nic;
	Pcidev*	pcidev;
	Ctlr*	next;
	Ether*	ether;			/* point back */
	Mii*	mii;
	int	port;
	int	active;

	Lock	reglock;
	int	tcr;			/* transmit configuration register */
	int	rcr;			/* receive configuration register */
	int	imr;
	int	isr;			/* sw copy for kprocs */
	int	intrignbits;		/* causes we don't care about */

	Rendez	trendez;
	Desc*	td;			/* descriptor ring */
	Block**	tb;			/* transmit buffers */
	uint	tdh;			/* head - producer index (host) */
	uint	tdt;			/* tail - consumer index (NIC) */
	int	ntdfree;
	int	ntq;

	Rendez	rrendez;
	Desc*	rd;			/* descriptor ring */
	Block**	rb;			/* receive buffers */
	uint	rdh;			/* head - producer index (NIC) */
	uint	rdt;			/* tail - consumer index (host) */
	int	nrdfree;
	Lock	replenlock;

	int	pciv;
	int	macv;			/* MAC version */
	int	phyv;			/* PHY version */
	int	pcie;			/* flag: pci-express device? */

	uvlong	mchash;			/* multicast hash */

	QLock	alock;			/* attach */
	Lock	ilock;			/* init */
	int	init;
	int	halted;

	Watermark wmrd;
	Watermark wmtd;

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

static void	restart(Ether *edev, char *why);

static int
rtl8169miimir(Mii* mii, int pa, int ra)
{
	uint r;
	int timeo;
	void *regs;
	Ctlr *ctlr;

	if(pa != 1)
		return -1;
	ctlr = mii->ctlr;
	regs = ctlr->nic;
	r = (ra<<16) & RegaddrMASK;
	csr32w(regs, Phyar, r);
	coherence();
	delay(1);
	for(timeo = 0; timeo < 2000; timeo++){
		if((r = csr32r(regs, Phyar)) & Flag)
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
	void *regs;
	Ctlr *ctlr;

	if(pa != 1)
		return -1;
	ctlr = mii->ctlr;
	regs = ctlr->nic;
	r = Flag|((ra<<16) & RegaddrMASK)|((data<<DataSHIFT) & DataMASK);
	csr32w(regs, Phyar, r);
	coherence();
	delay(1);
	for(timeo = 0; timeo < 2000; timeo++){
		if(!((r = csr32r(regs, Phyar)) & Flag))
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
		csr8w(ctlr->nic, 0x82, 1);			/* magic */
		rtl8169miimiw(ctlr->mii, 1, 0x0B, 0x0000);	/* magic */
	}

	if(mii(ctlr->mii, (1<<1)) == 0 || (phy = ctlr->mii->curphy) == nil){
		iunlock(&ctlr->reglock);
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
//	print("rtl8169: oui %#ux phyno %d, macv = %#8.8ux phyv = %#4.4ux\n",
//		phy->oui, phy->phyno, ctlr->macv, ctlr->phyv);
	USED(phy);

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
		ainc(&bp->ref);		/* prevent bp from being freed */
	}
	iunlock(&rblock);
	return bp;
}

/*
 * bp->rp must be a multiple of 8, which is assured because
 * BLOCKALIGN is CACHELINESZ is 32, thus bp->base is a multiple.
 */
static void
rbfree(Block *bp)
{
	bp->wp = bp->rp = (uchar *)ROUNDDN((ulong)bp->lim - Mps, BLOCKALIGN);
	assert(bp->rp >= bp->base);
	assert(((ulong)bp->rp & (BLOCKALIGN-1)) == 0);
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
	csr32w(ctlr->nic, Rcr, ctlr->rcr);
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
	void *regs;
	Ether *edev;
	Ctlr *ctlr;

	if (!add)
		return;	/* ok to keep receiving on old mcast addrs */

	edev = ether;
	ctlr = edev->ctlr;
	regs = ctlr->nic;
	ilock(&ctlr->ilock);
	ilock(&ctlr->reglock);

	ctlr->mchash |= 1ULL << (ethercrcbe(eaddr, Eaddrlen) >> 26);

	ctlr->rcr |= Am;
	csr32w(regs, Rcr, ctlr->rcr);

	/* pci-e variants reverse the order of the hash byte registers */
	if (ctlr->pcie) {
		csr32w(regs, Mar0,   swabl(ctlr->mchash>>32));
		csr32w(regs, Mar0+4, swabl(ctlr->mchash));
	} else {
		csr32w(regs, Mar0,   ctlr->mchash);
		csr32w(regs, Mar0+4, ctlr->mchash>>32);
	}

	iunlock(&ctlr->reglock);
	iunlock(&ctlr->ilock);
}

static int
fmtstat(char *p, int l, char *name, uvlong val)
{
	if (val == 0)
		return l;
	return l + snprint(p+l, READSTR-l, "%s: %llud\n", name, val);
}

static long
rtl8169ifstat(Ether* edev, void* a, long n, ulong offset)
{
	char *p, *s, *e;
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
	allcachesinvse(dtcc, sizeof *dtcc);
	ilock(&ctlr->reglock);
	csr32w(ctlr->nic, Dtccr+4, 0);
	coherence();
	csr32w(ctlr->nic, Dtccr, PCIWADDR(dtcc)|Cmd);	/* initiate dma? */
	coherence();
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(ctlr->nic, Dtccr) & Cmd))
			break;
		delay(1);
	}
	iunlock(&ctlr->reglock);
	if(csr32r(ctlr->nic, Dtccr) & Cmd)
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
	e = p + READSTR;

	l = fmtstat(p, 0, "TxOk", dtcc->txok);
	l = fmtstat(p, l, "RxOk", dtcc->rxok);
	l = fmtstat(p, l, "TxEr", dtcc->txer);
	l = fmtstat(p, l, "RxEr", dtcc->rxer);
	l = fmtstat(p, l, "MissPkt", dtcc->misspkt);
	l = fmtstat(p, l, "FAE", dtcc->fae);
	l = fmtstat(p, l, "Tx1Col", dtcc->tx1col);
	l = fmtstat(p, l, "TxMCol", dtcc->txmcol);
	l = fmtstat(p, l, "RxOkPh", dtcc->rxokph);
	l = fmtstat(p, l, "RxOkBrd", dtcc->rxokbrd);
	l = fmtstat(p, l, "RxOkMu", dtcc->rxokmu);
	l = fmtstat(p, l, "TxAbt", dtcc->txabt);
	l = fmtstat(p, l, "TxUndrn", dtcc->txundrn);

	l = fmtstat(p, l, "txdu", ctlr->txdu);
	l = fmtstat(p, l, "tcpf", ctlr->tcpf);
	l = fmtstat(p, l, "udpf", ctlr->udpf);
	l = fmtstat(p, l, "ipf", ctlr->ipf);
	l = fmtstat(p, l, "fovf", ctlr->fovf);
	l = fmtstat(p, l, "ierrs", ctlr->ierrs);
	l = fmtstat(p, l, "rer", ctlr->rer);
	l = fmtstat(p, l, "rdu", ctlr->rdu);
	l = fmtstat(p, l, "punlc", ctlr->punlc);
	l = fmtstat(p, l, "fovw", ctlr->fovw);

	l += snprint(p+l, READSTR-l, "tcr: %#8.8ux\n", ctlr->tcr);
	l += snprint(p+l, READSTR-l, "rcr: %#8.8ux\n", ctlr->rcr);
	l = fmtstat(p, l, "multicast", ctlr->mcast);

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
	s = p + l + 1;
	s = seprintmark(s, e, &ctlr->wmrd);
	s = seprintmark(s, e, &ctlr->wmtd);
	USED(s);

	n = readstr(offset, a, n, p);

	qunlock(&ctlr->slock);
	poperror();
	free(p);

	return n;
}

static void
rtl8169halt(Ctlr* ctlr)
{
	void *regs;

	if (ctlr->halted)
		return;		/* don't try to talk to it if it's halted */
	regs = ctlr->nic;
	ilock(&ctlr->reglock);
	csr32w(regs, Timerint, 0);
	csr8w(regs, Cr, 0);
	csr16w(regs, Imr, 0);
	csr16w(regs, Isr, ~0);
	iunlock(&ctlr->reglock);
	ctlr->halted = 1;
}

static int
rtl8169reset(Ctlr* ctlr)
{
	ulong r;
	int timeo;
	void *regs;

	/*
	 * Soft reset the controller.
	 */
	regs = ctlr->nic;
	ilock(&ctlr->reglock);
	csr8w(regs, Cr, Rst);
	coherence();
	delay(1);
	for(r = timeo = 0; timeo < 1000; timeo++){
		r = csr8r(regs, Cr);
		if(!(r & Rst))
			break;
		delay(1);
	}
	iunlock(&ctlr->reglock);

	rtl8169halt(ctlr);

	if(r & Rst) {
		iprint("8169: ctlr didn't reset\n");
		return -1;
	}
	delay(1);	/* hold it in reset, hope it settles its brains */
	return 0;
}

static void
rtl8169shutdown(Ether *ether)
{
	rtl8169reset(ether->ctlr);
}

static int
rtl8169replenish(Ctlr *ctlr)
{
	uint rdt;
	Block *bp;
	Block **rbp;
	Desc *rd;

	rdt = ctlr->rdt;
	while(NEXT(rdt, Nrd) != ctlr->rdh){
		rd = &ctlr->rd[rdt];
		if (rd == nil)
			panic("rtl8169replenish: nil ctlr->rd[%d]", rdt);
		if (rd->control & Own) {  /* ctlr owns it? shouldn't happen */
			iprint("rtl8169replenish: descriptor owned by hw\n");
			break;
		}
		rbp = &ctlr->rb[rdt];
		if(*rbp == nil){
			bp = rballoc();
			if(bp == nil){
				iprint("rtl8169replenish: no available buffers\n");
				break;
			}
			/* qpkt() invalidates the buffer's cache after dma */
			rd->addrhi = 0;
			rd->addrlo = PCIWADDR(bp->rp);
			coherence();
			*rbp = bp;
		} else
			iprint("rtl8169replenish: rx overrun\n");
		rd->control = (rd->control & ~RxflMASK) | Mps | Own;
		rdt = NEXT(rdt, Nrd);
		ctlr->nrdfree++;
	}
	coherence();
	ctlr->rdt = rdt;
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
		} else
			bp->flag |= Btcpck;
		break;
	case Pid1:
		if(control & Udpf){
			iprint("8169: bad udp checksum\n");
			ctlr->udpf++;
		} else
			bp->flag |= Budpck;
		break;
	case Pid1|Pid0:
		if(control & Ipf){
			iprint("8169: bad ip checksum\n");
			ctlr->ipf++;
		} else
			bp->flag |= Bipck;
		break;
	}
}

static ulong
badpkt(Ether *edev, int rdh, ulong control)
{
	int link;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	/* Res is only valid if Fs is set */
	if((control & (Fs|Res)) == (Fs|Res))
		iprint("8169: rcv error; d->control %#.8lux\n", control);
	else if ((control & ~Eor) == 0) {
		ilock(&ctlr->reglock);
		link = edev->link;
		restart(edev, "read desc. marked filled but not "
			"yet given back to hw & control==0");
		iprint("... rdh %d link %d\n", rdh, link);
		iunlock(&ctlr->reglock);
		return ~0;
	} else if ((control & (Fs|Ls)) != (Fs|Ls)) {
		ctlr->frag++;
		iprint("8169: rcv'd frag; d->control %#.8lux\n", control);
	} else
		iprint("8169: rcv confused; d->control %#.8lux\n", control);
	freeb(ctlr->rb[rdh]);
	return 0;
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
	if (bp == nil) {
		iprint("8169: nil Block* at ctlr->rb[%d]\n", rdh);
		return;
	}
	/*
	 * invalidate cache for buffer after dma filled it, in case the
	 * caches have prefetched old (pre-dma) data in them.
	 */
	allcachesinvse(bp->rp, len);
	bp->wp = bp->rp + len;
	bp->next = nil;

	ckrderrs(ctlr, bp, control);
	etheriq(edev, bp, 1);

	if(Debug > 1)
		iprint("R%d ", len);
}

static int
pktstoread(void* v)
{
	Ctlr *ctlr = v;

	return ctlr->isr & ((Fovw|Rdu|Rer|Rok) & ~ctlr->intrignbits) &&
//		NEXT(ctlr->rdt, Nrd) != ctlr->rdh &&
		!(ctlr->rd[ctlr->rdh].control & Own);
}

static void
rproc(void* arg)
{
	uint rdh, passed;
	ulong control;
	Ctlr *ctlr;
	Desc *rd;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		/* wait for next interrupt */
		ilock(&ctlr->reglock);
		ctlr->imr |= (Fovw|Rdu|Rer|Rok) & ~ctlr->intrignbits;
		csr16w(ctlr->nic, Imr, ctlr->imr);
		iunlock(&ctlr->reglock);

		sleep(&ctlr->rrendez, pktstoread, ctlr);

		/* clear saved isr bits */
		ilock(&ctlr->reglock);
		ctlr->isr &= ~(Fovw|Rdu|Rer|Rok);
		iunlock(&ctlr->reglock);

		passed = 0;
		while (passed < Nrd - 64) {
			rdh = ctlr->rdh;
			/* if ring is empty, rdh's Own bit might be invalid */
//			if (NEXT(ctlr->rdt, Nrd) == rdh)
//				break;
			rd = &ctlr->rd[rdh];
			control = rd->control;
			if (control & Own)
				break;
			if((control & (Fs|Ls|Res)) == (Fs|Ls)) {
				qpkt(edev, rdh, control);
				passed++;
			} else if (badpkt(edev, rdh, control) == ~0)
				break;	/* ctlr was reset; start again */

			/* reuse this descriptor */
			ilock(&ctlr->replenlock);
			ctlr->rb[rdh] = nil;
			rd->control &= Eor;	/* zero unless end of ring */
			ctlr->rdh = NEXT(rdh, Nrd);
			ctlr->nrdfree--;
			rtl8169replenish(ctlr);
			iunlock(&ctlr->replenlock);
		}
		if (passed > 0)
			/* note how many rds had full buffers */
			notemark(&ctlr->wmrd, passed);
	}
}

static int
kickxmit(Ctlr *ctlr)			/* return 1 if xmiter kicked */
{
	if (ctlr->ntq > 0) {
		coherence();
		csr8w(ctlr->nic, Tppoll, Npq);	/* kick xmiter again */
		coherence();
		return 1;
	}
	return 0;
}

static int
newtdt(Ctlr *ctlr, int x)
{
	ctlr->tdt = x;
	return kickxmit(ctlr);
}

static void
reclaimtxblks(Ctlr *ctlr)
{
	uint x;
	Desc *td;

	/* reclaim transmitted Blocks */
	for(x = ctlr->tdh; ctlr->ntq > 0; x = NEXT(x, Ntd)){
		td = &ctlr->td[x];
		if(td == nil || td->control & Own)
			break;
		freeb(ctlr->tb[x]);	/* not freeblist() */
		ctlr->tb[x] = nil;
		td->control &= Eor;
		ctlr->ntq--;
	}
	coherence();
	ctlr->tdh = x;
}

static int
pktstosend(void* v)
{
	Ether *edev = v;
	Ctlr *ctlr = edev->ctlr;

	return ctlr->isr & (Ter|Tok) && !(ctlr->td[ctlr->tdh].control & Own) &&
		edev->link;
}

static void
tproc(void* arg)
{
	uint tdt, kicked;
	Block *bp;
	Ctlr *ctlr;
	Desc *td;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		/* wait for next interrupt */
		ilock(&ctlr->reglock);
		ctlr->imr |= Ter|Tok;
		csr16w(ctlr->nic, Imr, ctlr->imr);
		iunlock(&ctlr->reglock);

		sleep(&ctlr->trendez, pktstosend, edev);

		/* clear saved isr bits */
		ilock(&ctlr->reglock);
		ctlr->isr &= ~(Ter|Tok);
		iunlock(&ctlr->reglock);

		reclaimtxblks(ctlr);
		kickxmit(ctlr);
		kicked = 0;

		/* copy as much of my output q as possible into output ring */
		tdt = ctlr->tdt;
		while(ctlr->ntq < Ntd-1){
			if((bp = qget(edev->oq)) == nil)
				break;

			/* devether has ensured that whole packet is in ram */
			td = &ctlr->td[tdt];
			// td->control & Own should be false
			td->addrhi = 0;
			td->addrlo = PCIWADDR(bp->rp);
			ctlr->tb[tdt] = bp;
			coherence();
			td->control = (td->control & ~TxflMASK) |
				Own | Fs | Ls | BLEN(bp);
			if(Debug > 1)
				iprint("T%ld ", BLEN(bp));

			/* note size of queue of tds awaiting transmission */
			notemark(&ctlr->wmtd, (tdt + Ntd - ctlr->tdh) % Ntd);
			ctlr->ntq++;
			tdt = NEXT(tdt, Ntd);
			if (!kicked)
				kicked = newtdt(ctlr, tdt);
		}
		if(tdt != ctlr->tdt)		/* added new packet(s)? */
			newtdt(ctlr, tdt);
		else if(ctlr->ntq >= Ntd - 1)
			ctlr->txdu++;
	}
}

/* called with ilock and reglock held */
static ushort
macmagic(Ctlr *ctlr)
{
	ushort cplusc;
	ulong r;

	/*
	 * Setting Mulrw in Cplusc disables the Tx/Rx DMA burst
	 * settings in Tcr/Rcr; the (1<<14) is magic.
	 */
	cplusc = csr16r(ctlr->nic, Cplusc) & ~(1<<14);
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
		r = csr8r(ctlr->nic, Config2) & 0x07;
		if(r == 0x01)				/* 66MHz PCI */
			csr32w(ctlr->nic, 0x7C, 0x0007FFFF);	/* magic */
		else
			csr32w(ctlr->nic, 0x7C, 0x0007FF00);	/* magic */
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
	return cplusc;
}

/* called with ilock and reglock held */
static void
ringsinit(Ctlr *ctlr)
{
	/*
	 * Transmitter.
	 */
	memset(ctlr->td, 0, sizeof(Desc)*Ntd);
	ctlr->tdh = ctlr->tdt = 0;
	ctlr->ntq = 0;
	ctlr->td[Ntd-1].control = Eor;

	/*
	 * Receiver.
	 * Need to do something here about the multicast filter.
	 */
	memset(ctlr->rd, 0, sizeof(Desc)*Nrd);
	ctlr->nrdfree = ctlr->rdh = ctlr->rdt = 0;
	ctlr->rd[Nrd-1].control = Eor;

	rtl8169replenish(ctlr);
}

/* called with ilock and reglock held */
static void
enablerxtx(Ctlr *ctlr)
{
	void *regs;

	regs = ctlr->nic;
	switch(ctlr->pciv){
	default:
		csr8w(regs, Cr, Te|Re);
		csr32w(regs, Tcr, Ifg1|Ifg0|Mtxdmaunlimited);
		csr32w(regs, Rcr, ctlr->rcr);
		coherence();
		break;
	case Rtl8169sc:
	case Rtl8168b:
		break;
	}
	ctlr->mchash = 0;
	csr32w(regs, Mar0,   0);
	csr32w(regs, Mar0+4, 0);
}

static int
setintrbits(Ctlr *ctlr)
{
	switch(ctlr->pciv){
	case Rtl8169sc:
	case Rtl8168b:
		/* alleged workaround for rx fifo overflow on 8168[bd] */
		return Rdu;
	default:
		return 0;
	}
}

/* called with ilock and reglock held */
static void
cfgintrs(Ctlr *ctlr, ushort cplusc)
{
	ulong r;
	void *regs;

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
	regs = ctlr->nic;
	csr32w(regs, Tctr, 0);
	ctlr->intrignbits = setintrbits(ctlr);
	/* Tok makes the whole system run faster */
	ctlr->imr = (Serr|Fovw|Punlc|Rdu|Rer|Rok|Tdu|Ter|Tok) &
		~ctlr->intrignbits;
	csr16w(regs, Imr, ctlr->imr);

	/*
	 * Clear missed-packet counter;
	 * clear early transmit threshold value;
	 * set the descriptor ring base addresses;
	 * set the maximum receive packet size;
	 * no early-receive interrupts.
	 *
	 * note: the maximum rx size is a filter.  the size of the buffer
	 * in the descriptor ring is still honored.  we will toss >ETHERMAXTU
	 * packets because they've been fragmented into multiple rx buffers.
	 */
	csr32w(regs, Mpc, 0);
	if (ctlr->pcie)
		csr8w(regs, Mtps, ROUNDUP(Mps, 128) / 128);
	else
		csr8w(regs, Etx, 0x3f);		/* max; no early transmission */
	csr32w(regs, Tnpds+4, 0);
	csr32w(regs, Tnpds, PCIWADDR(ctlr->td));
	csr32w(regs, Rdsar+4, 0);
	csr32w(regs, Rdsar, PCIWADDR(ctlr->rd));
	csr16w(regs, Rms, 2048);		/* was Mps; see above comment */
	r = csr16r(regs, Mulint) & 0xF000;	/* no early rx interrupts */
	csr16w(regs, Mulint, r);
	csr16w(regs, Cplusc, cplusc);
	csr16w(regs, Coal, 0);
}

static int
rtl8169init(Ether* edev)
{
	Ctlr *ctlr;
	ushort cplusc;
	void *regs;

	ctlr = edev->ctlr;
	regs = ctlr->nic;
	ilock(&ctlr->ilock);
	rtl8169reset(ctlr);

	ilock(&ctlr->reglock);
	switch(ctlr->pciv){
	case Rtl8168b:
	case Rtl8169c:
		/* 8168b manual says set c+ reg first, then command */
		csr16w(regs, Cplusc, 0x2000);		/* magic */
		/* fall through */
	case Rtl8169sc:
		csr8w(regs, Cr, 0);
		break;
	}

	/*
	 * MAC Address is not settable on some (all?) chips.
	 * Must put chip into config register write enable mode.
	 */
	csr8w(regs, Cr9346, Eem1|Eem0);

	ringsinit(ctlr);

	switch(ctlr->pciv){
	default:
		ctlr->rcr = Rxfthnone|Mrxdmaunlimited|Ab|Apm;
		break;
	case Rtl8168b:
	case Rtl8169c:
		ctlr->rcr = Rxfthnone|6<<MrxdmaSHIFT|Ab|Apm; /* DMA max 1024 */
		break;
	}

	cplusc = macmagic(ctlr);
	/*
	 * Enable receiver/transmitter.
	 * Need to do this first or some of the settings below
	 * won't take.
	 */
	enablerxtx(ctlr);
	cfgintrs(ctlr, cplusc);

	/*
	 * Set configuration.
	 */
	switch(ctlr->pciv){
	case Rtl8169sc:
		csr8w(regs, Cr, Te|Re);
		csr32w(regs, Tcr, Ifg1|Ifg0|Mtxdmaunlimited);
		csr32w(regs, Rcr, ctlr->rcr);
		break;
	case Rtl8168b:
	case Rtl8169c:
		csr16w(regs, Cplusc, 0x2000);		/* magic */
		csr8w(regs, Cr, Te|Re);
		csr32w(regs, Tcr, Ifg1|Ifg0|6<<MtxdmaSHIFT); /* DMA max 1024 */
		csr32w(regs, Rcr, ctlr->rcr);
		break;
	}
	ctlr->tcr = csr32r(regs, Tcr);
	csr8w(regs, Cr9346, 0);
	ctlr->halted = 0;

	iunlock(&ctlr->reglock);
	iunlock(&ctlr->ilock);
	/* we used to call rtl8169mii here, but it was called in rtl8169pci */
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
	ctlr->td = ucallocalign(sizeof(Desc)*Ntd, 256, 0);
	ctlr->tb = malloc(Ntd*sizeof(Block*));

	ctlr->rd = ucallocalign(sizeof(Desc)*Nrd, 256, 0);
	ctlr->rb = malloc(Nrd*sizeof(Block*));

	/*
	 * round up so that cache ops on dtcc only operate on dtcc.
	 */
	ctlr->dtcc = mallocalign(ROUNDUP(sizeof(Dtcc), CACHELINESZ), 64, 0, 0);
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

	/*
	 * allocate private receive-buffer pool.  round up so that cache ops
	 * on the buffer only operate on the buffer.  we must be in a process
	 * context (up != nil) for *allocb().
	 */
	for(i = 0; i < Nrb; i++){
		if((bp = allocb(ROUNDUP(Mps, CACHELINESZ))) == nil)
			error(Enomem);
		bp->free = rbfree;
		freeb(bp);
	}

	rtl8169init(edev);			/* enables rx & tx */
	initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
	initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");
	ctlr->init = 1;
	qunlock(&ctlr->alock);
	splx(s);
	poperror();				/* free */
	poperror();				/* qunlock */

	s = spllo();
	if (miistatus(ctlr->mii) != 0) {
		iprint("#l%d: mii wait...", edev->ctlrno);
		delay(1);
	}
	/* This can take seconds.  Don't wait long for link to be ready. */
	for(timeo = 0; timeo < 100 && miistatus(ctlr->mii) != 0; timeo++)
		delay(10);

	if (!edev->link) {
		iprint("#l%d: link wait...", edev->ctlrno);
		delay(1);
		for(timeo = 0; timeo < 500 && !edev->link; timeo++)
			tsleep(&up->sleep, return0, 0, 10);
		/* maybe it's unplugged; proceed */
	}

	/*
	 * signal other cpus that cpu0's l1 ptes are stable and kernel mappings
	 * are in their final configuration.  this may seem an odd place
	 * for it, but we just allocated uncached storage, thus completing
	 * the kernel mappings.  once another cpu starts scheduling,
	 * genrandom can grind away on it for a while.
	 */
	signall1ptstable();

	if (!edev->link) {
		for(timeo = 0; timeo < 2500 && !edev->link; timeo++)
			tsleep(&up->sleep, return0, 0, 10);
		if (!edev->link)
			iprint("#l%d: link still down; proceeding\n",
				edev->ctlrno);
	}
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
	uint r, ombps;
	int limit;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(!((r = csr8r(ctlr->nic, Phystatus)) & Linksts)){
		if (edev->link) {
			edev->link = 0;
			csr8w(ctlr->nic, Cr, Re);
			coherence();
			iprint("#l%d: link down\n", edev->ctlrno);
		}
		return;
	}
	if (edev->link == 0) {
		edev->link = 1;
		csr8w(ctlr->nic, Cr, Te|Re);
		coherence();
		iprint("#l%d: link up\n", edev->ctlrno);
	}
	limit = 256*1024;
	ombps = edev->mbps;
	if(r & Speed10){
		edev->mbps = 10;
		limit = 65*1024;
	} else if(r & Speed100)
		edev->mbps = 100;
	else if(r & Speed1000)
		edev->mbps = 1000;
	if (edev->mbps != ombps)
		iprint("8169: link at %d Mb/s, with %d-byte oq\n",
			edev->mbps, limit);

	if(edev->oq != nil)
		qsetlimit(edev->oq, limit);
}

static void
rtl8169transmit(Ether* edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if (ctlr == nil) {
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
	int i, del;
	Ctlr *ctlr;
	static int inrestart;
	static Lock rstrtlck;

	/* keep other cpus out */
	if (inrestart)
		return;

	ilock(&rstrtlck);
	ctlr = edev->ctlr;
	if (ctlr == nil || !ctlr->init || inrestart) {
		iunlock(&rstrtlck);
		return;
	}
	inrestart = 1;
	iprint("#l%d: restart due to %s\n", edev->ctlrno, why);

	/* process any pkts in the rings */
	wakeup(&ctlr->rrendez);
	coherence();
	rtl8169transmit(edev);
	/* allow time to drain a 1024-buffer ring */
	for (del = 0; del < 13 && ctlr->ntq > 0; del++)
		delay(1);

	iunlock(&ctlr->reglock);	/* release reglock for reset */
	rtl8169reset(ctlr);
	ilock(&ctlr->reglock);

	/* free any remaining unprocessed input buffers */
	for (i = 0; i < Nrd; i++) {
		freeb(ctlr->rb[i]);
		ctlr->rb[i] = nil;
	}

	iunlock(&ctlr->reglock);	/* release reglock for init */
	rtl8169init(edev);
	ilock(&ctlr->reglock);

	rtl8169link(edev);
	rtl8169transmit(edev);		/* drain any output queue */
	inrestart = 0;
	wakeup(&ctlr->rrendez);
	iunlock(&rstrtlck);
}

static ulong
rcvdiag(Ether *edev, ulong isr)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	isr &= ~ctlr->intrignbits;
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
		if (isr & Fovw)
			iprint("#l%d: rcv fifo overflow\n", edev->ctlrno);
		if (isr & Rdu)
			iprint("#l%d: rcv desc unavailable\n", edev->ctlrno);
		if (isr & Rer)
			iprint("#l%d: read error\n", edev->ctlrno);
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
	ulong isr;
	void *regs;

	edev = arg;
	ctlr = edev->ctlr;
	regs = ctlr->nic;
	ilock(&ctlr->reglock);
	while((isr = csr16r(regs, Isr)) != 0 && isr != 0xFFFF){
		ctlr->isr |= isr;		/* merge bits for [rt]proc */
		csr16w(regs, Isr, isr);		/* dismiss? */
		coherence();
		isr &= ~ctlr->intrignbits;
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
			csr16w(regs, Imr, ctlr->imr);
			coherence();
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
			csr16w(regs, Imr, ctlr->imr);
			coherence();
			wakeup(&ctlr->trendez);

			if (isr & Ter)
				iprint("8169: xmit err; isr %8.8#lux\n", isr);
			isr &= ~(Ter|Tok);
		}
		if(isr & Punlc){
			rtl8169link(edev);
			isr &= ~Punlc;
		}
		/*
		 * Some of the reserved bits get set sometimes...
		 */
		isr &= ~(Tdu|Tok|Rok);			/* harmless */
		if(isr & ~(Serr|Fovw|Punlc|Rdu|Rer|Rok|Tdu|Ter|Tok))
			panic("rtl8169interrupt: imr %#4.4ux isr %#4.4lux",
				csr16r(regs, Imr), isr);
	}
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
	*macv = csr32r(ctlr->nic, Tcr) & HwveridMASK;
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
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;

		pcie = 0;
		switch(i = ((p->did<<16)|p->vid)){
		default:
			continue;
		case Rtl8100e:			/* RTL810[01]E ? */
		case Rtl8168b:			/* RTL8168B: buggy */
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
	ulong r;
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
	/* edev->irq != ctlr->pcidev->intl on trimslice */
	edev->irq = Pcieirq;		/* trimslice: non-msi pci-e intr */
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->maxmtu = ETHERMAXTU;

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the device and set in edev->ea.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, edev->ea, Eaddrlen) == 0){
		r = csr32r(ctlr->nic, Idr0);
		edev->ea[0] = r;
		edev->ea[1] = r>>8;
		edev->ea[2] = r>>16;
		edev->ea[3] = r>>24;
		r = csr32r(ctlr->nic, Idr0+4);
		edev->ea[4] = r;
		edev->ea[5] = r>>8;
	}

	edev->arg = edev;
	edev->attach = rtl8169attach;
	edev->transmit = rtl8169transmit;
	edev->interrupt = rtl8169interrupt;
	edev->ifstat = rtl8169ifstat;

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
