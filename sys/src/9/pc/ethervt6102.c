/*
 * VIA VT6102 Fast Ethernet Controller (Rhine II).
 * To do:
 *	cache-line size alignments - done
 *	reduce tx interrupts
 *	use 2 descriptors on tx for alignment - done
 *	reorganise initialisation/shutdown/reset
 *	adjust Tx FIFO threshold on underflow - untested
 *	why does the link status never cause an interrupt?
 *	use the lproc as a periodic timer for stalls, etc.
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
	Par0		= 0x00,		/* Ethernet Address */
	Rcr		= 0x06,		/* Receive Configuration */
	Tcr		= 0x07,		/* Transmit Configuration */
	Cr		= 0x08,		/* Control */
	Isr		= 0x0C,		/* Interrupt Status */
	Imr		= 0x0E,		/* Interrupt Mask */
	Mcfilt0		= 0x10,		/* Multicast Filter 0 */
	Mcfilt1		= 0x14,		/* Multicast Filter 1 */
	Rxdaddr		= 0x18,		/* Current Rx Descriptor Address */
	Txdaddr		= 0x1C,		/* Current Tx Descriptor Address */
	Phyadr		= 0x6C,		/* Phy Address */
	Miisr		= 0x6D,		/* MII Status */
	Bcr0		= 0x6E,		/* Bus Control */
	Bcr1		= 0x6F,
	Miicr		= 0x70,		/* MII Control */
	Miiadr		= 0x71,		/* MII Address */
	Miidata		= 0x72,		/* MII Data */
	Eecsr		= 0x74,		/* EEPROM Control and Status */
	Stickhw		= 0x83,		/* Sticky Hardware Control */
	Wolcrclr	= 0xA4,
	Wolcgclr	= 0xA7,
	Pwrcsrclr	= 0xAC,
};

enum {					/* Rcr */
	Sep		= 0x01,		/* Accept Error Packets */
	Ar		= 0x02,		/* Accept Small Packets */
	Am		= 0x04,		/* Accept Multicast */
	Ab		= 0x08,		/* Accept Broadcast */
	Prom		= 0x10,		/* Accept Physical Address Packets */
	RrftMASK	= 0xE0,		/* Receive FIFO Threshold */
	RrftSHIFT	= 5,
	Rrft64		= 0<<RrftSHIFT,
	Rrft32		= 1<<RrftSHIFT,
	Rrft128		= 2<<RrftSHIFT,
	Rrft256		= 3<<RrftSHIFT,
	Rrft512		= 4<<RrftSHIFT,
	Rrft768		= 5<<RrftSHIFT,
	Rrft1024	= 6<<RrftSHIFT,
	RrftSAF		= 7<<RrftSHIFT,
};

enum {					/* Tcr */
	Lb0		= 0x02,		/* Loopback Mode */
	Lb1		= 0x04,
	Ofset		= 0x08,		/* Back-off Priority Selection */
	RtsfMASK	= 0xE0,		/* Transmit FIFO Threshold */
	RtsfSHIFT	= 5,
	Rtsf128		= 0<<RtsfSHIFT,
	Rtsf256		= 1<<RtsfSHIFT,
	Rtsf512		= 2<<RtsfSHIFT,
	Rtsf1024	= 3<<RtsfSHIFT,
	RtsfSAF		= 7<<RtsfSHIFT,
};

enum {					/* Cr */
	Init		= 0x0001,	/* INIT Process Begin */
	Strt		= 0x0002,	/* Start NIC */
	Stop		= 0x0004,	/* Stop NIC */
	Rxon		= 0x0008,	/* Turn on Receive Process */
	Txon		= 0x0010,	/* Turn on Transmit Process */
	Tdmd		= 0x0020,	/* Transmit Poll Demand */
	Rdmd		= 0x0040,	/* Receive Poll Demand */
	Eren		= 0x0100,	/* Early Receive Enable */
	Fdx		= 0x0400,	/* Set MAC to Full Duplex Mode */
	Dpoll		= 0x0800,	/* Disable Td/Rd Auto Polling */
	Tdmd1		= 0x2000,	/* Transmit Poll Demand 1 */
	Rdmd1		= 0x4000,	/* Receive Poll Demand 1 */
	Sfrst		= 0x8000,	/* Software Reset */
};

