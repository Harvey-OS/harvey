/*
 * Intel 8256[367], 8257[1-9], 8258[03], i350
 *	Gigabit Ethernet PCI-Express Controllers
 * Coraid EtherDrive® hba
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
//#include "../port/pci.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

/*
 * note: the 82575, 82576 and 82580 are operated using registers aliased
 * to the 82563-style architecture.  many features seen in the 82598
 * are also seen in the 82575 part.
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
	Fcal		= 0x0028,	/* Flow Control Address Low */
	Fcah		= 0x002C,	/* Flow Control Address High */
	Fct		= 0x0030,	/* Flow Control Type */
	Kumctrlsta	= 0x0034,	/* Kumeran Control and Status Register */
	Connsw		= 0x0034,	/* copper / fiber switch control; 82575/82576 */
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
	Eitr		= 0x1680,	/* Extended itr; 82575/6 80 only */

	/* Receive */

	Rctl		= 0x0100,	/* Control */
	Ert		= 0x2008,	/* Early Receive Threshold (573[EVL], 82578 only) */
	Fcrtl		= 0x2160,	/* Flow Control RX Threshold Low */
	Fcrth		= 0x2168,	/* Flow Control Rx Threshold High */
	Psrctl		= 0x2170,	/* Packet Split Receive Control */
	Drxmxod		= 0x2540,	/* dma max outstanding bytes (82575) */
	Rdbal		= 0x2800,	/* Rdesc Base Address Low Queue 0 */
	Rdbah		= 0x2804,	/* Rdesc Base Address High Queue 0 */
	Rdlen		= 0x2808,	/* Descriptor Length Queue 0 */
	Srrctl		= 0x280c,	/* split and replication rx control (82575) */
	Rdh		= 0x2810,	/* Descriptor Head Queue 0 */
	Rdt		= 0x2818,	/* Descriptor Tail Queue 0 */
	Rdtr		= 0x2820,	/* Descriptor Timer Ring */
	Rxdctl		= 0x2828,	/* Descriptor Control */
	Radv		= 0x282C,	/* Interrupt Absolute Delay Timer */
	Rsrpd		= 0x2c00,	/* Small Packet Detect */
	Raid		= 0x2c08,	/* ACK interrupt delay */
	Cpuvec		= 0x2c10,	/* CPU Vector */
	Rxcsum		= 0x5000,	/* Checksum Control */
	Rmpl		= 0x5004,	/* rx maximum packet length (82575) */
	Rfctl		= 0x5008,	/* Filter Control */
	Mta		= 0x5200,	/* Multicast Table Array */
	Ral		= 0x5400,	/* Receive Address Low */
	Rah		= 0x5404,	/* Receive Address High */
	Vfta		= 0x5600,	/* VLAN Filter Table Array */
	Mrqc		= 0x5818,	/* Multiple Receive Queues Command */

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

	/* Statistics */

	Statistics	= 0x4000,	/* Start of Statistics Area */
	Gorcl		= 0x88/4,	/* Good Octets Received Count */
	Gotcl		= 0x90/4,	/* Good Octets Transmitted Count */
	Torl		= 0xC0/4,	/* Total Octets Received */
	Totl		= 0xC8/4,	/* Total Octets Transmitted */
	Nstatistics	= 0x124/4,

	/* iNVM (i211) */
	Invmdata0	= 0x12120,
};

enum {					/* Ctrl */
	Lrst		= 1<<3,		/* link reset */
	Slu		= 1<<6,		/* Set Link Up */
	Devrst		= 1<<26,	/* Device Reset */
	Rfce		= 1<<27,	/* Receive Flow Control Enable */
	Tfce		= 1<<28,	/* Transmit Flow Control Enable */
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

enum {					/* Eec */
	Nvpres		= 1<<8,
	Autord		= 1<<9,
	Flupd		= 1<<19,
	Sec1val		= 1<<22,
};

enum {					/* Eerd */
	EEstart		= 1<<0,		/* Start Read */
	EEdone		= 1<<1,		/* Read done */
};

enum {					/* Ctrlext */
	Eerst		= 1<<13,	/* EEPROM Reset */
	Linkmode	= 3<<22,	/* linkmode */
	Internalphy	= 0<<22,	/* " internal phy (copper) */
	Sgmii		= 2<<22,	/* " sgmii */
	Serdes		= 3<<22,	/* " serdes */
};

enum {
	/* Connsw */
	Enrgirq		= 1<<2,	/* interrupt on power detect (enrgsrc) */
};

enum {					/* EEPROM content offsets */
	Ea		= 0x00,		/* Ethernet Address */
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

enum {					/* phy interface */
	Phyctl		= 0,		/* phy ctl register */
	Physr		= 1,		/* phy status register */
	Phyid1		= 2,		/* phy id1 */
	Phyid2		= 3,		/* phy id2 */
	Phyisr		= 19,		/* 82563 phy interrupt status register */
	Phylhr		= 19,		/* 8257[12] link health register */
	Physsr		= 17,		/* phy secondary status register */
	Phyprst		= 193<<8 | 17,	/* 8256[34] phy port reset */
	Phyier		= 18,		/* 82573 phy interrupt enable register */
	Phypage		= 22,		/* 8256[34] page register */
	Phystat		= 26,		/* 82580 phy status */
	Phyapage	= 29,
	Rtlink		= 1<<10,	/* realtime link status */
	Phyan		= 1<<11,	/* phy has autonegotiated */

	/* Phyctl bits */
	Ran		= 1<<9,	/* restart auto negotiation */
	Ean		= 1<<12,	/* enable auto negotiation */

	/* Phyprst bits */
	Prst		= 1<<0,	/* reset the port */

	/* 82573 Phyier bits */
	Lscie		= 1<<10,	/* link status changed ie */
	Ancie		= 1<<11,	/* auto negotiation complete ie */
	Spdie		= 1<<14,	/* speed changed ie */
	Panie		= 1<<15,	/* phy auto negotiation error ie */

	/* Phylhr/Phyisr bits */
	Anf		= 1<<6,	/* lhr: auto negotiation fault */
	Ane		= 1<<15,	/* isr: auto negotiation error */

	/* 82580 Phystat bits */
	Ans	= 1<<14 | 1<<15,	/* 82580 autoneg. status */
	Link	= 1<<6,		/* 82580 Link */

	/* Rxcw builtin serdes */
	Anc		= 1<<31,
	Rxsynch		= 1<<30,
	Rxcfg		= 1<<29,
	Rxcfgch		= 1<<28,
	Rxcfgbad	= 1<<27,
	Rxnc		= 1<<26,

	/* Txcw */
	Txane		= 1<<31,
	Txcfg		= 1<<30,
};

enum {					/* fiber (pcs) interface */
	Pcsctl	= 0x4208,		/* pcs control */
	Pcsstat	= 0x420c,		/* pcs status */

