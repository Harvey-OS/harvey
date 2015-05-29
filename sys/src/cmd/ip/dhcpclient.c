/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dhcp.h"

void	bootpdump(uint8_t *p, int n);
void	dhcpinit(void);
void	dhcprecv(void);
void	dhcpsend(int);
void	myfatal(char *fmt, ...);
int	openlisten(char*);
uint8_t	*optaddaddr(uint8_t*, int, uint8_t*);
uint8_t	*optaddbyte(uint8_t*, int, int);
uint8_t	*optadd(uint8_t*, int, void*, int);
uint8_t	*optaddulong(uint8_t*, int, uint32_t);
uint8_t	*optget(Bootp*, int, int);
int	optgetaddr(Bootp*, int, uint8_t*);
int	optgetbyte(Bootp*, int);
uint32_t	optgetulong(Bootp*, int);
Bootp	*parse(uint8_t*, int);
void	stdinthread(void*);
uint32_t	thread(void(*f)(void*), void *a);
void	timerthread(void*);
void	usage(void);

struct {
	QLock	lk;
	int	state;
	int	fd;
	uint32_t	xid;
	uint32_t	starttime;
	char	cid[100];
	char	sname[64];
	uint8_t	server[IPaddrlen];		/* server IP address */
	uint8_t	client[IPaddrlen];		/* client IP address */
	uint8_t	mask[IPaddrlen];		/* client mask */
	uint32_t	lease;		/* lease time */
	uint32_t	resend;		/* number of resends for current state */
	uint32_t	timeout;	/* time to timeout - seconds */
} dhcp;

char	net[64];

char optmagic[4] = { 0x63, 0x82, 0x53, 0x63 };

void
main(int argc, char *argv[])
{
	char *p;

	setnetmtpt(net, sizeof(net), nil);

	ARGBEGIN{
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(net, sizeof(net), p);
	}ARGEND;

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);

	dhcpinit();

	rfork(RFNOTEG|RFREND);

	thread(timerthread, 0);
	thread(stdinthread, 0);

	qlock(&dhcp.lk);
	dhcp.starttime = time(0);
	dhcp.fd = openlisten(net);
	dhcpsend(Discover);
	dhcp.state = Sselecting;
	dhcp.resend = 0;
	dhcp.timeout = 4;

	while(dhcp.state != Sbound)
		dhcprecv();

	/* allows other clients on this machine */
	close(dhcp.fd);
	dhcp.fd = -1;

	print("ip=%I\n", dhcp.client);
	print("mask=%I\n", dhcp.mask);
	print("end\n");

	/* keep lease alive */
	for(;;) {
//fprint(2, "got lease for %d\n", dhcp.lease);
		qunlock(&dhcp.lk);
		sleep(dhcp.lease*500);	/* wait half of lease time */
		qlock(&dhcp.lk);

//fprint(2, "try renue\n", dhcp.lease);
		dhcp.starttime = time(0);
		dhcp.fd = openlisten(net);
		dhcp.xid = time(0)*getpid();
		dhcpsend(Request);
		dhcp.state = Srenewing;
		dhcp.resend = 0;
		dhcp.timeout = 1;

		while(dhcp.state != Sbound)
			dhcprecv();

		/* allows other clients on this machine */
		close(dhcp.fd);
		dhcp.fd = -1;
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [-x netextension]\n", argv0);
	exits("usage");
}

void
timerthread(void* v)
{
	for(;;) {
		sleep(1000);
		qlock(&dhcp.lk);
		if(--dhcp.timeout > 0) {
			qunlock(&dhcp.lk);
			continue;
		}

		switch(dhcp.state) {
		default:
			myfatal("timerthread: unknown state %d", dhcp.state);
		case Sinit:
			break;
		case Sselecting:
			dhcpsend(Discover);
			dhcp.timeout = 4;
			dhcp.resend++;
			if(dhcp.resend>5)
				myfatal("dhcp: giving up: selecting");
			break;
		case Srequesting:
			dhcpsend(Request);
			dhcp.timeout = 4;
			dhcp.resend++;
			if(dhcp.resend>5)
				myfatal("dhcp: giving up: requesting");
			break;
		case Srenewing:
			dhcpsend(Request);
			dhcp.timeout = 1;
			dhcp.resend++;
			if(dhcp.resend>3) {
				dhcp.state = Srebinding;
				dhcp.resend = 0;
			}
			break;
		case Srebinding:
			dhcpsend(Request);
			dhcp.timeout = 4;
			dhcp.resend++;
			if(dhcp.resend>5)
				myfatal("dhcp: giving up: rebinding");
			break;
		case Sbound:
			break;
		}
		qunlock(&dhcp.lk);
	}
}

