/*
 * Realtek RTL8110S/8169S.
 * Mostly there. There are some magic register values used
 * which are not described in any datasheet or driver but seem
 * to be necessary.
 * Why is the Fovf descriptor bit set for every received packet?
 * Occasionally the hardware indicates an input TCP checksum error
 * although the higher-level software seems to check the packet OK?
 * No tuning has been done. Only tested on an RTL8110S, there
 * are slight differences between the chips in the series so some
 * tweaks may be needed.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"


typedef struct QLock { int r; } QLock;
#define qlock(i)	while(0)
#define qunlock(i)	while(0)
#define iallocb		allocb
extern void mb386(void);
#define coherence()	mb386()
#define iprint		print
#define mallocalign(n, a, o, s)	ialloc((n), (a))

#include "etherif.h"
#include "ethermii.h"

enum {					/* registers */
	Idr0		= 0x00,		/* MAC address */
	Mar0		= 0x08,		/* Multicast address */
	Dtccr		= 0x10,		/* Dump Tally Counter Command */
	Tnpds		= 0x20,		/* Transmit Normal Priority Descriptors */
	Thpds		= 0x28,		/* Transmit High Priority Descriptors */
	Flash		= 0x30,		/* Flash Memory Read/Write */
	Erbcr		= 0x34,		/* Early Receive Byte Count */
	Ersr		= 0x36,		/* Early Receive Status */
	Cr		= 0x37,		/* Command Register */
	Tppoll		= 0x38,		/* Transmit Priority Polling */
	Imr		= 0x3C,		/* Interrupt Mask */
	Isr		= 0x3E,		/* Interrupt Status */
	Tcr		= 0x40,		/* Transmit Configuration */
	Rcr		= 0x44,		/* Receive Configuration */
	Tctr		= 0x48,		/* Timer Count */
	Mpc		= 0x4C,		/* Missed Packet Counter */
	Cr9346		= 0x50,		/* 9346 Command Register */
	Config0		= 0x51,		/* Configuration Register 0 */
	Config1		= 0x52,		/* Configuration Register 1 */
	Config2		= 0x53,		/* Configuration Register 2 */
	Config3		= 0x54,		/* Configuration Register 3 */
	Config4		= 0x55,		/* Configuration Register 4 */
	Config5		= 0x56,		/* Configuration Register 5 */
	Timerint	= 0x58,		/* Timer Interrupt */
	Mulint		= 0x5C,		/* Multiple Interrupt Select */
	Phyar		= 0x60,		/* PHY Access */
	Tbicsr0		= 0x64,		/* TBI Control and Status */
	Tbianar		= 0x68,		/* TBI Auto-Negotiation Advertisment */
	Tbilpar		= 0x6A,		/* TBI Auto-Negotiation Link Partner */
	Phystatus	= 0x6C,		/* PHY Status */

	Rms		= 0xDA,		/* Receive Packet Maximum Size */
	Cplusc		= 0xE0,		/* C+ Command */
	Rdsar		= 0xE4,		/* Receive Descriptor Start Address */
	Mtps		= 0xEC,		/* Max. Transmit Packet Size */
};

enum {					/* Dtccr */
	Cmd		= 0x00000008,	/* Command */
};

enum {					/* Cr */
	Te		= 0x04,		/* Transmitter Enable */
	Re		= 0x08,		/* Receiver Enable */
	Rst		= 0x10,		/* Software Reset */
};

enum {					/* Tppoll */
	Fswint		= 0x01,		/* Forced Software Interrupt */
	Npq		= 0x40,		/* Normal Priority Queue polling */
	Hpq		= 0x80,		/* High Priority Queue polling */
};

