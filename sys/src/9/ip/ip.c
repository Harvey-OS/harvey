#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../ip/ip.h"

typedef struct Iphdr	Iphdr;
typedef struct Fragment	Fragment;
typedef struct Ipfrag	Ipfrag;

enum
{
	IPHDR		= 20,		/* sizeof(Iphdr) */
	IP_VER		= 0x40,		/* Using IP version 4 */
	IP_HLEN		= 0x05,		/* Header length in words */
	IP_DF		= 0x4000,	/* Don't fragment */
	IP_MF		= 0x2000,	/* More fragments */
	IP_MAX		= (32*1024),	/* Maximum Internet packet size */
};

struct Iphdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* ip->identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	uchar	src[4];		/* IP source */
	uchar	dst[4];		/* IP destination */
};

struct Fragment
{
	Block*	blist;
	Fragment* next;
	ulong 	src;
	ulong 	dst;
	ushort	id;
	ulong 	age;
};

struct Ipfrag
{
	ushort	foff;
	ushort	flen;
};

/* MIB II counters */
enum
{
	Forwarding,
	DefaultTTL,
	InReceives,
	InHdrErrors,
	InAddrErrors,
	ForwDatagrams,
	InUnknownProtos,
	InDiscards,
	InDelivers,
	OutRequests,
	OutDiscards,
	OutNoRoutes,
	ReasmTimeout,
	ReasmReqds,
	ReasmOKs,
	ReasmFails,
	FragOKs,
	FragFails,
	FragCreates,

	Nstats,
};

static char *statnames[] =
{
[Forwarding]	"Forwarding",
[DefaultTTL]	"DefaultTTL",
[InReceives]	"InReceives",
[InHdrErrors]	"InHdrErrors",
[InAddrErrors]	"InAddrErrors",
[ForwDatagrams]	"ForwDatagrams",
[InUnknownProtos]	"InUnknownProtos",
[InDiscards]	"InDiscards",
[InDelivers]	"InDelivers",
[OutRequests]	"OutRequests",
[OutDiscards]	"OutDiscards",
[OutNoRoutes]	"OutNoRoutes",
[ReasmTimeout]	"ReasmTimeout",
[ReasmReqds]	"ReasmReqds",
[ReasmOKs]	"ReasmOKs",
[ReasmFails]	"ReasmFails",
[FragOKs]	"FragOKs",
[FragFails]	"FragFails",
[FragCreates]	"FragCreates",
};

/* an instance of IP */
struct IP
{
	ulong		stats[Nstats];

	QLock		fraglock;
	Fragment*	flisthead;
	Fragment*	fragfree;

	Ref		id;
	int		iprouting;			/* true if we route like a gateway */
};

#define BLKIP(xp)	((Iphdr*)((xp)->rp))
/*
 * This sleazy macro relies on the media header size being
 * larger than sizeof(Ipfrag). ipreassemble checks this is true
 */
#define BKFG(xp)	((Ipfrag*)((xp)->base))

ushort		ipcsum(uchar*);
Block*		ipreassemble(IP*, int, Block*, Iphdr*);
void		ipfragfree(IP*, Fragment*);
Fragment*	ipfragallo(IP*);

void
ip_init(Fs *f)
{
	IP *ip;

	ip = smalloc(sizeof(IP));
	initfrag(ip, 100);
	f->ip = ip;
}

void
iprouting(Fs *f, int on)
{
	f->ip->iprouting = on;
	if(f->ip->iprouting==0)
		f->ip->stats[Forwarding] = 2;
	else
		f->ip->stats[Forwarding] = 1;	
}

