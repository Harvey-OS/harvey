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
	Nrd	= 256,		/* multiple of 8, power of 2 for NEXTPOW2 */
	Nrb	= 1024,
	Ntd	= 128,		/* multiple of 8, power of 2 for NEXTPOW2 */
	Goslow	= 0,		/* flag: go slow by throttling intrs, etc. */
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

	Lock	slock;
	Lock	alock;			/* attach lock */
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

static void
readstats(Ctlr *c)
{
	int i;

	lock(&c->slock);
	for(i = 0; i < nelem(c->stats); i++)
		c->stats[i] += c->reg[stattab[i].reg >> 2];
	unlock(&c->slock);
}

static int speedtab[] = {
	0,
	1000,
	10000,
};

static long
ifstat(Ether *e, void *a, long n, ulong offset)
{
	uint i, *t;
	char *s, *p, *q;
	Ctlr *c;

	c = e->ctlr;
	p = s = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	q = p + READSTR;

	readstats(c);
	for(i = 0; i < nelem(stattab); i++)
		if(c->stats[i] > 0)
			p = seprint(p, q, "%.10s  %uld\n", stattab[i].name,
				c->stats[i]);
	t = c->speeds;
	p = seprint(p, q, "speeds: 0:%d 1000:%d 10000:%d\n", t[0], t[1], t[2]);
	p = seprint(p, q, "mtu: min:%d max:%d\n", e->minmtu, e->maxmtu);
	seprint(p, q, "rdfree %d rdh %d rdt %d\n", c->rdfree, c->reg[Rdt],
		c->reg[Rdh]);
	n = readstr(offset, a, n, s);
	free(s);

	return n;
}

