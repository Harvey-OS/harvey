/*
 * Intel RS-82543GC Gigabit Ethernet Controller
 * as found on the Intel PRO/1000[FT] Server Adapter.
 * The older non-[FT] cards use the 82542 (LSI L2A1157) chip; no attempt
 * is made to handle the older chip although it should be possible.
 * The datasheet is not very clear about running on a big-endian system
 * and this driver assumes little-endian throughout.
 * To do:
 *	GMII/MII
 *	receive tuning
 *	transmit tuning
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
	Txcw		= 0x00000178,	/* Transmit configuration word reg. */
	Rxcw		= 0x00000180,	/* Receive configuration word reg. */
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
	Rdbal		= 0x00002800,	/* Rdesc Base Address Low */
	Rdbah		= 0x00002804,	/* Rdesc Base Address High */
	Rdlen		= 0x00002808,	/* Receive Descriptor Length */
	Rdh		= 0x00002810,	/* Receive Descriptor Head */
	Rdt		= 0x00002818,	/* Receive Descriptor Tail */
	Rdtr		= 0x00002820,	/* Receive Descriptor Timer Ring */
	Rxdctl		= 0x00002828,	/* Receive Descriptor Control */
	Txdmac		= 0x00003000,	/* Transfer DMA Control */
	Ett		= 0x00003008,	/* Early Transmit Control */
	Tdfh		= 0x00003410,	/* Transmit data fifo head */
	Tdft		= 0x00003418,	/* Transmit data fifo tail */
	Tdfhs		= 0x00003420,	/* Transmit data Fifo Head saved */
	Tdfts		= 0x00003428,	/* Transmit data fifo tail saved */
	Tdfpc		= 0x00003430,	/* Trasnmit data Fifo packet count */
	Tdbal		= 0x00003800,	/* Tdesc Base Address Low */
	Tdbah		= 0x00003804,	/* Tdesc Base Address High */
	Tdlen		= 0x00003808,	/* Transmit Descriptor Length */
	Tdh		= 0x00003810,	/* Transmit Descriptor Head */
	Tdt		= 0x00003818,	/* Transmit Descriptor Tail */
	Tidv		= 0x00003820,	/* Transmit Interrupt Delay Value */
	Txdctl		= 0x00003828,	/* Transmit Descriptor Control */

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
};

enum {					/* Ctrl */
	Bem		= 0x00000002,	/* Big Endian Mode */
	Prior		= 0x00000004,	/* Priority on the PCI bus */
	Lrst		= 0x00000008,	/* Link Reset */
	Asde		= 0x00000020,	/* Auto-Speed Detection Enable */
	Slu		= 0x00000040,	/* Set Link Up */
	Ilos		= 0x00000080,	/* Invert Loss of Signal (LOS) */
	Frcspd		= 0x00000800,	/* Force Speed */
	Frcdplx		= 0x00001000,	/* Force Duplex */
	Swdpinslo	= 0x003C0000,	/* Software Defined Pins - lo nibble */
	Swdpin0		= 0x00040000,
	Swdpin1		= 0x00080000,
	Swdpin2		= 0x00100000,
	Swdpin3		= 0x00200000,
	Swdpiolo	= 0x03C00000,	/* Software Defined I/O Pins */
	Swdpio0		= 0x00400000,
	Swdpio1		= 0x00800000,
	Swdpio2		= 0x01000000,
	Swdpio3		= 0x02000000,
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
};

