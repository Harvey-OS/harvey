/*
 * intel pci-express 10Gb ethernet driver for 8259[89]
 * copyright © 2007, coraid, inc.
 * depessimised and made to work on the 82599 at bell labs, 2013.
 *
 * 82599 requests should ideally not cross a 4KB (page) boundary.
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

#define NEXTPOW2(x, m)	(((x)+1) & (m))

enum {
	Rbsz	= ETHERMAXTU+32, /* +slop is for vlan headers, crcs, etc. */
	Descalign= 128,		/* 599 manual needs 128-byte alignment */

	/* tunable parameters */
	Goslow	= 0,		/* flag: go slow by throttling intrs, etc. */
	/* were 256, 1024 & 64, but 30, 47 and 1 are ample. */
	Nrd	= 64,		/* multiple of 8, power of 2 for NEXTPOW2 */
	Nrb	= 128,
	Ntd	= 32,		/* multiple of 8, power of 2 for NEXTPOW2 */
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
	Ecc		= 0x110b0/4,	/* errata ecc control magic (pcie intr cause on 599) */

	/* nvm */
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
	/* msi interrupt */
	Msixt		= 0x0000/4,	/* msix table (bar3) */
	Msipba		= 0x2000/4,	/* msix pending bit array (bar3) */
	Pbacl		= 0x11068/4,	/* pba clear */
	Gpie		= 0x00898/4,	/* general purpose int enable */

	/* flow control */
	Pfctop		= 0x03008/4,	/* priority flow ctl type opcode */
	Fcttv		= 0x03200/4,	/* " transmit timer value (0-3) */
	Fcrtl		= 0x03220/4,	/* " rx threshold low (0-7) +8n */
	Fcrth		= 0x03260/4,	/* " rx threshold high (0-7) +8n */
	Rcrtv		= 0x032a0/4,	/* " refresh value threshold */
	Tfcs		= 0x0ce00/4,	/* " tx status */

	/* rx dma */
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

	/* rx */
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

	/* tx */
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
	Upe 		= 1<<9,		/* unicast promiscuous */
	Mpe 		= 1<<8,		/* multicast promiscuous */

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
	u32int	addr[2];
	ushort	length;
	ushort	cksum;
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
	u32int	addr[2];
	ushort	length;
	uchar	cso;
	uchar	cmd;
	uchar	status;
	uchar	css;
	ushort	vlan;
};

struct Ctlr {
	Pcidev	*p;
	Ether	*edev;
	int	type;

	/* virtual */
	u32int	*reg;
	u32int	*msix;			/* unused */

	/* physical */
	u32int	*physreg;
	u32int	*physmsix;		/* unused */

	uchar	flag;
	int	nrd;
	int	ntd;
	int	nrb;			/* # bufs this Ctlr has in the pool */
	uint	rbsz;
	int	procsrunning;
	int	attached;

	Watermark wmrb;
	Watermark wmrd;
	Watermark wmtd;

	QLock	slock;
	QLock	alock;			/* attach lock */
	QLock	tlock;
	Rendez	lrendez;
	Rendez	trendez;
	Rendez	rrendez;

	uint	im;			/* interrupt mask */
	uint	lim;
	uint	rim;
	uint	tim;
	Lock	imlock;

	Rd*	rdba;			/* receive descriptor base address */
	Block**	rb;			/* receive buffers */
	int	rdt;			/* receive descriptor tail */
	int	rdfree;			/* rx descriptors awaiting packets */

	Td*	tdba;			/* transmit descriptor base address */
	int	tdh;			/* transmit descriptor head */
	int	tdt;			/* transmit descriptor tail */
	Block**	tb;			/* transmit buffers */

	uchar	ra[Eaddrlen];		/* receive address */
	uchar	mta[128];		/* multicast table array */
	ulong	stats[nelem(stattab)];
	uint	speeds[3];
};

enum {
	I82598 = 1,
	I82599,
};

static	Ctlr	*ctlrtab[4];
static	int	nctlr;
static	Lock	rblock;
static	Block	*rbpool;
static	int	nrbfull;  /* # of rcv Blocks with data awaiting processing */

