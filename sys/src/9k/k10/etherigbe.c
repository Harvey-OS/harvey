/*
 * Intel Gigabit Ethernet PCI-Express Controllers:
 *	8256[367], 8257[1-79], 82580, i21[018]
 *
 * Note the exception for the i217.  My i217s (built into Lenovo Thinkserver
 * TS140s) will not transmit and I haven't been able to determine why.  You
 * might have better luck.  The 217 code is quarantined and disabled by default;
 * see ALLOW_217.
 *
 * The i21[789] datasheets are incomplete and point to an unspecified
 * `integrated LAN controller', claimed to be compatible with the 82566.
 * Apparently it's whatever is built in to the ICH8/9/10, which have manuals.
 * I would have concluded that the 217 is just broken, but Windows can drive it
 * with Lenovo's driver, but not with Intel's(!) and the TS140's pxe boot rom
 * can boot through it.  One theory is that Intel's ME is preventing
 * transmission.  Another is that the missing nvram is failing to initialise
 * one of the zillions of registers.
 *
 * Pretty basic, does not use many of the chip `smarts', which tend to
 * be buggy anyway.
 *
 * On the assumption that allowing jumbo packets may make the controller
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

/*
 * if donebit (Rdd or Tdd) is 0, hardware is operating (or will be) on desc.;
 * else, hardware has finished filling or draining this descriptor's buffer.
 */
#define SWOWNS(desc, donebit) ((desc)->status & (donebit))

typedef struct Ctlr Ctlr;
typedef struct Rd Rd;
typedef struct Td Td;

enum {
	/* per-controller tunable parameters */
	/*
	 * [RT]dlen have to be multiples of 128 bytes (maximum cache-line size)
	 * and are 20 bits wide.  [RT]d[th] are 16 bits wide, so N[rt]d must
	 * be <= 64k.  Descriptors are 16 bytes long and naturally aligned,
	 * so N[rt]d must be multiples of 8; making them powers of two makes
	 * arithmetic faster and smaller in NEXT() and watermark computations.
	 */
	Ntd	= 64,		/* too small just slows xmit */
	Maxtd	= 60,		/* 57[12] erratum 70: at most 60 tds in q */
	Nrd	= 128,		/* too small drops packets */
	Nrb	= 1024,		/* private receive buffers, >> Nrd */

	Descalign= 128,		/* required by later models; was initially 16 */
	Slop	= 32,		/* for nested vlan headers, crcs, etc. */
	Rbsz	= 2048,		/* enforced by hw; could be ETHERMAXTU+Slop */
};

/*
 * memory-mapped device registers.
 * these are used as indices to ctlr->regs[], thus divided by 4 (BY2WD).
 * these are in the order they appear in the manual, not numeric order.
 * It was too hard to find them in the book. Ref 21489, rev 2.6.
 *
 * starting with the i21[78], some registers have other meanings.
 */

enum {
	/* General */
	Ctrl	= 0x0000/4,	/* Device Control */
	Status	= 0x0008/4,	/* Device Status */
	Ctrlext	= 0x0018/4,	/* Extended Device Control */
	Mdic	= 0x0020/4,	/* MDI Control */
	Kumctrlsta= 0x0034/4,	/* MAC-PHY Interface */

	/* pre-i21[78] */
	Eec	= 0x0010/4,	/* EEPROM/Flash Control/Data */
	Eerd	= 0x0014/4,	/* EEPROM Read */
	Fla	= 0x001c/4,	/* Flash Access */
	Serdesctl= 0x0024/4,	/* Serdes ana */
	Fcal	= 0x0028/4,	/* Flow Control Address Low */
	Fcah	= 0x002C/4,	/* Flow Control Address High */
	Fct	= 0x0030/4,	/* Flow Control Type */
	Vet	= 0x0038/4,	/* VLAN EtherType */

	/* i21[78] and later */
	Fextnvm6= 0x0010/4,	/* Future Extended NVM 6 */
	Fextnvm5= 0x0014/4,	/* Future Extended NVM 5 */
	Fextnvm4= 0x0024/4,	/* Future Extended NVM 4 */
	Fextnvm	= 0x0028/4,	/* Future Extended NVM */
	Fext	= 0x002c/4,	/* Future Extended */
	Fextnvm2= 0x0030/4,	/* Future Extended NVM 2 */
	Busnum	= 0x0038/4,	/* VLAN EtherType */

	Lpic	= 0x00fc/4,	/* low power idle */
	Fcttv	= 0x0170/4,	/* Flow Control Transmit Timer Value */
	Txcw	= 0x0178/4,	/* Transmit Cfg. Word (571 auto-neg) */
	Rxcw	= 0x0180/4,	/* Receive  Cfg. Word (571 auto-neg) */
	Ledctl	= 0x0E00/4,	/* LED control */
	Extcnf	= 0x0f00/4,	/* extended config (i217) */
	Phyctrl	= 0x0f10/4,	/* phy control (i217) */
	Pba	= 0x1000/4,	/* Packet Buffer Allocation */
	Pbs	= 0x1008/4,	/* " " size */
	Pbeccsts= 0x100c/4,	/* " " ecc status (i217) */

	/* Interrupt */
	Icr	= 0x00C0/4,	/* Interrupt Cause Read */
	Itr	= 0x00c4/4,	/* Interrupt Throttling Rate */
	Ics	= 0x00C8/4,	/* Interrupt Cause Set */
	Ims	= 0x00D0/4,	/* Interrupt Mask Set/Read: enabled causes */
	Imc	= 0x00D8/4,	/* Interrupt mask Clear */
	Iam	= 0x00E0/4,	/* Interrupt acknowledge Auto Mask */

	/* Receive */
	Rctl	= 0x0100/4,	/* Control */
	Rctl1	= 0x0104/4,	/* Queue 1 Control */
	Ert	= 0x2008/4,	/* Early Receive Threshold (573[EVL], 579 only) */
	Fcrtl	= 0x2160/4,	/* Flow Control RX Threshold Low */
	Fcrth	= 0x2168/4,	/* Flow Control Rx Threshold High */
	Psrctl	= 0x2170/4,	/* Packet Split Receive Control */
	Rxpbsize= 0x2404/4,	/* Packet Buffer Size (i210) */
	Rdbal	= 0x2800/4,	/* Rdesc Base Address Low Queue 0 */
	Rdbah	= 0x2804/4,	/* Rdesc Base Address High Queue 0 */
	Rdlen	= 0x2808/4,	/* Descriptor Length Queue 0 (20 bits) */
	Rdh	= 0x2810/4,	/* Descriptor Head Queue 0; index 1st valid */
	Rdt	= 0x2818/4,	/* Descriptor Tail Queue 0; index 1st invalid */
	Rdtr	= 0x2820/4,	/* Descriptor Timer Ring */
	Rxdctl	= 0x2828/4,	/* Descriptor Control */
	Radv	= 0x282C/4,	/* Interrupt Absolute Delay Timer */
	/* 2nd queue, unused except to zero them */
	Rdbal1	= 0x2900/4,	/* Rdesc Base Address Low Queue 1 */
	Rdbah1	= 0x2804/4,	/* Rdesc Base Address High Queue 1 */
	Rdlen1	= 0x2908/4,	/* Descriptor Length Queue 1 */
	Rdh1	= 0x2910/4,	/* Descriptor Head Queue 1 */
	Rdt1	= 0x2918/4,	/* Descriptor Tail Queue 1 */
	Rxdctl1	= 0x2928/4,	/* Descriptor Control Queue 1 */
	/* */
	Rsrpd	= 0x2c00/4,	/* Small Packet Detect */
	Raid	= 0x2c08/4,	/* ACK interrupt delay */
	Cpuvec	= 0x2c10/4,	/* CPU Vector */
	Rxcsum	= 0x5000/4,	/* Checksum Control */
	Rfctl	= 0x5008/4,	/* Filter Control */
	Mta	= 0x5200/4,	/* Multicast Table Array */
	Ral	= 0x5400/4,	/* Receive Address Low */
	Rah	= 0x5404/4,	/* Receive Address High */
	Vfta	= 0x5600/4,	/* VLAN Filter Table Array */
	Mrqc	= 0x5818/4,	/* Multiple Receive Queues Command */
	Rssim	= 0x5864/4,	/* RSS Interrupt Mask */
	Rssir	= 0x5868/4,	/* RSS Interrupt Request */
	Reta	= 0x5c00/4,	/* Redirection Table */
	Rssrk	= 0x5c80/4,	/* RSS Random Key */

	/* Transmit */
	Tctl	= 0x0400/4,	/* Transmit Control */
	Tipg	= 0x0410/4,	/* Transmit IPG */
	Tkabgtxd= 0x3004/4,	/* glci afe band gap transmit ref data, or something */
	Txpbsize= 0x3404/4,	/* Packet Buffer Size (i210) */
	Tdbal	= 0x3800/4,	/* Tdesc Base Address Low Queue 0 */
	Tdbah	= 0x3804/4,	/* Tdesc Base Address High Queue 0 */
	Tdlen	= 0x3808/4,	/* Descriptor Length (20 bits) */
	Tdh	= 0x3810/4,	/* Descriptor Head, index 1st valid */
	Tdt	= 0x3818/4,	/* Descriptor Tail, index 1st invalid */
	Tidv	= 0x3820/4,	/* Interrupt Delay Value */
	Txdctl	= 0x3828/4,	/* Descriptor Control */
	Tadv	= 0x382C/4,	/* Interrupt Absolute Delay Timer */
	/* Tarc also functions as undocumented errata workaround */
	Tarc0	= 0x3840/4,	/* Arbitration Counter Queue 0 */
	/* 2nd queue, unused except to zero them */
	Tdbal1	= 0x3900/4,	/* Descriptor Base Low Queue 1 */
	Tdbah1	= 0x3904/4,	/* Descriptor Base High Queue 1 */
	Tdlen1	= 0x3908/4,	/* Descriptor Length Queue 1 */
	Tdh1	= 0x3910/4,	/* Descriptor Head Queue 1 */
	Tdt1	= 0x3918/4,	/* Descriptor Tail Queue 1 */
	Txdctl1	= 0x3928/4,	/* Descriptor Control 1 */
	Tarc1	= 0x3940/4,	/* Arbitration Counter Queue 1 */