enum {					/* Imr/Isr */
	Rok		= 0x0001,	/* Receive OK */
	Rer		= 0x0002,	/* Receive Error */
	Tok		= 0x0004,	/* Transmit OK */
	Ter		= 0x0008,	/* Transmit Error */
	Rdu		= 0x0010,	/* Receive Descriptor Unavailable */
	Punlc		= 0x0020,	/* Packet Underrun or Link Change */
	Fovw		= 0x0040,	/* Receive FIFO Overflow */
	Tdu		= 0x0080,	/* Transmit Descriptor Unavailable */
	Swint		= 0x0100,	/* Software Interrupt */
	Timeout		= 0x4000,	/* Timer */
	Serr		= 0x8000,	/* System Error */
};

enum {					/* Tcr */
	MtxdmaSHIFT	= 8,		/* Max. DMA Burst Size */
	MtxdmaMASK	= 0x00000700,
	Mtxdmaunlimited	= 0x00000700,
	Acrc		= 0x00010000,	/* Append CRC (not) */
	Lbk0		= 0x00020000,	/* Loopback Test 0 */
	Lbk1		= 0x00040000,	/* Loopback Test 1 */
	Ifg2		= 0x00080000,	/* Interframe Gap 2 */
	HwveridSHIFT	= 23,		/* Hardware Version ID */
	HwveridMASK	= 0x7C800000,
	Ifg0		= 0x01000000,	/* Interframe Gap 0 */
	Ifg1		= 0x02000000,	/* Interframe Gap 1 */
};

enum {					/* Rcr */
	Aap		= 0x00000001,	/* Accept All Packets */
	Apm		= 0x00000002,	/* Accept Physical Match */
	Am		= 0x00000004,	/* Accept Multicast */
	Ab		= 0x00000008,	/* Accept Broadcast */
	Ar		= 0x00000010,	/* Accept Runt */
	Aer		= 0x00000020,	/* Accept Error */
	Sel9356		= 0x00000040,	/* 9356 EEPROM used */
	MrxdmaSHIFT	= 8,		/* Max. DMA Burst Size */
	MrxdmaMASK	= 0x00000700,
	Mrxdmaunlimited	= 0x00000700,
	RxfthSHIFT	= 13,		/* Receive Buffer Length */
	RxfthMASK	= 0x0000E000,
	Rxfth256	= 0x00008000,
	Rxfthnone	= 0x0000E000,
	Rer8		= 0x00010000,	/* Accept Error Packets > 8 bytes */
	MulERINT	= 0x01000000,	/* Multiple Early Interrupt Select */
};

enum {					/* Cr9346 */
	Eedo		= 0x01,		/* */
	Eedi		= 0x02,		/* */
	Eesk		= 0x04,		/* */
	Eecs		= 0x08,		/* */
	Eem0		= 0x40,		/* Operating Mode */
	Eem1		= 0x80,
};

enum {					/* Phyar */
	DataMASK	= 0x0000FFFF,	/* 16-bit GMII/MII Register Data */
	DataSHIFT	= 0,
	RegaddrMASK	= 0x001F0000,	/* 5-bit GMII/MII Register Address */
	RegaddrSHIFT	= 16,
	Flag		= 0x80000000,	/* */
};

enum {					/* Phystatus */
	Fd		= 0x01,		/* Full Duplex */
	Linksts		= 0x02,		/* Link Status */
	Speed10		= 0x04,		/* */
	Speed100	= 0x08,		/* */
	Speed1000	= 0x10,		/* */
	Rxflow		= 0x20,		/* */
	Txflow		= 0x40,		/* */
	Entbi		= 0x80,		/* */
};

enum {					/* Cplusc */
	Mulrw		= 0x0008,	/* PCI Multiple R/W Enable */
	Dac		= 0x0010,	/* PCI Dual Address Cycle Enable */
	Rxchksum	= 0x0020,	/* Receive Checksum Offload Enable */
	Rxvlan		= 0x0040,	/* Receive VLAN De-tagging Enable */
	Endian		= 0x0200,	/* Endian Mode */
};

typedef struct D D;			/* Transmit/Receive Descriptor */
struct D {
	u32int	control;
	u32int	vlan;
	u32int	addrlo;
	u32int	addrhi;
};

