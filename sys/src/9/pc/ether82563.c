/*
 * Intel Gigabit Ethernet PCI-Express Controllers.
 *	8256[367], 8257[1-79], 21[078]
 * Pretty basic, does not use many of the chip smarts.
 * The interrupt mitigation tuning for each chip variant
 * is probably different. The reset/initialisation
 * sequence needs straightened out. Doubt the PHY code
 * for the 82575eb is right.
 *
 * on the assumption that allowing jumbo packets makes the controller
 * much slower (as is true of the 82579), never allow jumbos.
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

#define now() TK2MS(MACHP(0)->ticks)

/*
 * these are in the order they appear in the manual, not numeric order.
 * It was too hard to find them in the book. Ref 21489, rev 2.6
 */

enum {
	/* General */
	Ctrl		= 0x0000,	/* Device Control */
	Status		= 0x0008,	/* Device Status */
	Eec		= 0x0010,	/* EEPROM/Flash Control/Data */
	Fextnvm6	= 0x0010,	/* Future Extended NVM 6 */
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
	Ert		= 0x2008,	/* Early Receive Threshold (573[EVL], 579 only) */
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
	Phyier218	= 24,		/* 218 (phy79?) phy interrupt enable */
	Phyisr218	= 25,		/* 218 (phy79?) phy interrupt status */
	Phystat		= 26,		/* 82580 (phy79?) phy status */
	Phypage		= 31,		/* page number */

	Rtlink		= 1<<10,	/* realtime link status */
	Phyan		= 1<<11,	/* phy has auto-negotiated */

	/* Phyctl bits */
	Ran		= 1<<9,		/* restart auto-negotiation */
	Ean		= 1<<12,	/* enable auto-negotiation */

	/* 82573 Phyier interrupt enable bits */
	Lscie		= 1<<10,	/* link status changed */
	Ancie		= 1<<11,	/* auto-negotiation complete */
	Spdie		= 1<<14,	/* speed changed */
	Panie		= 1<<15,	/* phy auto-negotiation error */

	/* Phylhr/Phyisr bits */
	Anf		= 1<<6,		/* lhr: auto-negotiation fault */
	Ane		= 1<<15,	/* isr: auto-negotiation error */

	/* 82580 Phystat bits */
	Ans		= 3<<14, 	/* 82580 autoneg. status */
	Link		= 1<<6,		/* 82580 link */

	/* 218 Phystat bits */
	Anfs		= 3<<13,	/* fault status */
	Ans218		= 1<<12,	/* autoneg complete */

	/* 218 Phyier218 interrupt enable bits */
	Spdie218	= 1<<1,		/* speed changed */
	Lscie218	= 1<<2,		/* link status changed */
	Ancie218	= 1<<8,		/* auto-negotiation changed */
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
	Ctmask		= 0x00000FF0,	/* Collision Threshold */
	Ctshift		= 4,
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
	Gran		= 0x01000000,	/* Granularity (descriptors, not cls) */
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

typedef struct Ctlr Ctlr;
typedef struct Rd Rd;
typedef struct Td Td;

struct Rd {				/* Receive Descriptor */
	u32int	addr[2];
	u16int	length;
	u16int	checksum;
	u8int	status;
	u8int	errors;
	u16int	special;
};

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

struct Td {				/* Transmit Descriptor */
	u32int	addr[2];		/* Data */
	u32int	control;
	u32int	status;
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
	u16int	base;
	u16int	lim;
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

/*
 * the kumeran interface is mac-to-phy for external gigabit ethernet on
 * intel's esb2 ich8 (io controller hub), it carries mii bits.  can be used
 * to reset the phy.  intel proprietary, see "kumeran specification".
 */
enum {
	I217inbandctlpage	= 770,		/* phy page */
	I217inbandctlreg	= 18,		/* phy register */
	I217inbandctllnkststxtmoutmask	= 0x3F00,
	I217inbandctllnkststxtmoutshift	= 8,

	Fextnvm6reqpllclk	= 0x100,
	Fextnvm6enak1entrycond	= 0x200,	/* extend K1 entry latency */

	Nvmk1cfg		= 0x1B,		/* NVM K1 Config Word */
	Nvmk1enable		= 0x1,		/* NVM Enable K1 bit */

	Kumctrlstaoff		= 0x1F0000,
	Kumctrlstaoffshift	= 16,
	Kumctrlstaren		= 0x200000,	/* read enable */
	Kumctrlstak1cfg		= 0x7,
	Kumctrlstak1enable	= 0x2,
};

enum {
	/*
	 * these were 512, 1024 & 64, but 52, 253 & 9 are usually ample;
	 * however cpu servers and terminals can need more receive buffers
	 * due to bursts of traffic.
	 *
	 * Tdlen and Rdlen have to be multiples of 128.  Rd and Td are both
	 * 16 bytes long, so Nrd and Ntd must be multiples of 8.
	 */
	Ntd		= 32,		/* power of two >= 8 */
	Nrd		= 128,		/* power of two >= 8 */
	Nrb		= 1024,		/* private receive buffers per Ctlr */
	Slop		= 32,		/* for vlan headers, crcs, etc. */
};

enum {
	Iany,
	i82563,
	i82566,
	i82567,
	i82571,
	i82572,
	i82573,
	i82574,
	i82575,
	i82576,
	i82577,
	i82579,
	i210,
	i217,
	i218,
};

static char *tname[] = {
[Iany]		"any",
[i82563]	"i82563",
[i82566]	"i82566",
[i82567]	"i82567",
[i82571]	"i82571",
[i82572]	"i82572",
[i82573]	"i82573",
[i82574]	"i82574",
[i82575]	"i82575",
[i82576]	"i82576",
[i82577]	"i82577",
[i82579]	"i82579",
[i210]		"i210",
[i217]		"i217",
[i218]		"i218",
};

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

