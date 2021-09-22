/*
 * Broadcom BCM4401.
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

enum {						/* Broadcom Registers */
	Bdctl		= 0x0000,		/* Device Control */
	Bis		= 0x0020,		/* Interrupt Status */
	Bim		= 0x0024,		/* Interrupt Mask */
	Bmctl		= 0x00A8,		/* MAC Control */
	Bmfctl		= 0x00AC,		/* MAC Flow Control */
	Blictl		= 0x0100,		/* Lazy Interrupt Control */

	TDMActl		= 0x0200,		/* Tx DMA Control */
	TDMAra		= 0x0204,		/* Tx DMA Ring Address */
	TDMAld		= 0x0208,		/* Tx DMA Last Descriptor */
	TDMAstatus	= 0x020C,		/* Tx DMA Status */

	RDMActl		= 0x0210,		/* Rx DMA Control */
	RDMAra		= 0x0214,		/* Rx DMA Ring Address */
	RDMAld		= 0x0218,		/* Rx DMA Last Descriptor */
	RDMAstatus	= 0x021C,		/* Rx DMA Status */

	Rxcfg		= 0x0400,		/* Rx Config */
	Rxmax		= 0x0404,		/* Rx Max. Packet Length */
	Txmax		= 0x0408,		/* Tx Max. Packet Length */
	MDIOctl		= 0x0410,		/* */
	MDIOdata	= 0x0414,		/* */
	MDIOstatus	= 0x041C,		/* */
	CAMdlo		= 0x0420,		/* CAM Data Low */
	CAMdhi		= 0x0424,		/* CAM Data High */
	CAMctl		= 0x0428,		/* CAM Control */
	EMACctl		= 0x042C,		/* */
	EMACtxctl	= 0x0430,		/* Transmit Control */
	EMACtxwmark	= 0x0434,		/* Transmit Watermark */
	MIBctl		= 0x0438,		/* */
	MIB		= 0x0500,		/* */

	Eeprom		= 0x1000,		/* EEPROM base */
	Eao		= Eeprom+78,		/* MAC address offset */
	Pao		= Eeprom+90,		/* Phy address offset */
};

enum {						/* Bdctl */
	Bpfe		= 0x00000080,		/* Pattern Filtering Enable */
	Bipp		= 0x00000400,		/* Internal PHY Present */
	Bpme		= 0x00001000,		/* PHY Mode Enable */
	Bpmce		= 0x00002000,		/* PHY Mode Clock Enable */
	Bepr		= 0x00008000,		/* External PHY Reset */
};

enum {						/* Bis/Bim */
	Ilc		= 0x00000020,		/* Link Change */
	Ipme		= 0x00000040,		/* Power Management Event */
	Igpt		= 0x00000080,		/* General Purpose Timeout */
	Idesce		= 0x00000400,		/* Descriptor Error */
	Idatae		= 0x00000800,		/* Data Error */
	Idpe		= 0x00001000,		/* Descriptor Protocol Error */
	Irdu		= 0x00002000,		/* Rx Descriptor Underflow */
	Irfo		= 0x00004000,		/* Rx FIFO Overflow */
	Itfu		= 0x00008000,		/* Tx FIFO Underflow */
	Ir		= 0x00010000,		/* Rx */
	It		= 0x01000000,		/* Tx */
	Ie		= 0x04000000,		/* EMAC */
	Imw		= 0x08000000,		/* MII Write */
	Imr		= 0x10000000,		/* MII Read */

	Ierror		= Itfu|Irfo|Irdu|Idpe|Idatae|Idesce,
};

enum {						/* Bmctl */
	Mcge		= 0x00000001,		/* CRC32 Generation Enable */
	Mppd		= 0x00000004,		/* Onchip PHY Power Down */
	Mped		= 0x00000008,		/* Onchip PHY Energy Detected */
	MlcMASK		= 0x000000E0,		/* Onchip PHY LED Control */
	MlcSHIFT	= 5,
};

enum {						/* Bmfctl */
	MrhiwatMASK	= 0x000000FF,		/* Rx FIFO High Watermark */
	MrhiwatSHIFT	= 0,
	Mgpf		= 0x00008000,		/* Generate Pause Frames */
};

enum {						/* Blictl */
	MtoMASK		= 0x00FFFFFF,		/* Timeout */
	MtoSHIFT	= 0,
	MfcMASK		= 0xFF000000,		/* Frame Count */
	MfcSHIFT	= 24,
};

enum {						/* TDMActl */
	Te		= 0x00000001,		/* Enable */
};

enum {						/* RDMActl */
	Re		= 0x00000001,		/* Enable */
	RfoMASK		= 0x000000FE,		/* Receive Frame Offset */
	RfoSHIFT	= 1,
};

