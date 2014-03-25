/*
 * Intel 8254[340]NN Gigabit Ethernet PCI Controllers
 * as found on the Intel PRO/1000 series of adapters:
 *	82543GC	Intel PRO/1000 T
 *	82544EI Intel PRO/1000 XT
 *	82540EM Intel PRO/1000 MT
 *	82541[GP]I
 *	82547GI
 *	82546GB
 *	82546EB
 * To Do:
 *	finish autonegotiation code;
 *	integrate fiber stuff back in (this ONLY handles
 *	the CAT5 cards at the moment);
 *	add tuning control via ctl file;
 *	this driver is little-endian specific.
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
	i82542		= (0x1000<<16)|0x8086,
	i82543gc	= (0x1004<<16)|0x8086,
	i82544ei	= (0x1008<<16)|0x8086,
	i82544eif	= (0x1009<<16)|0x8086,
	i82544gc	= (0x100d<<16)|0x8086,
	i82540em	= (0x100E<<16)|0x8086,
	i82540eplp	= (0x101E<<16)|0x8086,
	i82545em	= (0x100F<<16)|0x8086,
	i82545gmc	= (0x1026<<16)|0x8086,
	i82547ei	= (0x1019<<16)|0x8086,
	i82547gi	= (0x1075<<16)|0x8086,
	i82541ei	= (0x1013<<16)|0x8086,
	i82541gi	= (0x1076<<16)|0x8086,
	i82541gi2	= (0x1077<<16)|0x8086,
	i82541pi	= (0x107c<<16)|0x8086,
	i82546gb	= (0x1079<<16)|0x8086,
	i82546eb	= (0x1010<<16)|0x8086,
};

enum {
	Ctrl		= 0x00000000,	/* Device Control */
	Ctrldup		= 0x00000004,	/* Device Control Duplicate */
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
	Rxcw		= 0x00000180,	/* Receive Configuration Word */
	/* on the oldest cards (8254[23]), the Mta register is at 0x200 */
	Tctl		= 0x00000400,	/* Transmit Control */
	Tipg		= 0x00000410,	/* Transmit IPG */
	Tbt		= 0x00000448,	/* Transmit Burst Timer */
	Ait		= 0x00000458,	/* Adaptive IFS Throttle */
	Fcrtl		= 0x00002160,	/* Flow Control RX Threshold Low */
	Fcrth		= 0x00002168,	/* Flow Control Rx Threshold High */
	Rdfh		= 0x00002410,	/* Receive data fifo head */
	Rdft		= 0x00002418,	/* Receive data fifo tail */
	Rdfhs		= 0x00002420,	/* Receive data fifo head saved */
	Rdfts		= 0x00002428,	/* Receive data fifo tail saved */
	Rdfpc		= 0x00002430,	/* Receive data fifo packet count */
	Rdbal		= 0x00002800,	/* Rd Base Address Low */
	Rdbah		= 0x00002804,	/* Rd Base Address High */
	Rdlen		= 0x00002808,	/* Receive Descriptor Length */
	Rdh		= 0x00002810,	/* Receive Descriptor Head */
	Rdt		= 0x00002818,	/* Receive Descriptor Tail */
	Rdtr		= 0x00002820,	/* Receive Descriptor Timer Ring */
	Rxdctl		= 0x00002828,	/* Receive Descriptor Control */
	Radv		= 0x0000282C,	/* Receive Interrupt Absolute Delay Timer */
	Txdmac		= 0x00003000,	/* Transfer DMA Control */
	Ett		= 0x00003008,	/* Early Transmit Control */
	Tdfh		= 0x00003410,	/* Transmit data fifo head */
	Tdft		= 0x00003418,	/* Transmit data fifo tail */
	Tdfhs		= 0x00003420,	/* Transmit data Fifo Head saved */
	Tdfts		= 0x00003428,	/* Transmit data fifo tail saved */
	Tdfpc		= 0x00003430,	/* Trasnmit data Fifo packet count */
	Tdbal		= 0x00003800,	/* Td Base Address Low */
	Tdbah		= 0x00003804,	/* Td Base Address High */
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

/*
 * can't find Tckok nor Rbcok in any Intel docs,
 * but even 82543gc docs define Lanid.
 */
