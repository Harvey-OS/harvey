/*
 * domain name resolvers, see rfcs 1035 and 1123
 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "dns.h"

typedef struct Dest Dest;
typedef struct Ipaddr Ipaddr;
typedef struct Query Query;
typedef struct Sluggards Sluggards;

enum
{
	Udp, Tcp,
	Maxdest=	24,	/* maximum destinations for a request message */
	Maxtrans=	3,	/* maximum transmissions to a server */
	Destmagic=	0xcafebabe,
	Querymagic=	0xdeadbeef,
};

struct Ipaddr {
	Ipaddr *next;
	uchar	ip[IPaddrlen];
};

struct Dest
{
	uchar	a[IPaddrlen];	/* ip address */
	DN	*s;		/* name server */
	int	nx;		/* number of transmissions */
	int	code;		/* response code; used to clear dp->respcode */

	ulong	magic;
};

struct Query {
	DN	*dp;		/* domain */
	int	type;		/* and type to look up */
	Request *req;
	RR	*nsrp;		/* name servers to consult */

	Dest	*dest;		/* array of destinations */
	Dest	*curdest;	/* pointer to one of them */
	int	ndest;

	int	udpfd;		/* can be shared by all udp users */

	QLock	tcplock;	/* only one tcp call at a time per query */
	int	tcpset;
	int	tcpfd;		/* if Tcp, read replies from here */
	int	tcpctlfd;
	uchar	tcpip[IPaddrlen];

	ulong	magic;
};

/* a list of sluggardly name servers */
struct Sluggards {
	QLock;
	Ipaddr *head;
	Ipaddr *tail;
};

static Sluggards slugs;

static RR*	dnresolve1(char*, int, int, Request*, int, int);
static int	netquery(Query *, int);

static Ipaddr *
newslug(void)
{
	return emalloc(sizeof(Ipaddr));
}

static void
addslug(uchar nsip[])
{
	Ipaddr *sp;
	static uchar zip[IPaddrlen];

	if (memcmp(nsip, zip, IPaddrlen) == 0)
		return;

	qlock(&slugs);
	for (sp = slugs.head; sp != nil; sp = sp->next)
		if (memcmp(sp->ip, nsip, IPaddrlen) == 0) {
			qunlock(&slugs);		/* already know it */
			return;
		}

	if (slugs.head == nil)
		slugs.head = slugs.tail = newslug();
	else {
		slugs.tail->next = newslug();
		slugs.tail = slugs.tail->next;
	}
	memmove(slugs.tail->ip, nsip, IPaddrlen);
	qunlock(&slugs);

	dnslog("%I is a slug", nsip);
}

int
isaslug(uchar nsip[])
{
	Ipaddr *sp;

	qlock(&slugs);
	for (sp = slugs.head; sp != nil; sp = sp->next)
		if (memcmp(sp->ip, nsip, IPaddrlen) == 0) {
			qunlock(&slugs);
			return 1;
		}
	qunlock(&slugs);
	return 0;
}

/*
 * reading /proc/pid/args yields either "name" or "name [display args]",
 * so return only display args, if any.
 */
static char *
procgetname(void)
{
	int fd, n;
	char *lp, *rp;
	char buf[256];

	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OREAD)) < 0)
		return strdup("");
	*buf = '\0';
	n = read(fd, buf, sizeof buf-1);
	close(fd);
	if (n >= 0)
		buf[n] = '\0';
	if ((lp = strchr(buf, '[')) == nil ||
	    (rp = strrchr(buf, ']')) == nil)
		return strdup("");
	*rp = '\0';
	return strdup(lp+1);
}

/*
 *  lookup 'type' info for domain name 'name'.  If it doesn't exist, try
 *  looking it up as a canonical name.
 */
RR*
dnresolve(char *name, int class, int type, Request *req, RR **cn, int depth,
	int recurse, int rooted, int *status)
{
	RR *rp, *nrp, *drp;
	DN *dp;
	int loops;
	char *procname;
	char nname[Domlen];

	if(status)
		*status = 0;

	procname = procgetname();
	/*
	 *  hack for systems that don't have resolve search
	 *  lists.  Just look up the simple name in the database.
	 */
	if(!rooted && strchr(name, '.') == 0){
		rp = nil;
		drp = domainlist(class);
		for(nrp = drp; nrp != nil; nrp = nrp->next){
			snprint(nname, sizeof nname, "%s.%s", name,
				nrp->ptr->name);
			rp = dnresolve(nname, class, type, req, cn, depth,
				recurse, rooted, status);
			rrfreelist(rrremneg(&rp));
			if(rp != nil)
				break;
		}
		if(drp != nil)
			rrfree(drp);
		procsetname(procname);
		free(procname);
		return rp;
	}

	/*
	 *  try the name directly
	 */
	rp = dnresolve1(name, class, type, req, depth, recurse);
	if(rp) {
		procsetname(procname);
		free(procname);
		return randomize(rp);
	}

	/* try it as a canonical name if we weren't told the name didn't exist */
	dp = dnlookup(name, class, 0);
	if(type != Tptr && dp->respcode != Rname)
		for(loops = 0; rp == nil && loops < 32; loops++){
			rp = dnresolve1(name, class, Tcname, req, depth, recurse);
			if(rp == nil)
				break;

			if(rp->negative){
				rrfreelist(rp);
				rp = nil;
				break;
			}

			name = rp->host->name;
			if(cn)
				rrcat(cn, rp);
			else
				rrfreelist(rp);

			rp = dnresolve1(name, class, type, req, depth, recurse);
		}

	/* distinction between not found and not good */
	if(rp == nil && status != nil && dp->respcode != 0)
		*status = dp->respcode;

	procsetname(procname);
	free(procname);
	return randomize(rp);
}

