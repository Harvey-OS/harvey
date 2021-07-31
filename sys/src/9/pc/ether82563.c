/*
 * Intel 82563 Gigabit Ethernet Controller
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

	Rctl		= 0x00000100,	/* Control */
	Ert		= 0x00002008,	/* Early Receive Threshold (573[EVL] only) */
	Fcrtl		= 0x00002160,	/* Flow Control RX Threshold Low */
	Fcrth		= 0x00002168,	/* Flow Control Rx Threshold High */
	Psrctl		= 0x00002170,	/* Packet Split Receive Control */
	Rdbal		= 0x00002800,	/* Rdesc Base Address Low Queue 0 */
	Rdbah		= 0x00002804,	/* Rdesc Base Address High Queue 0 */
	Rdlen		= 0x00002808,	/* Descriptor Length Queue 0 */
	Rdh		= 0x00002810,	/* Descriptor Head Queue 0 */
	Rdt		= 0x00002818,	/* Descriptor Tail Queue 0 */
	Rdtr		= 0x00002820,	/* Descriptor Timer Ring */
	Rxdctl		= 0x00002828,	/* Descriptor Control */
	Radv		= 0x0000282C,	/* Interrupt Absolute Delay Timer */
	Rdbal1		= 0x00002900,	/* Rdesc Base Address Low Queue 1 */
	Rdbah1		= 0x00002804,	/* Rdesc Base Address High Queue 1 */
	Rdlen1		= 0x00002908,	/* Descriptor Length Queue 1 */
	Rdh1		= 0x00002910,	/* Descriptor Head Queue 1 */
	Rdt1		= 0x00002918,	/* Descriptor Tail Queue 1 */
	Rxdctl1		= 0x00002928,	/* Descriptor Control Queue 1 */
	Rsrpd		= 0x00002c00,	/* Small Packet Detect */
	Raid		= 0x00002c08,	/* ACK interrupt delay */
	Cpuvec		= 0x00002c10,	/* CPU Vector */
	Rxcsum		= 0x00005000,	/* Checksum Control */
	Rfctl		= 0x00005008,	/* Filter Control */
	Mta		= 0x00005200,	/* Multicast Table Array */
	Ral		= 0x00005400,	/* Address Low */
	Rah		= 0x00005404,	/* Address High */
	Vfta		= 0x00005600,	/* VLAN Filter Table Array */
	Mrqc		= 0x00005818,	/* Multiple Receive Queues Command */
	Rssim		= 0x00005864,	/* RSS Interrupt Mask */
	Rssir		= 0x00005868,	/* RSS Interrupt Request */
	Reta		= 0x00005c00,	/* Redirection Table */
	Rssrk		= 0x00005c80,	/* RSS Random Key */

	/* Transmit */

	Tctl		= 0x00000400,	/* Control */
	Tipg		= 0x00000410,	/* IPG */
	Tdbal		= 0x00003800,	/* Tdesc Base Address Low */
	Tdbah		= 0x00003804,	/* Tdesc Base Address High */
	Tdlen		= 0x00003808,	/* Descriptor Length */
	Tdh		= 0x00003810,	/* Descriptor Head */
	Tdt		= 0x00003818,	/* Descriptor Tail */
	Tidv		= 0x00003820,	/* Interrupt Delay Value */
	Txdctl		= 0x00003828,	/* Descriptor Control */
	Tadv		= 0x0000382C,	/* Interrupt Absolute Delay Timer */
	Tarc0		= 0x00003840,	/* Arbitration Counter Queue 0 */
	Tdbal1		= 0x00003900,	/* Descriptor Base Low Queue 1 */
	Tdbah1		= 0x00003904,	/* Descriptor Base High Queue 1 */
	Tdlen1		= 0x00003908,	/* Descriptor Length Queue 1 */
	Tdh1		= 0x00003910,	/* Descriptor Head Queue 1 */
	Tdt1		= 0x00003918,	/* Descriptor Tail Queue 1 */
	Txdctl1		= 0x00003928,	/* Descriptor Control 1 */
	Tarc1		= 0x00003940,	/* Arbitration Counter Queue 1 */

	/* Statistics */

	Statistics	= 0x00004000,	/* Start of Statistics Area */
	Gorcl		= 0x88/4,	/* Good Octets Received Count */
	Gotcl		= 0x90/4,	/* Good Octets Transmitted Count */
	Torl		= 0xC0/4,	/* Total Octets Received */
	Totl		= 0xC8/4,	/* Total Octets Transmitted */
	Nstatistics	= 64,

};