	int	*nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Rendez	lrendez;
	int	lim;
	int	phynum;
	int	didk1fix;

	Watermark wmrb;
	Watermark wmrd;
	Watermark wmtd;

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
	ulong	mta[128];		/* maximal multicast table array */

	Rendez	rrendez;
	int	rim;
	int	rdfree;			/* rx descriptors awaiting packets */
	Rd	*rdba;			/* receive descriptor base address */
	Block	**rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */

	Rendez	trendez;
	QLock	tlock;
	Td	*tdba;			/* transmit descriptor base address */
	Block	**tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */

	int	fcrtl;
	int	fcrth;

	uint	pbs;			/* packet buffer size */
	uint	pba;			/* packet buffer allocation */
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr* i82563ctlrhead;
static Ctlr* i82563ctlrtail;

static Lock i82563rblock;		/* free receive Blocks */
static Block* i82563rbpool;
static int nrbfull;	/* # of rcv Blocks with data awaiting processing */

static int speedtab[] = {
	10, 100, 1000, 0
};

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

static int i82563reset(Ctlr *);

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
	if(p == nil) {
		qunlock(&ctlr->slock);
		error(Enomem);
	}
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
	p = seprint(p, e, "ctrl: %.8ux\n", csr32r(ctlr, Ctrl));
	p = seprint(p, e, "ctrlext: %.8ux\n", csr32r(ctlr, Ctrlext));
	p = seprint(p, e, "status: %.8ux\n", csr32r(ctlr, Status));
	p = seprint(p, e, "txcw: %.8ux\n", csr32r(ctlr, Txcw));
	p = seprint(p, e, "txdctl: %.8ux\n", csr32r(ctlr, Txdctl));
	p = seprint(p, e, "pbs: %dKB\n", ctlr->pbs);
	p = seprint(p, e, "pba: %#.8ux\n", ctlr->pba);

	p = seprint(p, e, "speeds: 10:%ud 100:%ud 1000:%ud ?:%ud\n",
		ctlr->speeds[0], ctlr->speeds[1], ctlr->speeds[2], ctlr->speeds[3]);
	p = seprint(p, e, "type: %s\n", tname[ctlr->type]);
	p = seprint(p, e, "nrbfull (rcv blocks outstanding): %d\n", nrbfull);

//	p = seprint(p, e, "eeprom:");
//	for(i = 0; i < 0x40; i++){
//		if(i && ((i & 7) == 0))
//			p = seprint(p, e, "\n       ");
//		p = seprint(p, e, " %4.4ux", ctlr->eeprom[i]);
//	}
//	p = seprint(p, e, "\n");

	p = seprintmark(p, e, &ctlr->wmrb);
	p = seprintmark(p, e, &ctlr->wmrd);
	p = seprintmark(p, e, &ctlr->wmtd);

	USED(p);
	n = readstr(offset, a, n, s);
	free(s);
	qunlock(&ctlr->slock);