enum {						/* RDMAstatus */
	DcdMASK		= 0x00000FFF,		/* Current Descriptor */
	DcdSHIFT	= 0,
	Dactive		= 0x00001000,		/* Active */
	Didle		= 0x00002000,		/* Idle */
	Dstopped	= 0x00003000,		/* Stopped */
	Dsm		= 0x0000F000,		/* State Mask */
	Dem		= 0x000F0000,		/* Error mask */
};

enum {						/* MDIOctl */
	Mfreq		= 0x0000000D,		/* MDC Frequency (62.5MHz) */
	Mpe		= 0x00000080,		/* Preamble Enable */
};

enum {						/* MDIOdata */
	Mack		= 0x00020000,		/* Ack */
	Mwop		= 0x10000000,		/* Write Op. */
	Mrop		= 0x20000000,		/* Read Op. */
	Msof		= 0x40000000,		/* Start of Frame */
};

enum {						/* MDIOstatus */
	Mmi		= 0x00000001,		/* MDIO Interrupt */
};

enum {						/* Rxcfg */
	Rbd		= 0x00000001,		/* Broadcast Disable */
	Raam		= 0x00000002,		/* Accept All Multicast */
	Rdtx		= 0x00000004,		/* Disable while Transmitting */
	Rpe		= 0x00000008,		/* Promiscuous Enable */
	Rle		= 0x00000010,		/* Loopback Enable */
	Rfce		= 0x00000020,		/* Flow Control Enable */
	Rafcf		= 0x00000040,		/* Accept Unicast FC Frame */
	Rrf		= 0x00000080,		/* Reject Filter */
};

enum {						/* CAMctl */
	Ce		= 0x00000001,		/* Enable */
	Cms		= 0x00000002,		/* Mask Select */
	Cr		= 0x00000004,		/* Read */
	Cw		= 0x00000008,		/* Write */
	CiMASK		= 0x003F0000,		/* Index Mask */
	CiSHIFT		= 16,
	Cbusy		= 0x80000000,		/* Busy */
};

enum {						/* CAMdhi */
	Cv		= 0x00010000,		/* Valid */
};

enum {						/* EMACctl */
	Eena		= 0x00000001,		/* Enable */
	Edis		= 0x00000002,		/* Disable */
	Erst		= 0x00000004,		/* Reset */
	Eeps		= 0x00000008,		/* External PHY Select */
};

enum {						/* EMACtxctl */
	Efd		= 0x00000001,		/* Fulle Duplex */
	Efc		= 0x00000002,		/* Flow Control */
	Esbe		= 0x00000004,		/* Single Backoff Enable */
	Ess		= 0x00000008,		/* Small Slot-time */
};

enum {						/* MIBctl */
	MIBcor		= 0x00000001,		/* Clear On Read */
};

enum {						/* SonicsBackplane Registers */
	SBs2pci2	= 0x0108,		/* Sonics to PCI translation */
	SBias		= 0x0F90,		/* Initiator Agent State */
	SBim		= 0x0F94,		/* Interrupt Mask */
	SBtslo		= 0x0F98,		/* Target State Low */
	SBtshi		= 0x0F9C,		/* Target State High */
};

enum {						/* SBs2pci2 */
	Spref		= 0x00000004,		/* Prefetch Enable */
	Sburst		= 0x00000008,		/* Burst Enable */
};

enum {						/* SBias */
	Sibe		= 0x00020000,		/* In-Band Error */
	Sto		= 0x00040000,		/* Timeout */
};

enum {						/* SBim */
	Senet0		= 0x00000002,		/* Ethernet 0 */
};

enum {						/* SBtslo */
	Sreset		= 0x00000001,		/*  */
	Sreject		= 0x00000002,		/*  */
	Sclock		= 0x00010000,		/*  */
	Sfgc		= 0x00020000,		/* Force Gated Clocks On */
};

enum {						/* SBtshi */
	Sserr		= 0x00000001,		/*  */
	Sbusy		= 0x00000004,		/*  */
};

// dma descriptor. must be 4096 aligned and fit in single 4096 page.
// rx and tx descriptors can't share the same page apparently so we'll
// have to malloc that even though we don't use it all.
// descriptors are only ever read by the nic, not written back.
typedef struct {
	u32int	control;
	u32int	address;
} DD;

enum {						/* DD.control */
	DDbbcMASK	= 0x00001FFF,		/* Buffer Byte Count */
	DDbbcSHIFT	= 0,
	DDeot		= 0x10000000,		/* End of Descriptor Table */
	DDioc		= 0x20000000,		/* Interrupt on Completion */
	DDeof		= 0x40000000,		/* End of Frame */
	DDsof		= 0x80000000,		/* Start of Frame */
};

