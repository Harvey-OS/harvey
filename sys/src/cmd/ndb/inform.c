/* RFC2136 DNS inform - necessary for Win2k3 DNS servers */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include "dns.h"

enum {
	FQDNMAX	= 255,
};

char *errmsgs[] = {
	[0]  "ok",
	[1]  "request format error",
	[2]  "internal server error",
	[3]  "domain name does not exist",
	[4]  "request not supported",
	[5]  "permission denied",
	[6]  "domain name already exists",
	[7]  "resource record already exists",
	[8]  "resource record does not exist",
	[9]  "server not authoritative",
	[10] "domain name not in zone",
};

char *dnsrch[] = {
	"dnsdomain",
	"dom",
};

void
usage(void)
{
	fprint(2, "usage: %s [-x netmtpt]\n", argv0);
	exits("usage");
}

void
ding(void *, char *msg)
{
	if(strstr(msg, "alarm") != nil)
		noted(NCONT);
	noted(NDFLT);
}

int
g16(uchar **p)
{
	int n;

	n  = *(*p)++ << 8;
	n |= *(*p)++;
	return n;
}

void
p16(uchar **p, int n)
{
	*(*p)++ = n >> 8;
	*(*p)++ = n;
}

void
p32(uchar **p, int n)
{
	*(*p)++ = n >> 24;
	*(*p)++ = n >> 16;
	*(*p)++ = n >> 8;
	*(*p)++ = n;
}

void
pmem(uchar **p, void *v, int len)
{
	memmove(*p, v, len);
	*p += len;
}

void
pname(uchar **p, char *s)
{
	uchar *len;

	while (*s){
		len = (*p)++;
		while(*s && *s != '.')
			*(*p)++ = *s++;
		*len = *p - len - 1;
		if(*s == '.')
			s++;
	}
	*(*p)++ = 0;
}

void
main(int argc, char *argv[])
{
	int debug, len, fd;
	uint err;
	char *sysname, *dnsdomain, *dom, *ns, net[32];
	uchar *p, buf[4096], addr[IPv4addrlen], v6addr[IPaddrlen];
	ushort txid;
	Ndb *db;
	Ndbtuple *t, *tt;
	static char *query[] = { "dom", "dnsdomain", "ns", };

	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	setnetmtpt(net, sizeof net, nil);

	debug = 0;
	ns = nil;
	dom = nil;
	dnsdomain = nil;
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'x':
		setnetmtpt(net, sizeof net, EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 0)
		usage();

	if((sysname = getenv("sysname")) == nil)
		sysfatal("$sysname not set");

	if((db = ndbopen(nil)) == nil)
		sysfatal("can't open ndb: %r");
	tt = ndbipinfo(db, "sys", sysname, query, nelem(query));
	for(t = tt; t; t = t->entry)
		if(strcmp(t->attr, "ns") == 0)
			ns = t->val;
		else if(strcmp(t->attr, "dom") == 0)
			dom = t->val;
		else if(strcmp(t->attr, "dnsdomain") == 0)
			dnsdomain = t->val;
	ndbfree(tt);
	ndbclose(db);

	if(!ns)
		sysfatal("no relevant ns=");
	if(!dom)
		sysfatal("no relevant dom=");
	if(!dnsdomain)
		sysfatal("no relevant dnsdomain=");

	myipaddr(v6addr, net);
	memmove(addr, v6addr + IPaddrlen - IPv4addrlen, IPv4addrlen);

	if(debug){
		print("ip=%V\n", addr);
		print("ns=%s\n", ns);
		print("dnsdomain=%s\n", dnsdomain);
		print("dom=%s\n", dom);
	}

	if((fd = dial(netmkaddr(ns, "udp", "dns"), 0, 0, 0)) < 0)
		sysfatal("can't dial %s: %r", ns);

	txid = time(nil) + getpid();

	p = buf;
	p16(&p, txid);		/* ID */
	p16(&p, 5<<11);		/* flags */
	p16(&p, 1);		/* # Zones */
	p16(&p, 0);		/* # prerequisites */
	p16(&p, 2);		/* # updates */
	p16(&p, 0);		/* # additionals */

        pname(&p, dnsdomain);	/* zone */
	p16(&p, Tsoa);		/* zone type */
	p16(&p, Cin);		/* zone class */

	/* delete old name */
        pname(&p, dom);		/* name */
	p16(&p, Ta);		/* type: v4 addr */
	p16(&p, Call);		/* class */
	p32(&p, 0);		/* TTL */
	p16(&p, 0);		/* data len */

	/* add new A record */
	pname(&p, dom);		/* name */
	p16(&p, Ta);		/* type: v4 addr */
	p16(&p, Cin);		/* class */
	p32(&p, 60*60*25);	/* TTL (25 hours) */
	p16(&p, IPv4addrlen);	/* data len */
	pmem(&p, addr, IPv4addrlen);	/* v4 address */

	len = p - buf;
	if(write(fd, buf, len) != len)
		sysfatal("write failed: %r");

	notify(ding);
	alarm(3000);
	do{
		if(read(fd, buf, sizeof buf) < 0)
			sysfatal("timeout");
		p = buf;
	}while(g16(&p) != txid);
	alarm(0);

	close(fd);

	err = g16(&p) & 7;
	if(err != 0 && err != 7)	/* err==7 is just a "yes, I know" warning */
		if(err < nelem(errmsgs))
			sysfatal("%s", errmsgs[err]);
		else
			sysfatal("unknown dns server error %d", err);
	exits(0);
}
