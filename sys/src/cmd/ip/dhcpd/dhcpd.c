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
#include <bio.h>
#include <ndb.h>
#include "dat.h"

/*
 *	ala rfc2131
 */

enum {
	Maxloglen = 1024,
};

typedef struct Req Req;
struct Req
{
	int	fd;			/* for reply */
	Bootp	*bp;
	Udphdr	*up;
	uint8_t	*e;			/* end of received message */
	uint8_t	*p;			/* options pointer */
	uint8_t	*max;			/* max end of reply */

	/* expanded to v6 */
	uint8_t	ciaddr[IPaddrlen];
	uint8_t	giaddr[IPaddrlen];

	/* parsed options */
	int	p9request;		/* flag: this is a bootp with plan9 options */
	int	genrequest;		/* flag: this is a bootp with generic options */
	int	broadcast;		/* flag: request was broadcast */
	int	dhcptype;		/* dhcp message type */
	int	leasetime;		/* dhcp lease */
	uint8_t	ip[IPaddrlen];		/* requested address */
	uint8_t	server[IPaddrlen];	/* server address */
	char	msg[ERRMAX];		/* error message */
	char	vci[32];		/* vendor class id */
	char	*id;			/* client id */
	uint8_t	requested[32];		/* requested params */
	uint8_t	vendorclass[32];
	char	cputype[32-3];

	Info	gii;			/* about target network */
	Info	ii;			/* about target system */
	int	staticbinding;

	uint8_t buf[2*1024];		/* message buffer */
};

#define TFTP "/lib/tftpd"

char	*blog = "ipboot";
char	mysysname[64];
Ipifc	*ipifcs;
int	debug;
int	nobootp;
int32_t	now;
int	slowstat, slowdyn;
char	net[256];

int	pptponly;	/* only answer request that came from the pptp server */
int	mute, mutestat;
int	minlease = MinLease;
int	staticlease = StaticLease;

uint64_t	start;

static int v6opts;

/* option magic */
char plan9opt[4] = { 'p', '9', ' ', ' ' };
char genericopt[4] = { 0x63, 0x82, 0x53, 0x63 };

/* well known addresses */
uint8_t zeros[Maxhwlen];

/* option debug buffer */
char optbuf[1024];
char *op;
char *oe = optbuf + sizeof(optbuf);

char *optname[256] =
{
	[OBend]			= "end",
	[OBpad]			= "pad",
	[OBmask]		= "mask",
	[OBtimeoff]		= "timeoff",
	[OBrouter]		= "router",
	[OBtimeserver]		= "time",
	[OBnameserver]		= "name",
	[OBdnserver]		= "dns",
	[OBlogserver]		= "log",
	[OBcookieserver]	= "cookie",
	[OBlprserver]		= "lpr",
	[OBimpressserver]	= "impress",
	[OBrlserver]		= "rl",
	[OBhostname]		= "host",
	[OBbflen]		= "bflen",
	[OBdumpfile]		= "dumpfile",
	[OBdomainname]		= "dom",
	[OBswapserver]		= "swap",
	[OBrootpath]		= "rootpath",
	[OBextpath]		= "extpath",
	[OBipforward]		= "ipforward",
	[OBnonlocal]		= "nonlocal",
	[OBpolicyfilter]	= "policyfilter",
	[OBmaxdatagram]		= "maxdatagram",
	[OBttl]			= "ttl",
	[OBpathtimeout]		= "pathtimeout",
	[OBpathplateau]		= "pathplateau",
	[OBmtu]			= "mtu",
	[OBsubnetslocal]	= "subnetslocal",
	[OBbaddr]		= "baddr",
	[OBdiscovermask]	= "discovermask",
	[OBsupplymask]		= "supplymask",
	[OBdiscoverrouter]	= "discoverrouter",
	[OBrsserver]		= "rsserver",
	[OBstaticroutes]	= "staticroutes",
	[OBtrailerencap]	= "trailerencap",
	[OBarptimeout]		= "arptimeout",
	[OBetherencap]		= "etherencap",
	[OBtcpttl]		= "tcpttl",
	[OBtcpka]		= "tcpka",
	[OBtcpkag]		= "tcpkag",
	[OBnisdomain]		= "nisdomain",
	[OBniserver]		= "niserver",
	[OBntpserver]		= "ntpserver",
	[OBvendorinfo]		= "vendorinfo",
	[OBnetbiosns]		= "NBns",
	[OBnetbiosdds]		= "NBdds",
	[OBnetbiostype]		= "NBtype",
	[OBnetbiosscope]	= "NBscope",
	[OBxfontserver]		= "xfont",
	[OBxdispmanager]	= "xdisp",
	[OBnisplusdomain]	= "NPdomain",
	[OBnisplusserver]	= "NP",
	[OBhomeagent]		= "homeagent",
	[OBsmtpserver]		= "smtp",
	[OBpop3server]		= "pop3",
	[OBnntpserver]		= "nntp",
	[OBwwwserver]		= "www",
	[OBfingerserver]	= "finger",
	[OBircserver]		= "ircserver",
	[OBstserver]		= "stserver",
	[OBstdaserver]		= "stdaserver",

	/* dhcp options */
	[ODipaddr]		= "ip",
	[ODlease]		= "leas",
	[ODoverload]		= "overload",
	[ODtype]		= "typ",
	[ODserverid]		= "sid",
	[ODparams]		= "params",
	[ODmessage]		= "message",
	[ODmaxmsg]		= "maxmsg",
	[ODrenewaltime]		= "renewaltime",
	[ODrebindingtime]	= "rebindingtime",
	[ODvendorclass]		= "vendorclass",
	[ODclientid]		= "cid",
	[ODtftpserver]		= "tftpserver",
	[ODbootfile]		= "bf",
};

