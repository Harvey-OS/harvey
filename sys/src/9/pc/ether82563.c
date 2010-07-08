/*
 * Intel Gigabit Ethernet PCI-Express Controllers.
 *	8256[36], 8257[12], 82573[ev]
 *	82575eb, 82576
 * Pretty basic, does not use many of the chip smarts.
 * The interrupt mitigation tuning for each chip variant
 * is probably different. The reset/initialisation
 * sequence needs straightened out. Doubt the PHY code
 * for the 82575eb is right.
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

	Ctrl		= 0x0000,	/* Device Control */
	Status		= 0x0008,	/* Device Status */
	Eec		= 0x0010,	/* EEPROM/Flash Control/Data */
	Eerd		= 0x0014,	/* EEPROM Read */
	Ctrlext		= 0x0018,	/* Extended Device Control */
	Fla		= 0x001c,	/* Flash Access */
	Mdic		= 0x0020,	/* MDI Control */
	Seresctl	= 0x0024,	/* Serdes ana */
	Fcal		= 0x0028,	/* Flow Control Address Low */
	Fcah		= 0x002C,	/* Flow Control Address High */
	Fct		= 0x0030,	/* Flow Control Type */
	Kumctrlsta	= 0x0034,	/* MAC-PHY Interface */
	Vet		= 0x0038,	/* VLAN EtherType */
	Fcttv		= 0x0170,	/* Flow Control Transmit Timer Value */
	Txcw		= 0x0178,	/* Transmit Configuration Word */
	Rxcw		= 0x0180,	/* Receive Configuration Word */
	Ledctl		= 0x0E00,	/* LED control */
	Pba		= 0x1000,	/* Packet Buffer Allocation */
	Pbs		= 0x1008,	/* Packet Buffer Size */

	/* Interrupt */

	Icr		= 0x00C0,	/* Interrupt Cause Read */
	Itr		= 0x00c4,	/* Interrupt Throttling Rate */
	Ics		= 0x00C8,	/* Interrupt Cause Set */
	Ims		= 0x00D0,	/* Interrupt Mask Set/Read */
	Imc		= 0x00D8,	/* Interrupt mask Clear */
	Iam		= 0x00E0,	/* Interrupt acknowledge Auto Mask */

	/* Receive */

	Rctl		= 0x0100,	/* Control */
	Ert		= 0x2008,	/* Early Receive Threshold (573[EVL] only) */
	Fcrtl		= 0x2160,	/* Flow Control RX Threshold Low */
	Fcrth		= 0x2168,	/* Flow Control Rx Threshold High */
	Psrctl		= 0x2170,	/* Packet Split Receive Control */
	Rdbal		= 0x2800,	/* Rdesc Base Address Low Queue 0 */
	Rdbah		= 0x2804,	/* Rdesc Base Address High Queue 0 */
	Rdlen		= 0x2808,	/* Descriptor Length Queue 0 */
	Rdh		= 0x2810,	/* Descriptor Head Queue 0 */
	Rdt		= 0x2818,	/* Descriptor Tail Queue 0 */
	Rdtr		= 0x2820,	/* Descriptor Timer Ring */
	Rxdctl		= 0x2828,	/* Descriptor Control */
	Radv		= 0x282C,	/* Interrupt Absolute Delay Timer */
	Rdbal1		= 0x2900,	/* Rdesc Base Address Low Queue 1 */
	Rdbah1		= 0x2804,	/* Rdesc Base Address High Queue 1 */
	Rdlen1		= 0x2908,	/* Descriptor Length Queue 1 */
	Rdh1		= 0x2910,	/* Descriptor Head Queue 1 */
	Rdt1		= 0x2918,	/* Descriptor Tail Queue 1 */
	Rxdctl1		= 0x2928,	/* Descriptor Control Queue 1 */
	Rsrpd		= 0x2c00,	/* Small Packet Detect */
	Raid		= 0x2c08,	/* ACK interrupt delay */
	Cpuvec		= 0x2c10,	/* CPU Vector */
	Rxcsum		= 0x5000,	/* Checksum Control */
	Rfctl		= 0x5008,	/* Filter Control */
	Mta		= 0x5200,	/* Multicast Table Array */
	Ral		= 0x5400,	/* Receive Address Low */
	Rah		= 0x5404,	/* Receive Address High */
	Vfta		= 0x5600,	/* VLAN Filter Table Array */
	Mrqc		= 0x5818,	/* Multiple Receive Queues Command */
	Rssim		= 0x5864,	/* RSS Interrupt Mask */
	Rssir		= 0x5868,	/* RSS Interrupt Request */
	Reta		= 0x5c00,	/* Redirection Table */
	Rssrk		= 0x5c80,	/* RSS Random Key */

	/* Transmit */

	Tctl		= 0x0400,	/* Transmit Control */
	Tipg		= 0x0410,	/* Transmit IPG */
	Tkabgtxd	= 0x3004,	/* glci afe band gap transmit ref data, or something */
	Tdbal		= 0x3800,	/* Tdesc Base Address Low */
	Tdbah		= 0x3804,	/* Tdesc Base Address High */
	Tdlen		= 0x3808,	/* Descriptor Length */
	Tdh		= 0x3810,	/* Descriptor Head */
	Tdt		= 0x3818,	/* Descriptor Tail */
	Tidv		= 0x3820,	/* Interrupt Delay Value */
	Txdctl		= 0x3828,	/* Descriptor Control */
	Tadv		= 0x382C,	/* Interrupt Absolute Delay Timer */
	Tarc0		= 0x3840,	/* Arbitration Counter Queue 0 */
	Tdbal1		= 0x3900,	/* Descriptor Base Low Queue 1 */
	Tdbah1		= 0x3904,	/* Descriptor Base High Queue 1 */
	Tdlen1		= 0x3908,	/* Descriptor Length Queue 1 */
	Tdh1		= 0x3910,	/* Descriptor Head Queue 1 */
	Tdt1		= 0x3918,	/* Descriptor Tail Queue 1 */
	Txdctl1		= 0x3928,	/* Descriptor Control 1 */
	Tarc1		= 0x3940,	/* Arbitration Counter Queue 1 */

	/* Statistics */

	Statistics	= 0x4000,	/* Start of Statistics Area */
	Gorcl		= 0x88/4,	/* Good Octets Received Count */
	Gotcl		= 0x90/4,	/* Good Octets Transmitted Count */
	Torl		= 0xC0/4,	/* Total Octets Received */
	Totl		= 0xC8/4,	/* Total Octets Transmitted */
	Nstatistics	= 0x124/4,
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
	Phyrst		= 1<<31,	/* Phy Reset */
};