enum {					/* Transmit Descriptor control */
	TxflMASK	= 0x0000FFFF,	/* Transmit Frame Length */
	TxflSHIFT	= 0,
	Tcps		= 0x00010000,	/* TCP Checksum Offload */
	Udpcs		= 0x00020000,	/* UDP Checksum Offload */
	Ipcs		= 0x00040000,	/* IP Checksum Offload */
	Lgsen		= 0x08000000,	/* Large Send */
};

enum {					/* Receive Descriptor control */
	RxflMASK	= 0x00003FFF,	/* Receive Frame Length */
	RxflSHIFT	= 0,
	Tcpf		= 0x00004000,	/* TCP Checksum Failure */
	Udpf		= 0x00008000,	/* UDP Checksum Failure */
	Ipf		= 0x00010000,	/* IP Checksum Failure */
	Pid0		= 0x00020000,	/* Protocol ID0 */
	Pid1		= 0x00040000,	/* Protocol ID1 */
	Crce		= 0x00080000,	/* CRC Error */
	Runt		= 0x00100000,	/* Runt Packet */
	Res		= 0x00200000,	/* Receive Error Summary */
	Rwt		= 0x00400000,	/* Receive Watchdog Timer Expired */
	Fovf		= 0x00800000,	/* FIFO Overflow */
	Bovf		= 0x01000000,	/* Buffer Overflow */
	Bar		= 0x02000000,	/* Broadcast Address Received */
	Pam		= 0x04000000,	/* Physical Address Matched */
	Mar		= 0x08000000,	/* Multicast Address Received */
};

enum {					/* General Descriptor control */
	Ls		= 0x10000000,	/* Last Segment Descriptor */
	Fs		= 0x20000000,	/* First Segment Descriptor */
	Eor		= 0x40000000,	/* End of Descriptor Ring */
	Own		= 0x80000000,	/* Ownership */
};

/*
 */
enum {					/* Ring sizes  (<= 1024) */
	Ntd		= 8,		/* Transmit Ring */
	Nrd		= 32,		/* Receive Ring */

	Mps		= ROUNDUP(ETHERMAXTU+4, 128),
};

typedef struct Dtcc Dtcc;
struct Dtcc {
	u64int	txok;
	u64int	rxok;
	u64int	txer;
	u32int	rxer;
	u16int	misspkt;
	u16int	fae;
	u32int	tx1col;
	u32int	txmcol;
	u64int	rxokph;
	u64int	rxokbrd;
	u32int	rxokmu;
	u16int	txabt;
	u16int	txundrn;
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	uint	id;

	QLock	alock;			/* attach */
	Lock	ilock;			/* init */
	int	init;			/*  */

	Mii*	mii;

	Lock	tlock;			/* transmit */
	D*	td;			/* descriptor ring */
	Block**	tb;			/* transmit buffers */
	int	ntd;

	int	tdh;			/* head - producer index (host) */
	int	tdt;			/* tail - consumer index (NIC) */
	int	ntdfree;
	int	ntq;

	int	mtps;			/* Max. Transmit Packet Size */

	Lock	rlock;			/* receive */
	D*	rd;			/* descriptor ring */
	void**	rb;			/* receive buffers */
	int	nrd;

	int	rdh;			/* head - producer index (NIC) */
	int	rdt;			/* tail - consumer index (host) */
	int	nrdfree;

	int	rcr;			/* receive configuration register */

	QLock	slock;			/* statistics */
	Dtcc*	dtcc;
	uint	txdu;
	uint	tcpf;
	uint	udpf;
	uint	ipf;
	uint	fovf;
	uint	ierrs;
	uint	rer;
	uint	rdu;
	uint	punlc;
	uint	fovw;
} Ctlr;

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

