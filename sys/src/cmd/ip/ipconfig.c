#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dhcp.h"

int	noconfig;
int	debug;
int	dodhcp;
int	nip;
int	myifc;
int	plan9 = 1;
Ipifc	*ifc;

// possible verbs
enum
{
	Vadd,
	Vremove,
	Vunbind
};

struct {
	// locally generated
	char	*type;
	char	*dev;
	char	mpoint[32];
	int	cfd;			// ifc control channel
	int	dfd;			// ifc data channel (for ppp)
	char	*cputype;
	uchar	hwa[32];		// hardware address
	int	hwatype;
	int	hwalen;
	uchar	cid[32];
	int	cidlen;
	char	*baud;

	// learned info
	uchar	gaddr[IPaddrlen];
	uchar	laddr[IPaddrlen];
	uchar	mask[IPaddrlen];
	uchar	raddr[IPaddrlen];
	uchar	dns[2*IPaddrlen];
	uchar	fs[2*IPaddrlen];
	uchar	auth[2*IPaddrlen];
	uchar	ntp[IPaddrlen];
	int	mtu;

	// dhcp specific
	int	state;
	int	fd;
	ulong	xid;
	ulong	starttime;
	char	sname[64];
	char	hostname[32];
	char	domainname[64];
	uchar	server[IPaddrlen];	/* server IP address */
	ulong	offered;		/* offered lease time */
	ulong	lease;			/* lease time */
	ulong	resend;			/* number of resends for current state */
	ulong	timeout;		/* time to timeout - seconds */
} conf;

void	adddefroute(char*, uchar*);
void	binddevice(void);
void	controldevice(void);
void	bootprequest(void);
void	dhcprecv(void);
void	dhcpsend(int);
int	dhcptimer(void);
void	dhcpquery(int, int);
void	dhcpwatch(int);
int	getndb(void);
int	ipconfig(void);
void	lookforip(char*);
uchar	*optadd(uchar*, int, void*, int);
uchar	*optaddbyte(uchar*, int, int);
uchar	*optaddulong(uchar*, int, ulong);
uchar	*optaddaddr(uchar*, int, uchar*);
uchar	*optaddstr(uchar*, int, char*);
uchar	*optaddvec(uchar*, int, uchar*, int);
uchar	*optget(uchar*, int, int*);
int	optgetbyte(uchar*, int);
ulong	optgetulong(uchar*, int);
int	optgetaddr(uchar*, int, uchar*);
int	optgetaddrs(uchar*, int, uchar*, int);
int	optgetstr(uchar*, int, char*, int);
int	optgetvec(uchar*, int, uchar*, int);
void	mkclientid(void);
int	nipifcs(char*);
int	openlisten(void);
int	parseoptions(uchar *p, int n);
Bootp	*parsebootp(uchar*, int);
void	putndb(void);
void	tweakservers(void);
void	writendb(char*, int, int);
void	usage(void);
int	validip(uchar*);
int	parseverb(char*);
void	doadd(int);
void	doremove(void);
void	dounbind(void);

char optmagic[4] = { 0x63, 0x82, 0x53, 0x63 };

#define DEBUG if(debug)print

typedef struct Ctl Ctl;
struct Ctl
{
	Ctl	*next;
	char	*ctl;
};
Ctl *firstctl, **ctll;