enum {					/* Ctrlext */
	Gpien		= 0x0000000F,	/* General Purpose Interrupt Enables */
	Swdpinshi	= 0x000000F0,	/* Software Defined Pins - hi nibble */
	Swdpiohi	= 0x00000F00,	/* Software Defined Pins - I or O */
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

enum {					/* Txcw */
	Ane		= 0x80000000,	/* Autonegotiate enable */
	Np		= 0x00008000,	/* Next Page */
	As		= 0x00000100,	/* Asymmetric Flow control desired */
	Ps		= 0x00000080,	/* Pause supported */
	Hd		= 0x00000040,	/* Half duplex supported */
	TxcwFd		= 0x00000020,	/* Full Duplex supported */
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
	RdtmsMASK	= 0x00000300,	/* Rdesc Minimum Threshold Size */
	RdtmsHALF	= 0x00000000,	/* Threshold is 1/2 Rdlen */
	RdtmsQUARTER	= 0x00000100,	/* Threshold is 1/4 Rdlen */
	RdtmsEIGHTH	= 0x00000200,	/* Threshold is 1/8 Rdlen */
	MoMASK		= 0x00003000,	/* Multicast Offset */
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
	Gran		= 0x00000000,	/* Granularity */
	RxGran		= 0x01000000,	/* Granularity */
};

enum {					/* Rxcsum */
	PcssMASK	= 0x000000FF,	/* Packet Checksum Start */
	PcssSHIFT	= 0,
	Ipofl		= 0x00000100,	/* IP Checksum Off-load Enable */
	Tuofl		= 0x00000200,	/* TCP/UDP Checksum Off-load Enable */
};

enum {					/* Receive Delay Timer Ring */
	Fpd		= 0x80000000,	/* Flush partial Descriptor Block */
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
	CsoMASK		= 0x00000F00,	/* Checksum Offset */
	CsoSHIFT	= 16,
	Teop		= 0x01000000,	/* End of Packet */
	Ifcs		= 0x02000000,	/* Insert FCS */
	Ic		= 0x04000000,	/* Insert Checksum (Dext == 0) */
	Tse		= 0x04000000,	/* TCP Segmentaion Enable (Dext == 1) */
	Rs		= 0x08000000,	/* Report Status */
	Rps		= 0x10000000,	/* Report Status Sent */
	Dext		= 0x20000000,	/* Extension (!legacy) */
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
	Nrdesc		= 256,		/* multiple of 8 */
	Ntdesc		= 256,		/* multiple of 8 */
	Nblocks		= 4098,		/* total number of blocks to use */

	SBLOCKSIZE	= 2048,
	JBLOCKSIZE	= 16384,

	NORMAL		= 1,
	JUMBO		= 2,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	started;
	int	id;
	ushort	eeprom[0x40];

	int*	nic;
	int	im;			/* interrupt mask */

	Lock	slock;
	uint	statistics[Nstatistics];

	Lock	rdlock;
	Rdesc*	rdba;			/* receive descriptor base address */
	Block*	rb[Nrdesc];		/* receive buffers */
	int	rdh;			/* receive descriptor head */
	int	rdt;			/* receive descriptor tail */
	Block**	freehead;		/* points to long or short head */

	Lock	tdlock;
	Tdesc*	tdba;			/* transmit descriptor base address */
	Block*	tb[Ntdesc];		/* transmit buffers */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */
	int	txstalled;		/* count of times unable to send */

	int	txcw;
	int	fcrtl;
	int	fcrth;

