#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"ip.h"

/*
 *  well known IP addresses
 */
uchar IPv4bcast[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};
uchar IPv4allsys[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0xe0, 0, 0, 0x01
};
uchar IPv4allrouter[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0xe0, 0, 0, 0x02
};
uchar IPallbits[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};
uchar IPnoaddr[IPaddrlen];

/*
 *  prefix of all v4 addresses
 */
uchar v4prefix[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0, 0, 0, 0
};


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

static uchar prefixvals[256] =
{
[0x00] 0 | Isprefix,
[0x80] 1 | Isprefix,
[0xC0] 2 | Isprefix,
[0xE0] 3 | Isprefix,
[0xF0] 4 | Isprefix,
[0xF8] 5 | Isprefix,
[0xFC] 6 | Isprefix,
[0xFE] 7 | Isprefix,
[0xFF] 8 | Isprefix,
};

int
eipfmt(Fmt *f)
{
	char buf[5*8];
	static char *efmt = "%.2lux%.2lux%.2lux%.2lux%.2lux%.2lux";
	static char *ifmt = "%d.%d.%d.%d";
	uchar *p, ip[16];
	ulong *lp;
	ushort s;
	int i, j, n, eln, eli;

	switch(f->r) {
	case 'E':		/* Ethernet address */
		p = va_arg(f->args, uchar*);
		snprint(buf, sizeof buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		return fmtstrcpy(f, buf);

	case 'I':		/* Ip address */
		p = va_arg(f->args, uchar*);
common:
		if(memcmp(p, v4prefix, 12) == 0){
			snprint(buf, sizeof buf, ifmt, p[12], p[13], p[14], p[15]);
			return fmtstrcpy(f, buf);
		}

		/* find longest elision */
		eln = eli = -1;
		for(i = 0; i < 16; i += 2){
			for(j = i; j < 16; j += 2)
				if(p[j] != 0 || p[j+1] != 0)
					break;
			if(j > i && j - i > eln){
				eli = i;
				eln = j - i;
			}
		}

		/* print with possible elision */
		n = 0;
		for(i = 0; i < 16; i += 2){
			if(i == eli){
				n += sprint(buf+n, "::");
				i += eln;
				if(i >= 16)
					break;
			} else if(i != 0)
				n += sprint(buf+n, ":");
			s = (p[i]<<8) + p[i+1];
			n += sprint(buf+n, "%ux", s);
		}
		return fmtstrcpy(f, buf);

	case 'i':		/* v6 address as 4 longs */
		lp = va_arg(f->args, ulong*);
		for(i = 0; i < 4; i++)
			hnputl(ip+4*i, *lp++);
		p = ip;
		goto common;

	case 'V':		/* v4 ip address */
		p = va_arg(f->args, uchar*);
		snprint(buf, sizeof buf, ifmt, p[0], p[1], p[2], p[3]);
		return fmtstrcpy(f, buf);

	case 'M':		/* ip mask */
		p = va_arg(f->args, uchar*);

		/* look for a prefix mask */
		for(i = 0; i < 16; i++)
			if(p[i] != 0xff)
				break;
		if(i < 16){
			if((prefixvals[p[i]] & Isprefix) == 0)
				goto common;
			for(j = i+1; j < 16; j++)
				if(p[j] != 0)
					goto common;
			n = 8*i + (prefixvals[p[i]] & ~Isprefix);
		} else
			n = 8*16;

		/* got one, use /xx format */
		snprint(buf, sizeof buf, "/%d", n);
		return fmtstrcpy(f, buf);
	}
	return fmtstrcpy(f, "(eipfmt)");
}

#define CLASS(p) ((*(uchar*)(p))>>6)

extern char*
v4parseip(uchar *to, char *from)
{
	int i;
	char *p;

	p = from;
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoul(p, &p, 0);
		if(*p == '.')
			p++;
	}
	switch(CLASS(to)){
	case 0:	/* class A - 1 uchar net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 uchar net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
	return p;
}

int
isv4(uchar *ip)
{
	return memcmp(ip, v4prefix, IPv4off) == 0;
}


/*
 *  the following routines are unrolled with no memset's to speed
 *  up the usual case
 */
void
v4tov6(uchar *v6, uchar *v4)
{
	v6[0] = 0;
	v6[1] = 0;
	v6[2] = 0;
	v6[3] = 0;
	v6[4] = 0;
	v6[5] = 0;
	v6[6] = 0;
	v6[7] = 0;
	v6[8] = 0;
	v6[9] = 0;
	v6[10] = 0xff;
	v6[11] = 0xff;
	v6[12] = v4[0];
	v6[13] = v4[1];
	v6[14] = v4[2];
	v6[15] = v4[3];
}

int
v6tov4(uchar *v4, uchar *v6)
{
	if(v6[0] == 0
	&& v6[1] == 0
	&& v6[2] == 0
	&& v6[3] == 0
	&& v6[4] == 0
	&& v6[5] == 0
	&& v6[6] == 0
	&& v6[7] == 0
	&& v6[8] == 0
	&& v6[9] == 0
	&& v6[10] == 0xff
	&& v6[11] == 0xff)
	{
		v4[0] = v6[12];
		v4[1] = v6[13];
		v4[2] = v6[14];
		v4[3] = v6[15];
		return 0;
	} else {
		memset(v4, 0, 4);
		return -1;
	}
}

ulong
parseip(uchar *to, char *from)
{
	int i, elipsis = 0, v4 = 1;
	ulong x;
	char *p, *op;

	memset(to, 0, IPaddrlen);
	p = from;
	for(i = 0; i < 16 && *p; i+=2){
		op = p;
		x = strtoul(p, &p, 16);
		if(*p == '.' || (*p == 0 && i == 0)){
			p = v4parseip(to+i, op);
			i += 4;
			break;
		} else {
			to[i] = x>>8;
			to[i+1] = x;
		}
		if(*p == ':'){
			v4 = 0;
			if(*++p == ':'){
				elipsis = i+2;
				p++;
			}
		}
	}
	if(i < 16){
		memmove(&to[elipsis+16-i], &to[elipsis], i-elipsis);
		memset(&to[elipsis], 0, 16-i);
	}
	if(v4){
		to[10] = to[11] = 0xff;
		return nhgetl(to+12);
	} else
		return 6;
}

/*
 *  hack to allow ip v4 masks to be entered in the old
 *  style
 */
ulong
parseipmask(uchar *to, char *from)
{
	ulong x;
	int i;
	uchar *p;

	if(*from == '/'){
		/* as a number of prefix bits */
		i = atoi(from+1);
		if(i < 0)
			i = 0;
		if(i > 128)
			i = 128;
		memset(to, 0, IPaddrlen);
		for(p = to; i >= 8; i -= 8)
			*p++ = 0xff;
		if(i > 0)
			*p = ~((1<<(8-i))-1);
		x = nhgetl(to+IPv4off);
	} else {
		/* as a straight bit mask */
		x = parseip(to, from);
		if(memcmp(to, v4prefix, IPv4off) == 0)
			memset(to, 0xff, IPv4off);
	}
	return x;
}

void
maskip(uchar *from, uchar *mask, uchar *to)
{
	int i;

	for(i = 0; i < IPaddrlen; i++)
		to[i] = from[i] & mask[i];
}

uchar classmask[4][16] = {
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0x00,0x00,
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0x00,
};

uchar*
defmask(uchar *ip)
{
	return classmask[ip[IPv4off]>>6];
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
	return ((sa[IPaddrlen-1]<<24) ^ (sp << 16) ^ (da[IPaddrlen-1]<<8) ^ dp ) % Nhash;
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
