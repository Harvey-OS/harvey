#include "all.h"

#include "../ip/ip.h"

#define	DEBUG	if(1||cons.flags&Fip)print

typedef	struct	Rock	Rock;
typedef	struct	Frag	Frag;

struct	Frag
{
	int	start;
	int	end;
};

struct	Rock
{
	uchar	src[Pasize];
	uchar	dst[Pasize];
	int	id;		/* src,dst,id are address of the rock */
	Msgbuf*	mb;		/* reassembly. if 0, the rock is empty */
	ulong	age;		/* timeout to throw away */
	int	last;		/* set to data size when last frag arrives */
	int	nfrag;
	Frag	frag[Nfrag];
};

static
struct
{
	Lock;
	Rock	rock[Nrock];
} ip;

void
ipreceive(Enpkt *ep, int l, Ifc *ifc)
{
	Ippkt *p;
	Msgbuf *mb;
	Rock *r, *or;
	Frag *f;
	int len, id, frag, off, loff, i, n;
	Ippkt pkt;
	ulong t;

	p = (Ippkt*)ep;
	if(l < Ensize+Ipsize) {
		ifc->sumerr++;
		print("ip: en too small\n");
		return;
	}
	if(l > LARGEBUF) {
		ifc->sumerr++;
		print("ip: en too large\n");
		return;
	}

	memmove(&pkt, p, Ensize+Ipsize);	/* copy pkt to 'real' memory */
	if(pkt.vihl != (IP_VER|IP_HLEN))
		return;
	if(!ipforme(pkt.dst, ifc))
		return;
	if(ipcsum(&pkt.vihl)) {
		ifc->sumerr++;
		print("ip: checksum error (from %I)\n", pkt.src);
		return;
	}

	frag = nhgets(pkt.frag);
	len = nhgets(pkt.length) - Ipsize;
	id = nhgets(pkt.id);

	/*
	 * total ip msg fits into one frag
	 */
	if((frag & ~IP_DF) == 0) {
		mb = mballoc(l, 0, Mbip3);
		memmove(mb->data, &pkt, Ensize+Ipsize);
		memmove(mb->data + (Ensize+Ipsize),
			(uchar*)p + (Ensize+Ipsize), l-(Ensize+Ipsize));
		goto send;
	}

	/*
	 * throw away old rocks.
	 */
	t = toytime();
	lock(&ip);
	r = ip.rock;
	for(i=0; i<Nrock; i++,r++)
		if(r->mb && t >= r->age) {
			mbfree(r->mb);
			r->mb = 0;
		}

	/*
	 * reassembly of fragments
	 * look up rock by src, dst, id.
	 */
	or = 0;
	r = ip.rock;
	for(i=0; i<Nrock; i++,r++) {
		if(r->mb == 0) {
			if(or == 0)
				or = r;
			continue;
		}
		if(id == r->id)
		if(memcmp(r->src, pkt.src, Pasize) == 0)
		if(memcmp(r->dst, pkt.dst, Pasize) == 0)
			goto found;
	}
	r = or;
	if(r == 0) {
		/* no available rocks */
		r = ip.rock;
		for(i=0; i<Nrock; i++,r++)
			if(r->mb) {
				mbfree(r->mb);
				r->mb = 0;
			}
		r = ip.rock;
	}
	r->id = id;
	r->mb = mballoc(LARGEBUF, 0, Mbip2);
	memmove(r->src, pkt.src, Pasize);
	memmove(r->dst, pkt.dst, Pasize);
	r->nfrag = 0;
	r->last = 0;

found:
	mb = r->mb;
	r->age = t + SECOND(30);

	off = (frag & ~(IP_DF|IP_MF)) << 3;
	if(len+off+Ensize+Ipsize > mb->count) {
		/* ip pkt too big */
		mbfree(mb);
		r->mb = 0;
		goto uout;
	}
	if(!(frag & IP_MF))
		r->last = off+len;		/* found the end */

	memmove(mb->data+(Ensize+Ipsize)+off,
		(uchar*)p + (Ensize+Ipsize), len);

	/*
	 * frag algorithm:
	 * first entry is easy
	 */
	n = r->nfrag;
	if(n == 0) {
		r->frag[0].start = off;
		r->frag[0].end = off+len;
		r->nfrag = 1;
		goto span;
	}

	/*
	 * two in a row is easy
	 */
	if(r->frag[n-1].end == off) {
		r->frag[n-1].end += len;
		goto span;
	}

	/*
	 * add this frag
	 */
	if(n >= Nfrag) {
		/* too many frags */
		mbfree(mb);
		r->mb = 0;
		goto uout;
	}
	r->frag[n].start = off;
	r->frag[n].end = off+len;
	n++;
	r->nfrag = n;

span:
	/*
	 * see if we span the whole list
	 * can be O(n**2), but usually much smaller
	 */
	if(r->last == 0)
		goto uout;
	off = 0;

spanloop:
	loff = off;
	f = r->frag;
	for(i=0; i<n; i++,f++)
		if(off >= f->start && off < f->end)
			off = f->end;
	if(loff == off)
		goto uout;
	if(off < r->last)
		goto spanloop;

	memmove(mb->data, &pkt, Ensize+Ipsize);
	p = (Ippkt*)mb->data;
	hnputs(p->length, r->last+Ipsize);
	l = r->last + (Ensize+Ipsize);
	mb->count = l;
	r->mb = 0;
	unlock(&ip);

send:
	switch(pkt.proto) {
	default:
		mbfree(mb);
		break;
	case Ilproto:
		ilrecv(mb, ifc);
		break;
	case Udpproto:
		udprecv(mb, ifc);
		break;
	case Icmpproto:
		icmprecv(mb, ifc);
		break;
	case Igmpproto:
		igmprecv(mb, ifc);
		break;
	case Tcpproto:
		tcprecv(mb, ifc);
		break;
	}
	return;

uout:
	unlock(&ip);
}