	ulong	multimask[128];		/* bit mask for multicast addresses */
} Ctlr;

static Ctlr* gc82543ctlrhead;
static Ctlr* gc82543ctlrtail;

static Lock freelistlock;
static Block* freeShortHead;
static Block* freeJumboHead;

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static void gc82543watchdog(void* arg);

static void
gc82543attach(Ether* edev)
{
	int ctl;
	Ctlr *ctlr;
	char name[KNAMELEN];

	/*
	 * To do here:
	 *	one-time stuff;
	 *		adjust queue length depending on speed;
	 *		flow control.
	 *	more needed here...
	 */
	ctlr = edev->ctlr;
	lock(&ctlr->slock);
	if(ctlr->started == 0){
		ctlr->started = 1;
		snprint(name, KNAMELEN, "#l%d82543", edev->ctlrno);
		kproc(name, gc82543watchdog, edev);
	}
	unlock(&ctlr->slock);

	ctl = csr32r(ctlr, Rctl)|Ren;
	csr32w(ctlr, Rctl, ctl);
	ctl = csr32r(ctlr, Tctl)|Ten;
	csr32w(ctlr, Tctl, ctl);

	csr32w(ctlr, Ims, ctlr->im);
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

static long
gc82543ifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *p, *s;
	int i, l, r;
	uvlong tuvl, ruvl;

	ctlr = edev->ctlr;
	lock(&ctlr->slock);
	p = malloc(READSTR);
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

	l += snprint(p+l, READSTR-l, "eeprom:");
	for(i = 0; i < 0x40; i++){
		if(i && ((i & 0x07) == 0))
			l += snprint(p+l, READSTR-l, "\n       ");
		l += snprint(p+l, READSTR-l, " %4.4uX", ctlr->eeprom[i]);
	}

	snprint(p+l, READSTR-l, "\ntxstalled %d\n", ctlr->txstalled);
	n = readstr(offset, a, n, p);
	free(p);
	unlock(&ctlr->slock);

	return n;
}

static void
gc82543promiscuous(void* arg, int on)
{
	int rctl;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	rctl = csr32r(ctlr, Rctl);
	rctl &= ~MoMASK;		/* make sure we're using bits 47:36 */
	if(on)
		rctl |= Upe|Mpe;
	else
		rctl &= ~(Upe|Mpe);
	csr32w(ctlr, Rctl, rctl);
}

static void
gc82543multicast(void* arg, uchar* addr, int on)
{
	int bit, x;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	x = addr[5]>>1;
	bit = ((addr[5] & 1)<<4)|(addr[4]>>4);
	if(on)
		ctlr->multimask[x] |= 1<<bit;
	else
		ctlr->multimask[x] &= ~(1<<bit);
	
	csr32w(ctlr, Mta+x*4, ctlr->multimask[x]);
}

static long
gc82543ctl(Ether* edev, void* buf, long n)
{
	Cmdbuf *cb;
	Ctlr *ctlr;
	int ctrl, i, r;

	ctlr = edev->ctlr;
	if(ctlr == nil)
		error(Enonexist);

	lock(&ctlr->slock);
	r = 0;
	cb = parsecmd(buf, n);
	if(cb->nf < 2)
		r = -1;
	else if(cistrcmp(cb->f[0], "auto") == 0){
		ctrl = csr32r(ctlr, Ctrl);
		if(cistrcmp(cb->f[1], "off") == 0){
			csr32w(ctlr, Txcw, ctlr->txcw & ~Ane);
			ctrl |= (Slu|Fd);
			if(ctlr->txcw & As)
				ctrl |= Rfce;
			if(ctlr->txcw & Ps)
				ctrl |= Tfce;
			csr32w(ctlr, Ctrl, ctrl);
		}
		else if(cistrcmp(cb->f[1], "on") == 0){
			csr32w(ctlr, Txcw, ctlr->txcw);
			ctrl &= ~(Slu|Fd);
			csr32w(ctlr, Ctrl, ctrl);
		}
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "clear") == 0){
		if(cistrcmp(cb->f[1], "stats") == 0){
			for(i = 0; i < Nstatistics; i++)
				ctlr->statistics[i] = 0;
		}
		else
			r = -1;
	}
	else
		r = -1;
	unlock(&ctlr->slock);

	free(cb);
	return (r == 0) ? n : r;
}

static void
gc82543txinit(Ctlr* ctlr)
{
	int i;
	int tdsize;
	Block *bp, **bpp;

	tdsize = ROUND(Ntdesc*sizeof(Tdesc), 4096);

	if(ctlr->tdba == nil)
		ctlr->tdba = xspanalloc(tdsize, 32, 0);

	for(i = 0; i < Ntdesc; i++){
		bpp = &ctlr->tb[i];
		bp = *bpp;
		if(bp != nil){
			*bpp = nil;
			freeb(bp);
		}
		memset(&ctlr->tdba[i], 0, sizeof(Tdesc));
	}

	csr32w(ctlr, Tdbal, PCIWADDR(ctlr->tdba));
	csr32w(ctlr, Tdbah, 0);
	csr32w(ctlr, Tdlen, Ntdesc*sizeof(Tdesc));

	/*
	 * set the ring head and tail pointers.
	 */
	ctlr->tdh = 0;
	csr32w(ctlr, Tdh, ctlr->tdh);
	ctlr->tdt = 0;
	csr32w(ctlr, Tdt, ctlr->tdt);

	csr32w(ctlr, Tipg, (6<<20)|(8<<10)|6);
	csr32w(ctlr, Tidv, 128);
	csr32w(ctlr, Ait, 0);
	csr32w(ctlr, Txdmac, 0);
	csr32w(ctlr, Txdctl, Gran|(4<<WthreshSHIFT)|(1<<HthreshSHIFT)|16);
	csr32w(ctlr, Tctl, (0x0F<<CtSHIFT)|Psp|(6<<ColdSHIFT));

	ctlr->im |= Txdw;
}

