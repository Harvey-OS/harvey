/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"ip.h"
#include	"ipv6.h"

enum
{
	IP6FHDR		= 8, 		/* sizeof(Fraghdr6) */
};

#define IPV6CLASS(hdr)	(((hdr)->vcf[0]&0x0F)<<2 | ((hdr)->vcf[1]&0xF0)>>2)
#define BLKIPVER(xp)	(((Ip6hdr*)((xp)->rp))->vcf[0] & 0xF0)
/*
 * This sleazy macro is stolen shamelessly from ip.c, see comment there.
 */
#define BKFG(xp)	((Ipfrag*)((xp)->base))

Block*		ip6reassemble(IP*, int, Block*, Ip6hdr*);
Fragment6*	ipfragallo6(IP*);
void		ipfragfree6(IP*, Fragment6*);
Block*		procopts(Block *bp);
static Block*	procxtns(IP *ip, Block *bp, int doreasm);
int		unfraglen(Block *bp, uint8_t *nexthdr, int setfh);

int
ipoput6(Fs *f, Block *bp, int gating, int ttl, int tos, Conv *c)
{
	Proc *up = externup();
	int medialen, len, chunk, uflen, flen, seglen, lid, offset, fragoff;
	int morefrags, blklen, rv = 0, tentative;
	uint8_t *gate, nexthdr;
	Block *xp, *nb;
	Fraghdr6 fraghdr;
	IP *ip;
	Ip6hdr *eh;
	Ipifc *ifc;
	Route *r, *sr;

	ip = f->ip;

	/* Fill out the ip header */
	eh = (Ip6hdr*)(bp->rp);

	ip->stats[OutRequests]++;

	/* Number of uint8_ts in data and ip header to write */
	len = blocklen(bp);

	tentative = iptentative(f, eh->src);
	if(tentative){
		netlog(f, Logip, "reject tx of packet with tentative src address %I\n",
			eh->src);
		goto free;
	}

	if(gating){
		chunk = nhgets(eh->ploadlen);
		if(chunk > len){
			ip->stats[OutDiscards]++;
			netlog(f, Logip, "short gated packet\n");
			goto free;
		}
		if(chunk + IP6HDR < len)
			len = chunk + IP6HDR;
	}

	if(len >= IP_MAX){
		ip->stats[OutDiscards]++;
		netlog(f, Logip, "exceeded ip max size %I\n", eh->dst);
		goto free;
	}

	r = v6lookup(f, eh->dst, c);
	if(r == nil){
//		print("no route for %I, src %I free\n", eh->dst, eh->src);
		ip->stats[OutNoRoutes]++;
		netlog(f, Logip, "no interface %I\n", eh->dst);
		rv = -1;
		goto free;
	}

	ifc = r->ifc;
	if(r->type & (Rifc|Runi))
		gate = eh->dst;
	else if(r->type & (Rbcast|Rmulti)) {
		gate = eh->dst;
		sr = v6lookup(f, eh->src, nil);
		if(sr && (sr->type & Runi))
			ifc = sr->ifc;
	}
	else
		gate = r->v6.gate;

	if(!gating)
		eh->vcf[0] = IP_VER6;
	eh->ttl = ttl;
	if(!gating) {
		eh->vcf[0] |= tos >> 4;
		eh->vcf[1]  = tos << 4;
	}

	if(!canrlock(&ifc->rwl))
		goto free;

	if(waserror()){
		runlock(&ifc->rwl);
		nexterror();
	}

	if(ifc->medium == nil)
		goto raise;

	/* If we dont need to fragment just send it */
	medialen = ifc->maxtu - ifc->medium->hsize;
	if(len <= medialen) {
		hnputs(eh->ploadlen, len - IP6HDR);
		ifc->medium->bwrite(ifc, bp, V6, gate);
		runlock(&ifc->rwl);
		poperror();
		return 0;
	}

	if(gating && ifc->reassemble <= 0) {
		/*
		 * v6 intermediate nodes are not supposed to fragment pkts;
		 * we fragment if ifc->reassemble is turned on; an exception
		 * needed for nat.
		 */
		ip->stats[OutDiscards]++;
		icmppkttoobig6(f, ifc, bp);
		netlog(f, Logip, "%I: gated pkts not fragmented\n", eh->dst);
		goto raise;
	}

	/* start v6 fragmentation */
	uflen = unfraglen(bp, &nexthdr, 1);
	if(uflen > medialen) {
		ip->stats[FragFails]++;
		ip->stats[OutDiscards]++;
		netlog(f, Logip, "%I: unfragmentable part too big\n", eh->dst);
		goto raise;
	}

	flen = len - uflen;
	seglen = (medialen - (uflen + IP6FHDR)) & ~7;
	if(seglen < 8) {
		ip->stats[FragFails]++;
		ip->stats[OutDiscards]++;
		netlog(f, Logip, "%I: seglen < 8\n", eh->dst);
		goto raise;
	}

	lid = incref(&ip->id6);
	fraghdr.nexthdr = nexthdr;
	fraghdr.res = 0;
	hnputl(fraghdr.id, lid);

	xp = bp;
	offset = uflen;
	while (xp && offset && offset >= BLEN(xp)) {
		offset -= BLEN(xp);
		xp = xp->next;
	}
	xp->rp += offset;

	fragoff = 0;
	morefrags = 1;

	for(; fragoff < flen; fragoff += seglen) {
		nb = allocb(uflen + IP6FHDR + seglen);

		if(fragoff + seglen >= flen) {
			seglen = flen - fragoff;
			morefrags = 0;
		}

		hnputs(eh->ploadlen, seglen+IP6FHDR);
		memmove(nb->wp, eh, uflen);
		nb->wp += uflen;

		hnputs(fraghdr.offsetRM, fragoff); /* last 3 bits must be 0 */
		fraghdr.offsetRM[1] |= morefrags;
		memmove(nb->wp, &fraghdr, IP6FHDR);
		nb->wp += IP6FHDR;

		/* Copy data */
		chunk = seglen;
		while (chunk) {
			if(!xp) {
				ip->stats[OutDiscards]++;
				ip->stats[FragFails]++;
				freeblist(nb);
				netlog(f, Logip, "!xp: chunk in v6%d\n", chunk);
				goto raise;
			}
			blklen = chunk;
			if(BLEN(xp) < chunk)
				blklen = BLEN(xp);
			memmove(nb->wp, xp->rp, blklen);

			nb->wp += blklen;
			xp->rp += blklen;
			chunk -= blklen;
			if(xp->rp == xp->wp)
				xp = xp->next;
		}

		ifc->medium->bwrite(ifc, nb, V6, gate);
		ip->stats[FragCreates]++;
	}
	ip->stats[FragOKs]++;

raise:
	runlock(&ifc->rwl);
	poperror();
free:
	freeblist(bp);
	return rv;
}