enum {					/* Status */
	Lu		= 1<<1,		/* Link Up */
	Lanid		= 3<<2,		/* mask for Lan ID. */
	Txoff		= 1<<4,		/* Transmission Paused */
	Tbimode		= 1<<5,		/* TBI Mode Indication */
	Phyra		= 1<<10,	/* PHY Reset Asserted */
	GIOme		= 1<<19,	/* GIO Master Enable Status */
};

enum {					/* Eerd */
	EEstart		= 1<<0,		/* Start Read */
	EEdone		= 1<<1,		/* Read done */
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

enum {					/* phy interface registers */
	Phyctl		= 0,		/* phy ctl */
	Physsr		= 17,		/* phy secondary status */
	Phyier		= 18,		/* 82573 phy interrupt enable */
	Phyisr		= 19,		/* 82563 phy interrupt status */
	Phylhr		= 19,		/* 8257[12] link health */

	Rtlink		= 1<<10,	/* realtime link status */
	Phyan		= 1<<11,	/* phy has auto-negotiated */

	/* Phyctl bits */
	Ran		= 1<<9,		/* restart auto-negotiation */
	Ean		= 1<<12,	/* enable auto-negotiation */

	/* 82573 Phyier bits */
	Lscie		= 1<<10,	/* link status changed ie */
	Ancie		= 1<<11,	/* auto-negotiation complete ie */
	Spdie		= 1<<14,	/* speed changed ie */
	Panie		= 1<<15,	/* phy auto-negotiation error ie */

	/* Phylhr/Phyisr bits */
	Anf		= 1<<6,		/* lhr: auto-negotiation fault */
	Ane		= 1<<15,	/* isr: auto-negotiation error */
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
	Ack		= 0x00020000,	/* Receive ACK frame */
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
	TxcwConfig	= 0x40000000,	/* Transmit Config Control */
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
	Bsize16384	= 0x00010000,	/* Bsex = 1 */
	Bsize8192	= 0x00020000, 	/* Bsex = 1 */
	Bsize2048	= 0x00000000,
	Bsize1024	= 0x00010000,
	Bsize512	= 0x00020000,
	Bsize256	= 0x00030000,
	BsizeFlex	= 0x08000000,	/* Flexible Bsize in 1KB increments */
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
	WthreshMASK	= 0x003F0000,	/* Writeback Threshold */
	WthreshSHIFT	= 16,
	Gran		= 0x01000000,	/* Granularity */
	Qenable		= 0x02000000,	/* Queue Enable (82575) */
};

enum {					/* Rxcsum */
	PcssMASK	= 0x00FF,	/* Packet Checksum Start */
	PcssSHIFT	= 0,
	Ipofl		= 0x0100,	/* IP Checksum Off-load Enable */
	Tuofl		= 0x0200,	/* TCP/UDP Checksum Off-load Enable */
};

enum {					/* Receive Delay Timer Ring */
	DelayMASK	= 0xFFFF,	/* delay timer in 1.024nS increments */
	DelaySHIFT	= 0,
	Fpd		= 0x80000000,	/* Flush partial Descriptor Block */
};

typedef struct Rd {			/* Receive Descriptor */
	u32int	addr[2];
	u16int	length;
	u16int	checksum;
	u8int	status;
	u8int	errors;
	u16int	special;
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

typedef struct {			/* Transmit Descriptor */
	u32int	addr[2];		/* Data */
	u32int	control;
	u32int	status;
} Td;

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
	Tdd		= 0x0001,	/* Descriptor Done */
	Ec		= 0x0002,	/* Excess Collisions */
	Lc		= 0x0004,	/* Late Collision */
	Tu		= 0x0008,	/* Transmit Underrun */
	CssMASK		= 0xFF00,	/* Checksum Start Field */
	CssSHIFT	= 8,
};

typedef struct {
	u16int	*reg;
	u32int	*reg32;
	int	sz;
} Flash;

enum {
	/* 16 and 32-bit flash registers for ich flash parts */
	Bfpr	= 0x00/4,		/* flash base 0:12; lim 16:28 */
	Fsts	= 0x04/2,		/* flash status;  Hsfsts */
	Fctl	= 0x06/2,		/* flash control; Hsfctl */
	Faddr	= 0x08/4,		/* flash address to r/w */
	Fdata	= 0x10/4,		/* data @ address */