void	addropt(Req*, int, uint8_t*);
void	addrsopt(Req*, int, uint8_t**, int);
void	arpenter(uint8_t*, uint8_t*);
void	bootp(Req*);
void	byteopt(Req*, int, uint8_t);
void	dhcp(Req*);
void	fatal(int, char*, ...);
void	hexopt(Req*, int, char*);
void	logdhcp(Req*);
void	logdhcpout(Req *, char *);
void	longopt(Req*, int, int32_t);
void	maskopt(Req*, int, uint8_t*);
void	miscoptions(Req*, uint8_t*);
int	openlisten(char *net);
void	p9addrsopt(Req *rp, int t, uint8_t **ip, int i);
void	parseoptions(Req*);
void	proto(Req*, int);
void	rcvdecline(Req*);
void	rcvdiscover(Req*);
void	rcvinform(Req*);
void	rcvrelease(Req*);
void	rcvrequest(Req*);
int	readlast(int, uint8_t*, int);
char*	readsysname(void);
void	remrequested(Req*, int);
void	sendack(Req*, uint8_t*, int, int);
void	sendnak(Req*, char*);
void	sendoffer(Req*, uint8_t*, int);
void	stringopt(Req*, int, char*);
void	termopt(Req*);
int	validip(uint8_t*);
void	vectoropt(Req*, int, uint8_t*, int);
void	warning(int, char*, ...);

void
timestamp(char *tag)
{
	uint64_t t;

	t = nsec()/1000;
	syslog(0, blog, "%s %lluµs", tag, t - start);
}

void
usage(void)
{
	fprint(2, "usage: dhcp [-dmnprsSZ] [-f directory] [-M minlease] "
		"[-x netmtpt] [-Z staticlease] addr n [addr n] ...\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, n, fd;
	uint8_t ip[IPaddrlen];
	Req r;

	setnetmtpt(net, sizeof net, nil);

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);
	ARGBEGIN {
	case '6':
		v6opts = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'f':
		ndbfile = EARGF(usage());
		break;
	case 'm':
		mute = 1;
		break;
	case 'M':
		minlease = atoi(EARGF(usage()));
		if(minlease <= 0)
			minlease = MinLease;
		break;
	case 'n':
		nobootp = 1;
		break;
	case 'p':
		pptponly = 1;
		break;
	case 'r':
		mutestat = 1;
		break;
	case 's':
		slowstat = 1;
		break;
	case 'S':
		slowdyn = 1;
		break;
	case 'x':
		setnetmtpt(net, sizeof net, EARGF(usage()));
		break;
	case 'Z':
		staticlease = atoi(EARGF(usage()));
		if(staticlease <= 0)
			staticlease = StaticLease;
		break;
	default:
		usage();
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

	/* for debugging */
	for(i = 0; i < 256; i++)
		if(optname[i] == 0)
			optname[i] = smprint("%d", i);

	/* what is my name? */
	strcpy(mysysname, readsysname());

	/* put process in background */
	if(!debug)
	switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
	case -1:
		fatal(1, "fork");
	case 0:
		break;
	default:
		exits(0);
	}

	if (chdir(TFTP) < 0)
		warning(1, "can't change directory to %s", TFTP);
	fd = openlisten(net);

	for(;;){
		memset(&r, 0, sizeof(r));
		r.fd = fd;
		n = readlast(r.fd, r.buf, sizeof(r.buf));
		if(n < Udphdrsize)
			fatal(1, "error reading requests");
		start = nsec()/1000;
		op = optbuf;
		*op = 0;
		proto(&r, n);
		if(r.id != nil)
			free(r.id);
	}
}

