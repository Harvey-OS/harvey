/*
 *  This protocol is compatible with UDP's packet format.
 *  It could be done over UDP if need be.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"ip.h"

#define DEBUG	0
#define DPRINT if(DEBUG)print

#define SEQDIFF(a,b) ( (a)>=(b)?\
			(a)-(b):\
			0xffffffffUL-((b)-(a)) )
#define INSEQ(a,start,end) ( (start)<=(end)?\
				((a)>(start)&&(a)<=(end)):\
				((a)>(start)||(a)<=(end)) )
#define UNACKED(r) SEQDIFF(r->sndseq, r->ackrcvd)
#define NEXTSEQ(a) ( (a)+1 == 0 ? 1 : (a)+1 )

enum
{
	UDP_HDRSIZE	= 20,	/* pseudo header + udp header */
	UDP_PHDRSIZE	= 12,	/* pseudo header */
	UDP_RHDRSIZE	= 36,	/* pseudo header + udp header + rudp header */
	UDP_IPHDR	= 8,	/* ip header */
	IP_UDPPROTO	= 254,
	UDP_USEAD7	= 52,
	UDP_USEAD6	= 36,
	UDP_USEAD4	= 12,

	Rudprxms	= 200,
	Rudptickms	= 50,
	Rudpmaxxmit	= 10,
	Maxunacked	= 100,

};

#define Hangupgen	0xffffffff	/* used only in hangup messages */

typedef struct Udphdr Udphdr;
struct Udphdr
{
	/* ip header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */

	/* pseudo header starts here */
	uchar	Unused;
	uchar	udpproto;	/* Protocol */
	uchar	udpplen[2];	/* Header plus data length */
	uchar	udpsrc[4];	/* Ip source */
	uchar	udpdst[4];	/* Ip destination */

	/* udp header */
	uchar	udpsport[2];	/* Source port */
	uchar	udpdport[2];	/* Destination port */
	uchar	udplen[2];	/* data length */
	uchar	udpcksum[2];	/* Checksum */
};

typedef struct Rudphdr Rudphdr;
struct Rudphdr
{
	/* ip header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */

	/* pseudo header starts here */
	uchar	Unused;
	uchar	udpproto;	/* Protocol */
	uchar	udpplen[2];	/* Header plus data length */
	uchar	udpsrc[4];	/* Ip source */
	uchar	udpdst[4];	/* Ip destination */

	/* udp header */
	uchar	udpsport[2];	/* Source port */
	uchar	udpdport[2];	/* Destination port */
	uchar	udplen[2];	/* data length (includes rudp header) */
	uchar	udpcksum[2];	/* Checksum */

	/* rudp header */
	uchar	relseq[4];	/* id of this packet (or 0) */
	uchar	relsgen[4];	/* generation/time stamp */
	uchar	relack[4];	/* packet being acked (or 0) */
	uchar	relagen[4];	/* generation/time stamp */
};


/*
 *  one state structure per destination
 */
typedef struct Reliable Reliable;
struct Reliable
{
	Ref;

	Reliable *next;

	uchar	addr[IPaddrlen];	/* always V6 when put here */
	ushort	port;

	Block	*unacked;	/* unacked msg list */
	Block	*unackedtail;	/*  and its tail */

	int	timeout;	/* time since first unacked msg sent */
	int	xmits;		/* number of times first unacked msg sent */

	ulong	sndseq;		/* next packet to be sent */
	ulong	sndgen;		/*  and its generation */

	ulong	rcvseq;		/* last packet received */
	ulong	rcvgen;		/*  and its generation */

	ulong	acksent;	/* last ack sent */
	ulong	ackrcvd;	/* last msg for which ack was rcvd */

	/* flow control */
	QLock	lock;
	Rendez	vous;
	int	blocked;
};