static void
gc82543transmit(Ether* edev)
{
	Block *bp, **bpp;
	Ctlr *ctlr;
	Tdesc *tdesc;
	int tdh, tdt, s;

	ctlr = edev->ctlr;

	ilock(&ctlr->tdlock);
	tdh = ctlr->tdh;
	for(;;){
		/*
		 * Free any completed packets
		 */
		tdesc = &ctlr->tdba[tdh];
		if(!(tdesc->status & Tdd))
			break;
		memset(tdesc, 0, sizeof(Tdesc));
		bpp = &ctlr->tb[tdh];
		bp = *bpp;
		if(bp != nil){
			*bpp = nil;
			freeb(bp);
		}
		tdh = NEXT(tdh, Ntdesc);
	}
	ctlr->tdh = tdh;
	s = csr32r(ctlr, Status);

	/*
	 * Try to fill the ring back up
	 * but only if link is up and transmission isn't paused.
	 */
	if((s & (Txoff|Lu)) == Lu){
		tdt = ctlr->tdt;
		while(NEXT(tdt, Ntdesc) != tdh){
			if((bp = qget(edev->oq)) == nil)
				break;

			tdesc = &ctlr->tdba[tdt];
			tdesc->addr[0] = PCIWADDR(bp->rp);
			tdesc->control = Ide|Rs|Ifcs|Teop|BLEN(bp);
			ctlr->tb[tdt] = bp;
			tdt = NEXT(tdt, Ntdesc);
		}

		if(tdt != ctlr->tdt){
			ctlr->tdt = tdt;
			csr32w(ctlr, Tdt, tdt);
		}
	}
	else
		ctlr->txstalled++;

	iunlock(&ctlr->tdlock);
}

static Block *
gc82543allocb(Ctlr* ctlr)
{
	Block *bp;

	ilock(&freelistlock);
	if((bp = *(ctlr->freehead)) != nil){
		*(ctlr->freehead) = bp->next;
		bp->next = nil;
		_xinc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&freelistlock);
	return bp;
}

static void
gc82543replenish(Ctlr* ctlr)
{
	int rdt;
	Block *bp;
	Rdesc *rdesc;

	ilock(&ctlr->rdlock);
	rdt = ctlr->rdt;
	while(NEXT(rdt, Nrdesc) != ctlr->rdh){
		rdesc = &ctlr->rdba[rdt];
		if(ctlr->rb[rdt] == nil){
			bp = gc82543allocb(ctlr);
			if(bp == nil){
				iprint("no available buffers\n");
				break;
			}
			ctlr->rb[rdt] = bp;
			rdesc->addr[0] = PCIWADDR(bp->rp);
			rdesc->addr[1] = 0;
		}
		coherence();
		rdesc->status = 0;
		rdt = NEXT(rdt, Nrdesc);
	}
	ctlr->rdt = rdt;
	csr32w(ctlr, Rdt, rdt);
	iunlock(&ctlr->rdlock);
}