enum {					/* Status */
	Lu		= 0x00000002,	/* Link Up */
	Lanid		= 0x0000000C,	/* mask for Lan ID. (function id) */
//	Tckok		= 0x00000004,	/* Transmit clock is running */
//	Rbcok		= 0x00000008,	/* Receive clock is running */
	Txoff		= 0x00000010,	/* Transmission Paused */
	Tbimode		= 0x00000020,	/* TBI Mode Indication */
	LspeedMASK	= 0x000000C0,	/* Link Speed Setting */
	LspeedSHIFT	= 6,
	Lspeed10	= 0x00000000,	/* 10Mb/s */
	Lspeed100	= 0x00000040,	/* 100Mb/s */
	Lspeed1000	= 0x00000080,	/* 1000Mb/s */
	Mtxckok		= 0x00000400,	/* MTX clock is running */
	Pci66		= 0x00000800,	/* PCI Bus speed indication */
	Bus64		= 0x00001000,	/* PCI Bus width indication */
	Pcixmode	= 0x00002000,	/* PCI-X mode */
	PcixspeedMASK	= 0x0000C000,	/* PCI-X bus speed */
	PcixspeedSHIFT	= 14,
	Pcix66		= 0x00000000,	/* 50-66MHz */
	Pcix100		= 0x00004000,	/* 66-100MHz */
	Pcix133		= 0x00008000,	/* 100-133MHz */
};

enum {					/* Ctrl and Status */
	Fd		= 0x00000001,	/* Full-Duplex */
	AsdvMASK	= 0x00000300,
	AsdvSHIFT	= 8,
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
	Eepresent	= 0x00000100,	/* EEPROM Present */
	Eesz256		= 0x00000200,	/* EEPROM is 256 words not 64 */
	Eeszaddr	= 0x00000400,	/* EEPROM size for 8254[17] */
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
	Rxdmt0		= 0x00000010,	/* Rd Minimum Threshold Reached */
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

enum {					/* Rxcw */
	Rxword		= 0x0000FFFF,	/* Data from auto-negotiation process */
	Rxnocarrier	= 0x04000000,	/* Carrier Sense indication */
	Rxinvalid	= 0x08000000,	/* Invalid Symbol during configuration */
	Rxchange	= 0x10000000,	/* Change to the Rxword indication */
	Rxconfig	= 0x20000000,	/* /C/ order set reception indication */
	Rxsync		= 0x40000000,	/* Lost bit synchronization indication */
	Anc		= 0x80000000,	/* Auto Negotiation Complete */
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
	RdtmsMASK	= 0x00000300,	/* Rd Minimum Threshold Size */
	RdtmsHALF	= 0x00000000,	/* Threshold is 1/2 Rdlen */
	RdtmsQUARTER	= 0x00000100,	/* Threshold is 1/4 Rdlen */
	RdtmsEIGHTH	= 0x00000200,	/* Threshold is 1/8 Rdlen */
	MoMASK		= 0x00003000,	/* Multicast Offset */
	Mo47b36		= 0x00000000,	/* bits [47:36] of received address */
	Mo46b35		= 0x00001000,	/* bits [46:35] of received address */
	Mo45b34		= 0x00002000,	/* bits [45:34] of received address */
	Mo43b32		= 0x00003000,	/* bits [43:32] of received address */
	Bam		= 0x00008000,	/* Broadcast Accept Mode */
	BsizeMASK	= 0x00030000,	/* Receive Buffer Size */
	Bsize2048	= 0x00000000,	/* Bsex = 0 */
	Bsize1024	= 0x00010000,	/* Bsex = 0 */
	Bsize512	= 0x00020000,	/* Bsex = 0 */
	Bsize256	= 0x00030000,	/* Bsex = 0 */
	Bsize16384	= 0x00010000,	/* Bsex = 1 */
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
	WthreshMASK	= 0x003F0000,	/* Writeback Threshold */
	WthreshSHIFT	= 16,
	Gran		= 0x01000000,	/* Granularity */
	LthreshMASK	= 0xFE000000,	/* Low Threshold */
	LthreshSHIFT	= 25,
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
struct Td {				/* Transmit Descriptor */
	union {
		uint	addr[2];	/* Data */
		struct {		/* Context */
			uchar	ipcss;
			uchar	ipcso;
			ushort	ipcse;
			uchar	tucss;
			uchar	tucso;
			ushort	tucse;
		};
	};
	uint	control;
	uint	status;
};

enum {					/* Td control */
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

enum {					/* Td status */
	Tdd		= 0x00000001,	/* Descriptor Done */
	Ec		= 0x00000002,	/* Excess Collisions */
	Lc		= 0x00000004,	/* Late Collision */
	Tu		= 0x00000008,	/* Transmit Underrun */
	Iixsm		= 0x00000100,	/* Insert IP Checksum */
	Itxsm		= 0x00000200,	/* Insert TCP/UDP Checksum */
	HdrlenMASK	= 0x0000FF00,	/* Header Length (Tse) */
	HdrlenSHIFT	= 8,
	VlanMASK	= 0x0FFF0000,	/* VLAN Identifier */
	VlanSHIFT	= 16,
	Tcfi		= 0x10000000,	/* Canonical Form Indicator */
	PriMASK		= 0xE0000000,	/* User Priority */
	PriSHIFT	= 29,
	MssMASK		= 0xFFFF0000,	/* Maximum Segment Size (Tse) */
	MssSHIFT	= 16,
};

enum {
	Rbsz		= 2048,
	/* were 256, 1024 & 64, but 52, 253 and 9 are ample. */
	Nrd		= 128,		/* multiple of 8 */
	Nrb		= 512,		/* private receive buffers per Ctlr */
	Ntd		= 32,		/* multiple of 8 */
};

typedef struct Ctlr Ctlr;
struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	Ether*	edev;
	int	active;
	int	started;
	int	id;
	int	cls;
	ushort	eeprom[0x40];

