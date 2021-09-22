/*
 * Synopsis Gb Ethernet Mac (GMAC)
 *
 * Only able to use the bottom 4GB.
 *
 * Its DMA is not cache coherent on Beagle V (StarFive JH7100);
 * supposedly fixed in JH7110.
 *
 * It appears that newer versions of the GMAC (above 3.5 or so) require the
 * alternate descriptor format.  BeagleV instance is version 3.7.
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

#define ALTDESC

typedef struct Ctlr Ctlr;
typedef struct Rd Rd;

enum {
	Crclen = 4,
	/*
	 * the STi7105 manual gives the maximum frame size as 1518 bytes
	 * for non-VLAN frames, and requires the descriptor's frame size
	 * to be a multiple of 4 or 8 or 16 to match the bus width.
	 */
	Rbsz	= ROUNDUP(ETHERMAXTU+Crclen, 8),
	Descalign= CACHELINESZ,

	/* tunable parameters */
	/*
	 * these were Nrd=256, Nrb=1024 & Ntd=128, but Nrd=30, Nrb=47 & Ntd=1
	 * are usually ample; however systems can need more receive buffers
	 * due to bursts of (non-9P) traffic.
	 */
	Ntd	= 64,
	Nrd	= 64,
	Nrb	= 8*Nrd,	/* private receive buffers, >> Nrd */
};

enum {				/* interesting registers, by ulong* index */
	Cfg	= 0,
	Frame	= 4/4,		/* rcv filters */
	Hashtop	= 8/4,		/* hash is upper 6 bits of crc register(?) */
	Hashbot	= 0xc/4,
	Miiaddr	= 0x10/4,
	Miidata	= 0x14/4,
	Version	= 0x20/4,
	Intsts	= 0x38/4,
	Intmask	= 0x3c/4,	/* uninteresting disable bits */
	Addrhi0	= 0x40/4,
	Addrlo0	= 0x44/4,
	/* there are 15 more pairs of addresses */

	/* stats registers at 0x100 */

	/* dma registers */
	Busmode	= 0x1000/4,
	Xmitpoll= 0x1004/4,
	Rcvpoll	= 0x1008/4,
	Rcvbase	= 0x100c/4,
	Xmitbase= 0x1010/4,
	Dmasts	= 0x1014/4,	/* r/w but writing 1 clears a bit */
	Dmactl	= 0x1018/4,
	Dmainten= 0x101c/4,	/* enable bits */
	/* rest are read-only */
	Curtd	= 0x1048/4,
	Currd	= 0x104c/4,
	Curtbuf	= 0x1050/4,
	Currbuf	= 0x1054/4,
};

enum {				/* Cfg bits */
	Wd	= 1<<23,	/* rcv up to 16k frames */
	Jd	= 1<<22,	/* transmit " */
	Je	= 1<<20,	/* jumbo enable (up to 9018 bytes)
	Dcrs	= 1<<16,	/* ignore carrier sense during xmit */
	Dm	= 1<<11,	/* full duplex */
	Ipc	= 1<<10,	/* checksum offload */
	Acs	= 1<<7,		/* strip crc of incoming packets ≤ 1500 bytes */
	Te	= 1<<3,		/* xmit on */
	Re	= 1<<2,		/* rcvr on */
};

enum {				/* Frame bits */
	Ra	= 1ul<<31,	/* disable address filters */
	Hpf	= 1<<10,	/* hash or perfect filter */
	Saf	= 1<<9,		/* src addr filter on */
	Saif	= 1<<8,		/* " " inverse filter on */
	Dbf	= 1<<5,		/* disable bcasts */
	Pm	= 1<<4,		/* accept all multicast */
	Daif	= 1<<3,		/* dest addr inverse filter on */
	Hmc	= 1<<2,		/* filter multicast by hash */
	Huc	= 1<<1,		/* " unicast " " */
	Pr	= 1<<0,		/* promiscuous */
};

enum {				/* Busmode bits */
	Dsl	= MASK(5)<<2,	/* desc gap in words; zero is contiguous */
	Dslshft	= 2,
	Swr	= 1<<0,		/* software reset */
};

