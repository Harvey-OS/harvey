/*
 * FCCn ethernet
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "imm.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"
#include "../ppc/ethermii.h"

#define DBG 1

enum {
	Nrdre		= 128,			/* receive descriptor ring entries */
	Ntdre		= 128,			/* transmit descriptor ring entries */

	Rbsize		= ETHERMAXTU+4,		/* ring buffer size (+4 for CRC) */
	Bufsize		= Rbsize+CACHELINESZ,	/* extra room for alignment */
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
	RxError=	(RxeLG|RxeNO|RxeSH|RxeCR|RxeOV|RxeCL),	/* various error flags */

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
	CRCE=		BIT(24),	/* Ethernet CRC */
	FCE=		BIT(10),	/* flow control */
	PRO=		BIT(9),		/* promiscuous mode */
	FDE=		BIT(5),		/* full duplex ethernet */
	LPB=		BIT(3),		/* local protect bit */

	/* gfmr */
	ENET=		0xc,		/* ethernet mode */
	ENT=		BIT(27),
	ENR=		BIT(26),
	TCI=		BIT(2),

	/* FCC function code register */
	GBL=		0x20,
	BO=		0x18,
	EB=		0x10,		/* Motorola byte order */
	TC2=		0x04,
	DTB=		0x02,
	BDB=		0x01,

	/* FCC Event/Mask bits */
	GRA=		SBIT(8),
	RXC=		SBIT(9),
	TXC=		SBIT(10),
	TXE=		SBIT(11),
	RXF=		SBIT(12),
	BSY=		SBIT(13),
	TXB=		SBIT(14),
	RXB=		SBIT(15),
};

enum {		/* Mcr */
	MDIread	=	0x60020000,	/* read opcode */
	MDIwrite =	0x50020000,	/* write opcode */
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

typedef struct Ctlr Ctlr;
struct Ctlr {
	Lock;
	int	fccid;
	int	port;
	ulong	pmdio;
	ulong	pmdck;
	int	init;
	int	active;
	int	duplex;		/* 1 == full */
	FCC*	fcc;

	Ring;
	Block*	rcvbufs[Nrdre];
	Mii*	mii;
	Timer;