void
ipiput6(Fs *f, Ipifc *ifc, Block *bp)
{
	int hl, hop, tos, notforme, tentative;
	uint8_t proto;
	uint8_t v6dst[IPaddrlen];
	IP *ip;
	Ip6hdr *h;
	Proto *p;
	Route *r, *sr;

	ip = f->ip;
	ip->stats[InReceives]++;

	/*
	 *  Ensure we have all the header info in the first
	 *  block.  Make life easier for other protocols by
	 *  collecting up to the first 64 bytes in the first block.
	 */
	if(BLEN(bp) < 64) {
		hl = blocklen(bp);
		if(hl < IP6HDR)
			hl = IP6HDR;
		if(hl > 64)
			hl = 64;
		bp = pullupblock(bp, hl);
		if(bp == nil)
			return;
	}

	h = (Ip6hdr *)bp->rp;

	memmove(&v6dst[0], &h->dst[0], IPaddrlen);
	notforme = ipforme(f, v6dst) == 0;
	tentative = iptentative(f, v6dst);

	if(tentative && h->proto != ICMPv6) {
		print("ipv6 non-icmp tentative addr %I, drop\n", v6dst);
		freeblist(bp);
		return;
	}

	/* Check header version */
	if(BLKIPVER(bp) != IP_VER6) {
		ip->stats[InHdrErrors]++;
		netlog(f, Logip, "ip: bad version %ux\n", (h->vcf[0]&0xF0)>>2);
		freeblist(bp);
		return;
	}

	/* route */
	if(notforme) {
		if(!ip->iprouting){
			freeblist(bp);
			return;
		}

		/* don't forward to link-local destinations */
		if(islinklocal(h->dst) ||
		   (isv6mcast(h->dst) && (h->dst[1]&0xF) <= Link_local_scop)){
			ip->stats[OutDiscards]++;
			freeblist(bp);
			return;
		}

		/* don't forward to source's network */
		sr = v6lookup(f, h->src, nil);
		r  = v6lookup(f, h->dst, nil);

		if(r == nil || sr == r){
			ip->stats[OutDiscards]++;
			freeblist(bp);
			return;
		}

		/* don't forward if packet has timed out */
		hop = h->ttl;
		if(hop < 1) {
			ip->stats[InHdrErrors]++;
			icmpttlexceeded6(f, ifc, bp);
			freeblist(bp);
			return;
		}

		/* process headers & reassemble if the interface expects it */
		bp = procxtns(ip, bp, r->ifc->reassemble);
		if(bp == nil)
			return;

		ip->stats[ForwDatagrams]++;
		h = (Ip6hdr *)bp->rp;
		tos = IPV6CLASS(h);
		hop = h->ttl;
		ipoput6(f, bp, 1, hop-1, tos, nil);
		return;
	}

	/* reassemble & process headers if needed */
	bp = procxtns(ip, bp, 1);
	if(bp == nil)
		return;

	h = (Ip6hdr *) (bp->rp);
	proto = h->proto;
	p = Fsrcvpcol(f, proto);
	if(p && p->rcv) {
		ip->stats[InDelivers]++;
		(*p->rcv)(p, ifc, bp);
		return;
	}

	ip->stats[InDiscards]++;
	ip->stats[InUnknownProtos]++;
	freeblist(bp);
}