	return n;
}

static long
i82563ctl(Ether*, void*, long)
{
	error(Enonexist);
	return 0;
}

static void
i82563promiscuous(void* arg, int on)
{
	int rctl;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	rctl = csr32r(ctlr, Rctl) & ~MoMASK;
	if(on)
		rctl |= Upe|Mpe;
	else
		rctl &= ~(Upe|Mpe);
	csr32w(ctlr, Rctl, rctl);
}

/*
 * Returns the number of bits of mac address used in multicast hash,
 * thus the number of longs of ctlr->mta (2^(bits-5)).
 * This must be right for multicast (thus ipv6) to work reliably.
 *
 * The default multicast hash for mta is based on 12 bits of MAC address;
 * the rightmost bit is a function of Rctl's Multicast Offset: 0=>36,
 * 1=>35, 2=>34, 3=>32.  Exceptions include the 578, 579, 217, 218, 219;
 * they use only 10 bits, ignoring the rightmost 2 of the 12.
 */
static int
mcastbits(Ctlr *ctlr)
{
	switch (ctlr->type) {
	/*
	 * openbsd says all `ich8' versions (ich8, ich9, ich10, pch, pch2 and
	 * pch_lpt) have 32 longs (use 10 bits of mac address for hash).
	 */
	case i82566:
	case i82567:
//	case i82578:
	case i82579:
	case i217:
	case i218:
//	case i219:
		return 10;		/* 32 longs */
	case i82563:
	case i82571:
	case i82572:
	case i82573:
	case i82574:
//	case i82575:
//	case i82583:
	case i210:			/* includes i211 */
		return 12;		/* 128 longs */
	default:
		print("82563: unsure of multicast bits in mac addresses; "
			"enabling promiscuous multicast reception\n");
		csr32w(ctlr, Rctl, csr32r(ctlr, Rctl) | Mpe);
		return 10;		/* be conservative (for mta size) */
	}
}

static int
mcbitstolongs(int nmcbits)
{
	return 1 << (nmcbits - 5);	/* 2^5 = 32 */
}

static void
i82563multicast(void* arg, uchar* addr, int on)
{
	ulong nbits, tblsz, hash, word, bit;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	nbits = mcastbits(ctlr);
	tblsz = mcbitstolongs(nbits);
	/* assume multicast offset in Rctl is 0 (we clear it above) */
	hash = addr[5] << 4 | addr[4] >> 4;	/* bits 47:36 of mac */
	if (nbits == 10)
		hash >>= 2;			/* discard 37:36 of mac */
	word = (hash / 32) & (tblsz - 1);
	bit = 1UL << (hash % 32);
	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	if(on)
		ctlr->mta[word] |= bit;
//	else
//		ctlr->mta[word] &= ~bit;
	csr32w(ctlr, Mta+word*4, ctlr->mta[word]);
}

static Block*
i82563rballoc(void)
{
	Block *bp;

	ilock(&i82563rblock);
	if((bp = i82563rbpool) != nil){
		i82563rbpool = bp->next;
		bp->next = nil;
		ainc(&bp->ref);	/* prevent bp from being freed */
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
	nrbfull--;
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
	int i, r, tctl;
	Block *bp;

	tctl = 0x0F<<Ctshift | Psp;
	switch (ctlr->type) {
	case i210:
		break;
	default:
		tctl |= Mulr;
		/* fall through */
	case i217:
	case i218:
		tctl |= 66<<ColdSHIFT;
		break;
	}
	csr32w(ctlr, Tctl, tctl);
	csr32w(ctlr, Tipg, 6<<20 | 8<<10 | 8);		/* yb sez: 0x702008 */
	for(i = 0; i < Ntd; i++)
		if((bp = ctlr->tb[i]) != nil) {
			ctlr->tb[i] = nil;
			freeb(bp);
		}
	memset(ctlr->tdba, 0, Ntd * sizeof(Td));
	coherence();
	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));
	csr32w(ctlr, Tdbah, 0);				/* 32-bit system */
	csr32w(ctlr, Tdlen, Ntd * sizeof(Td));
	ctlr->tdh = PREV(0, Ntd);
	csr32w(ctlr, Tdh, 0);
	ctlr->tdt = 0;
	csr32w(ctlr, Tdt, 0);
	csr32w(ctlr, Tidv, 0);			/* don't coalesce interrupts */
	csr32w(ctlr, Tadv, 0);
	r = csr32r(ctlr, Txdctl) & ~(WthreshMASK|PthreshMASK);
	r |= 4<<WthreshSHIFT | 4<<PthreshSHIFT;
	if(ctlr->type == i82575 || ctlr->type == i82576 || ctlr->type == i210)
		r |= Qenable;
	csr32w(ctlr, Txdctl, r);
	coherence();
	csr32w(ctlr, Tctl, csr32r(ctlr, Tctl) | Ten);
}

static int
i82563cleanup(Ctlr *ctlr)
{
	Block *bp;
	int tdh, n;

	tdh = ctlr->tdh;
	while(ctlr->tdba[n = NEXT(tdh, Ntd)].status & Tdd){
		tdh = n;
		if((bp = ctlr->tb[tdh]) != nil){
			ctlr->tb[tdh] = nil;
			freeb(bp);
		}else
			iprint("82563 tx underrun!\n");
		ctlr->tdba[tdh].status = 0;
	}
	return ctlr->tdh = tdh;
}

static void
i82563transmit(Ether* edev)
{
	Td *td;
	Block *bp;
	Ctlr *ctlr;
	int tdh, tdt;

	ctlr = edev->ctlr;
	qlock(&ctlr->tlock);

	/*
	 * Free any completed packets
	 */
	tdh = i82563cleanup(ctlr);

	/* if link down on 218, don't try since we need k1fix to run first */
	if (!edev->link && ctlr->type == i218 && !ctlr->didk1fix) {
		qunlock(&ctlr->tlock);
		return;
	}

	/*
	 * Try to fill the ring back up.
	 */
	tdt = ctlr->tdt;
	for(;;){
		if(NEXT(tdt, Ntd) == tdh){	/* ring full? */
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
		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (tdt + Ntd - tdh) % Ntd);
		tdt = NEXT(tdt, Ntd);
	}
	if(ctlr->tdt != tdt){
		ctlr->tdt = tdt;
		coherence();
		csr32w(ctlr, Tdt, tdt);
	}
	/* else may not be any new ones, but could be some still in flight */
	qunlock(&ctlr->tlock);
}

static void
i82563replenish(Ctlr* ctlr)
{
	Rd *rd;
	int rdt;
	Block *bp;

	rdt = ctlr->rdt;
	while(NEXT(rdt, Nrd) != ctlr->rdh){
		rd = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] != nil){
			print("#l%d: 82563: rx overrun\n", ctlr->edev->ctlrno);
			break;
		}
		bp = i82563rballoc();
		if(bp == nil)
			/*
			 * this almost never gets better.  likely there's a bug
			 * elsewhere in the kernel that is failing to free a
			 * receive Block.
			 */
			panic("#l%d: 82563: all %d rx buffers in use, nrbfull %d",
				ctlr->edev->ctlrno, Nrb, nrbfull);
		ctlr->rb[rdt] = bp;
		rd->addr[0] = PCIWADDR(bp->rp);