void
proto(Req *rp, int n)
{
	uint8_t relip[IPaddrlen];
	char buf[64];

	now = time(0);

	rp->e = rp->buf + n;
	rp->bp = (Bootp*)rp->buf;
	rp->up = (Udphdr*)rp->buf;
	if (ipcmp(rp->up->laddr, IPv4bcast) == 0){
		ipmove(rp->up->laddr, rp->up->ifcaddr);
		rp->broadcast = 1;
	}
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
	if(rp->e < (uint8_t*)rp->bp->sname){
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
		}
	}
	rp->p = rp->bp->optdata;

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
		sprint(buf, "hwa%2.2x_", rp->bp->htype);
		rp->id = tohex(buf, rp->bp->chaddr, rp->bp->hlen);
	}

	/* info about gateway */
	if(lookupip(relip, &rp->gii, 1) < 0){
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
	timestamp("done");
}

/*
 * since we are single-threaded, this causes us to effectively
 * stop listening while we sleep.
 */
static void
slowdelay(Req *rp)
{
	if(slowstat && rp->staticbinding || slowdyn && !rp->staticbinding)
		sleep(1000);
}

void
dhcp(Req *rp)
{
	logdhcp(rp);

	switch(rp->dhcptype){
	case Discover:
		slowdelay(rp);
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
		sendoffer(rp, rp->ii.ipaddr, (staticlease > minlease? staticlease: minlease));
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
		warning(0, "!Discover(%s via %I): no binding %I",
			rp->id, rp->gii.ipaddr, rp->ip);
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
				sendack(rp, rp->ii.ipaddr,
					(staticlease > minlease? staticlease:
					minlease), 1);
			else
				warning(0, "!Request(%s via %I): for server %I not me",
					rp->id, rp->gii.ipaddr, rp->server);
			return;
		}

		b = idtooffer(rp->id, &rp->gii);

		/* if we don't have an offer, nak */
		if(b == nil){
			warning(0, "!Request(%s via %I): no offer",
				rp->id, rp->gii.ipaddr);
			if(forme(rp->server))
				sendnak(rp, "no offer for you");
			return;
		}

		/* if not for me, retract offer */
		if(!forme(rp->server)){
			b->expoffer = 0;
			warning(0, "!Request(%s via %I): for server %I not me",
				rp->id, rp->gii.ipaddr, rp->server);
			return;
		}

		/*
		 *  if the client is confused about what we offered, nak.
		 *  client really shouldn't be specifying this when selecting
		 */
		if(validip(rp->ip) && ipcmp(rp->ip, b->ip) != 0){
			warning(0, "!Request(%s via %I): requests %I, not %I",
				rp->id, rp->gii.ipaddr, rp->ip, b->ip);
			sendnak(rp, "bad ip address option");
			return;
		}
		if(commitbinding(b) < 0){
			warning(0, "!Request(%s via %I): can't commit %I",
				rp->id, rp->gii.ipaddr, b->ip);
			sendnak(rp, "can't commit binding");
			return;
		}
		sendack(rp, b->ip, b->offer, 1);
	} else if(validip(rp->ip)){
		/*
		 *  checking address/net - INIT-REBOOT
		 *
		 *  This is a rebooting client that remembers its old
		 *  address.
		 */
		/* check for hard assignment */
		if(rp->staticbinding){
			if(memcmp(rp->ip, rp->ii.ipaddr, IPaddrlen) != 0){
				warning(0, "!Request(%s via %I): %I not valid for %E",
					rp->id, rp->gii.ipaddr, rp->ip, rp->bp->chaddr);
				sendnak(rp, "not valid");
			}
			sendack(rp, rp->ii.ipaddr, (staticlease > minlease?
				staticlease: minlease), 1);
			return;
		}

		/* make sure the network makes sense */
		if(!samenet(rp->ip, &rp->gii)){
			warning(0, "!Request(%s via %I): bad forward of %I",
				rp->id, rp->gii.ipaddr, rp->ip);
			sendnak(rp, "wrong network");
			return;
		}
		b = iptobinding(rp->ip, 0);
		if(b == nil){
			warning(0, "!Request(%s via %I): no binding for %I",
				rp->id, rp->gii.ipaddr, rp->ip);
			return;
		}
		if(memcmp(rp->ip, b->ip, IPaddrlen) != 0 || now > b->lease){
			warning(0, "!Request(%s via %I): %I not valid",
				rp->id, rp->gii.ipaddr, rp->ip);
			sendnak(rp, "not valid");
			return;
		}
		b->offer = b->lease - now;
		sendack(rp, b->ip, b->offer, 1);
	} else if(validip(rp->ciaddr)){
		/*
		 *  checking address - RENEWING or REBINDING
		 *
		 *  these states are indistinguishable in our action.  The only
		 *  difference is how close to lease expiration the client is.
		 *  If it is really close, it broadcasts the request hoping that
		 *  some server will answer.
		 */

		/* check for hard assignment */
		if(rp->staticbinding){
			if(ipcmp(rp->ciaddr, rp->ii.ipaddr) != 0){
				warning(0, "!Request(%s via %I): %I not valid",
					rp->id, rp->gii.ipaddr, rp->ciaddr);
				sendnak(rp, "not valid");
			}
			sendack(rp, rp->ii.ipaddr, (staticlease > minlease?
				staticlease: minlease), 1);
			return;
		}

		/* make sure the network makes sense */
		if(!samenet(rp->ciaddr, &rp->gii)){
			warning(0, "!Request(%s via %I): bad forward of %I",
				rp->id, rp->gii.ipaddr, rp->ip);
			sendnak(rp, "wrong network");
			return;
		}
		b = iptobinding(rp->ciaddr, 0);
		if(b == nil){
			warning(0, "!Request(%s via %I): no binding for %I",
				rp->id, rp->gii.ipaddr, rp->ciaddr);
			return;
		}
		if(ipcmp(rp->ciaddr, b->ip) != 0){
			warning(0, "!Request(%I via %s): %I not valid",
				rp->id, rp->gii.ipaddr, rp->ciaddr);
			sendnak(rp, "invalid ip address");
			return;
		}
		mkoffer(b, rp->id, rp->leasetime);
		if(commitbinding(b) < 0){
			warning(0, "!Request(%s via %I): can't commit %I",
				rp->id, rp->gii.ipaddr, b->ip);
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
		warning(0, "!Decline(%s via %I): no binding",
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
		warning(0, "!Release(%s via %I): no binding",
			rp->id, rp->gii.ipaddr);
		return;
	}
	if(strcmp(rp->id, b->boundto) != 0){
		warning(0, "!Release(%s via %I): invalid release of %I",
			rp->id, rp->gii.ipaddr, rp->ip);
		return;
	}
	warning(0, "Release(%s via %I): releasing %I", b->boundto, rp->gii.ipaddr, b->ip);
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
		warning(0, "!Inform(%s via %I): no binding for %I",
			rp->id, rp->gii.ipaddr, rp->ip);
		return;
	}
	sendack(rp, b->ip, 0, 0);
}