	/* Statistics */
	Statistics= 0x4000/4,	/* Start of Statistics Area */
	Gorcl	= 0x88/4,	/* Good Octets Received Count */
	Gotcl	= 0x90/4,	/* Good Octets Transmitted Count */
	Torl	= 0xC0/4,	/* Total Octets Received */
	Totl	= 0xC8/4,	/* Total Octets Transmitted */
	Nstatistics= 0x124/4,

	/* Management */
	/* the wake-up registers are only reset by power-cycle, damn them */
	Wuc	= 0x5800/4,	/* Wake Up Control */
	Wufc	= 0x5808/4,	/* Wake Up Filter Control */

	/* undocumented errata workaround registers */
	Fextnvm11= 0x5bbc/4,
};

enum {					/* Ctrl */
	Fd		= 1<<0,		/* full duplex */
	GIOmd		= 1<<2,		/* BIO master disable; self-clearing */
	Lrst		= 1<<3,		/* link reset */
	Slu		= 1<<6,		/* Set Link Up */
	SspeedMASK	= 3<<8,		/* Speed Selection */
	SspeedSHIFT	= 8,
	Sspeed10	= 0x00000000,	/* 10Mb/s */
	Sspeed100	= 0x00000100,	/* 100Mb/s */
	Sspeed1000	= 0x00000200,	/* 1000Mb/s */
	Frcspd		= 1<<11,	/* Force Speed */
	Frcdplx		= 1<<12,	/* Force Duplex */
	Lanphycovr	= 1<<16,	/* lanphyc bit override */
	Lanphyc 	= 1<<17,	/* lanphyc bit */
	Mehe		= 1<<19, /* Memory Error Handling Enable (217, allegedly) */
	Pfrstdone	= 1<<21,	/* sw or dev reset is done (i210) */
	Devrst		= 1<<26,	/* Device Reset (SWRST) */
	Rfce		= 1<<27,	/* Receive Flow Control Enable */
	Tfce		= 1<<28,	/* Transmit Flow Control Enable */
	Vme		= 1<<30,	/* VLAN Mode Enable */
	Phyrst		= 1<<31,	/* Phy Reset */
};

enum {					/* Status */
	Lu		= 1<<1,		/* Link Up */
	Lanid		= 3<<2,		/* mask for Lan ID. (!i217) */
	Txoff		= 1<<4,		/* Transmission Paused */
	Tbimode		= 1<<5,		/* TBI Mode Indication (!i217) */
	Speed		= 3<<6,		/* link speed (i217) */
	Laninitdone	= 1<<9,		/* (i217) */
	Phyra		= 1<<10,	/* PHY Reset Asserted */
	GIOme		= 1<<19,	/* GIO Master Enable Status */
};

enum {					/* Eec */
	EEflsdet	= 1<<19,	/* i210 flash detected */
	EEwdunits	= 1<<22,	/* units are words (shorts)? */
};

enum {					/* Eerd */
	EEstart		= 1<<0,		/* Start Read */
	EEdone		= 1<<1,		/* Read done */
};

enum {				/* Ctrlext */
	Lpcd	= 1<<2,		/* lan ctlr power cycle done (217) */
	Frcmacsmbus = 1<<11,	/* Force SMBus mode on MAC (217, undoc) */
	Eerst	= 1<<13,	/* EEPROM Reset (!217) */
	Spdbyps	= 1<<15,	/* Speed Select Bypass */
	Rodis	= 1<<17,	/* relaxed ordering disabled (210) */
	Phypden	= 1<<20,	/* enables phy low-power state on idle (217) */
	Extmbo57 = 1<<22,	/* must be on for ich8 */
	Iame	= 1<<27,	/* intr ack auto-mask enable (217) */
	Drvload	= 1<<28,	/* drive is loaded (217) */
	Inttmrsclrena = 1<<29,	/* intr timers clear enable (217) */
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

enum {					/* Phyctrl (i217) */
	Gbedis		= 1<<6,		/* don't negotiate Gb speed */
};

/*
 * there may be two phy addresses (e.g., 1 and 2) but there is only one phy.
 * registers are ultimately addressed as (phy, page, register), but we derive
 * phy from page & register (which won't work for some odd-ball cases).
 */
enum {					/* phy interface registers */
	/* initial 16 regs are standardized by ieee & same in all pages */
	Phyctl		= 0,		/* phy ctl */
	Physts		= 1,
	Phyfixedend	= 15,		/* after this reg, each page differs */

					/* page 0 */
	Physsr		= 17,		/* phy secondary status */
	Phyier		= 18,		/* 82573 phy interrupt enable */
	Phyisr		= 19,		/* 82563 phy interrupt status */
	Phylhr		= 19,		/* 8257[1256] link health */
	Phyctl0_218	= 22,
	Phyctl1_218	= 23,
	Phyier218	= 24,		/* 218 (phy79?) phy interrupt enable */
	Phyisr218	= 25,		/* 218 (phy79?) phy interrupt status */
	Phystat		= 26,		/* 82580 (phy79?) phy status */

	Phypage217	= 31,		/* page number (in every page), 21[78] */
	Phyregend	= 31,

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
	Ans		= 3<<14,	/* 82580 autoneg. status */
	Link		= 1<<6,		/* 82580, 217 link */

	/* 21[78] Phystat bits */
	Anfs		= 3<<13,	/* fault status */
	Ans218		= 1<<12,	/* autoneg complete */

