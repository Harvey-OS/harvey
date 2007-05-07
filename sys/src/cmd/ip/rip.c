#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>

enum
{
	Version=	1,
	Pasize=		4,

	/*
	 *  definitions that are innately tied to BSD
	 */
	AF_INET=	2,
	AF_UNSPEC=	0,

	/*
	 *  Packet types.
	 */
	Request=	1,
	Response=	2,
	Traceon=	3,
	Traceoff=	4,

	Infinity=	16,	/* infinite hop count */
	Maxpacket=	488,	/* largest packet body */
};


/*
 *  network info
 */
typedef struct Rip	Rip;
struct Rip
{
	uchar	family[2];
	uchar	port[2];
	uchar	addr[Pasize];
	uchar	pad[8];
	uchar	metric[4];
};
typedef struct Ripmsg	Ripmsg;
struct Ripmsg
{
	uchar	type;
	uchar	vers;
	uchar	pad[2];
	Rip	rip[1];		/* the rest of the packet consists of routes */
};

enum
{
	Maxroutes=	(Maxpacket-4)/sizeof(Ripmsg),
};

/*
 *  internal route info
 */
enum
{
	Nroute=	2048,		/* this has to be smaller than what /ip has */
	Nhash=	256,		/* routing hash buckets */
	Nifc=	16,
};

typedef struct Route	Route;
struct Route
{
	Route	*next;

	uchar	dest[Pasize];
	uchar	mask[Pasize];
	uchar	gate[Pasize];
	int	metric;
	int	inuse;
	long	time;
};
struct {
	Route	route[Nroute];
	Route	*hash[Nhash];
	int	nroute;
	Route	def;	/* default route (immutable by us) */
} ralloc;

typedef struct Ifc	Ifc;
struct Ifc
{
	int	bcast;
	uchar	addr[Pasize];	/* my address */
	uchar	mask[Pasize];	/* subnet mask */
	uchar	net[Pasize];	/* subnet */
	uchar	*cmask;		/* class mask */
	uchar	cnet[Pasize];	/* class net */
};
struct {
	Ifc	ifc[Nifc];
	int	nifc;
} ialloc;

/*
 *  specific networks to broadcast on
 */
typedef struct Bnet Bnet;
struct Bnet
{
	Bnet	*next;
	uchar	addr[Pasize];
};
Bnet	*bnets;

int	ripfd;
long	now;
int	debug;
int	readonly;
char	routefile[256];
char	netdir[256];

int	openport(void);
void	readroutes(void);
void	readifcs(void);
void	considerroute(Route*);
void	installroute(Route*);
void	removeroute(Route*);
uchar	*getmask(uchar*);
void	broadcast(void);
void	timeoutroutes(void);

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRMAX], sysbuf[ERRMAX];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(syserr) {
		errstr(sysbuf, sizeof sysbuf);
		fprint(2, "routed: %s: %s\n", buf, sysbuf);
	}
	else
		fprint(2, "routed: %s\n", buf);
	exits(buf);
}

ulong
v4parseipmask(uchar *ip, char *p)
{
	ulong x;
	uchar v6ip[IPaddrlen];

	x = parseipmask(v6ip, p);
	memmove(ip, v6ip+IPv4off, 4);
	return x;
}

uchar*
v4defmask(uchar *ip)
{
	uchar v6ip[IPaddrlen];

	v4tov6(v6ip, ip);
	ip = defmask(v6ip);
	return ip+IPv4off;
}

void
v4maskip(uchar *from, uchar *mask, uchar *to)
{
	int i;

	for(i = 0; i < Pasize; i++)
		*to++ = *from++ & *mask++;
}

void
v6tov4mask(uchar *v4, uchar *v6)
{
	memmove(v4, v6+IPv4off, 4);
}

#define equivip(a, b) (memcmp((a), (b), Pasize) == 0)

void
ding(void *u, char *msg)
{
	USED(u);

	if(strstr(msg, "alarm"))
		noted(NCONT);
	noted(NDFLT);
}