// the core returns 8 bytes - 4 bytes length and status
// and 4 bytes undefined.
// so we pad for alignment of the data...? but why in the struct
// since the offset is defined in a register and the struct is
// 28 and the offset is usually set to 30?
// bollocks. it looks to me like the status written is 28 bytes.
typedef struct {
	u16int	length;
	u16int	status;
	uchar	undefined[4];
	uchar	pad[20];

//	void*	bp;				/* software */

//	uchar	data[];				/* flexible array member... */
} H;

enum {						/* H.status */
	Hfo		= 0x0001,		/* FIFO Overflow */
	Hce		= 0x0002,		/* CRC Error */
	Hse		= 0x0004,		/* Symbol Error */
	Hodd		= 0x0008,		/* Odd Number of Nibbles */
	Hl		= 0x0010,		/* Frame too Large */
	Hmf		= 0x0020,		/* Multicast Frame */
	Hbf		= 0x0040,		/* Broadcast Frame */
	Hpf		= 0x0080,		/* Promiscuous Frame */
	Hlbf		= 0x0800,		/* Last Buffer in Frame */
};

enum {
	Ncam		= 64,
	Nmib		= 55,

	SBPCIWADDR	= 0x40000000,

	Nrd		= 128,
	Ntd		= 64,

	Rbhsz		= 30,			// er...
	Rbdsz		= 1550,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	u32int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;
	uchar	pa[Eaddrlen];

	void*	nic;

	QLock	alock;				/* attach */
	void*	alloc;				/*  */

	u32int	im;
	u32int	rcr;

	Lock	tlock;				/* transmit */
	DD*	td;				/* descriptor ring */
	Block**	tb;				/* transmit buffers */
	int	ntd;

	int	tdh;				/* head - producer index (host) */
	int	tdt;				/* tail - consumer index (NIC) */
	int	ntdfree;
	int	ntq;

	Lock	rlock;				/* receive */
	DD*	rd;				/* descriptor ring */
	Block**	rb;				/* receive buffers */
	int	nrd;

	int	rdh;				/* head - producer index (NIC) */
	int	rdt;				/* tail - consumer index (host) */
	int	nrdfree;


	DD*	tddr;

	Mii*	mii;

	uint	intr;				/* statistics */
	uint	lintr;			
	uint	lsleep;
	uint	rintr;
	uint	tintr;

	uint	txdu;

	uint	mib[Nmib];
} Ctlr;

#define csr8r(c, r)	(*((u8int*)((c)->nic)+(r)))
#define csr16r(c, r)	(*((u16int*)((c)->nic)+((r)/2)))
#define csr32p(c, r)	((u32int*)((c)->nic)+((r)/4))
#define csr32r(c, r)	(*csr32p(c, r))
#define csr32w(c, r, v)	(*csr32p(c, r) = (v))
#define csr32a(c, r)	{ u32int dummy = csr32r(c, r); USED(dummy);}
// hmmm. maybe we need a csr32wrb instead, write and read back?

static Ctlr* bcm4401ctlrhead;
static Ctlr* bcm4401ctlrtail;

static char* statistics[Nmib] = {
	"Tx Good Octets",
	"Tx Good Pkts",
	"Tx Octets",
	"Tx Pkts",
	"Tx Broadcast Pkts",
	"Tx Multicast Pkts",
	"Tx (64)",
	"Tx (65-127)",
	"Tx (128-255)",
	"Tx (256-511)",
	"Tx (512-1023)",
	"Tx (1024-max)",
	"Tx Jabber Pkts",
	"Tx Oversize Pkts",
	"Tx Fragment Pkts",
	"Tx Underruns",
	"Tx Total Cols",
	"Tx Single Cols",
	"Tx Multiple Cols",
	"Tx Excessive Cols",
	"Tx Late Cols",
	"Tx Deferred",
	"Tx Carrier Lost",
	"Tx Pause Pkts",

	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,

	"Rx Good Octets",
	"Rx Good Pkts",
	"Rx Octets",
	"Rx Pkts",
	"Rx Broadcast Pkts",
	"Rx Multicast Pkts",
	"Rx (64)",
	"Rx (65-127)",
	"Rx (128-255)",
	"Rx (256-511)",
	"Rx (512-1023)",
	"Rx (1024-Max)",
	"Rx Jabber Pkts",
	"Rx Oversize Pkts",
	"Rx Fragment Pkts",
	"Rx Missed Pkts",
	"Rx CRC Align Errs",
	"Rx Undersize",
	"Rx CRC Errs",
	"Rx Align Errs",
	"Rx Symbol Errs",
	"Rx Pause Pkts",
	"Rx Non-pause Pkts",
};

