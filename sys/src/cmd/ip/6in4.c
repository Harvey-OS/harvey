/*
 * 6in4 - tunnel client for automatic 6to4 or configured v6-in-v4 tunnels
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

enum {
	IP_IPV6PROTO	= 41,
};

int anysender;
int gateway;

uchar local6[IPaddrlen];
uchar remote6[IPaddrlen];
uchar remote4[IPaddrlen];
uchar localmask[IPaddrlen];
uchar localnet[IPaddrlen];
uchar myip[IPaddrlen];

/* magic anycast address from rfc3068 */
uchar anycast6to4[IPv4addrlen] = { 192, 88, 99, 1 };

static char *net = "/net";

static int	badipv4(uchar*);
static int	badipv6(uchar*);
static void	ip2tunnel(int, int);
static void	tunnel2ip(int, int);

static void
usage(void)
{
	fprint(2, "usage: %s [-ag] [-x mtpt] [local6[/mask]] [remote4 [remote6]]\n",
		argv0);
	exits("Usage");
}

void
main(int argc, char **argv)
{
	int n, tunnel, ifc, cfd;
	char *p, *cl, *ir, *loc6;
	char buf[128], path[64];

	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);

	ARGBEGIN {
	case 'a':
		anysender++;
		break;
	case 'g':
		gateway++;
		break;
	case 'x':
		net = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (myipaddr(myip, net) < 0)
		sysfatal("can't find my ipv4 address on %s", net);

	if (argc < 1)
		loc6 = smprint("2002:%2.2x%2.2x:%2.2x%2.2x::1/48",
			myip[IPaddrlen - IPv4addrlen],
			myip[IPaddrlen - IPv4addrlen + 1],
			myip[IPaddrlen - IPv4addrlen + 2],
			myip[IPaddrlen - IPv4addrlen + 3]);
	else {
		loc6 = argv[0];
		argv++;
		argc--;
	}

	/* local v6 address (mask defaults to /128) */
	memcpy(localmask, IPallbits, sizeof localmask);
	p = strchr(loc6, '/');
	if (p != nil) {
		parseipmask(localmask, p);
		*p = 0;
	}
	parseip(local6, loc6);
	if (isv4(local6))
		usage();
	if (argc >= 1 && argv[0][0] == '/') {
		parseipmask(localmask, argv[0]);
		argv++;
		argc--;
	}

	/* remote v4 address (defaults to anycast 6to4) */
	if (argc >= 1) {
		parseip(remote4, argv[0]);
		if (!isv4(remote4))
			usage();
		argv++;
		argc--;
	} else {
		v4tov6(remote4, anycast6to4);
		anysender++;
	}

	/* remote v6 address (defaults to link-local w/ v4 as interface part) */
	if (argc >= 1) {
		parseip(remote6, argv[0]);
		if (isv4(remote4))
			usage();
		argv++;
		argc--;
	} else {
		remote6[0] = 0xFE;		/* link local */
		remote6[1] = 0x80;
		memcpy(remote6 + IPv4off, remote4 + IPv4off, IPv4addrlen);
	}
	USED(argv);
	if (argc != 0)
		usage();

	maskip(local6, localmask, localnet);

	/*
	 * open IPv6-in-IPv4 tunnel
	 */
	p = seprint(buf, buf + sizeof buf, "ipmux!proto=%2.2x;dst=%V",
		IP_IPV6PROTO, myip + IPv4off);
	if (!anysender)
		seprint(p, buf + sizeof buf, ";src=%V", remote4 + IPv4off);
	tunnel = dial(buf, 0, 0, 0);
	if (tunnel < 0)
		sysfatal("can't make 6in4 tunnel with dial str %s: %r", buf);

	/*
	 * open local IPv6 interface (as a packet interface)
	 */
	cl = smprint("%s/ipifc/clone", net);
	cfd = open(cl, ORDWR);			/* allocate a conversation */
	free(cl);
	n = 0;
	if (cfd < 0 || (n = read(cfd, buf, sizeof buf - 1)) <= 0)
		sysfatal("can't make packet interface: %r");
	buf[n] = 0;

	snprint(path, sizeof path, "%s/ipifc/%s/data", net, buf);
	ifc = open(path, ORDWR);
	if (ifc < 0 || fprint(cfd, "bind pkt") < 0)
		sysfatal("can't bind packet interface: %r");
	/* 1280 is MTU, apparently from rfc2460 */
	if (fprint(cfd, "add %I /128 %I 1280", local6, remote6) <= 0)
		sysfatal("can't set local ipv6 address: %r");
	close(cfd);

	if (gateway) {
		/* route global addresses through the tunnel to remote6 */
		ir = smprint("%s/iproute", net);
		cfd = open(ir, OWRITE);
		free(ir);
		if (cfd < 0 || fprint(cfd, "add 2000:: /3 %I", remote6) <= 0)
			sysfatal("can't set default global route: %r");
	}

	switch (rfork(RFPROC|RFNOWAIT|RFMEM)) {
	case -1:
		sysfatal("rfork");
	case 0:
		ip2tunnel(ifc, tunnel);
		break;
	default:
		tunnel2ip(tunnel, ifc);
		break;
	}
	exits("tunnel gone");
}