void
usage(void)
{
	fprint(2, "usage: %s [-bnd] [-x netmtpt]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int dobroadcast, i, n;
	long diff;
	char *p;
	char buf[2*1024];
	uchar raddr[Pasize];
	Bnet *bn, **l;
	Udphdr *up;
	Rip *r;
	Ripmsg *m;
	Route route;
	static long btime;

	setnetmtpt(netdir, sizeof(netdir), nil);
	dobroadcast = 0;
	ARGBEGIN{
	case 'b':
		dobroadcast++;
		break;
	case 'd':
		debug++;
		break;
	case 'n':
		readonly++;
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(netdir, sizeof(netdir), p);
		break;
	default:
		usage();
	}ARGEND

	/* specific broadcast nets */
	l = &bnets;
	while(argc > 0){
		bn = (Bnet*)malloc(sizeof(Bnet));
		if(bn == 0)
			fatal(1, "out of mem");
		v4parseip(bn->addr, *argv);
		*l = bn;
		l = &bn->next;
		argc--;
		argv++;
		dobroadcast++;
	}

	/* command returns */
	if(!debug)
		switch(rfork(RFNOTEG|RFPROC|RFFDG|RFNOWAIT)) {
		case -1:
			fatal(1, "fork");
		case 0:
			break;
		default:
			exits(0);
		}


	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);

	snprint(routefile, sizeof(routefile), "%s/iproute", netdir);
	snprint(buf, sizeof(buf), "%s/iproute", netdir);

	now = time(0);
	readifcs();
	readroutes();

	notify(ding);

	ripfd = openport();
	for(;;) {
		diff = btime - time(0);
		if(diff <= 0){
			if(dobroadcast)
				broadcast();
			timeoutroutes();

			btime = time(0) + 2*60;
			diff = 2*60;
		}
		alarm(diff*1000);
		n = read(ripfd, buf, sizeof(buf));
		alarm(0);
		if(n <= 0)
			continue;

		n = (n - Udphdrsize - 4) / sizeof(Rip);
		if(n <= 0)
			continue;

		up = (Udphdr*)buf;
		m = (Ripmsg*)(buf+Udphdrsize);
		if(m->type != Response || m->vers != Version)
			continue;
		v6tov4(raddr, up->raddr);

		/* ignore our own messages */
		for(i = 0; i < ialloc.nifc; i++)
			if(equivip(ialloc.ifc[i].addr, raddr))
				continue;

		now = time(0);
		for(r = m->rip; r < &m->rip[n]; r++){
			memmove(route.gate, raddr, Pasize);
			memmove(route.mask, getmask(r->addr), Pasize);
			v4maskip(r->addr, route.mask, route.dest);
			route.metric = nhgetl(r->metric) + 1;
			if(route.metric < 1)
				continue;
			considerroute(&route);
		}
	}
	/* not reached */
}

int
openport(void)
{
	int ripctl, rip;
	char data[128], devdir[40];

	snprint(data, sizeof(data), "%s/udp!*!rip", netdir);
	ripctl = announce(data, devdir);
	if(ripctl < 0)
		fatal(1, "can't announce");
	if(fprint(ripctl, "headers") < 0)
		fatal(1, "can't set header mode");

	sprint(data, "%s/data", devdir);
	rip = open(data, ORDWR);
	if(rip < 0)
		fatal(1, "open udp data");
	return rip;
}

Ipifc *ifcs;

void
readifcs(void)
{
	Ipifc *ifc;
	Iplifc *lifc;
	Ifc *ip;
	Bnet *bn;
	Route route;
	int i;

	ifcs = readipifc(netdir, ifcs, -1);
	i = 0;
	for(ifc = ifcs; ifc != nil; ifc = ifc->next){
		for(lifc = ifc->lifc; lifc != nil && i < Nifc; lifc = lifc->next){
			// ignore any interfaces that aren't v4
			if(memcmp(lifc->ip, v4prefix, IPaddrlen-IPv4addrlen) != 0)
				continue;
			ip = &ialloc.ifc[i++];
			v6tov4(ip->addr, lifc->ip);
			v6tov4mask(ip->mask, lifc->mask);
			v6tov4(ip->net, lifc->net);
			ip->cmask = v4defmask(ip->net);
			v4maskip(ip->net, ip->cmask, ip->cnet);
			ip->bcast = 0;

			/* add as a route */
			memmove(route.mask, ip->mask, Pasize);
			memmove(route.dest, ip->net, Pasize);
			memset(route.gate, 0, Pasize);
			route.metric = 0;
			considerroute(&route);

			/* mark as broadcast */
			if(bnets == 0)
				ip->bcast = 1;
			else for(bn = bnets; bn; bn = bn->next)
				if(memcmp(bn->addr, ip->net, Pasize) == 0){
					ip->bcast = 1;
					break;
				}
		}
	}
	ialloc.nifc = i;
}

