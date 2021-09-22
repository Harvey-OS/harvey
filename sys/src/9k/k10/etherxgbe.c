/*
 * intel 10Gb ethernet pci-express controllers, notably i8259[89] and x5[45]0.
 * x520 is just an 82599, so should work.
 *
 * NB: keep these cards cool or the x540 cards at least may overheat; even
 * more so with multi-port cards.  they have to be kept under 55°C (131°F).
 *
 * copyright © 2007, coraid, inc.
 *
 * initially only for the 82598.
 * depessimised and made to work on the 82599 at bell labs, 2013.
 *
 * Some of the cache prefetchers used to stop at page boundaries, but it
 * appears that Intel fixed this in 2012, so it's no longer an issue.
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
 * if donebit (Rdd or Tdd) is 0, hardware is operating (or will be) on desc.,
 * else hardware has finished filling or draining this descriptor's buffer.
 */
#define SWOWNS(desc, donebit) ((desc)->status & (donebit))

enum {
	Slop	= 32,		/* for vlan headers, crcs, etc. */
	Rbsz	= ETHERMAXTU+Slop,
	Descalign= 128,

	Nmcbits = 12,		/* # mac addr bits used in multicast filter */
	Mctblsz = 1 << (Nmcbits - 5),	/* filter size in words; 2^5 = 32 */

	/* tunable parameters */
	/*
	 * these were Nrd=256, Nrb=1024 & Ntd=128, but Nrd=30, Nrb=47 & Ntd=1
	 * are usually ample; however systems can need more receive buffers
	 * due to bursts of (non-9P) traffic.  At 10Gb/s, a 1500-byte packet
	 * takes only 1.2µs to transmit.
	 */
	Ntd	= 32,		/* multiple of 8 */
	Nrd	= 256,		/* multiple of 8 */
	Nrb	= 1500,		/* private receive buffers, >> Nrd */

	I82598 = 1,
	I82599,			/* includes X540 for now */
};

enum {
	/* general */
	Ctrl		= 0x00000/4,	/* Device Control */
	Status		= 0x00008/4,	/* Device Status */
	Ctrlext		= 0x00018/4,	/* Extended Device Control */
	Esdp		= 0x00020/4,	/* extended sdp control */
	/* 0x28 is phy gpio on x540, mac gpio at 0x30 (x540) */
	Esodp		= 0x00028/4,	/* extended od sdp control (i2cctl on 599) */
	Ledctl		= 0x00200/4,	/* led control */
	Tcptimer	= 0x0004c/4,	/* tcp timer */
	Ecc		= 0x110b0/4,	/* 598 errata ecc control magic */
					/* (pcie intr cause on 599, x540) */

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
	Itr		= 0x00820/4,	/* " throttling rate regs (0-19 or -23) */
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
	Rdh		= 0x01010/4,	/* " head, index 1st valid desc */
	Rdt		= 0x01018/4,	/* " tail, index 1st invalid desc */
	Rxdctl		= 0x01028/4,	/* " control */

	Srrctl		= 0x02100/4,	/* split & replication rx ctl. array */
	Dcarxctl	= 0x02200/4,	/* rx dca control */
	Rdrxctl		= 0x02f00/4,	/* rx dma control */
	Rxctl		= 0x03000/4,	/* rx control */
	Rxpbsize	= 0x03c00/4,	/* rx packet buffer size */
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
	Mcstctrl	= 0x05090/4,	/* multicast control */
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
	Tdh		= 0x06010/4,	/* " head, index 1st valid desc */
	Tdt		= 0x06018/4,	/* " tail, index 1st invalid desc */
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
	Macc		= 0x04330/4,	/* mac control (x540) */

	/* management (ME/BMC/AMT/IPMI) rubbish */
	Manc		= 0x5820/4,	/* mgmt control (x540) */
	Mfval		= 0x5824/4,	/* mgmt filters valid (x540) */
};

enum {
	Factive		= 1<<0,
	Enable		= 1<<31,

	/* Ctrl */
	Rst		= 1<<26,	/* full nic reset */

	/* Txdctl */
	PthreshShift	= 0,		/* prefresh threshold shift in bits */
	HthreshShift	= 8,		/* host buffer minimum threshold " */
	WthreshShift	= 16,		/* writeback threshold */
	Ten		= 1<<25,

	/* Dtxctl99 */
	Te		= 1<<0,		/* dma tx enable */