static void
queryinit(Query *qp, DN *dp, int type, Request *req)
{
	memset(qp, 0, sizeof *qp);
	qp->udpfd = qp->tcpfd = qp->tcpctlfd = -1;
	qp->dp = dp;
	qp->type = type;
	qp->req = req;
	qp->nsrp = nil;
	qp->dest = qp->curdest = nil;
	qp->magic = Querymagic;
}

static void
queryck(Query *qp)
{
	assert(qp);
	assert(qp->magic == Querymagic);
}

static void
querydestroy(Query *qp)
{
	queryck(qp);
	if (qp->udpfd > 0)
		close(qp->udpfd);
	if (qp->tcpfd > 0)
		close(qp->tcpfd);
	if (qp->tcpctlfd > 0) {
		hangup(qp->tcpctlfd);
		close(qp->tcpctlfd);
	}
	memset(qp, 0, sizeof *qp);	/* prevent accidents */
	qp->udpfd = qp->tcpfd = qp->tcpctlfd = -1;
}

static void
destinit(Dest *p)
{
	memset(p, 0, sizeof *p);
	p->magic = Destmagic;
}

static void
destck(Dest *p)
{
	assert(p);
	assert(p->magic == Destmagic);
}

static RR*
dnresolve1(char *name, int class, int type, Request *req, int depth,
	int recurse)
{
	DN *dp, *nsdp;
	RR *rp, *nsrp, *dbnsrp;
	char *cp;
	Query query;

	if(debug)
		dnslog("[%d] dnresolve1 %s %d %d", getpid(), name, type, class);

	/* only class Cin implemented so far */
	if(class != Cin)
		return nil;

	dp = dnlookup(name, class, 1);

	/*
	 *  Try the cache first
	 */
	rp = rrlookup(dp, type, OKneg);
	if(rp)
		if(rp->db){
			/* unauthoritative db entries are hints */
			if(rp->auth)
				return rp;
		} else
			/* cached entry must still be valid */
			if(rp->ttl > now)
				/* but Tall entries are special */
				if(type != Tall || rp->query == Tall)
					return rp;

	rrfreelist(rp);

	/*
	 * try the cache for a canonical name. if found punt
	 * since we'll find it during the canonical name search
	 * in dnresolve().
	 */
	if(type != Tcname){
		rp = rrlookup(dp, Tcname, NOneg);
		rrfreelist(rp);
		if(rp)
			return nil;
	}

	queryinit(&query, dp, type, req);

	/*
	 *  if we're running as just a resolver, query our
	 *  designated name servers
	 */
	if(cfg.resolver){
		nsrp = randomize(getdnsservers(class));
		if(nsrp != nil) {
			query.nsrp = nsrp;
			if(netquery(&query, depth+1)){
				rrfreelist(nsrp);
				querydestroy(&query);
				return rrlookup(dp, type, OKneg);
			}
			rrfreelist(nsrp);
		}
	}

	/*
 	 *  walk up the domain name looking for
	 *  a name server for the domain.
	 */
	for(cp = name; cp; cp = walkup(cp)){
		/*
		 *  if this is a local (served by us) domain,
		 *  return answer
		 */
		dbnsrp = randomize(dblookup(cp, class, Tns, 0, 0));
		if(dbnsrp && dbnsrp->local){
			rp = dblookup(name, class, type, 1, dbnsrp->ttl);
			rrfreelist(dbnsrp);
			querydestroy(&query);
			return rp;
		}

		/*
		 *  if recursion isn't set, just accept local
		 *  entries
		 */
		if(recurse == Dontrecurse){
			if(dbnsrp)
				rrfreelist(dbnsrp);
			continue;
		}

		/* look for ns in cache */
		nsdp = dnlookup(cp, class, 0);
		nsrp = nil;
		if(nsdp)
			nsrp = randomize(rrlookup(nsdp, Tns, NOneg));

		/* if the entry timed out, ignore it */
		if(nsrp && nsrp->ttl < now){
			rrfreelist(nsrp);
			nsrp = nil;
		}

		if(nsrp){
			rrfreelist(dbnsrp);

			/* query the name servers found in cache */
			query.nsrp = nsrp;
			if(netquery(&query, depth+1)){
				rrfreelist(nsrp);
				querydestroy(&query);
				return rrlookup(dp, type, OKneg);
			}
			rrfreelist(nsrp);
			continue;
		}

		/* use ns from db */
		if(dbnsrp){
			/* try the name servers found in db */
			query.nsrp = dbnsrp;
			if(netquery(&query, depth+1)){
				/* we got an answer */
				rrfreelist(dbnsrp);
				querydestroy(&query);
				return rrlookup(dp, type, NOneg);
			}
			rrfreelist(dbnsrp);
		}
	}
	querydestroy(&query);

	/* settle for a non-authoritative answer */
	rp = rrlookup(dp, type, OKneg);
	if(rp)
		return rp;

	/* noone answered.  try the database, we might have a chance. */
	return dblookup(name, class, type, 0, 0);
}

