/*
 * bootstrap driver for
 * Intel 82563, 82571, 82573 Gigabit Ethernet PCI-Express Controllers
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"

/* compatibility with cpu kernels */
#define iallocb allocb
#ifndef CACHELINESZ
#define CACHELINESZ	32		/* pentium & later */
#endif

/* from pci.c */
enum
{					/* command register pcidev->pcr */
	IOen		= 1<<0,
	MEMen		= 1<<1,
	MASen		= 1<<2,
	MemWrInv	= 1<<4,
	PErrEn		= 1<<6,
	SErrEn		= 1<<8,
};

/*
 * these are in the order they appear in the manual, not numeric order.
 * It was too hard to find them in the book. Ref 21489, rev 2.6
 */

enum {
	/* General */

	Ctrl		= 0x00000000,	/* Device Control */
	Status		= 0x00000008,	/* Device Status */
	Eec		= 0x00000010,	/* EEPROM/Flash Control/Data */
	Eerd		= 0x00000014,	/* EEPROM Read */
	Ctrlext		= 0x00000018,	/* Extended Device Control */
	Fla		= 0x0000001c,	/* Flash Access */
	Mdic		= 0x00000020,	/* MDI Control */
	Seresctl	= 0x00000024,	/* Serdes ana */
	Fcal		= 0x00000028,	/* Flow Control Address Low */
	Fcah		= 0x0000002C,	/* Flow Control Address High */
	Fct		= 0x00000030,	/* Flow Control Type */
	Kumctrlsta	= 0x00000034,	/* Kumeran Controll and Status Register */
	Vet		= 0x00000038,	/* VLAN EtherType */
	Fcttv		= 0x00000170,	/* Flow Control Transmit Timer Value */
	Txcw		= 0x00000178,	/* Transmit Configuration Word */
	Rxcw		= 0x00000180,	/* Receive Configuration Word */
	Ledctl		= 0x00000E00,	/* LED control */
	Pba		= 0x00001000,	/* Packet Buffer Allocation */

	/* Interrupt */

	Icr		= 0x000000C0,	/* Interrupt Cause Read */
	Ics		= 0x000000C8,	/* Interrupt Cause Set */
	Ims		= 0x000000D0,	/* Interrupt Mask Set/Read */
	Imc		= 0x000000D8,	/* Interrupt mask Clear */
	Iam		= 0x000000E0,	/* Interrupt acknowledge Auto Mask */

	/* Receive */

	Rctl		= 0x00000100,	/* Receive Control */
	Ert		= 0x00002008,	/* Early Receive Threshold (573[EVL] only) */
	Fcrtl		= 0x00002160,	/* Flow Control RX Threshold Low */
	Fcrth		= 0x00002168,	/* Flow Control Rx Threshold High */
	Psrctl		= 0x00002170,	/* Packet Split Receive Control */
	Rdbal		= 0x00002800,	/* Rdesc Base Address Low Queue 0 */
	Rdbah		= 0x00002804,	/* Rdesc Base Address High Queue 0 */
	Rdlen		= 0x00002808,	/* Receive Descriptor Length Queue 0 */
	Rdh		= 0x00002810,	/* Receive Descriptor Head Queue 0 */
	Rdt		= 0x00002818,	/* Receive Descriptor Tail Queue 0 */
	Rdtr		= 0x00002820,	/* Receive Descriptor Timer Ring */
	Rxdctl		= 0x00002828,	/* Receive Descriptor Control */
	Radv		= 0x0000282C,	/* Receive Interrupt Absolute Delay Timer */
	Rdbal1		= 0x00002900,	/* Rdesc Base Address Low Queue 1 */
	Rdbah1		= 0x00002804,	/* Rdesc Base Address High Queue 1 */
	Rdlen1		= 0x00002908,	/* Receive Descriptor Length Queue 1 */
	Rdh1		= 0x00002910,	/* Receive Descriptor Head Queue 1 */
	Rdt1		= 0x00002918,	/* Receive Descriptor Tail Queue 1 */
	Rxdctl1		= 0x00002928,	/* Receive Descriptor Control Queue 1 */
	Rsrpd		= 0x00002c00,	/* Receive Small Packet Detect */
	Raid		= 0x00002c08,	/* Receive ACK interrupt delay */
	Cpuvec		= 0x00002c10,	/* CPU Vector */
	Rxcsum		= 0x00005000,	/* Receive Checksum Control */
	Rfctl		= 0x00005008,	/* Receive Filter Control */
	Mta		= 0x00005200,	/* Multicast Table Array */
	Ral		= 0x00005400,	/* Receive Address Low */
	Rah		= 0x00005404,	/* Receive Address High */
	Vfta		= 0x00005600,	/* VLAN Filter Table Array */
	Mrqc		= 0x00005818,	/* Multiple Receive Queues Command */
	Rssim		= 0x00005864,	/* RSS Interrupt Mask */
	Rssir		= 0x00005868,	/* RSS Interrupt Request */
	Reta		= 0x00005c00,	/* Redirection Table */
	Rssrk		= 0x00005c80,	/* RSS Random Key */