enum {				/* Dmactl bits */
	Dff	= 1<<24,	/* don't drop incoming frames on error */
	Dmatxen	= 1<<13,
	Fuf	= 1<<6,		/* accept undersized frames */
	Dmarxen	= 1<<1,
};

enum {				/* Dmainten bits */
	Nie	= 1<<16,	/* normal intr summary enable */
	Aie	= 1<<15,	/* abnormal intr summary enable */
	Rie	= 1<<6,		/* rcv intr */
	Tie	= 1<<0,		/* xmit intr */
};

typedef struct {
	uint	reg;
	char	*name;
} Stat;

static Stat stattab[] = {
	0,	nil,
};

/* this is a register layout, which can't tolerate bogus padding */
#define DESCPAD	(CACHELINESZ/4 - 4)
struct Rd {			/* Receive Descriptor */
	ulong	status;
	ulong	ctlcnts;
	ulong	zero;		/* unused 1st buffer */
	ulong	addr;		/* buffer 2 */
	ulong	pad[DESCPAD];	/* avoid sharing cache lines with other descs */
};

enum {				/* Rd->status bits */
	Own	= 1ul<<31,	/* owned by hw */
#ifdef ALTDESC
	Altintcompl = 1<<30,
	Altls	= 1<<29,
	Altfs	= 1<<28,
	Altdc	= 1<<27,	/* don't add crc */
	Altdp	= 1<<26,	/* disable padding of short packets */
	Altteor	= 1<<21,	/* end of tx ring */
#endif
	Flenshft = 16,
	Flen	= MASK(14)<<Flenshft,
	Errsum	= 1<<15,
};

enum {				/* Rd->ctlcnts bits */
#ifdef ALTDESC
	Altreor	= 1<<15,	/* end of rx ring */
	Buflenshft = 16,	/* for buffer 2 */
	Buflen	= MASK(13)<<Buflenshft,
#else
	Intcompl= 1ul<<31,	/* intr on completion */
	Ls	= 1<<30,	/* last segment */
	Fseg	= 1<<29,	/* first segment */
	Eor	= 1<<25,	/* end of ring */
	Buflenshft = 11,
	Buflen	= MASK(11)<<Buflenshft,
#endif
};

#define Td Rd			/* Transmit Descriptor */

/* Td->addr is a byte address; there are no other bits in it */

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
	p = seprint(p, e, "cfg %#ux\n", ctlr->regs[Cfg]);
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
	ctlr->im |= ie | Nie | Aie;
	ctlr->regs[Dmainten] = ctlr->im;
	iunlock(&ctlr->reglock);
}

static int
lim(void *v)
{
	return ((Ctlr*)v)->lim != 0;
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
	if (bp)
		l2cacheflush(bp->rp, Rbsz);
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
	while (td = &ctlr->tdba[n], n != ctlr->tdt && i++ < Ntd-2) {
		l2cacheflush(td, sizeof *td);
		if (td->status & Own)
			break;
		tdh = n;
		bp = ctlr->tb[tdh];
		ctlr->tb[tdh] = nil;
		if (bp)
			freeb(bp);
		/*
		 * setting Own here would allow this (non-existent) buffer
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
		ienable(ctlr, Tie);
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
			ienable(ctlr, Tie);
			break;
		}
		if((bp = qget(edev->oq)) == nil)
			break;

		assert(ctlr->tdba != nil);
		td = &ctlr->tdba[tdt];
		/* set up this xmit descriptor for this Block's buffer */
		l2cacheflush(bp->rp, BLEN(bp));
		assert((uintptr)PCIWADDR(bp->rp) < 4ull*GB);
		td->addr = PCIWADDR(bp->rp);
#ifdef ALTDESC
		td->ctlcnts = BLEN(bp)<<Buflenshft;
		td->status = (td->status & Altteor) | Altintcompl |
			Altls | Altfs;
#else
		td->ctlcnts = (td->ctlcnts & Eor) | Intcompl | Fseg | Ls |
			BLEN(bp)<<Buflenshft;
#endif
		l2cacheflush(td, sizeof *td);
		td->status |= Own;		/* allows xmit */
		l2cacheflush(td, sizeof *td);

		if (ctlr->tb[tdt])
			panic("%æ: xmit q full", ctlr);
		ctlr->tb[tdt] = bp;

		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (uint)(tdt + Ntd - tdh) % Ntd);
		tdt = NEXT(tdt, Ntd);
	}
	ilock(&ctlr->reglock);
	if(nqd) {
		ctlr->tdt = tdt;
		ctlr->txtick = sys->ticks;
	}
	ctlr->regs[Cfg] |= Te;		/* kick transmitter */
	ctlr->regs[Dmactl] |= Dmatxen;
	ctlr->regs[Xmitpoll] = 0;
	iunlock(&ctlr->reglock);
	ienable(ctlr, Tie);
	qunlock(&ctlr->tlock);
}

