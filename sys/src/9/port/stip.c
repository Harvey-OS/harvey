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
#include 	"ipdat.h"

#define DPRINT if(pip)print
int pip = 0;
int ipcksum = 1;
int Id = 1;

Fragq		*flisthead;
Fragq		*fragfree;
QLock		fraglock;

Queue 		*Etherq;

Ipaddr		Myip[7];
Ipaddr		Mymask;
Ipaddr		Mynetmask;
uchar		Netmyip[4];	/* In Network byte order */
uchar		bcast[4] = { 0xff, 0xff, 0xff, 0xff };

/* Predeclaration */
static void	ipetherclose(Queue*);
static void	ipetheriput(Queue*, Block*);
static void	ipetheropen(Queue*, Stream*);
static void	ipetheroput(Queue*, Block*);

/*
 *  the ethernet multiplexor stream module definition
 */
Qinfo ipinfo =
{
	ipetheriput,
	ipetheroput,
	ipetheropen,
	ipetherclose,
	"internet"
};

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
ipetheropen(Queue *q, Stream *s)
{
	Ipconv *ipc;

	/* First open is by ipconfig and sets up channel
	 * to ethernet
	 */
	if(!Etherq) {
		Etherq = WR(q);
		s->opens++;		/* Hold this queue in place */
		s->inuse++;
		ipsetaddrs();
	} else {
		ipc = ipcreateconv(ipifc[s->dev], s->id);
		RD(q)->ptr = (void *)ipc;
		WR(q)->ptr = (void *)ipc;
		ipc->ref = 1;
	}

	DPRINT("ipetheropen EQ %lux dev=%d id=%d RD %lux WR %lux\n",
		Etherq, s->dev, s->id, RD(q), WR(q));
}

/*
 *  initipifc - set parameters of an ip protocol interface
 */
void
initipifc(Ipifc *ifc, uchar ptcl, void (*recvfun)(Ipifc*, Block *bp), int max,
	int min, int hdrsize)
{
	qlock(ifc);
	ifc->iprcv = recvfun;

	/* If media supports large transfer units limit maxmtu
	 * to max ip size */
	if(max > IP_MAX)
		max = IP_MAX;
	ifc->maxmtu = max;
	ifc->minmtu = min;
	ifc->hsize = hdrsize;

	ifc->protocol = ptcl;
	ifc->inited = 1;

	qunlock(ifc);
}

static void
ipetherclose(Queue *q)
{
	Ipconv *ipc;

	ipc = (Ipconv *)(q->ptr);
	if(ipc){
		netdisown(ipc);
		ipc->ref = 0;
	}
}

static void
ipetheroput(Queue *q, Block *bp)
{
	Etherhdr *eh, *feh;
	int	 lid, len, seglen, chunk, dlen, blklen, offset;
	Ipifc	 *ifp;
	ushort	 fragoff;
	Block	 *xp, *nb;
	uchar 	 *ptr;

	if(bp->type != M_DATA){
		/* Allow one setting of the ip address */
		if(streamparse("setip", bp)) {
			if(strncmp(eve, u->p->user, sizeof(eve)) != 0)
				error(Eperm);
			ptr = bp->rptr;
			Myip[Myself] = ipparse((char *)ptr);
			Netmyip[0] = (Myip[Myself]>>24)&0xff;
			Netmyip[1] = (Myip[Myself]>>16)&0xff;
			Netmyip[2] = (Myip[Myself]>>8)&0xff;
			Netmyip[3] = Myip[Myself]&0xff;
			Mymask = classmask[Myip[Myself]>>30];
			while(*ptr != ' ' && *ptr)
				ptr++;
			if(*ptr)
				Mynetmask = ipparse((char *)ptr);
			else
				Mynetmask = Mymask;
			freeb(bp);
			ipsetaddrs();
		}
		else
			PUTNEXT(Etherq, bp);
		return;
	}

	ifp = (Ipifc *)(q->ptr);

	/* Number of bytes in ip and media header to write */
	len = blen(bp);

	/* Fill out the ip header */
	eh = (Etherhdr *)(bp->rptr);
	eh->vihl = IP_VER|IP_HLEN;
	eh->tos = 0;
	eh->ttl = 255;

	/* If we dont need to fragment just send it */
	if(len <= ifp->maxmtu) {
		hnputs(eh->length, len-ETHER_HDR);
		hnputs(eh->id, Id++);
		eh->frag[0] = 0;
		eh->frag[1] = 0;
		eh->cksum[0] = 0;
		eh->cksum[1] = 0;
		hnputs(eh->cksum, ip_csum(&eh->vihl));

		/* Finally put in the type and pass down to the arp layer */
		hnputs(eh->type, ET_IP);
		PUTNEXT(Etherq, bp);
		return;
	}

	if(eh->frag[0] & (IP_DF>>8))
		goto drop;

	seglen = (ifp->maxmtu - (ETHER_HDR+ETHER_IPHDR)) & ~7;
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
		PUTNEXT(Etherq, nb);
	}
drop:
	freeb(bp);	
}


/*
 *  Input a packet and use the ip protocol to select the correct
 *  device to pass it to.
 *
 */
static void
ipetheriput(Queue *q, Block *bp)
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

	/* Look to see if its for me before we waste time checksuming it */
	if(ipforme(h->dst) == 0)
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
ipsetaddrs(void)
{
	Myip[Mybcast] = ~0;				/* local broadcast */
	Myip[2] = 0;					/* local broadcast - old */
	Myip[Mysubnet] = Myip[Myself] | ~Mynetmask;	/* subnet broadcast */
	Myip[Mysubnet+1] = Myip[Myself] & Mynetmask;	/* subnet broadcast - old */
	Myip[Mynet] = Myip[Myself] | ~Mymask;		/* net broadcast */
	Myip[Mynet+1] = Myip[Myself] & Mymask;		/* net broadcast - old */
}

int
ipforme(uchar *addr)
{
	Ipaddr haddr;
	Ipaddr *p;

	haddr = nhgetl(addr);

	/* try my address plus all the forms of ip broadcast */
	for(p = Myip; p < &Myip[7]; p++)
		if(haddr == *p);
			return 1;

	/* if we haven't set an address yet, accept anything we get */
	if(Myip[Myself] == 0)
		return 1;

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
	int end;

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
		end = BLKFRAG(bp)->foff + BLKFRAG(bp)->flen;
		/* Take completely covered segements out */
		while(*l){
			ovlap = end - BLKFRAG(*l)->foff;
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
