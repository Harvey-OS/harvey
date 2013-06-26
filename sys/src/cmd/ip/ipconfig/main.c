/*
 * ipconfig - configure parameters of an ip stack
 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "../dhcp.h"
#include "ipconfig.h"

#define DEBUG if(debug)warning

/* possible verbs */
enum
{
	/* commands */
	Vadd,
	Vremove,
	Vunbind,
	Vaddpref6,
	Vra6,
	/* media */
	Vether,
	Vgbe,
	Vppp,
	Vloopback,
	Vtorus,
	Vtree,
	Vpkt,
};

enum
{
	Taddr,
	Taddrs,
	Tstr,
	Tbyte,
	Tulong,
	Tvec,
};

typedef struct Option Option;
struct Option
{
	char	*name;
	int	type;
};

/*
 * I was too lazy to look up the types for each of these
 * options.  If someone feels like it, please mail me a
 * corrected array -- presotto
 */
Option option[256] =
{
[OBmask]		{ "ipmask",		Taddr },
[OBtimeoff]		{ "timeoff",		Tulong },
[OBrouter]		{ "ipgw",		Taddrs },
[OBtimeserver]		{ "time",		Taddrs },
[OBnameserver]		{ "name",		Taddrs },
[OBdnserver]		{ "dns",		Taddrs },
[OBlogserver]		{ "log",		Taddrs },
[OBcookieserver]	{ "cookie",		Taddrs },
[OBlprserver]		{ "lpr",		Taddrs },
[OBimpressserver]	{ "impress",		Taddrs },
[OBrlserver]		{ "rl",			Taddrs },
[OBhostname]		{ "sys",		Tstr },
[OBbflen]		{ "bflen",		Tulong },
[OBdumpfile]		{ "dumpfile",		Tstr },
[OBdomainname]		{ "dom",		Tstr },
[OBswapserver]		{ "swap",		Taddrs },
[OBrootpath]		{ "rootpath",		Tstr },
[OBextpath]		{ "extpath",		Tstr },
[OBipforward]		{ "ipforward",		Taddrs },
[OBnonlocal]		{ "nonlocal",		Taddrs },
[OBpolicyfilter]	{ "policyfilter",	Taddrs },
[OBmaxdatagram]		{ "maxdatagram",	Tulong },
[OBttl]			{ "ttl",		Tulong },
[OBpathtimeout]		{ "pathtimeout",	Taddrs },
[OBpathplateau]		{ "pathplateau",	Taddrs },
[OBmtu]			{ "mtu",		Tulong },
[OBsubnetslocal]	{ "subnetslocal",	Taddrs },
[OBbaddr]		{ "baddr",		Taddrs },
[OBdiscovermask]	{ "discovermask",	Taddrs },
[OBsupplymask]		{ "supplymask",		Taddrs },
[OBdiscoverrouter]	{ "discoverrouter",	Taddrs },
[OBrsserver]		{ "rs",			Taddrs },
[OBstaticroutes]	{ "staticroutes",	Taddrs },
[OBtrailerencap]	{ "trailerencap",	Taddrs },
[OBarptimeout]		{ "arptimeout",		Tulong },
[OBetherencap]		{ "etherencap",		Taddrs },
[OBtcpttl]		{ "tcpttl",		Tulong },
[OBtcpka]		{ "tcpka",		Tulong },
[OBtcpkag]		{ "tcpkag",		Tulong },
[OBnisdomain]		{ "nisdomain",		Tstr },
[OBniserver]		{ "ni",			Taddrs },
[OBntpserver]		{ "ntp",		Taddrs },
[OBnetbiosns]		{ "netbiosns",		Taddrs },
[OBnetbiosdds]		{ "netbiosdds",		Taddrs },
[OBnetbiostype]		{ "netbiostype",	Taddrs },
[OBnetbiosscope]	{ "netbiosscope",	Taddrs },
[OBxfontserver]		{ "xfont",		Taddrs },
[OBxdispmanager]	{ "xdispmanager",	Taddrs },
[OBnisplusdomain]	{ "nisplusdomain",	Tstr },
[OBnisplusserver]	{ "nisplus",		Taddrs },
[OBhomeagent]		{ "homeagent",		Taddrs },
[OBsmtpserver]		{ "smtp",		Taddrs },
[OBpop3server]		{ "pop3",		Taddrs },
[OBnntpserver]		{ "nntp",		Taddrs },
[OBwwwserver]		{ "www",		Taddrs },
[OBfingerserver]	{ "finger",		Taddrs },
[OBircserver]		{ "irc",		Taddrs },
[OBstserver]		{ "st",			Taddrs },
[OBstdaserver]		{ "stdar",		Taddrs },

[ODipaddr]		{ "ipaddr",		Taddr },
[ODlease]		{ "lease",		Tulong },
[ODoverload]		{ "overload",		Taddr },
[ODtype]		{ "type",		Tbyte },
[ODserverid]		{ "serverid",		Taddr },
[ODparams]		{ "params",		Tvec },
[ODmessage]		{ "message",		Tstr },
[ODmaxmsg]		{ "maxmsg",		Tulong },
[ODrenewaltime]		{ "renewaltime",	Tulong },
[ODrebindingtime]	{ "rebindingtime",	Tulong },
[ODvendorclass]		{ "vendorclass",	Tvec },
[ODclientid]		{ "clientid",		Tvec },
[ODtftpserver]		{ "tftp",		Taddr },
[ODbootfile]		{ "bootfile",		Tstr },
};

uchar defrequested[] = {
	OBmask, OBrouter, OBdnserver, OBhostname, OBdomainname, OBntpserver,
};

uchar	requested[256];
int	nrequested;

int	Oflag;
int	beprimary = -1;
Conf	conf;
int	debug;
int	dodhcp;
int	dolog;
int	dondbconfig = 0;
int	dupl_disc = 1;		/* flag: V6 duplicate neighbor discovery */
Ctl	*firstctl, **ctll;
Ipifc	*ifc;
int	ipv6auto = 0;
int	myifc = -1;
char	*ndboptions;
int	nip;
int	noconfig;
int	nodhcpwatch;
char 	optmagic[4] = { 0x63, 0x82, 0x53, 0x63 };
int	plan9 = 1;
int	sendhostname;

