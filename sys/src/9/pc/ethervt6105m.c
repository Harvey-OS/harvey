/*
 * VIA VT6105M Fast Ethernet Controller (Rhine III).
 * To do:
 *	cache-line size alignments - done
 *	reduce tx interrupts - done
 *	reorganise initialisation/shutdown/reset
 *	adjust Tx FIFO threshold on underflow - untested
 *	why does the link status never cause an interrupt?
 *	use the lproc as a periodic timer for stalls, etc.
 *	checksum offload - done
 *	take non-HW stuff out of descriptor for 64-bit
 *	cleanliness
 *	why does the receive buffer alloc have a +3?
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

enum {
	Par0		= 0x00,			/* Ethernet Address */
	Rcr		= 0x06,			/* Receive Configuration */
	Tcr		= 0x07,			/* Transmit Configuration */
	Cr		= 0x08,			/* Control */
	Tqw		= 0x0A,			/* Transmit Queue Wake */
	Isr		= 0x0C,			/* Interrupt Status */
	Imr		= 0x0E,			/* Interrupt Mask */
	Mcfilt0		= 0x10,			/* Multicast Filter 0 */
	Mcfilt1		= 0x14,			/* Multicast Filter 1 */
	Rxdaddr		= 0x18,			/* Current Rd Address */
	Txdaddr		= 0x1C,			/* Current Td Address */
	Phyadr		= 0x6C,			/* Phy Address */
	Miisr		= 0x6D,			/* MII Status */
	Bcr0		= 0x6E,			/* Bus Control */
	Bcr1		= 0x6F,
	Miicr		= 0x70,			/* MII Control */
	Miiadr		= 0x71,			/* MII Address */
	Miidata		= 0x72,			/* MII Data */
	Eecsr		= 0x74,			/* EEPROM Control and Status */
	CfgA		= 0x78,			/* Chip Configuration A */
	CfgB		= 0x79,
	CfgC		= 0x7A,
	CfgD		= 0x7B,
	Cr0		= 0x80,			/* Miscellaneous Control */
	Cr1		= 0x81,
	Pmcc		= 0x82,			/* Power Mgmt Capability Control */
	Stickhw		= 0x83,			/* Sticky Hardware Control */
	Misr		= 0x84,			/* MII Interrupt Control */
	Mimr		= 0x85,			/* MII Interrupt Mask */
	Wolcrclr	= 0xA4,
	Wolcgclr	= 0xA7,
	Pwrcsrclr	= 0xAC,
};

