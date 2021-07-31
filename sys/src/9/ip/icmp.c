#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

typedef struct Icmp {
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	ipcksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
} Icmp;

enum {			/* Packet Types */
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	Redirect	= 5,
	EchoRequest	= 8,
	TimeExceed	= 11,
	InParmProblem	= 12,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,
	AddrMaskRequest = 17,
	AddrMaskReply   = 18,

	Maxtype		= 18,
};

enum
{
	MinAdvise	= 24,	/* minimum needed for us to advise another protocol */ 
};

char *icmpnames[Maxtype+1] =
{
[EchoReply]		"EchoReply",
[Unreachable]		"Unreachable",
[SrcQuench]		"SrcQuench",
[Redirect]		"Redirect",
[EchoRequest]		"EchoRequest",
[TimeExceed]		"TimeExceed",
[InParmProblem]		"InParmProblem",
[Timestamp]		"Timestamp",
[TimestampReply]	"TimestampReply",
[InfoRequest]		"InfoRequest",
[InfoReply]		"InfoReply",
[AddrMaskRequest]	"AddrMaskRequest",
[AddrMaskReply  ]	"AddrMaskReply  ",
};

enum {
	IP_ICMPPROTO	= 1,
	ICMP_IPSIZE	= 20,
	ICMP_HDRSIZE	= 8,
};

enum
{
	InMsgs,
	InErrors,
	OutMsgs,
	CsumErrs,
	LenErrs,
	HlenErrs,

	Nstats,
};

static char *statnames[Nstats] =
{
[InMsgs]	"InMsgs",
[InErrors]	"InErrors",
[OutMsgs]	"OutMsgs",
[CsumErrs]	"CsumErrs",
[LenErrs]	"LenErrs",
[HlenErrs]	"HlenErrs",
};

typedef struct Icmppriv Icmppriv;
struct Icmppriv
{
	ulong	stats[Nstats];

	/* message counts */
	ulong	in[Maxtype+1];
	ulong	out[Maxtype+1];
};

static void icmpkick(void *x, Block*);

static void
icmpcreate(Conv *c)
{
	c->rq = qopen(64*1024, Qmsg, 0, c);
	c->wq = qbypass(icmpkick, c);
}

extern char*
icmpconnect(Conv *c, char **argv, int argc)
{
	char *e;

	e = Fsstdconnect(c, argv, argc);
	if(e != nil)
		return e;
	Fsconnected(c, e);

	return nil;
}

extern int
icmpstate(Conv *c, char *state, int n)
{
	USED(c);
	return snprint(state, n, "%s qin %d qout %d",
		"Datagram",
		c->rq ? qlen(c->rq) : 0,
		c->wq ? qlen(c->wq) : 0
	);
}

extern char*
icmpannounce(Conv *c, char **argv, int argc)
{
	char *e;

	e = Fsstdannounce(c, argv, argc);
	if(e != nil)
		return e;
	Fsconnected(c, nil);

	return nil;
}

extern void
icmpclose(Conv *c)
{
	qclose(c->rq);
	qclose(c->wq);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->lport = 0;
}

static void
icmpkick(void *x, Block *bp)
{
	Conv *c = x;
	Icmp *p;
	Icmppriv *ipriv;

	if(bp == nil)
		return;

	if(blocklen(bp) < ICMP_IPSIZE + ICMP_HDRSIZE){
		freeblist(bp);
		return;
	}
	p = (Icmp *)(bp->rp);
	p->vihl = IP_VER4;
	ipriv = c->p->priv;
	if(p->type <= Maxtype)	
		ipriv->out[p->type]++;
	
	v6tov4(p->dst, c->raddr);
	v6tov4(p->src, c->laddr);
	p->proto = IP_ICMPPROTO;
	hnputs(p->icmpid, c->lport);
	memset(p->cksum, 0, sizeof(p->cksum));
	hnputs(p->cksum, ptclcsum(bp, ICMP_IPSIZE, blocklen(bp) - ICMP_IPSIZE));
	ipriv->stats[OutMsgs]++;
	ipoput4(c->p->f, bp, 0, c->ttl, c->tos, nil);
}

