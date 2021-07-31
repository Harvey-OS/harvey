/*
 *  ethernet specific multiplexor for ip
 *
 *  this line discipline gets pushed onto an ethernet channel
 *  to demultiplex/multiplex ip conversations.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"arp.h"
#include 	"../port/ipdat.h"

#define DPRINT if(pip)print
int pip = 0;
int ipcksum = 1;
int Id = 1;

Fragq		*flisthead;
Fragq		*fragfree;
QLock		fraglock;

uchar		bcast[4] = { 0xff, 0xff, 0xff, 0xff };

Ipdevice ipd[Nipd];
Lock ipdlock;
static int dorouting = 0;

/* Predeclaration */
static void	ipmuxclose(Queue*), ipconvclose(Queue*);
static void	ipmuxiput(Queue*, Block*);
static void	ipmuxopen(Queue*, Stream*), ipconvopen(Queue*, Stream*);
static void	ipconvoput(Queue*, Block*);

/*
 *  the ethernet multiplexor stream module definition
 */
Qinfo ipinfo =
{
	ipmuxiput,
	ipmuxoput,
	ipmuxopen,
	ipmuxclose,
	"internet"
};

/*
 *  one per conversation
 */
Qinfo ipconvinfo =
{
	0,
	ipmuxoput,
	ipconvopen,
	ipconvclose,
	"ipconv"
};

void
stiplink(void)
{
	newqinfo(&ipinfo);
}

void
initfrag(int size)
{
	Fragq *fq, *eq;

	fragfree = (Fragq*)xalloc(sizeof(Fragq) * size);

	eq = &fragfree[size];
	for(fq = fragfree; fq < eq; fq++)
		fq->next = fq+1;

	fragfree[size-1].next = 0;
}

/*
 *  set up an ether interface
 */
static void
ipmuxopen(Queue *q, Stream *s)
{
	Ipdevice *p;

	/* First open is by ipconfig and sets up channel
	 * to ethernet
	 */
	lock(&ipdlock);
	for(p = ipd; p < &ipd[Nipd] && p->q; p++)
		;
	if(p == &ipd[Nipd]){
		unlock(&ipdlock);
		error(Enoifc);
	}
	p->q = WR(q);
	unlock(&ipdlock);
	RD(q)->ptr = p;
	WR(q)->ptr = p;
	ipsetaddrs(p);
/* FIX MTU! */
	p->maxmtu = 1500;	/* default is ethernet */
	p->minmtu = 60;
	p->hsize = ETHER_HDR;
	p->dev = s->dev;
	p->type = s->type;

	DPRINT("ipmuxopen EQ %lux dev=%d id=%d RD %lux WR %lux\n",
		q, s->dev, s->id, RD(q), WR(q));
}

void
ipmuxclose(Queue *q)
{
	Ipdevice *p;

	p = q->ptr;
	qlock(&p->outl);
	p->q = 0;
	qunlock(&p->outl);
}

/*
 *  start up a new conversation
 */
static void
ipconvopen(Queue *q, Stream *s)
{
	Ipconv *ipc;

	ipc = ipcreateconv(ipifc[s->dev], s->id);
	RD(q)->ptr = (void *)ipc;
	WR(q)->ptr = (void *)ipc;
	ipc->ref = 1;
}

/*
 *  initipifc - set parameters of an ip protocol interface
 */
void
initipifc(Ipifc *ifc, uchar ptcl, void (*recvfun)(Ipifc*, Block *bp))
{
	qlock(ifc);
	ifc->iprcv = recvfun;

	ifc->protocol = ptcl;
	ifc->inited = 1;

	qunlock(ifc);
}

static void
ipconvclose(Queue *q)
{
	Ipconv *ipc;

	ipc = (Ipconv *)(q->ptr);
	if(ipc){
		netdisown(ipc);
		ipc->ref = 0;
	}
}