enum {						/* Rcr */
	Sep		= 0x01,			/* Accept Error Packets */
	Ar		= 0x02,			/* Accept Small Packets */
	Am		= 0x04,			/* Accept Multicast */
	Ab		= 0x08,			/* Accept Broadcast */
	Prom		= 0x10,			/* Accept Physical Address Packets */
	RrftMASK	= 0xE0,			/* Receive FIFO Threshold */
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

enum {						/* Tcr */
	Lb0		= 0x02,			/* Loopback Mode */
	Lb1		= 0x04,
	Ofset		= 0x08,			/* Select Back-off Priority */
	RtsfMASK	= 0xE0,			/* Transmit FIFO Threshold */
	RtsfSHIFT	= 5,
	Rtsf128		= 0<<RtsfSHIFT,
	Rtsf256		= 1<<RtsfSHIFT,
	Rtsf512		= 2<<RtsfSHIFT,
	Rtsf1024	= 3<<RtsfSHIFT,
	RtsfSAF		= 7<<RtsfSHIFT,
};

enum {						/* Cr */
	Init		= 0x0001,		/* INIT Process Begin */
	Strt		= 0x0002,		/* Start NIC */
	Stop		= 0x0004,		/* Stop NIC */
	Rxon		= 0x0008,		/* Turn on Receive Process */
	Txon		= 0x0010,		/* Turn on Transmit Process */
	Tdmd		= 0x0020,		/* Transmit Poll Demand */
	Rdmd		= 0x0040,		/* Receive Poll Demand */
	Eren		= 0x0100,		/* Early Receive Enable */
	Fdx		= 0x0400,		/* Set MAC to Full Duplex */
	Dpoll		= 0x0800,		/* Disable Td/Rd Auto Polling */
	Tdmd1		= 0x2000,		/* Transmit Poll Demand 1 */
	Rdmd1		= 0x4000,		/* Receive Poll Demand 1 */
	Sfrst		= 0x8000,		/* Software Reset */
};

enum {						/* Isr/Imr */
	Prx		= 0x0001,		/* Packet Received OK */
	Ptx		= 0x0002,		/* Packet Transmitted OK */
	Rxe		= 0x0004,		/* Receive Error */
	Txe		= 0x0008,		/* Transmit Error */
	Tu		= 0x0010,		/* Transmit Buffer Underflow */
	Ru		= 0x0020,		/* Receive Buffer Link Error */
	Be		= 0x0040,		/* PCI Bus Error */
	Cnt		= 0x0080,		/* Counter Overflow */
	Eri		= 0x0100,		/* Early Receive Interrupt */
	Udfi		= 0x0200,		/* Tx FIFO Underflow */
	Ovfi		= 0x0400,		/* Receive FIFO Overflow */
	Pktrace		= 0x0800,		/* Hmmm... */
	Norbf		= 0x1000,		/* No Receive Buffers */
	Abti		= 0x2000,		/* Transmission Abort */
	Srci		= 0x4000,		/* Port State Change */
	Geni		= 0x8000,		/* General Purpose Interrupt */
};

enum {						/* Phyadr */
	PhyadMASK	= 0x1F,			/* PHY Address */
	PhyadSHIFT	= 0,
	Mfdc		= 0x20,			/* Accelerate MDC Speed */
	Mpo0		= 0x40,			/* MII Polling Timer Interval */
	Mpo1		= 0x80,
};

enum {						/* Bcr0 */
	DmaMASK		= 0x07,			/* DMA Length */
	DmaSHIFT	= 0,
	Dma32		= 0<<DmaSHIFT,
	Dma64		= 1<<DmaSHIFT,
	Dma128		= 2<<DmaSHIFT,
	Dma256		= 3<<DmaSHIFT,
	Dma512		= 4<<DmaSHIFT,
	Dma1024		= 5<<DmaSHIFT,
	DmaSAF		= 7<<DmaSHIFT,
	CrftMASK	= 0x38,			/* Rx FIFO Threshold */
	CrftSHIFT	= 3,
	Crft64		= 1<<CrftSHIFT,
	Crft128		= 2<<CrftSHIFT,
	Crft256		= 3<<CrftSHIFT,
	Crft512		= 4<<CrftSHIFT,
	Crft1024	= 5<<CrftSHIFT,
	CrftSAF		= 7<<CrftSHIFT,
	Extled		= 0x40,			/* Extra LED Support Control */
	Med2		= 0x80,			/* Medium Select Control */
};

enum {						/* Bcr1 */
	PotMASK		= 0x07,			/* Polling Timer Interval */
	PotSHIFT	= 0,
	CtftMASK	= 0x38,			/* Tx FIFO Threshold */
	CtftSHIFT	= 3,
	Ctft64		= 1<<CtftSHIFT,
	Ctft128		= 2<<CtftSHIFT,
	Ctft256		= 3<<CtftSHIFT,
	Ctft512		= 4<<CtftSHIFT,
	Ctft1024	= 5<<CtftSHIFT,
	CtftSAF		= 7<<CtftSHIFT,
};

enum {						/* Miicr */
	Mdc		= 0x01,			/* Clock */
	Mdi		= 0x02,			/* Data In */
	Mdo		= 0x04,			/* Data Out */
	Mout		= 0x08,			/* Output Enable */
	Mdpm		= 0x10,			/* Direct Program Mode Enable */
	Wcmd		= 0x20,			/* Write Enable */
	Rcmd		= 0x40,			/* Read Enable */
	Mauto		= 0x80,			/* Auto Polling Enable */
};

enum {						/* Miiadr */
	MadMASK		= 0x1F,			/* MII Port Address */
	MadSHIFT	= 0,
	Mdone		= 0x20,			/* Accelerate MDC Speed */
	Msrcen		= 0x40,			/* MII Polling Timer Interval */
	Midle		= 0x80,
};

enum {						/* Eecsr */
	Edo		= 0x01,			/* Data Out */
	Edi		= 0x02,			/* Data In */
	Eck		= 0x04,			/* Clock */
	Ecs		= 0x08,			/* Chip Select */
	Dpm		= 0x10,			/* Direct Program Mode Enable */
	Autold		= 0x20,			/* Dynamic Reload */
	Embp		= 0x40,			/* Embedded Program Enable */
	Eepr		= 0x80,			/* Programmed */
};

/*
 * Ring descriptor. The space allocated for each
 * of these will be rounded up to a cache-line boundary.
 * The first 4 elements are known to the hardware.
 */
typedef struct Ds Ds;
typedef struct Ds {
	u32int	status;
	u32int	control;
	u32int	addr;
	u32int	branch;

	Block*	bp;
	Ds*	next;
	Ds*	prev;
} Ds;

enum {						/* Rx Ds status */
	Rerr		= 0x00000001,		/* Buff|Rxserr|Fov|Fae|Crc */
	Crc		= 0x00000002,		/* CRC Error */
	Fae		= 0x00000004,		/* Frame Alignment Error */
	Fov		= 0x00000008,		/* FIFO Overflow */
	Long		= 0x00000010,		/* A Long Packet */
	Runt		= 0x00000020,		/* A Runt Packet */
	Rxserr		= 0x00000040,		/* System Error */
	Buff		= 0x00000080,		/* Buffer Underflow Error */
	Rxedp		= 0x00000100,		/* End of Packet Buffer */
	Rxstp		= 0x00000200,		/* Packet Start */
	Chn		= 0x00000400,		/* Chain Buffer */
	Phy		= 0x00000800,		/* Physical Address Packet */
	Bar		= 0x00001000,		/* Broadcast Packet */
	Mar		= 0x00002000,		/* Multicast Packet */
	Rxok		= 0x00008000,		/* Packet Received OK */
	LengthMASK	= 0x07FF0000,		/* Received Packet Length */
	LengthSHIFT	= 16,