int
setsiaddr(uint8_t *siaddr, uint8_t *saddr, uint8_t *laddr)
{
	if(ipcmp(saddr, IPnoaddr) != 0){
		v6tov4(siaddr, saddr);
		return 0;
	} else {
		v6tov4(siaddr, laddr);
		return 1;
	}
}

int
ismuted(Req *rp)
{
	return mute || (mutestat && rp->staticbinding);
}

void
sendoffer(Req *rp, uint8_t *ip, int offer)
{
	int n;
	uint16_t flags;
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
	setsiaddr(bp->siaddr, rp->ii.tftp, up->laddr);
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

	logdhcpout(rp, "Offer");

	/*
	 *  send
	 */
	n = rp->p - rp->buf;
	if(!ismuted(rp) && write(rp->fd, rp->buf, n) != n)
		warning(0, "offer: write failed: %r");
}

void
sendack(Req *rp, uint8_t *ip, int offer, int sendlease)
{
	int n;
	uint16_t flags;
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
	v6tov4(bp->giaddr, rp->giaddr);
	v6tov4(bp->yiaddr, ip);
	setsiaddr(bp->siaddr, rp->ii.tftp, up->laddr);
	strncpy(bp->sname, mysysname, sizeof(bp->sname));
	strncpy(bp->file, rp->ii.bootf, sizeof(bp->file));

	/*
	 *  set options
	 */
	byteopt(rp, ODtype, Ack);
	if(sendlease){
		longopt(rp, ODlease, offer);
	}
	addropt(rp, ODserverid, up->laddr);
	miscoptions(rp, ip);
	termopt(rp);

	logdhcpout(rp, "Ack");

	/*
	 *  send
	 */
	n = rp->p - rp->buf;
	if(!ismuted(rp) && write(rp->fd, rp->buf, n) != n)
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

	logdhcpout(rp, "Nak");

	/*
	 *  send nak
	 */
	n = rp->p - rp->buf;
	if(!ismuted(rp) && write(rp->fd, rp->buf, n) != n)
		warning(0, "nak: write failed: %r");
}

