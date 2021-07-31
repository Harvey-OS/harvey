#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "dat.h"

//
//	ala rfc2131
//

typedef struct Req Req;
struct Req
{
	int	fd;			/* for reply */
	Bootp	*bp;
	Udphdr	*up;
	uchar	*e;			/* end of received message */
	uchar	*p;			/* options pointer */
	uchar	*max;			/* max end of reply */

	/* expanded to v6 */
	uchar	ciaddr[IPaddrlen];
	uchar	giaddr[IPaddrlen];

	/* parsed options */
	int	p9request;		/* true if this is a bootp with plan9 options */
	int	genrequest;		/* true if this is a bootp with generic options */
	int	dhcptype;		/* dhcp message type */
	int	leasetime;		/* dhcp lease */
	uchar	ip[IPaddrlen];		/* requested address */
	uchar	server[IPaddrlen];	/* server address */
	char	msg[ERRMAX];		/* error message */
	char	vci[32];		/* vendor class id */
	char	*id;			/* client id */
	uchar	requested[32];		/* requested params */
	uchar	vendorclass[32];
	char	cputype[32-3];

	Info	gii;			/* about target network */
	Info	ii;			/* about target system */
	int	staticbinding;

	uchar buf[2*1024];		/* message buffer */
};

#define TFTP "/lib/tftpd"
char	*blog = "ipboot";
char	mysysname[64];
Ipifc	*ipifcs;
int	debug;
int	nobootp;
long	now;
int	slow;
char	net[256];

int	pptponly;	// only answer request that came from the pptp server
int	mute;
int	minlease = MinLease;	

/* option magic */
char plan9opt[4] = { 'p', '9', ' ', ' ' };
char genericopt[4] = { 0x63, 0x82, 0x53, 0x63 };

/* well known addresses */
uchar zeros[Maxhwlen];

void	addropt(Req*, int, uchar*);
void	addrsopt(Req*, int, uchar**, int);
void	arpenter(uchar*, uchar*);
void	bootp(Req*);
void	byteopt(Req*, int, uchar);
void	dhcp(Req*);
void	fatal(int, char*, ...);
void	hexopt(Req*, int, char*);
void	longopt(Req*, int, long);
void	miscoptions(Req*, uchar*);
int	openlisten(char *net);
void	parseoptions(Req*);
void	debugoptions(Req*, char*, char*);
void	proto(Req*, int);
void	rcvdecline(Req*);
void	rcvdiscover(Req*);
void	rcvinform(Req*);
void	rcvrelease(Req*);
void	rcvrequest(Req*);
char*	readsysname(void);
void	remrequested(Req*, int);
void	sendack(Req*, uchar*, int, int);
void	sendnak(Req*, char*);
void	sendoffer(Req*, uchar*, int);
void	stringopt(Req*, int, char*);
void	termopt(Req*);
int	validip(uchar*);
void	vectoropt(Req*, int, uchar*, int);
void	warning(int, char*, ...);
void	logdhcp(Req*);

void
usage(void)
{
	fprint(2, "usage: dhcp [-dmsnp] [-f directory] [-x netmtpt] [-M minlease] addr n [addr n ...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int n, fd;
	char *p;
	uchar ip[IPaddrlen];
	Req r;

	setnetmtpt(net, sizeof(net), nil);

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);
	ARGBEGIN {
	case 'm':
		mute = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'f':
		p = ARGF();
		if(p == nil)
			usage();
		ndbfile = p;
		break;
	case 's':
		slow = 1;
		break;
	case 'n':
		nobootp = 1;
		break;
	case 'p':
		pptponly = 1;
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(net, sizeof(net), p);
		break;
	case 'M':
		p = ARGF();
		if(p == nil)
			usage();
		minlease = atoi(p);
		if(minlease <= 0)
			minlease = MinLease;
		break;
	} ARGEND;

	while(argc > 1){
		parseip(ip, argv[0]);
		if(!validip(ip))
			usage();
		n = atoi(argv[1]);
		if(n <= 0)
			usage();
		initbinding(ip, n);
		argc -= 2;
		argv += 2;
	}

	/* what is my name? */
	p = readsysname();
	strcpy(mysysname, p);

	/* put process in background */
	if(!debug) switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
	case -1:
		fatal(1, "fork");
	case 0:
		break;
	default:
		exits(0);
	}

	chdir(TFTP);
	fd = openlisten(net);

	for(;;){
		memset(&r, 0, sizeof(r));
		r.fd = fd;
		n = read(r.fd, r.buf, sizeof(r.buf));
		if(n < Udphdrsize)
			fatal(1, "error reading requests");
		proto(&r, n);
		if(r.id != nil)
			free(r.id);
	}
}