void
usage(void)
{
	fprint(2, "usage: %s [-ndDrG] [-x netmtpt] [-m mtu] [-b baud] [-g gateway] [-h hostname] [-c control-string]* type device [verb] [localaddr [mask [remoteaddr [fsaddr [authaddr]]]]]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *p;
	int retry, verb;
	Ctl *cp;

	srand(truerand());
	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);
	fmtinstall('V', eipfmt);

	setnetmtpt(conf.mpoint, sizeof(conf.mpoint), nil);
	conf.cputype = getenv("cputype");
	if(conf.cputype == nil)
		conf.cputype = "386";

	retry = 0;
	ctll = &firstctl;

	ARGBEGIN {
	case 'c':
		p = ARGF();
		if(p == nil)
			usage();
		cp = malloc(sizeof(*cp));
		if(cp == nil)
			sysfatal("%r");
		*ctll = cp;
		ctll = &cp->next;
		cp->next = nil;
		cp->ctl = p;
		break;
	case 'D':
		debug = 1;
		break;
	case 'd':
		dodhcp = 1;
		break;
	case 'h':
		p = ARGF();
		if(p == nil)
			usage();
		snprint(conf.hostname, sizeof(conf.hostname), "%s", p);
		break;
	case 'n':
		noconfig = 1;
		break;
	case 'm':
		p = ARGF();
		if(p == nil)
			usage();
		conf.mtu = atoi(p);
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(conf.mpoint, sizeof(conf.mpoint), p);
		break;
	case 'g':
		p = ARGF();
		if(p == nil)
			usage();
		parseip(conf.gaddr, p);
		break;
	case 'b':
		p = ARGF();
		if(p == nil)
			usage();
		conf.baud = p;
		break;
	case 'r':
		retry = 1;
		break;
	case 'G':
		plan9 = 0;
		break;
	} ARGEND;

	// get medium, default is ether
	switch(argc){
	default:
		conf.dev = argv[1];
		conf.type = argv[0];
		argc -= 2;
		argv += 2;
		break;
	case 1:
		conf.type = argv[0];
		if(strcmp(conf.type, "ether") == 0)
			conf.dev = "/net/ether0";
		else if(strcmp(conf.type, "ppp") == 0)
			conf.dev = "/dev/eia0";
		else
			usage();
		argc -= 1;
		argv += 1;
		break;
	case 0:
		conf.type = "ether";
		conf.dev = "/net/ether0";
		break;
	}

	// get verb, default is "add"
	verb = Vadd;
	if(argc > 0){
		verb = parseverb(argv[0]);
		if(verb >= 0){
			argc--;
			argv++;
		} else
			verb = Vadd;
	}

	// get addresses
	switch(argc){
	case 5:
		parseip(conf.auth, argv[4]);
		/* fall through */
	case 4:
		parseip(conf.fs, argv[3]);
		/* fall through */
	case 3:
		parseip(conf.raddr, argv[2]);
		/* fall through */
	case 2:
		parseipmask(conf.mask, argv[1]);
		/* fall through */
	case 1:
		parseip(conf.laddr, argv[0]);
		break;
	}

	switch(verb){
	case Vadd:
		doadd(retry);
		break;
	case Vremove:
		doremove();
		break;
	case Vunbind:
		dounbind();
		break;
	}

	exits(0);
}

void
doadd(int retry)
{
	int tries;

	// get number of preexisting interfaces
	nip = nipifcs(conf.mpoint);

	// get ipifc into name space and condition device for ip
	if(!noconfig){
		lookforip(conf.mpoint);
		controldevice();
		binddevice();
	}

	if(!validip(conf.laddr) && strcmp(conf.type, "ppp") != 0)
		dodhcp = 1;

	// run dhcp if we need something
	if(dodhcp){
		mkclientid();
		for(tries = 0; tries < 6; tries++){
			dhcpquery(!noconfig, Sselecting);
			if(conf.state == Sbound)
				break;
			sleep(1000);
		}
	}

	if(!validip(conf.laddr)){
		if(retry && dodhcp && !noconfig){
			fprint(2, "%s: couldn't determine ip address, retrying\n", argv0);
			dhcpwatch(1);
			return;
		} else {
			fprint(2, "%s: no success with DHCP\n", argv0);
			exits("failed");
		}
	}

	if(!noconfig){
		if(ipconfig() < 0)
			sysfatal("can't start ip");
		if(dodhcp && conf.lease != Lforever)
			dhcpwatch(0);
	}

	// leave everything we've learned somewhere other procs can find it
	if(nip == 0){
		putndb();
		tweakservers();
	}
}

void
doremove(void)
{
	char file[128];
	int cfd;
	Ipifc *nifc;
	Iplifc *lifc;

	if(!validip(conf.laddr)){
		fprint(2, "%s: remove requires an address\n", argv0);
		exits("usage");
	}

	ifc = readipifc(conf.mpoint, ifc, -1);
	for(nifc = ifc; nifc != nil; nifc = nifc->next){
		if(strcmp(nifc->dev, conf.dev) != 0)
			continue;
		for(lifc = nifc->lifc; lifc != nil; lifc = lifc->next){
			if(ipcmp(conf.laddr, lifc->ip) != 0)
				continue;
			if(validip(conf.mask) && ipcmp(conf.mask, lifc->mask) != 0)
				continue;
			if(validip(conf.raddr) && ipcmp(conf.raddr, lifc->net) != 0)
				continue;

			snprint(file, sizeof(file), "%s/ipifc/%d/ctl", conf.mpoint, nifc->index);
			cfd = open(file, ORDWR);
			if(cfd < 0){
				fprint(2, "%s: can't open %s: %r\n", argv0, conf.mpoint);
				continue;
			}
			if(fprint(cfd, "remove %I %I", lifc->ip, lifc->mask) < 0)
				fprint(2, "%s: can't remove %I %I from %s: %r\n", argv0,
					lifc->ip, lifc->mask, file);
		}
	}
}

