/*
 * Digital Semiconductor DECchip 21140 PCI Fast Ethernet LAN Controller
 * as found on the Digital Fast EtherWORKS PCI 10/100 adapter (DE-500-X).
 * To do:
 *	thresholds;
 *	ring sizing;
 *	handle more error conditions;
 *	all the rest of it...
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"

#define DEBUG		if(1)print

enum {
	Nrde		= 4,
	Ntde		= 1,
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
	void*	bp;
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

	uchar	srom[128*2];
	uchar*	sromea;
	int	phy[32];
	uchar	fd;

	int	ctlrno;
	char*	type;
	int	csr6;			/* CSR6 - operating mode */
	int	mask;			/* CSR[57] - interrupt mask */
	int	mbps;

	Des*	rdr;			/* receive descriptor ring */
	int	nrdr;			/* size of rdr */
	int	rdrx;			/* index into rdr */

	Des*	tdr;			/* transmit descriptor ring */
	int	ntdr;			/* size of tdr */
	int	tdrh;			/* host index into tdr */
	int	tdri;			/* interface index into tdr */

	ulong	of;			/* receive statistics */
	ulong	ce;
	ulong	cs;
	ulong	tl;
	ulong	rf;
	ulong	de;

	ulong	uf;			/* transmit statistics */
	ulong	ec;
	ulong	lc;
	ulong	nc;
	ulong	lo;
	ulong	to;

} Ctlr;

#define csr32r(c, r)	(inl((c)->port+((r)*8)))
#define csr32w(c, r, l)	(outl((c)->port+((r)*8), (ulong)(l)))

static uchar setup[Eaddrlen*2*16];

static void
attach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	if((ctlr->csr6 & Sr) == 0){
		ctlr->csr6 |= Sr;
		csr32w(ctlr, 6, ctlr->csr6);
	}
}

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;
	Des *des;
	RingBuf *tb;

	ctlr = ether->ctlr;

	des = &ctlr->tdr[ctlr->tdri];
	tb = &ether->tb[ether->ti];
	if((des->status & Own) == 0 && tb->owner == Interface){
		ctlr->tdr[PREV(ctlr->tdri, ctlr->ntdr)].control &= ~Ic;
		des->addr = PADDR(tb->pkt);
		des->bp = tb->pkt;
		des->control |= Ic|Lseg|Fseg|tb->len;
		des->status = Own;
		csr32w(ctlr, 1, 0);
	}
}

static void
interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *ether;
	int len, status;
	Des *des;
	RingBuf *ring;

	ether = arg;
	ctlr = ether->ctlr;

	while((status = csr32r(ctlr, 5)) & (Nis|Ais)){
		/*
		 * Acknowledge the interrupts and mask-out
		 * the ones that are implicitly handled.
		 */
		csr32w(ctlr, 5, status);
		status &= (ctlr->mask & ~(Nis|Ais|Ti));

		/*
		 * Received packets.
		 */
		if(status & Ri){
			des = &ctlr->rdr[ctlr->rdrx];
			while((des->status & Own) == 0){
				len = ((des->status & Fl)>>16)-4;
				if(des->status & Es){
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
				}
				else{
					ring = &ether->rb[ether->ri];
					if(ring->owner == Interface){
						ring->owner = Host;
						ring->len = len;
						memmove(ring->pkt, des->bp, len);
						ether->ri = NEXT(ether->ri, ether->nrb);
					}
				}

				des->control &= Er;
				des->control |= ROUNDUP(sizeof(Etherpkt)+4, 4);
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
			csr32w(ctlr, 5, Tps);
			status &= ~(Unf|Tps);
		}

		des = &ctlr->tdr[ctlr->tdri];
		while((des->status & Own) == 0 && des->bp){
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
			}

			ring = &ether->tb[ether->ti];
			ring->owner = Host;

			/*
			 */
			if((uchar*)des->bp != setup)
				ether->ti = NEXT(ether->ti, ether->ntb);

			des->control &= Er;
			des->bp = 0;

			ctlr->tdri = NEXT(ctlr->tdri, ctlr->ntdr);
			des = &ctlr->tdr[ctlr->tdri];
		}
		transmit(ether);

		/*
		 * Anything left not catered for?
		 */
		if(status)
			panic("%s#%d: status %8.8uX\n", ctlr->type, ctlr->ctlrno, status);
	}
}