void
proto(Req *rp, int n)
{
	uchar relip[IPaddrlen];
	char buf[64];

	now = time(0);

	rp->e = rp->buf + n;
	rp->bp = (Bootp*)rp->buf;
	rp->up = (Udphdr*)rp->buf;
	rp->max = rp->buf + Udphdrsize + MINSUPPORTED - IPUDPHDRSIZE;
	rp->p = rp->bp->optdata;
	v4tov6(rp->giaddr, rp->bp->giaddr);
	v4tov6(rp->ciaddr, rp->bp->ciaddr);

	if(pptponly && rp->bp->htype != 0)
		return;

	ipifcs = readipifc(net, ipifcs, -1);
	if(validip(rp->giaddr))
		ipmove(relip, rp->giaddr);
	else if(validip(rp->up->raddr))
		ipmove(relip, rp->up->raddr);
	else
		ipmove(relip, rp->up->laddr);
	if(rp->e < (uchar*)rp->bp->sname){
		warning(0, "packet too short");
		return;
	}
	if(rp->bp->op != Bootrequest){
		warning(0, "not bootrequest");
		return;
	}

	if(rp->e >= rp->bp->optdata){
		if(memcmp(rp->bp->optmagic, plan9opt, sizeof(rp->bp->optmagic)) == 0)
			rp->p9request = 1;
		if(memcmp(rp->bp->optmagic, genericopt, sizeof(rp->bp->optmagic)) == 0) {
			rp->genrequest = 1;
			parseoptions(rp);
			rp->p = rp->bp->optdata;
		}
	}

	/*  If no id is specified, make one from the hardware address
	 *  of the target.  We assume all zeros is not a hardware address
	 *  which could be a mistake.
	 */
	if(rp->id == nil){
		if(rp->bp->hlen > Maxhwlen){
			warning(0, "hlen %d", rp->bp->hlen);
			return;
		}
		if(memcmp(zeros, rp->bp->chaddr, rp->bp->hlen) == 0){
			warning(0, "no chaddr");
			return;
		}
		sprint(buf, "hwa%2.2ux_", rp->bp->htype);
		rp->id = tohex(buf, rp->bp->chaddr, rp->bp->hlen);
	}

	/* info about gateway */
	if(lookupip(relip, &rp->gii) < 0){
		warning(0, "lookupip failed");
		return;
	}

	/* info about target system */
	if(lookup(rp->bp, &rp->ii, &rp->gii) == 0)
		if(rp->ii.indb && rp->ii.dhcpgroup[0] == 0)
			rp->staticbinding = 1;

	if(rp->dhcptype)
		dhcp(rp);
	else
		bootp(rp);
}

void
dhcp(Req *rp)
{
	logdhcp(rp);

	switch(rp->dhcptype){
	case Discover:
		if(slow)
			sleep(500);
		rcvdiscover(rp);
		break;
	case Request:
		rcvrequest(rp);
		break;
	case Decline:
		rcvdecline(rp);
		break;
	case Release:
		rcvrelease(rp);
		break;
	case Inform:
		rcvinform(rp);
		break;
	}
}

void
rcvdiscover(Req *rp)
{
	Binding *b, *nb;

	if(rp->staticbinding){
		sendoffer(rp, rp->ii.ipaddr, StaticLease);
		return;
	}

	/*
	 *  first look for an outstanding offer
	 */
	b = idtooffer(rp->id, &rp->gii);

	/*
	 * rfc2131 says:
	 *   If an address is available, the new address
	 *   SHOULD be chosen as follows:
	 *
	 *      o The client's current address as recorded in the client's current
	 *        binding, ELSE
	 *
	 *      o The client's previous address as recorded in the client's (now
	 *        expired or released) binding, if that address is in the server's
	 *        pool of available addresses and not already allocated, ELSE
	 *
	 *      o The address requested in the 'Requested IP Address' option, if that
	 *        address is valid and not already allocated, ELSE
	 *
	 *      o A new address allocated from the server's pool of available
	 *        addresses; the address is selected based on the subnet from which
	 *        the message was received (if 'giaddr' is 0) or on the address of
	 *        the relay agent that forwarded the message ('giaddr' when not 0).
	 */
	if(b == nil){
		b = idtobinding(rp->id, &rp->gii, 1);
		if(b && b->boundto && strcmp(b->boundto, rp->id) != 0)
		if(validip(rp->ip) && samenet(rp->ip, &rp->gii)){
			nb = iptobinding(rp->ip, 0);
			if(nb && nb->lease < now)
				b = nb;
		}
	}
	if(b == nil){
		warning(0, "discover: no binding for %s ip %I via %I",
			rp->id, rp->ip, rp->gii.ipaddr);
		return;
	}
	mkoffer(b, rp->id, rp->leasetime);
	sendoffer(rp, b->ip, b->offer);
}

