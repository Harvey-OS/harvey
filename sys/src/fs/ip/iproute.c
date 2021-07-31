#include "all.h"

#include "../ip/ip.h"

#define	DEBUG	if(cons.flags&ralloc.flag)print

enum
{
	Version=	1,

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
	Nroute	= 2*1024,
	Nhash	= 256,		/* routing hash buckets */
	Nifc	= 16,
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
struct
{
	Lock;
	Route	route[Nroute];
	Route	*hash[Nhash];
	int	nroute;
	ulong	flag;
	int 	dorip;
} ralloc;

uchar classmask[4][4] =
{
	0xff, 0x00, 0x00, 0x00,
	0xff, 0x00, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xff, 0x00,
};

#define CLASS(p) ((*(uchar*)(p))>>6)

static void	considerroute(Route*);
static void	deleteroute(Route*);
static void	printroute(Route*);
static void	printroutes(void);
static void	installroute(Route*);
static void	getmask(uchar*, uchar*);
static void	maskip(uchar*, uchar*, uchar*);
static int	equivip(uchar*, uchar*);
static void	cmd_route(int, char*[]);
static ulong	rhash(uchar*);

void
iprouteinit(void)
{
	cmd_install("route", "subcommand -- ip routes", cmd_route);
	ralloc.flag = flag_install("route", "-- verbose");
	if(!conf.ripoff)
		ralloc.dorip = 1;
}

static void
cmd_route(int argc, char *argv[])
{
	Route r;

	if(argc < 2) {
usage:
		print("route add dest gate [mask] -- add a route\n");
		print("route delete dest -- remote a route\n");
		print("route print [dest] -- print routes\n");
		print("route ripon -- listen to RIP packets\n");
		print("route ripoff -- ignore RIP packets\n");
		return;
	}
	if(strcmp(argv[1], "ripoff") == 0)
		ralloc.dorip = 0;
	else
	if(strcmp(argv[1], "ripon") == 0)
		ralloc.dorip = 1;
	else
	if(strcmp(argv[1], "add") == 0) {
		switch(argc){
		default:
			goto usage;
		case 4:
			memmove(r.mask, classmask[CLASS(r.dest)], Pasize);
			break;
		case 5:
			if(chartoip(r.mask, argv[4]))
				goto usage;
			break;
		}
		if(chartoip(r.dest, argv[2]))
			goto usage;
		if(chartoip(r.gate, argv[3]))
			goto usage;
		r.metric = 0;			/* rip can't nuke these */
		deleteroute(&r);
		considerroute(&r);
	} else
	if(strcmp(argv[1], "delete") == 0) {
		if(argc != 3)
			goto usage;
		if(chartoip(r.dest, argv[2]))
			goto usage;
		deleteroute(&r);
	} else
	if(strcmp(argv[1], "print") == 0) {
		if(argc == 3) {
			if(chartoip(r.dest, argv[2]))
				goto usage;
			printroute(&r);
		} else
			printroutes();
	}
}

/*
 *  consider installing a route.  Do so only if it is better than what
 *  we have.
 */
static void
considerroute(Route *r)
{
	ulong h, i;
	ulong m, nm;
	Route *hp, **l;

	r->next = 0;
	r->time = time();
	r->inuse = 1;

	lock(&ralloc);
	h = rhash(r->dest);
	for(hp = ralloc.hash[h]; hp; hp = hp->next) {
		if(equivip(hp->dest, r->dest) && equivip(hp->mask, r->mask)) {
			/*
			 *  found a match, replace if better (or much newer)
			 */
			if(r->metric < hp->metric || time()-hp->time > 10*60) {
				memmove(hp->gate, r->gate, Pasize);
				hp->metric = r->metric;
				DEBUG("route: replacement %I & %I -> %I (%d)\n",
					hp->dest, hp->mask, hp->dest, hp->metric);
			}
			if(equivip(r->gate, hp->gate))
				hp->time = time();
			goto done;
		}
	}

	/*
	 *  no match, look for space
	 */
	for(hp = ralloc.route; hp < &ralloc.route[Nroute]; hp++)
		if(hp->inuse == 0)
			break;
	if(hp == &ralloc.route[Nroute])
		hp = 0;

	/*
	 *  look for an old entry
	 */
	for(i = 0; hp == 0 && i < Nhash; i++) {
		l = &ralloc.hash[i];
		for(hp = *l; hp; hp = *l) {
			if(time() - hp->time > 10*60 && hp->metric > 0){
				*l = hp->next;
				break;
			}
			l = &hp->next;
		}
	}

	if(hp == 0) {
		print("no more routes");
		goto done;
	}

	memmove(hp, r, sizeof(Route));

	/*
	 *  insert largest mask first
	 */
	m = nhgetl(hp->mask);
	for(l = &ralloc.hash[h]; *l; l = &(*l)->next){
		nm = nhgetl((*l)->mask);
		if(nm < m)
			break;
	}
	hp->next = *l;
	*l = hp;
	DEBUG("route: new %I & %I -> %I (%d)\n", hp->dest, hp->mask,
		hp->dest, hp->metric);
done:
	unlock(&ralloc);
}

static void
deleteroute(Route *r)
{
	int h;
	Route *hp, **l;

	lock(&ralloc);
	for(h = 0; h < Nhash; h++) {
		l = &ralloc.hash[h];
		for(hp = *l; hp; hp = *l){
			if(equivip(r->dest, hp->dest)) {
				*l = hp->next;
				hp->next = 0;
				hp->inuse = 0;
				break;
			}
			l = &hp->next;
		}
	}
	unlock(&ralloc);
}

static void
printroutes(void)
{
	Ifc *i;
	int h;
	Route *hp;
	uchar mask[Pasize];

	lock(&ralloc);
	for(h = 0; h < Nhash; h++)
		for(hp = ralloc.hash[h]; hp; hp = hp->next)
			print("%I & %I -> %I\n", hp->dest, hp->mask, hp->gate);
	unlock(&ralloc);

	print("\nifc's\n");
	for(i = enets; i; i = i->next) {
		hnputl(mask, i->mask);
		print("addr %I mask %I defgate %I\n", i->ipa, mask, i->netgate);
	}
}

static void
printroute(Route *r)
{
	int h;
	Route *hp;
	uchar net[Pasize];

	h = rhash(r->dest);
	for(hp = ralloc.hash[h]; hp; hp = hp->next){
		maskip(r->dest, hp->mask, net);
		if(equivip(hp->dest, net)){
			print("%I & %I -> %I\n", hp->dest, hp->mask, hp->gate);
			return;
		}
	}
	print("default * -> %I\n", enets[0].netgate);
}

void
iproute(uchar *to, uchar *dst, uchar *def)
{
	int h;
	Route *hp;
	uchar net[Pasize];

	h = rhash(dst);
	for(hp = ralloc.hash[h]; hp; hp = hp->next) {
		maskip(dst, hp->mask, net);
		if(equivip(hp->dest, net)) {
			def = hp->gate;
			break;
		}
	}
	memmove(to, def, Pasize);
}

void
riprecv(Msgbuf *mb, Ifc*)
{
	int n;
	Rip *r;
	Ripmsg *m;
	Udppkt *uh;
	Route route;

	if(ralloc.dorip == 0)
		goto drop;

	uh = (Udppkt*)mb->data;
	m = (Ripmsg*)(mb->data + Ensize + Ipsize + Udpsize);
	if(m->type != Response || m->vers != Version)
		goto drop;

	n = nhgets(uh->udplen);
	n -= Udpsize;
	n = n/sizeof(Rip);

	DEBUG("%d routes from %I\n", n, uh->src);

	memmove(route.gate, uh->src, Pasize);
	for(r = m->rip; r < &m->rip[n]; r++){
		getmask(route.mask, r->addr);
		maskip(r->addr, route.mask, route.dest);
		route.metric = nhgetl(r->metric) + 1;
		if(route.metric < 1)
			continue;
		considerroute(&route);
	}
drop:
	mbfree(mb);
}

/*
 *  route's hashed by net, not subnet
 */
static ulong
rhash(uchar *d)
{
	ulong h;
	uchar net[Pasize];

	maskip(d, classmask[CLASS(d)], net);
	h = net[0] + net[1] + net[2];
	return h % Nhash;
}

/*
 *  figure out what mask to use, if we have a direct connected network
 *  with the same class net use its subnet mask.
 */
static void
getmask(uchar *mask, uchar *dest)
{
	Ifc *i;
	long ip;

	ip = nhgetl(dest);
	for(i = enets; i; i = i->next)
		if((i->ipaddr & i->cmask) == (ip & i->cmask)) {
			hnputl(mask, i->mask);
			return;
		}

	memmove(mask, classmask[CLASS(dest)], Pasize);
}

static void
maskip(uchar *a, uchar *m, uchar *n)
{
	int i;

	for(i = 0; i < 4; i++)
		n[i] = a[i] & m[i];
}

static int
equivip(uchar *a, uchar *b)
{
	int i;

	for(i = 0; i < 4; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

long
ipclassmask(uchar *ip)
{
	return nhgetl(classmask[CLASS(ip)]);
}
