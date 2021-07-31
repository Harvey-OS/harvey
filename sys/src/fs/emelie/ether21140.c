/*
 * Digital Semiconductor DECchip 21140 PCI Fast Ethernet LAN Controller
 * as found on the Digital Fast EtherWORKS PCI 10/100 adapter (DE-500-X).
 * To do:
 *	thresholds;
 *	ring sizing;
 *	handle more error conditions;
 *	tidy setup packet mess;
 *	make sure reset works (e.g. Ps of csr6);
 *	all the rest of it...
 */
#include "all.h"
#include "io.h"
#include "mem.h"

#include "etherif.h"
#define DEBUG		if(0)print

enum {
	Nrde		= 64,
	Ntde		= 64,
};

enum {
	VendorID	= 0x1011,	/* PCI configuration vendor ID */
	DeviceID	= 0x0009,	/* PCI configuration device ID */
};

enum {					/* CRS0 - Bus Mode */
	Swr		= 0x00000001,	/* Software Reset */
	Bar		= 0x00000002,	/* Bus Arbitration */
	Dsl		= 0x0000007C,	/* Descriptor Skip Length (field) */
	Ble		= 0x00000080,	/* Big/Little Endian */
	Pbl		= 0x00003F00,	/* Programmable Burst Length (field) */
	Cal		= 0x0000C000,	/* Cache Alignment (field) */
	Cal8		= 0x00004000,	/* 8 longword boundary alignment */
	Cal16		= 0x00008000,	/* 16 longword boundary alignment */
	Cal32		= 0x0000C000,	/* 32 longword boundary alignment */
	Tap		= 0x000E0000,	/* Transmit Automatic Polling (field) */
	Dbo		= 0x00100000,	/* Descriptor Byte Ordering Mode */
	Rml		= 0x00200000,	/* Read Multiple */
}; 

enum {					/* CSR[57] - Status and Interrupt Enable */
	Ti		= 0x00000001,	/* Transmit Interrupt */
	Tps		= 0x00000002,	/* Transmit Process Stopped */
	Tu		= 0x00000004,	/* Transmit buffer Unavailable */
	Tjt		= 0x00000008,	/* Transmit Jabber Timeout */
	Unf		= 0x00000020,	/* transmit UNderFlow */
	Ri		= 0x00000040,	/* Receive Interrupt */
	Ru		= 0x00000080,	/* Receive buffer Unavailable */
	Rps		= 0x00000100,	/* Receive Process Stopped */
	Rwt		= 0x00000200,	/* Receive Watchdog Timeout */
	Eti		= 0x00000400,	/* Early Transmit Interrupt */
	Gte		= 0x00000800,	/* General purpose Timer Expired */
	Fbe		= 0x00002000,	/* Fatal Bit Error */
	Ais		= 0x00008000,	/* Abnormal Interrupt Summary */
	Nis		= 0x00010000,	/* Normal Interrupt Summary */
	Rs		= 0x000E0000,	/* Receive process State (field) */
	Ts		= 0x00700000,	/* Transmit process State (field) */
	Eb		= 0x03800000,	/* Error bits */
};

enum {					/* CSR6 - Operating Mode */
	Hp		= 0x00000001,	/* Hash/Perfect receive filtering mode */
	Sr		= 0x00000002,	/* Start/stop Receive */
	Ho		= 0x00000004,	/* Hash-Only filtering mode */
	Pb		= 0x00000008,	/* Pass Bad frames */
	If		= 0x00000010,	/* Inverse Filtering */
	Sb		= 0x00000020,	/* Start/stop Backoff counter */
	Pr		= 0x00000040,	/* Promiscuous Mode */
	Pm		= 0x00000080,	/* Pass all Multicast */
	Fd		= 0x00000200,	/* Full Duplex mode */
	Om		= 0x00000C00,	/* Operating Mode (field) */
	Fc		= 0x00001000,	/* Force Collision */
	St		= 0x00002000,	/* Start/stop Transmission Command */
	Tr		= 0x0000C000,	/* ThReshold control bits (field) */
	Tr128		= 0x00000000,
	Tr256		= 0x00004000,
	Tr512		= 0x00008000,
	Tr1024		= 0x0000C000,
	Ca		= 0x00020000,	/* CApture effect enable */
	Ps		= 0x00040000,	/* Port Select */
	Hbd		= 0x00080000,	/* HeartBeat Disable */
	Imm		= 0x00100000,	/* IMMediate mode */
	Sf		= 0x00200000,	/* Store and Forward */
	Ttm		= 0x00400000,	/* Transmit Threshold Mode */
	Pcs		= 0x00800000,	/* PCS function */
	Scr		= 0x01000000,	/* SCRambler mode */
	Mbo		= 0x02000000,	/* Must Be One */
	Ra		= 0x40000000,	/* Receive All */
	Sc		= 0x80000000,	/* Special Capture effect enable */