enum {					/* Ctrl */
	GIOmd		= (1<<2),	/* BIO master disable */
	Lrst		= (1<<3),	/* link reset */
	Slu		= (1<<6),	/* Set Link Up */
	SspeedMASK	= (3<<8),	/* Speed Selection */
	SspeedSHIFT	= 8,
	Sspeed10	= 0x00000000,	/* 10Mb/s */
	Sspeed100	= 0x00000100,	/* 100Mb/s */
	Sspeed1000	= 0x00000200,	/* 1000Mb/s */
	Frcspd		= (1<<11),	/* Force Speed */
	Frcdplx		= (1<<12),	/* Force Duplex */
	SwdpinsloMASK	= 0x003C0000,	/* Software Defined Pins - lo nibble */
	SwdpinsloSHIFT	= 18,
	SwdpioloMASK	= 0x03C00000,	/* Software Defined Pins - I or O */
	SwdpioloSHIFT	= 22,
	Devrst		= (1<<26),	/* Device Reset */
	Rfce		= (1<<27),	/* Receive Flow Control Enable */
	Tfce		= (1<<28),	/* Transmit Flow Control Enable */
	Vme		= (1<<30),	/* VLAN Mode Enable */
	Phy_rst		= (1<<31),	/* Phy Reset */
};

enum {					/* Status */
	Lu		= (1<<1),	/* Link Up */
	Lanid		= (3<<2),	/* mask for Lan ID.
	Txoff		= (1<<4),	/* Transmission Paused */
	Tbimode		= (1<<5),	/* TBI Mode Indication */
	SpeedMASK	= 0x000000C0,
	Speed10		= 0x00000000,	/* 10Mb/s */
	Speed100	= 0x00000040,	/* 100Mb/s */
	Speed1000	= 0x00000080,	/* 1000Mb/s */
	Phyra		= (1<<10),	/* PHY Reset Asserted */
	GIOme		= (1<<19),	/* GIO Master Enable Status */
};

enum {					/* Ctrl and Status */
	Fd		= 0x00000001,	/* Full-Duplex */
	AsdvMASK	= 0x00000300,
	Asdv10		= 0x00000000,	/* 10Mb/s */
	Asdv100		= 0x00000100,	/* 100Mb/s */
	Asdv1000	= 0x00000200,	/* 1000Mb/s */
};

enum {					/* Eec */
	Sk		= (1<<0),	/* Clock input to the EEPROM */
	Cs		= (1<<1),	/* Chip Select */
	Di		= (1<<2),	/* Data Input to the EEPROM */
	Do		= (1<<3),	/* Data Output from the EEPROM */
	Areq		= (1<<6),	/* EEPROM Access Request */
	Agnt		= (1<<7),	/* EEPROM Access Grant */
};

enum {					/* Eerd */
	ee_start	= (1<<0),	/* Start Read */
	ee_done		= (1<<1),	/* Read done */
	ee_addr		= (0xfff8<<2),	/* Read address [15:2] */
	ee_data		= (0xffff<<16),	/* Read Data; Data returned from eeprom/nvm */
};

enum {					/* Ctrlext */
	Asdchk		= (1<<12),	/* ASD Check */
	Eerst		= (1<<13),	/* EEPROM Reset */
	Spdbyps		= (1<<15),	/* Speed Select Bypass */
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
};

enum {					/* Txcw */
	TxcwFd		= 0x00000020,	/* Full Duplex */
	TxcwHd		= 0x00000040,	/* Half Duplex */
	TxcwPauseMASK	= 0x00000180,	/* Pause */
	TxcwPauseSHIFT	= 7,
	TxcwPs		= (1<<TxcwPauseSHIFT),	/* Pause Supported */
	TxcwAs		= (2<<TxcwPauseSHIFT),	/* Asymmetric FC desired */
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
	Bsize8192	= 0x00020000, 	/* Bsex = 1 */
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

enum {					/* Receive Delay Timer Ring */
	DelayMASK	= 0x0000FFFF,	/* delay timer in 1.024nS increments */
	DelaySHIFT	= 0,
	Fpd		= 0x80000000,	/* Flush partial Descriptor Block */
};

typedef struct Rd {			/* Receive Descriptor */
	uint	addr[2];
	ushort	length;
	ushort	checksum;
	uchar	status;
	uchar	errors;
	ushort	special;
} Rd;

enum {					/* Rd status */
	Rdd		= 0x01,		/* Descriptor Done */
	Reop		= 0x02,		/* End of Packet */
	Ixsm		= 0x04,		/* Ignore Checksum Indication */
	Vp		= 0x08,		/* Packet is 802.1Q (matched VET) */
	Tcpcs		= 0x20,		/* TCP Checksum Calculated on Packet */
	Ipcs		= 0x40,		/* IP Checksum Calculated on Packet */
	Pif		= 0x80,		/* Passed in-exact filter */
};

enum {					/* Rd errors */
	Ce		= 0x01,		/* CRC Error or Alignment Error */
	Se		= 0x02,		/* Symbol Error */
	Seq		= 0x04,		/* Sequence Error */
	Cxe		= 0x10,		/* Carrier Extension Error */
	Tcpe		= 0x20,		/* TCP/UDP Checksum Error */
	Ipe		= 0x40,		/* IP Checksum Error */
	Rxe		= 0x80,		/* RX Data Error */
};

typedef struct Td Td;
struct Td {			/* Transmit Descriptor */
	uint	addr[2];	/* Data */
	uint	control;
	uint	status;
};

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
	Nrd		= 256,		/* multiple of 8 */
	Ntd		= 64,		/* multiple of 8 */
	Nrb		= 1024,		/* private receive buffers per Ctlr */
	Rbsz		= 8192,
};