//		rd->addr[1] = 0;
		rd->status = 0;
		ctlr->rdfree++;
		rdt = NEXT(rdt, Nrd);
	}
	ctlr->rdt = rdt;
	coherence();
	csr32w(ctlr, Rdt, rdt);
}

static void
i82563rxinit(Ctlr* ctlr)
{
	Block *bp;
	int i, r, rctl, type;

	rctl = Dpf|Bsize2048|Bam|RdtmsHALF;
	type = ctlr->type;
	if(type == i82575 || type == i82576 || type == i210){
		/*
		 * Setting Qenable in Rxdctl does not
		 * appear to stick unless Ren is on.
		 */
		csr32w(ctlr, Rctl, Ren|rctl);
		csr32w(ctlr, Rxdctl, csr32r(ctlr, Rxdctl) | Qenable);
	}
	csr32w(ctlr, Rctl, rctl);

	switch (type) {
	case i82573:
	case i82577:
//	case i82577:				/* not yet implemented */
	case i82579:
	case i210:
	case i217:
	case i218:
		csr32w(ctlr, Ert, 1024/8);	/* early rx threshold */
		break;
	}

	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);			/* 32-bit system */
	csr32w(ctlr, Rdlen, Nrd * sizeof(Rd));
	ctlr->rdh = ctlr->rdt = 0;
	csr32w(ctlr, Rdh, 0);
	csr32w(ctlr, Rdt, 0);

	/* to hell with interrupt moderation, we want low latency */
	csr32w(ctlr, Rdtr, 0);
	csr32w(ctlr, Radv, 0);

	for(i = 0; i < Nrd; i++)
		if((bp = ctlr->rb[i]) != nil){
			ctlr->rb[i] = nil;
			freeb(bp);
		}
	i82563replenish(ctlr);

	if(type == i82575 || type == i82576 || type == i210){
		/*
		 * See comment above for Qenable.
		 * Could shuffle the code?
		 */
		r = csr32r(ctlr, Rxdctl) & ~(WthreshMASK|PthreshMASK);
		csr32w(ctlr, Rxdctl, r | 2<<WthreshSHIFT | 2<<PthreshSHIFT);
	}

	/*
	 * Don't enable checksum offload.  In practice, it interferes with
	 * tftp booting on at least the 82575.
	 */
	csr32w(ctlr, Rxcsum, 0);
}

static int
i82563rim(void* ctlr)
{
	return ((Ctlr*)ctlr)->rim != 0;
}

/*
 * With no errors and the Ixsm bit set,
 * the descriptor status Tpcs and Ipcs bits give
 * an indication of whether the checksums were
 * calculated and valid.
 *
 * Must be called with rd->errors == 0.
 */
static void
ckcksums(Ctlr *ctlr, Rd *rd, Block *bp)
{
if (0) {
	if(rd->status & Ixsm)
		return;
	ctlr->ixsm++;
	if(rd->status & Ipcs){
		/*
		 * IP checksum calculated (and valid as errors == 0).
		 */
		ctlr->ipcs++;
		bp->flag |= Bipck;
	}
	if(rd->status & Tcpcs){
		/*
		 * TCP/UDP checksum calculated (and valid as errors == 0).
		 */
		ctlr->tcpcs++;
		bp->flag |= Btcpck|Budpck;
	}
	bp->checksum = rd->checksum;
	bp->flag |= Bpktck;
}
}

static void
i82563rproc(void* arg)
{
	Rd *rd;
	Block *bp;
	Ctlr *ctlr;
	int rdh, rim, passed;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	i82563rxinit(ctlr);
	coherence();
	csr32w(ctlr, Rctl, csr32r(ctlr, Rctl) | Ren);

	if(ctlr->type == i210)
		csr32w(ctlr, Rxdctl, csr32r(ctlr, Rxdctl) | Qenable);

	for(;;){
		i82563replenish(ctlr);
		i82563im(ctlr, Rxt0|Rxo|Rxdmt0|Rxseq|Ack);
		ctlr->rsleep++;
		sleep(&ctlr->rrendez, i82563rim, ctlr);

		rdh = ctlr->rdh;
		passed = 0;
		for(;;){
			rim = ctlr->rim;
			ctlr->rim = 0;
			rd = &ctlr->rdba[rdh];
			if(!(rd->status & Rdd))
				break;

			/*
			 * Accept eop packets with no errors.
			 */
			bp = ctlr->rb[rdh];
			if((rd->status & Reop) && rd->errors == 0){
				bp->wp += rd->length;
				bp->lim = bp->wp;	/* lie like a dog. */
				if(0)
					ckcksums(ctlr, rd, bp);
				ilock(&i82563rblock);
				nrbfull++;
				iunlock(&i82563rblock);
				notemark(&ctlr->wmrb, nrbfull);
				etheriq(edev, bp, 1);	/* pass pkt upstream */
				passed++;
			} else {
				if (rd->status & Reop && rd->errors)
					print("%s: input packet error %#ux\n",
						tname[ctlr->type], rd->errors);
				freeb(bp);
			}
			ctlr->rb[rdh] = nil;

			/* rd needs to be replenished to accept another pkt */
			rd->status = 0;
			ctlr->rdfree--;
			ctlr->rdh = rdh = NEXT(rdh, Nrd);
			/*
			 * if number of rds ready for packets is too low,
			 * set up the unready ones.
			 */
			if(ctlr->rdfree <= Nrd - 32 || (rim & Rxdmt0))
				i82563replenish(ctlr);
		}
		/* note how many rds had full buffers */
		notemark(&ctlr->wmrd, passed);
	}
}

