#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"ip.h"

#define DPRINT if(0)print

enum
{
	UDP_PHDRSIZE	= 12,
	UDP_HDRSIZE	= 20,
	UDP_IPHDR	= 8,
	IP_UDPPROTO	= 17,
	UDP_USEAD6	= 36,
	UDP_USEAD4	= 12,

	Udprxms		= 200,
	Udptickms	= 100,
	Udpmaxxmit	= 10,
};

typedef struct Udphdr Udphdr;
struct Udphdr
{
	/* ip header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
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

/* MIB II counters */
typedef struct Udpstats Udpstats;
struct Udpstats
{
	ulong	udpInDatagrams;
	ulong	udpNoPorts;
	ulong	udpInErrors;
	ulong	udpOutDatagrams;
};

typedef struct Udppriv Udppriv;
struct Udppriv
{
	Ipht		ht;

	/* MIB counters */
	Udpstats	ustats;

	/* non-MIB stats */
	ulong		csumerr;		/* checksum errors */
	ulong		lenerr;			/* short packet */
};

/*
 *  protocol specific part of Conv
 */
typedef struct Udpcb Udpcb;
struct Udpcb
{
	QLock;
	uchar	headers;
};

static char*
udpconnect(Conv *c, char **argv, int argc)
{
	char *e;
	Udppriv *upriv;

	upriv = c->p->priv;
	e = Fsstdconnect(c, argv, argc);
	Fsconnected(c, e);
	iphtadd(&upriv->ht, c);

	return e;
}


static int
udpstate(Conv *c, char *state, int n)
{
	return snprint(state, n, "%s", c->inuse?"Open":"Closed");
}

static char*
udpannounce(Conv *c, char** argv, int argc)
{
	char *e;
	Udppriv *upriv;

	upriv = c->p->priv;
	e = Fsstdannounce(c, argv, argc);
	if(e != nil)
		return e;
	Fsconnected(c, nil);
	iphtadd(&upriv->ht, c);

	return nil;
}

static void
udpcreate(Conv *c)
{
	c->rq = qopen(64*1024, 1, 0, 0);
	c->wq = qopen(64*1024, 0, 0, 0);
}

static void
udpclose(Conv *c)
{
	Udpcb *ucb;
	Udppriv *upriv;

	upriv = c->p->priv;
	iphtrem(&upriv->ht, c);

	c->state = 0;
	qclose(c->rq);
	qclose(c->wq);
	qclose(c->eq);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->lport = 0;
	c->rport = 0;

	ucb = (Udpcb*)c->ptcl;
	ucb->headers = 0;
}

void
udpkick(Conv *c)
{
	Udphdr *uh;
	ushort rport;
	uchar laddr[IPaddrlen], raddr[IPaddrlen];
	Block *bp;
	Udpcb *ucb;
	int dlen, ptcllen;
	Udppriv *upriv;
	Fs *f;

	upriv = c->p->priv;
	f = c->p->f;

	netlog(c->p->f, Logudp, "udp: kick\n");
	bp = qget(c->wq);
	if(bp == nil)
		return;

	ucb = (Udpcb*)c->ptcl;
	switch(ucb->headers) {
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
		bp->rp += 2+2;			/* Igonore local port */
		break;
	case 4:
		bp = pullupblock(bp, UDP_USEAD4);
		if(bp == nil)
			return;
		v4tov6(raddr, bp->rp);
		bp->rp += IPv4addrlen;
		v4tov6(laddr, bp->rp);
		bp->rp += IPv4addrlen;
		if(ipforme(f, laddr) != Runi)
			findlocalip(f, laddr, raddr);
		rport = nhgets(bp->rp);
		bp->rp += 2+2;
		break;
	default:
		rport = 0;
		break;
	}

	dlen = blocklen(bp);

	/* Make space to fit udp & ip header */
	bp = padblock(bp, UDP_IPHDR+UDP_HDRSIZE);
	if(bp == nil)
		return;

	uh = (Udphdr *)(bp->rp);

	ptcllen = dlen + (UDP_HDRSIZE-UDP_PHDRSIZE);
	uh->Unused = 0;
	uh->udpproto = IP_UDPPROTO;
	uh->frag[0] = 0;
	uh->frag[1] = 0;
	hnputs(uh->udpplen, ptcllen);
	switch(ucb->headers){
	case 4:
	case 6:
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

	hnputs(uh->udpcksum, ptclcsum(bp, UDP_IPHDR, dlen+UDP_HDRSIZE));

	upriv->ustats.udpOutDatagrams++;
	ipoput(f, bp, 0, c->ttl, c->tos);
}

Conv*
udpincoming(Conv *c, uchar *raddr, ushort rport, uchar *laddr, ushort lport)
{
	Conv *new;

	new = Fsnewcall(c, raddr, rport, laddr, lport);
	if(new == nil)
		return nil;
}

void
udpiput(Proto *udp, Ipifc *ifc, Block *bp)
{
	int len, olen, ottl;
	Udphdr *uh;
	Conv *c;
	Udpcb *ucb;
	uchar raddr[IPaddrlen], laddr[IPaddrlen];
	ushort rport, lport;
	Udppriv *upriv;
	Fs *f;

	upriv = udp->priv;
	f = udp->f;

	upriv->ustats.udpInDatagrams++;

	uh = (Udphdr*)(bp->rp);

	/* Put back pseudo header for checksum (remember old values for icmpnoconv()) */
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
			upriv->ustats.udpInErrors++;
			netlog(f, Logudp, "udp: checksum error %I\n", raddr);
			DPRINT("udp: checksum error %I\n", raddr);
			freeblist(bp);
			return;
		}
	}