static void
gc82543rxinit(Ctlr* ctlr)
{
	int rdsize, i;

	csr32w(ctlr, Rctl, Dpf|Bsize2048|Bam|RdtmsHALF);

	/*
	 * Allocate the descriptor ring and load its
	 * address and length into the NIC.
	 */
	rdsize = ROUND(Nrdesc*sizeof(Rdesc), 4096);
	if(ctlr->rdba == nil)
		ctlr->rdba = xspanalloc(rdsize, 32, 0);
	memset(ctlr->rdba, 0, rdsize);

	ctlr->rdh = 0;
	ctlr->rdt = 0;

	csr32w(ctlr, Rdtr, Fpd|64);
	csr32w(ctlr, Rdbal, PCIWADDR(ctlr->rdba));
	csr32w(ctlr, Rdbah, 0);
	csr32w(ctlr, Rdlen, Nrdesc*sizeof(Rdesc));
	csr32w(ctlr, Rdh, 0);
	csr32w(ctlr, Rdt, 0);
	for(i = 0; i < Nrdesc; i++){
		if(ctlr->rb[i] != nil){
			freeb(ctlr->rb[i]);
			ctlr->rb[i] = nil;
		}
	}
	gc82543replenish(ctlr);

	csr32w(ctlr, Rxdctl, RxGran|(8<<WthreshSHIFT)|(4<<HthreshSHIFT)|1);
	ctlr->im |= Rxt0|Rxo|Rxdmt0|Rxseq;
}

static void
gc82543recv(Ether* edev, int icr)
{
	Block *bp;
	Ctlr *ctlr;
	Rdesc *rdesc;
	int rdh;

	ctlr = edev->ctlr;

	rdh = ctlr->rdh;
	for(;;){
		rdesc = &ctlr->rdba[rdh];

		if(!(rdesc->status & Rdd))
			break;

		if((rdesc->status & Reop) && rdesc->errors == 0){
			bp = ctlr->rb[rdh];
			ctlr->rb[rdh] = nil;
			bp->wp += rdesc->length;
			bp->next = nil;
			etheriq(edev, bp, 1);
		}

		if(ctlr->rb[rdh] != nil){
			/* either non eop packet, or error */
			freeb(ctlr->rb[rdh]);
			ctlr->rb[rdh] = nil;
		}
		memset(rdesc, 0, sizeof(Rdesc));
		coherence();
		rdh = NEXT(rdh, Nrdesc);
	}
	ctlr->rdh = rdh;

	if(icr & Rxdmt0)
		gc82543replenish(ctlr);
}

static void
freegc82543short(Block *bp)
{
	ilock(&freelistlock);
	/* reset read/write pointer to proper positions */
	bp->rp = bp->lim - ROUND(SBLOCKSIZE, BLOCKALIGN);
	bp->wp = bp->rp;
	bp->next = freeShortHead;
	freeShortHead = bp;
	iunlock(&freelistlock);
}

static void
freegc82532jumbo(Block *bp)
{
	ilock(&freelistlock);
	/* reset read/write pointer to proper positions */
	bp->rp = bp->lim - ROUND(JBLOCKSIZE, BLOCKALIGN);
	bp->wp = bp->rp;
	bp->next = freeJumboHead;
	freeJumboHead = bp;
	iunlock(&freelistlock);
}

static void
linkintr(Ctlr* ctlr)
{
	int ctrl;

	ctrl = csr32r(ctlr, Ctrl);

	if((ctrl & Swdpin1) ||
	  ((csr32r(ctlr, Rxcw) & Rxconfig) && !(csr32r(ctlr, Txcw) & Ane))){
 		csr32w(ctlr, Txcw, ctlr->txcw);
		ctrl &= ~(Slu|Fd|Frcdplx);
		csr32w(ctlr, Ctrl, ctrl);
	}
}

static void
gc82543interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	int icr;

	edev = arg;
	ctlr = edev->ctlr;

	while((icr = csr32r(ctlr, Icr) & ctlr->im) != 0){
		/*
		 * Link status changed.
		 */
		if(icr & (Lsc|Rxseq))
			linkintr(ctlr);

		/*
		 * Process recv buffers.
		 */
		gc82543recv(edev, icr);

		/*
		 * Refill transmit ring and free packets.
		 */
		gc82543transmit(edev);
	}
}