	/* Fctrl */
	Bam		= 1<<10,	/* broadcast accept mode */
	Upe		= 1<<9,		/* unicast promiscuous */
	Mpe		= 1<<8,		/* multicast promiscuous */

	/* Rxdctl */
	Rlpmlen		= 1<<15,	/* enable long-packet filter */
	Renable		= 1<<25,
	Vme		= 1<<30,	/* vlan mode (strip vlan tag) enable */

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

	/* interrupt causes (Eicr, etc.) */
	Irx0bitno	= 0,
	Irx0		= 1<<Irx0bitno,	/* driver defined: rcv q knocking */
	Itx0bitno	= 1,
	Itx0		= 1<<Itx0bitno,	/* driver defined: tx q knocking */
	Lsc		= 1<<20,	/* link status change */
	Mng		= 1<<22,	/* ME buggery: likely overheated */
	Phyintr		= 1<<29,	/* likely overheated */

	/* Links */
	Lnkup		= 1<<30,
	Lnkspd		= 3<<28,
	Lnkshift	= 28,

	/* Hlreg0 */
	Txcrcen		= 1<<0,		/* add crc during xmit */
	Rxcrcstrip	= 1<<1,		/* strip crc during recv */
	Jumboen		= 1<<2,
	Txpaden		= 1<<10,	/* pad short frames during xmit */

	/* Autoc (598) */
	Flu		= 1<<0,		/* force link up */
	Lmsshift	= 13,		/* link mode select shift */
	Lmsmask		= 7,

	/* Macc (x540) */
//	Flu		= 1<<0,		/* force link up */
	Rx2txloop	= 1<<1,		/* rx-to-tx loopback */

	/* Mcstctrl */
	MoMASK		= MASK(2)<<0,	/* Multicast Offset */
	Mfe		= 1<<2,		/* multicast filter enable */
};

typedef struct Ctlr Ctlr;
typedef struct Rd Rd;
typedef struct Td Td;

typedef struct {
	uint	reg;
	char	*name;
} Stat;

Stat stattab[] = {
	0x4000,	"crc errors",
	0x4004,	"bad byte count",
	0x4008,	"short packets",
	0x3fa0,	"missed pkt0",			/* not in x540 */
	0x4034,	"mac local faults",
	0x4038,	"mac remote faults",

	0x40d0,	"rcv pkts",
	0x4074,	"rcv okay",
	0x4078,	"rcv broadcast",
	0x407c,	"rcv multicast",
//	0x4120,	"rcv ip checksum errors",	/* seems to be garbage */
	0x3fc0,	"rcv no buf0",			/* not in x540 */
	0x4040,	"rcv length errors",
	0x40a4,	"rcv runt",
	0x40a8,	"rcv fragment",
	0x40ac,	"rcv oversize",
	0x40b0,	"rcv jabber",

	0x40d4,	"xmit pkts",
	0x40f4,	"xmit broadcast",
	0x40f0,	"xmit multicast",
};

enum {
	/* status */
	Pif	= 1<<7,	/* past exact filter (sic) */
	Ipcs	= 1<<6,	/* ip checksum calculated */
	L4cs	= 1<<5,	/* layer 4 checksum calculated */
	Tcpcs	= 1<<4,	/* tcp checksum calculated */
	Vp	= 1<<3,	/* 802.1q packet matched vet */
//	Ixsm	= 1<<2,	/* ignore checksum */
	Reop	= 1<<1,	/* end of packet */
	Rdd	= 1<<0,	/* descriptor done */

	/* errors */
	Ipe	= 1<<7,	/* ipv4 checksum error */
	Tcpe	= 1<<6,	/* tcp/udp checksum error */
	Rxe	= 1<<0,	/* mac error: bad crc, wrong size */
};

struct Rd {			/* Receive Descriptor */
	uvlong	vladdr;
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
	uvlong	vladdr;
	ushort	length;
	uchar	cso;
	uchar	cmd;
	uchar	status;
	uchar	css;
	ushort	vlan;
};

struct Ctlr {
	Ethident;		/* see etherif.h */
	Pcidev	*pcidev;
	int	is598;		/* flag: is 82598, the 1st cut? */
	int	is540;		/* flag: is x540 (slight variant of 599)? */

	Lock	imlock;
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

	Rendez	lrendez;

	uchar	flag;
	int	procsrunning;	/* flag: kprocs started for this Ctlr? */
	QLock	alock;		/* attach lock */
	int	attached;