void
readroutes(void)
{
	int n;
	char *p;
	Biobuf *b;
	char *f[6];
	Route route;

	b = Bopen(routefile, OREAD);
	if(b == 0)
		return;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		n = getfields(p, f, 6, 1, " \t");
		if(n < 5)
			continue;
		v4parseip(route.dest, f[0]);
		v4parseipmask(route.mask, f[1]);
		v4parseip(route.gate, f[2]);
		route.metric = Infinity;
		if(equivip(route.dest, ralloc.def.dest)
		&& equivip(route.mask, ralloc.def.mask))
			memmove(ralloc.def.gate, route.gate, Pasize);
		else if(!equivip(route.dest, route.gate) && strchr(f[3], 'i') == 0)
			considerroute(&route);
	}
	Bterm(b);
}

/*
 *  route's hashed by net, not subnet
 */
ulong
rhash(uchar *d)
{
	ulong h;
	uchar net[Pasize];

	v4maskip(d, v4defmask(d), net);
	h = net[0] + net[1] + net[2];
	return h % Nhash;
}

/*
 *  consider installing a route.  Do so only if it is better than what
 *  we have.
 */
void
considerroute(Route *r)
{
	ulong h;
	Route *hp;

	if(debug)
		fprint(2, "consider %16V & %16V -> %16V %d\n", r->dest, r->mask, r->gate, r->metric);

	r->next = 0;
	r->time = now;
	r->inuse = 1;

	/* don't allow our default route to be highjacked */
	if(equivip(r->dest, ralloc.def.dest) || equivip(r->mask, ralloc.def.mask))
		return;

	h = rhash(r->dest);
	for(hp = ralloc.hash[h]; hp; hp = hp->next){
		if(equivip(hp->dest, r->dest)){
			/*
			 *  found a match, replace if better (or much newer)
			 */
			if(r->metric < hp->metric || now-hp->time > 5*60){
				removeroute(hp);
				memmove(hp->mask, r->mask, Pasize);
				memmove(hp->gate, r->gate, Pasize);
				hp->metric = r->metric;
				installroute(hp);
			}
			if(equivip(hp->gate, r->gate))
				hp->time = now;
			return;
		}
	}

	/*
	 *  no match, look for space
	 */
	for(hp = ralloc.route; hp < &ralloc.route[Nroute]; hp++)
		if(hp->inuse == 0)
			break;

	if(hp == 0)
		fatal(0, "no more routes");

	memmove(hp, r, sizeof(Route));
	hp->next = ralloc.hash[h];
	ralloc.hash[h] = hp;
	installroute(hp);
}

void
removeroute(Route *r)
{
	int fd;

	fd = open(routefile, ORDWR);
	if(fd < 0){
		fprint(2, "can't open oproute\n");
		return;
	}
	if(!readonly)
		fprint(fd, "delete %V", r->dest);
	if(debug)
		fprint(2, "removeroute %V\n", r->dest);
	close(fd);
}

/*
 *  pass a route to the kernel or /ip.  Don't bother if it is just the default
 *  gateway.
 */
void
installroute(Route *r)
{
	int fd;
	ulong h;
	Route *hp;
	uchar net[Pasize];

	/*
	 *  don't install routes whose gateway is 00000000
	 */
	if(equivip(r->gate, ralloc.def.dest))
		return;

	fd = open(routefile, ORDWR);
	if(fd < 0){
		fprint(2, "can't open oproute\n");
		return;
	}
	h = rhash(r->dest);

	/*
	 *  if the gateway is the same as the default gateway
	 *  we may be able to avoid a entry in the kernel
	 */
	if(equivip(r->gate, ralloc.def.gate)){
		/*
		 *  look for a less specific match
		 */
		for(hp = ralloc.hash[h]; hp; hp = hp->next){
			v4maskip(hp->mask, r->dest, net);
			if(equivip(net, hp->dest) && !equivip(hp->gate, ralloc.def.gate))
				break;
		}
		/*
		 *  if no less specific match, just use the default
		 */
		if(hp == 0){
			if(!readonly)
				fprint(fd, "delete %V", r->dest);
			if(debug)
				fprint(2, "delete %V\n", r->dest);
			close(fd);
			return;
		}
	}
	if(!readonly)
		fprint(fd, "add %V %V %V", r->dest, r->mask, r->gate);
	if(debug)
		fprint(2, "add %V & %V -> %V\n", r->dest, r->mask, r->gate);
	close(fd);
}