	/* Pcsctl bits */
	Pan	= 1<<16,		/* autoegotiate */
	Prestart	= 1<<17,		/* restart an (self clearing) */

	/* Pcsstat bits */
	Linkok	= 1<<0,		/* link is okay */
	Andone	= 1<<16,		/* an phase is done see below for success */
	Anbad	= 1<<19 | 1<<20,	/* Anerror | Anremfault */
};

enum {					/* Icr, Ics, Ims, Imc */
	Txdw		= 0x00000001,	/* Transmit Descriptor Written Back */
	Txqe		= 0x00000002,	/* Transmit Queue Empty */
	Lsc		= 0x00000004,	/* Link Status Change */
	Rxseq		= 0x00000008,	/* Receive Sequence Error */
	Rxdmt0		= 0x00000010,	/* Rdesc Minimum Threshold Reached */
	Rxo		= 0x00000040,	/* Receiver Overrun */
	Rxt0		= 0x00000080,	/* Receiver Timer Interrupt; !82575/6/80 only */
	Rxdw		= 0x00000080,	/* Rdesc write back; 82575/6/80 only */
	Mdac		= 0x00000200,	/* MDIO Access Completed */
	Rxcfgset		= 0x00000400,	/* Receiving /C/ ordered sets */
	Ack		= 0x00020000,	/* Receive ACK frame */
	Omed		= 1<<20,	/* media change; pcs interface */
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
	BsizeFlex	= 0x08000000,	/* Flexable Bsize in 1kb increments */
	Vfe		= 0x00040000,	/* VLAN Filter Enable */
	Cfien		= 0x00080000,	/* Canonical Form Indicator Enable */
	Cfi		= 0x00100000,	/* Canonical Form Indicator value */
	Dpf		= 0x00400000,	/* Discard Pause Frames */
	Pmcf		= 0x00800000,	/* Pass MAC Control Frames */
	Bsex		= 0x02000000,	/* Buffer Size Extension */
	Secrc		= 0x04000000,	/* Strip CRC from incoming packet */
};

enum {					/* Srrctl */
	Dropen		= 1<<31,
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
	Gran		= 0x01000000,	/* Granularity; not 82575 */
	Enable		= 0x02000000,
};

enum {					/* Rxcsum */
	Ipofl		= 0x0100,	/* IP Checksum Off-load Enable */
	Tuofl		= 0x0200,	/* TCP/UDP Checksum Off-load Enable */
};

typedef struct Rd {			/* Receive Descriptor */
	u32int	addr[2];
	u16int	length;
	u16int	checksum;
	uchar	status;
	uchar	errors;
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
	uint	base;
	uint	lim;
} Flash;

enum {
	/* 16 and 32-bit flash registers for ich flash parts */
	Bfpr	= 0x00/4,		/* flash base 0:12; lim 16:28 */
	Fsts	= 0x04/2,		/* flash status; Hsfsts */
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
	Nrb		= 512+512,	/* private receive buffers per Ctlr */
	Rbalign		= BY2PG,	/* rx buffer alignment */
};

/*
 * cavet emptor: 82577/78 have been entered speculatitively.
 * awating datasheet from intel.
 */
enum {
	i82563,
	i82566,
	i82567,
	i82567m,
	i82571,
	i82572,
	i82573,
	i82574,
	i82575,
	i82576,
	i82577,
	i82577m,	
	i82578,
	i82578m,
	i82579,
	i82580,
	i82583,
	i210,
	i217,
	i218,
	i219,
	i350,
	Nctlrtype,
};

enum {
	Fload	= 1<<0,
	Fert	= 1<<1,
	F75	= 1<<2,
	Fpba	= 1<<3,
	Fflashea= 1<<4,
	F79phy	= 1<<5,
	Fnofct	= 1<<6,
	Fbadcsum= 1<<7,
	Fnofca	= 1<<8,
};

typedef struct Ctlrtype Ctlrtype;
struct Ctlrtype {
	char	*name;
	int	mtu;
	int	flag;
};

static Ctlrtype cttab[Nctlrtype] = {
[i82563]	"i82563",	9014,	Fpba,
[i82566]	"i82566",	1514,	Fload,
[i82567]	"i82567",	9234,	Fload,
[i82567m]	"i82567m",	1514,	Fload,
[i82571]	"i82571",	9234,	Fpba,
[i82572]	"i82572",	9234,	Fpba,
[i82573]	"i82573",	8192,	Fert|Fbadcsum,		/* terrible perf above 8k */
[i82574]	"i82574",	9018,	0,
[i82575]	"i82575",	9728,	F75|Fflashea,
[i82576]	"i82576",	9728,	F75,
[i82577]	"i82577",	4096,	Fload|Fert,
[i82577m]	"i82577",	1514,	Fload|Fert,
[i82578]	"i82578",	4096,	Fload|Fert,
[i82578m]	"i82578",	1514,	Fload|Fert,
[i82579]	"i82579",	9018,	Fload|Fert|F79phy|Fnofct,
[i82580]	"i82580",	9728,	F75|F79phy,
[i82583]	"i82583",	1514,	0,
[i210]		"i210",		9728,	F75|Fnofct|Fert,
[i217]		"i217",		2048,	Fload|Fert|F79phy|Fnofct|Fnofca|Fbadcsum,/* 9018, but unstable above 2k */
[i218]		"i218",		9018,	Fload|Fert|F79phy|Fnofct|Fnofca|Fbadcsum,
[i219]		"i219",		9018,	Fload|Fert|F79phy|Fnofct|Fnofca|Fbadcsum,
[i350]		"i350",		9728,	F75|F79phy|Fnofct,
};

typedef void (*Freefn)(Block*);

typedef struct Ctlr Ctlr;
struct Ctlr {
	uvlong	port;
	Pcidev	*pcidev;
	Ctlr	*next;
	int	active;
	int	type;
	u16int	eeprom[0x40];

	QLock	alock;			/* attach */
	void	*alloc;			/* receive/transmit descriptors */
	int	nrd;
	int	ntd;
	int	rbsz;

	u32int	*nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

	Rendez	lrendez;
	int	lim;

	QLock	slock;
	u32int	statistics[Nstatistics];
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
	uint	phyerrata;

	uchar	ra[Eaddrlen];		/* receive address */
	u32int	mta[128];		/* multicast table array */

	Rendez	rrendez;
	int	rim;
	int	rdfree;
	Rd	*rdba;			/* receive descriptor base address */
	Block	**rb;			/* receive buffers */
	uint	rdh;			/* receive descriptor head */
	uint	rdt;			/* receive descriptor tail */
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

