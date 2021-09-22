/*
 * Ethernet bridge and IP tunnel (IPv4 only so far)
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../ip/ip.h"
#include "../ip/ipv6.h"
#include "../port/netif.h"
#include "etherif.h"
#include "../port/error.h"

typedef struct Bridge 	Bridge;
typedef struct Port 	Port;
typedef struct Centry	Centry;
typedef struct Ip4hdr	Ip4hdr;
typedef struct Ip4hinfo Ip4hinfo;
typedef struct Tcphdr	Tcphdr;

enum
{
	Qtopdir=	1,		/* top level directory */

	Qbridgedir,			/* bridge* directory */
	Qbctl,
	Qstats,
	Qcache,
	Qlog,

	Qportdir,			/* directory for a protocol */
	Qpctl,
	Qlocal,
	Qstatus,

	MaxQ,

	Maxbridge=	4,
	Maxport=	128,		/* power of 2 */
	CacheHash=	257,		/* prime */
	CacheLook=	5,		/* how many cache entries to examine */
	CacheSize=	(CacheHash+CacheLook-1),
	CacheTimeout=	5*60,		/* timeout for cache entry in seconds */

	TcpMssMax = 1300,		/* max desirable Tcp MSS value */
	TunnelMtu = 1400,

	/* ethernet packet types */
	ETARP		= 0x0806,
	ETIP4		= 0x0800,
	ETIP6		= 0x86DD,
};

static Dirtab bridgedirtab[]={
	"ctl",		{Qbctl},	0,	0666,
	"stats",	{Qstats},	0,	0444,
	"cache",	{Qcache},	0,	0444,
	"log",		{Qlog},		0,	0666,
};

static Dirtab portdirtab[]={
	"ctl",		{Qpctl},	0,	0666,
	"local",	{Qlocal},	0,	0444,
	"status",	{Qstatus},	0,	0444,
};

enum {
	Logcache=	(1<<0),
	Logmcast=	(1<<1),
};

/* types of interfaces */
enum
{
	Tether,
	Ttun,
	Tnone,
};

static Logflag logflags[] =
{
	{ "cache",	Logcache, },
	{ "multicast",	Logmcast, },
	{ nil,		0, },
};

static Dirtab	*dirtab[MaxQ];

#define TYPE(x) 	(((ulong)(x).path) & 0xff)
#define PORT(x) 	((((ulong)(x).path) >> 8) & (Maxport-1))
#define QID(x, y) 	(((x)<<8) | (y))

struct Centry
{
	uchar	d[Eaddrlen];
	int	port;
	ulong	expire;	/* entry expires this many seconds after bootime */
	long	src;	/* counts */
	long	dst;
};

struct Bridge
{
	QLock;
	int	nport;
	Port	*port[Maxport];
	Centry	cache[CacheSize];
	ulong	hit;
	ulong	miss;
	ulong	copy;
	long	delay0;		/* constant microsecond delay per packet */
	long	delayn;		/* microsecond delay per byte */
	int	tcpmss;		/* modify tcpmss value */

	Log;
};

struct Port
{
	int	id;
	Bridge	*bridge;
	int	ref;
	int	closed;

	Chan	*data[2];	/* channel to data */

	Proc	*readp;		/* read proc */

	/* the following uniquely identifies the port */
	int	type;
	char	name[KNAMELEN];

	/* owner hash - avoids bind/unbind races */
	ulong	ownhash;

	/* various stats */
	int	in;		/* number of packets read */
	int	inmulti;	/* multicast or broadcast */
	int	inunknown;	/* unknown address */
	int	out;		/* number of packets written */
	int	outmulti;	/* multicast or broadcast */
	int	outunknown;	/* unknown address */
	int	outfrag;	/* fragmented the packet */
	int	nentry;		/* number of cache entries for this port */
};

enum {
	IP_TCPPROTO	= 6,
	EOLOPT		= 0,
	NOOPOPT		= 1,
	MSSOPT		= 2,
	MSS_LENGTH	= 4,		/* Mean segment size */
	SYN		= 0x02,		/* Pkt. is synchronise */
};

/* only used to adjust MSS */
struct Tcphdr
{
	uchar	sport[2];
	uchar	dport[2];
	uchar	seq[4];
	uchar	ack[4];
	uchar	flag[2];
	uchar	win[2];
	uchar	cksum[2];
	uchar	urg[2];
};

