/*
  *	Dummy ethernet
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

enum {
	Rbsz	= ETHERMAXTU+32, /* +slop is for vlan headers, crcs, etc. */
	Descalign= 128,		/* 599 manual needs 128-byte alignment */

	/* tunable parameters */
	Nrd	= 4,
	Nrb	= 8,
	Ntd	= 4,
};

enum {
	Factive		= 1<<0,
};

struct Ctlr {
	Pcidev	*p;
	Ether	*edev;
	int	type;

	/* virtual */
	uintptr	*reg;

	uchar	flag;
	int	nrd;
	int	ntd;
	int	nrb;
	uint	rbsz;
	int	procsrunning;
	int	attached;

	Lock	slock;
	Lock	alock;			/* attach lock */
	QLock	tlock;

	uint	im;
	Lock	imlock;

	Block	**rb;
	uint	rdt;
	uint	rdfree;

	uint	tdh;
	uint	tdt;
	Block	**tb;

	uchar	ra[Eaddrlen];
};

static	Ctlr	*ctlrtab[4];
static	int	nctlr;
static	Lock	rblock;
static	Block	*rbpool;

static long
ifstat(Ether *e, void *a, long n, ulong offset)
{
	char *s, *p, *q;
	Ctlr *c;

	c = e->ctlr;
	p = s = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	q = p + READSTR;

	p = seprint(p, q, "mtu: min:%d max:%d\n", e->minmtu, e->maxmtu);
	seprint(p, q, "rdfree %d\n", c->rdfree);
	n = readstr(offset, a, n, s);
	free(s);

	return n;
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
//		ainc(&bp->ref);		/* prevent bp from being freed */
	}
	iunlock(&rblock);
	return bp;
}

void
drbfree(Block *b)
{
	b->rp = b->wp = (uchar*)ROUNDUP((uintptr)b->base, 2*KB);
	b->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);
	ilock(&rblock);
	b->next = rbpool;
	rbpool = b;
	iunlock(&rblock);
}

void
dtransmit(Ether *e)
{
	Block *b;

	while((b = qget(e->oq)) != nil)
		freeb(b);
}

static void
rxinit(Ctlr *c)
{
	int i;
	Block *b;

	for(i = 0; i < c->nrd; i++){
		b = c->rb[i];
		c->rb[i] = 0;
		if(b)
			freeb(b);
	}
	c->rdfree = 0;
}

static void
promiscuous(void*, int)
{
}

static void
multicast(void *, uchar *, int)
{
}

static void
freemem(Ctlr *c)
{
	Block *b;

	while(b = rballoc()){
		b->free = 0;
		freeb(b);
	}
	free(c->rb);
	c->rb = nil;
	free(c->tb);
	c->tb = nil;
}

static int
detach(Ctlr *c)
{
	c->attached = 0;
	return 0;
}

static void
shutdown(Ether *e)
{
	detach(e->ctlr);
//	freemem(e->ctlr);
}


static int
reset(Ctlr *c)
{
	if(detach(c)){
		print("dummy: reset timeout\n");
		return -1;
	}
	return 0;
}

static void
txinit(Ctlr *c)
{
	Block *b;
	int i;

	for(i = 0; i < c->ntd; i++){
		b = c->tb[i];
		c->tb[i] = 0;
		if(b)
			freeb(b);
	}
}

static void
attach(Ether *e)
{
	Block *b;
	Ctlr *c;

	c = e->ctlr;
	c->edev = e;			/* point back to Ether* */
	lock(&c->alock);
	if(waserror()){
		unlock(&c->alock);
		freemem(c);
		nexterror();
	}
	if(c->rb == nil) {
		c->nrd = Nrd;
		c->ntd = Ntd;
		c->rb = malloc(c->nrd * sizeof(Block *));
		c->tb = malloc(c->ntd * sizeof(Block *));
		if (c->rb == nil || c->tb == nil)
			error(Enomem);

		for(c->nrb = 0; c->nrb < 2*Nrb; c->nrb++){
			b = allocb(c->rbsz + 2*KB);	/* see rbfree() */
			if(b == nil)
				error(Enomem);
			b->free = drbfree;
			freeb(b);
		}
	}
	if (!c->attached) {
		rxinit(c);
		txinit(c);
		c->attached = 1;
	}
	unlock(&c->alock);
	poperror();
}

static void
interrupt(Ureg*, void *)
{
}

static void
scan(void)
{
	Ctlr *c;
	int i;

	for(i = 0; i < 2; i++){
		if(nctlr == nelem(ctlrtab)){
			print("dummy: too many controllers\n");
			return;
		}

		c = malloc(sizeof *c);
		c->rbsz = Rbsz;
		if(reset(c)){
			print("dummy: can't reset\n");
			free(c);
			continue;
		}
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
		if(e->port == 0 || e->port == PTR2UINT(c->reg))
			break;
	}
	if (i >= nctlr)
		return -1;

	c->flag |= Factive;
	e->ctlr = c;
	e->irq = -1;
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
	e->transmit = dtransmit;

	return 0;
}

void
etherdummylink(void)
{
	addethercard("dummy", pnp);
}