	/* status register */
	Fdone	= 1<<0,			/* flash cycle done */
	Fcerr	= 1<<1,			/* cycle error; write 1 to clear */
	Ael	= 1<<2,			/* direct access error log; 1 to clear */
	Scip	= 1<<5,			/* spi cycle in progress */
	Fvalid	= 1<<14,		/* flash descriptor valid */

	/* control register */
	Fgo	= 1<<0,			/* start cycle */
	Flcycle	= 1<<1,			/* two bits: r=0; w=2 */
	Fdbc	= 1<<8,			/* bytes to read; 5 bits */
};

enum {
	Nrd		= 256,		/* power of two */
	Ntd		= 128,		/* power of two */
	Nrb		= 1024,		/* private receive buffers per Ctlr */
};

enum {
	Iany,
	i82563,
	i82566,
	i82567,
	i82571,
	i82572,
	i82573,
	i82575,
	i82576,
};

static int rbtab[] = {
	0,
	9014,
	1514,
	1514,
	9234,
	9234,
	8192,				/* terrible performance above 8k */
	1514,
	1514,
};

static char *tname[] = {
	"any",
	"i82563",
	"i82566",
	"i82567",
	"i82571",
	"i82572",
	"i82573",
	"i82575",
	"i82576",
};

typedef struct Ctlr Ctlr;
struct Ctlr {
	int	port;
	Pcidev	*pcidev;
	Ctlr	*next;
	Ether	*edev;
	int	active;
	int	type;
	ushort	eeprom[0x40];

	QLock	alock;			/* attach */
	int	attached;
	int	nrd;
	int	ntd;
	int	nrb;			/* how many this Ctlr has in the pool */
	unsigned rbsz;			/* unsigned for % and / by 1024 */

	int	*nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Rendez	lrendez;
	int	lim;

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
	uint	speeds[4];

	uchar	ra[Eaddrlen];		/* receive address */
	ulong	mta[128];		/* multicast table array */

	Rendez	rrendez;
	int	rim;
	int	rdfree;
	Rd	*rdba;			/* receive descriptor base address */
	Block	**rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */
	int	rdtr;			/* receive delay timer ring value */
	int	radv;			/* receive interrupt absolute delay timer */

	Rendez	trendez;
	QLock	tlock;
	int	tbusy;
	Td	*tdba;			/* transmit descriptor base address */
	Block	**tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */

	int	fcrtl;
	int	fcrth;

	uint	pba;			/* packet buffer allocation */
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr* i82563ctlrhead;
static Ctlr* i82563ctlrtail;

static Lock i82563rblock;		/* free receive Blocks */
static Block* i82563rbpool;

static char* statistics[] = {
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
	"Packets Received (1024-mtu Bytes)",
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
	"Management Packets Rx",
	"Management Packets Drop",
	"Management Packets Tx",
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
	"Packets Transmitted (1024-mtu Bytes)",
	"Multicast Packets Transmitted",
	"Broadcast Packets Transmitted",
	"TCP Segmentation Context Transmitted",
	"TCP Segmentation Context Fail",
	"Interrupt Assertion",
	"Interrupt Rx Pkt Timer",
	"Interrupt Rx Abs Timer",
	"Interrupt Tx Pkt Timer",
	"Interrupt Tx Abs Timer",
	"Interrupt Tx Queue Empty",
	"Interrupt Tx Desc Low",
	"Interrupt Rx Min",
	"Interrupt Rx Overrun",
};

static long
i82563ifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *s, *p, *e, *stat;
	int i, r;
	uvlong tuvl, ruvl;

	ctlr = edev->ctlr;
	qlock(&ctlr->slock);
	p = s = malloc(READSTR);
	e = p + READSTR;

	for(i = 0; i < Nstatistics; i++){
		r = csr32r(ctlr, Statistics + i*4);
		if((stat = statistics[i]) == nil)
			continue;
		switch(i){
		case Gorcl:
		case Gotcl:
		case Torl:
		case Totl:
			ruvl = r;
			ruvl += (uvlong)csr32r(ctlr, Statistics+(i+1)*4) << 32;
			tuvl = ruvl;
			tuvl += ctlr->statistics[i];
			tuvl += (uvlong)ctlr->statistics[i+1] << 32;
			if(tuvl == 0)
				continue;
			ctlr->statistics[i] = tuvl;
			ctlr->statistics[i+1] = tuvl >> 32;
			p = seprint(p, e, "%s: %llud %llud\n", stat, tuvl, ruvl);
			i++;
			break;

		default:
			ctlr->statistics[i] += r;
			if(ctlr->statistics[i] == 0)
				continue;
			p = seprint(p, e, "%s: %ud %ud\n", stat,
				ctlr->statistics[i], r);
			break;
		}
	}