static int
tim(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	if(ctlr->tim != 0)
		return 1;
	if(qlen(ctlr->edev->oq) == 0)
		return 0;
	l2cacheflush(&ctlr->tdba[ctlr->tdt], sizeof(Td));
	return (ctlr->tdba[ctlr->tdt].status & Own) == 0;
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
		tsleep(&ctlr->trendez, tim, ctlr, 100);
		ctlr->tim = 0;
		txtick = ctlr->txtick;
		if (txtick && sys->ticks - txtick > 30*HZ) {
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
		rd->status = 0;		/* Own=0: prevent rcv filling for now */
		rd->ctlcnts = Rbsz<<Buflenshft;
	}
#ifdef ALTDESC
	ctlr->rdba[ndescs - 1].ctlcnts |= Altreor;
#else
	ctlr->rdba[ndescs - 1].ctlcnts |= Eor;
#endif
	coherence();
	l2cacheflush(ctlr->rdba, ndescs * sizeof(Rd));
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
		td->status = 0;		/* Own=0: available to fill to send */
		td->ctlcnts = 0;
	}
#ifdef ALTDESC
	ctlr->tdba[ndescs - 1].status |= Altteor;
#else
	ctlr->tdba[ndescs - 1].ctlcnts |= Eor;
#endif
	coherence();
	l2cacheflush(ctlr->tdba, ndescs * sizeof(Td));
}

static void
rxon(Ctlr *ctlr)
{
	ilock(&ctlr->reglock);
	ctlr->regs[Cfg] |= Re;
	ctlr->regs[Dmactl] |= Dmarxen;
	iunlock(&ctlr->reglock);
	ienable(ctlr, Rie);
}

/*
 * Get the receiver into a usable state.  Some of this is boilerplate
 * that could be (or is) done automatically as part of reset (hint, hint),
 * but we also disable broken features (e.g., IP checksums).
 */
static void
rxinit(Ctlr *ctlr)
{
	uint *regs;
	ulong cfg;
	uvlong physbda;

	ctlr->edev->link = 1;
	regs = ctlr->regs;
	ilock(&ctlr->reglock);
	regs[Dmainten] = 0;
	regs[Dmactl] &= ~Dmarxen;
	coherence();
	delay(1);
	regs[Cfg] &= ~(Wd|Jd|Je|Ipc|Re);
	regs[Cfg] &= ~Acs;		/* include CRC in rcv buf & length */
	regs[Dmactl] |= Dff | Fuf;
	regs[Dmasts] = ~0;		/* clear all bits */
	regs[Busmode] &= ~Dsl;		/* contiguous rings */
	regs[Busmode] |= ((DESCPAD/2)<<Dslshft);
	coherence();
	delay(10);
	ctlr->rdfree = 0;
iprint("Rbsz %d\n", Rbsz);
	cfg = regs[Frame];
	cfg &= ~Dbf;
	cfg |= Hmc;
//	cfg |= Pm;		/* emergency use if mcasthash is wrong */
	regs[Frame] = cfg;
	coherence();
	regs[Hashtop] = regs[Hashbot] = 0;
//	regs[Hashtop] = regs[Hashbot] = ~0;	/* accept all; emergency use */
	iunlock(&ctlr->reglock);

	/* set up the rcv ring */
	initrxring(ctlr, ctlr->rdba, Nrd);
	/* don't need to use uncached memory nor addresses on icicle */
	physbda = (uvlong)PADDR(ctlr->rdba);
	assert(physbda < 4ull*GB);
	regs[Rcvbase] = physbda;
	coherence();
	replenish(ctlr);
	rxon(ctlr);
}