static void
bcm4401mib(Ctlr* ctlr, uint* mib)
{
	int i;
	u32int r;

	csr32w(ctlr, MIBctl, MIBcor);
	for(i = 0; i < Nmib; i++){
		r = csr32r(ctlr, MIB+i*sizeof(u32int));
		if(mib == nil)
			continue;
		if(statistics[i] != nil)
			*mib += r;
		mib++;
	}
}

static long
bcm4401ifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *p, *s;
	int i, l, r;

	ctlr = edev->ctlr;

	p = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	l = 0;
	bcm4401mib(ctlr, ctlr->mib);
	for(i = 0; i < Nmib; i++){
		if((s = statistics[i]) == nil)
			continue;
		l += snprint(p+l, READSTR-l, "%s: %ud\n", s, ctlr->mib[i]);
	}
	l += snprint(p+l, READSTR-l, "intr: %ud\n", ctlr->intr);
	l += snprint(p+l, READSTR-l, "lintr: %ud\n", ctlr->lintr);
	l += snprint(p+l, READSTR-l, "lsleep: %ud\n", ctlr->lsleep);
	l += snprint(p+l, READSTR-l, "rintr: %ud\n", ctlr->rintr);
	l += snprint(p+l, READSTR-l, "tintr: %ud\n", ctlr->tintr);
	l += snprint(p+l, READSTR-l, "txdu: %ud\n", ctlr->txdu);

	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		l += snprint(p+l, READSTR, "phy:   ");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				l += snprint(p+l, READSTR-l, "\n       ");
			r = miimir(ctlr->mii, i);
			l += snprint(p+l, READSTR-l, " %4.4ux", r);
		}
		snprint(p+l, READSTR-l, "\n");
	}
	snprint(p+l, READSTR-l, "\n");

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
_bcm4401promiscuous(Ctlr* ctlr, int on)
{
	u32int r;

	r = csr32r(ctlr, CAMctl);
	if(on){
		ctlr->rcr |= Rpe;
		r &= ~Ce;
	}
	else{
		ctlr->rcr &= ~Rpe;
		r |= Ce;
	}
	csr32w(ctlr, Rxcfg, ctlr->rcr);
	csr32w(ctlr, CAMctl, r);
}

static void
bcm4401promiscuous(void* arg, int on)
{
	_bcm4401promiscuous(((Ether*)arg)->ctlr, on);
}

/* multicast already on, don't need to do anything */
static void
bcm4401multicast(void*, uchar*, int)
{
}

static void
bcm4401attach(Ether* edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){
		qunlock(&ctlr->alock);
		return;
	}

	if(waserror()){
		qunlock(&ctlr->alock);
		nexterror();
	}

	qunlock(&ctlr->alock);
	poperror();
}

static void
bcm4401transmit(Ether* edev)
{
	DD *d;
	Block *bp;
	Ctlr *ctlr;
	int control, s, tdh, tdt;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);
	s = csr32r(ctlr, TDMAstatus);
	tdt = ((s & DcdMASK) >> DcdSHIFT)/sizeof(DD);
	for(tdh = ctlr->tdh; tdh != tdt; tdh = NEXT(tdh, ctlr->ntd)){
		d = &ctlr->td[tdh];

		/*
		 * Check errors and log here.
		 */
		SET(control);
		USED(control);
		// seems to be no way to tell whether the packet
		// went out or not or had errors?

		/*
		 * Free it up.
		 * Need to clean the descriptor here? Not really.
		 * Simple freeb for now (no chain and freeblist).
		 * Use ntq count for now.
		 */
		freeb(ctlr->tb[tdh]);
		ctlr->tb[tdh] = nil;
		d->control &= DDeot;
		d->address = 0;

		ctlr->ntq--;
	}
	ctlr->tdh = tdh;

	tdt = ctlr->tdt;
	while(ctlr->ntq < (ctlr->ntd-1)){
		if((bp = qget(edev->oq)) == nil)
			break;

		d = &ctlr->td[tdt];
		d->address = PCIWADDR(bp->rp) + SBPCIWADDR;
		ctlr->tb[tdt] = bp;
		d->control |= DDsof|DDeof|((BLEN(bp)<<DDbbcSHIFT) & DDbbcMASK);
//d->control |= DDioc;

		tdt = NEXT(tdt, ctlr->ntd);
		ctlr->ntq++;
	}
	if(tdt != ctlr->tdt){
		ctlr->tdt = tdt;
		coherence();
		csr32w(ctlr, TDMAld, tdt*sizeof(DD));
	}
	else if(ctlr->ntq >= (ctlr->ntd-1))
		ctlr->txdu++;

	iunlock(&ctlr->tlock);
}