	TrMODE		= Sf,		/* default transmission threshold */
};

enum {					/* CSR9 - ROM and MII Management */
	Scs		= 0x00000001,	/* serial ROM chip select */
	Sclk		= 0x00000002,	/* serial ROM clock */
	Sdi		= 0x00000004,	/* serial ROM data in */
	Sdo		= 0x00000008,	/* serial ROM data out */
	Ss		= 0x00000800,	/* serial ROM select */
	Wr		= 0x00002000,	/* write */
	Rd		= 0x00004000,	/* read */

	Mdc		= 0x00010000,	/* MII management clock */
	Mdo		= 0x00020000,	/* MII management write data */
	Mii		= 0x00040000,	/* MII management operation mode */
	Mdi		= 0x00080000,	/* MII management data in */
};

enum {					/* CSR12 - General-Purpose Port */
	Gpc		= 0x00000100,	/* General Purpose Control */

	De500xFSYM	= 0x00000001,	/* output, force 100Mb mode */
	De500xHD	= 0x00000008,	/* output, half-duplex mode */
	De500xNoSYM	= 0x00000040,	/* input, 100Mb mode unavailable */
	De500xNoTBT	= 0x00000080,	/* input, 10Mb mode unavailable */
};

typedef struct Des {
	int	status;
	int	control;
	ulong	addr;
	Msgbuf*	mb;
} Des;

enum {					/* status */
	Of		= 0x00000001,	/* Rx: OverFlow */
	Ce		= 0x00000002,	/* Rx: CRC Error */
	Db		= 0x00000004,	/* Rx: Dribbling Bit */
	Re		= 0x00000008,	/* Rx: Report on MII Error */
	Rw		= 0x00000010,	/* Rx: Receive Watchdog */
	Ft		= 0x00000020,	/* Rx: Frame Type */
	Cs		= 0x00000040,	/* Rx: Collision Seen */
	Tl		= 0x00000080,	/* Rx: Frame too Long */
	Ls		= 0x00000100,	/* Rx: Last deScriptor */
	Fs		= 0x00000200,	/* Rx: First deScriptor */
	Mf		= 0x00000400,	/* Rx: Multicast Frame */
	Rf		= 0x00000800,	/* Rx: Runt Frame */
	Dt		= 0x00003000,	/* Rx: Data Type (field) */
	De		= 0x00004000,	/* Rx: Descriptor Error */
	Fl		= 0x3FFF0000,	/* Rx: Frame Length (field) */
	Ff		= 0x40000000,	/* Rx: Filtering Fail */

	Def		= 0x00000001,	/* Tx: DEFerred */
	Uf		= 0x00000002,	/* Tx: UnderFlow error */
	Lf		= 0x00000004,	/* Tx: Link Fail report */
	Cc		= 0x00000078,	/* Tx: Collision Count (field) */
	Hf		= 0x00000080,	/* Tx: Heartbeat Fail */
	Ec		= 0x00000100,	/* Tx: Excessive Collisions */
	Lc		= 0x00000200,	/* Tx: Late Collision */
	Nc		= 0x00000400,	/* Tx: No Carrier */
	Lo		= 0x00000800,	/* Tx: LOss of carrier */
	To		= 0x00004000,	/* Tx: Transmission jabber timeOut */

	Es		= 0x00008000,	/* [RT]x: Error Summary */
	Own		= 0x80000000,	/* [RT]x: OWN bit */
};

enum {					/* control */
	Bs1		= 0x000007FF,	/* [RT]x: Buffer 1 Size */
	Bs2		= 0x003FF800,	/* [RT]x: Buffer 2 Size */

	Ch		= 0x01000000,	/* [RT]x: second address CHained */
	Er		= 0x02000000,	/* [RT]x: End of Ring */