static void
replenish(Ctlr *ctlr)
{
	uint rdt, rdh, i;
	Block *bp;
	Block **rb;
	Rd *rd;

	i = 0;
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
		/* set up this rcv descriptor for new Block's buffer */
		assert((uintptr)PCIWADDR(bp->rp) < 4ull*GB);
		rd->addr = PCIWADDR(bp->rp);
#ifdef ALTDESC
		rd->ctlcnts = (rd->ctlcnts & Altreor) | Rbsz<<Buflenshft;
#else
		rd->ctlcnts = (rd->ctlcnts & Eor) | Intcompl | Rbsz<<Buflenshft;
#endif
		l2cacheflush(rd, sizeof *rd);
		rd->status |= Own;	/* hand off to hw */
		l2cacheflush(rd, sizeof *rd);
		coherence();
		ctlr->rdfree++;
		i++;
	}
	ctlr->rdt = rdt;
	if (i)
		ctlr->regs[Rcvpoll] = 0;
}

static int
rim(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	if(ctlr->rim != 0)
		return 1;
	l2cacheflush(&ctlr->rdba[ctlr->rdh], sizeof(Rd));
	return (ctlr->rdba[ctlr->rdh].status & Own) == 0;
}

static void
ckcksum(Ctlr *ctlr, Rd *, Block *bp)
{
	ctlr->ixcs++;
	bp->flag |= Bpktck;
}

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
	l2cacheflush(rd, sizeof *rd);
	if (rd->status & Own)
		return -1;		/* wait for pkts to arrive */

	/*
	 * Accept packets with no errors.
	 */
	passed = 0;
	coherence();
	bp = ctlr->rb[rdh];
	if ((rd->status & (Own|Errsum)) == 0){
		/* no errors and buffer available */
		if (bp == nil)
			panic("%æ: nil Block* from ctlr->rb", ctlr);
		len = ((rd->status & Flen) >> Flenshft) - Crclen;
		if ((int)len < 64)
			len = 64;		/* for 60-byte arps? */
		else if (len > ETHERMAXTU)
			len = ETHERMAXTU;	/* truncate jumbo */
		bp->wp += len;
		ckcksum(ctlr, rd, bp);
		notemark(&ctlr->wmrb, ainc(&nrbfull));
		/* pass pkt upstream, where it will be freed eventually */
		l2cacheflush(bp->rp, len);
		etheriq(ctlr->edev, bp, 1);
		passed++;
	} else {
		ainc(&nrbfull);
		freeb(bp);			/* toss bad pkt */
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
			ienable(ctlr, Rie);
		ctlr->rsleep++;
		tsleep(&ctlr->rrendez, rim, ctlr, 100);

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
		ctlr->regs[Frame] |= Pr;
	else
		ctlr->regs[Frame] &= ~Pr;
	iunlock(&ctlr->reglock);
}

static ulong
mcasthash(uchar *mac)
{
	uint i, hash, rev;

	hash = ethercrc(mac, Eaddrlen);
	/* reverse bits, take 6 top-most. equivalently, reverse bottom 6. */
	rev = 0;
	for (i = 0; i < 6; i++) {
		rev |= (hash & 1) << (5-i);
		hash >>= 1;
	}
	return rev;
}

