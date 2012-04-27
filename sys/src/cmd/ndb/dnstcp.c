/*
 * dnstcp - serve dns via tcp
 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dns.h"

Cfg cfg;

char	*caller = "";
char	*dbfile;
int	debug;
uchar	ipaddr[IPaddrlen];	/* my ip address */
char	*logfile = "dns";
int	maxage = 60*60;
char	mntpt[Maxpath];
int	needrefresh;
ulong	now;
vlong	nowns;
int	testing;
int	traceactivity;
char	*zonerefreshprogram;

static int	readmsg(int, uchar*, int);
static void	reply(int, DNSmsg*, Request*);
static void	dnzone(DNSmsg*, DNSmsg*, Request*);
static void	getcaller(char*);
static void	refreshmain(char*);

void
usage(void)
{
	fprint(2, "usage: %s [-rR] [-f ndb-file] [-x netmtpt] [conndir]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	volatile int len, rcode;
	volatile char tname[32];
	char *volatile err, *volatile ext = "";
	volatile uchar buf[64*1024], callip[IPaddrlen];
	volatile DNSmsg reqmsg, repmsg;
	volatile Request req;

	alarm(2*60*1000);
	cfg.cachedb = 1;
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'f':
		dbfile = EARGF(usage());
		break;
	case 'r':
		cfg.resolver = 1;
		break;
	case 'R':
		norecursion = 1;
		break;
	case 'x':
		ext = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(debug < 2)
		debug = 0;

	if(argc > 0)
		getcaller(argv[0]);

	cfg.inside = 1;
	dninit();

	snprint(mntpt, sizeof mntpt, "/net%s", ext);
	if(myipaddr(ipaddr, mntpt) < 0)
		sysfatal("can't read my ip address");
	dnslog("dnstcp call from %s to %I", caller, ipaddr);
	memset(callip, 0, sizeof callip);
	parseip(callip, caller);

	db2cache(1);

	memset(&req, 0, sizeof req);
	setjmp(req.mret);
	req.isslave = 0;
	procsetname("main loop");

	/* loop on requests */
	for(;; putactivity(0)){
		now = time(nil);
		memset(&repmsg, 0, sizeof repmsg);
		len = readmsg(0, buf, sizeof buf);
		if(len <= 0)
			break;

		getactivity(&req, 0);
		req.aborttime = timems() + S2MS(15*Min);
		rcode = 0;
		memset(&reqmsg, 0, sizeof reqmsg);
		err = convM2DNS(buf, len, &reqmsg, &rcode);
		if(err){
			dnslog("server: input error: %s from %s", err, caller);
			free(err);
			break;
		}
		if (rcode == 0)
			if(reqmsg.qdcount < 1){
				dnslog("server: no questions from %s", caller);
				break;
			} else if(reqmsg.flags & Fresp){
				dnslog("server: reply not request from %s",
					caller);
				break;
			} else if((reqmsg.flags & Omask) != Oquery){
				dnslog("server: op %d from %s",
					reqmsg.flags & Omask, caller);
				break;
			}
		if(debug)
			dnslog("[%d] %d: serve (%s) %d %s %s",
				getpid(), req.id, caller,
				reqmsg.id, reqmsg.qd->owner->name,
				rrname(reqmsg.qd->type, tname, sizeof tname));

		/* loop through each question */
		while(reqmsg.qd)
			if(reqmsg.qd->type == Taxfr)
				dnzone(&reqmsg, &repmsg, &req);
			else {
				dnserver(&reqmsg, &repmsg, &req, callip, rcode);
				reply(1, &repmsg, &req);
				rrfreelist(repmsg.qd);
				rrfreelist(repmsg.an);
				rrfreelist(repmsg.ns);
				rrfreelist(repmsg.ar);
			}
		rrfreelist(reqmsg.qd);		/* qd will be nil */
		rrfreelist(reqmsg.an);
		rrfreelist(reqmsg.ns);
		rrfreelist(reqmsg.ar);

		if(req.isslave){
			putactivity(0);
			_exits(0);
		}
	}
	refreshmain(mntpt);
}

static int
readmsg(int fd, uchar *buf, int max)
{
	int n;
	uchar x[2];

	if(readn(fd, x, 2) != 2)
		return -1;
	n = x[0]<<8 | x[1];
	if(n > max)
		return -1;
	if(readn(fd, buf, n) != n)
		return -1;
	return n;
}

static void
reply(int fd, DNSmsg *rep, Request *req)
{
	int len, rv;
	char tname[32];
	uchar buf[64*1024];
	RR *rp;

	if(debug){
		dnslog("%d: reply (%s) %s %s %ux",
			req->id, caller,
			rep->qd->owner->name,
			rrname(rep->qd->type, tname, sizeof tname),
			rep->flags);
		for(rp = rep->an; rp; rp = rp->next)
			dnslog("an %R", rp);
		for(rp = rep->ns; rp; rp = rp->next)
			dnslog("ns %R", rp);
		for(rp = rep->ar; rp; rp = rp->next)
			dnslog("ar %R", rp);
	}


	len = convDNS2M(rep, buf+2, sizeof(buf) - 2);
	buf[0] = len>>8;
	buf[1] = len;
	rv = write(fd, buf, len+2);
	if(rv != len+2){
		dnslog("[%d] sending reply: %d instead of %d", getpid(), rv,
			len+2);
		exits(0);
	}
}

/*
 *  Hash table for domain names.  The hash is based only on the
 *  first element of the domain name.
 */
extern DN	*ht[HTLEN];

static int
numelem(char *name)
{
	int i;

	i = 1;
	for(; *name; name++)
		if(*name == '.')
			i++;
	return i;
}

int
inzone(DN *dp, char *name, int namelen, int depth)
{
	int n;

	if(dp->name == nil)
		return 0;
	if(numelem(dp->name) != depth)
		return 0;
	n = strlen(dp->name);
	if(n < namelen)
		return 0;
	if(strcmp(name, dp->name + n - namelen) != 0)
		return 0;
	if(n > namelen && dp->name[n - namelen - 1] != '.')
		return 0;
	return 1;
}

static void
dnzone(DNSmsg *reqp, DNSmsg *repp, Request *req)
{
	DN *dp, *ndp;
	RR r, *rp;
	int h, depth, found, nlen;

	memset(repp, 0, sizeof(*repp));
	repp->id = reqp->id;
	repp->qd = reqp->qd;
	reqp->qd = reqp->qd->next;
	repp->qd->next = 0;
	repp->flags = Fauth | Fresp | Oquery;
	if(!norecursion)
		repp->flags |= Fcanrec;
	dp = repp->qd->owner;

	/* send the soa */
	repp->an = rrlookup(dp, Tsoa, NOneg);
	reply(1, repp, req);
	if(repp->an == 0)
		goto out;
	rrfreelist(repp->an);
	repp->an = nil;

	nlen = strlen(dp->name);

	/* construct a breadth-first search of the name space (hard with a hash) */
	repp->an = &r;
	for(depth = numelem(dp->name); ; depth++){
		found = 0;
		for(h = 0; h < HTLEN; h++)
			for(ndp = ht[h]; ndp; ndp = ndp->next)
				if(inzone(ndp, dp->name, nlen, depth)){
					for(rp = ndp->rr; rp; rp = rp->next){
						/*
						 * there shouldn't be negatives,
						 * but just in case.
						 * don't send any soa's,
						 * ns's are enough.
						 */
						if (rp->negative ||
						    rp->type == Tsoa)
							continue;
						r = *rp;
						r.next = 0;
						reply(1, repp, req);
					}
					found = 1;
				}
		if(!found)
			break;
	}

	/* resend the soa */
	repp->an = rrlookup(dp, Tsoa, NOneg);
	reply(1, repp, req);
	rrfreelist(repp->an);
	repp->an = nil;
out:
	rrfree(repp->qd);
	repp->qd = nil;
}

static void
getcaller(char *dir)
{
	int fd, n;
	static char remote[128];

	snprint(remote, sizeof(remote), "%s/remote", dir);
	fd = open(remote, OREAD);
	if(fd < 0)
		return;
	n = read(fd, remote, sizeof remote - 1);
	close(fd);
	if(n <= 0)
		return;
	if(remote[n-1] == '\n')
		n--;
	remote[n] = 0;
	caller = remote;
}

static void
refreshmain(char *net)
{
	int fd;
	char file[128];

	snprint(file, sizeof(file), "%s/dns", net);
	if(debug)
		dnslog("refreshing %s", file);
	fd = open(file, ORDWR);
	if(fd < 0)
		dnslog("can't refresh %s", file);
	else {
		fprint(fd, "refresh");
		close(fd);
	}
}

/*
 *  the following varies between dnsdebug and dns
 */
void
logreply(int id, uchar *addr, DNSmsg *mp)
{
	RR *rp;

	dnslog("%d: rcvd %I flags:%s%s%s%s%s", id, addr,
		mp->flags & Fauth? " auth": "",
		mp->flags & Ftrunc? " trunc": "",
		mp->flags & Frecurse? " rd": "",
		mp->flags & Fcanrec? " ra": "",
		(mp->flags & (Fauth|Rmask)) == (Fauth|Rname)? " nx": "");
	for(rp = mp->qd; rp != nil; rp = rp->next)
		dnslog("%d: rcvd %I qd %s", id, addr, rp->owner->name);
	for(rp = mp->an; rp != nil; rp = rp->next)
		dnslog("%d: rcvd %I an %R", id, addr, rp);
	for(rp = mp->ns; rp != nil; rp = rp->next)
		dnslog("%d: rcvd %I ns %R", id, addr, rp);
	for(rp = mp->ar; rp != nil; rp = rp->next)
		dnslog("%d: rcvd %I ar %R", id, addr, rp);
}

void
logsend(int id, int subid, uchar *addr, char *sname, char *rname, int type)
{
	char buf[12];

	dnslog("%d.%d: sending to %I/%s %s %s",
		id, subid, addr, sname, rname, rrname(type, buf, sizeof buf));
}

RR*
getdnsservers(int class)
{
	return dnsservers(class);
}
