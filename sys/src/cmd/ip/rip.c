#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>

#define LOG if(debug)syslog

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
	Nroute=	1000,		/* this has to be smaller than what /ip has */
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

char	*logname = "rip";
int	ripfd;
long	now;
int	debug;

int	nhgets(uchar*);
long	nhgetl(uchar*);
void	hnputs(uchar*, int);
void	hnputl(uchar*, long);
int	openport(void);
void	readroutes(void);
void	readifcs(void);
void	considerroute(Route*);
void	installroute(Route*);
void	removeroute(Route*);
uchar	*getmask(uchar*);
void	broadcast(void);

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRLEN], sysbuf[ERRLEN];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(syserr) {
		errstr(sysbuf);
		fprint(2, "rip: %s: %s\n", buf, sysbuf);
	}
	else
		fprint(2, "rip: %s\n", buf);
	exits(buf);
}

void
ding(void *u, char *msg)
{
	USED(u);

	if(strstr(msg, "alarm"))
		noted(NCONT);
	noted(NDFLT);
}

void
main(int argc, char *argv[])
{
	int i, n;
	Udphdr *up;
	Ripmsg *m;
	Rip *r;
	Route route;
	Bnet *bn, **l;
	int dobroadcast;
	char buf[2*1024];

	dobroadcast = 0;
	ARGBEGIN{
	case 'b':
		dobroadcast++;
		break;
	case 'd':
		debug++;
		break;
	default:
		fprint(2, "usage: rip [-b]\n");
		exits("usage");
	}ARGEND

	/* specific broadcast nets */
	l = &bnets;
	while(argc > 0){
		bn = (Bnet*)malloc(sizeof(Bnet));
		if(bn == 0)
			fatal(1, "out of mem");
		parseip(bn->addr, *argv);
		*l = bn;
		l = &bn->next;
		argc--;
		argv++;
		dobroadcast++;
	}

	/* command returns */
	switch(rfork(RFNOTEG|RFPROC|RFFDG|RFNOWAIT)) {
	case -1:
		fatal(1, "fork");
	case 0:
		break;
	default:
		exits(0);
	}


	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	if(stat("/net/iproute", buf) < 0)
		bind("#P", "/net", MAFTER);

	readifcs();
	readroutes();

	if(dobroadcast)
		notify(ding);

	ripfd = openport();
	for(;;) {
		if(dobroadcast){
			long diff;
			static long btime;

			diff = btime - time(0);
			if(diff <= 0){
				broadcast();
				btime = time(0) + 2*60;
				diff = 2*60;
			}
			alarm(diff*1000);
		}

		n = read(ripfd, buf, sizeof(buf));
		alarm(0);
		if(n <= 0)
			continue;

		n = (n-Udphdrsize-4)/sizeof(Rip);
		if(n <= 0)
			continue;

		up = (Udphdr*)buf;
		m = (Ripmsg*)(buf+Udphdrsize);
		if(m->type != Response || m->vers != Version)
			continue;

		/* ignore our own messages */
		for(i = 0; i < ialloc.nifc; i++)
			if(equivip(ialloc.ifc[i].addr, up->ipaddr))
				continue;

		now = time(0);
		for(r = m->rip; r < &m->rip[n]; r++){
			memmove(route.gate, up->ipaddr, Pasize);
			memmove(route.mask, getmask(r->addr), Pasize);
			maskip(r->addr, route.mask, route.dest);
			route.metric = nhgetl(r->metric) + 1;
			if(route.metric < 1)
				continue;
			considerroute(&route);
		}
	}
	exits(0);
}

int
openport(void)
{
	char data[128];
	char devdir[40];
	int ripctl, rip;

	ripctl = announce("udp!*!rip", devdir);
	if(ripctl < 0)
		fatal(1, "can't announce");
	if(write(ripctl, "headers", sizeof("headers")) < 0)
		fatal(1, "can't set header mode");

	sprint(data, "%s/data", devdir);

	rip = open(data, ORDWR);
	if(rip < 0)
		fatal(1, "open udp data");
	return rip;
}