static void
bcm4401replenish(Ctlr* ctlr)
{
	DD *d;
	int rdt;
	Block *bp;
	H* h;

	rdt = ctlr->rdt;
	while(NEXT(rdt, ctlr->nrd) != ctlr->rdh){
		d = &ctlr->rd[rdt];
		if((bp = ctlr->rb[rdt]) == nil){
			/*
			 * simple allocation for now
			 */
			bp = iallocb(Rbdsz);
			if(bp == nil){
				iprint("bcm4401: no available buffers\n");
				break;
			}
			ctlr->rb[rdt] = bp;
		}
		d->address = PCIWADDR(bp->rp) + SBPCIWADDR;
		d->control |= ((Rbdsz<<DDbbcSHIFT) & DDbbcMASK);
		coherence();
		h = (H*)bp->rp;
		h->length = 0;
		h->status = 0;
		//h->bp = bp;
		rdt = NEXT(rdt, ctlr->nrd);
		ctlr->nrdfree++;
	}
	ctlr->rdt = rdt;
}

static void
bcm4401receive(Ether* edev)
{
	DD *d;
	H* h;
	Block *bp;
	int rdh, rdt, s;
	Ctlr *ctlr;
	u16int status;

	ctlr = edev->ctlr;

	s = csr32r(ctlr, RDMAstatus);
	rdt = ((s & DcdMASK) >> DcdSHIFT)/sizeof(DD);

	for(rdh = ctlr->rdh; rdh != rdt; rdh = NEXT(rdh, ctlr->nrd)){
		d = &ctlr->rd[rdh];
		bp = ctlr->rb[rdh];
		h = (H*)bp->rp;

		status = h->status;
		if(h->length == 0 || h->length >(Rbdsz-Rbhsz)){
			print("status %ux l %d rdh %d rdt %d\n",
				status, h->length, rdh, rdt);
			continue;
		}

		if(!(status & (Hl|Hodd|Hse|Hce|Hfo)) && h->length != 0){
			ctlr->rb[rdh] = nil;
			bp->rp += Rbhsz;
			bp->wp = bp->rp + h->length - 4;
			bp->next = nil;

			etheriq(edev, bp, 1);

		}
		else{
			/*
			 * Error stuff here.
			 */
			print("status %ux l %d rdh %d rdt %d\n",
				status, h->length, rdh, rdt);
		}
		d->address = 0;
		d->control &= DDeot;
		ctlr->nrdfree--;
	}
	ctlr->rdh = rdh;
	
	if(ctlr->nrdfree < ctlr->nrd/2)
		bcm4401replenish(ctlr);
}

static void
bcm4401link(Ether* edev)
{
	u32int r;
	Ctlr *ctlr;
	MiiPhy *phy;

	ctlr = edev->ctlr;
	phy = ctlr->mii->curphy;

	r = csr32r(ctlr, EMACtxctl);
	if(phy->fd)
		r |= Efd;
	else
		r &= ~Efd;
	csr32w(ctlr, EMACtxctl, r);

	r = csr32r(ctlr, Rxcfg);
	if(phy->rfc)
		r |= Rfce;
	else
		r &= ~Rfce;
	csr32w(ctlr, Rxcfg, r);

	r = csr32r(ctlr, Bmfctl);
	if(phy->tfc)
		r = Mgpf|(0xC0 & MrhiwatMASK);	// |= ?
	else
		r &= ~Mgpf;
	csr32w(ctlr, Bmfctl, r);
}

static void
bcm4401interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	u32int im, is;

	edev = arg;
	ctlr = edev->ctlr;

	ctlr->intr++;
	for(;;){
		is = csr32r(ctlr, Bis);
		im = csr32r(ctlr, Bim);
		is &= im;
		if(is == 0)
			break;
		csr32w(ctlr, Bis, is);
		csr32a(ctlr, Bis);

		if(is & Ir)
			bcm4401receive(edev);
		if(is & It)
			bcm4401transmit(edev);
		if(is & Ilc)
			bcm4401link(edev);
		if(is & ~(It|Ir|Ilc))
			panic("is %ux\n", is);
	}
}

static int
bcm4401spin(u32int* csr32, int bit, int set, int µs)
{
	int t;
	u32int r;

	if(µs < 10)
		µs = 10;

	for(t = 0; t < µs; t += 10){
		r = *csr32;
		if(set){
			if(r & bit)
				return 0;
		}
		else if(!(r & bit))
			return 0;

		microdelay(10);
	}

	return -1;
}