static int
gc82543init(Ether* edev)
{
	int csr, i;
	Block *bp;
	Ctlr *ctlr;

	ctlr = edev->ctlr;

	/*
	 * Allocate private buffer pool to use for receiving packets.
	 */
	ilock(&freelistlock);
	if (ctlr->freehead == nil){
		for(i = 0; i < Nblocks; i++){
			bp = iallocb(SBLOCKSIZE);
			if(bp != nil){
				bp->next = freeShortHead;
				bp->free = freegc82543short;
				freeShortHead = bp;
			}
			else{
				print("82543gc: no memory\n");
				break;
			}
		}
		ctlr->freehead = &freeShortHead;
	}
	iunlock(&freelistlock);

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

	gc82543txinit(ctlr);
	gc82543rxinit(ctlr);

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
			if(lp != nil){
				if(p != (lp+1) || loop != 7)
					return -1;
				lp = p;
				loop = 15;
				continue;
			}
			lp = p;
			loop = 7;
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
	int addr, data;

	sum = 0;
	for(addr = 0; addr < 0x40; addr++){
		/*
		 * Read a word at address 'addr' from the Atmel AT93C46
		 * 3-Wire Serial EEPROM or compatible. The EEPROM access is
		 * controlled by 4 bits in Eecd. See the AT93C46 datasheet
		 * for protocol details.
		 */
		if(at93c46io(ctlr, "S ICc :DCc;", (0x02<<6)|addr) != 0)
			break;
		data = at93c46io(ctlr, "::COc;", 0);
		at93c46io(ctlr, "sic", 0);
		ctlr->eeprom[addr] = data;
		sum += data;
	}

	return sum;
}

static void
gc82543detach(Ctlr* ctlr)
{
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
	while(csr32r(ctlr, Ctrl) & Devrst)
		;

	csr32w(ctlr, Ctrlext, Eerst);
	while(csr32r(ctlr, Ctrlext) & Eerst)
		;

	csr32w(ctlr, Imc, ~0);
	while(csr32r(ctlr, Icr))
		;
}

static void
gc82543checklink(Ctlr* ctlr)
{
	int ctrl, status, rxcw;

	ctrl = csr32r(ctlr, Ctrl);
	status = csr32r(ctlr, Status);
	rxcw = csr32r(ctlr, Rxcw);

	if(!(status & Lu)){
		if(!(ctrl & (Swdpin1|Slu)) && !(rxcw & Rxconfig)){
			csr32w(ctlr, Txcw, ctlr->txcw & ~Ane);
			ctrl |= (Slu|Fd);
			if(ctlr->txcw & As)
				ctrl |= Rfce;
			if(ctlr->txcw & Ps)
				ctrl |= Tfce;
			csr32w(ctlr, Ctrl, ctrl);
		}
	}
	else if((ctrl & Slu) && (rxcw & Rxconfig)){
		csr32w(ctlr, Txcw, ctlr->txcw);
		ctrl &= ~(Slu|Fd);
		csr32w(ctlr, Ctrl, ctrl);
	}
}

static void
gc82543shutdown(Ether* ether)
{
	gc82543detach(ether->ctlr);
}

