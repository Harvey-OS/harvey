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
uint8_t v6Unspecified[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
uint8_t v6loopback[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};

uint8_t v6linklocal[IPaddrlen] = {
	0xfe, 0x80, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
uint8_t v6linklocalmask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6llpreflen = 8;	/* link-local prefix length in bytes */

uint8_t v6multicast[IPaddrlen] = {
	0xff, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
uint8_t v6multicastmask[IPaddrlen] = {
	0xff, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6mcpreflen = 1;	/* multicast prefix length */

uint8_t v6allnodesN[IPaddrlen] = {
	0xff, 0x01, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};
uint8_t v6allroutersN[IPaddrlen] = {
	0xff, 0x01, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x02
};
uint8_t v6allnodesNmask[IPaddrlen] = {
	0xff, 0xff, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6aNpreflen = 2;	/* all nodes (N) prefix */

uint8_t v6allnodesL[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};
uint8_t v6allroutersL[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x02
};
uint8_t v6allnodesLmask[IPaddrlen] = {
	0xff, 0xff, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
int v6aLpreflen = 2;	/* all nodes (L) prefix */

uint8_t v6solicitednode[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01,
	0xff, 0, 0, 0
};
uint8_t v6solicitednodemask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0x0, 0x0, 0x0
};
int v6snpreflen = 13;

uint16_t
ptclcsum(Block *bp, int offset, int len)
{
	uint8_t *addr;
	uint32_t losum, hisum;
	uint16_t csum;
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

#define CLASS(p) ((*(uint8_t*)(p))>>6)

void
ipv62smcast(uint8_t *smcast, uint8_t *a)
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
parsemac(uint8_t *to, char *from, int len)
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
uint32_t
iphash(uint8_t *sa, uint16_t sp, uint8_t *da, uint16_t dp)
{
	uint32_t kludge;
	kludge = ((sa[IPaddrlen-1]<<24) ^ (sp << 16) ^ (da[IPaddrlen-1]<<8) ^ dp );
	return kludge % Nipht;
#if 0
	somebody please figure this out.
	This can return a negative number.
	return ((sa[IPaddrlen-1]<<24) ^ (sp << 16) ^ (da[IPaddrlen-1]<<8) ^ dp ) % Nipht;
#endif
}

void
iphtadd(Ipht *ht, Conv *c)
{
	uint32_t hv;
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

	lock(&ht->l);
	h->next = ht->tab[hv];
	ht->tab[hv] = h;
	unlock(&ht->l);
}

void
iphtrem(Ipht *ht, Conv *c)
{
	uint32_t hv;
	Iphash **l, *h;

	hv = iphash(c->raddr, c->rport, c->laddr, c->lport);
	lock(&ht->l);
	for(l = &ht->tab[hv]; (*l) != nil; l = &(*l)->next)
		if((*l)->c == c){
			h = *l;
			(*l) = h->next;
			free(h);
			break;
		}
	unlock(&ht->l);
}

/* look for a matching conversation with the following precedence
 *	connected && raddr,rport,laddr,lport
 *	announced && laddr,lport
 *	announced && *,lport
 *	announced && laddr,*
 *	announced && *,*
 */
Conv*
iphtlook(Ipht *ht, uint8_t *sa, uint16_t sp, uint8_t *da, uint16_t dp)
{
	uint32_t hv;
	Iphash *h;
	Conv *c;

	/* exact 4 pair match (connection) */
	hv = iphash(sa, sp, da, dp);
	lock(&ht->l);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchexact)
			continue;
		c = h->c;
		if(sp == c->rport && dp == c->lport
		&& ipcmp(sa, c->raddr) == 0 && ipcmp(da, c->laddr) == 0){
			unlock(&ht->l);
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
			unlock(&ht->l);
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
			unlock(&ht->l);
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
			unlock(&ht->l);
			return c;
		}
	}

	/* look for something that matches anything */
	hv = iphash(IPnoaddr, 0, IPnoaddr, 0);
	for(h = ht->tab[hv]; h != nil; h = h->next){
		if(h->match != IPmatchany)
			continue;
		c = h->c;
		unlock(&ht->l);
		return c;
	}
	unlock(&ht->l);
	return nil;
}