static int
i82563lim(void* ctlr)
{
	return ((Ctlr*)ctlr)->lim != 0;
}

static int
phynum(Ctlr *ctlr)
{
	if (ctlr->phynum < 0)
		switch (ctlr->type) {
		case i82577:
//		case i82578:			/* not yet implemented */
		case i82579:
		case i217:
		case i218:
			ctlr->phynum = 2;	/* pcie phy */
			break;
		default:
			ctlr->phynum = 1;	/* gbe phy */
			break;
		}
	return ctlr->phynum;
}

static uint
phyread(Ctlr *ctlr, int reg)
{
	uint phy, i;

	if (reg >= 32)
		iprint("phyread: reg %d >= 32\n", reg);
	csr32w(ctlr, Mdic, MDIrop | phynum(ctlr)<<MDIpSHIFT | reg<<MDIrSHIFT);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = csr32r(ctlr, Mdic);
		if(phy & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((phy & (MDIe|MDIready)) != MDIready)
		return ~0;
	return phy & 0xffff;
}

static uint
phywrite(Ctlr *ctlr, int reg, ushort val)
{
	uint phy, i;

	if (reg >= 32)
		iprint("phyread: reg %d >= 32\n", reg);
	csr32w(ctlr, Mdic, MDIwop | phynum(ctlr)<<MDIpSHIFT | reg<<MDIrSHIFT |
		val);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = csr32r(ctlr, Mdic);
		if(phy & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((phy & (MDIe|MDIready)) != MDIready)
		return ~0;
	return 0;
}

static void
kmrnwrite(Ctlr *ctlr, ulong reg_addr, ushort data)
{
	csr32w(ctlr, Kumctrlsta, ((reg_addr << Kumctrlstaoffshift) &
		Kumctrlstaoff) | data);
	microdelay(2);
}

static ulong
kmrnread(Ctlr *ctlr, ulong reg_addr)
{
	csr32w(ctlr, Kumctrlsta, ((reg_addr << Kumctrlstaoffshift) &
		Kumctrlstaoff) | Kumctrlstaren);  /* write register address */
	microdelay(2);
	return csr32r(ctlr, Kumctrlsta);	/* read data */
}

/*
 * this is essentially black magic.  we blindly follow the incantations
 * prescribed by the god Intel:
 *
 * On ESB2, the MAC-to-PHY (Kumeran) interface must be configured after
 * link is up before any traffic is sent.
 *
 * workaround DMA unit hang on I218
 *
 * At 1Gbps link speed, one of the MAC's internal clocks can be stopped
 * for up to 4us when entering K1 (a power mode of the MAC-PHY
 * interconnect).  If the MAC is waiting for completion indications for 2
 * DMA write requests into Host memory (e.g.  descriptor writeback or Rx
 * packet writing) and the indications occur while the clock is stopped,
 * both indications will be missed by the MAC, causing the MAC to wait
 * for the completion indications and be unable to generate further DMA
 * write requests.  This results in an apparent hardware hang.
 *
 * Work-around the bug by disabling the de-assertion of the clock request
 * when 1Gbps link is acquired (K1 must be disabled while doing this).
 * Also, set appropriate Tx re-transmission timeouts for 10 and 100-half
 * link speeds to avoid Tx hangs.
 */
static void
k1fix(Ctlr *ctlr)
{
	int txtmout;			/* units of 10µs */
	ulong fextnvm6, status;
	ushort reg;
	Ether *edev;

	edev = ctlr->edev;
	fextnvm6 = csr32r(ctlr, Fextnvm6);
	status = csr32r(ctlr, Status);
	/* status speed bits are different on 21[78] than earlier ctlrs */
	if (edev->link && status & (Sspeed1000>>2)) {
		reg = kmrnread(ctlr, Kumctrlstak1cfg);
		kmrnwrite(ctlr, Kumctrlstak1cfg, reg & ~Kumctrlstak1enable);
		microdelay(10);
		csr32w(ctlr, Fextnvm6, fextnvm6 | Fextnvm6reqpllclk);
		kmrnwrite(ctlr, Kumctrlstak1cfg, reg);
		ctlr->didk1fix = 1;
		return;
	}
	/* else uncommon cases */

	fextnvm6 &= ~Fextnvm6reqpllclk;
	/*
	 * 217 manual claims not to have Frcdplx bit in status;
	 * 218 manual just omits the non-phy registers.
	 */
	if (!edev->link ||
	    (status & (Sspeed100>>2|Frcdplx)) == (Sspeed100>>2|Frcdplx)) {
		csr32w(ctlr, Fextnvm6, fextnvm6);
		ctlr->didk1fix = 1;
		return;
	}

	/* access other page via phy addr 1 reg 31, then access reg 16-30 */
	phywrite(ctlr, Phypage, I217inbandctlpage<<5);
	reg = phyread(ctlr, I217inbandctlreg) & ~I217inbandctllnkststxtmoutmask;
	if (status & (Sspeed100>>2)) {		/* 100Mb/s half-duplex? */
		txtmout = 5;
		fextnvm6 &= ~Fextnvm6enak1entrycond;
	} else {				/* 10Mb/s */
		txtmout = 50;
		fextnvm6 |= Fextnvm6enak1entrycond;
	}
	phywrite(ctlr, I217inbandctlreg, reg |
		txtmout << I217inbandctllnkststxtmoutshift);
	csr32w(ctlr, Fextnvm6, fextnvm6);
	phywrite(ctlr, Phypage, 0<<5);		/* reset page to usual 0 */
	ctlr->didk1fix = 1;
}

/*
 * watch for changes of link state
 */
static void
i82563lproc(void *v)
{
	uint phy, sp, a, phy79, prevlink;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	phy79 = 0;
	switch (ctlr->type) {
	case i82579:
//	case i82580:
	case i217:
	case i218:
//	case i219:
//	case i350:
//	case i354:
		phy79 = 1;
		break;
	}
	if(ctlr->type == i82573 && (phy = phyread(ctlr, Phyier)) != ~0)
		phywrite(ctlr, Phyier, phy | Lscie | Ancie | Spdie | Panie);
	else if(phy79 && (phy = phyread(ctlr, Phyier218)) != ~0)
		phywrite(ctlr, Phyier218, phy | Lscie218 | Ancie218 | Spdie218);
	prevlink = 0;
	for(;;){
		a = 0;
		phy = phyread(ctlr, phy79? Phystat: Physsr);
		if(phy == ~0)
			goto next;
		if (phy79) {
			sp = (phy>>8) & 3;
			// a = phy & (ctlr->type == i218? Anfs: Ans);
			a = phy & Anfs;
		} else {
			sp = (phy>>14) & 3;
			switch(ctlr->type){
			case i82563:
			case i210:
				a = phyread(ctlr, Phyisr) & Ane; /* a-n error */
				break;
			case i82571:
			case i82572:
			case i82575:
			case i82576:
				a = phyread(ctlr, Phylhr) & Anf; /* a-n fault */
				sp = (sp-1) & 3;
				break;
			}
		}
		if(a)
			phywrite(ctlr, Phyctl, phyread(ctlr, Phyctl) |
				Ran | Ean);	/* enable & restart autoneg */
		edev->link = (phy & (phy79? Link: Rtlink)) != 0;
		if(edev->link){
			ctlr->speeds[sp]++;
			if (speedtab[sp])
				edev->mbps = speedtab[sp];
			if (prevlink == 0 && ctlr->type == i218)
				k1fix(ctlr);	/* link newly up: kludge away */
		} else
			ctlr->didk1fix = 0;	/* force fix at next link up */
		prevlink = edev->link;
next:
		ctlr->lim = 0;
		i82563im(ctlr, Lsc);
		ctlr->lsleep++;
		sleep(&ctlr->lrendez, i82563lim, ctlr);
	}
}

static void
i82563tproc(void *v)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = v;
	ctlr = edev->ctlr;
	for(;;){
		sleep(&ctlr->trendez, return0, 0);
		i82563transmit(edev);
	}
}

/*
 * controller is buggered; shock it back to life.
 */
static void
restart(Ctlr *ctlr)
{
if (0) {
	static Lock rstlock;

	qlock(&ctlr->tlock);
	ilock(&rstlock);
	iprint("#l%d: resetting...", ctlr->edev->ctlrno);
	i82563reset(ctlr);
	/* [rt]xinit reset the ring indices */
	i82563txinit(ctlr);
	i82563rxinit(ctlr);
	coherence();
	csr32w(ctlr, Rctl, csr32r(ctlr, Rctl) | Ren);
	iunlock(&rstlock);
	qunlock(&ctlr->tlock);
	iprint("reset\n");
}
}

static void
freerbs(Ctlr *)
{
	int i;
	Block *bp;

	for(i = Nrb; i > 0; i--){
		bp = i82563rballoc();
		bp->free = nil;
		freeb(bp);
	}
}

static void
freemem(Ctlr *ctlr)
{
	freerbs(ctlr);
	free(ctlr->tb);
	ctlr->tb = nil;
	free(ctlr->rb);
	ctlr->rb = nil;
	free(ctlr->tdba);
	ctlr->tdba = nil;
	free(ctlr->rdba);
	ctlr->rdba = nil;
}

static void
i82563attach(Ether* edev)
{
	int i;
	Block *bp;
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);

	if(ctlr->attached){
		qunlock(&ctlr->alock);
		return;
	}

	if(waserror()){
		freemem(ctlr);
		qunlock(&ctlr->alock);
		nexterror();
	}

	ctlr->rdba = mallocalign(Nrd * sizeof(Rd), 128, 0, 0);
	ctlr->tdba = mallocalign(Ntd * sizeof(Td), 128, 0, 0);
	if(ctlr->rdba == nil || ctlr->tdba == nil ||
	   (ctlr->rb = malloc(Nrd*sizeof(Block*))) == nil ||
	   (ctlr->tb = malloc(Ntd*sizeof(Block*))) == nil)
		error(Enomem);

	for(i = 0; i < Nrb; i++){
		if((bp = allocb(ETHERMAXTU + Slop + BY2PG)) == nil)
			error(Enomem);
		bp->free = i82563rbfree;
		freeb(bp);
	}
	nrbfull = 0;

	ctlr->edev = edev;			/* point back to Ether* */
	ctlr->attached = 1;
	initmark(&ctlr->wmrb, Nrb, "rcv bufs unprocessed");
	initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
	initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");

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
	int icr, im, i;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	im = ctlr->im;
	i = Nrd;			/* don't livelock */
	for(icr = csr32r(ctlr, Icr); icr & ctlr->im && i-- > 0;
	    icr = csr32r(ctlr, Icr)){
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

	for (ctlr = i82563ctlrhead; ctlr != nil && ctlr->edev != nil;
	     ctlr = ctlr->next)
		i82563interrupt(nil, ctlr->edev);
}

static int
i82563detach0(Ctlr* ctlr)
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

	/*
	 * Balance Rx/Tx packet buffer.
	 * No need to set PBA register unless using jumbo, defaults to 32KB
	 * for receive. If it is changed, then have to do a MAC reset,
	 * and need to do that at the the right time as it will wipe stuff.
	 */
	ctlr->pba = csr32r(ctlr, Pba);

	/* set packet buffer size if present.  no effect until soft reset. */
	switch (ctlr->type) {
	case i82566:
	case i82567:
	case i217:
		ctlr->pbs = 16;			/* in KB */
		csr32w(ctlr, Pbs, ctlr->pbs);
		break;
	case i218:
		// after pxe or 9fat boot, pba is always 0xe0012 on i218 => 32K
		ctlr->pbs = (ctlr->pba >> 16) + (ushort)ctlr->pba;
		csr32w(ctlr, Pbs, ctlr->pbs);
		break;
	}

	r = csr32r(ctlr, Ctrl);
	if(ctlr->type == i82566 || ctlr->type == i82567 || ctlr->type == i82579)
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

	csr32w(ctlr, Ctrl, Slu | csr32r(ctlr, Ctrl));
	return 0;
}