	Ft0		= 0x00400000,	/* Tx: Filtering Type 0 */
	Dpd		= 0x00800000,	/* Tx: Disabled PaDding */
	Ac		= 0x04000000,	/* Tx: Add CRC disable */
	Set		= 0x08000000,	/* Tx: SETup packet */
	Ft1		= 0x10000000,	/* Tx: Filtering Type 1 */
	Fseg		= 0x20000000,	/* Tx: First SEGment */
	Lseg		= 0x40000000,	/* Tx: Last SEGment */
	Ic		= 0x80000000,	/* Tx: Interrupt on Completion */
};

typedef struct Ctlr {
	int	port;

	uchar	srom[128];
	uchar*	sromea;
	uchar	fd;

	int	csr6;			/* CSR6 - operating mode */
	int	mask;			/* CSR[57] - interrupt mask */
	int	mbps;

	Lock	lock;

	Des*	rdr;			/* receive descriptor ring */
	int	nrdr;			/* size of rdr */
	int	rdrx;			/* index into rdr */

	Lock	tlock;
	Des*	tdr;			/* transmit descriptor ring */
	int	ntdr;			/* size of tdr */
	int	tdrh;			/* host index into tdr */
	int	tdri;			/* interface index into tdr */
	int	ntq;			/* descriptors active */
	Msgbuf*	setupmb;

	ulong	of;			/* receive statistics */
	ulong	ce;
	ulong	cs;
	ulong	tl;
	ulong	rf;
	ulong	de;

	ulong	ru;
	ulong	rps;
	ulong	rwt;

	ulong	uf;			/* transmit statistics */
	ulong	ec;
	ulong	lc;
	ulong	nc;
	ulong	lo;
	ulong	to;

	ulong	tps;
	ulong	tu;
	ulong	tjt;
	ulong	unf;
} Ctlr;

#define csr32r(c, r)	(inl((c)->port+((r)*8)))
#define csr32w(c, r, l)	(outl((c)->port+((r)*8), (ulong)(l)))

typedef struct Adapter Adapter;
struct Adapter {
	int	port;
	int	irq;
	int	tbdf;
};
static Msgbuf* adapter;

static void
attach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->lock);
	if(!(ctlr->csr6 & Sr)){
		ctlr->csr6 |= Sr;
		csr32w(ctlr, 6, ctlr->csr6);
	}
	iunlock(&ctlr->lock);
}

static void
txstart(Ether* ether)
{
	Ctlr *ctlr;
	Msgbuf *mb;
	Des *des;
	int control;

	ctlr = ether->ctlr;
	while(ctlr->ntq < (ctlr->ntdr-1)){
		if(ctlr->setupmb){
			mb = ctlr->setupmb;
			ctlr->setupmb = 0;
			control = Ic|Set|mb->count;
		}
		else{
			mb = etheroq(ether);
			if(mb == nil)
				break;
			control = Ic|Lseg|Fseg|mb->count;
		}

		ctlr->tdr[PREV(ctlr->tdrh, ctlr->ntdr)].control &= ~Ic;
		des = &ctlr->tdr[ctlr->tdrh];
		des->mb = mb;
		des->addr = PADDR(mb->data);
		des->control |= control;
		ctlr->ntq++;
		des->status = Own;
		csr32w(ctlr, 1, 0);
		ctlr->tdrh = NEXT(ctlr->tdrh, ctlr->ntdr);
	}
}

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->tlock);
	txstart(ether);
	iunlock(&ctlr->tlock);
}