static void
bcm4401cam(Ctlr* ctlr, int cvix, uchar* a)
{
	csr32w(ctlr, CAMdlo, (a[2]<<24)|(a[3]<<16)|(a[4]<<8)|a[5]);
	csr32w(ctlr, CAMdhi, (cvix & Cv)|(a[0]<<8)|a[1]);
	csr32w(ctlr, CAMctl, ((cvix<<CiSHIFT) & CiMASK)|Cw);
	bcm4401spin(csr32p(ctlr, CAMctl), Cbusy, 0, 1000);
}

static void
bcm4401sbreset(Ctlr* ctlr)
{
	u32int bar0, r;

	/*
	 * If the core is running, stop it.
	 */
	r = csr32r(ctlr, SBtslo);
	if((r & (Sclock|Sreject|Sreset)) == Sclock){
		csr32w(ctlr, Blictl, 0);
		csr32w(ctlr, EMACctl, Edis);
		bcm4401spin(csr32p(ctlr, EMACctl), Edis, 0, 1000);

		csr32w(ctlr, TDMActl, 0);

		r = csr32r(ctlr, RDMAstatus);
		if(r & Dem)
			bcm4401spin(csr32p(ctlr, RDMAstatus), Didle, 1, 1000);
		csr32w(ctlr, RDMActl, 0);

		csr32w(ctlr, EMACctl, Erst);
	}
	else{
		// set up sonic pci
		// reg80 holds the window space accessed - 0x18000000 is
		// the emac, 0x18002000 are the pci registers
		bar0 = pcicfgr32(ctlr->pcidev, 0x80);
		pcicfgw32(ctlr->pcidev, 0x80, 0x18002000);

		r = pcicfgr32(ctlr->pcidev, SBim);
		r |= Senet0;
		pcicfgw32(ctlr->pcidev, SBim, r);

		r = pcicfgr32(ctlr->pcidev, SBs2pci2);
		r |= Sburst|Spref;
		pcicfgw32(ctlr->pcidev, SBs2pci2, r);
	
		pcicfgw32(ctlr->pcidev, 0x80, bar0);
	}

	// disable core
	// must return if core is already in reset
	// set the reject bit
	// spin until reject is set
	// spin until sbtmstatehigh.busy is clear
	// set reset and reject while enabling the clocks
	// leave reset and reject asserted
	r = csr32r(ctlr, SBtslo);
	if(!(r & Sreset)){
		csr32w(ctlr, SBtslo, Sclock|Sreject);
		bcm4401spin(csr32p(ctlr, SBtslo), Sreject, 1, 1000000);
		bcm4401spin(csr32p(ctlr, SBtshi), Sbusy, 0, 1000000);

		csr32w(ctlr, SBtslo, Sfgc|Sclock|Sreject|Sreset);
		csr32a(ctlr, SBtslo);
		microdelay(10);

		csr32w(ctlr, SBtslo, Sreject|Sreset);
		microdelay(10);
	}

	// core reset - initialisation, surely?
	/*
	 * Now do the initialization sequence.
	 */
	// set reset while enabling the clock and forcing them on
	// throughout the core
	// clear any error bits that may be o
	// clear reset and allow it to propagate throughout the core
	// leave clock enabled
	csr32w(ctlr, SBtslo, Sfgc|Sclock|Sreset);
	csr32a(ctlr, SBtslo);
	microdelay(1);
	if(csr32r(ctlr, SBtshi) & Sserr)
		csr32w(ctlr, SBtshi, 0);
	r = csr32r(ctlr, SBias);
	if(r & (Sto|Sibe))
		csr32w(ctlr, SBias, r & ~(Sto|Sibe));
	csr32w(ctlr, SBtslo, Sfgc|Sclock);
	csr32a(ctlr, SBtslo);
	microdelay(1);
	csr32w(ctlr, SBtslo, Sclock);
	csr32a(ctlr, SBtslo);
	microdelay(1);
}

static void
bcm4401shutdown(Ether *ether)
{
	bcm4401sbreset(ether->ctlr);
}

static int
bcm4401detach(Ctlr* ctlr)
{
	/*
	 * Soft reset the controller.
	 */
	bcm4401sbreset(ctlr);

	// need to clean up software state somewhere

	return 0;
}

