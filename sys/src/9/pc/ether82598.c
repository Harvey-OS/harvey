/*
 * intel 10GB ethernet pci-express driver
 * copyright © 2007, coraid, inc.
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
 * // comments note conflicts with 82563-style drivers,
 * and the registers are all different.
 */

enum {
	/* general */
	Ctrl		= 0x00000/4,	/* Device Control */
	Status		= 0x00008/4,	/* Device Status */
	Ctrlext		= 0x00018/4,	/* Extended Device Control */
	Esdp		= 0x00020/4,	/* extended sdp control */
	Esodp		= 0x00028/4,	/* extended od sdp control */
	Ledctl		= 0x00200/4,	/* led control */
	Tcptimer	= 0x0004c/4,	/* tcp timer */
	Ecc		= 0x110b0/4,	/* errata ecc control magic */

	/* nvm */
	Eec		= 0x10010/4,	/* eeprom/flash control */
	Eerd		= 0x10014/4,	/* eeprom read */
	Fla		= 0x1001c/4,	/* flash access */
	Flop		= 0x1013c/4,	/* flash opcode */
	Grc		= 0x10200/4,	/* general rx control */

	/* interrupt */
	Icr		= 0x00800/4,	/* interrupt cause read */
	Ics		= 0x00808/4,	/* " set */
	Ims		= 0x00880/4,	/* " mask read/set */
	Imc		= 0x00888/4,	/* " mask clear */
	Iac		= 0x00810/4,	/* " ayto clear */
	Iam		= 0x00890/4,	/* " auto mask enable */
	Itr		= 0x00820/4,	/* " throttling rate (0-19) */
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

	Srrctl		= 0x02100/4,	/* split and replication rx ctl. */
	Dcarxctl	= 0x02200/4,	/* rx dca control */
	Rdrxctl		= 0x02f00/4,	/* rx dma control */
	Rxpbsize	= 0x03c00/4,	/* rx packet buffer size */
	Rxctl		= 0x03000/4,	/* rx control */
	Dropen		= 0x03d04/4,	/* drop enable control */

	/* rx */
	Rxcsum		= 0x05000/4,	/* rx checksum control */
	Rfctl		= 0x04008/4,	/* rx filter control */
	Mta		= 0x05200/4,	/* multicast table array (0-127) */
	Ral		= 0x05400/4,	/* rx address low */
	Rah		= 0x05404/4,
	Psrtype		= 0x05480/4,	/* packet split rx type. */
	Vfta		= 0x0a000/4,	/* vlan filter table array. */
	Fctrl		= 0x05080/4,	/* filter control */
	Vlnctrl		= 0x05088/4,	/* vlan control */
	Msctctrl	= 0x05090/4,	/* multicast control */
	Mrqc		= 0x05818/4,	/* multiple rx queues cmd */
	Vmdctl		= 0x0581c/4,	/* vmdq control */
	Imir		= 0x05a80/4,	/* immediate irq rx (0-7) */
	Imirext		= 0x05aa0/4,	/* immediate irq rx ext */
	Imirvp		= 0x05ac0/4,	/* immediate irq vlan priority */
	Reta		= 0x05c00/4,	/* redirection table */
	Rssrk		= 0x05c80/4,	/* rss random key */

	/* tx */
	Tdbal		= 0x06000/4,	/* tx desc base low +0x40n */
	Tdbah		= 0x06004/4,	/* " high */
	Tdlen		= 0x06008/4,	/* " len */
	Tdh		= 0x06010/4,	/* " head */
	Tdt		= 0x06018/4,	/* " tail */
	Txdctl		= 0x06028/4,	/* " control */
	Tdwbal		= 0x06038/4,	/* " write-back address low */
	Tdwbah		= 0x0603c/4,

	Dtxctl		= 0x07e00/4,	/* tx dma control */
	Tdcatxctrl	= 0x07200/4,	/* tx dca register (0-15) */
	Tipg		= 0x0cb00/4,	/* tx inter-packet gap */
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
	Autoc2		= 0x042a8/4,
};

enum {
	/* Ctrl */
	Rst		= 1<<26,	/* full nic reset */

	/* Txdctl */
	Ten		= 1<<25,

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
	Dmbyps		= 1<<1,

	/* Rdrxctl */
	Rdmt½		= 0,
	Rdmt¼		= 1,
	Rdmt⅛		= 2,

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
	Lnkup	= 1<<30,
	Lnkspd	= 1<<29,

