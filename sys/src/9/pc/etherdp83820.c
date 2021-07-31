/*
 * National Semiconductor DP83820
 * 10/100/1000 Mb/s Ethernet Network Interface Controller
 * (Gig-NIC).
 * Driver assumes little-endian and 32-bit host throughout.
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

enum {					/* Registers */
	Cr		= 0x00,		/* Command */
	Cfg		= 0x04,		/* Configuration and Media Status */
	Mear		= 0x08,		/* MII/EEPROM Access */
	Ptscr		= 0x0C,		/* PCI Test Control */
	Isr		= 0x10,		/* Interrupt Status */
	Imr		= 0x14,		/* Interrupt Mask */
	Ier		= 0x18,		/* Interrupt Enable */
	Ihr		= 0x1C,		/* Interrupt Holdoff */
	Txdp		= 0x20,		/* Transmit Descriptor Pointer */
	Txdphi		= 0x24,		/* Transmit Descriptor Pointer Hi */
	Txcfg		= 0x28,		/* Transmit Configuration */
	Gpior		= 0x2C,		/* General Purpose I/O Control */
	Rxdp		= 0x30,		/* Receive Descriptor Pointer */
	Rxdphi		= 0x34,		/* Receive Descriptor Pointer Hi */
	Rxcfg		= 0x38,		/* Receive Configuration */
	Pqcr		= 0x3C,		/* Priority Queueing Control */
	Wcsr		= 0x40,		/* Wake on LAN Control/Status */
	Pcr		= 0x44,		/* Pause Control/Status */
	Rfcr		= 0x48,		/* Receive Filter/Match Control */
	Rfdr		= 0x4C,		/* Receive Filter/Match Data */
	Brar		= 0x50,		/* Boot ROM Address */
	Brdr		= 0x54,		/* Boot ROM Data */
	Srr		= 0x58,		/* Silicon Revision */
	Mibc		= 0x5C,		/* MIB Control */
	Mibd		= 0x60,		/* MIB Data */
	Txdp1		= 0xA0,		/* Txdp Priority 1 */
	Txdp2		= 0xA4,		/* Txdp Priority 2 */
	Txdp3		= 0xA8,		/* Txdp Priority 3 */
	Rxdp1		= 0xB0,		/* Rxdp Priority 1 */
	Rxdp2		= 0xB4,		/* Rxdp Priority 2 */
	Rxdp3		= 0xB8,		/* Rxdp Priority 3 */
	Vrcr		= 0xBC,		/* VLAN/IP Receive Control */
	Vtcr		= 0xC0,		/* VLAN/IP Transmit Control */
	Vdr		= 0xC4,		/* VLAN Data */
	Ccsr		= 0xCC,		/* Clockrun Control/Status */
	Tbicr		= 0xE0,		/* TBI Control */
	Tbisr		= 0xE4,		/* TBI Status */
	Tanar		= 0xE8,		/* TBI ANAR */
	Tanlpar		= 0xEC,		/* TBI ANLPAR */
	Taner		= 0xF0,		/* TBI ANER */
	Tesr		= 0xF4,		/* TBI ESR */
};

enum {					/* Cr */
	Txe		= 0x00000001,	/* Transmit Enable */
	Txd		= 0x00000002,	/* Transmit Disable */
	Rxe		= 0x00000004,	/* Receiver Enable */
	Rxd		= 0x00000008,	/* Receiver Disable */
	Txr		= 0x00000010,	/* Transmitter Reset */
	Rxr		= 0x00000020,	/* Receiver Reset */
	Swien		= 0x00000080,	/* Software Interrupt Enable */
	Rst		= 0x00000100,	/* Reset */
	TxpriSHFT	= 9,		/* Tx Priority Queue Select */
	TxpriMASK	= 0x00001E00,
	RxpriSHFT	= 13,		/* Rx Priority Queue Select */
	RxpriMASK	= 0x0001E000,
};

enum {					/* Configuration and Media Status */
	Bem		= 0x00000001,	/* Big Endian Mode */
	Ext125		= 0x00000002,	/* External 125MHz reference Select */
	Bromdis		= 0x00000004,	/* Disable Boot ROM interface */
	Pesel		= 0x00000008,	/* Parity Error Detection Action */
	Exd		= 0x00000010,	/* Excessive Deferral Abort */
	Pow		= 0x00000020,	/* Program Out of Window Timer */
	Sb		= 0x00000040,	/* Single Back-off */
	Reqalg		= 0x00000080,	/* PCI Bus Request Algorithm */
	Extstsen	= 0x00000100,	/* Extended Status Enable */
	Phydis		= 0x00000200,	/* Disable PHY */
	Phyrst		= 0x00000400,	/* Reset PHY */
	M64addren	= 0x00000800,	/* Master 64-bit Addressing Enable */
	Data64en	= 0x00001000,	/* 64-bit Data Enable */
	Pci64det	= 0x00002000,	/* PCI 64-bit Bus Detected */
	T64addren	= 0x00004000,	/* Target 64-bit Addressing Enable */
	Mwidis		= 0x00008000,	/* MWI Disable */
	Mrmdis		= 0x00010000,	/* MRM Disable */
	Tmrtest		= 0x00020000,	/* Timer Test Mode */
	Spdstsien	= 0x00040000,	/* PHY Spdsts Interrupt Enable */
	Lnkstsien	= 0x00080000,	/* PHY Lnksts Interrupt Enable */
	Dupstsien	= 0x00100000,	/* PHY Dupsts Interrupt Enable */
	Mode1000	= 0x00400000,	/* 1000Mb/s Mode Control */
	Tbien		= 0x01000000,	/* Ten-Bit Interface Enable */
	Dupsts		= 0x10000000,	/* Full Duplex Status */
	Spdsts100	= 0x20000000,	/* SPEED100 Input Pin Status */
	Spdsts1000	= 0x40000000,	/* SPEED1000 Input Pin Status */
	Lnksts		= 0x80000000,	/* Link Status */
};

