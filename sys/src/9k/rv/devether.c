/*
 * ethernet driver framework
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#include "../port/netif.h"
#include "etherif.h"

static Ether *etherxx[MaxEther];

extern Dev etherdevtab;

Chan*
etherattach(char* spec)
{
	ulong ctlrno;
	char *p;
	Chan *chan;
	Ether *ether;

	ctlrno = 0;
	if(spec && *spec){
		ctlrno = strtoul(spec, &p, 0);
		if((ctlrno == 0 && p == spec) || *p != 0 || ctlrno >= MaxEther)
			error(Ebadarg);
	}
	ether = etherxx[ctlrno];
	if(ether == nil)
		error(Enodev);

	chan = devattach(etherdevtab.dc, spec);
	if(waserror()){
		chanfree(chan);
		nexterror();
	}
	chan->devno = ctlrno;
	if(ether->attach)
		ether->attach(ether);
	poperror();
	return chan;
}

static Walkqid*
etherwalk(Chan* chan, Chan* nchan, char** name, int nname)
{
	return netifwalk(etherxx[chan->devno], chan, nchan, name, nname);
}

static long
etherstat(Chan* chan, uchar* dp, long n)
{
	return netifstat(etherxx[chan->devno], chan, dp, n);
}

static Chan*
etheropen(Chan* chan, int omode)
{
	return netifopen(etherxx[chan->devno], chan, omode);
}

static void
ethercreate(Chan*, char*, int, int)
{
}

static void
etherclose(Chan* chan)
{
	netifclose(etherxx[chan->devno], chan);
}

static long
etherread(Chan* chan, void* buf, long n, vlong off)
{
	Ether *ether;
	ulong offset = off;

	ether = etherxx[chan->devno];
	if((chan->qid.type & QTDIR) == 0 && ether->ifstat)
		/*
		 * With some controllers it is necessary to reach
		 * into the chip to extract statistics.
		 */
		if(NETTYPE(chan->qid.path) == Nifstatqid)
			return ether->ifstat(ether, buf, n, offset);
		else if(NETTYPE(chan->qid.path) == Nstatqid)
			ether->ifstat(ether, buf, 0, offset);
	return netifread(ether, chan, buf, n, offset);
}

static Block*
etherbread(Chan* chan, long n, vlong offset)
{
	return netifbread(etherxx[chan->devno], chan, n, offset);
}

static long
etherwstat(Chan* chan, uchar* dp, long n)
{
	return netifwstat(etherxx[chan->devno], chan, dp, n);
}

static void
etherrtrace(Netfile* f, Etherpkt* pkt, int len)
{
	Block *bp;

	if(qwindow(f->iq) <= 0)
		return;
	bp = iallocb(64);
	if(bp == nil)
		return;
	memmove(bp->wp, pkt->d, MIN(len, 58));
	bp->wp[58] = len>>8;
	bp->wp[59] = len;
	bp->wp = beputl(&bp->wp[60], TK2MS(sys->ticks));
	qpass(f->iq, bp);
}

void dump(uchar *p, int len);

/* qpass or freeb bp if fromwire */
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
	len = BLEN(bp);
	type = (pkt->type[0]<<8)|pkt->type[1];
	fx = 0;
	ep = &ether->f[Ntypes];

	/* check for valid multicast addresses */
	multi = pkt->d[0] & 1;
	if(multi && memcmp(pkt->d, ether->bcast, sizeof(pkt->d)) != 0 &&
	    ether->prom == 0 && !activemulti(ether, pkt->d, sizeof(pkt->d))){
		if(fromwire){
			freeb(bp);
			bp = nil;
		}
		return bp;
	}

	/* is it for me? */
	tome = memcmp(pkt->d, ether->ea, sizeof(pkt->d)) == 0;
	fromme = memcmp(pkt->s, ether->ea, sizeof(pkt->s)) == 0;
	/*
	 * Multiplex the packet to all the connections which want it.
	 * If the packet is not to be used subsequently (fromwire != 0),
	 * attempt to simply pass it into one of the connections, thereby
	 * saving a copy of the data (usual case hopefully).
	 */
	for(fp = ether->f; fp < ep; fp++)
		if((f = *fp) != nil && (f->type == type || f->type < 0) &&
		    (tome || multi || f->prom))
			/* Don't want to hear bridged packets */
			if(f->bridge && !fromwire && !fromme)
				continue;
			else if(f->headersonly)
				etherrtrace(f, pkt, len);
			else if(fromwire && fx == 0)
				fx = f;
			else if((xbp = iallocb(len)) == nil)
				ether->soverflows++;
			else {
				/* pass a copy up f->iq */
				memmove(xbp->wp, pkt, len);
				xbp->wp += len;
				if(qpass(f->iq, xbp) < 0)
					ether->soverflows++;
			}

	if(fx){
		if(qpass(fx->iq, bp) < 0)
			ether->soverflows++;
		return nil;
	}
	if(fromwire){
		freeb(bp);
		return nil;
	}
	return bp;
}

static int
etheroq(Ether* ether, Block* bp)
{
	int len, loopback;
	Etherpkt *pkt;
	Mpl s;

	ether->outpackets++;

	/*
	 * Check if the packet has to be placed back onto the input queue,
	 * i.e., if it's a loopback or broadcast packet or the interface is
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
		/* bp is still valid */
		splx(s);
	}

	if(!loopback){
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

	ether = etherxx[chan->devno];
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
		if(ether->ctl!=nil)
			return ether->ctl(ether,buf,n);

		error(Ebadctl);
	}

	if(n > ether->maxmtu)
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
etherbwrite(Chan* chan, Block* bp, vlong)
{
	Ether *ether;
	long n;

	n = BLEN(bp);
	if(NETTYPE(chan->qid.path) != Ndataqid){
		if(waserror()) {
			freeb(bp);
			nexterror();
		}
		n = etherwrite(chan, bp->rp, n, 0);
		poperror();
		freeb(bp);
		return n;
	}
	ether = etherxx[chan->devno];

	if(n > ether->maxmtu){
		freeb(bp);
		error(Etoobig);
	}
	if(n < ether->minmtu){
		freeb(bp);
		error(Etoosmall);
	}

	return etheroq(ether, bp);
}