typedef struct Ctlr Ctlr;
struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	started;
	int	id;
	int	cls;
	ushort	eeprom[0x40];

	QLock	alock;			/* attach */
	void*	alloc;			/* receive/transmit descriptors */
	int	nrd;
	int	ntd;
	int	nrb;			/* how many this Ctlr has in the pool */

	int*	nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Rendez	lrendez;
	int	lim;

	int	link;

	QLock	slock;
	uint	statistics[Nstatistics];
	uint	lsleep;
	uint	lintr;
	uint	rsleep;
	uint	rintr;
	uint	txdw;
	uint	tintr;
	uint	ixsm;
	uint	ipcs;
	uint	tcpcs;

	uchar	ra[Eaddrlen];		/* receive address */
	ulong	mta[128];		/* multicast table array */

	Rendez	rrendez;
	int	rim;
	int	rdfree;
	Rd*	rdba;			/* receive descriptor base address */
	Block**	rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */
	int	rdtr;			/* receive delay timer ring value */
	int	radv;			/* receive interrupt absolute delay timer */

	Lock	tlock;
	int	tbusy;
	int	tdfree;
	Td*	tdba;			/* transmit descriptor base address */
	Block**	tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */

	int	txcw;
	int	fcrtl;
	int	fcrth;
};

enum{
	Easize	= 6,
	Maxmac	= 16,
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr* i82563ctlrhead;
static Ctlr* i82563ctlrtail;

static Lock i82563rblock;		/* free receive Blocks */
static Block* i82563rbpool;

static char* statistics[Nstatistics] = {
	"CRC Error",
	"Alignment Error",
	"Symbol Error",
	"RX Error",
	"Missed Packets",
	"Single Collision",
	"Excessive Collisions",
	"Multiple Collision",
	"Late Collisions",
	nil,
	"Collision",
	"Transmit Underrun",
	"Defer",
	"Transmit - No CRS",
	"Sequence Error",
	"Carrier Extension Error",
	"Receive Error Length",
	nil,
	"XON Received",
	"XON Transmitted",
	"XOFF Received",
	"XOFF Transmitted",
	"FC Received Unsupported",
	"Packets Received (64 Bytes)",
	"Packets Received (65-127 Bytes)",
	"Packets Received (128-255 Bytes)",
	"Packets Received (256-511 Bytes)",
	"Packets Received (512-1023 Bytes)",
	"Packets Received (1024-1522 Bytes)",
	"Good Packets Received",
	"Broadcast Packets Received",
	"Multicast Packets Received",
	"Good Packets Transmitted",
	nil,
	"Good Octets Received",
	nil,
	"Good Octets Transmitted",
	nil,
	nil,
	nil,
	"Receive No Buffers",
	"Receive Undersize",
	"Receive Fragment",
	"Receive Oversize",
	"Receive Jabber",
	nil,
	nil,
	nil,
	"Total Octets Received",
	nil,
	"Total Octets Transmitted",
	nil,
	"Total Packets Received",
	"Total Packets Transmitted",
	"Packets Transmitted (64 Bytes)",
	"Packets Transmitted (65-127 Bytes)",
	"Packets Transmitted (128-255 Bytes)",
	"Packets Transmitted (256-511 Bytes)",
	"Packets Transmitted (512-1023 Bytes)",
	"Packets Transmitted (1024-1522 Bytes)",
	"Multicast Packets Transmitted",
	"Broadcast Packets Transmitted",
	"TCP Segmentation Context Transmitted",
	"TCP Segmentation Context Fail",
};

static long
i82563ifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *p, *s;
	int i, l, r;
	uvlong tuvl, ruvl;

