/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Internet Control Message Protocol for IPv6
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "ip.h"
#include "ipv6.h"

enum
{
	InMsgs6,
	InErrors6,
	OutMsgs6,
	CsumErrs6,
	LenErrs6,
	HlenErrs6,
	HoplimErrs6,
	IcmpCodeErrs6,
	TargetErrs6,
	OptlenErrs6,
	AddrmxpErrs6,
	RouterAddrErrs6,

	Nstats6,
};

enum {
	ICMP_USEAD6	= 40,
};

enum {
	Oflag	= 1<<5,
	Sflag	= 1<<6,
	Rflag	= 1<<7,
};

enum {
	/* ICMPv6 types */
	EchoReply	= 0,
	UnreachableV6	= 1,
	PacketTooBigV6	= 2,
	TimeExceedV6	= 3,
	SrcQuench	= 4,
	ParamProblemV6	= 4,
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
	EchoRequestV6	= 128,
	EchoReplyV6	= 129,
	RouterSolicit	= 133,
	RouterAdvert	= 134,
	NbrSolicit	= 135,
	NbrAdvert	= 136,
	RedirectV6	= 137,

	Maxtype6	= 137,
};

/* on-the-wire packet formats */
typedef struct IPICMP IPICMP;
typedef struct Ndpkt Ndpkt;
typedef struct NdiscC NdiscC;

/* we do this to avoid possible struct padding  */
#define ICMPHDR \
	IPV6HDR; \
	uint8_t	type; \
	uint8_t	code; \
	uint8_t	cksum[2]; \
	uint8_t	icmpid[2]; \
	uint8_t	seq[2]

struct IPICMP {
	ICMPHDR;
	uint8_t	payload[];
};

#define IPICMPSZ offsetof(IPICMP, payload[0])

struct NdiscC {
	ICMPHDR;
	uint8_t	target[IPaddrlen];
	uint8_t	payload[];
};

#define NDISCSZ offsetof(NdiscC, payload[0])

struct Ndpkt {
	ICMPHDR;
	uint8_t	target[IPaddrlen];
	uint8_t	otype;
	uint8_t	olen;		/* length in units of 8 octets(incl type, code),
				 * 1 for IEEE 802 addresses */
	uint8_t	lnaddr[6];	/* link-layer address */
	uint8_t	payload[];
};

#define NDPKTSZ offsetof(Ndpkt, payload[0])

typedef struct Icmppriv6
{
	uint32_t	stats[Nstats6];

	/* message counts */
	uint32_t	in[Maxtype6+1];
	uint32_t	out[Maxtype6+1];
} Icmppriv6;

typedef struct Icmpcb6
{
	QLock ql;
	uint8_t	headers;
} Icmpcb6;

char *icmpnames6[Maxtype6+1] =
{
[EchoReply] = "EchoReply",
[UnreachableV6] = "UnreachableV6",
[PacketTooBigV6] = "PacketTooBigV6",
[TimeExceedV6] = "TimeExceedV6",
[SrcQuench] = "SrcQuench",
[Redirect] = "Redirect",
[EchoRequest] = "EchoRequest",
[TimeExceed] = "TimeExceed",
[InParmProblem] = "InParmProblem",
[Timestamp] = "Timestamp",
[TimestampReply] = "TimestampReply",
[InfoRequest] = "InfoRequest",
[InfoReply] = "InfoReply",
[AddrMaskRequest] = "AddrMaskRequest",
[AddrMaskReply] = "AddrMaskReply",
[EchoRequestV6] = "EchoRequestV6",
[EchoReplyV6] = "EchoReplyV6",
[RouterSolicit] = "RouterSolicit",
[RouterAdvert] = "RouterAdvert",
[NbrSolicit] = "NbrSolicit",
[NbrAdvert] = "NbrAdvert",
[RedirectV6] = "RedirectV6",
};

