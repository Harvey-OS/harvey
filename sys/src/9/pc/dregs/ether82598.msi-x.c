/*
 * intel pci-express 10Gb ethernet driver for 8259[89].
 * can probably drive the x5[245]0 with a little work.
 * copyright © 2007, coraid, inc.
 * depessimised and made to work on the 82599 at bell labs, 2013.
 * TODO: test on hardware again.
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

/*
 * if donebit (Rdd or Tdd) is 0, hardware is operating (or will be) on desc.;
 * else, hardware has finished filling or draining this descriptor's buffer.
 */
#define HWOWNS(desc, donebit) (((desc)->status & (donebit)) == 0)

enum {
	/* +slop is for vlan headers, crcs, etc. */
	Rbsz	= ETHERMAXTU+32,
	Descalign= 128,

	/* tunable parameters */
	Goslow	= 0,		/* flag: go slow by throttling intrs, etc. */
	Giveadamn= 0,		/* TODO flag: worry about page boundaries */
	/*
	 * these were Nrd=256, Nrb=1024 & Ntd=128, but Nrd=30, Nrb=47 & Ntd=1		 * are usually ample; however systems can need more receive buffers
	 * due to bursts of traffic.
	 */
	Ntd	= 32,		/* multiple of 8 */
	Nrd	= 128,		/* multiple of 8 */
	Nrb	= 1024,
};

enum {
	/* general */
	Ctrl		= 0x00000/4,	/* Device Control */
	Status		= 0x00008/4,	/* Device Status */
	Ctrlext		= 0x00018/4,	/* Extended Device Control */
	Esdp		= 0x00020/4,	/* extended sdp control */
	Esodp		= 0x00028/4,	/* extended od sdp control (i2cctl on 599) */
	Ledctl		= 0x00200/4,	/* led control */
	Tcptimer	= 0x0004c/4,	/* tcp timer */
	Ecc		= 0x110b0/4,	/* 598 errata ecc control magic */
					/* (pcie intr cause on 599) */

	/* non-volatile memories */
	Eec		= 0x10010/4,	/* eeprom/flash control */
	Eerd		= 0x10014/4,	/* eeprom read */
	Fla		= 0x1001c/4,	/* flash access */
	Flop		= 0x1013c/4,	/* flash opcode */
	Grc		= 0x10200/4,	/* general rx control */

	/* interrupt */
	Icr		= 0x00800/4,	/* interrupt cause read */
	Ics		= 0x00808/4,	/* " set */
	Ims		= 0x00880/4,	/* " mask read/set (actually enable) */
	Imc		= 0x00888/4,	/* " mask clear */
	Iac		= 0x00810/4,	/* " auto clear */
	Iam		= 0x00890/4,	/* " auto mask enable */
	Itr		= 0x00820/4,	/* " throttling rate regs (0-19) */
	Ivar		= 0x00900/4,	/* " vector allocation regs. */
	/* msi(-x) interrupt */
//	Msixt		= 0x0000/4,	/* msi-x table (bar3) */
//	Msipba		= 0x2000/4,	/* msi-x pending bit array (bar3) */
	Pbacl		= 0x11068/4,	/* pba clear */
	Gpie		= 0x00898/4,	/* general purpose int enable */

	/* flow control */
	Pfctop		= 0x03008/4,	/* priority flow ctl type opcode */
	Fcttv		= 0x03200/4,	/* " transmit timer value (0-3) */
	Fcrtl		= 0x03220/4,	/* " rx threshold low (0-7) +8n */
	Fcrth		= 0x03260/4,	/* " rx threshold high (0-7) +8n */
	Rcrtv		= 0x032a0/4,	/* " refresh value threshold */
	Tfcs		= 0x0ce00/4,	/* " tx status */

	/* receive dma */
	Rbal		= 0x01000/4,	/* rx desc base low (0-63) +0x40n */
	Rbah		= 0x01004/4,	/* " high */
	Rdlen		= 0x01008/4,	/* " length */
	Rdh		= 0x01010/4,	/* " head */
	Rdt		= 0x01018/4,	/* " tail */
	Rxdctl		= 0x01028/4,	/* " control */

	Srrctl		= 0x02100/4,	/* split & replication rx ctl. array */
	Dcarxctl	= 0x02200/4,	/* rx dca control */
	Rdrxctl		= 0x02f00/4,	/* rx dma control */
	Rxpbsize	= 0x03c00/4,	/* rx packet buffer size */
	Rxctl		= 0x03000/4,	/* rx control */
	Dropen		= 0x03d04/4,	/* drop enable control (598 only) */

