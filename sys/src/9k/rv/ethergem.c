/*
 * Gb Ethernet Mac (GEM) from microsemi/microchip,
 * which is actually the Cadence GEM, used, for example,
 * in the Xilinx Zynq-7000 SoC.
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

#include "riscv.h"

#undef	DMA64		/* apparently polarfire lacks the 64-bit support */

typedef struct Ctlr Ctlr;
typedef struct Rd Rd;

enum {
	Rbsz	= ROUNDUP(ETHERMAXTU+32, 64),	/* manual recommends 1600 */
	Descalign= 8,

	/* tunable parameters */
	/*
	 * these were Nrd=256, Nrb=1024 & Ntd=128, but Nrd=30, Nrb=47 & Ntd=1
	 * are usually ample; however systems can need more receive buffers
	 * due to bursts of (non-9P) traffic.
	 */
	Ntd	= 64,		/* power of 2, >= 64 */
	Nrd	= 64,		/* power of 2, >= 64 */
	Nrb	= 8*Nrd,	/* private receive buffers, >> Nrd */
};

enum {				/* registers, by ulong* index */
	Ctl	= 0,
	Cfg	= 4/4,
	Sts	= 8/4,
	Dmacfg	= 0x10/4,
	Txsts	= 0x14/4,
	Rxdescq	= 0x18/4,	/* write: desc list base; read: current desc */
	Txdescq	= 0x1c/4,	/* write: desc list base; read: current desc */
	Rxsts	= 0x20/4,
	Intsts	= 0x24/4,
	Intena	= 0x28/4,	/* write only; same bits as Intsts */
	Intdis	= 0x2c/4,	/* write only; same bits as Intsts */
	Phymgmt	= 0x34/4,
	Hashbot	= 0x80/4,	/* low bits of multicast hash filter */
	Hashtop	= 0x84/4,
	Specaddr1bot	= 0x88/4,	/* our mac address */
	Specaddr1top	= 0x8c/4,
	Laddr2l		= 0x90/4,	/* zero all ... */
	Match4off	= 0xb4/4,	/* ... of these */
	Specaddr1maskbot = 0xc8/4,
	Specaddr1masktop = 0xcc/4,
	Pcsctl	= 0x200/4,
	Pcssts	= 0x204/4,
	Pcsanadv= 0x210/4,
	Txdescqhi = 0x4c8/4,
	Rxdescqhi = 0x4d4/4,
};

enum {				/* Ctl bits */
	Rxstudpoff = 1<<22,	/* store udp or tcp offset in crc's high long */
	Rxaccptpunicast = 1<<20, /* enable receiving PTP unicast packets */
	Txhalt	= 1<<10,	/* stop output dma (wo) */
	Txstart	= 1<<9,		/* resume output dma (wo) */
	Statsclr= 1<<5,		/* (wo) */
	Mden	= 1<<4,		/* mgmt port enable */
	Txallow	= 1<<3,	/* also sets tx q ptr to start of tx desc list */
	Rxallow	= 1<<2,
};

enum {					/* Cfg bits */
	Sgmii		= 1<<27,	/* enable gb mii, thus gb speed */
	Rxigncrc	= 1<<26,
	Rxckoffl	= 1<<24,	/* rx checksum offload: drop bad pkts */
	Mdcclkdivshft	= 18,
	Mdcclkdiv	= 7<<18,	/* default 2; 4 for Gb/s */
	Rxomitfcs	= 1<<17,	/* rx omit crc in memory */
	Pcsmand		= 1<<11,	/* not TBI */
	Gbmode		= 1<<10,	/* gigabit mode enable */
	Rxunihashfilt	= 1<<7,
	Rxmultihashfilt	= 1<<6,
	Rxnobcast	= 1<<5,		/* discard broadcasts */
	Caf		= 1<<4,		/* copy all frames (promiscuous mode) */
	Fd		= 1<<1,		/* full duplex */
};

enum {					/* Dmacfg bits */
	Dmaaddr64	= 1 << 30,	/* use 64-bit dma addresses */
	Txextbd		= 1 << 29,	/* extended tx buffer descriptors */
	Rxextbd		= 1 << 28,	/* extended rx buffer descriptors */
	Rxbsshft	= 16,
	Rxbufsize	= MASK(8)<<Rxbsshft,	/* rx buffer size / 64 */
	Bepkt		= 1 << 7,	/* big-endian packet data swap; def 1 */
	Bemgmt		= 1 << 6,	/* big-endian mgmt desc access */
	/* ether0 has 4 (incr16) set, ether1 has 3 (incr8) set */
	Blen		= MASK(5),	/* dma burst len; def 4 (incr4) */
};

enum {					/* Txsts bits */
	/* all rw bits are cleared by writing 1 */
	Txdone		= 1<<5,
	Txretries	= 1<<2,		/* too many retries */
	Txcoll		= 1<<1,		/* collision */
	Txusedbitread	= 1<<0,	/*  tx buf desc read with its used bit set */
};

