#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "pool.h"
#include "ureg.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"

static Ether *etherxx[MaxEther];

Chan*
etherattach(char* spec)
{
	ulong ctlrno;
	char *p;
	Chan *chan;

	ctlrno = 0;
	if(spec && *spec){
		ctlrno = strtoul(spec, &p, 0);
		if((ctlrno == 0 && p == spec) || *p || (ctlrno >= MaxEther))
			error(Ebadarg);
	}
	if(etherxx[ctlrno] == 0)
		error(Enodev);

	chan = devattach('l', spec);
	if(waserror()){
		chanfree(chan);
		nexterror();
	}
	chan->dev = ctlrno;
	if(etherxx[ctlrno]->attach)
		etherxx[ctlrno]->attach(etherxx[ctlrno]);
	poperror();
	return chan;
}

static Walkqid*
etherwalk(Chan* chan, Chan* nchan, char** name, int nname)
{
	return netifwalk(etherxx[chan->dev], chan, nchan, name, nname);
}

static int
etherstat(Chan* chan, uchar* dp, int n)
{
	return netifstat(etherxx[chan->dev], chan, dp, n);
}

static Chan*
etheropen(Chan* chan, int omode)
{
	return netifopen(etherxx[chan->dev], chan, omode);
}

static void
ethercreate(Chan*, char*, int, ulong)
{
}

static void
etherclose(Chan* chan)
{
	netifclose(etherxx[chan->dev], chan);
}

static long
etherread(Chan* chan, void* buf, long n, vlong off)
{
	Ether *ether;
	ulong offset = off;

	ether = etherxx[chan->dev];
	if((chan->qid.type & QTDIR) == 0 && ether->ifstat){
		/*
		 * With some controllers it is necessary to reach
		 * into the chip to extract statistics.
		 */
		if(NETTYPE(chan->qid.path) == Nifstatqid)
			return ether->ifstat(ether, buf, n, offset);
		else if(NETTYPE(chan->qid.path) == Nstatqid)
			ether->ifstat(ether, buf, 0, offset);
	}

	return netifread(ether, chan, buf, n, offset);
}

static Block*
etherbread(Chan* chan, long n, ulong offset)
{
	return netifbread(etherxx[chan->dev], chan, n, offset);
}

static int
etherwstat(Chan* chan, uchar* dp, int n)
{
	return netifwstat(etherxx[chan->dev], chan, dp, n);
}

static void
etherrtrace(Netfile* f, Etherpkt* pkt, int len)
{
	int i, n;
	Block *bp;

	if(qwindow(f->in) <= 0)
		return;
	if(len > 58)
		n = 58;
	else
		n = len;
	bp = iallocb(64);
	if(bp == nil)
		return;
	memmove(bp->wp, pkt->d, n);
	i = TK2MS(sys->ticks);
	bp->wp[58] = len>>8;
	bp->wp[59] = len;
	bp->wp[60] = i>>24;
	bp->wp[61] = i>>16;
	bp->wp[62] = i>>8;
	bp->wp[63] = i;
	bp->wp += 64;
	qpass(f->in, bp);
}

static void
softovflow(Ether *ether, char *context)
{
	if (0)
		iprint("#l%d: etheriq: soverflow for %s\n", ether->ctlrno,
			context);
	ether->soverflows++;
}

