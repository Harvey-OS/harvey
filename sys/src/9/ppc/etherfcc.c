/*
 * FCCn ethernet
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "m8260.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"

enum {
	Nrdre		= 128,	/* receive descriptor ring entries */
	Ntdre		= 128,	/* transmit descriptor ring entries */

	Rbsize		= ETHERMAXTU+4,		/* ring buffer size (+4 for CRC) */
	Bufsize		= (Rbsize+7)&~7,		/* aligned */
};

enum {

	/* ether-specific Rx BD bits */
	RxMiss=		SBIT(7),
	RxeLG=		SBIT(10),
	RxeNO=		SBIT(11),
	RxeSH=		SBIT(12),
	RxeCR=		SBIT(13),
	RxeOV=		SBIT(14),
	RxeCL=		SBIT(15),
	RxError=		(RxeLG|RxeNO|RxeSH|RxeCR|RxeOV|RxeCL),	/* various error flags */

	/* ether-specific Tx BD bits */
	TxPad=		SBIT(1),	/* pad short frames */
	TxTC=		SBIT(5),	/* transmit CRC */
	TxeDEF=		SBIT(6),
	TxeHB=		SBIT(7),
	TxeLC=		SBIT(8),
	TxeRL=		SBIT(9),
	TxeUN=		SBIT(14),
	TxeCSL=		SBIT(15),

	/* psmr */
	PRO=		BIT(9),	/* promiscuous mode */
	LPB=			BIT(3),	/* local protect bit */
	FDE=		BIT(5),	/* full duplex ethernet */
	CRCE=		BIT(24),	/* Ethernet CRC */

	/* gfmr */
	ENET=		0xc,		/* ethernet mode */
	ENT=		BIT(27),
	ENR=		BIT(26),
	TCI=			BIT(2),

	/* FCC function code register */
	GBL=		0x20,
	BO=			0x18,
	EB=			0x10,	/* Motorola byte order */
	TC2=		0x04,
	DTB=		0x02,
	BDB=		0x01,

	/* FCC Event/Mask bits */
	GRA=		SBIT(8),
	RXC=		SBIT(9),
	TXC=		SBIT(10),
	TXE=			SBIT(11),
	RXF=			SBIT(12),
	BSY=			SBIT(13),
	TXB=		SBIT(14),
	RXB=		SBIT(15),
};

typedef struct Etherparam Etherparam;
struct Etherparam {
/*0x00*/	FCCparam;
/*0x3c*/	ulong	stat_buf;
/*0x40*/	ulong	cam_ptr;
/*0x44*/	ulong	cmask;
/*0x48*/	ulong	cpres;
/*0x4c*/	ulong	crcec;
/*0x50*/	ulong	alec;
/*0x54*/	ulong	disfc;
/*0x58*/	ushort	retlim;
/*0x5a*/	ushort	retcnt;
/*0x5c*/	ushort	p_per;
/*0x5e*/	ushort	boff_cnt;
/*0x60*/	ulong	gaddr[2];
/*0x68*/	ushort	tfcstat;
/*0x6a*/	ushort	tfclen;
/*0x6c*/	ulong	tfcptr;
/*0x70*/	ushort	mflr;
/*0x72*/	ushort	paddr[3];
/*0x78*/	ushort	ibd_cnt;
/*0x7a*/	ushort	ibd_start;
/*0x7c*/	ushort	ibd_end;
/*0x7e*/	ushort	tx_len;
/*0x80*/	uchar	ibd_base[32];
/*0xa0*/	ulong	iaddr[2];
/*0xa8*/	ushort	minflr;
/*0xaa*/	ushort	taddr[3];
/*0xb0*/	ushort	padptr;
/*0xb2*/	ushort	Rsvdb2;
/*0xb4*/	ushort	cf_range;
/*0xb6*/	ushort	max_b;
/*0xb8*/	ushort	maxd1;
/*0xba*/	ushort	maxd2;
/*0xbc*/	ushort	maxd;
/*0xbe*/	ushort	dma_cnt;
/*0xc0*/	ulong	octc;
/*0xc4*/	ulong	colc;
/*0xc8*/	ulong	broc;
/*0xcc*/	ulong	mulc;
/*0xd0*/	ulong	uspc;
/*0xd4*/	ulong	frgc;
/*0xd8*/	ulong	ospc;
/*0xdc*/	ulong	jbrc;
/*0xe0*/	ulong	p64c;
/*0xe4*/	ulong	p65c;
/*0xe8*/	ulong	p128c;
/*0xec*/	ulong	p256c;
/*0xf0*/	ulong	p512c;
/*0xf4*/	ulong	p1024c;
/*0xf8*/	ulong	cam_buf;
/*0xfc*/	ulong	Rsvdfc;
/*0x100*/
};