enum {					/* Rxsts bits */
	/* all rw bits are cleared by writing 1 */
	Rxoverrun	= 1<<2,
	Rxrcvd		= 1<<1,
	Rxbufnotavail	= 1<<0,
};

enum {					/* Int* bits */
	/* all rw bits are cleared by writing 1 */
	Linkupdate	= 1<<17,
	Autonegdone	= 1<<16,
	Extintr		= 1<<15,
	Rxoverrunint	= 1<<10,
	Linkchange	= 1<<9,
	Txdoneint	= 1<<7,
	Txretriesint	= 1<<5,		/* too many retries */
	Txunderrun	= 1<<4,
	Txusedbitreadint= 1<<3,
	Rxusedbitreadint= 1<<2,
	Rxrcvdint	= 1<<1,
};

enum {					/* Phymgmt */
	Phyaddrshft	= 23,
	Phyaddrmask	= MASK(5)<<23,
};

enum {					/* Pcsctl bits */
	Pcsswreset	= 1<<15,	/* software reset */
	Pcsenan		= 1<<12,	/* enable auto negotiation */
	Pcsrestartan	= 1<<9,		/* restart " " */
};

enum {					/* Pcssts bits */
	Andone		= 1<<5,		/* auto. neg. done */
	Linksts		= 1<<2,		/* link status */
};

typedef struct {
	uint	reg;
	char	*name;
} Stat;

static Stat stattab[] = {
	0,	nil,
};

/* this is a register layout, which can't tolerate bogus padding */
struct Rd {			/* Receive Descriptor */
	ulong	addr;		/* low physical */
	ulong	size_flags;
#ifdef DMA64
	ulong	addrhi;
	ulong	_0_;
#endif
};

enum {				/* Rd->addr bits (other than rcv buffer addr) */
	Rdlast	= 1<<1,		/* last Rd */
	Full	= 1<<0,		/* pkt recv'd; setting to 0 allows input */
};
enum {				/* interesting Rd->size_flags bits */
	Rdbcast	= 1u<<31,
	Rdmcast	= 1<<30,
	Rducast	= 1<<29,
	Rdsumscked = MASK(2)<<22,
	Rdvlan	= 1<<21,
	Rdeofr	= 1<<15,	/* buffer contains end of frame (packet) */
	Rdsofr	= 1<<14,	/* buffer contains start of frame (packet) */
	Rdbadfcs= 1<<13,	/* bad crc & ignore-fcs enabled */
	Rdpktsize= MASK(13),
};

#define Td Rd			/* Transmit Descriptor */

/* Td->addr is a byte address; there are no other bits in it */

enum {				/* Td->size_flags bits */
	Sent	= 1u<<31,	/* tx done; setting to 0 enables output */
	Tdlast	= 1<<30,	/* last Td */
	Tdretries= 1<<29,
	Tdcorrupt= 1<<27,
	Tdlatecoll= 1<<26,
	Tdckerrs=  MASK(3) << 20,
	Tdaddnocrc= 1<<16,
	Tdeofr	= 1<<15,	/* buffer contains end of frame (packet) */
	Tdpktsize= MASK(14),
};

struct Ctlr {
	Ethident;		/* see etherif.h */

	Lock	reglock;
	uint	im;		/* interrupt mask (enables) */
	uint	lim;
	uint	rim;
	uint	tim;

	Rendez	rrendez;
	Rd*	rdba;		/* receive descriptor base address */
	uint	rdh;		/* " " head */
	uint	rdt;		/* " " tail */
	Block**	rb;		/* " buffers */
	uint	rintr;
	uint	rsleep;
	Watermark wmrd;
	Watermark wmrb;
	int	rdfree;		/* " descriptors awaiting packets */

	Rendez	trendez;
	Td*	tdba;		/* transmit descriptor base address */
	uint	tdh;		/* " " head */
	uint	tdt;		/* " " tail */
	Block**	tb;		/* " buffers */
	uint	tintr;
	uint	tsleep;
	Watermark wmtd;
	QLock	tlock;
	ulong	txtick;		/* tick at start of last tx start */

	Rendez	lrendez;

	uchar	flag;
	int	procsrunning;	/* flag: kprocs started for this Ctlr? */
	QLock	alock;		/* attach lock */
	int	attached;

	uchar	ra[Eaddrlen];	/* receive address */

	QLock	slock;
	ulong	stats[nelem(stattab)];

	uint	ixcs;		/* valid hw checksum count */
	uint	ipcs;		/* good hw ip checksums */
	uint	tcpcs;		/* good hw tcp/udp checksums */
};

enum {				/* flag bits */
	Factive	= 1<<0,
};

static	Ctlr	*ctlrtab[8];
static	int	nctlr;