void
dounbind(void)
{
	Ipifc *nifc;
	char file[128];
	int cfd;

	ifc = readipifc(conf.mpoint, ifc, -1);
	for(nifc = ifc; nifc != nil; nifc = nifc->next){
		if(strcmp(nifc->dev, conf.dev) == 0){
			snprint(file, sizeof(file), "%s/ipifc/%d/ctl", conf.mpoint, nifc->index);
			cfd = open(file, ORDWR);
			if(cfd < 0){
				fprint(2, "%s: can't open %s: %r\n", argv0, conf.mpoint);
				break;
			}
			if(fprint(cfd, "unbind") < 0)
				fprint(2, "%s: can't unbind from %s: %r\n", argv0, file);
			break;
		}
	}
}

// set the default route
void
adddefroute(char *mpoint, uchar *gaddr)
{
	char buf[256];
	int cfd;

	sprint(buf, "%s/iproute", mpoint);
	cfd = open(buf, ORDWR);
	if(cfd < 0)
		return;
	fprint(cfd, "add 0 0 %I", gaddr);
	close(cfd);
}

// create a client id
void
mkclientid(void)
{
	if(strcmp(conf.type, "ether") == 0)
	if(myetheraddr(conf.hwa, conf.dev) == 0){
		conf.hwalen = 6;
		conf.hwatype = 1;
		conf.cid[0] = conf.hwatype;
		memmove(&conf.cid[1], conf.hwa, conf.hwalen);
		conf.cidlen = conf.hwalen+1;
	} else {
		conf.hwatype = -1;
		snprint((char*)conf.cid, sizeof(conf.cid), "plan9_%ld.%d", lrand(), getpid());
		conf.cidlen = strlen((char*)conf.cid);
	}
}

// bind ip into the namespace
void
lookforip(char *net)
{
	char proto[64];

	snprint(proto, sizeof(proto), "%s/ipifc", net);
	if(access(proto, 0) == 0)
		return;
	sysfatal("no ip stack bound onto %s", net);
}

// send some ctls to a device
void
controldevice(void)
{
	char ctlfile[256];
	int fd;
	Ctl *cp;

	if(firstctl == nil)
		return;

	if(strcmp(conf.type, "ether") == 0)
		snprint(ctlfile, sizeof(ctlfile), "%s/clone", conf.dev);
	else
		return;

	fd = open(ctlfile, ORDWR);
	if(fd < 0)
		sysfatal("can't open %s", ctlfile);

	for(cp = firstctl; cp != nil; cp = cp->next){
		if(write(fd, cp->ctl, strlen(cp->ctl)) < 0)
			sysfatal("ctl message %s: %r", cp->ctl);
		seek(fd, 0, 0);
	}
}

// bind an ip stack to a device, leave the control channel open
void
binddevice(void)
{
	char buf[256];
	int ac, pid;
	char *av[12];
	Waitmsg *w;

	if(strcmp(conf.type, "ppp") == 0){
		// ppp does the binding

		// start with an empty config file
		if(nip == 0)
			writendb("", 0, 0);

		switch(pid = rfork(RFPROC|RFFDG|RFMEM)){
		case -1:
			sysfatal("can't start ppp: %r");
			break;
		case 0:
			ac = 0;
			av[ac++] = "ppp";
			av[ac++] = "-uf";
			av[ac++] = "-p";
			av[ac++] = conf.dev;
			av[ac++] = "-x";
			av[ac++] = conf.mpoint;
			if(conf.baud != nil){
				av[ac++] = "-b";
				av[ac++] = conf.baud;
			}
			av[ac] = 0;
			exec("/bin/ip/ppp", av);
			exec("/ppp", av);
			sysfatal("execing /ppp: %r");
		default:
			break;
		}

		// wait for ppp to finish connecting and configuring
		w = nil;
		for(;;){
			free(w);
			w = wait();
			if(w == nil)
				sysfatal("/ppp disappeated");
			if(w->pid == pid){
				if(w->msg[0] != 0)
					sysfatal("/ppp exited with status: %s", w->msg);
				free(w);
				break;
			}
		}

		// ppp sets up the configuration itself
		noconfig = 1;
		getndb();
	} else if(myifc == 0){
		// get a new ip interface
		snprint(buf, sizeof(buf), "%s/ipifc/clone", conf.mpoint);
		conf.cfd = open(buf, ORDWR);
		if(conf.cfd < 0)
			sysfatal("opening %s/ipifc/clone: %r", conf.mpoint);

		// specify the medium as an ethernet, and bind the interface to it
		if(fprint(conf.cfd, "bind %s %s", conf.type, conf.dev) < 0)
			sysfatal("binding device: %r");
	} else {
		// open the old interface
		snprint(buf, sizeof(buf), "%s/ipifc/%d/ctl", conf.mpoint, myifc);
		conf.cfd = open(buf, ORDWR);
		if(conf.cfd < 0)
			sysfatal("opening %s/ipifc/%d/ctl: %r", conf.mpoint, myifc);
	}

}