static char *statnames6[Nstats6] =
{
[InMsgs6] = "InMsgs",
[InErrors6] = "InErrors",
[OutMsgs6] = "OutMsgs",
[CsumErrs6] = "CsumErrs",
[LenErrs6] = "LenErrs",
[HlenErrs6] = "HlenErrs",
[HoplimErrs6] = "HoplimErrs",
[IcmpCodeErrs6] = "IcmpCodeErrs",
[TargetErrs6] = "TargetErrs",
[OptlenErrs6] = "OptlenErrs",
[AddrmxpErrs6] = "AddrmxpErrs",
[RouterAddrErrs6] = "RouterAddrErrs",
};

static char *unreachcode[] =
{
[Icmp6_no_route] = "no route to destination",
[Icmp6_ad_prohib] = "comm with destination administratively prohibited",
[Icmp6_out_src_scope] = "beyond scope of source address",
[Icmp6_adr_unreach] = "address unreachable",
[Icmp6_port_unreach] = "port unreachable",
[Icmp6_gress_src_fail] = "source address failed ingress/egress policy",
[Icmp6_rej_route] = "reject route to destination",
[Icmp6_unknown] = "icmp unreachable: unknown code",
};

static void icmpkick6(void *x, Block *bp);

static void
icmpcreate6(Conv *c)
{
	c->rq = qopen(64*1024, Qmsg, 0, c);
	c->wq = qbypass(icmpkick6, c);
}

static void
set_cksum(Block *bp)
{
	IPICMP *p = (IPICMP *)(bp->rp);

	hnputl(p->vcf, 0);  	/* borrow IP header as pseudoheader */
	hnputs(p->ploadlen, blocklen(bp) - IP6HDR);
	p->proto = 0;
	p->ttl = ICMPv6;	/* ttl gets set later */
	hnputs(p->cksum, 0);
	hnputs(p->cksum, ptclcsum(bp, 0, blocklen(bp)));
	p->proto = ICMPv6;
}

static Block *
newIPICMP(int packetlen)
{
	Block *nbp;

	nbp = allocb(packetlen);
	nbp->wp += packetlen;
	memset(nbp->rp, 0, packetlen);
	return nbp;
}

void
icmpadvise6(Proto *icmp, Block *bp, char *msg)
{
	uint16_t recid;
	Conv **c, *s;
	IPICMP *p;

	p = (IPICMP *)bp->rp;
	recid = nhgets(p->icmpid);

	for(c = icmp->conv; *c; c++) {
		s = *c;
		if(s->lport == recid && ipcmp(s->raddr, p->dst) == 0){
			qhangup(s->rq, msg);
			qhangup(s->wq, msg);
			break;
		}
	}
	freeblist(bp);
}

static void
icmpkick6(void *x, Block *bp)
{
	uint8_t laddr[IPaddrlen], raddr[IPaddrlen];
	Conv *c = x;
	IPICMP *p;
	Icmppriv6 *ipriv = c->p->priv;
	Icmpcb6 *icb = (Icmpcb6*)c->ptcl;

	if(bp == nil)
		return;

	if(icb->headers==6) {
		/* get user specified addresses */
		bp = pullupblock(bp, ICMP_USEAD6);
		if(bp == nil)
			return;
		bp->rp += 8;
		ipmove(laddr, bp->rp);
		bp->rp += IPaddrlen;
		ipmove(raddr, bp->rp);
		bp->rp += IPaddrlen;
		bp = padblock(bp, IP6HDR);
	}

	if(blocklen(bp) < IPICMPSZ){
		freeblist(bp);
		return;
	}
	p = (IPICMP *)(bp->rp);
	if(icb->headers == 6) {
		ipmove(p->dst, raddr);
		ipmove(p->src, laddr);
	} else {
		ipmove(p->dst, c->raddr);
		ipmove(p->src, c->laddr);
		hnputs(p->icmpid, c->lport);
	}

	set_cksum(bp);
	p->vcf[0] = 0x06 << 4;
	if(p->type <= Maxtype6)
		ipriv->out[p->type]++;
	ipoput6(c->p->f, bp, 0, c->ttl, c->tos, nil);
}