static void
readstats(Ctlr *ctlr)
{
	int i;

	qlock(&ctlr->slock);
	for(i = 0; i < nelem(ctlr->stats); i++)
		ctlr->stats[i] += ctlr->reg[stattab[i].reg >> 2];
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
	p = seprint(p, e, "rdfree %d rdh %d rdt %d\n", ctlr->rdfree, ctlr->reg[Rdt],
		ctlr->reg[Rdh]);
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
	ctlr->reg[Ims] = ctlr->im;
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
	Ether *e;

	e = v;
	ctlr = e->ctlr;
	for (;;) {
		r = ctlr->reg[Links];
		e->link = (r & Lnkup) != 0;
		i = 0;
		if(e->link)
			i = 1 + ((r & Lnkspd) != 0);
		ctlr->speeds[i]++;
		e->mbps = speedtab[i];
		ctlr->lim = 0;
		ienable(ctlr, Lsc);
		sleep(&ctlr->lrendez, lim, ctlr);
		ctlr->lim = 0;
	}
}

static long
ctl(Ether *, void *, long)
{
	error(Ebadarg);
	return -1;
}

static Block*
rballoc(void)
{
	Block *bp;

	ilock(&rblock);
	if((bp = rbpool) != nil){
		rbpool = bp->next;
		bp->next = 0;
		_xinc(&bp->ref);	/* prevent bp from being freed */
	}
	iunlock(&rblock);
	return bp;
}

void
rbfree(Block *b)
{
	b->rp = b->wp = (uchar*)PGROUND((uintptr)b->base);
 	b->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);
	ilock(&rblock);
	b->next = rbpool;
	rbpool = b;
	nrbfull--;
	iunlock(&rblock);
}

static int
cleanup(Ctlr *ctlr, int tdh)
{
	Block *b;
	uint m, n;

	m = ctlr->ntd - 1;
	while(ctlr->tdba[n = NEXTPOW2(tdh, m)].status & Tdd){
		tdh = n;
		b = ctlr->tb[tdh];
		ctlr->tb[tdh] = 0;
		if (b)
			freeb(b);
		ctlr->tdba[tdh].status = 0;
	}
	return tdh;
}

void
transmit(Ether *e)
{
	uint i, m, tdt, tdh;
	Ctlr *ctlr;
	Block *b;
	Td *t;

	ctlr = e->ctlr;
	if(!canqlock(&ctlr->tlock)){
		ienable(ctlr, Itx0);
		return;
	}
	tdh = ctlr->tdh = cleanup(ctlr, ctlr->tdh);
	tdt = ctlr->tdt;
	m = ctlr->ntd - 1;
	for(i = 0; ; i++){
		if(NEXTPOW2(tdt, m) == tdh){	/* ring full? */
			ienable(ctlr, Itx0);
			break;
		}
		if((b = qget(e->oq)) == nil)
			break;
		assert(ctlr->tdba != nil);
		t = ctlr->tdba + tdt;
		t->addr[0] = PCIWADDR(b->rp);
		t->length = BLEN(b);
		t->cmd = Ifcs | Teop;
		if (!Goslow)
			t->cmd |= Rs;
		ctlr->tb[tdt] = b;
		/* note size of queue of tds awaiting transmission */
		notemark(&ctlr->wmtd, (tdt + Ntd - tdh) % Ntd);
		tdt = NEXTPOW2(tdt, m);
	}
	if(i) {
		coherence();
		ctlr->reg[Tdt] = ctlr->tdt = tdt;  /* make new Tds active */
		coherence();
		ienable(ctlr, Itx0);
	}
	qunlock(&ctlr->tlock);
}

static int
tim(void *c)
{
	return ((Ctlr*)c)->tim != 0;
}

static void
tproc(void *v)
{
	Ctlr *ctlr;
	Ether *e;

	e = v;
	ctlr = e->ctlr;
	for (;;) {
		sleep(&ctlr->trendez, tim, ctlr); /* xmit interrupt kicks us */
		ctlr->tim = 0;
		transmit(e);
	}
}