void
rcvrequest(Req *rp)
{
	Binding *b;

	if(validip(rp->server)){
		/* this is a reply to an offer - SELECTING */

		/* check for hard assignment */
		if(rp->staticbinding){
			if(forme(rp->server))
				sendack(rp, rp->ii.ipaddr, StaticLease, 1);
			else
				warning(0, "request: for server %I not me: %s via %I",
					rp->server, rp->id, rp->gii.ipaddr);
			return;
		}

		b = idtooffer(rp->id, &rp->gii);

		/* if we don't have an offer, nak */
		if(b == nil){
			warning(0, "request: no offer for %s via %I",
				rp->id, rp->gii.ipaddr);
			if(forme(rp->server))
				sendnak(rp, "no offer for you");
			return;
		}
	
		/* if not for me, retract offer */
		if(!forme(rp->server)){
			b->expoffer = 0;
			warning(0, "request: for server %I not me: %s via %I",
				rp->server, rp->id, rp->gii.ipaddr);
			return;
		}

		/*
		 *  if the client is confused about what we offered, nak.
		 *  client really shouldn't be specifying this when selecting
		 */
		if(validip(rp->ip) && ipcmp(rp->ip, b->ip) != 0){
			warning(0, "request: %s via %I requests %I, not %I",
				rp->id, rp->gii.ipaddr, rp->ip, b->ip);
			sendnak(rp, "bad ip address option");
			return;
		}
		if(commitbinding(b) < 0){
			warning(0, "request: can't commit %I for %s via %I",
				b->ip, rp->id, rp->gii.ipaddr);
			sendnak(rp, "can't commit binding");
			return;
		}
		sendack(rp, b->ip, b->offer, 1);
	} else if(validip(rp->ip)){
		/*
		 *  checking address/net - INIT-REBOOT
		 *
		 *  the standard says nothing about acking these requests,
		 *  only nacking them.  we ack nevertheless since some
		 *  boxes seem to require it.
		 */
		/* check for hard assignment */
		if(rp->staticbinding){
			if(memcmp(rp->ip, rp->ii.ipaddr, IPaddrlen) != 0){
				warning(0, "request: %I not valid for %s/%E via %I",
					rp->ip, rp->id, rp->bp->chaddr, rp->gii.ipaddr);
				sendnak(rp, "not valid");
			}
			sendack(rp, rp->ii.ipaddr, StaticLease, 1);
			return;
		}

		/* make sure the network makes sense */
		if(!samenet(rp->ip, &rp->gii)){
			warning(0, "request: bad forward of %I for %s via %I",
				rp->ip, rp->id, rp->gii.ipaddr);
			sendnak(rp, "wrong network");
			return;
		}
		b = iptobinding(rp->ip, 0);
		if(b == nil){
			warning(0, "request: no binding for %I for %s via %I",
				rp->ip, rp->id, rp->gii.ipaddr);
			return;
		}
		if(memcmp(rp->ip, b->ip, IPaddrlen) != 0 || now > b->lease){
			warning(0, "request: %I not valid for %s via %I",
				rp->ip, rp->id, rp->gii.ipaddr);
			sendnak(rp, "not valid");
			return;
		}
		b->offer = b->lease - now;
		sendack(rp, b->ip, b->offer, 1);
	} else if(validip(rp->ciaddr)){
		/* checking address - INIT-REBOOT and RENEWING or REBINDING */

		/* check for hard assignment */
		if(rp->staticbinding){
			if(ipcmp(rp->ciaddr, rp->ii.ipaddr) != 0){
				warning(0, "request: %I not valid for %s via %I",
					rp->ciaddr, rp->id, rp->gii.ipaddr);
				sendnak(rp, "not valid");
			}
			sendack(rp, rp->ii.ipaddr, StaticLease, 1);
			return;
		}

		/* make sure the network makes sense */
		if(!samenet(rp->ciaddr, &rp->gii)){
			warning(0, "request: bad forward of %I for %s via %I",
				rp->ip, rp->id, rp->gii.ipaddr);
			sendnak(rp, "wrong network");
			return;
		}
		b = iptobinding(rp->ciaddr, 0);
		if(b == nil){
			warning(0, "request: no binding for %I for %s via %I",
				rp->ciaddr, rp->id, rp->gii.ipaddr);
			return;
		}
		if(ipcmp(rp->ciaddr, b->ip) != 0){
			warning(0, "request: %I not valid for %s via %I",
				rp->ciaddr, rp->id, rp->gii.ipaddr);
			sendnak(rp, "invalid ip address");
			return;
		}
		mkoffer(b, rp->id, rp->leasetime);
		if(commitbinding(b) < 0){
			warning(0, "request: can't commit %I for %s via %I",
				b->ip, rp->id, rp->gii.ipaddr);
			sendnak(rp, "can't commit binding");
			return;
		}
		sendack(rp, b->ip, b->offer, 1);
	}
}

