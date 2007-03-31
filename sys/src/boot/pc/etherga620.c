/*
 * bootstrap driver for
 * Netgear GA620 Gigabit Ethernet Card.
 * Specific for the Alteon Tigon 2 and Intel Pentium or later.
 * To Do:
 *	cache alignment for PCI Write-and-Invalidate
 *	mini ring (what size)?
 *	tune coalescing values
 *	statistics formatting
 *	don't update Spi if nothing to send
 *	receive ring alignment
 *	watchdog for link management?
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define malign(n)	xspanalloc((n), 32, 0)

#include "etherif.h"
#include "etherga620fw.h"

enum {
	Mhc		= 0x0040,	/* Miscellaneous Host Control */
	Mlc		= 0x0044,	/* Miscellaneous Local Control */
	Mc		= 0x0050,	/* Miscellaneous Configuration */
	Ps		= 0x005C,	/* PCI State */
	Wba		= 0x0068,	/* Window Base Address */
	Wd		= 0x006C,	/* Window Data */

	DMAas		= 0x011C,	/* DMA Assist State */

	CPUAstate	= 0x0140,	/* CPU A State */
	CPUApc		= 0x0144,	/* CPU A Programme Counter */

	CPUBstate	= 0x0240,	/* CPU B State */

	Hi		= 0x0504,	/* Host In Interrupt Handler */
	Cpi		= 0x050C,	/* Command Producer Index */
	Spi		= 0x0514,	/* Send Producer Index */
	Rspi		= 0x051C,	/* Receive Standard Producer Index */
	Rjpi		= 0x0524,	/* Receive Jumbo Producer Index */
	Rmpi		= 0x052C,	/* Receive Mini Producer Index */

	Mac		= 0x0600,	/* MAC Address */
	Gip		= 0x0608,	/* General Information Pointer */
	Om		= 0x0618,	/* Operating Mode */
	DMArc		= 0x061C,	/* DMA Read Configuration */
	DMAwc		= 0x0620,	/* DMA Write Configuration */
	Tbr		= 0x0624,	/* Transmit Buffer Ratio */
	Eci		= 0x0628,	/* Event Consumer Index */
	Cci		= 0x062C,	/* Command Consumer Index */

	Rct		= 0x0630,	/* Receive Coalesced Ticks */
	Sct		= 0x0634,	/* Send Coalesced Ticks */
	St		= 0x0638,	/* Stat Ticks */
	SmcBD		= 0x063C,	/* Send Max. Coalesced BDs */
	RmcBD		= 0x0640,	/* Receive Max. Coalesced BDs */
	Nt		= 0x0644,	/* NIC Tracing */
	Gln		= 0x0648,	/* Gigabit Link Negotiation */
	Fln		= 0x064C,	/* 10/100 Link Negotiation */
	Ifx		= 0x065C,	/* Interface Index */
	IfMTU		= 0x0660,	/* Interface MTU */
	Mi		= 0x0664,	/* Mask Interrupts */
	Gls		= 0x0668,	/* Gigabit Link State */
	Fls		= 0x066C,	/* 10/100 Link State */

	Cr		= 0x0700,	/* Command Ring */

	Lmw		= 0x0800,	/* Local Memory Window */
};

enum {					/* Mhc */
	Is		= 0x00000001,	/* Interrupt State */
	Ci		= 0x00000002,	/* Clear Interrupt */
	Hr		= 0x00000008,	/* Hard Reset */
	Eebs		= 0x00000010,	/* Enable Endian Byte Swap */
	Eews		= 0x00000020,	/* Enable Endian Word (64-bit) swap */
	Mpio		= 0x00000040,	/* Mask PCI Interrupt Output */
};

enum {					/* Mlc */
	SRAM512		= 0x00000200,	/* SRAM Bank Size of 512KB */
	SRAMmask	= 0x00000300,
	EEclk		= 0x00100000,	/* Serial EEPROM Clock Output */
	EEdoe		= 0x00200000,	/* Serial EEPROM Data Out Enable */
	EEdo		= 0x00400000,	/* Serial EEPROM Data Out Value */
	EEdi		= 0x00800000,	/* Serial EEPROM Data Input */
};

enum {					/* Mc */
	SyncSRAM	= 0x00100000,	/* Set Synchronous SRAM Timing */
};

enum {					/* Ps */
	PCIwm32		= 0x000000C0,	/* Write Max DMA 32 */
	PCImrm		= 0x00020000,	/* Use Memory Read Multiple Command */
	PCI66		= 0x00080000,
	PCI32		= 0x00100000,
	PCIrcmd		= 0x06000000,	/* PCI Read Command */
	PCIwcmd		= 0x70000000,	/* PCI Write Command */
};