#define csr8r(c, r)	(inb((c)->port+(r)))
#define csr16r(c, r)	(ins((c)->port+(r)))
#define csr32r(c, r)	(inl((c)->port+(r)))
#define csr8w(c, r, b)	(outb((c)->port+(r), (int)(b)))
#define csr16w(c, r, w)	(outs((c)->port+(r), (ushort)(w)))
#define csr32w(c, r, l)	(outl((c)->port+(r), (ulong)(l)))

static int
rtl8169miimir(Mii* mii, int pa, int ra)
{
	uint r;
	int timeo;
	Ctlr *ctlr;

	if(pa != 1)
		return -1;
	ctlr = mii->ctlr;

	r = (ra<<16) & RegaddrMASK;
	csr32w(ctlr, Phyar, r);
	delay(1);
	for(timeo = 0; timeo < 2000; timeo++){
		if((r = csr32r(ctlr, Phyar)) & Flag)
			break;
		microdelay(100);
	}
	if(!(r & Flag))
		return -1;

	return (r & DataMASK)>>DataSHIFT;
}

static int
rtl8169miimiw(Mii* mii, int pa, int ra, int data)
{
	uint r;
	int timeo;
	Ctlr *ctlr;

	if(pa != 1)
		return -1;
	ctlr = mii->ctlr;

	r = Flag|((ra<<16) & RegaddrMASK)|((data<<DataSHIFT) & DataMASK);
	csr32w(ctlr, Phyar, r);
	delay(1);
	for(timeo = 0; timeo < 2000; timeo++){
		if(!((r = csr32r(ctlr, Phyar)) & Flag))
			break;
		microdelay(100);
	}
	if(r & Flag)
		return -1;

	return 0;
}