typedef struct {
	Lock;
	int		fccid;
	int		port;
	ulong	pmdio;
	ulong	pmdck;
	int		init;
	int		active;
	FCC*		fcc;

	Ring;

	ulong	interrupts;			/* statistics */
	ulong	deferred;
	ulong	heartbeat;
	ulong	latecoll;
	ulong	retrylim;
	ulong	underrun;
	ulong	overrun;
	ulong	carrierlost;
	ulong	retrycount;
} Ctlr;

static	int	fccirq[] = {0x20, 0x21, 0x22};
static	int	fccid[] = {FCC1ID, FCC2ID, FCC3ID};

int	ioringinit(Ring* r, int nrdre, int ntdre, int bufsize);

static void
attach(Ether *ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ctlr->active = 1;
	ctlr->fcc->gfmr |= ENR|ENT;
	eieio();
}

static void
closed(Ether *ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(ctlr);
	ctlr->active = 0;
	ctlr->fcc->gfmr &= ~(ENR|ENT);
	iunlock(ctlr);
	print("Ether closed\n");
}

static void
promiscuous(void* arg, int on)
{
	Ether *ether;
	Ctlr *ctlr;

	ether = (Ether*)arg;
	ctlr = ether->ctlr;

	ilock(ctlr);
	if(on || ether->nmaddr)
		ctlr->fcc->fpsmr |= PRO;
	else
		ctlr->fcc->fpsmr &= ~PRO;
	iunlock(ctlr);
}

static void
multicast(void* arg, uchar *addr, int on)
{
	Ether *ether;
	Ctlr *ctlr;

	USED(addr, on);	/* if on, could SetGroupAddress; if !on, it's hard */

	ether = (Ether*)arg;
	ctlr = ether->ctlr;

	ilock(ctlr);
	if(ether->prom || ether->nmaddr)
		ctlr->fcc->fpsmr |= PRO;
	else
		ctlr->fcc->fpsmr &= ~PRO;
	iunlock(ctlr);
}

static void
txstart(Ether *ether)
{
	int len;
	Ctlr *ctlr;
	Block *b;
	BD *dre;

	ctlr = ether->ctlr;
	if(ctlr->init)
		return;
	while(ctlr->ntq < Ntdre-1){
		b = qget(ether->oq);
		if(b == 0)
			break;

		dre = &ctlr->tdr[ctlr->tdrh];
		dczap(dre, sizeof(BD));
		if(dre->status & BDReady)
			panic("ether: txstart");

		/*
		 * Give ownership of the descriptor to the chip, increment the
		 * software ring descriptor pointer and tell the chip to poll.
		 */
		len = BLEN(b);
		if(ctlr->txb[ctlr->tdrh] != nil)
			panic("fcc/ether: txstart");
		ctlr->txb[ctlr->tdrh] = b;
		if((ulong)b->rp&1)
			panic("fcc/ether: txstart align");	/* TO DO: ensure alignment */
		dre->addr = PADDR(b->rp);
		dre->length = len;
		dcflush(b->rp, len);
		dcflush(dre, sizeof(BD));
		dre->status = (dre->status & BDWrap) | BDReady|TxPad|BDInt|BDLast|TxTC;
		dcflush(dre, sizeof(BD));
		ctlr->fcc->ftodr = 1<<15;	/* transmit now */
		ctlr->ntq++;
		ctlr->tdrh = NEXT(ctlr->tdrh, Ntdre);
	}
}

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(ctlr);
	txstart(ether);
	iunlock(ctlr);
}