static void
ipmuxctl(Queue *q, Block *bp)
{
	char *ptr;
	Ipdevice *p;
	char msg[64];
	int n;

	/* Allow one setting of the ip address */
	if(streamparse("setip", bp)) {
		if(strncmp(eve, u->p->user, NAMELEN) != 0) {
			freeb(bp);
			error(Eperm);
		}
		p = q->ptr;
		n = BLEN(bp);
		if(n >= sizeof msg)
			n = sizeof(msg) - 1;
		memmove(msg, bp->rptr, n);
		msg[n] = 0;
		ptr = msg;
		p->Myip[Myself] = ipparse((char *)ptr);
		p->Netmyip[0] = (p->Myip[Myself]>>24)&0xff;
		p->Netmyip[1] = (p->Myip[Myself]>>16)&0xff;
		p->Netmyip[2] = (p->Myip[Myself]>>8)&0xff;
		p->Netmyip[3] = p->Myip[Myself]&0xff;
		p->Mymask = classmask[p->Myip[Myself]>>30];
		while(*ptr != ' ' && *ptr)
			ptr++;
		if(*ptr)
			p->Mynetmask = ipparse((char *)++ptr);
		else
			p->Mynetmask = p->Mymask;
		while(*ptr != ' ' && *ptr)
			ptr++;
		if(*ptr)
			p->Remip = ipparse((char *)(ptr+1));
		else
			p->Remip = p->Mynetmask & p->Myip[Myself];
		freeb(bp);
		ipsetaddrs(p);
	} else if(streamparse("mntblocksize", bp)) {
		n = BLEN(bp);
		memmove(msg, bp->rptr, n);
		msg[n] = 0;
		n = atoi(msg);
		mntblocksize(n);
		freeb(bp);
	} else if(streamparse("routing", bp)) {
		n = BLEN(bp);
		memmove(msg, bp->rptr, n);
		msg[n] = 0;
		n = atoi(msg);
		dorouting = n;
		freeb(bp);
	} else
		freeb(bp);
}

void
ipmuxoput(Queue *q, Block *bp)
{
	Ipdevice *p;
	ushort fragoff;
	Block *xp, *nb;
	uchar gate[4];
	Etherhdr *eh, *feh;
	int lid, len, seglen, chunk, dlen, blklen, offset;

	if(bp->type == M_CTL){
		if(q)
			ipmuxctl(q, bp);
		else
			freeb(bp);
		return;
	}

	/* Number of bytes in ip and media header to write */
	len = blen(bp);

	/* Fill out the ip header */
	eh = (Etherhdr *)(bp->rptr);
	eh->vihl = IP_VER|IP_HLEN;
	eh->tos = 0;
	eh->ttl = 255;

	p = iproute(eh->dst, gate);
	if(p->q == 0){
		print("no interface %d.%d.%d.%d\n", eh->dst[0], eh->dst[1], eh->dst[2],
			eh->dst[3]);
		goto drop;
	}

	/* If we dont need to fragment just send it */
	if(len <= p->maxmtu) {
		hnputs(eh->length, len-ETHER_HDR);
		hnputs(eh->id, Id++);
		eh->frag[0] = 0;
		eh->frag[1] = 0;
		eh->cksum[0] = 0;
		eh->cksum[1] = 0;
		hnputs(eh->cksum, ip_csum(&eh->vihl));

		/* Finally put in the type and pass down to the arp layer */
		hnputs(eh->type, ET_IP);
		memmove(eh->d, gate, sizeof gate);	/* sleaze - pass next hop */
		eh->s[0] = p - ipd;
		PUTNEXT(p->q, bp);
		return;
	}

	if(eh->frag[0] & (IP_DF>>8))
		goto drop;

	seglen = (p->maxmtu - (ETHER_HDR+ETHER_IPHDR)) & ~7;
	if(seglen < 8)
		goto drop;

	/* Make prototype output header */
	hnputs(eh->type, ET_IP);
	
	dlen = len - (ETHER_HDR+ETHER_IPHDR);
	xp = bp;
	lid = Id++;

	offset = ETHER_HDR+ETHER_IPHDR;
	while(xp && offset && offset >= BLEN(xp)) {
		offset -= BLEN(xp);
		xp = xp->next;
	}
	xp->rptr += offset;

	for(fragoff = 0; fragoff < dlen; fragoff += seglen) {
		nb = allocb(ETHER_HDR+ETHER_IPHDR+seglen);
		feh = (Etherhdr *)(nb->rptr);

		memmove(nb->wptr, eh, ETHER_HDR+ETHER_IPHDR);
		nb->wptr += ETHER_HDR+ETHER_IPHDR;

		if((fragoff + seglen) >= dlen) {
			seglen = dlen - fragoff;
			hnputs(feh->frag, fragoff>>3);
		}
		else {	
			hnputs(feh->frag, (fragoff>>3)|IP_MF);
		}

		hnputs(feh->length, seglen + ETHER_IPHDR);
		hnputs(feh->id, lid);

		/* Copy up the data area */
		chunk = seglen;
		while(chunk) {
			if(!xp) {
				freeb(nb);
				goto drop;
			}
			blklen = MIN(BLEN(xp), chunk);
			memmove(nb->wptr, xp->rptr, blklen);
			nb->wptr += blklen;
			xp->rptr += blklen;
			chunk -= blklen;
			if(xp->rptr == xp->wptr)
				xp = xp->next;
		} 

		feh->cksum[0] = 0;
		feh->cksum[1] = 0;
		hnputs(feh->cksum, ip_csum(&feh->vihl));
		nb->flags |= S_DELIM;
		memmove(feh->d, gate, sizeof gate);	/* pass next hop */
		eh->s[0] = p - ipd;

		/* sync with close of the queue */
		if(waserror()){
			qunlock(&p->outl);
			nexterror();
		}
		qlock(&p->outl);
		if(p->q)
			PUTNEXT(p->q, nb);
		else
			freeb(nb);
		qunlock(&p->outl);
		poperror();
	}
drop:
	freeb(bp);	
}

