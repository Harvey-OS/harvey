/*
 * bootstrap driver for
 * Intel RS-82543GC Gigabit Ethernet Controller
 * as found on the Intel PRO/1000[FT] Server Adapter.
 * The older non-[FT] cards use the 82542 (LSI L2A1157) chip; no attempt
 * is made to handle the older chip although it should be possible.
 *
 * updated just enough to cope with the
 * Intel 8254[0347]NN Gigabit Ethernet Controller
 * as found on the Intel PRO/1000 series of adapters:
 *	82540EM Intel PRO/1000 MT
 *	82543GC	Intel PRO/1000 T
 *	82544EI Intel PRO/1000 XT
 *	82547EI built-in
 *
 * The datasheet is not very clear about running on a big-endian system
 * and this driver assumes little-endian throughout.
 * To do:
 *	GMII/MII
 *	check recovery from receive no buffers condition
 *	automatic ett adjustment
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"
#include "ethermii.h"

enum {
	i82542     = (0x1000<<16)|0x8086,
	i82543gc   = (0x1004<<16)|0x8086,
	i82544ei   = (0x1008<<16)|0x8086,
	i82547ei   = (0x1019<<16)|0x8086,
	i82540em   = (0x100E<<16)|0x8086,
	i82540eplp = (0x101E<<16)|0x8086,
	i82547gi   = (0x1075<<16)|0x8086,
	i82541gi   = (0x1076<<16)|0x8086,
	i82546gb   = (0x1079<<16)|0x8086,
	i82541pi   = (0x107c<<16)|0x8086,
	i82546eb   = (0x1010<<16)|0x8086,
};

/* compatibility with cpu kernels */
#define iallocb allocb
#ifndef CACHELINESZ
#define CACHELINESZ	32		/* pentium & later */
#endif

/* from pci.c */
enum
{					/* command register (pcidev->pcr) */
	IOen		= (1<<0),
	MEMen		= (1<<1),
	MASen		= (1<<2),
	MemWrInv	= (1<<4),
	PErrEn		= (1<<6),
	SErrEn		= (1<<8),
};
enum {
	Ctrl		= 0x00000000,	/* Device Control */
	Status		= 0x00000008,	/* Device Status */
	Eecd		= 0x00000010,	/* EEPROM/Flash Control/Data */
	Ctrlext		= 0x00000018,	/* Extended Device Control */
	Mdic		= 0x00000020,	/* MDI Control */
	Fcal		= 0x00000028,	/* Flow Control Address Low */
	Fcah		= 0x0000002C,	/* Flow Control Address High */
	Fct		= 0x00000030,	/* Flow Control Type */
	Icr		= 0x000000C0,	/* Interrupt Cause Read */
	Ics		= 0x000000C8,	/* Interrupt Cause Set */
	Ims		= 0x000000D0,	/* Interrupt Mask Set/Read */
	Imc		= 0x000000D8,	/* Interrupt mask Clear */
	Rctl		= 0x00000100,	/* Receive Control */
	Fcttv		= 0x00000170,	/* Flow Control Transmit Timer Value */
	Txcw		= 0x00000178,	/* Transmit Configuration Word */
	Tctl		= 0x00000400,	/* Transmit Control */
	Tipg		= 0x00000410,	/* Transmit IPG */
	Tbt		= 0x00000448,	/* Transmit Burst Timer */
	Ait		= 0x00000458,	/* Adaptive IFS Throttle */
	Fcrtl		= 0x00002160,	/* Flow Control RX Threshold Low */
	Fcrth		= 0x00002168,	/* Flow Control Rx Threshold High */
	Rdbal		= 0x00002800,	/* Rdesc Base Address Low */
	Rdbah		= 0x00002804,	/* Rdesc Base Address High */
	Rdlen		= 0x00002808,	/* Receive Descriptor Length */
	Rdh		= 0x00002810,	/* Receive Descriptor Head */
	Rdt		= 0x00002818,	/* Receive Descriptor Tail */
	Rdtr		= 0x00002820,	/* Receive Descriptor Timer Ring */
	Rxdctl		= 0x00002828,	/* Receive Descriptor Control */
	Radv		= 0x0000282C,	/* Receive Interrupt Absolute Delay Timer */
	Txdmac		= 0x00003000,	/* Transfer DMA Control */
	Ett		= 0x00003008,	/* Early Transmit Control */
	Tdbal		= 0x00003800,	/* Tdesc Base Address Low */
	Tdbah		= 0x00003804,	/* Tdesc Base Address High */
	Tdlen		= 0x00003808,	/* Transmit Descriptor Length */
	Tdh		= 0x00003810,	/* Transmit Descriptor Head */
	Tdt		= 0x00003818,	/* Transmit Descriptor Tail */
	Tidv		= 0x00003820,	/* Transmit Interrupt Delay Value */
	Txdctl		= 0x00003828,	/* Transmit Descriptor Control */
	Tadv		= 0x0000382C,	/* Transmit Interrupt Absolute Delay Timer */

	Statistics	= 0x00004000,	/* Start of Statistics Area */
	Gorcl		= 0x88/4,	/* Good Octets Received Count */
	Gotcl		= 0x90/4,	/* Good Octets Transmitted Count */
	Torl		= 0xC0/4,	/* Total Octets Received */
	Totl		= 0xC8/4,	/* Total Octets Transmitted */
	Nstatistics	= 64,