/* MIB II counters */
typedef struct Rudpstats Rudpstats;
struct Rudpstats
{
	ulong	rudpInDatagrams;
	ulong	rudpNoPorts;
	ulong	rudpInErrors;
	ulong	rudpOutDatagrams;
};

typedef struct Rudppriv Rudppriv;
struct Rudppriv
{
	Ipht	ht;

	/* MIB counters */
	Rudpstats	ustats;

	/* non-MIB stats */
	ulong	csumerr;		/* checksum errors */
	ulong	lenerr;			/* short packet */
	ulong	rxmits;			/* # of retransmissions */
	ulong	orders;			/* # of out of order pkts */

	/* keeping track of the ack kproc */
	int	ackprocstarted;
	QLock	apl;
};


static ulong generation = 0;
static Rendez rend;

/*
 *  protocol specific part of Conv
 */
typedef struct Rudpcb Rudpcb;
struct Rudpcb
{
	QLock;
	uchar	headers;
	uchar	randdrop;
	Reliable *r;
};

/*
 * local functions 
 */
void	relsendack(Conv*, Reliable*, int);
int	reliput(Conv*, Block*, uchar*, ushort);
Reliable *relstate(Rudpcb*, uchar*, ushort, char*);
void	relput(Reliable*);
void	relforget(Conv *, uchar*, int, int);
void	relackproc(void *);
void	relackq(Reliable *, Block*);
void	relhangup(Conv *, Reliable*);
void	relrexmit(Conv *, Reliable*);
void	relput(Reliable*);
void	rudpkick(void *x);

static void
rudpstartackproc(Proto *rudp)
{
	Rudppriv *rpriv;
	char kpname[KNAMELEN];

	rpriv = rudp->priv;
	if(rpriv->ackprocstarted == 0){
		qlock(&rpriv->apl);
		if(rpriv->ackprocstarted == 0){
			sprint(kpname, "#I%drudpack", rudp->f->dev);
			kproc(kpname, relackproc, rudp);
			rpriv->ackprocstarted = 1;
		}
		qunlock(&rpriv->apl);
	}
}

static char*
rudpconnect(Conv *c, char **argv, int argc)
{
	char *e;
	Rudppriv *upriv;

	upriv = c->p->priv;
	rudpstartackproc(c->p);
	e = Fsstdconnect(c, argv, argc);
	Fsconnected(c, e);
	iphtadd(&upriv->ht, c);

	return e;
}


static int
rudpstate(Conv *c, char *state, int n)
{
	Rudpcb *ucb;
	Reliable *r;
	int m;

	m = snprint(state, n, "%s", c->inuse?"Open":"Closed");
	ucb = (Rudpcb*)c->ptcl;
	qlock(ucb);
	for(r = ucb->r; r; r = r->next)
		m += snprint(state+m, n-m, " %I/%ld", r->addr, UNACKED(r));
	qunlock(ucb);
	return m;
}

static char*
rudpannounce(Conv *c, char** argv, int argc)
{
	char *e;
	Rudppriv *upriv;

	upriv = c->p->priv;
	rudpstartackproc(c->p);
	e = Fsstdannounce(c, argv, argc);
	if(e != nil)
		return e;
	Fsconnected(c, nil);
	iphtadd(&upriv->ht, c);

	return nil;
}

static void
rudpcreate(Conv *c)
{
	c->rq = qopen(64*1024, Qmsg, 0, 0);
	c->wq = qopen(64*1024, Qkick, rudpkick, c);
}

static void
rudpclose(Conv *c)
{
	Rudpcb *ucb;
	Reliable *r, *nr;
	Rudppriv *upriv;

	upriv = c->p->priv;
	iphtrem(&upriv->ht, c);

	/* force out any delayed acks */
	ucb = (Rudpcb*)c->ptcl;
	qlock(ucb);
	for(r = ucb->r; r; r = r->next){
		if(r->acksent != r->rcvseq)
			relsendack(c, r, 0);
	}
	qunlock(ucb);

	qclose(c->rq);
	qclose(c->wq);
	qclose(c->eq);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->lport = 0;
	c->rport = 0;

	ucb->headers = 0;
	ucb->randdrop = 0;
	qlock(ucb);
	for(r = ucb->r; r; r = nr){
		if(r->acksent != r->rcvseq)
			relsendack(c, r, 0);
		nr = r->next;
		relhangup(c, r);
		relput(r);
	}
	ucb->r = 0;

	qunlock(ucb);
}