	Own		= 0x80000000,		/* Descriptor Owned by NIC */
};

enum {						/* Rx Ds control */
	RbsizeMASK	= 0x000007FF,		/* Receive Buffer Size */
	RbsizeSHIFT	= 0,
	Tag		= 0x00010000,		/* Receive a Tagged Packet */
	Udpkt		= 0x00020000,		/* Receive a UDP Packet */
	Tcpkt		= 0x00040000,		/* Receive a TCP Packet */
	Ipkt		= 0x00080000,		/* Receive an IP Packet */
	Tuok		= 0x00100000,		/* TCP/UDP Checksum OK */
	Ipok		= 0x00200000,		/* IP Checksum OK */
	Snaptag		= 0x00400000,		/* Snap Packet + 802.1q Tag */
	Rxlerr		= 0x00800000,		/* Receive Length Check Error */
	IpktMASK	= 0xff000000,		/* Interesting Packet */
	IpktSHIFT	= 24,
};

enum {						/* Tx Ds status */
	NcrMASK		= 0x0000000F,		/* Collision Retry Count */
	NcrSHIFT	= 0,
	Cols		= 0x00000010,		/* Experienced Collisions */
	Cdh		= 0x00000080,		/* CD Heartbeat */
	Abt		= 0x00000100,		/* Aborted after Excessive Collisions */
	Owc		= 0x00000200,		/* Out of Window Collision */
	Crs		= 0x00000400,		/* Carrier Sense Lost */
	Udf		= 0x00000800,		/* FIFO Underflow */
	Tbuff		= 0x00001000,		/* Invalid Td */
	Txserr		= 0x00002000,		/* System Error */
	Terr		= 0x00008000,		/* Excessive Collisions */
};

enum {						/* Tx Ds control */
	TbsMASK		= 0x000007FF,		/* Tx Buffer Size */
	TbsSHIFT	= 0,
	Chain		= 0x00008000,		/* Chain Buffer */
	Crcdisable	= 0x00010000,		/* Disable CRC generation */
	Stp		= 0x00200000,		/* Start of Packet */
	Edp		= 0x00400000,		/* End of Packet */
	Ic		= 0x00800000,		/* Interrupt Control */
};

enum {						/* Tx Ds branch */
	Tdctl		= 0x00000001,		/* No Interrupt Generated */
};

enum {
	Nrd		= 196,
	Ntd		= 64,
	Crcsz		= 4,
	Bslop		= 48,
	Rdbsz		= ETHERMAXTU+Crcsz+Bslop,

	Nrxstats	= 8,
	Ntxstats	= 9,

	Txcopy		= 128,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;
	uchar	par[Eaddrlen];

	QLock	alock;				/* attach */
	void*	alloc;				/* descriptors, etc. */
	int	cls;				/* alignment */
	int	nrd;
	int	ntd;

	Ds*	rd;
	Ds*	rdh;

	Lock	tlock;
	Ds*	td;
	Ds*	tdh;
	Ds*	tdt;
	int	tdused;

	Lock	clock;				/*  */
	int	cr;
	int	imr;
	int	tft;				/* Tx threshold */

	Mii*	mii;
	Rendez	lrendez;
	int	lwakeup;

	uint	rxstats[Nrxstats];		/* statistics */
	uint	txstats[Ntxstats];
	ulong	totalt;
	uint	intr;
	uint	lintr;			
	uint	lsleep;
	uint	rintr;
	uint	tintr;
	uint	txdw;
	int	tdumax;

	uint	abt;
	uint	tbuff;
	uint	udf;

	uint	abti;
	uint	udfi;
	uint	tu;