	ulong	interrupts;	/* statistics */
	ulong	deferred;
	ulong	heartbeat;
	ulong	latecoll;
	ulong	retrylim;
	ulong	underrun;
	ulong	overrun;
	ulong	carrierlost;
	ulong	retrycount;
};

static	int	fccirq[] = {0x20, 0x21, 0x22};
static	int	fccid[] = {FCC1ID, FCC2ID, FCC3ID};

#ifdef DBG
ulong fccrhisto[16];
ulong fccthisto[16];
ulong fccrthisto[16];
ulong fcctrhisto[16];
ulong ehisto[0x80];
#endif

static int fccmiimir(Mii*, int, int);
static int fccmiimiw(Mii*, int, int, int);
static void fccltimer(Ureg*, Timer*);

static void
attach(Ether *ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(ctlr);
	ctlr->active = 1;
	ctlr->fcc->gfmr |= ENR|ENT;
	iunlock(ctlr);
	ctlr->tmode = Tperiodic;
	ctlr->tf = fccltimer;
	ctlr->ta = ether;
	ctlr->tns = 5000000000LL;	/* 5 seconds */
	timeradd(ctlr);
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
/*		ctlr->fcc->ftodr = 1<<15;	/* transmit now; Don't do this according to errata */
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
	int len, status, rcvd, xmtd, restart;
	ushort events;
	Ctlr *ctlr;
	BD *dre;
	Block *b, *nb;
	Ether *ether = arg;

	ctlr = ether->ctlr;
	if(!ctlr->active)
		return;	/* not ours */

	/*
	 * Acknowledge all interrupts and whine about those that shouldn't
	 * happen.
	 */
	events = ctlr->fcc->fcce;
	ctlr->fcc->fcce = events;		/* clear events */

#ifdef DBG
	ehisto[events & 0x7f]++;
#endif

	ctlr->interrupts++;

	if(events & BSY)
		ctlr->overrun++;
	if(events & TXE)
		ether->oerrs++;

#ifdef DBG
	rcvd = xmtd = 0;
#endif
	/*
	 * Receiver interrupt: run round the descriptor ring logging
	 * errors and passing valid receive data up to the higher levels
	 * until we encounter a descriptor still owned by the chip.
	 */
	if(events & RXF){
		dre = &ctlr->rdr[ctlr->rdrx];
		dczap(dre, sizeof(BD));
		while(((status = dre->status) & BDEmpty) == 0){
			rcvd++;
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
			}else{
				/*
				 * We have a packet. Read it in.
				 */
				len = dre->length-4;
				b = ctlr->rcvbufs[ctlr->rdrx];
				assert(dre->addr == PADDR(b->rp));
				dczap(b->rp, len);
				if(nb = iallocb(Bufsize)){
					b->wp += len;
					etheriq(ether, b, 1);
					b = nb;
					b->rp = (uchar*)(((ulong)b->rp + CACHELINESZ-1) & ~(CACHELINESZ-1));
					b->wp = b->rp;
					ctlr->rcvbufs[ctlr->rdrx] = b;
					ctlr->rdr[ctlr->rdrx].addr = PADDR(b->wp);
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
		ilock(ctlr);
		restart = 0;
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
			if(status & (TxeLC|TxeRL|TxeUN))
				restart = 1;
			ctlr->retrycount += (status>>2)&0xF;
			b = ctlr->txb[ctlr->tdri];
			if(b == nil)
				panic("fcce/interrupt: bufp");
			ctlr->txb[ctlr->tdri] = nil;
			freeb(b);
			ctlr->ntq--;
			ctlr->tdri = NEXT(ctlr->tdri, Ntdre);
			xmtd++;
		}

		if(restart){
			ctlr->fcc->gfmr &= ~ENT;
			delay(10);
			ctlr->fcc->gfmr |= ENT;
			cpmop(RestartTx, ctlr->fccid, 0xc);
		}
		txstart(ether);
		iunlock(ctlr);
	}
#ifdef DBG
	if(rcvd >= nelem(fccrhisto))
		rcvd = nelem(fccrhisto) - 1;
	if(xmtd >= nelem(fccthisto))
		xmtd = nelem(fccthisto) - 1;
	if(rcvd)
		fcctrhisto[xmtd]++;
	else
		fccthisto[xmtd]++;
	if(xmtd)
		fccrthisto[rcvd]++;
	else
		fccrhisto[rcvd]++;
#endif
}

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	char *p;
	int len, i, r;
	Ctlr *ctlr;
	MiiPhy *phy;

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
	len += snprint(p+len, READSTR-len, "framesdeferred: %lud\n", ctlr->deferred);
	miistatus(ctlr->mii);
	phy = ctlr->mii->curphy;
	len += snprint(p+len, READSTR-len, "phy: link=%d, tfc=%d, rfc=%d, speed=%d, fd=%d\n",
		phy->link, phy->tfc, phy->rfc, phy->speed, phy->fd);

#ifdef DBG
	if(ctlr->mii != nil && ctlr->mii->curphy != nil){
		len += snprint(p+len, READSTR, "phy:   ");
		for(i = 0; i < NMiiPhyr; i++){
			if(i && ((i & 0x07) == 0))
				len += snprint(p+len, READSTR-len, "\n       ");
			r = miimir(ctlr->mii, i);
			len += snprint(p+len, READSTR-len, " %4.4uX", r);
		}
		snprint(p+len, READSTR-len, "\n");
	}
#endif
	snprint(p+len, READSTR-len, "\n");

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

/*
 * This follows the MPC8260 user guide: section28.9's initialisation sequence.
 */
static int
fccsetup(Ctlr *ctlr, FCC *fcc, uchar *ea)
{
	int i;
	Etherparam *p;
	MiiPhy *phy;

	/* Turn Ethernet off */
	fcc->gfmr &= ~(ENR | ENT);

	ioplock();
	switch(ctlr->port) {
	default:
		iopunlock();
		return -1;
	case 0:
		/* Step 1 (Section 28.9), write the parallel ports */
		ctlr->pmdio = 0x01000000;
		ctlr->pmdck = 0x08000000;
		imm->port[0].pdir &= ~A1dir0;
		imm->port[0].pdir |= A1dir1;
		imm->port[0].psor &= ~A1psor0;
		imm->port[0].psor |= A1psor1;
		imm->port[0].ppar |= (A1dir0 | A1dir1);
		/* Step 2, Port C clocks */
		imm->port[2].psor &= ~0x00000c00;
		imm->port[2].pdir &= ~0x00000c00;
		imm->port[2].ppar |= 0x00000c00;
		imm->port[3].pdat |= (ctlr->pmdio | ctlr->pmdck);
		imm->port[3].podr |= ctlr->pmdio;
		imm->port[3].pdir |= (ctlr->pmdio | ctlr->pmdck);
		imm->port[3].ppar &= ~(ctlr->pmdio | ctlr->pmdck);
		eieio();
		/* Step 3, Serial Interface clock routing */
		imm->cmxfcr &= ~0xff000000;	/* Clock mask */
		imm->cmxfcr |= 0x37000000;	/* Clock route */
		break;

	case 1:
		/* Step 1 (Section 28.9), write the parallel ports */
		ctlr->pmdio = 0x00400000;
		ctlr->pmdck = 0x00200000;
		imm->port[1].pdir &= ~B2dir0;
		imm->port[1].pdir |= B2dir1;
		imm->port[1].psor &= ~B2psor0;
		imm->port[1].psor |= B2psor1;
		imm->port[1].ppar |= (B2dir0 | B2dir1);
		/* Step 2, Port C clocks */
		imm->port[2].psor &= ~0x00003000;
		imm->port[2].pdir &= ~0x00003000;
		imm->port[2].ppar |= 0x00003000;

		imm->port[2].pdat |= (ctlr->pmdio | ctlr->pmdck);
		imm->port[2].podr |= ctlr->pmdio;
		imm->port[2].pdir |= (ctlr->pmdio | ctlr->pmdck);
		imm->port[2].ppar &= ~(ctlr->pmdio | ctlr->pmdck);
		eieio();
		/* Step 3, Serial Interface clock routing */
		imm->cmxfcr &= ~0x00ff0000;
		imm->cmxfcr |= 0x00250000;
		break;

	case 2:
		/* Step 1 (Section 28.9), write the parallel ports */
		imm->port[1].pdir &= ~B3dir0;
		imm->port[1].pdir |= B3dir1;
		imm->port[1].psor &= ~B3psor0;
		imm->port[1].psor |= B3psor1;
		imm->port[1].ppar |= (B3dir0 | B3dir1);
		/* Step 2, Port C clocks */
		imm->port[2].psor &= ~0x0000c000;
		imm->port[2].pdir &= ~0x0000c000;
		imm->port[2].ppar |= 0x0000c000;
		imm->port[3].pdat |= (ctlr->pmdio | ctlr->pmdck);
		imm->port[3].podr |= ctlr->pmdio;
		imm->port[3].pdir |= (ctlr->pmdio | ctlr->pmdck);
		imm->port[3].ppar &= ~(ctlr->pmdio | ctlr->pmdck);
		eieio();
		/* Step 3, Serial Interface clock routing */
		imm->cmxfcr &= ~0x0000ff00;
		imm->cmxfcr |= 0x00003700;
		break;
	}
	iopunlock();

	p = (Etherparam*)(m->immr->prmfcc + ctlr->port);
	memset(p, 0, sizeof(Etherparam));

	/* Step 4 */
	fcc->gfmr |= ENET;

	/* Step 5 */
	fcc->fpsmr = CRCE | FDE | LPB;	/* full duplex operation */
	ctlr->duplex = ~0;

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

	p->mrblr = (Rbsize+0x1f)&~0x1f;		/* multiple of 32 */
	p->mflr = Rbsize;
	p->minflr = ETHERMINTU;
	p->maxd1 = (Rbsize+7) & ~7;
	p->maxd2 = (Rbsize+7) & ~7;

	for(i=0; i<Eaddrlen; i+=2)
		p->paddr[2-i/2] = (ea[i+1]<<8)|ea[i];

	/* Step 7, initialize parameter ram, configuration-dependent values */
	p->riptr = m->immr->fccextra[ctlr->port].ri - (uchar*)IMMR;
	p->tiptr = m->immr->fccextra[ctlr->port].ti - (uchar*)IMMR;
	p->padptr = m->immr->fccextra[ctlr->port].pad - (uchar*)IMMR;
	memset(m->immr->fccextra[ctlr->port].pad, 0x88, 0x20);

	/* Step 8, clear out events */
	fcc->fcce = ~0;

	/* Step 9, Interrupt enable */
	fcc->fccm = TXE | RXF | TXB;

	/* Step 10, Configure interrupt priority (not done here) */
	/* Step 11, Clear out current events */
	/* Step 12, Enable interrupts to the CP interrupt controller */

	/* Step 13, Issue the Init Tx and Rx command, specifying 0xc for ethernet*/
	cpmop(InitRxTx, fccid[ctlr->port], 0xc);

	/* Step 14, Link management */
	if((ctlr->mii = malloc(sizeof(Mii))) == nil)
		return -1;
	ctlr->mii->mir = fccmiimir;
	ctlr->mii->miw = fccmiimiw;
	ctlr->mii->ctlr = ctlr;

	if(mii(ctlr->mii, ~0) == 0 || (phy = ctlr->mii->curphy) == nil){
		free(ctlr->mii);
		ctlr->mii = nil;
		return -1;
	}
	miiane(ctlr->mii, ~0, ~0, ~0);
#ifdef DBG
	print("oui=%X, phyno=%d, ", phy->oui, phy->phyno);
	print("anar=%ux, ", phy->anar);
	print("fc=%ux, ", phy->fc);
	print("mscr=%ux, ", phy->mscr);

	print("link=%ux, ", phy->link);
	print("speed=%ux, ", phy->speed);
	print("fd=%ux, ", phy->fd);
	print("rfc=%ux, ", phy->rfc);
	print("tfc=%ux\n", phy->tfc);
#endif
	/* Step 15, Enable ethernet: done at attach time */
	return 0;
}

static int
reset(Ether* ether)
{
	uchar ea[Eaddrlen];
	Ctlr *ctlr;
	FCC *fcc;
	Block *b;
	int i;

	if(m->cpuhz < 24000000){
		print("%s ether: system speed must be >= 24MHz for ether use\n", ether->type);
		return -1;
	}

	if(ether->port > 3){
		print("%s ether: no FCC port %ld\n", ether->type, ether->port);
		return -1;
	}
	ether->irq = fccirq[ether->port];
	ether->tbdf = BusPPC;
	fcc = imm->fcc + ether->port;

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
	if(ioringinit(ctlr, Nrdre, Ntdre, 0) < 0)
		panic("etherfcc init");
	for(i = 0; i < Nrdre; i++){
		b = iallocb(Bufsize);
		b->rp = (uchar*)(((ulong)b->rp + CACHELINESZ-1) & ~(CACHELINESZ-1));
		b->wp = b->rp;
		ctlr->rcvbufs[i] = b;
		ctlr->rdr[i].addr = PADDR(b->wp);
	}

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

static void
nanodelay(void)
{
	static int count;
	int i;

	for(i = 0; i < 500; i++)
		count++;
	return;
}

static
void miiwriteloop(Ctlr *ctlr, Port *port, int cnt, ulong cmd)
{
	int i;

	for(i = 0; i < cnt; i++){
		port->pdat &= ~ctlr->pmdck;
		if(cmd & BIT(i))
			port->pdat |= ctlr->pmdio;
		else
			port->pdat &= ~ctlr->pmdio;
		nanodelay();
		port->pdat |= ctlr->pmdck;
		nanodelay();
	}
}

static int
fccmiimiw(Mii *mii, int pa, int ra, int data)
{
	int x;
	Port *port;
	ulong cmd;
	Ctlr *ctlr;

	/*
	 * MII Management Interface Write.
	 */

	ctlr = mii->ctlr;
	port = imm->port + 3;
	cmd = MDIwrite | (pa<<(5+2+16))| (ra<<(2+16)) | (data & 0xffff);

	x = splhi();

	port->pdir |= (ctlr->pmdio|ctlr->pmdck);
	nanodelay();

	miiwriteloop(ctlr, port, 32, ~0);
	miiwriteloop(ctlr, port, 32, cmd);

	port->pdir |= (ctlr->pmdio|ctlr->pmdck);
	nanodelay();

	miiwriteloop(ctlr, port, 32, ~0);

	splx(x);
	return 1;
}

static int
fccmiimir(Mii *mii, int pa, int ra)
{
	int data, i, x;
	Port *port;
	ulong cmd;
	Ctlr *ctlr;

	ctlr = mii->ctlr;
	port = imm->port + 3;

	cmd = MDIread | pa<<(5+2+16) | ra<<(2+16);

	x = splhi();
	port->pdir |= (ctlr->pmdio|ctlr->pmdck);
	nanodelay();

	miiwriteloop(ctlr, port, 32, ~0);

	/* Clock out the first 14 MS bits of the command */
	miiwriteloop(ctlr, port, 14, cmd);

	/* Turn-around */
	port->pdat &= ~ctlr->pmdck;
	port->pdir &= ~ctlr->pmdio;
	nanodelay();

	/* For read, clock in 18 bits, use 16 */
	data = 0;
	for(i=0; i<18; i++){
		data <<= 1;
		if(port->pdat & ctlr->pmdio)
			data |= 1;
		port->pdat |= ctlr->pmdck;
		nanodelay();
		port->pdat &= ~ctlr->pmdck;
		nanodelay();
	}
	port->pdir |= (ctlr->pmdio|ctlr->pmdck);
	nanodelay();
	miiwriteloop(ctlr, port, 32, ~0);
	splx(x);
	return data & 0xffff;
}

static void
fccltimer(Ureg*, Timer *t)
{
	Ether *ether;
	Ctlr *ctlr;
	MiiPhy *phy;
	ulong gfmr;

	ether = t->ta;
	ctlr = ether->ctlr;
	if(ctlr->mii == nil || ctlr->mii->curphy == nil)
		return;
	phy = ctlr->mii->curphy;
	if(miistatus(ctlr->mii) < 0){
		print("miistatus failed\n");
		return;
	}
	if(phy->link == 0){
		print("link lost\n");
		return;
	}
	ether->mbps = phy->speed;

	if(phy->fd != ctlr->duplex)
		print("set duplex\n");
	ilock(ctlr);
	gfmr = ctlr->fcc->gfmr;
	if(phy->fd != ctlr->duplex){
		ctlr->fcc->gfmr &= ~(ENR|ENT);
		if(phy->fd)
			ctlr->fcc->fpsmr |= FDE | LPB;		/* full duplex operation */
		else
			ctlr->fcc->fpsmr &= ~(FDE | LPB);	/* half duplex operation */
		ctlr->duplex = phy->fd;
	}
	ctlr->fcc->gfmr = gfmr;
	iunlock(ctlr);
}