/*
 *  return true of dest is on net
 */
int
onnet(uchar *dest, uchar *net, uchar *netmask)
{
	uchar dnet[Pasize];

	v4maskip(dest, netmask, dnet);
	return equivip(dnet, net);
}

/*
 *  figure out what mask to use, if we have a direct connected network
 *  with the same class net use its subnet mask.
 */
uchar*
getmask(uchar *dest)
{
	int i;
	Ifc *ip;
	ulong mask, nmask;
	uchar *m;

	m = 0;
	mask = 0xffffffff;
	for(i = 0; i < ialloc.nifc; i++){
		ip = &ialloc.ifc[i];
		if(onnet(dest, ip->cnet, ip->cmask)){
			nmask = nhgetl(ip->mask);
			if(nmask < mask){
				mask = nmask;
				m = ip->mask;
			}
		}
	}

	if(m == 0)
		m = v4defmask(dest);
	return m;
}

/*
 *  broadcast routes onto all networks
 */
void
sendto(Ifc *ip)
{
	int h, n;
	uchar raddr[Pasize], mbuf[Udphdrsize+512];
	Ripmsg *m;
	Route *r;
	Udphdr *u;

	u = (Udphdr*)mbuf;
	for(n = 0; n < Pasize; n++)
		raddr[n] = ip->net[n] | ~(ip->mask[n]);
	v4tov6(u->raddr, raddr);
	hnputs(u->rport, 520);
	m = (Ripmsg*)(mbuf+Udphdrsize);
	m->type = Response;
	m->vers = Version;
	if(debug)
		fprint(2, "to %V\n", u->raddr);

	n = 0;
	for(h = 0; h < Nhash; h++){
		for(r = ralloc.hash[h]; r; r = r->next){
			/*
			 *  don't send any route back to the net
			 *  it came from
			 */
			if(onnet(r->gate, ip->net, ip->mask))
				continue;

			/*
			 *  don't tell a network about itself
			 */
			if(equivip(r->dest, ip->net))
				continue;

			/*
			 *  don't tell nets about other net's subnets
			 */
			if(!equivip(r->mask, v4defmask(r->dest))
			&& !equivip(ip->cmask, v4defmask(r->dest)))
				continue;

			memset(&m->rip[n], 0, sizeof(m->rip[n]));
			memmove(m->rip[n].addr, r->dest, Pasize);
			if(r->metric < 1)
				hnputl(m->rip[n].metric, 1);
			else
				hnputl(m->rip[n].metric, r->metric);
			hnputs(m->rip[n].family, AF_INET);

			if(debug)
				fprint(2, " %16V & %16V -> %16V %2d\n",
					r->dest, r->mask, r->gate, r->metric);

			if(++n == Maxroutes && !readonly){
				write(ripfd, mbuf, Udphdrsize + 4 + n*20);
				n = 0;
			}
		}
	}

	if(n && !readonly)
		write(ripfd, mbuf, Udphdrsize+4+n*20);
}
void
broadcast(void)
{
	int i;

	readifcs();
	for(i = 0; i < ialloc.nifc; i++){
		if(ialloc.ifc[i].bcast)
			sendto(&ialloc.ifc[i]);
	}
}

/*
 *  timeout any routes that haven't been refreshed and aren't wired
 */
void
timeoutroutes(void)
{
	int h;
	long now;
	Route *r, **l;

	now = time(0);

	for(h = 0; h < Nhash; h++){
		l = &ralloc.hash[h];
		for(r = *l; r; r = *l){
			if(r->metric < Infinity && now - r->time > 10*60){
				removeroute(r);
				r->inuse = 0;
				*l = r->next;
				continue;
			}
			l = &r->next;
		}
	}
}