enum {					/* MII/EEPROM Access */
	Eedi		= 0x00000001,	/* EEPROM Data In */
	Eedo		= 0x00000002,	/* EEPROM Data Out */
	Eeclk		= 0x00000004,	/* EEPROM Serial Clock */
	Eesel		= 0x00000008,	/* EEPROM Chip Select */
	Mdio		= 0x00000010,	/* MII Management Data */
	Mddir		= 0x00000020,	/* MII Management Direction */
	Mdc		= 0x00000040,	/* MII Management Clock */
};

enum {					/* Interrupts */
	Rxok		= 0x00000001,	/* Rx OK */
	Rxdesc		= 0x00000002,	/* Rx Descriptor */
	Rxerr		= 0x00000004,	/* Rx Packet Error */
	Rxearly		= 0x00000008,	/* Rx Early Threshold */
	Rxidle		= 0x00000010,	/* Rx Idle */
	Rxorn		= 0x00000020,	/* Rx Overrun */
	Txok		= 0x00000040,	/* Tx Packet OK */
	Txdesc		= 0x00000080,	/* Tx Descriptor */
	Txerr		= 0x00000100,	/* Tx Packet Error */
	Txidle		= 0x00000200,	/* Tx Idle */
	Txurn		= 0x00000400,	/* Tx Underrun */
	Mib		= 0x00000800,	/* MIB Service */
	Swi		= 0x00001000,	/* Software Interrupt */
	Pme		= 0x00002000,	/* Power Management Event */
	Phy		= 0x00004000,	/* PHY Interrupt */
	Hibint		= 0x00008000,	/* High Bits Interrupt Set */
	Rxsovr		= 0x00010000,	/* Rx Status FIFO Overrun */
	Rtabt		= 0x00020000,	/* Received Target Abort */
	Rmabt		= 0x00040000,	/* Received Master Abort */
	Sserr		= 0x00080000,	/* Signalled System Error */
	Dperr		= 0x00100000,	/* Detected Parity Error */
	Rxrcmp		= 0x00200000,	/* Receive Reset Complete */
	Txrcmp		= 0x00400000,	/* Transmit Reset Complete */
	Rxdesc0		= 0x00800000,	/* Rx Descriptor for Priority Queue 0 */
	Rxdesc1		= 0x01000000,	/* Rx Descriptor for Priority Queue 1 */
	Rxdesc2		= 0x02000000,	/* Rx Descriptor for Priority Queue 2 */
	Rxdesc3		= 0x04000000,	/* Rx Descriptor for Priority Queue 3 */
	Txdesc0		= 0x08000000,	/* Tx Descriptor for Priority Queue 0 */
	Txdesc1		= 0x10000000,	/* Tx Descriptor for Priority Queue 1 */
	Txdesc2		= 0x20000000,	/* Tx Descriptor for Priority Queue 2 */
	Txdesc3		= 0x40000000,	/* Tx Descriptor for Priority Queue 3 */
};

enum {					/* Interrupt Enable */
	Ien		= 0x00000001,	/* Interrupt Enable */
};

enum {					/* Interrupt Holdoff */
	IhSHFT		= 0,		/* Interrupt Holdoff */
	IhMASK		= 0x000000FF,
	Ihctl		= 0x00000100,	/* Interrupt Holdoff Control */
};

enum {					/* Transmit Configuration */
	TxdrthSHFT	= 0,		/* Tx Drain Threshold */
	TxdrthMASK	= 0x000000FF,
	FlthSHFT	= 16,		/* Tx Fill Threshold */
	FlthMASK	= 0x0000FF00,
	Brstdis		= 0x00080000,	/* 1000Mb/s Burst Disable */
	MxdmaSHFT	= 20,		/* Max Size per Tx DMA Burst */
	MxdmaMASK	= 0x00700000,
	Ecretryen	= 0x00800000,	/* Excessive Collision Retry Enable */
	Atp		= 0x10000000,	/* Automatic Transmit Padding */
	Mlb		= 0x20000000,	/* MAC Loopback */
	Hbi		= 0x40000000,	/* Heartbeat Ignore */
	Csi		= 0x80000000,	/* Carrier Sense Ignore */
};

enum {					/* Receive Configuration */
	RxdrthSHFT	= 1,		/* Rx Drain Threshold */
	RxdrthMASK	= 0x0000003E,
	Airl		= 0x04000000,	/* Accept In-Range Length Errored */
	Alp		= 0x08000000,	/* Accept Long Packets */
	Rxfd		= 0x10000000,	/* Receive Full Duplex */
	Stripcrc	= 0x20000000,	/* Strip CRC */
	Arp		= 0x40000000,	/* Accept Runt Packets */
	Aep		= 0x80000000,	/* Accept Errored Packets */
};

enum {					/* Priority Queueing Control */
	Txpqen		= 0x00000001,	/* Transmit Priority Queuing Enable */
	Txfairen	= 0x00000002,	/* Transmit Fairness Enable */
	RxpqenSHFT	= 2,		/* Receive Priority Queue Enable */
	RxpqenMASK	= 0x0000000C,
};