/* these are shared by all the controllers of this type */
static	Lock	rblock;
static	Block	*rbpool;
/* # of rcv Blocks awaiting processing; can be briefly < 0 */
static	int	nrbfull;
static	int	nrbavail;

static void	replenish(Ctlr *ctlr);

/* wait up to a second for bit in ctlr->regs[reg] to become zero */
static int
awaitregbitzero(Ctlr *ctlr, int reg, ulong bit)
{
	return awaitbitpat(&ctlr->regs[reg], bit, 0);
}

static void
readstats(Ctlr *ctlr)
{
	uint i, reg;

	qlock(&ctlr->slock);
	for(i = 0; i < nelem(ctlr->stats); i++) {
		reg = stattab[i].reg;
		ctlr->stats[i] += ctlr->regs[reg / BY2WD];
	}
	qunlock(&ctlr->slock);
}

static long
ifstat(Ether *edev, void *a, long n, ulong offset)
{
	uint i;
	char *s, *p, *e;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	p = s = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	e = p + READSTR;

	readstats(ctlr);
	for(i = 0; i < nelem(stattab); i++)
		if(stattab[i].name != nil && ctlr->stats[i] > 0)
			p = seprint(p, e, "%s\t%uld\n", stattab[i].name,
				ctlr->stats[i]);
	p = seprint(p, e, "%æ\n", ctlr);
	p = seprint(p, e, "ctl %#ux\n",  ctlr->regs[Ctl]);
	p = seprint(p, e, "cfg %#ux\n", ctlr->regs[Cfg]);
	p = seprint(p, e, "dmacfg %#ux\n", ctlr->regs[Dmacfg]);
	p = seprint(p, e, "rintr %d rsleep %d\n", ctlr->rintr, ctlr->rsleep);
	p = seprint(p, e, "tintr %d tsleep %d\n", ctlr->tintr, ctlr->tsleep);
	p = seprintmark(p, e, &ctlr->wmrb);
	p = seprintmark(p, e, &ctlr->wmrd);
	seprintmark(p, e, &ctlr->wmtd);
	n = readstr(offset, a, n, s);
	free(s);

	return n;
}

static void
ienable(Ctlr *ctlr, int ie)
{
	ilock(&ctlr->reglock);
	ctlr->im |= ie;
	ctlr->regs[Intena] = ctlr->im;
	iunlock(&ctlr->reglock);
}

static int
lim(void *v)
{
	return ((Ctlr*)v)->lim != 0;
}

static void
lproc(void *v)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (;;) {
		ienable(ctlr, Linkchange);
		sleep(&ctlr->lrendez, lim, ctlr);
		ctlr->lim = 0;
		if (ctlr->regs[Pcssts] & Andone)
			edev->link = (ctlr->regs[Pcssts] & Linksts) != 0;
	}
}

/* return a Block from our pool */
static Block*
rballoc(void)
{
	Block *bp;

	ilock(&rblock);
	if((bp = rbpool) != nil){
		rbpool = bp->next;
		bp->next = nil;
		adec(&nrbavail);
		if (nrbavail < 0)
			print("rballoc: nrbavail negative\n");
	}
	iunlock(&rblock);
	return bp;
}

/* called from freeb for receive Blocks, returns them to our pool */
void
rbfree(Block *bp)
{
	bp->wp = bp->rp = (uchar *)ROUNDDN((uintptr)bp->lim - Rbsz, BLOCKALIGN);
	assert(bp->rp >= bp->base);
	assert(((uintptr)bp->rp & (BLOCKALIGN-1)) == 0);
	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(&rblock);
	bp->next = rbpool;
	rbpool = bp;
	adec(&nrbfull);
	ainc(&nrbavail);
	iunlock(&rblock);
}

/* reclaim sent buffers & descriptors */
static void
cleanup(Ctlr *ctlr)
{
	uint i, n, tdh;
	Block *bp;
	Td *td;

	tdh = ctlr->tdh;
	i = 0;
	n = NEXT(tdh, Ntd);
	while (td = &ctlr->tdba[n], td->size_flags & Sent && n != ctlr->tdt &&
	    i++ < Ntd-2) {
		tdh = n;
		bp = ctlr->tb[tdh];
		ctlr->tb[tdh] = nil;
		if (bp)
			freeb(bp);
		/*
		 * clearing Sent here would allow this (non-existent) buffer
		 * to be xmitted.
		 */
		n = NEXT(tdh, Ntd);
	}
	ctlr->tdh = tdh;
}