	/* 218 Phyier218 interrupt enable bits */
	Spdie218	= 1<<1,		/* speed changed */
	Lscie218	= 1<<2,		/* link status changed */
	Ancie218	= 1<<8,		/* auto-negotiation changed */
};

enum {					/* Extcnf */
	Swflag		= 1<<5,		/* give me access to certain regs */
};

enum {					/* Pbeccsts */
	Pbeccena	= 1<<16,	/* i217, allegedly */
};
enum {
	Enable		= 1<<31,	/* used in several registers */
};

enum {					/* interrupts: Icr, Ics, Ims, Imc */
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
	Txdlow		= 1<<15,	/* td low threshold hit (i217) */
	Ack		= 0x00020000,	/* Receive ACK frame */
	/*
	 * bits 20-25 used to indicate parity errors; they've been repurposed
	 * at least once.
	 */
	Parityeccrst	= MASK(6) << 20, /* were parity; now ecc, me reset */
	Rxq0		= 0x00100000,	/* Rx Queue 0 */
	Eprst		= 1<<20,	/* ME reset occurred (i217) */
	RxQ1		= 0x00200000,	/* Rx Queue 1 */
	Eccer		= 1<<22,	/* uncorrectable ECC error (i217) */
	Txq0		= 0x00400000,	/* Tx Queue 0 */
	Txq1		= 0x00800000,	/* Tx Queue 1 */
	Intassert	= 1u<<31,	/* interrupt pending (i217) */
};

enum {					/* Rctl */
	Rrst		= 0x00000001,	/* Receiver Software Reset */
	Ren		= 0x00000002,	/* Receiver Enable */
	Sbp		= 0x00000004,	/* Store Bad Packets */
	Upe		= 0x00000008,	/* Unicast Promiscuous Enable */
	Mpe		= 0x00000010,	/* Multicast Promiscuous Enable */
	Lpe		= 0x00000020,	/* Long Packet Reception Enable */
	LbmOFF		= 0x00000000,	/* No Loopback */
	LbmTBI		= 0x00000040,	/* TBI Loopback */
	LbmMII		= 0x00000080,	/* GMII/MII Loopback */
	LbmMASK		= 0x000000C0,	/* Loopback Mode */
	LbmXCVR		= 0x000000C0,	/* Transceiver Loopback */
	RdtmsHALF	= 0x00000000,	/* Threshold is 1/2 Rdlen */
	RdtmsQUARTER	= 0x00000100,	/* Threshold is 1/4 Rdlen */
	RdtmsEIGHTH	= 0x00000200,	/* Threshold is 1/8 Rdlen */
	RdtmsMASK	= 0x00000300,	/* Rdesc Minimum Threshold Size */
	Dtyp		= 0x00000c00,	/* 0 is legacy */
	MoMASK		= 0x00003000,	/* Multicast Offset */
	Bam		= 0x00008000,	/* Broadcast Accept Mode */
	Bsize2048	= 0x00000000,
	Bsize1024	= 0x00010000,
	Bsize16384	= 0x00010000,	/* Bsex = 1 */
	Bsize512	= 0x00020000,
	Bsize8192	= 0x00020000,	/* Bsex = 1 */
	Bsize256	= 0x00030000,
	BsizeMASK	= 0x00030000,	/* Receive Buffer Size */
	Vfe		= 0x00040000,	/* VLAN Filter Enable */
	Cfien		= 0x00080000,	/* Canonical Form Indicator Enable */
	Cfi		= 0x00100000,	/* Canonical Form Indicator value */
	Dpf		= 0x00400000,	/* Discard Pause Frames */
	Pmcf		= 0x00800000,	/* Pass MAC Control Frames */
	Bsex		= 0x02000000,	/* Buffer Size Extension */
	Secrc		= 0x04000000,	/* Strip CRC from incoming packet */
	BsizeFlex	= 0x08000000,	/* Flexible Bsize in 1KB increments */
};

enum {					/* Tctl */
	Trst		= 0x00000001,	/* Transmitter Software Reset */
	Ten		= 0x00000002,	/* Transmit Enable */
	Psp		= 0x00000008,	/* Pad Short Packets */
	Ctmask		= 0x00000FF0,	/* Collision Threshold */
	Ctshift		= 4,
	ColdMASK	= 0x003FF000,	/* Collision Distance */
	ColdSHIFT	= 12,
	Swxoff		= 0x00400000,	/* Sofware XOFF Transmission */
	Pbe		= 0x00800000,	/* Packet Burst Enable */
	Rtlc		= 0x01000000,	/* Re-transmit on Late Collision */
	Nrtu		= 0x02000000,	/* No Re-transmit on Underrrun */
	Mulr		= 0x10000000,	/* Allow multiple concurrent requests */
};

enum {					/* [RT]xdctl */
	PthreshMASK	= 0x0000003F,	/* Prefetch Threshold */
	PthreshSHIFT	= 0,
	HthreshMASK	= 0x00003F00,	/* Host Threshold */
	HthreshSHIFT	= 8,
	WthreshMASK	= 0x003F0000,	/* Writeback Threshold */
	WthreshSHIFT	= 16,
	Mbo57		= 1<<22,	/* tx: must be 1 on 57[124], ich8 */
	Gran		= 0x01000000,	/* Granularity (descriptors, not cls) */
	Qenable		= 0x02000000,	/* Queue Enable (82575, i210) */
	Swflush		= 0x04000000,	/* (i210) */
	LwthreshMASK	= 0xfe000000,	/* tx: descriptor low threshold (!575) */
	LwthreshSHIFT	= 25,
};

enum {					/* Rxcsum */
	PcssMASK	= 0x00FF,	/* Packet Checksum Start */
	PcssSHIFT	= 0,
	Ipofl		= 0x0100,	/* IP Checksum Off-load Enable */
	Tuofl		= 0x0200,	/* TCP/UDP Checksum Off-load Enable */
};

enum {		/* Receive Delay Timer Ring & Transmit Interrupt Delay Value */
	DelayMASK	= 0xFFFF,	/* delay timer in 1.024nS increments */
	DelaySHIFT	= 0,
	Fpd		= 0x80000000,	/* Flush partial Descriptor Block (WO) */
};

/* there are lots of undocumented magic errata workaround bits in the upper ½ */
enum {					/* Transmit Arbitration Counter (Tarc?) */
	Tarcount	= 1,		/* required */
	Tarcqena	= 1<<10,	/* enable this transmit queue */
	Tarcmbo		= 1<<26,	/* errata requires this */
	Tarcpreserve	= 1<<27,	/* must not be changed */
};

enum {					/* Wake Up Control */
	Apme		= 1<<0,		/* advanced power mgmt. enable */
	Pme_en		= 1<<1,		/* power mgmt enable */
	Apmpme		= 1<<3,		/* assert pme on apm wakeup */
};

enum {					/* Fextnvm11 mystery register */
	F11norsthang	= 1<<13,	/* avoid reset hang (i219) */
};

struct Rd {				/* Receive Descriptor */
	uvlong	vladdr;
	ushort	length;
	ushort	checksum;		/* claimed valid iff (status&Ixsm)==0 */
	uchar	status;
	uchar	errors;
	ushort	special;
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
	uvlong	vladdr;			/* Data */
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
	Tdd		= 0x0001,	/* Descriptor Done */
	Ec		= 0x0002,	/* Excess Collisions */
	Lc		= 0x0004,	/* Late Collision */
	Tu		= 0x0008,	/* Transmit Underrun */
	CssMASK		= 0xFF00,	/* Checksum Start Field */
	CssSHIFT	= 8,
};

typedef struct {
	ushort	*reg;
	uint	*reg32;
	ushort	base;
	ushort	lim;
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
	/* i21[78] in-band control */
	I217icpage		= 770,		/* phy page */
	I217icreg		= 18,		/* phy register */
	I217iclstmoutmask	= 0x3F00,	/* link status tx timeout mask */
	I217iclstmoutshift	= 8,		/* " " " " shift */

	Fextnvm6reqpllclk	= 0x100,
	Fextnvm6enak1entrycond	= 0x200,	/* extend K1 entry latency */

//	Nvmk1cfg		= 0x1B,		/* NVM K1 Config Word */
//	Nvmk1enable		= 0x1,		/* NVM Enable K1 bit */

	Kumctrlstaoff		= 0x1F0000,
	Kumctrlstaoffshift	= 16,
	Kumctrlstaren		= 0x200000,	/* read enable */

	Kumctrlstak1cfg		= 7,		/* register address */
	Kumctrlstak1enable	= 0x2,
};

enum {
	Iany,
	i82563, i82566, i82567,
	i82571, i82572, i82573, i82574, i82575, i82576, i82577, i82579,
	i82580,
	i210, i217, i218,
	Nctlrtypes,
};

static char *tname[] = {
[Iany] "any",
[i82563] "i82563",
[i82566] "i82566",
[i82567] "i82567",
[i82571] "i82571",		/* very fussy */
[i82572] "i82572",
[i82573] "i82573",
[i82574] "i82574",
[i82575] "i82575",
[i82576] "i82576",
[i82577] "i82577",
[i82579] "i82579",
[i82580] "i82580",
[i210] "i210",
[i217] "i217",			/* so fussy it won't transmit */
[i218] "i218",
};

struct Ctlr {
	Ethident;		/* see etherif.h */
	Pcidev	*pcidev;
	Ctlr	*next;		/* next in chain of discovered Ctlrs */
	uintptr	port;		/* used during discovery & printing */

	Lock	imlock;
	int	im;		/* interrupt mask (enables) */

	/* receiver */
	uint	rdt;		/* rx desc tail index (add descs here) */
	uint	rdh;		/* " " head index (hw filling head) */
	Rendez	rrendez;
	int	rim;		/* interrupt cause bits from interrupt svc */
	Block	**rb;		/* rx pkt buffers */
	Watermark wmrd;
	Watermark wmrb;
	Rd	*rdba;		/* rx descs base address */
	int	rdfree;		/* # of rx descs awaiting packets */
	int	mcastbits;

	/* transmitter */
	uint	tdt;		/* tx desc tail index (add descs here) */
	uint	tdh;		/* " " head index (hw sending head) */
	Rendez	trendez;
	int	tim;		/* interrupt cause bits from interrupt svc */
	Block	**tb;		/* tx pkt buffers */
	Watermark wmtd;
	QLock	tlock;
	Td	*tdba;		/* tx descs base address */

	/* link status */
	Rendez	lrendez;
	int	lim;		/* interrupt cause bits from interrupt svc */
	int	didk1fix;

	/* phy */
	int	phyaddgen;	/* general registers addr */
	int	phyaddspec;	/* phy-specific registers addr */
	int	phypage;

	QLock	alock;		/* attach */
	int	attached;
	int	active;

	/* statistics */
	QLock	slock;
	uint	lsleep;
	uint	lintr;
	uint	rsleep;
	uint	rintr;
	uint	txdw;		/* count of ring-full in transmit */
	uint	tintr;
	uint	statistics[Nstatistics];