	/* Hlreg0 */
	Jumboen	= 1<<2,
};

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
	Ipcs	= 1<<6,	/* ip checksum calcuated */
	L4cs	= 1<<5,	/* layer 2 */
	Tcpcs	= 1<<4,	/* tcp checksum calcuated */
	Vp	= 1<<3,	/* 802.1q packet matched vet */
	Ixsm	= 1<<2,	/* ignore checksum */
	Reop	= 1<<1,	/* end of packet */
	Rdd	= 1<<0,	/* descriptor done */
};

typedef struct {
	u32int	addr[2];
	ushort	length;
	ushort	cksum;
	uchar	status;
	uchar	errors;
	ushort	vlan;
} Rd;

enum {
	/* Td cmd */
	Rs	= 1<<3,
	Ic	= 1<<2,
	Ifcs	= 1<<1,
	Teop	= 1<<0,

	/* Td status */
	Tdd	= 1<<0,
};

typedef struct {
	u32int	addr[2];
	ushort	length;
	uchar	cso;
	uchar	cmd;
	uchar	status;
	uchar	css;
	ushort	vlan;
} Td;

enum {
	Factive		= 1<<0,
	Fstarted	= 1<<1,
};

typedef struct {
	Pcidev	*p;
	Ether	*edev;
	u32int	*reg;
	u32int	*reg3;
	uchar	flag;
	int	nrd;
	int	ntd;
	int	nrb;
	int	rbsz;
	Lock	slock;
	Lock	alock;
	QLock	tlock;
	Rendez	lrendez;
	Rendez	trendez;
	Rendez	rrendez;
	uint	im;
	uint	lim;
	uint	rim;
	uint	tim;
	Lock	imlock;
	char	*alloc;

	Rd	*rdba;
	Block	**rb;
	uint	rdt;
	uint	rdfree;

	Td	*tdba;
	uint	tdh;
	uint	tdt;
	Block	**tb;

	uchar	ra[Eaddrlen];
	uchar	mta[128];
	ulong	stats[nelem(stattab)];
	uint	speeds[3];
} Ctlr;

/* tweakable paramaters */
enum {
	Rbsz	= 12*1024,
	Nrd	= 256,
	Ntd	= 256,
	Nrb	= 256,
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
	q = p + READSTR;

	readstats(c);
	for(i = 0; i < nelem(stattab); i++)
		if(c->stats[i] > 0)
			p = seprint(p, q, "%.10s  %uld\n", stattab[i].name,					c->stats[i]);
	t = c->speeds;
	p = seprint(p, q, "speeds: 0:%d 1000:%d 10000:%d\n", t[0], t[1], t[2]);
	seprint(p, q, "rdfree %d rdh %d rdt %d\n", c->rdfree, c->reg[Rdt],
		c->reg[Rdh]);
	n = readstr(offset, a, n, s);
	free(s);

	return n;
}

static void
im(Ctlr *c, int i)
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
		im(c, Lsc);
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

#define Next(x, m)	(((x)+1) & (m))

static int
cleanup(Ctlr *c, int tdh)
{
	Block *b;
	uint m, n;

	m = c->ntd - 1;
	while(c->tdba[n = Next(tdh, m)].status & Tdd){
		tdh = n;
		b = c->tb[tdh];
		c->tb[tdh] = 0;
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
		im(c, Itx0);
		return;
	}
	tdh = c->tdh = cleanup(c, c->tdh);
	tdt = c->tdt;
	m = c->ntd - 1;
	for(i = 0; i < 8; i++){
		if(Next(tdt, m) == tdh){
			im(c, Itx0);
			break;
		}
		if(!(b = qget(e->oq)))
			break;
		t = c->tdba + tdt;
		t->addr[0] = PCIWADDR(b->rp);
		t->length = BLEN(b);
		t->cmd = Rs | Ifcs | Teop;
		c->tb[tdt] = b;
		tdt = Next(tdt, m);
	}
	if(i){
		c->tdt = tdt;
		c->reg[Tdt] = tdt;
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
		sleep(&c->trendez, tim, c);	/* transmit kicks us */
		c->tim = 0;
		transmit(e);
	}
}

static void
rxinit(Ctlr *c)
{
	int i;
	Block *b;

	c->reg[Rxctl] &= ~Rxen;
	for(i = 0; i < c->nrd; i++){
		b = c->rb[i];
		c->rb[i] = 0;
		if(b)
			freeb(b);
	}
	c->rdfree = 0;

	c->reg[Fctrl] |= Bam;
	c->reg[Rxcsum] |= Ipcs;
	c->reg[Srrctl] = (c->rbsz + 1023)/1024;
	c->reg[Mhadd] = c->rbsz << 16;
	c->reg[Hlreg0] |= Jumboen;

	c->reg[Rbal] = PCIWADDR(c->rdba);
	c->reg[Rbah] = 0;
	c->reg[Rdlen] = c->nrd*sizeof(Rd);
	c->reg[Rdh] = 0;
	c->reg[Rdt] = c->rdt = 0;

	c->reg[Rdrxctl] = Rdmt¼;
	c->reg[Rxdctl] = 8<<Wthresh | 8<<Pthresh | 4<<Hthresh | Renable;
	c->reg[Rxctl] |= Rxen | Dmbyps;
}