/*
 *  randomly don't send packets
 */
static void
doipoput(Conv *c, Fs *f, Block *bp, int x, int ttl, int tos)
{
	Rudpcb *ucb;

	ucb = (Rudpcb*)c->ptcl;
	if(ucb->randdrop && nrand(100) < ucb->randdrop)
		freeblist(bp);
	else
		ipoput4(f, bp, x, ttl, tos, nil);
}

int
flow(void *v)
{
	Reliable *r = v;

	return UNACKED(r) <= Maxunacked;
}

void
rudpkick(void *x)
{
	Conv *c = x;
	Udphdr *uh;
	ushort rport;
	uchar laddr[IPaddrlen], raddr[IPaddrlen];
	Block *bp;
	Rudpcb *ucb;
	Rudphdr *rh;
	Reliable *r;
	int dlen, ptcllen;
	Rudppriv *upriv;
	Fs *f;

	upriv = c->p->priv;
	f = c->p->f;

	netlog(c->p->f, Logrudp, "rudp: kick\n");
	bp = qget(c->wq);
	if(bp == nil)
		return;

	ucb = (Rudpcb*)c->ptcl;
	switch(ucb->headers) {
	case 7:
		/* get user specified addresses */
		bp = pullupblock(bp, UDP_USEAD7);
		if(bp == nil)
			return;
		ipmove(raddr, bp->rp);
		bp->rp += IPaddrlen;
		ipmove(laddr, bp->rp);
		bp->rp += IPaddrlen;
		/* pick interface closest to dest */
		if(ipforme(f, laddr) != Runi)
			findlocalip(f, laddr, raddr);
		bp->rp += IPaddrlen;		/* Ignore ifc address */
		rport = nhgets(bp->rp);
		bp->rp += 2+2;			/* Ignore local port */
		break;
	case 6:
		/* get user specified addresses */
		bp = pullupblock(bp, UDP_USEAD6);
		if(bp == nil)
			return;
		ipmove(raddr, bp->rp);
		bp->rp += IPaddrlen;
		ipmove(laddr, bp->rp);
		bp->rp += IPaddrlen;
		/* pick interface closest to dest */
		if(ipforme(f, laddr) != Runi)
			findlocalip(f, laddr, raddr);
		rport = nhgets(bp->rp);

		bp->rp += 4;			/* Igonore local port */
		break;
	default:
		ipmove(raddr, c->raddr);
		ipmove(laddr, c->laddr);
		rport = c->rport;

		break;
	}

	dlen = blocklen(bp);

	/* Make space to fit rudp & ip header */
	bp = padblock(bp, UDP_IPHDR+UDP_RHDRSIZE);
	if(bp == nil)
		return;

	uh = (Udphdr *)(bp->rp);
	uh->vihl = IP_VER4;

	rh = (Rudphdr*)uh;

	ptcllen = dlen + (UDP_RHDRSIZE-UDP_PHDRSIZE);
	uh->Unused = 0;
	uh->udpproto = IP_UDPPROTO;
	uh->frag[0] = 0;
	uh->frag[1] = 0;
	hnputs(uh->udpplen, ptcllen);
	switch(ucb->headers){
	case 6:
	case 7:
		v6tov4(uh->udpdst, raddr);
		hnputs(uh->udpdport, rport);
		v6tov4(uh->udpsrc, laddr);
		break;
	default:
		v6tov4(uh->udpdst, c->raddr);
		hnputs(uh->udpdport, c->rport);
		if(ipcmp(c->laddr, IPnoaddr) == 0)
			findlocalip(f, c->laddr, c->raddr);
		v6tov4(uh->udpsrc, c->laddr);
		break;
	}
	hnputs(uh->udpsport, c->lport);
	hnputs(uh->udplen, ptcllen);
	uh->udpcksum[0] = 0;
	uh->udpcksum[1] = 0;

	qlock(ucb);
	r = relstate(ucb, raddr, rport, "kick");
	r->sndseq = NEXTSEQ(r->sndseq);
	hnputl(rh->relseq, r->sndseq);
	hnputl(rh->relsgen, r->sndgen);

	hnputl(rh->relack, r->rcvseq);  /* ACK last rcvd packet */
	hnputl(rh->relagen, r->rcvgen);

	if(r->rcvseq != r->acksent)
		r->acksent = r->rcvseq;

	hnputs(uh->udpcksum, ptclcsum(bp, UDP_IPHDR, dlen+UDP_RHDRSIZE));

	relackq(r, bp);
	qunlock(ucb);

	upriv->ustats.rudpOutDatagrams++;

	DPRINT("sent: %lud/%lud, %lud/%lud\n", 
		r->sndseq, r->sndgen, r->rcvseq, r->rcvgen);

	doipoput(c, f, bp, 0, c->ttl, c->tos);

	if(waserror()) {
		relput(r);
		qunlock(&r->lock);
		nexterror();
	}

	/* flow control of sorts */
	qlock(&r->lock);
	if(UNACKED(r) > Maxunacked){
		r->blocked = 1;
		sleep(&r->vous, flow, r);
		r->blocked = 0;
	}

	qunlock(&r->lock);
	relput(r);
	poperror();
}