	/* sprawling arrays */
	uint	speeds[4];
	uchar	ra[Eaddrlen];	/* receive address */
	ushort	eeprom[0x40];
	ulong	mta[1 << (12-5)]; /* maximal multicast table (copy) */
};

static Ctlr* i82563ctlrhead;
static Ctlr* i82563ctlrtail;

/* these are shared by all the controllers of this type */
static Lock i82563rblock;
static Block* i82563rbpool;	/* free receive Blocks */
/* # of rcv Blocks awaiting processing; can be briefly < 0 */
static	int	nrbfull;

/* forwards */
static int	phynum(Ctlr *ctlr, int reg);
static uint	phypage217(Ctlr *ctlr, int page);
static uint	phyread(Ctlr *ctlr, int reg);
static uint	phywrite(Ctlr *ctlr, int reg, ushort val);

static int speedtab[] = {
	10, 100, 1000, 0
};

/* order here corresponds to Stats registers */
static char* statistics[] = {
	"crc err",
	"alignment err",
	"symbol err",
	"rcv err",
	"missed",
	"single collision",
	"excessive collisions",
	"multiple collision",
	"late collisions",
	nil,
	"collision",
	"xmit underrun",
	"defer",
	"xmit - no crs",
	"sequence err",
	"carrier extension err",
	"rcv err length",
	nil,
	"xon rcved",
	"xon xmited",
	"xoff rcved",
	"xoff xmited",
	"fc rcved unsupported",
	nil,	/* "pkts rcved (64 bytes)", */
	nil,	/* "pkts rcved (65-127 bytes)", */
	nil,	/* "pkts rcved (128-255 bytes)", */
	nil,	/* "pkts rcved (256-511 bytes)", */
	nil,	/* "pkts rcved (512-1023 bytes)", */
	nil,	/* "pkts rcved (1024-1522 bytes)", */
	"rcv good",
	"rcv broadcast",
	"rcv multicast",
	"xmit good",
	nil,
	"rcv good bytes",
	nil,
	"xmit good bytes",
	nil,
	nil,
	nil,
	"rcv no buffers",
	"rcv undersize",
	"rcv fragment",
	"rcv oversize",
	"rcv jabber",
	"rcv mgmt",
	"drop mgmt",
	"xmit mgmt",
	"rcv bytes",
	nil,
	"xmit bytes",
	nil,
	"rcv pkts",
	"xmit pkts",
	nil,	/* "pkts xmited (64 bytes)", */
	nil,	/* "pkts xmited (65-127 bytes)", */
	nil,	/* "pkts xmited (128-255 bytes)", */
	nil,	/* "pkts xmited (256-511 bytes)", */
	nil,	/* "pkts xmited (512-1023 bytes)", */
	nil,	/* "pkts xmited (1024-1522 bytes)", */
	"xmit multicast",
	"xmit broadcast",
	"tcp segmentation context xmited",
	"tcp segmentation context fail",
	"intr assertion",
	"intr rcv pkt timer",
	"intr rcv abs timer",
	"intr xmit pkt timer",
	"intr xmit abs timer",
	"intr xmit queue empty",
	"intr xmit desc low",
	"intr rcv min",
	"intr rcv overrun",
};

/*
 *  End of declarations and start of code.
 */

static ulong
regwrbarr(Ctlr *ctlr)			/* work around hw bugs */
{
	coherence();
	return ctlr->regs[Status];
}

#undef ALLOW_217		/* define if you want to try to debug it */
#ifdef ALLOW_217
#include "etheri217.c"
#else
/* i217 stubs */
static void
takeswflag(Ctlr *) {}

static void
relswflag(Ctlr *) {}

static void
unstall217(Ctlr *) {}

static void
coddle217(Ctlr *) {}

static int
coddle217tx(Ctlr *)
{
	return 0;
}

static ulong
eccerr217(Ctlr *, ulong, ulong im)
{
	return im;
}
#endif

static int
needsk1fix(Ctlr *ctlr)
{
	return ctlr->type == i217 || ctlr->type == i218;
}

static int
hasfutextnvm(Ctlr *ctlr)
{
	/* including the 218, which has futextnvm regs, here seems to break it */
	return ctlr->type == i217;
}

static long
i82563ifstat(Ether* edev, void* a, long n, ulong offset)
{
	int i;
	uint r;
	uint *regs, *stats, *spds;
	uvlong tuvl, ruvl;
	char *s, *p, *e, *stat;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	regs = ctlr->regs;
	qlock(&ctlr->slock);
	p = s = malloc(READSTR);
	if(p == nil) {
		qunlock(&ctlr->slock);
		error(Enomem);
	}
	e = p + READSTR;

	/* copy counts from regs to ctlr */
	stats = ctlr->statistics;
	for(i = 0; i < Nstatistics; i++){
		r = regs[Statistics + i];
		if((stat = statistics[i]) == nil)
			continue;
		switch(i){
		case Gorcl:
		case Gotcl:
		case Torl:
		case Totl:
			/* r is low ulong of a uvlong */
			ruvl = (uvlong)regs[Statistics+i+1]<<32 | r;
			tuvl = ruvl + (stats[i] | (uvlong)stats[i+1] << 32);
			if(tuvl != 0) {
				stats[i] = tuvl;
				stats[i+1] = tuvl >> 32;
				p = seprint(p, e, "%s\t%llud %llud\n",
					stat, tuvl, ruvl);
			}
			i++;				/* skip high long */
			break;

		default:
			stats[i] += r;
			if(stats[i] != 0)
				p = seprint(p, e, "%s\t%ud %ud\n", stat,
					stats[i], r);
			break;
		}
	}

	p = seprint(p, e, "rintr\t%ud %ud\n", ctlr->rintr, ctlr->rsleep);
	p = seprint(p, e, "tintr\t%ud %ud\n", ctlr->tintr, ctlr->txdw);
	p = seprint(p, e, "lintr\t%ud %ud\n", ctlr->lintr, ctlr->lsleep);
	p = seprint(p, e, "ctrl\t%#.8ux\n", regs[Ctrl]);
	p = seprint(p, e, "ctrlext\t%#.8ux\n", regs[Ctrlext]);
	p = seprint(p, e, "status\t%#.8ux\n", regs[Status]);
	p = seprint(p, e, "rctl\t%#.8ux\n", regs[Rctl]);
	p = seprint(p, e, "txdctl\t%#.8ux\n", regs[Txdctl]);
	if (0) {
		p = seprint(p, e, "eeprom:");
		for(i = 0; i < nelem(ctlr->eeprom); i++){
			if(i && ((i & 7) == 0))
				p = seprint(p, e, "\n       ");
			p = seprint(p, e, " %#4.4ux", ctlr->eeprom[i]);
		}
		p = seprint(p, e, "\n");
	}

	spds = ctlr->speeds;
	p = seprint(p, e, "speeds: 10:%ud 100:%ud 1000:%ud ?:%ud\n",
		spds[0], spds[1], spds[2], spds[3]);
	p = seprint(p, e, "%æ\n", ctlr);
	p = seprint(p, e, "nrbfull (rcv Blocks outstanding): %d\n", nrbfull);
	p = seprintmark(p, e, &ctlr->wmrb);
	p = seprintmark(p, e, &ctlr->wmrd);
	seprintmark(p, e, &ctlr->wmtd);
	n = readstr(offset, a, n, s);
	free(s);
	qunlock(&ctlr->slock);

	return n;
}

/*
 *  Block buffer allocation
 */

static Block*
i82563rballoc(void)
{
	Block *bp;

	ilock(&i82563rblock);
	if((bp = i82563rbpool) != nil){
		i82563rbpool = bp->next;
		bp->next = nil;
#ifndef PLAN9K
		/* avoid bp being freed so we can reuse it (9 only, for gre) */
		ainc(&bp->ref);
#endif
	}
	iunlock(&i82563rblock);

	return bp;
}

static void
i82563rbfree(Block* bp)
{
	bp->wp = bp->rp = (uchar *)ROUNDDN((uintptr)bp->lim - Rbsz, BLOCKALIGN);
	assert(bp->rp >= bp->base);
	assert(((uintptr)bp->rp & (BLOCKALIGN-1)) == 0);
	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(&i82563rblock);
	bp->next = i82563rbpool;
	i82563rbpool = bp;
	iunlock(&i82563rblock);
	adec(&nrbfull);
}

/* add interrupt causes in bit mask `im' to set of causes allowed to interrupt */
static void
enabintrcauses(Ctlr* ctlr, int im)
{
	ilock(&ctlr->imlock);
	ctlr->im |= im;
	ctlr->regs[Ims] = ctlr->im;
	iunlock(&ctlr->imlock);
	regwrbarr(ctlr);
}

/*
 *  transmitter
 */

/*
 * 57[12] manuals at least require same Wthresh for rx & tx, although we
 * don't do that.  all of these controllers have the same P, H and W
 * thresholds.  all have tx L too but 575, i210 & i217.  575 & i210 have
 * Qenable & Swflush and no Gran; others have Gran & L threshold only.
 * NB: Qenable and LwthreshMASK are mutually exclusive.
 *
 * Gran: use descriptor (16 bytes), not `cache-line' (256 bytes), counts
 * in thresholds.
 */
static ulong
settxdctl(Ctlr *ctlr)
{
	uint *regs;
	ulong r;

	regs = ctlr->regs;
	r = regs[Txdctl] & ~(Gran|WthreshMASK|PthreshMASK|HthreshMASK);
	switch (ctlr->type) {
	case i82571:
		/* the 571 is fussy and this works, so just leave it alone. */
		r = regs[Txdctl] & ~(Qenable|WthreshMASK|PthreshMASK);
		r |= 4<<WthreshSHIFT | 4<<PthreshSHIFT;
		break;
	case i82572:
	case i82574:
		r |= /* Gran | */ Mbo57 | 2<<WthreshSHIFT | 2<<PthreshSHIFT;
		break;
	case i217:
		r = coddle217tx(ctlr);
		break;
	case i82575:				/* multi-queue controllers */
	case i82576:
	case i82580:
	case i210:
		r |= Qenable;
		/* fall through */
	default:
		r |= Gran | 1<<WthreshSHIFT | 1<<HthreshSHIFT | 1<<PthreshSHIFT;
		break;
	}
	if (!(r & Qenable)) {
		r &= ~LwthreshMASK;
		r |= 1<<LwthreshSHIFT;
	}
	if (ctlr->type != i217) {
		regs[Tarc0] = Tarcount | Tarcqena | Tarcmbo |
			regs[Tarc0] & Tarcpreserve;
		regwrbarr(ctlr);
	}
	regs[Txdctl] = r;
	return r;
}

static void
i82563txinit(Ctlr* ctlr)
{
	int i;
	ulong tctl;
	uvlong phys;		/* instead of uintptr, for 9 compat */
	uint *regs;
	Block *bp;

	regs = ctlr->regs;
	regs[Imc] = ~0;
	tctl = 0x0F<<Ctshift | Psp;
	switch (ctlr->type) {
	case i210:
		break;
	case i217:
		tctl |= 64<<ColdSHIFT | Rtlc;
		break;
	default:
	case i218:
		tctl |= 66<<ColdSHIFT;
		break;
	}
	regs[Tctl] = tctl;		/* disable tx before reconfiguring */
	regwrbarr(ctlr);

	delay(10);

	/* we could perhaps be re-initialising */
	for(i = 0; i < Ntd; i++)
		if((bp = ctlr->tb[i]) != nil) {
			ctlr->tb[i] = nil;
			freeb(bp);
		}

	/* setup only transmit queue 0 */
	memset(ctlr->tdba, 0, Ntd * sizeof(Td));
	regwrbarr(ctlr);

	regs[Tdlen] = Ntd * sizeof(Td);
	regwrbarr(ctlr);

	phys = PCIWADDR(ctlr->tdba);
	regs[Tdbal] = phys;
	regs[Tdbah] = phys>>32;
	regwrbarr(ctlr);

	/* Tdh and Tdt are supposed to have been zeroed by reset */
	ctlr->tdh = PREV(0, Ntd);
	ctlr->tdt = 0;

	/* don't coalesce interrupts, as far as possible */
	regs[Tidv] = 1 | Fpd;	/* 1.024µs units; mustn't be 0. Fpd for 217 */
	regs[Tadv] = 0;
	regs[Tipg] = 6<<20 | 8<<10 | 8;		/* yb sez: 0x702008 */

	settxdctl(ctlr);
	regs[Imc] = ~0;
	regwrbarr(ctlr);

	if (ctlr->edev->link) {
		regs[Tctl] |= Ten;
		regwrbarr(ctlr);
	}
}

/*
 * reclaim any available transmit descs.
 */
static void
i82563cleanup(Ctlr *ctlr)
{
	uint tdh, n;
	Block *bp;

	tdh = ctlr->tdh;
	/* Tdd indicates a reusable td; sw owns it */
	while(ctlr->tdba[n = NEXT(tdh, Ntd)].status & Tdd){
		tdh = n;
		bp = ctlr->tb[tdh];
		ctlr->tb[tdh] = nil;
		freeb(bp);
		ctlr->tdba[tdh].status = 0;
	}
	ctlr->tdh = tdh;
}

static void
newtdt(Ctlr *ctlr, uint tdt)
{
	ctlr->tdt = tdt;
	if ((ctlr->regs[Tctl] & Ten) == 0)
		return;		/* don't update Tdt while xmitter disabled */
	coherence();
	ctlr->regs[Tdt] = tdt;	/* notify ctlr of new pkts before this */
	regwrbarr(ctlr);
}

/*
 * copy Blocks out of edev->oq and attach to transmit descs until we exhaust
 * the queue or the available descs.
 */
static void
i82563transmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	Td *td;
	uint tdh, tdt, nxt, kicked, tdqlen;

	ctlr = edev->ctlr;
	if (ctlr == nil)
		panic("transmit: nil ctlr");
	qlock(&ctlr->tlock);
	if (!ctlr->attached) {
		qunlock(&ctlr->tlock);
		return;
	}