	uchar	ra[Eaddrlen];	/* receive address */
	uchar	mta[Mctblsz];	/* multicast table array (copy) */
	uint	speeds[3];

	QLock	slock;
	ulong	stats[nelem(stattab)];

	uint	ixcs;		/* valid hw checksum count */
	uint	ipcs;		/* good hw ip checksums */
	uint	tcpcs;		/* good hw tcp/udp checksums */
};

static	Ctlr	*ctlrtab[16];
static	int	nctlr;

/* these are shared by all the controllers of this type */
static	Lock	rblock;
static	Block	*rbpool;
/* # of rcv Blocks awaiting processing; can be briefly < 0 */
static	int	nrbfull;

/* wait up to a second for bit in ctlr->regs[reg] to become zero */
static int
awaitregbitzero(Ctlr *ctlr, int reg, ulong bit)
{
	return awaitbitpat(&ctlr->regs[reg], bit, 0);
}

static void
readstats(Ctlr *ctlr)
{
	uint i, reg, fam;

	qlock(&ctlr->slock);
	for(i = 0; i < nelem(ctlr->stats); i++) {
		reg = stattab[i].reg;
		fam = reg & 0xff00;
		if (ctlr->is540 && (fam == 0x3f00 || fam == 0xcf00))
			continue;	/* register not present on x540 */
		ctlr->stats[i] += ctlr->regs[reg / BY2WD];
	}
	qunlock(&ctlr->slock);
}

static int speedtab[] = {
	100,
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
			p = seprint(p, e, "%s\t%uld\n", stattab[i].name,
				ctlr->stats[i]);
	t = ctlr->speeds;
	p = seprint(p, e, "%æ\n", ctlr);
	p = seprint(p, e, "rxctl %#ux\n",  ctlr->regs[Rxctl]);
	p = seprint(p, e, "rxdctl %#ux\n", ctlr->regs[Rxdctl]);
	p = seprint(p, e, "rdrxctl %#ux\n", ctlr->regs[Rdrxctl]);
	p = seprint(p, e, "txdctl %#ux\n", ctlr->regs[Txdctl]);
	p = seprint(p, e, "speeds: 0:%d 1G:%d 10G:%d\n", t[0], t[1], t[2]);
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
	ilock(&ctlr->imlock);
	ctlr->im |= ie;
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
	int r, sp;
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (;;) {
		r = ctlr->regs[Links];
		edev->link = (r & Lnkup) != 0;
		if(edev->link) {
			sp = (r & Lnkspd) >> Lnkshift;
			if (sp >= 2)	/* 1 or 10 Gb/s? */
				sp--;
			else
				sp = 0;	/* lump reserved & 100 Mb/s together */
			ctlr->speeds[sp]++;
			edev->mbps = speedtab[sp];
		}

		ienable(ctlr, Lsc);
		sleep(&ctlr->lrendez, lim, ctlr);
		ctlr->lim = 0;
	}
}

static Block*
rballoc(void)
{
	Block *bp;

	ilock(&rblock);
	if((bp = rbpool) != nil){
		rbpool = bp->next;
		bp->next = nil;
#ifndef PLAN9K
		/* avoid bp being freed so we can reuse it (9 only, for gre) */
		ainc(&bp->ref);
#endif
	}
	iunlock(&rblock);
	return bp;
}

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
	for (td = &ctlr->tdba[n]; td->status & Tdd; td = &ctlr->tdba[n]){
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
		td->vladdr = PCIWADDR(bp->rp);
		td->length = BLEN(bp);
		td->cmd = Ifcs | Teop | Rs;
		ctlr->tb[tdt] = bp;
		coherence();
		ctlr->regs[Tdt] = ctlr->tdt = tdt;  /* make new Tds active */
		coherence();

		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (uint)(tdt + Ntd - tdh) % Ntd);
		tdt = NEXT(tdt, Ntd);
	}
	if(i) {
		coherence();
		ctlr->regs[Tdt] = ctlr->tdt = tdt;  /* make prior Tds active */
		coherence();
		ienable(ctlr, Itx0);
	}
	qunlock(&ctlr->tlock);
}

static int
tim(void *vc)
{
	Ctlr *ctlr = (void *)vc;

	return ctlr->tim != 0 || qlen(ctlr->edev->oq) > 0 &&
		SWOWNS(&ctlr->tdba[ctlr->tdt], Tdd);
}