static char logfile[] = "ipconfig";

char *verbs[] = {
[Vadd]		"add",
[Vremove]	"remove",
[Vunbind]	"unbind",
[Vether]	"ether",
[Vgbe]		"gbe",
[Vppp]		"ppp",
[Vloopback]	"loopback",
[Vaddpref6]	"add6",
[Vra6]		"ra6",
[Vtorus]	"torus",
[Vtree]		"tree",
[Vpkt]		"pkt",
};

void	adddefroute(char*, uchar*);
int	addoption(char*);
void	binddevice(void);
void	bootprequest(void);
void	controldevice(void);
void	dhcpquery(int, int);
void	dhcprecv(void);
void	dhcpsend(int);
int	dhcptimer(void);
void	dhcpwatch(int);
void	doadd(int);
void	doremove(void);
void	dounbind(void);
int	getndb(void);
void	getoptions(uchar*);
int	ip4cfg(void);
int	ip6cfg(int a);
void	lookforip(char*);
void	mkclientid(void);
void	ndbconfig(void);
int	nipifcs(char*);
int	openlisten(void);
uchar*	optaddaddr(uchar*, int, uchar*);
uchar*	optaddbyte(uchar*, int, int);
uchar*	optaddstr(uchar*, int, char*);
uchar*	optadd(uchar*, int, void*, int);
uchar*	optaddulong(uchar*, int, ulong);
uchar*	optaddvec(uchar*, int, uchar*, int);
int	optgetaddrs(uchar*, int, uchar*, int);
int	optgetp9addrs(uchar*, int, uchar*, int);
int	optgetaddr(uchar*, int, uchar*);
int	optgetbyte(uchar*, int);
int	optgetstr(uchar*, int, char*, int);
uchar*	optget(uchar*, int, int*);
ulong	optgetulong(uchar*, int);
int	optgetvec(uchar*, int, uchar*, int);
char*	optgetx(uchar*, uchar);
Bootp*	parsebootp(uchar*, int);
int	parseoptions(uchar *p, int n);
int	parseverb(char*);
void	pppbinddev(void);
void	putndb(void);
void	tweakservers(void);
void	usage(void);
int	validip(uchar*);
void	writendb(char*, int, int);

void
usage(void)
{
	fprint(2, "usage: %s [-6dDGnNOpPruX][-b baud][-c ctl]* [-g gw]"
		"[-h host][-m mtu]\n"
		"\t[-x mtpt][-o dhcpopt] type dev [verb] [laddr [mask "
		"[raddr [fs [auth]]]]]\n", argv0);
	exits("usage");
}

void
warning(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf + sizeof buf, fmt, arg);
	va_end(arg);
	if (dolog)
		syslog(0, logfile, "%s", buf);
	else
		fprint(2, "%s: %s\n", argv0, buf);
}

static void
parsenorm(int argc, char **argv)
{
	switch(argc){
	case 5:
		 if (parseip(conf.auth, argv[4]) == -1)
			usage();
		/* fall through */
	case 4:
		 if (parseip(conf.fs, argv[3]) == -1)
			usage();
		/* fall through */
	case 3:
		 if (parseip(conf.raddr, argv[2]) == -1)
			usage();
		/* fall through */
	case 2:
		/*
		 * can't test for parseipmask()==-1 cuz 255.255.255.255
		 * looks like that.
		 */
		if (strcmp(argv[1], "0") != 0)
			parseipmask(conf.mask, argv[1]);
		/* fall through */
	case 1:
		 if (parseip(conf.laddr, argv[0]) == -1)
			usage();
		/* fall through */
	case 0:
		break;
	default:
		usage();
	}
}

static void
parse6pref(int argc, char **argv)
{
	switch(argc){
	case 6:
		conf.preflt = strtoul(argv[5], 0, 10);
		/* fall through */
	case 5:
		conf.validlt = strtoul(argv[4], 0, 10);
		/* fall through */
	case 4:
		conf.autoflag = (atoi(argv[3]) != 0);
		/* fall through */
	case 3:
		conf.onlink = (atoi(argv[2]) != 0);
		/* fall through */
	case 2:
		conf.prefixlen = atoi(argv[1]);
		/* fall through */
	case 1:
		if (parseip(conf.v6pref, argv[0]) == -1)
			sysfatal("bad address %s", argv[0]);
		break;
	}
	DEBUG("parse6pref: pref %I len %d", conf.v6pref, conf.prefixlen);
}

/* parse router advertisement (keyword, value) pairs */
static void
parse6ra(int argc, char **argv)
{
	int i, argsleft;
	char *kw, *val;

	if (argc % 2 != 0)
		usage();

	i = 0;
	for (argsleft = argc; argsleft > 1; argsleft -= 2) {
		kw =  argv[i];
		val = argv[i+1];
		if (strcmp(kw, "recvra") == 0)
			conf.recvra = (atoi(val) != 0);
		else if (strcmp(kw, "sendra") == 0)
			conf.sendra = (atoi(val) != 0);
		else if (strcmp(kw, "mflag") == 0)
			conf.mflag = (atoi(val) != 0);
		else if (strcmp(kw, "oflag") == 0)
			conf.oflag = (atoi(val) != 0);
		else if (strcmp(kw, "maxraint") == 0)
			conf.maxraint = atoi(val);
		else if (strcmp(kw, "minraint") == 0)
			conf.minraint = atoi(val);
		else if (strcmp(kw, "linkmtu") == 0)
			conf.linkmtu = atoi(val);
		else if (strcmp(kw, "reachtime") == 0)
			conf.reachtime = atoi(val);
		else if (strcmp(kw, "rxmitra") == 0)
			conf.rxmitra = atoi(val);
		else if (strcmp(kw, "ttl") == 0)
			conf.ttl = atoi(val);
		else if (strcmp(kw, "routerlt") == 0)
			conf.routerlt = atoi(val);
		else {
			warning("bad ra6 keyword %s", kw);
			usage();
		}
		i += 2;
	}

	/* consistency check */
	if (conf.maxraint < conf.minraint)
		sysfatal("maxraint %d < minraint %d",
			conf.maxraint, conf.minraint);
}