enum {					/* Pause Control/Status */
	PscntSHFT	= 0,		/* Pause Counter Value */
	PscntMASK	= 0x0000FFFF,
	Pstx		= 0x00020000,	/* Transmit Pause Frame */
	PsffloSHFT	= 18,		/* Rx Data FIFO Lo Threshold */
	PsffloMASK	= 0x000C0000,
	PsffhiSHFT	= 20,		/* Rx Data FIFO Hi Threshold */
	PsffhiMASK	= 0x00300000,
	PsstloSHFT	= 22,		/* Rx Stat FIFO Hi Threshold */
	PsstloMASK	= 0x00C00000,
	PssthiSHFT	= 24,		/* Rx Stat FIFO Hi Threshold */
	PssthiMASK	= 0x03000000,
	Psrcvd		= 0x08000000,	/* Pause Frame Received */
	Psact		= 0x10000000,	/* Pause Active */
	Psda		= 0x20000000,	/* Pause on Destination Address */
	Psmcast		= 0x40000000,	/* Pause on Multicast */
	Psen		= 0x80000000,	/* Pause Enable */
};

enum {					/* Receive Filter/Match Control */
	RfaddrSHFT	= 0,		/* Extended Register Address */
	RfaddrMASK	= 0x000003FF,
	Ulm		= 0x00080000,	/* U/L bit mask */
	Uhen		= 0x00100000,	/* Unicast Hash Enable */
	Mhen		= 0x00200000,	/* Multicast Hash Enable */
	Aarp		= 0x00400000,	/* Accept ARP Packets */
	ApatSHFT	= 23,		/* Accept on Pattern Match */
	ApatMASK	= 0x07800000,
	Apm		= 0x08000000,	/* Accept on Perfect Match */
	Aau		= 0x10000000,	/* Accept All Unicast */
	Aam		= 0x20000000,	/* Accept All Multicast */
	Aab		= 0x40000000,	/* Accept All Broadcast */
	Rfen		= 0x80000000,	/* Rx Filter Enable */
};

enum {					/* Receive Filter/Match Data */
	RfdataSHFT	= 0,		/* Receive Filter Data */
	RfdataMASK	= 0x0000FFFF,
	BmaskSHFT	= 16,		/* Byte Mask */
	BmaskMASK	= 0x00030000,
};

enum {					/* MIB Control */
	Wrn		= 0x00000001,	/* Warning Test Indicator */
	Frz		= 0x00000002,	/* Freeze All Counters */
	Aclr		= 0x00000004,	/* Clear All Counters */
	Mibs		= 0x00000008,	/* MIB Counter Strobe */
};

enum {					/* MIB Data */
	Nmibd		= 11,		/* Number of MIB Data Registers */
};

enum {					/* VLAN/IP Receive Control */
	Vtden		= 0x00000001,	/* VLAN Tag Detection Enable */
	Vtren		= 0x00000002,	/* VLAN Tag Removal Enable */
	Dvtf		= 0x00000004,	/* Discard VLAN Tagged Frames */
	Dutf		= 0x00000008,	/* Discard Untagged Frames */
	Ipen		= 0x00000010,	/* IP Checksum Enable */
	Ripe		= 0x00000020,	/* Reject IP Checksum Errors */
	Rtcpe		= 0x00000040,	/* Reject TCP Checksum Errors */
	Rudpe		= 0x00000080,	/* Reject UDP Checksum Errors */
};

enum {					/* VLAN/IP Transmit Control */
	Vgti		= 0x00000001,	/* VLAN Global Tag Insertion */
	Vppti		= 0x00000002,	/* VLAN Per-Packet Tag Insertion */
	Gchk		= 0x00000004,	/* Global Checksum Generation */
	Ppchk		= 0x00000008,	/* Per-Packet Checksum Generation */
};

enum {					/* VLAN Data */
	VtypeSHFT	= 0,		/* VLAN Type Field */
	VtypeMASK	= 0x0000FFFF,
	VtciSHFT	= 16,		/* VLAN Tag Control Information */
	VtciMASK	= 0xFFFF0000,
};

enum {					/* Clockrun Control/Status */
	Clkrunen	= 0x00000001,	/* CLKRUN Enable */
	Pmeen		= 0x00000100,	/* PME Enable */
	Pmests		= 0x00008000,	/* PME Status */
};

typedef struct {
	u32int	link;			/* Link to the next descriptor */
	u32int	bufptr;			/* pointer to data Buffer */
	int	cmdsts;			/* Command/Status */
	int	extsts;			/* optional Extended Status */

	Block*	bp;			/* Block containing bufptr */
	u32int	unused;			/* pad to 64-bit */
} Desc;

enum {					/* Common cmdsts bits */
	SizeMASK	= 0x0000FFFF,	/* Descriptor Byte Count */
	SizeSHFT	= 0,
	Ok		= 0x08000000,	/* Packet OK */
	Crc		= 0x10000000,	/* Suppress/Include CRC */
	Intr		= 0x20000000,	/* Interrupt on ownership transfer */
	More		= 0x40000000,	/* not last descriptor in a packet */
	Own		= 0x80000000,	/* Descriptor Ownership */
};

enum {					/* Transmit cmdsts bits */
	CcntMASK	= 0x000F0000,	/* Collision Count */
	CcntSHFT	= 16,
	Ec		= 0x00100000,	/* Excessive Collisions */
	Owc		= 0x00200000,	/* Out of Window Collision */
	Ed		= 0x00400000,	/* Excessive Deferral */
	Td		= 0x00800000,	/* Transmit Deferred */
	Crs		= 0x01000000,	/* Carrier Sense Lost */
	Tfu		= 0x02000000,	/* Transmit FIFO Underrun */
	Txa		= 0x04000000,	/* Transmit Abort */
};