void
transmit(Ether *edev)
{
	Block *bp;
	Ctlr *ctlr;
	Td *td;
	uint nqd, tdt, tdh;

	ctlr = edev->ctlr;
	if(!canqlock(&ctlr->tlock)){
		ienable(ctlr, Txdoneint);
		return;
	}
	if (!ctlr->attached) {
		qunlock(&ctlr->tlock);
		return;
	}
	cleanup(ctlr);				/* free transmitted buffers */
	tdh = ctlr->tdh;
	tdt = ctlr->tdt;
	for(nqd = 0; ; nqd++){
		if(NEXT(tdt, Ntd) == tdh){	/* ring full? */
			ienable(ctlr, Txdoneint);
			break;
		}
		if((bp = qget(edev->oq)) == nil)
			break;

		assert(ctlr->tdba != nil);
		td = &ctlr->tdba[tdt];
#ifdef DMA64
		td->addrhi = (uvlong)PCIWADDR(bp->rp) >> 32;
#endif
		td->addr = PCIWADDR(bp->rp);
		coherence();
		/* clears Sent in size_flags, allows xmit */
		td->size_flags = (td->size_flags & Tdlast) | Tdeofr | BLEN(bp);
		coherence();

		if (ctlr->tb[tdt])
			panic("ethergem: %æ: xmit q full", ctlr);
		ctlr->tb[tdt] = bp;

		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (uint)(tdt + Ntd - tdh) % Ntd);
		tdt = NEXT(tdt, Ntd);
	}
	ilock(&ctlr->reglock);
	if(nqd) {
		ctlr->tdt = tdt;
		ctlr->regs[Txsts] |= Txdone|Txusedbitread;	/* reset */
		coherence();
		ctlr->txtick = sys->ticks;
	}
#ifdef	PARANOIA
	ctlr->regs[Ctl] |= Txallow|Mden;
	coherence();
#endif
	ctlr->regs[Ctl] |= Txstart;		/* kick transmitter */
#ifdef	PARANOIA
	coherence();
	ctlr->regs[Ctl] |= Txallow;
#endif
	iunlock(&ctlr->reglock);
	ienable(ctlr, Txdoneint);
	qunlock(&ctlr->tlock);
}

static int
tim(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	return ctlr->tim != 0 || qlen(ctlr->edev->oq) > 0 &&
		ctlr->tdba[ctlr->tdt].size_flags & Sent;
}

static void
tproc(void *v)
{
	ulong txtick;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (;;) {
		ctlr->tsleep++;
		/* the time-out is only for diagnosis */
		tsleep(&ctlr->trendez, tim, ctlr, 1000);
		ctlr->tim = 0;
		txtick = ctlr->txtick;
		if (txtick && sys->ticks - txtick > 2*HZ) {
			print("%æ: not transmitting\n", ctlr);
			ctlr->txtick = 0;
		}
		/*
		 * perhaps some buffers have been transmitted and their
		 * descriptors can be reused to copy Blocks out of edev->oq.
		 */
		transmit(edev);
	}
}

/* free any buffer Blocks in an array */
static void
freeblks(Block **blks, int count)
{
	Block *bp;

	if (blks == nil)
		return;
	while(count-- > 0){
		bp = blks[count];
		blks[count] = nil;
		if (bp) {
			bp->free = nil;
			freeb(bp);
		}
	}
}

static void
initrxring(Ctlr *ctlr, Rd *rdbase, int ndescs)
{
	int i;
	Rd *rd;

	ctlr->rdba = rdbase;
	memset(ctlr->rdba, 0, ndescs * sizeof(Rd));
	for (i = 0; i < ndescs; i++) {
		rd = &ctlr->rdba[i];
		rd->addr = Full;	/* prevent rcv filling for now */
		rd->size_flags = Rbsz;
	}
	ctlr->rdba[ndescs - 1].addr |= Rdlast;
	coherence();
}

static void
inittxring(Ctlr *ctlr, Td *tdbase, int ndescs)
{
	int i;
	Td *td;

	ctlr->tdba = tdbase;
	memset(ctlr->tdba, 0, ndescs * sizeof(Td));
	for (i = 0; i < ndescs; i++) {
		td = &ctlr->tdba[i];
		td->size_flags = Sent;		/* available to fill to send */
	}
	ctlr->tdba[ndescs - 1].size_flags |= Tdlast;
	coherence();
}

static void
rxon(Ctlr *ctlr)
{
	ilock(&ctlr->reglock);
	ctlr->regs[Ctl] |= Rxallow;
	coherence();
	iunlock(&ctlr->reglock);
	ienable(ctlr, Rxrcvdint);
}

/*
 * Get the receiver into a usable state.  Some of this is boilerplate
 * that could be (or is) done automatically as part of reset (hint, hint),
 * but we also disable broken features (e.g., IP checksums).
 *
 * We're called before txinit, so arrange link auto negotiation here.
 */