	/* Transmit */

	Tctl		= 0x00000400,	/* Transmit Control */
	Tipg		= 0x00000410,	/* Transmit IPG */
	Tdbal		= 0x00003800,	/* Tdesc Base Address Low */
	Tdbah		= 0x00003804,	/* Tdesc Base Address High */
	Tdlen		= 0x00003808,	/* Transmit Descriptor Length */
	Tdh		= 0x00003810,	/* Transmit Descriptor Head */
	Tdt		= 0x00003818,	/* Transmit Descriptor Tail */
	Tidv		= 0x00003820,	/* Transmit Interrupt Delay Value */
	Txdctl		= 0x00003828,	/* Transmit Descriptor Control */
	Tadv		= 0x0000382C,	/* Transmit Interrupt Absolute Delay Timer */
	Tarc0		= 0x00003840,	/* Transmit Arbitration Counter Queue 0 */
	Tdbal1		= 0x00003900,	/* Transmit Descriptor Base Low Queue 1 */
	Tdbah1		= 0x00003904,	/* Transmit Descriptor Base High Queue 1 */
	Tdlen1		= 0x00003908,	/* Transmit Descriptor Length Queue 1 */
	Tdh1		= 0x00003910,	/* Transmit Descriptor Head Queue 1 */
	Tdt1		= 0x00003918,	/* Transmit Descriptor Tail Queue 1 */
	Txdctl1		= 0x00003928,	/* Transmit Descriptor Control 1 */
	Tarc1		= 0x00003940,	/* Transmit Arbitration Counter Queue 1 */

	/* Statistics */

	Statistics	= 0x00004000,	/* Start of Statistics Area */
	Gorcl		= 0x88/4,	/* Good Octets Received Count */
	Gotcl		= 0x90/4,	/* Good Octets Transmitted Count */
	Torl		= 0xC0/4,	/* Total Octets Received */
	Totl		= 0xC8/4,	/* Total Octets Transmitted */
	Nstatistics	= 64,

};

enum {					/* Ctrl */
	GIOmd		= 1<<2,		/* BIO master disable */
	Lrst		= 1<<3,		/* link reset */
	Slu		= 1<<6,		/* Set Link Up */
	SspeedMASK	= 3<<8,		/* Speed Selection */
	SspeedSHIFT	= 8,
	Sspeed10	= 0x00000000,	/* 10Mb/s */
	Sspeed100	= 0x00000100,	/* 100Mb/s */
	Sspeed1000	= 0x00000200,	/* 1000Mb/s */
	Frcspd		= 1<<11,	/* Force Speed */
	Frcdplx		= 1<<12,	/* Force Duplex */
	SwdpinsloMASK	= 0x003C0000,	/* Software Defined Pins - lo nibble */
	SwdpinsloSHIFT	= 18,
	SwdpioloMASK	= 0x03C00000,	/* Software Defined Pins - I or O */
	SwdpioloSHIFT	= 22,
	Devrst		= 1<<26,	/* Device Reset */
	Rfce		= 1<<27,	/* Receive Flow Control Enable */
	Tfce		= 1<<28,	/* Transmit Flow Control Enable */
	Vme		= 1<<30,	/* VLAN Mode Enable */
	Phy_rst		= 1<<31,	/* Phy Reset */
};

enum {					/* Status */
	Lu		= 1<<1,		/* Link Up */
	Lanid		= 3<<2,		/* mask for Lan ID. */
	Txoff		= 1<<4,		/* Transmission Paused */
	Tbimode		= 1<<5,		/* TBI Mode Indication */
	SpeedMASK	= 0x000000C0,
	Speed10		= 0x00000000,	/* 10Mb/s */
	Speed100	= 0x00000040,	/* 100Mb/s */
	Speed1000	= 0x00000080,	/* 1000Mb/s */
	Phyra		= 1<<10,	/* PHY Reset Asserted */
	GIOme		= 1<<19,	/* GIO Master Enable Status */
};