	Rxcsum		= 0x00005000,	/* Receive Checksum Control */
	Mta		= 0x00005200,	/* Multicast Table Array */
	Ral		= 0x00005400,	/* Receive Address Low */
	Rah		= 0x00005404,	/* Receive Address High */
	Manc		= 0x00005820,	/* Management Control */
};

enum {					/* Ctrl */
	Bem		= 0x00000002,	/* Big Endian Mode */
	Prior		= 0x00000004,	/* Priority on the PCI bus */
	Lrst		= 0x00000008,	/* Link Reset */
	Asde		= 0x00000020,	/* Auto-Speed Detection Enable */
	Slu		= 0x00000040,	/* Set Link Up */
	Ilos		= 0x00000080,	/* Invert Loss of Signal (LOS) */
	SspeedMASK	= 0x00000300,	/* Speed Selection */
	SspeedSHIFT	= 8,
	Sspeed10	= 0x00000000,	/* 10Mb/s */
	Sspeed100	= 0x00000100,	/* 100Mb/s */
	Sspeed1000	= 0x00000200,	/* 1000Mb/s */
	Frcspd		= 0x00000800,	/* Force Speed */
	Frcdplx		= 0x00001000,	/* Force Duplex */
	SwdpinsloMASK	= 0x003C0000,	/* Software Defined Pins - lo nibble */
	SwdpinsloSHIFT	= 18,
	SwdpioloMASK	= 0x03C00000,	/* Software Defined Pins - I or O */
	SwdpioloSHIFT	= 22,
	Devrst		= 0x04000000,	/* Device Reset */
	Rfce		= 0x08000000,	/* Receive Flow Control Enable */
	Tfce		= 0x10000000,	/* Transmit Flow Control Enable */
	Vme		= 0x40000000,	/* VLAN Mode Enable */
};

enum {					/* Status */
	Lu		= 0x00000002,	/* Link Up */
	Tckok		= 0x00000004,	/* Transmit clock is running */
	Rbcok		= 0x00000008,	/* Receive clock is running */
	Txoff		= 0x00000010,	/* Transmission Paused */
	Tbimode		= 0x00000020,	/* TBI Mode Indication */
	SpeedMASK	= 0x000000C0,
	Speed10		= 0x00000000,	/* 10Mb/s */
	Speed100	= 0x00000040,	/* 100Mb/s */
	Speed1000	= 0x00000080,	/* 1000Mb/s */
	Mtxckok		= 0x00000400,	/* MTX clock is running */
	Pci66		= 0x00000800,	/* PCI Bus speed indication */
	Bus64		= 0x00001000,	/* PCI Bus width indication */
};

enum {					/* Ctrl and Status */
	Fd		= 0x00000001,	/* Full-Duplex */
	AsdvMASK	= 0x00000300,
	Asdv10		= 0x00000000,	/* 10Mb/s */
	Asdv100		= 0x00000100,	/* 100Mb/s */
	Asdv1000	= 0x00000200,	/* 1000Mb/s */
};

enum {					/* Eecd */
	Sk		= 0x00000001,	/* Clock input to the EEPROM */
	Cs		= 0x00000002,	/* Chip Select */
	Di		= 0x00000004,	/* Data Input to the EEPROM */
	Do		= 0x00000008,	/* Data Output from the EEPROM */
	Areq		= 0x00000040,	/* EEPROM Access Request */
	Agnt		= 0x00000080,	/* EEPROM Access Grant */
	Eesz256		= 0x00000200,	/* EEPROM is 256 words not 64 */
	Spi		= 0x00002000,	/* EEPROM is SPI not Microwire */
};

enum {					/* Ctrlext */
	Gpien		= 0x0000000F,	/* General Purpose Interrupt Enables */
	SwdpinshiMASK	= 0x000000F0,	/* Software Defined Pins - hi nibble */
	SwdpinshiSHIFT	= 4,
	SwdpiohiMASK	= 0x00000F00,	/* Software Defined Pins - I or O */
	SwdpiohiSHIFT	= 8,
	Asdchk		= 0x00001000,	/* ASD Check */
	Eerst		= 0x00002000,	/* EEPROM Reset */
	Ips		= 0x00004000,	/* Invert Power State */
	Spdbyps		= 0x00008000,	/* Speed Select Bypass */
};

enum {					/* EEPROM content offsets */
	Ea		= 0x00,		/* Ethernet Address */
	Cf		= 0x03,		/* Compatibility Field */
	Pba		= 0x08,		/* Printed Board Assembly number */
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

/*
 * The Mdic register isn't implemented on the 82543GC,
 * the software defined pins are used instead.
 * These definitions work for the Intel PRO/1000 T Server Adapter.
 * The direction pin bits are read from the EEPROM.
 */
enum {
	Mdd		= ((1<<2)<<SwdpinsloSHIFT),	/* data */
	Mddo		= ((1<<2)<<SwdpioloSHIFT),	/* pin direction */
	Mdc		= ((1<<3)<<SwdpinsloSHIFT),	/* clock */
	Mdco		= ((1<<3)<<SwdpioloSHIFT),	/* pin direction */
	Mdr		= ((1<<0)<<SwdpinshiSHIFT),	/* reset */
	Mdro		= ((1<<0)<<SwdpiohiSHIFT),	/* pin direction */
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

enum {					/* Manc */
	Arpen		= 0x00002000,	/* Enable ARP Request Filtering */
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

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;
	int	cls;
	ushort	eeprom[0x40];

