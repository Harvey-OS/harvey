/*
 * dnrev - convert ip v4 or v6 addr to a dns ptr record name
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

enum {
	Nibwidth	= 4,
	Nibmask		= (1<<Nibwidth) - 1,
	V6maxrevdomdepth = 128 / Nibwidth,	/* v6 bits / bits-per-nibble */
};

static char *v4ptrdom = "in-addr.arpa";
static char *v6ptrdom = "ip6.arpa";	/* ip6.int deprecated, rfc 3152 */

char *
revv6(char *ip)
{
	char *s, *e;
	char rev[V6maxrevdomdepth*2 + 20];
	uchar *p;
	uchar addr[IPaddrlen];

	if (parseip(addr, ip) < 0)
		return nil;
	s = rev;
	e = rev + sizeof rev;
	for (p = addr + IPaddrlen - 1; p >= addr; p--)
		s = seprint(s, e, "%x.%x.", *p & Nibmask, *p >> Nibwidth);
	seprint(s, e, "%s", v6ptrdom);
	return strdup(rev);
}

char *
revv4(char *ip)
{
	int len;
	char *p, *np;
	char buf[256];

	if (*ip == '\0')
		return nil;
	len = strlen(ip);
	if (len > sizeof buf - 2)
		return nil;
	p = ip + len;
	p[0] = '.';
	p[1] = '\0';

	np = buf;
	len = 0;
	while(p >= ip){
		len++;
		p--;
		if(*p == '.'){
			memmove(np, p+1, len);
			np += len;
			len = 0;
		}
	}
	memmove(np, p+1, len);
	np += len;
	strcpy(np, v4ptrdom);
	return strdup(buf);
}