static int
rtl8169mii(Ctlr* ctlr)
{
	MiiPhy *phy;

	/*
	 * Link management.
	 */
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->mir = rtl8169miimir;
	ctlr->mii->miw = rtl8169miimiw;
	ctlr->mii->ctlr = ctlr;
	rtl8169miimiw(ctlr->mii, 1, 0x0B, 0x0000);	/* magic */

	if(mii(ctlr->mii, ~0) == 0 || (phy = ctlr->mii->curphy) == nil){
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	print("oui %X phyno %d\n", phy->oui, phy->phyno);

	miiane(ctlr->mii, ~0, ~0, ~0);

	return 0;
}

static int
rtl8169reset(Ctlr* ctlr)
{
	int timeo;

	/*
	 * Soft reset the controller.
	 */
	csr8w(ctlr, Cr, Rst);
	for(timeo = 0; timeo < 1000; timeo++){
		if(!(csr8r(ctlr, Cr) & Rst))
			return 0;
		delay(1);
	}

	return -1;
}

static void
rtl8169detach(Ether* edev)
{
	rtl8169reset(edev->ctlr);
}

static void
rtl8169halt(Ctlr* ctlr)
{
	csr8w(ctlr, Cr, 0);
	csr16w(ctlr, Imr, 0);
	csr16w(ctlr, Isr, ~0);
}

static void
rtl8169replenish(Ctlr* ctlr)
{
	D *d;
	int rdt;
	void *bp;

	rdt = ctlr->rdt;
	while(NEXT(rdt, ctlr->nrd) != ctlr->rdh){
		d = &ctlr->rd[rdt];
		if(ctlr->rb[rdt] == nil){
			/*
			 * simple allocation for now
			 */
			bp = malloc(Mps);
			ctlr->rb[rdt] = bp;
			d->addrlo = PCIWADDR(bp);
			d->addrhi = 0;
		}
		coherence();
		d->control |= Own|Mps;
		rdt = NEXT(rdt, ctlr->nrd);
		ctlr->nrdfree++;
	}
	ctlr->rdt = rdt;
}

static void
rtl8169init(Ether* edev)
{
	uint r;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	ilock(&ctlr->ilock);

	rtl8169halt(ctlr);

	/*
	 * Setting Mulrw in Cplusc disables the Tx/Rx DMA burst settings
	 * in Tcr/Rcr.
	 */
	csr16w(ctlr, Cplusc, (1<<14)|Rxchksum|Mulrw);	/* magic (1<<14) */

	/*
	 * MAC Address.
	 * Must put chip into config register write enable mode.
	 */
	csr8w(ctlr, Cr9346, Eem1|Eem0);
	r = (edev->ea[3]<<24)|(edev->ea[2]<<16)|(edev->ea[1]<<8)|edev->ea[0];
	csr32w(ctlr, Idr0, r);
	r = (edev->ea[5]<<8)|edev->ea[4];
	csr32w(ctlr, Idr0+4, r);

	/*
	 * Enable receiver/transmitter.
	 * Need to do this first or some of the settings below
	 * won't take.
	 */
	csr8w(ctlr, Cr, Te|Re);

	/*
	 * Transmitter.
	 * Mtps is in units of 128.
	 */
	memset(ctlr->td, 0, sizeof(D)*ctlr->ntd);
	ctlr->tdh = ctlr->tdt = 0;
	ctlr->td[ctlr->ntd-1].control = Eor;
	ctlr->mtps = HOWMANY(Mps, 128);

	/*
	 * Receiver.
	 */
	memset(ctlr->rd, 0, sizeof(D)*ctlr->nrd);
	ctlr->rdh = ctlr->rdt = 0;
	ctlr->rd[ctlr->nrd-1].control = Eor;

	//for(i = 0; i < ctlr->nrd; i++){
	//	if((bp = ctlr->rb[i]) != nil){
	//		ctlr->rb[i] = nil;
	//		freeb(bp);
	//	}
	//}
	rtl8169replenish(ctlr);
	ctlr->rcr = Rxfth256|Mrxdmaunlimited|Ab|Apm;

	/*
	 * Interrupts.
	 * Disable Tdu|Tok for now, the transmit routine will tidy.
	 * Tdu means the NIC ran out of descritors to send, so it
	 * doesn't really need to ever be on.
	 */
	csr32w(ctlr, Timerint, 0);
	csr16w(ctlr, Imr, Serr|Timeout/*|Tdu*/|Fovw|Punlc|Rdu|Ter/*|Tok*/|Rer|Rok);

	/*
	 * Clear missed-packet counter;
	 * initial early transmit threshold value;
	 * set the descriptor ring base addresses;
	 * set the maximum receive packet size;
	 * no early-receive interrupts.
	 */
	csr32w(ctlr, Mpc, 0);
	csr8w(ctlr, Mtps, ctlr->mtps);
	csr32w(ctlr, Tnpds+4, 0);
	csr32w(ctlr, Tnpds, PCIWADDR(ctlr->td));
	csr32w(ctlr, Rdsar+4, 0);
	csr32w(ctlr, Rdsar, PCIWADDR(ctlr->rd));
	csr16w(ctlr, Rms, Mps);
	csr16w(ctlr, Mulint, 0);

	/*
	 * Set configuration.
	 */
	csr32w(ctlr, Tcr, Ifg1|Ifg0|Mtxdmaunlimited);
	csr32w(ctlr, Rcr, ctlr->rcr);
	csr16w(ctlr, 0xE2, 0);			/* magic */

	csr8w(ctlr, Cr9346, 0);

	iunlock(&ctlr->ilock);

	//rtl8169mii(ctlr);
}

static void
rtl8169attach(Ether* edev)
{
	int timeo;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->init == 0){
		/*
		 * Handle allocation/init errors here.
		 */
		ctlr->td = xspanalloc(sizeof(D)*Ntd, 256, 0);
		ctlr->tb = malloc(Ntd*sizeof(Block*));
		ctlr->ntd = Ntd;
		ctlr->rd = xspanalloc(sizeof(D)*Nrd, 256, 0);
		ctlr->rb = malloc(Nrd*sizeof(Block*));
		ctlr->nrd = Nrd;
		ctlr->dtcc = xspanalloc(sizeof(Dtcc), 64, 0);
		rtl8169init(edev);
		ctlr->init = 1;
	}
	qunlock(&ctlr->alock);

	for(timeo = 0; timeo < 3500; timeo++){
		if(miistatus(ctlr->mii) == 0)
			break;
		delay(10);
	}
}