	/*
	 * Free any completed Blocks (packets)
	 */
	i82563cleanup(ctlr);

	/*
	 * if link is down on 21[78], we need k1fix to run first, so bail.
	 */
	if (!edev->link && !ctlr->didk1fix && needsk1fix(ctlr)) {
		qunlock(&ctlr->tlock);
		return;
	}

	/*
	 * Try to fill the ring back up.
	 */
	tdt = ctlr->tdt;
	kicked = 0;
	for(;;){
		nxt = NEXT(tdt, Ntd);
		/*
		 * note size of queue of tds awaiting transmission.
		 */
		tdh = ctlr->tdh;
		tdqlen = (uint)(tdt + Ntd - tdh) % Ntd;
		notemark(&ctlr->wmtd, tdqlen);
		if(tdqlen >= Maxtd || nxt == tdh){  /* ring full (enough)? */
			ctlr->txdw++;
			/* intr. when next pkt sent */
			enabintrcauses(ctlr, Txdw|Txqe|Txdlow|Parityeccrst);
			break;
		}
		if((bp = qget(edev->oq)) == nil)
			break;

		td = &ctlr->tdba[tdt];
		/* td->status, thus Tdd, will have been zeroed by i82563cleanup */
		td->vladdr = PCIWADDR(bp->rp);
		td->control = Rs | Ifcs | Teop | BLEN(bp);
		ctlr->tb[tdt] = bp;
		regwrbarr(ctlr);

		tdt = nxt;
		if (!kicked) {
			newtdt(ctlr, tdt);
			kicked = 1;
		}
	}
	if(ctlr->tdt != tdt)
		newtdt(ctlr, tdt);
	/* else may not be any new ones, but could be some still in flight */
	qunlock(&ctlr->tlock);
}

/*
 * reclaim any available read descs and attach a Block buffer to each.
 */
static void
i82563replenish(Ctlr* ctlr)
{
	Rd *rd;
	uint rdt, nxt;
	Block *bp;
	Block **rb;

	for(rdt = ctlr->rdt; nxt = NEXT(rdt, Nrd), nxt != ctlr->rdh; rdt = nxt){
		rd = &ctlr->rdba[rdt];
		rb = &ctlr->rb[rdt];
		if(*rb != nil){
			print("%æ: rx overrun\n", ctlr);
			break;
		}
		*rb = bp = i82563rballoc();
		if(bp == nil)
			/*
			 * this can probably be changed to a "break", but
			 * that hasn't been tested in this driver.
			 */
			panic("%æ: all %d rx buffers in use, nrbfull %d",
				ctlr, Nrb, nrbfull);
		rd->vladdr = PCIWADDR(bp->rp);
		rd->status = 0;		/* clear Rdd, among others */
		ctlr->rdfree++;
	}
	coherence();
	ctlr->regs[Rdt] = ctlr->rdt = rdt;	/* notify ctlr of new buffers */
	coherence();
}

static int
txrdy(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	/*
	 * the SWOWNS test will always be true because we leave a gap of a few
	 * invalid descs between tail and head.
	 */
	coherence();
	return ctlr->tim != 0 || qlen(ctlr->edev->oq) > 0;
		// && SWOWNS(&ctlr->tdba[ctlr->tdt], Tdd);
}

static void
i82563tproc(void *v)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = v;
	ctlr = edev->ctlr;
	for(;;){
		enabintrcauses(ctlr, Txdw|Txqe|Txdlow|Parityeccrst);
		sleep(&ctlr->trendez, txrdy, ctlr);
		ctlr->tim = 0;
		/*
		 * perhaps some buffers have been transmitted and their
		 * descriptors can be reused to copy Blocks out of edev->oq.
		 */
		i82563transmit(edev);
	}
}

/*
 *  eeprom/flash/nvram/etc. access
 */

static ushort
eeread(Ctlr *ctlr, int adr)
{
	ulong n;

	ctlr->regs[Eerd] = EEstart | adr << 2;
	coherence();
	for (n = 1000000; (ctlr->regs[Eerd] & EEdone) == 0 && --n > 0; )
		pause();
	if (n == 0)
		panic("%æ: eeread stuck", ctlr);
	return ctlr->regs[Eerd] >> 16;
}

/* load eeprom into ctlr */
static int
eeload(Ctlr *ctlr)
{
	ushort sum;
	int data, adr;

	if (hasfutextnvm(ctlr))
		return 0;	/* Eerd doesn't exist; it's now Fextnvm5 */

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

	for (n = 1000000; (f->reg[Fsts] & Fdone) == 0 && --n > 0; )
		pause();
	if (n == 0)
		panic("%æ: fread stuck", ctlr);
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

	if (hasfutextnvm(ctlr))
		return 0;	/* Eec doesn't exist; it's now Fextnvm6 */

	io = ctlr->pcidev->mem[1].bar & ~0x0f;
	f.reg = vmap(io, ctlr->pcidev->mem[1].size);
	if(f.reg == nil)
		return -1;
	f.reg32 = (void*)f.reg;
	f.base = f.reg32[Bfpr] & MASK(13);
	f.lim = (f.reg32[Bfpr]>>16) & MASK(13);
	if(ctlr->regs[Eec] & EEwdunits)
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

/*
 *  receiver
 */

static int
loadnvrammac(Ctlr *ctlr)
{
	int r;

	if (hasfutextnvm(ctlr))		/* no Eec, Eerd registers */
		return 0;

	switch (ctlr->type) {
	case i82566:
	case i82567:
	case i82577:
//	case i82578:			/* not yet implemented */
	case i82579:
		r = fload(ctlr);
		break;
	case i210:			/* includes i211 */
		if(ctlr->regs[Eec] & EEflsdet)
			r = fload(ctlr);
		else
			r = 0;		/* no flash, skip checksum */
		break;
	default:
		r = eeload(ctlr);
		break;
	}
	/*
	 * upper 4 bits of checksum short contain an `update nvm' bit that
	 * we can't readily set, so just compare the low 12 bits.
	 */
	if (r != 0 && (r & MASK(12)) != (0xBABA & MASK(12))){
		print("%æ: bad EEPROM checksum - %#.4ux\n", ctlr, r);
		return -1;
	}
	return r;
}

/*
 * set up flow control in case it's negotiated, even though we don't
 * want it and it is no longer recommended.
 * beware registers with changed meaning.
 */
static void
flowctl(Ctlr *ctlr)
{
	uint *regs;

	regs = ctlr->regs;
	regs[Fcrtl] = 8*KB;		/* rcv low water mark */
	regs[Fcrth] = 16*KB;  /* rcv high water mark: < rcv buf in PBA & RXA */
	regs[Fcttv] = 0x0100;		/* for XOFF frame */
	if (!hasfutextnvm(ctlr)) {
		/* old-style registers: have Fcal, Fcah & maybe Fct */
		/* fixed flow control ethernet address 0x0180c2000001 */
		regs[Fcal] = 0x00C28001;
		regs[Fcah] = 0x0100;
		if (ctlr->type != i82579 && ctlr->type != i210)
			/* flow control type, dictated by Intel */
			regs[Fct] = 0x8808;
	}
	coherence();
}

static void
setmacs(Ctlr *ctlr)
{
	int i;
	uint *regs;
	ulong eal, eah;
	uchar *p;

	regs = ctlr->regs;
	eal = regs[Ral];
	eah = regs[Rah];
	p = ctlr->ra;
	if (eal == 0 && eah == 0)		/* not yet set in hw? */
		/* load ctlr->ra from non-volatile memory, if present */
		if (loadnvrammac(ctlr) < 0)
			/* may have no flash, like the i210 */
			print("%æ: no mac address found\n", ctlr);
		else {
			/* set mac addr in ctlr & regs from eeprom */
			for(i = 0; i < Eaddrlen/2; i++){
				p[2*i]   = ctlr->eeprom[Ea+i];
				p[2*i+1] = ctlr->eeprom[Ea+i] >> 8;
			}
			/* ea ctlr[1] = ea ctlr[0]+1 */
			if (ctlr->type != i217)
				p[5] += (regs[Status] & Lanid) >> 2;
			/* set mac addr in regs from ctlr->ra */
			regs[Ral] = eal = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
			regs[Rah] = eah = p[5]<<8 | p[4] | Enable;
			/*
			 * zero other mac addresses.
			 * AV bits should be zeroed by master reset & there may
			 * only be 11 other registers on e.g., the i217.
			 */
			for(i = 1; i < 12; i++)	/* 12 used to be 16 here */
				regs[Ral+i*2] = regs[Rah+i*2] = 0;
		}
	p = leputl(p, eal);
	*p++ = eah;
	*p = eah >> 8;
}

static void
i82563promiscuous(void* arg, int on)
{
	int rctl;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	rctl = ctlr->regs[Rctl] & ~MoMASK;
	if(on)
		rctl |= Upe|Mpe;
	else
		rctl &= ~(Upe|Mpe);
	if (ctlr->mcastbits < 0)
		rctl |= Mpe;
	ctlr->regs[Rctl] = rctl;
	coherence();
}

/*
 * Returns the number of bits of mac address used in multicast hash,
 * thus the number of longs of ctlr->mta (2^(bits-5)).
 * This must be right for multicast (thus ipv6) to work reliably.
 *
 * The default multicast hash for mta is based on 12 bits of MAC address;
 * the rightmost bit is a function of Rctl's Multicast Offset: 0=>36,
 * 1=>35, 2=>34, 3=>32.  Exceptions include the 57[89] and 21[789] (in general,
 * newer ones); they use only 10 bits, ignoring the rightmost 2 of the 12.
 */
static int
mcastbits(Ctlr *ctlr)
{
	if (ctlr->mcastbits == 0)
		switch (ctlr->type) {
		/*
		 * openbsd says all ich8 versions (ich8, ich9, ich10, pch, pch2
		 * & pch_lpt) have 32 longs (use 10 bits of mac addr. for hash).
		 */
		case i82566:
		case i82567:
//		case i82578:
		case i82579:
		case i217:
		case i218:
//		case i219:
			ctlr->mcastbits = 10;		/* 32 longs */
			break;
		case i82563:
		case i82571:
		case i82572:
		case i82573:
		case i82574:
//		case i82575:
		case i82580:
//		case i82583:
		case i210:				/* includes i211 */
			ctlr->mcastbits = 12;		/* 128 longs */
			break;
		default:
			print("%æ: unsure of multicast bits in mac addresses; "
				"enabling promiscuous multicast reception\n",
				ctlr);
			ctlr->regs[Rctl] |= Mpe;
			coherence();
			ctlr->mcastbits = -1;
			break;
		}
	/* if unknown, be conservative (for mta size) */
	return ctlr->mcastbits < 0? 10: ctlr->mcastbits;
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
	word = (hash / BI2WD) & (tblsz - 1);
	bit = 1UL << (hash % BI2WD);
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
	ctlr->regs[Mta+word] = ctlr->mta[word];
	coherence();
}

static ulong
setrxdctl(Ctlr *ctlr)
{
	uint *regs;
	ulong r;

	regs = ctlr->regs;
	r = regs[Rxdctl] & ~(Gran|WthreshMASK|PthreshMASK|HthreshMASK);
	switch (ctlr->type) {
	case i82571:
		/* the 571 is fussy and this works, so just leave it alone. */
		break;
	case i82575:				/* multi-queue controllers */
	case i82576:
	case i82580:
	case i210:
		/* use known good values; don't frighten the horses. */
		r = regs[Rxdctl] & ~(WthreshMASK|PthreshMASK);
		r |= Qenable | 2<<WthreshSHIFT | 2<<PthreshSHIFT;
		break;
	case i217:				/* nothing works */
//		r = 0x10000;	/* works for pxe boot rom; manual suggests 0 */
		/* fall through */
	default:
		r = Gran | 1<<WthreshSHIFT | 1 <<PthreshSHIFT | 1<<HthreshSHIFT;
		break;
	}
	regs[Rxdctl] = r;
	return r;
}

static void
i82563rxinit(Ctlr* ctlr)
{
	Block *bp;
	int i, r, rctl, type, multiq;
	uint *regs;
	uvlong phys;		/* instead of uintptr, for 9 compat */

	regs = ctlr->regs;
	type = ctlr->type;
	rctl = Dpf | Bsize2048 | Bam | RdtmsHALF;
	multiq = type == i82575 || type == i82576 || type == i82580 ||
		type == i210;
	if(multiq){
		/*
		 * Setting Qenable in Rxdctl does not stick unless Ren is on.
		 */
		regs[Rctl] = Ren | rctl;
		coherence();
		regs[Rxdctl] |= Qenable;
		coherence();
	}
	regs[Rctl] = rctl;	/* disable rcvr before we reconfigure it */
	coherence();
	delay(1);

	switch (type) {
	case i82573:
	case i82577:
	case i82579:
	case i210:
	case i218:
		regs[Ert] = 1024/8;		/* early rx threshold */
		break;
	case i217:
		regs[Ert] = 1<<13 | (1024/8);	/* early rx threshold */
		/* use only receive queue 0 */
		regs[Rdbah1] = regs[Rdbal1] = regs[Rdlen1] = 0;
		regs[Rxdctl1] = 0;
		coherence();
		break;
	}

	phys = PCIWADDR(ctlr->rdba);
	regs[Rdbal] = phys;
	regs[Rdbah] = phys>>32;
	regs[Rdlen] = Nrd * sizeof(Rd);
	/* Rdh and Rdt are supposed to have been zeroed by reset */
	ctlr->rdh = ctlr->rdt = 0;

	/* to hell with interrupt moderation/throttling, we want low latency */
	regs[Radv] = 0;
	regs[Rdtr] = Fpd;			/* Fpd for 217 */

	for(i = 0; i < Nrd; i++)
		if((bp = ctlr->rb[i]) != nil){
			ctlr->rb[i] = nil;
			freeb(bp);
		}
	i82563replenish(ctlr);
	r = setrxdctl(ctlr);

	/*
	 * Don't enable checksum offload.  In practice, it interferes with
	 * bootp/tftp booting on at least the 82575, and Intel have admitted to
	 * checksum bugs on various of their gbe controllers (see e.g., errata
	 * 8257[12] #39), so do them in software instead.
	 */
	regs[Rxcsum] = 0;
	regwrbarr(ctlr);
	regs[Rctl] |= Ren;
	regwrbarr(ctlr);
	if(multiq) {
		regs[Rxdctl] = r;
		coherence();
	}
}

/*
 * Accept eop packets with no errors.
 * Returns number of packets passed upstream.
 */
static int
enqinpkt(Ether *edev, Rd *rd, Block *bp)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if((rd->status & Reop) == 0 || rd->errors) {
		if(rd->status & Reop)
			print("%æ: input packet error %#ux\n", ctlr, rd->errors);
		ainc(&nrbfull);			/* rbfree will adec */
		freeb(bp);			/* toss bad pkt */
		return 0;
	}
	if (bp == nil)
		panic("%æ: nil Block* from ctlr->rb", ctlr);
	bp->wp += rd->length;
	ainc(&nrbfull);
	notemark(&ctlr->wmrb, nrbfull);
	etheriq(edev, bp, 1);		/* pass pkt upstream */
	return 1;
}