enum {					/* Ctrl and Status */
	Fd		= 0x00000001,	/* Full-Duplex */
	AsdvMASK	= 0x00000300,
	Asdv10		= 0x00000000,	/* 10Mb/s */
	Asdv100		= 0x00000100,	/* 100Mb/s */
	Asdv1000	= 0x00000200,	/* 1000Mb/s */
};

enum {					/* Eec */
	Sk		= 1<<0,		/* Clock input to the EEPROM */
	Cs		= 1<<1,		/* Chip Select */
	Di		= 1<<2,		/* Data Input to the EEPROM */
	Do		= 1<<3,		/* Data Output from the EEPROM */
	Areq		= 1<<6,		/* EEPROM Access Request */
	Agnt		= 1<<7,		/* EEPROM Access Grant */
};

enum {					/* Eerd */
	ee_start	= 1<<0,		/* Start Read */
	ee_done		= 1<<1,		/* Read done */
	ee_addr		= 0xfff8<<2,	/* Read address [15:2] */
	ee_data		= 0xffff<<16,	/* Read Data; Data returned from eeprom/nvm */
};

enum {					/* Ctrlext */
	Asdchk		= 1<<12,	/* ASD Check */
	Eerst		= 1<<13,	/* EEPROM Reset */
	Spdbyps		= 1<<15,	/* Speed Select Bypass */
};

enum {					/* EEPROM content offsets */
	Ea		= 0x00,		/* Ethernet Address */
	Cf		= 0x03,		/* Compatibility Field */
	Icw1		= 0x0A,		/* Initialization Control Word 1 */
	Sid		= 0x0B,		/* Subsystem ID */
	Svid		= 0x0C,		/* Subsystem Vendor ID */
	Did		= 0x0D,		/* Device ID */
	Vid		= 0x0E,		/* Vendor ID */
	Icw2		= 0x0F,		/* Initialization Control Word 2 */
};

enum {					/* Mdic */
	MDIdMASK	= 0x0000FFFF,	/* Data */
	MDIdSHIFT	= 0,
	MDIrMASK	= 0x001F0000,	/* PHY Register Address */
	MDIrSHIFT	= 16,
	MDIpMASK	= 0x03E00000,	/* PHY Address */
	MDIpSHIFT	= 21,
	MDIwop		= 0x04000000,	/* Write Operation */
	MDIrop		= 0x08000000,	/* Read Operation */
	MDIready	= 0x10000000,	/* End of Transaction */
	MDIie		= 0x20000000,	/* Interrupt Enable */
	MDIe		= 0x40000000,	/* Error */
};

enum {					/* Icr, Ics, Ims, Imc */
	Txdw		= 0x00000001,	/* Transmit Descriptor Written Back */
	Txqe		= 0x00000002,	/* Transmit Queue Empty */
	Lsc		= 0x00000004,	/* Link Status Change */
	Rxseq		= 0x00000008,	/* Receive Sequence Error */
	Rxdmt0		= 0x00000010,	/* Rdesc Minimum Threshold Reached */
	Rxo		= 0x00000040,	/* Receiver Overrun */
	Rxt0		= 0x00000080,	/* Receiver Timer Interrupt */
	Mdac		= 0x00000200,	/* MDIO Access Completed */
	Rxcfg		= 0x00000400,	/* Receiving /C/ ordered sets */
	Gpi0		= 0x00000800,	/* General Purpose Interrupts */
	Gpi1		= 0x00001000,
	Gpi2		= 0x00002000,
	Gpi3		= 0x00004000,
	Ack		= 0x00020000,	/* receive ACK frame */
};

enum {					/* Txcw */
	TxcwFd		= 0x00000020,	/* Full Duplex */
	TxcwHd		= 0x00000040,	/* Half Duplex */
	TxcwPauseMASK	= 0x00000180,	/* Pause */
	TxcwPauseSHIFT	= 7,
	TxcwPs		= 1<<TxcwPauseSHIFT,	/* Pause Supported */
	TxcwAs		= 2<<TxcwPauseSHIFT,	/* Asymmetric FC desired */
	TxcwRfiMASK	= 0x00003000,	/* Remote Fault Indication */
	TxcwRfiSHIFT	= 12,
	TxcwNpr		= 0x00008000,	/* Next Page Request */
	TxcwConfig	= 0x40000000,	/* Transmit COnfig Control */
	TxcwAne		= 0x80000000,	/* Auto-Negotiation Enable */
};