void
router(Block *bp)
{
	Etherhdr	*eh;
	Ipdevice	*p;
	uchar		gate[4];

	eh = (Etherhdr *)(bp->rptr);
	p = iproute(eh->dst, gate);
	if(p->q == 0){
		print("no interface %d.%d.%d.%d\n",
			eh->dst[0], eh->dst[1], eh->dst[2], eh->dst[3]);
		freeb(bp);
		return;
	}

	/* Finally put in the type and pass down to the arp layer */
	hnputs(eh->type, ET_IP);
	memmove(eh->d, gate, sizeof gate);	/* sleaze - pass next hop */
	eh->s[0] = p - ipd;
	if(QFULL(p->q->next))
		freeb(bp);
	else
		PUTNEXT(p->q, bp);
}

/*
 *  Input a packet and use the ip protocol to select the correct
 *  device to pass it to.
 *
 */
static void
ipmuxiput(Queue *q, Block *bp)
{
	Ipifc 	 *ifc, **ifp;
	Etherhdr *h;
	ushort   frag;

	if(bp->type != M_DATA){
		PUTNEXT(q, bp);
		return;
	}

	h = (Etherhdr *)(bp->rptr);

	/* Ensure we have enough data to process */
	if(BLEN(bp) < (ETHER_HDR+ETHER_IPHDR)) {
		bp = pullup(bp, ETHER_HDR+ETHER_IPHDR);
		if(bp == 0)
			return;
	}

	if(ipforme(h->dst) == 0)
		if(dorouting) {
			router(bp);
			return;
		} else
			goto drop;

	if(ipcksum && ip_csum(&h->vihl)) {
		print("ip: checksum error (from %d.%d.%d.%d ?)\n",
		      h->src[0], h->src[1], h->src[2], h->src[3]);
		goto drop;
	}

	/* Check header length and version */
	if(h->vihl != (IP_VER|IP_HLEN))
		goto drop;

	frag = nhgets(h->frag);
	if(frag) {
		h->tos = frag & IP_MF ? 1 : 0;
		bp = ip_reassemble(frag, bp, h);
		if(!bp)
			return;
	}

	/*
 	 * Look for an ip interface attached to this protocol
	 */
	for(ifp = ipifc; ; ifp++){
		ifc = *ifp;
		if(ifc == 0)
			break;
		if(ifc->protocol == h->proto) {
			(*ifc->iprcv)(ifc, bp);
			return;
		}
	}
drop:
	freeb(bp);
}