enum {
	Rcvbaseintrs = Rxo | Rxdmt0 | Rxseq | Ack,
	Rcvallintrs = Rxt0 | Rcvbaseintrs,
};

static int
i82563rim(void* vc)
{
	Ctlr *ctlr = (void *)vc;

	return ctlr->rim != 0 || SWOWNS(&ctlr->rdba[ctlr->rdh], Rdd);
}

static void
i82563rproc(void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	Rd *rd;
	uint rdh, rim, passed;
	ulong rxt0;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		i82563replenish(ctlr);
		/*
		 * Prevent an idle or unplugged interface from interrupting.
		 * Allow receiver timeout interrupts initially and again
		 * if the interface (and transmitter) see actual use.
		 */
		rxt0 = 0;
		if (ctlr->rintr < 2*Nrd || edev->outpackets > 10)
			rxt0 = Rxt0;
		enabintrcauses(ctlr, Rcvbaseintrs | rxt0 |Parityeccrst);
		ctlr->rsleep++;
		sleep(&ctlr->rrendez, i82563rim, ctlr);

		rdh = ctlr->rdh;  /* index of next filled buffer, if done */
		passed = 0;
		for(;;){
			rim = ctlr->rim;
			ctlr->rim = 0;
			rd = &ctlr->rdba[rdh];
			/* Rdd indicates a reusable rd; sw owns it */
			if(!(rd->status & Rdd))
				break;		/* wait for pkts to arrive */

			passed += enqinpkt(edev, rd, ctlr->rb[rdh]);
			/* Block has been passed upstream or freed */
			ctlr->rb[rdh] = nil;

			/* rd needs to be replenished to accept another pkt */
			rd->status = 0;
			ctlr->rdfree--;
			ctlr->rdh = rdh = NEXT(rdh, Nrd);
			/*
			 * if number of rds ready for packets is too low (more
			 * than 32 in use), set up the unready ones.
			 */
			if(ctlr->rdfree <= Nrd - 32 || (rim & Rxdmt0))
				i82563replenish(ctlr);
		}
		/* note how many rds had full buffers on this pass */
		notemark(&ctlr->wmrd, passed);
	}
}

/*
 *  phy, kumeran, etc. rubbish
 */

static int
phynum(Ctlr *ctlr, int reg)
{
	if (ctlr->phyaddgen < 0)
		switch (ctlr->type) {
		case i82577:
//		case i82578:			/* not yet implemented */
		case i82579:
		case i217:
		case i218:
			ctlr->phyaddspec = 2;	/* pcie phy */
			ctlr->phyaddgen = 1;
			break;
		default:
			ctlr->phyaddspec = 1;	/* gbe phy */
			ctlr->phyaddgen = 1;
			break;
		}
	if (reg == Phypage217) /* bug: renders diag sts @ 2,0,31 inaccessible */
		return ctlr->phyaddgen;
	else if (ctlr->phypage == 0 || reg <= Phyfixedend)
		/* bug: makes 1,0,* inaccessible */
		return ctlr->phyaddspec;
	else
		return ctlr->phyaddgen;
}

static uint
phyread(Ctlr *ctlr, int reg)
{
	uint phy, i;

	if (reg > Phyregend) {
		iprint("phyread: reg %d > %d\n", reg, Phyregend);
		return ~0;
	}
	takeswflag(ctlr);
	ctlr->regs[Mdic] = MDIrop | phynum(ctlr, reg)<<MDIpSHIFT |
		reg<<MDIrSHIFT;
	relswflag(ctlr);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = ctlr->regs[Mdic];
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

	if (reg > Phyregend) {
		iprint("phywrite: reg %d > %d\n", reg, Phyregend);
		return ~0;
	}
	takeswflag(ctlr);
	ctlr->regs[Mdic] = MDIwop | phynum(ctlr, reg)<<MDIpSHIFT |
		reg<<MDIrSHIFT | val;
	relswflag(ctlr);
	phy = 0;
	for(i = 0; i < 64; i++){
		phy = ctlr->regs[Mdic];
		if(phy & (MDIe|MDIready))
			break;
		microdelay(1);
	}
	if((phy & (MDIe|MDIready)) != MDIready)
		return ~0;
	return 0;
}

/* only works for phy addr 1 and only on i21[78] */
static uint
phypage217(Ctlr *ctlr, int page)
{
	if (ctlr->type != i217 && ctlr->type != i218)
		return ~0;
	if ((page<<5) > MASK(16)) {
		iprint("phypage217: page %d too big\n", page);
		return ~0;
	}
	ctlr->phypage = page;
	return phywrite(ctlr, Phypage217, page<<5);
}

static void
kmrnwrite(Ctlr *ctlr, ulong reg_addr, ushort data)
{
	ctlr->regs[Kumctrlsta] = ((reg_addr << Kumctrlstaoffshift) &
		Kumctrlstaoff) | data;
	coherence();
	microdelay(2);
}