	QLock	alock;			/* attach */
	void*	alloc;			/* receive/transmit descriptors */
	int	nrd;
	int	ntd;
	int	nrb;			/* # bufs this Ctlr has in the pool */

	int*	nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Mii*	mii;
	Rendez	lrendez;
	int	lim;

	int	link;

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

	uchar	ra[Eaddrlen];		/* receive address */
	ulong	mta[128];		/* multicast table array */

	Rendez	rrendez;
	int	rim;
	int	rdfree;			/* rx descriptors awaiting packets */
	Rd*	rdba;			/* receive descriptor base address */
	Block**	rb;			/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */
	int	rdtr;			/* receive delay timer ring value */

	Lock	tlock;
	int	tdfree;
	Td*	tdba;			/* transmit descriptor base address */
	Block**	tb;			/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */

	int	txcw;
	int	fcrtl;
	int	fcrth;
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr* igbectlrhead;
static Ctlr* igbectlrtail;

static Lock igberblock;		/* free receive Blocks */
static Block* igberbpool;	/* receive Blocks for all igbe controllers */
static int nrbfull;	/* # of rcv Blocks with data awaiting processing */

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
igbeifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *p, *s, *e;
	int i, l, r;
	uvlong tuvl, ruvl;

	ctlr = edev->ctlr;
	qlock(&ctlr->slock);
	p = malloc(READSTR);
	if(p == nil) {
		qunlock(&ctlr->slock);
		error(Enomem);
	}
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
			l += snprint(p+l, READSTR-l, "%s: %llud %llud\n",
				s, tuvl, ruvl);
			i++;
			break;

		default:
			ctlr->statistics[i] += r;
			if(ctlr->statistics[i] == 0)
				continue;
			l += snprint(p+l, READSTR-l, "%s: %ud %ud\n",
				s, ctlr->statistics[i], r);
			break;
		}
	}

	l += snprint(p+l, READSTR-l, "lintr: %ud %ud\n",
		ctlr->lintr, ctlr->lsleep);
	l += snprint(p+l, READSTR-l, "rintr: %ud %ud\n",
		ctlr->rintr, ctlr->rsleep);
	l += snprint(p+l, READSTR-l, "tintr: %ud %ud\n",
		ctlr->tintr, ctlr->txdw);
	l += snprint(p+l, READSTR-l, "ixcs: %ud %ud %ud\n",
		ctlr->ixsm, ctlr->ipcs, ctlr->tcpcs);
	l += snprint(p+l, READSTR-l, "rdtr: %ud\n", ctlr->rdtr);
	l += snprint(p+l, READSTR-l, "Ctrlext: %08x\n", csr32r(ctlr, Ctrlext));