static void
interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *ether;
	int len, status;
	Des *des;
	Msgbuf *mb, *rmb;

	ether = arg;
	ctlr = ether->ctlr;

	while((status = csr32r(ctlr, 5)) & (Nis|Ais)){
		/*
		 * Acknowledge the interrupts and mask-out
		 * the ones that are implicitly handled.
		 */
		csr32w(ctlr, 5, status);
		status &= (ctlr->mask & ~(Nis|Ti));

		if(status & Ais){
			if(status & Tps)
				ctlr->tps++;
			if(status & Tu)
				ctlr->tu++;
			if(status & Tjt)
				ctlr->tjt++;
			if(status & Ru)
				ctlr->ru++;
			if(status & Rps)
				ctlr->rps++;
			if(status & Rwt)
				ctlr->rwt++;
			status &= ~(Ais|Rwt|Rps|Ru|Tjt|Tu|Tps);
		}

		/*
		 * Received packets.
		 */
		if(status & Ri) {
			des = &ctlr->rdr[ctlr->rdrx];
			while(!(des->status & Own)){
				if(des->status & Es) {
					if(des->status & Of)
						ctlr->of++;
					if(des->status & Ce)
						ctlr->ce++;
					if(des->status & Cs)
						ctlr->cs++;
					if(des->status & Tl)
						ctlr->tl++;
					if(des->status & Rf)
						ctlr->rf++;
					if(des->status & De)
						ctlr->de++;
				} else
				if(mb = mballoc(sizeof(Enpkt)+4, 0, Mbeth1)){
					rmb = des->mb;
					rmb->count = ((des->status & Fl)>>16)-4;

					etheriq(ether, rmb);

					des->mb = mb;
					des->addr = PADDR(mb->data);
				}

				des->control &= Er;
				des->control |= ROUNDUP(sizeof(Enpkt)+4, 4);
				des->status = Own;

				ctlr->rdrx = NEXT(ctlr->rdrx, ctlr->nrdr);
				des = &ctlr->rdr[ctlr->rdrx];
			}
			status &= ~Ri;
		}

		/*
		 * Check the transmit side:
		 *	check for Transmit Underflow and Adjust
		 *	the threshold upwards;
		 *	free any transmitted buffers and try to
		 *	top-up the ring.
		 */
		if(status & Unf){
			ctlr->unf++;
			ilock(&ctlr->lock);
			csr32w(ctlr, 6, ctlr->csr6 & ~St);
			switch(ctlr->csr6 & Tr){
			case Tr128:
				len = Tr256;
				break;
			case Tr256:
				len = Tr512;
				break;
			case Tr512:
				len = Tr1024;
				break;
			default:
			case Tr1024:
				len = Sf;
				break;
			}
			ctlr->csr6 = (ctlr->csr6 & ~Tr)|len;
			csr32w(ctlr, 6, ctlr->csr6);
			iunlock(&ctlr->lock);
			csr32w(ctlr, 5, Tps);
			status &= ~(Unf|Tps);
		}

		ilock(&ctlr->tlock);
		while(ctlr->ntq){
			des = &ctlr->tdr[ctlr->tdri];
			if(des->status & Own)
				break;

			if(des->status & Es){
				if(des->status & Uf)
					ctlr->uf++;
				if(des->status & Ec)
					ctlr->ec++;
				if(des->status & Lc)
					ctlr->lc++;
				if(des->status & Nc)
					ctlr->nc++;
				if(des->status & Lo)
					ctlr->lo++;
				if(des->status & To)
					ctlr->to++;
				ether->ifc.txerr++;
			}

			mbfree(des->mb);
			des->control &= Er;

			ctlr->ntq--;
			ctlr->tdri = NEXT(ctlr->tdri, ctlr->ntdr);
		}
		txstart(ether);
		iunlock(&ctlr->tlock);

		/*
		 * Anything left not catered for?
		 */
		if(status)
			panic("#l%d: status %8.8ux\n", ether->ctlrno, status);
	}
}

static void
ctlrinit(Ether* ether)
{
	Ctlr *ctlr;
	Des *des;
	Msgbuf *mb;
	int i;
	uchar bi[Easize*2];

	ctlr = ether->ctlr;

	/*
	 * Allocate and initialise the receive ring;
	 * allocate and initialise the transmit ring;
	 * unmask interrupts and start the transmit side;
	 * create and post a setup packet to initialise
	 * the physical ethernet address.
	 */
	ctlr->rdr = ialloc(ctlr->nrdr*sizeof(Des), 0);
	for(des = ctlr->rdr; des < &ctlr->rdr[ctlr->nrdr]; des++){
		des->mb = mballoc(sizeof(Enpkt)+4, 0, Mbeth2);
		des->status = Own;
		des->control = ROUNDUP(sizeof(Enpkt)+4, 4);
		des->addr = PADDR(des->mb->data);
	}
	ctlr->rdr[ctlr->nrdr-1].control |= Er;
	ctlr->rdrx = 0;
	csr32w(ctlr, 3, PADDR(ctlr->rdr));

	ctlr->tdr = ialloc(ctlr->ntdr*sizeof(Des), 8*sizeof(ulong));
	ctlr->tdr[ctlr->ntdr-1].control |= Er;
	ctlr->tdrh = 0;
	ctlr->tdri = 0;
	csr32w(ctlr, 4, PADDR(ctlr->tdr));

	ctlr->mask = Nis|Ais|Fbe|Rwt|Rps|Ru|Ri|Unf|Tjt|Tps|Ti;
	csr32w(ctlr, 7, ctlr->mask);
	ctlr->csr6 |= St;
	csr32w(ctlr, 6, ctlr->csr6);

	for(i = 0; i < Easize/2; i++){
		bi[i*4] = ether->ea[i*2];
		bi[i*4+1] = ether->ea[i*2+1];
		bi[i*4+2] = ether->ea[i*2+1];
		bi[i*4+3] = ether->ea[i*2];
	}
	mb = mballoc(Easize*2*16, 0, Mbeth3);
	memset(mb->data, 0xFF, sizeof(bi));
	for(i = sizeof(bi); i < sizeof(bi)*16; i += sizeof(bi))
		memmove(mb->data+i, bi, sizeof(bi));
	mb->count = sizeof(bi)*16;

	ctlr->setupmb = mb;
	transmit(ether);
}