void
rcvdecline(Req *rp)
{
	Binding *b;
	char buf[64];

	if(rp->staticbinding)
		return;

	b = idtooffer(rp->id, &rp->gii);
	if(b == nil){
		warning(0, "decline: no binding for %s via %I",
			rp->id, rp->gii.ipaddr);
		return;
	}

	/* mark ip address as in use */
	snprint(buf, sizeof(buf), "declined by %s", rp->id);
	mkoffer(b, buf, 0x7fffffff);
	commitbinding(b);
}

void
rcvrelease(Req *rp)
{
	Binding *b;

	if(rp->staticbinding)
		return;

	b = idtobinding(rp->id, &rp->gii, 0);
	if(b == nil){
		warning(0, "release: no binding for %s via %I",
			rp->id, rp->gii.ipaddr);
		return;
	}
	if(strcmp(rp->id, b->boundto) != 0){
		warning(0, "release: invalid release of %I from %s via %I",
			rp->ip, rp->id, rp->gii.ipaddr);
		return;
	}
	warning(0, "release: releasing %I from %s", b->ip, b->boundto);
	if(releasebinding(b, rp->id) < 0)
		warning(0, "release: couldn't release");
}

void
rcvinform(Req *rp)
{
	Binding *b;

	if(rp->staticbinding){
		sendack(rp, rp->ii.ipaddr, 0, 0);
		return;
	}

	b = iptobinding(rp->ciaddr, 0);
	if(b == nil){
		warning(0, "inform: no binding for %I for %s via %I",
			rp->ip, rp->id, rp->gii.ipaddr);
		return;
	}
	sendack(rp, b->ip, 0, 0);
}

int
setsiaddr(char * attr, uchar *siaddr, uchar *ip, uchar *laddr)
{
	uchar *addrs[2];
	uchar x[2*IPaddrlen];

	addrs[0] = x;
	addrs[1] = x+IPaddrlen;
	if(lookupserver(attr, addrs, ip) > 0){
		v6tov4(siaddr, addrs[0]);
		return 0;
	} else {
		v6tov4(siaddr, laddr);
		return 1;
	}
}

void
sendoffer(Req *rp, uchar *ip, int offer)
{
	int n;
	ushort flags;
	Bootp *bp;
	Udphdr *up;

	bp = rp->bp;
	up = rp->up;

	/*
	 *  set destination
	 */
	flags = nhgets(bp->flags);
	if(validip(rp->giaddr)){
		ipmove(up->raddr, rp->giaddr);
		hnputs(up->rport, 67);
	} else if(flags & Fbroadcast){
		ipmove(up->raddr, IPv4bcast);
		hnputs(up->rport, 68);
	} else {
		ipmove(up->raddr, ip);
		if(bp->htype == 1)
			arpenter(up->raddr, bp->chaddr);
		hnputs(up->rport, 68);
	}

	/*
	 *  fill in standard bootp part
	 */
	bp->op = Bootreply;
	bp->hops = 0;
	hnputs(bp->secs, 0);
	memset(bp->ciaddr, 0, sizeof(bp->ciaddr));
	v6tov4(bp->giaddr, rp->giaddr);
	v6tov4(bp->yiaddr, ip);
	setsiaddr("tftp", bp->siaddr, ip, up->laddr);
	strncpy(bp->sname, mysysname, sizeof(bp->sname));
	strncpy(bp->file, rp->ii.bootf, sizeof(bp->file));

	/*
	 *  set options
	 */
	byteopt(rp, ODtype, Offer);
	longopt(rp, ODlease, offer);
	addropt(rp, ODserverid, up->laddr);
	miscoptions(rp, ip);
	termopt(rp);

	warning(0, "Offer %d to %s from %I, ip %I mask %M gw %I", offer, rp->id,
		up->laddr, ip, rp->gii.ipmask, rp->gii.gwip);

	/*
	 *  send
	 */
	n = rp->p - rp->buf;
	if(!mute && write(rp->fd, rp->buf, n) != n)
		warning(0, "offer: write failed: %r");
}