void
ipsetaddrs(Ipdevice *p)
{
	p->Myip[Mybcast] = ~0;				/* local broadcast */
	p->Myip[2] = 0;					/* local broadcast - old */
	p->Myip[Mysubnet] = p->Myip[Myself] | ~p->Mynetmask;	/* subnet broadcast */
	p->Myip[Mysubnet+1] = p->Myip[Myself] & p->Mynetmask;	/* subnet broadcast - old */
	p->Myip[Mynet] = p->Myip[Myself] | ~p->Mymask;		/* net broadcast */
	p->Myip[Mynet+1] = p->Myip[Myself] & p->Mymask;		/* net broadcast - old */
	if(p->Remip == 0)
		p->Remip = p->Myip[Mysubnet+1];
}

int
ipforme(uchar *addr)
{
	Ipaddr haddr;
	Ipaddr *p;
	Ipdevice *d;

	haddr = nhgetl(addr);

	for(d = ipd; d < &ipd[Nipd]; d++){
		if(d->q == 0)
			continue;

		/* try my address plus all the forms of ip broadcast */
		for(p = d->Myip; p < &d->Myip[7]; p++)
			if(haddr == *p)
				return 1;
		/* if we haven't set an address, accept anything we get */
		if(d->Myip[Myself] == 0)
			return 1;
	}
	return 0;
}

typedef struct Fragstat	Fragstat;
struct Fragstat
{
	ulong	prev;
	ulong	prevall;
	ulong	succ;
	ulong	succall;
};
Fragstat fragstat;

Block *
ip_reassemble(int offset, Block *bp, Etherhdr *ip)
{
	Fragq *f;
	Ipaddr src, dst;
	ushort id;
	Block *bl, **l, *last, *prev;
	int ovlap, len, fragsize, pktposn;
	int fend;

	/* Check ethrnet has handed us a contiguous buffer */
	if(bp->next)
		panic("ip: reass ?");

	src = nhgetl(ip->src);
	dst = nhgetl(ip->dst);
	id = nhgets(ip->id);

	/*
	 *  find a reassembly queue for this fragment
	 */
	qlock(&fraglock);
	for(f = flisthead; f; f = f->next)
		if(f->src == src && f->dst == dst && f->id == id)
			break;
	qunlock(&fraglock);

	/*
	 *  if this isn't a fragmented packet, accept it
	 *  and get rid of any fragments that might go
	 *  with it.
	 */
	if(!ip->tos && (offset & ~(IP_MF|IP_DF)) == 0) {
		if(f != 0) {
			qlock(f);
			ipfragfree(f, 1);
		}
		return bp;
	}

	BLKFRAG(bp)->foff = offset<<3;
	BLKFRAG(bp)->flen = nhgets(ip->length) - ETHER_IPHDR; /* Ip data length */
	bp->flags &= ~S_DELIM;

	/* First fragment allocates a reassembly queue */
	if(f == 0) {
		f = ipfragallo();
		qlock(f);
		f->id = id;
		f->src = src;
		f->dst = dst;

		f->blist = bp;

		qunlock(f);
		return 0;
	}
	qlock(f);

	/*
	 *  find the new fragment's position in the queue
	 */
	prev = 0;
	l = &f->blist;
	for(bl = f->blist; bl && BLKFRAG(bp)->foff > BLKFRAG(bl)->foff; bl = bl->next) {
		prev = bl;
		l = &bl->next;
	}

	/* Check overlap of a previous fragment - trim away as necessary */
	if(prev) {
		ovlap = BLKFRAG(prev)->foff + BLKFRAG(prev)->flen - BLKFRAG(bp)->foff;
		if(ovlap > 0) {
			fragstat.prev++;
			if(ovlap >= BLKFRAG(bp)->flen) {
				fragstat.prevall++;
				freeb(bp);
				qunlock(f);
				return 0;
			}
			BLKFRAG(prev)->flen -= ovlap;
		}
	}

	/* Link onto assembly queue */
	bp->next = *l;
	*l = bp;

	/* Check to see if succeeding segments overlap */
	if(bp->next) {
		l = &bp->next;
		fend = BLKFRAG(bp)->foff + BLKFRAG(bp)->flen;
		/* Take completely covered segements out */
		while(*l){
			ovlap = fend - BLKFRAG(*l)->foff;
			if(ovlap <= 0)
				break;
			fragstat.succ++;
			if(ovlap < BLKFRAG(*l)->flen) {
				fragstat.succall++;
				BLKFRAG(*l)->flen -= ovlap;
				BLKFRAG(*l)->foff += ovlap;
				/* move up ether+ip hdrs */
				memmove((*l)->rptr + ovlap, (*l)->rptr,
					 ETHER_HDR+ETHER_IPHDR);
				(*l)->rptr += ovlap;
				break;
			}
			last = (*l)->next;
			(*l)->next = 0;
			freeb(*l);
			*l = last;
		}
	}

	/*
	 *  look for a complete packet.  if we get to a fragment
	 *  without IP_MF set, we're done.
	 */
	pktposn = 0;
	for(bl = f->blist; bl; bl = bl->next) {
		if(BLKFRAG(bl)->foff != pktposn)
			break;
		if((BLKIP(bl)->frag[0]&(IP_MF>>8)) == 0)
			goto complete;

		pktposn += BLKFRAG(bl)->flen;
	}
	qunlock(f);
	return 0;

complete:
	bl = f->blist;
	last = bl;
	len = nhgets(BLKIP(bl)->length);
	bl->wptr = bl->rptr + len + ETHER_HDR;

	/* Pullup all the fragment headers and return a complete packet */
	for(bl = bl->next; bl; bl = bl->next) {
		fragsize = BLKFRAG(bl)->flen;
		len += fragsize;
		bl->rptr += (ETHER_HDR+ETHER_IPHDR);
		bl->wptr = bl->rptr + fragsize;
		last = bl;
	}

	last->flags |= S_DELIM;
	bl = f->blist;
	f->blist = 0;
	ipfragfree(f, 1);

	ip = BLKIP(bl);
	hnputs(ip->length, len);

	return(bl);		
}