void
stdinthread(void* v)
{
	uint8_t buf[100];
	int n;

	for(;;) {
		n = read(0, buf, sizeof(buf));
		if(n <= 0)
			break;
	}
	/* shutdown cleanly */
	qlock(&dhcp.lk);
	if(dhcp.client) {
		if(dhcp.fd < 0)
			dhcp.fd = openlisten(net);
		dhcpsend(Release);
	}
	qunlock(&dhcp.lk);

	postnote(PNGROUP, getpid(), "die");
	exits(0);
}

void
dhcpinit(void)
{
	int fd;

	dhcp.state = Sinit;
	dhcp.timeout = 4;

	fd = open("/dev/random", 0);
	if(fd >= 0) {
		read(fd, &dhcp.xid, sizeof(dhcp.xid));
		close(fd);
	} else
		dhcp.xid = time(0)*getpid();
	srand(dhcp.xid);

	sprint(dhcp.cid, "%s.%d", getenv("sysname"), getpid());
}

void
dhcpsend(int type)
{
	int n;
	uint8_t *p;
	Bootp bp;
	Udphdr *up;

	memset(&bp, 0, sizeof bp);
	up = (Udphdr*)bp.udphdr;

	hnputs(up->rport, 67);
	bp.op = Bootrequest;
	hnputl(bp.xid, dhcp.xid);
	hnputs(bp.secs, time(0) - dhcp.starttime);
	hnputs(bp.flags, Fbroadcast);		/* reply must be broadcast */
	memmove(bp.optmagic, optmagic, 4);
	p = bp.optdata;
	p = optaddbyte(p, ODtype, type);
	p = optadd(p, ODclientid, dhcp.cid, strlen(dhcp.cid));
	switch(type) {
	default:
		myfatal("dhcpsend: unknown message type: %d", type);
	case Discover:
		ipmove(up->raddr, IPv4bcast);	/* broadcast */
		break;
	case Request:
		if(dhcp.state == Sbound || dhcp.state == Srenewing)
			ipmove(up->raddr, dhcp.server);
		else
			ipmove(up->raddr, IPv4bcast);	/* broadcast */
		p = optaddulong(p, ODlease, dhcp.lease);
		if(dhcp.state == Sselecting || dhcp.state == Srequesting) {
			p = optaddaddr(p, ODipaddr, dhcp.client);	/* mistake?? */
			p = optaddaddr(p, ODserverid, dhcp.server);
		} else
			v6tov4(bp.ciaddr, dhcp.client);
		break;
	case Release:
		ipmove(up->raddr, dhcp.server);
		v6tov4(bp.ciaddr, dhcp.client);
		p = optaddaddr(p, ODipaddr, dhcp.client);
		p = optaddaddr(p, ODserverid, dhcp.server);
		break;
	}

	*p++ = OBend;

	n = p - (uint8_t*)&bp;

	if(write(dhcp.fd, &bp, n) != n)
		myfatal("dhcpsend: write failed: %r");
}