void
sendack(Req *rp, uchar *ip, int offer, int sendlease)
{
	int n;
	ushort flags;
	Bootp *bp;
	Udphdr *up;
	char buf[1024];

	bp = rp->bp;
	up = rp->up;

	/*
	 *  set destination
	 */
	flags = nhgets(bp->flags);
	if(validip(rp->giaddr)){
		ipmove(up->raddr, rp->giaddr);
		hnputs(up->rport, 67);
	} else if(flags & Fbroadcast){
		ipmove(up->raddr, IPv4bcast);
		hnputs(up->rport, 68);
	} else {
		ipmove(up->raddr, ip);
		if(bp->htype == 1)
			arpenter(up->raddr, bp->chaddr);
		hnputs(up->rport, 68);
	}

	/*
	 *  fill in standard bootp part
	 */
	bp->op = Bootreply;
	bp->hops = 0;
	hnputs(bp->secs, 0);
	v6tov4(bp->giaddr, rp->giaddr);
	v6tov4(bp->yiaddr, ip);
	setsiaddr("tftp", bp->siaddr, ip, up->laddr);
	strncpy(bp->sname, mysysname, sizeof(bp->sname));
	strncpy(bp->file, rp->ii.bootf, sizeof(bp->file));

	/*
	 *  set options
	 */
	byteopt(rp, ODtype, Ack);
	if(sendlease)
		longopt(rp, ODlease, offer);
	addropt(rp, ODserverid, up->laddr);
	miscoptions(rp, ip);
	termopt(rp);

	debugoptions(rp, buf, buf+sizeof(buf));
	warning(0, "Ack %d to %s: %I->%I, ip %I mask %M gw %I %s", offer, rp->id,
		up->laddr, up->raddr, ip, rp->gii.ipmask, rp->gii.gwip, buf);

	/*
	 *  send
	 */
	n = rp->p - rp->buf;
	if(!mute && write(rp->fd, rp->buf, n) != n)
		warning(0, "ack: write failed: %r");
}

void
sendnak(Req *rp, char *msg)
{
	int n;
	Bootp *bp;
	Udphdr *up;

	bp = rp->bp;
	up = rp->up;

	/*
	 *  set destination (always broadcast)
	 */
	if(validip(rp->giaddr)){
		ipmove(up->raddr, rp->giaddr);
		hnputs(up->rport, 67);
	} else {
		ipmove(up->raddr, IPv4bcast);
		hnputs(up->rport, 68);
	}

	/*
	 *  fill in standard bootp part
	 */
	bp->op = Bootreply;
	bp->hops = 0;
	hnputs(bp->secs, 0);
	v6tov4(bp->giaddr, rp->giaddr);
	memset(bp->ciaddr, 0, sizeof(bp->ciaddr));
	memset(bp->yiaddr, 0, sizeof(bp->yiaddr));
	memset(bp->siaddr, 0, sizeof(bp->siaddr));

	/*
	 *  set options
	 */
	byteopt(rp, ODtype, Nak);
	addropt(rp, ODserverid, up->laddr);
	if(msg)
		stringopt(rp, ODmessage, msg);
	if(strncmp(rp->id, "id", 2) == 0)
		hexopt(rp, ODclientid, rp->id+2);
	termopt(rp);

	/*
	 *  send nak
	 */
	n = rp->p - rp->buf;
	if(!mute && write(rp->fd, rp->buf, n) != n)
		warning(0, "nak: write failed: %r");
}