enum {					/* Isr/Imr */
	Prx		= 0x0001,	/* Received Packet Successfully */
	Ptx		= 0x0002,	/* Transmitted Packet Successfully */
	Rxe		= 0x0004,	/* Receive Error */
	Txe		= 0x0008,	/* Transmit Error */
	Tu		= 0x0010,	/* Transmit Buffer Underflow */
	Ru		= 0x0020,	/* Receive Buffer Link Error */
	Be		= 0x0040,	/* PCI Bus Error */
	Cnt		= 0x0080,	/* Counter Overflow */
	Eri		= 0x0100,	/* Early Receive Interrupt */
	Udfi		= 0x0200,	/* Tx FIFO Underflow */
	Ovfi		= 0x0400,	/* Receive FIFO Overflow */
	Pktrace		= 0x0800,	/* Hmmm... */
	Norbf		= 0x1000,	/* No Receive Buffers */
	Abti		= 0x2000,	/* Transmission Abort */
	Srci		= 0x4000,	/* Port State Change */
	Geni		= 0x8000,	/* General Purpose Interrupt */
};

enum {					/* Phyadr */
	PhyadMASK	= 0x1F,		/* PHY Address */
	PhyadSHIFT	= 0,
	Mfdc		= 0x20,		/* Accelerate MDC Speed */
	Mpo0		= 0x40,		/* MII Polling Timer Interval */
	Mpo1		= 0x80,
};

enum {					/* Bcr0 */
	DmaMASK		= 0x07,		/* DMA Length */
	DmaSHIFT	= 0,
	Dma32		= 0<<DmaSHIFT,
	Dma64		= 1<<DmaSHIFT,
	Dma128		= 2<<DmaSHIFT,
	Dma256		= 3<<DmaSHIFT,
	Dma512		= 4<<DmaSHIFT,
	Dma1024		= 5<<DmaSHIFT,
	DmaSAF		= 7<<DmaSHIFT,
	CrftMASK	= 0x38,		/* Rx FIFO Threshold */
	CrftSHIFT	= 3,
	Crft64		= 1<<CrftSHIFT,
	Crft128		= 2<<CrftSHIFT,
	Crft256		= 3<<CrftSHIFT,
	Crft512		= 4<<CrftSHIFT,
	Crft1024	= 5<<CrftSHIFT,
	CrftSAF		= 7<<CrftSHIFT,
	Extled		= 0x40,		/* Extra LED Support Control */
	Med2		= 0x80,		/* Medium Select Control */
};

enum {					/* Bcr1 */
	PotMASK		= 0x07,		/* Polling Timer Interval */
	PotSHIFT	= 0,
	CtftMASK	= 0x38,		/* Tx FIFO Threshold */
	CtftSHIFT	= 3,
	Ctft64		= 1<<CtftSHIFT,
	Ctft128		= 2<<CtftSHIFT,
	Ctft256		= 3<<CtftSHIFT,
	Ctft512		= 4<<CtftSHIFT,
	Ctft1024	= 5<<CtftSHIFT,
	CtftSAF		= 7<<CtftSHIFT,
};

enum {					/* Miicr */
	Mdc		= 0x01,		/* Clock */
	Mdi		= 0x02,		/* Data In */
	Mdo		= 0x04,		/* Data Out */
	Mout		= 0x08,		/* Output Enable */
	Mdpm		= 0x10,		/* Direct Program Mode Enable */
	Wcmd		= 0x20,		/* Write Enable */
	Rcmd		= 0x40,		/* Read Enable */
	Mauto		= 0x80,		/* Auto Polling Enable */
};

enum {					/* Miiadr */
	MadMASK		= 0x1F,		/* MII Port Address */
	MadSHIFT	= 0,
	Mdone		= 0x20,		/* Accelerate MDC Speed */
	Msrcen		= 0x40,		/* MII Polling Timer Interval */
	Midle		= 0x80,
};

enum {					/* Eecsr */
	Edo		= 0x01,		/* Data Out */
	Edi		= 0x02,		/* Data In */
	Eck		= 0x04,		/* Clock */
	Ecs		= 0x08,		/* Chip Select */
	Dpm		= 0x10,		/* Direct Program Mode Enable */
	Autold		= 0x20,		/* Dynamic Reload */
	Embp		= 0x40,		/* Embedded Program Enable */
	Eepr		= 0x80,		/* Programmed */
};