	p = seprint(p, e, "lintr: %ud %ud\n", ctlr->lintr, ctlr->lsleep);
	p = seprint(p, e, "rintr: %ud %ud\n", ctlr->rintr, ctlr->rsleep);
	p = seprint(p, e, "tintr: %ud %ud\n", ctlr->tintr, ctlr->txdw);
	p = seprint(p, e, "ixcs: %ud %ud %ud\n", ctlr->ixsm, ctlr->ipcs, ctlr->tcpcs);
	p = seprint(p, e, "rdtr: %ud\n", ctlr->rdtr);
	p = seprint(p, e, "radv: %ud\n", ctlr->radv);
	p = seprint(p, e, "ctrl: %.8ux\n", csr32r(ctlr, Ctrl));
	p = seprint(p, e, "ctrlext: %.8ux\n", csr32r(ctlr, Ctrlext));
	p = seprint(p, e, "status: %.8ux\n", csr32r(ctlr, Status));
	p = seprint(p, e, "txcw: %.8ux\n", csr32r(ctlr, Txcw));
	p = seprint(p, e, "txdctl: %.8ux\n", csr32r(ctlr, Txdctl));
	p = seprint(p, e, "pba: %.8ux\n", ctlr->pba);

	p = seprint(p, e, "speeds: 10:%ud 100:%ud 1000:%ud ?:%ud\n",
		ctlr->speeds[0], ctlr->speeds[1], ctlr->speeds[2], ctlr->speeds[3]);
	p = seprint(p, e, "type: %s\n", tname[ctlr->type]);

//	p = seprint(p, e, "eeprom:");
//	for(i = 0; i < 0x40; i++){
//		if(i && ((i & 7) == 0))
//			p = seprint(p, e, "\n       ");
//		p = seprint(p, e, " %4.4ux", ctlr->eeprom[i]);
//	}
//	p = seprint(p, e, "\n");

	USED(p);
	n = readstr(offset, a, n, s);
	free(s);
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
	ulong v;
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
		v = strtoul(cb->f[1], &p, 0);
		if(p == cb->f[1] || v > 0xFFFF)
			error(Ebadarg);
		ctlr->rdtr = v;
		csr32w(ctlr, Rdtr, v);
		break;
	case CMradv:
		v = strtoul(cb->f[1], &p, 0);
		if(p == cb->f[1] || v > 0xFFFF)
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
	if(ctlr->type == i82566 || ctlr->type == i82567)
		x &= 31;
	bit = ((addr[5] & 1)<<4)|(addr[4]>>4);
	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	if(on)
		ctlr->mta[x] |= 1<<bit;
//	else
//		ctlr->mta[x] &= ~(1<<bit);

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
		_xinc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&i82563rblock);

	return bp;
}

static void
i82563rbfree(Block* b)
{
	b->rp = b->wp = (uchar*)PGROUND((uintptr)b->base);
 	b->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);
	ilock(&i82563rblock);
	b->next = i82563rbpool;
	i82563rbpool = b;
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

	csr32w(ctlr, Tctl, 0x0F<<CtSHIFT | Psp | 66<<ColdSHIFT | Mulr);
	csr32w(ctlr, Tipg, 6<<20 | 8<<10 | 8);		/* yb sez: 0x702008 */
	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));
	csr32w(ctlr, Tdbah, 0);
	csr32w(ctlr, Tdlen, ctlr->ntd * sizeof(Td));
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
	csr32w(ctlr, Tidv, 128);
	r = csr32r(ctlr, Txdctl);
	r &= ~(WthreshMASK|PthreshMASK);
	r |= 4<<WthreshSHIFT | 4<<PthreshSHIFT;
	if(ctlr->type == i82575 || ctlr->type == i82576)
		r |= Qenable;
	csr32w(ctlr, Tadv, 64);
	csr32w(ctlr, Txdctl, r);
	r = csr32r(ctlr, Tctl);
	r |= Ten;
	csr32w(ctlr, Tctl, r);
//	if(ctlr->type == i82671)
//		csr32w(ctlr, Tarc0, csr32r(ctlr, Tarc0) | 7<<24); /* yb sez? */
}

#define Next(x, m)	(((x)+1) & (m))

static int
i82563cleanup(Ctlr *c)
{
	Block *b;
	int tdh, m, n;

	tdh = c->tdh;
	m = c->ntd-1;
	while(c->tdba[n = Next(tdh, m)].status & Tdd){
		tdh = n;
		if((b = c->tb[tdh]) != nil){
			c->tb[tdh] = nil;
			freeb(b);
		}else
			iprint("82563 tx underrun!\n");
		c->tdba[tdh].status = 0;
	}

	return c->tdh = tdh;
}