static void
replenish(Ctlr *c, uint rdh)
{
	int rdt, m, i;
	Block *b;
	Rd *r;

	m = c->nrd - 1;
	i = 0;
	for(rdt = c->rdt; Next(rdt, m) != rdh; rdt = Next(rdt, m)){
		r = c->rdba + rdt;
		if(!(b = rballoc())){
			print("82598: no buffers\n");
			break;
		}
		c->rb[rdt] = b;
		r->addr[0] = PCIWADDR(b->rp);
		r->status = 0;
		c->rdfree++;
		i++;
	}
	if(i)
		c->reg[Rdt] = c->rdt = rdt;
}

static int
rim(void *v)
{
	return ((Ctlr*)v)->rim != 0;
}

static uchar zeroea[Eaddrlen];

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
	rdh = 0;
loop:
	replenish(c, rdh);
	im(c, Irx0);
	sleep(&c->rrendez, rim, c);
loop1:
	c->rim = 0;
	if(c->nrd - c->rdfree >= 16)
		replenish(c, rdh);
	r = c->rdba + rdh;
	if(!(r->status & Rdd))
		goto loop;		/* UGH */
	b = c->rb[rdh];
	c->rb[rdh] = 0;
	b->wp += r->length;
	b->lim = b->wp;		/* lie like a dog */
	if(!(r->status & Ixsm)){
		if(r->status & Ipcs)
			b->flag |= Bipck;
		if(r->status & Tcpcs)
			b->flag |= Btcpck | Budpck;
		b->checksum = r->cksum;
	}
//	r->status = 0;
	etheriq(e, b, 1);
	c->rdfree--;
	rdh = Next(rdh, m);
	goto loop1;			/* UGH */
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

static int
detach(Ctlr *c)
{
	int i;

	c->reg[Imc] = ~0;
	c->reg[Ctrl] |= Rst;
	for(i = 0; i < 100; i++){
		delay(1);
		if((c->reg[Ctrl] & Rst) == 0)
			break;
	}
	if (i >= 100)
		return -1;
	/* errata */
	delay(50);
	c->reg[Ecc] &= ~(1<<21 | 1<<18 | 1<<9 | 1<<6);

	/* not cleared by reset; kill it manually. */
	for(i = 1; i < 16; i++)
		c->reg[Rah] &= ~(1 << 31);
	for(i = 0; i < 128; i++)
		c->reg[Mta + i] = 0;
	for(i = 1; i < 640; i++)
		c->reg[Vfta + i] = 0;
	return 0;
}

static void
shutdown(Ether *e)
{
	detach(e->ctlr);
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
	int i;
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
	c->reg[Ral] = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0];
	c->reg[Rah] = p[5]<<8 | p[4] | 1<<31;

	readstats(c);
	for(i = 0; i<nelem(c->stats); i++)
		c->stats[i] = 0;

	c->reg[Ctrlext] |= 1 << 16;
	/* make some guesses for flow control */
	c->reg[Fcrtl] = 0x10000 | 1<<31;
	c->reg[Fcrth] = 0x40000 | 1<<31;
	c->reg[Rcrtv] = 0x6000;

	/* configure interrupt mapping (don't ask) */
	c->reg[Ivar+0] =     0 | 1<<7;
	c->reg[Ivar+64/4] =  1 | 1<<7;
//	c->reg[Ivar+97/4] = (2 | 1<<7) << (8*(97%4));

	/* interrupt throttling goes here. */
	for(i = Itr; i < Itr + 20; i++)
		c->reg[i] = 128;		/* ¼µs intervals */
	c->reg[Itr + Itx0] = 256;
	return 0;
}