char*
icmpctl6(Conv *c, char **argv, int argc)
{
	Icmpcb6 *icb;

	icb = (Icmpcb6*) c->ptcl;
	if(argc==1 && strcmp(argv[0], "headers")==0) {
		icb->headers = 6;
		return nil;
	}
	return "unknown control request";
}

static void
goticmpkt6(Proto *icmp, Block *bp, int muxkey)
{
	uint16_t recid;
	uint8_t *addr;
	Conv **c, *s;
	IPICMP *p = (IPICMP *)bp->rp;

	if(muxkey == 0) {
		recid = nhgets(p->icmpid);
		addr = p->src;
	} else {
		recid = muxkey;
		addr = p->dst;
	}

	for(c = icmp->conv; *c; c++){
		s = *c;
		if(s->lport == recid && ipcmp(s->raddr, addr) == 0){
			bp = concatblock(bp);
			if(bp != nil)
				qpass(s->rq, bp);
			return;
		}
	}

	freeblist(bp);
}

static Block *
mkechoreply6(Block *bp, Ipifc *ifc)
{
	uint8_t addr[IPaddrlen];
	IPICMP *p = (IPICMP *)(bp->rp);

	ipmove(addr, p->src);
	if(!isv6mcast(p->dst))
		ipmove(p->src, p->dst);
	else if (!ipv6anylocal(ifc, p->src))
		return nil;
	ipmove(p->dst, addr);
	p->type = EchoReplyV6;
	set_cksum(bp);
	return bp;
}

/*
 * sends out an ICMPv6 neighbor solicitation
 * 	suni == SRC_UNSPEC or SRC_UNI,
 *	tuni == TARG_MULTI => multicast for address resolution,
 * 	and tuni == TARG_UNI => neighbor reachability.
 */
extern void
icmpns(Fs *f, uint8_t* src, int suni, uint8_t* targ, int tuni, uint8_t* mac)
{
	Block *nbp;
	Ndpkt *np;
	Proto *icmp = f->t2p[ICMPv6];
	Icmppriv6 *ipriv = icmp->priv;

	nbp = newIPICMP(NDPKTSZ);
	np = (Ndpkt*) nbp->rp;

	if(suni == SRC_UNSPEC)
		memmove(np->src, v6Unspecified, IPaddrlen);
	else
		memmove(np->src, src, IPaddrlen);

	if(tuni == TARG_UNI)
		memmove(np->dst, targ, IPaddrlen);
	else
		ipv62smcast(np->dst, targ);

	np->type = NbrSolicit;
	np->code = 0;
	memmove(np->target, targ, IPaddrlen);
	if(suni != SRC_UNSPEC) {
		np->otype = SRC_LLADDR;
		np->olen = 1;		/* 1+1+6 = 8 = 1 8-octet */
		memmove(np->lnaddr, mac, sizeof(np->lnaddr));
	} else
		nbp->wp -= NDPKTSZ - NDISCSZ;

	set_cksum(nbp);
	np = (Ndpkt*)nbp->rp;
	np->ttl = HOP_LIMIT;
	np->vcf[0] = 0x06 << 4;
	ipriv->out[NbrSolicit]++;
	netlog(f, Logicmp, "sending neighbor solicitation %I\n", targ);
	ipoput6(f, nbp, 0, MAXTTL, DFLTTOS, nil);
}

/*
 * sends out an ICMPv6 neighbor advertisement. pktflags == RSO flags.
 */
extern void
icmpna(Fs *f, uint8_t* src, uint8_t* dst, uint8_t* targ, uint8_t* mac, uint8_t flags)
{
	Block *nbp;
	Ndpkt *np;
	Proto *icmp = f->t2p[ICMPv6];
	Icmppriv6 *ipriv = icmp->priv;

	nbp = newIPICMP(NDPKTSZ);
	np = (Ndpkt*)nbp->rp;

	memmove(np->src, src, IPaddrlen);
	memmove(np->dst, dst, IPaddrlen);

	np->type = NbrAdvert;
	np->code = 0;
	np->icmpid[0] = flags;
	memmove(np->target, targ, IPaddrlen);

	np->otype = TARGET_LLADDR;
	np->olen = 1;
	memmove(np->lnaddr, mac, sizeof(np->lnaddr));

	set_cksum(nbp);
	np = (Ndpkt*) nbp->rp;
	np->ttl = HOP_LIMIT;
	np->vcf[0] = 0x06 << 4;
	ipriv->out[NbrAdvert]++;
	netlog(f, Logicmp, "sending neighbor advertisement %I\n", src);
	ipoput6(f, nbp, 0, MAXTTL, DFLTTOS, nil);
}