// add a logical interface to the ip stack
int
ipconfig(void)
{
	char buf[256];
	int n;

	if(!validip(conf.laddr))
		return -1;

	n = sprint(buf, "add");
	n += snprint(buf+n, sizeof(buf)-n, " %I", conf.laddr);

	if(!validip(conf.mask))
		ipmove(conf.mask, defmask(conf.laddr));
	n += snprint(buf+n, sizeof(buf)-n, " %I", conf.mask);

	if(validip(conf.raddr)){
		n += snprint(buf+n, sizeof(buf)-n, " %I", conf.raddr);
		if(conf.mtu != 0)
			n += snprint(buf+n, sizeof(buf)-n, " %d", conf.mtu);
	}

	if(write(conf.cfd, buf, n) < 0){
		fprint(2, "ipconfig: write(%s): %r\n", buf);
		return -1;
	}

	if(nip == 0 && validip(conf.gaddr))
		adddefroute(conf.mpoint, conf.gaddr);

	return 0;
}

// remove a logical interface to the ip stack
void
ipunconfig(void)
{
	char buf[256];
	int n;

	if(!validip(conf.laddr))
		return;
	DEBUG("couldn't renew IP lease, releasing %I\n", conf.laddr);
	n = sprint(buf, "remove");
	n += snprint(buf+n, sizeof(buf)-n, " %I", conf.laddr);

	if(!validip(conf.mask))
		ipmove(conf.mask, defmask(conf.laddr));
	n += snprint(buf+n, sizeof(buf)-n, " %I", conf.mask);

	write(conf.cfd, buf, n);

	ipmove(conf.laddr, IPnoaddr);
	ipmove(conf.raddr, IPnoaddr);
	ipmove(conf.mask, IPnoaddr);

	// forget configuration info
	if(nip == 0)
		writendb("", 0, 0);
}

void
ding(void*, char *msg)
{
	if(strstr(msg, "alarm"))
		noted(NCONT);
	noted(NDFLT);
}

void
dhcpquery(int needconfig, int startstate)
{
	if(needconfig)
		fprint(conf.cfd, "add %I %I", IPnoaddr, IPnoaddr);

	conf.fd = openlisten();
	if(conf.fd < 0){
		conf.state = Sinit;
		return;
	}
	notify(ding);

	// try dhcp for 10 seconds
	conf.xid = lrand();
	conf.starttime = time(0);
	conf.state = startstate;
	switch(startstate){
	case Sselecting:
		conf.offered = 0;
		dhcpsend(Discover);
		break;
	case Srenewing:
		dhcpsend(Request);
		break;
	default:
		sysfatal("internal error 0");
	}
	conf.resend = 0;
	conf.timeout = time(0) + 4;

	while(conf.state != Sbound){
		dhcprecv();
		if(dhcptimer() < 0)
			break;
		if(time(0) - conf.starttime > 10)
			break;
	}
	close(conf.fd);

	if(needconfig)
		fprint(conf.cfd, "remove %I %I", IPnoaddr, IPnoaddr);

}

#define HOUR (60*60)

void
dhcpwatch(int needconfig)
{
	int secs, s;
	ulong t;

	switch(rfork(RFPROC|RFFDG|RFNOWAIT|RFNOTEG)){
	default:
		return;
	case 0:
		break;
	}

	// keep trying to renew the lease
	for(;;){
		if(conf.lease == 0)
			secs = 5;
		else
			secs = conf.lease>>1;

		// avoid overflows
		for(s = secs; s > 0; s -= t){
			if(s > HOUR)
				t = HOUR;
			else
				t = s;
			sleep(t*1000);
		}

		if(conf.lease > 0){
			// during boot, the starttime can be bogus so avoid
			// spurious ipinconfig's
			t = time(0) - conf.starttime;
			if(t > (3*secs)/2)
				t = secs;
			if(t >= conf.lease){
				conf.lease = 0;
				if(!noconfig){
					ipunconfig();
					needconfig = 1;
				}
			} else
				conf.lease -= t;
		}

		dhcpquery(needconfig, needconfig ? Sselecting : Srenewing);

		if(needconfig && conf.state == Sbound){
			if(ipconfig() < 0)
				sysfatal("can't start ip: %r");
			needconfig = 0;

			// leave everything we've learned somewhere other procs can find it
			if(nip == 0){
				putndb();
				tweakservers();
			}
		}
	}
}