	l += snprint(p+l, READSTR-l, "eeprom:");
	for(i = 0; i < 0x40; i++){
		if(i && ((i & 0x07) == 0))
			l += snprint(p+l, READSTR-l, "\n       ");
		l += snprint(p+l, READSTR-l, " %4.4uX", ctlr->eeprom[i]);
	}
	l += snprint(p+l, READSTR-l, "\n");

	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		l += snprint(p+l, READSTR-l, "phy:   ");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				l += snprint(p+l, READSTR-l, "\n       ");
			r = miimir(ctlr->mii, i);
			l += snprint(p+l, READSTR-l, " %4.4uX", r);
		}
		snprint(p+l, READSTR-l, "\n");
	}
	e = p + READSTR;
	s = p + l + 1;
	s = seprintmark(s, e, &ctlr->wmrb);
	s = seprintmark(s, e, &ctlr->wmrd);
	s = seprintmark(s, e, &ctlr->wmtd);
	USED(s);

	n = readstr(offset, a, n, p);
	free(p);
	qunlock(&ctlr->slock);

	return n;
}

enum {
	CMrdtr,
};

static Cmdtab igbectlmsg[] = {
	CMrdtr,	"rdtr",	2,
};

static long
igbectl(Ether* edev, void* buf, long n)
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

	ct = lookupcmd(cb, igbectlmsg, nelem(igbectlmsg));
	switch(ct->index){
	case CMrdtr:
		v = strtol(cb->f[1], &p, 0);
		if(v < 0 || p == cb->f[1] || v > 0xFFFF)
			error(Ebadarg);
		ctlr->rdtr = v;
		csr32w(ctlr, Rdtr, Fpd|v);
		break;
	}
	free(cb);
	poperror();

	return n;
}

static void
igbepromiscuous(void* arg, int on)
{
	int rctl;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	rctl = csr32r(ctlr, Rctl);
	rctl &= ~MoMASK;
	rctl |= Mo47b36;
	if(on)
		rctl |= Upe|Mpe;
	else
		rctl &= ~(Upe|Mpe);
	csr32w(ctlr, Rctl, rctl|Mpe);	/* temporarily keep Mpe on */
}

static void
igbemulticast(void* arg, uchar* addr, int add)
{
	int bit, x;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	x = addr[5]>>1;
	bit = ((addr[5] & 1)<<4)|(addr[4]>>4);
	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	if(add)
		ctlr->mta[x] |= 1<<bit;
//	else
//		ctlr->mta[x] &= ~(1<<bit);

	csr32w(ctlr, Mta+x*4, ctlr->mta[x]);
}

static Block*
igberballoc(void)
{
	Block *bp;

	ilock(&igberblock);
	if((bp = igberbpool) != nil){
		igberbpool = bp->next;
		bp->next = nil;
		_xinc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&igberblock);

	return bp;
}

static void
igberbfree(Block* bp)
{
	bp->rp = bp->lim - Rbsz;
	bp->wp = bp->rp;
 	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(&igberblock);
	bp->next = igberbpool;
	igberbpool = bp;
	nrbfull--;
	iunlock(&igberblock);
}

static void
igbeim(Ctlr* ctlr, int im)
{
	ilock(&ctlr->imlock);
	ctlr->im |= im;
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);
}

static int
igbelim(void* ctlr)
{
	return ((Ctlr*)ctlr)->lim != 0;
}