static void
rxinit(Ctlr *ctlr)
{
	int i, is598, autoc;
	ulong until;
	Block *b;

	ctlr->reg[Rxctl] &= ~Rxen;
	ctlr->reg[Rxdctl] = 0;
	for(i = 0; i < ctlr->nrd; i++){
		b = ctlr->rb[i];
		ctlr->rb[i] = 0;
		if(b)
			freeb(b);
	}
	ctlr->rdfree = 0;

	coherence();
	ctlr->reg[Fctrl] |= Bam;
	ctlr->reg[Fctrl] &= ~(Upe | Mpe);

	/* intel gets some csums wrong (e.g., errata 44) */
	ctlr->reg[Rxcsum] &= ~Ippcse;
	ctlr->reg[Hlreg0] &= ~Jumboen;		/* jumbos are a bad idea */
	ctlr->reg[Hlreg0] |= Txcrcen | Rxcrcstrip | Txpaden;
	ctlr->reg[Srrctl] = (ctlr->rbsz + 1024 - 1) / 1024;
	ctlr->reg[Mhadd] = ctlr->rbsz << 16;

	ctlr->reg[Rbal] = PCIWADDR(ctlr->rdba);
	ctlr->reg[Rbah] = 0;
	ctlr->reg[Rdlen] = ctlr->nrd*sizeof(Rd); /* must be multiple of 128 */
	ctlr->reg[Rdh] = 0;
	ctlr->reg[Rdt] = ctlr->rdt = 0;
	coherence();

	is598 = (ctlr->type == I82598);
	if (is598)
		ctlr->reg[Rdrxctl] = Rdmt¼;
	else {
		ctlr->reg[Rdrxctl] |= Crcstrip;
		ctlr->reg[Rdrxctl] &= ~Rscfrstsize;
	}
	if (Goslow && is598)
		ctlr->reg[Rxdctl] = 8<<Wthresh | 8<<Pthresh | 4<<Hthresh | Renable;
	else
		ctlr->reg[Rxdctl] = Renable;
	coherence();

	/*
	 * don't wait forever like an idiot (and hang the system),
	 * maybe it's disconnected.
	 */
	until = TK2MS(MACHP(0)->ticks) + 250;
	while (!(ctlr->reg[Rxdctl] & Renable) && TK2MS(MACHP(0)->ticks) < until)
		;
	if(!(ctlr->reg[Rxdctl] & Renable))
		print("#l%d: Renable didn't come on, might be disconnected\n",
			ctlr->edev->ctlrno);

	ctlr->reg[Rxctl] |= Rxen | (is598? Dmbyps: 0);

	if (is598){
		autoc = ctlr->reg[Autoc];
		/* what is this rubbish and why do we care? */
		print("#l%d: autoc %#ux; lms %d (3 is 10g sfp)\n",
			ctlr->edev->ctlrno, autoc, (autoc>>Lmsshift) & Lmsmask);
		ctlr->reg[Autoc] |= Flu;
		coherence();
		delay(50);
	}
}

static void
replenish(Ctlr *ctlr, uint rdh)
{
	int rdt, m, i;
	Block *b;
	Rd *r;

	m = ctlr->nrd - 1;
	i = 0;
	for(rdt = ctlr->rdt; NEXTPOW2(rdt, m) != rdh; rdt = NEXTPOW2(rdt, m)){
		r = ctlr->rdba + rdt;
		if((b = rballoc()) == nil){
			print("#l%d: no buffers\n", ctlr->edev->ctlrno);
			break;
		}
		ctlr->rb[rdt] = b;
		r->addr[0] = PCIWADDR(b->rp);
		r->status = 0;
		ctlr->rdfree++;
		i++;
	}
	if(i) {
		coherence();
		ctlr->reg[Rdt] = ctlr->rdt = rdt; /* hand back recycled rdescs */
		coherence();
	}
}

static int
rim(void *v)
{
	return ((Ctlr*)v)->rim != 0;
}

void
rproc(void *v)
{
	int passed;
	uint m, rdh;
	Block *bp;
	Ctlr *ctlr;
	Ether *e;
	Rd *r;

	e = v;
	ctlr = e->ctlr;
	m = ctlr->nrd - 1;
	for (rdh = 0; ; ) {
		replenish(ctlr, rdh);
		ienable(ctlr, Irx0);
		sleep(&ctlr->rrendez, rim, ctlr);
		passed = 0;
		for (;;) {
			ctlr->rim = 0;
			r = ctlr->rdba + rdh;
			if(!(r->status & Rdd))
				break;		/* wait for pkts to arrive */
			bp = ctlr->rb[rdh];
			ctlr->rb[rdh] = 0;
			if (r->length > ETHERMAXTU)
				print("#l%d: got jumbo of %d bytes\n",
					e->ctlrno, r->length);
			bp->wp += r->length;
			bp->lim = bp->wp;		/* lie like a dog */
//			r->status = 0;

			ilock(&rblock);
			nrbfull++;
			iunlock(&rblock);
			notemark(&ctlr->wmrb, nrbfull);
			etheriq(e, bp, 1);

			passed++;
			ctlr->rdfree--;
			rdh = NEXTPOW2(rdh, m);
			if (ctlr->rdfree <= ctlr->nrd - 16)
				replenish(ctlr, rdh);
		}
		/* note how many rds had full buffers */
		notemark(&ctlr->wmrd, passed);
	}
}