static void
tproc(void *v)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = v;
	ctlr = edev->ctlr;
	for (;;) {
		ctlr->tsleep++;
		tsleep(&ctlr->trendez, tim, ctlr, 1103); /* xmit intr kicks us */
		ctlr->tim = 0;
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

/*
 * Get the receiver into a usable state.  Some of this is boilerplate
 * that could be (or is) done automatically as part of reset (hint, hint),
 * but we also disable broken features (e.g., IP checksums).
 */
static void
rxinit(Ctlr *ctlr)
{
	int i;
	uint *regs;
	uvlong phys;		/* instead of uintptr, for 9 compat */

	regs = ctlr->regs;
	regs[Rxctl] &= ~Rxen;		/* shutdown rcvr */
	regs[Rxdctl] = 0;
	coherence();
	ctlr->rdfree = 0;

	if (ctlr->is540)
		regs[Manc] = regs[Mfval] = 0;	/* try to exclude ME */
	/*
	 * avoid hw cksums on 82599 due to errata 43 or 44: incorrect L4 error
	 * for UDP w 0 checksum.
	 */
	if (ctlr->type == I82599 && !ctlr->is540)
		regs[Rxcsum] &= ~Ippcse;
	else
		regs[Rxcsum] |= Ippcse;
	regs[Hlreg0] &= ~Jumboen;		/* jumbos are a bad idea */
	regs[Hlreg0] |= Txcrcen | Rxcrcstrip | Txpaden;
	regs[Srrctl] = HOWMANY(Rbsz, 1024);
	regs[Mhadd] = Rbsz << 16;

	/* clear the multicast tables (regs & dram copy) */
	memset(ctlr->mta, 0, sizeof ctlr->mta);
	for(i = 0; i < Mctblsz; i++)
		regs[Mta + i] = 0;

	regs[Mcstctrl] &= ~MoMASK;		/* 0: use mac bits 47-36 */
	regs[Mcstctrl] |= Mfe;

	/*
	 * multicast filtering isn't working on x540, perhaps because we have
	 * to configure mac pool filtering or something.  for now, just
	 * accept them all.
	 */
	regs[Fctrl] &= ~Upe;
	regs[Fctrl] |= Mpe | Bam;

	/* set up the rcv ring */
	phys = PCIWADDR(ctlr->rdba);
	regs[Rbal] = phys;
	regs[Rbah] = phys>>32;
	regs[Rdlen] = Nrd*sizeof(Rd);		/* must be multiple of 128 */
	regs[Rdh] = 0;
	regs[Rdt] = ctlr->rdt = 0;
	coherence();

	if (ctlr->is598)
		regs[Rdrxctl] = Rdmt¼;
	else {
		regs[Rdrxctl] |= Crcstrip;
		regs[Rdrxctl] &= ~Rscfrstsize;
	}
	regs[Rxdctl] &= ~Rlpmlen;
	regs[Rxdctl] |= Renable;
	coherence();
	/*
	 * don't wait forever like an idiot (and hang the system),
	 * maybe it's disconnected.
	 */
	if (awaitbitpat(&regs[Rxdctl], Renable, Renable) < 0)
		print("%æ: Renable didn't come on, might be disconnected\n",
			ctlr);

	regs[Rxctl] |= Rxen | (ctlr->is598? Dmbyps: 0);
	coherence();

	if (ctlr->is598)
		regs[Autoc] |= Flu;
	else if (0 && ctlr->is540) {		/* doesn't seem to be needed */
		regs[Macc] &= ~Rx2txloop;
		regs[Macc] |= Flu;
	}
	coherence();
	delay(50);
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
			print("%æ: rx overrun\n", ctlr);
			break;
		}
		*rb = bp = rballoc();
		if(bp == nil)
			/*
			 * this can probably be changed to a "break", but
			 * that hasn't been tested in this driver.
			 */
			panic("%æ: all %d rx buffers in use, nrbfull %d",
				ctlr, Nrb, nrbfull);
		rd->vladdr = PCIWADDR(bp->rp);
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

	return ctlr->rim != 0 || SWOWNS(&ctlr->rdba[ctlr->rdh], Rdd);
}

/*
 * In theory, with no errors,
 * the descriptor status L4cs and Ipcs bits give
 * an indication of whether the checksums were
 * calculated and valid.
 */
