/*
 * Intel 82563 Gigabit Ethernet Controller
 */
#include "all.h"
#include "io.h"
#include "../ip/ip.h"
#include "etherif.h"
#include "portfns.h"
#include "mem.h"

extern ulong upamalloc(ulong, int, int);

#define PCIWADDR(x)	PADDR(x)+0
#define	ROUND(s, sz)	(((s)+((sz)-1)) & ~((sz)-1))

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
	Kumctrlsta	= 0x00000034,	/* Kumeran Control and Status Register */
	Vet		= 0x00000038,	/* VLAN EtherType */
	Fcttv		= 0x00000170,	/* Flow Control Transmit Timer Value */
	Txcw		= 0x00000178,	/* Transmit Configuration Word */
	Rxcw		= 0x00000180,	/* Receive Configuration Word */
	Ledctl		= 0x00000E00,	/* LED control */
	Pba		= 0x00001000,	/* Packet Buffer Allocation */

	/* Interrupt */

	Icr		= 0x000000C0,	/* Interrupt Cause Read */
	Ics		= 0x000000C8,	/* Interrupt Cause csr32w */
	Ims		= 0x000000D0,	/* Interrupt Mask csr32w/Read */
	Imc		= 0x000000D8,	/* Interrupt mask Clear */
	Iam		= 0x000000E0,	/* Interrupt acknowledge Auto Mask */

	/* Receive */

	Rctl		= 0x00000100,	/* Control */
	Ert		= 0x00002008,	/* Early Receive Threshold (573[EVL] only) */
	Fcrtl		= 0x00002160,	/* Flow Control Rx Threshold Low */
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
	Slu		= (1<<6),	/* csr32w Link Up */
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
	xIcw1		= 0x0A,		/* Initialization Control Word 1 */
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
	Rbsz		= ETHERMAXTU,
};

typedef struct{
	int	port;
	Pcidev*	pcidev;
	int	active;
	int	started;
	int	id;
	int	cls;
	ushort	eeprom[0x40];

	void*	alloc;			/* receive/transmit descriptors */
	int	nrd;
	int	ntd;
	int	nrb;			/* how many this Ctlr has in the pool */

	int*	nic;
	int	im;			/* interrupt mask */

	int	lrendez;
	int	lim;

	int	link;

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

	uchar	ra[Easize];		/* receive address */
	ulong	mta[128];		/* multicast table array */

	Rendez	txrendez;
	Rendez	rxrendez;

	Lock	imlock;
	Lock	txlock;

	int	rim;
	int	rdfree;
	Rd*	rdba;			/* receive descriptor base address */
	Msgbuf	**rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */
	int	rdtr;			/* receive delay timer ring value */
	int	radv;			/* receive interrupt absolute delay timer */

	int	tbusy;
	int	tdfree;
	Td*	tdba;			/* transmit descriptor base address */
	Msgbuf	**tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */

	int	txcw;
	int	fcrtl;
	int	fcrth;

	Queue	*reply;			/* alias of ifc->reply */
	Filter	rate;
	Filter	work;
}Ctlr;