int
dhcptimer(void)
{
	ulong now;

	now = time(0);

	if(now < conf.timeout)
		return 0;

	switch(conf.state) {
	default:
		sysfatal("dhcptimer: unknown state %d", conf.state);
	case Sinit:
		break;
	case Sselecting:
		dhcpsend(Discover);
		conf.timeout = now + 4;
		conf.resend++;
		if(conf.resend>5)
			goto err;
		break;
	case Srequesting:
		dhcpsend(Request);
		conf.timeout = now + 4;
		conf.resend++;
		if(conf.resend>5)
			goto err;
		break;
	case Srenewing:
		dhcpsend(Request);
		conf.timeout = now + 1;
		conf.resend++;
		if(conf.resend>3) {
			conf.state = Srebinding;
			conf.resend = 0;
		}
		break;
	case Srebinding:
		dhcpsend(Request);
		conf.timeout = now + 4;
		conf.resend++;
		if(conf.resend>5)
			goto err;
		break;
	case Sbound:
		break;
	}
	return 0;
err:
	conf.state = Sinit;
	return -1;
}

uchar requested[] = {
	OBmask, OBrouter, OBdnserver, OBhostname, OBdomainname, OBntpserver,
};

void
dhcpsend(int type)
{
	Bootp bp;
	uchar *p;
	int n;
	uchar vendor[64];
	Udphdr *up = (Udphdr*)bp.udphdr;

	memset(&bp, 0, sizeof(bp));

	hnputs(up->rport, 67);
	bp.op = Bootrequest;
	hnputl(bp.xid, conf.xid);
	hnputs(bp.secs, time(0)-conf.starttime);
	hnputs(bp.flags, 0);
	memmove(bp.optmagic, optmagic, 4);
	if(conf.hwatype >= 0 && conf.hwalen < sizeof(bp.chaddr)){
		memmove(bp.chaddr, conf.hwa, conf.hwalen);
		bp.hlen = conf.hwalen;
		bp.htype = conf.hwatype;
	}
	p = bp.optdata;
	p = optaddbyte(p, ODtype, type);
	p = optadd(p, ODclientid, conf.cid, conf.cidlen);
	switch(type) {
	default:
		sysfatal("dhcpsend: unknown message type: %d", type);
	case Discover:
		ipmove(up->raddr, IPv4bcast);	// broadcast
		if(*conf.hostname)
			p = optaddstr(p, OBhostname, conf.hostname);
		if(plan9){
			n = snprint((char*)vendor, sizeof(vendor), "plan9_%s", conf.cputype);
			p = optaddvec(p, ODvendorclass, vendor, n);
		}
		p = optaddvec(p, ODparams, requested, sizeof(requested));
		if(validip(conf.laddr))
			p = optaddaddr(p, ODipaddr, conf.laddr);
		break;
	case Request:
		switch(conf.state){
		case Srenewing:
			ipmove(up->raddr, conf.server);
			v6tov4(bp.ciaddr, conf.laddr);
			break;
		case Srebinding:
			ipmove(up->raddr, IPv4bcast);	// broadcast
			v6tov4(bp.ciaddr, conf.laddr);
			break;
		case Srequesting:
			ipmove(up->raddr, IPv4bcast);	// broadcast
			p = optaddaddr(p, ODipaddr, conf.laddr);
			p = optaddaddr(p, ODserverid, conf.server);
			break;
		}
		p = optaddulong(p, ODlease, conf.offered);
		if(plan9){
			n = snprint((char*)vendor, sizeof(vendor), "plan9_%s", conf.cputype);
			p = optaddvec(p, ODvendorclass, vendor, n);
		}
		p = optaddvec(p, ODparams, requested, sizeof(requested));
		if(*conf.hostname)
			p = optaddstr(p, OBhostname, conf.hostname);
		break;	
	case Release:
		ipmove(up->raddr, conf.server);
		v6tov4(bp.ciaddr, conf.laddr);
		p = optaddaddr(p, ODipaddr, conf.laddr);
		p = optaddaddr(p, ODserverid, conf.server);
		break;
	}

	*p++ = OBend;

	n = p - (uchar*)&bp;
	USED(n);

	/*
	 *  We use a maximum size DHCP packet to survive the
	 *  All_Aboard NAT package from Internet Share.  It
	 *  always replies to DHCP requests with a packet of the
	 *  same size, so if the request is too short the reply
	 *  is truncated.
	 */
	if(write(conf.fd, &bp, sizeof(bp)) != sizeof(bp))
		fprint(2, "dhcpsend: write failed: %r\n");
}