enum {					/* Receive cmdsts bits */
	Irl		= 0x00010000,	/* In-Range Length Error */
	Lbp		= 0x00020000,	/* Loopback Packet */
	Fae		= 0x00040000,	/* Frame Alignment Error */
	Crce		= 0x00080000,	/* CRC Error */
	Ise		= 0x00100000,	/* Invalid Symbol Error */
	Runt		= 0x00200000,	/* Runt Packet Received */
	Long		= 0x00400000,	/* Too Long Packet Received */
	DestMASK	= 0x01800000,	/* Destination Class */
	DestSHFT	= 23,
	Rxo		= 0x02000000,	/* Receive Overrun */
	Rxa		= 0x04000000,	/* Receive Aborted */
};

enum {					/* extsts bits */
	EvtciMASK	= 0x0000FFFF,	/* VLAN Tag Control Information */
	EvtciSHFT	= 0,
	Vpkt		= 0x00010000,	/* VLAN Packet */
	Ippkt		= 0x00020000,	/* IP Packet */
	Iperr		= 0x00040000,	/* IP Checksum Error */
	Tcppkt		= 0x00080000,	/* TCP Packet */
	Tcperr		= 0x00100000,	/* TCP Checksum Error */
	Udppkt		= 0x00200000,	/* UDP Packet */
	Udperr		= 0x00400000,	/* UDP Checksum Error */
};

enum {
	Nrd		= 256,
	Nrb		= 4*Nrd,
	Rbsz		= ROUNDUP(sizeof(Etherpkt)+8, 8),
	Ntd		= 128,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;

	int	eepromsz;		/* address size in bits */
	ushort*	eeprom;

	int*	nic;
	int	cfg;
	int	imr;

	QLock	alock;			/* attach */
	Lock	ilock;			/* init */
	void*	alloc;			/* base of per-Ctlr allocated data */

	Mii*	mii;

	Lock	rdlock;			/* receive */
	Desc*	rd;
	int	nrd;
	int	nrb;
	int	rdx;
	int	rxcfg;

	Lock	tlock;			/* transmit */
	Desc*	td;
	int	ntd;
	int	tdh;
	int	tdt;
	int	ntq;
	int	txcfg;

	int	rxidle;

	uint	mibd[Nmibd];

	int	ec;
	int	owc;
	int	ed;
	int	crs;
	int	tfu;
	int	txa;
} Ctlr;

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr* dp83820ctlrhead;
static Ctlr* dp83820ctlrtail;

static Lock dp83820rblock;		/* free receive Blocks */
static Block* dp83820rbpool;

static char* dp83820mibs[Nmibd] = {
	"RXErroredPkts",
	"RXFCSErrors",
	"RXMsdPktErrors",
	"RXFAErrors",
	"RXSymbolErrors",
	"RXFrameToLong",
	"RXIRLErrors",
	"RXBadOpcodes",
	"RXPauseFrames",
	"TXPauseFrames",
	"TXSQEErrors",
};

static int
mdior(Ctlr* ctlr, int n)
{
	int data, i, mear, r;

	mear = csr32r(ctlr, Mear);
	r = ~(Mdc|Mddir) & mear;
	data = 0;
	for(i = n-1; i >= 0; i--){
		if(csr32r(ctlr, Mear) & Mdio)
			data |= (1<<i);
		csr32w(ctlr, Mear, Mdc|r);
		csr32w(ctlr, Mear, r);
	}
	csr32w(ctlr, Mear, mear);

	return data;
}

static void
mdiow(Ctlr* ctlr, int bits, int n)
{
	int i, mear, r;

	mear = csr32r(ctlr, Mear);
	r = Mddir|(~Mdc & mear);
	for(i = n-1; i >= 0; i--){
		if(bits & (1<<i))
			r |= Mdio;
		else
			r &= ~Mdio;
		csr32w(ctlr, Mear, r);
		csr32w(ctlr, Mear, Mdc|r);
	}
	csr32w(ctlr, Mear, mear);
}

static int
dp83820miimir(Mii* mii, int pa, int ra)
{
	int data;
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	/*
	 * MII Management Interface Read.
	 *
	 * Preamble;
	 * ST+OP+PA+RA;
	 * LT + 16 data bits.
	 */
	mdiow(ctlr, 0xFFFFFFFF, 32);
	mdiow(ctlr, 0x1800|(pa<<5)|ra, 14);
	data = mdior(ctlr, 18);

	if(data & 0x10000)
		return -1;

	return data & 0xFFFF;
}

static int
dp83820miimiw(Mii* mii, int pa, int ra, int data)
{
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	/*
	 * MII Management Interface Write.
	 *
	 * Preamble;
	 * ST+OP+PA+RA+LT + 16 data bits;
	 * Z.
	 */
	mdiow(ctlr, 0xFFFFFFFF, 32);
	data &= 0xFFFF;
	data |= (0x05<<(5+5+2+16))|(pa<<(5+2+16))|(ra<<(2+16))|(0x02<<16);
	mdiow(ctlr, data, 32);

	return 0;
}