	int*	nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Mii*	mii;

	Lock	slock;
	uint	statistics[Nstatistics];

	uchar	ra[Eaddrlen];		/* receive address */
	ulong	mta[128];		/* multicast table array */

	Rdesc*	rdba;			/* receive descriptor base address */
	Block**	rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */

	Tdesc*	tdba;			/* transmit descriptor base address */
	Lock	tdlock;
	Block**	tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */
	int	ett;			/* early transmit threshold */

	int	txcw;
	int	fcrtl;
	int	fcrth;

	/* bootstrap goo */
	Block*	bqhead;	/* transmission queue */
	Block*	bqtail;
} Ctlr;

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static void
igbeim(Ctlr* ctlr, int im)
{
	ilock(&ctlr->imlock);
	ctlr->im |= im;
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);
}

static void
igbeattach(Ether* edev)
{
	int ctl;
	Ctlr *ctlr;

	/*
	 * To do here:
	 *	one-time stuff;
	 *	start off a kproc for link status change:
	 *		adjust queue length depending on speed;
	 *		flow control.
	 *	more needed here...
	 */
	ctlr = edev->ctlr;
	igbeim(ctlr, 0);
	ctl = csr32r(ctlr, Rctl)|Ren;
	csr32w(ctlr, Rctl, ctl);
	ctl = csr32r(ctlr, Tctl)|Ten;
	csr32w(ctlr, Tctl, ctl);
}

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
txstart(Ether *edev)
{
	int tdh, tdt, len, olen;
	Ctlr *ctlr = edev->ctlr;
	Block *bp;
	Tdesc *tdesc;

	/*
	 * Try to fill the ring back up, moving buffers from the transmit q.
	 */
	tdh = PREV(ctlr->tdh, Ntdesc);
	for(tdt = ctlr->tdt; tdt != tdh; tdt = NEXT(tdt, Ntdesc)){
		/* pull off the head of the transmission queue */
		if((bp = ctlr->bqhead) == nil)		/* was qget(edev->oq) */
			break;
		ctlr->bqhead = bp->next;
		if (ctlr->bqtail == bp)
			ctlr->bqtail = nil;
		len = olen = BLEN(bp);

		/*
		 * if packet is too short, make it longer rather than relying
		 * on ethernet interface to pad it and complain so the caller
		 * will get fixed.  I don't think Psp is working right, or it's
		 * getting cleared.
		 */
		if (len < ETHERMINTU) {
			if (bp->rp + ETHERMINTU <= bp->lim)
				bp->wp = bp->rp + ETHERMINTU;
			else
				bp->wp = bp->lim;
			len = BLEN(bp);
			print("txstart: extended short pkt %d -> %d bytes\n",
				olen, len);
		}

		/* set up a descriptor for it */
		tdesc = &ctlr->tdba[tdt];
		tdesc->addr[0] = PCIWADDR(bp->rp);
		tdesc->addr[1] = 0;
		tdesc->control = /* Ide| */ Rs|Dext|Ifcs|Teop|DtypeDD|len;
		tdesc->status = 0;

		ctlr->tb[tdt] = bp;
	}
	ctlr->tdt = tdt;
	csr32w(ctlr, Tdt, tdt);
	igbeim(ctlr, Txdw);
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
igbetransmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	Tdesc *tdesc;
	RingBuf *tb;
	int tdh;

	/*
	 * For now there are no smarts here. Tuning comes later.
	 */
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
		if(tdesc->status & Tu){
			ctlr->ett++;
			csr32w(ctlr, Ett, ctlr->ett);
		}
		tdesc->status = 0;
		if(ctlr->tb[tdh] != nil){
			freeb(ctlr->tb[tdh]);
			ctlr->tb[tdh] = nil;
		}
		tdh = NEXT(tdh, Ntdesc);
	}
	ctlr->tdh = tdh;

	/* copy packets from the software RingBuf to the transmission q */
	/* from boot ether83815.c */
	while((tb = &edev->tb[edev->ti])->owner == Interface){
		bp = fromringbuf(edev);

		/* put the buffer on the transmit queue */
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
igbereplenish(Ctlr* ctlr)
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
	}
	/* else no one is expecting packets from the network */
}

static void
igbeinterrupt(Ureg*, void* arg)
{
	Block *bp;
	Ctlr *ctlr;
	Ether *edev;
	Rdesc *rdesc;
	int icr, im, rdh, txdw = 0;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	im = ctlr->im;

	for(icr = csr32r(ctlr, Icr); icr & ctlr->im; icr = csr32r(ctlr, Icr)){
		/*
		 * Link status changed.
		 */
		if(icr & (Rxseq|Lsc)){
			/*
			 * More here...
			 */
		}

		/*
		 * Process any received packets.
		 */
		rdh = ctlr->rdh;
		for(;;){
			rdesc = &ctlr->rdba[rdh];
			if(!(rdesc->status & Rdd))
				break;
			if ((rdesc->status & Reop) && rdesc->errors == 0) {
				bp = ctlr->rb[rdh];
				ctlr->rb[rdh] = nil;
				/*
				 * it appears that the original 82543 needed
				 * to have the Ethernet CRC excluded, but that
				 * the newer chips do not?
				 */
				bp->wp += rdesc->length /* -4 */;
				toringbuf(edev, bp);
				freeb(bp);
			} else if ((rdesc->status & Reop) && rdesc->errors)
				print("igbe: input packet error 0x%ux\n",
					rdesc->errors);
			rdesc->status = 0;
			rdh = NEXT(rdh, Nrdesc);
		}
		ctlr->rdh = rdh;

		if(icr & Rxdmt0)
			igbereplenish(ctlr);
		if(icr & Txdw){
			im &= ~Txdw;
			txdw++;
		}
	}

	ctlr->im = im;
	csr32w(ctlr, Ims, im);
	iunlock(&ctlr->imlock);

	if(txdw)
		igbetransmit(edev);
}