static void
rtl8169transmit(Ether* edev)
{
	D *d;
	Block *bp;
	Ctlr *ctlr;
	int control, x;
	RingBuf *tb;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);
	for(x = ctlr->tdh; ctlr->ntq > 0; x = NEXT(x, ctlr->ntd)){
		d = &ctlr->td[x];
		if((control = d->control) & Own)
			break;

		/*
		 * Check errors and log here.
		 */
		USED(control);

		/*
		 * Free it up.
		 * Need to clean the descriptor here? Not really.
		 * Simple freeb for now (no chain and freeblist).
		 * Use ntq count for now.
		 */
		freeb(ctlr->tb[x]);
		ctlr->tb[x] = nil;
		d->control &= Eor;

		ctlr->ntq--;
	}
	ctlr->tdh = x;

	x = ctlr->tdt;
	while(ctlr->ntq < (ctlr->ntd-1)){
		tb = &edev->tb[edev->ti];
		if(tb->owner != Interface)
			break;

		bp = allocb(tb->len);
		memmove(bp->wp, tb->pkt, tb->len);
		memmove(bp->wp+Eaddrlen, edev->ea, Eaddrlen);
		bp->wp += tb->len;

		tb->owner = Host;
		edev->ti = NEXT(edev->ti, edev->ntb);

		d = &ctlr->td[x];
		d->addrlo = PCIWADDR(bp->rp);
		d->addrhi = 0;
		ctlr->tb[x] = bp;
		coherence();
		d->control |= Own|Fs|Ls|((BLEN(bp)<<TxflSHIFT) & TxflMASK);

		x = NEXT(x, ctlr->ntd);
		ctlr->ntq++;
	}
	if(x != ctlr->tdt){
		ctlr->tdt = x;
		csr8w(ctlr, Tppoll, Npq);
	}
	else if(ctlr->ntq >= (ctlr->ntd-1))
		ctlr->txdu++;

	iunlock(&ctlr->tlock);
}

static void
rtl8169receive(Ether* edev)
{
	D *d;
	int len, rdh;
	Ctlr *ctlr;
	u32int control;
	RingBuf *ring;

	ctlr = edev->ctlr;

	rdh = ctlr->rdh;
	for(;;){
		d = &ctlr->rd[rdh];
	
		if(d->control & Own)
			break;

		control = d->control;
		if((control & (Fs|Ls|Res)) == (Fs|Ls)){
			len = ((control & RxflMASK)>>RxflSHIFT) - 4;

			ring = &edev->rb[edev->ri];
			if(ring->owner == Interface){
				ring->owner = Host;
				ring->len = len;
				memmove(ring->pkt, ctlr->rb[rdh], len);
				edev->ri = NEXT(edev->ri, edev->nrb);
			}
		}
		else{
			/*
			 * Error stuff here.
			print("control %8.8uX\n", control);
			 */
		}
		d->control &= Eor;
		ctlr->nrdfree--;
		rdh = NEXT(rdh, ctlr->nrd);
	}
	ctlr->rdh = rdh;
	
	if(ctlr->nrdfree < ctlr->nrd/2)
		rtl8169replenish(ctlr);
}