static Block *
dp83820rballoc(Desc* desc)
{
	Block *bp;

	if(desc->bp == nil){
		ilock(&dp83820rblock);
		if((bp = dp83820rbpool) == nil){
			iunlock(&dp83820rblock);
			desc->bp = nil;
			desc->cmdsts = Own;
			return nil;
		}
		dp83820rbpool = bp->next;
		bp->next = nil;
		iunlock(&dp83820rblock);
	
		desc->bufptr = PCIWADDR(bp->rp);
		desc->bp = bp;
	}
	else{
		bp = desc->bp;
		bp->rp = bp->lim - Rbsz;
		bp->wp = bp->rp;
	}

	coherence();
	desc->cmdsts = Intr|Rbsz;

	return bp;
}

static void
dp83820rbfree(Block *bp)
{
	bp->rp = bp->lim - Rbsz;
	bp->wp = bp->rp;

	ilock(&dp83820rblock);
	bp->next = dp83820rbpool;
	dp83820rbpool = bp;
	iunlock(&dp83820rblock);
}

static void
dp83820halt(Ctlr* ctlr)
{
	int i, timeo;

	ilock(&ctlr->ilock);
	csr32w(ctlr, Imr, 0);
	csr32w(ctlr, Ier, 0);
	csr32w(ctlr, Cr, Rxd|Txd);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr32r(ctlr, Cr) & (Rxe|Txe)))
			break;
		microdelay(1);
	}
	csr32w(ctlr, Mibc, Frz);
	iunlock(&ctlr->ilock);

	if(ctlr->rd != nil){
		for(i = 0; i < ctlr->nrd; i++){
			if(ctlr->rd[i].bp == nil)
				continue;
			freeb(ctlr->rd[i].bp);
			ctlr->rd[i].bp = nil;
		}
	}
	if(ctlr->td != nil){
		for(i = 0; i < ctlr->ntd; i++){
			if(ctlr->td[i].bp == nil)
				continue;
			freeb(ctlr->td[i].bp);
			ctlr->td[i].bp = nil;
		}
	}
}

static void
dp83820cfg(Ctlr* ctlr)
{
	int cfg;

	/*
	 * Don't know how to deal with a TBI yet.
	 */
	if(ctlr->mii == nil)
		return;

	/*
	 * The polarity of these bits is at the mercy
	 * of the board designer.
	 * The correct answer for all speed and duplex questions
	 * should be to query the phy.
	 */
	cfg = csr32r(ctlr, Cfg);
	if(!(cfg & Dupsts)){
		ctlr->rxcfg |= Rxfd;
		ctlr->txcfg |= Csi|Hbi;
		iprint("83820: full duplex, ");
	}
	else{
		ctlr->rxcfg &= ~Rxfd;
		ctlr->txcfg &= ~(Csi|Hbi);
		iprint("83820: half duplex, ");
	}
	csr32w(ctlr, Rxcfg, ctlr->rxcfg);
	csr32w(ctlr, Txcfg, ctlr->txcfg);

	switch(cfg & (Spdsts1000|Spdsts100)){
	case Spdsts1000:		/* 100Mbps */
	default:			/* 10Mbps */
		ctlr->cfg &= ~Mode1000;
		if((cfg & (Spdsts1000|Spdsts100)) == Spdsts1000)
			iprint("100Mb/s\n");
		else
			iprint("10Mb/s\n");
		break;
	case Spdsts100:			/* 1Gbps */
		ctlr->cfg |= Mode1000;
		iprint("1Gb/s\n");
		break;
	}
	csr32w(ctlr, Cfg, ctlr->cfg);
}

static void
dp83820init(Ether* edev)
{
	int i;
	Ctlr *ctlr;
	Desc *desc;
	uchar *alloc;

	ctlr = edev->ctlr;

	dp83820halt(ctlr);

	/*
	 * Receiver
	 */
	alloc = (uchar*)ROUNDUP((ulong)ctlr->alloc, 8);
	ctlr->rd = (Desc*)alloc;
	alloc += ctlr->nrd*sizeof(Desc);
	memset(ctlr->rd, 0, ctlr->nrd*sizeof(Desc));
	ctlr->rdx = 0;
	for(i = 0; i < ctlr->nrd; i++){
		desc = &ctlr->rd[i];
		desc->link = PCIWADDR(&ctlr->rd[NEXT(i, ctlr->nrd)]);
		if(dp83820rballoc(desc) == nil)
			continue;
	}
	csr32w(ctlr, Rxdphi, 0);
	csr32w(ctlr, Rxdp, PCIWADDR(ctlr->rd));

	for(i = 0; i < Eaddrlen; i += 2){
		csr32w(ctlr, Rfcr, i);
		csr32w(ctlr, Rfdr, (edev->ea[i+1]<<8)|edev->ea[i]);
	}
	csr32w(ctlr, Rfcr, Rfen|Aab|Aam|Apm);

	ctlr->rxcfg = Stripcrc|(((2*(ETHERMINTU+4))/8)<<RxdrthSHFT);
	ctlr->imr |= Rxorn|Rxidle|Rxearly|Rxdesc|Rxok;

	/*
	 * Transmitter.
	 */
	ctlr->td = (Desc*)alloc;
	memset(ctlr->td, 0, ctlr->ntd*sizeof(Desc));
	ctlr->tdh = ctlr->tdt = ctlr->ntq = 0;
	for(i = 0; i < ctlr->ntd; i++){
		desc = &ctlr->td[i];
		desc->link = PCIWADDR(&ctlr->td[NEXT(i, ctlr->ntd)]);
	}
	csr32w(ctlr, Txdphi, 0);
	csr32w(ctlr, Txdp, PCIWADDR(ctlr->td));

	ctlr->txcfg = Atp|(((2*(ETHERMINTU+4))/32)<<FlthSHFT)|((4096/32)<<TxdrthSHFT);
	ctlr->imr |= Txurn|Txidle|Txdesc|Txok;

	ilock(&ctlr->ilock);

	dp83820cfg(ctlr);

	csr32w(ctlr, Mibc, Aclr);
	ctlr->imr |= Mib;

	csr32w(ctlr, Imr, ctlr->imr);

	/* try coalescing adjacent interrupts; use hold-off interval of 100µs */
	csr32w(ctlr, Ihr, Ihctl|(1<<IhSHFT));

	csr32w(ctlr, Ier, Ien);
	csr32w(ctlr, Cr, Rxe|Txe);

	iunlock(&ctlr->ilock);
}