/*
 *  walk a domain name one element to the right.
 *  return a pointer to that element.
 *  in other words, return a pointer to the parent domain name.
 */
char*
walkup(char *name)
{
	char *cp;

	cp = strchr(name, '.');
	if(cp)
		return cp+1;
	else if(*name)
		return "";
	else
		return 0;
}

/*
 *  Get a udpport for requests and replies.  Put the port
 *  into "headers" mode.
 */
static char *hmsg = "headers";

int
udpport(char *mtpt)
{
	int fd, ctl;
	char ds[64], adir[64];

	/* get a udp port */
	snprint(ds, sizeof ds, "%s/udp!*!0", (mtpt? mtpt: "/net"));
	ctl = announce(ds, adir);
	if(ctl < 0){
		/* warning("can't get udp port"); */
		return -1;
	}

	/* turn on header style interface */
	if(write(ctl, hmsg, strlen(hmsg)) , 0){
		close(ctl);
		warning(hmsg);
		return -1;
	}

	/* grab the data file */
	snprint(ds, sizeof ds, "%s/data", adir);
	fd = open(ds, ORDWR);
	close(ctl);
	if(fd < 0)
		warning("can't open udp port %s: %r", ds);
	return fd;
}

/* generate a DNS UDP query packet */
int
mkreq(DN *dp, int type, uchar *buf, int flags, ushort reqno)
{
	DNSmsg m;
	int len;
	Udphdr *uh = (Udphdr*)buf;

	/* stuff port number into output buffer */
	memset(uh, 0, sizeof *uh);
	hnputs(uh->rport, 53);

	/* make request and convert it to output format */
	memset(&m, 0, sizeof m);
	m.flags = flags;
	m.id = reqno;
	m.qd = rralloc(type);
	m.qd->owner = dp;
	m.qd->type = type;
	len = convDNS2M(&m, &buf[Udphdrsize], Maxudp);
	rrfree(m.qd);
	return len;
}

/* for alarms in readreply */
static void
ding(void *x, char *msg)
{
	USED(x);
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	else
		noted(NDFLT);
}

static void
freeanswers(DNSmsg *mp)
{
	rrfreelist(mp->qd);
	rrfreelist(mp->an);
	rrfreelist(mp->ns);
	rrfreelist(mp->ar);
	mp->qd = mp->an = mp->ns = mp->ar = nil;
}

/* sets srcip */
static int
readnet(Query *qp, int medium, uchar *ibuf, ulong endtime, uchar **replyp,
	uchar *srcip)
{
	int len, fd;
	uchar *reply;
	uchar lenbuf[2];

	/* timed read of reply */
	alarm((endtime - time(nil)) * 1000);
	reply = ibuf;
	len = -1;			/* pessimism */
	memset(srcip, 0, IPaddrlen);
	if (medium == Udp) {
		if (qp->udpfd <= 0) 
			dnslog("readnet: qp->udpfd closed");
		else {
			len = read(qp->udpfd, ibuf, Udphdrsize+Maxudpin);
			if (len >= IPaddrlen)
				memmove(srcip, ibuf, IPaddrlen);
			if (len >= Udphdrsize) {
				len   -= Udphdrsize;
				reply += Udphdrsize;
			}
		}
	} else {
		if (!qp->tcpset)
			dnslog("readnet: tcp params not set");
		fd = qp->tcpfd;
		if (fd <= 0)
			dnslog("readnet: %s: tcp fd unset for dest %I",
				qp->dp->name, qp->tcpip);
		else if (readn(fd, lenbuf, 2) != 2) {
			dnslog("readnet: short read of tcp size from %I",
				qp->tcpip);
			/*
			 * probably a time-out; demote the ns.
			 * actually, the problem may be the query, not the ns.
			 */
			addslug(qp->tcpip);
		} else {
			len = lenbuf[0]<<8 | lenbuf[1];
			if (readn(fd, ibuf, len) != len) {
				dnslog("readnet: short read of tcp data from %I",
					qp->tcpip);
				/* probably a time-out; demote the ns */
				addslug(qp->tcpip);
				len = -1;
			}
		}
		memmove(srcip, qp->tcpip, IPaddrlen);
	}
	alarm(0);
	*replyp = reply;
	return len;
}