static void
rxinit(Ctlr *ctlr)
{
	uint *regs;
	ulong cfg;
	uvlong physbda;

	regs = ctlr->regs;
	ilock(&ctlr->reglock);
	regs[Ctl] &= ~Rxallow;		/* allow state is unknown: clear it */
	coherence();
	delay(10);
	ctlr->rdfree = 0;

	regs[Pcsctl] |= Pcsenan;
	cfg = regs[Cfg];
	cfg &= ~(Rxnobcast|Rxckoffl|Mdcclkdiv);
	cfg |= Pcsmand | Rxomitfcs | Fd | Gbmode | Sgmii |
		Rxunihashfilt | Rxmultihashfilt | 4<<Mdcclkdivshft;
	regs[Cfg] = cfg;

	cfg = regs[Dmacfg];
	cfg &= ~(Rxbufsize|Blen);
	cfg |= HOWMANY(Rbsz, 64) << Rxbsshft | 4;	/* 4 = incr16 */
#ifdef DMA64
	cfg |= Dmaaddr64 | Txextbd | Rxextbd;
#else
	cfg &= ~(Dmaaddr64 | Txextbd | Rxextbd);
#endif
	regs[Dmacfg] = cfg;

	regs[Hashtop] = regs[Hashbot] = ~0;	/* accept all for now */
	iunlock(&ctlr->reglock);

	/* set up the rcv ring */
	initrxring(ctlr, ctlr->rdba, Nrd);
	/* don't need to use uncached memory nor addresses on icicle */
	physbda = (uvlong)PADDR(ctlr->rdba);
	ilock(&ctlr->reglock);
	regs[Rxdescq] = physbda;
	regs[Rxdescqhi] = physbda >> 32;
	iunlock(&ctlr->reglock);
	replenish(ctlr);
	rxon(ctlr);
}

static void
replenish(Ctlr *ctlr)
{
	uint rdt, rdh;
	Block *bp;
	Block **rb;
	Rd *rd;

	rdh = ctlr->rdh;
	for(rdt = ctlr->rdt; NEXT(rdt, Nrd) != rdh; rdt = NEXT(rdt, Nrd)){
		rd = &ctlr->rdba[rdt];
		rb = &ctlr->rb[rdt];
		if(*rb != nil){
			iprint("%æ: rx overrun\n", ctlr);
			break;
		}
		*rb = bp = rballoc();
		if(bp == nil)		/* don't have a buffer for this desc? */
			break;
		rd->size_flags = Rbsz;
#ifdef DMA64
		rd->addrhi = (uvlong)PCIWADDR(bp->rp) >> 32;
#endif
		coherence();
		/* Full=0 allows reception */
		rd->addr = rd->addr & Rdlast | PCIWADDR(bp->rp);
		coherence();
		ctlr->rdfree++;
	}
	ctlr->rdt = rdt;
}

static int
rim(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	return ctlr->rim != 0 || ctlr->rdba[ctlr->rdh].addr & Full;
}

static void
ckcksum(Ctlr *ctlr, Rd *, Block *bp)
{
	ctlr->ixcs++;
	bp->flag |= Bpktck;
}

enum {
	Rxdonebits = Rxoverrun | Rxbufnotavail | Rxrcvd,
};

static int
qinpkt(Ctlr *ctlr)
{
	int passed;
	uint rdh, len;
	Block *bp;
	Rd *rd;

	ctlr->rim = 0;
	rdh = ctlr->rdh;
	rd = &ctlr->rdba[rdh];
	if (!(rd->addr & Full))
		return -1;		/* wait for pkts to arrive */

	/*
	 * Accept packets with no errors.
	 */
	passed = 0;
	coherence();
	bp = ctlr->rb[rdh];
	if ((ctlr->regs[Rxsts] & Rxdonebits) == Rxrcvd){
		/* no overrun and buffer available */
		if (bp == nil)
			panic("%æ: nil Block* from ctlr->rb", ctlr);
		len = rd->size_flags & Rdpktsize;
		if (len == ETHERMINTU)
			len += 4;	/* for 60-byte arps? */
		if (len <= ETHERMAXTU)
			bp->wp += len;
		ckcksum(ctlr, rd, bp);
		notemark(&ctlr->wmrb, ainc(&nrbfull));
		coherence();
		/* pass pkt upstream, where it will be freed eventually */
		etheriq(ctlr->edev, bp, 1);
		passed++;
	} else {
		ainc(&nrbfull);
		freeb(bp);			/* toss bad pkt */
		ilock(&ctlr->reglock);
		ctlr->regs[Rxsts] |= Rxdonebits; /* reset done bits */
		iunlock(&ctlr->reglock);
	}

	ctlr->rb[rdh] = nil;
	ctlr->rdh = NEXT(rdh, Nrd);
	ctlr->rdfree--;
	/*
	 * if number of rds ready for packets is too low (more
	 * than 16 in use), set up the unready ones.
	 */
	if (ctlr->rdfree <= Nrd - 16)
		replenish(ctlr);
	return passed;
}