void
rudpiput(Proto *rudp, Ipifc *ifc, Block *bp)
{
	int len, olen, ottl;
	Udphdr *uh;
	Conv *c;
	Rudpcb *ucb;
	uchar raddr[IPaddrlen], laddr[IPaddrlen];
	ushort rport, lport;
	Rudppriv *upriv;
	Fs *f;
	uchar *p;

	upriv = rudp->priv;
	f = rudp->f;

	upriv->ustats.rudpInDatagrams++;

	uh = (Udphdr*)(bp->rp);

	/* Put back pseudo header for checksum 
	 * (remember old values for icmpnoconv()) 
	 */
	ottl = uh->Unused;
	uh->Unused = 0;
	len = nhgets(uh->udplen);
	olen = nhgets(uh->udpplen);
	hnputs(uh->udpplen, len);

	v4tov6(raddr, uh->udpsrc);
	v4tov6(laddr, uh->udpdst);
	lport = nhgets(uh->udpdport);
	rport = nhgets(uh->udpsport);

	if(nhgets(uh->udpcksum)) {
		if(ptclcsum(bp, UDP_IPHDR, len+UDP_PHDRSIZE)) {
			upriv->ustats.rudpInErrors++;
			upriv->csumerr++;
			netlog(f, Logrudp, "rudp: checksum error %I\n", raddr);
			DPRINT("rudp: checksum error %I\n", raddr);
			freeblist(bp);
			return;
		}
	}

	qlock(rudp);

	c = iphtlook(&upriv->ht, raddr, rport, laddr, lport);
	if(c == nil){
		/* no converstation found */
		upriv->ustats.rudpNoPorts++;
		qunlock(rudp);
		netlog(f, Logudp, "udp: no conv %I!%d -> %I!%d\n", raddr, rport,
			laddr, lport);
		uh->Unused = ottl;
		hnputs(uh->udpplen, olen);
		icmpnoconv(f, bp);
		freeblist(bp);
		return;
	}
	ucb = (Rudpcb*)c->ptcl;
	qlock(ucb);
	qunlock(rudp);

	if(reliput(c, bp, raddr, rport) < 0){
		qunlock(ucb);
		freeb(bp);
		return;
	}

	/*
	 * Trim the packet down to data size
	 */

	len -= (UDP_RHDRSIZE-UDP_PHDRSIZE);
	bp = trimblock(bp, UDP_IPHDR+UDP_RHDRSIZE, len);
	if(bp == nil) {
		netlog(f, Logrudp, "rudp: len err %I.%d -> %I.%d\n", 
			raddr, rport, laddr, lport);
		DPRINT("rudp: len err %I.%d -> %I.%d\n", 
			raddr, rport, laddr, lport);
		upriv->lenerr++;
		return;
	}

	netlog(f, Logrudpmsg, "rudp: %I.%d -> %I.%d l %d\n", 
		raddr, rport, laddr, lport, len);

	switch(ucb->headers){
	case 7:
		/* pass the src address */
		bp = padblock(bp, UDP_USEAD7);
		p = bp->rp;
		ipmove(p, raddr); p += IPaddrlen;
		ipmove(p, laddr); p += IPaddrlen;
		ipmove(p, ifc->lifc->local); p += IPaddrlen;
		hnputs(p, rport); p += 2;
		hnputs(p, lport);
		break;
	case 6:
		/* pass the src address */
		bp = padblock(bp, UDP_USEAD6);
		p = bp->rp;
		ipmove(p, raddr); p += IPaddrlen;
		ipmove(p, ipforme(f, laddr)==Runi ? laddr : ifc->lifc->local); p += IPaddrlen;
		hnputs(p, rport); p += 2;
		hnputs(p, lport);
		break;
	default:
		/* connection oriented rudp */
		if(ipcmp(c->raddr, IPnoaddr) == 0){
			/* save the src address in the conversation */
		 	ipmove(c->raddr, raddr);
			c->rport = rport;

			/* reply with the same ip address (if not broadcast) */
			if(ipforme(f, laddr) == Runi)
				ipmove(c->laddr, laddr);
			else
				v4tov6(c->laddr, ifc->lifc->local);
		}
		break;
	}
	if(bp->next)
		bp = concatblock(bp);

	if(qfull(c->rq)) {
		netlog(f, Logrudp, "rudp: qfull %I.%d -> %I.%d\n", raddr, rport,
			laddr, lport);
		freeblist(bp);
	}
	else
		qpass(c->rq, bp);
	
	qunlock(ucb);
}