void
bootp(Req *rp)
{
	int n;
	Bootp *bp;
	Udphdr *up;
	uint16_t flags;
	Iplifc *lifc;
	Info *iip;

	warning(0, "bootp %s %I->%I from %s via %I, file %s %s",
		rp->genrequest? "generic": (rp->p9request? "p9": ""),
		rp->up->raddr, rp->up->laddr,
		rp->id, rp->gii.ipaddr,
		rp->bp->file, rp->broadcast? "broadcast": "unicast");

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
	} else
		slowdelay(rp);

	/* ignore if we don't know what file to load */
	if(*bp->file == 0){
		if(rp->genrequest && *iip->bootf2) /* if not plan 9 & have alternate file... */
			strncpy(bp->file, iip->bootf2, sizeof(bp->file));
		else if(*iip->bootf)
			strncpy(bp->file, iip->bootf, sizeof(bp->file));
		else if(*bp->sname) /* if we were asked, respond no matter what */
			bp->file[0] = '\0';
		else {
			warning(0, "no bootfile for %I", iip->ipaddr);
			return;
		}
	}

	/* ignore if the file is unreadable */
	if((!rp->genrequest) && bp->file[0] && access(bp->file, 4) < 0){
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
		rp->p += sprint((char*)rp->p, "%V %I %I %I",
				iip->ipmask+IPv4off, iip->fsip,
				iip->auip, iip->gwip);
		sprint(optbuf, "%s", (char*)(bp->optmagic));
	} else if(rp->genrequest){
		warning(0, "genericbootp: %I", iip->ipaddr);
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
	strncpy(bp->sname, mysysname, sizeof(bp->sname));

	/*
	 *  set tftp server
	 */
	setsiaddr(bp->siaddr, iip->tftp, up->laddr);
	if(rp->genrequest && *iip->bootf2)
		setsiaddr(bp->siaddr, iip->tftp2, up->laddr);

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
	if(!ismuted(rp) && write(rp->fd, rp->buf, n) != n)
		warning(0, "bootp: write failed: %r");

	warning(0, "bootp via %I: file %s xid(%x)flag(%x)ci(%V)gi(%V)yi(%V)si(%V) %s",
			up->raddr, bp->file, nhgetl(bp->xid), nhgets(bp->flags),
			bp->ciaddr, bp->giaddr, bp->yiaddr, bp->siaddr,
			optbuf);
}

void
parseoptions(Req *rp)
{
	int n, c, code;
	uint8_t *o, *p;

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
				strcpy(rp->cputype,
				       (char*)rp->vendorclass+3);
			break;
		case OBend:
			return;
		}
	}
}

void
remrequested(Req *rp, int opt)
{
	uint8_t *p;

	p = memchr(rp->requested, opt, sizeof(rp->requested));
	if(p != nil)
		*p = OBpad;
}