extern void
icmpttlexceeded(Fs *f, uchar *ia, Block *bp)
{
	Block	*nbp;
	Icmp	*p, *np;

	p = (Icmp *)bp->rp;

	netlog(f, Logicmp, "sending icmpttlexceeded -> %V\n", p->src);
	nbp = allocb(ICMP_IPSIZE + ICMP_HDRSIZE + ICMP_IPSIZE + 8);
	nbp->wp += ICMP_IPSIZE + ICMP_HDRSIZE + ICMP_IPSIZE + 8;
	np = (Icmp *)nbp->rp;
	np->vihl = IP_VER4;
	memmove(np->dst, p->src, sizeof(np->dst));
	v6tov4(np->src, ia);
	memmove(np->data, bp->rp, ICMP_IPSIZE + 8);
	np->type = TimeExceed;
	np->code = 0;
	np->proto = IP_ICMPPROTO;
	hnputs(np->icmpid, 0);
	hnputs(np->seq, 0);
	memset(np->cksum, 0, sizeof(np->cksum));
	hnputs(np->cksum, ptclcsum(nbp, ICMP_IPSIZE, blocklen(nbp) - ICMP_IPSIZE));
	ipoput4(f, nbp, 0, MAXTTL, DFLTTOS, nil);

}

static void
icmpunreachable(Fs *f, Block *bp, int code, int seq)
{
	Block	*nbp;
	Icmp	*p, *np;
	int	i;
	uchar	addr[IPaddrlen];

	p = (Icmp *)bp->rp;

	/* only do this for unicast sources and destinations */
	v4tov6(addr, p->dst);
	i = ipforme(f, addr);
	if((i&Runi) == 0)
		return;
	v4tov6(addr, p->src);
	i = ipforme(f, addr);
	if(i != 0 && (i&Runi) == 0)
		return;

	netlog(f, Logicmp, "sending icmpnoconv -> %V\n", p->src);
	nbp = allocb(ICMP_IPSIZE + ICMP_HDRSIZE + ICMP_IPSIZE + 8);
	nbp->wp += ICMP_IPSIZE + ICMP_HDRSIZE + ICMP_IPSIZE + 8;
	np = (Icmp *)nbp->rp;
	np->vihl = IP_VER4;
	memmove(np->dst, p->src, sizeof(np->dst));
	memmove(np->src, p->dst, sizeof(np->src));
	memmove(np->data, bp->rp, ICMP_IPSIZE + 8);
	np->type = Unreachable;
	np->code = code;
	np->proto = IP_ICMPPROTO;
	hnputs(np->icmpid, 0);
	hnputs(np->seq, seq);
	memset(np->cksum, 0, sizeof(np->cksum));
	hnputs(np->cksum, ptclcsum(nbp, ICMP_IPSIZE, blocklen(nbp) - ICMP_IPSIZE));
	ipoput4(f, nbp, 0, MAXTTL, DFLTTOS, nil);
}

extern void
icmpnoconv(Fs *f, Block *bp)
{
	icmpunreachable(f, bp, 3, 0);
}

extern void
icmpcantfrag(Fs *f, Block *bp, int mtu)
{
	icmpunreachable(f, bp, 4, mtu);
}

static void
goticmpkt(Proto *icmp, Block *bp)
{
	Conv	**c, *s;
	Icmp	*p;
	uchar	dst[IPaddrlen];
	ushort	recid;

	p = (Icmp *) bp->rp;
	v4tov6(dst, p->src);
	recid = nhgets(p->icmpid);

	for(c = icmp->conv; *c; c++) {
		s = *c;
		if(s->lport == recid)
		if(ipcmp(s->raddr, dst) == 0){
			bp = concatblock(bp);
			if(bp != nil)
				qpass(s->rq, bp);
			return;
		}
	}
	freeblist(bp);
}

static Block *
mkechoreply(Block *bp)
{
	Icmp	*q;
	uchar	ip[4];

	q = (Icmp *)bp->rp;
	q->vihl = IP_VER4;
	memmove(ip, q->src, sizeof(q->dst));
	memmove(q->src, q->dst, sizeof(q->src));
	memmove(q->dst, ip,  sizeof(q->dst));
	q->type = EchoReply;
	memset(q->cksum, 0, sizeof(q->cksum));
	hnputs(q->cksum, ptclcsum(bp, ICMP_IPSIZE, blocklen(bp) - ICMP_IPSIZE));

	return bp;
}

static char *unreachcode[] =
{
[0]	"net unreachable",
[1]	"host unreachable",
[2]	"protocol unreachable",
[3]	"port unreachable",
[4]	"fragmentation needed and DF set",
[5]	"source route failed",
};