/* if free is true, freeblist(bp) before return. */
extern void
icmphostunr(Fs *f, Ipifc *ifc, Block *bp, int code, int free)
{
	int osz, sz;
	Block *nbp;
	IPICMP *np;
	Icmppriv6 *ipriv;
	Ip6hdr *p;
	Proto *icmp;

	osz = BLEN(bp);
	sz = MIN(IPICMPSZ + osz, v6MINTU);
	icmp = f->t2p[ICMPv6];
	ipriv = icmp->priv;
	p = (Ip6hdr *)bp->rp;
	if(isv6mcast(p->src))
		goto freebl;
	nbp = newIPICMP(sz);
	np = (IPICMP *)nbp->rp;

	rlock(&ifc->rwl);
	if(!ipv6anylocal(ifc, np->src)){
		netlog(f, Logicmp, "icmphostunr fail -> src %I dst %I\n",
			p->src, p->dst);
		runlock(&ifc->rwl);
		freeblist(nbp);
		goto freebl;
	}

	netlog(f, Logicmp, "send icmphostunr -> src %I dst %I\n", p->src, p->dst);
	memmove(np->dst, p->src, IPaddrlen);
	np->type = UnreachableV6;
	np->code = code;
	memmove(nbp->rp + IPICMPSZ, bp->rp, sz - IPICMPSZ);
	set_cksum(nbp);
	np->ttl = HOP_LIMIT;
	np->vcf[0] = 0x06 << 4;
	ipriv->out[UnreachableV6]++;

	if(free)
		ipiput6(f, ifc, nbp);
	else
		ipoput6(f, nbp, 0, MAXTTL, DFLTTOS, nil);
	runlock(&ifc->rwl);
freebl:
	if(free)
		freeblist(bp);
}

extern void
icmpttlexceeded6(Fs *f, Ipifc *ifc, Block *bp)
{
	int osz = BLEN(bp);
	int sz = MIN(IPICMPSZ + osz, v6MINTU);
	Block *nbp;
	IPICMP *np;
	Ip6hdr *p;
	Proto *icmp = f->t2p[ICMPv6];
	Icmppriv6 *ipriv = icmp->priv;

	p = (Ip6hdr *)bp->rp;
	if(isv6mcast(p->src))
		return;

	nbp = newIPICMP(sz);
	np = (IPICMP *) nbp->rp;
	if(ipv6anylocal(ifc, np->src))
		netlog(f, Logicmp, "send icmpttlexceeded6 -> src %I dst %I\n",
			p->src, p->dst);
	else {
		netlog(f, Logicmp, "icmpttlexceeded6 fail -> src %I dst %I\n",
			p->src, p->dst);
		return;
	}

	memmove(np->dst, p->src, IPaddrlen);
	np->type = TimeExceedV6;
	np->code = 0;
	memmove(nbp->rp + IPICMPSZ, bp->rp, sz - IPICMPSZ);
	set_cksum(nbp);
	np->ttl = HOP_LIMIT;
	np->vcf[0] = 0x06 << 4;
	ipriv->out[TimeExceedV6]++;
	ipoput6(f, nbp, 0, MAXTTL, DFLTTOS, nil);
}