	/* receive non-dma */
	Rxcsum		= 0x05000/4,	/* rx checksum control */
	Rfctl		= 0x05008/4,	/* rx filter control */
	Mta		= 0x05200/4,	/* multicast table array (0-127) */
	Ral98		= 0x05400/4,	/* rx address low (598) */
	Rah98		= 0x05404/4,
	Ral99		= 0x0a200/4,	/* rx address low array (599) */
	Rah99		= 0x0a204/4,
	Psrtype		= 0x05480/4,	/* packet split rx type. */
	Vfta		= 0x0a000/4,	/* vlan filter table array. */
	Fctrl		= 0x05080/4,	/* filter control */
	Vlnctrl		= 0x05088/4,	/* vlan control */
	Msctctrl	= 0x05090/4,	/* multicast control */
	Mrqc		= 0x05818/4,	/* multiple rx queues cmd */
	Vmdctl		= 0x0581c/4,	/* vmdq control (598 only) */
	Imir		= 0x05a80/4,	/* immediate irq rx (0-7) (598 only) */
	Imirext		= 0x05aa0/4,	/* immediate irq rx ext (598 only) */
	Imirvp		= 0x05ac0/4,	/* immediate irq vlan priority (598 only) */
	Reta		= 0x05c00/4,	/* redirection table */
	Rssrk		= 0x05c80/4,	/* rss random key */

	/* transmit */
	Tdbal		= 0x06000/4,	/* tx desc base low +0x40n array */
	Tdbah		= 0x06004/4,	/* " high */
	Tdlen		= 0x06008/4,	/* " len */
	Tdh		= 0x06010/4,	/* " head */
	Tdt		= 0x06018/4,	/* " tail */
	Txdctl		= 0x06028/4,	/* " control */
	Tdwbal		= 0x06038/4,	/* " write-back address low */
	Tdwbah		= 0x0603c/4,

	Dtxctl98	= 0x07e00/4,	/* tx dma control (598 only) */
	Dtxctl99	= 0x04a80/4,	/* tx dma control (599 only) */
	Tdcatxctrl98	= 0x07200/4,	/* tx dca register (0-15) (598 only) */
	Tdcatxctrl99	= 0x0600c/4,	/* tx dca register (0-127) (599 only) */
	Tipg		= 0x0cb00/4,	/* tx inter-packet gap (598 only) */
	Txpbsize	= 0x0cc00/4,	/* tx packet-buffer size (0-15) */

	/* mac */
	Hlreg0		= 0x04240/4,	/* highlander control reg 0 */
	Hlreg1		= 0x04244/4,	/* highlander control reg 1 (ro) */
	Msca		= 0x0425c/4,	/* mdi signal cmd & addr */
	Msrwd		= 0x04260/4,	/* mdi single rw data */
	Mhadd		= 0x04268/4,	/* mac addr high & max frame */
	Pcss1		= 0x04288/4,	/* xgxs status 1 */
	Pcss2		= 0x0428c/4,
	Xpcss		= 0x04290/4,	/* 10gb-x pcs status */
	Serdesc		= 0x04298/4,	/* serdes control */
	Macs		= 0x0429c/4,	/* fifo control & report */
	Autoc		= 0x042a0/4,	/* autodetect control & status */
	Links		= 0x042a4/4,	/* link status */
	Links2		= 0x04324/4,	/* 599 only */
	Autoc2		= 0x042a8/4,
};

enum {
	Factive		= 1<<0,
	Enable		= 1<<31,

	/* Ctrl */
	Rst		= 1<<26,	/* full nic reset */

	/* Txdctl */
	Ten		= 1<<25,

	/* Dtxctl99 */
	Te		= 1<<0,		/* dma tx enable */

	/* Fctrl */
	Bam		= 1<<10,	/* broadcast accept mode */
	Upe		= 1<<9,		/* unicast promiscuous */
	Mpe		= 1<<8,		/* multicast promiscuous */

	/* Rxdctl */
	Pthresh		= 0,		/* prefresh threshold shift in bits */
	Hthresh		= 8,		/* host buffer minimum threshold " */
	Wthresh		= 16,		/* writeback threshold */
	Renable		= 1<<25,

	/* Rxctl */
	Rxen		= 1<<0,
	Dmbyps		= 1<<1,		/* descr. monitor bypass (598 only) */

	/* Rdrxctl */
	Rdmt½		= 0,		/* 598 */
	Rdmt¼		= 1,		/* 598 */
	Rdmt⅛		= 2,		/* 598 */
	Crcstrip	= 1<<1,		/* 599 */
	Rscfrstsize	= 037<<17,	/* 599; should be zero */

	/* Rxcsum */
	Ippcse		= 1<<12,	/* ip payload checksum enable */

	/* Eerd */
	EEstart		= 1<<0,		/* Start Read */
	EEdone		= 1<<1,		/* Read done */

	/* interrupts */
	Irx0		= 1<<0,		/* driver defined */
	Itx0		= 1<<1,		/* driver defined */
	Lsc		= 1<<20,	/* link status change */

	/* Links */
	Lnkup		= 1<<30,
	Lnkspd		= 1<<29,

	/* Hlreg0 */
	Txcrcen		= 1<<0,		/* add crc during xmit */
	Rxcrcstrip	= 1<<1,		/* strip crc during recv */
	Jumboen		= 1<<2,
	Txpaden		= 1<<10,	/* pad short frames during xmit */

	/* Autoc */
	Flu		= 1<<0,		/* force link up */
	Lmsshift	= 13,		/* link mode select shift */
	Lmsmask		= 7,
};