static void
i82563transmit(Ether* edev)
{
	Td *td;
	Block *bp;
	Ctlr *ctlr;
	int tdh, tdt, m;

	ctlr = edev->ctlr;

	qlock(&ctlr->tlock);

	/*
	 * Free any completed packets
	 */
	tdh = i82563cleanup(ctlr);

	/*
	 * Try to fill the ring back up.
	 */
	tdt = ctlr->tdt;
	m = ctlr->ntd-1;
	for(;;){
		if(Next(tdt, m) == tdh){
			ctlr->txdw++;
			i82563im(ctlr, Txdw);
			break;
		}
		if((bp = qget(edev->oq)) == nil)
			break;
		td = &ctlr->tdba[tdt];
		td->addr[0] = PCIWADDR(bp->rp);
		td->control = Ide|Rs|Ifcs|Teop|BLEN(bp);
		ctlr->tb[tdt] = bp;
		tdt = Next(tdt, m);
	}
	if(ctlr->tdt != tdt){
		ctlr->tdt = tdt;
		csr32w(ctlr, Tdt, tdt);
	}
	qunlock(&ctlr->tlock);
}

static void
i82563replenish(Ctlr* ctlr)
{
	Rd *rd;
	int rdt, m;
	Block *bp;

	rdt = ctlr->rdt;
	m = ctlr->nrd-1;
	while(Next(rdt, m) != ctlr->rdh){
		rd = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] != nil){
			iprint("82563: tx overrun\n");
			break;
		}
		bp = i82563rballoc();
		if(bp == nil){
			vlong now;
			static vlong lasttime;

			/* don't flood the console */
			now = tk2ms(MACHP(0)->ticks);
			if (now - lasttime > 2000)
				iprint("#l%d: 82563: all %d rx buffers in use\n",
					ctlr->edev->ctlrno, ctlr->nrb);
			lasttime = now;
			break;
		}
		ctlr->rb[rdt] = bp;
		rd->addr[0] = PCIWADDR(bp->rp);
//		rd->addr[1] = 0;
		rd->status = 0;
		ctlr->rdfree++;
		rdt = Next(rdt, m);
	}
	ctlr->rdt = rdt;
	csr32w(ctlr, Rdt, rdt);
}

static void
i82563rxinit(Ctlr* ctlr)
{
	Block *bp;
	int i, r, rctl;

	if(ctlr->rbsz <= 2048)
		rctl = Dpf|Bsize2048|Bam|RdtmsHALF;
	else if(ctlr->rbsz <= 8192)
		rctl = Lpe|Dpf|Bsize8192|Bsex|Bam|RdtmsHALF|Secrc;
	else if(ctlr->rbsz <= 12*1024){
		i = ctlr->rbsz / 1024;
		if(ctlr->rbsz % 1024)
			i++;
		rctl = Lpe|Dpf|BsizeFlex*i|Bam|RdtmsHALF|Secrc;
	}
	else
		rctl = Lpe|Dpf|Bsize16384|Bsex|Bam|RdtmsHALF|Secrc;

	if(ctlr->type == i82575 || ctlr->type == i82576){
		/*
		 * Setting Qenable in Rxdctl does not
		 * appear to stick unless Ren is on.
		 */
		csr32w(ctlr, Rctl, Ren|rctl);
		r = csr32r(ctlr, Rxdctl);
		r |= Qenable;
		csr32w(ctlr, Rxdctl, r);
	}
	csr32w(ctlr, Rctl, rctl);

	if(ctlr->type == i82573)
		csr32w(ctlr, Ert, 1024/8);

	if(ctlr->type == i82566 || ctlr->type == i82567)
		csr32w(ctlr, Pbs, 16);

	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, ctlr->nrd * sizeof(Rd));
	ctlr->rdh = 0;
	csr32w(ctlr, Rdh, 0);
	ctlr->rdt = 0;
	csr32w(ctlr, Rdt, 0);
	/* to hell with interrupt moderation, we've got fast cpus */
//	ctlr->rdtr = 25;		/* µs units? */
//	ctlr->radv = 500;		/* µs units? */
	ctlr->radv = ctlr->rdtr = 0;
	csr32w(ctlr, Rdtr, ctlr->rdtr);
	csr32w(ctlr, Radv, ctlr->radv);

	for(i = 0; i < ctlr->nrd; i++){
		if((bp = ctlr->rb[i]) != nil){
			ctlr->rb[i] = nil;
			freeb(bp);
		}
	}
	i82563replenish(ctlr);

	if(ctlr->type != i82575 || ctlr->type == i82576){
		/*
		 * See comment above for Qenable.
		 * Could shuffle the code?
		 */
		r = csr32r(ctlr, Rxdctl);
		r &= ~(WthreshMASK|PthreshMASK);
		r |= (2<<WthreshSHIFT)|(2<<PthreshSHIFT);
		csr32w(ctlr, Rxdctl, r);
	}

	/*
	 * Don't enable checksum offload.  In practice, it interferes with
	 * tftp booting on at least the 82575.
	 */