	uint	tuok;
	uint	ipok;
} Ctlr;

static Ctlr* vt6105Mctlrhead;
static Ctlr* vt6105Mctlrtail;

#define csr8r(c, r)	(inb((c)->port+(r)))
#define csr16r(c, r)	(ins((c)->port+(r)))
#define csr32r(c, r)	(inl((c)->port+(r)))
#define csr8w(c, r, b)	(outb((c)->port+(r), (int)(b)))
#define csr16w(c, r, w)	(outs((c)->port+(r), (ushort)(w)))
#define csr32w(c, r, w)	(outl((c)->port+(r), (ulong)(w)))

static Lock vt6105Mrblock;			/* receive Block freelist */
static Block* vt6105Mrbpool;
static uint vt6105Mrbpoolsz;

typedef struct Regs Regs;
typedef struct Regs {
	char*	name;
	int	offset;
	int	size;
} Regs;

static Regs regs[] = {
//	"Par0",		Par0,	1,
//	"Par1",		Par0+1,	1,
//	"Par2",		Par0+2,	1,
//	"Par3",		Par0+3,	1,
//	"Par4",		Par0+4,	1,
//	"Par5",		Par0+5,	1,
	"Rcr",		Rcr,	1,
	"Tcr",		Tcr,	1,
	"Cr0",		Cr,	1,
	"Cr1",		Cr+1,	1,
	"Isr0",		Isr,	1,
	"Isr1",		Isr+1,	1,
	"Imr0",		Imr,	1,
	"Imr1",		Imr+1,	1,
//	"Mcfilt0",	Mcfilt0,4,
//	"Mcfilt1",	Mcfilt1,4,
//	"Rxdaddr",	Rxdaddr,4,
//	"Txdaddr",	Txdaddr,4,
	"Phyadr",	Phyadr,	1,
	"Miisr",	Miisr,	1,
	"Bcr0",		Bcr0,	1,
	"Bcr1",		Bcr1,	1,
	"Miicr",	Miicr,	1,
	"Miiadr",	Miiadr,	1,
//	"Miidata",	Miidata,2,
	"Eecsr",	Eecsr,	1,
	"CfgA",		CfgA,	1,
	"CfgB",		CfgB,	1,
	"CfgC",		CfgC,	1,
	"CfgD",		CfgD,	1,
	"Cr0",		Cr0,	1,
	"Cr1",		Cr1,	1,
	"Pmcc",		Pmcc,	1,
	"Stickhw",	Stickhw,1,
	"Misr",		Misr,	1,
	"Mimr",		Mimr,	1,
	nil,
};

static char* rxstats[Nrxstats] = {
	"Receiver Error",
	"CRC Error",
	"Frame Alignment Error",
	"FIFO Overflow",
	"Long Packet",
	"Runt Packet",
	"System Error",
	"Buffer Underflow Error",
};
static char* txstats[Ntxstats] = {
	"Aborted after Excessive Collisions",
	"Out of Window Collision Seen",
	"Carrier Sense Lost",
	"FIFO Underflow",
	"Invalid Td",
	"System Error",
	nil,
	"Excessive Collisions",
};

static long
vt6105Mifstat(Ether* edev, void* a, long n, ulong offset)
{
	int i, r;
	Ctlr *ctlr;
	char *alloc, *e, *p;

	ctlr = edev->ctlr;

	alloc = malloc(READSTR);
	p = alloc;
	if(p == nil)
		error(Enomem);
	e = p + READSTR;
	for(i = 0; i < Nrxstats; i++){
		p = seprint(p, e, "%s: %ud\n", rxstats[i], ctlr->rxstats[i]);
	}
	for(i = 0; i < Ntxstats; i++){
		if(txstats[i] == nil)
			continue;
		p = seprint(p, e, "%s: %ud\n", txstats[i], ctlr->txstats[i]);
	}
	p = seprint(p, e, "cls: %ud\n", ctlr->cls);
	p = seprint(p, e, "intr: %ud\n", ctlr->intr);
	p = seprint(p, e, "lintr: %ud\n", ctlr->lintr);
	p = seprint(p, e, "lsleep: %ud\n", ctlr->lsleep);
	p = seprint(p, e, "rintr: %ud\n", ctlr->rintr);
	p = seprint(p, e, "tintr: %ud\n", ctlr->tintr);
	p = seprint(p, e, "txdw: %ud\n", ctlr->txdw);
	p = seprint(p, e, "tdumax: %ud\n", ctlr->tdumax);
	p = seprint(p, e, "tft: %ud\n", ctlr->tft);

	p = seprint(p, e, "abt: %ud\n", ctlr->abt);
	p = seprint(p, e, "tbuff: %ud\n", ctlr->tbuff);
	p = seprint(p, e, "udf: %ud\n", ctlr->udf);
	p = seprint(p, e, "abti: %ud\n", ctlr->abti);
	p = seprint(p, e, "udfi: %ud\n", ctlr->udfi);
	p = seprint(p, e, "tu: %ud\n", ctlr->tu);

	p = seprint(p, e, "tuok: %ud\n", ctlr->tuok);
	p = seprint(p, e, "ipok: %ud\n", ctlr->ipok);

	p = seprint(p, e, "rbpoolsz: %ud\n", vt6105Mrbpoolsz);
	p = seprint(p, e, "totalt: %uld\n", ctlr->totalt);

	for(i = 0; regs[i].name != nil; i++){
		p = seprint(p, e, "%s: %2.2x\n",
			regs[i].name,  csr8r(ctlr, regs[i].offset));
	}

	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		p = seprint(p, e, "phy:   ");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				p = seprint(p, e, "\n       ");
			r = miimir(ctlr->mii, i);
			p = seprint(p, e, " %4.4uX", r);
		}
		seprint(p, e, "\n");
	}

	n = readstr(offset, a, n, alloc);
	free(alloc);

	return n;
}

static void
vt6105Mpromiscuous(void* arg, int on)
{
	int rcr;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	rcr = csr8r(ctlr, Rcr);
	if(on)
		rcr |= Prom;
	else
		rcr &= ~Prom;
	csr8w(ctlr, Rcr, rcr);
}

static void
vt6105Mmulticast(void* arg, uchar* addr, int on)
{
	/*
	 * For now Am is set in Rcr.
	 * Will need to interlock with promiscuous
	 * when this gets filled in.
	 */
	USED(arg, addr, on);
}