typedef struct Ctlr Ctlr;
typedef struct Rd Rd;
typedef struct Td Td;

typedef struct {
	uint	reg;
	char	*name;
} Stat;

Stat stattab[] = {
	0x4000,	"crc error",
	0x4004,	"illegal byte",
	0x4008,	"short packet",
	0x3fa0,	"missed pkt0",
	0x4034,	"mac local flt",
	0x4038,	"mac rmt flt",
	0x4040,	"rx length err",
	0x3f60,	"xon tx",
	0xcf60,	"xon rx",
	0x3f68,	"xoff tx",
	0xcf68,	"xoff rx",
	0x405c,	"rx 040",
	0x4060,	"rx 07f",
	0x4064,	"rx 100",
	0x4068,	"rx 200",
	0x406c,	"rx 3ff",
	0x4070,	"rx big",
	0x4074,	"rx ok",
	0x4078,	"rx bcast",
	0x3fc0,	"rx no buf0",
	0x40a4,	"rx runt",
	0x40a8,	"rx frag",
	0x40ac,	"rx ovrsz",
	0x40b0,	"rx jab",
	0x40d0,	"rx pkt",

	0x40d4,	"tx pkt",
	0x40d8,	"tx 040",
	0x40dc,	"tx 07f",
	0x40e0,	"tx 100",
	0x40e4,	"tx 200",
	0x40e8,	"tx 3ff",
	0x40ec,	"tx big",
	0x40f4,	"tx bcast",
	0x4120,	"xsum err",
};

/* status */
enum {
	Pif	= 1<<7,	/* past exact filter (sic) */
	Ipcs	= 1<<6,	/* ip checksum calculated */
	L4cs	= 1<<5,	/* layer 2 */
	Tcpcs	= 1<<4,	/* tcp checksum calculated */
	Vp	= 1<<3,	/* 802.1q packet matched vet */
	Ixsm	= 1<<2,	/* ignore checksum */
	Reop	= 1<<1,	/* end of packet */
	Rdd	= 1<<0,	/* descriptor done */
};

struct Rd {			/* Receive Descriptor */
	uint	addr[2];
	ushort	length;
	ushort	checksum;	/* valid iff (status&Ixsm)==0 */
	uchar	status;
	uchar	errors;
	ushort	vlan;
};

enum {
	/* Td cmd */
	Rs	= 1<<3,		/* report status */
	Ic	= 1<<2,		/* insert checksum */
	Ifcs	= 1<<1,		/* insert FCS (ethernet crc) */
	Teop	= 1<<0,		/* end of packet */

	/* Td status */
	Tdd	= 1<<0,		/* descriptor done */
};

struct Td {			/* Transmit Descriptor */
	uint	addr[2];
	ushort	length;
	uchar	cso;
	uchar	cmd;
	uchar	status;
	uchar	css;
	ushort	vlan;
};

struct Ctlr {
	uint	*regs;			/* memory-mapped device registers */
	Ether	*edev;
	Pcidev	*pcidev;
	int	type;

	Lock	imlock;
	uint	im;			/* interrupt mask */
	uint	lim;
	uint	rim;
	uint	tim;

	Rendez	rrendez;
	Rd*	rdba;			/* receive descriptor base address */
	Block**	rb;			/* " buffers */
	uint	rdh;			/* " descriptor head */
	uint	rdt;			/* " descriptor tail */
	int	rdfree;			/* " descriptors awaiting packets */

	Rendez	trendez;
	QLock	tlock;
	Td*	tdba;			/* transmit descriptor base address */
	uint	tdh;			/* " descriptor head */
	uint	tdt;			/* " descriptor tail */
	Block**	tb;			/* " buffers */

	Watermark wmrb;
	Watermark wmrd;
	Watermark wmtd;

	Rendez	lrendez;

	uchar	flag;
	int	procsrunning;
	QLock	alock;			/* attach lock */
	int	attached;

	uint	*physreg;		/* phys addr of reg */
#ifdef MSIX
	uint	*msix;			/* unused */
	uint	*physmsix;		/* unused */
#endif

	uchar	ra[Eaddrlen];		/* receive address */
	uchar	mta[128];		/* multicast table array */
	uint	speeds[3];

	QLock	slock;
	ulong	stats[nelem(stattab)];
};

enum {
	I82598 = 1,
	I82599,
};

static	Ctlr	*ctlrtab[8];
static	int	nctlr;

static	Lock	rblock;
static	Block	*rbpool;
static	long	nrbfull;  /* # of rcv Blocks with data awaiting processing */

static int
awaitregbitzero(Ctlr *ctlr, int reg, ulong bit)
{
	int timeo;

//	delay(1);		/* important to delay before testing */
	for(timeo = 1000; ctlr->regs[reg] & bit && timeo > 0; timeo--)
		delay(1);
	return ctlr->regs[reg] & bit? -1: 0;
}