static char* mediatable[9] = {
	"10BASE-T",				/* TP */
	"10BASE-2",				/* BNC */
	"10BASE-5",				/* AUI */
	"100BASE-TX",
	"10BASE-TFD",
	"100BASE-TXFD",
	"100BASE-T4",
	"100BASE-FX",
	"100BASE-FXFD",
};

static uchar en1207[] = {			/* Accton EN1207-COMBO */
	0x00, 0x00, 0xE8,			/* [0]  vendor ethernet code */
	0x00,					/* [3]  spare */

	0x00, 0x08,				/* [4]  connection (LSB, MSB = 0x0800) */
	0x1F,					/* [6]  general purpose control */
	2,					/* [7]  block count */

	0x00,					/* [8]  media code (10BASE-TX) */
	0x0B,					/* [9]  general purpose port data */
	0x9E, 0x00,				/* [10] command (LSB, MSB = 0x009E) */

	0x03,					/* [8]  media code (100BASE-TX) */
	0x1B,					/* [9]  general purpose port data */
	0x6D, 0x00,				/* [10] command (LSB, MSB = 0x006D) */

						/* There is 10BASE-2 as well, but how to test? */
};

static uchar ana6910fx[] = {			/* Adaptec (Cogent) ANA-6910FX */
	0x00, 0x00, 0x92,			/* [0]  vendor ethernet code */
	0x00,					/* [3]  spare */

	0x00, 0x08,				/* [4]  connection (LSB, MSB = 0x0800) */
	0x3F,					/* [6]  general purpose control */
	1,					/* [7]  block count */

	0x07,					/* [8]  media code (100BASE-FX) */
	0x03,					/* [9]  general purpose port data */
	0x2D, 0x00				/* [10] command (LSB, MSB = 0x000D) */
};

static uchar smc9332[] = {			/* SMC 9332 */
	0x00, 0x00, 0xC0,			/* [0]  vendor ethernet code */
	0x00,					/* [3]  spare */

	0x00, 0x08,				/* [4]  connection (LSB, MSB = 0x0800) */
	0x1F,					/* [6]  general purpose control */
	2,					/* [7]  block count */

	0x00,					/* [8]  media code (10BASE-TX) */
	0x00,					/* [9]  general purpose port data */
	0x9E, 0x00,				/* [10] command (LSB, MSB = 0x009E) */

	0x03,					/* [8]  media code (100BASE-TX) */
	0x09,					/* [9]  general purpose port data */
	0x6D, 0x00,				/* [10] command (LSB, MSB = 0x006D) */
};

static uchar* sromtable[] = {
	en1207,					/* Accton EN1207-COMBO */
	ana6910fx,				/* Adaptec (Cogent) ANA-6910FX */
	smc9332,				/* SMC 9332 */
	0,
};

static void
softreset(Ctlr* ctlr)
{
	csr32w(ctlr, 0, Swr);
	microdelay(10);
	csr32w(ctlr, 0, Rml|Cal16);
	delay(1);
}