/*
 *  read replies to a request and remember the rrs in the answer(s).
 *  ignore any of the wrong type.
 *  wait at most until endtime.
 */
static int
readreply(Query *qp, int medium, ushort req, uchar *ibuf, DNSmsg *mp,
	ulong endtime)
{
	int len = -1, rv;
	char *err;
	uchar *reply;
	uchar srcip[IPaddrlen];
	RR *rp;

	notify(ding);

	queryck(qp);
	rv = 0;
	memset(mp, 0, sizeof *mp);
	if (time(nil) >= endtime)
		return -1;		/* timed out before we started */

	for (; time(nil) < endtime &&
	    (len = readnet(qp, medium, ibuf, endtime, &reply, srcip)) >= 0;
	    freeanswers(mp)){
		/* convert into internal format  */
		memset(mp, 0, sizeof *mp);
		err = convM2DNS(reply, len, mp, nil);
		if (mp->flags & Ftrunc) {
//			dnslog("readreply: %s: truncated reply, len %d from %I",
//				qp->dp->name, len, srcip);
			/* notify the caller to retry the query via tcp. */
			return -1;
		} else if(err){
			dnslog("readreply: %s: input err, len %d: %s: %I",
				qp->dp->name, len, err, srcip);
			free(err);
			continue;
		}
		if (err)
			free(err);
		if(debug)
			logreply(qp->req->id, srcip, mp);

		/* answering the right question? */
		if(mp->id != req)
			dnslog("%d: id %d instead of %d: %I", qp->req->id,
				mp->id, req, srcip);
		else if(mp->qd == 0)
			dnslog("%d: no question RR: %I", qp->req->id, srcip);
		else if(mp->qd->owner != qp->dp)
			dnslog("%d: owner %s instead of %s: %I", qp->req->id,
				mp->qd->owner->name, qp->dp->name, srcip);
		else if(mp->qd->type != qp->type)
			dnslog("%d: qp->type %d instead of %d: %I",
				qp->req->id, mp->qd->type, qp->type, srcip);
		else {
			/* remember what request this is in answer to */
			for(rp = mp->an; rp; rp = rp->next)
				rp->query = qp->type;
			return rv;
		}
	}
	if (time(nil) >= endtime)
		addslug(srcip);
	else
		dnslog("readreply: %s: %I read error or eof (returned %d)",
			qp->dp->name, srcip, len);
	return -1;
}

/*
 *	return non-0 if first list includes second list
 */
int
contains(RR *rp1, RR *rp2)
{
	RR *trp1, *trp2;

	for(trp2 = rp2; trp2; trp2 = trp2->next){
		for(trp1 = rp1; trp1; trp1 = trp1->next)
			if(trp1->type == trp2->type)
			if(trp1->host == trp2->host)
			if(trp1->owner == trp2->owner)
				break;
		if(trp1 == nil)
			return 0;
	}
	return 1;
}


/*
 *  return multicast version if any
 */
int
ipisbm(uchar *ip)
{
	if(isv4(ip)){
		if (ip[IPv4off] >= 0xe0 && ip[IPv4off] < 0xf0 ||
		    ipcmp(ip, IPv4bcast) == 0)
			return 4;
	} else
		if(ip[0] == 0xff)
			return 6;
	return 0;
}

/*
 *  Get next server address
 */
static int
serveraddrs(Query *qp, int nd, int depth)
{
	RR *rp, *arp, *trp;
	Dest *cur;

	if(nd >= Maxdest)
		return 0;

	/*
	 *  look for a server whose address we already know.
	 *  if we find one, mark it so we ignore this on
	 *  subsequent passes.
	 */
	arp = 0;
	for(rp = qp->nsrp; rp; rp = rp->next){
		assert(rp->magic == RRmagic);
		if(rp->marker)
			continue;
		arp = rrlookup(rp->host, Ta, NOneg);
		if(arp){
			rp->marker = 1;
			break;
		}
		arp = dblookup(rp->host->name, Cin, Ta, 0, 0);
		if(arp){
			rp->marker = 1;
			break;
		}
	}

	/*
	 *  if the cache and database lookup didn't find any new
	 *  server addresses, try resolving one via the network.
	 *  Mark any we try to resolve so we don't try a second time.
	 */
	if(arp == 0)
		for(rp = qp->nsrp; rp; rp = rp->next){
			if(rp->marker)
				continue;
			rp->marker = 1;

			/*
			 *  avoid loops looking up a server under itself
			 */
			if(subsume(rp->owner->name, rp->host->name))
				continue;

			arp = dnresolve(rp->host->name, Cin, Ta, qp->req, 0,
				depth+1, Recurse, 1, 0);
			rrfreelist(rrremneg(&arp));
			if(arp)
				break;
		}

	/* use any addresses that we found */
	for(trp = arp; trp && nd < Maxdest; trp = trp->next){
		cur = &qp->dest[nd];
		parseip(cur->a, trp->ip->name);
		/*
		 * straddling servers can reject all nameservers if they are all
		 * inside, so be sure to list at least one outside ns at
		 * the end of the ns list in /lib/ndb for `dom='.
		 */
		if (ipisbm(cur->a) ||
		    cfg.straddle && !insideaddr(qp->dp->name) && insidens(cur->a))
			continue;
		cur->nx = 0;
		cur->s = trp->owner;
		cur->code = Rtimeout;
		nd++;
	}
	rrfreelist(arp);
	return nd;
}