static int
i82563detach(Ctlr* ctlr)
{
	int r;
	static Lock detlck;

	ilock(&detlck);
	r = i82563detach0(ctlr);
	iunlock(&detlck);
	return r;
}

static void
i82563shutdown(Ether* ether)
{
	i82563detach(ether->ctlr);
}

static ushort
eeread(Ctlr *ctlr, int adr)
{
	ulong n;

	csr32w(ctlr, Eerd, EEstart | adr << 2);
	for (n = 1000000; (csr32r(ctlr, Eerd) & EEdone) == 0 && n-- > 0; )
		;
	if (n == 0)
		panic("i82563: eeread stuck");
	return csr32r(ctlr, Eerd) >> 16;
}

/* load eeprom into ctlr */
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
		if((s&Scip) == 0)	/* spi cycle done? */
			return 0;
		delay(1);
		s = f->reg[Fsts];
	}
	return -1;
}

static int
fread(Ctlr *ctlr, Flash *f, int ladr)
{
	ushort s;
	ulong n;

	delay(1);
	if(fcycle(ctlr, f) == -1)
		return -1;
	f->reg[Fsts] |= Fdone;
	f->reg32[Faddr] = ladr;

	/* setup flash control register */
	s = f->reg[Fctl] & ~(0x1f << 8);
	s |= (2-1) << 8;		/* 2 bytes */
	s &= ~(2*Flcycle);		/* read */
	f->reg[Fctl] = s | Fgo;

	for (n = 1000000; (f->reg[Fsts] & Fdone) == 0 && n-- > 0; )
		;
	if (n == 0)
		panic("i82563: fread stuck");
	if(f->reg[Fsts] & (Fcerr|Ael))
		return -1;
	return f->reg32[Fdata] & 0xffff;
}