enum{
	Nether	= 8,
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr	ports[Nether];
static int	nports;

static Lock i82563rblock;		/* free receive Blocks */
static Msgbuf *i82563rbpool;

#ifdef notdef
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

static void
i82563ifstat(Ctlr *c)
{
	char *s;
	int i, r;
	uvlong tuvl, ruvl;

	for(i = 0; i < Nstatistics; i++){
		r = csr32r(c, Statistics+i*4);
		if((s = statistics[i]) == nil)
			continue;
		switch(i){
		case Gorcl:
		case Gotcl:
		case Torl:
		case Totl:
			ruvl = r;
			ruvl += ((uvlong)csr32r(c, Statistics+(i+1)*4))<<32;
			tuvl = ruvl;
			tuvl += c->statistics[i];
			tuvl += ((uvlong)c->statistics[i+1])<<32;
			if(tuvl == 0)
				continue;
			c->statistics[i] = tuvl;
			c->statistics[i+1] = tuvl>>32;
			print("%s: %llud %llud\n", s, tuvl, ruvl);
			i++;
			break;

		default:
			c->statistics[i] += r;
			if(c->statistics[i] == 0)
				continue;
			print("%s: %ud %ud\n", s, c->statistics[i], r);
			break;
		}
	}

	print("lintr: %ud %ud\n", c->lintr, c->lsleep);
	print("rintr: %ud %ud\n", c->rintr, c->rsleep);
	print("tintr: %ud %ud\n", c->tintr, c->txdw);
	print("ixcs: %ud %ud %ud\n", c->ixsm, c->ipcs, c->tcpcs);
	print("rdtr: %ud\n", c->rdtr);
	print("radv: %ud\n", c->radv);
	print("Ctrlext: %08x\n", csr32r(c, Ctrlext));

	print("eeprom:");
	for(i = 0; i < 0x40; i++){
		if(i && ((i & 0x07) == 0))
			print("\n       ");
		print(" %4.4uX", c->eeprom[i]);
	}
}
#endif

static Msgbuf*
i82563rballoc(void)
{
	Msgbuf *m;

	ilock(&i82563rblock);
	if((m = i82563rbpool) != nil){
		i82563rbpool = m->next;
		m->next = nil;
	}
	iunlock(&i82563rblock);
	m->flags &= ~FREE;
	m->count = 0;
	m->data = (uchar*)PGROUND((uintptr)m->xdata);
	return m;
}

static void
i82563rbfree(Msgbuf *m)
{
	m->flags |= FREE;
	ilock(&i82563rblock);
	m->next = i82563rbpool;
	i82563rbpool = m;
	iunlock(&i82563rblock);
}

static void
i82563im(Ctlr* c, int im)
{
	ilock(&c->imlock);
	c->im |= im;
	csr32w(c, Ims, c->im);
	iunlock(&c->imlock);
}

static void
i82563txinit(Ctlr* c)
{
	int i, r;
	Msgbuf *m;

	csr32w(c, Tctl, 0x0F<<CtSHIFT | Psp | 66<<ColdSHIFT);
	csr32w(c, Tipg, 6<<20 | 8<<10 | 8);
	csr32w(c, Tdbal, PCIWADDR(c->tdba));
	csr32w(c, Tdbah, 0);
	csr32w(c, Tdlen, c->ntd*sizeof(Td));
	c->tdh = PREV(0, c->ntd);
	csr32w(c, Tdh, 0);
	c->tdt = 0;
	csr32w(c, Tdt, 0);
	for(i = 0; i < c->ntd; i++){
		if((m = c->tb[i]) != nil){
			c->tb[i] = nil;
			mbfree(m);
		}
		memset(&c->tdba[i], 0, sizeof(Td));
	}
	c->tdfree = c->ntd;
	csr32w(c, Tidv, 128);
	r = csr32r(c, Txdctl);
	r &= ~WthreshMASK;
	r |= Gran | 4<<WthreshSHIFT;
	csr32w(c, Tadv, 64);
	csr32w(c, Txdctl, r);
	r = csr32r(c, Tctl);
	r |= Ten;
	csr32w(c, Tctl, r);
}

static int
ret0(void*)
{
	return 0;
}

static void
i82563transmit(Ether *e)
{
	int tdh, tdt, ctdh;
	Ctlr *c;
	Msgbuf *m;
	Td *td;

	c = e->ctlr;
	ilock(&c->txlock);
	/*
	 * Free any completed packets
	 */
	tdh = c->tdh;
	ctdh = csr32r(c, Tdh);
	while(NEXT(tdh, c->ntd) != ctdh){
		if((m = c->tb[tdh]) != nil){
			c->tb[tdh] = nil;
			mbfree(m);
		}
		memset(&c->tdba[tdh], 0, sizeof(Td));
		tdh = NEXT(tdh, c->ntd);
	}
	c->tdh = tdh;

	/*
	 * Try to fill the ring back up.
	 */
	tdt = c->tdt;
	while(NEXT(tdt, c->ntd) != tdh){
		if((m = etheroq(e)) == nil)
			break;
		td = &c->tdba[tdt];
		td->addr[0] = PCIWADDR(m->data);
		td->control = (m->count & LenMASK) << LenSHIFT;
		td->control |= Ifcs | Teop | DtypeDD;
		c->tb[tdt] = m;
		tdt = NEXT(tdt, c->ntd);
		c->tdt = tdt;
		if(NEXT(tdt, c->ntd) == tdh){
			td->control |= Rs;
			c->txdw++;
			i82563im(c, Txdw);
			break;
		}
	}
	csr32w(c, Tdt, tdt);
	iunlock(&c->txlock);
}

static void
i82563replenish(Ctlr* c)
{
	int rdt;
	Msgbuf *m;
	Rd *rd;

	rdt = c->rdt;
	while(NEXT(rdt, c->nrd) != c->rdh){
		rd = &c->rdba[rdt];
		if(c->rb[rdt] == nil){
			if((m = i82563rballoc()) == nil){
				print("no available buffers\n");
				break;
			}
			c->rb[rdt] = m;
			rd->addr[0] = PCIWADDR(m->data);
			rd->addr[1] = 0;
		}
		rd->status = 0;
		rdt = NEXT(rdt, c->nrd);
		c->rdfree++;
	}
	c->rdt = rdt;
	csr32w(c, Rdt, rdt);
}

static void
i82563rxinit(Ctlr* c)
{
	int i;
	Msgbuf *m;

//	csr32w(c, Rctl, Dpf | Bsize2048 | Bam | RdtmsHALF);
//	csr32w(c, Rctl, Lpe| Dpf | Bsize16384 | Bam | RdtmsHALF | Bsex | Secrc);
	csr32w(c, Rctl, Lpe| Dpf | Bsize8192  | Bam | RdtmsHALF | Bsex | Secrc);

	csr32w(c, Rdbal, PCIWADDR(c->rdba));
	csr32w(c, Rdbah, 0);
	csr32w(c, Rdlen, c->nrd*sizeof(Rd));
	c->rdh = 0;
	csr32w(c, Rdh, 0);
	c->rdt = 0;
	csr32w(c, Rdt, 0);
	c->rdtr = 0;
	c->radv = 0;
	csr32w(c, Rdtr, Fpd | 0);
	csr32w(c, Radv, 0);

	for(i = 0; i < c->nrd; i++){
		if((m = c->rb[i]) != nil){
			c->rb[i] = nil;
			mbfree(m);
		}
	}
	i82563replenish(c);
	csr32w(c, Radv, 64);
//	csr32w(c, Rxdctl, 8<<WthreshSHIFT | 8<<HthreshSHIFT | 4);
//	csr32w(c, Rxdctl, 0<<WthreshSHIFT | 0<<HthreshSHIFT | 0);
	csr32w(c, Rxdctl, 1<<16 | 1<<24);

	/*
	 * Enable checksum offload.
	 */
//#define ETHERHDRSIZE 14
//	csr32w(c, Rxcsum, Tuofl | Ipofl | ETHERHDRSIZE<<PcssSHIFT);
}

static int
rim0(void *rim)
{
	return *(int*)rim != 0;
}

static void
i82563rxproc(void)
{
	int r, rdh, rim;
	Ctlr *c;
	Ether *e;
	Msgbuf *m;
	Rd *rd;

	e = u->arg;
	c = e->ctlr;
	i82563rxinit(c);
	r = csr32r(c, Rctl);
	r |= Ren;
	csr32w(c, Rctl, r);

	for(;;){
		i82563im(c, Rxt0|Rxo|Rxdmt0|Rxseq);
		c->rsleep++;
		coherence();
		sleep(&c->rxrendez, rim0, &c->rim);

		rdh = c->rdh;
		for(;;){
			rd = &c->rdba[rdh];
			rim = c->rim;
			c->rim = 0;
			if(!(rd->status & Rdd))
				break;

			/*
			 * Accept eop packets with no errors.
			 * With no errors and the Ixsm bit set,
			 * the descriptor status Tpcs and Ipcs bits give
			 * an indication of whether the checksums were
			 * calculated and valid.
			 */
			if (m = c->rb[rdh]) {
				if((rd->status & Reop) && rd->errors == 0){
					m->count = rd->length;
					m->next = nil;
					etheriq(e, m);
				} else
					mbfree(m);
				c->rb[rdh] = nil;
			}
			memset(rd, 0, sizeof(Rd));
			c->rdfree--;
			c->rdh = rdh = NEXT(rdh, c->nrd);
			coherence();
			if(c->rdfree < (c->nrd/4)*3 || (rim&Rxdmt0))
				i82563replenish(c);
		}
	}
}

static void
i82563attach(Ether *e)
{
	char name[NAMELEN];
	Ctlr *c;
	Msgbuf *m;

	c = e->ctlr;
	c->nrd = ROUND(Nrd, 8);
	c->ntd = ROUND(Ntd, 8);
	c->alloc = ialloc(c->nrd*sizeof(Rd)+c->ntd*sizeof(Td) + 255, 0);
	c->rdba = (Rd*)ROUNDUP((ulong)c->alloc, 256);
	c->tdba = (Td*)(c->rdba+c->nrd);

	c->rb = ialloc(c->nrd*sizeof m, 0);
	c->tb = ialloc(c->ntd*sizeof m, 0);

	for(c->nrb = 0; c->nrb < Nrb; c->nrb++){
		m = mballoc(Rbsz+BY2PG, 0, Mbeth1);
		m->free = i82563rbfree;
		mbfree(m);
	}
	snprint(name, sizeof name, "82563rx%ld", c-ports);
	userinit(i82563rxproc, e, name);
	i82563txinit(c);
}

static void
i82563interrupt(Ureg*, void* v)
{
	int icr, im, txdw;
	Ether *e;
	Ctlr *c;

	e = v;
	c = e->ctlr;

	ilock(&c->imlock);
	csr32w(c, Imc, ~0);
	im = c->im;
	txdw = 0;
	while(icr = csr32r(c, Icr) & c->im){
		if(icr & Lsc){
			im &= ~Lsc;
			c->lim = icr & Lsc;
			c->lintr++;
		}
		if(icr & (Rxt0|Rxo|Rxdmt0|Rxseq)){
			c->rim = icr & (Rxt0|Rxo|Rxdmt0|Rxseq);
			im &= ~(Rxt0|Rxo|Rxdmt0|Rxseq);
			wakeup(&c->rxrendez);
			c->rintr++;
		}
		if(icr & Txdw){
			im &= ~Txdw;
			txdw++;
			c->tintr++;
		}
	}
	c->im = im;
	csr32w(c, Ims, im);
	iunlock(&c->imlock);
	if(txdw)
		i82563transmit(e);
}

static int
i82563detach(Ctlr* c)
{
	int r, timeo;

	/*
	 * Perform a device reset to get the chip back to the
	 * power-on state, followed by an EEPROM reset to read
	 * the defaults for some internal registers.
	 */
	csr32w(c, Imc, ~0);
	csr32w(c, Rctl, 0);
	csr32w(c, Tctl, 0);

	delay(10);

	csr32w(c, Ctrl, Devrst);
	delay(1);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(c, Ctrl) & Devrst))
			break;
		delay(1);
	}
	if(csr32r(c, Ctrl) & Devrst)
		return -1;
	r = csr32r(c, Ctrlext);
	csr32w(c, Ctrlext, r | Eerst);
	delay(1);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(c, Ctrlext) & Eerst))
			break;
		delay(1);
	}
	if(csr32r(c, Ctrlext) & Eerst)
		return -1;

	csr32w(c, Imc, ~0);
	delay(1);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!csr32r(c, Icr))
			break;
		delay(1);
	}
	if(csr32r(c, Icr))
		return -1;

	return 0;
}