static int
gc82543reset(Ctlr* ctlr)
{
	int ctl;
	int te;

	/*
	 * Read the EEPROM, validate the checksum
	 * then get the device back to a power-on state.
	 */
	if(at93c46r(ctlr) != 0xBABA)
		return -1;

	gc82543detach(ctlr);

	te = ctlr->eeprom[Icw2];
	if((te & 0x3000) == 0){
		ctlr->fcrtl = 0x00002000;
		ctlr->fcrth = 0x00004000;
		ctlr->txcw = Ane|TxcwFd;
	}
	else if((te & 0x3000) == 0x2000){
		ctlr->fcrtl = 0;
		ctlr->fcrth = 0;
		ctlr->txcw = Ane|TxcwFd|As;
	}
	else{
		ctlr->fcrtl = 0x00002000;
		ctlr->fcrth = 0x00004000;
		ctlr->txcw = Ane|TxcwFd|As|Ps;
	}

	csr32w(ctlr, Txcw, ctlr->txcw);

	csr32w(ctlr, Ctrlext, (te & 0x00f0)<<4);

	csr32w(ctlr, Tctl, csr32r(ctlr, Tctl)|(64<<ColdSHIFT));

	te = ctlr->eeprom[Icw1];
	ctl = ((te & 0x01E0)<<17)|(te & 0x0010)<<3;
	csr32w(ctlr, Ctrl, ctl);

	delay(10);

	/*
	 * Flow control - values from the datasheet.
	 */
	csr32w(ctlr, Fcal, 0x00C28001);
	csr32w(ctlr, Fcah, 0x00000100);
	csr32w(ctlr, Fct, 0x00008808);
	csr32w(ctlr, Fcttv, 0x00000100);

	csr32w(ctlr, Fcrtl, ctlr->fcrtl);
	csr32w(ctlr, Fcrth, ctlr->fcrth);

	ctlr->im = Lsc;
	gc82543checklink(ctlr);

	return 0;
}

static void
gc82543watchdog(void* arg)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = arg;
	for(;;){
		tsleep(&up->sleep, return0, 0, 1000);

		ctlr = edev->ctlr;
		if(ctlr == nil){
			print("%s: exiting\n", up->text);
			pexit("disabled", 0);
		}

		gc82543checklink(ctlr);
		gc82543replenish(ctlr);
	}
}

static void
gc82543pci(void)
{
	int cls;
	void *mem;
	Pcidev *p;
	Ctlr *ctlr;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		switch((p->did<<16)|p->vid){
		case (0x1000<<16)|0x8086:	/* LSI L2A1157 (82542) */
		case (0x1004<<16)|0x8086:	/* Intel PRO/1000 T */
		case (0x1008<<16)|0x8086:	/* Intel PRO/1000 XT */
		default:
			continue;
		case (0x1001<<16)|0x8086:	/* Intel PRO/1000 F */
			break;
		}

		mem = vmap(p->mem[0].bar & ~0x0F, p->mem[0].size);
		if(mem == 0){
			print("gc82543: can't map %8.8luX\n", p->mem[0].bar);
			continue;
		}
		cls = pcicfgr8(p, PciCLS);
		switch(cls){
			case 0x00:
			case 0xFF:
				print("82543gc: unusable cache line size\n");
				continue;
			case 0x08:
				break;
			default:
				print("82543gc: cache line size %d, expected 32\n",
					cls*4);
		}
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = p->mem[0].bar & ~0x0F;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;
		ctlr->nic = mem;

		if(gc82543reset(ctlr)){
			free(ctlr);
			continue;
		}

		if(gc82543ctlrhead != nil)
			gc82543ctlrtail->next = ctlr;
		else
			gc82543ctlrhead = ctlr;
		gc82543ctlrtail = ctlr;
	}
}

static int
gc82543pnp(Ether* edev)
{
	int i;
	Ctlr *ctlr;
	uchar ea[Eaddrlen];

	if(gc82543ctlrhead == nil)
		gc82543pci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = gc82543ctlrhead; ctlr != nil; ctlr = ctlr->next){
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

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to
	 * loading the station address in the hardware.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, edev->ea, Eaddrlen) == 0){
		for(i = Ea; i < Eaddrlen/2; i++){
			edev->ea[2*i] = ctlr->eeprom[i];
			edev->ea[2*i+1] = ctlr->eeprom[i]>>8;
		}
	}
	gc82543init(edev);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = gc82543attach;
	edev->transmit = gc82543transmit;
	edev->interrupt = gc82543interrupt;
	edev->ifstat = gc82543ifstat;
	edev->shutdown = gc82543shutdown;
	edev->ctl = gc82543ctl;
	edev->arg = edev;
	edev->promiscuous = gc82543promiscuous;
	edev->multicast = gc82543multicast;

	return 0;
}

void
ether82543gclink(void)
{
	addethercard("82543GC", gc82543pnp);
}