static int
igbeinit(Ether* edev)
{
	int csr, i, r, ctrl;
	MiiPhy *phy;
	Ctlr *ctlr;

	ctlr = edev->ctlr;

	/*
	 * Set up the receive addresses.
	 * There are 16 addresses. The first should be the MAC address.
	 * The others are cleared and not marked valid (MS bit of Rah).
	 */
	csr = (edev->ea[3]<<24)|(edev->ea[2]<<16)|(edev->ea[1]<<8)|edev->ea[0];
	csr32w(ctlr, Ral, csr);
	csr = 0x80000000|(edev->ea[5]<<8)|edev->ea[4];
	csr32w(ctlr, Rah, csr);
	for(i = 1; i < 16; i++){
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}

	/*
	 * Clear the Multicast Table Array.
	 * It's a 4096 bit vector accessed as 128 32-bit registers.
	 */
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta+i*4, 0);

	/*
	 * Receive initialisation.
	 * Mostly defaults from the datasheet, will
	 * need some tuning for performance:
	 *	Rctl	descriptor mimimum threshold size
	 *		discard pause frames
	 *		strip CRC
	 * 	Rdtr	interrupt delay
	 * 	Rxdctl	all the thresholds
	 */
	csr32w(ctlr, Rctl, 0);

	/*
	 * Allocate the descriptor ring and load its
	 * address and length into the NIC.
	 */
	ctlr->rdba = xspanalloc(Nrdesc*sizeof(Rdesc), 128 /* was 16 */, 0);
	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, Nrdesc*sizeof(Rdesc));

	/*
	 * Initialise the ring head and tail pointers and
	 * populate the ring with Blocks.
	 * The datasheet says the tail pointer is set to beyond the last
	 * descriptor hardware can process, which implies the initial
	 * condition is Rdh == Rdt. However, experience shows Rdt must
	 * always be 'behind' Rdh; the replenish routine ensures this.
	 */
	ctlr->rdh = 0;
	csr32w(ctlr, Rdh, ctlr->rdh);
	ctlr->rdt = 0;
	csr32w(ctlr, Rdt, ctlr->rdt);
	ctlr->rb = malloc(sizeof(Block*)*Nrdesc);
	igbereplenish(ctlr);

	/*
	 * Set up Rctl but don't enable receiver (yet).
	 */
	csr32w(ctlr, Rdtr, 0);
	switch(ctlr->id){
	case i82540em:
	case i82540eplp:
	case i82541gi:
	case i82546gb:
	case i82546eb:
	case i82547gi:
	case i82541pi:
		csr32w(ctlr, Radv, 64);
		break;
	}
	csr32w(ctlr, Rxdctl, (8<<WthreshSHIFT)|(8<<HthreshSHIFT)|4);
	/*
	 * Enable checksum offload.
	 */
	csr32w(ctlr, Rxcsum, Tuofl|Ipofl|(ETHERHDRSIZE<<PcssSHIFT));

	csr32w(ctlr, Rctl, Dpf|Bsize2048|Bam|RdtmsHALF);
	igbeim(ctlr, Rxt0|Rxo|Rxdmt0|Rxseq);

	/*
	 * Transmit initialisation.
	 * Mostly defaults from the datasheet, will
	 * need some tuning for performance. The normal mode will
	 * be full-duplex and things to tune for half-duplex are
	 *	Tctl	re-transmit on late collision
	 *	Tipg	all IPG times
	 *	Tbt	burst timer
	 *	Ait	adaptive IFS throttle
	 * and in general
	 *	Txdmac	packet prefetching
	 *	Ett	transmit early threshold
	 *	Tidv	interrupt delay value
	 *	Txdctl	all the thresholds
	 */
	csr32w(ctlr, Tctl, (0x0F<<CtSHIFT)|Psp|(66<<ColdSHIFT));	/* Fd */
	switch(ctlr->id){
	default:
		r = 6;
		break;
	case i82543gc:
	case i82544ei:
	case i82547ei:
	case i82540em:
	case i82540eplp:
	case i82541gi:
	case i82541pi:
	case i82546gb:
	case i82546eb:
	case i82547gi:
		r = 8;
		break;
	}
	csr32w(ctlr, Tipg, (6<<20)|(8<<10)|r);
	csr32w(ctlr, Ait, 0);
	csr32w(ctlr, Txdmac, 0);
	csr32w(ctlr, Tidv, 128);

	/*
	 * Allocate the descriptor ring and load its
	 * address and length into the NIC.
	 */
	ctlr->tdba = xspanalloc(Ntdesc*sizeof(Tdesc), 128 /* was 16 */, 0);
	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));
	csr32w(ctlr, Tdbah, 0);
	csr32w(ctlr, Tdlen, Ntdesc*sizeof(Tdesc));

	/*
	 * Initialise the ring head and tail pointers.
	 */
	ctlr->tdh = 0;
	csr32w(ctlr, Tdh, ctlr->tdh);
	ctlr->tdt = 0;
	csr32w(ctlr, Tdt, ctlr->tdt);
	ctlr->tb = malloc(sizeof(Block*)*Ntdesc);
