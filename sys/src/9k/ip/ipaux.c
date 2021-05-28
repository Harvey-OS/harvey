#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"ip.h"
#include	"ipv6.h"

char *v6hdrtypes[Maxhdrtype] =
{
	[HBH]		"HopbyHop",
	[ICMP]		"ICMP",
	[IGMP]		"IGMP",
	[GGP]		"GGP",
	[IPINIP]	"IP",
	[ST]		"ST",
	[TCP]		"TCP",
	[UDP]		"UDP",
	[ISO_TP4]	"ISO_TP4",
	[RH]		"Routinghdr",
	[FH]		"Fraghdr",
	[IDRP]		"IDRP",
	[RSVP]		"RSVP",
	[AH]		"Authhdr",
	[ESP]		"ESP",
	[ICMPv6]	"ICMPv6",
	[NNH]		"Nonexthdr",
	[ISO_IP]	"ISO_IP",
	[IGRP]		"IGRP",
	[OSPF]		"OSPF",
};

/*
 *  well known IPv6 addresses
 */
uchar v6Unspecified[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
uchar v6loopback[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};

uchar v6linklocal[IPaddrlen] = {
	0xfe, 0x80, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
uchar v6linklocalmask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6llpreflen = 8;	/* link-local prefix length in bytes */

uchar v6multicast[IPaddrlen] = {
	0xff, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
uchar v6multicastmask[IPaddrlen] = {
	0xff, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6mcpreflen = 1;	/* multicast prefix length */

uchar v6allnodesN[IPaddrlen] = {
	0xff, 0x01, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};
uchar v6allroutersN[IPaddrlen] = {
	0xff, 0x01, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x02
};
uchar v6allnodesNmask[IPaddrlen] = {
	0xff, 0xff, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6aNpreflen = 2;	/* all nodes (N) prefix */

uchar v6allnodesL[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};
uchar v6allroutersL[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x02
};
uchar v6allnodesLmask[IPaddrlen] = {
	0xff, 0xff, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6aLpreflen = 2;	/* all nodes (L) prefix */

uchar v6solicitednode[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01,
	0xff, 0, 0, 0
};
uchar v6solicitednodemask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0x0, 0x0, 0x0
};
int v6snpreflen = 13;

ushort
ptclcsum(Block *bp, int offset, int len)
{
	uchar *addr;
	ulong losum, hisum;
	ushort csum;
	int odd, blocklen, x;

	/* Correct to front of data area */
	while(bp != nil && offset && offset >= BLEN(bp)) {
		offset -= BLEN(bp);
		bp = bp->next;
	}
	if(bp == nil)
		return 0;

	addr = bp->rp + offset;
	blocklen = BLEN(bp) - offset;

	if(bp->next == nil) {
		if(blocklen < len)
			len = blocklen;
		return ~ptclbsum(addr, len) & 0xffff;
	}

	losum = 0;
	hisum = 0;

	odd = 0;
	while(len) {
		x = blocklen;
		if(len < x)
			x = len;

		csum = ptclbsum(addr, x);
		if(odd)
			hisum += csum;
		else
			losum += csum;
		odd = (odd+x) & 1;
		len -= x;

		bp = bp->next;
		if(bp == nil)
			break;
		blocklen = BLEN(bp);
		addr = bp->rp;
	}

	losum += hisum>>8;
	losum += (hisum&0xff)<<8;
	while((csum = losum>>16) != 0)
		losum = csum + (losum & 0xffff);

	return ~losum & 0xffff;
}

enum
{
	Isprefix= 16,
};

#define CLASS(p) ((*(uchar*)(p))>>6)

void
ipv62smcast(uchar *smcast, uchar *a)
{
	assert(IPaddrlen == 16);
	memmove(smcast, v6solicitednode, IPaddrlen);
	smcast[13] = a[13];
	smcast[14] = a[14];
	smcast[15] = a[15];
}


/*
 *  parse a hex mac address
 */
int
parsemac(uchar *to, char *from, int len)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	memset(to, 0, len);
	for(i = 0; i < len; i++){
		if(p[0] == '\0' || p[1] == '\0')
			break;

		nip[0] = p[0];
		nip[1] = p[1];
		nip[2] = '\0';
		p += 2;

		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return i;
}

/*
 *  hashing tcp, udp, ... connections
 */
ulong
iphash(uchar *sa, ushort sp, uchar *da, ushort dp)
{
	return ((sa[IPaddrlen-1]<<24) ^ (sp << 16) ^ (da[IPaddrlen-1]<<8) ^ dp ) % Nipht;
}

void
iphtadd(Ipht *ht, Conv *c)
{
	ulong hv;
	Iphash *h;

	hv = iphash(c->raddr, c->rport, c->laddr, c->lport);
	h = smalloc(sizeof(*h));
	if(ipcmp(c->raddr, IPnoaddr) != 0)
		h->match = IPmatchexact;
	else {
		if(ipcmp(c->laddr, IPnoaddr) != 0){
			if(c->lport == 0)
				h->match = IPmatchaddr;
			else
				h->match = IPmatchpa;
		} else {
			if(c->lport == 0)
				h->match = IPmatchany;
			else
				h->match = IPmatchport;
		}
	}
	h->c = c;

	lock(ht);
	h->next = ht->tab[hv];
	ht->tab[hv] = h;
	unlock(ht);
}

void
iphtrem(Ipht *ht, Conv *c)
{
	ulong hv;
	Iphash **l, *h;

	hv = iphash(c->raddr, c->rport, c->laddr, c->lport);
	lock(ht);
	for(l = &ht->tab[hv]; (*l) != nil; l = &(*l)->next)
		if((*l)->c == c){
			h = *l;
			(*l) = h->next;
			free(h);
			break;
		}
	unlock(ht);
}

/* look for a matching conversation with the following precedence
 *	connected && raddr,rport,laddr,lport
 *	announced && laddr,lport
 *	announced && *,lport
 *	announced && laddr,*
 *	announced && *,*
 */
Conv*
iphtlook(Ipht *ht, uchar *sa, ushort sp, uchar *da, ushort dp)
{
	ulong hv;
	Iphash *h;
	Conv *c;

	/* exact 4 pair match (connection) */
	hv = iphash(sa, sp, da, dp);
	lock(ht);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchexact)
			continue;
		c = h->c;
		if(sp == c->rport && dp == c->lport
		&& ipcmp(sa, c->raddr) == 0 && ipcmp(da, c->laddr) == 0){
			unlock(ht);
			return c;
		}
	}

	/* match local address and port */
	hv = iphash(IPnoaddr, 0, da, dp);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchpa)
			continue;
		c = h->c;
		if(dp == c->lport && ipcmp(da, c->laddr) == 0){
			unlock(ht);
			return c;
		}
	}

	/* match just port */
	hv = iphash(IPnoaddr, 0, IPnoaddr, dp);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchport)
			continue;
		c = h->c;
		if(dp == c->lport){
			unlock(ht);
			return c;
		}
	}

	/* match local address */
	hv = iphash(IPnoaddr, 0, da, 0);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchaddr)
			continue;
		c = h->c;
		if(ipcmp(da, c->laddr) == 0){
			unlock(ht);
			return c;
		}
	}

	/* look for something that matches anything */
	hv = iphash(IPnoaddr, 0, IPnoaddr, 0);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchany)
			continue;
		c = h->c;
		unlock(ht);
		return c;
	}
	unlock(ht);
	return nil;
}