void
dhcprecv(void)
{
	uchar buf[8000];
	Bootp *bp;
	int i, n, type;
	ulong lease;
	char err[ERRMAX];
	uchar vopts[256];

	alarm(1000);
	n = read(conf.fd, buf, sizeof(buf));
	alarm(0);

	if(n < 0){
		errstr(err, sizeof err);
		if(strstr(err, "interrupt") == nil)
			fprint(2, "ipconfig: bad read: %s\n", err);
		return;
	}

	bp = parsebootp(buf, n);
	if(bp == 0)
		return;

	type = optgetbyte(bp->optdata, ODtype);
	switch(type) {
	default:
		fprint(2, "%s: unknown type: %d\n", argv0, type);
		break;
	case Offer:
		DEBUG("got offer from %V ", bp->siaddr);
		if(conf.state != Sselecting){
			DEBUG("\n");
			break;
		}
		lease = optgetulong(bp->optdata, ODlease);
		if(lease == 0){
			/*
			 *  The All_Aboard NAT package from Internet Share doesn't
			 *  give a lease time, so we have to assume one.
			 */
			fprint(2, "%s: Offer with %lud lease, using %d\n", argv0, lease, MinLease);
			lease = MinLease;
		}
		DEBUG("lease %lud ", lease);
		if(!optgetaddr(bp->optdata, ODserverid, conf.server)) {
			fprint(2, "%s: Offer from server with invalid serverid\n", argv0);
			break;
		}

		v4tov6(conf.laddr, bp->yiaddr);
		memmove(conf.sname, bp->sname, sizeof(conf.sname));
		conf.sname[sizeof(conf.sname)-1] = 0;
		DEBUG("server %I %s\n", conf.server, conf.sname);
		conf.offered = lease;
		conf.state = Srequesting;
		dhcpsend(Request);
		conf.resend = 0;
		conf.timeout = time(0) + 4;
		break;
	case Ack:
		DEBUG("got ack from %V ", bp->siaddr);
		if(conf.state != Srequesting)
		if(conf.state != Srenewing)
		if(conf.state != Srebinding)
			break;

		// ignore a bad lease
		lease = optgetulong(bp->optdata, ODlease);
		if(lease == 0){
			/*
			 *  The All_Aboard NAT package from Internet Share doesn't
			 *  give a lease time, so we have to assume one.
			 */
			fprint(2, "%s: Ack with %lud lease, using %d\n", argv0, lease, MinLease);
			lease = MinLease;
		}
		DEBUG("lease %lud ", lease);

		// address and mask
		v4tov6(conf.laddr, bp->yiaddr);
		if(!optgetaddr(bp->optdata, OBmask, conf.mask))
			ipmove(conf.mask, IPnoaddr);
		DEBUG("addr %I mask %M ", conf.laddr, conf.mask);

		// get a router address either from the router option
		// or from the router that forwarded the dhcp packet
		if(optgetaddr(bp->optdata, OBrouter, conf.gaddr)){
			DEBUG("router %I ", conf.gaddr);
		} else {
			if(memcmp(bp->giaddr, IPnoaddr+IPv4off, IPv4addrlen) != 0){
				v4tov6(conf.gaddr, bp->giaddr);
				DEBUG("giaddr %I ", conf.gaddr);
			}
		}

		// get dns servers
		memset(conf.dns, 0, sizeof(conf.dns));
		n = optgetaddrs(bp->optdata, OBdnserver, conf.dns,
				sizeof(conf.dns)/IPaddrlen);
		for(i = 0; i < n; i++)
			DEBUG("dns %I ", conf.dns+i*IPaddrlen);

		// get ntp servers
		memset(conf.ntp, 0, sizeof(conf.ntp));
		n = optgetaddrs(bp->optdata, OBntpserver, conf.ntp,
				sizeof(conf.ntp)/IPaddrlen);
		for(i = 0; i < n; i++)
			DEBUG("ntp %I ", conf.ntp+i*IPaddrlen);

		// get names
		optgetstr(bp->optdata, OBhostname, conf.hostname, sizeof(conf.hostname));
		optgetstr(bp->optdata, OBdomainname, conf.domainname, sizeof(conf.domainname));

		// get plan9 specific options
		n = optgetvec(bp->optdata, OBvendorinfo, vopts, sizeof(vopts)-1);
		if(n > 0){
			if(parseoptions(vopts, n) == 0){
				n = optgetaddrs(vopts, OP9fs, conf.fs, 2);
				for(i = 0; i < n; i++)
					DEBUG("fs %I ", conf.fs+i*IPaddrlen);
				n = optgetaddrs(vopts, OP9auth, conf.auth, 2);
				for(i = 0; i < n; i++)
					DEBUG("auth %I ", conf.auth+i*IPaddrlen);
			}
		}
		conf.lease = lease;
		conf.state = Sbound;
		DEBUG("server %I %s\n", conf.server, conf.sname);
		break;
	case Nak:
		conf.state = Sinit;
		fprint(2, "%s: recved dhcpnak on %s\n", argv0, conf.mpoint);
		break;
	}

}