enum {					/* Rctl */
	Rrst		= 0x00000001,	/* Receiver Software Reset */
	Ren		= 0x00000002,	/* Receiver Enable */
	Sbp		= 0x00000004,	/* Store Bad Packets */
	Upe		= 0x00000008,	/* Unicast Promiscuous Enable */
	Mpe		= 0x00000010,	/* Multicast Promiscuous Enable */
	Lpe		= 0x00000020,	/* Long Packet Reception Enable */
	LbmMASK		= 0x000000C0,	/* Loopback Mode */
	LbmOFF		= 0x00000000,	/* No Loopback */
	LbmTBI		= 0x00000040,	/* TBI Loopback */
	LbmMII		= 0x00000080,	/* GMII/MII Loopback */
	LbmXCVR		= 0x000000C0,	/* Transceiver Loopback */
	RdtmsMASK	= 0x00000300,	/* Rdesc Minimum Threshold Size */
	RdtmsHALF	= 0x00000000,	/* Threshold is 1/2 Rdlen */
	RdtmsQUARTER	= 0x00000100,	/* Threshold is 1/4 Rdlen */
	RdtmsEIGHTH	= 0x00000200,	/* Threshold is 1/8 Rdlen */
	MoMASK		= 0x00003000,	/* Multicast Offset */
	Bam		= 0x00008000,	/* Broadcast Accept Mode */
	BsizeMASK	= 0x00030000,	/* Receive Buffer Size */
	Bsize2048	= 0x00000000,
	Bsize1024	= 0x00010000,
	Bsize512	= 0x00020000,
	Bsize256	= 0x00030000,
	Vfe		= 0x00040000,	/* VLAN Filter Enable */
	Cfien		= 0x00080000,	/* Canonical Form Indicator Enable */
	Cfi		= 0x00100000,	/* Canonical Form Indicator value */
	Dpf		= 0x00400000,	/* Discard Pause Frames */
	Pmcf		= 0x00800000,	/* Pass MAC Control Frames */
	Bsex		= 0x02000000,	/* Buffer Size Extension */
	Secrc		= 0x04000000,	/* Strip CRC from incoming packet */
};

enum {					/* Tctl */
	Trst		= 0x00000001,	/* Transmitter Software Reset */
	Ten		= 0x00000002,	/* Transmit Enable */
	Psp		= 0x00000008,	/* Pad Short Packets */
	Mulr		= 0x10000000,	/* Allow multiple concurrent requests */
	CtMASK		= 0x00000FF0,	/* Collision Threshold */
	CtSHIFT		= 4,
	ColdMASK	= 0x003FF000,	/* Collision Distance */
	ColdSHIFT	= 12,
	Swxoff		= 0x00400000,	/* Sofware XOFF Transmission */
	Pbe		= 0x00800000,	/* Packet Burst Enable */
	Rtlc		= 0x01000000,	/* Re-transmit on Late Collision */
	Nrtu		= 0x02000000,	/* No Re-transmit on Underrrun */
};

enum {					/* [RT]xdctl */
	PthreshMASK	= 0x0000003F,	/* Prefetch Threshold */
	PthreshSHIFT	= 0,
	HthreshMASK	= 0x00003F00,	/* Host Threshold */
	HthreshSHIFT	= 8,
	WthreshMASK	= 0x003F0000,	/* Writebacj Threshold */
	WthreshSHIFT	= 16,
	Gran		= 0x01000000,	/* Granularity */
};

enum {					/* Rxcsum */
	PcssMASK	= 0x000000FF,	/* Packet Checksum Start */
	PcssSHIFT	= 0,
	Ipofl		= 0x00000100,	/* IP Checksum Off-load Enable */
	Tuofl		= 0x00000200,	/* TCP/UDP Checksum Off-load Enable */
};

typedef struct Rdesc {			/* Receive Descriptor */
	uint	addr[2];
	ushort	length;
	ushort	checksum;
	uchar	status;
	uchar	errors;
	ushort	special;
} Rdesc;

enum {					/* Rdesc status */
	Rdd		= 0x01,		/* Descriptor Done */
	Reop		= 0x02,		/* End of Packet */
	Ixsm		= 0x04,		/* Ignore Checksum Indication */
	Vp		= 0x08,		/* Packet is 802.1Q (matched VET) */
	Tcpcs		= 0x20,		/* TCP Checksum Calculated on Packet */
	Ipcs		= 0x40,		/* IP Checksum Calculated on Packet */
	Pif		= 0x80,		/* Passed in-exact filter */
};

enum {					/* Rdesc errors */
	Ce		= 0x01,		/* CRC Error or Alignment Error */
	Se		= 0x02,		/* Symbol Error */
	Seq		= 0x04,		/* Sequence Error */
	Cxe		= 0x10,		/* Carrier Extension Error */
	Tcpe		= 0x20,		/* TCP/UDP Checksum Error */
	Ipe		= 0x40,		/* IP Checksum Error */
	Rxe		= 0x80,		/* RX Data Error */
};