	ctlr = edev->ctlr;
	qlock(&ctlr->slock);
	p = malloc(2*READSTR);
	l = 0;
	for(i = 0; i < Nstatistics; i++){
		r = csr32r(ctlr, Statistics+i*4);
		if((s = statistics[i]) == nil)
			continue;
		switch(i){
		case Gorcl:
		case Gotcl:
		case Torl:
		case Totl:
			ruvl = r;
			ruvl += ((uvlong)csr32r(ctlr, Statistics+(i+1)*4))<<32;
			tuvl = ruvl;
			tuvl += ctlr->statistics[i];
			tuvl += ((uvlong)ctlr->statistics[i+1])<<32;
			if(tuvl == 0)
				continue;
			ctlr->statistics[i] = tuvl;
			ctlr->statistics[i+1] = tuvl>>32;
			l += snprint(p+l, 2*READSTR-l, "%s: %llud %llud\n",
				s, tuvl, ruvl);
			i++;
			break;

		default:
			ctlr->statistics[i] += r;
			if(ctlr->statistics[i] == 0)
				continue;
			l += snprint(p+l, 2*READSTR-l, "%s: %ud %ud\n",
				s, ctlr->statistics[i], r);
			break;
		}
	}

	l += snprint(p+l, 2*READSTR-l, "lintr: %ud %ud\n",
		ctlr->lintr, ctlr->lsleep);
	l += snprint(p+l, 2*READSTR-l, "rintr: %ud %ud\n",
		ctlr->rintr, ctlr->rsleep);
	l += snprint(p+l, 2*READSTR-l, "tintr: %ud %ud\n",
		ctlr->tintr, ctlr->txdw);
	l += snprint(p+l, 2*READSTR-l, "ixcs: %ud %ud %ud\n",
		ctlr->ixsm, ctlr->ipcs, ctlr->tcpcs);
	l += snprint(p+l, 2*READSTR-l, "rdtr: %ud\n", ctlr->rdtr);
	l += snprint(p+l, 2*READSTR-l, "radv: %ud\n", ctlr->radv);
	l += snprint(p+l, 2*READSTR-l, "Ctrlext: %08x\n", csr32r(ctlr, Ctrlext));

	l += snprint(p+l, 2*READSTR-l, "eeprom:");
	for(i = 0; i < 0x40; i++){
		if(i && ((i & 0x07) == 0))
			l += snprint(p+l, 2*READSTR-l, "\n       ");
		l += snprint(p+l, 2*READSTR-l, " %4.4uX", ctlr->eeprom[i]);
	}
	snprint(p+l, 2*READSTR-l, "\n");
	n = readstr(offset, a, n, p);
	free(p);
	qunlock(&ctlr->slock);

	return n;
}

enum {
	CMrdtr,
	CMradv,
};

static Cmdtab i82563ctlmsg[] = {
	CMrdtr,	"rdtr",	2,
	CMradv,	"radv",	2,
};

static long
i82563ctl(Ether* edev, void* buf, long n)
{
	int v;
	char *p;
	Ctlr *ctlr;
	Cmdbuf *cb;
	Cmdtab *ct;

	if((ctlr = edev->ctlr) == nil)
		error(Enonexist);

	cb = parsecmd(buf, n);
	if(waserror()){
		free(cb);
		nexterror();
	}

	ct = lookupcmd(cb, i82563ctlmsg, nelem(i82563ctlmsg));
	switch(ct->index){
	case CMrdtr:
		v = strtol(cb->f[1], &p, 0);
		if(v < 0 || p == cb->f[1] || v > 0xFFFF)
			error(Ebadarg);
		ctlr->rdtr = v;
		csr32w(ctlr, Rdtr, Fpd|v);
		break;
	case CMradv:
		v = strtol(cb->f[1], &p, 0);
		if(v < 0 || p == cb->f[1] || v > 0xFFFF)
			error(Ebadarg);
		ctlr->radv = v;
		csr32w(ctlr, Radv, v);
	}
	free(cb);
	poperror();

	return n;
}

static void
i82563promiscuous(void* arg, int on)
{
	int rctl;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	rctl = csr32r(ctlr, Rctl);
	rctl &= ~MoMASK;
	if(on)
		rctl |= Upe|Mpe;
	else
		rctl &= ~(Upe|Mpe);
	csr32w(ctlr, Rctl, rctl);
}

static void
i82563multicast(void* arg, uchar* addr, int on)
{
	int bit, x;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	x = addr[5]>>1;
	bit = ((addr[5] & 1)<<4)|(addr[4]>>4);
	if(on)
		ctlr->mta[x] |= 1<<bit;
	else
		ctlr->mta[x] &= ~(1<<bit);

	csr32w(ctlr, Mta+x*4, ctlr->mta[x]);
}