static void
dp83820attach(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){
		qunlock(&ctlr->alock);
		return;
	}

	if(waserror()){
		if(ctlr->mii != nil){
			free(ctlr->mii);
			ctlr->mii = nil;
		}
		if(ctlr->alloc != nil){
			free(ctlr->alloc);
			ctlr->alloc = nil;
		}
		qunlock(&ctlr->alock);
		nexterror();
	}

	if(!(ctlr->cfg & Tbien)){
		if((ctlr->mii = malloc(sizeof(Mii))) == nil)
			error(Enomem);
		ctlr->mii->ctlr = ctlr;
		ctlr->mii->mir = dp83820miimir;
		ctlr->mii->miw = dp83820miimiw;
		if(mii(ctlr->mii, ~0) == 0)
			error("no PHY");
		ctlr->cfg |= Dupstsien|Lnkstsien|Spdstsien;
		ctlr->imr |= Phy;
	}

	ctlr->nrd = Nrd;
	ctlr->nrb = Nrb;
	ctlr->ntd = Ntd;
	ctlr->alloc = mallocz((ctlr->nrd+ctlr->ntd)*sizeof(Desc) + 7, 0);
	if(ctlr->alloc == nil)
		error(Enomem);

	for(ctlr->nrb = 0; ctlr->nrb < Nrb; ctlr->nrb++){
		if((bp = allocb(Rbsz)) == nil)
			break;
		bp->free = dp83820rbfree;
		dp83820rbfree(bp);
	}

	dp83820init(edev);

	qunlock(&ctlr->alock);
	poperror();
}

static void
dp83820transmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	Desc *desc;
	int cmdsts, r, x;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);

	bp = nil;
	for(x = ctlr->tdh; ctlr->ntq; x = NEXT(x, ctlr->ntd)){
		desc = &ctlr->td[x];
		if((cmdsts = desc->cmdsts) & Own)
			break;
		if(!(cmdsts & Ok)){
			if(cmdsts & Ec)
				ctlr->ec++;
			if(cmdsts & Owc)
				ctlr->owc++;
			if(cmdsts & Ed)
				ctlr->ed++;
			if(cmdsts & Crs)
				ctlr->crs++;
			if(cmdsts & Tfu)
				ctlr->tfu++;
			if(cmdsts & Txa)
				ctlr->txa++;
			edev->oerrs++;
		}
		desc->bp->next = bp;
		bp = desc->bp;
		desc->bp = nil;

		ctlr->ntq--;
	}
	ctlr->tdh = x;
	if(bp != nil)
		freeblist(bp);

	x = ctlr->tdt;
	while(ctlr->ntq < (ctlr->ntd-1)){
		if((bp = qget(edev->oq)) == nil)
			break;

		desc = &ctlr->td[x];
		desc->bufptr = PCIWADDR(bp->rp);
		desc->bp = bp;
		ctlr->ntq++;
		coherence();
		desc->cmdsts = Own|Intr|BLEN(bp);

		x = NEXT(x, ctlr->ntd);
	}
	if(x != ctlr->tdt){
		ctlr->tdt = x;
		r = csr32r(ctlr, Cr);
		csr32w(ctlr, Cr, Txe|r);
	}

	iunlock(&ctlr->tlock);
}