static void
readstats(Ctlr *ctlr)
{
	int i;

	qlock(&ctlr->slock);
	for(i = 0; i < nelem(ctlr->stats); i++)
		ctlr->stats[i] += ctlr->regs[stattab[i].reg >> 2];
	qunlock(&ctlr->slock);
}

static int speedtab[] = {
	0,
	1000,
	10000,
};

static long
ifstat(Ether *edev, void *a, long n, ulong offset)
{
	uint i, *t;
	char *s, *p, *e;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	p = s = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	e = p + READSTR;

	readstats(ctlr);
	for(i = 0; i < nelem(stattab); i++)
		if(ctlr->stats[i] > 0)
			p = seprint(p, e, "%.10s  %uld\n", stattab[i].name,
				ctlr->stats[i]);
	t = ctlr->speeds;
	p = seprint(p, e, "speeds: 0:%d 1000:%d 10000:%d\n", t[0], t[1], t[2]);
	p = seprint(p, e, "mtu: min:%d max:%d\n", edev->minmtu, edev->maxmtu);
	p = seprint(p, e, "rdfree %d rdh %d rdt %d\n", ctlr->rdfree, ctlr->rdh,
		ctlr->rdt);
	p = seprintmark(p, e, &ctlr->wmrb);
	p = seprintmark(p, e, &ctlr->wmrd);
	p = seprintmark(p, e, &ctlr->wmtd);
	USED(p);
	n = readstr(offset, a, n, s);
	free(s);

	return n;
}

static void
ienable(Ctlr *ctlr, int i)
{
	ilock(&ctlr->imlock);
	ctlr->im |= i;
	ctlr->regs[Ims] = ctlr->im;
	iunlock(&ctlr->imlock);
}

static int
lim(void *v)
{
	return ((Ctlr*)v)->lim != 0;
}

static void
lproc(void *v)
{
	int r, i;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (;;) {
		r = ctlr->regs[Links];
		edev->link = (r & Lnkup) != 0;
		i = 0;
		if(edev->link)
			i = 1 + ((r & Lnkspd) != 0);
		ctlr->speeds[i]++;
		edev->mbps = speedtab[i];
		ctlr->lim = 0;
		ienable(ctlr, Lsc);
		sleep(&ctlr->lrendez, lim, ctlr);
		ctlr->lim = 0;
	}
}

static long
ctl(Ether *, void *, long)
{
	error(Enonexist);
	return -1;
}

static Block*
rballoc(void)
{
	Block *bp;

	ilock(&rblock);
	if((bp = rbpool) != nil){
		rbpool = bp->next;
		bp->next = nil;
		ainc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&rblock);
	return bp;
}

void
rbfree(Block *bp)
{
	if (Giveadamn) {
		/* put buffer on a page boundary so we don't cross page boundaries */
		bp->wp = bp->rp = (uchar*)PGROUND((uintptr)bp->base);
		checkb(bp, "82598 rbfree incoming rcv buf");
	} else
		bp->wp = bp->rp = (uchar *)
			ROUNDDN((ulong)bp->lim - Rbsz, BLOCKALIGN);
	assert(bp->rp >= bp->base);
	assert(((ulong)bp->rp & (BLOCKALIGN-1)) == 0);
	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(&rblock);
	bp->next = rbpool;
	rbpool = bp;
	iunlock(&rblock);
	adec(&nrbfull);
}

static void
cleanup(Ctlr *ctlr)
{
	uint n, tdh;
	Block *bp;
	Td *td;

	tdh = ctlr->tdh;
	n = NEXT(tdh, Ntd);
	/* Tdd indicates a reusable td; sw owns it */
	for (td = &ctlr->tdba[n]; !HWOWNS(td, Tdd); td = &ctlr->tdba[n]){
		tdh = n;

		bp = ctlr->tb[tdh];
		ctlr->tb[tdh] = nil;
		freeb(bp);
		td->status = 0;		/* clear Tdd, among others */

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
	uint i, tdt, tdh;

	ctlr = edev->ctlr;
	if(!canqlock(&ctlr->tlock)){
		ienable(ctlr, Itx0);
		return;
	}
	if (!ctlr->attached) {
		qunlock(&ctlr->tlock);
		return;
	}
	cleanup(ctlr);				/* free transmitted buffers */
	tdh = ctlr->tdh;
	tdt = ctlr->tdt;
	for(i = 0; ; i++){
		if(NEXT(tdt, Ntd) == tdh){	/* ring full? */
			ienable(ctlr, Itx0);
			break;
		}
		if((bp = qget(edev->oq)) == nil)
			break;

		assert(ctlr->tdba != nil);
		td = &ctlr->tdba[tdt];
		/* td->status, thus Tdd, will have been zeroed by cleanup */
		td->addr[0] = PCIWADDR(bp->rp);
		td->length = BLEN(bp);
		td->cmd = Ifcs | Teop | (Goslow? 0: Rs);
		ctlr->tb[tdt] = bp;

		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (uint)(tdt + Ntd - tdh) % Ntd);
		tdt = NEXT(tdt, Ntd);
	}
	if(i) {
		coherence();
		ctlr->regs[Tdt] = ctlr->tdt = tdt;  /* make new Tds active */
		coherence();
		ienable(ctlr, Itx0);
	}
	qunlock(&ctlr->tlock);
}