static ulong
kmrnread(Ctlr *ctlr, ulong reg_addr)
{
	ctlr->regs[Kumctrlsta] = ((reg_addr << Kumctrlstaoffshift) &
		Kumctrlstaoff) | Kumctrlstaren;  /* write register address */
	coherence();
	microdelay(2);
	return ctlr->regs[Kumctrlsta];		/* read data */
}

/*
 * work around DMA unit hang on i21[78]
 *
 * this is essentially black magic.  we blindly follow the incantations
 * prescribed by the god Intel:
 *
 * On ESB2, the MAC-to-PHY (Kumeran) interface must be configured after
 * link is up before any traffic is sent.
 *
 * At 1Gbps link speed, one of the MAC's internal clocks can be stopped
 * for up to 4us when entering K1 (a power mode of the MAC-PHY
 * interconnect).  If the MAC is waiting for completion indications for 2
 * DMA write requests into Host memory (e.g., descriptor writeback or Rx
 * packet writing) and the indications occur while the clock is stopped,
 * both indications will be missed by the MAC, causing the MAC to wait
 * for the completion indications and be unable to generate further DMA
 * write requests.  This results in an apparent hardware hang.
 *
 * Work around the bug by disabling the de-assertion of the clock request
 * when 1Gbps link is acquired (K1 must be disabled while doing this).
 * Also, set appropriate Tx re-transmission timeouts for 10 and 100-half
 * link speeds to avoid Tx hangs.
 */
static void
k1fix(Ctlr *ctlr)
{
	int txtmout;			/* units of 10µs */
	uint *regs;
	ulong fextnvm6, status;
	ushort reg;
	Ether *edev;

	edev = ctlr->edev;
	regs = ctlr->regs;
	takeswflag(ctlr);
	regs[Ctrl] |= Fd | Frcdplx;
	relswflag(ctlr);
	fextnvm6 = regs[Fextnvm6];
	status = regs[Status];
	/* status speed bits are different on 21[78] than earlier ctlrs */
	if (edev->link && status & (Sspeed1000>>2)) {
		reg = kmrnread(ctlr, Kumctrlstak1cfg);
		kmrnwrite(ctlr, Kumctrlstak1cfg, reg & ~Kumctrlstak1enable);
		microdelay(10);

		regs[Fextnvm6] = fextnvm6 | Fextnvm6reqpllclk;
		coherence();

		kmrnwrite(ctlr, Kumctrlstak1cfg, reg);
		unstall217(ctlr);
		ctlr->didk1fix = 1;
		return;
	}
	/* else uncommon cases */

	fextnvm6 &= ~Fextnvm6reqpllclk;
	/*
	 * 218 manual omits the non-phy registers.
	 */
	if (!edev->link ||
	    (status & (Sspeed100>>2|Frcdplx)) == (Sspeed100>>2|Frcdplx)) {
		regs[Fextnvm6] = fextnvm6;
		coherence();
		ctlr->didk1fix = 1;
		return;
	}

	/* access other page via phy addr 1 reg 31, then access reg 16-30 */
	phypage217(ctlr, I217icpage);
	reg = phyread(ctlr, I217icreg) & ~I217iclstmoutmask;
	if (status & (Sspeed100>>2)) {		/* 100Mb/s half-duplex? */
		txtmout = 5;
		fextnvm6 &= ~Fextnvm6enak1entrycond;
	} else {				/* 10Mb/s */
		txtmout = 50;
		fextnvm6 |= Fextnvm6enak1entrycond;
	}
	phywrite(ctlr, I217icreg, reg | txtmout << I217iclstmoutshift);
	regs[Fextnvm6] = fextnvm6;
	coherence();
	phypage217(ctlr, 0);			/* reset page to usual 0 */
	takeswflag(ctlr);
	regs[Ctrl] |= Fd | Frcdplx;
	relswflag(ctlr);
	ctlr->didk1fix = 1;
}

/* return auto-neg fault bits, with speed encoding stored into *spp */
static int
getspdanfault(Ctlr *ctlr, int phy, int phy79, uint *spp)
{
	uint a, sp;

	if (phy79) {
		*spp = (phy>>8) & 3;
		return phy & Anfs;
	}

	sp = (phy>>14) & 3;
	switch(ctlr->type){
	case i82563:
	case i82574:
	case i210:
		a = phyread(ctlr, Phyisr) & Ane;	/* a-n error */
		break;
	case i82571:
	case i82572:
	case i82575:
	case i82576:
	case i82580:
		a = phyread(ctlr, Phylhr) & Anf;	/* a-n fault */
		sp = (sp-1) & 3;
		break;
	default:
		a = 0;
		print("%æ: getspdanfault: unknown ctlr type %d\n", ctlr,
			ctlr->type);
		break;
	}
	*spp = sp;
	return a;
}

static int
i82563lim(void* ctlr)
{
	return ((Ctlr*)ctlr)->lim != 0;
}

/*
 * watch for changes of link state
 * txinit must run first
 */
static void
i82563lproc(void *v)
{
	uint phy, sp, phy79, prevlink;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	phy79 = 0;
	switch (ctlr->type) {
	case i82579:
	case i82580:
	case i217:
	case i218:
//	case i219:
//	case i350:
//	case i354:
		phy79 = 1;		/* new-style phy */
		break;
	}
	if(ctlr->type == i82573 && (phy = phyread(ctlr, Phyier)) != ~0)
		phywrite(ctlr, Phyier, phy | Lscie | Ancie | Spdie | Panie);
	else if(phy79 && (phy = phyread(ctlr, Phyier218)) != ~0)
		phywrite(ctlr, Phyier218, phy | Lscie218 | Ancie218 | Spdie218);
	prevlink = 0;
	for(;;){
		phy = phyread(ctlr, phy79? Phystat: Physsr);
		if(phy == ~0)
			goto next;
		if (getspdanfault(ctlr, phy, phy79, &sp) != 0) {
			phywrite(ctlr, Phyctl, phyread(ctlr, Phyctl) |
				Ran | Ean);	/* enable & restart autoneg */
			delay(1000);
			continue;
		}

		/* link state is stable */
		edev->link = phy & (phy79? Link: Rtlink) ||
			ctlr->regs[Status] & Lu;
		if(edev->link){
			if (sp < 1)		/* implausible nowadays */
				sp = 1;
			ctlr->speeds[sp]++;
			if (speedtab[sp])
				edev->mbps = speedtab[sp];
			if (prevlink == 0) {
				/* link newly up: kludge away */
				if (needsk1fix(ctlr))
					k1fix(ctlr);
				qlock(&ctlr->tlock);
				ctlr->regs[Tctl] |= Ten;
				regwrbarr(ctlr);
				/* notify ctlr of q'd pkts */
				ctlr->regs[Tdt] = ctlr->tdt;
				qunlock(&ctlr->tlock);
				regwrbarr(ctlr);
				wakeup(&ctlr->trendez);
			}
		} else
			ctlr->didk1fix = 0;	/* force fix at next link up */
		prevlink = edev->link;
next:
		ctlr->lim = 0;
		enabintrcauses(ctlr, Lsc|Parityeccrst|Intassert);
		ctlr->lsleep++;
		sleep(&ctlr->lrendez, i82563lim, ctlr);
	}
}

/*
 *  attachment, allocation
 */

static void
freerbs(Ctlr *)
{
	int i;
	Block *bp;

	for(i = Nrb; i > 0; i--){
		bp = i82563rballoc();
		if (bp == nil)
			break;
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
	uint *regs;
	Block *bp;
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);

	if(ctlr->attached){
		qunlock(&ctlr->alock);
		return;
	}

	ctlr->regs[Imc] = ~0;
	regwrbarr(ctlr);

	if(waserror()){
		freemem(ctlr);
		qunlock(&ctlr->alock);
		nexterror();
	}

	ctlr->edev = edev;			/* point back to Ether */
	/* small allocations */
	ctlr->rdba = mallocalign(Nrd * sizeof(Rd), Descalign, 0, 0);
	ctlr->tdba = mallocalign(Ntd * sizeof(Td), Descalign, 0, 0);
	if(ctlr->rdba == nil || ctlr->tdba == nil ||
	   (ctlr->rb = malloc(Nrd*sizeof(Block*))) == nil ||
	   (ctlr->tb = malloc(Ntd*sizeof(Block*))) == nil)
		error(Enomem);

	/* bigger allocations */
	for(i = 0; i < Nrb; i++){
		if((bp = allocb(Rbsz)) == nil)
			error(Enomem);
		if (BALLOC(bp) < Rbsz)
			panic("%æ: allocb broken", ctlr);
		bp->free = i82563rbfree;
		freeb(bp);
	}
	aadd(&nrbfull, Nrb);		/* compensate for adecs in rbfree */

	initmark(&ctlr->wmrb, Nrb,   "rcv Blocks not yet processed");
	initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
	initmark(&ctlr->wmtd, Maxtd, "xmit descriptor queue");

	regs = ctlr->regs;
	switch (ctlr->type) {
	case i217:
		coddle217(ctlr);
		break;
	default:
		regs[Itr] = 0;		/* no interrupt throttling */
		break;
	}
	takeswflag(ctlr);
	regs[Ctrl] |= Fd | Frcdplx;
	relswflag(ctlr);

	ctlr->attached = 1;

	/*
	 * perform complete initialisation here, not in the kprocs, to simplify
	 * link-state handling.
	 */
	i82563txinit(ctlr);
	i82563rxinit(ctlr);

	snprint(name, sizeof name, "#l%dlink", edev->ctlrno);
	kproc(name, i82563lproc, edev);

	snprint(name, sizeof name, "#l%dxmit", edev->ctlrno);
	kproc(name, i82563tproc, edev);

	snprint(name, sizeof name, "#l%drcv", edev->ctlrno);
	kproc(name, i82563rproc, edev);

	qunlock(&ctlr->alock);
	poperror();
}

/*
 *  interrupt service
 */