void
bootp(Req *rp)
{
	int n;
	Bootp *bp;
	Udphdr *up;
	ushort flags;
	Iplifc *lifc;
	Info *iip;
	int servedbyme;

	warning(0, "bootp %I->%I from %s via %I",
		rp->up->raddr, rp->up->laddr,
		rp->id, rp->gii.ipaddr);

	if(nobootp)
		return;

	bp = rp->bp;
	up = rp->up;
	iip = &rp->ii;

	if(rp->staticbinding == 0){
		warning(0, "bootp from unknown %s via %I", rp->id, rp->gii.ipaddr);
		return;
	}

	/* ignore if not for us */
	if(*bp->sname){
		if(strcmp(bp->sname, mysysname) != 0){
			bp->sname[20] = 0;
			warning(0, "bootp for server %s", bp->sname);
			return;
		}
	} else if(slow)
		sleep(500);

	/* ignore if we don't know what file to load */
	if(*bp->file == 0){
		if(iip->bootf)
			strncpy(bp->file, iip->bootf, sizeof(bp->file));
		else if(*bp->sname)		/* if we were asked, respond no matter what */
			bp->file[0] = '\0';
		else {
			warning(0, "no bootfile for %I", iip->ipaddr);
			return;
		}
	}

	/* ignore if the file is unreadable */
	if(!rp->genrequest && access(bp->file, 4) < 0){
		warning(0, "inaccessible bootfile1 %s", bp->file);
		return;
	}

	bp->op = Bootreply;
	v6tov4(bp->yiaddr, iip->ipaddr);
	if(rp->p9request){
		warning(0, "p9bootp: %I", iip->ipaddr);
		memmove(bp->optmagic, plan9opt, 4);
		if(iip->gwip == 0)
			v4tov6(iip->gwip, bp->giaddr);
		rp->p += sprint((char*)rp->p, "%V %I %I %I", iip->ipmask+IPv4off, iip->fsip,
				iip->auip, iip->gwip);
	} else if(rp->genrequest){
		warning(0, "genericbootp: %I", iip->ipaddr);
		if( *iip->bootf2 ) /* if not plan9, change the boot file to bootf2 */
			strncpy(bp->file, iip->bootf2, sizeof(bp->file));
		memmove(bp->optmagic, genericopt, 4);
		miscoptions(rp, iip->ipaddr);
		termopt(rp);
	} else if(iip->vendor[0] != 0) {
		warning(0, "bootp vendor field: %s", iip->vendor);
		memset(rp->p, 0, 128-4);
		rp->p += sprint((char*)bp->optmagic, "%s", iip->vendor);
	} else {
		memset(rp->p, 0, 128-4);
		rp->p += 128-4;
	}

	/*
	 *  set destination
	 */
	flags = nhgets(bp->flags);
	if(validip(rp->giaddr)){
		ipmove(up->raddr, rp->giaddr);
		hnputs(up->rport, 67);
	} else if(flags & Fbroadcast){
		ipmove(up->raddr, IPv4bcast);
		hnputs(up->rport, 68);
	} else {
		v4tov6(up->raddr, bp->yiaddr);
		if(bp->htype == 1)
			arpenter(up->raddr, bp->chaddr);
		hnputs(up->rport, 68);
	}

	/*
	 *  select best local address if destination is directly connected
	 */
	lifc = findlifc(up->raddr);
	if(lifc)
		ipmove(up->laddr, lifc->ip);

	/*
	 *  our identity
	 */
	servedbyme = setsiaddr("tftp", bp->siaddr, iip->ipaddr, up->laddr);
	strncpy(bp->sname, mysysname, sizeof(bp->sname));
	/* not plan9 and has alternative bootf */
	if(servedbyme && rp->genrequest && *iip->bootf2)
		servedbyme = setsiaddr("tftp2", bp->siaddr, iip->ipaddr, up->laddr);
	/* havn't we done this a few lines above? why again? */
	/* ignore if booting plan9 and the file is unreadable */
	if(servedbyme && access(bp->file, 4) < 0){
		warning(0, "inaccessible bootfile2 %s", bp->file);
		return;
	}

	/*
	 * RFC 1048 says that we must pad vendor field with
	 * zeros until we have a 64 byte field.
	 */
	n = rp->p - rp->bp->optdata;
	if(n < 64-4) {
		memset(rp->p, 0, (64-4)-n);
		rp->p += (64-4)-n;
	}

	/*
	 *  send
	 */
	n = rp->p - rp->buf;
	if(!mute && write(rp->fd, rp->buf, n) != n)
		warning(0, "bootp: write failed: %r");
	else if(rp->p9request)
		warning(0, "bootp reply to: %V %s via %I: %s", bp->yiaddr, bp->file,
			up->raddr, bp->optmagic);
	else
		warning(0, "bootp reply to: %V %s via %I", bp->yiaddr, bp->file, up->raddr);
}

void
parseoptions(Req *rp)
{
	int n, c, code;
	uchar *o, *p;

	p = rp->p;

	while(p < rp->e){
		code = *p++;
		if(code == 255)
			break;
		if(code == 0)
			continue;

		/* ignore anything that's too long */
		n = *p++;
		o = p;
		p += n;
		if(p > rp->e)
			return;
		
		switch(code){
		case ODipaddr:	/* requested ip address */
			if(n == IPv4addrlen)
				v4tov6(rp->ip, o);
			break;
		case ODlease:	/* requested lease time */
			rp->leasetime = nhgetl(o);
			if(rp->leasetime > MaxLease || rp->leasetime < 0)
				rp->leasetime = MaxLease;
			break;
		case ODtype:
			c = *o;
			if(c < 10 && c > 0)
				rp->dhcptype = c;
			break;
		case ODserverid:
			if(n == IPv4addrlen)
				v4tov6(rp->server, o);
			break;
		case ODmessage:
			if(n > sizeof rp->msg-1)
				n = sizeof rp->msg-1;
			memmove(rp->msg, o, n);
			rp->msg[n] = 0;
			break;
		case ODmaxmsg:
			c = nhgets(o);
			c -= 28;
			c += Udphdrsize;
			if(c > 0)
				rp->max = rp->buf + c;
			break;
		case ODclientid:
			if(n <= 1)
				break;
			rp->id = toid( o, n);
			break;
		case ODparams:
			if(n > sizeof(rp->requested))
				n = sizeof(rp->requested);
			memmove(rp->requested, o, n);
			break;
		case ODvendorclass:
			if(n >= sizeof(rp->vendorclass))
				n = sizeof(rp->vendorclass)-1;
			memmove(rp->vendorclass, o, n);
			rp->vendorclass[n] = 0;
			if(strncmp((char*)rp->vendorclass, "p9-", 3) == 0)
				strcpy(rp->cputype, (char*)rp->vendorclass+3);
			break;
		case OBend:
			return;
		}
	}
}

void
remrequested(Req *rp, int opt)
{
	uchar *p;

	p = memchr(rp->requested, opt, sizeof(rp->requested));
	if(p != nil)
		*p = OBpad;
}