static void
multicast(void *a, uchar *addr, int on)
{
	ulong hash, word, bit;
	Ctlr *ctlr;

	if (a == nil)
		panic("multicast: nil edev arg");
	ctlr = ((Ether *)a)->ctlr;
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
	word = Hashbot + (hash/BI2WD) % 2;	/* mod 2 enforces bounds */
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
	regs[Intmask] = ~0;		/* not interesting */
	regs[Cfg] &= ~(Te|Re);
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

extern uchar ether0mac[];

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
	bot = regs[Addrlo0];
	top = regs[Addrhi0];
	if (bot || top & MASK(16)) {
		ra[0] = bot;
		ra[1] = bot>>8;
		ra[2] = bot>>16;
		ra[3] = bot>>24;
		ra[4] = top;
		ra[5] = top>>8;
	} else {
		memmove(ra, ether0mac, sizeof ra);
		ra[5] += ctlrno++;		/* big endian mac */
		regs[Addrhi0] = ra[5]<<8  | ra[4];
		regs[Addrlo0] = ra[3]<<24 | ra[2]<<16 | ra[1]<<8 | ra[0];
	}
	for (i = Addrlo0+1; i <= Addrlo0+2*15; i++)
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
	regs[Cfg] &= ~Te;
	regs[Dmactl] &= ~Dmatxen;
	iunlock(&ctlr->reglock);
	delay(10);

	/* set up tx queue ring */
	assert(ctlr->tdba != nil);
	inittxring(ctlr, ctlr->tdba, Ntd);
	ctlr->tdh = Ntd - 1;
	/* don't need to use uncached memory nor addresses on icicle */
	physbda = (uvlong)PADDR(ctlr->tdba);
	assert(physbda < 4ull*GB);
	ilock(&ctlr->reglock);
	regs[Xmitbase] = physbda;
	coherence();

	regs[Cfg] |= Te;
	regs[Dmactl] |= Dmatxen;
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
	 * microchip assures us that we don't need to use uncached
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
	assert((uintptr)ctlr->rb < 4ull*GB);
	assert((uintptr)ctlr->tb < 4ull*GB);

	/* add enough rcv bufs for one controller to the pool */
	for(i = 0; i < Nrb; i++){
		bp = allocb(Rbsz);
		if(bp == nil)
			error(Enomem);
		assert((uintptr)bp < 4ull*GB);
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
	int icr, icrreg, im, waker, waket;
	uint *regs;
	Ctlr *ctlr;

	coherence();
	ctlr = ((Ether *)v)->ctlr;
	regs = ctlr->regs;
	if(regs[Dmainten] == 0)
		iprint("ether interrupt when disabled\n");
	ilock(&ctlr->reglock);
	icrreg = regs[Dmasts];
	regs[Dmainten] = 0;		/* disable all my intrs */
	coherence();
	im = ctlr->im;
	icr = icrreg & im;

	waker = waket = 0;
	if(icr & Rie){
		ctlr->rim = Rie;
		waker = 1;
		im &= ~Rie;
		ctlr->rintr++;
	}
	if(icr & Tie){
		ctlr->tim = Tie;
		ctlr->txtick = 0;
		waket = 1;
		im &= ~Tie;
		ctlr->tintr++;
	}

	/* enable only intr sources we didn't service and are interested in */
if(0)	iprint("etherintr sta %x icrreg %x im %x -> %x, cur rx %ux tx %ux\n",
		regs[Dmasts], icrreg, ctlr->im, im, ctlr->regs[Currd],
		ctlr->regs[Curtd]);
	regs[Dmainten] = ctlr->im = im;
	regs[Dmasts] = icrreg;	/* clear in Dmasts the bits set in icrreg */
	iunlock(&ctlr->reglock);

	/* now that registers have been updated, wake sleepers */
	if(waker)
		wakeup(&ctlr->rrendez);
	if(waket)
		wakeup(&ctlr->trendez);
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
	mem = vmap(io, 64*KB);
	if(mem == nil){
		print("#l%d: ethersyngmac: can't map regs %#p\n", nctlr, regs);
		return nil;
	}

	ctlr = malloc(sizeof *ctlr);
	if(ctlr == nil) {
		vunmap(mem, 64*KB);
		error(Enomem);
	}
	ctlr->regs = (uint*)mem;
	ctlr->physreg = (uint*)io;
	ctlr->prtype = "syngmac";
	if(reset(ctlr)){
		print("%æ: can't reset\n", ctlr);
		free(ctlr);
		vunmap(mem, 64*KB);
		return nil;
	}
	ctlrtab[nctlr++] = ctlr;
	return ctlr;
}

static void
discover(void)
{
	fmtinstall(L'æ', etherfmt);
	newctlr(soc.ether[0]);
}

static int
addsyngmac(Ether *edev)
{
	int i;
	Ctlr *ctlr;
	static int ctlrno;

	if (edev == nil)
		panic("ethersyngmac pnp: nil edev arg");
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
	edev->irq = ioconf("ether", 0)->irq;
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
ethersyngmaclink(void)
{
	addethercard("syngmac", addsyngmac);
}
