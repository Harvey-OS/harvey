#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include "ppp.h"

typedef struct Iphdr Iphdr;
struct Iphdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	ulong	src;		/* Ip source (uchar ordering unimportant) */
	ulong	dst;		/* Ip destination (uchar ordering unimportant) */
};

typedef struct Tcphdr Tcphdr;
struct Tcphdr
{
	ulong	ports;		/* defined as a ulong to make comparisons easier */
	uchar	seq[4];
	uchar	ack[4];
	uchar	flag[2];
	uchar	win[2];
	uchar	cksum[2];
	uchar	urg[2];
};

typedef struct Ilhdr Ilhdr;
struct Ilhdr
{
	uchar	sum[2];	/* Checksum including header */
	uchar	len[2];	/* Packet length */
	uchar	type;		/* Packet type */
	uchar	spec;		/* Special */
	uchar	src[2];	/* Src port */
	uchar	dst[2];	/* Dst port */
	uchar	id[4];	/* Sequence id */
	uchar	ack[4];	/* Acked sequence */
};

enum
{
	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Aknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */

	IP_DF		= 0x4000,	/* Don't fragment */

	IP_TCPPROTO	= 6,
	IP_ILPROTO	= 40,
	IL_IPHDR	= 20,
};

typedef struct Hdr Hdr;
struct Hdr
{
	uchar	buf[128];
	Iphdr	*ip;
	Tcphdr	*tcp;
	int	len;
};

typedef struct Tcpc Tcpc;
struct Tcpc
{
	uchar	lastrecv;
	uchar	lastxmit;
	uchar	basexmit;
	uchar	err;
	uchar	compressid;
	Hdr	t[MAX_STATES];
	Hdr	r[MAX_STATES];
};

enum
{	/* flag bits for what changed in a packet */
	NEW_U=(1<<0),	/* tcp only */
	NEW_W=(1<<1),	/* tcp only */
	NEW_A=(1<<2),	/* il tcp */
	NEW_S=(1<<3),	/* tcp only */
	NEW_P=(1<<4),	/* tcp only */
	NEW_I=(1<<5),	/* il tcp */
	NEW_C=(1<<6),	/* il tcp */
	NEW_T=(1<<7),	/* il only */
	TCP_PUSH_BIT	= 0x10,
};

/* reserved, special-case values of above for tcp */
#define SPECIAL_I (NEW_S|NEW_W|NEW_U)		/* echoed interactive traffic */
#define SPECIAL_D (NEW_S|NEW_A|NEW_W|NEW_U)	/* unidirectional data */
#define SPECIALS_MASK (NEW_S|NEW_A|NEW_W|NEW_U)

int
encode(void *p, ulong n)
{
	uchar	*cp;

	cp = p;
	if(n >= 256 || n == 0) {
		*cp++ = 0;
		cp[0] = n >> 8;
		cp[1] = n;
		return 3;
	}
	*cp = n;
	return 1;
}