static void
interrupt(Ureg*, void *arg)
{
	int len, status;
	ushort events;
	Ctlr *ctlr;
	BD *dre;
	Block *b;
	Ether *ether = arg;

	ctlr = ether->ctlr;
	if(!ctlr->active)
		return;	/* not ours */

	/*
	 * Acknowledge all interrupts and whine about those that shouldn't
	 * happen.
	 */
	events = ctlr->fcc->fcce;
	eieio();
	ctlr->fcc->fcce = events;		/* clear events */
	eieio();
	ctlr->interrupts++;

	if(events & RXB)
		ctlr->overrun++;
	if(events & TXE)
		ether->oerrs++;

	/*
	 * Receiver interrupt: run round the descriptor ring logging
	 * errors and passing valid receive data up to the higher levels
	 * until we encounter a descriptor still owned by the chip.
	 */
	if(events & (RXF|RXB)){
		dre = &ctlr->rdr[ctlr->rdrx];
		dczap(dre, sizeof(BD));
		while(((status = dre->status) & BDEmpty) == 0){
			if(status & RxError || (status & (BDFirst|BDLast)) != (BDFirst|BDLast)){
				if(status & (RxeLG|RxeSH))
					ether->buffs++;
				if(status & RxeNO)
					ether->frames++;
				if(status & RxeCR)
					ether->crcs++;
				if(status & RxeOV)
					ether->overflows++;
				print("eth rx: %ux\n", status);
			}
			else{
				/*
				 * We have a packet. Read it in.
				 */
				len = dre->length-4;
				dczap(KADDR(dre->addr), len);
				if((b = iallocb(len)) != 0){
					memmove(b->wp, KADDR(dre->addr), len);
					b->wp += len;
					etheriq(ether, b, 1);
				}else
					ether->soverflows++;
			}

			/*
			 * Finished with this descriptor, reinitialise it,
			 * give it back to the chip, then on to the next...
			 */
			dre->length = 0;
			dre->status = (status & BDWrap) | BDEmpty | BDInt;
			dcflush(dre, sizeof(BD));

			ctlr->rdrx = NEXT(ctlr->rdrx, Nrdre);
			dre = &ctlr->rdr[ctlr->rdrx];
			dczap(dre, sizeof(BD));
		}
	}

	/*
	 * Transmitter interrupt: handle anything queued for a free descriptor.
	 */
	if(events & (TXB|TXE)){
		lock(ctlr);
		while(ctlr->ntq){
			dre = &ctlr->tdr[ctlr->tdri];
			dczap(dre, sizeof(BD));
			status = dre->status;
			if(status & BDReady)
				break;
			if(status & TxeDEF)
				ctlr->deferred++;
			if(status & TxeHB)
				ctlr->heartbeat++;
			if(status & TxeLC)
				ctlr->latecoll++;
			if(status & TxeRL)
				ctlr->retrylim++;
			if(status & TxeUN)
				ctlr->underrun++;
			if(status & TxeCSL)
				ctlr->carrierlost++;
			ctlr->retrycount += (status>>2)&0xF;
			b = ctlr->txb[ctlr->tdri];
			if(b == nil)
				panic("fcce/interrupt: bufp");
			ctlr->txb[ctlr->tdri] = nil;
			freeb(b);
			ctlr->ntq--;
			ctlr->tdri = NEXT(ctlr->tdri, Ntdre);
		}

		if(events & TXE)
			cpmop(RestartTx, ctlr->fccid, 0xc);

		txstart(ether);
		unlock(ctlr);
	}
}

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	char *p;
	int len;
	Ctlr *ctlr;

	if(n == 0)
		return 0;

	ctlr = ether->ctlr;

	p = malloc(READSTR);
	len = snprint(p, READSTR, "interrupts: %lud\n", ctlr->interrupts);
	len += snprint(p+len, READSTR-len, "carrierlost: %lud\n", ctlr->carrierlost);
	len += snprint(p+len, READSTR-len, "heartbeat: %lud\n", ctlr->heartbeat);
	len += snprint(p+len, READSTR-len, "retrylimit: %lud\n", ctlr->retrylim);
	len += snprint(p+len, READSTR-len, "retrycount: %lud\n", ctlr->retrycount);
	len += snprint(p+len, READSTR-len, "latecollisions: %lud\n", ctlr->latecoll);
	len += snprint(p+len, READSTR-len, "rxoverruns: %lud\n", ctlr->overrun);
	len += snprint(p+len, READSTR-len, "txunderruns: %lud\n", ctlr->underrun);
	snprint(p+len, READSTR-len, "framesdeferred: %lud\n", ctlr->deferred);
	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