	qlock(udp);

	c = iphtlook(&upriv->ht, raddr, rport, laddr, lport);
	if(c == nil){
		/* no converstation found */
		upriv->ustats.udpNoPorts++;
		qunlock(udp);
		netlog(f, Logudp, "udp: no conv %I!%d -> %I!%d\n", raddr, rport,
			laddr, lport);
		uh->Unused = ottl;
		hnputs(uh->udpplen, olen);
		icmpnoconv(f, bp);
		freeblist(bp);
		return;
	}
	ucb = (Udpcb*)c->ptcl;

	if(c->state == Announced){
		if(ucb->headers == 0){
			/* create a new conversation */
			if(ipforme(f, laddr) != Runi)
				v4tov6(laddr, ifc->lifc->local);
			c = Fsnewcall(c, raddr, rport, laddr, lport);
			if(c == nil){
				qunlock(udp);
				freeblist(bp);
				return;
			}
			iphtadd(&upriv->ht, c);
			ucb = (Udpcb*)c->ptcl;
		}
	}

	qlock(c);
	qunlock(udp);

	/*
	 * Trim the packet down to data size
	 */
	len -= (UDP_HDRSIZE-UDP_PHDRSIZE);
	bp = trimblock(bp, UDP_IPHDR+UDP_HDRSIZE, len);
	if(bp == nil){
		qunlock(c);
		netlog(f, Logudp, "udp: len err %I.%d -> %I.%d\n", raddr, rport,
			laddr, lport);
		upriv->lenerr++;
		return;
	}

	netlog(f, Logudpmsg, "udp: %I.%d -> %I.%d l %d\n", raddr, rport,
		laddr, lport, len);

	switch(ucb->headers){
	case 6:
		/* pass the src address */
		bp = padblock(bp, UDP_USEAD6);
		ipmove(bp->rp, raddr);
		if(ipforme(f, laddr) == Runi)
			ipmove(bp->rp+IPaddrlen, laddr);
		else
			ipmove(bp->rp+IPaddrlen, ifc->lifc->local);
		hnputs(bp->rp+2*IPaddrlen, rport);
		hnputs(bp->rp+2*IPaddrlen+2, lport);
		break;
	case 4:
		/* pass the src address */
		bp = padblock(bp, UDP_USEAD4);
		v6tov4(bp->rp, raddr);
		if(ipforme(f, laddr) == Runi)
			v6tov4(bp->rp+IPv4addrlen, laddr);
		else
			v6tov4(bp->rp+IPv4addrlen, ifc->lifc->local);
		hnputs(bp->rp + 2*IPv4addrlen, rport);
		hnputs(bp->rp + 2*IPv4addrlen + 2, lport);
		break;
	}

	if(bp->next)
		bp = concatblock(bp);

	if(qfull(c->rq)){
		qunlock(c);
		netlog(f, Logudp, "udp: qfull %I.%d -> %I.%d\n", raddr, rport,
			laddr, lport);
		freeblist(bp);
		return;
	}

	qpass(c->rq, bp);
	qunlock(c);

}

char*
udpctl(Conv *c, char **f, int n)
{
	Udpcb *ucb;

	ucb = (Udpcb*)c->ptcl;
	if(n == 1){
		if(strcmp(f[0], "headers4") == 0){
			ucb->headers = 4;
			return nil;
		} else if(strcmp(f[0], "headers") == 0){
			ucb->headers = 6;
			return nil;
		}
	}
	return "unknown control request";
}

void
udpadvise(Proto *udp, Block *bp, char *msg)
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
	qlock(udp);
	for(p = udp->conv; *p; p++) {
		s = *p;
		if(s->rport == pdest)
		if(s->lport == psource)
		if(ipcmp(s->raddr, dest) == 0)
		if(ipcmp(s->laddr, source) == 0){
			qlock(s);
			qunlock(udp);
			qhangup(s->rq, msg);
			qhangup(s->wq, msg);
			qunlock(s);
			freeblist(bp);
			return;
		}
	}
	qunlock(udp);
	freeblist(bp);
}

int
udpstats(Proto *udp, char *buf, int len)
{
	Udppriv *upriv;

	upriv = udp->priv;
	return snprint(buf, len, "InDatagrams: %lud\nNoPorts: %lud\nInErrors: %lud\nOutDatagrams: %lud\n",
		upriv->ustats.udpInDatagrams,
		upriv->ustats.udpNoPorts,
		upriv->ustats.udpInErrors,
		upriv->ustats.udpOutDatagrams);
}

void
udpinit(Fs *fs)
{
	Proto *udp;

	udp = smalloc(sizeof(Proto));
	udp->priv = smalloc(sizeof(Udppriv));
	udp->name = "udp";
	udp->kick = udpkick;
	udp->connect = udpconnect;
	udp->announce = udpannounce;
	udp->ctl = udpctl;
	udp->state = udpstate;
	udp->create = udpcreate;
	udp->close = udpclose;
	udp->rcv = udpiput;
	udp->advise = udpadvise;
	udp->stats = udpstats;
	udp->ipproto = IP_UDPPROTO;
	udp->nc = Nchans;
	udp->ptclsize = sizeof(Udpcb);

	Fsproto(fs, udp);
}