void
ipoput(Fs *f, Block *bp, int gating, int ttl, int tos)
{
	Ipifc *ifc;
	uchar *gate;
	ulong fragoff;
	Block *xp, *nb;
	Iphdr *eh, *feh;
	int lid, len, seglen, chunk, dlen, blklen, offset, medialen;
	Route *r, *sr;
	IP *ip;

	ip = f->ip;

	/* Fill out the ip header */
	eh = (Iphdr*)(bp->rp);

	ip->stats[OutRequests]++;

	/* Number of uchars in data and ip header to write */
	len = blocklen(bp);

	if(gating){
		chunk = nhgets(eh->length);
		if(chunk > len){
			ip->stats[OutDiscards]++;
			netlog(f, Logip, "short gated packet\n");
			goto free;
		}
		if(chunk < len)
			len = chunk;
	}
	if(len >= IP_MAX){
		ip->stats[OutDiscards]++;
		netlog(f, Logip, "exceeded ip max size %V\n", eh->dst);
		goto free;
	}

	r = v4lookup(f, eh->dst);
	if(r == nil){
		ip->stats[OutNoRoutes]++;
		netlog(f, Logip, "no interface %V\n", eh->dst);
		goto free;
	}

	ifc = r->ifc;
	if(r->type & (Rifc|Runi))
		gate = eh->dst;
	else
	if(r->type & (Rbcast|Rmulti)) {
		gate = eh->dst;
		sr = v4lookup(f, eh->src);
		if(sr != nil && (sr->type & Runi))
			ifc = sr->ifc;
	}
	else
		gate = r->v4.gate;

	if(!gating)
		eh->vihl = IP_VER|IP_HLEN;
	eh->ttl = ttl;
	if(!gating)
		eh->tos = tos;

	if(!canrlock(ifc))
		goto free;
	if(waserror()){
		runlock(ifc);
		nexterror();
	}
	if(ifc->m == nil)
		goto raise;

	/* If we dont need to fragment just send it */
	medialen = ifc->maxmtu - ifc->m->hsize;
	if(len <= medialen) {
		if(!gating)
			hnputs(eh->id, incref(&ip->id));
		hnputs(eh->length, len);
		if(!gating){
			eh->frag[0] = 0;
			eh->frag[1] = 0;
		}
		eh->cksum[0] = 0;
		eh->cksum[1] = 0;
		hnputs(eh->cksum, ipcsum(&eh->vihl));

/*		print("ipoput %V->%V via %V\n", eh->src, eh->dst, gate); /**/
		ifc->m->bwrite(ifc, bp, V4, gate);
		runlock(ifc);
		poperror();
		return;
	}

	if(eh->frag[0] & (IP_DF>>8)){
		ip->stats[FragFails]++;
		ip->stats[OutDiscards]++;
		netlog(f, Logip, "%V: eh->frag[0] & (IP_DF>>8)\n", eh->dst);
		goto raise;
	}

	seglen = (medialen - IPHDR) & ~7;
	if(seglen < 8){
		ip->stats[FragFails]++;
		ip->stats[OutDiscards]++;
		netlog(f, Logip, "%V seglen < 8\n", eh->dst);
		goto raise;
	}

	dlen = len - IPHDR;
	xp = bp;
	if(gating)
		lid = nhgets(eh->id);
	else
		lid = incref(&ip->id);

	offset = IPHDR;
	while(xp != nil && offset && offset >= BLEN(xp)) {
		offset -= BLEN(xp);
		xp = xp->next;
	}
	xp->rp += offset;

	if(gating)
		fragoff = nhgets(eh->frag)<<3;
	else
		fragoff = 0;
	dlen += fragoff;
	for(; fragoff < dlen; fragoff += seglen) {
		nb = allocb(IPHDR+seglen);
		feh = (Iphdr*)(nb->rp);

		memmove(nb->wp, eh, IPHDR);
		nb->wp += IPHDR;

		if((fragoff + seglen) >= dlen) {
			seglen = dlen - fragoff;
			hnputs(feh->frag, fragoff>>3);
		}
		else	
			hnputs(feh->frag, (fragoff>>3)|IP_MF);

		hnputs(feh->length, seglen + IPHDR);
		hnputs(feh->id, lid);

		/* Copy up the data area */
		chunk = seglen;
		while(chunk) {
			if(!xp) {
				ip->stats[OutDiscards]++;
				ip->stats[FragFails]++;
				freeblist(nb);
				netlog(f, Logip, "!xp: chunk %d\n", chunk);
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

		feh->cksum[0] = 0;
		feh->cksum[1] = 0;
		hnputs(feh->cksum, ipcsum(&feh->vihl));
		ifc->m->bwrite(ifc, nb, V4, gate);
		ip->stats[FragCreates]++;
	}
	ip->stats[FragOKs]++;
raise:
	runlock(ifc);
	poperror();
free:
	freeblist(bp);	
}

void
initfrag(IP *ip, int size)
{
	Fragment *fq, *eq;

	ip->fragfree = (Fragment*)malloc(sizeof(Fragment) * size);
	if(ip->fragfree == nil)
		panic("initfrag");

	eq = &ip->fragfree[size];
	for(fq = ip->fragfree; fq < eq; fq++)
		fq->next = fq+1;

	ip->fragfree[size-1].next = nil;
}

#define DBG(x)	if((logmask & Logipmsg) && (iponly == 0 || x == iponly))netlog

void
ipiput(Fs *f, Ipifc *ifc, Block *bp)
{
	int hl;
	Iphdr *h;
	Proto *p;
	ushort frag;
	int notforme;
	uchar *dp, v6dst[IPaddrlen];
	IP *ip;
	Route *r, *sr;

	ip = f->ip;
	ip->stats[InReceives]++;

//	h = (Iphdr *)(bp->rp);
//	DBG(nhgetl(h->src))(Logipmsg, "ipiput %V %V len %d proto %d\n",
//				h->src, h->dst, BLEN(bp), h->proto);

	/*
	 *  Ensure we have all the header info in the first
	 *  block.  Make life easier for other protocols by
	 *  collecting up to the first 64 bytes in the first block.
	 */
	if(BLEN(bp) < 64) {
		hl = blocklen(bp);
		if(hl < IPHDR)
			hl = IPHDR;
		if(hl > 64)
			hl = 64;
		bp = pullupblock(bp, hl);
		if(bp == nil)
			return;
	}
	h = (Iphdr *)(bp->rp);

	/* dump anything that whose header doesn't checksum */
	if(ipcsum(&h->vihl)) {
		ip->stats[InHdrErrors]++;
		netlog(f, Logip, "ip: checksum error %V\n", h->src);
		freeblist(bp);
		return;
	}

	v4tov6(v6dst, h->dst);
	notforme = ipforme(f, v6dst) == 0;

	/* Check header length and version */
	if(h->vihl != (IP_VER|IP_HLEN)) {
		hl = (h->vihl&0xF)<<2;
		if((h->vihl&0xF0) != IP_VER || hl < (IP_HLEN<<2)) {
			ip->stats[InHdrErrors]++;
			netlog(f, Logip, "ip: %V bad hivl %ux\n", h->src, h->vihl);
			freeblist(bp);
			return;
		}
		/* If this is not routed strip off the options */
		if(notforme == 0) {
			dp = bp->rp + (hl - (IP_HLEN<<2));
			memmove(dp, h, IP_HLEN<<2);
			bp->rp = dp;
			h = (Iphdr *)(bp->rp);
			h->vihl = (IP_VER|IP_HLEN);
		}
	}

	/* route */
	if(notforme) {
		if(!ip->iprouting){
			freeb(bp);
			return;
		}

		/* don't forward to source's network */
		sr = v4lookup(f, h->src);
		r = v4lookup(f, h->dst);
		if(r == nil || sr == r){
			ip->stats[OutDiscards]++;
			freeblist(bp);
			return;
		}

		/* don't forward if packet has timed out */
		if(h->ttl <= 1){
			ip->stats[InHdrErrors]++;
			icmpttlexceeded(f, ifc, bp);
			freeblist(bp);
			return;
		}
		
		/* reassemble if the interface expects it */
		if(r->ifc->reassemble){
			frag = nhgets(h->frag);
			if(frag) {
				h->tos = 0;
				if(frag & IP_MF)
					h->tos = 1;
				bp = ipreassemble(ip, frag, bp, h);
				if(bp == nil)
					return;
				h = (Iphdr *)(bp->rp);
			}
		}

		ip->stats[ForwDatagrams]++;
		ipoput(f, bp, 1, h->ttl - 1, h->tos);

		return;
	}

	/* reassemble */
	frag = nhgets(h->frag);
	if(frag) {
		h->tos = 0;
		if(frag & IP_MF)
			h->tos = 1;
		bp = ipreassemble(ip, frag, bp, h);
		if(bp == nil)
			return;
		h = (Iphdr *)(bp->rp);
	}

	p = Fsrcvpcol(f, h->proto);
	if(p != nil && p->rcv != nil) {
		ip->stats[InDelivers]++;
		(*p->rcv)(p, ifc, bp);
		return;
	}
	ip->stats[InDiscards]++;
	ip->stats[InUnknownProtos]++;
	freeblist(bp);
}

int
ipstats(Fs *f, char *buf, int len)
{
	IP *ip;
	char *p, *e;
	int i;

	ip = f->ip;
	ip->stats[DefaultTTL] = MAXTTL;

	p = buf;
	e = p+len;
	for(i = 0; i < Nstats; i++)
		p = seprint(p, e, "%s: %lud\n", statnames[i], ip->stats[i]);
	return p - buf;
}

Block*
ipreassemble(IP *ip, int offset, Block *bp, Iphdr *ih)
{
	int fend;
	ushort id;
	Fragment *f, *fnext;
	ulong src, dst;
	Block *bl, **l, *last, *prev;
	int ovlap, len, fragsize, pktposn;

	src = nhgetl(ih->src);
	dst = nhgetl(ih->dst);
	id = nhgets(ih->id);

	/*
	 *  block lists are too hard, pullupblock into a single block
	 */
	if(bp->next){
		bp = pullupblock(bp, blocklen(bp));
		ih = (Iphdr *)(bp->rp);
	}

	qlock(&ip->fraglock);

	/*
	 *  find a reassembly queue for this fragment
	 */
	for(f = ip->flisthead; f; f = fnext){
		fnext = f->next;	/* because ipfragfree changes the list */
		if(f->src == src && f->dst == dst && f->id == id)
			break;
		if(f->age < msec){
			ip->stats[ReasmTimeout]++;
			ipfragfree(ip, f);
		}
	}

	/*
	 *  if this isn't a fragmented packet, accept it
	 *  and get rid of any fragments that might go
	 *  with it.
	 */
	if(!ih->tos && (offset & ~(IP_MF|IP_DF)) == 0) {
		if(f != nil) {
			ipfragfree(ip, f);
			ip->stats[ReasmFails]++;
		}
		qunlock(&ip->fraglock);
		return bp;
	}

	if(bp->base+sizeof(Ipfrag) >= bp->rp){
		bp = padblock(bp, sizeof(Ipfrag));
		bp->rp += sizeof(Ipfrag);
	}

	BKFG(bp)->foff = offset<<3;
	BKFG(bp)->flen = nhgets(ih->length)-IPHDR;

	/* First fragment allocates a reassembly queue */
	if(f == nil) {
		f = ipfragallo(ip);
		f->id = id;
		f->src = src;
		f->dst = dst;

		f->blist = bp;

		qunlock(&ip->fraglock);
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
				qunlock(&ip->fraglock);
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
				memmove((*l)->rp + ovlap, (*l)->rp, IPHDR);
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
	 *  without IP_MF set, we're done.
	 */
	pktposn = 0;
	for(bl = f->blist; bl; bl = bl->next) {
		if(BKFG(bl)->foff != pktposn)
			break;
		if((BLKIP(bl)->frag[0]&(IP_MF>>8)) == 0) {
			bl = f->blist;
			len = nhgets(BLKIP(bl)->length);
			bl->wp = bl->rp + len;

			/* Pullup all the fragment headers and
			 * return a complete packet
			 */
			for(bl = bl->next; bl; bl = bl->next) {
				fragsize = BKFG(bl)->flen;
				len += fragsize;
				bl->rp += IPHDR;
				bl->wp = bl->rp + fragsize;
			}

			bl = f->blist;
			f->blist = nil;
			ipfragfree(ip, f);
			ih = BLKIP(bl);
			hnputs(ih->length, len);
			qunlock(&ip->fraglock);
			ip->stats[ReasmOKs]++;
			return bl;		
		}
		pktposn += BKFG(bl)->flen;
	}
	qunlock(&ip->fraglock);
	return nil;
}

/*
 * ipfragfree - Free a list of fragments - assume hold fraglock
 */
void
ipfragfree(IP *ip, Fragment *frag)
{
	Fragment *fl, **l;

	if(frag->blist)
		freeblist(frag->blist);

	frag->src = 0;
	frag->id = 0;
	frag->blist = nil;

	l = &ip->flisthead;
	for(fl = *l; fl; fl = fl->next) {
		if(fl == frag) {
			*l = frag->next;
			break;
		}
		l = &fl->next;
	}

	frag->next = ip->fragfree;
	ip->fragfree = frag;

}

/*
 * ipfragallo - allocate a reassembly queue - assume hold fraglock
 */
Fragment *
ipfragallo(IP *ip)
{
	Fragment *f;

	while(ip->fragfree == nil) {
		/* free last entry on fraglist */
		for(f = ip->flisthead; f->next; f = f->next)
			;
		ipfragfree(ip, f);
	}
	f = ip->fragfree;
	ip->fragfree = f->next;
	f->next = ip->flisthead;
	ip->flisthead = f;
	f->age = msec + 30000;

	return f;
}

ushort
ipcsum(uchar *addr)
{
	int len;
	ulong sum;

	sum = 0;
	len = (addr[0]&0xf)<<2;

	while(len > 0) {
		sum += addr[0]<<8 | addr[1] ;
		len -= 2;
		addr += 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	return (sum^0xffff);
}

enum
{
	Nmtucache=	128,
};

typedef struct MTUcache MTUcache;

struct MTUcache
{
	uchar	ip[IPaddrlen];
	ulong	mtu;
	ulong	ms;
};

static struct {
	Lock;
	MTUcache c[Nmtucache];
} mc;

void
update_mtucache(uchar *ip, ulong mtu)
{
	MTUcache *oldest, *p;

	if(mtu < 512)
		return;

	lock(&mc);
	oldest = mc.c;
	for(p = mc.c; p < &mc.c[Nmtucache]; p++){
		if(ipcmp(ip, p->ip) == 0){
			p->mtu = mtu;
			p->ms = msec;
			break;
		}
		if(p->ms < oldest->ms)
			oldest = p;
	}
	if(p == &mc.c[Nmtucache]){
		ipmove(oldest->ip, ip);
		oldest->mtu = mtu;
		oldest->ms = msec;
	}
	unlock(&mc);
}

ulong
restrict_mtu(uchar *ip, ulong mtu)
{
	MTUcache *p;

	lock(&mc);
	for(p = mc.c; p < &mc.c[Nmtucache]; p++){
		if(p->ms + 1000*10*60 < msec){
			memset(p->ip, 0, sizeof(p->ip));
			p->ms = 0;
		}
		if(ipcmp(ip, p->ip) == 0){
			if(p->mtu < mtu)
				mtu = p->mtu;
			break;
		}
	}
	unlock(&mc);

	return mtu;
}