//	csr32w(ctlr, Rxcsum, Tuofl | Ipofl | ETHERHDRSIZE<<PcssSHIFT);
	csr32w(ctlr, Rxcsum, 0);
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
	int r, m, rdh, rim;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	i82563rxinit(ctlr);
	r = csr32r(ctlr, Rctl);
	r |= Ren;
	csr32w(ctlr, Rctl, r);
	m = ctlr->nrd-1;

	for(;;){
		i82563im(ctlr, Rxt0|Rxo|Rxdmt0|Rxseq|Ack);
		ctlr->rsleep++;
//		coherence();
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
			bp = ctlr->rb[rdh];
			if((rd->status & Reop) && rd->errors == 0){
				bp->wp += rd->length;
				bp->lim = bp->wp;	/* lie like a dog. */
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
			} else if (rd->status & Reop && rd->errors)
				print("%s: input packet error %#ux\n",
					tname[ctlr->type], rd->errors);
			else
				freeb(bp);
			ctlr->rb[rdh] = nil;

			rd->status = 0;
			ctlr->rdfree--;
			ctlr->rdh = rdh = Next(rdh, m);
			if(ctlr->nrd-ctlr->rdfree >= 32 || (rim & Rxdmt0))
				i82563replenish(ctlr);
		}
	}
}

static int
i82563lim(void* c)
{
	return ((Ctlr*)c)->lim != 0;
}

static int speedtab[] = {
	10, 100, 1000, 0
};

static uint
phyread(Ctlr *c, int reg)
{
	uint phy, i;

	csr32w(c, Mdic, MDIrop | 1<<MDIpSHIFT | reg<<MDIrSHIFT);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = csr32r(c, Mdic);
		if(phy & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((phy & (MDIe|MDIready)) != MDIready)
		return ~0;
	return phy & 0xffff;
}

static uint
phywrite(Ctlr *c, int reg, ushort val)
{
	uint phy, i;

	csr32w(c, Mdic, MDIwop | 1<<MDIpSHIFT | reg<<MDIrSHIFT | val);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = csr32r(c, Mdic);
		if(phy & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((phy & (MDIe|MDIready)) != MDIready)
		return ~0;
	return 0;
}

/*
 * watch for changes of link state
 */
static void
i82563lproc(void *v)
{
	uint phy, i, a;
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;

	if(c->type == i82573 && (phy = phyread(c, Phyier)) != ~0)
		phywrite(c, Phyier, phy | Lscie | Ancie | Spdie | Panie);
	for(;;){
		phy = phyread(c, Physsr);
		if(phy == ~0)
			goto next;
		i = (phy>>14) & 3;

		switch(c->type){
		case i82563:
			a = phyread(c, Phyisr) & Ane;
			break;
		case i82571:
		case i82572:
			a = phyread(c, Phylhr) & Anf;
			i = (i-1) & 3;
			break;
		default:
			a = 0;
			break;
		}
		if(a)
			phywrite(c, Phyctl, phyread(c, Phyctl) | Ran | Ean);
		e->link = (phy & Rtlink) != 0;
		if(e->link){
			c->speeds[i]++;
			if (speedtab[i])
				e->mbps = speedtab[i];
		}
next:
		c->lim = 0;
		i82563im(c, Lsc);
		c->lsleep++;
		sleep(&c->lrendez, i82563lim, c);
	}
}

static void
i82563tproc(void *v)
{
	Ether *e;
	Ctlr *c;

	e = v;
	c = e->ctlr;
	for(;;){
		sleep(&c->trendez, return0, 0);
		i82563transmit(e);
	}
}

static void
i82563attach(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	ctlr->edev = edev;			/* point back to Ether* */
	qlock(&ctlr->alock);
	if(ctlr->attached){
		qunlock(&ctlr->alock);
		return;
	}

	ctlr->nrd = Nrd;
	ctlr->ntd = Ntd;

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
		free(ctlr->tdba);
		ctlr->tdba = nil;
		free(ctlr->rdba);
		ctlr->rdba = nil;
		qunlock(&ctlr->alock);
		nexterror();
	}

	if((ctlr->rdba = mallocalign(ctlr->nrd*sizeof(Rd), 128, 0, 0)) == nil ||
	   (ctlr->tdba = mallocalign(ctlr->ntd*sizeof(Td), 128, 0, 0)) == nil ||
	   (ctlr->rb = malloc(ctlr->nrd*sizeof(Block*))) == nil ||
	   (ctlr->tb = malloc(ctlr->ntd*sizeof(Block*))) == nil)
		error(Enomem);

	for(ctlr->nrb = 0; ctlr->nrb < Nrb; ctlr->nrb++){
		if((bp = allocb(ctlr->rbsz + BY2PG)) == nil)
			break;
		bp->free = i82563rbfree;
		freeb(bp);
	}

	ctlr->attached = 1;

	snprint(name, sizeof name, "#l%dl", edev->ctlrno);
	kproc(name, i82563lproc, edev);

	snprint(name, sizeof name, "#l%dr", edev->ctlrno);
	kproc(name, i82563rproc, edev);

	snprint(name, sizeof name, "#l%dt", edev->ctlrno);
	kproc(name, i82563tproc, edev);

	i82563txinit(ctlr);

	qunlock(&ctlr->alock);
	poperror();
}

static void
i82563interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	int icr, im;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	im = ctlr->im;

	for(icr = csr32r(ctlr, Icr); icr & ctlr->im; icr = csr32r(ctlr, Icr)){
		if(icr & Lsc){
			im &= ~Lsc;
			ctlr->lim = icr & Lsc;
			wakeup(&ctlr->lrendez);
			ctlr->lintr++;
		}
		if(icr & (Rxt0|Rxo|Rxdmt0|Rxseq|Ack)){
			ctlr->rim = icr & (Rxt0|Rxo|Rxdmt0|Rxseq|Ack);
			im &= ~(Rxt0|Rxo|Rxdmt0|Rxseq|Ack);
			wakeup(&ctlr->rrendez);
			ctlr->rintr++;
		}
		if(icr & Txdw){
			im &= ~Txdw;
			ctlr->tintr++;
			wakeup(&ctlr->trendez);
		}
	}

	ctlr->im = im;
	csr32w(ctlr, Ims, im);
	iunlock(&ctlr->imlock);
}