void
miscoptions(Req *rp, uchar *ip)
{
	char *p;
	int i, j;
	uchar *addrs[2];
	uchar x[2*IPaddrlen];
	uchar vopts[64];
	uchar *op, *omax;

	addrs[0] = x;
	addrs[1] = x+IPaddrlen;

	
	/* always supply these */
	addropt(rp, OBmask, rp->gii.ipmask);
	if(validip(rp->gii.gwip)){
		remrequested(rp, OBrouter);
		addropt(rp, OBrouter, rp->gii.gwip);
	} else if(validip(rp->giaddr)){
		remrequested(rp, OBrouter);
		addropt(rp, OBrouter, rp->giaddr);
	}

	// OBhostname for the HP4000M switches
	// (this causes NT to log infinite errors - tough shit )
	if(*rp->ii.domain){
		remrequested(rp, OBhostname);
		stringopt(rp, OBhostname, rp->ii.domain);
	}
	if( *rp->ii.rootpath ) {
		stringopt(rp, OBrootpath, rp->ii.rootpath);
	}

	/* lookup anything we might be missing */
	if(*rp->ii.domain == 0)
		lookupname(ip, rp->ii.domain);

	/* add any requested ones that we know about */
	for(i = 0; i < sizeof(rp->requested); i++)
		switch(rp->requested[i]){
		case OBdnserver:
			j = lookupserver("dns", addrs, ip);
			addrsopt(rp, OBdnserver, addrs, j);
			break;
		case OBhostname:
			if(*rp->ii.domain)
				stringopt(rp, OBhostname, rp->ii.domain);
			break;
		case OBdomainname:
			p = strchr(rp->ii.domain, '.');
			if(p)
				stringopt(rp, OBdomainname, p+1);
			break;
		case OBnetbiosns:
			j = lookupserver("wins", addrs, ip);
			addrsopt(rp, OBnetbiosns, addrs, j);
			break;
		case OBnetbiostype:
			/* p-node: peer to peer WINS queries */
			byteopt(rp, OBnetbiostype, 0x2);
			break;
		case OBsmtpserver:
			j = lookupserver("smtp", addrs, ip);
			addrsopt(rp, OBsmtpserver, addrs, j);
			break;
		case OBpop3server:
			j = lookupserver("pop3", addrs, ip);
			addrsopt(rp, OBpop3server, addrs, j);
			break;
		case OBwwwserver:
			j = lookupserver("www", addrs, ip);
			addrsopt(rp, OBwwwserver, addrs, j);
			break;
		case OBntpserver:
			j = lookupserver("ntp", addrs, ip);
			addrsopt(rp, OBntpserver, addrs, j);
			break;
		case OBtimeserver:
			j = lookupserver("time", addrs, ip);
			addrsopt(rp, OBtimeserver, addrs, j);
			break;
		case OBttl:
			byteopt(rp, OBttl, 255);
			break;
		}

	// add plan9 specific options
	if(strncmp((char*)rp->vendorclass, "plan9_", 6) == 0
	|| strncmp((char*)rp->vendorclass, "p9-", 3) == 0){
		// point to temporary area
		op = rp->p;
		omax = rp->max;
		rp->p = vopts;
		rp->max = vopts + sizeof(vopts) - 1;

		j = lookupserver("fs", addrs, ip);
		addrsopt(rp, OP9fs, addrs, j);
		j = lookupserver("auth", addrs, ip);
		addrsopt(rp, OP9auth, addrs, j);

		// point back
		j = rp->p - vopts;
		rp->p = op;
		rp->max = omax;
		vectoropt(rp, OBvendorinfo, vopts, j);
	}
}

void
debugoptions(Req *rp, char *cp, char *ep)
{
	int n, code;
	uchar *p, *e;

	e = rp->p;
	p = rp->bp->optdata;

	while(p < e){
		code = *p++;
		if(code == 255)
			break;
		if(code == 0)
			continue;

		n = *p++;
		p += n;
		cp = seprint(cp, ep, "(%d %d)", code, n);
	}
}

int
openlisten(char *net)
{
	int fd, cfd;
	char data[128];
	char devdir[40];

	sprint(data, "%s/udp!*!bootp", net);
	cfd = announce(data, devdir);
	if(cfd < 0)
		fatal(1, "can't announce");
	if(fprint(cfd, "headers") < 0)
		fatal(1, "can't set header mode");

	sprint(data, "%s/data", devdir);

	fd = open(data, ORDWR);
	if(fd < 0)
		fatal(1, "open udp data");
	return fd;
}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRMAX];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(syserr)
		syslog(1, blog, "dhcp: %s: %r", buf);
	else
		syslog(1, blog, "dhcp: %s", buf);
	exits(buf);
}