static int
tim(void *vc)
{
	Ctlr *ctlr = (void *)vc;
	Ether *edev = ctlr->edev;

	return ctlr->tim != 0 ||
		qlen(edev->oq) > 0 && !HWOWNS(&ctlr->tdba[ctlr->tdh], Tdd);
}

static void
tproc(void *v)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (;;) {
		sleep(&ctlr->trendez, tim, ctlr); /* xmit interrupt kicks us */
		ctlr->tim = 0;
		/*
		 * perhaps some buffers have been transmitted and their
		 * descriptors can be reused to copy Blocks out of edev->oq.
		 */
		transmit(edev);
	}
}

static void
rxinit(Ctlr *ctlr)
{
	int i, is598, autoc;
	uint *regs;
	ulong until;
	Block *bp;

	regs = ctlr->regs;
	regs[Rxctl] &= ~Rxen;
	regs[Rxdctl] = 0;
	for(i = 0; i < Nrd; i++){
		bp = ctlr->rb[i];
		ctlr->rb[i] = nil;
		freeb(bp);
	}
	ctlr->rdfree = 0;

	coherence();
	regs[Fctrl] |= Bam;
	regs[Fctrl] &= ~(Upe | Mpe);

	/* intel gets some csums wrong (e.g., errata 44) */
	regs[Rxcsum] &= ~Ippcse;
	regs[Hlreg0] &= ~Jumboen;		/* jumbos are a bad idea */
	regs[Hlreg0] |= Txcrcen | Rxcrcstrip | Txpaden;
	regs[Srrctl] = HOWMANY(Rbsz, 1024);
	regs[Mhadd] = Rbsz << 16;

	regs[Rbal] = PCIWADDR(ctlr->rdba);
	regs[Rbah] = 0;
	regs[Rdlen] = Nrd*sizeof(Rd);	/* must be multiple of 128 */
	regs[Rdh] = 0;
	regs[Rdt] = ctlr->rdt = 0;
	coherence();

	is598 = (ctlr->type == I82598);
	if (is598)
		regs[Rdrxctl] = Rdmt¼;
	else {
		regs[Rdrxctl] |= Crcstrip;
		regs[Rdrxctl] &= ~Rscfrstsize;
	}
	if (Goslow && is598)
		regs[Rxdctl] = 8<<Wthresh | 8<<Pthresh | 4<<Hthresh |
			Renable;
	else
		regs[Rxdctl] = Renable;
	coherence();

	/*
	 * don't wait forever like an idiot (and hang the system),
	 * maybe it's disconnected.
	 */
	until = TK2MS(MACHP(0)->ticks) + 250;
	while (!(regs[Rxdctl] & Renable) && TK2MS(MACHP(0)->ticks) < until)
		;
	if(!(regs[Rxdctl] & Renable))
		print("#l%d: Renable didn't come on, might be disconnected\n",
			ctlr->edev->ctlrno);

	regs[Rxctl] |= Rxen | (is598? Dmbyps: 0);

	if (is598){
		autoc = regs[Autoc];
		/* what is this rubbish and why do we care? */
		print("#l%d: autoc %#ux; lms %d (3 is 10g sfp)\n",
			ctlr->edev->ctlrno, autoc, (autoc>>Lmsshift) & Lmsmask);
		regs[Autoc] |= Flu;
		coherence();
		delay(50);
	}
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
			print("#l%d: 82598: rx overrun\n", ctlr->edev->ctlrno);
			break;
		}
		*rb = bp = rballoc();
		if(bp == nil){
			print("#l%d: 82598: no rx buffers\n", ctlr->edev->ctlrno);
			break;
		}
		rd->addr[0] = PCIWADDR(bp->rp);
		rd->status = 0;
		ctlr->rdfree++;
		i++;
	}
	if(i) {
		coherence();
		ctlr->regs[Rdt] = ctlr->rdt = rdt; /* hand back recycled rdescs */
		coherence();
	}
}

static int
rim(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	return ctlr->rim != 0 || !HWOWNS(&ctlr->rdba[ctlr->rdh], Rdd);
}