static void
promiscuous(void *a, int on)
{
	Ctlr *ctlr;
	Ether *e;

	e = a;
	ctlr = e->ctlr;
	if(on)
		ctlr->reg[Fctrl] |= Upe | Mpe;
	else
		ctlr->reg[Fctrl] &= ~(Upe | Mpe);
}

static void
multicast(void *a, uchar *ea, int on)
{
	int b, i;
	Ctlr *ctlr;
	Ether *e;

	e = a;
	ctlr = e->ctlr;

	/*
	 * multiple ether addresses can hash to the same filter bit,
	 * so it's never safe to clear a filter bit.
	 * if we want to clear filter bits, we need to keep track of
	 * all the multicast addresses in use, clear all the filter bits,
	 * then set the ones corresponding to in-use addresses.
	 */
	i = ea[5] >> 1;
	b = (ea[5]&1)<<4 | ea[4]>>4;
	b = 1 << b;
	if(on)
		ctlr->mta[i] |= b;
//	else
//		ctlr->mta[i] &= ~b;
	ctlr->reg[Mta+i] = ctlr->mta[i];
}

static void
freemem(Ctlr *ctlr)
{
	Block *b;

	while(b = rballoc()){
		b->free = 0;
		freeb(b);
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

	ctlr->reg[Imc] = ~0;
	ctlr->reg[Ctrl] |= Rst;
	for(i = 0; i < 100; i++){
		delay(1);
		if((ctlr->reg[Ctrl] & Rst) == 0)
			break;
	}
	if (i >= 100)
		return -1;
	is598 = (ctlr->type == I82598);
	if (is598) {			/* errata */
		delay(50);
		ctlr->reg[Ecc] &= ~(1<<21 | 1<<18 | 1<<9 | 1<<6);
	}

	/* not cleared by reset; kill it manually. */
	for(i = 1; i < 16; i++)
		ctlr->reg[is598? Rah98: Rah99] &= ~Enable;
	for(i = 0; i < 128; i++)
		ctlr->reg[Mta + i] = 0;
	for(i = 1; i < (is598? 640: 128); i++)
		ctlr->reg[Vfta + i] = 0;

//	freemem(ctlr);			// TODO
	ctlr->attached = 0;
	return 0;
}

static void
shutdown(Ether *e)
{
	detach(e->ctlr);
//	freemem(e->ctlr);
}

/* ≤ 20ms */
static ushort
eeread(Ctlr *ctlr, int i)
{
	ctlr->reg[Eerd] = EEstart | i<<2;
	while((ctlr->reg[Eerd] & EEdone) == 0)
		;
	return ctlr->reg[Eerd] >> 16;
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
	if(ctlr->reg[Status] & (1<<3))
		u = eeread(ctlr, 10);
	else
		u = eeread(ctlr, 9);
	u++;
	for(i = 0; i < Eaddrlen;){
		v = eeread(ctlr, u + i/2);
		ctlr->ra[i++] = v;
		ctlr->ra[i++] = v>>8;
	}
	ctlr->ra[5] += (ctlr->reg[Status] & 0xc) >> 2;
	return 0;
}

static int
reset(Ctlr *ctlr)
{
	int i, is598;
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
	is598 = (ctlr->type == I82598);
	ctlr->reg[is598? Ral98: Ral99] = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
	ctlr->reg[is598? Rah98: Rah99] = p[5]<<8 | p[4] | Enable;

	readstats(ctlr);
	for(i = 0; i<nelem(ctlr->stats); i++)
		ctlr->stats[i] = 0;

	ctlr->reg[Ctrlext] |= 1 << 16;	/* required by errata (spec change 4) */
	if (Goslow) {
		/* make some guesses for flow control */
		ctlr->reg[Fcrtl] = 0x10000 | Enable;
		ctlr->reg[Fcrth] = 0x40000 | Enable;
		ctlr->reg[Rcrtv] = 0x6000;
	} else
		ctlr->reg[Fcrtl] = ctlr->reg[Fcrth] = ctlr->reg[Rcrtv] = 0;

	/* configure interrupt mapping (don't ask) */
	ctlr->reg[Ivar+0] =     0 | 1<<7;
	ctlr->reg[Ivar+64/4] =  1 | 1<<7;
//	ctlr->reg[Ivar+97/4] = (2 | 1<<7) << (8*(97%4));

	if (Goslow) {
		/* interrupt throttling goes here. */
		for(i = Itr; i < Itr + 20; i++)
			ctlr->reg[i] = 128;		/* ¼µs intervals */
		ctlr->reg[Itr + Itx0] = 256;
	} else {					/* don't throttle */
		for(i = Itr; i < Itr + 20; i++)
			ctlr->reg[i] = 0;		/* ¼µs intervals */
		ctlr->reg[Itr + Itx0] = 0;
	}
	return 0;
}

static void
txinit(Ctlr *ctlr)
{
	Block *b;
	int i;

	if (Goslow)
		ctlr->reg[Txdctl] = 16<<Wthresh | 16<<Pthresh;
	else
		ctlr->reg[Txdctl] = 0;
	if (ctlr->type == I82599)
		ctlr->reg[Dtxctl99] = 0;
	coherence();
	for(i = 0; i < ctlr->ntd; i++){
		b = ctlr->tb[i];
		ctlr->tb[i] = 0;
		if(b)
			freeb(b);
	}

	assert(ctlr->tdba != nil);
	memset(ctlr->tdba, 0, ctlr->ntd * sizeof(Td));
	ctlr->reg[Tdbal] = PCIWADDR(ctlr->tdba);
	ctlr->reg[Tdbah] = 0;
	ctlr->reg[Tdlen] = ctlr->ntd*sizeof(Td); /* must be multiple of 128 */
	ctlr->reg[Tdh] = 0;
	ctlr->tdh = ctlr->ntd - 1;
	ctlr->reg[Tdt] = ctlr->tdt = 0;
	coherence();
	if (ctlr->type == I82599)
		ctlr->reg[Dtxctl99] |= Te;
	coherence();
	ctlr->reg[Txdctl] |= Ten;
	coherence();
	while (!(ctlr->reg[Txdctl] & Ten))
		;
}

static void
attach(Ether *e)
{
	Block *b;
	Ctlr *ctlr;
	char buf[KNAMELEN];

	ctlr = e->ctlr;
	ctlr->edev = e;			/* point back to Ether* */
	qlock(&ctlr->alock);
	if(waserror()){
		reset(ctlr);
		freemem(ctlr);
		qunlock(&ctlr->alock);
		nexterror();
	}
	if(ctlr->rdba == nil) {
		ctlr->nrd = Nrd;
		ctlr->ntd = Ntd;
		ctlr->rdba = mallocalign(ctlr->nrd * sizeof *ctlr->rdba,
			Descalign, 0, 0);
		ctlr->tdba = mallocalign(ctlr->ntd * sizeof *ctlr->tdba,
			Descalign, 0, 0);
		ctlr->rb = malloc(ctlr->nrd * sizeof(Block *));
		ctlr->tb = malloc(ctlr->ntd * sizeof(Block *));
		if (ctlr->rdba == nil || ctlr->tdba == nil ||
		    ctlr->rb == nil || ctlr->tb == nil)
			error(Enomem);

		for(ctlr->nrb = 0; ctlr->nrb < 2*Nrb; ctlr->nrb++){
			b = allocb(ctlr->rbsz + BY2PG);	/* see rbfree() */
			if(b == nil)
				error(Enomem);
			b->free = rbfree;
			freeb(b);
		}
	}
	if (!ctlr->attached) {
		rxinit(ctlr);
		txinit(ctlr);
		nrbfull = 0;
		if (!ctlr->procsrunning) {
			snprint(buf, sizeof buf, "#l%dl", e->ctlrno);
			kproc(buf, lproc, e);
			snprint(buf, sizeof buf, "#l%dr", e->ctlrno);
			kproc(buf, rproc, e);
			snprint(buf, sizeof buf, "#l%dt", e->ctlrno);
			kproc(buf, tproc, e);
			ctlr->procsrunning = 1;
		}
		initmark(&ctlr->wmrb, Nrb, "rcv bufs unprocessed");
		initmark(&ctlr->wmrd, Nrd-1, "rcv descrs processed at once");
		initmark(&ctlr->wmtd, Ntd-1, "xmit descr queue len");
		ctlr->attached = 1;
	}
	qunlock(&ctlr->alock);
	poperror();
}

static void
interrupt(Ureg*, void *v)
{
	int icr, im;
	Ctlr *ctlr;
	Ether *e;

	e = v;
	ctlr = e->ctlr;
	ilock(&ctlr->imlock);
	ctlr->reg[Imc] = ~0;			/* disable all intrs */
	im = ctlr->im;
	while((icr = ctlr->reg[Icr] & ctlr->im) != 0){
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
	ctlr->reg[Ims] = ctlr->im = im; /* enable only intrs we didn't service */
	iunlock(&ctlr->imlock);
}

static void
scan(void)
{
	int pciregs, pcimsix, type;
	ulong io, iomsi;
	void *mem, *memmsi;
	Ctlr *ctlr;
	Pcidev *p;

	p = 0;
	while(p = pcimatch(p, Vintel, 0)){
		switch(p->did){
		case 0x10b6:		/* 82598 backplane */
		case 0x10c6:		/* 82598 af dual port */
		case 0x10c7:		/* 82598 af single port */
		case 0x10dd:		/* 82598 at cx4 */
		case 0x10ec:		/* 82598 at cx4 dual port */
			pcimsix = 3;
			type = I82598;
			break;
		case 0x10f7:		/* 82599 kx/kx4 */
		case 0x10f8:		/* 82599 kx/kx4/kx */
		case 0x10f9:		/* 82599 cx4 */
		case 0x10fb:		/* 82599 sfi/sfp+ */
		case 0x10fc:		/* 82599 xaui/bx4 */
		case 0x1557:		/* 82599 single-port sfi */
			pcimsix = 4;
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

		iomsi = p->mem[pcimsix].bar & ~0xf;
		memmsi = vmap(iomsi, p->mem[pcimsix].size);
		if(memmsi == nil){
			print("i82598: can't map msi-x regs %#p\n",
				p->mem[pcimsix].bar);
			vunmap(mem, p->mem[pciregs].size);
			continue;
		}

		ctlr = malloc(sizeof *ctlr);
		if(ctlr == nil) {
			vunmap(mem, p->mem[pciregs].size);
			vunmap(memmsi, p->mem[pcimsix].size);
			error(Enomem);
		}
		ctlr->p = p;
		ctlr->type = type;
		ctlr->physreg = (u32int*)io;
		ctlr->physmsix = (u32int*)iomsi;
		ctlr->reg = (u32int*)mem;
		ctlr->msix = (u32int*)memmsi;	/* unused */
		ctlr->rbsz = Rbsz;
		if(reset(ctlr)){
			print("i82598: can't reset\n");
			free(ctlr);
			vunmap(mem, p->mem[pciregs].size);
			vunmap(memmsi, p->mem[pcimsix].size);
			continue;
		}
		pcisetbme(p);
		ctlrtab[nctlr++] = ctlr;
	}
}

static int
pnp(Ether *e)
{
	int i;
	Ctlr *ctlr;

	if(nctlr == 0)
		scan();
	ctlr = nil;
	for(i = 0; i < nctlr; i++){
		ctlr = ctlrtab[i];
		if(ctlr == nil || ctlr->flag & Factive)
			continue;
		if(e->port == 0 || e->port == (ulong)ctlr->reg)
			break;
	}
	if (i >= nctlr)
		return -1;
	ctlr->flag |= Factive;
	e->ctlr = ctlr;
	e->port = (uintptr)ctlr->physreg;
	e->irq = ctlr->p->intl;
	e->tbdf = ctlr->p->tbdf;
	e->mbps = 10000;
	e->maxmtu = ETHERMAXTU;
	memmove(e->ea, ctlr->ra, Eaddrlen);

	e->arg = e;
	e->attach = attach;
	e->detach = shutdown;
	e->transmit = transmit;
	e->interrupt = interrupt;
	e->ifstat = ifstat;
	e->shutdown = shutdown;
	e->ctl = ctl;
	e->multicast = multicast;
	e->promiscuous = promiscuous;

	return 0;
}

void
ether82598link(void)
{
	addethercard("i82598", pnp);
	addethercard("i10gbe", pnp);
}