static int
vt6105Mwakeup(void* v)
{
	return *((int*)v) != 0;
}

static void
vt6105Mimr(Ctlr* ctlr, int imr)
{
	ilock(&ctlr->clock);
	ctlr->imr |= imr;
	csr16w(ctlr, Imr, ctlr->imr);
	iunlock(&ctlr->clock);
}

static void
vt6105Mlproc(void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	MiiPhy *phy;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		if(ctlr->mii == nil || ctlr->mii->curphy == nil)
			break;
		if(miistatus(ctlr->mii) < 0)
			goto enable;

		phy = ctlr->mii->curphy;
		ilock(&ctlr->clock);
		csr16w(ctlr, Cr, ctlr->cr & ~(Txon|Rxon));
		if(phy->fd)
			ctlr->cr |= Fdx;
		else
			ctlr->cr &= ~Fdx;
		csr16w(ctlr, Cr, ctlr->cr);
		iunlock(&ctlr->clock);
enable:
		ctlr->lwakeup = 0;
		vt6105Mimr(ctlr, Srci);

		ctlr->lsleep++;
		sleep(&ctlr->lrendez, vt6105Mwakeup, &ctlr->lwakeup);

	}
	pexit("vt6105Mlproc: done", 1);
}

static void
vt6105Mrbfree(Block* bp)
{
	bp->rp = bp->lim - (Rdbsz+3);
	bp->wp = bp->rp;
 	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(&vt6105Mrblock);
	bp->next = vt6105Mrbpool;
	vt6105Mrbpool = bp;
	iunlock(&vt6105Mrblock);
}

static Block*
vt6105Mrballoc(void)
{
	Block *bp;

	ilock(&vt6105Mrblock);
	if((bp = vt6105Mrbpool) != nil){
		vt6105Mrbpool = bp->next;
		bp->next = nil;
		_xinc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&vt6105Mrblock);

	if(bp == nil && (bp = iallocb(Rdbsz+3)) != nil){
		bp->free = vt6105Mrbfree;
		vt6105Mrbpoolsz++;
	}
	return bp;
}

static void
vt6105Mattach(Ether* edev)
{
	Ctlr *ctlr;
//	MiiPhy *phy;
	uchar *alloc;
	Ds *ds, *prev;
	int dsz, i, timeo;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){
		qunlock(&ctlr->alock);
		return;
	}

	/*
	 * Descriptor space.
	 * Receive descriptors should all be aligned on a 4-byte boundary,
	 * but try to do cache-line alignment.
	 */
	ctlr->nrd = Nrd;
	ctlr->ntd = Ntd;
	dsz = ROUNDUP(sizeof(Ds), ctlr->cls);
	alloc = mallocalign((ctlr->nrd+ctlr->ntd)*dsz, dsz, 0, 0);
	if(alloc == nil){
		qunlock(&ctlr->alock);
		error(Enomem);
	}
	ctlr->alloc = alloc;

	ctlr->rd = (Ds*)alloc;

	if(waserror()){
		ds = ctlr->rd;
		for(i = 0; i < ctlr->nrd; i++){
			if(ds->bp != nil){
				freeb(ds->bp);
				ds->bp = nil;
			}
			if((ds = ds->next) == nil)
				break;
		}
		free(ctlr->alloc);
		ctlr->alloc = nil;
		qunlock(&ctlr->alock);
		nexterror();
	}

	prev = (Ds*)(alloc + (ctlr->nrd-1)*dsz);
	for(i = 0; i < ctlr->nrd; i++){
		ds = (Ds*)alloc;
		alloc += dsz;

		ds->control = Ipkt|Tcpkt|Udpkt|Rdbsz;
		ds->branch = PCIWADDR(alloc);

		ds->bp = vt6105Mrballoc();
		if(ds->bp == nil)
			error("vt6105M: can't allocate receive ring\n");
		ds->bp->rp = (uchar*)ROUNDUP((ulong)ds->bp->rp, 4);
		ds->addr = PCIWADDR(ds->bp->rp);

		ds->next = (Ds*)alloc;
		ds->prev = prev;
		prev = ds;

		ds->status = Own;
	}
	prev->branch = 0;
	prev->next = ctlr->rd;
	prev->status = 0;
	ctlr->rdh = ctlr->rd;

	ctlr->td = (Ds*)alloc;
	prev = (Ds*)(alloc + (ctlr->ntd-1)*dsz);
	for(i = 0; i < ctlr->ntd; i++){
		ds = (Ds*)alloc;
		alloc += dsz;

		ds->next = (Ds*)alloc;
		ds->prev = prev;
		prev = ds;
	}
	prev->next = ctlr->td;
	ctlr->tdh = ctlr->tdt = ctlr->td;
	ctlr->tdused = 0;

	ctlr->cr = Dpoll|Rdmd/*|Txon|Rxon*/|Strt;
	/*Srci|Abti|Norbf|Pktrace|Ovfi|Udfi|Be|Ru|Tu|Txe|Rxe|Ptx|Prx*/
	ctlr->imr = Abti|Norbf|Pktrace|Ovfi|Udfi|Be|Ru|Tu|Txe|Rxe|Ptx|Prx;

	ilock(&ctlr->clock);
	csr32w(ctlr, Rxdaddr, PCIWADDR(ctlr->rd));
	csr32w(ctlr, Txdaddr, PCIWADDR(ctlr->td));
	csr16w(ctlr, Isr, ~0);
	csr16w(ctlr, Imr, ctlr->imr);
	csr16w(ctlr, Cr, ctlr->cr);
	iunlock(&ctlr->clock);

	/*
	 * Wait for link to be ready.
	 */
	for(timeo = 0; timeo < 350; timeo++){
		if(miistatus(ctlr->mii) == 0)
			break;
		tsleep(&up->sleep, return0, 0, 10);
	}