typedef struct Tdesc {			/* Legacy+Normal Transmit Descriptor */
	uint	addr[2];
	uint	control;		/* varies with descriptor type */
	uint	status;			/* varies with descriptor type */
} Tdesc;

enum {					/* Tdesc control */
	LenMASK		= 0x000FFFFF,	/* Data/Packet Length Field */
	LenSHIFT	= 0,
	DtypeCD		= 0x00000000,	/* Data Type 'Context Descriptor' */
	DtypeDD		= 0x00100000,	/* Data Type 'Data Descriptor' */
	PtypeTCP	= 0x01000000,	/* TCP/UDP Packet Type (CD) */
	Teop		= 0x01000000,	/* End of Packet (DD) */
	PtypeIP		= 0x02000000,	/* IP Packet Type (CD) */
	Ifcs		= 0x02000000,	/* Insert FCS (DD) */
	Tse		= 0x04000000,	/* TCP Segmentation Enable */
	Rs		= 0x08000000,	/* Report Status */
	Rps		= 0x10000000,	/* Report Status Sent */
	Dext		= 0x20000000,	/* Descriptor Extension */
	Vle		= 0x40000000,	/* VLAN Packet Enable */
	Ide		= 0x80000000,	/* Interrupt Delay Enable */
};

enum {					/* Tdesc status */
	Tdd		= 0x00000001,	/* Descriptor Done */
	Ec		= 0x00000002,	/* Excess Collisions */
	Lc		= 0x00000004,	/* Late Collision */
	Tu		= 0x00000008,	/* Transmit Underrun */
	CssMASK		= 0x0000FF00,	/* Checksum Start Field */
	CssSHIFT	= 8,
};

enum {
	Nrdesc		= 128,		/* multiple of 8 */
	Ntdesc		= 128,		/* multiple of 8 */
};

enum {
	i82563,
	i82571,
	i82573,
};

static char *tname[] = {
	"i82563",
	"i82571",
	"i82573",
};

#define Type	tname[ctlr->type]

typedef struct Ctlr Ctlr;
struct Ctlr {
	int	port;
	Pcidev	*pcidev;
	Ctlr	*next;
	int	active;
	int	cls;
	ushort	eeprom[0x40];
	uchar	ra[Eaddrlen];		/* receive address */
	int	type;

	int*	nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Lock	slock;
	uint	statistics[Nstatistics];

	Rdesc	*rdba;			/* receive descriptor base address */
	Block	**rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */

	Tdesc	*tdba;			/* transmit descriptor base address */
	Lock	tdlock;
	Block	**tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */

	int	txcw;
	int	fcrtl;
	int	fcrth;

	/* bootstrap goo */
	Block	*bqhead;		/* transmission queue */
	Block	*bqtail;
};

static Ctlr	*ctlrhead;
static Ctlr	*ctlrtail;

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static void
i82563im(Ctlr* ctlr, int im)
{
	ilock(&ctlr->imlock);
	ctlr->im |= im;
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);
}

static void
i82563attach(Ether* edev)
{
	int ctl;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	i82563im(ctlr, 0);
	ctl = csr32r(ctlr, Rctl)|Ren;
	csr32w(ctlr, Rctl, ctl);
	ctl = csr32r(ctlr, Tctl)|Ten;
	csr32w(ctlr, Tctl, ctl);
}


static void
txstart(Ether *edev)
{
	int tdh, tdt;
	Ctlr *ctlr = edev->ctlr;
	Block *bp;
	Tdesc *tdesc;

	/*
	 * Try to fill the ring back up, moving buffers from the transmit q.
	 */
	tdh = PREV(ctlr->tdh, Ntdesc);
	for(tdt = ctlr->tdt; tdt != tdh; tdt = NEXT(tdt, Ntdesc)){
		/* pull off the head of the transmission queue */
		if((bp = ctlr->bqhead) == nil)	/* was qget(edev->oq) */
			break;
		ctlr->bqhead = bp->next;
		if (ctlr->bqtail == bp)
			ctlr->bqtail = nil;

		/* set up a descriptor for it */
		tdesc = &ctlr->tdba[tdt];
		tdesc->addr[0] = PCIWADDR(bp->rp);
		tdesc->addr[1] = 0;
		tdesc->control = /* Ide | */ Rs | Ifcs | Teop | BLEN(bp);

		ctlr->tb[tdt] = bp;
	}
	ctlr->tdt = tdt;
	csr32w(ctlr, Tdt, tdt);
	i82563im(ctlr, Txdw);
}