static int
bcm4401init(Ctlr* ctlr)
{
	u32int r;

	// Clear the stats on reset.
	bcm4401mib(ctlr, nil);

	// Enable CRC32, set proper LED modes and power on PHY
	csr32w(ctlr, Bmctl, ((7<<MlcSHIFT) & MlcMASK)|Mcge);
	csr32w(ctlr, Blictl, ((1<<MfcSHIFT) & MfcMASK));

	// is this the max length of a packet or the max length of the buffer?
	// packet is 1522 (mtu+header+crc+vlantag = 1522), buffer is that +30
	// for prepended rx header
	csr32w(ctlr, Rxmax, Rbdsz);
	csr32w(ctlr, Txmax, 1522);
	// no explanation of this anywhere
	csr32w(ctlr, EMACtxwmark, 56);

	// this is software init - elsewhere before this (attach?)
	ctlr->ntd = Ntd;
	ctlr->td = mallocalign(4096/*sizeof(DD)*ctlr->ntd*/, 4096, 0, 0);
	ctlr->tb = malloc(ctlr->ntd*sizeof(Block*));
	if(ctlr->td == nil || ctlr->tb == nil) {
		free(ctlr->td);
		free(ctlr->tb);
		error(Enomem);
	}
	memset(ctlr->td, 0, 4096);
	ctlr->td[ctlr->ntd-1].control = DDeot;
	ctlr->tdh = ctlr->tdt = 0;
	ctlr->ntq = 0;

	// ring dma address is the phys address of the rx ring
	// plus where the sb core sees the phys memory base
	csr32w(ctlr, TDMAra, PCIWADDR(ctlr->td)+SBPCIWADDR);
	csr32w(ctlr, TDMActl, Te);

	// this is software init - elsewhere (attach?)
	ctlr->nrd = Nrd;
	ctlr->rd = mallocalign(4096/*sizeof(DD)*ctlr->nrd*/, 4096, 0, 0);
	ctlr->rb = malloc(ctlr->nrd*sizeof(void*));
	if(ctlr->rd == nil || ctlr->rb == nil) {
		free(ctlr->td);
		free(ctlr->tb);
		free(ctlr->rd);
		free(ctlr->rb);
		error(Enomem);
	}
	memset(ctlr->rd, 0, 4096);
	ctlr->rd[ctlr->nrd-1].control = DDeot;
	ctlr->rdh = ctlr->rdt = 0;
	bcm4401replenish(ctlr);

	csr32w(ctlr, RDMAra, PCIWADDR(ctlr->rd)+SBPCIWADDR);
	csr32w(ctlr, RDMAld, 0);

	// pre-packet header (usually 30)
	csr32w(ctlr, RDMActl, ((Rbhsz<<RfoSHIFT) & RfoMASK)|Re);
	csr32w(ctlr, RDMAld, (ctlr->nrd)*sizeof(DD));

	// this is done in mib read too - only need once
	csr32w(ctlr, MIBctl, MIBcor);

	// EMACctl? ENETctl?
	r = csr32r(ctlr, EMACctl);
	csr32w(ctlr, EMACctl, Eena|r);

	return 0;
}

static int
bcm4401miimir(Mii* mii, int pa, int ra)
{
	u32int r;
	int timeo;
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	csr32w(ctlr, MDIOstatus, 1);
	csr32w(ctlr, MDIOdata, Msof|Mrop|(pa<<23)|(ra<<18)|Mack);
	for(r = timeo = 0; timeo < 100; timeo++){
		if((r = csr32r(ctlr, MDIOstatus)) & Mmi)
			break;
		microdelay(10);
	}
	if(!(r & Mmi))
		return -1;

	return csr32r(ctlr, MDIOdata) & 0xFFFF;
}

static int
bcm4401miimiw(Mii* mii, int pa, int ra, int data)
{
	u32int r;
	int timeo;
	Ctlr *ctlr;

	ctlr = mii->ctlr;

	csr32w(ctlr, MDIOstatus, 1);
	csr32w(ctlr, MDIOdata, Msof|Mwop|(pa<<23)|(ra<<18)|Mack|data);
	for(r = timeo = 0; timeo < 100; timeo++){
		if((r = csr32r(ctlr, MDIOstatus)) & Mmi)
			break;
		microdelay(10);
	}
	if(!(r & Mmi))
		return -1;

	return csr32r(ctlr, MDIOdata) & 0xFFFF;
}