/*
 * ipfragfree6 - copied from ipfragfree4 - assume hold fraglock6
 */
void
ipfragfree6(IP *ip, Fragment6 *frag)
{
	Fragment6 *fl, **l;

	if(frag->blist)
		freeblist(frag->blist);

	memset(frag->src, 0, IPaddrlen);
	frag->id = 0;
	frag->blist = nil;

	l = &ip->flisthead6;
	for(fl = *l; fl; fl = fl->next) {
		if(fl == frag) {
			*l = frag->next;
			break;
		}
		l = &fl->next;
	}

	frag->next = ip->fragfree6;
	ip->fragfree6 = frag;
}

/*
 * ipfragallo6 - copied from ipfragalloc4
 */
Fragment6*
ipfragallo6(IP *ip)
{
	Fragment6 *f;

	while(ip->fragfree6 == nil) {
		/* free last entry on fraglist */
		for(f = ip->flisthead6; f->next; f = f->next)
			;
		ipfragfree6(ip, f);
	}
	f = ip->fragfree6;
	ip->fragfree6 = f->next;
	f->next = ip->flisthead6;
	ip->flisthead6 = f;
	f->age = NOW + 30000;

	return f;
}

static Block*
procxtns(IP *ip, Block *bp, int doreasm)
{
	int offset;
	uint8_t proto;
	Ip6hdr *h;

	h = (Ip6hdr *)bp->rp;
	offset = unfraglen(bp, &proto, 0);

	if(proto == FH && doreasm != 0) {
		bp = ip6reassemble(ip, offset, bp, h);
		if(bp == nil)
			return nil;
		offset = unfraglen(bp, &proto, 0);
	}

	if(proto == DOH || offset > IP6HDR)
		bp = procopts(bp);
	return bp;
}

/*
 * returns length of "Unfragmentable part", i.e., sum of lengths of ipv6 hdr,
 * hop-by-hop & routing headers if present; *nexthdr is set to nexthdr value
 * of the last header in the "Unfragmentable part"; if setfh != 0, nexthdr
 * field of the last header in the "Unfragmentable part" is set to FH.
 */
int
unfraglen(Block *bp, uint8_t *nexthdr, int setfh)
{
	uint8_t *p, *q;
	int ufl, hs;

	p = bp->rp;
	q = p+6;   /* proto, = p+sizeof(Ip6hdr.vcf)+sizeof(Ip6hdr.ploadlen) */
	*nexthdr = *q;
	ufl = IP6HDR;
	p += ufl;

	while (*nexthdr == HBH || *nexthdr == RH) {
		*nexthdr = *p;
		hs = ((int)*(p+1) + 1) * 8;
		ufl += hs;
		q = p;
		p += hs;
	}

	if(*nexthdr == FH)
		*q = *p;
	if(setfh)
		*q = FH;
	return ufl;
}

Block*
procopts(Block *bp)
{
	return bp;
}