extern void
warning(int syserr, char *fmt, ...)
{
	char buf[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(syserr){
		syslog(0, blog, "dhcp: %s: %r", buf);
		if(debug)
			fprint(2, "dhcp: %s: %r\n", buf);
	} else {
		syslog(0, blog, "dhcp: %s", buf);
		if(debug)
			fprint(2, "dhcp: %s\n", buf);
	}
}

char*
readsysname(void)
{
	static char name[128];
	char *p;
	int n, fd;

	fd = open("/dev/sysname", OREAD);
	if(fd >= 0){
		n = read(fd, name, sizeof(name)-1);
		close(fd);
		if(n > 0){
			name[n] = 0;
			return name;
		}
	}
	p = getenv("sysname");
	if(p == nil || *p == 0)
		return "unknown";
	return p;
}

extern int
validip(uchar *ip)
{
	if(ipcmp(ip, IPnoaddr) == 0)
		return 0;
	if(ipcmp(ip, v4prefix) == 0)
		return 0;
	return 1;
}

void
longopt(Req *rp, int t, long v)
{
	if(rp->p + 6 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 4;
	hnputl(rp->p, v);
	rp->p += 4;
}

void
addropt(Req *rp, int t, uchar *ip)
{
	if(rp->p + 6 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 4;
	memmove(rp->p, ip+IPv4off, 4);
	rp->p += 4;
}

void
addrsopt(Req *rp, int t, uchar **ip, int i)
{
	if(i <= 0)
		return;
	if(rp->p + 2 + 4*i > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 4*i;
	while(i-- > 0){
		v6tov4(rp->p, *ip);
		rp->p += 4;
		ip++;
	}
}

void
byteopt(Req *rp, int t, uchar v)
{
	if(rp->p + 3 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 1;
	*rp->p++ = v;
}

void
termopt(Req *rp)
{
	if(rp->p + 1 > rp->max)
		return;
	*rp->p++ = OBend;
}

void
stringopt(Req *rp, int t, char *str)
{
	int n;

	n = strlen(str);
	if(n > 255)
		n = 255;
	if(rp->p+n+2 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = n;
	memmove(rp->p, str, n);
	rp->p += n;
}

void
vectoropt(Req *rp, int t, uchar *v, int n)
{
	if(n > 255)
		n = 255;
	if(rp->p+n+2 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = n;
	memmove(rp->p, v, n);
	rp->p += n;
}

int
fromhex(int x)
{
	if(x >= '0' && x <= '9')
		return x - '0';
	return x - 'a';
}

void
hexopt(Req *rp, int t, char *str)
{
	int n;

	n = strlen(str);
	n /= 2;
	if(n > 255)
		n = 255;
	if(rp->p+n+2 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = n;
	while(n-- > 0){
		*rp->p++ = (fromhex(str[0])<<4)|fromhex(str[1]);
		str += 2;
	}
}

void
arpenter(uchar *ip, uchar *ether)
{
	int f;
	char buf[256];

	/* brazil */
	sprint(buf, "%s/arp", net);
	f = open(buf, OWRITE);
	if(f < 0){
		syslog(debug, blog, "open %s: %r", buf);
		return;
	}
	fprint(f, "add ether %I %E", ip, ether);
	close(f);
}

char *dhcpmsgname[] =
{
	[Discover]	"Discover",
	[Offer]		"Offer",
	[Request]	"Request",
	[Decline]	"Decline",
	[Ack]		"Ack",
	[Nak]		"Nak",
	[Release]	"Release",
	[Inform]	"Inform",
};

void
logdhcp(Req *rp)
{
	char buf[4096];
	char *p, *e;
	int i;

	p = buf;
	e = buf + sizeof(buf);
	p = seprint(p, e, "dhcp %I->%I", rp->up->raddr, rp->up->laddr);
	if(rp->dhcptype > 0 && rp->dhcptype <= Inform)
		p = seprint(p, e, " %s", dhcpmsgname[rp->dhcptype]);
	else
		p = seprint(p, e, " %d", rp->dhcptype);
	if(rp->id != nil)
		p = seprint(p, e, " from %s", rp->id);
	if(validip(rp->ciaddr))
		p = seprint(p, e, " ciaddr %I", rp->ciaddr);
	if(validip(rp->giaddr))
		p = seprint(p, e, " giaddr %I", rp->giaddr);
	if(rp->leasetime)
		p = seprint(p, e, " lease %d", rp->leasetime);
	if(validip(rp->ip))
		p = seprint(p, e, " ip %I", rp->ip);
	if(validip(rp->server))
		p = seprint(p, e, " server %I", rp->server);
	if(rp->bp->htype == 1)
		p = seprint(p, e, " ea %E", rp->bp->chaddr);
	p = seprint(p, e, " (");
	for(i = 0; i < sizeof(rp->requested); i++)
		if(rp->requested[i] != 0)
			p = seprint(p, e, "%d ", rp->requested[i]);
	p = seprint(p, e, ")");

	USED(p);
	syslog(0, blog, "%s", buf);
}