static void
ctlrinit(Ctlr* ctlr, uchar ea[Eaddrlen])
{
	Des *des;
	int i;
	uchar bi[Eaddrlen*2];

	/*
	 * Allocate and initialise the receive ring;
	 * allocate and initialise the transmit ring;
	 * unmask interrupts and start the transmit side;
	 * create and post a setup packet to initialise
	 * the physical ethernet address.
	 */
	ctlr->rdr = malloc(ctlr->nrdr*sizeof(Des));
	for(des = ctlr->rdr; des < &ctlr->rdr[ctlr->nrdr]; des++){
		des->bp = malloc(sizeof(Etherpkt)+4);
		des->status = Own;
		des->control = ROUNDUP(sizeof(Etherpkt)+4, 4);
		des->addr = PADDR(des->bp);
	}
	ctlr->rdr[ctlr->nrdr-1].control |= Er;
	ctlr->rdrx = 0;
	csr32w(ctlr, 3, PADDR(ctlr->rdr));

	ctlr->tdr = malloc(ctlr->ntdr*sizeof(Des));
	ctlr->tdr[ctlr->ntdr-1].control |= Er;
	ctlr->tdrh = 0;
	ctlr->tdri = 0;
	csr32w(ctlr, 4, PADDR(ctlr->tdr));

	ctlr->mask = Nis|Ais|Fbe|Rwt|Rps|Ru|Ri|Unf|Tjt|Tps|Ti;
	csr32w(ctlr, 7, ctlr->mask);
	ctlr->csr6 |= St;
	csr32w(ctlr, 6, ctlr->csr6);

	for(i = 0; i < Eaddrlen/2; i++){
		bi[i*4] = ea[i*2];
		bi[i*4+1] = ea[i*2+1];
		bi[i*4+2] = ea[i*2+1];
		bi[i*4+3] = ea[i*2];
	}
	memset(setup, 0xFF, sizeof(bi));
	for(i = sizeof(bi); i < sizeof(bi)*16; i += sizeof(bi))
		memmove(setup+i, bi, sizeof(bi));

	des = &ctlr->tdr[ctlr->tdrh];
	des->bp = setup;
	des->addr = PADDR(setup);
	des->control |= Ic|Set|sizeof(setup);
	des->status = Own;
	ctlr->tdrh = NEXT(ctlr->tdrh, ctlr->ntdr);
	csr32w(ctlr, 1, 0);
}

static void
srmiiw(Ctlr* ctlr, int data)
{
	csr32w(ctlr, 9, data);
	microdelay(1);
}

static void
miimrw(Ctlr* ctlr, int bits, int n)
{
	int i, mdo;

	/*
	 * Write n bits to the MII Management Register.
	 */
	for(i = n-1; i >= 0; i--){
		if(bits & (1<<i))
			mdo = Mdo;
		else
			mdo = 0;
		srmiiw(ctlr, mdo);
		srmiiw(ctlr, mdo|Mdc);
		srmiiw(ctlr, mdo);
	}
}