static ushort
eeread(Ctlr* c, int adr)
{
	csr32w(c, Eerd, ee_start | adr<<2);
	while ((csr32r(c, Eerd) & ee_done) == 0)
		;
	return csr32r(c, Eerd) >> 16;
}

static int
eeload(Ctlr* c)
{
	int data, adr;
	ushort sum;

	sum = 0;
	for (adr = 0; adr < 0x40; adr++) {
		data = eeread(c, adr);
		c->eeprom[adr] = data;
		sum += data;
	}
	return sum;
}

static uchar*
etheradd(uchar *u, uint n)
{
	int i;
	uint j;

	for(i = 5; n != 0 && i >= 0; i--){
		j = n + u[i];
		u[i] = j;
		n = j >> 8;
	}
	return u;
}

typedef struct {
	uchar	ea[Easize];
	int	n;
} Basetab;

static Basetab btab[Nether];
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
reset(Ctlr *c)
{
	int i, r;

	if(i82563detach(c))
		return -1;
	r = eeload(c);
	if (r != 0 && r != 0xbaba){
		print("i82563: bad EEPROM checksum - 0x%4.4ux\n", r);
		return -1;
	}

	for(i = Ea; i < Easize/2; i++){
		c->ra[2*i]   = c->eeprom[i];
		c->ra[2*i+1] = c->eeprom[i] >> 8;
	}
	etheradd(c->ra, nthether(c->ra));
	r = c->ra[3]<<24 | c->ra[2]<<16 | c->ra[1]<<8 | c->ra[0];
	csr32w(c, Ral, r);
	r = 0x80000000 | c->ra[5]<<8 | c->ra[4];
	csr32w(c, Rah, r);
	for(i = 1; i < 16; i++){
		csr32w(c, Ral+i*8, 0);
		csr32w(c, Rah+i*8, 0);
	}
	memset(c->mta, 0, sizeof c->mta);
	for(i = 0; i < 128; i++)
		csr32w(c, Mta+i*4, 0);
	csr32w(c, Fcal, 0x00C28001);
	csr32w(c, Fcah, 0x00000100);
	csr32w(c, Fct,  0x00008808);
	csr32w(c, Fcttv, 0x00000100);
	csr32w(c, Fcrtl, c->fcrtl);
	csr32w(c, Fcrth, c->fcrth);
	return 0;
}