enum {					/* CPUAstate */
	CPUrf		= 0x00000010,	/* ROM Fail */
	CPUhalt		= 0x00010000,	/* Halt the internal CPU */
	CPUhie		= 0x00040000,	/* HALT instruction executed */
};

enum {					/* Om */
	BswapBD		= 0x00000002,	/* Byte Swap Buffer Descriptors */
	WswapBD		= 0x00000004,	/* Word Swap Buffer Descriptors */
	Warn		= 0x00000008,
	BswapDMA	= 0x00000010,	/* Byte Swap DMA Data */
	Only1DMA	= 0x00000040,	/* Only One DMA Active at a time */
	NoJFrag		= 0x00000200,	/* Don't Fragment Jumbo Frames */
	Fatal		= 0x40000000,
};

enum {					/* Lmw */
	Lmwsz		= 2*1024,	/* Local Memory Window Size */

	/*
	 * legal values are 0x3800 iff Nsr is 128, 0x3000 iff Nsr is 256,
	 * or 0x2000 iff Nsr is 512.
	 */
	Sr		= 0x3800,	/* Send Ring (accessed via Lmw) */
};

enum {					/* Link */
	Lpref		= 0x00008000,	/* Preferred Link */
	L10MB		= 0x00010000,
	L100MB		= 0x00020000,
	L1000MB		= 0x00040000,
	Lfd		= 0x00080000,	/* Full Duplex */
	Lhd		= 0x00100000,	/* Half Duplex */
	Lefc		= 0x00200000,	/* Emit Flow Control Packets */
	Lofc		= 0x00800000,	/* Obey Flow Control Packets */
	Lean		= 0x20000000,	/* Enable Autonegotiation/Sensing */
	Le		= 0x40000000,	/* Link Enable */
};

typedef struct Host64 {
	uint	hi;
	uint	lo;
} Host64;

typedef struct Ere {			/* Event Ring Element */
	int	event;			/* event<<24 | code<<12 | index */
	int	unused;
} Ere;

typedef int Cmd;			/* cmd<<24 | flags<<12 | index */

typedef struct Rbd {			/* Receive Buffer Descriptor */
	Host64	addr;
	int	indexlen;		/* ring-index<<16 | buffer-length */
	int	flags;			/* only lower 16-bits */
	int	checksum;		/* ip<<16 |tcp/udp */
	int	error;			/* only upper 16-bits */
	int	reserved;
	void*	opaque;			/* passed to receive return ring */
} Rbd;

typedef struct Sbd {			/* Send Buffer Descriptor */
	Host64	addr;
	int	lenflags;		/* len<<16 |flags */
	int	reserved;
} Sbd;

enum {					/* Buffer Descriptor Flags */
	Fend		= 0x00000004,	/* Frame Ends in this Buffer */
	Frjr		= 0x00000010,	/* Receive Jumbo Ring Buffer */
	Funicast	= 0x00000020,	/* Unicast packet (2-bit field) */
	Fmulticast	= 0x00000040,	/* Multicast packet */
	Fbroadcast	= 0x00000060,	/* Broadcast packet */
	Ferror		= 0x00000400,	/* Frame Has Error */
	Frmr		= 0x00001000,	/* Receive Mini Ring Buffer */
};

enum {					/* Buffer Error Flags */
	Ecrc		= 0x00010000,	/* bad CRC */
	Ecollision	= 0x00020000,	/* collision */
	Elink		= 0x00040000,	/* link lost */
	Ephy		= 0x00080000,	/* unspecified PHY frame decode error */
	Eodd		= 0x00100000,	/* odd number of nibbles */
	Emac		= 0x00200000,	/* unspecified MAC abort */
	Elen64		= 0x00400000,	/* short packet */
	Eresources	= 0x00800000,	/* MAC out of internal resources */
	Egiant		= 0x01000000,	/* packet too big */
};

typedef struct Rcb {			/* Ring Control Block */
	Host64	addr;			/* points to the Rbd ring */
	int	control;		/* max_len<<16 |flags */
	int	unused;
} Rcb;

enum {
	TcpUdpCksum	= 0x0001,	/* Perform TCP or UDP checksum */
	IpCksum		= 0x0002,	/* Perform IP checksum */
	NoPseudoHdrCksum= 0x0008,	/* Don't include the pseudo header */
	VlanAssist	= 0x0010,	/* Enable VLAN tagging */
	CoalUpdateOnly	= 0x0020,	/* Coalesce transmit interrupts */
	HostRing	= 0x0040,	/* Sr in host memory */
	SnapCksum	= 0x0080,	/* Parse + offload 802.3 SNAP frames */
	UseExtRxBd	= 0x0100,	/* Extended Rbd for Jumbo frames */
	RingDisabled	= 0x0200,	/* Jumbo or Mini RCB only */
};