static char *rudpunknown = "unknown rudp ctl request";

char*
rudpctl(Conv *c, char **f, int n)
{
	Rudpcb *ucb;
	uchar ip[IPaddrlen];
	int x;

	ucb = (Rudpcb*)c->ptcl;
	if(n < 1)
		return rudpunknown;

	if(strcmp(f[0], "headers++4") == 0){
		ucb->headers = 7;
		return nil;
	} else if(strcmp(f[0], "headers") == 0){
		ucb->headers = 6;
		return nil;
	} else if(strcmp(f[0], "hangup") == 0){
		if(n < 3)
			return "bad syntax";
		parseip(ip, f[1]);
		x = atoi(f[2]);
		qlock(ucb);
		relforget(c, ip, x, 1);
		qunlock(ucb);
		return nil;
	} else if(strcmp(f[0], "randdrop") == 0){
		x = 10;		/* default is 10% */
		if(n > 1)
			x = atoi(f[1]);
		if(x > 100 || x < 0)
			return "illegal rudp drop rate";
		ucb->randdrop = x;
		return nil;
	}
	return rudpunknown;
}

void
rudpadvise(Proto *rudp, Block *bp, char *msg)
{
	Udphdr *h;
	uchar source[IPaddrlen], dest[IPaddrlen];
	ushort psource, pdest;
	Conv *s, **p;

	h = (Udphdr*)(bp->rp);

	v4tov6(dest, h->udpdst);
	v4tov6(source, h->udpsrc);
	psource = nhgets(h->udpsport);
	pdest = nhgets(h->udpdport);

	/* Look for a connection */
	for(p = rudp->conv; *p; p++) {
		s = *p;
		if(s->rport == pdest)
		if(s->lport == psource)
		if(ipcmp(s->raddr, dest) == 0)
		if(ipcmp(s->laddr, source) == 0){
			qhangup(s->rq, msg);
			qhangup(s->wq, msg);
			break;
		}
	}
	freeblist(bp);
}