static Block *
fromringbuf(Ether *ether)
{
	RingBuf *tb = &ether->tb[ether->ti];
	Block *bp = allocb(tb->len);

	memmove(bp->wp, tb->pkt, tb->len);
	memmove(bp->wp+Eaddrlen, ether->ea, Eaddrlen);
	bp->wp += tb->len;
	return bp;
}

static void
i82563transmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	Tdesc *tdesc;
	RingBuf *tb;
	int tdh;

	ctlr = edev->ctlr;
	ilock(&ctlr->tdlock);

	/*
	 * Free any completed packets
	 * - try to get the soft tdh to catch the tdt;
	 * - if the packet had an underrun bump the threshold
	 *   - the Tu bit doesn't seem to ever be set, perhaps
	 *     because Rs mode is used?
	 */
	tdh = ctlr->tdh;
	for(;;){
		tdesc = &ctlr->tdba[tdh];
		if(!(tdesc->status & Tdd))
			break;
		if(ctlr->tb[tdh] != nil){
			freeb(ctlr->tb[tdh]);
			ctlr->tb[tdh] = nil;
		}
		tdesc->status = 0;
		tdh = NEXT(tdh, Ntdesc);
	}
	ctlr->tdh = tdh;

	/* copy packets from the software RingBuf to the transmission q */
	while((tb = &edev->tb[edev->ti])->owner == Interface){
		bp = fromringbuf(edev);
//		print("#l%d: tx %d %E %E\n", edev->ctlrno, edev->ti, bp->rp,
//			bp->rp+6);

		if(ctlr->bqhead)
			ctlr->bqtail->next = bp;
		else
			ctlr->bqhead = bp;
		ctlr->bqtail = bp;

		txstart(edev);		/* kick transmitter */
		tb->owner = Host;	/* give descriptor back */
		edev->ti = NEXT(edev->ti, edev->ntb);
	}
	iunlock(&ctlr->tdlock);
}

static void
i82563replenish(Ctlr* ctlr)
{
	int rdt;
	Block *bp;
	Rdesc *rdesc;

	rdt = ctlr->rdt;
	while(NEXT(rdt, Nrdesc) != ctlr->rdh){
		rdesc = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] != nil){
			/* nothing to do */
		}
		else if((bp = iallocb(2048)) != nil){
			ctlr->rb[rdt] = bp;
			rdesc->addr[0] = PCIWADDR(bp->rp);
			rdesc->addr[1] = 0;
		}
		else
			break;
		rdesc->status = 0;

		rdt = NEXT(rdt, Nrdesc);
	}
	ctlr->rdt = rdt;
	csr32w(ctlr, Rdt, rdt);
}

static void
toringbuf(Ether *ether, Block *bp)
{
	RingBuf *rb = &ether->rb[ether->ri];

	if (rb->owner == Interface) {
		rb->len = BLEN(bp);
		memmove(rb->pkt, bp->rp, rb->len);
		rb->owner = Host;
		ether->ri = NEXT(ether->ri, ether->nrb);
	} else if (debug)
		print("#l%d: toringbuf: dropping packets @ ri %d\n",
			ether->ctlrno, ether->ri);
}

static void
i82563interrupt(Ureg*, void* arg)
{
	int icr, im, rdh, txdw = 0;
	Block *bp;
	Ctlr *ctlr;
	Ether *edev;
	Rdesc *rdesc;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	im = ctlr->im;

	for(icr = csr32r(ctlr, Icr); icr & ctlr->im; icr = csr32r(ctlr, Icr)){
		if(icr & (Rxseq|Lsc)){
			/* should be more here */
		}

		rdh = ctlr->rdh;
		for (;;) {
			rdesc = &ctlr->rdba[rdh];
			if(!(rdesc->status & Rdd))
				break;
			if ((rdesc->status & Reop) && rdesc->errors == 0) {
				bp = ctlr->rb[rdh];
				if(0 && memcmp(bp->rp, broadcast, 6) != 0)
					print("#l%d: rx %d %E %E %d\n",
						edev->ctlrno, rdh, bp->rp,
						bp->rp+6, rdesc->length);
				ctlr->rb[rdh] = nil;
				bp->wp += rdesc->length;
				toringbuf(edev, bp);
				freeb(bp);
			} else if (rdesc->status & Reop && rdesc->errors)
				print("%s: input packet error 0x%ux\n",
					Type, rdesc->errors);
			rdesc->status = 0;
			rdh = NEXT(rdh, Nrdesc);
		}
		ctlr->rdh = rdh;
		if(icr & Rxdmt0)
			i82563replenish(ctlr);
		if(icr & Txdw){
			im &= ~Txdw;
			txdw++;
		}
	}
	ctlr->im = im;
	csr32w(ctlr, Ims, im);
	iunlock(&ctlr->imlock);
	if(txdw)
		i82563transmit(edev);
}