static struct {
	char*	type;
	int	(*reset)(Ether*);
} cards[MaxEther+1];

void
addethercard(char* t, int (*r)(Ether*))
{
	static uint ncard;

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

static Ether *
newether(int ctlrno)
{
	Ether *ether;

	ether = malloc(sizeof(Ether));
	ether->ctlrno = ctlrno;
	ether->tbdf = -1;
	ether->mbps = 1000;
	ether->minmtu = ETHERMINTU;
	ether->maxmtu = ETHERMAXTU;
	return ether;
}

static int
ethercardno(Ether *ether)
{
	int i, cardno;

	USED(ether);
	if(isaconfig("ether", ether->ctlrno, ether) == 0)
		return -1;
	for(cardno = 0; cards[cardno].type; cardno++)
		if(cistrcmp(cards[cardno].type, ether->type) == 0) {
			for(i = 0; i < ether->nopt; i++)
				if(strncmp(ether->opt[i], "ea=", 3) == 0 &&
				    parseether(ether->ea, &ether->opt[i][3]))
					memset(ether->ea, 0, Eaddrlen);
			break;
		}
	return cardno;
}

static char *
etherconffmt(char *p, char *e, Ether *ether, int cardno)
{
	p = seprint(p, e, "#l%d: %s: ", ether->ctlrno, cards[cardno].type);
	if(ether->mbps >= 1000)
		p = seprint(p, e, "%dGbps", ether->mbps/1000);
	else
		p = seprint(p, e, "%dMbps", ether->mbps);
	p = seprint(p, e, " regs %#p irq %d", ether->port, ether->irq);
	if(ether->mem)
		p = seprint(p, e, " addr %#p", ether->mem);
	if(ether->size)
		p = seprint(p, e, " size %lld", (vlong)ether->size);
	return seprint(p, e, ": %2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux\n",
		ether->ea[0], ether->ea[1], ether->ea[2],
		ether->ea[3], ether->ea[4], ether->ea[5]);
}

/*
 * input queues are allocated by ../port/netif.c:/^openfile.
 * the size will be the last argument to netifinit() below.
 *
 * output queues should be small, to minimise `bufferbloat',
 * which confuses tcp's feedback loop.  at 1Gb/s, it only takes
 * ~15µs to transmit a full-sized non-jumbo packet.
 */
static void
etherallocqs(Ether *ether, char *name)
{
	ulong bsz;

	if (ether->mbps >= 1000)
		bsz = 512*1024;
	else if(ether->mbps >= 100)
		bsz = 256*1024;
	else
		bsz = 128*1024;
	netifinit(ether, name, Ntypes, bsz);
	if(ether->oq == 0)
		ether->oq = qopen(bsz, Qmsg, 0, 0);
	if(ether->oq == 0)
		panic("etherprobe: no output q for %s", name);
}

static Ether*
etherprobe(int cardno, int ctlrno)
{
	Ether *ether;
	char buf[128], name[32];

	ether = newether(ctlrno);
	if(cardno < 0)
		cardno = ethercardno(ether);
	if((uint)cardno >= MaxEther || cards[cardno].type == nil ||
	    cards[cardno].reset(ether) < 0){
		free(ether);
		return nil;
	}

	snprint(name, sizeof(name), "ether%d", ctlrno);
	/*
	 * If ether->irq is <0, it is a hack to indicate no interrupt used by
	 * ethersink.  Or perhaps the driver has some other way to configure
	 * interrupts for itself (e.g., MSI).
	 */
	if(ether->irq >= 0)
		enableintr(ether, ether->interrupt, ether, name);

	etherconffmt(buf, buf + sizeof buf, ether, cardno);
	print(buf);

	etherallocqs(ether, name);
	ether->alen = Eaddrlen;
	memmove(ether->addr, ether->ea, Eaddrlen);
	memset(ether->bcast, 0xFF, Eaddrlen);
	return ether;
}

void
etherintr(Ureg *ur)
{
	int i;
	Ether *ether;

	for(i = 0; i < MaxEther; i++){
		if((ether = etherxx[i]) == nil)
			break;
		ether->interrupt(ur, ether);
	}
}

static void
etherreset(void)
{
	Ether *ether;
	int cardno, ctlrno;

	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++)
		if((ether = etherprobe(-1, ctlrno)) != nil)
			etherxx[ctlrno] = ether;

	if(getconf("*noetherprobe"))
		return;

	cardno = ctlrno = 0;
	while(cards[cardno].type != nil && ctlrno < MaxEther)
		if(etherxx[ctlrno] != nil)
			ctlrno++;
		else if((ether = etherprobe(cardno, ctlrno)) == nil)
			cardno++;
		else
			etherxx[ctlrno++] = ether;
	if (ctlrno == 0) {
		print("no known ethernet interfaces found\n");
		for(;;)
			idlehands();
	}
}

static void
ethershutdown(void)
{
	Ether *ether;
	int i;

	for(i = 0; i < MaxEther; i++){
		ether = etherxx[i];
		if(ether != nil)
			if(ether->shutdown == nil)
				print("#l%d: no shutdown function\n", i);
			else
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

	crc = ~0ul;
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