int
rudpstats(Proto *rudp, char *buf, int len)
{
	Rudppriv *upriv;

	upriv = rudp->priv;
	return snprint(buf, len, "%lud %lud %lud %lud %lud %lud\n",
		upriv->ustats.rudpInDatagrams,
		upriv->ustats.rudpNoPorts,
		upriv->ustats.rudpInErrors,
		upriv->ustats.rudpOutDatagrams,
		upriv->rxmits,
		upriv->orders);
}

void
rudpinit(Fs *fs)
{

	Proto *rudp;

	rudp = smalloc(sizeof(Proto));
	rudp->priv = smalloc(sizeof(Rudppriv));
	rudp->name = "rudp";
	rudp->connect = rudpconnect;
	rudp->announce = rudpannounce;
	rudp->ctl = rudpctl;
	rudp->state = rudpstate;
	rudp->create = rudpcreate;
	rudp->close = rudpclose;
	rudp->rcv = rudpiput;
	rudp->advise = rudpadvise;
	rudp->stats = rudpstats;
	rudp->ipproto = IP_UDPPROTO;
	rudp->nc = 16;
	rudp->ptclsize = sizeof(Rudpcb);

	Fsproto(fs, rudp);
}

/*********************************************/
/* Here starts the reliable helper functions */
/*********************************************/
/*
 *  Enqueue a copy of an unacked block for possible retransmissions
 */
void
relackq(Reliable *r, Block *bp)
{
	Block *np;

	np = copyblock(bp, blocklen(bp));
	if(r->unacked)
		r->unackedtail->list = np;
	else {
		/* restart timer */
		r->timeout = 0;
		r->xmits = 1;
		r->unacked = np;
	}
	r->unackedtail = np;
	np->list = nil;
}

/*
 *  retransmit unacked blocks
 */
void
relackproc(void *a)
{
	Rudpcb *ucb;
	Proto *rudp;
	Reliable *r;
	Conv **s, *c;

	rudp = (Proto *)a;

loop:
	tsleep(&up->sleep, return0, 0, Rudptickms);

	for(s = rudp->conv; *s; s++) {
		c = *s;
		ucb = (Rudpcb*)c->ptcl;
		qlock(ucb);

		for(r = ucb->r; r; r = r->next) {
			if(r->unacked != nil){
				r->timeout += Rudptickms;
				if(r->timeout > Rudprxms*r->xmits)
					relrexmit(c, r);
			}
			if(r->acksent != r->rcvseq)
				relsendack(c, r, 0);
		}
		qunlock(ucb);
	}
	goto loop;
}

/*
 *  get the state record for a conversation
 */
Reliable*
relstate(Rudpcb *ucb, uchar *addr, ushort port, char *from)
{
	Reliable *r, **l;

	l = &ucb->r;
	for(r = *l; r; r = *l){
		if(memcmp(addr, r->addr, IPaddrlen) == 0 && 
		    port == r->port)
			break;
		l = &r->next;
	}

	/* no state for this addr/port, create some */
	if(r == nil){
		while(generation == 0)
			generation = rand();

		DPRINT("from %s new state %lud for %I!%ud\n", 
		        from, generation, addr, port);

		r = smalloc(sizeof(Reliable));
		memmove(r->addr, addr, IPaddrlen);
		r->port = port;
		r->unacked = 0;
		if(generation == Hangupgen)
			generation++;
		r->sndgen = generation++;
		r->sndseq = 0;
		r->ackrcvd = 0;
		r->rcvgen = 0;
		r->rcvseq = 0;
		r->acksent = 0;
		r->xmits = 0;
		r->timeout = 0;
		r->ref = 0;
		incref(r);	/* one reference for being in the list */

		*l = r;
	}

	incref(r);
	return r;
}