/*
 *  cache negative responses
 */
static void
cacheneg(DN *dp, int type, int rcode, RR *soarr)
{
	RR *rp;
	DN *soaowner;
	ulong ttl;

	/* no cache time specified, don't make anything up */
	if(soarr != nil){
		if(soarr->next != nil){
			rrfreelist(soarr->next);
			soarr->next = nil;
		}
		soaowner = soarr->owner;
	} else
		soaowner = nil;

	/* the attach can cause soarr to be freed so mine it now */
	if(soarr != nil && soarr->soa != nil)
		ttl = soarr->soa->minttl+now;
	else
		ttl = 5*Min;

	/* add soa and negative RR to the database */
	rrattach(soarr, 1);

	rp = rralloc(type);
	rp->owner = dp;
	rp->negative = 1;
	rp->negsoaowner = soaowner;
	rp->negrcode = rcode;
	rp->ttl = ttl;
	rrattach(rp, 1);
}

static int
setdestoutns(Dest *p, int n)
{
	uchar *outns = outsidens(n);

	destck(p);
	destinit(p);
	if (outns == nil) {
		if (n == 0)
			dnslog("[%d] no outside-ns in ndb", getpid());
		return -1;
	}
	memmove(p->a, outns, sizeof p->a);
	p->s = dnlookup("outside-ns-ips", Cin, 1);
	return 0;
}

/*
 * issue query via UDP or TCP as appropriate.
 * for TCP, returns with qp->tcpip set from udppkt header.
 */
static int
mydnsquery(Query *qp, int medium, uchar *udppkt, int len)
{
	int rv = -1;
	char *domain;
	char conndir[40];
	NetConnInfo *nci;

	queryck(qp);
	switch (medium) {
	case Udp:
		if (qp->udpfd <= 0)
			dnslog("mydnsquery: qp->udpfd closed");
		else {
			if (write(qp->udpfd, udppkt, len+Udphdrsize) !=
			    len+Udphdrsize)
				warning("sending udp msg %r");
			rv = 0;
		}
		break;
	case Tcp:
		/* send via TCP & keep fd around for reply */
		domain = smprint("%I", udppkt);
		alarm(10*1000);
		qp->tcpfd = rv = dial(netmkaddr(domain, "tcp", "dns"), nil,
			conndir, &qp->tcpctlfd);
		alarm(0);
		if (qp->tcpfd < 0) {
			dnslog("can't dial tcp!%s!dns: %r", domain);
			addslug(udppkt);
		} else {
			uchar belen[2];

			nci = getnetconninfo(conndir, qp->tcpfd);
			if (nci) {
				parseip(qp->tcpip, nci->rsys);
				freenetconninfo(nci);
			} else
				dnslog("mydnsquery: getnetconninfo failed");
			qp->tcpset = 1;

			belen[0] = len >> 8;
			belen[1] = len;
			if (write(qp->tcpfd, belen, 2) != 2 ||
			    write(qp->tcpfd, udppkt + Udphdrsize, len) != len)
				warning("sending tcp msg %r");
		}
		free(domain);
		break;
	default:
		sysfatal("mydnsquery: bad medium");
	}
	return rv;
}

/*
 * send query to all UDP destinations or one TCP destination,
 * taken from obuf (udp packet) header
 */