/*
 * This follows the MPC8260 user guide: section28.9's initialisation sequence.
 */
static void
fccsetup(Ctlr *ctlr, FCC *fcc, uchar *ea)
{
	int i;
	Etherparam *p;

	/* Turn Ethernet off */
	fcc->gfmr &= ~(ENR | ENT);

	ioplock();
	switch(ctlr->port) {
	default:
		iopunlock();
		return;
	case 0:
		/* Step 1 (Section 28.9), write the parallel ports */
		ctlr->pmdio = 0x01000000;
		ctlr->pmdck = 0x08000000;
		iomem->port[0].pdir &= ~A1dir0;
		iomem->port[0].pdir |= A1dir1;
		iomem->port[0].psor &= ~A1psor0;
		iomem->port[0].psor |= A1psor1;
		iomem->port[0].ppar |= (A1dir0 | A1dir1);
		/* Step 2, Port C clocks */
		iomem->port[2].psor &= ~0x00000c00;
		iomem->port[2].pdir &= ~0x00000c00;
		iomem->port[2].ppar |= 0x00000c00;
		iomem->port[3].pdat |= (ctlr->pmdio | ctlr->pmdck);
		iomem->port[3].podr |= ctlr->pmdio;
		iomem->port[3].pdir |= (ctlr->pmdio | ctlr->pmdck);
		iomem->port[3].ppar &= ~(ctlr->pmdio | ctlr->pmdck);
		eieio();
		/* Step 3, Serial Interface clock routing */
		iomem->cmxfcr &= ~0xff000000;	/* Clock mask */
		iomem->cmxfcr |= 0x37000000;	/* Clock route */
		break;

	case 1:
		/* Step 1 (Section 28.9), write the parallel ports */
		ctlr->pmdio = 0x00400000;
		ctlr->pmdck = 0x00200000;
		iomem->port[1].pdir &= ~B2dir0;
		iomem->port[1].pdir |= B2dir1;
		iomem->port[1].psor &= ~B2psor0;
		iomem->port[1].psor |= B2psor1;
		iomem->port[1].ppar |= (B2dir0 | B2dir1);
		/* Step 2, Port C clocks */
		iomem->port[2].psor &= ~0x00003000;
		iomem->port[2].pdir &= ~0x00003000;
		iomem->port[2].ppar |= 0x00003000;

		iomem->port[2].pdat |= (ctlr->pmdio | ctlr->pmdck);
		iomem->port[2].podr |= ctlr->pmdio;
		iomem->port[2].pdir |= (ctlr->pmdio | ctlr->pmdck);
		iomem->port[2].ppar &= ~(ctlr->pmdio | ctlr->pmdck);
		eieio();
		/* Step 3, Serial Interface clock routing */
		iomem->cmxfcr &= ~0x00ff0000;
		iomem->cmxfcr |= 0x00250000;
		break;

	case 2:
		/* Step 1 (Section 28.9), write the parallel ports */
		iomem->port[1].pdir &= ~B3dir0;
		iomem->port[1].pdir |= B3dir1;
		iomem->port[1].psor &= ~B3psor0;
		iomem->port[1].psor |= B3psor1;
		iomem->port[1].ppar |= (B3dir0 | B3dir1);
		/* Step 2, Port C clocks */
		iomem->port[2].psor &= ~0x0000c000;
		iomem->port[2].pdir &= ~0x0000c000;
		iomem->port[2].ppar |= 0x0000c000;
		iomem->port[3].pdat |= (ctlr->pmdio | ctlr->pmdck);
		iomem->port[3].podr |= ctlr->pmdio;
		iomem->port[3].pdir |= (ctlr->pmdio | ctlr->pmdck);
		iomem->port[3].ppar &= ~(ctlr->pmdio | ctlr->pmdck);
		eieio();
		/* Step 3, Serial Interface clock routing */
		iomem->cmxfcr &= ~0x0000ff00;
		iomem->cmxfcr |= 0x00003700;
		break;
	}
	iopunlock();

	p = (Etherparam*)(m->imap->prmfcc + ctlr->port);
	memset(p, 0, sizeof(Etherparam));

	/* Step 4 */
//	fcc->gfmr |= (TCI | ENET);
	fcc->gfmr |= ENET;

	/* Step 5 */
	fcc->fpsmr = CRCE | FDE | LPB;		/* full duplex operation */
//	fcc->fpsmr = CRCE;		/* half duplex operation */

	/* Step 6 */
	fcc->fdsr = 0xd555;

	/* Step 7, initialize parameter ram */
	p->rbase = PADDR(ctlr->rdr);
	p->tbase = PADDR(ctlr->tdr);
	p->rstate = (GBL | EB) << 24;
	p->tstate = (GBL | EB) << 24;

	p->cmask = 0xdebb20e3;
	p->cpres = 0xffffffff;

	p->retlim = 15;	/* retry limit */

	p->mrblr = Bufsize;
	p->mflr = Rbsize;
	p->minflr = ETHERMINTU + 4;
	p->maxd1 = (Rbsize+3) & ~3;
	p->maxd2 = (Rbsize+3) & ~3;

	for(i=0; i<Eaddrlen; i+=2)
		p->paddr[2-i/2] = (ea[i+1]<<8)|ea[i];

	/* Step 7, initialize parameter ram, configuration-dependent values */
	p->riptr = m->imap->fccextra[ctlr->port].ri - (uchar*)INTMEM;
	p->tiptr = m->imap->fccextra[ctlr->port].ti - (uchar*)INTMEM;
	p->padptr = m->imap->fccextra[ctlr->port].pad - (uchar*)INTMEM;
	memset(m->imap->fccextra[ctlr->port].pad, 0x88, 0x20);

	/* Step 8, clear out events */
	fcc->fcce = ~0;

	/* Step 9, Interrupt enable */
	fcc->fccm = TXE | RXF | TXB;

	/* Step 10, Configure interrupt priority (not done here) */
	/* Step 11, Clear out current events */
	/* Step 12, Enable interrupts to the CP interrupt controller */

	/* Step 13, Issue the Init Tx and Rx command, specifying 0xc for ethernet*/
	cpmop(InitRxTx, fccid[ctlr->port], 0xc);

	// Step 14, Enable ethernet: done at attach time
}