#define DECODEL(f) { \
	if (*cp == 0) {\
		hnputl(f, nhgetl(f) + ((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		hnputl(f, nhgetl(f) + (ulong)*cp++); \
	} \
}
#define DECODES(f) { \
	if (*cp == 0) {\
		hnputs(f, nhgets(f) + ((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		hnputs(f, nhgets(f) + (ulong)*cp++); \
	} \
}

Block*
tcpcompress(Tcpc *comp, Block *b, int *protop)
{
	Iphdr	*ip;		/* current packet */
	Tcphdr	*tcp;		/* current pkt */
	ulong 	iplen, tcplen, hlen;	/* header length in uchars */
	ulong 	deltaS, deltaA;	/* general purpose temporaries */
	ulong 	changes;	/* change mask */
	uchar 	new_seq[16];	/* changes from last to current */
	uchar 	*cp;
	Hdr	*h;		/* last packet */
	int 	i, j;

	/*
	 * Bail if this is not a compressible TCP/IP packet
	 */
	ip = (Iphdr*)b->rptr;
	iplen = (ip->vihl & 0xf) << 2;
	tcp = (Tcphdr*)(b->rptr + iplen);
	tcplen = (tcp->flag[0] & 0xf0) >> 2;
	hlen = iplen + tcplen;
	if((tcp->flag[1] & (SYN|FIN|RST|ACK)) != ACK){
		*protop = Pip;
		return b;		/* connection control */
	}

	/*
	 * Packet is compressible, look for a connection
	 */
	changes = 0;
	cp = new_seq;
	j = comp->lastxmit;
	h = &comp->t[j];
	if(ip->src != h->ip->src || ip->dst != h->ip->dst
	|| tcp->ports != h->tcp->ports) {
		for(i = 0; i < MAX_STATES; ++i) {
			j = (comp->basexmit + i) % MAX_STATES;
			h = &comp->t[j];
			if(ip->src == h->ip->src && ip->dst == h->ip->dst
			&& tcp->ports == h->tcp->ports)
				goto found;
		}

		/* no connection, reuse the oldest */
		if(i == MAX_STATES) {
			j = comp->basexmit;
			j = (j + MAX_STATES - 1) % MAX_STATES;
			comp->basexmit = j;
			h = &comp->t[j];
			goto rescue;
		}
	}
found:

	/*
	 * Make sure that only what we expect to change changed. 
	 */
	if(ip->vihl  != h->ip->vihl || ip->tos   != h->ip->tos ||
	   ip->ttl   != h->ip->ttl  || ip->proto != h->ip->proto)
		goto rescue;	/* headers changed */
	if(iplen != sizeof(Iphdr) && memcmp(ip+1, h->ip+1, iplen - sizeof(Iphdr)))
		goto rescue;	/* ip options changed */
	if(tcplen != sizeof(Tcphdr) && memcmp(tcp+1, h->tcp+1, tcplen - sizeof(Tcphdr)))
		goto rescue;	/* tcp options changed */

	if(tcp->flag[1] & URG) {
		cp += encode(cp, nhgets(tcp->urg));
		changes |= NEW_U;
	} else if(memcmp(tcp->urg, h->tcp->urg, sizeof(tcp->urg)) != 0)
		goto rescue;
	if(deltaS = nhgets(tcp->win) - nhgets(h->tcp->win)) {
		cp += encode(cp, deltaS);
		changes |= NEW_W;
	}
	if(deltaA = nhgetl(tcp->ack) - nhgetl(h->tcp->ack)) {
		if(deltaA > 0xffff)
			goto rescue;
		cp += encode(cp, deltaA);
		changes |= NEW_A;
	}
	if(deltaS = nhgetl(tcp->seq) - nhgetl(h->tcp->seq)) {
		if (deltaS > 0xffff)
			goto rescue;
		cp += encode(cp, deltaS);
		changes |= NEW_S;
	}

	/*
	 * Look for the special-case encodings.
	 */
	switch(changes) {
	case 0:
		/*
		 * Nothing changed. If this packet contains data and the last
		 * one didn't, this is probably a data packet following an
		 * ack (normal on an interactive connection) and we send it
		 * compressed. Otherwise it's probably a retransmit,
		 * retransmitted ack or window probe.  Send it uncompressed
		 * in case the other side missed the compressed version.
		 */
		if(nhgets(ip->length) == nhgets(h->ip->length) ||
		   nhgets(h->ip->length) != hlen)
			goto rescue;
		break;
	case SPECIAL_I:
	case SPECIAL_D:
		/*
		 * Actual changes match one of our special case encodings --
		 * send packet uncompressed.
		 */
		goto rescue;
	case NEW_S | NEW_A:
		if (deltaS == deltaA &&
			deltaS == nhgets(h->ip->length) - hlen) {
			/* special case for echoed terminal traffic */
			changes = SPECIAL_I;
			cp = new_seq;
		}
		break;
	case NEW_S:
		if (deltaS == nhgets(h->ip->length) - hlen) {
			/* special case for data xfer */
			changes = SPECIAL_D;
			cp = new_seq;
		}
		break;
	}
	deltaS = nhgets(ip->id) - nhgets(h->ip->id);
	if(deltaS != 1) {
		cp += encode(cp, deltaS);
		changes |= NEW_I;
	}
	if (tcp->flag[1] & PSH)
		changes |= TCP_PUSH_BIT;
	/*
	 * Grab the cksum before we overwrite it below. Then update our
	 * state with this packet's header.
	 */
	deltaA = nhgets(tcp->cksum);
	memmove(h->buf, b->rptr, hlen);
	h->len = hlen;
	h->tcp = (Tcphdr*)(h->buf + iplen);

	/*
	 * We want to use the original packet as our compressed packet. (cp -
	 * new_seq) is the number of uchars we need for compressed sequence
	 * numbers. In addition we need one uchar for the change mask, one
	 * for the connection id and two for the tcp checksum. So, (cp -
	 * new_seq) + 4 uchars of header are needed. hlen is how many uchars
	 * of the original packet to toss so subtract the two to get the new
	 * packet size. The temporaries are gross -egs.
	 */
	deltaS = cp - new_seq;
	cp = b->rptr;
	if(comp->lastxmit != j || comp->compressid == 0) {
		comp->lastxmit = j;
		hlen -= deltaS + 4;
		cp += hlen;
		*cp++ = (changes | NEW_C);
		*cp++ = j;
	} else {
		hlen -= deltaS + 3;
		cp += hlen;
		*cp++ = changes;
	}
	b->rptr += hlen;
	hnputs(cp, deltaA);
	cp += 2;
	memmove(cp, new_seq, deltaS);
	*protop = Pvjctcp;
	return b;

rescue:
	/*
	 * Update connection state & send uncompressed packet
	 */
	memmove(h->buf, b->rptr, hlen);
	h->tcp = (Tcphdr*)(h->buf + iplen);
	h->len = hlen;
	ip->proto = j;
	comp->lastxmit = j;
	*protop = Pvjutcp;
	return b;
}

Block*
tcpuncompress(Tcpc *comp, Block *b, int type)
{
	uchar	*cp, changes;
	int	i;
	int	iplen, len;
	Iphdr	*ip;
	Tcphdr	*tcp;
	Hdr	*h;

	if(type == Pvjutcp) {
		/*
		 *  Locate the saved state for this connection. If the state
		 *  index is legal, clear the 'discard' flag.
		 */
		ip = (Iphdr*)b->rptr;
		if(ip->proto >= MAX_STATES)
			goto rescue;
		iplen = (ip->vihl & 0xf) << 2;
		tcp = (Tcphdr*)(b->rptr + iplen);
		comp->lastrecv = ip->proto;
		len = iplen + ((tcp->flag[0] & 0xf0) >> 2);
		comp->err = 0;
		/*
		 * Restore the IP protocol field then save a copy of this
		 * packet header. The checksum is zeroed in the copy so we
		 * don't have to zero it each time we process a compressed
		 * packet.
		 */
		ip->proto = IP_TCPPROTO;
		h = &comp->r[comp->lastrecv];
		memmove(h->buf, b->rptr, len);
		h->tcp = (Tcphdr*)(h->buf + iplen);
		h->len = len;
		h->ip->cksum[0] = h->ip->cksum[1] = 0;
		return b;
	}

	cp = b->rptr;
	changes = *cp++;
	if(changes & NEW_C) {
		/*
		 * Make sure the state index is in range, then grab the
		 * state. If we have a good state index, clear the 'discard'
		 * flag.
		 */
		if(*cp >= MAX_STATES)
			goto rescue;
		comp->err = 0;
		comp->lastrecv = *cp++;
	} else {
		/*
		 * This packet has no state index. If we've had a
		 * line error since the last time we got an explicit state
		 * index, we have to toss the packet.
		 */
		if(comp->err != 0){
			freeb(b);
			return nil;
		}
	}

	/*
	 * Find the state then fill in the TCP checksum and PUSH bit.
	 */
	h = &comp->r[comp->lastrecv];
	ip = h->ip;
	tcp = h->tcp;
	len = h->len;
	memmove(tcp->cksum, cp, sizeof tcp->cksum);
	cp += 2;
	if(changes & TCP_PUSH_BIT)
		tcp->flag[1] |= PSH;
	else
		tcp->flag[1] &= ~PSH;
	/*
	 * Fix up the state's ack, seq, urg and win fields based on the
	 * changemask.
	 */
	switch (changes & SPECIALS_MASK) {
	case SPECIAL_I:
		i = nhgets(ip->length) - len;
		hnputl(tcp->ack, nhgetl(tcp->ack) + i);
		hnputl(tcp->seq, nhgetl(tcp->seq) + i);
		break;

	case SPECIAL_D:
		hnputl(tcp->seq, nhgetl(tcp->seq) + nhgets(ip->length) - len);
		break;

	default:
		if(changes & NEW_U) {
			tcp->flag[1] |= URG;
			if(*cp == 0){
				hnputs(tcp->urg, nhgets(cp+1));
				cp += 3;
			}else
				hnputs(tcp->urg, *cp++);
		} else
			tcp->flag[1] &= ~URG;
		if(changes & NEW_W)
			DECODES(tcp->win)
		if(changes & NEW_A)
			DECODEL(tcp->ack)
		if(changes & NEW_S)
			DECODEL(tcp->seq)
		break;
	}

	/* Update the IP ID */
	if(changes & NEW_I)
		DECODES(ip->id)
	else
		hnputs(ip->id, nhgets(ip->id) + 1);

	/*
	 *  At this point, cp points to the first uchar of data in the packet.
	 *  Back up cp by the TCP/IP header length to make room for the
	 *  reconstructed header.
	 *  We assume the packet we were handed has enough space to prepend
	 *  up to 128 uchars of header.
	 */
	b->rptr = cp;
	if(b->rptr - b->base < len){
		b = padb(b, len);
		b = pullup(b, blen(b));
	} else
		b->rptr -= len;
	hnputs(ip->length, BLEN(b));
	memmove(b->rptr, ip, len);
	
	/* recompute the ip header checksum */
	ip = (Iphdr*)b->rptr;
	ip->cksum[0] = ip->cksum[1] = 0;
	hnputs(ip->cksum, ipcsum(b->rptr));

	return b;

rescue:
	netlog("ppp: vj: Bad Packet!\n");
	comp->err = 1;
	freeb(b);
	return nil;
}

Tcpc*
compress_init(Tcpc *c)
{
	int i;
	Hdr *h;

	if(c == nil)
		c = malloc(sizeof(Tcpc));

	memset(c, 0, sizeof(*c));
	for(i = 0; i < MAX_STATES; i++){
		h = &c->t[i];
		h->ip = (Iphdr*)h->buf;
		h->tcp = (Tcphdr*)(h->buf + 20);
		h->len = 40;
		h = &c->r[i];
		h->ip = (Iphdr*)h->buf;
		h->tcp = (Tcphdr*)(h->buf + 20);
		h->len = 40;
	}

	return c;
}

Block*
compress(Tcpc *tcp, Block *b, int *protop)
{
	Iphdr		*ip;

	/*
	 * Bail if this is not a compressible IP packet
	 */
	ip = (Iphdr*)b->rptr;
	if((nhgets(ip->frag) & 0x3fff) != 0){
		*protop = Pip;
		return b;
	}

	switch(ip->proto) {
	case IP_TCPPROTO:
		return tcpcompress(tcp, b, protop);
	default:
		*protop = Pip;
		return b;
	}
}

int
compress_negotiate(Tcpc *tcp, uchar *data)
{
	if(data[0] != MAX_STATES - 1)
		return -1;
	tcp->compressid = data[1];
	return 0;
}

/* called by ppp when there was a bad frame received */
void
compress_error(Tcpc *tcp)
{
	tcp->err = 1;
}