/*
 * Ring descriptor. The space allocated for each
 * of these will be rounded up to a cache-line boundary.
 * The first 4 elements are known to the hardware.
 */
typedef struct Ds Ds;
typedef struct Ds {
	uint	status;
	uint	control;
	uint	addr;
	uint	branch;

	Block*	bp;
	void*	bounce;
	Ds*	next;
	Ds*	prev;
} Ds;

enum {					/* Rx Ds status */
	Rerr		= 0x00000001,	/* Receiver Error */
	Crc		= 0x00000002,	/* CRC Error */
	Fae		= 0x00000004,	/* Frame Alignment Error */
	Fov		= 0x00000008,	/* FIFO Overflow */
	Long		= 0x00000010,	/* A Long Packet */
	Runt		= 0x00000020,	/* A Runt Packet */
	Rxserr		= 0x00000040,	/* System Error */
	Buff		= 0x00000080,	/* Buffer Underflow Error */
	Rxedp		= 0x00000100,	/* End of Packet Buffer */
	Rxstp		= 0x00000200,	/* Packet Start */
	Chn		= 0x00000400,	/* Chain Buffer */
	Phy		= 0x00000800,	/* Physical Address Packet */
	Bar		= 0x00001000,	/* Broadcast Packet */
	Mar		= 0x00002000,	/* Multicast Packet */
	Rxok		= 0x00008000,	/* Packet Received Successfully */
	LengthMASK	= 0x07FF0000,	/* Received Packet Length */
	LengthSHIFT	= 16,

	Own		= 0x80000000,	/* Descriptor Owned by NIC */
};

enum {					/* Tx Ds status */
	NcrMASK		= 0x0000000F,	/* Collision Retry Count */
	NcrSHIFT	= 0,
	Cols		= 0x00000010,	/* Experienced Collisions */
	Cdh		= 0x00000080,	/* CD Heartbeat */
	Abt		= 0x00000100,	/* Aborted after Excessive Collisions */
	Owc		= 0x00000200,	/* Out of Window Collision Seen */
	Crs		= 0x00000400,	/* Carrier Sense Lost */
	Udf		= 0x00000800,	/* FIFO Underflow */
	Tbuff		= 0x00001000,	/* Invalid Td */
	Txserr		= 0x00002000,	/* System Error */
	Terr		= 0x00008000,	/* Excessive Collisions */
};

enum {					/* Tx Ds control */
	TbsMASK		= 0x000007FF,	/* Tx Buffer Size */
	TbsSHIFT	= 0,
	Chain		= 0x00008000,	/* Chain Buffer */
	Crcdisable	= 0x00010000,	/* Disable CRC generation */
	Stp		= 0x00200000,	/* Start of Packet */
	Edp		= 0x00400000,	/* End of Packet */
	Ic		= 0x00800000,	/* Assert Interrupt Immediately */
};

enum {
	Nrd		= 64,
	Ntd		= 64,
	Rdbsz		= ROUNDUP(ETHERMAXTU+4, 4),

	Nrxstats	= 8,
	Ntxstats	= 9,

	Txcopy		= 128,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;
	uchar	par[Eaddrlen];

	QLock	alock;			/* attach */
	void*	alloc;			/* receive/transmit descriptors */
	int	cls;			/* alignment */
	int	nrd;
	int	ntd;

	Ds*	rd;
	Ds*	rdh;

	Lock	tlock;
	Ds*	td;
	Ds*	tdh;
	Ds*	tdt;
	int	tdused;

	Lock	clock;			/*  */
	int	cr;
	int	imr;
	int	tft;			/* Tx threshold */

	Mii*	mii;
	Rendez	lrendez;
	int	lwakeup;

	uint	rxstats[Nrxstats];	/* statistics */
	uint	txstats[Ntxstats];
	uint	intr;
	uint	lintr;
	uint	lsleep;
	uint	rintr;
	uint	tintr;
	uint	taligned;
	uint	tsplit;
	uint	tcopied;
	uint	txdw;
} Ctlr;

static Ctlr* vt6102ctlrhead;
static Ctlr* vt6102ctlrtail;