static void
igbelproc(void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	MiiPhy *phy;
	int ctrl, r;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		if(ctlr->mii == nil || ctlr->mii->curphy == nil) {
			sched();
			continue;
		}

		/*
		 * To do:
		 *	logic to manage status change,
		 *	this is incomplete but should work
		 *	one time to set up the hardware.
		 *
		 *	MiiPhy.speed, etc. should be in Mii.
		 */
		if(miistatus(ctlr->mii) < 0)
			//continue;
			goto enable;

		phy = ctlr->mii->curphy;
		ctrl = csr32r(ctlr, Ctrl);

		switch(ctlr->id){
		case i82543gc:
		case i82544ei:
		case i82544eif:
		default:
			if(!(ctrl & Asde)){
				ctrl &= ~(SspeedMASK|Ilos|Fd);
				ctrl |= Frcdplx|Frcspd;
				if(phy->speed == 1000)
					ctrl |= Sspeed1000;
				else if(phy->speed == 100)
					ctrl |= Sspeed100;
				if(phy->fd)
					ctrl |= Fd;
			}
			break;

		case i82540em:
		case i82540eplp:
		case i82547gi:
		case i82541gi:
		case i82541gi2:
		case i82541pi:
			break;
		}

		/*
		 * Collision Distance.
		 */
		r = csr32r(ctlr, Tctl);
		r &= ~ColdMASK;
		if(phy->fd)
			r |= 64<<ColdSHIFT;
		else
			r |= 512<<ColdSHIFT;
		csr32w(ctlr, Tctl, r);

		/*
		 * Flow control.
		 */
		if(phy->rfc)
			ctrl |= Rfce;
		if(phy->tfc)
			ctrl |= Tfce;
		csr32w(ctlr, Ctrl, ctrl);

enable:
		ctlr->lim = 0;
		igbeim(ctlr, Lsc);

		ctlr->lsleep++;
		sleep(&ctlr->lrendez, igbelim, ctlr);
	}
}

static void
igbetxinit(Ctlr* ctlr)
{
	int i, r;
	Block *bp;

	csr32w(ctlr, Tctl, (0x0F<<CtSHIFT)|Psp|(66<<ColdSHIFT));
	switch(ctlr->id){
	default:
		r = 6;
		break;
	case i82543gc:
	case i82544ei:
	case i82544eif:
	case i82544gc:
	case i82540em:
	case i82540eplp:
	case i82541ei:
	case i82541gi:
	case i82541gi2:
	case i82541pi:
	case i82545em:
	case i82545gmc:
	case i82546gb:
	case i82546eb:
	case i82547ei:
	case i82547gi:
		r = 8;
		break;
	}
	csr32w(ctlr, Tipg, (6<<20)|(8<<10)|r);
	csr32w(ctlr, Ait, 0);
	csr32w(ctlr, Txdmac, 0);

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
	r = (4<<WthreshSHIFT)|(4<<HthreshSHIFT)|(8<<PthreshSHIFT);

	switch(ctlr->id){
	default:
		break;
	case i82540em:
	case i82540eplp:
	case i82547gi:
	case i82545em:
	case i82545gmc:
	case i82546gb:
	case i82546eb:
	case i82541gi:
	case i82541gi2:
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
}

static void
igbetransmit(Ether* edev)
{
	Td *td;
	Block *bp;
	Ctlr *ctlr;
	int tdh, tdt;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);

	/*
	 * Free any completed packets
	 */
	tdh = ctlr->tdh;
	while(NEXT(tdh, ctlr->ntd) != csr32r(ctlr, Tdh)){
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
		td->control |= Dext|Ifcs|Teop|DtypeDD;
		ctlr->tb[tdt] = bp;
		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (tdt + Ntd - tdh) % Ntd);
		tdt = NEXT(tdt, ctlr->ntd);
		if(NEXT(tdt, ctlr->ntd) == tdh){
			td->control |= Rs;
			ctlr->txdw++;
			ctlr->tdt = tdt;
			csr32w(ctlr, Tdt, tdt);
			igbeim(ctlr, Txdw);
			break;
		}
		ctlr->tdt = tdt;
		csr32w(ctlr, Tdt, tdt);
	}

	iunlock(&ctlr->tlock);
}