void
dhcprecv(void)
{
	uint8_t buf[2000];
	Bootp *bp;
	int n, type;
	uint32_t lease;
	uint8_t mask[IPaddrlen];

	qunlock(&dhcp.lk);
	n = read(dhcp.fd, buf, sizeof(buf));
	qlock(&dhcp.lk);

	if(n <= 0)
		myfatal("dhcprecv: bad read: %r");

	bp = parse(buf, n);
	if(bp == 0)
		return;

if(1) {
fprint(2, "recved\n");
bootpdump(buf, n);
}

	type = optgetbyte(bp, ODtype);
	switch(type) {
	default:
		fprint(2, "dhcprecv: unknown type: %d\n", type);
		break;
	case Offer:
		if(dhcp.state != Sselecting)
			break;
		lease = optgetulong(bp, ODlease);
		if(lease == 0)
			myfatal("bad lease");
		if(!optgetaddr(bp, OBmask, mask))
			memset(mask, 0xff, sizeof(mask));
		v4tov6(dhcp.client, bp->yiaddr);
		if(!optgetaddr(bp, ODserverid, dhcp.server)) {
			fprint(2, "dhcprecv: Offer from server with invalid serverid\n");
			break;
		}

		dhcp.lease = lease;
		ipmove(dhcp.mask, mask);
		memmove(dhcp.sname, bp->sname, sizeof(dhcp.sname));
		dhcp.sname[sizeof(dhcp.sname)-1] = 0;

		dhcpsend(Request);
		dhcp.state = Srequesting;
		dhcp.resend = 0;
		dhcp.timeout = 4;
		break;
	case Ack:
		if(dhcp.state != Srequesting)
		if(dhcp.state != Srenewing)
		if(dhcp.state != Srebinding)
			break;
		lease = optgetulong(bp, ODlease);
		if(lease == 0)
			myfatal("bad lease");
		if(!optgetaddr(bp, OBmask, mask))
			memset(mask, 0xff, sizeof(mask));
		v4tov6(dhcp.client, bp->yiaddr);
		dhcp.lease = lease;
		ipmove(dhcp.mask, mask);
		dhcp.state = Sbound;
		break;
	case Nak:
		myfatal("recved nak");
		break;
	}

}

int
openlisten(char *net)
{
	int n, fd, cfd;
	char data[128], devdir[40];

//	sprint(data, "%s/udp!*!bootpc", net);
	sprint(data, "%s/udp!*!68", net);
	for(n = 0; ; n++) {
		cfd = announce(data, devdir);
		if(cfd >= 0)
			break;
		/* might be another client - wait and try again */
		fprint(2, "dhcpclient: can't announce %s: %r", data);
		sleep(1000);
		if(n > 10)
			myfatal("can't announce: giving up: %r");
	}

	if(fprint(cfd, "headers") < 0)
		myfatal("can't set header mode: %r");

	sprint(data, "%s/data", devdir);
	fd = open(data, ORDWR);
	if(fd < 0)
		myfatal("open %s: %r", data);
	close(cfd);
	return fd;
}

uint8_t*
optadd(uint8_t *p, int op, void *d, int n)
{
	p[0] = op;
	p[1] = n;
	memmove(p+2, d, n);
	return p+n+2;
}

uint8_t*
optaddbyte(uint8_t *p, int op, int b)
{
	p[0] = op;
	p[1] = 1;
	p[2] = b;
	return p+3;
}

uint8_t*
optaddulong(uint8_t *p, int op, uint32_t x)
{
	p[0] = op;
	p[1] = 4;
	hnputl(p+2, x);
	return p+6;
}

uint8_t *
optaddaddr(uint8_t *p, int op, uint8_t *ip)
{
	p[0] = op;
	p[1] = 4;
	v6tov4(p+2, ip);
	return p+6;
}

uint8_t*
optget(Bootp *bp, int op, int n)
{
	int len, code;
	uint8_t *p;

	p = bp->optdata;
	for(;;) {
		code = *p++;
		if(code == OBpad)
			continue;
		if(code == OBend)
			return 0;
		len = *p++;
		if(code != op) {
			p += len;
			continue;
		}
		if(n && n != len)
			return 0;
		return p;
	}
}


int
optgetbyte(Bootp *bp, int op)
{
	uint8_t *p;

	p = optget(bp, op, 1);
	if(p == 0)
		return 0;
	return *p;
}

uint32_t
optgetulong(Bootp *bp, int op)
{
	uint8_t *p;

	p = optget(bp, op, 4);
	if(p == 0)
		return 0;
	return nhgetl(p);
}

int
optgetaddr(Bootp *bp, int op, uint8_t *ip)
{
	uint8_t *p;

	p = optget(bp, op, 4);
	if(p == 0)
		return 0;
	v4tov6(ip, p);
	return 1;
}