static void
ienable(Ctlr *c, int i)
{
	ilock(&c->imlock);
	c->im |= i;
	c->reg[Ims] = c->im;
	iunlock(&c->imlock);
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
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	for (;;) {
		r = c->reg[Links];
		e->link = (r & Lnkup) != 0;
		i = 0;
		if(e->link)
			i = 1 + ((r & Lnkspd) != 0);
		c->speeds[i]++;
		e->mbps = speedtab[i];
		c->lim = 0;
		ienable(c, Lsc);
		sleep(&c->lrendez, lim, c);
		c->lim = 0;
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
	iunlock(&rblock);
}

static int
cleanup(Ctlr *c, int tdh)
{
	Block *b;
	uint m, n;

	m = c->ntd - 1;
	while(c->tdba[n = NEXTPOW2(tdh, m)].status & Tdd){
		tdh = n;
		b = c->tb[tdh];
		c->tb[tdh] = 0;
		if (b)
			freeb(b);
		c->tdba[tdh].status = 0;
	}
	return tdh;
}

void
transmit(Ether *e)
{
	uint i, m, tdt, tdh;
	Ctlr *c;
	Block *b;
	Td *t;

	c = e->ctlr;
	if(!canqlock(&c->tlock)){
		ienable(c, Itx0);
		return;
	}
	tdh = c->tdh = cleanup(c, c->tdh);
	tdt = c->tdt;
	m = c->ntd - 1;
	for(i = 0; ; i++){
		if(NEXTPOW2(tdt, m) == tdh){	/* ring full? */
			ienable(c, Itx0);
			break;
		}
		if((b = qget(e->oq)) == nil)
			break;
		assert(c->tdba != nil);
		t = c->tdba + tdt;
		t->addr[0] = PCIWADDR(b->rp);
		t->length = BLEN(b);
		t->cmd = Ifcs | Teop;
		if (!Goslow)
			t->cmd |= Rs;
		c->tb[tdt] = b;
		tdt = NEXTPOW2(tdt, m);
	}
	if(i) {
		coherence();
		c->reg[Tdt] = c->tdt = tdt;	/* make new Tds active */
		coherence();
		ienable(c, Itx0);
	}
	qunlock(&c->tlock);
}

static int
tim(void *c)
{
	return ((Ctlr*)c)->tim != 0;
}

static void
tproc(void *v)
{
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	for (;;) {
		sleep(&c->trendez, tim, c);  /* transmit interrupt kicks us */
		c->tim = 0;
		transmit(e);
	}
}

static void
rxinit(Ctlr *c)
{
	int i, is598;
	Block *b;

	c->reg[Rxctl] &= ~Rxen;
	c->reg[Rxdctl] = 0;
	for(i = 0; i < c->nrd; i++){
		b = c->rb[i];
		c->rb[i] = 0;
		if(b)
			freeb(b);
	}
	c->rdfree = 0;

	coherence();
	c->reg[Fctrl] |= Bam;
	c->reg[Fctrl] &= ~(Upe | Mpe);

	/* intel gets some csums wrong (e.g., errata 44) */
	c->reg[Rxcsum] &= ~Ippcse;
	c->reg[Hlreg0] &= ~Jumboen;		/* jumbos are a bad idea */
	c->reg[Hlreg0] |= Txcrcen | Rxcrcstrip | Txpaden;
	c->reg[Srrctl] = (c->rbsz + 1024 - 1) / 1024;
	c->reg[Mhadd] = c->rbsz << 16;

	c->reg[Rbal] = PCIWADDR(c->rdba);
	c->reg[Rbah] = 0;
	c->reg[Rdlen] = c->nrd*sizeof(Rd);	/* must be multiple of 128 */
	c->reg[Rdh] = 0;
	c->reg[Rdt] = c->rdt = 0;
	coherence();

	is598 = (c->type == I82598);
	if (is598)
		c->reg[Rdrxctl] = Rdmt¼;
	else {
		c->reg[Rdrxctl] |= Crcstrip;
		c->reg[Rdrxctl] &= ~Rscfrstsize;
	}
	if (Goslow && is598)
		c->reg[Rxdctl] = 8<<Wthresh | 8<<Pthresh | 4<<Hthresh | Renable;
	else
		c->reg[Rxdctl] = Renable;
	coherence();
	while (!(c->reg[Rxdctl] & Renable))
		;
	c->reg[Rxctl] |= Rxen | (c->type == I82598? Dmbyps: 0);
}

static void
replenish(Ctlr *c, uint rdh)
{
	int rdt, m, i;
	Block *b;
	Rd *r;

	m = c->nrd - 1;
	i = 0;
	for(rdt = c->rdt; NEXTPOW2(rdt, m) != rdh; rdt = NEXTPOW2(rdt, m)){
		r = c->rdba + rdt;
		if((b = rballoc()) == nil){
			print("82598: no buffers\n");
			break;
		}
		c->rb[rdt] = b;
		r->addr[0] = PCIWADDR(b->rp);
		r->status = 0;
		c->rdfree++;
		i++;
	}
	if(i) {
		coherence();
		c->reg[Rdt] = c->rdt = rdt;	/* hand back recycled rdescs */
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
	uint m, rdh;
	Block *b;
	Ctlr *c;
	Ether *e;
	Rd *r;

	e = v;
	c = e->ctlr;
	m = c->nrd - 1;
	for (rdh = 0; ; ) {
		replenish(c, rdh);
		ienable(c, Irx0);
		sleep(&c->rrendez, rim, c);
		for (;;) {
			c->rim = 0;
			r = c->rdba + rdh;
			if(!(r->status & Rdd))
				break;		/* wait for pkts to arrive */
			b = c->rb[rdh];
			c->rb[rdh] = 0;
			if (r->length > ETHERMAXTU)
				print("82598: got jumbo of %d bytes\n", r->length);
			b->wp += r->length;
			b->lim = b->wp;			/* lie like a dog */
//			r->status = 0;
			etheriq(e, b, 1);
			c->rdfree--;
			rdh = NEXTPOW2(rdh, m);
			if (c->rdfree <= c->nrd - 16)
				replenish(c, rdh);
		}
	}
}

static void
promiscuous(void *a, int on)
{
	Ctlr *c;
	Ether *e;

	e = a;
	c = e->ctlr;
	if(on)
		c->reg[Fctrl] |= Upe | Mpe;
	else
		c->reg[Fctrl] &= ~(Upe | Mpe);
}

static void
multicast(void *a, uchar *ea, int on)
{
	int b, i;
	Ctlr *c;
	Ether *e;

	e = a;
	c = e->ctlr;

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
		c->mta[i] |= b;
//	else
//		c->mta[i] &= ~b;
	c->reg[Mta+i] = c->mta[i];
}

static void
freemem(Ctlr *c)
{
	Block *b;

	while(b = rballoc()){
		b->free = 0;
		freeb(b);
	}
	free(c->rdba);
	c->rdba = nil;
	free(c->tdba);
	c->tdba = nil;
	free(c->rb);
	c->rb = nil;
	free(c->tb);
	c->tb = nil;
}

static int
detach(Ctlr *c)
{
	int i, is598;

	c->reg[Imc] = ~0;
	c->reg[Ctrl] |= Rst;
	for(i = 0; i < 100; i++){
		delay(1);
		if((c->reg[Ctrl] & Rst) == 0)
			break;
	}
	if (i >= 100)
		return -1;
	is598 = (c->type == I82598);
	if (is598) {			/* errata */
		delay(50);
		c->reg[Ecc] &= ~(1<<21 | 1<<18 | 1<<9 | 1<<6);
	}

	/* not cleared by reset; kill it manually. */
	for(i = 1; i < 16; i++)
		c->reg[is598? Rah98: Rah99] &= ~Enable;
	for(i = 0; i < 128; i++)
		c->reg[Mta + i] = 0;
	for(i = 1; i < (is598? 640: 128); i++)
		c->reg[Vfta + i] = 0;

//	freemem(c);			// TODO
	c->attached = 0;
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
eeread(Ctlr *c, int i)
{
	c->reg[Eerd] = EEstart | i<<2;
	while((c->reg[Eerd] & EEdone) == 0)
		;
	return c->reg[Eerd] >> 16;
}

static int
eeload(Ctlr *c)
{
	ushort u, v, p, l, i, j;

	if((eeread(c, 0) & 0xc0) != 0x40)
		return -1;
	u = 0;
	for(i = 0; i < 0x40; i++)
		u +=  eeread(c, i);
	for(i = 3; i < 0xf; i++){
		p = eeread(c, i);
		l = eeread(c, p++);
		if((int)p + l + 1 > 0xffff)
			continue;
		for(j = p; j < p + l; j++)
			u += eeread(c, j);
	}
	if(u != 0xbaba)
		return -1;
	if(c->reg[Status] & (1<<3))
		u = eeread(c, 10);
	else
		u = eeread(c, 9);
	u++;
	for(i = 0; i < Eaddrlen;){
		v = eeread(c, u + i/2);
		c->ra[i++] = v;
		c->ra[i++] = v>>8;
	}
	c->ra[5] += (c->reg[Status] & 0xc) >> 2;
	return 0;
}

static int
reset(Ctlr *c)
{
	int i, is598;
	uchar *p;

	if(detach(c)){
		print("82598: reset timeout\n");
		return -1;
	}
	if(eeload(c)){
		print("82598: eeprom failure\n");
		return -1;
	}
	p = c->ra;
	is598 = (c->type == I82598);
	c->reg[is598? Ral98: Ral99] = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
	c->reg[is598? Rah98: Rah99] = p[5]<<8 | p[4] | Enable;

	readstats(c);
	for(i = 0; i<nelem(c->stats); i++)
		c->stats[i] = 0;

	c->reg[Ctrlext] |= 1 << 16;	/* required by errata (spec change 4) */
	if (Goslow) {
		/* make some guesses for flow control */
		c->reg[Fcrtl] = 0x10000 | Enable;
		c->reg[Fcrth] = 0x40000 | Enable;
		c->reg[Rcrtv] = 0x6000;
	} else
		c->reg[Fcrtl] = c->reg[Fcrth] = c->reg[Rcrtv] = 0;

	/* configure interrupt mapping (don't ask) */
	c->reg[Ivar+0] =     0 | 1<<7;
	c->reg[Ivar+64/4] =  1 | 1<<7;
//	c->reg[Ivar+97/4] = (2 | 1<<7) << (8*(97%4));

	if (Goslow) {
		/* interrupt throttling goes here. */
		for(i = Itr; i < Itr + 20; i++)
			c->reg[i] = 128;		/* ¼µs intervals */
		c->reg[Itr + Itx0] = 256;
	} else {					/* don't throttle */
		for(i = Itr; i < Itr + 20; i++)
			c->reg[i] = 0;			/* ¼µs intervals */
		c->reg[Itr + Itx0] = 0;
	}
	return 0;
}

static void
txinit(Ctlr *c)
{
	Block *b;
	int i;

	if (Goslow)
		c->reg[Txdctl] = 16<<Wthresh | 16<<Pthresh;
	else
		c->reg[Txdctl] = 0;
	if (c->type == I82599)
		c->reg[Dtxctl99] = 0;
	coherence();
	for(i = 0; i < c->ntd; i++){
		b = c->tb[i];
		c->tb[i] = 0;
		if(b)
			freeb(b);
	}

	assert(c->tdba != nil);
	memset(c->tdba, 0, c->ntd * sizeof(Td));
	c->reg[Tdbal] = PCIWADDR(c->tdba);
	c->reg[Tdbah] = 0;
	c->reg[Tdlen] = c->ntd*sizeof(Td);	/* must be multiple of 128 */
	c->reg[Tdh] = 0;
	c->tdh = c->ntd - 1;
	c->reg[Tdt] = c->tdt = 0;
	coherence();
	if (c->type == I82599)
		c->reg[Dtxctl99] |= Te;
	coherence();
	c->reg[Txdctl] |= Ten;
	coherence();
	while (!(c->reg[Txdctl] & Ten))
		;
}

static void
attach(Ether *e)
{
	Block *b;
	Ctlr *c;
	char buf[KNAMELEN];

	c = e->ctlr;
	c->edev = e;			/* point back to Ether* */
	lock(&c->alock);
	if(waserror()){
		unlock(&c->alock);
		freemem(c);
		nexterror();
	}
	if(c->rdba == nil) {
		c->nrd = Nrd;
		c->ntd = Ntd;
		c->rdba = mallocalign(c->nrd * sizeof *c->rdba, Descalign, 0, 0);
		c->tdba = mallocalign(c->ntd * sizeof *c->tdba, Descalign, 0, 0);
		c->rb = malloc(c->nrd * sizeof(Block *));
		c->tb = malloc(c->ntd * sizeof(Block *));
		if (c->rdba == nil || c->tdba == nil ||
		    c->rb == nil || c->tb == nil)
			error(Enomem);

		for(c->nrb = 0; c->nrb < 2*Nrb; c->nrb++){
			b = allocb(c->rbsz + BY2PG);	/* see rbfree() */
			if(b == nil)
				error(Enomem);
			b->free = rbfree;
			freeb(b);
		}
	}
	if (!c->attached) {
		rxinit(c);
		txinit(c);
		if (!c->procsrunning) {
			snprint(buf, sizeof buf, "#l%dl", e->ctlrno);
			kproc(buf, lproc, e);
			snprint(buf, sizeof buf, "#l%dr", e->ctlrno);
			kproc(buf, rproc, e);
			snprint(buf, sizeof buf, "#l%dt", e->ctlrno);
			kproc(buf, tproc, e);
			c->procsrunning = 1;
		}
		c->attached = 1;
	}
	unlock(&c->alock);
	poperror();
}

static void
interrupt(Ureg*, void *v)
{
	int icr, im;
	Ctlr *c;
	Ether *e;

	e = v;
	c = e->ctlr;
	ilock(&c->imlock);
	c->reg[Imc] = ~0;			/* disable all intrs */
	im = c->im;
	while((icr = c->reg[Icr] & c->im) != 0){
		if(icr & Irx0){
			im &= ~Irx0;
			c->rim = Irx0;
			wakeup(&c->rrendez);
		}
		if(icr & Itx0){
			im &= ~Itx0;
			c->tim = Itx0;
			wakeup(&c->trendez);
		}
		if(icr & Lsc){
			im &= ~Lsc;
			c->lim = Lsc;
			wakeup(&c->lrendez);
		}
	}
	c->reg[Ims] = c->im = im;  /* enable only intrs we didn't service */
	iunlock(&c->imlock);
}

static void
scan(void)
{
	int pciregs, pcimsix, type;
	ulong io, iomsi;
	void *mem, *memmsi;
	Ctlr *c;
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
		if(nctlr == nelem(ctlrtab)){
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

		c = malloc(sizeof *c);
		if(c == nil) {
			vunmap(mem, p->mem[pciregs].size);
			vunmap(memmsi, p->mem[pcimsix].size);
			error(Enomem);
		}
		c->p = p;
		c->type = type;
		c->physreg = (u32int*)io;
		c->physmsix = (u32int*)iomsi;
		c->reg = (u32int*)mem;
		c->msix = (u32int*)memmsi;	/* unused */
		c->rbsz = Rbsz;
		if(reset(c)){
			print("i82598: can't reset\n");
			free(c);
			vunmap(mem, p->mem[pciregs].size);
			vunmap(memmsi, p->mem[pcimsix].size);
			continue;
		}
		pcisetbme(p);
		ctlrtab[nctlr++] = c;
	}
}

static int
pnp(Ether *e)
{
	int i;
	Ctlr *c = nil;

	if(nctlr == 0)
		scan();
	for(i = 0; i < nctlr; i++){
		c = ctlrtab[i];
		if(c == nil || c->flag & Factive)
			continue;
		if(e->port == 0 || e->port == (ulong)c->reg)
			break;
	}
	if (i >= nctlr)
		return -1;
	c->flag |= Factive;
	e->ctlr = c;
	e->port = (uintptr)c->physreg;
	e->irq = c->p->intl;
	e->tbdf = c->p->tbdf;
	e->mbps = 10000;
	e->maxmtu = ETHERMAXTU;
	memmove(e->ea, c->ra, Eaddrlen);
	e->arg = e;
	e->attach = attach;
	e->ctl = ctl;
	e->ifstat = ifstat;
	e->interrupt = interrupt;
	e->multicast = multicast;
	e->promiscuous = promiscuous;
	e->shutdown = shutdown;
	e->transmit = transmit;

	return 0;
}

void
ether82598link(void)
{
	addethercard("i82598", pnp);
}