static int
mediadetect(Ctlr* ctlr, uchar* leaf, uchar csr12)
{
	int csr6, m, polarity, sense, timeo;

	m = (leaf[3]<<8)|leaf[2];
	csr6 = Sc|Mbo|Hbd|Ca|Sb|TrMODE;
	if(m & 0x0001)
		csr6 |= Ps;
	if(m & 0x0010)
		csr6 |= Ttm;
	if(m & 0x0020)
		csr6 |= Pcs;
	if(m & 0x0040)
		csr6 |= Scr;
	if(ctlr->fd == 1 || (leaf[0] == 0x04 || leaf[0] == 0x05 || leaf[0] == 0x08))
		csr6 |= Fd;

	sense = 1<<((m & 0x000E)>>1);
	if(m & 0x0080)
		polarity = sense;
	else
		polarity = 0;

	csr32w(ctlr, 6, csr6);
	softreset(ctlr);
	csr32w(ctlr, 12, Gpc|csr12);
	delay(200);
	csr32w(ctlr, 12, leaf[1]);
	microdelay(10);

	csr32w(ctlr, 6, csr6);

	for(timeo = 0; timeo < 30; timeo++){
		if((csr32r(ctlr, 12) & sense)^polarity){
			DEBUG("timeo=%d CSR6=%ux CSR12=%lux m=%ux sense=%ux polarity=%ux\n",
    				timeo, csr6, csr32r(ctlr, 12), m, sense, polarity);
			return csr6;
		}
		delay(100);
	}
	DEBUG("timeo=%d CSR6=%ux CSR12=%lux m=%ux sense=%ux polarity=%ux\n",
    		timeo, csr6, csr32r(ctlr, 12), m, sense, polarity);

	return 0;
}

static int
media(Ether* ether)
{
	Ctlr *ctlr;
	uchar *p, *leaf;
	ushort m;
	int csr6, i, k, medium;

	ctlr = ether->ctlr;

	/*
	 * If it's a non-conforming card try to match the vendor ethernet
	 * code and point p at a fake SROM table entry.
	 */
	if(ctlr->sromea == ctlr->srom){
		p = nil;
		for(i = 0; sromtable[i]; i++){
			if(memcmp(sromtable[i], ctlr->sromea, 3) == 0){
				p = &sromtable[i][4];
				break;
			}
		}
		if(p == nil)
			return 0;
	}
	else
		p = &ctlr->srom[(ctlr->srom[28]<<8)|ctlr->srom[27]];

	m = (p[1]<<8)|p[0];

	medium = -1;
	for(i = 0; i < ether->nopt; i++){
		for(k = 0; k < nelem(mediatable); k++){
			DEBUG("compare <%s> <%s>\n", mediatable[k], ether->opt[i]);
			if(cistrcmp(mediatable[k], ether->opt[i]))
				continue;
			medium = k;
			break;
		}

		if(medium == 0x04 || medium == 0x05 || medium == 0x08)
			ctlr->fd = 1;
	}

	DEBUG("medium = %d, m = 0x%ux\n", medium, m);
	for(k = p[3]-1; k >= 0; k--){
		leaf = &p[4+k*4];
		DEBUG("leaf %ux %ux %ux %ux\n", leaf[0], leaf[1], leaf[2], leaf[3]);
		if(medium >= 0 && leaf[0] != medium)
			continue;
		if(m != 0x0800 && leaf[0] != (m & 0xFF))
			continue;
		if(ctlr->fd == 0 && (leaf[0] == 0x04 || leaf[0] == 0x05 || leaf[0] == 0x08))
			continue;
		csr6 = mediadetect(ctlr, leaf, p[2]);
		DEBUG("csr6 = %ux\n", csr6);
		if(csr6){
			ctlr->csr6 = csr6;
			break;
		}
	}

	if(ctlr->csr6){
		if(ctlr->csr6 & Ps)
			return 100;
		else
			return 10;
	}

	return 0;
}

static void
srmiiw(Ctlr* ctlr, int data)
{
	csr32w(ctlr, 9, data);
	microdelay(1);
}

static int
sromr(Ctlr* ctlr, int r)
{
	int i, op, data;

	/*
	 * This sequence for reading a 16-bit register 'r'
	 * in the EEPROM is taken straight from Section
	 * 7.4 of the 21140 Hardware Reference Manual.
	 */
	srmiiw(ctlr, Rd|Ss);
	srmiiw(ctlr, Rd|Ss|Scs);
	srmiiw(ctlr, Rd|Ss|Sclk|Scs);
	srmiiw(ctlr, Rd|Ss);

	op = 0x06;
	for(i = 3-1; i >= 0; i--){
		data = Rd|Ss|(((op>>i) & 0x01)<<2)|Scs;
		srmiiw(ctlr, data);
		srmiiw(ctlr, data|Sclk);
		srmiiw(ctlr, data);
	}

	for(i = 6-1; i >= 0; i--){
		data = Rd|Ss|(((r>>i) & 0x01)<<2)|Scs;
		srmiiw(ctlr, data);
		srmiiw(ctlr, data|Sclk);
		srmiiw(ctlr, data);
	}

	data = 0;
	for(i = 16-1; i >= 0; i--){
		srmiiw(ctlr, Rd|Ss|Sclk|Scs);
		if(csr32r(ctlr, 9) & Sdo)
			data |= (1<<i);
		srmiiw(ctlr, Rd|Ss|Scs);
	}

	srmiiw(ctlr, 0);

	return data & 0xFFFF;
}