static void
txinit(Ctlr *c)
{
	Block *b;
	int i;

	c->reg[Txdctl] = 16<<Wthresh | 16<<Pthresh;
	for(i = 0; i < c->ntd; i++){
		b = c->tb[i];
		c->tb[i] = 0;
		if(b)
			freeb(b);
	}
	memset(c->tdba, 0, c->ntd * sizeof(Td));
	c->reg[Tdbal] = PCIWADDR(c->tdba);
	c->reg[Tdbah] = 0;
	c->reg[Tdlen] = c->ntd*sizeof(Td);
	c->reg[Tdh] = 0;
	c->reg[Tdt] = 0;
	c->tdh = c->ntd - 1;
	c->tdt = 0;
	c->reg[Txdctl] |= Ten;
}

static void
attach(Ether *e)
{
	Block *b;
	Ctlr *c;
	int t;
	char buf[KNAMELEN];

	c = e->ctlr;
	c->edev = e;			/* point back to Ether* */
	lock(&c->alock);
	if(c->alloc){
		unlock(&c->alock);
		return;
	}

	c->nrd = Nrd;
	c->ntd = Ntd;
	t  = c->nrd * sizeof *c->rdba + 255;
	t += c->ntd * sizeof *c->tdba + 255;
	t += (c->ntd + c->nrd) * sizeof(Block*);
	c->alloc = malloc(t);
	unlock(&c->alock);
	if(c->alloc == nil)
		error(Enomem);

	c->rdba = (Rd*)ROUNDUP((uintptr)c->alloc, 256);
	c->tdba = (Td*)ROUNDUP((uintptr)(c->rdba + c->nrd), 256);
	c->rb = (Block**)(c->tdba + c->ntd);
	c->tb = (Block**)(c->rb + c->nrd);

	if(waserror()){
		while(b = rballoc()){
			b->free = 0;
			freeb(b);
		}
		free(c->alloc);
		c->alloc = nil;
		nexterror();
	}
	for(c->nrb = 0; c->nrb < 2*Nrb; c->nrb++){
		if(!(b = allocb(c->rbsz+BY2PG)))
			error(Enomem);
		b->free = rbfree;
		freeb(b);
	}
	poperror();

	rxinit(c);
	txinit(c);

	snprint(buf, sizeof buf, "#l%dl", e->ctlrno);
	kproc(buf, lproc, e);
	snprint(buf, sizeof buf, "#l%dr", e->ctlrno);
	kproc(buf, rproc, e);
	snprint(buf, sizeof buf, "#l%dt", e->ctlrno);
	kproc(buf, tproc, e);
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
	c->reg[Imc] = ~0;
	im = c->im;
	while((icr = c->reg[Icr] & c->im) != 0){
		if(icr & Lsc){
			im &= ~Lsc;
			c->lim = icr & Lsc;
			wakeup(&c->lrendez);
		}
		if(icr & Irx0){
			im &= ~Irx0;
			c->rim = icr & Irx0;
			wakeup(&c->rrendez);
		}
		if(icr & Itx0){
			im &= ~Itx0;
			c->tim = icr & Itx0;
			wakeup(&c->trendez);
		}
	}
	c->reg[Ims] = c->im = im;
	iunlock(&c->imlock);
}

static void
scan(void)
{
	ulong io, io3;
	void *mem, *mem3;
	Ctlr *c;
	Pcidev *p;

	p = 0;
	while(p = pcimatch(p, 0x8086, 0)){
		switch(p->did){
		case 0x10c6:		/* 82598 af dual port */
		case 0x10c7:		/* 82598 af single port */
		case 0x10b6:		/* 82598 backplane */
		case 0x10dd:		/* 82598 at cx4 */
		case 0x10ec:		/* 82598 at cx4 dual port */
			break;
		default:
			continue;
		}
		if(nctlr == nelem(ctlrtab)){
			print("i82598: too many controllers\n");
			return;
		}
		io = p->mem[0].bar & ~0xf;
		mem = vmap(io, p->mem[0].size);
		if(mem == nil){
			print("i82598: can't map %#p\n", p->mem[0].bar);
			continue;
		}
		io3 = p->mem[3].bar & ~0xf;
		mem3 = vmap(io3, p->mem[3].size);
		if(mem3 == nil){
			print("i82598: can't map %#p\n", p->mem[3].bar);
			vunmap(mem, p->mem[0].size);
			continue;
		}
		c = malloc(sizeof *c);
		c->p = p;
		c->reg = (u32int*)mem;
		c->reg3 = (u32int*)mem3;
		c->rbsz = Rbsz;
		if(reset(c)){
			print("i82598: can't reset\n");
			free(c);
			vunmap(mem, p->mem[0].size);
			vunmap(mem3, p->mem[3].size);
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
	e->port = (uintptr)c->reg;
	e->irq = c->p->intl;
	e->tbdf = c->p->tbdf;
	e->mbps = 10000;
	e->maxmtu = c->rbsz;
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