#define csr8r(c, r)	(inb((c)->port+(r)))
#define csr16r(c, r)	(ins((c)->port+(r)))
#define csr32r(c, r)	(inl((c)->port+(r)))
#define csr8w(c, r, b)	(outb((c)->port+(r), (int)(b)))
#define csr16w(c, r, w)	(outs((c)->port+(r), (ushort)(w)))
#define csr32w(c, r, w)	(outl((c)->port+(r), (ulong)(w)))

static char* rxstats[Nrxstats] = {
	"Receiver Error",
	"CRC Error",
	"Frame Alignment Error",
	"FIFO Overflow",
	"Long Packet",
	"Runt Packet",
	"System Error",
	"Buffer Underflow Error",
};
static char* txstats[Ntxstats] = {
	"Aborted after Excessive Collisions",
	"Out of Window Collision Seen",
	"Carrier Sense Lost",
	"FIFO Underflow",
	"Invalid Td",
	"System Error",
	nil,
	"Excessive Collisions",
};

static long
vt6102ifstat(Ether* edev, void* a, long n, ulong offset)
{
	char *p;
	Ctlr *ctlr;
	int i, l, r;

	ctlr = edev->ctlr;

	p = malloc(READSTR);
	l = 0;
	for(i = 0; i < Nrxstats; i++){
		l += snprint(p+l, READSTR-l, "%s: %ud\n",
			rxstats[i], ctlr->rxstats[i]);
	}
	for(i = 0; i < Ntxstats; i++){
		if(txstats[i] == nil)
			continue;
		l += snprint(p+l, READSTR-l, "%s: %ud\n",
			txstats[i], ctlr->txstats[i]);
	}
	l += snprint(p+l, READSTR-l, "cls: %ud\n", ctlr->cls);
	l += snprint(p+l, READSTR-l, "intr: %ud\n", ctlr->intr);
	l += snprint(p+l, READSTR-l, "lintr: %ud\n", ctlr->lintr);
	l += snprint(p+l, READSTR-l, "lsleep: %ud\n", ctlr->lsleep);
	l += snprint(p+l, READSTR-l, "rintr: %ud\n", ctlr->rintr);
	l += snprint(p+l, READSTR-l, "tintr: %ud\n", ctlr->tintr);
	l += snprint(p+l, READSTR-l, "taligned: %ud\n", ctlr->taligned);
	l += snprint(p+l, READSTR-l, "tsplit: %ud\n", ctlr->tsplit);
	l += snprint(p+l, READSTR-l, "tcopied: %ud\n", ctlr->tcopied);
	l += snprint(p+l, READSTR-l, "txdw: %ud\n", ctlr->txdw);
	l += snprint(p+l, READSTR-l, "tft: %ud\n", ctlr->tft);

	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		l += snprint(p+l, READSTR, "phy:   ");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				l += snprint(p+l, READSTR-l, "\n       ");
			r = miimir(ctlr->mii, i);
			l += snprint(p+l, READSTR-l, " %4.4uX", r);
		}
		snprint(p+l, READSTR-l, "\n");
	}
	snprint(p+l, READSTR-l, "\n");

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
vt6102promiscuous(void* arg, int on)
{
	int rcr;
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	rcr = csr8r(ctlr, Rcr);
	if(on)
		rcr |= Prom;
	else
		rcr &= ~Prom;
	csr8w(ctlr, Rcr, rcr);
}

static void
vt6102multicast(void* arg, uchar* addr, int on)
{
	/*
	 * For now Am is set in Rcr.
	 * Will need to interlock with promiscuous
	 * when this gets filled in.
	 */
	USED(arg, addr, on);
}

static int
vt6102wakeup(void* v)
{
	return *((int*)v) != 0;
}

static void
vt6102imr(Ctlr* ctlr, int imr)
{
	ilock(&ctlr->clock);
	ctlr->imr |= imr;
	csr16w(ctlr, Imr, ctlr->imr);
	iunlock(&ctlr->clock);
}

static void
vt6102lproc(void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	MiiPhy *phy;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		if(ctlr->mii == nil || ctlr->mii->curphy == nil)
			break;
		if(miistatus(ctlr->mii) < 0)
			goto enable;

		phy = ctlr->mii->curphy;
		ilock(&ctlr->clock);
		if(phy->fd)
			ctlr->cr |= Fdx;
		else
			ctlr->cr &= ~Fdx;
		csr16w(ctlr, Cr, ctlr->cr);
		iunlock(&ctlr->clock);
enable:
		ctlr->lwakeup = 0;
		vt6102imr(ctlr, Srci);

		ctlr->lsleep++;
		sleep(&ctlr->lrendez, vt6102wakeup, &ctlr->lwakeup);

	}
	pexit("vt6102lproc: done", 1);
}