void
readifcs(void)
{
	int i, n;
	char *p;
	Biobuf *b;
	char *f[6];
	Ifc *ip;
	Bnet *bn;
	Route route;

	b = Bopen("/net/ipifc", OREAD);
	if(b == 0)
		return;
	setfields(" ");
	i = 0;
	while(p = Brdline(b, '\n')){
		if(i >= Nifc)
			break;
		p[Blinelen(b)-1] = 0;
		if(strchr(p, ':'))
			break;			/* error summaries in brazil */
		n = getmfields(p, f, 6);
		if(n < 5)
			continue;
		ip = &ialloc.ifc[i++];
		parseip(ip->addr, f[2]);
		parseip(ip->mask, f[3]);
		parseip(ip->net, f[4]);
		ip->cmask = classmask[CLASS(ip->net)];
		maskip(ip->net, ip->cmask, ip->cnet);
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
	ialloc.nifc = i;
	Bterm(b);
}

void
readroutes(void)
{
	int n;
	char *p;
	Biobuf *b;
	char *f[6];
	Route route;

	b = Bopen("/net/iproute", OREAD);
	if(b == 0)
		return;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		n = getmfields(p, f, 6);
		if(n < 5)
			continue;
		parseip(route.dest, f[0]);
		parseip(route.mask, f[2]);
		parseip(route.gate, f[4]);
		route.metric = Infinity;
		if(equivip(route.dest, ralloc.def.dest)
		&& equivip(route.mask, ralloc.def.mask))
			memmove(ralloc.def.gate, route.gate, Pasize);
		else
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

	maskip(d, classmask[CLASS(d)], net);
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
	ulong h, i;
	Route *hp, **l;

	if(debug)
		fprint(2, "consider %16I & %16I -> %16I %d\n", r->dest, r->mask, r->gate, r->metric);

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

	/*
	 *  look for an old entry
	 */
	if(hp == &ralloc.route[Nroute])
		for(i = 0; hp == 0 && i < Nhash; i++){
			l = &ralloc.hash[i];
			for(hp = *l; hp; hp = *l){
				if(now - hp->time > 10*60){
					removeroute(hp);
					*l = hp->next;
					break;
				}
				l = &hp->next;
			}
		}

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

	fd = open("/net/iproute", ORDWR);
	if(fd < 0){
		fprint(2, "rip: can't open iproute: %r\n");
		return;
	}
	fprint(fd, "delete %I", r->dest);
	if(debug)
		LOG(0, logname, "delete %I", r->dest);
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

	/* don't install routes that already go through the default */
	if(equivip(r->gate, ralloc.def.dest))
		return;

	fd = open("/net/iproute", ORDWR);
	if(fd < 0){
		fprint(2, "rip: can't open iproute: %r\n");
		return;
	}
	h = rhash(r->dest);
	if(equivip(r->gate, ralloc.def.gate)){
		for(hp = ralloc.hash[h]; hp; hp = hp->next){
			maskip(hp->mask, r->dest, net);
			if(equivip(net, hp->dest) && !equivip(hp->gate, ralloc.def.gate))
				break;
		}
		if(hp == 0){
			fprint(fd, "delete %I", r->dest);
			if(debug)
				LOG(0, logname, "delete %I", r->dest);
			close(fd);
			return;
		}
	}
	fprint(fd, "add %I %I %I", r->dest, r->mask, r->gate);
	if(debug)
		LOG(0, logname, "add %I & %I -> %I", r->dest, r->mask, r->gate);
	close(fd);
}

/*
 *  return true of dest is on net
 */
int
onnet(uchar *dest, uchar *net, uchar *netmask)
{
	uchar dnet[Pasize];

	maskip(dest, netmask, dnet);
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
		m = classmask[CLASS(dest)];
	return m;
}

/*
 *  broadcast routes onto all networks
 */
void
sendto(Ifc *ip)
{
	int h, n;
	Route *r;
	uchar mbuf[Udphdrsize+512];
	Ripmsg *m;
	Udphdr *u;

	u = (Udphdr*)mbuf;
	for(n = 0; n < Pasize; n++)
		u->ipaddr[n] = ip->net[n] | ~(ip->mask[n]);
	hnputs(u->port, 520);
	m = (Ripmsg*)(mbuf+Udphdrsize);
	m->type = Response;
	m->vers = Version;
	if(debug)
		fprint(2, "to %I\n", u->ipaddr);

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
			if(!equivip(r->mask, classmask[CLASS(r->dest)])
			&& !equivip(ip->cmask, classmask[CLASS(r->dest)]))
				continue;

			memset(&m->rip[n], 0, sizeof(m->rip[n]));
			memmove(m->rip[n].addr, r->dest, Pasize);
			if(r->metric < 1)
				hnputl(m->rip[n].metric, 1);
			else
				hnputl(m->rip[n].metric, r->metric);
			hnputs(m->rip[n].family, AF_INET);

			if(debug)
				fprint(2, " %16I & %16I -> %16I %2d\n", r->dest, r->mask, r->gate, r->metric);

			if(++n == Maxroutes){
				write(ripfd, mbuf, Udphdrsize+4+n*20);
				n = 0;
			}
		}
	}

	if(n)
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

int
nhgets(uchar *p)
{
	return (p[0]<<8) | p[1];
}

long
nhgetl(uchar *p)
{
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

void
hnputs(uchar *p, int x)
{
	p[0] = x>>8;
	p[1] = x;
}

void
hnputl(uchar *p, long x)
{
	p[0] = x>>24;
	p[1] = x>>16;
	p[2] = x>>8;
	p[3] = x;
}
