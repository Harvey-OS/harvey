#include "all.h"

#include "../ip/ip.h"

#define	DEBUG	if(cons.flags&arpcache.flag)print
#define	ORDER	1	/* 1 send last frag first, faster */

typedef struct	Arpentry	Arpentry;
typedef struct	Arpstats	Arpstats;
typedef	struct	Arpe		Arpe;

struct	Arpe
{
	uchar	tpa[Pasize];
	uchar	tha[Easize];
};

static	int	ipahash(uchar*);
static	void	cmd_arp(int, char*[]);

static
struct
{
	Lock;
	uchar	null[Pasize];
	int	start;
	int	idgen;
	ulong	flag;
	Msgbuf*	unresol;
	struct
	{
		int	laste;
		Arpe	arpe[Ne];
	} abkt[Nb];
} arpcache;

int
nhgets(uchar *p)
{
	return (p[0]<<8) | p[1];
}

long
nhgetl(uchar *p)
{
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

void
hnputs(uchar *p, int x)
{
	p[0] = x>>8;
	p[1] = x;
}

void
hnputl(uchar *p, long x)
{
	p[0] = x>>24;
	p[1] = x>>16;
	p[2] = x>>8;
	p[3] = x;
}

void
arpstart(void)
{
	if(arpcache.start == 0) {
		lock(&arpcache);
		if(arpcache.start == 0) {
			cmd_install("arp", "subcommand -- arp protocol", cmd_arp);
			arpcache.flag = flag_install("arp", "-- verbose");
			arpcache.start = 1;
			iprouteinit();
		}
		unlock(&arpcache);
	}
}

void
arpreceive(Enpkt *ep, int l, Ifc *ifc)
{
	Ilp* ilp;
	Arppkt *p, *q;
	Msgbuf *mb, **mbp;
	Arpe *a;
	uchar *tpa;
	int type, i, h;
	ulong t;

	if(l < Ensize+Arpsize)
		return;

	p = (Arppkt*)ep;

	if(nhgets(p->pro) != Iptype ||
	   nhgets(p->hrd) != 1 ||
	   p->pln != Pasize ||
	   p->hln != Easize)
		return;

	type = nhgets(p->op);
	switch(type) {
	case Arprequest:
		/* update entry for this source */
		h = ipahash(p->spa);
		a = arpcache.abkt[h].arpe;
		lock(&arpcache);
		for(i=0; i<Ne; i++,a++) {
			if(memcmp(a->tpa, p->spa, Pasize) == 0) {
				memmove(a->tha, p->sha, Easize);
				break;
			}
		}
		unlock(&arpcache);

		if(memcmp(p->tpa, ifc->ipa, Pasize) != 0)
			break;

		DEBUG("rcv arp req for %I from %I\n", p->tpa, p->spa);

		mb = mballoc(Ensize+Arpsize, 0, Mbarp1);
		q = (Arppkt*)mb->data;

		memmove(q, p, Ensize+Arpsize);

		hnputs(q->op, Arpreply);
		memmove(q->tha, p->sha, Easize);
		memmove(q->tpa, p->spa, Pasize);
		memmove(q->sha, ifc->ea, Easize);
		memmove(q->spa, ifc->ipa, Pasize);
		memmove(q->d, q->s, Easize);

		send(ifc->reply, mb);
		break;

	case Arpreply:
		DEBUG("rcv arp rpl for %I is %E\n", p->spa, p->sha);

		h = ipahash(p->spa);
		a = arpcache.abkt[h].arpe;
		lock(&arpcache);
		for(i=0; i<Ne; i++,a++) {
			if(memcmp(a->tpa, p->spa, Pasize) == 0) {
				memmove(a->tha, p->sha, Easize);
				goto out;
			}
		}

		i = arpcache.abkt[h].laste + 1;
		if(i < 0 || i >= Ne)
			i = 0;
		arpcache.abkt[h].laste = i;

		a = &arpcache.abkt[h].arpe[i];
		memmove(a->tpa, p->spa, Pasize);
		memmove(a->tha, p->sha, Easize);

		/*
		 * go thru unresolved queue
		 */
	out:
		t = toytime();
		mbp = &arpcache.unresol;
		for(mb = *mbp; mb; mb = *mbp) {
			if(t >= mb->param) {
				*mbp = mb->next;
				unlock(&arpcache);
				mbfree(mb);
				lock(&arpcache);
				goto out;
			}
			ilp = mb->chan->pdata;
			tpa = ilp->ipgate;
			if(memcmp(a->tpa, tpa, Pasize) == 0) {
				*mbp = mb->next;
				mb->next = 0;
				unlock(&arpcache);
				ipsend(mb);
				lock(&arpcache);
				goto out;
			}
			mbp = &mb->next;
		}
		unlock(&arpcache);
		break;
	}
}

static
int
ipahash(uchar *p)
{
	ulong h;

	h = p[0];
	h = h*7 + p[1];
	h = h*7 + p[2];
	h = h*7 + p[3];
	return h%Nb;
}

void
ipsend1(Msgbuf *mb, Ifc *ifc, uchar *ipgate)
{
	Msgbuf **mbp, *m;
	Ippkt *p;
	Arppkt *q;
	Arpe *a;
	int i, id, len, dlen, off;
	ulong t;

	p = (Ippkt*)mb->data;

	a = arpcache.abkt[ipahash(ipgate)].arpe;
	lock(&arpcache);
	for(i=0; i<Ne; i++,a++)
		if(memcmp(a->tpa, ipgate, Pasize) == 0)
			goto found;

	/*
	 * queue ip pkt to be resolved later
	 */
again:
	i = 0;		// q length
	t = toytime();
	mbp = &arpcache.unresol;
	for(m = *mbp; m; m = *mbp) {
		if(t >= m->param) {
			*mbp = m->next;
			unlock(&arpcache);
			mbfree(m);
			lock(&arpcache);
			goto again;
		}
		mbp = &m->next;
		i++;
	}
	if(mb->chan && i < 10) {
		mb->param = t + SECOND(10);
		mb->next = 0;
		*mbp = mb;
		unlock(&arpcache);
	} else {
		unlock(&arpcache);
		mbfree(mb);
	}

	/*
	 * send an arp request
	 */

	m = mballoc(Ensize+Arpsize, 0, Mbarp2);
	q = (Arppkt*)m->data;

	DEBUG("snd arp req target %I ip dest %I\n", ipgate, p->dst);

	memset(q->d, 0xff, Easize);		/* broadcast */
	hnputs(q->type, Arptype);
	hnputs(q->hrd, 1);
	hnputs(q->pro, Iptype);
	q->hln = Easize;
	q->pln = Pasize;
	hnputs(q->op, Arprequest);
	memmove(q->sha, ifc->ea, Easize);
	memmove(q->spa, ifc->ipa, Pasize);
	memset(q->tha, 0, Easize);
	memmove(q->tpa, ipgate, Pasize);

	send(ifc->reply, m);

	return;

found:
	len = mb->count;		/* includes Ensize+Ipsize+Ilsize */
	memmove(p->d, a->tha, Easize);
	p->vihl = IP_VER|IP_HLEN;
	p->tos = 0;
	p->ttl = 255;
	id = arpcache.idgen;
	if(id == 0)
		id = toytime() * 80021;
	arpcache.idgen = id+1;
	unlock(&arpcache);
	hnputs(p->id, id);
	hnputs(p->type, Iptype);

	/*
	 * If we dont need to fragment just send it
	 */
	if(len <= ETHERMAXTU) {
		hnputs(p->length, len-Ensize);
		p->frag[0] = 0;
		p->frag[1] = 0;
		p->cksum[0] = 0;
		p->cksum[1] = 0;
		hnputs(p->cksum, ipcsum(&p->vihl));

		send(ifc->reply, mb);
		return;
	}

	off = 0;
	len -= Ensize+Ipsize;		/* just ip data */

	while(len > 0) {
		dlen = (ETHERMAXTU-(Ensize+Ipsize)) & ~7;
		if(dlen > len)
			dlen = len;
		len -= dlen;

		/*
		 * use first frag in place,
		 * make copies of subsequent frags
		 * this saves a copy of a MTU-size buffer
		 */
		if(ORDER && off == 0) {
			m = 0;
			mb->count = (Ensize+Ipsize)+dlen;
			p = (Ippkt*)mb->data;
		} else {
			m = mballoc((Ensize+Ipsize)+dlen, 0, Mbip1);
			p = (Ippkt*)m->data;

			memmove(m->data, mb->data, Ensize+Ipsize);
			memmove(m->data+(Ensize+Ipsize),
				mb->data+(Ensize+Ipsize)+off, dlen);
		}

		hnputs(p->length, dlen+Ipsize);
		if(len == 0)
			hnputs(p->frag, off>>3);
		else
			hnputs(p->frag, (off>>3)|IP_MF);
		p->cksum[0] = 0;
		p->cksum[1] = 0;
		hnputs(p->cksum, ipcsum(&p->vihl));

		if(m)
			send(ifc->reply, m);

		off += dlen;
	}
	if(ORDER)
		send(ifc->reply, mb);
	else
		mbfree(mb);
}

void
ipsend(Msgbuf *mb)
{
	Ilp *ilp;
	Chan *cp;

	cp = mb->chan;
	if(cp == 0) {
		print("cp = 0\n");
		mbfree(mb);
		return;
	}
	ilp = cp->pdata;
	ipsend1(mb, cp->ifc, ilp->ipgate);
}

int
ipforme(uchar addr[Pasize], Ifc *ifc)
{
	ulong haddr;

	if(memcmp(addr, ifc->ipa, Pasize) == 0)
		return 1;

	haddr = nhgetl(addr);

	/* My subnet broadcast */
	if((haddr&ifc->mask) == (ifc->ipaddr&ifc->mask))
		return 1;

	/* Real ip broadcast */
	if(haddr == 0)
		return 1;

	/* Old style 255.255.255.255 address */
	if(haddr == ~0)
		return 1;

	return 0;
}

/*
 * ipcsum - Compute internet header checksums
 */
int
ipcsum(uchar *addr)
{
	int len;
	ulong sum = 0;

	len = (addr[0]&0xf) << 2;

	while(len > 0) {
		sum += (addr[0]<<8) | addr[1] ;
		len -= 2;
		addr += 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return sum^0xffff;
}

/* 
 * protcol checksum routine
 */

static	short	endian	= 1;
static	char*	aendian	= (char*)&endian;
#define	LITTLE	*aendian

int
ptclcsum(uchar *addr, int len)
{
	ulong losum, hisum, mdsum, x;
	ulong t1, t2;

	losum = 0;
	hisum = 0;
	mdsum = 0;

	x = 0;
	if((ulong)addr & 1) {
		if(len) {
			hisum += addr[0];
			len--;
			addr++;
		}
		x = 1;
	}
	while(len >= 16) {
		t1 = *(ushort*)(addr+0);
		t2 = *(ushort*)(addr+2);	mdsum += t1;
		t1 = *(ushort*)(addr+4);	mdsum += t2;
		t2 = *(ushort*)(addr+6);	mdsum += t1;
		t1 = *(ushort*)(addr+8);	mdsum += t2;
		t2 = *(ushort*)(addr+10);	mdsum += t1;
		t1 = *(ushort*)(addr+12);	mdsum += t2;
		t2 = *(ushort*)(addr+14);	mdsum += t1;
		mdsum += t2;
		len -= 16;
		addr += 16;
	}
	while(len >= 2) {
		mdsum += *(ushort*)addr;
		len -= 2;
		addr += 2;
	}
	if(x) {
		if(len)
			losum += addr[0];
		if(LITTLE)
			losum += mdsum;
		else
			hisum += mdsum;
	} else {
		if(len)
			hisum += addr[0];
		if(LITTLE)
			hisum += mdsum;
		else
			losum += mdsum;
	}

	losum += hisum >> 8;
	losum += (hisum & 0xff) << 8;
	while(hisum = losum>>16)
		losum = hisum + (losum & 0xffff);

	return ~losum & 0xffff;
}

static
void
cmd_arp(int argc, char *argv[])
{
	int h, i, j;
	Arpe *a;

	if(argc <= 1) {
		print("arp flush -- clear cache\n");
		print("arp print -- print cache\n");
		return;
	}
	for(i=1; i<argc; i++) {
		if(strcmp(argv[i], "flush") == 0) {
			lock(&arpcache);
			for(h=0; h<Nb; h++)
				memset(&arpcache.abkt[h], 0, sizeof(arpcache.abkt[0]));
			unlock(&arpcache);
			continue;
		}
		if(strcmp(argv[i], "print") == 0) {
			for(h=0; h<Nb; h++) {
				a = arpcache.abkt[h].arpe;
				for(j=0; j<Ne; j++,a++) {
					if(memcmp(arpcache.null, a->tpa, Pasize) == 0)
						continue;
					print("%-15I %E\n", a->tpa, a->tha);
					prflush();
				}
			}
			continue;
		}
	}
}