static int
xmitquery(Query *qp, int medium, int depth, uchar *obuf, int inns, int len)
{
	int j, n;
	char buf[32];
	Dest *p;

	queryck(qp);
	if(time(nil) >= qp->req->aborttime)
		return -1;

	/*
	 * get a nameserver address if we need one.
	 * serveraddrs populates qp->dest.
	 */
	p = qp->dest;
	destck(p);
	if (qp->ndest < 0 || qp->ndest > Maxdest)
		dnslog("qp->ndest %d out of range", qp->ndest);
	if (qp->ndest > qp->curdest - p)
		qp->curdest = &qp->dest[serveraddrs(qp, qp->curdest - p, depth)];
	destck(qp->curdest);

	/* no servers, punt */
	if (qp->curdest == qp->dest)
		if (cfg.straddle && cfg.inside) {
			/* get ips of "outside-ns-ips" */
			p = qp->curdest = qp->dest;
			for(n = 0; n < Maxdest; n++, qp->curdest++)
				if (setdestoutns(qp->curdest, n) < 0)
					break;
		} else {
			/* it's probably just a bogus domain, don't log it */
			// dnslog("xmitquery: %s: no nameservers", qp->dp->name);
			return -1;
		}

	/* send to first 'qp->ndest' destinations */
	j = 0;
	if (medium == Tcp) {
		j++;
		queryck(qp);
		assert(qp->dp);
		procsetname("tcp %sside query for %s %s", (inns? "in": "out"),
			qp->dp->name, rrname(qp->type, buf, sizeof buf));
		mydnsquery(qp, medium, obuf, len); /* sets qp->tcpip from obuf */
		if(debug)
			logsend(qp->req->id, depth, qp->tcpip, "", qp->dp->name,
				qp->type);
	} else
		for(; p < &qp->dest[qp->ndest] && p < qp->curdest; p++){
			/* skip destinations we've finished with */
			if(p->nx >= Maxtrans)
				continue;

			j++;

			/* exponential backoff of requests */
			if((1<<p->nx) > qp->ndest)
				continue;

			procsetname("udp %sside query to %I/%s %s %s",
				(inns? "in": "out"), p->a, p->s->name,
				qp->dp->name, rrname(qp->type, buf, sizeof buf));
			if(debug)
				logsend(qp->req->id, depth, p->a, p->s->name,
					qp->dp->name, qp->type);

			/* fill in UDP destination addr & send it */
			memmove(obuf, p->a, sizeof p->a);
			mydnsquery(qp, medium, obuf, len);
			p->nx++;
		}
	if(j == 0) {
		// dnslog("xmitquery: %s: no destinations left", qp->dp->name);
		return -1;
	}
	return 0;
}

static int
procansw(Query *qp, DNSmsg *mp, uchar *srcip, int depth, Dest *p)
{
	int rv;
	char buf[32];
	DN *ndp;
	Query nquery;
	RR *tp, *soarr;

	/* ignore any error replies */
	if((mp->flags & Rmask) == Rserver){
		rrfreelist(mp->qd);
		rrfreelist(mp->an);
		rrfreelist(mp->ar);
		rrfreelist(mp->ns);
		if(p != qp->curdest)
			p->code = Rserver;
		return -1;
	}

	/* ignore any bad delegations */
	if(mp->ns && baddelegation(mp->ns, qp->nsrp, srcip)){
		rrfreelist(mp->ns);
		mp->ns = nil;
		if(mp->an == nil){
			rrfreelist(mp->qd);
			rrfreelist(mp->ar);
			if(p != qp->curdest)
				p->code = Rserver;
			return -1;
		}
	}

	/* remove any soa's from the authority section */
	soarr = rrremtype(&mp->ns, Tsoa);

	/* incorporate answers */
	if(mp->an)
		rrattach(mp->an, (mp->flags & Fauth) != 0);
	if(mp->ar)
		rrattach(mp->ar, 0);
	if(mp->ns){
		ndp = mp->ns->owner;
		rrattach(mp->ns, 0);
	} else
		ndp = nil;

	/* free the question */
	if(mp->qd)
		rrfreelist(mp->qd);

	/*
	 *  Any reply from an authoritative server,
	 *  or a positive reply terminates the search
	 */
	if(mp->an != nil || (mp->flags & Fauth)){
		if(mp->an == nil && (mp->flags & Rmask) == Rname)
			qp->dp->respcode = Rname;
		else
			qp->dp->respcode = 0;

		/*
		 *  cache any negative responses, free soarr
		 */
		if((mp->flags & Fauth) && mp->an == nil)
			cacheneg(qp->dp, qp->type, (mp->flags & Rmask), soarr);
		else
			rrfreelist(soarr);
		return 1;
	}
	rrfreelist(soarr);

	/*
	 *  if we've been given better name servers,
	 *  recurse.  we're called from udpquery, called from
	 *  netquery, which current holds qp->dp->querylck,
	 *  so release it now and acquire it upon return.
	 */
	if(!mp->ns)
		return 0;
	tp = rrlookup(ndp, Tns, NOneg);
	if(contains(qp->nsrp, tp)){
		rrfreelist(tp);
		return 0;
	}
	procsetname("recursive query for %s %s", qp->dp->name,
		rrname(qp->type, buf, sizeof buf));
//	qunlock(&qp->dp->querylck);

	queryinit(&nquery, qp->dp, qp->type, qp->req);
	nquery.nsrp = tp;
	rv = netquery(&nquery, depth+1);

//	qlock(&qp->dp->querylck);
	rrfreelist(tp);
	querydestroy(&nquery);
	return rv;
}

/*
 * send a query via tcp to a single address (from ibuf's udp header)
 * and read the answer(s) into mp->an.
 */