//	phy = ctlr->mii->curphy;
//	print("%s: speed %d fd %d link %d rfc %d tfc %d\n",
//		edev->name, phy->speed, phy->fd, phy->link, phy->rfc, phy->tfc);

	ilock(&ctlr->clock);
	ctlr->cr |= Txon|Rxon;
	csr16w(ctlr, Cr, ctlr->cr);
	iunlock(&ctlr->clock);

	snprint(name, KNAMELEN, "#l%dlproc", edev->ctlrno);
	kproc(name, vt6105Mlproc, edev);

	qunlock(&ctlr->alock);
	poperror();
}

static void
vt6105Mtransmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	Ds *ds, *next;
	int control, i, size, tdused, timeo;
	long t;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);
	t = lcycles();

	/*
	 * Free any completed packets
	 */
	ds = ctlr->tdh;
	for(tdused = ctlr->tdused; tdused > 0; tdused--){
		/*
		 * For some errors the chip will turn the Tx engine
		 * off. Wait for that to happen.
		 * Could reset and re-init the chip here if it doesn't
		 * play fair.
		 * To do: adjust Tx FIFO threshold on underflow.
		 */
		if(ds->status & (Abt|Tbuff|Udf)){
			if(ds->status & Abt)
				ctlr->abt++;
			if(ds->status & Tbuff)
				ctlr->tbuff++;
			if(ds->status & Udf)
				ctlr->udf++;
			for(timeo = 0; timeo < 1000; timeo++){
				if(!(csr16r(ctlr, Cr) & Txon))
					break;
				microdelay(1);
			}
			ds->status = Own;
			csr32w(ctlr, Txdaddr, PCIWADDR(ds));
		}

		if(ds->status & Own)
			break;
		ds->addr = 0;
		ds->branch = 0;

		if(ds->bp != nil){
			freeb(ds->bp);
			ds->bp = nil;
		}
		for(i = 0; i < Ntxstats-1; i++){
			if(ds->status & (1<<i))
				ctlr->txstats[i]++;
		}
		ctlr->txstats[i] += (ds->status & NcrMASK)>>NcrSHIFT;

		ds = ds->next;
	}
	ctlr->tdh = ds;

	/*
	 * Try to fill the ring back up.
	 */
	ds = ctlr->tdt;
	while(tdused < ctlr->ntd-2){
		if((bp = qget(edev->oq)) == nil)
			break;
		tdused++;

		size = BLEN(bp);

		next = ds->next;
		ds->branch = PCIWADDR(ds->next)|Tdctl;

		ds->bp = bp;
		ds->addr = PCIWADDR(bp->rp);
		control = Edp|Stp|((size<<TbsSHIFT) & TbsMASK);

		ds->control = control;
		if(tdused >= ctlr->ntd-2){
			ctlr->txdw++;
			ds->branch &= ~Tdctl;
		}
		coherence();
		ds->status = Own;

		ds = next;
	}
	ctlr->tdt = ds;
	ctlr->tdused = tdused;
	if(ctlr->tdused){
		csr16w(ctlr, Cr, Tdmd|ctlr->cr);
		if(tdused > ctlr->tdumax)
			ctlr->tdumax = tdused;
	}

	ctlr->totalt += lcycles() - t;
	iunlock(&ctlr->tlock);
}

static void
vt6105Mreceive(Ether* edev)
{
	Ds *ds;
	Block *bp;
	Ctlr *ctlr;
	int i, len;

	ctlr = edev->ctlr;

	ds = ctlr->rdh;
	while(!(ds->status & Own) && ds->status != 0){
		/*
		 * Can Long packets be received OK?
		 * What happens to the Rxok bit?
		 */
		if(ds->status & Rerr){
			for(i = 0; i < Nrxstats; i++){
				if(ds->status & (1<<i))
					ctlr->rxstats[i]++;
			}
		}
		else if(bp = vt6105Mrballoc()){
			if(ds->control & Tuok){
				ds->bp->flag |= Btcpck|Budpck;
				ctlr->tuok++;
			}
			if(ds->control & Ipok){
				ds->bp->flag |= Bipck;
				ctlr->ipok++;
			}
			len = ((ds->status & LengthMASK)>>LengthSHIFT)-4;
			ds->bp->wp = ds->bp->rp+len;
			etheriq(edev, ds->bp, 1);
			bp->rp = (uchar*)ROUNDUP((ulong)bp->rp, 4);
			ds->addr = PCIWADDR(bp->rp);
			ds->bp = bp;
		}
		ds->control = Ipkt|Tcpkt|Udpkt|Rdbsz;
		ds->branch = 0;
		ds->status = 0;

		ds->prev->branch = PCIWADDR(ds);
		coherence();
		ds->prev->status = Own;

		ds = ds->next;
	}
	ctlr->rdh = ds;

	csr16w(ctlr, Cr, ctlr->cr);
}