static void
vt6102attach(Ether* edev)
{
	int i;
	Ctlr *ctlr;
	Ds *ds, *prev;
	uchar *alloc, *bounce;
	char name[KNAMELEN];

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(ctlr->alloc != nil){
		qunlock(&ctlr->alock);
		return;
	}

	/*
	 * Descriptor and bounce-buffer space.
	 * Must all be aligned on a 4-byte boundary,
	 * but try to align on cache-lines.
	 */
	ctlr->nrd = Nrd;
	ctlr->ntd = Ntd;
	alloc = malloc((ctlr->nrd+ctlr->ntd)*ctlr->cls + ctlr->ntd*Txcopy + ctlr->cls-1);
	if(alloc == nil){
		qunlock(&ctlr->alock);
		return;
	}
	ctlr->alloc = alloc;
	alloc = (uchar*)ROUNDUP((ulong)alloc, ctlr->cls);

	ctlr->rd = (Ds*)alloc;

	if(waserror()){
		ds = ctlr->rd;
		for(i = 0; i < ctlr->nrd; i++){
			if(ds->bp != nil){
				freeb(ds->bp);
				ds->bp = nil;
			}
			if((ds = ds->next) == nil)
				break;
		}
		free(ctlr->alloc);
		ctlr->alloc = nil;
		qunlock(&ctlr->alock);
		nexterror();
	}

	prev = ctlr->rd + ctlr->nrd-1;
	for(i = 0; i < ctlr->nrd; i++){
		ds = (Ds*)alloc;
		alloc += ctlr->cls;

		ds->control = Rdbsz;
		ds->branch = PCIWADDR(alloc);

		ds->bp = iallocb(Rdbsz+3);
		if(ds->bp == nil)
			error("vt6102: can't allocate receive ring\n");
		ds->bp->rp = (uchar*)ROUNDUP((ulong)ds->bp->rp, 4);
		ds->addr = PCIWADDR(ds->bp->rp);

		ds->next = (Ds*)alloc;
		ds->prev = prev;
		prev = ds;

		ds->status = Own;
	}
	prev->branch = 0;
	prev->next = ctlr->rd;
	prev->status = 0;
	ctlr->rdh = ctlr->rd;

	ctlr->td = (Ds*)alloc;
	prev = ctlr->td + ctlr->ntd-1;
	bounce = alloc + ctlr->ntd*ctlr->cls;
	for(i = 0; i < ctlr->ntd; i++){
		ds = (Ds*)alloc;
		alloc += ctlr->cls;

		ds->bounce = bounce;
		bounce += Txcopy;
		ds->next = (Ds*)alloc;
		ds->prev = prev;
		prev = ds;
	}
	prev->next = ctlr->td;
	ctlr->tdh = ctlr->tdt = ctlr->td;
	ctlr->tdused = 0;

	ctlr->cr = Dpoll|Rdmd|Txon|Rxon|Strt;
	/*Srci|Abti|Norbf|Pktrace|Ovfi|Udfi|Be|Ru|Tu|Txe|Rxe|Ptx|Prx*/
	ctlr->imr = Abti|Norbf|Pktrace|Ovfi|Udfi|Be|Ru|Tu|Txe|Rxe|Ptx|Prx;

	ilock(&ctlr->clock);
	csr32w(ctlr, Rxdaddr, PCIWADDR(ctlr->rd));
	csr32w(ctlr, Txdaddr, PCIWADDR(ctlr->td));
	csr16w(ctlr, Isr, ~0);
	csr16w(ctlr, Imr, ctlr->imr);
	csr16w(ctlr, Cr, ctlr->cr);
	iunlock(&ctlr->clock);

	snprint(name, KNAMELEN, "#l%dlproc", edev->ctlrno);
	kproc(name, vt6102lproc, edev);

	qunlock(&ctlr->alock);
	poperror();
}