//	ctlr->im |= Txqe|Txdw;

	r = (4<<WthreshSHIFT)|(4<<HthreshSHIFT)|(8<<PthreshSHIFT);
	switch(ctlr->id){
	default:
		break;
	case i82540em:
	case i82540eplp:
	case i82547gi:
	case i82546gb:
	case i82546eb:
	case i82541gi:
	case i82541pi:
		r = csr32r(ctlr, Txdctl);
		r &= ~WthreshMASK;
		r |= Gran|(4<<WthreshSHIFT);

		csr32w(ctlr, Tadv, 64);
		break;
	}
	csr32w(ctlr, Txdctl, r);

	r = csr32r(ctlr, Tctl);
	r |= Ten;
	csr32w(ctlr, Tctl, r);

	if(ctlr->mii == nil || ctlr->mii->curphy == nil) {
		print("igbe: no mii (yet)\n");
		return 0;
	}
	/* wait for the link to come up */
	if (miistatus(ctlr->mii) < 0)
		return -1;
	print("igbe: phy: ");
	phy = ctlr->mii->curphy;
	if (phy->fd)
		print("full duplex");
	else
		print("half duplex");
	print(", %d Mb/s\n", phy->speed);

	/*
	 * Flow control.
	 */
	ctrl = csr32r(ctlr, Ctrl);
	if(phy->rfc)
		ctrl |= Rfce;
	if(phy->tfc)
		ctrl |= Tfce;
	csr32w(ctlr, Ctrl, ctrl);

	return 0;
}

static int
i82543mdior(Ctlr* ctlr, int n)
{
	int ctrl, data, i, r;

	/*
	 * Read n bits from the Management Data I/O Interface.
	 */
	ctrl = csr32r(ctlr, Ctrl);
	r = (ctrl & ~Mddo)|Mdco;
	data = 0;
	for(i = n-1; i >= 0; i--){
		if(csr32r(ctlr, Ctrl) & Mdd)
			data |= (1<<i);
		csr32w(ctlr, Ctrl, Mdc|r);
		csr32w(ctlr, Ctrl, r);
	}
	csr32w(ctlr, Ctrl, ctrl);

	return data;
}

static int
i82543mdiow(Ctlr* ctlr, int bits, int n)
{
	int ctrl, i, r;

	/*
	 * Write n bits to the Management Data I/O Interface.
	 */
	ctrl = csr32r(ctlr, Ctrl);
	r = Mdco|Mddo|ctrl;
	for(i = n-1; i >= 0; i--){
		if(bits & (1<<i))
			r |= Mdd;
		else
			r &= ~Mdd;
		csr32w(ctlr, Ctrl, Mdc|r);
		csr32w(ctlr, Ctrl, r);
	}
	csr32w(ctlr, Ctrl, ctrl);

	return 0;
}

static int
i82543miimir(Mii* mii, int pa, int ra)
{
	int data;
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	/*
	 * MII Management Interface Read.
	 *
	 * Preamble;
	 * ST+OP+PHYAD+REGAD;
	 * TA + 16 data bits.
	 */
	i82543mdiow(ctlr, 0xFFFFFFFF, 32);
	i82543mdiow(ctlr, 0x1800|(pa<<5)|ra, 14);
	data = i82543mdior(ctlr, 18);

	if(data & 0x10000)
		return -1;

	return data & 0xFFFF;
}

static int
i82543miimiw(Mii* mii, int pa, int ra, int data)
{
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	/*
	 * MII Management Interface Write.
	 *
	 * Preamble;
	 * ST+OP+PHYAD+REGAD+TA + 16 data bits;
	 * Z.
	 */
	i82543mdiow(ctlr, 0xFFFFFFFF, 32);
	data &= 0xFFFF;
	data |= (0x05<<(5+5+2+16))|(pa<<(5+2+16))|(ra<<(2+16))|(0x02<<16);
	i82543mdiow(ctlr, data, 32);

	return 0;
}

static int
igbemiimir(Mii* mii, int pa, int ra)
{
	Ctlr *ctlr;
	int mdic, timo;

	ctlr = mii->ctlr;

	csr32w(ctlr, Mdic, MDIrop|(pa<<MDIpSHIFT)|(ra<<MDIrSHIFT));
	mdic = 0;
	for(timo = 64; timo; timo--){
		mdic = csr32r(ctlr, Mdic);
		if(mdic & (MDIe|MDIready))
			break;
		microdelay(1);
	}

	if((mdic & (MDIe|MDIready)) == MDIready)
		return mdic & 0xFFFF;
	return -1;
}