struct Ip4hinfo {		/* stuff derived from an Ip4hdr */
	int	dlen;
	int	lid;
	ushort	frag;
	ushort	mf;
};

static Bridge bridgetab[Maxbridge];

static int m2p[] = {
	[OREAD]		4,
	[OWRITE]	2,
	[ORDWR]		6
};

static int	bridgegen(Chan *c, char*, Dirtab*, int, int s, Dir *dp);
static char	*cachedump(Bridge *b);
static void	cacheflushport(Bridge *b, int port);
static void	bretherread(void *a);
static void	etherwrite(Port *port, Block *bp);
static void	portbind(Bridge *b, int argc, char *argv[]);
static void	portfree(Port *port);
static void	portunbind(Bridge *b, int argc, char *argv[]);

static void
bridgeinit(void)
{
	int i;
	Dirtab *dt;

	// setup dirtab with non directory entries
	for(i=0; i<nelem(bridgedirtab); i++) {
		dt = bridgedirtab + i;
		dirtab[TYPE(dt->qid)] = dt;
	}
	for(i=0; i<nelem(portdirtab); i++) {
		dt = portdirtab + i;
		dirtab[TYPE(dt->qid)] = dt;
	}
}

static Chan*
bridgeattach(char* spec)
{
	Chan *c;
	uint dev;

	dev = atoi(spec);
	if(dev >= Maxbridge)
		error(Ebadspec);

	c = devattach('B', spec);
	mkqid(&c->qid, QID(0, Qtopdir), 0, QTDIR);
	c->devno = dev;
	return c;
}

static Walkqid*
bridgewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab*)0, 0, bridgegen);
}

static long
bridgestat(Chan* c, uchar* db, long n)
{
	return devstat(c, db, n, (Dirtab *)0, 0L, bridgegen);
}