Block*
ip6reassemble(IP* ip, int uflen, Block* bp, Ip6hdr* ih)
{
	int fend, offset, ovlap, len, fragsize, pktposn;
	uint id;
	uint8_t src[IPaddrlen], dst[IPaddrlen];
	Block *bl, **l, *last, *prev;
	Fraghdr6 *fraghdr;
	Fragment6 *f, *fnext;

	fraghdr = (Fraghdr6 *)(bp->rp + uflen);
	memmove(src, ih->src, IPaddrlen);
	memmove(dst, ih->dst, IPaddrlen);
	id = nhgetl(fraghdr->id);
	offset = nhgets(fraghdr->offsetRM) & ~7;

	/*
	 *  block lists are too hard, pullupblock into a single block
	 */
	if(bp->next){
		bp = pullupblock(bp, blocklen(bp));
		ih = (Ip6hdr *)bp->rp;
	}

	qlock(&ip->fraglock6);

	/*
	 *  find a reassembly queue for this fragment
	 */
	for(f = ip->flisthead6; f; f = fnext){
		fnext = f->next;
		if(ipcmp(f->src, src)==0 && ipcmp(f->dst, dst)==0 && f->id == id)
			break;
		if(f->age < NOW){
			ip->stats[ReasmTimeout]++;
			ipfragfree6(ip, f);
		}
	}

	/*
	 *  if this isn't a fragmented packet, accept it
	 *  and get rid of any fragments that might go
	 *  with it.
	 */
	if(nhgets(fraghdr->offsetRM) == 0) {	/* 1st frag is also last */
		if(f) {
			ipfragfree6(ip, f);
			ip->stats[ReasmFails]++;
		}
		qunlock(&ip->fraglock6);
		return bp;
	}

	if(bp->base+IPFRAGSZ >= bp->rp){
		bp = padblock(bp, IPFRAGSZ);
		bp->rp += IPFRAGSZ;
	}

	BKFG(bp)->foff = offset;
	BKFG(bp)->flen = nhgets(ih->ploadlen) + IP6HDR - uflen - IP6FHDR;

	/* First fragment allocates a reassembly queue */
	if(f == nil) {
		f = ipfragallo6(ip);
		f->id = id;
		memmove(f->src, src, IPaddrlen);
		memmove(f->dst, dst, IPaddrlen);

		f->blist = bp;

		qunlock(&ip->fraglock6);
		ip->stats[ReasmReqds]++;
		return nil;
	}

	/*
	 *  find the new fragment's position in the queue
	 */
	prev = nil;
	l = &f->blist;
	bl = f->blist;
	while(bl != nil && BKFG(bp)->foff > BKFG(bl)->foff) {
		prev = bl;
		l = &bl->next;
		bl = bl->next;
	}

	/* Check overlap of a previous fragment - trim away as necessary */
	if(prev) {
		ovlap = BKFG(prev)->foff + BKFG(prev)->flen - BKFG(bp)->foff;
		if(ovlap > 0) {
			if(ovlap >= BKFG(bp)->flen) {
				freeblist(bp);
				qunlock(&ip->fraglock6);
				return nil;
			}
			BKFG(prev)->flen -= ovlap;
		}
	}

	/* Link onto assembly queue */
	bp->next = *l;
	*l = bp;

	/* Check to see if succeeding segments overlap */
	if(bp->next) {
		l = &bp->next;
		fend = BKFG(bp)->foff + BKFG(bp)->flen;

		/* Take completely covered segments out */
		while(*l) {
			ovlap = fend - BKFG(*l)->foff;
			if(ovlap <= 0)
				break;
			if(ovlap < BKFG(*l)->flen) {
				BKFG(*l)->flen -= ovlap;
				BKFG(*l)->foff += ovlap;
				/* move up ih hdrs */
				memmove((*l)->rp + ovlap, (*l)->rp, uflen);
				(*l)->rp += ovlap;
				break;
			}
			last = (*l)->next;
			(*l)->next = nil;
			freeblist(*l);
			*l = last;
		}
	}

	/*
	 *  look for a complete packet.  if we get to a fragment
	 *  with the trailing bit of fraghdr->offsetRM[1] set, we're done.
	 */
	pktposn = 0;
	for(bl = f->blist; bl && BKFG(bl)->foff == pktposn; bl = bl->next) {
		fraghdr = (Fraghdr6 *)(bl->rp + uflen);
		if((fraghdr->offsetRM[1] & 1) == 0) {
			bl = f->blist;

			/* get rid of frag header in first fragment */
			memmove(bl->rp + IP6FHDR, bl->rp, uflen);
			bl->rp += IP6FHDR;
			len = nhgets(((Ip6hdr*)bl->rp)->ploadlen) - IP6FHDR;
			bl->wp = bl->rp + len + IP6HDR;
			/*
			 * Pullup all the fragment headers and
			 * return a complete packet
			 */
			for(bl = bl->next; bl; bl = bl->next) {
				fragsize = BKFG(bl)->flen;
				len += fragsize;
				bl->rp += uflen + IP6FHDR;
				bl->wp = bl->rp + fragsize;
			}

			bl = f->blist;
			f->blist = nil;
			ipfragfree6(ip, f);
			ih = (Ip6hdr*)bl->rp;
			hnputs(ih->ploadlen, len);
			qunlock(&ip->fraglock6);
			ip->stats[ReasmOKs]++;
			return bl;
		}
		pktposn += BKFG(bl)->flen;
	}
	qunlock(&ip->fraglock6);
	return nil;
}