Block*
etheriq(Ether* ether, Block* bp, int fromwire)
{
	Etherpkt *pkt;
	ushort type;
	int len, multi, tome, fromme;
	Netfile **ep, *f, **fp, *fx;
	Block *xbp;

	ether->inpackets++;

	pkt = (Etherpkt*)bp->rp;
	multi = pkt->d[0] & 1;
	/* check for valid multicast addresses */
	if(multi && memcmp(pkt->d, ether->bcast, sizeof(pkt->d)) != 0 &&
	    ether->prom == 0 && !activemulti(ether, pkt->d, sizeof(pkt->d))){
		if(fromwire){
			freeb(bp);
			bp = nil;
		}
		return bp;
	}

	/* is it for me? */
	tome   = memcmp(pkt->d, ether->ea, sizeof(pkt->d)) == 0;
	fromme = memcmp(pkt->s, ether->ea, sizeof(pkt->s)) == 0;

	/*
	 * Multiplex the packet to all the connections which want it.
	 * If the packet is not to be used subsequently (fromwire != 0),
	 * attempt to simply pass it into one of the connections, thereby
	 * saving a copy of the data (usual case hopefully).
	 */
	fx = nil;
	len = BLEN(bp);
	type = (pkt->type[0]<<8)|pkt->type[1];
	ep = &ether->f[Ntypes];
	for(fp = ether->f; fp < ep; fp++)
		if((f = *fp) == nil || f->type != type && f->type >= 0)
			continue;
		else if(!tome && !multi && !f->prom)
			continue;
		/* Don't want to hear bridged packets */
		else if(f->bridge && !fromwire && !fromme)
			continue;
		else if(f->headersonly)
			etherrtrace(f, pkt, len);
		else if(fromwire && fx == nil)
			fx = f;
		else if(xbp = iallocb(len)){
			memmove(xbp->wp, pkt, len);
			xbp->wp += len;
			if(qpass(f->in, xbp) < 0)
				softovflow(ether, "fx->in copy");
		} else
			softovflow(ether, "iallocb");

	if(fx){
		if(qpass(fx->in, bp) < 0)
			softovflow(ether, "fx->in normal");
		return nil;
	} else if(fromwire){
		freeb(bp);
		return nil;
	} else
		return bp;
}

static int
etheroq(Ether* ether, Block* bp)
{
	int len, loopback, s;
	Etherpkt *pkt;

	if (active.exiting) {
		freeb(bp);
		return 0;
	}
	ether->outpackets++;

	/*
	 * Check if the packet has to be placed back onto the input queue,
	 * i.e. if it's a loopback or broadcast packet or the interface is
	 * in promiscuous mode.
	 * If it's a loopback packet indicate to etheriq that the data isn't
	 * needed and return, etheriq will pass-on or free the block.
	 * To enable bridging to work, only packets that were originated
	 * by this interface are fed back.
	 */
	pkt = (Etherpkt*)bp->rp;
	len = BLEN(bp);
	loopback = memcmp(pkt->d, ether->ea, sizeof(pkt->d)) == 0;
	if(loopback || memcmp(pkt->d, ether->bcast, sizeof(pkt->d)) == 0 ||
	    ether->prom){
		s = splhi();
		etheriq(ether, bp, 0);
		splx(s);
	}

	if(!loopback){
		if(qfull(ether->oq))
			print("etheroq: WARNING: ether->oq full!\n");
		qbwrite(ether->oq, bp);
		if(ether->transmit != nil)
			ether->transmit(ether);
	} else
		freeb(bp);

	return len;
}

static long
etherwrite(Chan* chan, void* buf, long n, vlong)
{
	Ether *ether;
	Block *bp;
	int nn, onoff;
	Cmdbuf *cb;

	ether = etherxx[chan->dev];
	if(NETTYPE(chan->qid.path) != Ndataqid) {
		nn = netifwrite(ether, chan, buf, n);
		if(nn >= 0)
			return nn;
		cb = parsecmd(buf, n);
		if(cb->f[0] && strcmp(cb->f[0], "nonblocking") == 0){
			if(cb->nf <= 1)
				onoff = 1;
			else
				onoff = atoi(cb->f[1]);
			qnoblock(ether->oq, onoff);
			free(cb);
			return n;
		}
		free(cb);
		if(ether->ctl != nil)
			return ether->ctl(ether, buf, n);

		error(Ebadctl);
	}

	if(n > ether->mtu)
		error(Etoobig);
	if(n < ether->minmtu)
		error(Etoosmall);

	bp = allocb(n);
	if(waserror()){
		freeb(bp);
		nexterror();
	}
	memmove(bp->rp, buf, n);
	memmove(bp->rp+Eaddrlen, ether->ea, Eaddrlen);
	poperror();
	bp->wp += n;

	return etheroq(ether, bp);
}