static void
rproc(void *v)
{
	int passed, npass;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (ctlr->rdh = 0; ; ) {
		replenish(ctlr);
		/*
		 * Prevent an idle or unplugged interface from interrupting.
		 * Allow receiver interrupts initially and again
		 * if the interface (and transmitter) see actual use.
		 */
		if (edev->outpackets > 10 || ctlr->rintr < 2*Nrd)
			ienable(ctlr, Rxrcvdint);
		ctlr->rsleep++;
		sleep(&ctlr->rrendez, rim, ctlr);

		for(passed = 0; (npass = qinpkt(ctlr)) >= 0; passed += npass)
			;
		/* note how many rds had full buffers */
		notemark(&ctlr->wmrd, passed);
	}
}

static void
promiscuous(void *a, int on)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = a;
	ctlr = edev->ctlr;
	ilock(&ctlr->reglock);
	if(on)
		ctlr->regs[Cfg] |= Caf;
	else
		ctlr->regs[Cfg] &= ~Caf;
	iunlock(&ctlr->reglock);
}

/* see §16.2.3 of the Xilinx Zynq-7000 SoC Tech. Ref. Man. */
static ulong
mcasthash(uchar *mac)
{
	uint chr, bit, hash;

	hash = 0;
	for (chr = 0; chr < Eaddrlen; chr++)
		for (bit = chr; bit < 8*Eaddrlen; bit += 6)
			if (mac[bit>>3] & (1 << (bit & 7)))
				hash ^= 1 << chr;
	return hash;
}

static void
multicast(void *a, uchar *addr, int on)
{
	ulong hash, word, bit;
	Ctlr *ctlr;
	Ether *edev;

	edev = a;
	if (edev == nil)
		panic("multicast: nil edev arg");
	ctlr = edev->ctlr;
	if (ctlr == nil)
		panic("multicast: nil edev->ctlr");

	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	hash = mcasthash(addr);
	word = Hashbot + (hash/BI2WD) % 2;
	bit = 1UL << (hash % BI2WD);
	ilock(&ctlr->reglock);
	if(on)
		ctlr->regs[word] |= bit;
//	else
//		ctlr->regs[word] &= ~bit;
	iunlock(&ctlr->reglock);
	coherence();
}

static void
freez(void **pptr)
{
	free(*pptr);
	*pptr = nil;
}

static void
freemem(Ctlr *ctlr)
{
	int i;
	Block *bp;

	/* only free enough rcv bufs for one controller */
	for (i = Nrb; i > 0 && (bp = rballoc()) != nil; i--){
		bp->free = nil;
		freeb(bp);
	}
	freez(&ctlr->rdba);
	freez(&ctlr->tdba);
	freez(&ctlr->rb);
	freez(&ctlr->tb);
}

/* don't discard all state; we may be attached again */
static int
detach(Ctlr *ctlr)
{
	uint *regs;

	regs = ctlr->regs;
	ilock(&ctlr->reglock);
	regs[Intdis] = ~0;
	regs[Ctl] |= Txhalt;
	coherence();
	regs[Ctl] &= ~(Txallow | Rxallow);
	iunlock(&ctlr->reglock);
	delay(10);

	ctlr->attached = 0;
	return 0;
}

static void
shutdown(Ether *edev)
{
	detach(edev->ctlr);
//	freemem(edev->ctlr);
}

uchar ether0mac[];

static void
setmacs(Ctlr *ctlr)
{
	int i;
	uint *regs;
	ulong top, bot;
	uchar *ra;
	static int ctlrno;

	regs = ctlr->regs;
	ra = ctlr->ra;
	ilock(&ctlr->reglock);
	bot = regs[Specaddr1bot];
	top = regs[Specaddr1top];
	if (bot || top & MASK(16)) {
		ra[0] = bot;
		ra[1] = bot>>8;
		ra[2] = bot>>16;
		ra[3] = bot>>24;
		ra[4] = top;
		ra[5] = top>>8;
	} else {
		iprint("ether0 mac unset; setting to %E\n", ether0mac);
		memmove(ra, ether0mac, sizeof ra);
		ra[5] += ctlrno++;		/* big endian mac */
		regs[Specaddr1top] = ra[5]<<8  | ra[4];
		regs[Specaddr1bot] = ra[3]<<24 | ra[2]<<16 | ra[1]<<8 | ra[0];
	}
	for (i = Laddr2l; i <= Match4off; i++)
		regs[i] = 0;
	iunlock(&ctlr->reglock);
	if (ctlr->edev)
		multicast(ctlr->edev, ra, 1);
}

/* may be called from discover with ctlr only partially populated */
static int
reset(Ctlr *ctlr)
{
	uint *regs;

	if(detach(ctlr)){
		print("%æ: reset timeout\n", ctlr);
		return -1;
	}

	regs = ctlr->regs;
	if (regs == nil) {
		print("%æ: nil regs\n", ctlr);
		return -1;
	}
	/* if unknown, load mac address from non-volatile memory, if present */
	setmacs(ctlr);
	readstats(ctlr);		/* zero stats by reading regs */
	memset(ctlr->stats, 0, sizeof ctlr->stats);
	return 0;
}