static void
igbereplenish(Ctlr* ctlr)
{
	Rd *rd;
	int rdt;
	Block *bp;

	rdt = ctlr->rdt;
	while(NEXT(rdt, ctlr->nrd) != ctlr->rdh){
		rd = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] == nil){
			bp = igberballoc();
			if(bp == nil){
				iprint("#l%d: igbereplenish: no available buffers\n",
					ctlr->edev->ctlrno);
				break;
			}
			ctlr->rb[rdt] = bp;
			rd->addr[0] = PCIWADDR(bp->rp);
			rd->addr[1] = 0;
		}
		coherence();
		rd->status = 0;
		rdt = NEXT(rdt, ctlr->nrd);
		ctlr->rdfree++;
	}
	ctlr->rdt = rdt;
	csr32w(ctlr, Rdt, rdt);
}

static void
igberxinit(Ctlr* ctlr)
{
	int i;
	Block *bp;

	/* temporarily keep Mpe on */
	csr32w(ctlr, Rctl, Dpf|Bsize2048|Bam|RdtmsHALF|Mpe);

	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, ctlr->nrd*sizeof(Rd));
	ctlr->rdh = 0;
	csr32w(ctlr, Rdh, 0);
	ctlr->rdt = 0;
	csr32w(ctlr, Rdt, 0);
	ctlr->rdtr = 0;
	csr32w(ctlr, Rdtr, Fpd|0);

	for(i = 0; i < ctlr->nrd; i++){
		if((bp = ctlr->rb[i]) != nil){
			ctlr->rb[i] = nil;
			freeb(bp);
		}
	}
	igbereplenish(ctlr);
	nrbfull = 0;

	switch(ctlr->id){
	case i82540em:
	case i82540eplp:
	case i82541gi:
	case i82541gi2:
	case i82541pi:
	case i82545em:
	case i82545gmc:
	case i82546gb:
	case i82546eb:
	case i82547gi:
		csr32w(ctlr, Radv, 64);
		break;
	}
	csr32w(ctlr, Rxdctl, (8<<WthreshSHIFT)|(8<<HthreshSHIFT)|4);

	/*
	 * Disable checksum offload as it has known bugs.
	 */
	csr32w(ctlr, Rxcsum, ETHERHDRSIZE<<PcssSHIFT);
}

static int
igberim(void* ctlr)
{
	return ((Ctlr*)ctlr)->rim != 0;
}

static void
igberproc(void* arg)
{
	Rd *rd;
	Block *bp;
	Ctlr *ctlr;
	int r, rdh, passed;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	igberxinit(ctlr);
	r = csr32r(ctlr, Rctl);
	r |= Ren;
	csr32w(ctlr, Rctl, r);
	for(;;){
		ctlr->rim = 0;
		igbeim(ctlr, Rxt0|Rxo|Rxdmt0|Rxseq);
		ctlr->rsleep++;
		sleep(&ctlr->rrendez, igberim, ctlr);

		rdh = ctlr->rdh;
		passed = 0;
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
			/* ignore checksum offload as it has known bugs. */
			rd->errors &= ~(Ipe | Tcpe);
			if((rd->status & Reop) && rd->errors == 0){
				bp = ctlr->rb[rdh];
				ctlr->rb[rdh] = nil;
				bp->wp += rd->length;
				bp->next = nil;
				/* ignore checksum offload as it has known bugs. */
				if(0 && !(rd->status & Ixsm)){
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
				ilock(&igberblock);
				nrbfull++;
				iunlock(&igberblock);
				notemark(&ctlr->wmrb, nrbfull);
				etheriq(edev, bp, 1);
				passed++;
			}
			else if(ctlr->rb[rdh] != nil){
				freeb(ctlr->rb[rdh]);
				ctlr->rb[rdh] = nil;
			}

			memset(rd, 0, sizeof(Rd));
			coherence();
			ctlr->rdfree--;
			rdh = NEXT(rdh, ctlr->nrd);
		}
		ctlr->rdh = rdh;

		if(ctlr->rdfree < ctlr->nrd/2 || (ctlr->rim & Rxdmt0))
			igbereplenish(ctlr);
		/* note how many rds had full buffers */
		notemark(&ctlr->wmrd, passed);
	}
}