static void
ckcksum(Ctlr *ctlr, Rd *rd, Block *bp)
{
	ctlr->ixcs++;
	if(rd->status & Ipcs){
		/*
		 * IP checksum calculated (and valid as errors == 0).
		 */
		ctlr->ipcs++;
		bp->flag |= Bipck;
	}
	if(rd->status & L4cs){
		/*
		 * TCP/UDP checksum calculated (and valid as errors == 0).
		 */
		ctlr->tcpcs++;
		bp->flag |= Btcpck|Budpck;
	}
	bp->checksum = rd->checksum;
	bp->flag |= Bpktck;
}

static int
qinpkt(Ctlr *ctlr)
{
	int passed;
	uint rdh;
	Block *bp;
	Rd *rd;

	ctlr->rim = 0;
	rdh = ctlr->rdh;
	rd = &ctlr->rdba[rdh];
	/* Rdd indicates a reusable rd; sw owns it */
	if (!(rd->status & Rdd))
		return -1;		/* wait for pkts to arrive */

	/*
	 * Accept eop packets with no errors.
	 */
	passed = 0;
	if(rd->status & Reop && rd->errors == 0){
		bp = ctlr->rb[rdh];
		if (bp == nil)
			panic("%æ: nil Block* from ctlr->rb", ctlr);
		if (rd->length > ETHERMAXTU)
			print("%æ: got jumbo of %d bytes\n", ctlr, rd->length);
		else
			bp->wp += rd->length;
		ckcksum(ctlr, rd, bp);
		ainc(&nrbfull);
		notemark(&ctlr->wmrb, nrbfull);
		etheriq(ctlr->edev, bp, 1);	/* pass pkt upstream */
		passed++;
	} else {
		ainc(&nrbfull);			/* rbfree will adec */
		freeb(ctlr->rb[rdh]);		/* toss bad pkt */
	}
	ctlr->rb[rdh] = nil;
	ctlr->rdfree--;
	ctlr->rdh = NEXT(rdh, Nrd);
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
		if (ctlr->rintr < 2*Nrd || edev->outpackets > 10)
			ienable(ctlr, Irx0);
		ctlr->rsleep++;
		tsleep(&ctlr->rrendez, rim, ctlr, 997);

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
	if(on)
		ctlr->regs[Fctrl] |= Upe | Mpe;
	else
		ctlr->regs[Fctrl] &= ~(Upe | Mpe);
	ctlr->regs[Fctrl] |= Mpe;		/* x540 workaround */
}

static void
multicast(void *a, uchar *addr, int on)
{
	ulong hash, word, bit;
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
	/* assume multicast offset in Mcstctrl is 0 (we clear it below) */
	hash = addr[5] << 4 | addr[4] >> 4;	/* bits 47:36 of mac */
	if (Nmcbits == 10)
		hash >>= 2;			/* discard 37:36 of mac */
	word = (hash / BI2WD) % Mctblsz;
	bit = 1UL << (hash % BI2WD);
	if(on)
		ctlr->mta[word] |= bit;
//	else
//		ctlr->mta[word] &= ~bit;
	ctlr->regs[Mta + word] = ctlr->mta[word];
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
	/* don't free uncached descriptors */
//	freez(&ctlr->rdba);
//	freez(&ctlr->tdba);
	freez(&ctlr->rb);
	freez(&ctlr->tb);
}

static int
detach(Ctlr *ctlr)
{
	int i;
	uint *regs;

	regs = ctlr->regs;
	regs[Imc] = ~0;
	regs[Ctrl] |= Rst;
	coherence();
	if (awaitregbitzero(ctlr, Ctrl, Rst) < 0) {
		print("%æ: Ctlr(%#ux).Rst(%#ux) never went to 0\n",
			ctlr, regs[Ctrl], Rst);
		return -1;
	}
	if (ctlr->is598) {			/* errata magic */
		delay(50);
		regs[Ecc] &= ~(1<<21 | 1<<18 | 1<<9 | 1<<6);
	}

	/* take no chances; shut it down manually too. */
	regs[Imc] = ~0;
	regs[Rxctl] &= ~Rxen;			/* shutdown rcvr */
	regs[Rxdctl] = regs[Txdctl] = 0;
	if (!ctlr->is598)
		regs[Dtxctl99] = 0;
	coherence();

	/* not cleared by reset on all models; disable it manually. */
	for(i = 1; i < (ctlr->is540? 128: 16); i++)
		regs[(ctlr->is598? Rah98: Rah99) + i] &= ~Enable;
	/* clear the multicast tables (regs & dram copy) */
	memset(ctlr->mta, 0, sizeof ctlr->mta);
	for(i = 0; i < Mctblsz; i++)
		regs[Mta + i] = 0;
	for(i = 1; i < (ctlr->is598? 640: 128); i++)
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
	uint *eerdp;

	eerdp = &ctlr->regs[Eerd];
	*eerdp = EEstart | i<<2;
	while((*eerdp & EEdone) == 0)
		pause();
	return *eerdp >> 16;
}