static Block*
i82563rballoc(void)
{
	Block *bp;

	ilock(&i82563rblock);
	if((bp = i82563rbpool) != nil){
		i82563rbpool = bp->next;
		bp->next = nil;
	}
	iunlock(&i82563rblock);

	return bp;
}

static void
i82563rbfree(Block* bp)
{
	bp->rp = bp->lim - Rbsz;
	bp->wp = bp->rp;

	ilock(&i82563rblock);
	bp->next = i82563rbpool;
	i82563rbpool = bp;
	iunlock(&i82563rblock);
}

static void
i82563im(Ctlr* ctlr, int im)
{
	ilock(&ctlr->imlock);
	ctlr->im |= im;
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);
}

static void
i82563txinit(Ctlr* ctlr)
{
	int i, r;
	Block *bp;

	csr32w(ctlr, Tctl, (0x0F<<CtSHIFT)|Psp|(66<<ColdSHIFT));
	csr32w(ctlr, Tipg, (6<<20)|(8<<10)|8);
	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));
	csr32w(ctlr, Tdbah, 0);
	csr32w(ctlr, Tdlen, ctlr->ntd*sizeof(Td));
	ctlr->tdh = PREV(0, ctlr->ntd);
	csr32w(ctlr, Tdh, 0);
	ctlr->tdt = 0;
	csr32w(ctlr, Tdt, 0);
	for(i = 0; i < ctlr->ntd; i++){
		if((bp = ctlr->tb[i]) != nil){
			ctlr->tb[i] = nil;
			freeb(bp);
		}
		memset(&ctlr->tdba[i], 0, sizeof(Td));
	}
	ctlr->tdfree = ctlr->ntd;
	csr32w(ctlr, Tidv, 128);
	r = csr32r(ctlr, Txdctl);
	r &= ~WthreshMASK;
	r |= Gran|(4<<WthreshSHIFT);
	csr32w(ctlr, Tadv, 64);
	csr32w(ctlr, Txdctl, r);
	r = csr32r(ctlr, Tctl);
	r |= Ten;
	csr32w(ctlr, Tctl, r);
}

static void
i82563transmit(Ether* edev)
{
	Td *td;
	Block *bp;
	Ctlr *ctlr;
	int tdh, tdt, ctdh;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);

	/*
	 * Free any completed packets
	 */
	tdh = ctlr->tdh;
	ctdh = csr32r(ctlr, Tdh);
	while(NEXT(tdh, ctlr->ntd) != ctdh){
		if((bp = ctlr->tb[tdh]) != nil){
			ctlr->tb[tdh] = nil;
			freeb(bp);
		}
		memset(&ctlr->tdba[tdh], 0, sizeof(Td));
		tdh = NEXT(tdh, ctlr->ntd);
	}
	ctlr->tdh = tdh;

	/*
	 * Try to fill the ring back up.
	 */
	tdt = ctlr->tdt;
	while(NEXT(tdt, ctlr->ntd) != tdh){
		if((bp = qget(edev->oq)) == nil)
			break;
		td = &ctlr->tdba[tdt];
		td->addr[0] = PCIWADDR(bp->rp);
		td->control = ((BLEN(bp) & LenMASK)<<LenSHIFT);
		td->control |= Ifcs|Teop|DtypeDD;
		ctlr->tb[tdt] = bp;
		tdt = NEXT(tdt, ctlr->ntd);
		ctlr->tdt = tdt;
		if(NEXT(tdt, ctlr->ntd) == tdh){
			td->control |= Rs;
			ctlr->txdw++;
			i82563im(ctlr, Txdw);
			break;
		}
	}
	csr32w(ctlr, Tdt, tdt);
	iunlock(&ctlr->tlock);
}

static void
i82563replenish(Ctlr* ctlr)
{
	Rd *rd;
	int rdt;
	Block *bp;

	rdt = ctlr->rdt;
	while(NEXT(rdt, ctlr->nrd) != ctlr->rdh){
		rd = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] == nil){
			bp = i82563rballoc();
			if(bp == nil){
				iprint("no available buffers\n");
				break;
			}
			ctlr->rb[rdt] = bp;
			rd->addr[0] = PCIWADDR(bp->rp);
			rd->addr[1] = 0;
		}
		rd->status = 0;
		rdt = NEXT(rdt, ctlr->nrd);
		ctlr->rdfree++;
	}
	ctlr->rdt = rdt;
	csr32w(ctlr, Rdt, rdt);
}