typedef struct Gib {			/* General Information Block */
	int	statistics[256];	/* Statistics */
	Rcb	ercb;			/* Event Ring */
	Rcb	crcb;			/* Command Ring */
	Rcb	srcb;			/* Send Ring */
	Rcb	rsrcb;			/* Receive Standard Ring */
	Rcb	rjrcb;			/* Receive Jumbo Ring */
	Rcb	rmrcb;			/* Receive Mini Ring */
	Rcb	rrrcb;			/* Receive Return Ring */
	Host64	epp;			/* Event Producer */
	Host64	rrrpp;			/* Receive Return Ring Producer */
	Host64	scp;			/* Send Consumer */
	Host64	rsp;			/* Refresh Stats */
} Gib;

/*
 * these sizes are all fixed in the card,
 * except for Nsr, which has only 3 valid sizes.
 */
enum {					/* Host/NIC Interface ring sizes */
	Ner		= 256,		/* event ring */
	Ncr		= 64,		/* command ring */
	Nsr		= 128,		/* send ring: 128, 256 or 512 */
	Nrsr		= 512,		/* receive standard ring */
	Nrjr		= 256,		/* receive jumbo ring */
	Nrmr		= 1024,		/* receive mini ring, optional */
	Nrrr		= 2048,		/* receive return ring */
};

enum {
	NrsrHI		= 72,		/* Fill-level of Rsr (m.b. < Nrsr) */
	NrsrLO		= 54,		/* Level at which to top-up ring */
	NrjrHI		= 0,		/* Fill-level of Rjr (m.b. < Nrjr) */
	NrjrLO		= 0,		/* Level at which to top-up ring */
	NrmrHI		= 0,		/* Fill-level of Rmr (m.b. < Nrmr) */
	NrmrLO		= 0,		/* Level at which to top-up ring */
};

typedef struct Ctlr Ctlr;
struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;

	uchar	ea[Eaddrlen];

	int*	nic;
	Gib*	gib;

	Ere*	er;

	Lock	srlock;
	Sbd*	sr;
	Block**	srb;
	int	nsr;			/* currently in send ring */

	Rbd*	rsr;
	int	nrsr;			/* currently in Receive Standard Ring */
	Rbd*	rjr;
	int	nrjr;			/* currently in Receive Jumbo Ring */
	Rbd*	rmr;
	int	nrmr;			/* currently in Receive Mini Ring */
	Rbd*	rrr;
	int	rrrci;			/* Receive Return Ring Consumer Index */

	int	epi[2];			/* Event Producer Index */
	int	rrrpi[2];		/* Receive Return Ring Producer Index */
	int	sci[3];			/* Send Consumer Index ([2] is host) */

	int	interrupts;		/* statistics */
	int	mi;
	uvlong	ticks;

	int	coalupdateonly;		/* tuning */
	int	hardwarecksum;
	int	rct;			/* Receive Coalesce Ticks */
	int	sct;			/* Send Coalesce Ticks */
	int	st;			/* Stat Ticks */
	int	smcbd;			/* Send Max. Coalesced BDs */
	int	rmcbd;			/* Receive Max. Coalesced BDs */
};

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static void
sethost64(Host64* host64, void* addr)
{
	uvlong uvl;

	uvl = PCIWADDR(addr);
	host64->hi = uvl>>32;
	host64->lo = uvl & 0xFFFFFFFFL;
}

static void
ga620command(Ctlr* ctlr, int cmd, int flags, int index)
{
	int cpi;

	cpi = csr32r(ctlr, Cpi);
	csr32w(ctlr, Cr+(cpi*4), cmd<<24 | flags<<12 | index);
	cpi = NEXT(cpi, Ncr);
	csr32w(ctlr, Cpi, cpi);
}

static void
ga620attach(Ether* )
{
}