/*
 * ipfragfree - Free a list of fragments, fragment list must be locked
 */

void
ipfragfree(Fragq *frag, int lockq)
{
	Fragq *fl, **l;

	if(frag->blist)
		freeb(frag->blist);

	frag->src = 0;
	frag->id = 0;
	frag->blist = 0;
	qunlock(frag);

	if(lockq)
		qlock(&fraglock);

	l = &flisthead;
	for(fl = *l; fl; fl = fl->next) {
		if(fl == frag) {
			*l = frag->next;
			break;
		}
		l = &fl->next;
	}

	frag->next = fragfree;
	fragfree = frag;

	if(lockq)
		qunlock(&fraglock);
}

/*
 * ipfragallo - allocate a reassembly queue
 */
Fragq *
ipfragallo(void)
{
	Fragq *f;

	qlock(&fraglock);
	while(fragfree == 0) {
		for(f = flisthead; f; f = f->next)
			if(canqlock(f)) {
				ipfragfree(f, 0);
				break;
			}
	}
	f = fragfree;
	fragfree = f->next;
	f->next = flisthead;
	flisthead = f;
	f->age = TK2MS(MACHP(0)->ticks) + 30000;

	qunlock(&fraglock);
	return f;
}

/*
 * ip_csum - Compute internet header checksums
 */
ushort
ip_csum(uchar *addr)
{
	int len;
	ulong sum = 0;

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

/*
 * ipparse - Parse an ip address out of a string
 */

Ipaddr classmask[4] = {
	0xff000000,
	0xff000000,
	0xffff0000,
	0xffffff00
};

Ipaddr
ipparse(char *ipa)
{
	Ipaddr address = 0;
	int shift;
	Ipaddr net;

	shift = 24;

	while(shift >= 0 && ipa != (char *)1) {
		address |= atoi(ipa) << shift;
		shift -= 8;
		ipa = strchr(ipa, '.')+1;
	}
	net = address & classmask[address>>30];

	shift += 8;
	return net | ((address & ~classmask[address>>30])>>shift);
}

Ipaddr
ipgetsrc(uchar *dst)
{
	Ipdevice *p;
	uchar gate[4];

	p = iproute(dst, gate);
	if(p)
		return p->Myip[Myself];
	return 0;
}