static int
miir(Ctlr* ctlr, int phyad, int regad)
{
	int data, i;

	/*
	 * Preamble;
	 * ST+OP+PHYAD+REGAD;
	 * turnaround + 16 data bits.
	 */
	miimrw(ctlr, 0xFFFFFFFF, 32);
	miimrw(ctlr, 0x1800|(phyad<<5)|regad, 14);
	data = 0;
	for(i = 18-1; i >= 0; i--){
		if(csr32r(ctlr, 9) & Mdi)
			data |= (1<<i);
		srmiiw(ctlr, Mii|Mdc);
		srmiiw(ctlr, Mii);
	}
	srmiiw(ctlr, 0);

	if(data & 0x10000)
		return -1;

	return data & 0xFFFF;
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

static char* mediatable[] = {
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

	0x03,					/* [8]  media code (100BASE-TX) */
	0x1B,					/* [9]  general purpose port data */
	0x6D, 0x00,				/* [10] command (LSB, MSB = 0x006D) */

	0x00,					/* [8]  media code (10BASE-TX) */
	0x0B,					/* [9]  general purpose port data */
	0x9E, 0x00,				/* [10] command (LSB, MSB = 0x009E) */

						/* There is 10BASE-2 as well, but... */
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

	0x03,					/* [8]  media code (100BASE-TX) */
	0x09,					/* [9]  general purpose port data */
	0x6D, 0x00,				/* [10] command (LSB, MSB = 0x006D) */

	0x00,					/* [8]  media code (10BASE-TX) */
	0x00,					/* [9]  general purpose port data */
	0x9E, 0x00,				/* [10] command (LSB, MSB = 0x009E) */
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
	switch(leaf[0]){
	default:
		if(!ctlr->fd)
			break;
		/*FALLTHROUGH*/

	case 0x04:				/* 10BASE-TFD */
	case 0x05:				/* 100BASE-TXFD */
	case 0x08:				/* 100BASE-FXFD */
		csr6 |= Fd;
		break;
	}

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
			DEBUG("timeo=%d CSR6=%uX CSR12=%uX m=%uX sense=%uX polarity=%uX\n",
    				timeo, csr6, csr32r(ctlr, 12), m, sense, polarity);
			return csr6;
		}
		delay(100);
	}
	DEBUG("timeo=%d CSR6=%uX CSR12=%uX m=%uX sense=%uX polarity=%uX\n",
    		timeo, csr6, csr32r(ctlr, 12), m, sense, polarity);

	return 0;
}

static void
scanphy(Ctlr* ctlr)
{
	int index, number, oui;

	number = 0;
	for(index = 0; index < 32; index++){
		if((oui = miir(ctlr, index, 2)) == -1 || oui == 0)
			continue;
		oui <<= 6;
		oui |= miir(ctlr, index, 3)>>10;
		DEBUG("phy%d: oui %uX reg1 %uX\n", index, oui, miir(ctlr, index, 1));
		USED(oui);
		ctlr->phy[number] = index;
	}
}

static void
type1(Ctlr* ctlr, uchar* block)
{
	int anar, anlpar, phyad;

	scanphy(ctlr);

	/*
	 * Resolve the highest common ability of the two
	 * link partners. In descending order:
	 *	0x0100		100BASE-TX Full Duplex
	 *	0x0200		100BASE-T4
	 *	0x0080		100BASE-TX
	 *	0x0040		10BASE-T Full Duplex
	 *	0x0020		10BASE-T
	 */
	phyad = ctlr->phy[block[2]];
	anar = miir(ctlr, phyad, 0x04);
	anlpar = miir(ctlr, phyad, 0x05) & 0x03E0;
	anar &= anlpar;
print("anar = %uX %uX %uX %uX\n", anar, anlpar,
  miir(ctlr, phyad, 0x00), miir(ctlr, phyad, 0x01));
	ctlr->csr6 = Sc|Mbo|Hbd|Ps|Ca|Sb|TrMODE;
	if(anar & 0x0100)
		ctlr->csr6 |= Fd;
	else if(anar & 0x0080)
		;
	else if(anar & 0x0040)
		ctlr->csr6 |= Ttm|Fd;
	else
		ctlr->csr6 |= Ttm;
}