static void
vt6102transmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;
	Ds *ds, *next;
	int control, i, o, prefix, size, tdused, timeo;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);

	/*
	 * Free any completed packets
	 */
	ds = ctlr->tdh;
	for(tdused = ctlr->tdused; tdused > 0; tdused--){
		/*
		 * For some errors the chip will turn the Tx engine
		 * off. Wait for that to happen.
		 * Could reset and re-init the chip here if it doesn't
		 * play fair.
		 * To do: adjust Tx FIFO threshold on underflow.
		 */
		if(ds->status & (Abt|Tbuff|Udf)){
			for(timeo = 0; timeo < 1000; timeo++){
				if(!(csr16r(ctlr, Cr) & Txon))
					break;
				microdelay(1);
			}
			ds->status = Own;
			csr32w(ctlr, Txdaddr, PCIWADDR(ds));
		}

		if(ds->status & Own)
			break;
		ds->addr = 0;
		ds->branch = 0;

		if(ds->bp != nil){
			freeb(ds->bp);
			ds->bp = nil;
		}
		for(i = 0; i < Ntxstats-1; i++){
			if(ds->status & (1<<i))
				ctlr->txstats[i]++;
		}
		ctlr->txstats[i] += (ds->status & NcrMASK)>>NcrSHIFT;

		ds = ds->next;
	}
	ctlr->tdh = ds;

	/*
	 * Try to fill the ring back up.
	 */
	ds = ctlr->tdt;
	while(tdused < ctlr->ntd-2){
		if((bp = qget(edev->oq)) == nil)
			break;
		tdused++;

		size = BLEN(bp);
		prefix = 0;

		if(o = (((int)bp->rp) & 0x03)){
			prefix = Txcopy-o;
			if(prefix > size)
				prefix = size;
			memmove(ds->bounce, bp->rp, prefix);
			ds->addr = PCIWADDR(ds->bounce);
			bp->rp += prefix;
			size -= prefix;
		}

		next = ds->next;
		ds->branch = PCIWADDR(ds->next);

		if(size){
			if(prefix){
				next->bp = bp;
				next->addr = PCIWADDR(bp->rp);
				next->branch = PCIWADDR(next->next);
				next->control = Edp|Chain|((size<<TbsSHIFT) & TbsMASK);

				control = Stp|Chain|((prefix<<TbsSHIFT) & TbsMASK);

				next = next->next;
				tdused++;
				ctlr->tsplit++;
			}
			else{
				ds->bp = bp;
				ds->addr = PCIWADDR(bp->rp);
				control = Edp|Stp|((size<<TbsSHIFT) & TbsMASK);
				ctlr->taligned++;
			}
		}
		else{
			freeb(bp);
			control = Edp|Stp|((prefix<<TbsSHIFT) & TbsMASK);
			ctlr->tcopied++;
		}

		ds->control = control;
		if(tdused >= ctlr->ntd-2){
			ds->control |= Ic;
			ctlr->txdw++;
		}
		coherence();
		ds->status = Own;

		ds = next;
	}
	ctlr->tdt = ds;
	ctlr->tdused = tdused;
	if(ctlr->tdused)
		csr16w(ctlr, Cr, Tdmd|ctlr->cr);

	iunlock(&ctlr->tlock);
}

static void
vt6102receive(Ether* edev)
{
	Ds *ds;
	Block *bp;
	Ctlr *ctlr;
	int i, len;

	ctlr = edev->ctlr;

	ds = ctlr->rdh;
	while(!(ds->status & Own) && ds->status != 0){
		if(ds->status & Rerr){
			for(i = 0; i < Nrxstats; i++){
				if(ds->status & (1<<i))
					ctlr->rxstats[i]++;
			}
		}
		else if(bp = iallocb(Rdbsz+3)){
			len = ((ds->status & LengthMASK)>>LengthSHIFT)-4;
			ds->bp->wp = ds->bp->rp+len;
			etheriq(edev, ds->bp, 1);
			bp->rp = (uchar*)ROUNDUP((ulong)bp->rp, 4);
			ds->addr = PCIWADDR(bp->rp);
			ds->bp = bp;
		}
		ds->control = Rdbsz;
		ds->branch = 0;
		ds->status = 0;

		ds->prev->branch = PCIWADDR(ds);
		coherence();
		ds->prev->status = Own;

		ds = ds->next;
	}
	ctlr->rdh = ds;

	csr16w(ctlr, Cr, ctlr->cr);
}