static void
i82563rxinit(Ctlr* ctlr)
{
	int i;
	Block *bp;

//	csr32w(ctlr, Rctl, Dpf|Bsize2048|Bam|RdtmsHALF);
//	csr32w(ctlr, Rctl, Lpe|Dpf|Bsize16384|Bam|RdtmsHALF|Bsex|Secrc);
	csr32w(ctlr, Rctl, Lpe|Dpf|Bsize8192|Bam|RdtmsHALF|Bsex|Secrc);

	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, ctlr->nrd*sizeof(Rd));
	ctlr->rdh = 0;
	csr32w(ctlr, Rdh, 0);
	ctlr->rdt = 0;
	csr32w(ctlr, Rdt, 0);
	ctlr->rdtr = 0;
	ctlr->radv = 0;
	csr32w(ctlr, Rdtr, Fpd|0);
	csr32w(ctlr, Radv, 0);

	for(i = 0; i < ctlr->nrd; i++){
		if((bp = ctlr->rb[i]) != nil){
			ctlr->rb[i] = nil;
			freeb(bp);
		}
	}
	i82563replenish(ctlr);
	csr32w(ctlr, Radv, 64);
//	csr32w(ctlr, Rxdctl, (8<<WthreshSHIFT)|(8<<HthreshSHIFT)|4);
//	csr32w(ctlr, Rxdctl, (0<<WthreshSHIFT)|(0<<HthreshSHIFT)|0);
	csr32w(ctlr, Rxdctl, (1<<16)|(1<<24));

	/*
	 * Enable checksum offload.
	 */
	csr32w(ctlr, Rxcsum, Tuofl|Ipofl|(ETHERHDRSIZE<<PcssSHIFT));
}

static void
i82563rcv(Ether *edev)
{
	Rd *rd;
	Block *bp;
	int rdh;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	rdh = ctlr->rdh;
	for(;;){
		rd = &ctlr->rdba[rdh];

		if(!(rd->status & Rdd))
			break;

		/*
		 * Accept eop packets with no errors.
		 * With no errors and the Ixsm bit set,
		 * the descriptor status Tpcs and Ipcs bits give
		 * an indication of whether the checksums were
		 * calculated and valid.
		 */
		if (bp = ctlr->rb[rdh]) {
			if((rd->status & Reop) && rd->errors == 0){
				bp->wp += rd->length;
				bp->next = nil;
				if(!(rd->status & Ixsm)){
					ctlr->ixsm++;
					if(rd->status & Ipcs){
						/*
						 * IP checksum calculated
						 * (and valid as errors == 0).
						 */
						ctlr->ipcs++;
						bp->flag |= Bipck;
					}
					if(rd->status & Tcpcs){
						/*
						 * TCP/UDP checksum calculated
						 * (and valid as errors == 0).
						 */
						ctlr->tcpcs++;
						bp->flag |= Btcpck|Budpck;
					}
					bp->checksum = rd->checksum;
					bp->flag |= Bpktck;
				}
				etheriq(edev, bp, 1);
			} else
				freeb(bp);
			ctlr->rb[rdh] = nil;
		}
		memset(rd, 0, sizeof(Rd));
		ctlr->rdfree--;
		ctlr->rdh = rdh = NEXT(rdh, ctlr->nrd);
		coherence();
		if(ctlr->rdfree < (ctlr->nrd/4)*3 || (ctlr->rim & Rxdmt0))
			i82563replenish(ctlr);
	}
}

static int
i82563rim(void* ctlr)
{
	return ((Ctlr*)ctlr)->rim != 0;
}

static void
i82563rproc(void* arg)
{
	Rd *rd;
	Block *bp;
	Ctlr *ctlr;
	int r, rdh, rim;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	i82563rxinit(ctlr);
	r = csr32r(ctlr, Rctl);
	r |= Ren;
	csr32w(ctlr, Rctl, r);

	for(;;){
		i82563im(ctlr, Rxt0|Rxo|Rxdmt0|Rxseq);
		ctlr->rsleep++;
		coherence();
		sleep(&ctlr->rrendez, i82563rim, ctlr);

		rdh = ctlr->rdh;
		for(;;){
			rd = &ctlr->rdba[rdh];
			rim = ctlr->rim;
			ctlr->rim = 0;
			if(!(rd->status & Rdd))
				break;

			/*
			 * Accept eop packets with no errors.
			 * With no errors and the Ixsm bit set,
			 * the descriptor status Tpcs and Ipcs bits give
			 * an indication of whether the checksums were
			 * calculated and valid.
			 */
			if (bp = ctlr->rb[rdh]) {
				if((rd->status & Reop) && rd->errors == 0){
					bp->wp += rd->length;
					bp->next = nil;
					if(!(rd->status & Ixsm)){
						ctlr->ixsm++;
						if(rd->status & Ipcs){
							/*
							 * IP checksum calculated
							 * (and valid as errors == 0).
							 */
							ctlr->ipcs++;
							bp->flag |= Bipck;
						}
						if(rd->status & Tcpcs){
							/*
							 * TCP/UDP checksum calculated
							 * (and valid as errors == 0).
							 */
							ctlr->tcpcs++;
							bp->flag |= Btcpck|Budpck;
						}
						bp->checksum = rd->checksum;
						bp->flag |= Bpktck;
					}
					etheriq(edev, bp, 1);
				} else
					freeb(bp);
				ctlr->rb[rdh] = nil;
			}
			memset(rd, 0, sizeof(Rd));
			ctlr->rdfree--;
			ctlr->rdh = rdh = NEXT(rdh, ctlr->nrd);
			coherence();
			if(ctlr->rdfree < (ctlr->nrd/4)*3 || (rim & Rxdmt0))
				i82563replenish(ctlr);
		}
	}
}