static long
etherbwrite(Chan* chan, Block* bp, ulong)
{
	Ether *ether;
	long n;

	n = BLEN(bp);
	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	if(NETTYPE(chan->qid.path) != Ndataqid){
		n = etherwrite(chan, bp->rp, n, 0);
		poperror();
		freeb(bp);
		return n;
	}
	ether = etherxx[chan->dev];
	if(n > ether->mtu)
		error(Etoobig);
	if(n < ether->minmtu)
		error(Etoosmall);
	poperror();

	return etheroq(ether, bp);
}

static struct {
	char*	type;
	int	(*reset)(Ether*);
} cards[MaxEther+1];

/* adds a model of ethernet card, not a specific instance */
void
addethercard(char* t, int (*r)(Ether*))
{
	static unsigned ncard;

	if(ncard >= MaxEther)
		panic("too many ether cards (%d)", ncard);
	cards[ncard].type = t;
	cards[ncard].reset = r;
	ncard++;
}

int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	for(i = 0; i < Eaddrlen; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

static Ether*
etherprobe(int cardno, int ctlrno)
{
	int i, lg;
	ulong mb, bsz;
	Ether *ether;
	char *p, *e;
	char buf[128], name[32];

	ether = malloc(sizeof(Ether));
	if(ether == nil)
		error(Enomem);
	memset(ether, 0, sizeof(Ether));
	ether->ctlrno = ctlrno;
	ether->tbdf = BUSUNKNOWN;
	ether->mbps = 1000;
	ether->minmtu = ETHERMINTU;
	ether->maxmtu = ether->mtu = ETHERMAXTU;

	if(cardno < 0){
		if(isaconfig("ether", ctlrno, ether) == 0){
			free(ether);
			return nil;
		}
		for(cardno = 0; cards[cardno].type; cardno++){
			if(cistrcmp(cards[cardno].type, ether->type))
				continue;
			for(i = 0; i < ether->nopt; i++){
				if(strncmp(ether->opt[i], "ea=", 3))
					continue;
				if(parseether(ether->ea, &ether->opt[i][3]))
					memset(ether->ea, 0, Eaddrlen);
			}
			break;
		}
	}

	if(cardno >= MaxEther || cards[cardno].type == nil){
		free(ether);
		return nil;
	}
	if(cards[cardno].reset(ether) < 0){	/* fills in irq, etc. */
		free(ether);
		return nil;
	}

	/*
	 * IRQ2 doesn't really exist, it's used to gang the interrupt
	 * controllers together. A device set to IRQ2 will appear on
	 * the second interrupt controller as IRQ9.
	 */
	if(ether->irq == 2)
		ether->irq = 9;
	snprint(name, sizeof(name), "ether%d", ctlrno);

	/*
	 * If ether->irq is <0, it is a hack to indicate no interrupt
	 * used by ethersink.
	 */
	if(ether->irq >= 0)
		ether->irq = intrenable(ether->irq, ether->interrupt, ether,
			ether->tbdf, name);

	p = buf;
	e = buf + sizeof buf;
	p = seprint(p, e, "#l%d: %s: ", ctlrno, cards[cardno].type);
	if(ether->mbps >= 1000)
		p = seprint(p, e, "%dGbps", ether->mbps/1000);
	else
		p = seprint(p, e, "%dMbps", ether->mbps);
	p = seprint(p, e, " regs %#p irq %d", ether->port, ether->irq);
	if(ether->mem)
		p = seprint(p, e, " addr %#p", ether->mem);
	if(ether->size)
		p = seprint(p, e, " size %ld", ether->size);
	p = seprint(p, e, ": %2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux",
		ether->ea[0], ether->ea[1], ether->ea[2],
		ether->ea[3], ether->ea[4], ether->ea[5]);
	seprint(p, e, "\n");
	print(buf);

	/*
	 * input queues are allocated by ../port/netif.c:/^openfile.
	 * the size will be the last argument to netifinit() below.
	 *
	 * output queues should be small, to minimise `bufferbloat',
	 * which confuses tcp's feedback loop.  at 1Gb/s, it only takes
	 * ~15µs to transmit a full-sized non-jumbo packet.
	 */

	/* compute log10(ether->mbps) into lg */
	mb = ether->mbps;
	if (mb < 1000)			/* implausible nowadays */
		mb = 1000;
	for(lg = 0; mb >= 10; lg++)
		mb /= 10;
	if (lg > 14)			/* sanity cap; 2**(14+15) = 2²⁹ */
		lg = 14;

	/* allocate larger input queues for higher-speed interfaces */
	bsz = 1UL << (lg + 15);		/* 2ⁱ⁵ = 32K, bsz = 2ⁿ × 32K */
	while (bsz > mainmem->maxsize / 8 && bsz > 128*1024)	/* sanity */
		bsz /= 2;
	netifinit(ether, name, Ntypes, bsz);

	if(ether->oq == nil)
		ether->oq = qopen(1 << (lg + 13), Qmsg, 0, 0);
	if(ether->oq == nil)
		panic("etherreset %s: can't allocate output queue", name);

	ether->alen = Eaddrlen;
	memmove(ether->addr, ether->ea, Eaddrlen);
	memset(ether->bcast, 0xFF, Eaddrlen);

	return ether;
}

static void
etherreset(void)
{
	Ether *ether;
	int cardno, ctlrno;

	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++){
		if((ether = etherprobe(-1, ctlrno)) == nil)
			continue;
		etherxx[ctlrno] = ether;
	}

	if(getconf("*noetherprobe"))
		return;

	cardno = ctlrno = 0;
	while(cards[cardno].type != nil && ctlrno < MaxEther){
		if(etherxx[ctlrno] != nil){
			ctlrno++;
			continue;
		}
		if((ether = etherprobe(cardno, ctlrno)) == nil){
			cardno++;
			continue;
		}
		etherxx[ctlrno] = ether;
		ctlrno++;
	}
	if (ctlrno == 0) {
		print("no known ethernet interfaces found\n");
		pcihinv(nil);
		for(;;)
			tsleep(&up->sleep, return0, 0, 1000);
	}
}