void
miscoptions(Req *rp, uint8_t *ip)
{
	int i, j, na;
	uint8_t x[2*IPaddrlen], vopts[Maxoptlen];
	uint8_t *op, *omax;
	uint8_t *addrs[2];
	char *p;
	char *attr[100], **a;
	Ndbtuple *t;

	addrs[0] = x;
	addrs[1] = x+IPaddrlen;

	/* always supply these */
	maskopt(rp, OBmask, rp->gii.ipmask);
	if(validip(rp->gii.gwip)){
		remrequested(rp, OBrouter);
		addropt(rp, OBrouter, rp->gii.gwip);
	} else if(validip(rp->giaddr)){
		remrequested(rp, OBrouter);
		addropt(rp, OBrouter, rp->giaddr);
	}

	/*
	 * OBhostname for the HP4000M switches
	 * (this causes NT to log infinite errors - tough shit)
	 */
	if(*rp->ii.domain){
		remrequested(rp, OBhostname);
		stringopt(rp, OBhostname, rp->ii.domain);
	}
	if(*rp->ii.rootpath)
		stringopt(rp, OBrootpath, rp->ii.rootpath);

	/* figure out what we need to lookup */
	na = 0;
	a = attr;
	if(*rp->ii.domain == 0)
		a[na++] = "dom";
	for(i = 0; i < sizeof(rp->requested); i++)
		switch(rp->requested[i]){
		case OBrouter:
			a[na++] = "@ipgw";
			break;
		case OBdnserver:
			a[na++] = "@dns";
			break;
		case OBnetbiosns:
			a[na++] = "@wins";
			break;
		case OBsmtpserver:
			a[na++] = "@smtp";
			break;
		case OBpop3server:
			a[na++] = "@pop3";
			break;
		case OBwwwserver:
			a[na++] = "@www";
			break;
		case OBntpserver:
			a[na++] = "@ntp";
			break;
		case OBtimeserver:
			a[na++] = "@time";
			break;
		}
	if(strncmp((char*)rp->vendorclass, "plan9_", 6) == 0
	|| strncmp((char*)rp->vendorclass, "p9-", 3) == 0){
		a[na++] = "@fs";
		a[na++] = "@auth";
	}
	t = lookupinfo(ip, a, na);

	/* lookup anything we might be missing */
	if(*rp->ii.domain == 0)
		lookupname(rp->ii.domain, t);

	/* add any requested ones that we know about */
	for(i = 0; i < sizeof(rp->requested); i++)
		switch(rp->requested[i]){
		case OBrouter:
			j = lookupserver("ipgw", addrs, t);
			addrsopt(rp, OBrouter, addrs, j);
			break;
		case OBdnserver:
			j = lookupserver("dns", addrs, t);
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
			j = lookupserver("wins", addrs, t);
			addrsopt(rp, OBnetbiosns, addrs, j);
			break;
		case OBnetbiostype:
			/* p-node: peer to peer WINS queries */
			byteopt(rp, OBnetbiostype, 0x2);
			break;
		case OBsmtpserver:
			j = lookupserver("smtp", addrs, t);
			addrsopt(rp, OBsmtpserver, addrs, j);
			break;
		case OBpop3server:
			j = lookupserver("pop3", addrs, t);
			addrsopt(rp, OBpop3server, addrs, j);
			break;
		case OBwwwserver:
			j = lookupserver("www", addrs, t);
			addrsopt(rp, OBwwwserver, addrs, j);
			break;
		case OBntpserver:
			j = lookupserver("ntp", addrs, t);
			addrsopt(rp, OBntpserver, addrs, j);
			break;
		case OBtimeserver:
			j = lookupserver("time", addrs, t);
			addrsopt(rp, OBtimeserver, addrs, j);
			break;
		case OBttl:
			byteopt(rp, OBttl, 255);
			break;
		}

	/* add plan9 specific options */
	if(strncmp((char*)rp->vendorclass, "plan9_", 6) == 0
	|| strncmp((char*)rp->vendorclass, "p9-", 3) == 0){
		/* point to temporary area */
		op = rp->p;
		omax = rp->max;
		/* stash encoded options in vopts */
		rp->p = vopts;
		rp->max = vopts + sizeof(vopts) - 1;

		/* emit old v4 addresses first to make sure that they fit */
		addrsopt(rp, OP9fsv4, addrs, lookupserver("fs", addrs, t));
		addrsopt(rp, OP9authv4, addrs, lookupserver("auth", addrs, t));

		p9addrsopt(rp, OP9fs, addrs, lookupserver("fs", addrs, t));
		p9addrsopt(rp, OP9auth, addrs, lookupserver("auth", addrs, t));
		p9addrsopt(rp, OP9ipaddr, addrs, lookupserver("ip", addrs, t));
		p9addrsopt(rp, OP9ipmask, addrs, lookupserver("ipmask", addrs, t));
		p9addrsopt(rp, OP9ipgw, addrs, lookupserver("ipgw", addrs, t));

		/* point back to packet, encapsulate vopts into packet */
		j = rp->p - vopts;
		rp->p = op;
		rp->max = omax;
		vectoropt(rp, OBvendorinfo, vopts, j);
	}

	ndbfree(t);
}