	u32int	pba;			/* packet buffer allocation */
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static	Ctlr	*i82563ctlrhead;
static	Ctlr	*i82563ctlrtail;

static char *statistics[Nstatistics] = {
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

static char*
cname(Ctlr *c)
{
	return cttab[c->type].name;
}

static long
i82563ifstat(Ether *edev, void *a, long n, ulong offset)
{
	char *s, *p, *e, *stat;
	int i, r;
	uvlong tuvl, ruvl;
	Ctlr *ctlr;

	p = s = smalloc(READSTR);
	e = p + READSTR;

	ctlr = edev->ctlr;
	qlock(&ctlr->slock);

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
	p = seprint(p, e, "type: %s\n", cname(ctlr));

	p = seprint(p, e, "eeprom:");
	for(i = 0; i < 0x40; i++){
		if(i && ((i & 7) == 0))
			p = seprint(p, e, "\n       ");
		p = seprint(p, e, " %4.4ux", ctlr->eeprom[i]);
	}
	p = seprint(p, e, "\n");

	USED(p);
	n = readstr(offset, a, n, s);
	free(s);
	qunlock(&ctlr->slock);

	return n;
}

static void
i82563promiscuous(void *arg, int on)
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
i82563multicast(void *arg, uchar *addr, int on)
{
	int bit, x;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	switch(ctlr->type){
	case i82566:
	case i82567:
	case i82567m:
	case i82577:
	case i82577m:
	case i82579:
	case i217:
	case i218:
	case i219:
		bit = (addr[5]<<2)|(addr[4]>>6);
		x = (bit>>5) & 31;
		break;
	default:
		bit = (addr[5]<<4)|(addr[4]>>4);
		x = (bit>>5) & 127;
		break;
	}
	bit &= 31;

	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	if(on)
		ctlr->mta[x] |= 1<<bit;

	csr32w(ctlr, Mta+x*4, ctlr->mta[x]);
}

static void
i82563im(Ctlr *ctlr, int im)
{
	ilock(&ctlr->imlock);
	ctlr->im |= im;
	csr32w(ctlr, Ims, ctlr->im);
	iunlock(&ctlr->imlock);
}

static void
i82563txinit(Ctlr *ctlr)
{
	u32int r;
	Block *b;
	int i;

	if(cttab[ctlr->type].flag & F75)
		csr32w(ctlr, Tctl, 0x0F<<CtSHIFT | Psp);
	else
		csr32w(ctlr, Tctl, 0x0F<<CtSHIFT | Psp | 66<<ColdSHIFT | Mulr);
	csr32w(ctlr, Tipg, 6<<20 | 8<<10 | 8);		/* yb sez: 0x702008 */
	for(i = 0; i < ctlr->ntd; i++){
		if((b = ctlr->tb[i]) != nil){
			ctlr->tb[i] = nil;
			freeb(b);
		}
		memset(&ctlr->tdba[i], 0, sizeof(Td));
	}
	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));
	csr32w(ctlr, Tdbah, 0);
	csr32w(ctlr, Tdlen, ctlr->ntd * sizeof(Td));
	ctlr->tdh = PREV(0, ctlr->ntd);
	csr32w(ctlr, Tdh, 0);
	ctlr->tdt = 0;
	csr32w(ctlr, Tdt, 0);
	csr32w(ctlr, Tidv, 128);
	csr32w(ctlr, Tadv, 64);
	csr32w(ctlr, Tctl, csr32r(ctlr, Tctl) | Ten);
	r = csr32r(ctlr, Txdctl) & ~WthreshMASK;
	r |= 4<<WthreshSHIFT | 4<<PthreshSHIFT;
	if(cttab[ctlr->type].flag & F75)
		r |= Enable;
	csr32w(ctlr, Txdctl, r);
}

static uint
i82563cleanup(Ctlr *c)
{
	Block *b;
	uint tdh, n;

	tdh = c->tdh;
	while(c->tdba[n = NEXT(tdh, c->ntd)].status & Tdd){
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

static int
notrim(void *v)
{
	Ctlr *c;

	c = v;
	return (c->im & Txdw) == 0;
}

static void
i82563tproc(void *v)
{
	Td *td;
	Block *bp;
	Ether *edev;
	Ctlr *ctlr;
	uint tdt, n;

	edev = v;
	ctlr = edev->ctlr;
	i82563txinit(ctlr);

	tdt = ctlr->tdt;
	while(waserror())
		;
	for(;;){
		n = NEXT(tdt, ctlr->ntd);
		if(n == i82563cleanup(ctlr)){
			ctlr->txdw++;
			i82563im(ctlr, Txdw);
			sleep(&ctlr->trendez, notrim, ctlr);
			continue;
		}
		bp = qbread(edev->oq, 100000);
		td = &ctlr->tdba[tdt];
		td->addr[0] = PCIWADDR(bp->rp);
		td->addr[1] = 0;
		td->control = Ide|Rs|Ifcs|Teop|BLEN(bp);
		coherence();
		ctlr->tb[tdt] = bp;
		ctlr->tdt = tdt = n;
		csr32w(ctlr, Tdt, tdt);
	}
}

static void
i82563replenish(Ctlr *ctlr)
{
	uint rdt, i;
	Block *bp;
	Rd *rd;

	i = 0;
	for(rdt = ctlr->rdt; NEXT(rdt, ctlr->nrd) != ctlr->rdh; rdt = NEXT(rdt, ctlr->nrd)){
		rd = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] != nil){
			iprint("82563: tx overrun\n");
			break;
		}
		i++;
		bp = allocb(ctlr->rbsz + Rbalign);
		bp->rp = bp->wp = (uchar*)ROUND((uintptr)bp->base, Rbalign);
		ctlr->rb[rdt] = bp;
		rd->addr[0] = PCIWADDR(bp->rp);
		rd->addr[1] = 0;
		rd->status = 0;
		ctlr->rdfree++;
	}
	if(i != 0){
		coherence();
		ctlr->rdt = rdt;
		csr32w(ctlr, Rdt, rdt);
	}
}

static void
i82563rxinit(Ctlr *ctlr)
{
	int i;
	Block *bp;

	if(ctlr->rbsz <= 2048)
		csr32w(ctlr, Rctl, Dpf|Bsize2048|Bam|RdtmsHALF);
	else{
		i = ctlr->rbsz / 1024;
		if(cttab[ctlr->type].flag & F75){
			csr32w(ctlr, Rctl, Lpe|Dpf|Bsize2048|Bam|RdtmsHALF|Secrc);
			if(ctlr->type != i82575)
				i |= (ctlr->nrd/2>>4)<<20;		/* RdmsHalf */
			csr32w(ctlr, Srrctl, i | Dropen);
			csr32w(ctlr, Rmpl, ctlr->rbsz);
//			csr32w(ctlr, Drxmxod, 0x7ff);
		}else
			csr32w(ctlr, Rctl, Lpe|Dpf|BsizeFlex*i|Bam|RdtmsHALF|Secrc);
	}

	if(cttab[ctlr->type].flag & Fert)
		csr32w(ctlr, Ert, 1024/8);

	if(ctlr->type == i82566)
		csr32w(ctlr, Pbs, 16);

	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, ctlr->nrd * sizeof(Rd));
	ctlr->rdh = 0;
	csr32w(ctlr, Rdh, 0);
	ctlr->rdt = 0;
	csr32w(ctlr, Rdt, 0);
	ctlr->rdtr = 25;
	ctlr->radv = 500;
	csr32w(ctlr, Rdtr, ctlr->rdtr);
	csr32w(ctlr, Radv, ctlr->radv);