static void
i82563attach(Ether* edev)
{
	Block *bp, *fbp;
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){
		qunlock(&ctlr->alock);
		return;
	}

	ctlr->nrd = ROUND(Nrd, 8);
	ctlr->ntd = ROUND(Ntd, 8);
	ctlr->alloc = malloc(ctlr->nrd*sizeof(Rd)+ctlr->ntd*sizeof(Td) + 255);
	if(ctlr->alloc == nil){
		qunlock(&ctlr->alock);
		return;
	}
	ctlr->rdba = (Rd*)ROUNDUP((ulong)ctlr->alloc, 256);
	ctlr->tdba = (Td*)(ctlr->rdba+ctlr->nrd);

	ctlr->rb = malloc(ctlr->nrd*sizeof(Block*));
	ctlr->tb = malloc(ctlr->ntd*sizeof(Block*));

	if(waserror()){
		while(ctlr->nrb > 0){
			bp = i82563rballoc();
			bp->free = nil;
			freeb(bp);
			ctlr->nrb--;
		}
		free(ctlr->tb);
		ctlr->tb = nil;
		free(ctlr->rb);
		ctlr->rb = nil;
		free(ctlr->alloc);
		ctlr->alloc = nil;
		qunlock(&ctlr->alock);
		nexterror();
	}

	fbp = nil;
	for(ctlr->nrb = 0; ctlr->nrb < Nrb; ){
		if((bp = allocb(Rbsz)) == nil)
			break;
		if (((ulong)bp->base ^ (ulong)bp->lim) & ~0xffff) {
			bp->next = fbp;
			fbp = bp;
			continue;
		}
		bp->free = i82563rbfree;
		freeb(bp);
		ctlr->nrb++;
	}
	freeblist(fbp);

	snprint(name, KNAMELEN, "#l%drproc", edev->ctlrno);
	kproc(name, i82563rproc, edev);

	i82563txinit(ctlr);

	qunlock(&ctlr->alock);
	poperror();
}

static void
i82563interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	int icr, im, txdw;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	im = ctlr->im;
	txdw = 0;

	while(icr = csr32r(ctlr, Icr) & ctlr->im){
		if(icr & Lsc){
			im &= ~Lsc;
			ctlr->lim = icr & Lsc;
			wakeup(&ctlr->lrendez);
			ctlr->lintr++;
		}
		if(icr & (Rxt0|Rxo|Rxdmt0|Rxseq)){
			ctlr->rim = icr & (Rxt0|Rxo|Rxdmt0|Rxseq);
			im &= ~(Rxt0|Rxo|Rxdmt0|Rxseq);
			wakeup(&ctlr->rrendez);
//			i82563rcv(edev);
			ctlr->rintr++;
		}
		if(icr & Txdw){
			im &= ~Txdw;
			txdw++;
			ctlr->tintr++;
		}
	}

	ctlr->im = im;
	csr32w(ctlr, Ims, im);
	iunlock(&ctlr->imlock);

	if(txdw)
		i82563transmit(edev);
}

static int
i82563detach(Ctlr* ctlr)
{
	int r, timeo;

	/*
	 * Perform a device reset to get the chip back to the
	 * power-on state, followed by an EEPROM reset to read
	 * the defaults for some internal registers.
	 */
	csr32w(ctlr, Imc, ~0);
	csr32w(ctlr, Rctl, 0);
	csr32w(ctlr, Tctl, 0);

	delay(10);

	csr32w(ctlr, Ctrl, Devrst);
	delay(1);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(ctlr, Ctrl) & Devrst))
			break;
		delay(1);
	}
	if(csr32r(ctlr, Ctrl) & Devrst)
		return -1;
	r = csr32r(ctlr, Ctrlext);
	csr32w(ctlr, Ctrlext, r|Eerst);
	delay(1);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(ctlr, Ctrlext) & Eerst))
			break;
		delay(1);
	}
	if(csr32r(ctlr, Ctrlext) & Eerst)
		return -1;

	csr32w(ctlr, Imc, ~0);
	delay(1);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!csr32r(ctlr, Icr))
			break;
		delay(1);
	}
	if(csr32r(ctlr, Icr))
		return -1;

	return 0;
}