static Intrsvcret
i82563interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	ulong icr, im;
	uint *regs;

	edev = arg;
	ctlr = edev->ctlr;
	if (ctlr == nil)
		panic("igbe: interrupt: nil ctlr");
	regs = ctlr->regs;
	if (regs == nil)
		panic("%æ: interrupt: nil regs", ctlr);
	ilock(&ctlr->imlock);
	/*
	 * some models (e.g., 57[12]) need Imc written before reading Icr,
	 * else they may lose an interrupt.
	 */
	regs[Imc] = ~0;
	coherence();
	icr = regs[Icr];
	im = ctlr->im;			/* intr causes we care about */
	if ((icr & im) == 0) {
		regs[Ims] = im;
		iunlock(&ctlr->imlock);
		return Intrnotforme;
	}

	if(icr & Rcvallintrs){
		ctlr->rim = icr & Rcvallintrs;
		wakeup(&ctlr->rrendez);
		im &= ~Rcvallintrs;
		ctlr->rintr++;
	}
	if(icr & (Txdw|Txqe|Txdlow)){
		ctlr->tim = icr & (Txdw|Txqe|Txdlow);
		wakeup(&ctlr->trendez);
		im &= ~(Txdw|Txqe|Txdlow);
		ctlr->tintr++;
	}
	if(icr & Lsc){
		ctlr->lim = Lsc;
		wakeup(&ctlr->lrendez);
		im &= ~Lsc;
		ctlr->lintr++;
	}
	im = eccerr217(ctlr, icr, im);
	regs[Ims] = ctlr->im = im;
	iunlock(&ctlr->imlock);
	return Intrforme;
}

/* assume misrouted interrupts and check all controllers */
static Intrsvcret
i82575interrupt(Ureg*, void *)
{
	int forme;
	Ctlr *ctlr;

	forme = 0;
	for (ctlr = i82563ctlrhead; ctlr != nil && ctlr->edev != nil;
	     ctlr = ctlr->next)
		forme |= (i82563interrupt(nil, ctlr->edev)
#ifdef PLAN9K
			, 0
#endif
			);
#ifndef PLAN9K
	return forme;
#endif
}

/*
 *  resets, detachment
 */

/* wait up to a second or two for bit in ctlr->regs[reg] to become zero */
static int
awaitregbitzero(Ctlr *ctlr, int reg, ulong bit)
{
	return awaitbitpat(&ctlr->regs[reg], bit, 0);
}

static int
ctlrreset(Ctlr *ctlr)
{
	uint rsts;

	rsts = 0;
	/*
	 * phy reset is utterly broken on the i217 & requires a power cycle
	 * to make a 217 usable again if you try it.  the i219 is reported
	 * to be worse with its device reset.  use undocumented magic to
	 * try to avoid hangs on the i219 some day.
	 */
	if (0 /* ctlr->type == i219 */)
		ctlr->regs[Fextnvm11] |= F11norsthang;	/* avoid reset hang */
	if(ctlr->type == i82566 || ctlr->type == i82567 || ctlr->type == i82579)
		rsts |= Phyrst;
	takeswflag(ctlr);
	ctlr->regs[Ctrl] |= Devrst | rsts;
	coherence();
	delay(20);			/* wait for eeprom reload */
	/* Swflag will be cleared by reset */
	if (awaitbitpat(&ctlr->regs[Ctrl], rsts, 0) < 0) {
		print("%æ: Ctlr(%#ux).Devrst(%#ux) never went to 0\n",
			ctlr, ctlr->regs[Ctrl], rsts);
		return -1;
	}
	if (0 /* ctlr->type == i219 */)
		ctlr->regs[Fextnvm11] |= F11norsthang;	/* avoid reset hang */
	return 0;
}

static void
disablerxtx(Ctlr *ctlr)
{
	uint *regs = ctlr->regs;

	regs[Imc] = ~0;
	regwrbarr(ctlr);
	regs[Rctl] = regs[Tctl] = 0;
	regwrbarr(ctlr);
	delay(10);
}

static int
eerst(Ctlr *ctlr)
{
	takeswflag(ctlr);
	ctlr->regs[Ctrlext] |= Eerst;
	relswflag(ctlr);
	/*
	 * on parallels 13, 82574's Ctrlext ends up with 0x2200;
	 * Eerst is 0x2000.
	 */
	if (awaitregbitzero(ctlr, Ctrlext, Eerst) < 0) {
		vmbotch(Parallels, "your actual igbepcie's Ctrlext's "
			"Eerst bit is stuck on");
//		if (!conf.vm)
//			return -1;
	}
	return 0;
}

/*
 * Perform a device reset to get the chip back to the
 * power-on state, followed by an EEPROM reset to read
 * the defaults for some internal registers.
 */
static int
i82563detach0(Ctlr* ctlr)
{
	uint *regs;

	regs = ctlr->regs;
	if (regs == nil)
		return -1;
	takeswflag(ctlr);
	disablerxtx(ctlr);

	/* reset controller: device, eerom, interrupts */
	if (ctlrreset(ctlr) < 0)
		return -1;

	/* the whole controller should be reset now */
	disablerxtx(ctlr);

	if (ctlr->type != i217 && eerst(ctlr) < 0)
		return -1;

	regs[Imc] = ~0;
	coherence();
	if (awaitregbitzero(ctlr, Icr, ~0) < 0) {
		print("%æ: Icr (%#ux) never went to 0\n", ctlr, regs[Icr]);
		return -1;
	}

	/* resets have finished; new packet buffer allocations are in effect */
	takeswflag(ctlr);
	regs[Ctrl] |= Slu;
	relswflag(ctlr);

	if (ctlr->type == i217) {
		/* manual says wait for Laninitdone to set; doesn't happen */
		delay(20);
		regs[Imc] = ~0;
		regs[Phyctrl] &= ~Gbedis;
		regwrbarr(ctlr);
	}
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
	if (ether && ether->ctlr)
		i82563detach(ether->ctlr);
}

/*
 *  discovery
 */

/* may be called from i82563pci with ctlr only partially populated */
static int
i82563reset(Ctlr *ctlr)
{
	int i;
	uint *regs;

	if(i82563detach(ctlr)) {
		iprint("%æ reset: detach failed\n", ctlr);
		return -1;
	}

	regs = ctlr->regs;
	/* if unknown, load mac address from non-volatile memory, if present */
	setmacs(ctlr);

	/* populate multicast table */
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
	for(i = 0; i < mcbitstolongs(mcastbits(ctlr)); i++)
		regs[Mta + i] = ctlr->mta[i];

	flowctl(ctlr);
	return 0;
}

/*
 * map device p and populate a new Ctlr for it.
 * add the new Ctlr to our list.
 */
static Ctlr *
newctlr(Pcidev *p, int type)
{
	ulong io;
	void *mem;
	Ctlr *ctlr;

	io = p->mem[0].bar & ~0x0F;		/* assume 32-bit bar */
	mem = vmap(io, p->mem[0].size);
	if(mem == nil){
		print("%s: can't map %.8lux\n", tname[type], io);
		return nil;
	}

	pcisetpms(p, 0);
	/* pci-e ignores PciCLS */

	ctlr = malloc(sizeof(Ctlr));
	if(ctlr == nil) {
		vunmap(mem, p->mem[0].size);
		error(Enomem);
	}
	ctlr->regs = mem;
	ctlr->port = io;
	ctlr->pcidev = p;
	ctlr->type = type;
	ctlr->prtype = tname[type];
	ctlr->phyaddgen = ctlr->phyaddspec = -1;	/* not yet known */

	if(i82563reset(ctlr)){
		vunmap(mem, p->mem[0].size);
		free(ctlr);
		return nil;
	}
	pcisetbme(p);

	if(i82563ctlrhead != nil)
		i82563ctlrtail->next = ctlr;
	else
		i82563ctlrhead = ctlr;
	i82563ctlrtail = ctlr;
	return ctlr;
}

static void
i82563pci(void)
{
	int type;
	Pcidev *p;

	fmtinstall(L'æ', etherfmt);
	p = nil;
	while(p = pcimatch(p, Vintel, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;
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
		case 0x150e:		/* 82580EB/DB */
			type = i82580;
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
#ifdef ALLOW_217
			print("igbe: using broken intel i217-lm\n");
#else
			print("igbe: skipping broken intel i217-lm\n");
			continue;
#endif
		case 0x153b:		/* i217-v */
			type = i217;	/* integrated w/ intel 8/c220 chipset */
			break;
		case 0x15a3:		/* i218 */
			type = i218;
			break;
		}
		newctlr(p, type);
	}
}

static int
pnp(Ether* edev, int type)
{
	Ctlr *ctlr;
	static int done;
	static QLock pnplock;

	qlock(&pnplock);
	if(!done) {
		done = 1;
		i82563pci();
	}
	qunlock(&pnplock);

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
	ctlr->edev = edev;			/* point back to Ether */
	edev->port = ctlr->port;
	edev->pcidev = ctlr->pcidev;
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
	edev->ctl = nil;

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

static int i82563pnp(Ether *e) { return pnp(e, i82563); }
static int i82566pnp(Ether *e) { return pnp(e, i82566); }
static int i82571pnp(Ether *e) { return pnp(e, i82571); }
static int i82572pnp(Ether *e) { return pnp(e, i82572); }
static int i82573pnp(Ether *e) { return pnp(e, i82573); }
static int i82574pnp(Ether *e) { return pnp(e, i82574); }
static int i82575pnp(Ether *e) { return pnp(e, i82575); }
static int i82579pnp(Ether *e) { return pnp(e, i82579); }
static int i82580pnp(Ether *e) { return pnp(e, i82580); }
static int i210pnp(Ether *e) { return pnp(e, i210); }
static int i217pnp(Ether *e) { return pnp(e, i217); }
static int i218pnp(Ether *e) { return pnp(e, i218); }

void
etherigbelink(void)
{
	/* recognise lots of model numbers to aid multi-homed configurations */
	addethercard("i82563", i82563pnp);
	addethercard("i82566", i82566pnp);
	addethercard("i82571", i82571pnp);
	addethercard("i82572", i82572pnp);
	addethercard("i82573", i82573pnp);
	addethercard("i82574", i82574pnp);
	addethercard("i82575", i82575pnp);
	addethercard("i82579", i82579pnp);
	addethercard("i82580", i82580pnp);
	addethercard("i210", i210pnp);
	addethercard("i217", i217pnp);
	addethercard("i218", i218pnp);

	addethercard("igbepcie", anypnp);
	addethercard("igbe", anypnp);
}