/*
 * Get the transmitter into a usable state.  Much of this is boilerplate
 * that could be (or is) done automatically as part of reset (hint, hint).
 */
static void
txinit(Ctlr *ctlr)
{
	uint *regs;
	uvlong physbda;

	regs = ctlr->regs;
	ilock(&ctlr->reglock);
	regs[Ctl] |= Txhalt;
	coherence();
	regs[Ctl] &= ~Txallow;
	iunlock(&ctlr->reglock);
	delay(10);

	/* set up tx queue 0 ring */
	assert(ctlr->tdba != nil);
	inittxring(ctlr, ctlr->tdba, Ntd);
	ctlr->tdh = Ntd - 1;
	/* don't need to use uncached memory nor addresses on icicle */
	physbda = (uvlong)PADDR(ctlr->tdba);
	ilock(&ctlr->reglock);
	regs[Txdescq] = physbda;
	regs[Txdescqhi] = physbda >> 32;
	coherence();

	regs[Ctl] |= Txallow; /* don't set Txstart until there's a pkt to send */
	iunlock(&ctlr->reglock);
}

static void
allocall(Ctlr *ctlr)
{
	int i;
	Block *bp;

	/* discard any buffer Blocks left over from before detach */
	freeblks(ctlr->tb, Ntd);
	freeblks(ctlr->rb, Nrd);

	/*
	 * the GEM manual suggests allocating buffer descriptors in uncached
	 * memory, but microchip assures us that we don't need to use uncached
	 * memory nor addresses on the icicle.
	 */
	if (ctlr->rdba == nil)
		ctlr->rdba = mallocalign(Nrd * sizeof(Rd), Descalign, 0, 0);
	if (ctlr->tdba == nil)
		ctlr->tdba = mallocalign(Ntd * sizeof(Td), Descalign, 0, 0);
	if (ctlr->rdba == nil || ctlr->tdba == nil)
		error(Enomem);

	ctlr->rb = malloc(Nrd * sizeof(Block *));
	ctlr->tb = malloc(Ntd * sizeof(Block *));
	if (ctlr->rb == nil || ctlr->tb == nil)
		error(Enomem);

	/* add enough rcv bufs for one controller to the pool */
	for(i = 0; i < Nrb; i++){
		bp = allocb(Rbsz);
		if(bp == nil)
			error(Enomem);
		bp->free = rbfree;
		freeb(bp);
	}
	aadd(&nrbfull, Nrb);		/* compensate for adecs in rbfree */
}

static void
etherkproc(Ether *edev, void (*kp)(void *), char *sfx)
{
	char buf[KNAMELEN];

	snprint(buf, sizeof buf, "#l%d%s", edev->ctlrno, sfx);
	kproc(buf, kp, edev);
}

static void
startkprocs(Ctlr *ctlr)
{
	Ether *edev;

	if (ctlr->procsrunning)
		return;
	edev = ctlr->edev;
	etherkproc(edev, lproc, "link");
	ienable(ctlr, Linkchange);
	etherkproc(edev, tproc, "xmit");
	etherkproc(edev, rproc, "rcv");
	ctlr->procsrunning = 1;
}

static void
wrvfy(uint *reg, uint val)
{
	uint rdval;

	*reg = val;
	coherence();
	rdval = *reg;
	if (rdval != val)
		print("wrote %#ux to %#p, but read back %#ux\n",
			val, reg, rdval);
}

/* could be attaching again after a detach */
static void
attach(Ether *edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if (ctlr->attached) {
		qunlock(&ctlr->alock);
		return;
	}
	if(waserror()){
		reset(ctlr);
		freemem(ctlr);
		qunlock(&ctlr->alock);
		nexterror();
	}
	if(ctlr->rdba == nil)
		allocall(ctlr);
	/* don't change nrbfull; it's shared by all controllers */
	initmark(&ctlr->wmrb, Nrb, "rcv Blocks not yet processed");
	initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
	initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");
	rxinit(ctlr);
	txinit(ctlr);
	startkprocs(ctlr);
	ctlr->attached = 1;
	qunlock(&ctlr->alock);
	poperror();
}