static void
i82563shutdown(Ether* ether)
{
	i82563detach(ether->ctlr);
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

/*
 * kind of unnecessary;
 * but just in case they add 4 or 16 macs to the same ctlr.
 */
static uchar*
etheradd(uchar *u, uint n)
{
	int i;
	uint j;

	for(i = 5; n != 0 && i >= 0; i--){
		j = n+u[i];
		u[i] = j;
		n = j>>8;
	}
	return u;
}

typedef struct {
	uchar	ea[Easize];
	int	n;
} Basetab;

static Basetab btab[Maxmac];
static int nbase;

int
nthether(uchar *ea)
{
	int i;

	for(i = 0; i < nelem(btab); i++)
		if(btab[i].n == 0 || memcmp(btab[i].ea, ea, Easize) == 0) {
			memmove(btab[i].ea, ea, Easize);
			return btab[i].n++;
		}
	return -1;
}

static int
i82563reset(Ctlr* ctlr)
{
	int i, r;

	if(i82563detach(ctlr))
		return -1;
	r = eeload(ctlr);
	if (r != 0 && r != 0xBABA){
		print("i82563: bad EEPROM checksum - 0x%4.4uX\n", r);
		return -1;
	}

	for(i = Ea; i < Eaddrlen/2; i++){
		ctlr->ra[2*i] = ctlr->eeprom[i];
		ctlr->ra[2*i+1] = ctlr->eeprom[i]>>8;
	}
	etheradd(ctlr->ra, nthether(ctlr->ra));
	r = (ctlr->ra[3]<<24)|(ctlr->ra[2]<<16)|(ctlr->ra[1]<<8)|ctlr->ra[0];
	csr32w(ctlr, Ral, r);
	r = 0x80000000|(ctlr->ra[5]<<8)|ctlr->ra[4];
	csr32w(ctlr, Rah, r);
	for(i = 1; i < 16; i++){
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta+i*4, 0);
	csr32w(ctlr, Fcal, 0x00C28001);
	csr32w(ctlr, Fcah, 0x00000100);
	csr32w(ctlr, Fct, 0x00008808);
	csr32w(ctlr, Fcttv, 0x00000100);
	csr32w(ctlr, Fcrtl, ctlr->fcrtl);
	csr32w(ctlr, Fcrth, ctlr->fcrth);
	return 0;
}

static void
i82563pci(void)
{
	int cls;
	Pcidev *p;
	Ctlr *ctlr;
	ulong io;
	void *mem;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != 0)
			continue;
		if (p->vid != 0x8086 || p->did != 0x1096)
			continue;

		io = p->mem[0].bar & ~0x0F;
		mem = vmap(io, p->mem[0].size);
		if(mem == nil){
			print("i82563: can't map %8.8luX\n", p->mem[0].bar);
			continue;
		}
		cls = pcicfgr8(p, PciCLS);
		switch(cls){
			default:
				print("i82563: unexpected CLS - %d\n", cls*4);
				break;
			case 0x00:
			case 0xFF:
				print("i82563: unusable CLS\n");
				continue;
			case 0x08:
			case 0x10:
				break;
		}
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = io;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;
		ctlr->cls = cls*4;
		ctlr->nic = mem;

		if(i82563reset(ctlr)){
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(i82563ctlrhead != nil)
			i82563ctlrtail->next = ctlr;
		else
			i82563ctlrhead = ctlr;
		i82563ctlrtail = ctlr;
	}
}

static int
i82563pnp(Ether* edev)
{
	Ctlr *ctlr;

	if(i82563ctlrhead == nil)
		i82563pci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = i82563ctlrhead; ctlr != nil; ctlr = ctlr->next)
		if(ctlr->active)
			continue;
		else if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->maxmtu = Rbsz;
	memmove(edev->ea, ctlr->ra, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = i82563attach;
	edev->transmit = i82563transmit;
	edev->interrupt = i82563interrupt;
	edev->ifstat = i82563ifstat;
	edev->ctl = i82563ctl;

	edev->arg = edev;
	edev->promiscuous = i82563promiscuous;
	edev->shutdown = i82563shutdown;
	edev->multicast = i82563multicast;

	return 0;
}

void
ether82563link(void)
{
	addethercard("i82563", i82563pnp);
}