void
relput(Reliable *r)
{
	if(decref(r) == 0)
		free(r);
}

/*
 *  forget a Reliable state
 */
void
relforget(Conv *c, uchar *ip, int port, int originator)
{
	Rudpcb *ucb;
	Reliable *r, **l;

	ucb = (Rudpcb*)c->ptcl;

	l = &ucb->r;
	for(r = *l; r; r = *l){
		if(ipcmp(ip, r->addr) == 0 && port == r->port){
			*l = r->next;
			if(originator)
				relsendack(c, r, 1);
			relhangup(c, r);
			relput(r);	/* remove from the list */
			break;
		}
		l = &r->next;
	}
}

/* 
 *  process a rcvd reliable packet. return -1 if not to be passed to user process,
 *  0 therwise.
 *
 *  called with ucb locked.
 */
int
reliput(Conv *c, Block *bp, uchar *addr, ushort port)
{
	Block *nbp;
	Rudpcb *ucb;
	Rudppriv *upriv;
	Udphdr *uh;
	Reliable *r;
	Rudphdr *rh;
	ulong seq, ack, sgen, agen, ackreal;
	int rv = -1;

	/* get fields */
	uh = (Udphdr*)(bp->rp);
	rh = (Rudphdr*)uh;
	seq = nhgetl(rh->relseq);
	sgen = nhgetl(rh->relsgen);
	ack = nhgetl(rh->relack);
	agen = nhgetl(rh->relagen);

	upriv = c->p->priv;
	ucb = (Rudpcb*)c->ptcl;
	r = relstate(ucb, addr, port, "input");

	DPRINT("rcvd %lud/%lud, %lud/%lud, r->sndgen = %lud\n", 
		seq, sgen, ack, agen, r->sndgen);

	/* if acking an incorrect generation, ignore */
	if(ack && agen != r->sndgen)
		goto out;

	/* Look for a hangup */
	if(sgen == Hangupgen) {
		if(agen == r->sndgen)
			relforget(c, addr, port, 0);
		goto out;
	}

	/* make sure we're not talking to a new remote side */
	if(r->rcvgen != sgen){
		if(seq != 0 && seq != 1)
			goto out;

		/* new connection */
		if(r->rcvgen != 0){
			DPRINT("new con r->rcvgen = %lud, sgen = %lud\n", r->rcvgen, sgen);
			relhangup(c, r);
		}
		r->rcvgen = sgen;
	}

	/* dequeue acked packets */
	if(ack && agen == r->sndgen){
		ackreal = 0;
		while(r->unacked != nil && INSEQ(ack, r->ackrcvd, r->sndseq)){
			nbp = r->unacked;
			r->unacked = nbp->list;
			DPRINT("%lud/%lud acked, r->sndgen = %lud\n", 
			       ack, agen, r->sndgen);
			freeb(nbp);
			r->ackrcvd = NEXTSEQ(r->ackrcvd);
			ackreal = 1;
		}

		/* flow control */
		if(UNACKED(r) < Maxunacked/8 && r->blocked)
			wakeup(&r->vous);

		/*
		 *  retransmit next packet if the acked packet
		 *  was transmitted more than once
		 */
		if(ackreal && r->unacked != nil){
			r->timeout = 0;
			if(r->xmits > 1){
				r->xmits = 1;
				relrexmit(c, r);
			}
		}
		
	}

	/* no message or input queue full */
	if(seq == 0 || qfull(c->rq))
		goto out;

	/* refuse out of order delivery */
	if(seq != NEXTSEQ(r->rcvseq)){
		relsendack(c, r, 0);	/* tell him we got it already */
		upriv->orders++;
		DPRINT("out of sequence %lud not %lud\n", seq, NEXTSEQ(r->rcvseq));
		goto out;
	}
	r->rcvseq = seq;

	rv = 0;
out:
	relput(r);
	return rv;
}