static Intrsvcret
interrupt(Ureg*, void *v)
{
	int icr, icrreg, im, waker, waket, wakel;
	uint *regs;
	Ctlr *ctlr;

	coherence();
	ctlr = ((Ether *)v)->ctlr;
	regs = ctlr->regs;
	ilock(&ctlr->reglock);
	icrreg = regs[Intsts];
	regs[Intdis] = ~0;		/* disable all my intrs */
	coherence();
	im = ctlr->im;
	icr = icrreg & im;
	if (0 && icr == 0) {		/* mainly for debugging */
		regs[Intena] = im;
		iunlock(&ctlr->reglock);
		if ((icrreg & ~Linkchange) != 0)
			iprint("%æ: uninteresting intr, icr %#ux vs im %#ux\n",
				ctlr, icrreg, im);
		return Intrnotforme;
	}

	waker = waket = wakel = 0;
	if(icr & Rxrcvdint){
		ctlr->rim = Rxrcvdint;
		regs[Intsts] |= Rxrcvdint;	/* reset */
		waker = 1;
		im &= ~Rxrcvdint;
		ctlr->rintr++;
	}
	if(icr & Txdoneint){
		ctlr->tim = Txdoneint;
		regs[Intsts] |= Txdoneint;	/* reset */
		ctlr->txtick = 0;
		waket = 1;
		im &= ~Txdoneint;
		ctlr->tintr++;
	}
	if(icr & Linkchange){
		ctlr->lim = Linkchange;
		regs[Intsts] |= Linkchange;	/* reset */
		wakel = 1;
		/* im &= ~Linkchange;	/* we always want to see link changes */
	}

	/* enable only intr sources we didn't service and are interested in */
	regs[Intena] = ctlr->im = im;
	iunlock(&ctlr->reglock);

	/* now that registers have been updated, wake sleepers */
	if(waker)
		wakeup(&ctlr->rrendez);
	if(waket)
		wakeup(&ctlr->trendez);
	if(wakel)
		wakeup(&ctlr->lrendez);
	return Intrforme;
}

/*
 * map device p and populate a new Ctlr for it.
 * add the new Ctlr to our list.
 */
static Ctlr *
newctlr(void *regs)
{
	uintptr io;
	void *mem;
	Ctlr *ctlr;

	if (regs == nil)
		return nil;
	io = (uintptr)regs;
	mem = vmap(io, 2*PGSZ);
	if(mem == nil){
		print("#l%d: ethergem: can't map regs %#p\n", nctlr, regs);
		return nil;
	}

	ctlr = malloc(sizeof *ctlr);
	if(ctlr == nil) {
		vunmap(mem, 2*PGSZ);
		error(Enomem);
	}
	ctlr->regs = (uint*)mem;
	ctlr->physreg = (uint*)io;
	ctlr->prtype = "gem";
	if(reset(ctlr)){
		print("%æ: can't reset\n", ctlr);
		free(ctlr);
		vunmap(mem, 2*PGSZ);
		return nil;
	}
	ctlrtab[nctlr++] = ctlr;
	return ctlr;
}

enum {
	SYSREG		= 0x20002000,
	SUBBLK_CLK	= (SYSREG+0x84),
	SOFT_RESET	= (SYSREG+0x88),

	/* clock & reset bits */
	Ether0		= 1<<1,		/* mac 0; we call it ether1 */
	Ether1		= 1<<2,		/* mac 1; we call it ether0 */
};

#include <ip.h>

static void
discover(void)
{
	fmtinstall(L'æ', etherfmt);

	fmtinstall('i', eipfmt);		/* paranoia */
	fmtinstall('I', eipfmt);
	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);
#ifdef MAYBE_NEEDED
	/*
	 * magic goo from microchip for icicle.
	 * zero bits 1 & 2 of soft_reset @ 0x20002088 to take both out of reset.
	 */
	*(ulong *)SUBBLK_CLK |= Ether0 | Ether1; /* enable ether clocks */
	coherence();
	delay(200);
	*(ulong *)SOFT_RESET &= ~(Ether0); /* ether1 out of reset */
	coherence();
	delay(200);
#endif
	newctlr(soc.ether[0]);		/* actually mac 1 */
//	newctlr(soc.ether[1]);
}

static int
addgem(Ether *edev)
{
	int i;
	Ctlr *ctlr;
	static int ctlrno;

	if (edev == nil)
		panic("ethergem pnp: nil edev arg");
	if(nctlr == 0)
		discover();
	ctlr = nil;
	for(i = 0; i < nctlr; i++){
		ctlr = ctlrtab[i];
		if(ctlr == nil || ctlr->flag & Factive)
			continue;
		if(edev->port == 0 || edev->port == (uintptr)ctlr->physreg)
			break;
	}
	if (i >= nctlr || ctlr == nil)
		return -1;
	ctlr->flag |= Factive;

	ctlr->edev = edev;		/* point back to Ether */
	edev->ctlr = ctlr;
	edev->port = (uintptr)ctlr->physreg;	/* for printing */
	edev->irq = ioconf("ether", ctlrno++)->irq;
//	edev->pcidev = ctlr->pcidev;
//	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->maxmtu = ETHERMAXTU;
	memmove(edev->ea, ctlr->ra, Eaddrlen);

	edev->arg = edev;
	edev->attach = attach;
	edev->detach = shutdown;
	edev->transmit = transmit;
	edev->interrupt = interrupt;
	edev->ifstat = ifstat;
	edev->shutdown = shutdown;
	edev->ctl = nil;
	edev->multicast = multicast;
	edev->promiscuous = promiscuous;
	return 0;
}

void
ethergemlink(void)
{
	addethercard("gem", addgem);
}