static int
media(Ether* ether)
{
	Ctlr *ctlr;
	uchar *p, *block;
	ushort m;
	int csr6, i, k, len, medium;

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

	medium = -1;
	for(i = 0; i < ether->nopt; i++){
		for(k = 0; k < nelem(mediatable); k++){
			DEBUG("compare <%s> <%s>\n", mediatable[k], ether->opt[i]);
			if(cistrcmp(mediatable[k], ether->opt[i]))
				continue;
			medium = k;
			break;
		}

		switch(medium){
		default:
			ctlr->fd = 0;
			break;

		case 0x04:			/* 10BASE-TFD */
		case 0x05:			/* 100BASE-TXFD */
		case 0x08:			/* 100BASE-FXFD */
			ctlr->fd = 1;
			break;
		}
	}

	m = (p[1]<<8)|p[0];
	DEBUG("medium = %d, m = 0x%uX %uX %uX\n", medium, m, p[2], p[3]);
	for(block = &p[4], k = p[3]; k > 0; k--, block += len){
		DEBUG("block %uX %uX %uX %uX\n", block[0], block[1], block[2], block[3]);
		if(block[0] & 0x80){
			len = (block[0] & ~0x80)+1;
			if(block[1] == 1)
				type1(ctlr, block);
			continue;
		}
		else
			len = 4;

		if(medium >= 0 && block[0] != medium)
			continue;
		if(m != 0x0800 && block[0] != (m & 0xFF))
			continue;

		/*
		 * If full-duplex wasn't explicitly requested above,
		 * don't attempt to to detect it.
		 */
		switch(block[0]){
		default:
			break;

		case 0x04:			/* 10BASE-TFD */
		case 0x05:			/* 100BASE-TXFD */
		case 0x08:			/* 100BASE-FXFD */
			if(!ctlr->fd)
				continue;
			break;
		}

		csr6 = mediadetect(ctlr, block, p[2]);
		DEBUG("csr6 = %uX\n", csr6);
		if(csr6){
			ctlr->csr6 = csr6;
			break;
		}
	}

	if(ctlr->csr6){
		if(!(ctlr->csr6 & Ps) || (ctlr->csr6 & Ttm))
			return 10;
		else
			return 100;
	}

	return 0;
}

typedef struct Adapter {
	int	port;
	int	irq;
	int	tbdf;
} Adapter;
static Block* adapter;

static void
dec21140adapter(Block** bpp, int port, int irq, int tbdf)
{
	Block *bp;
	Adapter *ap;

	bp = allocb(sizeof(Adapter));
	ap = (Adapter*)bp->rp;
	ap->port = port;
	ap->irq = irq;
	ap->tbdf = tbdf;
	DEBUG("port 0x%luX intl 0x%luX tbdf 0x%luX\n", port, irq, tbdf);

	bp->next = *bpp;
	*bpp = bp;
}

static void
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
		pcisetbme(p);
	}
}

static void
detach(Ether* ether)
{
	softreset(ether->ctlr);
}

int
ether21140reset(Ether* ether)
{
	int i, port, x;
	Block *bp, **bpp;
	Adapter *ap;
	Ctlr *ctlr;
	uchar ea[Eaddrlen*2];
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
	bpp = &adapter;
	for(bp = *bpp; bp; bp = bp->next){
		ap = (Adapter*)bp->rp;
		if(ether->port == 0 || ether->port == ap->port){
			port = ap->port;
			ether->irq = ap->irq;
			ether->tbdf = ap->tbdf;
			*bpp = bp->next;
			freeb(bp);
			break;
		}
	}
	if(port == 0)
		return -1;

	/*
	 * Allocate a controller structure and start to initialise it.
	 */
	ether->ctlr = malloc(sizeof(Ctlr));
	ctlr = ether->ctlr;
	ctlr->ctlrno = ether->ctlrno;
	ctlr->type = ether->type;
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

	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0)
		memmove(ether->ea, ctlr->sromea, Eaddrlen);

	/*
	 * Determine media.
	 */
	ctlr->mbps = media(ether);

	/*
	 * Initialise descriptor rings, ethernet address.
	 */
	ctlr->nrdr = Nrde;
	ctlr->ntdr = Ntde;
	ctlrinit(ctlr, ether->ea);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->port = port;
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->detach = detach;

	return 0;
}