static void
ethershutdown(void)
{
	Ether *ether;
	int i;

	for(i = 0; i < MaxEther; i++){
		ether = etherxx[i];
		if(ether == nil)
			continue;
		if(ether->shutdown == nil) {
			print("#l%d: no shutdown function\n", i);
			continue;
		}
		(*ether->shutdown)(ether);
	}
}


#define POLY 0xedb88320

/* really slow 32 bit crc for ethers */
ulong
ethercrc(uchar *p, int len)
{
	int i, j;
	ulong crc, b;

	crc = 0xffffffff;
	for(i = 0; i < len; i++){
		b = *p++;
		for(j = 0; j < 8; j++){
			crc = (crc>>1) ^ (((crc^b) & 1) ? POLY : 0);
			b >>= 1;
		}
	}
	return crc;
}

Dev etherdevtab = {
	'l',
	"ether",

	etherreset,
	devinit,
	ethershutdown,
	etherattach,
	etherwalk,
	etherstat,
	etheropen,
	ethercreate,
	etherclose,
	etherread,
	etherbread,
	etherwrite,
	etherbwrite,
	devremove,
	etherwstat,
};

enum {
	Ethidlen	= 32,
};

/* common to intel *gbe drivers, so far */
int
etherfmt(Fmt* fmt)
{
	char *p;
	int r, ctlrno;
	Ctlr *ctlr;
	Ethident *ctlrcomm;

	if((p = malloc(Ethidlen)) == nil)
		return fmtstrcpy(fmt, "(etherfmt)");
	switch(fmt->r){
	case L'æ':
		ctlr = va_arg(fmt->args, Ctlr *);
		if(ctlr == nil) {
			snprint(p, Ethidlen, "#l?");
			break;
		}
		ctlrcomm = (Ethident *)ctlr;
		ctlrno = ctlrcomm->edev == nil? -1: ctlrcomm->edev->ctlrno;
		snprint(p, Ethidlen, "#l%d: %s", ctlrno, ctlrcomm->prtype);
		break;
	default:
		snprint(p, Ethidlen, "(etherfmt)");
		break;
	}
	r = fmtstrcpy(fmt, p);
	free(p);
	return r;
}