extern void
icmppkttoobig6(Fs *f, Ipifc *ifc, Block *bp)
{
	int osz = BLEN(bp);
	int sz = MIN(IPICMPSZ + osz, v6MINTU);
	Block *nbp;
	IPICMP *np;
	Ip6hdr *p;
	Proto *icmp = f->t2p[ICMPv6];
	Icmppriv6 *ipriv = icmp->priv;

	p = (Ip6hdr *)bp->rp;
	if(isv6mcast(p->src))
		return;

	nbp = newIPICMP(sz);
	np = (IPICMP *)nbp->rp;
	if(ipv6anylocal(ifc, np->src))
		netlog(f, Logicmp, "send icmppkttoobig6 -> src %I dst %I\n",
			p->src, p->dst);
	else {
		netlog(f, Logicmp, "icmppkttoobig6 fail -> src %I dst %I\n",
			p->src, p->dst);
		return;
	}

	memmove(np->dst, p->src, IPaddrlen);
	np->type = PacketTooBigV6;
	np->code = 0;
	hnputl(np->icmpid, ifc->maxtu - ifc->medium->hsize);
	memmove(nbp->rp + IPICMPSZ, bp->rp, sz - IPICMPSZ);
	set_cksum(nbp);
	np->ttl = HOP_LIMIT;
	np->vcf[0] = 0x06 << 4;
	ipriv->out[PacketTooBigV6]++;
	ipoput6(f, nbp, 0, MAXTTL, DFLTTOS, nil);
}

/*
 * RFC 2461, pages 39-40, pages 57-58.
 */
static int
valid(Proto *icmp, Ipifc *ipifc, Block *bp, Icmppriv6 *ipriv)
{
	int sz, osz, unsp, n, ttl, iplen, pktsz;
	uint8_t *packet;
	IPICMP *p;
	Ndpkt *np;

	n = blocklen(bp);
	if(n < IPICMPSZ) {
		ipriv->stats[HlenErrs6]++;
		netlog(icmp->f, Logicmp, "icmp hlen %d\n", n);
		goto err;
	}

	packet = bp->rp;
	p = (IPICMP *)packet;
	pktsz = BLEN(bp);
	iplen = nhgets(p->ploadlen);
	if(iplen > n - IP6HDR) {
		ipriv->stats[LenErrs6]++;
		netlog(icmp->f, Logicmp, "icmp length %d\n", iplen);
		goto err;
	}

	/* Rather than construct explicit pseudoheader, overwrite IPv6 header */
	if(p->proto != ICMPv6) {
		/* This code assumes no extension headers!!! */
		netlog(icmp->f, Logicmp, "icmp error: extension header\n");
		goto err;
	}
	memset(packet, 0, 4);
	ttl = p->ttl;
	p->ttl = p->proto;
	p->proto = 0;
	if(ptclcsum(bp, 0, iplen + IP6HDR)) {
		ipriv->stats[CsumErrs6]++;
		netlog(icmp->f, Logicmp, "icmp checksum error\n");
		goto err;
	}
	p->proto = p->ttl;
	p->ttl = ttl;

	/* additional tests for some pkt types */
	if (p->type != NbrSolicit   && p->type != NbrAdvert &&
	    p->type != RouterAdvert && p->type != RouterSolicit &&
	    p->type != RedirectV6)
		return 1;	/* TODO: unknown, presumed valid; why? */
	if(p->ttl != HOP_LIMIT) {
		ipriv->stats[HoplimErrs6]++;
		goto err;
	}
	if(p->code != 0) {
		ipriv->stats[IcmpCodeErrs6]++;
		goto err;
	}

	switch (p->type) {
	case NbrSolicit:
	case NbrAdvert:
		np = (Ndpkt*) p;
		if(isv6mcast(np->target)) {
			ipriv->stats[TargetErrs6]++;
			goto err;
		}
		if(optexsts(np) && np->olen == 0) {
			ipriv->stats[OptlenErrs6]++;
			goto err;
		}
		if (p->type == NbrSolicit && ipcmp(np->src, v6Unspecified) == 0)
			if(!issmcast(np->dst) || optexsts(np)) {
				ipriv->stats[AddrmxpErrs6]++;
				goto err;
			}
		if(p->type == NbrAdvert && isv6mcast(np->dst) &&
		    nhgets(np->icmpid) & Sflag){
			ipriv->stats[AddrmxpErrs6]++;
			goto err;
		}
		break;
	case RouterAdvert:
		if(pktsz - IP6HDR < 16) {
			ipriv->stats[HlenErrs6]++;
			goto err;
		}
		if(!islinklocal(p->src)) {
			ipriv->stats[RouterAddrErrs6]++;
			goto err;
		}
		for (sz = IPICMPSZ + 8; sz+1 < pktsz; sz += 8*osz) {
			osz = packet[sz+1];
			if(osz <= 0) {
				ipriv->stats[OptlenErrs6]++;
				goto err;
			}
		}
		break;
	case RouterSolicit:
		if(pktsz - IP6HDR < 8) {
			ipriv->stats[HlenErrs6]++;
			goto err;
		}
		unsp = (ipcmp(p->src, v6Unspecified) == 0);
		for (sz = IPICMPSZ + 8; sz+1 < pktsz; sz += 8*osz) {
			osz = packet[sz+1];
			if(osz <= 0 || (unsp && packet[sz] == SRC_LLADDR)) {
				ipriv->stats[OptlenErrs6]++;
				goto err;
			}
		}
		break;
	case RedirectV6:
		/* TODO: fill in */
		break;
	default:
		goto err;
	}
	return 1;
err:
	ipriv->stats[InErrors6]++;
	return 0;
}