static void
vt6105Minterrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	int imr, isr, r, timeo;
	long t;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->clock);
	t = lcycles();

	csr16w(ctlr, Imr, 0);
	imr = ctlr->imr;
	ctlr->intr++;
	for(;;){
		if((isr = csr16r(ctlr, Isr)) != 0)
			csr16w(ctlr, Isr, isr);
		if((isr & ctlr->imr) == 0)
			break;
			
		if(isr & Srci){
			imr &= ~Srci;
			ctlr->lwakeup = isr & Srci;
			wakeup(&ctlr->lrendez);
			isr &= ~Srci;
			ctlr->lintr++;
		}
		if(isr & (Norbf|Pktrace|Ovfi|Ru|Rxe|Prx)){
			vt6105Mreceive(edev);
			isr &= ~(Norbf|Pktrace|Ovfi|Ru|Rxe|Prx);
			ctlr->rintr++;
		}
		if(isr & (Abti|Udfi|Tu|Txe|Ptx)){
			if(isr & (Abti|Udfi|Tu)){
				if(isr & Abti)
					ctlr->abti++;
				if(isr & Udfi)
					ctlr->udfi++;
				if(isr & Tu)
					ctlr->tu++;
				for(timeo = 0; timeo < 1000; timeo++){
					if(!(csr16r(ctlr, Cr) & Txon))
						break;
					microdelay(1);
				}

				if((isr & Udfi) && ctlr->tft < CtftSAF){
					ctlr->tft += 1<<CtftSHIFT;
					r = csr8r(ctlr, Bcr1) & ~CtftMASK;
					csr8w(ctlr, Bcr1, r|ctlr->tft);
				}
			}
			
			
			ctlr->totalt += lcycles() - t;
			vt6105Mtransmit(edev);
			t = lcycles();
			isr &= ~(Abti|Udfi|Tu|Txe|Ptx);
			ctlr->tintr++;
		}
		if(isr)
			panic("vt6105M: isr %4.4uX\n", isr);
	}
	ctlr->imr = imr;
	csr16w(ctlr, Imr, ctlr->imr);
	
	ctlr->totalt += lcycles() - t;
	iunlock(&ctlr->clock);
}

static int
vt6105Mmiimicmd(Mii* mii, int pa, int ra, int cmd, int data)
{
	Ctlr *ctlr;
	int r, timeo;

	ctlr = mii->ctlr;

	csr8w(ctlr, Miicr, 0);
	r = csr8r(ctlr, Phyadr);
	csr8w(ctlr, Phyadr, (r & ~PhyadMASK)|pa);
	csr8w(ctlr, Phyadr, pa);
	csr8w(ctlr, Miiadr, ra);
	if(cmd == Wcmd)
		csr16w(ctlr, Miidata, data);
	csr8w(ctlr, Miicr, cmd);

	for(timeo = 0; timeo < 10000; timeo++){
		if(!(csr8r(ctlr, Miicr) & cmd))
			break;
		microdelay(1);
	}
	if(timeo >= 10000)
		return -1;

	if(cmd == Wcmd)
		return 0;
	return csr16r(ctlr, Miidata);
}

static int
vt6105Mmiimir(Mii* mii, int pa, int ra)
{
	return vt6105Mmiimicmd(mii, pa, ra, Rcmd, 0);
}

static int
vt6105Mmiimiw(Mii* mii, int pa, int ra, int data)
{
	return vt6105Mmiimicmd(mii, pa, ra, Wcmd, data);
}

static int
vt6105Mdetach(Ctlr* ctlr)
{
	int revid, timeo;

	/*
	 * Reset power management registers.
	 */
	revid = pcicfgr8(ctlr->pcidev, PciRID);
	if(revid >= 0x40){
		/* Set power state D0. */
		csr8w(ctlr, Stickhw, csr8r(ctlr, Stickhw) & 0xFC);

		/* Disable force PME-enable. */
		csr8w(ctlr, Wolcgclr, 0x80);

		/* Clear WOL config and status bits. */
		csr8w(ctlr, Wolcrclr, 0xFF);
		csr8w(ctlr, Pwrcsrclr, 0xFF);
	}

	/*
	 * Soft reset the controller.
	 */
	csr16w(ctlr, Cr, Stop);
	csr16w(ctlr, Cr, Stop|Sfrst);
	/* limit used to be 10000, but that wasn't enough for our Soekris 5501s */
	for(timeo = 0; timeo < 100000; timeo++){
		if(!(csr16r(ctlr, Cr) & Sfrst))
			break;
		microdelay(1);
	}
	if(timeo >= 1000)
		return -1;

	return 0;
}