/* assume misrouted interrupts and check all controllers */
static void
i82575interrupt(Ureg*, void *)
{
	Ctlr *ctlr;

	for (ctlr = i82563ctlrhead; ctlr != nil; ctlr = ctlr->next)
		i82563interrupt(nil, ctlr->edev);
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

	r = csr32r(ctlr, Ctrl);
	if(ctlr->type == i82566 || ctlr->type == i82567)
		r |= Phyrst;
	csr32w(ctlr, Ctrl, Devrst | r);
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

	/*
	 * Balance Rx/Tx packet buffer.
	 * No need to set PBA register unless using jumbo, defaults to 32KB
	 * for receive. If it is changed, then have to do a MAC reset,
	 * and need to do that at the the right time as it will wipe stuff.
	 */
	if(ctlr->rbsz > 8192 && (ctlr->type == i82563 || ctlr->type == i82571 ||
	    ctlr->type == i82572)){
		ctlr->pba = csr32r(ctlr, Pba);
		r = ctlr->pba >> 16;
		r += ctlr->pba & 0xffff;
		r >>= 1;
		csr32w(ctlr, Pba, r);
	} else if(ctlr->type == i82573 && ctlr->rbsz > 1514)
		csr32w(ctlr, Pba, 14);
	ctlr->pba = csr32r(ctlr, Pba);

	r = csr32r(ctlr, Ctrl);
	csr32w(ctlr, Ctrl, Slu|r);

	return 0;
}

static void
i82563shutdown(Ether* ether)
{
	i82563detach(ether->ctlr);
}

static ushort
eeread(Ctlr *ctlr, int adr)
{
	csr32w(ctlr, Eerd, EEstart | adr << 2);
	while ((csr32r(ctlr, Eerd) & EEdone) == 0)
		;
	return csr32r(ctlr, Eerd) >> 16;
}

static int
eeload(Ctlr *ctlr)
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

static int
fcycle(Ctlr *, Flash *f)
{
	ushort s, i;

	s = f->reg[Fsts];
	if((s&Fvalid) == 0)
		return -1;
	f->reg[Fsts] |= Fcerr | Ael;
	for(i = 0; i < 10; i++){
		if((s&Scip) == 0)
			return 0;
		delay(1);
		s = f->reg[Fsts];
	}
	return -1;
}

static int
fread(Ctlr *c, Flash *f, int ladr)
{
	ushort s;

	delay(1);
	if(fcycle(c, f) == -1)
		return -1;
	f->reg[Fsts] |= Fdone;
	f->reg32[Faddr] = ladr;

	/* setup flash control register */
	s = f->reg[Fctl];
	s &= ~(0x1f << 8);
	s |= (2-1) << 8;		/* 2 bytes */
	s &= ~(2*Flcycle);		/* read */
	f->reg[Fctl] = s | Fgo;

	while((f->reg[Fsts] & Fdone) == 0)
		;
	if(f->reg[Fsts] & (Fcerr|Ael))
		return -1;
	return f->reg32[Fdata] & 0xffff;
}

static int
fload(Ctlr *c)
{
	ulong data, io, r, adr;
	ushort sum;
	Flash f;

	io = c->pcidev->mem[1].bar & ~0x0f;
	f.reg = vmap(io, c->pcidev->mem[1].size);
	if(f.reg == nil)
		return -1;
	f.reg32 = (void*)f.reg;
	f.sz = f.reg32[Bfpr];
	r = f.sz & 0x1fff;
	if(csr32r(c, Eec) & (1<<22))
		++r;
	r <<= 12;

	sum = 0;
	for (adr = 0; adr < 0x40; adr++) {
		data = fread(c, &f, r + adr*2);
		if(data == -1)
			break;
		c->eeprom[adr] = data;
		sum += data;
	}
	vunmap(f.reg, c->pcidev->mem[1].size);
	return sum;
}