static int
reset(Ether* ether)
{
	uchar ea[Eaddrlen];
	Ctlr *ctlr;
	FCC *fcc;

	if(m->cpuhz < 24000000){
		print("%s ether: system speed must be >= 24MHz for ether use\n", ether->type);
		return -1;
	}

	if(!(ether->port >= 0 && ether->port < 3)){
		print("%s ether: no FCC port %ld\n", ether->type, ether->port);
		return -1;
	}
	ether->irq = fccirq[ether->port];
	ether->tbdf = BusPPC;
	fcc = iomem->fcc + ether->port;

	ctlr = malloc(sizeof(*ctlr));
	ether->ctlr = ctlr;
	memset(ctlr, 0, sizeof(*ctlr));
	ctlr->fcc = fcc;
	ctlr->port = ether->port;
	ctlr->fccid = fccid[ether->port];

	/* Ioringinit will allocate the buffer descriptors in normal memory
	 * and NOT in Dual-Ported Ram, as prescribed by the MPC8260
	 * PowerQUICC II manual (Section 28.6).  When they are allocated
	 * in DPram and the Dcache is enabled, the processor will hang
	 */
	if(ioringinit(ctlr, Nrdre, Ntdre, Bufsize) < 0)
		panic("etherfcc init");

	fccsetup(ctlr, fcc, ether->ea);

	ether->mbps = 100;	/* TO DO: could be 10mbps */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;

	ether->arg = ether;
	ether->promiscuous = promiscuous;
	ether->multicast = multicast;

	/*
	 * Until we know where to find it, insist that the plan9.ini
	 * entry holds the Ethernet address.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0){
		print("no ether address");
		return -1;
	}

	return 0;
}

void
etherfcclink(void)
{
	addethercard("fcc", reset);
}