/* make sure packet looks ok */
Bootp *
parse(uint8_t *p, int n)
{
	int len, code;
	Bootp *bp;

	bp = (Bootp*)p;
	if(n < bp->optmagic - p) {
		fprint(2, "dhcpclient: parse: short bootp packet");
		return 0;
	}

	if(dhcp.xid != nhgetl(bp->xid)) {
		fprint(2, "dhcpclient: parse: bad xid: got %ux expected %lux\n",
			nhgetl(bp->xid), dhcp.xid);
		return 0;
	}

	if(bp->op != Bootreply) {
		fprint(2, "dhcpclient: parse: bad op\n");
		return 0;
	}

	n -= bp->optmagic - p;
	p = bp->optmagic;

	if(n < 4) {
		fprint(2, "dhcpclient: parse: not option data");
		return 0;
	}
	if(memcmp(optmagic, p, 4) != 0) {
		fprint(2, "dhcpclient: parse: bad opt magic %ux %ux %ux %ux\n",
			p[0], p[1], p[2], p[3]);
		return 0;
	}
	p += 4;
	n -= 4;
	while(n>0) {
		code = *p++;
		n--;
		if(code == OBpad)
			continue;
		if(code == OBend)
			return bp;
		if(n == 0) {
			fprint(2, "dhcpclient: parse: bad option: %d", code);
			return 0;
		}
		len = *p++;
		n--;
		if(len > n) {
			fprint(2, "dhcpclient: parse: bad option: %d", code);
			return 0;
		}
		p += len;
		n -= len;
	}

	/* fix up nonstandard packets */
	/* assume there is space */
	*p = OBend;

	return bp;
}

void
bootpdump(uint8_t *p, int n)
{
	int len, i, code;
	Bootp *bp;
	Udphdr *up;

	bp = (Bootp*)p;
	up = (Udphdr*)bp->udphdr;

	if(n < bp->optmagic - p) {
		fprint(2, "dhcpclient: short bootp packet");
		return;
	}

	fprint(2, "laddr=%I lport=%d raddr=%I rport=%d\n", up->laddr,
		nhgets(up->lport), up->raddr, nhgets(up->rport));
	fprint(2, "op=%d htype=%d hlen=%d hops=%d\n", bp->op, bp->htype,
		bp->hlen, bp->hops);
	fprint(2, "xid=%ux secs=%d flags=%ux\n", nhgetl(bp->xid),
		nhgets(bp->secs), nhgets(bp->flags));
	fprint(2, "ciaddr=%V yiaddr=%V siaddr=%V giaddr=%V\n",
		bp->ciaddr, bp->yiaddr, bp->siaddr, bp->giaddr);
	fprint(2, "chaddr=");
	for(i=0; i<16; i++)
		fprint(2, "%ux ", bp->chaddr[i]);
	fprint(2, "\n");
	fprint(2, "sname=%s\n", bp->sname);
	fprint(2, "file = %s\n", bp->file);

	n -= bp->optmagic - p;
	p = bp->optmagic;

	if(n < 4)
		return;
	if(memcmp(optmagic, p, 4) != 0)
		fprint(2, "dhcpclient: bad opt magic %ux %ux %ux %ux\n",
			p[0], p[1], p[2], p[3]);
	p += 4;
	n -= 4;

	while(n>0) {
		code = *p++;
		n--;
		if(code == OBpad)
			continue;
		if(code == OBend)
			break;
		if(n == 0) {
			fprint(2, " bad option: %d", code);
			return;
		}
		len = *p++;
		n--;
		if(len > n) {
			fprint(2, " bad option: %d", code);
			return;
		}
		switch(code) {
		default:
			fprint(2, "unknown option %d\n", code);
			for(i = 0; i<len; i++)
				fprint(2, "%ux ", p[i]);
		case ODtype:
			fprint(2, "DHCP type %d\n", p[0]);
			break;
		case ODclientid:
			fprint(2, "client id=");
			for(i = 0; i<len; i++)
				fprint(2, "%ux ", p[i]);
			fprint(2, "\n");
			break;
		case ODlease:
			fprint(2, "lease=%d\n", nhgetl(p));
			break;
		case ODserverid:
			fprint(2, "server id=%V\n", p);
			break;
		case OBmask:
			fprint(2, "mask=%V\n", p);
			break;
		case OBrouter:
			fprint(2, "router=%V\n", p);
			break;
		}
		p += len;
		n -= len;
	}
}

uint32_t
thread(void(*f)(void*), void *a)
{
	int pid;

	pid = rfork(RFNOWAIT|RFMEM|RFPROC);
	if(pid < 0)
		myfatal("rfork failed: %r");
	if(pid != 0)
		return pid;
	(*f)(a);
	return 0;	/* never reaches here */
}

void
myfatal(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", argv0, buf);
	postnote(PNGROUP, getpid(), "die");
	exits(buf);
}