static int
i82563reset(Ctlr *ctlr)
{
	int i, r;

	if(i82563detach(ctlr))
		return -1;
	if(ctlr->type == i82566 || ctlr->type == i82567)
		r = fload(ctlr);
	else
		r = eeload(ctlr);
	if (r != 0 && r != 0xBABA){
		print("%s: bad EEPROM checksum - %#.4ux\n",
			tname[ctlr->type], r);
		return -1;
	}

	for(i = 0; i < Eaddrlen/2; i++){
		ctlr->ra[2*i]   = ctlr->eeprom[Ea+i];
		ctlr->ra[2*i+1] = ctlr->eeprom[Ea+i] >> 8;
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
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta + i*4, 0);

	/*
	 * Does autonegotiation affect this manual setting?
	 * The correct values here should depend on the PBA value
	 * and maximum frame length, no?
	 * ctlr->fcrt[lh] are never set, so default to 0.
	 */
	csr32w(ctlr, Fcal, 0x00C28001);
	csr32w(ctlr, Fcah, 0x0100);
	csr32w(ctlr, Fct, 0x8808);
	csr32w(ctlr, Fcttv, 0x0100);

	ctlr->fcrtl = ctlr->fcrth = 0;
	// ctlr->fcrtl = 0x00002000;
	// ctlr->fcrth = 0x00004000;
	csr32w(ctlr, Fcrtl, ctlr->fcrtl);
	csr32w(ctlr, Fcrth, ctlr->fcrth);

	return 0;
}

static void
i82563pci(void)
{
	int type;
	ulong io;
	void *mem;
	Pcidev *p;
	Ctlr *ctlr;

	p = nil;
	while(p = pcimatch(p, 0x8086, 0)){
		switch(p->did){
		default:
			continue;
		case 0x1096:
		case 0x10ba:
			type = i82563;
			break;
		case 0x1049:		/* mm */
		case 0x104a:		/* dm */
		case 0x104b:		/* dc */
		case 0x104d:		/* mc */
		case 0x10bd:		/* dm */
		case 0x294c:		/* dc-2 */
			type = i82566;
			break;
		case 0x10cd:		/* lf */
		case 0x10ce:		/* v-2 */
		case 0x10de:		/* lm-3 */
		case 0x10f5:		/* lm-2 */
			type = i82567;
			break;
		case 0x10a4:
		case 0x105e:
			type = i82571;
			break;
		case 0x10b9:		/* sic, 82572gi */
			type = i82572;
			break;
		case 0x108b:		/*  v */
		case 0x108c:		/*  e (iamt) */
		case 0x109a:		/*  l */
			type = i82573;
			break;
//		case 0x10d3:		/* l */
//			type = i82574;	/* never heard of it */
//			break;
		case 0x10a7:	/* 82575eb: one of a pair of controllers */
			type = i82575;
			break;
		case 0x10c9:		/* 82576 copper */
		case 0x10e6:		/* 82576 fiber */
		case 0x10e7:		/* 82576 serdes */
			type = i82576;
			break;
		}

		io = p->mem[0].bar & ~0x0F;
		mem = vmap(io, p->mem[0].size);
		if(mem == nil){
			print("%s: can't map %.8lux\n", tname[type], io);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = io;
		ctlr->pcidev = p;
		ctlr->type = type;
		ctlr->rbsz = rbtab[type];
		ctlr->nic = mem;

		if(i82563reset(ctlr)){
			vunmap(mem, p->mem[0].size);
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
pnp(Ether* edev, int type)
{
	Ctlr *ctlr;
	static int done;

	if(!done) {
		i82563pci();
		done = 1;
	}

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = i82563ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(type != Iany && ctlr->type != type)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	ctlr->edev = edev;			/* point back to Ether* */
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->maxmtu = ctlr->rbsz;
	memmove(edev->ea, ctlr->ra, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = i82563attach;
	edev->transmit = i82563transmit;
	edev->interrupt = (ctlr->type == i82575?
		i82575interrupt: i82563interrupt);
	edev->ifstat = i82563ifstat;
	edev->ctl = i82563ctl;

	edev->arg = edev;
	edev->promiscuous = i82563promiscuous;
	edev->shutdown = i82563shutdown;
	edev->multicast = i82563multicast;

	return 0;
}

static int
anypnp(Ether *e)
{
	return pnp(e, Iany);
}

static int
i82563pnp(Ether *e)
{
	return pnp(e, i82563);
}

static int
i82566pnp(Ether *e)
{
	return pnp(e, i82566);
}

static int
i82571pnp(Ether *e)
{
	return pnp(e, i82571);
}

static int
i82572pnp(Ether *e)
{
	return pnp(e, i82572);
}

static int
i82573pnp(Ether *e)
{
	return pnp(e, i82573);
}

static int
i82575pnp(Ether *e)
{
	return pnp(e, i82575);
}

void
ether82563link(void)
{
	/* recognise lots of model numbers for debugging assistance */
	addethercard("i82563", i82563pnp);
	addethercard("i82566", i82566pnp);
	addethercard("i82571", i82571pnp);
	addethercard("i82572", i82572pnp);
	addethercard("i82573", i82573pnp);
	addethercard("i82575", i82575pnp);
	addethercard("igbepcie", anypnp);
}