static void
rproc(void *v)
{
	int passed;
	uint rdh;
	Block *bp;
	Ctlr *ctlr;
	Ether *edev;
	Rd *rd;

	edev = v;
	ctlr = edev->ctlr;
	for (ctlr->rdh = 0; ; ) {
		replenish(ctlr);
		ienable(ctlr, Irx0);
		sleep(&ctlr->rrendez, rim, ctlr);

		passed = 0;
		for (;;) {
			ctlr->rim = 0;
			rdh = ctlr->rdh;
			rd = &ctlr->rdba[rdh];
			/* Rdd indicates a reusable rd; sw owns it */
			if (HWOWNS(rd, Rdd))
				break;		/* wait for pkts to arrive */

			bp = ctlr->rb[rdh];
			ctlr->rb[rdh] = nil;
			if (rd->length > ETHERMAXTU)
				print("#l%d: 82598 got jumbo of %d bytes\n",
					edev->ctlrno, rd->length);
			if (bp == nil)
				panic("82598: nil buffer from ctlr->rb");
			bp->wp += rd->length;
//			rd->status = 0;

			etheriq(edev, bp, 1);
			ainc(&nrbfull);
			notemark(&ctlr->wmrb, nrbfull);

			passed++;
			ctlr->rdfree--;
			ctlr->rdh = NEXT(rdh, Nrd);
			/*
			 * if number of rds ready for packets is too low (more
			 * than 16 in use), set up the unready ones.
			 */
			if (ctlr->rdfree <= Nrd - 16)
				replenish(ctlr);
		}
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
	if(on)
		ctlr->regs[Fctrl] |= Upe | Mpe;
	else
		ctlr->regs[Fctrl] &= ~(Upe | Mpe);
}

static void
multicast(void *a, uchar *ea, int on)
{
	int bit, wd;
	Ctlr *ctlr;
	Ether *edev;

	edev = a;
	ctlr = edev->ctlr;

	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	wd = ea[5] >> 1;
	bit = (ea[5]&1)<<4 | ea[4]>>4;
	if(on)
		ctlr->mta[wd] |= 1 << bit;
//	else
//		ctlr->mta[wd] &= ~(1 << bit);
	ctlr->regs[Mta+wd] = ctlr->mta[wd];
}

static void
freemem(Ctlr *ctlr)
{
	Block *bp;

	while(bp = rballoc()){
		bp->free = nil;
		freeb(bp);
	}
	free(ctlr->rdba);
	ctlr->rdba = nil;
	free(ctlr->tdba);
	ctlr->tdba = nil;
	free(ctlr->rb);
	ctlr->rb = nil;
	free(ctlr->tb);
	ctlr->tb = nil;
}

static int
detach(Ctlr *ctlr)
{
	int i, is598;
	uint *regs;

	regs = ctlr->regs;
	regs[Imc] = ~0;
	regs[Ctrl] |= Rst;
	if (awaitregbitzero(ctlr, Ctrl, Rst) < 0) {
		print("#l%d: 82598: Ctlr(%#ux).Rst(%#ux) never went to 0\n",
			ctlr->edev->ctlrno, regs[Ctrl], Rst);
		return -1;
	}
	is598 = (ctlr->type == I82598);
	if (is598) {				/* errata magic */
		delay(50);
		regs[Ecc] &= ~(1<<21 | 1<<18 | 1<<9 | 1<<6);
	}

	/* not cleared by reset; disable it manually. */
	for(i = 1; i < 16; i++)
		regs[is598? Rah98: Rah99] &= ~Enable;
	for(i = 0; i < 128; i++)
		regs[Mta + i] = 0;
	for(i = 1; i < (is598? 640: 128); i++)
		regs[Vfta + i] = 0;

	ctlr->attached = 0;
	return 0;
}

static void
shutdown(Ether *edev)
{
	detach(edev->ctlr);
//	freemem(edev->ctlr);
}

/* ≤ 20ms */
static ushort
eeread(Ctlr *ctlr, int i)
{
	ctlr->regs[Eerd] = EEstart | i<<2;
	while((ctlr->regs[Eerd] & EEdone) == 0)
		;
	return ctlr->regs[Eerd] >> 16;
}

static int
eeload(Ctlr *ctlr)
{
	ushort u, v, p, l, i, j;

	if((eeread(ctlr, 0) & 0xc0) != 0x40)
		return -1;
	u = 0;
	for(i = 0; i < 0x40; i++)
		u +=  eeread(ctlr, i);
	for(i = 3; i < 0xf; i++){
		p = eeread(ctlr, i);
		l = eeread(ctlr, p++);
		if((int)p + l + 1 > 0xffff)
			continue;
		for(j = p; j < p + l; j++)
			u += eeread(ctlr, j);
	}
	if(u != 0xbaba)
		return -1;
	if(ctlr->regs[Status] & (1<<3))
		u = eeread(ctlr, 10);
	else
		u = eeread(ctlr, 9);
	u++;
	for(i = 0; i < Eaddrlen;){
		v = eeread(ctlr, u + i/2);
		ctlr->ra[i++] = v;
		ctlr->ra[i++] = v>>8;
	}
	ctlr->ra[5] += (ctlr->regs[Status] & 0xc) >> 2;
	return 0;
}

static int
reset(Ctlr *ctlr)
{
	int i, is598;
	uint *regs;
	uchar *p;

	if(detach(ctlr)){
		print("82598: reset timeout\n");
		return -1;
	}
	if(eeload(ctlr)){
		print("82598: eeprom failure\n");
		return -1;
	}
	p = ctlr->ra;
	regs = ctlr->regs;
	is598 = (ctlr->type == I82598);
	regs[is598? Ral98: Ral99] = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
	regs[is598? Rah98: Rah99] = p[5]<<8 | p[4] | Enable;

	readstats(ctlr);
	for(i = 0; i<nelem(ctlr->stats); i++)
		ctlr->stats[i] = 0;

	regs[Ctrlext] |= 1 << 16;	/* required by errata (spec change 4) */
	regs[Fcrtl] = regs[Fcrth] = regs[Rcrtv] = 0;
	if (Goslow) {
		/* make some guesses for flow control */
		regs[Fcrtl] = 0x10000 | Enable;
		regs[Fcrth] = 0x40000 | Enable;
		regs[Rcrtv] = 0x6000;
	}

	/* configure interrupt mapping (don't ask) */
	regs[Ivar+0] =     0 | 1<<7;
	regs[Ivar+64/4] =  1 | 1<<7;
//	regs[Ivar+97/4] = (2 | 1<<7) << (BI2BY*(97%4));

#define setcpumap(regs, byte, val) \
	regs[Ivar+(byte)/4] = ((val) | 1<<7) << (BI2BY*((byte)%4));
#ifdef notyet			// TODO: convert Ivar intr mapping to use macro
	setcpumap(regs, 0, 0);
	setcpumap(regs, 64, 1);
//	setcpumap(regs, 97, 2);
#endif

	for(i = Itr; i < Itr + 20; i++)
		regs[i] = 0;			/* ¼µs intervals */
	regs[Itr + Itx0] = 0;
	if (Goslow) {				/* interrupt throttling? */
		for(i = Itr; i < Itr + 20; i++)
			regs[i] = 128;		/* ¼µs intervals */
		regs[Itr + Itx0] = 256;
	}
	return 0;
}

static void
txinit(Ctlr *ctlr)
{
	Block *bp;
	int i;
	uint *regs;

	regs = ctlr->regs;
	if (Goslow)
		regs[Txdctl] = 16<<Wthresh | 16<<Pthresh;
	else
		regs[Txdctl] = 0;
	if (ctlr->type == I82599)
		regs[Dtxctl99] = 0;
	coherence();
	for(i = 0; i < Ntd; i++){
		bp = ctlr->tb[i];
		ctlr->tb[i] = nil;
		freeb(bp);
	}

	assert(ctlr->tdba != nil);
	memset(ctlr->tdba, 0, Ntd * sizeof(Td));
	regs[Tdbal] = PCIWADDR(ctlr->tdba);
	regs[Tdbah] = 0;
	regs[Tdlen] = Ntd*sizeof(Td);	/* must be multiple of 128 */
	regs[Tdh] = 0;
	ctlr->tdh = Ntd - 1;
	regs[Tdt] = ctlr->tdt = 0;
	coherence();
	if (ctlr->type == I82599)
		regs[Dtxctl99] |= Te;
	coherence();
	regs[Txdctl] |= Ten;
	coherence();
	while (!(regs[Txdctl] & Ten))
		;
}

static void
attach(Ether *edev)
{
	int i;
	Block *bp;
	Ctlr *ctlr;
	char buf[KNAMELEN];

	ctlr = edev->ctlr;
	ctlr->edev = edev;			/* point back to Ether* */
	qlock(&ctlr->alock);
	if(waserror()){
		reset(ctlr);
		freemem(ctlr);
		qunlock(&ctlr->alock);
		nexterror();
	}
	if(ctlr->rdba == nil) {
		ctlr->rdba = mallocalign(Nrd * sizeof *ctlr->rdba,
			Descalign, 0, 0);
		ctlr->tdba = mallocalign(Ntd * sizeof *ctlr->tdba,
			Descalign, 0, 0);
		ctlr->rb = malloc(Nrd * sizeof(Block *));
		ctlr->tb = malloc(Ntd * sizeof(Block *));
		if (ctlr->rdba == nil || ctlr->tdba == nil ||
		    ctlr->rb == nil || ctlr->tb == nil)
			error(Enomem);

		for(i = 0; i < Nrb; i++){
			if (Giveadamn)
				bp = allocb(Rbsz + BY2PG); /* see rbfree() */
			else
				bp = allocb(Rbsz);
			if(bp == nil)
				error(Enomem);
			bp->free = rbfree;
			freeb(bp);
		}
	}
	if (!ctlr->attached) {
		rxinit(ctlr);
		txinit(ctlr);
		nrbfull = 0;
		if (!ctlr->procsrunning) {
			snprint(buf, sizeof buf, "#l%dlink", edev->ctlrno);
			kproc(buf, lproc, edev);
			snprint(buf, sizeof buf, "#l%dtransmit", edev->ctlrno);
			kproc(buf, tproc, edev);
			snprint(buf, sizeof buf, "#l%dreceive", edev->ctlrno);
			kproc(buf, rproc, edev);
			ctlr->procsrunning = 1;
		}
		initmark(&ctlr->wmrb, Nrb, "rcv Blocks not yet processed");
		initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
		initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");
		ctlr->attached = 1;
	}
	qunlock(&ctlr->alock);
	poperror();
}

static int
interrupt(Ureg*, void *v)
{
	int icr, im, forme;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	ilock(&ctlr->imlock);
	ctlr->regs[Imc] = ~0;			/* disable all intrs */
	im = ctlr->im;
	forme = (ctlr->regs[Icr] & im) != 0;
	// TODO: any good reason to loop here?
	while((icr = ctlr->regs[Icr] & ctlr->im) != 0){
		if(icr & Irx0){
			im &= ~Irx0;
			ctlr->rim = Irx0;
			wakeup(&ctlr->rrendez);
		}
		if(icr & Itx0){
			im &= ~Itx0;
			ctlr->tim = Itx0;
			wakeup(&ctlr->trendez);
		}
		if(icr & Lsc){
			im &= ~Lsc;
			ctlr->lim = Lsc;
			wakeup(&ctlr->lrendez);
		}
	}
	ctlr->regs[Ims] = ctlr->im = im; /* enable only intrs we didn't service */
	iunlock(&ctlr->imlock);
	return forme;
}

static void
scanpci(void)
{
	int pciregs, type;
	ulong io;
	void *mem;
	Ctlr *ctlr;
	Pcidev *p;
#ifdef MSIX
	int pcimsix;
	ulong iomsi;
	void *memmsi;
#endif

	p = nil;
	while(p = pcimatch(p, Vintel, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;
		switch(p->did){
		case 0x10b6:		/* 82598 backplane */
		case 0x10c6:		/* 82598 af dual port */
		case 0x10c7:		/* 82598 af single port */
		case 0x10dd:		/* 82598 at cx4 */
		case 0x10ec:		/* 82598 at cx4 dual port */
#ifdef MSIX
			pcimsix = 3;
#endif
			type = I82598;
			break;
		case 0x10f7:		/* 82599 kx/kx4 */
		case 0x10f8:		/* 82599 kx/kx4/kx */
		case 0x10f9:		/* 82599 cx4 */
		case 0x10fb:		/* 82599 sfi/sfp+ */
		case 0x10fc:		/* 82599 xaui/bx4 */
		case 0x1557:		/* 82599 single-port sfi */
#ifdef MSIX
			pcimsix = 4;
#endif
			type = I82599;
			break;
		default:
			continue;
		}
		pciregs = 0;
		if(nctlr >= nelem(ctlrtab)){
			print("i82598: too many controllers\n");
			return;
		}

		io = p->mem[pciregs].bar & ~0xf;
		mem = vmap(io, p->mem[pciregs].size);
		if(mem == nil){
			print("i82598: can't map regs %#p\n",
				p->mem[pciregs].bar);
			continue;
		}

#ifdef MSIX
		iomsi = p->mem[pcimsix].bar & ~0xf;
		memmsi = vmap(iomsi, p->mem[pcimsix].size);
		if(memmsi == nil){
			print("i82598: can't map msi-x regs %#p\n",
				p->mem[pcimsix].bar);
			vunmap(mem, p->mem[pciregs].size);
			continue;
		}
#endif

		ctlr = malloc(sizeof *ctlr);
		if(ctlr == nil) {
			vunmap(mem, p->mem[pciregs].size);
#ifdef MSIX
			vunmap(memmsi, p->mem[pcimsix].size);
#endif
			error(Enomem);
		}
		ctlr->pcidev = p;
		ctlr->type = type;
		ctlr->physreg = (uint*)io;
		ctlr->regs = (uint*)mem;
#ifdef MSIX
		ctlr->physmsix = (uint*)iomsi; /* unused */
		ctlr->msix = (uint*)memmsi;	/* unused */
#endif
		if(reset(ctlr)){
			print("i82598: can't reset\n");
			free(ctlr);
			vunmap(mem, p->mem[pciregs].size);
#ifdef MSIX
			vunmap(memmsi, p->mem[pcimsix].size);
#endif
			continue;
		}
		pcisetbme(p);
		ctlrtab[nctlr++] = ctlr;
	}
}

static int
pnp(Ether *edev)
{
	int i;
	Ctlr *ctlr;

	if(nctlr == 0)
		scanpci();
	ctlr = nil;
	for(i = 0; i < nctlr; i++){
		ctlr = ctlrtab[i];
		if(ctlr == nil || ctlr->flag & Factive)
			continue;
		if(edev->port == 0 || edev->port == (ulong)ctlr->regs)
			break;
	}
	if (i >= nctlr)
		return -1;
	ctlr->flag |= Factive;
	edev->ctlr = ctlr;
	edev->port = (uintptr)ctlr->physreg;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 10000;
	edev->maxmtu = ETHERMAXTU;
	memmove(edev->ea, ctlr->ra, Eaddrlen);

	edev->arg = edev;
	edev->attach = attach;
	edev->detach = shutdown;
	edev->transmit = transmit;
	edev->interrupt = interrupt;
	edev->ifstat = ifstat;
	edev->shutdown = shutdown;
	edev->ctl = ctl;
	edev->multicast = multicast;
	edev->promiscuous = promiscuous;
	return 0;
}

void
ether82598link(void)
{
	addethercard("i82598", pnp);
	addethercard("i10gbe", pnp);
}