static void
init(void)
{
	srand(truerand());
	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);
	fmtinstall('V', eipfmt);
 	nsec();			/* make sure time file is open before forking */

	setnetmtpt(conf.mpoint, sizeof conf.mpoint, nil);
	conf.cputype = getenv("cputype");
	if(conf.cputype == nil)
		conf.cputype = "386";

	ctll = &firstctl;
	v6paraminit(&conf);

	/* init set of requested dhcp parameters with the default */
	nrequested = sizeof defrequested;
	memcpy(requested, defrequested, nrequested);
}

static int
parseargs(int argc, char **argv)
{
	char *p;
	int action, verb;

	/* default to any host name we already have */
	if(*conf.hostname == 0){
		p = getenv("sysname");
		if(p == nil || *p == 0)
			p = sysname();
		if(p != nil)
			strncpy(conf.hostname, p, sizeof conf.hostname-1);
	}

	/* defaults */
	conf.type = "ether";
	conf.dev = "/net/ether0";
	action = Vadd;

	/* get optional medium and device */
	if (argc > 0){
		verb = parseverb(*argv);
		switch(verb){
		case Vether:
		case Vgbe:
		case Vppp:
		case Vloopback:
		case Vtorus:
		case Vtree:
		case Vpkt:
			conf.type = *argv++;
			argc--;
			if(argc > 0){
				conf.dev = *argv++;
				argc--;
			} else if(verb == Vppp)
				conf.dev = "/dev/eia0";
			break;
		}
	}

	/* get optional verb */
	if (argc > 0){
		verb = parseverb(*argv);
		switch(verb){
		case Vether:
		case Vgbe:
		case Vppp:
		case Vloopback:
		case Vtorus:
		case Vtree:
		case Vpkt:
			sysfatal("medium %s already specified", conf.type);
		case Vadd:
		case Vremove:
		case Vunbind:
		case Vaddpref6:
		case Vra6:
			argv++;
			argc--;
			action = verb;
			break;
		}
	}

	/* get verb-dependent arguments */
	switch (action) {
	case Vadd:
	case Vremove:
	case Vunbind:
		parsenorm(argc, argv);
		break;
	case Vaddpref6:
		parse6pref(argc, argv);
		break;
	case Vra6:
		parse6ra(argc, argv);
		break;
	}
	return action;
}

void
main(int argc, char **argv)
{
	int retry, action;
	Ctl *cp;

	init();
	retry = 0;
	ARGBEGIN {
	case '6': 			/* IPv6 auto config */
		ipv6auto = 1;
		break;
	case 'b':
		conf.baud = EARGF(usage());
		break;
	case 'c':
		cp = malloc(sizeof *cp);
		if(cp == nil)
			sysfatal("%r");
		*ctll = cp;
		ctll = &cp->next;
		cp->next = nil;
		cp->ctl = EARGF(usage());
		break;
	case 'd':
		dodhcp = 1;
		break;
	case 'D':
		debug = 1;
		break;
	case 'g':
		if (parseip(conf.gaddr, EARGF(usage())) == -1)
			usage();
		break;
	case 'G':
		plan9 = 0;
		break;
	case 'h':
		snprint(conf.hostname, sizeof conf.hostname, "%s",
			EARGF(usage()));
		sendhostname = 1;
		break;
	case 'm':
		conf.mtu = atoi(EARGF(usage()));
		break;
	case 'n':
		noconfig = 1;
		break;
	case 'N':
		dondbconfig = 1;
		break;
	case 'o':
		if(addoption(EARGF(usage())) < 0)
			usage();
		break;
	case 'O':
		Oflag = 1;
		break;
	case 'p':
		beprimary = 1;
		break;
	case 'P':
		beprimary = 0;
		break;
	case 'r':
		retry = 1;
		break;
	case 'u':		/* IPv6: duplicate neighbour disc. off */
		dupl_disc = 0;
		break;
	case 'x':
		setnetmtpt(conf.mpoint, sizeof conf.mpoint, EARGF(usage()));
		break;
	case 'X':
		nodhcpwatch = 1;
		break;
	default:
		usage();
	} ARGEND;
	argv0 = "ipconfig";		/* boot invokes us as tcp? */

	action = parseargs(argc, argv);
	switch(action){
	case Vadd:
		doadd(retry);
		break;
	case Vremove:
		doremove();
		break;
	case Vunbind:
		dounbind();
		break;
	case Vaddpref6:
	case Vra6:
		doipv6(action);
		break;
	}
	exits(0);
}

int
havendb(char *net)
{
	Dir *d;
	char buf[128];

	snprint(buf, sizeof buf, "%s/ndb", net);
	if((d = dirstat(buf)) == nil)
		return 0;
	if(d->length == 0){
		free(d);
		return 0;
	}
	free(d);
	return 1;
}