int
openlisten()
{
	int fd, cfd;
	char data[128];
	char devdir[40];
	int n;

	if(validip(conf.laddr)
	&& (conf.state == Srenewing || conf.state == Srebinding))
		sprint(data, "%s/udp!%I!68", conf.mpoint, conf.laddr);
	else
		sprint(data, "%s/udp!*!68", conf.mpoint);
	for(n=0;;n++) {
		cfd = announce(data, devdir);
		if(cfd >= 0)
			break;

		// might be another client - wait and try again
		fprint(2, "%s: can't announce: %r\n", argv0);
		sleep((nrand(10)+1)*1000);
		if(n > 10)
			return -1;
	}

	if(fprint(cfd, "headers") < 0)
		sysfatal("can't set header mode: %r");

	sprint(data, "%s/data", devdir);

	fd = open(data, ORDWR);
	if(fd < 0)
		sysfatal("open udp data: %r");
	close(cfd);

	return fd;
}

uchar*
optadd(uchar *p, int op, void *d, int n)
{
	p[0] = op;
	p[1] = n;
	memmove(p+2, d, n);
	return p+n+2;
}

uchar*
optaddbyte(uchar *p, int op, int b)
{
	p[0] = op;
	p[1] = 1;
	p[2] = b;
	return p+3;
}

uchar*
optaddulong(uchar *p, int op, ulong x)
{
	p[0] = op;
	p[1] = 4;
	hnputl(p+2, x);
	return p+6;
}

uchar *
optaddaddr(uchar *p, int op, uchar *ip)
{
	p[0] = op;
	p[1] = 4;
	v6tov4(p+2, ip);
	return p+6;
}

uchar *
optaddvec(uchar *p, int op, uchar *v, int n)
{
	p[0] = op;
	p[1] = n;
	memmove(p+2, v, n);
	return p+2+n;
}

uchar *
optaddstr(uchar *p, int op, char *v)
{
	int n;

	n = strlen(v)+1;	// microsoft leaves on the null, so we do too
	p[0] = op;
	p[1] = n;
	memmove(p+2, v, n);
	return p+2+n;
}

uchar*
optget(uchar *p, int op, int *np)
{
	int len, code;

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
		if(np != nil){
			if(*np > len)
				return 0;
			*np = len;
		}
		return p;
	}
}

int
optgetbyte(uchar *p, int op)
{
	int len;

	len = 1;
	p = optget(p, op, &len);
	if(p == 0)
		return 0;
	return *p;
}

ulong
optgetulong(uchar *p, int op)
{
	int len;

	len = 4;
	p = optget(p, op, &len);
	if(p == 0)
		return 0;
	return nhgetl(p);
}

int
optgetaddr(uchar *p, int op, uchar *ip)
{
	int len;

	len = 4;
	p = optget(p, op, &len);
	if(p == 0)
		return 0;
	v4tov6(ip, p);
	return 1;
}

int
optgetaddrs(uchar *p, int op, uchar *ip, int n)
{
	int len, i;

	len = 4;
	p = optget(p, op, &len);
	if(p == 0)
		return 0;
	len /= IPv4addrlen;
	if(len > n)
		len = n;
	for(i = 0; i < len; i++)
		v4tov6(&ip[i*IPaddrlen], &p[i*IPv4addrlen]);

	return i;
}

int
optgetvec(uchar *p, int op, uchar *v, int n)
{
	int len;

	len = 1;
	p = optget(p, op, &len);
	if(p == 0)
		return 0;
	if(len > n)
		len = n;
	memmove(v, p, len);

	return len;
}

int
optgetstr(uchar *p, int op, char *s, int n)
{
	int len;

	len = 1;
	p = optget(p, op, &len);
	if(p == 0)
		return 0;
	if(len >= n)
		len = n-1;
	memmove(s, p, len);
	s[len] = 0;

	return len;
}

// sanity check options area
//	- options don't overflow packet
//	- options end with an OBend
int
parseoptions(uchar *p, int n)
{
	int code, len;
	int nin = n;

	while(n>0) {
		code = *p++;
		n--;
		if(code == OBpad)
			continue;
		if(code == OBend)
			return 0;
		if(n == 0) {
			fprint(2, "%s: parse: bad option: 0x%ux: truncated: opt length = %d\n",
				argv0, code, nin);
			return -1;
		}
		len = *p++;
		n--;
		if(len > n) {
			fprint(2, "%s: parse: bad option: 0x%ux: %d > %d: opt length = %d\n",
				argv0, code, len, n, nin);
			return -1;
		}
		p += len;
		n -= len;		
	}

	// make sure packet ends with an OBend all the optget code
	*p = OBend;
	return 0;
}