typedef struct Iphdr Iphdr;
typedef struct Ip6hdr Ip6hdr;

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
	uchar	src[4];		/* Ip source (uchar ordering unimportant) */
	uchar	dst[4];		/* Ip destination (uchar ordering unimportant) */
};

struct	Ip6hdr {
	uchar	vcf[4];		/* version:4, traffic class:8, flow label:20 */
	uchar	ploadlen[2];	/* payload length: packet length - 40 */
	uchar	proto;		/* next header type */
	uchar	ttl;		/* hop limit */
	uchar	src[IPaddrlen];
	uchar	dst[IPaddrlen];
};

#define STFHDR (sizeof(Iphdr))

static void
ip2tunnel(int in, int out)
{
	int n, m;
	char buf[64*1024];
	Iphdr *op;
	Ip6hdr *ip;

	op = (Iphdr*)buf;
	op->vihl = 0x45;		/* v4, hdr is 5 longs? */
	memcpy(op->src, myip + IPv4off, sizeof op->src);
	op->proto = IP_IPV6PROTO;
	op->ttl = 100;

	/* get a V6 packet destined for the tunnel */
	while ((n = read(in, buf + STFHDR, sizeof buf - STFHDR)) > 0) {
		/* if not IPV6, drop it */
		ip = (Ip6hdr*)(buf + STFHDR);
		if ((ip->vcf[0]&0xF0) != 0x60)
			continue;

		/* check length: drop if too short, trim if too long */
		m = nhgets(ip->ploadlen) + sizeof(Ip6hdr);
		if (m > n)
			continue;
		if (m < n)
			n = m;

		/* drop if v6 source or destination address is naughty */
		if (badipv6(ip->src) ||
		    (!equivip6(ip->dst, remote6) && badipv6(ip->dst))) {
			syslog(0, "6in4", "egress filtered %I -> %I",
				ip->src, ip->dst);
			continue;
		}

		/* send 6to4 packets (2002::) directly to ipv4 target */
		if (ip->dst[0] == 0x20 && ip->dst[1] == 0x02)
			memcpy(op->dst, ip->dst+2, sizeof op->dst);
		else
			memcpy(op->dst, remote4+IPv4off, sizeof op->dst);

		n += STFHDR;
		/* pass packet to the other end of the tunnel */
		if (write(out, op, n) != n) {
			syslog(0, "6in4", "error writing to tunnel (%r), giving up");
			break;
		}
	}
}

static void
tunnel2ip(int in, int out)
{
	int n, m;
	char buf[64*1024];
	uchar a[IPaddrlen];
	Ip6hdr *op;
	Iphdr *ip;

	for (;;) {
		/* get a packet from the tunnel */
		n = read(in, buf, sizeof buf);
		ip = (Iphdr*)(buf + IPaddrlen);
		n -= IPaddrlen;
		if (n <= 0) {
			syslog(0, "6in4", "error reading from tunnel (%r), giving up");
			break;
		}

		/* if not IPv4 nor IP protocol IPv6, drop it */
		if ((ip->vihl&0xF0) != 0x40 || ip->proto != IP_IPV6PROTO)
			continue;

		/* check length: drop if too short, trim if too long */
		m = nhgets(ip->length);
		if (m > n)
			continue;
		if (m < n)
			n = m;

		op = (Ip6hdr*)(buf + IPaddrlen + STFHDR);
		n -= STFHDR;

		/* don't relay: just accept packets for local host/subnet */
		/* (this blocks link-local and multicast addresses as well) */
		maskip(op->dst, localmask, a);
		if (!equivip6(a, localnet)) {
			syslog(0, "6in4", "ingress filtered %I -> %I",
				op->src, op->dst);
			continue;
		}

		/* pass V6 packet to the interface */
		write(out, op, n);
	}
}

static int
badipv4(uchar *a)
{
	switch (a[0]) {
	case 0:				/* unassigned */
	case 10:			/* private */
	case 127:			/* loopback */
		return 1;
	case 172:
		return a[1] >= 16;	/* 172.16.0.0/12 private */
	case 192:
		return a[1] == 168;	/* 192.168.0.0/16 private */
	case 169:
		return a[1] == 254;	/* 169.254.0.0/16 DHCP link-local */
	}
	/* 224.0.0.0/4 multicast, 240.0.0.0/4 reserved, broadcast */
	return a[0] >= 240;
}

static int
badipv6(uchar *a)
{
	int h = a[0]<<8 | a[1];

	if (h == 0 ||		/* compatible, mapped, loopback, unspecified ... */
	    h >= 0xFE80)	/* multicast, link-local or site-local */
		return 1;
	if (h == 0x2002 &&	/* 6to4 address */
	    badipv4(a+2))
		return 1;
	return 0;
}