static void
vt6102interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *edev;
	int imr, isr, r, timeo;

	edev = arg;
	ctlr = edev->ctlr;

	ilock(&ctlr->clock);
	csr16w(ctlr, Imr, 0);
	imr = ctlr->imr;
	ctlr->intr++;
	for(;;){
		if((isr = csr16r(ctlr, Isr)) != 0)
			csr16w(ctlr, Isr, isr);
		if((isr & ctlr->imr) == 0)
			break;
			
		if(isr & Srci){
			imr &= ~Srci;
			ctlr->lwakeup = isr & Srci;
			wakeup(&ctlr->lrendez);
			isr &= ~Srci;
			ctlr->lintr++;
		}
		if(isr & (Norbf|Pktrace|Ovfi|Ru|Rxe|Prx)){
			vt6102receive(edev);
			isr &= ~(Norbf|Pktrace|Ovfi|Ru|Rxe|Prx);
			ctlr->rintr++;
		}
		if(isr & (Abti|Udfi|Tu|Txe|Ptx)){
			if(isr & (Abti|Udfi|Tu)){
				for(timeo = 0; timeo < 1000; timeo++){
					if(!(csr16r(ctlr, Cr) & Txon))
						break;
					microdelay(1);
				}

				if((isr & Udfi) && ctlr->tft < CtftSAF){
					ctlr->tft += 1<<CtftSHIFT;
					r = csr8r(ctlr, Bcr1) & ~CtftMASK;
					csr8w(ctlr, Bcr1, r|ctlr->tft);
				}
			}
			vt6102transmit(edev);
			isr &= ~(Abti|Udfi|Tu|Txe|Ptx);
			ctlr->tintr++;
		}
		if(isr)
			panic("vt6102: isr %4.4uX\n", isr);
	}
	ctlr->imr = imr;
	csr16w(ctlr, Imr, ctlr->imr);
	iunlock(&ctlr->clock);
}

static int
vt6102miimicmd(Mii* mii, int pa, int ra, int cmd, int data)
{
	Ctlr *ctlr;
	int r, timeo;

	ctlr = mii->ctlr;

	csr8w(ctlr, Miicr, 0);
	r = csr8r(ctlr, Phyadr);
	csr8w(ctlr, Phyadr, (r & ~PhyadMASK)|pa);
	csr8w(ctlr, Phyadr, pa);
	csr8w(ctlr, Miiadr, ra);
	if(cmd == Wcmd)
		csr16w(ctlr, Miidata, data);
	csr8w(ctlr, Miicr, cmd);

	for(timeo = 0; timeo < 10000; timeo++){
		if(!(csr8r(ctlr, Miicr) & cmd))
			break;
		microdelay(1);
	}
	if(timeo >= 10000)
		return -1;

	if(cmd == Wcmd)
		return 0;
	return csr16r(ctlr, Miidata);
}

static int
vt6102miimir(Mii* mii, int pa, int ra)
{
	return vt6102miimicmd(mii, pa, ra, Rcmd, 0);
}

static int
vt6102miimiw(Mii* mii, int pa, int ra, int data)
{
	return vt6102miimicmd(mii, pa, ra, Wcmd, data);
}

static int
vt6102detach(Ctlr* ctlr)
{
	int revid, timeo;

	/*
	 * Reset power management registers.
	 */
	revid = pcicfgr8(ctlr->pcidev, PciRID);
	if(revid >= 0x40){
		/* Set power state D0. */
		csr8w(ctlr, Stickhw, csr8r(ctlr, Stickhw) & 0xFC);

		/* Disable force PME-enable. */
		csr8w(ctlr, Wolcgclr, 0x80);

		/* Clear WOL config and status bits. */
		csr8w(ctlr, Wolcrclr, 0xFF);
		csr8w(ctlr, Pwrcsrclr, 0xFF);
	}

	/*
	 * Soft reset the controller.
	 */
	csr16w(ctlr, Cr, Stop);
	csr16w(ctlr, Cr, Stop|Sfrst);
	for(timeo = 0; timeo < 10000; timeo++){
		if(!(csr16r(ctlr, Cr) & Sfrst))
			break;
		microdelay(1);
	}
	if(timeo >= 1000)
		return -1;

	return 0;
}