	for(i = 0; i < ctlr->nrd; i++)
		if((bp = ctlr->rb[i]) != nil){
			ctlr->rb[i] = nil;
			freeb(bp);
		}
	if(cttab[ctlr->type].flag & F75)
		csr32w(ctlr, Rxdctl, 1<<WthreshSHIFT | 8<<PthreshSHIFT | 1<<HthreshSHIFT | Enable);
	else
		csr32w(ctlr, Rxdctl, 2<<WthreshSHIFT | 2<<PthreshSHIFT);

	/*
	 * Enable checksum offload.
	 */
	csr32w(ctlr, Rxcsum, Tuofl | Ipofl | ETHERHDRSIZE);
}

static int
i82563rim(void *v)
{
	return ((Ctlr*)v)->rim != 0;
}

static void
i82563rproc(void *arg)
{
	uint rdh, rim, im;
	Block *bp;
	Ctlr *ctlr;
	Ether *edev;
	Rd *rd;

	edev = arg;
	ctlr = edev->ctlr;

	i82563rxinit(ctlr);
	csr32w(ctlr, Rctl, csr32r(ctlr, Rctl) | Ren);
	if(cttab[ctlr->type].flag & F75){
		csr32w(ctlr, Rxdctl, csr32r(ctlr, Rxdctl) | Enable);
		im = Rxt0|Rxo|Rxdmt0|Rxseq|Ack;
	}else
		im = Rxt0|Rxo|Rxdmt0|Rxseq|Ack;

	while(waserror())
		;
	for(;;){
		i82563im(ctlr, im);
		ctlr->rsleep++;
		i82563replenish(ctlr);
		sleep(&ctlr->rrendez, i82563rim, ctlr);

		rdh = ctlr->rdh;
		for(;;){
			rim = ctlr->rim;
			ctlr->rim = 0;
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
			bp = ctlr->rb[rdh];
			if((rd->status & Reop) && rd->errors == 0){
				bp->wp += rd->length;
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
			ctlr->rdfree--;
			ctlr->rdh = rdh = NEXT(rdh, ctlr->nrd);
			if(ctlr->nrd-ctlr->rdfree >= 32 || (rim & Rxdmt0))
				i82563replenish(ctlr);
		}
	}
}

static int
i82563lim(void *v)
{
	return ((Ctlr*)v)->lim != 0;
}

static int speedtab[] = {
	10, 100, 1000, 0
};

static uint
phyread(Ctlr *c, int phyno, int reg)
{
	uint phy, i;

	csr32w(c, Mdic, MDIrop | phyno<<MDIpSHIFT | reg<<MDIrSHIFT);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = csr32r(c, Mdic);
		if(phy & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((phy & (MDIe|MDIready)) != MDIready){
		print("%s: phy %d wedged %.8ux\n", cname(c), phyno, phy);
		return ~0;
	}
	return phy & 0xffff;
}

static uint
phywrite0(Ctlr *c, int phyno, int reg, ushort val)
{
	uint phy, i;

	csr32w(c, Mdic, MDIwop | phyno<<MDIpSHIFT | reg<<MDIrSHIFT | val);
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

static uint
setpage(Ctlr *c, uint phyno, uint p, uint r)
{
	uint pr;

	if(c->type == i82563){
		if(r >= 16 && r <= 28 && r != 22)
			pr = Phypage;
		else if(r == 30 || r == 31)
			pr = Phyapage;
		else
			return 0;
		return phywrite0(c, phyno, pr, p);
	}else if(p == 0)
		return 0;
	return ~0;
}

static uint
phywrite(Ctlr *c, uint phyno, uint reg, ushort v)
{
	if(setpage(c, phyno, reg>>8, reg & 0xff) == ~0)
		panic("%s: bad phy reg %.4ux", cname(c), reg);
	return phywrite0(c, phyno, reg & 0xff, v);
}

static void
phyerrata(Ether *e, Ctlr *c, uint phyno)
{
	if(e->mbps == 0){
		if(c->phyerrata == 0){
			c->phyerrata++;
			phywrite(c, phyno, Phyprst, Prst);	/* try a port reset */
			print("%s: phy port reset\n", cname(c));
		}
	}else
		c->phyerrata = 0;
}

static uint
phyprobe(Ctlr *c, uint mask)
{
	uint phy, phyno;

	for(phyno=0; mask != 0; phyno++, mask>>=1){
		if((mask & 1) == 0)
			continue;
		if(phyread(c, phyno, Physr) == ~0)
			continue;
		phy = (phyread(c, phyno, Phyid1) & 0x3FFF)<<6;
		phy |= phyread(c, phyno, Phyid2) >> 10;
		if(phy == 0xFFFFF || phy == 0)
			continue;
		print("%s: phy%d oui %#ux\n", cname(c), phyno, phy);
		return phyno;
	}
	print("%s: no phy\n", cname(c));
	return ~0;
}

static void
lsleep(Ctlr *c, uint m)
{
	c->lim = 0;
	i82563im(c, m);
	c->lsleep++;
	sleep(&c->lrendez, i82563lim, c);
}

static void
phyl79proc(void *v)
{
	uint i, r, phy, phyno;
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	while(waserror())
		;

	while((phyno = phyprobe(c, 3<<1)) == ~0)
		lsleep(c, Lsc);

	for(;;){
		phy = 0;
		for(i=0; i<4; i++){
			tsleep(&up->sleep, return0, 0, 150);
			phy = phyread(c, phyno, Phystat);
			if(phy == ~0)
				continue;
			if(phy & Ans){
				r = phyread(c, phyno, Phyctl);
				if(r == ~0)
					continue;
				phywrite(c, phyno, Phyctl, r | Ran | Ean);
			}
			break;
		}
		i = (phy>>8) & 3;
		e->link = i != 3 && (phy & Link) != 0;
		if(e->link == 0)
			i = 3;
		c->speeds[i]++;
		e->mbps = speedtab[i];
		lsleep(c, Lsc);
	}
}

static void
phylproc(void *v)
{
	uint a, i, phy, phyno;
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	while(waserror())
		;

	while((phyno = phyprobe(c, 3<<1)) == ~0)
		lsleep(c, Lsc);

	if(c->type == i82573 && (phy = phyread(c, phyno, Phyier)) != ~0)
		phywrite(c, phyno, Phyier, phy | Lscie | Ancie | Spdie | Panie);

	for(;;){
		phy = phyread(c, phyno, Physsr);
		if(phy == ~0){
			phy = 0;
			i = 3;
			goto next;
		}
		i = (phy>>14) & 3;
		switch(c->type){
		default:
			a = 0;
			break;
		case i82563:
		case i82578:
		case i82578m:
		case i82583:
			a = phyread(c, phyno, Phyisr) & Ane;
			break;
		case i82571:
		case i82572:
		case i82575:
		case i82576:
			a = phyread(c, phyno, Phylhr) & Anf;
			i = (i-1) & 3;
			break;
		}
		if(a)
			phywrite(c, phyno, Phyctl, phyread(c, phyno, Phyctl) | Ran | Ean);
next:
		e->link = (phy & Rtlink) != 0;
		if(e->link == 0)
			i = 3;
		c->speeds[i]++;
		e->mbps = speedtab[i];
		if(c->type == i82563)
			phyerrata(e, c, phyno);
		lsleep(c, Lsc);
	}
}

static void
pcslproc(void *v)
{
	uint i, phy;
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	while(waserror())
		;

	if(c->type == i82575 || c->type == i82576)
		csr32w(c, Connsw, Enrgirq);
	for(;;){
		phy = csr32r(c, Pcsstat);
		e->link = phy & Linkok;
		i = 3;
		if(e->link)
			i = (phy & 6) >> 1;
		else if(phy & Anbad)
			csr32w(c, Pcsctl, csr32r(c, Pcsctl) | Pan | Prestart);
		c->speeds[i]++;
		e->mbps = speedtab[i];
		lsleep(c, Lsc | Omed);
	}
}

static void
serdeslproc(void *v)
{
	uint i, tx, rx;
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	while(waserror())
		;
	for(;;){
		rx = csr32r(c, Rxcw);
		tx = csr32r(c, Txcw);
		USED(tx);
		e->link = (rx & 1<<31) != 0;
//		e->link = (csr32r(c, Status) & Lu) != 0;
		i = 3;
		if(e->link)
			i = 2;
		c->speeds[i]++;
		e->mbps = speedtab[i];
		lsleep(c, Lsc);
	}
}

static void
i82563attach(Ether *edev)
{
	char name[KNAMELEN];
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){
		qunlock(&ctlr->alock);
		return;
	}

	ctlr->nrd = Nrd;
	ctlr->ntd = Ntd;
	ctlr->alloc = malloc(ctlr->nrd*sizeof(Rd)+ctlr->ntd*sizeof(Td) + 255);
	ctlr->rb = malloc(ctlr->nrd * sizeof(Block*));
	ctlr->tb = malloc(ctlr->ntd * sizeof(Block*));
	if(ctlr->alloc == nil || ctlr->rb == nil || ctlr->tb == nil){
		free(ctlr->rb);
		ctlr->rb = nil;
		free(ctlr->tb);
		ctlr->tb = nil;
		free(ctlr->alloc);
		ctlr->alloc = nil;
		qunlock(&ctlr->alock);
		error(Enomem);
	}
	ctlr->rdba = (Rd*)ROUNDUP((uintptr)ctlr->alloc, 256);
	ctlr->tdba = (Td*)(ctlr->rdba + ctlr->nrd);

	if(waserror()){
		free(ctlr->tb);
		ctlr->tb = nil;
		free(ctlr->rb);
		ctlr->rb = nil;
		free(ctlr->alloc);
		ctlr->alloc = nil;
		qunlock(&ctlr->alock);
		nexterror();
	}

	snprint(name, sizeof name, "#l%dl", edev->ctlrno);
	if(csr32r(ctlr, Status) & Tbimode)
		kproc(name, serdeslproc, edev);		/* mac based serdes */
	else if((csr32r(ctlr, Ctrlext) & Linkmode) == Serdes)
		kproc(name, pcslproc, edev);		/* phy based serdes */
	else if(cttab[ctlr->type].flag & F79phy)
		kproc(name, phyl79proc, edev);
	else
		kproc(name, phylproc, edev);

	snprint(name, sizeof name, "#l%dr", edev->ctlrno);
	kproc(name, i82563rproc, edev);

	snprint(name, sizeof name, "#l%dt", edev->ctlrno);
	kproc(name, i82563tproc, edev);

	qunlock(&ctlr->alock);
	poperror();
}

static void
i82563interrupt(Ureg*, void *arg)
{
	Ctlr *ctlr;
	Ether *edev;
	int icr, im;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->imlock);
	csr32w(ctlr, Imc, ~0);
	im = ctlr->im;

	while(icr = csr32r(ctlr, Icr) & ctlr->im){
		if(icr & (Lsc | Omed)){
			im &= ~(Lsc | Omed);
			ctlr->lim = icr & (Lsc | Omed);
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

static int
i82563detach(Ctlr *ctlr)
{
	int r, timeo;

	/* balance rx/tx packet buffer; survives reset */
	if(ctlr->rbsz > 8192 && cttab[ctlr->type].flag & Fpba){
		ctlr->pba = csr32r(ctlr, Pba);
		r = ctlr->pba >> 16;
		r += ctlr->pba & 0xffff;
		r >>= 1;
		csr32w(ctlr, Pba, r);
	}else if(ctlr->type == i82573 && ctlr->rbsz > 1514)
		csr32w(ctlr, Pba, 14);
	ctlr->pba = csr32r(ctlr, Pba);

	/*
	 * Perform a device reset to get the chip back to the
	 * power-on state, followed by an EEPROM reset to read
	 * the defaults for some internal registers.
	 */
	csr32w(ctlr, Imc, ~0);
	csr32w(ctlr, Rctl, 0);
	csr32w(ctlr, Tctl, csr32r(ctlr, Tctl) & ~Ten);

	delay(10);

	r = csr32r(ctlr, Ctrl);
	if(ctlr->type == i82566 || ctlr->type == i82579)
		r |= Phyrst;
	/*
	 * hack: 82579LM on lenovo X230 is stuck at 10mbps after
	 * reseting the phy, but works fine if we dont reset.
	 */
	if(ctlr->pcidev->did == 0x1502)
		r &= ~Phyrst;
	csr32w(ctlr, Ctrl, Devrst | r);
	delay(1);
	for(timeo = 0;; timeo++){
		if((csr32r(ctlr, Ctrl) & (Devrst|Phyrst)) == 0)
			break;
		if(timeo >= 1000)
			return -1;
		delay(1);
	}

	r = csr32r(ctlr, Ctrl);
	csr32w(ctlr, Ctrl, Slu|r);

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
		if((csr32r(ctlr, Icr) & ~Rxcfg) == 0)
			break;
		delay(1);
	}
	if(csr32r(ctlr, Icr) & ~Rxcfg)
		return -1;

	return 0;
}

static void
i82563shutdown(Ether *edev)
{
	i82563detach(edev->ctlr);
}

static int
eeread(Ctlr *ctlr, int adr)
{
	int timeout;

	csr32w(ctlr, Eerd, EEstart | adr << 2);
	timeout = 1000;
	while ((csr32r(ctlr, Eerd) & EEdone) == 0 && timeout--)
		microdelay(5);
	if (timeout < 0) {
		print("%s: eeread timeout\n", cname(ctlr));
		return -1;
	}
	return (csr32r(ctlr, Eerd) >> 16) & 0xffff;
}

static int
eeload(Ctlr *ctlr)
{
	u16int sum;
	int data, adr;

	sum = 0;
	for (adr = 0; adr < 0x40; adr++) {
		data = eeread(ctlr, adr);
		if(data == -1) return -1;
		ctlr->eeprom[adr] = data;
		sum += data;
	}
	return sum;
}

static int
fread16(Ctlr *c, Flash *f, int ladr)
{
	u16int s;
	int timeout;

	delay(1);
	s = f->reg[Fsts];
	if((s&Fvalid) == 0)
		return -1;
	f->reg[Fsts] |= Fcerr | Ael;
	for(timeout = 0; timeout < 10; timeout++){
		if((s&Scip) == 0)
			goto done;
		delay(1);
		s = f->reg[Fsts];
	}
	return -1;
done:
	f->reg[Fsts] |= Fdone;
	f->reg32[Faddr] = ladr;

	/* setup flash control register */
	s = f->reg[Fctl] & ~0x3ff;
	f->reg[Fctl] = s | 1<<8 | Fgo;	/* 2 byte read */
	timeout = 1000;
	while((f->reg[Fsts] & Fdone) == 0 && timeout--)
		microdelay(5);
	if(timeout < 0){
		print("%s: fread timeout\n", cname(c));
		return -1;
	}
	if(f->reg[Fsts] & (Fcerr|Ael))
		return -1;
	return f->reg32[Fdata] & 0xffff;
}

static int
fread32(Ctlr *c, Flash *f, int ladr, u32int *data)
{
	u32int s;
	int timeout;

	delay(1);
	s = f->reg32[Fsts/2];
	if((s&Fvalid) == 0)
		return -1;
	f->reg32[Fsts/2] |= Fcerr | Ael;
	for(timeout = 0; timeout < 10; timeout++){
		if((s&Scip) == 0)
			goto done;
		delay(1);
		s = f->reg32[Fsts/2];
	}
	return -1;
done:
	f->reg32[Fsts/2] |= Fdone;
	f->reg32[Faddr] = ladr;

	/* setup flash control register */
	s = (f->reg32[Fctl/2] >> 16) & ~0x3ff;
	f->reg32[Fctl/2] = (s | 3<<8 | Fgo) << 16;	/* 4 byte read */
	timeout = 1000;
	while((f->reg32[Fsts/2] & Fdone) == 0 && timeout--)
		microdelay(5);
	if(timeout < 0){
		print("%s: fread timeout\n", cname(c));
		return -1;
	}
	if(f->reg32[Fsts/2] & (Fcerr|Ael))
		return -1;
	*data = f->reg32[Fdata];
	return 0;
}

static int
fload32(Ctlr *c)
{
	int r, adr;
	u16int sum;
	u32int w;
	Flash f;

	f.reg32 = &c->nic[0xe000/4];
	f.reg = nil;
	f.base = 0;
	f.lim = (((csr32r(c, 0xC) >> 1) & 0x1F) + 1) << 12;
	r = f.lim >> 1;
	if(fread32(c, &f, r + 0x24, &w) == -1  || (w & 0xC000) != 0x8000)
		r = 0;
	sum = 0;
	for(adr = 0; adr < 0x20; adr++) {
		if(fread32(c, &f, r + adr*4, &w) == -1)
			return -1;
		c->eeprom[adr*2+0] = w;
		c->eeprom[adr*2+1] = w>>16;
		sum += w & 0xFFFF;
		sum += w >> 16;
	}
	return sum;
}

static int
fload(Ctlr *c)
{
	int data, r, adr;
	u16int sum;
	void *va;
	Flash f;

	memset(c->eeprom, 0xFF, sizeof(c->eeprom));
	if(c->pcidev->mem[1].bar == 0)
		return fload32(c);	/* i219 */

	if(c->pcidev->mem[1].bar & 1)
		return -1;

	va = vmap(c->pcidev->mem[1].bar & ~0xF, c->pcidev->mem[1].size);
	if(va == nil)
		return -1;
	f.reg = va;
	f.reg32 = va;
	f.base = f.reg32[Bfpr] & 0x1fff;
	f.lim = f.reg32[Bfpr]>>16 & 0x1fff;
	if(csr32r(c, Eec) & Sec1val)
		f.base += f.lim+1 - f.base >> 1;
	r = f.base << 12;
	sum = 0;
	for(adr = 0; adr < 0x40; adr++) {
		data = fread16(c, &f, r + adr*2);
		if(data == -1)
			goto out;
		c->eeprom[adr] = data;
		sum += data;
	}
out:
	vunmap(va, c->pcidev->mem[1].size);
	return sum;
}

static int
invmload(Ctlr *c)
{
	int i, a;
	u32int w;

	memset(c->eeprom, 0xFF, sizeof(c->eeprom));
	for(i=0; i<64; i++){
		w = csr32r(c, Invmdata0 + i*4);
		switch(w & 7){
		case 0:	// uninitialized structure
			break;
		case 1:	// word auto load
			a = (w & 0xFE00) >> 9;
			if(a < nelem(c->eeprom))
				c->eeprom[a] = w >> 16;
			continue;
		case 2:	// csr auto load
			i++;
		case 3:	// phy auto load
			continue;
		case 4:	// rsa key sha256
			i += 256/32;
		case 5:	// invalidated structure
			continue;
		default:
			print("invm: %.2x %.8ux\n", i, w);
			continue;
		}
		break;
	}
	return 0;
}

static void
defaultea(Ctlr *ctlr, uchar *ra)
{
	uint i, r;
	uvlong u;
	static uchar nilea[Eaddrlen];

	if(memcmp(ra, nilea, Eaddrlen) != 0)
		return;
	if(cttab[ctlr->type].flag & Fflashea){
		/* intel mb bug */
		u = (uvlong)csr32r(ctlr, Rah)<<32u | (ulong)csr32r(ctlr, Ral);
		for(i = 0; i < Eaddrlen; i++)
			ra[i] = u >> 8*i;
	}
	if(memcmp(ra, nilea, Eaddrlen) != 0)
		return;
	for(i = 0; i < Eaddrlen/2; i++){
		ra[2*i] = ctlr->eeprom[Ea+i];
		ra[2*i+1] = ctlr->eeprom[Ea+i] >> 8;
	}
	r = (csr32r(ctlr, Status) & Lanid) >> 2;
	ra[5] += r;				/* ea ctlr[n] = ea ctlr[0]+n */
}

static int
i82563reset(Ctlr *ctlr)
{
	uchar *ra;
	int i, r, flag;

	if(i82563detach(ctlr))
		return -1;
	flag = cttab[ctlr->type].flag;

	if(ctlr->type == i210 && (csr32r(ctlr, Eec) & Flupd) == 0)
		r = invmload(ctlr);
	else if(flag & Fload)
		r = fload(ctlr);
	else
		r = eeload(ctlr);

	if(r != 0 && r != 0xbaba){
		print("%s: bad eeprom checksum - %#.4ux", cname(ctlr), r);
		if(flag & Fbadcsum)
			print("; ignored\n");
		else {
			print("\n");
			return -1;
		}
	}

	ra = ctlr->ra;
	defaultea(ctlr, ra);
	csr32w(ctlr, Ral, ra[3]<<24 | ra[2]<<16 | ra[1]<<8 | ra[0]);
	csr32w(ctlr, Rah, 1<<31 | ra[5]<<8 | ra[4]);
	for(i = 1; i < 16; i++){
		csr32w(ctlr, Ral+i*8, 0);
		csr32w(ctlr, Rah+i*8, 0);
	}
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta + i*4, 0);
	if((flag & Fnofca) == 0){
		csr32w(ctlr, Fcal, 0x00C28001);
		csr32w(ctlr, Fcah, 0x0100);
	}
	if((flag & Fnofct) == 0)
		csr32w(ctlr, Fct, 0x8808);
	csr32w(ctlr, Fcttv, 0x0100);
	csr32w(ctlr, Fcrtl, ctlr->fcrtl);
	csr32w(ctlr, Fcrth, ctlr->fcrth);
	if(flag & F75)
		csr32w(ctlr, Eitr, 128<<2);		/* 128 ¼ microsecond intervals */
	return 0;
}

enum {
	CMrdtr,
	CMradv,
	CMpause,
	CMan,
};

static Cmdtab i82563ctlmsg[] = {
	CMrdtr,	"rdtr",	2,
	CMradv,	"radv",	2,
	CMpause, "pause", 1,
	CMan,	"an",	1,
};

static long
i82563ctl(Ether *edev, void *buf, long n)
{
	char *p;
	ulong v;
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
		if(*p || v > 0xffff)
			error(Ebadarg);
		ctlr->rdtr = v;
		csr32w(ctlr, Rdtr, v);
		break;
	case CMradv:
		v = strtoul(cb->f[1], &p, 0);
		if(*p || v > 0xffff)
			error(Ebadarg);
		ctlr->radv = v;
		csr32w(ctlr, Radv, v);
		break;
	case CMpause:
		csr32w(ctlr, Ctrl, csr32r(ctlr, Ctrl) ^ (Rfce | Tfce));
		break;
	case CMan:
		csr32w(ctlr, Ctrl, csr32r(ctlr, Ctrl) | Lrst | Phyrst);
		break;
	}
	free(cb);
	poperror();

	return n;
}

static int
didtype(int d)
{
	/*
	 * Some names and did values are from
	 * OpenBSD's em(4) Intel driver.
	 */
	switch(d){
	case 0x1096:		/* copper */
	case 0x10ba:		/* copper “gilgal” */
	case 0x1098:		/* serdes; not seen */
	case 0x10bb:		/* serdes */
		return i82563;
	case 0x1049:		/* ich8; mm */
	case 0x104a:		/* ich8; dm */
	case 0x104b:		/* ich8; dc */
	case 0x104d:		/* ich8; v “ninevah” */
	case 0x10bd:		/* ich9; dm-2 */
	case 0x294c:		/* ich9 */
	case 0x104c:		/* ich8; untested */
	case 0x10c4:		/* ich8; untested */
	case 0x10c5:		/* ich8; untested */
		return i82566;
	case 0x10de:		/* lm ich10d */
	case 0x10df:		/* lf ich10 */
	case 0x10e5:		/* lm ich9 */
	case 0x10f5:		/* lm ich9m; “boazman” */
	case 0x10ce:		/* v ich10 */
	case 0x10c0:		/* ich9 */
	case 0x10c2:		/* ich9; untested */
	case 0x10c3:		/* ich9; untested */
	case 0x1501:		/* ich8; untested */
		return i82567;
	case 0x10bf:		/* lf ich9m */
	case 0x10cb:		/* v ich9m */
	case 0x10cd:		/* lf ich10 */
	case 0x10cc:		/* lm ich10 */
		return i82567m;
	case 0x105e:		/* eb copper */
	case 0x105f:		/* eb fiber */
	case 0x1060:		/* eb serdes */
	case 0x10a4:		/* eb copper */
	case 0x10a5:		/* eb fiber */
	case 0x10bc:		/* eb copper */
	case 0x10d9:		/* eb serdes */
	case 0x10da:		/* eb serdes “ophir” */
	case 0x10a0:		/* eb; untested */
	case 0x10a1:		/* eb; untested */
	case 0x10d5:		/* copper; untested */
		return i82571;
	case 0x107d:		/* ei copper */
	case 0x107e:		/* ei fiber */
	case 0x107f:		/* ei serdes */
	case 0x10b9:		/* ei “rimon” */
		return i82572;
	case 0x108b:		/* e “vidalia” */
	case 0x108c:		/* e (iamt) */
	case 0x109a:		/* l “tekoa” */
	case 0x10b0:		/* l; untested */
	case 0x10b2:		/* v; untested */
	case 0x10b3:		/* e; untested */
	case 0x10b4:		/* l; untested */
		return i82573;
	case 0x10d3:		/* l or it; “hartwell” */
	case 0x10f6:		/* la; untested */
		return i82574;
	case 0x10a7:		/* eb */
	case 0x10a9:		/* eb fiber/serdes */
	case 0x10d6:		/* untested */
	case 0x10e2:		/* untested */
		return i82575;
	case 0x10c9:		/* copper */
	case 0x10e6:		/* fiber */
	case 0x10e7:		/* serdes; “kawela” */
	case 0x10e8:		/* copper; untested */
	case 0x150a:		/* untested */
	case 0x150d:		/* serdes backplane */
	case 0x1518:		/* serdes; untested */
	case 0x1526:		/* untested */
		return i82576;
	case 0x10ea:		/* lc “calpella”; aka pch lan */
		return i82577;
	case 0x10eb:		/* lm “calpella” */
		return i82577m;
	case 0x10ef:		/* dc “piketon” */
		return i82578;
	case 0x1502:		/* lm */
	case 0x1503:		/* v “lewisville” */
		return i82579;
	case 0x10f0:		/* dm “king's creek” */
		return i82578m;
	case 0x150e:		/* copper “barton hills” */
	case 0x150f:		/* fiber */
	case 0x1510:		/* serdes backplane */
	case 0x1511:		/* sgmii sfp */
	case 0x1516:		/* copper */
		return i82580;
	case 0x1506:		/* v */
	case 0x150c:		/* untested */
		return i82583;
	case 0x1533:		/* i210-t1 */
	case 0x1534:		/* i210 */
	case 0x1536:		/* i210-fiber */
	case 0x1537:		/* i210-backplane */
	case 0x1538:		/* i210 sgmii */
	case 0x1539:		/* i211 copper */
	case 0x157b:		/* i210 copper flashless */
	case 0x157c:		/* i210 serdes flashless */
		return i210;
	case 0x153a:		/* i217-lm */
	case 0x153b:		/* i217-v */
		return i217;
	case 0x1559:		/* i218-v */
	case 0x155a:		/* i218-lm */
	case 0x15a0:		/* i218-lm */
	case 0x15a1:		/* i218-v */
	case 0x15a2:		/* i218-lm */
	case 0x15a3:		/* i218-v */
		return i218;
	case 0x156f:		/* i219-lm */
	case 0x15b7:		/* i219-lm */
	case 0x1570:		/* i219-v */
	case 0x15b8:		/* i219-v */
	case 0x15b9:		/* i219-lm */
	case 0x15bb:		/* i219-lm */
	case 0x15bd:		/* i219-lm */
	case 0x15be:		/* i219-v */
	case 0x15d6:		/* i219-v */
	case 0x15d7:		/* i219-lm */
	case 0x15d8:		/* i219-v */
	case 0x15e3:		/* i219-lm */
	case 0x0d4c:		/* i219-lm */
		return i219;
	case 0x151f:		/* i350 “powerville” eeprom-less */
	case 0x1521:		/* i350 copper */
	case 0x1522:		/* i350 fiber */
	case 0x1523:		/* i350 serdes */
	case 0x1524:		/* i350 sgmii */
	case 0x1546:		/* i350 DA4 (untested) */
	case 0x1f40:		/* i354 backplane */
	case 0x1f41:		/* i354 sgmii */
	case 0x1f42:		/* i354 sgmii (c2000) */
	case 0x1f45:		/* i354 backplane 2.5 */
		return i350;
	}
	return -1;
}

static void
hbafixup(Pcidev *p)
{
	uint i;

	i = pcicfgr32(p, PciSVID);
	if((i & 0xffff) == 0x1b52 && p->did == 1)
		p->did = i>>16;
}

static void
i82563pci(void)
{
	int type;
	Ctlr *ctlr;
	Pcidev *p;

	for(p = nil; p = pcimatch(p, 0x8086, 0);){
		hbafixup(p);
		if(p->mem[0].bar & 1)
			continue;
		if((type = didtype(p->did)) == -1)
			continue;
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil){
			print("%s: can't allocate memory\n", cttab[type].name);
			continue;
		}
		ctlr->type = type;
		ctlr->pcidev = p;
		ctlr->rbsz = ROUND(cttab[type].mtu, 1024);
		ctlr->port = p->mem[0].bar & ~0xF;
		if(i82563ctlrhead != nil)
			i82563ctlrtail->next = ctlr;
		else
			i82563ctlrhead = ctlr;
		i82563ctlrtail = ctlr;
	}
}

static int
setup(Ctlr *ctlr)
{
	Pcidev *p;

	p = ctlr->pcidev;
	ctlr->nic = vmap(ctlr->port, p->mem[0].size);
	if(ctlr->nic == nil){
		print("%s: can't map %llux\n", cname(ctlr), ctlr->port);
		return -1;
	}
	//pcienable(p);
	if(i82563reset(ctlr)){
		//pcidisable(p);
		vunmap(ctlr->nic, p->mem[0].size);
		return -1;
	}
	pcisetbme(p);
	return 0;
}

static int
pnp(Ether *edev, int type)
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
	for(ctlr = i82563ctlrhead; ; ctlr = ctlr->next){
		if(ctlr == nil)
			return -1;
		if(ctlr->active)
			continue;
		if(type != -1 && ctlr->type != type)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			memmove(ctlr->ra, edev->ea, Eaddrlen);
			if(setup(ctlr) == 0)
				break;
		}
	}

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->maxmtu = cttab[ctlr->type].mtu;
	memmove(edev->ea, ctlr->ra, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = i82563attach;
//	edev->transmit = i82563transmit;
	edev->ifstat = i82563ifstat;
	edev->ctl = i82563ctl;

	edev->arg = edev;
	edev->promiscuous = i82563promiscuous;
	edev->shutdown = i82563shutdown;
	edev->multicast = i82563multicast;

	intrenable(edev->irq, i82563interrupt, edev, edev->tbdf, edev->name);

	return 0;
}