static int
bcm4401reset(Ctlr* ctlr)
{
	int i;
	u32int r;
	MiiPhy *phy;
	uchar ea[Eaddrlen];

	// disable ints - here or where?
	csr32w(ctlr, Bim, 0);
	csr32a(ctlr, Bim);
	
	if(bcm4401detach(ctlr) < 0)
		return -1;

	// make the phy accessible; depends on internal/external.
	csr32w(ctlr, MDIOctl, Mpe|Mfreq);
	r = csr32r(ctlr, Bdctl);
	if(!(r & Bipp))
		csr32w(ctlr, EMACctl, Eeps);
	else if(r & Bepr){
		csr32w(ctlr, Bdctl, r & ~Bepr);
		microdelay(100);
	}

	/*
	 * Read the MAC address.
	 */
	for(i = 0; i < Eaddrlen; i += 2){
		ctlr->pa[i] = csr8r(ctlr, Eao+i+1);
		ctlr->pa[i+1] = csr8r(ctlr, Eao+i);
	}

	// initialise the mac address and cam
	bcm4401cam(ctlr, Cv|0, ctlr->pa);
	memset(ea, 0, Eaddrlen);
	for(i = 1; i < Ncam; i++)
		bcm4401cam(ctlr, i, ea);

	// default initial Rxcfg. call promiscuous to set rcr and cam.
	ctlr->rcr = Raam;
	_bcm4401promiscuous(ctlr, 0);

	bcm4401init(ctlr);

	// default initial interrupt mask.
	ctlr->im = Ierror|It|Ir|Igpt|Ilc;
	csr32w(ctlr, Bim, ctlr->im);
	csr32a(ctlr, Bim);

	/*
	 * Link management.
	 */
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->mir = bcm4401miimir;
	ctlr->mii->miw = bcm4401miimiw;
	ctlr->mii->ctlr = ctlr;

	/*
	 * Read the PHY address.
	 */
	r = csr8r(ctlr, Pao);
	r = 1<<(r & 0x1F);
	if(mii(ctlr->mii, r) == 0 || (phy = ctlr->mii->curphy) == nil){
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	miireset(ctlr->mii);

	// need to do some phy-specific init somewhere - why not here?
	// enable activity led
	i = miimir(ctlr->mii, 26);
	miimiw(ctlr->mii, 26, i & 0x7FFF);
	// enable traffic meter led mode
	i = miimir(ctlr->mii, 27);
	miimiw(ctlr->mii, 27, i|0x0040);

	print("oui %#ux phyno %d\n", phy->oui, phy->phyno);

	// need to set rxconfig before this and then call promiscuous
	// later once we know the link settings
	// By default, auto-negotiate PAUSE
	miiane(ctlr->mii, ~0, ~0, ~0);
	// set rcr according to pause, etc.

	return 0;
}

static void
bcm4401pci(void)
{
	int i;
	Pcidev *p;
	void *mem;
	Ctlr *ctlr;
	u32int bar;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x170C<<16)|0x14E4:	/* BCM4401-B0 */
		case (0x4401<<16)|0x14E4:	/* BCM4401 */
		case (0x4402<<16)|0x14E4:	/* BCM440? */
			break;
		}

		bar = p->mem[0].bar & ~0x0F;
		mem = vmap(bar, p->mem[0].size);
		if(mem == 0){
			print("bcm4401: can't map %#ux\n", bar);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil) {
			vunmap(mem, p->mem[0].size);
			error(Enomem);
		}
		ctlr->port = bar;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;
		ctlr->nic = mem;

		if(pcigetpms(p) > 0){
			pcisetpms(p, 0);
	
			for(i = 0; i < 6; i++)
				pcicfgw32(p, PciBAR0+i*4, p->mem[i].bar);
			pcicfgw8(p, PciINTL, p->intl);
			pcicfgw8(p, PciLTR, p->ltr);
			pcicfgw8(p, PciCLS, p->cls);
			pcicfgw16(p, PciPCR, p->pcr);
		}

		if(bcm4401reset(ctlr)){
			vunmap(mem, p->mem[0].size);
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(bcm4401ctlrhead != nil)
			bcm4401ctlrtail->next = ctlr;
		else
			bcm4401ctlrhead = ctlr;
		bcm4401ctlrtail = ctlr;
	}
}

static int
bcm4401pnp(Ether* edev)
{
	Ctlr *ctlr;

	if(bcm4401ctlrhead == nil)
		bcm4401pci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = bcm4401ctlrhead; ctlr != nil; ctlr = ctlr->next){
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
	edev->mbps = 100;
	memmove(edev->ea, ctlr->pa, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = bcm4401attach;
	edev->transmit = bcm4401transmit;
	edev->interrupt = bcm4401interrupt;
	edev->ifstat = bcm4401ifstat;
	edev->ctl = nil;

	edev->arg = edev;
	edev->promiscuous = bcm4401promiscuous;
	edev->multicast = bcm4401multicast;
	edev->shutdown = bcm4401shutdown;

	return 0;
}

void
etherbcm4401link(void)
{
	addethercard("bcm4401", bcm4401pnp);
}