static int
tcpquery(Query *qp, DNSmsg *mp, int depth, uchar *ibuf, uchar *obuf, int len,
	int waitsecs, int inns, ushort req)
{
	int rv = 0;
	ulong endtime;

	endtime = time(nil) + waitsecs;
	if(endtime > qp->req->aborttime)
		endtime = qp->req->aborttime;

	dnslog("%s: udp reply truncated; retrying query via tcp to %I",
		qp->dp->name, qp->tcpip);

	qlock(&qp->tcplock);
	memmove(obuf, ibuf, IPaddrlen);		/* send back to respondent */
	/* sets qp->tcpip from obuf's udp header */
	if (xmitquery(qp, Tcp, depth, obuf, inns, len) < 0 ||
	    readreply(qp, Tcp, req, ibuf, mp, endtime) < 0)
		rv = -1;
	if (qp->tcpfd > 0) {
		hangup(qp->tcpctlfd);
		close(qp->tcpctlfd);
		close(qp->tcpfd);
	}
	qp->tcpfd = qp->tcpctlfd = -1;
	qunlock(&qp->tcplock);
	return rv;
}

/*
 *  query name servers.  If the name server returns a pointer to another
 *  name server, recurse.
 */
static int
netquery1(Query *qp, int depth, uchar *ibuf, uchar *obuf, int waitsecs, int inns)
{
	int ndest, len, replywaits, rv;
	ushort req;
	ulong endtime;
	char buf[12];
	uchar srcip[IPaddrlen];
	DNSmsg m;
	Dest *p, *np;
	Dest dest[Maxdest];

	/* pack request into a udp message */
	req = rand();
	len = mkreq(qp->dp, qp->type, obuf, Frecurse|Oquery, req);

	/* no server addresses yet */
	queryck(qp);
	for (p = dest; p < dest + nelem(dest); p++)
		destinit(p);
	qp->curdest = qp->dest = dest;

	/*
	 *  transmit udp requests and wait for answers.
	 *  at most Maxtrans attempts to each address.
	 *  each cycle send one more message than the previous.
	 *  retry a query via tcp if its response is truncated.
	 */
	for(ndest = 1; ndest < Maxdest; ndest++){
		qp->ndest = ndest;
		qp->tcpset = 0;
		if (xmitquery(qp, Udp, depth, obuf, inns, len) < 0)
			break;

		endtime = time(nil) + waitsecs;
		if(endtime > qp->req->aborttime)
			endtime = qp->req->aborttime;

		for(replywaits = 0; replywaits < ndest; replywaits++){
			procsetname("reading %sside reply from %s%I for %s %s",
				(inns? "in": "out"),
				(isaslug(qp->tcpip)? "sluggard ": ""), obuf,
				qp->dp->name, rrname(qp->type, buf, sizeof buf));

			/* read udp answer */
			if (readreply(qp, Udp, req, ibuf, &m, endtime) >= 0)
				memmove(srcip, ibuf, IPaddrlen);
			else if (!(m.flags & Ftrunc)) {
				addslug(ibuf);
				break;		/* timed out on this dest */
			} else {
				/* whoops, it was truncated! ask again via tcp */
				rv = tcpquery(qp, &m, depth, ibuf, obuf, len,
					waitsecs, inns, req);
				if (rv < 0)
					break;		/* failed via tcp too */
				memmove(srcip, qp->tcpip, IPaddrlen);
			}

			/* find responder */
			// dnslog("netquery1 got reply from %I", srcip);
			for(p = qp->dest; p < qp->curdest; p++)
				if(memcmp(p->a, srcip, sizeof p->a) == 0)
					break;

			/* remove all addrs of responding server from list */
			for(np = qp->dest; np < qp->curdest; np++)
				if(np->s == p->s)
					p->nx = Maxtrans;

			rv = procansw(qp, &m, srcip, depth, p);
			if (rv > 0)
				return rv;
		}
	}

	/* if all servers returned failure, propagate it */
	qp->dp->respcode = Rserver;
	for(p = dest; p < qp->curdest; p++) {
		destck(p);
		if(p->code != Rserver)
			qp->dp->respcode = 0;
		p->magic = 0;			/* prevent accidents */
	}

//	if (qp->dp->respcode)
//		dnslog("netquery1 setting Rserver for %s", qp->dp->name);

	qp->dest = qp->curdest = nil;		/* prevent accidents */
	return 0;
}

/*
 *  run a command with a supplied fd as standard input
 */
char *
system(int fd, char *cmd)
{
	int pid, p, i;
	static Waitmsg msg;

	if((pid = fork()) == -1)
		sysfatal("fork failed: %r");
	else if(pid == 0){
		dup(fd, 0);
		close(fd);
		for (i = 3; i < 200; i++)
			close(i);		/* don't leak fds */
		execl("/bin/rc", "rc", "-c", cmd, nil);
		sysfatal("exec rc: %r");
	}
	for(p = waitpid(); p >= 0; p = waitpid())
		if(p == pid)
			return msg.msg;
	return "lost child";
}