static Chan*
bridgeopen(Chan* c, int omode)
{
	int perm;
	Bridge *b;

	omode &= 3;
	perm = m2p[omode];
	USED(perm);

	b = bridgetab + c->devno;
	switch(TYPE(c->qid)) {
	case Qlog:
		logopen(b);
		break;
	case Qcache:
		c->aux = cachedump(b);
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
bridgeclose(Chan* c)
{
	Bridge *b = bridgetab + c->devno;

	if(!(c->flag & COPEN))
		return;
	switch(TYPE(c->qid)) {
	case Qcache:
		free(c->aux);
		c->aux = nil;
		break;
	case Qlog:
		logclose(b);
		break;
	}
	c->flag &= ~COPEN;
}

static long
bridgeread(Chan *c, void *a, long n, vlong off)
{
	char buf[256];
	Bridge *b = bridgetab + c->devno;
	Port *port;
	int i, ingood, outgood;

	USED(off);
	switch(TYPE(c->qid)) {
	default:
		error(Eperm);
	case Qtopdir:
	case Qbridgedir:
	case Qportdir:
		return devdirread(c, a, n, 0, 0, bridgegen);
	case Qlog:
		return logread(b, a, off, n);
	case Qcache:
		return readstr(off, a, n, c->aux);
	case Qstatus:
		qlock(b);
		port = b->port[PORT(c->qid)];
		if(port == nil)
			strcpy(buf, "unbound\n");
		else {
			i = 0;
			switch(port->type) {
			default:
				panic("bridgeread: unknown port type: %d",
					port->type);
			case Tether:
				i += snprint(buf+i, sizeof buf-i, "ether %s: ",
					port->name);
				break;
			case Ttun:
				i += snprint(buf+i, sizeof buf-i, "tunnel %s: ",
					port->name);
				break;
			}
			ingood = port->in - port->inmulti - port->inunknown;
			outgood = port->out - port->outmulti - port->outunknown;
			i += snprint(buf+i, sizeof(buf)-i,
				"in=%d(%d:%d:%d) out=%d(%d:%d:%d:%d)\n",
				port->in, ingood, port->inmulti, port->inunknown,
				port->out, outgood, port->outmulti,
				port->outunknown, port->outfrag);
			USED(i);
		}
		n = readstr(off, a, n, buf);
		qunlock(b);
		return n;
	case Qbctl:
		snprint(buf, sizeof(buf), "%s tcpmss\ndelay %ld %ld\n",
			b->tcpmss ? "set" : "clear", b->delay0, b->delayn);
		break;
	case Qstats:
		snprint(buf, sizeof(buf), "hit=%uld miss=%uld copy=%uld\n",
			b->hit, b->miss, b->copy);
		break;
	}
	return readstr(off, a, n, buf);
}

static void
bridgeoption(Bridge *b, char *option, int value)
{
	if(strcmp(option, "tcpmss") == 0)
		b->tcpmss = value;
	else
		error("unknown bridge option");
}

static long
bridgewrite(Chan *c, void *a, long n, vlong off)
{
	Bridge *b = bridgetab + c->devno;
	Cmdbuf *cb;
	char *arg0, *p;

	USED(off);
	p = nil;
	switch(TYPE(c->qid)) {
	default:
		error(Eperm);
	case Qbctl:
		cb = parsecmd(a, n);
		qlock(b);
		if(waserror()) {
			qunlock(b);
			free(cb);
			nexterror();
		}
		if(cb->nf == 0)
			error("short write");
		arg0 = cb->f[0];
		if(strcmp(arg0, "bind") == 0) {
			portbind(b, cb->nf-1, cb->f+1);
		} else if(strcmp(arg0, "unbind") == 0) {
			portunbind(b, cb->nf-1, cb->f+1);
		} else if(strcmp(arg0, "cacheflush") == 0) {
			log(b, Logcache, "cache flush\n");
			memset(b->cache, 0, CacheSize*sizeof(Centry));
		} else if(strcmp(arg0, "set") == 0) {
			if(cb->nf != 2)
				error("usage: set option");
			bridgeoption(b, cb->f[1], 1);
		} else if(strcmp(arg0, "clear") == 0) {
			if(cb->nf != 2)
				error("usage: clear option");
			bridgeoption(b, cb->f[1], 0);
		} else if(strcmp(arg0, "delay") == 0) {
			if(cb->nf != 3)
				error("usage: delay delay0 delayn");
			b->delay0 = strtol(cb->f[1], nil, 10);
			b->delayn = strtol(cb->f[2], nil, 10);
		} else
			error("unknown control request");
		poperror();
		qunlock(b);
		break;
	case Qlog:
		cb = parsecmd(a, n);
		p = logctl(b, cb->nf, cb->f, logflags);
		break;
	}
	if(p != nil)
		error(p);
	free(cb);
	return n;
}

static int
bridgegen(Chan *c, char *, Dirtab*, int, int s, Dir *dp)
{
	Bridge *b = bridgetab + c->devno;
	int type = TYPE(c->qid);
	Dirtab *dt;
	Qid qid;

	if(s == DEVDOTDOT){
		switch(TYPE(c->qid)){
		case Qtopdir:
		case Qbridgedir:
			snprint(up->genbuf, sizeof(up->genbuf), "#B%d", c->devno);
			mkqid(&qid, Qtopdir, 0, QTDIR);
			break;
		case Qportdir:
			snprint(up->genbuf, sizeof up->genbuf, "bridge%d", c->devno);
			mkqid(&qid, Qbridgedir, 0, QTDIR);
			break;
		default:
			panic("bridgewalk %llux", c->qid.path);
		}
		devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
		return 1;
	}

	switch(type) {
	default:
		/* non-directory entries end up here */
		if(c->qid.type & QTDIR)
			panic("bridgegen: unexpected directory");
		if(s != 0)
			return -1;
		dt = dirtab[TYPE(c->qid)];
		if(dt == nil)
			panic("bridgegen: unknown type: %lud", TYPE(c->qid));
		devdir(c, c->qid, dt->name, dt->length, eve, dt->perm, dp);
		return 1;
	case Qtopdir:
		if(s != 0)
			return -1;
		snprint(up->genbuf, sizeof up->genbuf, "bridge%d", c->devno);
		mkqid(&qid, QID(0, Qbridgedir), 0, QTDIR);
		break;
	case Qbridgedir:
		if(s<nelem(bridgedirtab)) {
			dt = bridgedirtab+s;
			devdir(c, dt->qid, dt->name, dt->length, eve, dt->perm, dp);
			return 1;
		}
		s -= nelem(bridgedirtab);
		if((uint)s >= b->nport)
			return -1;
		mkqid(&qid, QID(s, Qportdir), 0, QTDIR);
		snprint(up->genbuf, sizeof(up->genbuf), "%d", s);
		break;
	case Qportdir:
		if((uint)s >= nelem(portdirtab))
			return -1;
		dt = portdirtab+s;
		mkqid(&qid, QID(PORT(c->qid),TYPE(dt->qid)), 0, QTFILE);
		devdir(c, qid, dt->name, dt->length, eve, dt->perm, dp);
		return 1;
	}
	devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
	return 1;
}

// parse mac address; also in netif.c
static int
parseaddr(uchar *to, char *from, int alen)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	for(i = 0; i < alen; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

static void
chanwrite(Chan *ctl, char *s)
{
	char buf[32];

	snprint(buf, sizeof buf, s);
	ctl->dev->write(ctl, buf, strlen(buf), 0);
}

static int
lookuptype(char *type)
{
	if(strcmp(type, "ether") == 0)
		return Tether;
	else if(strcmp(type, "tunnel") == 0)
		return Ttun;
	else
		return Tnone;
}

static Port *
lookupport(Bridge *b, char *name, int type, int *idxp)
{
	int i;
	Port *port;

	if (idxp)
		*idxp = -1;
	for (i = 0; i < b->nport; i++) {
		port = b->port[i];
		if (port != nil && port->type == type &&
		    memcmp(port->name, name, sizeof name) == 0) {
			if (idxp)
				*idxp = i;
			return port;
		}
	}
	return nil;
}

// assumes b is locked
static void
portbind(Bridge *b, int argc, char *argv[])
{
	Port *port;
	Chan *ctl;
	int type, i, n;
	ulong ownhash;
	char *dev, *dev2 = nil, *p;
	char buf[100], name[KNAMELEN], path[8*KNAMELEN];
	static char usage[] = "usage: bind ether|tunnel name ownhash dev [dev2]";

	if(argc < 4) {
		error(usage);
		notreached();
	}
	type = lookuptype(argv[0]);
	if (type == Tnone) {
		error(usage);
		notreached();
	}
	if(type == Tether) {
		if(argc != 4)
			error(usage);
	} else {
		if(argc != 5)
			error(usage);
		dev2 = argv[4];
	}

	strncpy(name, argv[1], sizeof name);
	name[sizeof name - 1] = 0;
	ownhash = atoi(argv[2]);
	dev = argv[3];

	port = lookupport(b, name, type, nil);
	if (port != nil)
		error("port in use on this bridge");

	/* find free port */
	for(i=0; i<Maxport; i++)
		if(b->port[i] == nil)
			break;
	if(i >= Maxport)
		error("no more ports on this bridge");

	port = smalloc(sizeof(Port));
	port->ref = 1;
	port->id = i;
	port->ownhash = ownhash;

	if(waserror()) {
		portfree(port);
		nexterror();
	}
	port->type = type;
	memmove(port->name, name, sizeof name);
	switch(port->type) {
	default:
		panic("portbind: unknown port type: %d", type);
	case Tether:
		snprint(path, sizeof(path), "%s/clone", dev);
		ctl = namec(path, Aopen, ORDWR, 0);
		if(waserror()) {
			cclose(ctl);
			nexterror();
		}
		// check addr?

		// get directory name
		n = ctl->dev->read(ctl, buf, sizeof(buf), 0);
		buf[n >= 0? n: 0] = 0;
		for(p = buf; *p == ' '; p++)
			;
		snprint(path, sizeof(path), "%s/%lud/data", dev,
			strtoul(p, 0, 0));

		// setup connection to be promiscuous
		chanwrite(ctl, "connect -1");
		chanwrite(ctl, "promiscuous");
		chanwrite(ctl, "bridge");

		// open data port
		port->data[0] = namec(path, Aopen, ORDWR, 0);
		// dup it
		incref(port->data[0]);
		port->data[1] = port->data[0];

		poperror();
		cclose(ctl);
		break;
	case Ttun:
		port->data[0] = namec(dev, Aopen, OREAD, 0);
		port->data[1] = namec(dev2, Aopen, OWRITE, 0);
		break;
	}

	poperror();				/* portfree(port) */

	/* committed to binding port */
	b->port[port->id] = port;
	port->bridge = b;
	if(b->nport <= port->id)
		b->nport = port->id+1;

	/* kproc specific to this port and type */
	kproc("bretherread", bretherread, port);	// poperror must be next
	port->ref++;
}

// assumes b is locked
static void
portunbind(Bridge *b, int argc, char *argv[])
{
	int type, i;
	char name[KNAMELEN];
	ulong ownhash;
	Port *port;
	static char usage[] = "usage: unbind ether|tunnel addr [ownhash]";

	if(argc < 2 || argc > 3) {
		error(usage);
		notreached();
	}
	type = lookuptype(argv[0]);
	if (type == Tnone) {
		error(usage);
		notreached();
	}
	strncpy(name, argv[1], sizeof name);
	name[sizeof name - 1] = 0;
	if(argc == 3)
		ownhash = atoi(argv[2]);
	else
		ownhash = 0;

	port = lookupport(b, name, type, &i);
	if (port == nil)
		error("port not found on this bridge");
	if(ownhash != 0 && port->ownhash != 0 && ownhash != port->ownhash)
		error("bad bridge port owner hash");

	port->closed = 1;
	b->port[i] = nil;	// port is now unbound
	cacheflushport(b, i);

	// try and stop reader
	if(port->readp)
		postnote(port->readp, 1, "unbind", 0);
	portfree(port);
}

static uint
cachehash(uchar *d, int len)
{
	int i;
	uint h;

	h = 0;
	for(i=0; i < len; i++) {
		h *= 7;
		h += d[i];
	}
	return h % CacheHash;
}

// assumes b is locked
static Centry *
cachelookup(Bridge *b, uchar d[Eaddrlen])
{
	int i;
	uint h;
	Centry *p;
	ulong sec;

	/* don't cache multicast or broadcast */
	if(d[0] & 1)
		return 0;

	h = cachehash(d, Eaddrlen);
	p = b->cache + h;
	sec = TK2SEC(sys->ticks);
	for(i=0; i<CacheLook; i++,p++)
		if(memcmp(d, p->d, Eaddrlen) == 0) {
			p->dst++;
			if(sec >= p->expire) {
				log(b, Logcache, "expired cache entry: %E %d\n",
					d, p->port);
				return nil;
			}
			p->expire = sec + CacheTimeout;
			return p;
		}
	log(b, Logcache, "cache miss: %E\n", d);
	return nil;
}

// assumes b is locked
static void
cacheupdate(Bridge *b, uchar d[Eaddrlen], int port)
{
	int i;
	uint h;
	Centry *p, *pp;
	ulong sec;

	/* don't cache multicast or broadcast */
	if(d[0] & 1) {
		log(b, Logcache, "bad source address: %E\n", d);
		return;
	}

	h = cachehash(d, Eaddrlen);
	pp = p = b->cache + h;
	sec = p->expire;

	// look for oldest entry
	for(i=0; i<CacheLook; i++,p++) {
		if(memcmp(p->d, d, Eaddrlen) == 0) {
			p->expire = TK2SEC(sys->ticks) + CacheTimeout;
			if(p->port != port) {
				log(b, Logcache, "NIC changed port %d->%d: %E\n",
					p->port, port, d);
				p->port = port;
			}
			p->src++;
			return;
		}
		if(p->expire < sec) {
			sec = p->expire;
			pp = p;
		}
	}
	if(pp->expire != 0)
		log(b, Logcache, "bumping from cache: %E %d\n", pp->d, pp->port);
	pp->expire = TK2SEC(sys->ticks) + CacheTimeout;
	memmove(pp->d, d, Eaddrlen);
	pp->port = port;
	pp->src = 1;
	pp->dst = 0;
	log(b, Logcache, "adding to cache: %E %d\n", pp->d, pp->port);
}

// assumes b is locked
static void
cacheflushport(Bridge *b, int port)
{
	Centry *ce;
	int i;

	ce = b->cache;
	for(i=0; i<CacheSize; i++, ce++)
		if(ce->port == port)
			memset(ce, 0, sizeof(Centry));
}

static char *
cachedump(Bridge *b)
{
	int i, n;
	long off;
	ulong sec;
	char *buf, *p, *ep;
	Centry *ce;
	char c;

	qlock(b);
	if(waserror()) {
		qunlock(b);
		nexterror();
	}
	sec = TK2SEC(sys->ticks);
	n = 0;
	for(i=0; i<CacheSize; i++)
		if(b->cache[i].expire != 0)
			n++;

	n *= 51;	// change if print format is changed
	n += 10;	// some slop at the end
	buf = malloc(n);
	if(buf == nil)
		error(Enomem);
	p = buf;
	ep = buf + n;
	ce = b->cache;
	off = seconds() - sec;
	for(i=0; i<CacheSize; i++,ce++) {
		if(ce->expire == 0)
			continue;
		c = (sec < ce->expire)?'v':'e';
		p += snprint(p, ep-p, "%E %2d %10ld %10ld %10ld %c\n", ce->d,
			ce->port, ce->src, ce->dst, ce->expire+off, c);
	}
	*p = 0;
	poperror();
	qunlock(b);
	return buf;
}

// assumes b is locked
static void
ethermultiwrite(Bridge *b, Block *bp, Port *port)
{
	Port *oport;
	Block *bp2;
	Etherpkt *ep;
	int i, mcast;

	if(waserror()) {
		freeb(bp);
		nexterror();
	}

	ep = (Etherpkt*)bp->rp;
	mcast = ep->d[0] & 1;		/* multicast bit of ethernet address */

	oport = nil;
	for(i=0; i<b->nport; i++) {
		if(i == port->id || b->port[i] == nil)
			continue;
		/*
		 * we need to forward multicast packets for ipv6,
		 * so always do it.
		 */
		if(mcast)
			b->port[i]->outmulti++;
		else
			b->port[i]->outunknown++;

		// delay one so that the last write does not copy
		if(oport != nil) {
			b->copy++;
			bp2 = copyblock(bp, blocklen(bp));
			if(!waserror()) {
				etherwrite(oport, bp2);
				poperror();
			}
		}
		oport = b->port[i];
	}

	// last write free block
	if(oport) {
		bp2 = bp; bp = nil; USED(bp);
		if(!waserror()) {
			etherwrite(oport, bp2);
			poperror();
		}
	} else
		freeb(bp);

	poperror();
}

static void
tcpmsshack(Etherpkt *epkt, int n)
{
	int hl, optlen;
	Ip4hdr *ip4hdr;
	Ip6hdr *ip6hdr;
	Tcphdr *tcphdr;
	ulong mss, cksum;
	uchar *optr, *iphdr;

	/* validate packet; return if too short, wrong ip version or not tcp */
	n -= ETHERHDRSIZE;
	iphdr = epkt->data;
	switch(nhgets(epkt->type)){
	default:
		return;
	case ETIP4:
		ip4hdr = (Ip4hdr*)epkt->data;
		if(n < IP4HDR)
			return;

		if(ip4hdr->vihl != (IP_VER4|IP_HLEN4)) {
			/* presumably this is the uncommon case */
			hl = (ip4hdr->vihl&0xF)<<2;
			if((ip4hdr->vihl&0xF0) != IP_VER4 || hl < (IP_HLEN4<<2))
				return;
		} else
			hl = IP_HLEN4<<2;

		if(ip4hdr->proto != IP_TCPPROTO)
			return;
		break;
	case ETIP6:
		ip6hdr = (Ip6hdr*)epkt->data;
		if(n < IP6HDR || (ip6hdr->vcf[0]&0xF0) != IP_VER6 ||
		    ip6hdr->proto != IP_TCPPROTO)
			return;
		hl = IP6HDR;
		break;
	}
	n -= hl;
	if(n < sizeof(Tcphdr))
		return;
	tcphdr = (Tcphdr*)(iphdr + hl);

	// MSS can only appear in SYN packet
	if(!(tcphdr->flag[1] & SYN))
		return;
	hl = (tcphdr->flag[0] & 0xf0)>>2;
	if(n < hl)
		return;

	// check for MSS option
	optr = (uchar*)tcphdr + sizeof(Tcphdr);
	n = hl - sizeof(Tcphdr);
	for(;;) {
		if(n <= 0 || *optr == EOLOPT)
			return;
		if(*optr == NOOPOPT) {
			n--;
			optr++;
			continue;
		}
		optlen = optr[1];
		if(optlen < 2 || optlen > n)
			return;
		if(*optr == MSSOPT && optlen == MSS_LENGTH)
			break;
		n -= optlen;
		optr += optlen;
	}

	mss = nhgets(optr+2);
	if(mss <= TcpMssMax)
		return;
	// fit checksum
	cksum = nhgets(tcphdr->cksum);
	if((optr - (uchar*)tcphdr) & 1) {
print("tcpmsshack: odd alignment!\n");
		// odd alignments are a pain
		cksum += nhgets(optr+1);
		cksum -= (optr[1]<<8)|(TcpMssMax>>8);
		cksum += (cksum>>16);
		cksum &= 0xffff;
		cksum += nhgets(optr+3);
		cksum -= ((TcpMssMax&0xff)<<8)|optr[4];
		cksum += (cksum>>16);
	} else {
		cksum += mss;
		cksum -= TcpMssMax;
		cksum += (cksum>>16);
	}
	hnputs(tcphdr->cksum, cksum);
	hnputs(optr+2, TcpMssMax);
}

/*
 *  process to read from the ethernet
 */
static void
bretherread(void *a)
{
	Port *port = a;
	Bridge *b = port->bridge;
	Block *bp, *bp2;
	Etherpkt *ep;
	Centry *ce;
	Chan *pdc;
	long md;

	qlock(b);
	port->readp = up;	/* hide identity under a rock for unbind */
	pdc = port->data[0];
	while(!port->closed){
		// release lock to read - error means it is time to quit
		qunlock(b);

		if(waserror()) {
			print("bretherread read error: %s\n", up->errstr);
			qlock(b);
			break;
		}
		if(0)
			print("devbridge: bretherread: reading\n");
		/* we'd need to add some slop to ETHERMAXTU to support VLANs */
		bp = pdc->dev->bread(pdc, ETHERMAXTU, 0);
		if(0)
			print("devbridge: bretherread: blocklen = %d\n",
				blocklen(bp));
		poperror();

		qlock(b);
		if(bp == nil || port->closed)
			break;
		if(waserror()) {
//			print("bretherread bridge error\n");
			freeb(bp);
			continue;
		}
		if(blocklen(bp) < ETHERMINTU)
			error("short packet");
		port->in++;

		ep = (Etherpkt*)bp->rp;
		cacheupdate(b, ep->s, port->id);
		if(b->tcpmss)
			tcpmsshack(ep, BLEN(bp));

		/*
		 * delay packets to simulate a slow link
		 */
		if(b->delay0 || b->delayn){
			md = b->delay0 + b->delayn * BLEN(bp);
			if(md > 0)
				microdelay(md);
		}

		if(ep->d[0] & 1) {
			log(b, Logmcast,
				"multicast: port=%d src=%E dst=%E type=%#.4ux\n",
				port->id, ep->s, ep->d, ep->type[0]<<8|ep->type[1]);
			port->inmulti++;
			bp2 = bp; bp = nil;
			ethermultiwrite(b, bp2, port);
		} else {
			ce = cachelookup(b, ep->d);
			if(ce == nil) {
				b->miss++;
				port->inunknown++;
				bp2 = bp; bp = nil;
				ethermultiwrite(b, bp2, port);
			}else if(ce->port != port->id){
				b->hit++;
				bp2 = bp; bp = nil;
				etherwrite(b->port[ce->port], bp2);
			}
		}

		poperror();
		freeb(bp);
	}
	port->readp = nil;
	portfree(port);
	qunlock(b);
	pexit("hangup", 1);
}

static int
fragment(Etherpkt *epkt, int n)
{
	Ip4hdr *ip4hdr;
#ifdef notyet			/* IPv6 */
	Ip6hdr *ip6hdr;
#endif

	if(n <= TunnelMtu)
		return 0;

	/*
	 * validate packet.  ignore: IP runt packets, bad packets (I
	 * don't handle IP options for the moment), packets with
	 * don't-fragment set, and short blocks.
	 */
	n -= ETHERHDRSIZE;
	switch(nhgets(epkt->type)){
	default:
		return 0;
	case ETIP4:
		ip4hdr = (Ip4hdr*)epkt->data;
		if(n < IP4HDR || ip4hdr->vihl != (IP_VER4|IP_HLEN4) ||
		    ip4hdr->frag[0] & (IP_DF>>8) || nhgets(ip4hdr->length) > n)
			return 0;
		break;
	case ETIP6:
#ifdef notyet			/* IPv6 */
		ip6hdr = (Ip6hdr*)epkt->data;
		if(n < IP6HDR || ip6hdr->vcf[0] != IP_VER6 ||
		    nhgets(ip6hdr->ploadlen) > n)
			return 0;
#endif
		break;
	}
	return 1;
}

static Ip4hdr *
clonev4hdr(Block *nb, Etherpkt *epkt)
{
	Ip4hdr *feh;

	feh = (Ip4hdr*)(nb->wp+ETHERHDRSIZE);
	memmove(nb->wp, epkt, ETHERHDRSIZE+IP4HDR);
	nb->wp += ETHERHDRSIZE+IP4HDR;
	return feh;
}

static int
setv4hdr(Ip4hdr *feh, ushort fragoff, int seglen, Ip4hinfo *hinfo)
{
	ushort mf;

	mf = hinfo->mf;
	if(fragoff + seglen >= hinfo->dlen)	/* last fragment to emit? */
		seglen = hinfo->dlen - fragoff;
	else
		mf = IP_MF;
	hnputs(feh->frag, (hinfo->frag + fragoff)>>3 | mf);
	hnputs(feh->length, seglen + IP4HDR);
	hnputs(feh->id, hinfo->lid);
	return seglen;
}

static Block *
copyfrag(Block *nb, Block *xp, int seglen)
{
	int chunk, blklen;

	for(chunk = seglen; chunk; chunk -= blklen) {
		blklen = chunk;
		if(BLEN(xp) < chunk)
			blklen = BLEN(xp);

		memmove(nb->wp, xp->rp, blklen);
		nb->wp += blklen;
		xp->rp += blklen;

		if(xp->rp == xp->wp)
			xp = xp->next;
	}
	return xp;
}

static void
addv4cksum(Ip4hdr *feh)
{
	feh->cksum[0] = 0;
	feh->cksum[1] = 0;
	hnputs(feh->cksum, ipcsum(&feh->vihl));
}

static void
extendrunt(Block *nb)
{
	/* don't generate small packets */
	if(BLEN(nb) < ETHERMINTU)
		nb->wp = nb->rp + ETHERMINTU;
}

static void
etherwrite(Port *port, Block *bp)
{
	Ip4hdr *eh, *feh;
	Ip4hinfo h4info;
	Etherpkt *epkt;
	int n, len, seglen, offset;
	Block *xp, *nb;
	ushort fragoff;

	port->out++;
	epkt = (Etherpkt*)bp->rp;
	n = blocklen(bp);
	if(port->type != Ttun || !fragment(epkt, n)) {
		/*
		 * not tunnelled or doesn't need fragmentation,
		 * just write it and go home.
		 */
		port->data[1]->dev->bwrite(port->data[1], bp, 0);
		return;
	}

	/* tunnelled and needs fragmentation */
	port->outfrag++;
	if(waserror()){
		freeblist(bp);
		nexterror();
	}

	switch(nhgets(epkt->type)){
	default:
		error("devbridge: etherwrite cannot fragment non-IPv4 packet");
	case ETIP4:
		break;
	}

	/*
	 * ipv4-specific tunnelling and fragmentation.
	 * assumes both tunnel and contents are ipv4.
	 */
	seglen = (TunnelMtu - ETHERHDRSIZE - IP4HDR) & ~7; /* max output payload */
	eh = (Ip4hdr*)(epkt->data);
	len = nhgets(eh->length);

	h4info.frag = nhgets(eh->frag);
	h4info.mf = h4info.frag & IP_MF;
	h4info.frag <<= 3;
	h4info.dlen = len - IP4HDR;
	h4info.lid = nhgets(eh->id);

	/*
	 * start fragmenting ipv4 packet at xp->rp,
	 * after bp->rp's ether & ipv4 hdrs.
	 * should pullupblock(bp, 64) first?
	 */
	offset = ETHERHDRSIZE+IP4HDR;
	for (xp = bp; xp != nil && offset && offset >= BLEN(xp); xp = xp->next)
		offset -= BLEN(xp);
	xp->rp += offset;

	if(0)
		print("seglen=%d, dlen=%d, mf=%x, frag=%d\n",
			seglen, h4info.dlen, h4info.mf, h4info.frag);
	for(fragoff = 0; fragoff < h4info.dlen; fragoff += seglen) {
		/* copy bp->rp's ether & ip4 hdrs into new packet */
		nb = allocb(ETHERHDRSIZE+IP4HDR+seglen);
		feh = clonev4hdr(nb, epkt);
		seglen = setv4hdr(feh, fragoff, seglen, &h4info);
		xp = copyfrag(nb, xp, seglen);
		addv4cksum(feh);
		extendrunt(nb);

		port->data[1]->dev->bwrite(port->data[1], nb, 0);
	}
	poperror();
	freeblist(bp);
}

// caller holds port->bridge lock
static void
portfree(Port *port)
{
	port->ref--;
	if(port->ref < 0)
		panic("portfree: bad ref");
	if(port->ref > 0)
		return;

	if(port->data[0])
		cclose(port->data[0]);
	if(port->data[1])
		cclose(port->data[1]);
	memset(port, 0, sizeof(Port));
	free(port);
}

Dev bridgedevtab = {
	'B',
	"bridge",

	devreset,
	bridgeinit,
	devshutdown,
	bridgeattach,
	bridgewalk,
	bridgestat,
	bridgeopen,
	devcreate,
	bridgeclose,
	bridgeread,
	devbread,
	bridgewrite,
	devbwrite,
	devremove,
	devwstat,
};