void
relsendack(Conv *c, Reliable *r, int hangup)
{
	Udphdr *uh;
	Block *bp;
	Rudphdr *rh;
	int ptcllen;
	Fs *f;

	bp = allocb(UDP_IPHDR + UDP_RHDRSIZE);
	if(bp == nil)
		return;
	bp->wp += UDP_IPHDR + UDP_RHDRSIZE;
	f = c->p->f;
	uh = (Udphdr *)(bp->rp);
	uh->vihl = IP_VER4;
	rh = (Rudphdr*)uh;

	ptcllen = (UDP_RHDRSIZE-UDP_PHDRSIZE);
	uh->Unused = 0;
	uh->udpproto = IP_UDPPROTO;
	uh->frag[0] = 0;
	uh->frag[1] = 0;
	hnputs(uh->udpplen, ptcllen);

	v6tov4(uh->udpdst, r->addr);
	hnputs(uh->udpdport, r->port);
	hnputs(uh->udpsport, c->lport);
	if(ipcmp(c->laddr, IPnoaddr) == 0)
		findlocalip(f, c->laddr, c->raddr);
	v6tov4(uh->udpsrc, c->laddr);
	hnputs(uh->udplen, ptcllen);

	if(hangup)
		hnputl(rh->relsgen, Hangupgen);
	else
		hnputl(rh->relsgen, r->sndgen);
	hnputl(rh->relseq, 0);
	hnputl(rh->relagen, r->rcvgen);
	hnputl(rh->relack, r->rcvseq);

	if(r->acksent < r->rcvseq)
		r->acksent = r->rcvseq;

	uh->udpcksum[0] = 0;
	uh->udpcksum[1] = 0;
	hnputs(uh->udpcksum, ptclcsum(bp, UDP_IPHDR, UDP_RHDRSIZE));

	DPRINT("sendack: %lud/%lud, %lud/%lud\n", 0L, r->sndgen, r->rcvseq, r->rcvgen);
	doipoput(c, f, bp, 0, c->ttl, c->tos);
}


/*
 *  called with ucb locked (and c locked if user initiated close)
 */
void
relhangup(Conv *c, Reliable *r)
{
	int n;
	Block *bp;
	char hup[ERRMAX];

	n = snprint(hup, sizeof(hup), "hangup %I!%d", r->addr, r->port);
	qproduce(c->eq, hup, n);

	/*
	 *  dump any unacked outgoing messages
	 */
	for(bp = r->unacked; bp != nil; bp = r->unacked){
		r->unacked = bp->list;
		bp->list = nil;
		freeb(bp);
	}

	r->rcvgen = 0;
	r->rcvseq = 0;
	r->acksent = 0;
	if(generation == Hangupgen)
		generation++;
	r->sndgen = generation++;
	r->sndseq = 0;
	r->ackrcvd = 0;
	r->xmits = 0;
	r->timeout = 0;
	wakeup(&r->vous);
}

/*
 *  called with ucb locked
 */
void
relrexmit(Conv *c, Reliable *r)
{
	Rudppriv *upriv;
	Block *np;
	Fs *f;

	upriv = c->p->priv;
	f = c->p->f;
	r->timeout = 0;
	if(r->xmits++ > Rudpmaxxmit){
		relhangup(c, r);
		return;
	}

	upriv->rxmits++;
	np = copyblock(r->unacked, blocklen(r->unacked));
	DPRINT("rxmit r->ackrvcd+1 = %lud\n", r->ackrcvd+1);
	doipoput(c, f, np, 0, c->ttl, c->tos);
}
