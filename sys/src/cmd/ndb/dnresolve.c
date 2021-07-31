#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "dns.h"

enum
{
	Maxdest=	24,	/* maximum destinations for a request message */
	Maxtrans=	3,	/* maximum transmissions to a server */
};

static int	netquery(DN*, int, RR*, Request*, int);
static RR*	dnresolve1(char*, int, int, Request*, int, int);

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

static RR*
dnresolve1(char *name, int class, int type, Request *req, int depth,
	int recurse)
{
	DN *dp, *nsdp;
	RR *rp, *nsrp, *dbnsrp;
	char *cp;

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

	/*
	 *  if we're running as just a resolver, query our
	 *  designated name servers
	 */
	if(cfg.resolver){
		nsrp = randomize(getdnsservers(class));
		if(nsrp != nil) {
			if(netquery(dp, type, nsrp, req, depth+1)){
				rrfreelist(nsrp);
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
//			dnslog("dnresolve1: local domain %s -> %#p", name, rp);
			rrfreelist(dbnsrp);
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
//			dnslog("dnresolve1: %s: trying ns in cache", dp->name);
			if(netquery(dp, type, nsrp, req, depth+1)){
				rrfreelist(nsrp);
				return rrlookup(dp, type, OKneg);
			}
			rrfreelist(nsrp);
			continue;
		}

		/* use ns from db */
		if(dbnsrp){
			/* try the name servers found in db */
//			dnslog("dnresolve1: %s: trying ns in db", dp->name);
			if(netquery(dp, type, dbnsrp, req, depth+1)){
				/* we got an answer */
				rrfreelist(dbnsrp);
				return rrlookup(dp, type, NOneg);
			}
			rrfreelist(dbnsrp);
		}
	}

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
static char *ohmsg = "oldheaders";

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
	write(ctl, ohmsg, strlen(ohmsg));

	/* grab the data file */
	snprint(ds, sizeof ds, "%s/data", adir);
	fd = open(ds, ORDWR);
	close(ctl);
	if(fd < 0)
		warning("can't open udp port %s: %r", ds);
	return fd;
}

int
mkreq(DN *dp, int type, uchar *buf, int flags, ushort reqno)
{
	DNSmsg m;
	int len;
	OUdphdr *uh = (OUdphdr*)buf;

	/* stuff port number into output buffer */
	memset(uh, 0, sizeof(*uh));
	hnputs(uh->rport, 53);

	/* make request and convert it to output format */
	memset(&m, 0, sizeof(m));
	m.flags = flags;
	m.id = reqno;
	m.qd = rralloc(type);
	m.qd->owner = dp;
	m.qd->type = type;
	len = convDNS2M(&m, &buf[OUdphdrsize], Maxudp);
	if(len < 0)
		abort();	/* "can't convert" */
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

/*
 *  read replies to a request.  ignore any of the wrong type.
 *  wait at most until endtime.
 */
static int
readreply(int fd, DN *dp, int type, ushort req, uchar *ibuf, DNSmsg *mp,
	ulong endtime, Request *reqp)
{
	char *err;
	int len;
	ulong now;
	RR *rp;

	notify(ding);

	for(; ; freeanswers(mp)){
		now = time(nil);
		if(now >= endtime)
			return -1;	/* timed out */

		/* timed read */
		alarm((endtime - now) * 1000);
		len = read(fd, ibuf, OUdphdrsize+Maxudpin);
		alarm(0);
		len -= OUdphdrsize;
		if(len < 0)
			return -1;	/* timed out */

		/* convert into internal format  */
		memset(mp, 0, sizeof(*mp));
		err = convM2DNS(&ibuf[OUdphdrsize], len, mp, nil);
		if(err){
			dnslog("input err: %s: %I", err, ibuf);
			continue;
		}
		if(debug)
			logreply(reqp->id, ibuf, mp);

		/* answering the right question? */
		if(mp->id != req){
			dnslog("%d: id %d instead of %d: %I", reqp->id,
					mp->id, req, ibuf);
			continue;
		}
		if(mp->qd == 0){
			dnslog("%d: no question RR: %I", reqp->id, ibuf);
			continue;
		}
		if(mp->qd->owner != dp){
			dnslog("%d: owner %s instead of %s: %I",
				reqp->id, mp->qd->owner->name, dp->name, ibuf);
			continue;
		}
		if(mp->qd->type != type){
			dnslog("%d: type %d instead of %d: %I",
				reqp->id, mp->qd->type, type, ibuf);
			continue;
		}

		/* remember what request this is in answer to */
		for(rp = mp->an; rp; rp = rp->next)
			rp->query = type;

		return 0;
	}
}

/*
 *	return non-0 if first list includes second list
 */
int
contains(RR *rp1, RR *rp2)
{
	RR *trp1, *trp2;

	for(trp2 = rp2; trp2; trp2 = trp2->next){
		for(trp1 = rp1; trp1; trp1 = trp1->next){
			if(trp1->type == trp2->type)
			if(trp1->host == trp2->host)
			if(trp1->owner == trp2->owner)
				break;
		}
		if(trp1 == nil)
			return 0;
	}
	return 1;
}


typedef struct Dest	Dest;
struct Dest
{
	uchar	a[IPaddrlen];	/* ip address */
	DN	*s;		/* name server */
	int	nx;		/* number of transmissions */
	int	code;		/* response code; used to clear dp->respcode */
};


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
serveraddrs(DN *dp, RR *nsrp, Dest *dest, int nd, int depth, Request *reqp)
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
	for(rp = nsrp; rp; rp = rp->next){
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
		for(rp = nsrp; rp; rp = rp->next){
			if(rp->marker)
				continue;
			rp->marker = 1;

			/*
			 *  avoid loops looking up a server under itself
			 */
			if(subsume(rp->owner->name, rp->host->name))
				continue;

			arp = dnresolve(rp->host->name, Cin, Ta, reqp, 0,
				depth+1, Recurse, 1, 0);
			rrfreelist(rrremneg(&arp));
			if(arp)
				break;
		}

	/* use any addresses that we found */
	for(trp = arp; trp; trp = trp->next){
		if(nd >= Maxdest)
			break;
		cur = &dest[nd];
		parseip(cur->a, trp->ip->name);
		/*
		 * straddling servers can reject all nameservers if they are all
		 * inside, so be sure to list at least one outside ns at
		 * the end of the ns list in /lib/ndb for `dom='.
		 */
		if (ipisbm(cur->a) ||
		    cfg.straddle && !insideaddr(dp->name) && insidens(cur->a))
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

	memset(p, 0, sizeof *p);
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
 *  query name servers.  If the name server returns a pointer to another
 *  name server, recurse.
 */
static int
netquery1(int fd, DN *dp, int type, RR *nsrp, Request *reqp, int depth,
	uchar *ibuf, uchar *obuf, int waitsecs, int inns)
{
	int ndest, j, len, replywaits, rv, n;
	ushort req;
	ulong endtime;
	char buf[12];
	DN *ndp;
	DNSmsg m;
	Dest *p, *l, *np;
	Dest dest[Maxdest];
	RR *tp, *soarr;
//	char fdbuf[1024];

//	fd2path(fd, fdbuf, sizeof fdbuf);
//	dnslog("netquery: on %s for %s %s ns", fdbuf, dp->name,
//		(inns? "inside": "outside"));

	/* pack request into a message */
	req = rand();
	len = mkreq(dp, type, obuf, Frecurse|Oquery, req);

	/* no server addresses yet */
	l = dest;

	/*
	 *  transmit requests and wait for answers.
	 *  at most Maxtrans attempts to each address.
	 *  each cycle send one more message than the previous.
	 */
	for(ndest = 1; ndest < Maxdest; ndest++){
//		dnslog("netquery1 xmit loop: now %ld aborttime %ld", time(nil),
//			reqp->aborttime);
		if(time(nil) >= reqp->aborttime)
			break;

		/* get a server address if we need one */
		p = dest;
		if(ndest > l - p){
			j = serveraddrs(dp, nsrp, dest, l - p, depth, reqp);
			l = &dest[j];
		}

		/* no servers, punt */
		if(l == dest)
			if (cfg.straddle && cfg.inside) {
				p = l = dest;
				for(n = 0; n < Maxdest; n++, l++)
					if (setdestoutns(l, n) < 0)
						break;
			} else {
//				dnslog("netquery1: %s: no servers", dp->name);
				break;
			}

		/* send to first 'ndest' destinations */
		j = 0;
		for(; p < &dest[ndest] && p < l; p++){
			/* skip destinations we've finished with */
			if(p->nx >= Maxtrans)
				continue;

			j++;

			/* exponential backoff of requests */
			if((1<<p->nx) > ndest)
				continue;

			memmove(obuf, p->a, sizeof p->a);
			procsetname("req slave: %sside query to %I/%s %s %s",
				(inns? "in": "out"), obuf, p->s->name, dp->name,
				rrname(type, buf, sizeof buf));
			if(debug)
				logsend(reqp->id, depth, obuf, p->s->name,
					dp->name, type);

			/* actually send the UDP packet */
			if(write(fd, obuf, len + OUdphdrsize) < 0)
				warning("sending udp msg %r");
			p->nx++;
		}
		if(j == 0)
			break;		/* no destinations left */

		endtime = time(nil) + waitsecs;
		if(endtime > reqp->aborttime)
			endtime = reqp->aborttime;

//		dnslog(
//		    "netquery1 reply wait: now %ld aborttime %ld endtime %ld",
//			time(nil), reqp->aborttime, endtime);
		for(replywaits = 0; replywaits < ndest; replywaits++){
			procsetname(
			    "req slave: reading %sside reply from %I for %s %s",
				(inns? "in": "out"), obuf, dp->name,
				rrname(type, buf, sizeof buf));
			memset(&m, 0, sizeof m);
			if(readreply(fd, dp, type, req, ibuf, &m, endtime, reqp)
			    < 0)
				break;		/* timed out */

//			dnslog("netquery1 got reply from %I", ibuf);
			/* find responder */
			for(p = dest; p < l; p++)
				if(memcmp(p->a, ibuf, sizeof p->a) == 0)
					break;

			/* remove all addrs of responding server from list */
			for(np = dest; np < l; np++)
				if(np->s == p->s)
					p->nx = Maxtrans;

			/* ignore any error replies */
			if((m.flags & Rmask) == Rserver){
//				dnslog(
//				 "netquery1 got Rserver for dest %s of name %s",
//					p->s->name, dp->name);
				rrfreelist(m.qd);
				rrfreelist(m.an);
				rrfreelist(m.ar);
				rrfreelist(m.ns);
				if(p != l) {
//					dnslog(
//	"netquery1 setting Rserver for dest %s of name %s due to Rserver reply",
//						p->s->name, dp->name);
					p->code = Rserver;
				}
				continue;
			}

			/* ignore any bad delegations */
			if(m.ns && baddelegation(m.ns, nsrp, ibuf)){
//				dnslog("netquery1 got a bad delegation from %s",
//					p->s->name);
				rrfreelist(m.ns);
				m.ns = nil;
				if(m.an == nil){
					rrfreelist(m.qd);
					rrfreelist(m.ar);
					if(p != l) {
//						dnslog(
//"netquery1 setting Rserver for dest %s of name %s due to bad delegation",
//							p->s->name, dp->name);
						p->code = Rserver;
					}
					continue;
				}
			}

			/* remove any soa's from the authority section */
			soarr = rrremtype(&m.ns, Tsoa);

			/* incorporate answers */
			if(m.an)
				rrattach(m.an, (m.flags & Fauth) != 0);
			if(m.ar)
				rrattach(m.ar, 0);
			if(m.ns){
				ndp = m.ns->owner;
				rrattach(m.ns, 0);
			} else
				ndp = nil;

			/* free the question */
			if(m.qd)
				rrfreelist(m.qd);

			/*
			 *  Any reply from an authoritative server,
			 *  or a positive reply terminates the search
			 */
			if(m.an != nil || (m.flags & Fauth)){
				if(m.an == nil && (m.flags & Rmask) == Rname)
					dp->respcode = Rname;
				else
					dp->respcode = 0;

				/*
				 *  cache any negative responses, free soarr
				 */
				if((m.flags & Fauth) && m.an == nil)
					cacheneg(dp, type, (m.flags & Rmask),
						soarr);
				else
					rrfreelist(soarr);
				return 1;
			}
			rrfreelist(soarr);

			/*
			 *  if we've been given better name servers,
			 *  recurse.  we're called from udpquery, called from
			 *  netquery, which current holds dp->querylck,
			 *  so release it now and acquire it upon return.
			 */
			if(m.ns){
				tp = rrlookup(ndp, Tns, NOneg);
				if(!contains(nsrp, tp)){
					procsetname(
					 "req slave: recursive query for %s %s",
						dp->name,
						rrname(type, buf, sizeof buf));
					qunlock(&dp->querylck);

					rv = netquery(dp, type, tp, reqp,
						depth+1);

					qlock(&dp->querylck);
					rrfreelist(tp);
					return rv;
				} else
					rrfreelist(tp);
			}
		}
	}

	/* if all servers returned failure, propagate it */
	dp->respcode = Rserver;
	for(p = dest; p < l; p++)
		if(p->code != Rserver)
			dp->respcode = 0;

//	if (dp->respcode)
//		dnslog("netquery1 setting Rserver for %s", dp->name);

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
udpquery(char *mntpt, DN *dp, int type, RR *nsrp, Request *reqp, int depth,
	int patient, int inns)
{
	int fd, rv = 0;
	long now;
	char *msg;
	uchar *obuf, *ibuf;
	static QLock mntlck;
	static ulong lastmount;

	/* use alloced buffers rather than ones from the stack */
	ibuf = emalloc(Maxudpin+OUdphdrsize);
	obuf = emalloc(Maxudp+OUdphdrsize);

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
		reqp->aborttime = time(nil) + (patient? Maxreqtm: Maxreqtm/2);
//		dnslog("udpquery: %s/udp for %s with %s ns", mntpt, dp->name,
//			(inns? "inside": "outside"));
		rv = netquery1(fd, dp, type, nsrp, reqp, depth,
			ibuf, obuf, (patient? 15: 10), inns);
		close(fd);
	} else
		dnslog("can't get udpport for %s query of name %s: %r",
			mntpt, dp->name);

	free(obuf);
	free(ibuf);
	return rv;
}

static int
dnssetup(int domount, char *dns, char *srv, char *mtpt)
{
	int fd;

	fd = open(dns, ORDWR);
	if(fd < 0){
		if(domount == 0){
			werrstr("can't open %s: %r", mtpt);
			return -1;
		}
		fd = open(srv, ORDWR);
		if(fd < 0){
			werrstr("can't open %s: %r", srv);
			return -1;
		}
		if(mount(fd, -1, mtpt, MBEFORE, "") < 0){
			werrstr("can't mount(%s, %s): %r", srv, mtpt);
			return -1;
		}
		fd = open(mtpt, ORDWR);
		if(fd < 0)
			werrstr("can't open %s: %r", mtpt);
	}
	return fd;
}

static RR *
rrparse(char *lines)
{
	int nl, nf, ln, type;
	char *line[100];
	char *field[32];
	RR *rp, *rplist;
//	Server *s;
	SOA *soa;
	Srv *srv;
//	Txt *t;

	rplist = nil;
	nl = tokenize(lines, line, nelem(line));
	for (ln = 0; ln < nl; ln++) {
		if (*line[ln] == '!' || *line[ln] == '?')
			continue;		/* error */
		nf = tokenize(line[ln], field, nelem(field));
		if (nf < 2)
			continue;		/* mal-formed */
		type = rrtype(field[1]);
		rp = rralloc(type);
		rp->owner = dnlookup(field[0], Cin, 1);
		rp->next = rplist;
		rplist = rp;
		switch (type) {		/* TODO: copy fields to *rp */
		case Thinfo:
			// "\t%s %s", dnname(rp->cpu), dnname(rp->os));
			break;
		case Tcname:
		case Tmb:
		case Tmd:
		case Tmf:
		case Tns:
			// "\t%s", dnname(rp->host));
			break;
		case Tmg:
		case Tmr:
			// "\t%s", dnname(rp->mb));
			break;
		case Tminfo:
			// "\t%s %s", dnname(rp->mb), dnname(rp->rmb));
			break;
		case Tmx:
			// "\t%lud %s", rp->pref, dnname(rp->host));
			break;
		case Ta:
		case Taaaa:
			// "\t%s", dnname(rp->ip));	// TODO parseip
			break;
		case Tptr:
			// "\t%s", dnname(rp->ptr));
			break;
		case Tsoa:
			soa = rp->soa;
			USED(soa);
			// "\t%s %s %lud %lud %lud %lud %lud",
			//	dnname(rp->host), dnname(rp->rmb),
			//	(soa? soa->serial: 0),
			//	(soa? soa->refresh: 0), (soa? soa->retry: 0),
			//	(soa? soa->expire: 0), (soa? soa->minttl: 0));
			break;
		case Tsrv:
			srv = rp->srv;
			USED(srv);
			break;
		case Tnull:
			// "\t%.*H", rp->null->dlen, rp->null->data);
			break;
		case Ttxt:
			// for(t = rp->txt; t != nil; t = t->next)
			//	"%s", t->p);
			break;
		case Trp:
			// "\t%s %s", dnname(rp->rmb), dnname(rp->rp));
			break;
		case Tkey:
			// "\t%d %d %d", rp->key->flags, rp->key->proto, rp->key->alg);
			break;
		case Tsig:
			// "\t%d %d %d %lud %lud %lud %d %s",
			//	rp->sig->type, rp->sig->alg, rp->sig->labels,
			//	rp->sig->ttl, rp->sig->exp, rp->sig->incep,
			//	rp->sig->tag, dnname(rp->sig->signer));
			break;
		case Tcert:
			// "\t%d %d %d", rp->cert->type, rp->cert->tag, rp->cert->alg);
			break;
		}
	}
	return nil;
}

static int
querydns(int fd, char *line, int n)
{
	int rv = 0;
	char buf[1024];

	seek(fd, 0, 0);
	if(write(fd, line, n) != n)
		return rv;

	seek(fd, 0, 0);
	buf[0] = '\0';
	while((n = read(fd, buf, sizeof buf - 1)) > 0) {
		buf[n] = 0;
		rrattach(rrparse(buf), 1);	/* incorporate answers */
		rv = 1;
		buf[0] = '\0';
	}
	return rv;
}

static void
askoutdns(DN *dp, int type)		/* ask /net.alt/dns directly */
{
	int len;
	char buf[32];
	char *query;
	char *mtpt = "/net.alt";
	char *dns =  "/net.alt/dns";
	char *srv =  "/srv/dns_net.alt";
	static int fd = -1;

	if (fd < 0)
		fd = dnssetup(1, dns, srv, mtpt);

	query = smprint("%s %s\n", dp->name, rrname(type, buf, sizeof buf));
	len = strlen(query);
	if (!querydns(fd, query, len)) {
		close(fd);
		/* could run outside here */
		fd = dnssetup(1, dns, srv, mtpt);
		querydns(fd, query, len);
	}
	free(query);
}

/* look up (dp->name,type) via *nsrp with results in *reqp */
static int
netquery(DN *dp, int type, RR *nsrp, Request *reqp, int depth)
{
	int lock, rv, triedin, inname;
	RR *rp;

	if(depth > 12)			/* in a recursive loop? */
		return 0;

	slave(reqp);			/* might fork */
	/* if so, parent process longjmped to req->mret; we're child slave */
	if (!reqp->isslave)
		dnslog("[%d] netquery: slave returned with reqp->isslave==0",
			getpid());

	/* don't lock before call to slave so only children can block */
	lock = reqp->isslave != 0;
	if(lock) {
		procsetname("waiting for query lock on %s", dp->name);
		/* don't make concurrent queries for this name */
		qlock(&dp->querylck);
		procsetname("netquery: %s", dp->name);
	}

	/* prepare server RR's for incremental lookup */
	for(rp = nsrp; rp; rp = rp->next)
		rp->marker = 0;

	rv = 0;				/* pessimism */
	triedin = 0;
	/*
	 * normal resolvers and servers will just use mntpt for all addresses,
	 * even on the outside.  straddling servers will use mntpt (/net)
	 * for inside addresses and /net.alt for outside addresses,
	 * thus bypassing other inside nameservers.
	 */
	inname = insideaddr(dp->name);
	if (!cfg.straddle || inname) {
		rv = udpquery(mntpt, dp, type, nsrp, reqp, depth, Hurry,
			(cfg.inside? Inns: Outns));
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
				getpid(), dp->name);

		/* prepare server RR's for incremental lookup */
		for(rp = nsrp; rp; rp = rp->next)
			rp->marker = 0;

		rv = udpquery("/net.alt", dp, type, nsrp, reqp, depth, Patient,
			Outns);
	}
	if (0 && rv == 0)		/* TODO: ask /net.alt/dns directly */
		askoutdns(dp, type);

	if(lock)
		qunlock(&dp->querylck);

	return rv;
}

int
seerootns(void)
{
	char root[] = "";
	Request req;

	memset(&req, 0, sizeof req);
	req.isslave = 1;
	req.aborttime = now + Maxreqtm*2;	/* be patient */
	return netquery(dnlookup(root, Cin, 1), Tns,
		dblookup(root, Cin, Tns, 0, 0), &req, 0);
}