//  sanity check received packet:
//	- magic is dhcp magic
//	- options don't overflow packet
Bootp *
parsebootp(uchar *p, int n)
{
	Bootp *bp;

	bp = (Bootp*)p;
	if(n < bp->optmagic - p) {
		fprint(2, "%s: parse: short bootp packet\n", argv0);
		return nil;
	}

	if(conf.xid != nhgetl(bp->xid))
		return nil;

	if(bp->op != Bootreply) {
		fprint(2, "%s: parse: bad op\n", argv0);
		return nil;
	}

	n -= bp->optmagic - p;
	p = bp->optmagic;

	if(n < 4) {
		fprint(2, "%s: parse: not option data\n", argv0);
		return nil;
	}
	if(memcmp(optmagic, p, 4) != 0) {
		fprint(2, "%s: parse: bad opt magic %ux %ux %ux %ux\n", argv0,
			p[0], p[1], p[2], p[3]);
		return nil;
	}
	p += 4;
	n -= 4;
	if(parseoptions(p, n) < 0)
		return nil;

	return bp;
}

// write out an ndb entry
void
writendb(char *s, int n, int append)
{
	char file[64];
	int fd;

	snprint(file, sizeof file, "%s/ndb", conf.mpoint);
	if(append){
		fd = open(file, OWRITE);
		seek(fd, 0, 2);
	} else
		fd = open(file, OWRITE|OTRUNC);
	write(fd, s, n);
	close(fd);
}

// put server addresses into the ndb entry
char*
putaddrs(char *p, char *e, char *attr, uchar *a, int len)
{
	int i;

	for(i = 0; i < len; i += IPaddrlen, a += IPaddrlen){
		if(!validip(a))
			break;
		p = seprint(p, e, "%s=%I\n", attr, a);
	}
	return p;
}

// make an ndb entry and put it into /net/ndb for the servers to see
void
putndb(void)
{
	char buf[1024];
	char *p, *e, *np;
	int append;

	e = buf + sizeof(buf);
	p = buf;
	if(getndb() == 0){
		append = 1;
	} else {
		append = 0;
		p = seprint(p, e, "ip=%I ipmask=%M ipgw=%I\n",
				conf.laddr, conf.mask, conf.gaddr);
	}
	if(np = strchr(conf.hostname, '.')){
		if(*conf.domainname == 0)
			strcpy(conf.domainname, np+1);
		*np = 0;
	}
	if(*conf.hostname)
		p = seprint(p, e, "\tsys=%s\n", conf.hostname);
	if(*conf.domainname)
		p = seprint(p, e, "\tdom=%s.%s\n", conf.hostname, conf.domainname);
	if(validip(conf.fs))
		p = putaddrs(p, e, "\tfs", conf.fs, sizeof(conf.fs));
	if(validip(conf.auth))
		p = putaddrs(p, e, "\tauth", conf.auth, sizeof(conf.auth));
	if(validip(conf.dns))
		p = putaddrs(p, e, "\tdns", conf.dns, sizeof(conf.dns));
	if(validip(conf.ntp))
		p = putaddrs(p, e, "\tntp", conf.ntp, sizeof(conf.ntp));
	if(p > buf)
		writendb(buf, p-buf, append);
}

// get an ndb entry someone else wrote
int
getndb(void)
{
	char buf[1024];
	int fd;
	int n;
	char *p;

	snprint(buf, sizeof buf, "%s/ndb", conf.mpoint);
	fd = open(buf, OREAD);
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;
	p = strstr(buf, "ip=");
	if(p == nil)
		return -1;
	parseip(conf.laddr, p+3);

	return 0;
}

// tell a server to refresh
void
tweakserver(char *server)
{
	char file[64];
	int fd;

	snprint(file, sizeof(file), "%s/%s", conf.mpoint, server);
	fd = open(file, ORDWR);
	if(fd < 0)
		return;
	fprint(fd, "refresh");
	close(fd);
}

// tell all servers to refresh their information
void
tweakservers(void)
{
	tweakserver("dns");
	tweakserver("cs");
}

// return number of networks
int
nipifcs(char *net)
{
	Ipifc *nifc;
	Iplifc *lifc;
	int n;

	n = 0;
	ifc = readipifc(net, ifc, -1);
	for(nifc = ifc; nifc != nil; nifc = nifc->next){
		for(lifc = nifc->lifc; lifc != nil; lifc = lifc->next)
			if(validip(lifc->ip)){
				n++;
				break;
			}
		if(strcmp(nifc->dev, conf.dev) == 0)
			myifc = nifc->index;
	}
	return n;
}

// return true if this is a valid v4 address
int
validip(uchar *addr)
{
	return ipcmp(addr, IPnoaddr) != 0 && ipcmp(addr, v4prefix) != 0;
}

char *verbs[] = {
[Vadd]		"add",
[Vremove]	"remove",
[Vunbind]	"unbind",
};

// look for an action
int
parseverb(char *name)
{
	int i;

	for(i = 0; i < nelem(verbs); i++){
		if(verbs[i] == 0)
			continue;
		if(strcmp(name, verbs[i]) == 0)
			return i;
	}
	return -1;
}