static void
i82563init(Ether* edev)
{
	int csr, i, r;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	csr = edev->ea[3]<<24 | edev->ea[2]<<16 | edev->ea[1]<<8 | edev->ea[0];
	csr32w(ctlr, Ral, csr);
	csr = 0x80000000 | edev->ea[5]<<8 | edev->ea[4];
	csr32w(ctlr, Rah, csr);
	for (i = 1; i < 16; i++) {
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta+i*4, 0);
	csr32w(ctlr, Rctl, 0);
	ctlr->rdba = xspanalloc(Nrdesc*sizeof(Rdesc), 256, 0);
	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, Nrdesc*sizeof(Rdesc));
	ctlr->rdh = 0;
	csr32w(ctlr, Rdh, ctlr->rdh);
	ctlr->rdt = 0;
	csr32w(ctlr, Rdt, ctlr->rdt);
	ctlr->rb = malloc(sizeof(Block*)*Nrdesc);
	i82563replenish(ctlr);
	csr32w(ctlr, Rdtr, 0);
	csr32w(ctlr, Rctl, Dpf | Bsize2048 | Bam | RdtmsHALF);
	i82563im(ctlr, Rxt0 | Rxo | Rxdmt0 | Rxseq | Ack);

	csr32w(ctlr, Tctl, 0x0F<<CtSHIFT | Psp | 0x3f<<ColdSHIFT | Mulr);
	csr32w(ctlr, Tipg, 6<<20 | 8<<10 | 8);
	csr32w(ctlr, Tidv, 1);

	ctlr->tdba = xspanalloc(Ntdesc*sizeof(Tdesc), 256, 0);
	memset(ctlr->tdba, 0, Ntdesc*sizeof(Tdesc));
	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));

	csr32w(ctlr, Tdbah, 0);
	csr32w(ctlr, Tdlen, Ntdesc*sizeof(Tdesc));
	ctlr->tdh = 0;
	csr32w(ctlr, Tdh, ctlr->tdh);
	ctlr->tdt = 0;
	csr32w(ctlr, Tdt, ctlr->tdt);
	ctlr->tb = malloc(sizeof(Block*)*Ntdesc);

//	r = 4<<WthreshSHIFT | 4<<HthreshSHIFT | 8<<PthreshSHIFT;
//	csr32w(ctlr, Txdctl, r);
	csr32w(ctlr, Rxcsum, Tuofl | Ipofl | ETHERHDRSIZE<<PcssSHIFT);
	r = csr32r(ctlr, Tctl);
	r |= Ten;
	csr32w(ctlr, Tctl, r);
}


static ushort
eeread(Ctlr* ctlr, int adr)
{
	csr32w(ctlr, Eerd, ee_start | adr << 2);
	while ((csr32r(ctlr, Eerd) & ee_done) == 0)
		;
	return csr32r(ctlr, Eerd) >> 16;
}

static int
eeload(Ctlr* ctlr)
{
	ushort sum;
	int data, adr;

	sum = 0;
	for (adr = 0; adr < 0x40; adr++) {
		data = eeread(ctlr, adr);
		ctlr->eeprom[adr] = data;
		sum += data;
	}
	return sum;
}


static void
detach(Ctlr *ctlr)
{
	int r;

	csr32w(ctlr, Imc, ~0);
	csr32w(ctlr, Rctl, 0);
	csr32w(ctlr, Tctl, 0);

	delay(10);

	r = csr32r(ctlr, Ctrl);
	csr32w(ctlr, Ctrl, Devrst | r);
	/* apparently needed on multi-GHz processors to avoid infinite loops */
	delay(1);
	while(csr32r(ctlr, Ctrl) & Devrst)
		;

	if(1 || ctlr->type != i82563){
		r = csr32r(ctlr, Ctrl);
		csr32w(ctlr, Ctrl, Slu | r);
	}

	csr32w(ctlr, Ctrlext, Eerst | csr32r(ctlr, Ctrlext));
	delay(1);
	while(csr32r(ctlr, Ctrlext) & Eerst)
		;

	csr32w(ctlr, Imc, ~0);
	delay(1);
	while(csr32r(ctlr, Icr))
		;
}