static void
rtl8169interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	u32int isr;

	edev = arg;
	ctlr = edev->ctlr;

	while((isr = csr16r(ctlr, Isr)) != 0){
		csr16w(ctlr, Isr, isr);
		if(isr & (Fovw|Punlc|Rdu|Rer|Rok)){
			rtl8169receive(edev);
			if(!(isr & (Punlc|Rok)))
				ctlr->ierrs++;
			if(isr & Rer)
				ctlr->rer++;
			if(isr & Rdu)
				ctlr->rdu++;
			if(isr & Punlc)
				ctlr->punlc++;
			if(isr & Fovw)
				ctlr->fovw++;
			isr &= ~(Fovw|Rdu|Rer|Rok);
		}

		if(isr & (Tdu|Ter|Tok)){
			rtl8169transmit(edev);
			isr &= ~(Tdu|Ter|Tok);
		}

		if(isr & Punlc){
			//rtl8169link(edev);
			isr &= ~Punlc;
		}

		/*
		 * Some of the reserved bits get set sometimes...
		 */
		if(isr & (Serr|Timeout|Tdu|Fovw|Punlc|Rdu|Ter|Tok|Rer|Rok))
			panic("rtl8169interrupt: imr %4.4uX isr %4.4uX\n",
				csr16r(ctlr, Imr), isr);
	}
}

static Ctlr*
rtl8169match(Ether* edev, int id)
{
	Pcidev *p;
	Ctlr *ctlr;
	int port;

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		p = ctlr->pcidev;
		if(((p->did<<16)|p->vid) != id)
			continue;
		port = p->mem[0].bar & ~0x01;
		if(edev->port != 0 && edev->port != port)
			continue;

		ctlr->port = port;
		if(rtl8169reset(ctlr))
			continue;

		csr8w(ctlr, 0x82, 1);		/* magic */

		rtl8169mii(ctlr);

		pcisetbme(p);
		ctlr->active = 1;
		return ctlr;
	}
	return nil;
}

static struct {
	char*	name;
	int	id;
} rtl8169pci[] = {
	{ "rtl8169",	(0x8169<<16)|0x10EC, },	/* generic */
	{ "CG-LAPCIGT",	(0xC107<<16)|0x1259, },	/* Corega CG-LAPCIGT */
	{ nil },
};

int
rtl8169pnp(Ether* edev)
{
	Pcidev *p;
	Ctlr *ctlr;
	int i, id;
	uchar ea[Eaddrlen];

	/*
	 * Make a list of all ethernet controllers
	 * if not already done.
	 */
	if(ctlrhead == nil){
		p = nil;
		while(p = pcimatch(p, 0, 0)){
			if(p->ccrb != 0x02 || p->ccru != 0)
				continue;
			ctlr = malloc(sizeof(Ctlr));
			ctlr->pcidev = p;
			ctlr->id = (p->did<<16)|p->vid;

			if(ctlrhead != nil)
				ctlrtail->next = ctlr;
			else
				ctlrhead = ctlr;
			ctlrtail = ctlr;
		}
	}

	/*
	 * Is it an RTL8169 under a different name?
	 * Normally a search is made through all the found controllers
	 * for one which matches any of the known vid+did pairs.
	 * If a vid+did pair is specified a search is made for that
	 * specific controller only.
	 */
	id = 0;
	for(i = 0; i < edev->nopt; i++){
		if(cistrncmp(edev->opt[i], "id=", 3) == 0)
			id = strtol(&edev->opt[i][3], nil, 0);
	}

	ctlr = nil;
	if(id != 0)
		ctlr = rtl8169match(edev, id);
	else for(i = 0; rtl8169pci[i].name; i++){
		if((ctlr = rtl8169match(edev, rtl8169pci[i].id)) != nil)
			break;
	}
	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;

	/*
	 */
	memset(ea, 0, Eaddrlen);
	i = csr32r(ctlr, Idr0);
	edev->ea[0] = i;
	edev->ea[1] = i>>8;
	edev->ea[2] = i>>16;
	edev->ea[3] = i>>24;
	i = csr32r(ctlr, Idr0+4);
	edev->ea[4] = i;
	edev->ea[5] = i>>8;

	edev->attach = rtl8169attach;
	edev->transmit = rtl8169transmit;
	edev->interrupt = rtl8169interrupt;
	edev->detach = rtl8169detach;

	return 0;
}