static void
waitforlink(Ether *edev)
{
	int i;

	if (edev->mbps == 0) {
		print("#l%d: ga620: waiting for link", edev->ctlrno);
		/* usually takes about 10 seconds */
		for (i = 0; i < 20 && edev->mbps == 0; i++) {
			print(".");
			delay(1000);
		}
		print("\n");
		if (i == 20 && edev->mbps == 0)
			edev->mbps = 1;			/* buggered */
	}
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

static Block *
fromringbuf(Ether *ether)
{
	RingBuf *tb = &ether->tb[ether->ti];
	Block *bp = allocb(tb->len);

	if (bp == nil)
		panic("fromringbuf: nil allocb return");
	if (bp->wp == nil)
		panic("fromringbuf: nil bp->wb");
	memmove(bp->wp, tb->pkt, tb->len);
	memmove(bp->wp+Eaddrlen, ether->ea, Eaddrlen);
	bp->wp += tb->len;
	return bp;
}

static int
_ga620transmit(Ether* edev)
{
	Sbd *sbd;
	Block *bp;
	Ctlr *ctlr;
	RingBuf *tb;
	int sci, spi, work;

	/*
	 * For now there are no smarts here, just empty the
	 * ring and try to fill it back up. Tuning comes later.
	 */
	ctlr = edev->ctlr;
	waitforlink(edev);
	ilock(&ctlr->srlock);

	/*
	 * Free any completed packets.
	 * Ctlr->sci[0] is where the NIC has got to consuming the ring.
	 * Ctlr->sci[2] is where the host has got to tidying up after the
	 * NIC has done with the packets.
	 */
	work = 0;
	for(sci = ctlr->sci[2]; sci != ctlr->sci[0]; sci = NEXT(sci, Nsr)){
		if(ctlr->srb[sci] == nil)
			continue;
		freeb(ctlr->srb[sci]);
		ctlr->srb[sci] = nil;
		work++;
	}
	ctlr->sci[2] = sci;

	sci = PREV(sci, Nsr);

	tb = &edev->tb[edev->ti];
	for(spi = csr32r(ctlr, Spi); spi != sci && tb->owner == Interface;
	    spi = NEXT(spi, Nsr)){
		bp = fromringbuf(edev);

		sbd = &ctlr->sr[spi];
		sethost64(&sbd->addr, bp->rp);
		sbd->lenflags = BLEN(bp)<<16 |Fend;

		ctlr->srb[spi] = bp;
		work++;

		tb->owner = Host;
		edev->ti = NEXT(edev->ti, edev->ntb);
		tb = &edev->tb[edev->ti];
	}
	csr32w(ctlr, Spi, spi);

	iunlock(&ctlr->srlock);

	return work;
}

static void
ga620transmit(Ether* edev)
{
	_ga620transmit(edev);
}

static void
ga620replenish(Ctlr* ctlr)
{
	Rbd *rbd;
	int rspi;
	Block *bp;

	rspi = csr32r(ctlr, Rspi);
	while(ctlr->nrsr < NrsrHI){
		if((bp = allocb(ETHERMAXTU+4)) == nil)
			break;
		rbd = &ctlr->rsr[rspi];
		sethost64(&rbd->addr, bp->rp);
		rbd->indexlen = rspi<<16 | (ETHERMAXTU+4);
		rbd->flags = 0;
		rbd->opaque = bp;

		rspi = NEXT(rspi, Nrsr);
		ctlr->nrsr++;
	}
	csr32w(ctlr, Rspi, rspi);
}

static void
ga620event(Ether *edev, int eci, int epi)
{
	unsigned event, code;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	while(eci != epi){
		event = ctlr->er[eci].event;
		code = (event >> 12) & ((1<<12)-1);
		switch(event>>24){
		case 0x01:		/* firmware operational */
			/* host stack (us) is up.  3rd arg of 2 means down. */
			ga620command(ctlr, 0x01, 0x01, 0x00);
			/*
			 * link negotiation: any speed is okay.
			 * 3rd arg of 1 selects gigabit only; 2 10/100 only.
			 */
			ga620command(ctlr, 0x0B, 0x00, 0x00);
			print("#l%d: ga620: port %8.8uX: firmware is up\n",
				edev->ctlrno, ctlr->port);
			break;
		case 0x04:		/* statistics updated */
			break;
		case 0x06:		/* link state changed */
			switch (code) {
			case 1:
				edev->mbps = 1000;
				break;
			case 2:
				print("#l%d: link down\n", edev->ctlrno);
				break;
			case 3:
				edev->mbps = 100;	/* it's 10 or 100 */
				break;
			}
			if (code != 2)
				print("#l%d: %dMbps link up\n",
					edev->ctlrno, edev->mbps);
			break;
		case 0x07:		/* event error */
		default:
			print("#l%d: ga620: er[%d] = %8.8uX\n", edev->ctlrno,
				eci, event);
			break;
		}
		eci = NEXT(eci, Ner);
	}
	csr32w(ctlr, Eci, eci);
}

static void
ga620receive(Ether* edev)
{
	int len;
	Rbd *rbd;
	Block *bp;
	Ctlr* ctlr;

	ctlr = edev->ctlr;
	while(ctlr->rrrci != ctlr->rrrpi[0]){
		rbd = &ctlr->rrr[ctlr->rrrci];
		/*
		 * Errors are collected in the statistics block so
		 * no need to tally them here, let ifstat do the work.
		 */
		len = rbd->indexlen & 0xFFFF;
		if(!(rbd->flags & Ferror) && len != 0){
			bp = rbd->opaque;
			bp->wp = bp->rp+len;

			toringbuf(edev, bp);
		} else
			freeb(rbd->opaque);
		rbd->opaque = nil;

		if(rbd->flags & Frjr)
			ctlr->nrjr--;
		else if(rbd->flags & Frmr)
			ctlr->nrmr--;
		else
			ctlr->nrsr--;

		ctlr->rrrci = NEXT(ctlr->rrrci, Nrrr);
	}
}

static void
ga620interrupt(Ureg*, void* arg)
{
	int csr, ie, work;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;

	if(!(csr32r(ctlr, Mhc) & Is))
		return;

	ctlr->interrupts++;
	csr32w(ctlr, Hi, 1);

	ie = 0;
	work = 0;
	while(ie < 2){
		if(ctlr->rrrci != ctlr->rrrpi[0]){
			ga620receive(edev);
			work = 1;
		}

		if(_ga620transmit(edev) != 0)
			work = 1;

		csr = csr32r(ctlr, Eci);
		if(csr != ctlr->epi[0]){
			ga620event(edev, csr, ctlr->epi[0]);
			work = 1;
		}

		if(ctlr->nrsr <= NrsrLO)
			ga620replenish(ctlr);
		if(work == 0){
			if(ie == 0)
				csr32w(ctlr, Hi, 0);
			ie++;
		}
		work = 0;
	}
}

static void
ga620lmw(Ctlr* ctlr, int addr, int* data, int len)
{
	int i, l, lmw, v;

	/*
	 * Write to or clear ('data' == nil) 'len' bytes of the NIC
	 * local memory at address 'addr'.
	 * The destination address and count should be 32-bit aligned.
	 */
	v = 0;
	while(len > 0){
		/*
		 * 1) Set the window. The (Lmwsz-1) bits are ignored
		 *    in Wba when accessing through the local memory window;
		 * 2) Find the minimum of how many bytes still to
		 *    transfer and how many left in this window;
		 * 3) Create the offset into the local memory window in the
		 *    shared memory space then copy (or zero) the data;
		 * 4) Bump the counts.
		 */
		csr32w(ctlr, Wba, addr);

		l = ROUNDUP(addr+1, Lmwsz) - addr;
		if(l > len)
			l = len;

		lmw = Lmw + (addr & (Lmwsz-1));
		for(i = 0; i < l; i += 4){
			if(data != nil)
				v = *data++;
			csr32w(ctlr, lmw+i, v);
		}

		len -= l;
		addr += l;
	}
}

static int
ga620init(Ether* edev)
{
	Ctlr *ctlr;
	Host64 host64;
	int csr, ea, i, flags;

	ctlr = edev->ctlr;

	/*
	 * Load the MAC address.
	 */
	ea = edev->ea[0]<<8 | edev->ea[1];
	csr32w(ctlr, Mac, ea);
	ea = edev->ea[2]<<24 | edev->ea[3]<<16 | edev->ea[4]<<8 | edev->ea[5];
	csr32w(ctlr, Mac+4, ea);

	/*
	 * General Information Block.
	 */
	ctlr->gib = malloc(sizeof(Gib));
	sethost64(&host64, ctlr->gib);
	csr32w(ctlr, Gip, host64.hi);
	csr32w(ctlr, Gip+4, host64.lo);

	/*
	 * Event Ring.
	 * This is located in host memory. Allocate the ring,
	 * tell the NIC where it is and initialise the indices.
	 */
	ctlr->er = malign(sizeof(Ere)*Ner);
	sethost64(&ctlr->gib->ercb.addr, ctlr->er);
	sethost64(&ctlr->gib->epp, ctlr->epi);
	csr32w(ctlr, Eci, 0);

	/*
	 * Command Ring.
	 * This is located in the General Communications Region
	 * and so the value placed in the Rcb is unused, the NIC
	 * knows where it is. Stick in the value according to
	 * the datasheet anyway.
	 * Initialise the ring and indices.
	 */
	ctlr->gib->crcb.addr.lo = Cr - 0x400;
	for(i = 0; i < Ncr*4; i += 4)
		csr32w(ctlr, Cr+i, 0);
	csr32w(ctlr, Cpi, 0);
	csr32w(ctlr, Cci, 0);

	/*
	 * Send Ring.
	 * This ring is either in NIC memory at a fixed location depending
	 * on how big the ring is or it is in host memory. If in NIC
	 * memory it is accessed via the Local Memory Window; with a send
	 * ring size of 128 the window covers the whole ring and then need
	 * only be set once:
	 *	ctlr->sr = (uchar*)ctlr->nic+Lmw;
	 *	ga620lmw(ctlr, Sr, nil, sizeof(Sbd)*Nsr);
	 *	ctlr->gib->srcb.addr.lo = Sr;
	 * There is nowhere in the Sbd to hold the Block* associated
	 * with this entry so an external array must be kept.
	 */
	ctlr->sr = malign(sizeof(Sbd)*Nsr);
	sethost64(&ctlr->gib->srcb.addr, ctlr->sr);
	if(ctlr->hardwarecksum)
		flags = TcpUdpCksum|NoPseudoHdrCksum|HostRing;
	else 
		flags = HostRing;
	if(ctlr->coalupdateonly) 
		flags |= CoalUpdateOnly;
	ctlr->gib->srcb.control = Nsr<<16 | flags;
	sethost64(&ctlr->gib->scp, ctlr->sci);
	csr32w(ctlr, Spi, 0);
	ctlr->srb = malloc(sizeof(Block*)*Nsr);

	/*
	 * Receive Standard Ring.
	 */
	ctlr->rsr = malign(sizeof(Rbd)*Nrsr);
	sethost64(&ctlr->gib->rsrcb.addr, ctlr->rsr);
	if(ctlr->hardwarecksum)
		flags = TcpUdpCksum|NoPseudoHdrCksum;
	else
		flags = 0;
	ctlr->gib->rsrcb.control = (ETHERMAXTU+4)<<16 | flags;
	csr32w(ctlr, Rspi, 0);

	/*
	 * Jumbo and Mini Rings. Unused for now.
	 */
	ctlr->gib->rjrcb.control = RingDisabled;
	ctlr->gib->rmrcb.control = RingDisabled;

	/*
	 * Receive Return Ring.
	 * This is located in host memory. Allocate the ring,
	 * tell the NIC where it is and initialise the indices.
	 */
	ctlr->rrr = malign(sizeof(Rbd)*Nrrr);
	sethost64(&ctlr->gib->rrrcb.addr, ctlr->rrr);
	ctlr->gib->rrrcb.control = Nrrr<<16 | 0;
	sethost64(&ctlr->gib->rrrpp, ctlr->rrrpi);
	ctlr->rrrci = 0;

	/*
	 * Refresh Stats Pointer.
	 * For now just point it at the existing statistics block.
	 */
	sethost64(&ctlr->gib->rsp, ctlr->gib->statistics);

	/*
	 * DMA configuration.
	 * Use the recommended values.
	 */
	csr32w(ctlr, DMArc, 0x80);
	csr32w(ctlr, DMAwc, 0x80);

	/*
	 * Transmit Buffer Ratio.
	 * Set to 1/3 of available buffer space (units are 1/64ths)
	 * if using Jumbo packets, ~64KB otherwise (assume 1MB on NIC).
	 */
	if(NrjrHI > 0 || Nsr > 128)
		csr32w(ctlr, Tbr, 64/3);
	else
		csr32w(ctlr, Tbr, 4);

	/*
	 * Tuneable parameters.
	 * These defaults are based on the tuning hints in the Alteon
	 * Host/NIC Software Interface Definition and example software.
	 */
	ctlr->rct = 1 /*100*/;
	csr32w(ctlr, Rct, ctlr->rct);
	ctlr->sct = 0;
	csr32w(ctlr, Sct, ctlr->sct);
	ctlr->st = 1000000;
	csr32w(ctlr, St, ctlr->st);
	ctlr->smcbd = Nsr/4;
	csr32w(ctlr, SmcBD, ctlr->smcbd);
	ctlr->rmcbd = 4 /*6*/;
	csr32w(ctlr, RmcBD, ctlr->rmcbd);

	/*
	 * Enable DMA Assist Logic.
	 */
	csr = csr32r(ctlr, DMAas) & ~0x03;
	csr32w(ctlr, DMAas, csr|0x01);

	/*
	 * Link negotiation.
	 * The bits are set here but the NIC must be given a command
	 * once it is running to set negotiation in motion.
	 */
	csr32w(ctlr, Gln, Le|Lean|Lofc|Lfd|L1000MB|Lpref);
	csr32w(ctlr, Fln, Le|Lean|Lhd|Lfd|L100MB|L10MB);

	/*
	 * A unique index for this controller and the maximum packet
	 * length expected.
	 * For now only standard packets are expected.
	 */
	csr32w(ctlr, Ifx, 1);
	csr32w(ctlr, IfMTU, ETHERMAXTU+4);

	/*
	 * Enable Interrupts.
	 * There are 3 ways to mask interrupts - a bit in the Mhc (which
	 * is already cleared), the Mi register and the Hi mailbox.
	 * Writing to the Hi mailbox has the side-effect of clearing the
	 * PCI interrupt.
	 */
	csr32w(ctlr, Mi, 0);
	csr32w(ctlr, Hi, 0);

	/*
	 * Start the firmware.
	 */
	csr32w(ctlr, CPUApc, tigon2FwStartAddr);
	csr = csr32r(ctlr, CPUAstate) & ~CPUhalt;
	csr32w(ctlr, CPUAstate, csr);

	return 0;
}

static int
at24c32io(Ctlr* ctlr, char* op, int data)
{
	char *lp, *p;
	int i, loop, mlc, r;

	mlc = csr32r(ctlr, Mlc);

	r = 0;
	loop = -1;
	lp = nil;
	for(p = op; *p != '\0'; p++){
		switch(*p){
		default:
			return -1;
		case ' ':
			continue;
		case ':':			/* start of 8-bit loop */
			if(lp != nil)
				return -1;
			lp = p;
			loop = 7;
			continue;
		case ';':			/* end of 8-bit loop */
			if(lp == nil)
				return -1;
			loop--;
			if(loop >= 0)
				p = lp;
			else
				lp = nil;
			continue;
		case 'C':			/* assert clock */
			mlc |= EEclk;
			break;
		case 'c':			/* deassert clock */
			mlc &= ~EEclk;
			break;
		case 'D':			/* next bit in 'data' byte */
			if(loop < 0)
				return -1;
			if(data & (1<<loop))
				mlc |= EEdo;
			else
				mlc &= ~EEdo;
			break;
		case 'E':			/* enable data output */
			mlc |= EEdoe;
			break;
		case 'e':			/* disable data output */
			mlc &= ~EEdoe;
			break;
		case 'I':			/* input bit */
			i = (csr32r(ctlr, Mlc) & EEdi) != 0;
			if(loop >= 0)
				r |= (i<<loop);
			else
				r = i;
			continue;
		case 'O':			/* assert data output */
			mlc |= EEdo;
			break;
		case 'o':			/* deassert data output */
			mlc &= ~EEdo;
			break;
		}
		csr32w(ctlr, Mlc, mlc);
		microdelay(1);
	}
	if(loop >= 0)
		return -1;
	return r;
}

static int
at24c32r(Ctlr* ctlr, int addr)
{
	int data;

	/*
	 * Read a byte at address 'addr' from the Atmel AT24C32
	 * Serial EEPROM. The 2-wire EEPROM access is controlled
	 * by 4 bits in Mlc. See the AT24C32 datasheet for
	 * protocol details.
	 */
	/*
	 * Start condition - a high to low transition of data
	 * with the clock high must precede any other command.
	 */
	at24c32io(ctlr, "OECoc", 0);

	/*
	 * Perform a random read at 'addr'. A dummy byte
	 * write sequence is performed to clock in the device
	 * and data word addresses (0 and 'addr' respectively).
	 */
	data = -1;
	if(at24c32io(ctlr, "oE :DCc; oeCIc", 0xA0) != 0)
		goto stop;
	if(at24c32io(ctlr, "oE :DCc; oeCIc", addr>>8) != 0)
		goto stop;
	if(at24c32io(ctlr, "oE :DCc; oeCIc", addr) != 0)
		goto stop;

	/*
	 * Now send another start condition followed by a
	 * request to read the device. The EEPROM responds
	 * by clocking out the data.
	 */
	at24c32io(ctlr, "OECoc", 0);
	if(at24c32io(ctlr, "oE :DCc; oeCIc", 0xA1) != 0)
		goto stop;
	data = at24c32io(ctlr, ":CIc;", 0xA1);

stop:
	/*
	 * Stop condition - a low to high transition of data
	 * with the clock high is a stop condition. After a read
	 * sequence, the stop command will place the EEPROM in
	 * a standby power mode.
	 */
	at24c32io(ctlr, "oECOc", 0);

	return data;
}

static int
ga620detach(Ctlr* ctlr)
{
	int timeo;

	/*
	 * Hard reset (don't know which endian so catch both);
	 * enable for little-endian mode;
	 * wait for code to be loaded from serial EEPROM or flash;
	 * make sure CPU A is halted.
	 */
	csr32w(ctlr, Mhc, Hr<<24 | Hr);
	csr32w(ctlr, Mhc, (Eews|Ci)<<24 | Eews|Ci);

	microdelay(1);
	for(timeo = 0; timeo < 500000; timeo++){
		if((csr32r(ctlr, CPUAstate) & (CPUhie|CPUrf)) == CPUhie)
			break;
		microdelay(1);
	}
	if((csr32r(ctlr, CPUAstate) & (CPUhie|CPUrf)) != CPUhie)
		return -1;
	csr32w(ctlr, CPUAstate, CPUhalt);

	/*
	 * After reset, CPU B seems to be stuck in 'CPUrf'.
	 * Worry about it later.
	 */
	csr32w(ctlr, CPUBstate, CPUhalt);

	return 0;
}

static void
ga620shutdown(Ether* ether)
{
print("ga620shutdown\n");
	ga620detach(ether->ctlr);
}

static int
ga620reset(Ctlr* ctlr)
{
	int cls, csr, i, r;

	if(ga620detach(ctlr) < 0)
		return -1;

	/*
	 * Tigon 2 PCI NICs have 512KB SRAM per bank.
	 * Clear out any lingering serial EEPROM state
	 * bits.
	 */
	csr = csr32r(ctlr, Mlc) & ~(EEdi|EEdo|EEdoe|EEclk|SRAMmask);
	csr32w(ctlr, Mlc, SRAM512|csr);
	csr = csr32r(ctlr, Mc);
	csr32w(ctlr, Mc, SyncSRAM|csr);

	/*
	 * Initialise PCI State register.
	 * If PCI Write-and-Invalidate is enabled set the max write DMA
	 * value to the host cache-line size (32 on Pentium or later).
	 */
	csr = csr32r(ctlr, Ps) & (PCI32|PCI66);
	csr |= PCIwcmd|PCIrcmd|PCImrm;
	if(ctlr->pcidev->pcr & 0x0010){
		cls = pcicfgr8(ctlr->pcidev, PciCLS) * 4;
		if(cls != 32)
			pcicfgw8(ctlr->pcidev, PciCLS, 32/4);
		csr |= PCIwm32;
	}
	csr32w(ctlr, Ps, csr);

	/*
	 * Operating Mode.
	 */
	csr32w(ctlr, Om, Fatal|NoJFrag|BswapDMA|WswapBD);

	/*
	 * Snarf the MAC address from the serial EEPROM.
	 */
	for(i = 0; i < Eaddrlen; i++){
		if((r = at24c32r(ctlr, 0x8E+i)) == -1)
			return -1;
		ctlr->ea[i] = r;
	}

	/*
	 * Load the firmware.
	 */
	ga620lmw(ctlr, tigon2FwTextAddr, tigon2FwText, tigon2FwTextLen);
	ga620lmw(ctlr, tigon2FwRodataAddr, tigon2FwRodata, tigon2FwRodataLen);
	ga620lmw(ctlr, tigon2FwDataAddr, tigon2FwData, tigon2FwDataLen);
	ga620lmw(ctlr, tigon2FwSbssAddr, nil, tigon2FwSbssLen);
	ga620lmw(ctlr, tigon2FwBssAddr, nil, tigon2FwBssLen);

	/*
	 * we will eventually get events telling us that the firmware is
	 * up and that the link is up.
	 */
	return 0;
}

static void
ga620pci(void)
{
	int port;
	Pcidev *p;
	Ctlr *ctlr;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		switch(p->did<<16 | p->vid){
		default:
			continue;
		case 0x620A<<16 | 0x1385:	/* Netgear GA620 fiber */
		case 0x630A<<16 | 0x1385:	/* Netgear GA620T copper */
		case 0x0001<<16 | 0x12AE:	/* Alteon Acenic fiber
						 * and DEC DEGPA-SA */
		case 0x0002<<16 | 0x12AE:	/* Alteon Acenic copper */
		case 0x0009<<16 | 0x10A9:	/* SGI Acenic */
			break;
		}

		port = upamalloc(p->mem[0].bar & ~0x0F, p->mem[0].size, 0);
		if(port == 0){
			print("ga620: can't map %d @ 0x%8.8luX\n",
				p->mem[0].size, p->mem[0].bar);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = port;
		ctlr->pcidev = p;
		ctlr->id = p->did<<16 | p->vid;

		ctlr->nic = KADDR(ctlr->port);
		if(ga620reset(ctlr)){
			free(ctlr);
			continue;
		}

		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;
	}
}

int
ga620pnp(Ether* edev)
{
	Ctlr *ctlr;
	uchar ea[Eaddrlen];

	if(ctlrhead == nil)
		ga620pci();

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

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to
	 * loading the station address in the hardware.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, edev->ea, Eaddrlen) == 0)
		memmove(edev->ea, ctlr->ea, Eaddrlen);

	ga620init(edev);		/* enables interrupts */

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = ga620attach;
	edev->transmit = ga620transmit;
	edev->interrupt = ga620interrupt;
	edev->detach = ga620shutdown;
	return 0;
}