static void
i82563init(Ether *)
{
	Ctlr *c;
	Pcidev *p;

	print("i82563init\n");
	p = 0;
	while(nports < nelem(ports) && (p = pcimatch(p, 0x8086, 0x1096))){
		c = ports + nports;
		memset(c, 0, sizeof *c);
		c->pcidev = p;
		c->id = p->did<<16 | p->vid;

		c->port = p->mem[0].bar & ~0xf;
		c->nic = (int*)upamalloc(c->port, p->mem[0].size, 0);
		c->cls = pcicfgr8(p, PciCLS) << 2;
		switch(c->cls){
		default:
			print("i82563: unexpected CLS - %d\n", c->cls);
		case 0x08<<2:
		case 0x10<<2:
			break;
		case 0x00<<2:
		case 0xFF<<2:
			print("i82563: unusable CLS\n");
			continue;
		}
		if(reset(c))
			continue;
		pcisetbme(p);
		print("82563 %d irq %d Ea %E\n", nports, p->intl,
			ports[nports].ra);
		nports++;
	}
}

int
i82563reset(Ether *e)
{
	int i;
	static int once;

	if(once++ == 0)
		i82563init(e);
	for(i = 0; i < nports; i++)
		if (!ports[i].active &&
		    (e->port == 0 || e->port == ports[i].port))
			break;
	if(i == nports)
		return -1;
	ports[i].active = 1;
	e->ctlr = ports+i;
	e->port = ports[i].port;
	e->irq =  ports[i].pcidev->intl;
	e->tbdf = ports[i].pcidev->tbdf;
	e->mbps = 1000;
	memmove(e->ea, ports[i].ra, Easize);
	e->attach = i82563attach;
	e->transmit = i82563transmit;
	e->interrupt = i82563interrupt;

	return 0;
}