static void
i82563detach(Ether *edev)
{
	detach(edev->ctlr);
}

static void
i82563shutdown(Ether* ether)
{
	i82563detach(ether);
}

static int
i82563reset(Ctlr* ctlr)
{
	int i, r;

	detach(ctlr);

	r = eeload(ctlr);
	if (r != 0 && r != 0xBABA){
		print("%s: bad EEPROM checksum - 0x%4.4ux\n", Type, r);
		return -1;
	}

	for(i = Ea; i < Eaddrlen/2; i++){
		ctlr->ra[2*i]   = ctlr->eeprom[i];
		ctlr->ra[2*i+1] = ctlr->eeprom[i]>>8;
	}
	r = (csr32r(ctlr, Status) & Lanid) >> 2;
	ctlr->ra[5] += r;		/* ea ctlr[1] = ea ctlr[0]+1 */

	r = ctlr->ra[3]<<24 | ctlr->ra[2]<<16 | ctlr->ra[1]<<8 | ctlr->ra[0];
	csr32w(ctlr, Ral, r);
	r = 0x80000000 | ctlr->ra[5]<<8 | ctlr->ra[4];
	csr32w(ctlr, Rah, r);
	for(i = 1; i < 16; i++){
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}

	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta+i*4, 0);

	csr32w(ctlr, Fcal, 0x00C28001);
	csr32w(ctlr, Fcah, 0x00000100);
	csr32w(ctlr, Fct,  0x00008808);
	csr32w(ctlr, Fcttv, 0x00000100);

	csr32w(ctlr, Fcrtl, ctlr->fcrtl);
	csr32w(ctlr, Fcrth, ctlr->fcrth);

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	ctlr->im = 0;		/* was = Lsc, which hangs some controllers */
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);

	return 0;
}

static void
i82563pci(void)
{
	int port, type, cls;
	Pcidev *p;
	Ctlr *ctlr;
	static int first = 1;

	if (first)
		first = 0;
	else
		return;

	p = nil;
	while(p = pcimatch(p, 0x8086, 0)){
		switch(p->did){
		case 0x1096:
		case 0x10ba:
			type = i82563;
			break;
		case 0x108b:		/*  e */
		case 0x108c:		/*  e (iamt) */
		case 0x109a:		/*  l */
			type = i82573;
			break;
		default:
			continue;
		}

		port = upamalloc(p->mem[0].bar & ~0x0F, p->mem[0].size, 0);
		if(port == 0){
			print("%s: can't map %d @ 0x%8.8lux\n", tname[type],
				p->mem[0].size, p->mem[0].bar);
			continue;
		}

		if(p->pcr & MemWrInv){
			cls = pcicfgr8(p, PciCLS) * 4;
			if(cls != CACHELINESZ)
				pcicfgw8(p, PciCLS, CACHELINESZ/4);
		}

		cls = pcicfgr8(p, PciCLS);
		switch(cls){
		default:
			print("%s: unexpected CLS - %d bytes\n",
				tname[type], cls*sizeof(long));
			break;
		case 0x00:
		case 0xFF:
			/* alphapc 164lx returns 0 */
			print("%s: unusable PciCLS: %d, using %d longs\n",
				tname[type], cls, CACHELINESZ/sizeof(long));
			cls = CACHELINESZ/sizeof(long);
			pcicfgw8(p, PciCLS, cls);
			break;
		case 0x08:
		case 0x10:
			break;
		}

		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = port;
		ctlr->pcidev = p;
		ctlr->cls = cls*4;
		ctlr->type = type;
		ctlr->nic = KADDR(ctlr->port);
		if(i82563reset(ctlr)){
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;
	}
}

static uchar nilea[Eaddrlen];

int
i82563pnp(Ether* edev)
{
	Ctlr *ctlr;

	if(ctlrhead == nil)
		i82563pci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
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
//	edev->mbps = 1000;

	if(memcmp(edev->ea, nilea, Eaddrlen) == 0)
		memmove(edev->ea, ctlr->ra, Eaddrlen);
	i82563init(edev);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = i82563attach;
	edev->transmit = i82563transmit;
	edev->interrupt = i82563interrupt;
	edev->detach = i82563detach;

	/*
	 * with the current structure, there is no right place for this.
	 * ideally, we recognize the interface, note it's down and move on.
	 * currently either we can skip the interface or note it is down,
	 * but not both.
	 */
	if((csr32r(ctlr, Status)&Lu) == 0){
		print("ether#%d: 82563 (%s): link down\n", edev->ctlrno, Type);
		return -1;
	}

	return 0;
}