static int
igbemiimiw(Mii* mii, int pa, int ra, int data)
{
	Ctlr *ctlr;
	int mdic, timo;

	ctlr = mii->ctlr;

	data &= MDIdMASK;
	csr32w(ctlr, Mdic, MDIwop|(pa<<MDIpSHIFT)|(ra<<MDIrSHIFT)|data);
	mdic = 0;
	for(timo = 64; timo; timo--){
		mdic = csr32r(ctlr, Mdic);
		if(mdic & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((mdic & (MDIe|MDIready)) == MDIready)
		return 0;
	return -1;
}

static int
igbemii(Ctlr* ctlr)
{
	MiiPhy *phy = (MiiPhy *)1;
	int ctrl, p, r;

	USED(phy);
	r = csr32r(ctlr, Status);
	if(r & Tbimode)
		return -1;
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->ctlr = ctlr;

	ctrl = csr32r(ctlr, Ctrl);
	ctrl |= Slu;

	switch(ctlr->id){
	case i82543gc:
		ctrl |= Frcdplx|Frcspd;
		csr32w(ctlr, Ctrl, ctrl);

		/*
		 * The reset pin direction (Mdro) should already
		 * be set from the EEPROM load.
		 * If it's not set this configuration is unexpected
		 * so bail.
		 */
		r = csr32r(ctlr, Ctrlext);
		if(!(r & Mdro))
			return -1;
		csr32w(ctlr, Ctrlext, r);
		delay(20);
		r = csr32r(ctlr, Ctrlext);
		r &= ~Mdr;
		csr32w(ctlr, Ctrlext, r);
		delay(20);
		r = csr32r(ctlr, Ctrlext);
		r |= Mdr;
		csr32w(ctlr, Ctrlext, r);
		delay(20);

		ctlr->mii->mir = i82543miimir;
		ctlr->mii->miw = i82543miimiw;
		break;
	case i82544ei:
	case i82547ei:
	case i82540em:
	case i82540eplp:
	case i82547gi:
	case i82541gi:
	case i82541pi:
	case i82546gb:
	case i82546eb:
		ctrl &= ~(Frcdplx|Frcspd);
		csr32w(ctlr, Ctrl, ctrl);
		ctlr->mii->mir = igbemiimir;
		ctlr->mii->miw = igbemiimiw;
		break;
	default:
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}

	if(mii(ctlr->mii, ~0) == 0 || (phy = ctlr->mii->curphy) == nil){
		if (0)
			print("phy trouble: phy = 0x%lux\n", (ulong)phy);
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	print("oui %X phyno %d\n", phy->oui, phy->phyno);

	/*
	 * 8254X-specific PHY registers not in 802.3:
	 *	0x10	PHY specific control
	 *	0x14	extended PHY specific control
	 * Set appropriate values then reset the PHY to have
	 * changes noted.
	 */
	switch(ctlr->id){
	case i82547gi:
	case i82541gi:
	case i82541pi:
	case i82546gb:
	case i82546eb:
		break;
	default:
		r = miimir(ctlr->mii, 16);
		r |= 0x0800;			/* assert CRS on Tx */
		r |= 0x0060;			/* auto-crossover all speeds */
		r |= 0x0002;			/* polarity reversal enabled */
		miimiw(ctlr->mii, 16, r);
	
		r = miimir(ctlr->mii, 20);
		r |= 0x0070;			/* +25MHz clock */
		r &= ~0x0F00;
		r |= 0x0100;			/* 1x downshift */
		miimiw(ctlr->mii, 20, r);
	
		miireset(ctlr->mii);
		break;
	}
	p = 0;
	if(ctlr->txcw & TxcwPs)
		p |= AnaP;
	if(ctlr->txcw & TxcwAs)
		p |= AnaAP;
	miiane(ctlr->mii, ~0, p, ~0);

	return 0;
}

static int
at93c46io(Ctlr* ctlr, char* op, int data)
{
	char *lp, *p;
	int i, loop, eecd, r;

	eecd = csr32r(ctlr, Eecd);

	r = 0;
	loop = -1;
	lp = nil;
	for(p = op; *p != '\0'; p++){
		switch(*p){
		default:
			return -1;
		case ' ':
			continue;
		case ':':			/* start of loop */
			loop = strtol(p+1, &lp, 0)-1;
			lp--;
			if(p == lp)
				loop = 7;
			p = lp;
			continue;
		case ';':			/* end of loop */
			if(lp == nil)
				return -1;
			loop--;
			if(loop >= 0)
				p = lp;
			else
				lp = nil;
			continue;
		case 'C':			/* assert clock */
			eecd |= Sk;
			break;
		case 'c':			/* deassert clock */
			eecd &= ~Sk;
			break;
		case 'D':			/* next bit in 'data' byte */
			if(loop < 0)
				return -1;
			if(data & (1<<loop))
				eecd |= Di;
			else
				eecd &= ~Di;
			break;
		case 'O':			/* collect data output */
			i = (csr32r(ctlr, Eecd) & Do) != 0;
			if(loop >= 0)
				r |= (i<<loop);
			else
				r = i;
			continue;
		case 'I':			/* assert data input */
			eecd |= Di;
			break;
		case 'i':			/* deassert data input */
			eecd &= ~Di;
			break;
		case 'S':			/* enable chip select */
			eecd |= Cs;
			break;
		case 's':			/* disable chip select */
			eecd &= ~Cs;
			break;
		}
		csr32w(ctlr, Eecd, eecd);
		microdelay(1);
	}
	if(loop >= 0)
		return -1;
	return r;
}

static int
at93c46r(Ctlr* ctlr)
{
	ushort sum;
	char rop[20];
	int addr, areq, bits, data, eecd, i;

	eecd = csr32r(ctlr, Eecd);
	if(eecd & Spi){
		print("igbe: SPI EEPROM access not implemented\n");
		return 0;
	}
	if(eecd & Eesz256)
		bits = 8;
	else
		bits = 6;
	snprint(rop, sizeof(rop), "S :%dDCc;", bits+3);

	sum = 0;

	switch(ctlr->id){
	default:
		areq = 0;
		break;
	case i82540em:
	case i82540eplp:
	case i82541gi:
	case i82541pi:
	case i82547gi:
	case i82546gb:
	case i82546eb:
		areq = 1;
		csr32w(ctlr, Eecd, eecd|Areq);
		for(i = 0; i < 1000; i++){
			if((eecd = csr32r(ctlr, Eecd)) & Agnt)
				break;
			microdelay(5);
		}
		if(!(eecd & Agnt)){
			print("igbe: not granted EEPROM access\n");
			goto release;
		}
		break;
	}

	for(addr = 0; addr < 0x40; addr++){
		/*
		 * Read a word at address 'addr' from the Atmel AT93C46
		 * 3-Wire Serial EEPROM or compatible. The EEPROM access is
		 * controlled by 4 bits in Eecd. See the AT93C46 datasheet
		 * for protocol details.
		 */
		if(at93c46io(ctlr, rop, (0x06<<bits)|addr) != 0){
			print("igbe: can't set EEPROM address 0x%2.2X\n", addr);
			goto release;
		}
		data = at93c46io(ctlr, ":16COc;", 0);
		at93c46io(ctlr, "sic", 0);
		ctlr->eeprom[addr] = data;
		sum += data;
	}

release:
	if(areq)
		csr32w(ctlr, Eecd, eecd & ~Areq);
	return sum;
}

static void
detach(Ctlr *ctlr)
{
	int r;

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
	/* apparently needed on multi-GHz processors to avoid infinite loops */
	delay(1);
	while(csr32r(ctlr, Ctrl) & Devrst)
		;

	csr32w(ctlr, Ctrlext, Eerst | csr32r(ctlr, Ctrlext));
	delay(1);
	while(csr32r(ctlr, Ctrlext) & Eerst)
		;

	switch(ctlr->id){
	default:
		break;
	case i82540em:
	case i82540eplp:
	case i82541gi:
	case i82541pi:
	case i82547gi:
	case i82546gb:
	case i82546eb:
		r = csr32r(ctlr, Manc);
		r &= ~Arpen;
		csr32w(ctlr, Manc, r);
		break;
	}

	csr32w(ctlr, Imc, ~0);
	delay(1);
	while(csr32r(ctlr, Icr))
		;
}

static void
igbedetach(Ether *edev)
{
	detach(edev->ctlr);
}

static void
igbeshutdown(Ether* ether)
{
print("igbeshutdown\n");
	igbedetach(ether);
}

static int
igbereset(Ctlr* ctlr)
{
	int ctrl, i, pause, r, swdpio, txcw;

	detach(ctlr);

	/*
	 * Read the EEPROM, validate the checksum
	 * then get the device back to a power-on state.
	 */
	r = at93c46r(ctlr);
	/* zero return means no SPI EEPROM access */
	if (r != 0 && r != 0xBABA){
		print("igbe: bad EEPROM checksum - 0x%4.4uX\n", r);
		return -1;
	}

	/*
	 * Snarf and set up the receive addresses.
	 * There are 16 addresses. The first should be the MAC address.
	 * The others are cleared and not marked valid (MS bit of Rah).
	 */
	if ((ctlr->id == i82546gb || ctlr->id == i82546eb) && BUSFNO(ctlr->pcidev->tbdf) == 1)
		ctlr->eeprom[Ea+2] += 0x100;	// second interface
	for(i = Ea; i < Eaddrlen/2; i++){
		ctlr->ra[2*i] = ctlr->eeprom[i];
		ctlr->ra[2*i+1] = ctlr->eeprom[i]>>8;
	}
	r = (ctlr->ra[3]<<24)|(ctlr->ra[2]<<16)|(ctlr->ra[1]<<8)|ctlr->ra[0];
	csr32w(ctlr, Ral, r);
	r = 0x80000000|(ctlr->ra[5]<<8)|ctlr->ra[4];
	csr32w(ctlr, Rah, r);
	for(i = 1; i < 16; i++){
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}

	/*
	 * Clear the Multicast Table Array.
	 * It's a 4096 bit vector accessed as 128 32-bit registers.
	 */
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta+i*4, 0);

	/*
	 * Just in case the Eerst didn't load the defaults
	 * (doesn't appear to fully on the 8243GC), do it manually.
	 */
	if (ctlr->id == i82543gc) {
		txcw = csr32r(ctlr, Txcw);
		txcw &= ~(TxcwAne|TxcwPauseMASK|TxcwFd);
		ctrl = csr32r(ctlr, Ctrl);
		ctrl &= ~(SwdpioloMASK|Frcspd|Ilos|Lrst|Fd);
	
		if(ctlr->eeprom[Icw1] & 0x0400){
			ctrl |= Fd;
			txcw |= TxcwFd;
		}
		if(ctlr->eeprom[Icw1] & 0x0200)
			ctrl |= Lrst;
		if(ctlr->eeprom[Icw1] & 0x0010)
			ctrl |= Ilos;
		if(ctlr->eeprom[Icw1] & 0x0800)
			ctrl |= Frcspd;
		swdpio = (ctlr->eeprom[Icw1] & 0x01E0)>>5;
		ctrl |= swdpio<<SwdpioloSHIFT;
		csr32w(ctlr, Ctrl, ctrl);

		ctrl = csr32r(ctlr, Ctrlext);
		ctrl &= ~(Ips|SwdpiohiMASK);
		swdpio = (ctlr->eeprom[Icw2] & 0x00F0)>>4;
		if(ctlr->eeprom[Icw1] & 0x1000)
			ctrl |= Ips;
		ctrl |= swdpio<<SwdpiohiSHIFT;
		csr32w(ctlr, Ctrlext, ctrl);
	
		if(ctlr->eeprom[Icw2] & 0x0800)
			txcw |= TxcwAne;
		pause = (ctlr->eeprom[Icw2] & 0x3000)>>12;
		txcw |= pause<<TxcwPauseSHIFT;
		switch(pause){
		default:
			ctlr->fcrtl = 0x00002000;
			ctlr->fcrth = 0x00004000;
			txcw |= TxcwAs|TxcwPs;
			break;
		case 0:
			ctlr->fcrtl = 0x00002000;
			ctlr->fcrth = 0x00004000;
			break;
		case 2:
			ctlr->fcrtl = 0;
			ctlr->fcrth = 0;
			txcw |= TxcwAs;
			break;
		}
		ctlr->txcw = txcw;
		csr32w(ctlr, Txcw, txcw);
	}
	/*
	 * Flow control - values from the datasheet.
	 */
	csr32w(ctlr, Fcal, 0x00C28001);
	csr32w(ctlr, Fcah, 0x00000100);
	csr32w(ctlr, Fct, 0x00008808);
	csr32w(ctlr, Fcttv, 0x00000100);

	csr32w(ctlr, Fcrtl, ctlr->fcrtl);
	csr32w(ctlr, Fcrth, ctlr->fcrth);

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	ctlr->im = Lsc;
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);

	if(!(csr32r(ctlr, Status) & Tbimode) && igbemii(ctlr) < 0) {
		print("igbe: igbemii failed\n");
		return -1;
	}

	return 0;
}

static void
igbepci(void)
{
	int port, cls;
	Pcidev *p;
	Ctlr *ctlr;
	static int first = 1;

	if (first)
		first = 0;
	else
		return;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		switch((p->did<<16)|p->vid){
		case i82542:
		default:
			continue;

		case (0x1001<<16)|0x8086:	/* Intel PRO/1000 F */
			break;
		case i82543gc:
		case i82544ei:
		case i82547ei:
		case i82540em:
		case i82540eplp:
		case i82547gi:
		case i82541gi:
		case i82541pi:
		case i82546gb:
		case i82546eb:
			break;
		}

		/* the 82547EI is on the CSA bus, whatever that is */
		port = upamalloc(p->mem[0].bar & ~0x0F, p->mem[0].size, 0);
		if(port == 0){
			print("igbe: can't map %d @ 0x%8.8luX\n",
				p->mem[0].size, p->mem[0].bar);
			continue;
		}

		/*
		 * from etherga620.c:
		 * If PCI Write-and-Invalidate is enabled set the max write DMA
		 * value to the host cache-line size (32 on Pentium or later).
		 */
		if(p->pcr & MemWrInv){
			cls = pcicfgr8(p, PciCLS) * 4;
			if(cls != CACHELINESZ)
				pcicfgw8(p, PciCLS, CACHELINESZ/4);
		}

		cls = pcicfgr8(p, PciCLS);
		switch(cls){
			default:
				print("igbe: unexpected CLS - %d bytes\n",
					cls*sizeof(long));
				break;
			case 0x00:
			case 0xFF:
				/* alphapc 164lx returns 0 */
				print("igbe: unusable PciCLS: %d, using %d longs\n",
					cls, CACHELINESZ/sizeof(long));
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
		ctlr->id = (p->did<<16)|p->vid;
		ctlr->cls = cls*4;
		ctlr->nic = KADDR(ctlr->port);
print("status0 %8.8uX\n", csr32r(ctlr, Status));
		if(igbereset(ctlr)){
			free(ctlr);
			continue;
		}
print("status1 %8.8uX\n", csr32r(ctlr, Status));
		pcisetbme(p);

		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;
	}
}

int
igbepnp(Ether* edev)
{
	int i;
	Ctlr *ctlr;
	uchar ea[Eaddrlen];

	if(ctlrhead == nil)
		igbepci();

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

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to
	 * loading the station address in the hardware.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, edev->ea, Eaddrlen) == 0){
		for(i = 0; i < Eaddrlen/2; i++){
			edev->ea[2*i] = ctlr->eeprom[i];
			edev->ea[2*i+1] = ctlr->eeprom[i]>>8;
		}
	}
	igbeinit(edev);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = igbeattach;
	edev->transmit = igbetransmit;
	edev->interrupt = igbeinterrupt;
	edev->detach = igbedetach;

	return 0;
}