int
openlisten(char *net)
{
	int fd, cfd;
	char data[128], devdir[40];

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
	char buf[Maxloglen];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(syserr)
		syslog(1, blog, "%s: %r", buf);
	else
		syslog(1, blog, "%s", buf);
	exits(buf);
}

void
warning(int syserr, char *fmt, ...)
{
	char buf[Maxloglen];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(syserr){
		syslog(0, blog, "%s: %r", buf);
		if(debug)
			fprint(2, "%s: %r\n", buf);
	} else {
		syslog(0, blog, "%s", buf);
		if(debug)
			fprint(2, "%s\n", buf);
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
validip(uint8_t *ip)
{
	if(ipcmp(ip, IPnoaddr) == 0)
		return 0;
	if(ipcmp(ip, v4prefix) == 0)
		return 0;
	return 1;
}

void
longopt(Req *rp, int t, int32_t v)
{
	if(rp->p + 6 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 4;
	hnputl(rp->p, v);
	rp->p += 4;

	op = seprint(op, oe, "%s(%ld)", optname[t], v);
}

void
addropt(Req *rp, int t, uint8_t *ip)
{
	if(rp->p + 6 > rp->max)
		return;
	if (!isv4(ip)) {
		if (debug)
			warning(0, "not a v4 %s server: %I", optname[t], ip);
		return;
	}
	*rp->p++ = t;
	*rp->p++ = 4;
	memmove(rp->p, ip+IPv4off, 4);
	rp->p += 4;

	op = seprint(op, oe, "%s(%I)", optname[t], ip);
}

void
maskopt(Req *rp, int t, uint8_t *ip)
{
	if(rp->p + 6 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 4;
	memmove(rp->p, ip+IPv4off, 4);
	rp->p += 4;

	op = seprint(op, oe, "%s(%M)", optname[t], ip);
}

void
addrsopt(Req *rp, int t, uint8_t **ip, int i)
{
	int v4s, n;

	if(i <= 0)
		return;
	if(rp->p + 2 + 4*i > rp->max)
		return;
	v4s = 0;
	for(n = i; n-- > 0; )
		if (isv4(ip[n]))
			v4s++;
	if (v4s <= 0) {
		if (debug)
			warning(0, "no v4 %s servers", optname[t]);
		return;
	}
	*rp->p++ = t;
	*rp->p++ = 4*v4s;
	op = seprint(op, oe, " %s(", optname[t]);
	while(i-- > 0){
		if (!isv4(*ip)) {
			op = seprint(op, oe, " skipping %I ", *ip);
			continue;
		}
		v6tov4(rp->p, *ip);
		rp->p += 4;
		op = seprint(op, oe, "%I", *ip);
		ip++;
		if(i > 0)
			op = seprint(op, oe, " ");
	}
	op = seprint(op, oe, ")");
}

void
p9addrsopt(Req *rp, int t, uint8_t **ip, int i)
{
	char *pkt, *payload;

	if(i <= 0 || !v6opts)
		return;
	pkt = (char *)rp->p;
	*pkt++ = t;			/* option */
	pkt++;				/* fill in payload length below */
	payload = pkt;
	*pkt++ = i;			/* plan 9 address count */
	op = seprint(op, oe, " %s(", optname[t]);
	while(i-- > 0){
		pkt = seprint(pkt, (char *)rp->max, "%I", *ip);
		if ((uint8_t *)pkt+1 >= rp->max) {
			op = seprint(op, oe, "<out of mem1>)");
			return;
		}
		pkt++;			/* leave NUL as terminator */
		op = seprint(op, oe, "%I", *ip);
		ip++;
		if(i > 0)
			op = seprint(op, oe, " ");
	}
	if ((uint8_t *)pkt - rp->p > 0377) {
		op = seprint(op, oe, "<out of mem2>)");
		return;
	}
	op = seprint(op, oe, ")");
	rp->p[1] = pkt - payload;	/* payload length */
	rp->p = (uint8_t *)pkt;
}

void
byteopt(Req *rp, int t, uint8_t v)
{
	if(rp->p + 3 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = 1;
	*rp->p++ = v;

	op = seprint(op, oe, "%s(%d)", optname[t], v);
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

	op = seprint(op, oe, "%s(%s)", optname[t], str);
}

void
vectoropt(Req *rp, int t, uint8_t *v, int n)
{
	int i;

	if(n > 255) {
		n = 255;
		op = seprint(op, oe, "vectoropt len %d > 255 ", n);
	}
	if(rp->p+n+2 > rp->max)
		return;
	*rp->p++ = t;
	*rp->p++ = n;
	memmove(rp->p, v, n);
	rp->p += n;

	op = seprint(op, oe, "%s(", optname[t]);
	if(n > 0)
		op = seprint(op, oe, "%u", v[0]);
	for(i = 1; i < n; i++)
		op = seprint(op, oe, " %u", v[i]);
	op = seprint(op, oe, ")");
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

	op = seprint(op, oe, "%s(%s)", optname[t], str);
}

void
arpenter(uint8_t *ip, uint8_t *ether)
{
	int f;
	char buf[256];

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
	[Discover]	= "Discover",
	[Offer]		= "Offer",
	[Request]	= "Request",
	[Decline]	= "Decline",
	[Ack]		= "Ack",
	[Nak]		= "Nak",
	[Release]	= "Release",
	[Inform]	= "Inform",
};

void
logdhcp(Req *rp)
{
	char buf[4096];
	char *p, *e;
	int i;

	p = buf;
	e = buf + sizeof(buf);
	if(rp->dhcptype > 0 && rp->dhcptype <= Inform)
		p = seprint(p, e, "%s(", dhcpmsgname[rp->dhcptype]);
	else
		p = seprint(p, e, "%d(", rp->dhcptype);
	p = seprint(p, e, "%I->%I) xid(%x)flag(%x)", rp->up->raddr, rp->up->laddr,
		nhgetl(rp->bp->xid), nhgets(rp->bp->flags));
	if(rp->bp->htype == 1)
		p = seprint(p, e, "ea(%E)", rp->bp->chaddr);
	if(validip(rp->ciaddr))
		p = seprint(p, e, "ci(%I)", rp->ciaddr);
	if(validip(rp->giaddr))
		p = seprint(p, e, "gi(%I)", rp->giaddr);
	if(validip(rp->ip))
		p = seprint(p, e, "ip(%I)", rp->ip);
	if(rp->id != nil)
		p = seprint(p, e, "id(%s)", rp->id);
	if(rp->leasetime)
		p = seprint(p, e, "leas(%d)", rp->leasetime);
	if(validip(rp->server))
		p = seprint(p, e, "sid(%I)", rp->server);
	p = seprint(p, e, "need(");
	for(i = 0; i < sizeof(rp->requested); i++)
		if(rp->requested[i] != 0)
			p = seprint(p, e, "%s ", optname[rp->requested[i]]);
	p = seprint(p, e, ")");
	p = seprint(p, e, " %s", rp->broadcast? "broadcast": "unicast");
	USED(p);
	syslog(0, blog, "%s", buf);
}

void
logdhcpout(Req *rp, char *type)
{
	syslog(0, blog, "%s(%I-%I)id(%s)ci(%V)gi(%V)yi(%V)si(%V) %s",
		type, rp->up->laddr, rp->up->raddr, rp->id,
		rp->bp->ciaddr, rp->bp->giaddr, rp->bp->yiaddr, rp->bp->siaddr, optbuf);
}

/*
 *  if we get behind, it's useless to try answering since the sender
 *  will probably have retransmitted with a differnt sequence number.
 *  So dump all but the last message in the queue.
 */
void
ding(void *v, char *msg)
{
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

int
readlast(int fd, uint8_t *buf, int len)
{
	int lastn, n;

	notify(ding);

	lastn = 0;
	for(;;){
		alarm(20);
		n = read(fd, buf, len);
		alarm(0);
		if(n < 0){
			if(lastn > 0)
				return lastn;
			break;
		}
		lastn = n;
	}
	return read(fd, buf, len);
}