static void
dp83820interrupt(Ureg*, void* arg)
{
	Block *bp;
	Ctlr *ctlr;
	Desc *desc;
	Ether *edev;
	int cmdsts, i, isr, r, x;

	edev = arg;
	ctlr = edev->ctlr;

	for(isr = csr32r(ctlr, Isr); isr & ctlr->imr; isr = csr32r(ctlr, Isr)){
		if(isr & (Rxorn|Rxidle|Rxearly|Rxerr|Rxdesc|Rxok)){
			x = ctlr->rdx;
			desc = &ctlr->rd[x];
			while((cmdsts = desc->cmdsts) & Own){
				if((cmdsts & Ok) && desc->bp != nil){
					bp = desc->bp;
					desc->bp = nil;
					bp->wp += cmdsts & SizeMASK;
					etheriq(edev, bp, 1);
				}
				//else if(!(cmdsts & Ok)){
				//	iprint("dp83820: rx %8.8uX:", cmdsts);
				//	bp = desc->bp;
				//	for(i = 0; i < 20; i++)
				//		iprint(" %2.2uX", bp->rp[i]);
				//	iprint("\n");
				//}
				dp83820rballoc(desc);

				x = NEXT(x, ctlr->nrd);
				desc = &ctlr->rd[x];
			}
			ctlr->rdx = x;

			if(isr & Rxidle){
				r = csr32r(ctlr, Cr);
				csr32w(ctlr, Cr, Rxe|r);
				ctlr->rxidle++;
			}

			isr &= ~(Rxorn|Rxidle|Rxearly|Rxerr|Rxdesc|Rxok);
		}

		if(isr & Txurn){
			x = (ctlr->txcfg & TxdrthMASK)>>TxdrthSHFT;
			r = (ctlr->txcfg & FlthMASK)>>FlthSHFT;
			if(x < ((TxdrthMASK)>>TxdrthSHFT)
			&& x < (2048/32 - r)){
				ctlr->txcfg &= ~TxdrthMASK;
				x++;
				ctlr->txcfg |= x<<TxdrthSHFT;
				csr32w(ctlr, Txcfg, ctlr->txcfg);
			}
		}

		if(isr & (Txurn|Txidle|Txdesc|Txok)){
			dp83820transmit(edev);
			isr &= ~(Txurn|Txidle|Txdesc|Txok);
		}

		if(isr & Mib){
			for(i = 0; i < Nmibd; i++){
				r = csr32r(ctlr, Mibd+(i*sizeof(int)));
				ctlr->mibd[i] += r & 0xFFFF;
			}
			isr &= ~Mib;
		}

		if((isr & Phy) && ctlr->mii != nil){
			ctlr->mii->mir(ctlr->mii, 1, Bmsr);
			print("phy: cfg %8.8uX bmsr %4.4uX\n",
				csr32r(ctlr, Cfg),
				ctlr->mii->mir(ctlr->mii, 1, Bmsr));
			dp83820cfg(ctlr);
			isr &= ~Phy;
		}
		if(isr)
			iprint("dp83820: isr %8.8uX\n", isr);
	}
}

static long
dp83820ifstat(Ether* edev, void* a, long n, ulong offset)
{
	char *p;
	Ctlr *ctlr;
	int i, l, r;

	ctlr = edev->ctlr;

	edev->crcs = ctlr->mibd[Mibd+(1*sizeof(int))];
	edev->frames = ctlr->mibd[Mibd+(3*sizeof(int))];
	edev->buffs = ctlr->mibd[Mibd+(5*sizeof(int))];
	edev->overflows = ctlr->mibd[Mibd+(2*sizeof(int))];

	if(n == 0)
		return 0;

	p = malloc(READSTR);
	l = 0;
	for(i = 0; i < Nmibd; i++){
		r = csr32r(ctlr, Mibd+(i*sizeof(int)));
		ctlr->mibd[i] += r & 0xFFFF;
		if(ctlr->mibd[i] != 0 && dp83820mibs[i] != nil)
			l += snprint(p+l, READSTR-l, "%s: %ud %ud\n",
				dp83820mibs[i], ctlr->mibd[i], r);
	}
	l += snprint(p+l, READSTR-l, "rxidle %d\n", ctlr->rxidle);
	l += snprint(p+l, READSTR-l, "ec %d\n", ctlr->ec);
	l += snprint(p+l, READSTR-l, "owc %d\n", ctlr->owc);
	l += snprint(p+l, READSTR-l, "ed %d\n", ctlr->ed);
	l += snprint(p+l, READSTR-l, "crs %d\n", ctlr->crs);
	l += snprint(p+l, READSTR-l, "tfu %d\n", ctlr->tfu);
	l += snprint(p+l, READSTR-l, "txa %d\n", ctlr->txa);

	l += snprint(p+l, READSTR, "rom:");
	for(i = 0; i < 0x10; i++){
		if(i && ((i & 0x07) == 0))
			l += snprint(p+l, READSTR-l, "\n    ");
		l += snprint(p+l, READSTR-l, " %4.4uX", ctlr->eeprom[i]);
	}
	l += snprint(p+l, READSTR-l, "\n");

	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		l += snprint(p+l, READSTR, "phy:");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				l += snprint(p+l, READSTR-l, "\n    ");
			r = miimir(ctlr->mii, i);
			l += snprint(p+l, READSTR-l, " %4.4uX", r);
		}
		snprint(p+l, READSTR-l, "\n");
	}

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
dp83820promiscuous(void* arg, int on)
{
	USED(arg, on);
}

/* multicast already on, don't need to do anything */
static void
dp83820multicast(void*, uchar*, int)
{
}

static int
dp83820detach(Ctlr* ctlr)
{
	/*
	 * Soft reset the controller.
	 */
	csr32w(ctlr, Cr, Rst);
	delay(1);
	while(csr32r(ctlr, Cr) & Rst)
		delay(1);
	return 0;
}

static void
dp83820shutdown(Ether* ether)
{
print("dp83820shutdown\n");
	dp83820detach(ether->ctlr);
}