static int
targettype(Fs *f, Ipifc *ifc, uint8_t *target)
{
	Iplifc *lifc;
	int t;

	rlock(&ifc->rwl);
	if(ipproxyifc(f, ifc, target)) {
		runlock(&ifc->rwl);
		return Tuniproxy;
	}

	for(lifc = ifc->lifc; lifc; lifc = lifc->next)
		if(ipcmp(lifc->local, target) == 0) {
			t = (lifc->tentative)? Tunitent: Tunirany;
			runlock(&ifc->rwl);
			return t;
		}

	runlock(&ifc->rwl);
	return 0;
}

/* bp needs to be freed with freeblist or passed on. */
static void
icmpiput6(Proto *icmp, Ipifc *ipifc, Block *bp)
{
	int type;
	char *msg, m2[128];
	uint8_t pktflags;
	uint8_t *packet, *src;
	uint8_t lsrc[IPaddrlen];
	Block *r;
	IPICMP *p;
	Icmppriv6 *ipriv;
	Iplifc *lifc;
	Ndpkt* np;
	Proto *pr;

	packet = bp->rp;
	p = (IPICMP *)packet;
	type = p->type;
	ipriv = icmp->priv;
	if(!valid(icmp, ipifc, bp, ipriv) || type > Maxtype6)
		goto raise;

	ipriv->in[type]++;
	switch(type) {
	case EchoRequestV6:
		bp = concatblock(bp);
		r = mkechoreply6(bp, ipifc);
		if(r == nil)
			goto raise;
		ipriv->out[EchoReply]++;
		ipoput6(icmp->f, r, 0, MAXTTL, DFLTTOS, nil);
		break;
	case UnreachableV6:
		if(p->code >= nelem(unreachcode))
			msg = unreachcode[Icmp6_unknown];
		else
			msg = unreachcode[p->code];

		bp->rp += IPICMPSZ;
		if(blocklen(bp) < 8){
			ipriv->stats[LenErrs6]++;
			goto raise;
		}
		p = (IPICMP *)bp->rp;
		pr = Fsrcvpcolx(icmp->f, p->proto);
		if(pr != nil && pr->advise != nil) {
			(*pr->advise)(pr, bp, msg);
			return;
		}

		bp->rp -= IPICMPSZ;
		goticmpkt6(icmp, bp, 0);
		break;
	case TimeExceedV6:
		if(p->code == 0){
			snprint(m2, sizeof m2, "ttl exceeded at %I", p->src);
			bp->rp += IPICMPSZ;
			if(blocklen(bp) < 8){
				ipriv->stats[LenErrs6]++;
				goto raise;
			}
			p = (IPICMP *)bp->rp;
			pr = Fsrcvpcolx(icmp->f, p->proto);
			if(pr && pr->advise) {
				(*pr->advise)(pr, bp, m2);
				return;
			}
			bp->rp -= IPICMPSZ;
		}
		goticmpkt6(icmp, bp, 0);
		break;
	case RouterAdvert:
	case RouterSolicit:
		/* using lsrc as a temp, munge hdr for goticmp6 */
		if (0) {
			memmove(lsrc, p->src, IPaddrlen);
			memmove(p->src, p->dst, IPaddrlen);
			memmove(p->dst, lsrc, IPaddrlen);
		}
		goticmpkt6(icmp, bp, type);
		break;
	case NbrSolicit:
		np = (Ndpkt*)p;			/* within bp */
		pktflags = 0;
		switch (targettype(icmp->f, ipifc, np->target)) {
		case Tunirany:
			pktflags |= Oflag;
			/* fall through */
		case Tuniproxy:
			if(ipcmp(np->src, v6Unspecified) != 0) {
				arpenter(icmp->f, V6, np->src, np->lnaddr,
					8*np->olen-2, 0);
				pktflags |= Sflag;
			}
			if(ipv6local(ipifc, lsrc)) {
				src = np->src;
				if(ipcmp(src, v6Unspecified) == 0)
					src = v6allnodesL;
				icmpna(icmp->f, lsrc, src, np->target,
					ipifc->mac, pktflags);
			}
			break;
		case Tunitent:
			/*
			 * not clear what needs to be done.  send up
			 * an icmp mesg saying `don't use this address'?
			 */
			break;
		}
		freeblist(bp);
		break;
	case NbrAdvert:
		/*
		 * if the target address matches one of the local interface
		 * addresses and the local interface address has tentative bit
		 * set, insert into ARP table. this is so the duplicate address
		 * detection part of ipconfig can discover duplication through
		 * the arp table.
		 */
		np = (Ndpkt*)p;			/* within bp */
		lifc = iplocalonifc(ipifc, np->target);
		arpenter(icmp->f, V6, np->target, np->lnaddr, 8*np->olen-2,
			lifc && lifc->tentative);
		freeblist(bp);
		break;
	case PacketTooBigV6:
	default:
		goticmpkt6(icmp, bp, 0);
		break;
	}
	return;

raise:
	freeblist(bp);
}