static void
igbeattach(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	ctlr->edev = edev;			/* point back to Ether* */
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){			/* already allocated? */
		qunlock(&ctlr->alock);
		return;
	}

	ctlr->tb = nil;
	ctlr->rb = nil;
	ctlr->alloc = nil;
	ctlr->nrb = 0;
	if(waserror()){
		while(ctlr->nrb > 0){
			bp = igberballoc();
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

	ctlr->nrd = ROUND(Nrd, 8);
	ctlr->ntd = ROUND(Ntd, 8);
	ctlr->alloc = malloc(ctlr->nrd*sizeof(Rd)+ctlr->ntd*sizeof(Td) + 127);
	if(ctlr->alloc == nil) {
		print("igbe: can't allocate ctlr->alloc\n");
		error(Enomem);
	}
	ctlr->rdba = (Rd*)ROUNDUP((uintptr)ctlr->alloc, 128);
	ctlr->tdba = (Td*)(ctlr->rdba+ctlr->nrd);

	ctlr->rb = malloc(ctlr->nrd*sizeof(Block*));
	ctlr->tb = malloc(ctlr->ntd*sizeof(Block*));
	if (ctlr->rb == nil || ctlr->tb == nil) {
		print("igbe: can't allocate ctlr->rb or ctlr->tb\n");
		error(Enomem);
	}

	for(ctlr->nrb = 0; ctlr->nrb < Nrb; ctlr->nrb++){
		if((bp = allocb(Rbsz)) == nil)
			break;
		bp->free = igberbfree;
		freeb(bp);
	}
	initmark(&ctlr->wmrb, Nrb, "rcv bufs unprocessed");
	initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
	initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");

	snprint(name, KNAMELEN, "#l%dlproc", edev->ctlrno);
	kproc(name, igbelproc, edev);

	snprint(name, KNAMELEN, "#l%drproc", edev->ctlrno);
	kproc(name, igberproc, edev);

	igbetxinit(ctlr);

	qunlock(&ctlr->alock);
	poperror();
}

static void
igbeinterrupt(Ureg*, void* arg)
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

	while((icr = csr32r(ctlr, Icr) & ctlr->im) != 0){
		if(icr & Lsc){
			im &= ~Lsc;
			ctlr->lim = icr & Lsc;
			wakeup(&ctlr->lrendez);
			ctlr->lintr++;
		}
		if(icr & (Rxt0|Rxo|Rxdmt0|Rxseq)){
			im &= ~(Rxt0|Rxo|Rxdmt0|Rxseq);
			ctlr->rim = icr & (Rxt0|Rxo|Rxdmt0|Rxseq);
			wakeup(&ctlr->rrendez);
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
		igbetransmit(edev);
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
	MiiPhy *phy;
	int ctrl, p, r;

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
		if(!(r & Mdro)) {
			print("igbe: 82543gc Mdro not set\n");
			return -1;
		}
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
	case i82544eif:
	case i82544gc:
	case i82540em:
	case i82540eplp:
	case i82547ei:
	case i82547gi:
	case i82541ei:
	case i82541gi:
	case i82541gi2:
	case i82541pi:
	case i82545em:
	case i82545gmc:
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
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	USED(phy);
	// print("oui %X phyno %d\n", phy->oui, phy->phyno);

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
	case i82541gi2:
	case i82541pi:
	case i82545em:
	case i82545gmc:
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
		p = 0;
		if(ctlr->txcw & TxcwPs)
			p |= AnaP;
		if(ctlr->txcw & TxcwAs)
			p |= AnaAP;
		miiane(ctlr->mii, ~0, p, ~0);
		break;
	}
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
		microdelay(50);
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
	if(eecd & (Eeszaddr|Eesz256))
		bits = 8;
	else
		bits = 6;

	sum = 0;

	switch(ctlr->id){
	default:
		areq = 0;
		break;
	case i82540em:
	case i82540eplp:
	case i82541ei:
	case i82541gi:
	case i82541gi2:
	case i82541pi:
	case i82545em:
	case i82545gmc:
	case i82546gb:
	case i82546eb:
	case i82547ei:
	case i82547gi:
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
	snprint(rop, sizeof(rop), "S :%dDCc;", bits+3);

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

static int
igbedetach(Ctlr* ctlr)
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

	switch(ctlr->id){
	default:
		break;
	case i82540em:
	case i82540eplp:
	case i82541gi:
	case i82541gi2:
	case i82541pi:
	case i82545em:
	case i82545gmc:
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
igbeshutdown(Ether* ether)
{
	igbedetach(ether->ctlr);
}

static int
igbereset(Ctlr* ctlr)
{
	int ctrl, i, pause, r, swdpio, txcw;

	if(igbedetach(ctlr))
		return -1;

	/*
	 * Read the EEPROM, validate the checksum
	 * then get the device back to a power-on state.
	 */
	if((r = at93c46r(ctlr)) != 0xBABA){
		print("igbe: bad EEPROM checksum - 0x%4.4uX\n", r);
		return -1;
	}

	/*
	 * Snarf and set up the receive addresses.
	 * There are 16 addresses. The first should be the MAC address.
	 * The others are cleared and not marked valid (MS bit of Rah).
	 */
	if ((ctlr->id == i82546gb || ctlr->id == i82546eb) &&
	    BUSFNO(ctlr->pcidev->tbdf) == 1)
		ctlr->eeprom[Ea+2] += 0x100;		/* second interface */
	if(ctlr->id == i82541gi && ctlr->eeprom[Ea] == 0xFFFF)
		ctlr->eeprom[Ea] = 0xD000;
	for(i = Ea; i < Eaddrlen/2; i++){
		ctlr->ra[2*i] = ctlr->eeprom[i];
		ctlr->ra[2*i+1] = ctlr->eeprom[i]>>8;
	}
	/* lan id seems to vary on 82543gc; don't use it */
	if (ctlr->id != i82543gc) {
		r = (csr32r(ctlr, Status) & Lanid) >> 2;
		ctlr->ra[5] += r;		/* ea ctlr[1] = ea ctlr[0]+1 */
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
	 * (doesn't appear to fully on the 82543GC), do it manually.
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

	if(!(csr32r(ctlr, Status) & Tbimode) && igbemii(ctlr) < 0)
		return -1;

	return 0;
}

static void
igbepci(void)
{
	int cls;
	Pcidev *p;
	Ctlr *ctlr;
	void *mem;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case i82543gc:
		case i82544ei:
		case i82544eif:
		case i82544gc:
		case i82547ei:
		case i82547gi:
		case i82540em:
		case i82540eplp:
		case i82541ei:
		case i82541gi:
		case i82541gi2:
		case i82541pi:
		case i82545em:
		case i82545gmc:
		case i82546gb:
		case i82546eb:
			break;
		}

		mem = vmap(p->mem[0].bar & ~0x0F, p->mem[0].size);
		if(mem == nil){
			print("igbe: can't map %8.8luX\n", p->mem[0].bar);
			continue;
		}
		cls = pcicfgr8(p, PciCLS);
		switch(cls){
		default:
			print("igbe: p->cls %#ux, setting to 0x10\n", p->cls);
			p->cls = 0x10;
			pcicfgw8(p, PciCLS, p->cls);
			break;
		case 0x08:
		case 0x10:
			break;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil) {
			vunmap(mem, p->mem[0].size);
			error(Enomem);
		}
		ctlr->port = p->mem[0].bar & ~0x0F;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;
		ctlr->cls = cls*4;
		ctlr->nic = mem;

		if(igbereset(ctlr)){
			free(ctlr);
			vunmap(mem, p->mem[0].size);
			continue;
		}
		pcisetbme(p);

		if(igbectlrhead != nil)
			igbectlrtail->next = ctlr;
		else
			igbectlrhead = ctlr;
		igbectlrtail = ctlr;
	}
}

static int
igbepnp(Ether* edev)
{
	Ctlr *ctlr;

	if(igbectlrhead == nil)
		igbepci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = igbectlrhead; ctlr != nil; ctlr = ctlr->next){
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
	edev->mbps = 1000;
	memmove(edev->ea, ctlr->ra, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = igbeattach;
	edev->transmit = igbetransmit;
	edev->interrupt = igbeinterrupt;
	edev->ifstat = igbeifstat;
	edev->ctl = igbectl;

	edev->arg = edev;
	edev->promiscuous = igbepromiscuous;
	edev->shutdown = igbeshutdown;
	edev->multicast = igbemulticast;

	return 0;
}

void
etherigbelink(void)
{
	addethercard("i82543", igbepnp);
	addethercard("igbe", igbepnp);
}