static int
atc93c46r(Ctlr* ctlr, int address)
{
	int data, i, mear, r, size;

	/*
	 * Analog Technology, Inc. ATC93C46
	 * or equivalent serial EEPROM.
	 */
	mear = csr32r(ctlr, Mear);
	mear &= ~(Eesel|Eeclk|Eedo|Eedi);
	r = Eesel|mear;

reread:
	csr32w(ctlr, Mear, r);
	data = 0x06;
	for(i = 3-1; i >= 0; i--){
		if(data & (1<<i))
			r |= Eedi;
		else
			r &= ~Eedi;
		csr32w(ctlr, Mear, r);
		csr32w(ctlr, Mear, Eeclk|r);
		microdelay(1);
		csr32w(ctlr, Mear, r);
		microdelay(1);
	}

	/*
	 * First time through must work out the EEPROM size.
	 */
	if((size = ctlr->eepromsz) == 0)
		size = 8;

	for(size = size-1; size >= 0; size--){
		if(address & (1<<size))
			r |= Eedi;
		else
			r &= ~Eedi;
		csr32w(ctlr, Mear, r);
		microdelay(1);
		csr32w(ctlr, Mear, Eeclk|r);
		microdelay(1);
		csr32w(ctlr, Mear, r);
		microdelay(1);
		if(!(csr32r(ctlr, Mear) & Eedo))
			break;
	}
	r &= ~Eedi;

	data = 0;
	for(i = 16-1; i >= 0; i--){
		csr32w(ctlr, Mear, Eeclk|r);
		microdelay(1);
		if(csr32r(ctlr, Mear) & Eedo)
			data |= (1<<i);
		csr32w(ctlr, Mear, r);
		microdelay(1);
	}

	csr32w(ctlr, Mear, mear);

	if(ctlr->eepromsz == 0){
		ctlr->eepromsz = 8-size;
		ctlr->eeprom = malloc((1<<ctlr->eepromsz)*sizeof(ushort));
		goto reread;
	}

	return data;
}

static int
dp83820reset(Ctlr* ctlr)
{
	int i, r;
	unsigned char sum;

	/*
	 * Soft reset the controller;
	 * read the EEPROM to get the initial settings
	 * of the Cfg and Gpior bits which should be cleared by
	 * the reset.
	 */
	dp83820detach(ctlr);

	atc93c46r(ctlr, 0);
	if(ctlr->eeprom == nil) {
		print("dp83820reset: no eeprom\n");
		return -1;
	}
	sum = 0;
	for(i = 0; i < 0x0E; i++){
		r = atc93c46r(ctlr, i);
		ctlr->eeprom[i] = r;
		sum += r;
		sum += r>>8;
	}

	if(sum != 0){
		print("dp83820reset: bad EEPROM checksum\n");
		return -1;
	}

#ifdef notdef
	csr32w(ctlr, Gpior, ctlr->eeprom[4]);

	cfg = Extstsen|Exd;
	r = csr32r(ctlr, Cfg);
	if(ctlr->eeprom[5] & 0x0001)
		cfg |= Ext125;
	if(ctlr->eeprom[5] & 0x0002)
		cfg |= M64addren;
	if((ctlr->eeprom[5] & 0x0004) && (r & Pci64det))
		cfg |= Data64en;
	if(ctlr->eeprom[5] & 0x0008)
		cfg |= T64addren;
	if(!(pcicfgr16(ctlr->pcidev, PciPCR) & 0x10))
		cfg |= Mwidis;
	if(ctlr->eeprom[5] & 0x0020)
		cfg |= Mrmdis;
	if(ctlr->eeprom[5] & 0x0080)
		cfg |= Mode1000;
	if(ctlr->eeprom[5] & 0x0200)
		cfg |= Tbien|Mode1000;
	/*
	 * What about RO bits we might have destroyed with Rst?
	 * What about Exd, Tmrtest, Extstsen, Pintctl?
	 * Why does it think it has detected a 64-bit bus when
	 * it hasn't?
	 */
#else
	//r = csr32r(ctlr, Cfg);
	//r &= ~(Mode1000|T64addren|Data64en|M64addren);
	//csr32w(ctlr, Cfg, r);
	//csr32w(ctlr, Cfg, 0x2000);
#endif /* notdef */
	ctlr->cfg = csr32r(ctlr, Cfg);
print("cfg %8.8uX pcicfg %8.8uX\n", ctlr->cfg, pcicfgr32(ctlr->pcidev, PciPCR));
	ctlr->cfg &= ~(T64addren|Data64en|M64addren);
	csr32w(ctlr, Cfg, ctlr->cfg);
	csr32w(ctlr, Mibc, Aclr|Frz);

	return 0;
}

static void
dp83820pci(void)
{
	void *mem;
	Pcidev *p;
	Ctlr *ctlr;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x0022<<16)|0x100B:	/* DP83820 (Gig-NIC) */
			break;
		}

		mem = vmap(p->mem[1].bar & ~0x0F, p->mem[1].size);
		if(mem == 0){
			print("DP83820: can't map %8.8luX\n", p->mem[1].bar);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = p->mem[1].bar & ~0x0F;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;

		ctlr->nic = mem;
		if(dp83820reset(ctlr)){
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(dp83820ctlrhead != nil)
			dp83820ctlrtail->next = ctlr;
		else
			dp83820ctlrhead = ctlr;
		dp83820ctlrtail = ctlr;
	}
}

static int
dp83820pnp(Ether* edev)
{
	int i;
	Ctlr *ctlr;
	uchar ea[Eaddrlen];

	if(dp83820ctlrhead == nil)
		dp83820pci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = dp83820ctlrhead; ctlr != nil; ctlr = ctlr->next){
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
	if(memcmp(ea, edev->ea, Eaddrlen) == 0)
		for(i = 0; i < Eaddrlen/2; i++){
			edev->ea[2*i] = ctlr->eeprom[0x0C-i];
			edev->ea[2*i+1] = ctlr->eeprom[0x0C-i]>>8;
		}

	edev->attach = dp83820attach;
	edev->transmit = dp83820transmit;
	edev->interrupt = dp83820interrupt;
	edev->ifstat = dp83820ifstat;

	edev->arg = edev;
	edev->promiscuous = dp83820promiscuous;
	edev->multicast = dp83820multicast;
	edev->shutdown = dp83820shutdown;

	return 0;
}

void
etherdp83820link(void)
{
	addethercard("DP83820", dp83820pnp);
}