int
icmpstats6(Proto *icmp6, char *buf, int len)
{
	Icmppriv6 *priv;
	char *p, *e;
	int i;

	priv = icmp6->priv;
	p = buf;
	e = p+len;
	for(i = 0; i < Nstats6; i++)
		p = seprint(p, e, "%s: %lu\n", statnames6[i], priv->stats[i]);
	for(i = 0; i <= Maxtype6; i++)
		if(icmpnames6[i])
			p = seprint(p, e, "%s: %lu %lu\n", icmpnames6[i],
				priv->in[i], priv->out[i]);
		else if (0)
			p = seprint(p, e, "%d: %lu %lu\n", i, priv->in[i],
				priv->out[i]);
	return p - buf;
}

/* import from icmp.c */
extern int	icmpstate(Conv *c, char *state, int n);
extern char*	icmpannounce(Conv *c, char **argv, int argc);
extern char*	icmpconnect(Conv *c, char **argv, int argc);
extern void	icmpclose(Conv *c);

void
icmp6init(Fs *fs)
{
	Proto *icmp6 = smalloc(sizeof(Proto));

	icmp6->priv = smalloc(sizeof(Icmppriv6));
	icmp6->name = "icmpv6";
	icmp6->connect = icmpconnect;
	icmp6->announce = icmpannounce;
	icmp6->state = icmpstate;
	icmp6->create = icmpcreate6;
	icmp6->close = icmpclose;
	icmp6->rcv = icmpiput6;
	icmp6->stats = icmpstats6;
	icmp6->ctl = icmpctl6;
	icmp6->advise = icmpadvise6;
	icmp6->gc = nil;
	icmp6->ipproto = ICMPv6;
	icmp6->nc = 16;
	icmp6->ptclsize = sizeof(Icmpcb6);

	Fsproto(fs, icmp6);
}