void
doadd(int retry)
{
	int tries, ppp;

	ppp = strcmp(conf.type, "ppp") == 0;

	/* get number of preexisting interfaces */
	nip = nipifcs(conf.mpoint);
	if(beprimary == -1 && (nip == 0 || !havendb(conf.mpoint)))
		beprimary = 1;

	/* get ipifc into name space and condition device for ip */
	if(!noconfig){
		lookforip(conf.mpoint);
		controldevice();
		binddevice();
	}

	if (ipv6auto && !ppp) {
		if (ip6cfg(ipv6auto) < 0)
			sysfatal("can't automatically start IPv6 on %s",
				conf.dev);
//		return;
	} else if (validip(conf.laddr) && !isv4(conf.laddr)) {
		if (ip6cfg(0) < 0)
			sysfatal("can't start IPv6 on %s, address %I",
				conf.dev, conf.laddr);
//		return;
	}

	if(!validip(conf.laddr) && !ppp)
		if(dondbconfig)
			ndbconfig();
		else
			dodhcp = 1;

	/* run dhcp if we need something */
	if(dodhcp){
		mkclientid();
		for(tries = 0; tries < 30; tries++){
			dhcpquery(!noconfig, Sselecting);
			if(conf.state == Sbound)
				break;
			sleep(1000);
		}
	}

	if(!validip(conf.laddr))
		if(retry && dodhcp && !noconfig){
			warning("couldn't determine ip address, retrying");
			dhcpwatch(1);
			return;
		} else
			sysfatal("no success with DHCP");

	if(!noconfig)
		if(ip4cfg() < 0)
			sysfatal("can't start ip");
		else if(dodhcp && conf.lease != Lforever)
			dhcpwatch(0);

	/* leave everything we've learned somewhere other procs can find it */
	if(beprimary == 1){
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

	if(!validip(conf.laddr))
		sysfatal("remove requires an address");
	ifc = readipifc(conf.mpoint, ifc, -1);
	for(nifc = ifc; nifc != nil; nifc = nifc->next){
		if(strcmp(nifc->dev, conf.dev) != 0)
			continue;
		for(lifc = nifc->lifc; lifc != nil; lifc = lifc->next){
			if(ipcmp(conf.laddr, lifc->ip) != 0)
				continue;
			if (validip(conf.mask) &&
			    ipcmp(conf.mask, lifc->mask) != 0)
				continue;
			if (validip(conf.raddr) &&
			    ipcmp(conf.raddr, lifc->net) != 0)
				continue;

			snprint(file, sizeof file, "%s/ipifc/%d/ctl",
				conf.mpoint, nifc->index);
			cfd = open(file, ORDWR);
			if(cfd < 0){
				warning("can't open %s: %r", conf.mpoint);
				continue;
			}
			if(fprint(cfd, "remove %I %M", lifc->ip, lifc->mask) < 0)
				warning("can't remove %I %M from %s: %r",
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
			snprint(file, sizeof file, "%s/ipifc/%d/ctl",
				conf.mpoint, nifc->index);
			cfd = open(file, ORDWR);
			if(cfd < 0){
				warning("can't open %s: %r", conf.mpoint);
				break;
			}
			if(fprint(cfd, "unbind") < 0)
				warning("can't unbind from %s: %r", file);
			break;
		}
	}
}

/* set the default route */
void
adddefroute(char *mpoint, uchar *gaddr)
{
	char buf[256];
	int cfd;

	sprint(buf, "%s/iproute", mpoint);
	cfd = open(buf, ORDWR);
	if(cfd < 0)
		return;

	if(isv4(gaddr))
		fprint(cfd, "add 0 0 %I", gaddr);
	else
		fprint(cfd, "add :: /0 %I", gaddr);
	close(cfd);
}

/* create a client id */
void
mkclientid(void)
{
	if(strcmp(conf.type, "ether") == 0 || strcmp(conf.type, "gbe") == 0)
		if(myetheraddr(conf.hwa, conf.dev) == 0){
			conf.hwalen = 6;
			conf.hwatype = 1;
			conf.cid[0] = conf.hwatype;
			memmove(&conf.cid[1], conf.hwa, conf.hwalen);
			conf.cidlen = conf.hwalen+1;
		} else {
			conf.hwatype = -1;
			snprint((char*)conf.cid, sizeof conf.cid,
				"plan9_%ld.%d", lrand(), getpid());
			conf.cidlen = strlen((char*)conf.cid);
		}
}

/* bind ip into the namespace */
void
lookforip(char *net)
{
	char proto[64];

	snprint(proto, sizeof proto, "%s/ipifc", net);
	if(access(proto, 0) == 0)
		return;
	sysfatal("no ip stack bound onto %s", net);
}

/* send some ctls to a device */
void
controldevice(void)
{
	char ctlfile[256];
	int fd;
	Ctl *cp;

	if (firstctl == nil ||
	    strcmp(conf.type, "ether") != 0 && strcmp(conf.type, "gbe") != 0)
		return;

	snprint(ctlfile, sizeof ctlfile, "%s/clone", conf.dev);
	fd = open(ctlfile, ORDWR);
	if(fd < 0)
		sysfatal("can't open %s", ctlfile);

	for(cp = firstctl; cp != nil; cp = cp->next){
		if(write(fd, cp->ctl, strlen(cp->ctl)) < 0)
			sysfatal("ctl message %s: %r", cp->ctl);
		seek(fd, 0, 0);
	}
//	close(fd);		/* or does it need to be left hanging? */
}

/* bind an ip stack to a device, leave the control channel open */
void
binddevice(void)
{
	char buf[256];

	if(strcmp(conf.type, "ppp") == 0)
		pppbinddev();
	else if(myifc < 0){
		/* get a new ip interface */
		snprint(buf, sizeof buf, "%s/ipifc/clone", conf.mpoint);
		conf.cfd = open(buf, ORDWR);
		if(conf.cfd < 0)
			sysfatal("opening %s/ipifc/clone: %r", conf.mpoint);

		/* specify medium as ethernet, bind the interface to it */
		if(fprint(conf.cfd, "bind %s %s", conf.type, conf.dev) < 0)
			sysfatal("%s: bind %s %s: %r", buf, conf.type, conf.dev);
	} else {
		/* open the old interface */
		snprint(buf, sizeof buf, "%s/ipifc/%d/ctl", conf.mpoint, myifc);
		conf.cfd = open(buf, ORDWR);
		if(conf.cfd < 0)
			sysfatal("open %s: %r", buf);
	}

}

/* add a logical interface to the ip stack */
int
ip4cfg(void)
{
	char buf[256];
	int n;

	if(!validip(conf.laddr))
		return -1;

	n = sprint(buf, "add");
	n += snprint(buf+n, sizeof buf-n, " %I", conf.laddr);

	if(!validip(conf.mask))
		ipmove(conf.mask, defmask(conf.laddr));
	n += snprint(buf+n, sizeof buf-n, " %I", conf.mask);

	if(validip(conf.raddr)){
		n += snprint(buf+n, sizeof buf-n, " %I", conf.raddr);
		if(conf.mtu != 0)
			n += snprint(buf+n, sizeof buf-n, " %d", conf.mtu);
	}

	if(write(conf.cfd, buf, n) < 0){
		warning("write(%s): %r", buf);
		return -1;
	}

	if(beprimary==1 && validip(conf.gaddr))
		adddefroute(conf.mpoint, conf.gaddr);

	return 0;
}

/* remove a logical interface to the ip stack */
void
ipunconfig(void)
{
	char buf[256];
	int n;

	if(!validip(conf.laddr))
		return;
	DEBUG("couldn't renew IP lease, releasing %I", conf.laddr);
	n = sprint(buf, "remove");
	n += snprint(buf+n, sizeof buf-n, " %I", conf.laddr);

	if(!validip(conf.mask))
		ipmove(conf.mask, defmask(conf.laddr));
	n += snprint(buf+n, sizeof buf-n, " %I", conf.mask);

	write(conf.cfd, buf, n);

	ipmove(conf.laddr, IPnoaddr);
	ipmove(conf.raddr, IPnoaddr);
	ipmove(conf.mask, IPnoaddr);

	/* forget configuration info */
	if(beprimary==1)
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

	/* try dhcp for 10 seconds */
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

enum {
	/*
	 * was an hour, needs to be less for the ARM/GS1 until the timer
	 * code has been cleaned up (pb).
	 */
	Maxsleep = 450,
};

void
dhcpwatch(int needconfig)
{
	int secs, s;
	ulong t;

	if(nodhcpwatch)
		return;

	switch(rfork(RFPROC|RFFDG|RFNOWAIT|RFNOTEG)){
	default:
		return;
	case 0:
		break;
	}

	dolog = 1;			/* log, don't print */
	procsetname("dhcpwatch");
	/* keep trying to renew the lease */
	for(;;){
		if(conf.lease == 0)
			secs = 5;
		else
			secs = conf.lease >> 1;

		/* avoid overflows */
		for(s = secs; s > 0; s -= t){
			if(s > Maxsleep)
				t = Maxsleep;
			else
				t = s;
			sleep(t*1000);
		}

		if(conf.lease > 0){
			/*
			 * during boot, the starttime can be bogus so avoid
			 * spurious ipunconfig's
			 */
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
		dhcpquery(needconfig, needconfig? Sselecting: Srenewing);

		if(needconfig && conf.state == Sbound){
			if(ip4cfg() < 0)
				sysfatal("can't start ip: %r");
			needconfig = 0;
			/*
			 * leave everything we've learned somewhere that
			 * other procs can find it.
			 */
			if(beprimary==1){
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
	case Sbound:
		break;
	case Sselecting:
	case Srequesting:
	case Srebinding:
		dhcpsend(conf.state == Sselecting? Discover: Request);
		conf.timeout = now + 4;
		if(++conf.resend > 5) {
			conf.state = Sinit;
			return -1;
		}
		break;
	case Srenewing:
		dhcpsend(Request);
		conf.timeout = now + 1;
		if(++conf.resend > 3) {
			conf.state = Srebinding;
			conf.resend = 0;
		}
		break;
	}
	return 0;
}

void
dhcpsend(int type)
{
	Bootp bp;
	uchar *p;
	int n;
	uchar vendor[64];
	Udphdr *up = (Udphdr*)bp.udphdr;

	memset(&bp, 0, sizeof bp);

	hnputs(up->rport, 67);
	bp.op = Bootrequest;
	hnputl(bp.xid, conf.xid);
	hnputs(bp.secs, time(0)-conf.starttime);
	hnputs(bp.flags, 0);
	memmove(bp.optmagic, optmagic, 4);
	if(conf.hwatype >= 0 && conf.hwalen < sizeof bp.chaddr){
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
		ipmove(up->raddr, IPv4bcast);	/* broadcast */
		if(*conf.hostname && sendhostname)
			p = optaddstr(p, OBhostname, conf.hostname);
		if(plan9){
			n = snprint((char*)vendor, sizeof vendor,
				"plan9_%s", conf.cputype);
			p = optaddvec(p, ODvendorclass, vendor, n);
		}
		p = optaddvec(p, ODparams, requested, nrequested);
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
			ipmove(up->raddr, IPv4bcast);	/* broadcast */
			v6tov4(bp.ciaddr, conf.laddr);
			break;
		case Srequesting:
			ipmove(up->raddr, IPv4bcast);	/* broadcast */
			p = optaddaddr(p, ODipaddr, conf.laddr);
			p = optaddaddr(p, ODserverid, conf.server);
			break;
		}
		p = optaddulong(p, ODlease, conf.offered);
		if(plan9){
			n = snprint((char*)vendor, sizeof vendor,
				"plan9_%s", conf.cputype);
			p = optaddvec(p, ODvendorclass, vendor, n);
		}
		p = optaddvec(p, ODparams, requested, nrequested);
		if(*conf.hostname && sendhostname)
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
	if(write(conf.fd, &bp, sizeof bp) != sizeof bp)
		warning("dhcpsend: write failed: %r");
}

void
dhcprecv(void)
{
	int i, n, type;
	ulong lease;
	char err[ERRMAX];
	uchar buf[8000], vopts[256], taddr[IPaddrlen];
	Bootp *bp;

	memset(buf, 0, sizeof buf);
	alarm(1000);
	n = read(conf.fd, buf, sizeof buf);
	alarm(0);

	if(n < 0){
		rerrstr(err, sizeof err);
		if(strstr(err, "interrupt") == nil)
			warning("dhcprecv: bad read: %s", err);
		else
			DEBUG("dhcprecv: read timed out");
		return;
	}
	if(n == 0){
		warning("dhcprecv: zero-length packet read");
		return;
	}

	bp = parsebootp(buf, n);
	if(bp == 0) {
		DEBUG("parsebootp failed: dropping packet");
		return;
	}

	type = optgetbyte(bp->optdata, ODtype);
	switch(type) {
	default:
		warning("dhcprecv: unknown type: %d", type);
		break;
	case Offer:
		DEBUG("got offer from %V ", bp->siaddr);
		if(conf.state != Sselecting){
//			DEBUG("");
			break;
		}
		lease = optgetulong(bp->optdata, ODlease);
		if(lease == 0){
			/*
			 * The All_Aboard NAT package from Internet Share
			 * doesn't give a lease time, so we have to assume one.
			 */
			warning("Offer with %lud lease, using %d", lease, MinLease);
			lease = MinLease;
		}
		DEBUG("lease=%lud ", lease);
		if(!optgetaddr(bp->optdata, ODserverid, conf.server)) {
			warning("Offer from server with invalid serverid");
			break;
		}

		v4tov6(conf.laddr, bp->yiaddr);
		memmove(conf.sname, bp->sname, sizeof conf.sname);
		conf.sname[sizeof conf.sname-1] = 0;
		DEBUG("server=%I sname=%s", conf.server, conf.sname);
		conf.offered = lease;
		conf.state = Srequesting;
		dhcpsend(Request);
		conf.resend = 0;
		conf.timeout = time(0) + 4;
		break;
	case Ack:
		DEBUG("got ack from %V ", bp->siaddr);
		if (conf.state != Srequesting && conf.state != Srenewing &&
		    conf.state != Srebinding)
			break;

		/* ignore a bad lease */
		lease = optgetulong(bp->optdata, ODlease);
		if(lease == 0){
			/*
			 * The All_Aboard NAT package from Internet Share
			 * doesn't give a lease time, so we have to assume one.
			 */
			warning("Ack with %lud lease, using %d", lease, MinLease);
			lease = MinLease;
		}
		DEBUG("lease=%lud ", lease);

		/* address and mask */
		if(!validip(conf.laddr) || !Oflag)
			v4tov6(conf.laddr, bp->yiaddr);
		if(!validip(conf.mask) || !Oflag){
			if(!optgetaddr(bp->optdata, OBmask, conf.mask))
				ipmove(conf.mask, IPnoaddr);
		}
		DEBUG("ipaddr=%I ipmask=%M ", conf.laddr, conf.mask);

		/*
		 * get a router address either from the router option
		 * or from the router that forwarded the dhcp packet
		 */
		if(validip(conf.gaddr) && Oflag) {
			DEBUG("ipgw=%I ", conf.gaddr);
		} else if(optgetaddr(bp->optdata, OBrouter, conf.gaddr)){
			DEBUG("ipgw=%I ", conf.gaddr);
		} else if(memcmp(bp->giaddr, IPnoaddr+IPv4off, IPv4addrlen)!=0){
			v4tov6(conf.gaddr, bp->giaddr);
			DEBUG("giaddr=%I ", conf.gaddr);
		}

		/* get dns servers */
		memset(conf.dns, 0, sizeof conf.dns);
		n = optgetaddrs(bp->optdata, OBdnserver, conf.dns,
			sizeof conf.dns/IPaddrlen);
		for(i = 0; i < n; i++)
			DEBUG("dns=%I ", conf.dns + i*IPaddrlen);

		/* get ntp servers */
		memset(conf.ntp, 0, sizeof conf.ntp);
		n = optgetaddrs(bp->optdata, OBntpserver, conf.ntp,
			sizeof conf.ntp/IPaddrlen);
		for(i = 0; i < n; i++)
			DEBUG("ntp=%I ", conf.ntp + i*IPaddrlen);

		/* get names */
		optgetstr(bp->optdata, OBhostname,
			conf.hostname, sizeof conf.hostname);
		optgetstr(bp->optdata, OBdomainname,
			conf.domainname, sizeof conf.domainname);

		/* get anything else we asked for */
		getoptions(bp->optdata);

		/* get plan9-specific options */
		n = optgetvec(bp->optdata, OBvendorinfo, vopts, sizeof vopts-1);
		if(n > 0 && parseoptions(vopts, n) == 0){
			if(validip(conf.fs) && Oflag)
				n = 1;
			else {
				n = optgetp9addrs(vopts, OP9fs, conf.fs, 2);
				if (n == 0)
					n = optgetaddrs(vopts, OP9fsv4,
						conf.fs, 2);
			}
			for(i = 0; i < n; i++)
				DEBUG("fs=%I ", conf.fs + i*IPaddrlen);

			if(validip(conf.auth) && Oflag)
				n = 1;
			else {
				n = optgetp9addrs(vopts, OP9auth, conf.auth, 2);
				if (n == 0)
					n = optgetaddrs(vopts, OP9authv4,
						conf.auth, 2);
			}
			for(i = 0; i < n; i++)
				DEBUG("auth=%I ", conf.auth + i*IPaddrlen);

			n = optgetp9addrs(vopts, OP9ipaddr, taddr, 1);
			if (n > 0)
				memmove(conf.laddr, taddr, IPaddrlen);
			n = optgetp9addrs(vopts, OP9ipmask, taddr, 1);
			if (n > 0)
				memmove(conf.mask, taddr, IPaddrlen);
			n = optgetp9addrs(vopts, OP9ipgw, taddr, 1);
			if (n > 0)
				memmove(conf.gaddr, taddr, IPaddrlen);
			DEBUG("new ipaddr=%I new ipmask=%M new ipgw=%I",
				conf.laddr, conf.mask, conf.gaddr);
		}
		conf.lease = lease;
		conf.state = Sbound;
		DEBUG("server=%I sname=%s", conf.server, conf.sname);
		break;
	case Nak:
		conf.state = Sinit;
		warning("recved dhcpnak on %s", conf.mpoint);
		break;
	}
}

/* return pseudo-random integer in range low...(hi-1) */
ulong
randint(ulong low, ulong hi)
{
	if (hi < low)
		return low;
	return low + nrand(hi - low);
}

long
jitter(void)		/* compute small pseudo-random delay in ms */
{
	return randint(0, 10*1000);
}

int
openlisten(void)
{
	int n, fd, cfd;
	char data[128], devdir[40];

	if (validip(conf.laddr) &&
	    (conf.state == Srenewing || conf.state == Srebinding))
		sprint(data, "%s/udp!%I!68", conf.mpoint, conf.laddr);
	else
		sprint(data, "%s/udp!*!68", conf.mpoint);
	for (n = 0; (cfd = announce(data, devdir)) < 0; n++) {
		if(!noconfig)
			sysfatal("can't announce for dhcp: %r");

		/* might be another client - wait and try again */
		warning("can't announce %s: %r", data);
		sleep(jitter());
		if(n > 10)
			return -1;
	}

	if(fprint(cfd, "headers") < 0)
		sysfatal("can't set header mode: %r");

	sprint(data, "%s/data", devdir);
	fd = open(data, ORDWR);
	if(fd < 0)
		sysfatal("open %s: %r", data);
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

/* add dhcp option op with value v of length n to dhcp option array p */
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

	n = strlen(v)+1;	/* microsoft leaves on the NUL, so we do too */
	p[0] = op;
	p[1] = n;
	memmove(p+2, v, n);
	return p+2+n;
}

/*
 * parse p, looking for option `op'.  if non-nil, np points to minimum length.
 * return nil if option is too small, else ptr to opt, and
 * store actual length via np if non-nil.
 */
uchar*
optget(uchar *p, int op, int *np)
{
	int len, code;

	while ((code = *p++) != OBend) {
		if(code == OBpad)
			continue;
		len = *p++;
		if(code != op) {
			p += len;
			continue;
		}
		if(np != nil){
			if(*np > len) {
				return 0;
			}
			*np = len;
		}
		return p;
	}
	return 0;
}

int
optgetbyte(uchar *p, int op)
{
	int len;

	len = 1;
	p = optget(p, op, &len);
	if(p == nil)
		return 0;
	return *p;
}

ulong
optgetulong(uchar *p, int op)
{
	int len;

	len = 4;
	p = optget(p, op, &len);
	if(p == nil)
		return 0;
	return nhgetl(p);
}

int
optgetaddr(uchar *p, int op, uchar *ip)
{
	int len;

	len = 4;
	p = optget(p, op, &len);
	if(p == nil)
		return 0;
	v4tov6(ip, p);
	return 1;
}

/* expect at most n addresses; ip[] only has room for that many */
int
optgetaddrs(uchar *p, int op, uchar *ip, int n)
{
	int len, i;

	len = 4;
	p = optget(p, op, &len);
	if(p == nil)
		return 0;
	len /= IPv4addrlen;
	if(len > n)
		len = n;
	for(i = 0; i < len; i++)
		v4tov6(&ip[i*IPaddrlen], &p[i*IPv4addrlen]);
	return i;
}

/* expect at most n addresses; ip[] only has room for that many */
int
optgetp9addrs(uchar *ap, int op, uchar *ip, int n)
{
	int len, i, slen, addrs;
	char *p;

	len = 1;			/* minimum bytes needed */
	p = (char *)optget(ap, op, &len);
	if(p == nil)
		return 0;
	addrs = *p++;			/* first byte is address count */
	for (i = 0; i < n  && i < addrs && len > 0; i++) {
		slen = strlen(p) + 1;
		if (parseip(&ip[i*IPaddrlen], p) == -1)
			fprint(2, "%s: bad address %s\n", argv0, p);
		DEBUG("got plan 9 option %d addr %I (%s)",
			op, &ip[i*IPaddrlen], p);
		p += slen;
		len -= slen;
	}
	return addrs;
}

int
optgetvec(uchar *p, int op, uchar *v, int n)
{
	int len;

	len = 1;
	p = optget(p, op, &len);
	if(p == nil)
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
	if(p == nil)
		return 0;
	if(len >= n)
		len = n-1;
	memmove(s, p, len);
	s[len] = 0;
	return len;
}

/*
 * sanity check options area
 * 	- options don't overflow packet
 * 	- options end with an OBend
 */
int
parseoptions(uchar *p, int n)
{
	int code, len, nin = n;

	while (n > 0) {
		code = *p++;
		n--;
		if(code == OBend)
			return 0;
		if(code == OBpad)
			continue;
		if(n == 0) {
			warning("parseoptions: bad option: 0x%ux: truncated: "
				"opt length = %d", code, nin);
			return -1;
		}

		len = *p++;
		n--;
		DEBUG("parseoptions: %s(%d) len %d, bytes left %d",
			option[code].name, code, len, n);
		if(len > n) {
			warning("parseoptions: bad option: 0x%ux: %d > %d: "
				"opt length = %d", code, len, n, nin);
			return -1;
		}
		p += len;
		n -= len;
	}

	/* make sure packet ends with an OBend after all the optget code */
	*p = OBend;
	return 0;
}

/*
 * sanity check received packet:
 * 	- magic is dhcp magic
 * 	- options don't overflow packet
 */
Bootp *
parsebootp(uchar *p, int n)
{
	Bootp *bp;

	bp = (Bootp*)p;
	if(n < bp->optmagic - p) {
		warning("parsebootp: short bootp packet; with options, "
			"need %d bytes, got %d", bp->optmagic - p, n);
		return nil;
	}

	if(conf.xid != nhgetl(bp->xid))		/* not meant for us */
		return nil;

	if(bp->op != Bootreply) {
		warning("parsebootp: bad op %d", bp->op);
		return nil;
	}

	n -= bp->optmagic - p;
	p = bp->optmagic;

	if(n < 4) {
		warning("parsebootp: no option data");
		return nil;
	}
	if(memcmp(optmagic, p, 4) != 0) {
		warning("parsebootp: bad opt magic %ux %ux %ux %ux",
			p[0], p[1], p[2], p[3]);
		return nil;
	}
	p += 4;
	n -= 4;
	DEBUG("parsebootp: new packet");
	if(parseoptions(p, n) < 0)
		return nil;
	return bp;
}

/* write out an ndb entry */
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

/* put server addresses into the ndb entry */
char*
putaddrs(char *p, char *e, char *attr, uchar *a, int len)
{
	int i;

	for(i = 0; i < len && validip(a); i += IPaddrlen, a += IPaddrlen)
		p = seprint(p, e, "%s=%I\n", attr, a);
	return p;
}

/* make an ndb entry and put it into /net/ndb for the servers to see */
void
putndb(void)
{
	int append;
	char buf[1024];
	char *p, *e, *np;

	p = buf;
	e = buf + sizeof buf;
	if(getndb() == 0)
		append = 1;
	else {
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
		p = seprint(p, e, "\tdom=%s.%s\n",
			conf.hostname, conf.domainname);
	if(validip(conf.fs))
		p = putaddrs(p, e, "\tfs", conf.fs, sizeof conf.fs);
	if(validip(conf.auth))
		p = putaddrs(p, e, "\tauth", conf.auth, sizeof conf.auth);
	if(validip(conf.dns))
		p = putaddrs(p, e, "\tdns", conf.dns, sizeof conf.dns);
	if(validip(conf.ntp))
		p = putaddrs(p, e, "\tntp", conf.ntp, sizeof conf.ntp);
	if(ndboptions)
		p = seprint(p, e, "%s\n", ndboptions);
	if(p > buf)
		writendb(buf, p-buf, append);
}

/* get an ndb entry someone else wrote */
int
getndb(void)
{
	char buf[1024];
	int fd, n;
	char *p;

	snprint(buf, sizeof buf, "%s/ndb", conf.mpoint);
	fd = open(buf, OREAD);
	n = read(fd, buf, sizeof buf-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;
	p = strstr(buf, "ip=");
	if(p == nil)
		return -1;
	if (parseip(conf.laddr, p+3) == -1)
		fprint(2, "%s: bad address %s\n", argv0, p+3);
	return 0;
}

/* tell a server to refresh */
void
tweakserver(char *server)
{
	int fd;
	char file[64];

	snprint(file, sizeof file, "%s/%s", conf.mpoint, server);
	fd = open(file, ORDWR);
	if(fd < 0)
		return;
	fprint(fd, "refresh");
	close(fd);
}

/* tell all servers to refresh their information */
void
tweakservers(void)
{
	tweakserver("dns");
	tweakserver("cs");
}

/* return number of networks */
int
nipifcs(char *net)
{
	int n;
	Ipifc *nifc;
	Iplifc *lifc;

	n = 0;
	ifc = readipifc(net, ifc, -1);
	for(nifc = ifc; nifc != nil; nifc = nifc->next){
		/*
		 * ignore loopback devices when trying to
		 * figure out if we're the primary interface.
		 */
		if(strcmp(nifc->dev, "/dev/null") != 0)
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

/* return true if this is a valid v4 address */
int
validip(uchar *addr)
{
	return ipcmp(addr, IPnoaddr) != 0 && ipcmp(addr, v4prefix) != 0;
}

/* look for an action */
int
parseverb(char *name)
{
	int i;

	for(i = 0; i < nelem(verbs); i++)
		if(verbs[i] != nil && strcmp(name, verbs[i]) == 0)
			return i;
	return -1;
}

/* get everything out of ndb */
void
ndbconfig(void)
{
	int nattr, nauth = 0, ndns = 0, nfs = 0, ok;
	char etheraddr[32];
	char *attrs[10];
	Ndb *db;
	Ndbtuple *t, *nt;

	db = ndbopen(0);
	if(db == nil)
		sysfatal("can't open ndb: %r");
	if (strcmp(conf.type, "ether") != 0 && strcmp(conf.type, "gbe") != 0 ||
	    myetheraddr(conf.hwa, conf.dev) != 0)
		sysfatal("can't read hardware address");
	sprint(etheraddr, "%E", conf.hwa);
	nattr = 0;
	attrs[nattr++] = "ip";
	attrs[nattr++] = "ipmask";
	attrs[nattr++] = "ipgw";
	/* the @ triggers resolution to an IP address; see ndb(2) */
	attrs[nattr++] = "@dns";
	attrs[nattr++] = "@ntp";
	attrs[nattr++] = "@fs";
	attrs[nattr++] = "@auth";
	attrs[nattr] = nil;
	t = ndbipinfo(db, "ether", etheraddr, attrs, nattr);
	for(nt = t; nt != nil; nt = nt->entry) {
		ok = 1;
		if(strcmp(nt->attr, "ip") == 0)
			ok = parseip(conf.laddr, nt->val);
		else if(strcmp(nt->attr, "ipmask") == 0)
			parseipmask(conf.mask, nt->val);  /* could be -1 */
		else if(strcmp(nt->attr, "ipgw") == 0)
			ok = parseip(conf.gaddr, nt->val);
		else if(ndns < 2 && strcmp(nt->attr, "dns") == 0)
			ok = parseip(conf.dns+IPaddrlen*ndns, nt->val);
		else if(strcmp(nt->attr, "ntp") == 0)
			ok = parseip(conf.ntp, nt->val);
		else if(nfs < 2 && strcmp(nt->attr, "fs") == 0)
			ok = parseip(conf.fs+IPaddrlen*nfs, nt->val);
		else if(nauth < 2 && strcmp(nt->attr, "auth") == 0)
			ok = parseip(conf.auth+IPaddrlen*nauth, nt->val);
		if (!ok)
			fprint(2, "%s: bad %s address in ndb: %s\n", argv0,
				nt->attr, nt->val);
	}
	ndbfree(t);
	if(!validip(conf.laddr))
		sysfatal("address not found in ndb");
}

int
addoption(char *opt)
{
	int i;
	Option *o;

	if(opt == nil)
		return -1;
	for(o = option; o < &option[nelem(option)]; o++)
		if(o->name && strcmp(opt, o->name) == 0){
			i = o - option;
			if(memchr(requested, i, nrequested) == 0 &&
			    nrequested < nelem(requested))
				requested[nrequested++] = i;
			return 0;
		}
	return -1;
}

char*
optgetx(uchar *p, uchar opt)
{
	int i, n;
	ulong x;
	char *s, *ns;
	char str[256];
	uchar ip[IPaddrlen], ips[16*IPaddrlen], vec[256];
	Option *o;

	o = &option[opt];
	if(o->name == nil)
		return nil;

	s = nil;
	switch(o->type){
	case Taddr:
		if(optgetaddr(p, opt, ip))
			s = smprint("%s=%I", o->name, ip);
		break;
	case Taddrs:
		n = optgetaddrs(p, opt, ips, 16);
		if(n > 0)
			s = smprint("%s=%I", o->name, ips);
		for(i = 1; i < n; i++){
			ns = smprint("%s %s=%I", s, o->name, &ips[i*IPaddrlen]);
			free(s);
			s = ns;
		}
		break;
	case Tulong:
		x = optgetulong(p, opt);
		if(x != 0)
			s = smprint("%s=%lud", o->name, x);
		break;
	case Tbyte:
		x = optgetbyte(p, opt);
		if(x != 0)
			s = smprint("%s=%lud", o->name, x);
		break;
	case Tstr:
		if(optgetstr(p, opt, str, sizeof str))
			s = smprint("%s=%s", o->name, str);
		break;
	case Tvec:
		n = optgetvec(p, opt, vec, sizeof vec);
		if(n > 0)
			/* what's %H?  it's not installed */
			s = smprint("%s=%.*H", o->name, n, vec);
		break;
	}
	return s;
}

void
getoptions(uchar *p)
{
	int i;
	char *s, *t;

	for(i = nelem(defrequested); i < nrequested; i++){
		s = optgetx(p, requested[i]);
		if(s != nil)
			DEBUG("%s ", s);
		if(ndboptions == nil)
			ndboptions = smprint("\t%s", s);
		else{
			t = ndboptions;
			ndboptions = smprint("\t%s%s", s, ndboptions);
			free(t);
		}
		free(s);
	}
}