static void
dec21140adapter(Msgbuf** mbb, int port, int irq, int tbdf)
{
	Msgbuf *mb;
	Adapter *ap;

	mb = mballoc(sizeof(Adapter), 0, Mbeth4);
	ap = (Adapter*)mb->data;
	ap->port = port;
	ap->irq = irq;
	ap->tbdf = tbdf;

	mb->next = *mbb;
	*mbb = mb;
}

static int
dec21140pci(void)
{
	Pcidev *p;

	p = nil;
	while(p = pcimatch(p, VendorID, DeviceID)){
		/*
		 * bar[0] is the I/O port register address and
		 * bar[1] is the memory-mapped register address.
		 */
		dec21140adapter(&adapter, p->mem[0].bar & ~0x01, p->intl, p->tbdf);
	}

	return 0;
}

int
ether21140reset(Ether* ether)
{
	int i, port, x;
	Msgbuf *mb, **mbb;
	Adapter *ap;
	Ctlr *ctlr;
	uchar ea[Easize];
	static int scandone;

	if(scandone == 0){
		dec21140pci();
		scandone = 1;
	}

	/*
	 * Any adapter matches if no ether->port is supplied,
	 * otherwise the ports must match.
	 */
	port = 0;
	mbb = &adapter;
	for(mb = *mbb; mb; mb = mb->next){
		ap = (Adapter*)mb->data;
		if(ether->port == 0 || ether->port == ap->port){
			port = ap->port;
			ether->irq = ap->irq;
			ether->tbdf = ap->tbdf;
			*mbb = mb->next;
			mbfree(mb);
			break;
		}
		mbb = &mb->next;
	}
	if(port == 0)
		return -1;

	/*
	 * Allocate a controller structure and start to initialise it.
	 */
	ether->ctlr = ialloc(sizeof(Ctlr), 0);
	ctlr = ether->ctlr;
	ctlr->port = port;

	/*
	 * Soft-reset the controller and initialise bus mode.
	 * Delay should be >= 50 PCI cycles (2×S @ 25MHz).
	 * Some cards (e.g. ANA-6910FX) seem to need the Ps bit set
	 * or they don't always work right after a hardware reset.
	 */
	csr32w(ctlr, 6, Mbo|Ps);
	softreset(ctlr);


	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to loading
	 * the station address with the Individual Address Setup command.
	 *
	 * There are 2 SROM layouts:
	 *	e.g. Digital EtherWORKS	station address at offset 20;
	 *				this complies with the 21140A SROM application
	 *				note from Digital;
	 * 	e.g. SMC9332		station address at offset 0 followed by 2
	 *				additional bytes, repeated at offset 16;
	 *				the 8 bytes are also repeated in reverse order
	 *				at offset 8.
	 * To check which it is, read the SROM and check for the repeating patterns
	 * of the non-compliant cards; if that fails use the one at offset 20.
	 */
	for(i = 0; i < sizeof(ctlr->srom)/2; i++){
		x = sromr(ctlr, i);
		ctlr->srom[2*i] = x;
		ctlr->srom[2*i+1] = x>>8;
	}

	ctlr->sromea = ctlr->srom;
	for(i = 0; i < 8; i++){
		x = ctlr->srom[i];
		if(x != ctlr->srom[15-i] || x != ctlr->srom[16+i]){
			ctlr->sromea = &ctlr->srom[20];
			break;
		}
	}

	memset(ea, 0, Easize);
	if(memcmp(ea, ether->ea, Easize) == 0)
		memmove(ether->ea, ctlr->sromea, Easize);

	ether->mbps = media(ether);

	/*
	 * Initialise descriptor rings, ethernet address.
	 */
	ctlr->nrdr = Nrde;
	ctlr->ntdr = Ntde;
	ctlrinit(ether);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->port = port;
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;

	return 0;
}