static void
icmpiput(Proto *icmp, Ipifc*, Block *bp)
{
	int	n, iplen;
	Icmp	*p;
	Block	*r;
	Proto	*pr;
	char	*msg;
	char	m2[128];
	Icmppriv *ipriv;

	ipriv = icmp->priv;
	
	ipriv->stats[InMsgs]++;

	p = (Icmp *)bp->rp;
	netlog(icmp->f, Logicmp, "icmpiput %d %d\n", p->type, p->code);
	n = blocklen(bp);
	if(n < ICMP_IPSIZE+ICMP_HDRSIZE){
		ipriv->stats[InErrors]++;
		ipriv->stats[HlenErrs]++;
		netlog(icmp->f, Logicmp, "icmp hlen %d\n", n);
		goto raise;
	}
	iplen = nhgets(p->length);
	if(iplen > n || (iplen % 1)){
		ipriv->stats[LenErrs]++;
		ipriv->stats[InErrors]++;
		netlog(icmp->f, Logicmp, "icmp length %d\n", iplen);
		goto raise;
	}
	if(ptclcsum(bp, ICMP_IPSIZE, iplen - ICMP_IPSIZE)){
		ipriv->stats[InErrors]++;
		ipriv->stats[CsumErrs]++;
		netlog(icmp->f, Logicmp, "icmp checksum error\n");
		goto raise;
	}
	if(p->type <= Maxtype)
		ipriv->in[p->type]++;

	switch(p->type) {
	case EchoRequest:
		if (iplen < n)
			bp = trimblock(bp, 0, iplen);
		r = mkechoreply(bp);
		ipriv->out[EchoReply]++;
		ipoput4(icmp->f, r, 0, MAXTTL, DFLTTOS, nil);
		break;
	case Unreachable:
		if(p->code > 5)
			msg = unreachcode[1];
		else
			msg = unreachcode[p->code];

		bp->rp += ICMP_IPSIZE+ICMP_HDRSIZE;
		if(blocklen(bp) < MinAdvise){
			ipriv->stats[LenErrs]++;
			goto raise;
		}
		p = (Icmp *)bp->rp;
		pr = Fsrcvpcolx(icmp->f, p->proto);
		if(pr != nil && pr->advise != nil) {
			(*pr->advise)(pr, bp, msg);
			return;
		}

		bp->rp -= ICMP_IPSIZE+ICMP_HDRSIZE;
		goticmpkt(icmp, bp);
		break;
	case TimeExceed:
		if(p->code == 0){
			sprint(m2, "ttl exceeded at %V", p->src);

			bp->rp += ICMP_IPSIZE+ICMP_HDRSIZE;
			if(blocklen(bp) < MinAdvise){
				ipriv->stats[LenErrs]++;
				goto raise;
			}
			p = (Icmp *)bp->rp;
			pr = Fsrcvpcolx(icmp->f, p->proto);
			if(pr != nil && pr->advise != nil) {
				(*pr->advise)(pr, bp, m2);
				return;
			}
			bp->rp -= ICMP_IPSIZE+ICMP_HDRSIZE;
		}

		goticmpkt(icmp, bp);
		break;
	default:
		goticmpkt(icmp, bp);
		break;
	}
	return;

raise:
	freeblist(bp);
}

void
icmpadvise(Proto *icmp, Block *bp, char *msg)
{
	Conv	**c, *s;
	Icmp	*p;
	uchar	dst[IPaddrlen];
	ushort	recid;

	p = (Icmp *) bp->rp;
	v4tov6(dst, p->dst);
	recid = nhgets(p->icmpid);

	for(c = icmp->conv; *c; c++) {
		s = *c;
		if(s->lport == recid)
		if(ipcmp(s->raddr, dst) == 0){
			qhangup(s->rq, msg);
			qhangup(s->wq, msg);
			break;
		}
	}
	freeblist(bp);
}

int
icmpstats(Proto *icmp, char *buf, int len)
{
	Icmppriv *priv;
	char *p, *e;
	int i;

	priv = icmp->priv;
	p = buf;
	e = p+len;
	for(i = 0; i < Nstats; i++)
		p = seprint(p, e, "%s: %lud\n", statnames[i], priv->stats[i]);
	for(i = 0; i <= Maxtype; i++){
		if(icmpnames[i])
			p = seprint(p, e, "%s: %lud %lud\n", icmpnames[i], priv->in[i], priv->out[i]);
		else
			p = seprint(p, e, "%d: %lud %lud\n", i, priv->in[i], priv->out[i]);
	}
	return p - buf;
}
	
void
icmpinit(Fs *fs)
{
	Proto *icmp;

	icmp = smalloc(sizeof(Proto));
	icmp->priv = smalloc(sizeof(Icmppriv));
	icmp->name = "icmp";
	icmp->connect = icmpconnect;
	icmp->announce = icmpannounce;
	icmp->state = icmpstate;
	icmp->create = icmpcreate;
	icmp->close = icmpclose;
	icmp->rcv = icmpiput;
	icmp->stats = icmpstats;
	icmp->ctl = nil;
	icmp->advise = icmpadvise;
	icmp->gc = nil;
	icmp->ipproto = IP_ICMPPROTO;
	icmp->nc = 128;
	icmp->ptclsize = 0;

	Fsproto(fs, icmp);
}