static void
vt6105Mshutdown(Ether *ether)
{
	Ctlr *ctlr = ether->ctlr;

	vt6105Mdetach(ctlr);
}

static int
vt6105Mreset(Ctlr* ctlr)
{
	MiiPhy *phy;
	int i, r, timeo;

	if(vt6105Mdetach(ctlr) < 0)
		return -1;

	/*
	 * Load the MAC address into the PAR[01]
	 * registers.
	 */
	r = csr8r(ctlr, Eecsr);
	csr8w(ctlr, Eecsr, Autold|r);
	/* limit used to be 100, but that wasn't enough for our Soekris 5501s */
	for(timeo = 0; timeo < 100000; timeo++){
		if(!(csr8r(ctlr, Cr) & Autold))
			break;
		microdelay(1);
	}
	if(timeo >= 100)
		return -1;

	for(i = 0; i < Eaddrlen; i++)
		ctlr->par[i] = csr8r(ctlr, Par0+i);

	/*
	 * Configure DMA and Rx/Tx thresholds.
	 * If the Rx/Tx threshold bits in Bcr[01] are 0 then
	 * the thresholds are determined by Rcr/Tcr.
	 */
	r = csr8r(ctlr, Bcr0) & ~(CrftMASK|DmaMASK);
	csr8w(ctlr, Bcr0, r|Crft128|DmaSAF);
	r = csr8r(ctlr, Bcr1) & ~CtftMASK;
	csr8w(ctlr, Bcr1, r|ctlr->tft);

	r = csr8r(ctlr, Rcr) & ~(RrftMASK|Prom|Ar|Sep);
	csr8w(ctlr, Rcr, r|Ab|Am);
	csr32w(ctlr, Mcfilt0, ~0UL);	/* accept all multicast */
	csr32w(ctlr, Mcfilt1, ~0UL);

	r = csr8r(ctlr, Tcr) & ~(RtsfMASK|Ofset|Lb1|Lb0);
	csr8w(ctlr, Tcr, r);

	/*
	 * Link management.
	 */
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->mir = vt6105Mmiimir;
	ctlr->mii->miw = vt6105Mmiimiw;
	ctlr->mii->ctlr = ctlr;

	if(mii(ctlr->mii, ~0) == 0 || (phy = ctlr->mii->curphy) == nil){
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
//	print("oui %X phyno %d\n", phy->oui, phy->phyno);
	USED(phy);

	if(miistatus(ctlr->mii) < 0){
//		miireset(ctlr->mii);
		miiane(ctlr->mii, ~0, ~0, ~0);
	}

	return 0;
}

static void
vt6105Mpci(void)
{
	Pcidev *p;
	Ctlr *ctlr;
	int cls, port;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x3053<<16)|0x1106:	/* Rhine III-M vt6105M */
			break;
		}

		port = p->mem[0].bar & ~0x01;
		if(ioalloc(port, p->mem[0].size, 0, "vt6105M") < 0){
			print("vt6105M: port 0x%uX in use\n", port);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil) {
			iofree(port);
			error(Enomem);
		}
		ctlr->port = port;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;
		if((cls = pcicfgr8(p, PciCLS)) == 0 || cls == 0xFF)
			cls = 0x10;
		ctlr->cls = cls*4;
		if(ctlr->cls < sizeof(Ds)){
			print("vt6105M: cls %d < sizeof(Ds)\n", ctlr->cls);
			iofree(port);
			free(ctlr);
			continue;
		}
		ctlr->tft = CtftSAF;

		if(vt6105Mreset(ctlr)){
			iofree(port);
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(vt6105Mctlrhead != nil)
			vt6105Mctlrtail->next = ctlr;
		else
			vt6105Mctlrhead = ctlr;
		vt6105Mctlrtail = ctlr;
	}
}

static int
vt6105Mpnp(Ether* edev)
{
	Ctlr *ctlr;

	if(vt6105Mctlrhead == nil)
		vt6105Mpci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = vt6105Mctlrhead; ctlr != nil; ctlr = ctlr->next){
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
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;
	/*
	 * Set to 1000Mb/s to fool the bsz calculation.  We need 
	 * something better, though.
	 */
	edev->mbps = 1000;
	memmove(edev->ea, ctlr->par, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = vt6105Mattach;
	edev->transmit = vt6105Mtransmit;
	edev->interrupt = vt6105Minterrupt;
	edev->ifstat = vt6105Mifstat;
	edev->shutdown = vt6105Mshutdown;
	edev->ctl = nil;

	edev->arg = edev;
	edev->promiscuous = vt6105Mpromiscuous;
	edev->multicast = vt6105Mmulticast;

	edev->maxmtu = ETHERMAXTU+Bslop;

	return 0;
}

void
ethervt6105mlink(void)
{
	addethercard("vt6105M", vt6105Mpnp);
}