/* load flash into ctlr */
static int
fload(Ctlr *ctlr)
{
	ulong data, io, r, adr;
	ushort sum;
	Flash f;

	io = ctlr->pcidev->mem[1].bar & ~0x0f;
	f.reg = vmap(io, ctlr->pcidev->mem[1].size);
	if(f.reg == nil)
		return -1;
	f.reg32 = (void*)f.reg;
	f.base = f.reg32[Bfpr] & FMASK(0, 13);
	f.lim = (f.reg32[Bfpr]>>16) & FMASK(0, 13);
	if(csr32r(ctlr, Eec) & (1<<22))
		f.base += (f.lim + 1 - f.base) >> 1;
	r = f.base << 12;

	sum = 0;
	for (adr = 0; adr < 0x40; adr++) {
		data = fread(ctlr, &f, r + adr*2);
		if(data == -1)
			break;
		ctlr->eeprom[adr] = data;
		sum += data;
	}
	vunmap(f.reg, ctlr->pcidev->mem[1].size);
	return sum;
}

static int
i82563reset(Ctlr *ctlr)
{
	int i, r, type;

	if(i82563detach(ctlr)) {
		iprint("82563 reset: detach failed\n");
		return -1;
	}
	type = ctlr->type;
	if (ctlr->ra[Eaddrlen - 1] != 0)
		goto macset;
	switch (type) {
	case i82566:
	case i82567:
	case i82577:
//	case i82578:			/* not yet implemented */
	case i82579:
	case i217:
	case i218:
		r = fload(ctlr);
		break;
	default:
		r = eeload(ctlr);
		break;
	}
	if (r != 0 && r != 0xBABA){
		print("%s: bad EEPROM checksum - %#.4ux\n",
			tname[type], r);
		return -1;
	}

	/* set mac addr */
	for(i = 0; i < Eaddrlen/2; i++){
		ctlr->ra[2*i]   = ctlr->eeprom[Ea+i];
		ctlr->ra[2*i+1] = ctlr->eeprom[Ea+i] >> 8;
	}
	/* ea ctlr[1] = ea ctlr[0]+1 */
	ctlr->ra[5] += (csr32r(ctlr, Status) & Lanid) >> 2;
	/*
	 * zero other mac addresses.
	 * AV bits should be zeroed by master reset & there may only be 11
	 * other registers on e.g., the i217.
	 */
	for(i = 1; i < 12; i++){		/* `12' used to be `16' here */
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
macset:
	csr32w(ctlr, Ral, ctlr->ra[3]<<24 | ctlr->ra[2]<<16 | ctlr->ra[1]<<8 |
		ctlr->ra[0]);			/* low mac addr */
	/* address valid | high mac addr */
	csr32w(ctlr, Rah, 0x80000000 | ctlr->ra[5]<<8 | ctlr->ra[4]);

	/* populate multicast table */
	for(i = 0; i < mcbitstolongs(mcastbits(ctlr)); i++)
		csr32w(ctlr, Mta + i*4, ctlr->mta[i]);

	/*
	 * Does autonegotiation affect this manual setting?
	 * The correct values here should depend on the PBA value
	 * and maximum frame length, no?
	 */
	/* fixed flow control ethernet address 0x0180c2000001 */
	csr32w(ctlr, Fcal, 0x00C28001);
	csr32w(ctlr, Fcah, 0x0100);
	if (type != i82579 && type != i210 && type != i217 && type != i218)
		/* flow control type, dictated by Intel */
		csr32w(ctlr, Fct, 0x8808);
	csr32w(ctlr, Fcttv, 0x0100);		/* for XOFF frame */
	// ctlr->fcrtl = 0x00002000;		/* rcv low water mark: 8KB */
	/* rcv high water mark: 16KB, < rcv buffer in PBA & RXA */
	// ctlr->fcrth = 0x00004000;
	ctlr->fcrtl = ctlr->fcrth = 0;
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
		case 0x107d:		/* eb copper */
		case 0x107e:		/* ei fiber */
		case 0x107f:		/* ei */
		case 0x10b9:		/* sic, 82572gi */
			type = i82572;
			break;
		case 0x108b:		/*  v */
		case 0x108c:		/*  e (iamt) */
		case 0x109a:		/*  l */
			type = i82573;
			break;
		case 0x10d3:		/* l */
			type = i82574;
			break;
		case 0x10a7:	/* 82575eb: one of a pair of controllers */
			type = i82575;
			break;
		case 0x10c9:		/* 82576 copper */
		case 0x10e6:		/* 82576 fiber */
		case 0x10e7:		/* 82576 serdes */
			type = i82576;
			break;
		case 0x10ea:		/* 82577lm */
			type = i82577;
			break;
		case 0x1502:		/* 82579lm */
		case 0x1503:		/* 82579v */
			type = i82579;
			break;
		case 0x1533:		/* i210-t1 */
		case 0x1534:		/* i210 */
		case 0x1536:		/* i210-fiber */
		case 0x1537:		/* i210-backplane */
		case 0x1538:
		case 0x1539:		/* i211 */
		case 0x157b:		/* i210 */
		case 0x157c:		/* i210 */
			type = i210;
			break;
		case 0x153a:		/* i217-lm */
		case 0x153b:		/* i217-v */
			type = i217;
			break;
		case 0x15a3:		/* i218 */
			type = i218;
			break;
		}

		io = p->mem[0].bar & ~0x0F;
		mem = vmap(io, p->mem[0].size);
		if(mem == nil){
			print("%s: can't map %.8lux\n", tname[type], io);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil) {
			vunmap(mem, p->mem[0].size);
			error(Enomem);
		}
		ctlr->port = io;
		ctlr->pcidev = p;
		ctlr->type = type;
		ctlr->nic = mem;
		ctlr->phynum = -1;		/* not yet known */

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
	edev->maxmtu = ETHERMAXTU;
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

static int
i82579pnp(Ether *e)
{
	return pnp(e, i82579);
}

static int
i210pnp(Ether *e)
{
	return pnp(e, i210);
}

static int
i217pnp(Ether *e)
{
	return pnp(e, i217);
}

static int
i218pnp(Ether *e)
{
	return pnp(e, i218);
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
	addethercard("i82579", i82579pnp);
	addethercard("i210", i210pnp);
	addethercard("i217", i217pnp);
	addethercard("i218", i218pnp);
	addethercard("igbepcie", anypnp);
}