static int
anypnp(Ether *e)
{
	return pnp(e, -1);
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
i82567pnp(Ether *e)
{
	return pnp(e, i82567m) & pnp(e, i82567);
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
i82574pnp(Ether *e)
{
	return pnp(e, i82574);
}

static int
i82575pnp(Ether *e)
{
	return pnp(e, i82575);
}

static int
i82576pnp(Ether *e)
{
	return pnp(e, i82576);
}

static int
i82577pnp(Ether *e)
{
	return pnp(e, i82577m) & pnp(e, i82577);
}

static int
i82578pnp(Ether *e)
{
	return pnp(e, i82578m) & pnp(e, i82578);
}

static int
i82579pnp(Ether *e)
{
	return pnp(e, i82579);
}

static int
i82580pnp(Ether *e)
{
	return pnp(e, i82580);
}

static int
i82583pnp(Ether *e)
{
	return pnp(e, i82583);
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

static int
i219pnp(Ether *e)
{
	return pnp(e, i219);
}

static int
i350pnp(Ether *e)
{
	return pnp(e, i350);
}

void
ether82563link(void)
{
	/*
	 * recognise lots of model numbers for debugging
	 * also good for forcing onboard nic(s) as ether0
	 * try to make that unnecessary by listing lom first.
	 */
	addethercard("i82563", i82563pnp);
	addethercard("i82566", i82566pnp);
	addethercard("i82574", i82574pnp);
	addethercard("i82576", i82576pnp);
	addethercard("i82567", i82567pnp);
	addethercard("i82573", i82573pnp);

	addethercard("i82571", i82571pnp);
	addethercard("i82572", i82572pnp);
	addethercard("i82575", i82575pnp);
	addethercard("i82577", i82577pnp);
	addethercard("i82578", i82578pnp);
	addethercard("i82579", i82579pnp);
	addethercard("i82580", i82580pnp);
	addethercard("i82583", i82583pnp);
	addethercard("i210", i210pnp);
	addethercard("i217", i217pnp);
	addethercard("i218", i218pnp);
	addethercard("i219", i219pnp);
	addethercard("i350", i350pnp);
	addethercard("igbepcie", anypnp);
}