/* sets ctlr->ra to mac address if successful */
static int
eeload(Ctlr *ctlr)
{
	ushort u, v, p, l, i, j;

	if((eeread(ctlr, 0) & 0xc0) != 0x40)
		return -1;
	u = 0;
	for(i = 0; i < 0x40; i++)
		u += eeread(ctlr, i);
	for(i = 3; i < 0xf; i++){
		p = eeread(ctlr, i);
		l = eeread(ctlr, p++);
		if((int)p + l + 1 < (1<<16))
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

static void
setmacs(Ctlr *ctlr)
{
	int ral, rah;
	uint *regs;
	ulong eal, eah;
	uchar *p;

	regs = ctlr->regs;
	ral = ctlr->is598? Ral98: Ral99;
	rah = ctlr->is598? Rah98: Rah99;
	eal = regs[ral];
	eah = regs[rah];
	p = ctlr->ra;
	if (eal == 0 && eah == 0)		/* not yet set in hw? */
		/* load ctlr->ra from non-volatile memory, if present */
		if (eeload(ctlr) >= 0){
			/* set mac addr in regs from ctlr->ra */
			regs[ral] = eal = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
			regs[rah] = eah = p[5]<<8 | p[4] | Enable;
		} else
			/* may have no flash, like the i210 */
			print("%æ: no mac address found\n", ctlr);
	p = leputl(p, eal);
	*p++ = eah;
	*p = eah >> 8;
}

enum {
	Ivarok	= 1<<7,
	Rxqoff	= 0,
	Txqoff,
};

/* map queue 2*qno+qoff to icr bit icrbitno */
static void
setivar(Ctlr *ctlr, uint qno, uint qoff, uint icrbitno)
{
	uint byte, shift;
	uint *ivarreg;

	byte = qno*2 + qoff;
	shift = BI2BY * (byte % 4);
	ivarreg = &ctlr->regs[Ivar + byte/BY2WD];
	*ivarreg &= ~(MASK(8) << shift);
	*ivarreg |= (icrbitno | Ivarok) << shift;
}

/* may be called from scanpci with ctlr only partially populated */
static int
reset(Ctlr *ctlr)
{
	int i;
	uint *regs;

	if(detach(ctlr)){
		print("%æ: reset timeout\n", ctlr);
		return -1;
	}

	regs = ctlr->regs;
	assert(regs);
	/* if unknown, load mac address from non-volatile memory, if present */
	setmacs(ctlr);

	readstats(ctlr);		/* zero stats by reading regs */
	memset(ctlr->stats, 0, sizeof ctlr->stats);

	regs[Ctrlext] |= 1 << 16;	/* required by errata (spec change 4) */
	regs[Fcrtl] = regs[Fcrth] = regs[Rcrtv] = 0;

	/* configure interrupt -> icr cause bit mapping */
	for (i = 0; i < 64; i++)
		regs[Ivar + i] = 0;
	setivar(ctlr, 0, Rxqoff, Irx0bitno);
	setivar(ctlr, 0, Txqoff, Itx0bitno);

	for(i = Itr; i < Itr + (ctlr->is598? 20: 24); i++)
		regs[i] = 0;			/* ¼µs intervals */
	return 0;
}

/*
 * Get the transmitter into a usable state.  Much of this is boilerplate
 * that could be (or is) done automatically as part of reset (hint, hint).
 */
static void
txinit(Ctlr *ctlr)
{
	uvlong phys;		/* instead of uintptr, for 9 compat */
	uint *regs;

	regs = ctlr->regs;
	regs[Txdctl] = 0;
	if (!ctlr->is598)
		regs[Dtxctl99] = 0;
	coherence();

	/* set up tx queue 0 ring */
	assert(ctlr->tdba != nil);
	memset(ctlr->tdba, 0, Ntd * sizeof(Td));
	phys = PCIWADDR(ctlr->tdba);
	regs[Tdbal] = phys;
	regs[Tdbah] = phys>>32;
	regs[Tdlen] = Ntd*sizeof(Td);	/* must be multiple of 128 */
	regs[Tdh] = 0;
	ctlr->tdh = Ntd - 1;
	regs[Tdt] = ctlr->tdt = 0;
	coherence();

	if (!ctlr->is598)
		regs[Dtxctl99] |= Te;	/* tx dma on */
	coherence();
	/* non-zero thresholds should be slower; leave them */
	regs[Txdctl] |= Ten;
	/*
	 * don't wait forever like an idiot (and hang the system),
	 * maybe it's disconnected.
	 */
	if (awaitbitpat(&regs[Txdctl], Ten, Ten) < 0)
		print("%æ: Ten didn't come on, might be disconnected\n", ctlr);
}

static void
allocall(Ctlr *ctlr)
{
	int i;
	Block *bp;

	/* discard any buffer Blocks left over from before detach */
	freeblks(ctlr->tb, Ntd);
	freeblks(ctlr->rb, Nrd);

	ctlr->rdba = mallocalign(Nrd * sizeof(Rd), Descalign, 0, 0);
	ctlr->tdba = mallocalign(Ntd * sizeof(Td), Descalign, 0, 0);
	ctlr->rb = malloc(Nrd * sizeof(Block *));
	ctlr->tb = malloc(Ntd * sizeof(Block *));
	if (ctlr->rdba == nil || ctlr->tdba == nil ||
	    ctlr->rb == nil || ctlr->tb == nil)
		error(Enomem);

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
	ienable(ctlr, Lsc);
	etherkproc(edev, tproc, "xmit");
	etherkproc(edev, rproc, "rcv");
	ctlr->procsrunning = 1;
}

/* could be attaching again after a detach */
static void
attach(Ether *edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(waserror()){
		reset(ctlr);
		freemem(ctlr);
		qunlock(&ctlr->alock);
		nexterror();
	}
	if(ctlr->rdba == nil)
		allocall(ctlr);
	if (!ctlr->attached) {
		rxinit(ctlr);
		txinit(ctlr);
		initmark(&ctlr->wmrb, Nrb, "rcv Blocks not yet processed");
		initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
		initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");
		startkprocs(ctlr);
		ctlr->attached = 1;
	}
	qunlock(&ctlr->alock);
	poperror();
}

enum {
	Ignerrata = 1<<23,		/* errata: ignore bit 23 as a cause */
};

static Intrsvcret
interrupt(Ureg*, void *v)
{
	int icr, icrreg, im;
	uint *regs;
	Ctlr *ctlr;

	ctlr = ((Ether *)v)->ctlr;
	regs = ctlr->regs;
	ilock(&ctlr->imlock);
	regs[Imc] = ~0;			/* disable all my intrs */
	coherence();
	icrreg = regs[Icr];
	regs[Icr] = icrreg; /* errata: clear causes in case read didn't */
	coherence();
	im = ctlr->im;
	icr = icrreg & im & ~Ignerrata;
	if (icr == 0) {
		regs[Ims] = im;
		iunlock(&ctlr->imlock);
		if ((icrreg & ~Lsc) != 0)
			iprint("%æ: uninteresting intr, icr %#ux\n", ctlr, icrreg);
		return Intrnotforme;
	}

	if(icr & Irx0){
		ctlr->rim = Irx0;
		wakeup(&ctlr->rrendez);
		im &= ~Irx0;
		ctlr->rintr++;
	}
	if(icr & Itx0){
		ctlr->tim = Itx0;
		wakeup(&ctlr->trendez);
		im &= ~Itx0;
		ctlr->tintr++;
	}
	if(icr & Lsc){
		ctlr->lim = Lsc;
		wakeup(&ctlr->lrendez);
		/* im &= ~Lsc;		/* we always want to see link changes */
	}
	/* if set, the controller has probably overheated. */
	if (icrreg & (Mng|Phyintr))
		iprint("%æ: Mng or Phyintr set @ %lud ticks; "
			"probably shutdown due to overheating\n",
			ctlr, (ulong)sys->ticks);
	/* enable only intr sources we didn't service and are interested in */
	regs[Ims] = ctlr->im = im;
	iunlock(&ctlr->imlock);
	return Intrforme;
}

/*
 * map device p and populate a new Ctlr for it.
 * add the new Ctlr to our list.
 */
static Ctlr *
newctlr(Pcidev *p, int type, int is540, int is550)
{
	int pciregs;
	ulong io;
	void *mem;
	Ctlr *ctlr;

	pciregs = 0;
	io = p->mem[pciregs].bar & ~0xf;
	mem = vmap(io, p->mem[pciregs].size);
	if(mem == nil){
		print("#l%d: etherxgbe: can't map regs %#lux\n",
			nctlr, p->mem[pciregs].bar);
		return nil;
	}

	ctlr = malloc(sizeof *ctlr);
	if(ctlr == nil) {
		vunmap(mem, p->mem[pciregs].size);
		error(Enomem);
	}
	ctlr->regs = (uint*)mem;
	ctlr->physreg = (uint*)io;
	ctlr->pcidev = p;
	ctlr->type = type;
	ctlr->is598 = type == I82598;
	ctlr->is540 = is540;
	ctlr->prtype = ctlr->is598? "i82598":
			is550? "x550":
			is540? "x540": "i82599";
	if(reset(ctlr)){
		print("%æ: can't reset\n", ctlr);
		free(ctlr);
		vunmap(mem, p->mem[pciregs].size);
		return nil;
	}
	pcisetbme(p);
	pcisetpms(p, 0);
	/* pci-e ignores PciCLS */
	ctlrtab[nctlr++] = ctlr;
	return ctlr;
}

static void
scanpci(void)
{
	int type, is540, is550;
	Pcidev *p;

	fmtinstall(L'æ', etherfmt);
	p = nil;
	while(p = pcimatch(p, Vintel, 0)){
		if(p->ccrb != Pcibcnet || p->ccru != Pciscether)
			continue;
		is540 = is550 = 0;
		switch(p->did){
			/* 82598, oldest */
		case 0x10b6:		/* backplane */
		case 0x10c6:		/* xf sr dual port */
		case 0x10c7:		/* xf sr */
		case 0x10cb:		/* af */
		case 0x10dd:		/* at cx4 */
		case 0x10ec:		/* at cx4 dual port */
		case 0x10f1:		/* af da */
		case 0x10f4:		/* xf lr */
		case 0x150b:		/* at2 */
			type = I82598;
			break;
			/*
			 * x5[45]0, current, 10g-base-t copper.  mostly act like
			 * 82599.
			 */
		case 0x1201:		/* x550 dual port */
		case 0x1562:		/* x550 */
		case 0x1563:		/* x550-t2 (e.g., sonnet) */
		case 0x1564:		/* " in a vm */
		case 0x1565:		/* " in a vm */
		case 0x15d1:		/* x550-t1 */
			is550 = 1;
			/* fall through */
		case 0x1515:		/* x540 in a vm */
		case 0x1528:		/* x540-t[12] */
		case 0x1530:		/* x540 in a vm */
		case 0x1560:		/* x540-t1 */
			is540 = 1;
			/* fall through */
			/* 82599, includes x520, second oldest */
		case 0x10f7:		/* kx/kx4 */
		case 0x10f8:		/* kx/kx4/kx */
		case 0x10f9:		/* cx4 */
		case 0x10fb:		/* x520-sr[12]/lr1/da2 sfi/sfp+ */
		case 0x10fc:		/* xaui/bx4 */
		case 0x151c:		/* x520-t2 10g-base-t */
		case 0x154d:		/* x520 sfp+ */
		case 0x1557:		/* single-port sfi */
		case 0x1558:		/* x520-qda1 */
			type = I82599;
			break;
		default:
			continue;
		}
		if(nctlr >= nelem(ctlrtab)){
			print("#l%d: etherxgbe: too many controllers\n", nctlr);
			return;
		}
		newctlr(p, type, is540, is550);
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
		if(edev->port == 0 || edev->port == (uintptr)ctlr->physreg)
			break;
	}
	if (i >= nctlr)
		return -1;

	ctlr->flag |= Factive;
	ctlr->edev = edev;		/* point back to Ether */
	edev->ctlr = ctlr;
	edev->port = (uintptr)ctlr->physreg;	/* for printing */
	edev->pcidev = ctlr->pcidev;
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
	edev->ctl = nil;
	edev->multicast = multicast;
	edev->promiscuous = promiscuous;
	return 0;
}

void
etherxgbelink(void)
{
	addethercard("xgbe", pnp);
}