static int
vt6102reset(Ctlr* ctlr)
{
	MiiPhy *phy;
	int i, r, timeo;

	if(vt6102detach(ctlr) < 0)
		return -1;

	/*
	 * Load the MAC address into the PAR[01]
	 * registers.
	 */
	r = csr8r(ctlr, Eecsr);
	csr8w(ctlr, Eecsr, Autold|r);
	for(timeo = 0; timeo < 100; timeo++){
		if(!(csr8r(ctlr, Cr) & Autold))
			break;
		microdelay(1);
	}
	if(timeo >= 100)
		return -1;

	for(i = 0; i < Eaddrlen; i++)
		ctlr->par[i] = csr8r(ctlr, Par0+i);

	/*
	 * Configure DMA and Rx/Tx thresholds.
	 * If the Rx/Tx threshold bits in Bcr[01] are 0 then
	 * the thresholds are determined by Rcr/Tcr.
	 */
	r = csr8r(ctlr, Bcr0) & ~(CrftMASK|DmaMASK);
	csr8w(ctlr, Bcr0, r|Crft64|Dma64);
	r = csr8r(ctlr, Bcr1) & ~CtftMASK;
	csr8w(ctlr, Bcr1, r|ctlr->tft);

	r = csr8r(ctlr, Rcr) & ~(RrftMASK|Prom|Ar|Sep);
	csr8w(ctlr, Rcr, r|Ab|Am);
	csr32w(ctlr, Mcfilt0, ~0UL);	/* accept all multicast */
	csr32w(ctlr, Mcfilt1, ~0UL);

	r = csr8r(ctlr, Tcr) & ~(RtsfMASK|Ofset|Lb1|Lb0);
	csr8w(ctlr, Tcr, r);

	/*
	 * Link management.
	 */
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->mir = vt6102miimir;
	ctlr->mii->miw = vt6102miimiw;
	ctlr->mii->ctlr = ctlr;

	if(mii(ctlr->mii, ~0) == 0 || (phy = ctlr->mii->curphy) == nil){
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	// print("oui %X phyno %d\n", phy->oui, phy->phyno);
	USED(phy);

	//miiane(ctlr->mii, ~0, ~0, ~0);

	return 0;
}

static void
vt6102pci(void)
{
	Pcidev *p;
	Ctlr *ctlr;
	int cls, port;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x3065<<16)|0x1106:	/* Rhine II */
		case (0x3106<<16)|0x1106:	/* Rhine III */
			break;
		}

		port = p->mem[0].bar & ~0x01;
		if(ioalloc(port, p->mem[0].size, 0, "vt6102") < 0){
			print("vt6102: port 0x%uX in use\n", port);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = port;
		ctlr->pcidev = p;
		ctlr->id = (p->did<<16)|p->vid;
		if((cls = pcicfgr8(p, PciCLS)) == 0 || cls == 0xFF)
			cls = 0x10;
		ctlr->cls = cls*4;
		if(ctlr->cls < sizeof(Ds)){
			print("vt6102: cls %d < sizeof(Ds)\n", ctlr->cls);
			iofree(port);
			free(ctlr);
			continue;
		}
		ctlr->tft = Ctft64;

		if(vt6102reset(ctlr)){
			iofree(port);
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(vt6102ctlrhead != nil)
			vt6102ctlrtail->next = ctlr;
		else
			vt6102ctlrhead = ctlr;
		vt6102ctlrtail = ctlr;
	}
}

static int
vt6102pnp(Ether* edev)
{
	Ctlr *ctlr;

	if(vt6102ctlrhead == nil)
		vt6102pci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = vt6102ctlrhead; ctlr != nil; ctlr = ctlr->next){
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
	memmove(edev->ea, ctlr->par, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = vt6102attach;
	edev->transmit = vt6102transmit;
	edev->interrupt = vt6102interrupt;
	edev->ifstat = vt6102ifstat;
	edev->ctl = nil;

	edev->arg = edev;
	edev->promiscuous = vt6102promiscuous;
	edev->multicast = vt6102multicast;

	return 0;
}

void
ethervt6102link(void)
{
	addethercard("vt6102", vt6102pnp);
	addethercard("rhine", vt6102pnp);
}