enum { Hurry, Patient, };
enum { Outns, Inns, };
enum { Remntretry = 15, };	/* min. sec.s between remount attempts */

static int
udpquery(Query *qp, char *mntpt, int depth, int patient, int inns)
{
	int fd, rv = 0;
	long now;
	char *msg;
	uchar *obuf, *ibuf;
	static QLock mntlck;
	static ulong lastmount;

	/* use alloced buffers rather than ones from the stack */
	// ibuf = emalloc(Maxudpin+Udphdrsize);
	ibuf = emalloc(64*1024);		/* max. tcp reply size */
	obuf = emalloc(Maxudp+Udphdrsize);

	fd = udpport(mntpt);
	while (fd < 0 && cfg.straddle && strcmp(mntpt, "/net.alt") == 0) {
		/* HACK: remount /net.alt */
		now = time(nil);
		if (now < lastmount + Remntretry)
			sleep((lastmount + Remntretry - now)*1000);
		qlock(&mntlck);
		fd = udpport(mntpt);	/* try again under lock */
		if (fd < 0) {
			dnslog("[%d] remounting /net.alt", getpid());
			unmount(nil, "/net.alt");

			msg = system(open("/dev/null", ORDWR), "outside");

			lastmount = time(nil);
			if (msg && *msg) {
				dnslog("[%d] can't remount /net.alt: %s",
					getpid(), msg);
				sleep(10*1000);		/* don't spin wildly */
			} else
				fd = udpport(mntpt);
		}
		qunlock(&mntlck);
	}
	if(fd >= 0) {
		qp->req->aborttime = time(nil) + (patient? Maxreqtm: Maxreqtm/2);
		qp->udpfd = fd;
		/* tune; was (patient? 15: 10) */
		rv = netquery1(qp, depth, ibuf, obuf, (patient? 10: 5), inns);
		close(fd);
	} else
		dnslog("can't get udpport for %s query of name %s: %r",
			mntpt, qp->dp->name);

	free(obuf);
	free(ibuf);
	return rv;
}

/* look up (dp->name,type) via *nsrp with results in *reqp */
static int
netquery(Query *qp, int depth)
{
	int lock, rv, triedin, inname;
	RR *rp;

	if(depth > 12)			/* in a recursive loop? */
		return 0;

	slave(qp->req);
	/*
	 * slave might have forked.  if so, the parent process longjmped to
	 * req->mret; we're usually the child slave, but if there are too
	 * many children already, we're still the same process.
	 */

	/* don't lock before call to slave so only children can block */
	if (0)
		lock = qp->req->isslave != 0;
	if(0 && lock) {
		procsetname("query lock wait for %s", qp->dp->name);
		/*
		 * don't make concurrent queries for this name.
		 *
		 * this seemed like a good idea, to avoid swamping
		 * an overloaded ns, but in practice, dns processes
		 * pile up quickly and dns becomes unresponsive for a while.
		 */
		qlock(&qp->dp->querylck);
	}
	procsetname("netquery: %s", qp->dp->name);

	/* prepare server RR's for incremental lookup */
	for(rp = qp->nsrp; rp; rp = rp->next)
		rp->marker = 0;

	rv = 0;				/* pessimism */
	triedin = 0;
	qp->nsrp = qp->nsrp;
	/*
	 * normal resolvers and servers will just use mntpt for all addresses,
	 * even on the outside.  straddling servers will use mntpt (/net)
	 * for inside addresses and /net.alt for outside addresses,
	 * thus bypassing other inside nameservers.
	 */
	inname = insideaddr(qp->dp->name);
	if (!cfg.straddle || inname) {
		rv = udpquery(qp, mntpt, depth, Hurry, (cfg.inside? Inns: Outns));
		triedin = 1;
	}

	/*
	 * if we're still looking, are inside, and have an outside domain,
	 * try it on our outside interface, if any.
	 */
	if (rv == 0 && cfg.inside && !inname) {
		if (triedin)
			dnslog(
	   "[%d] netquery: internal nameservers failed for %s; trying external",
				getpid(), qp->dp->name);

		/* prepare server RR's for incremental lookup */
		for(rp = qp->nsrp; rp; rp = rp->next)
			rp->marker = 0;

		rv = udpquery(qp, "/net.alt", depth, Patient, Outns);
	}
//	if (rv == 0)		/* could ask /net.alt/dns directly */
//		askoutdns(qp->dp, qp->type);

	if(0 && lock)
		qunlock(&qp->dp->querylck);
	return rv;
}

int
seerootns(void)
{
	int rv;
	char root[] = "";
	Request req;
	Query query;

	memset(&req, 0, sizeof req);
	req.isslave = 1;
	req.aborttime = now + Maxreqtm;
	queryinit(&query, dnlookup(root, Cin, 1), Tns, &req);
	query.nsrp = dblookup(root, Cin, Tns, 0, 0);
	rv = netquery(&query, 0);
	querydestroy(&query);
	return rv;
}
