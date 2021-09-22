/*
 * mxdial - dial mail exchangers (MXs)
 *
 * makes heavy use of cs and dns.
 */
#include "common.h"
#include <smtp.h>	/* to publish dial_string_parse */

enum
{
	Nmx=		16,
	Maxstring=	256,
	Maxipstr=	8*5,		/* big enough for ipv6 with NUL */
};

typedef struct Mx	Mx;
struct Mx
{
	char	host[Maxstring];
	char	ip[Maxipstr];		/* this is just the first ip */
	int	pref;
	char	*sts;			/* nil, Giveup or Retry */
};

int	alarmscale;			/* from smtp.c */

char	*bustedmxs[Maxbustedmx];

static Mx mx[Nmx];

static int	callmx(DS*, char*, char*, char**);
static int	compar(void*, void*);
static void	expand_meta(DS *ds);
static int	mxlookup(DS*, char*);
static int	mxlookup1(DS*, char*);

int
mxdial(char *addr, char *ddomain, char *gdomain, char **statusp)
{
	int fd;
	DS ds;
	char err[Errlen];

	addr = netmkaddr(addr, 0, "smtp");
	dial_string_parse(addr, &ds);

	/* try connecting to destination or any of it's mail routers */
	werrstr("");
	fd = callmx(&ds, addr, ddomain, statusp);
	rerrstr(err, sizeof(err));
	if(fd < 0 && gdomain && strstr(err, "can't translate") != 0)
		/* try our mail gateway, if any */
		fd = dial(netmkaddr(gdomain, 0, "smtp"), 0, 0, 0);
	return fd;
}

static int
busted(char *mx)
{
	char **bmp;

	for (bmp = bustedmxs; *bmp != nil; bmp++)
		if (strcmp(mx, *bmp) == 0)
			return 1;
	return 0;
}

static int
mxtimeout(void*, char *msg)
{
	if(strstr(msg, "alarm"))
		return 1;
	return 0;
}

long
timedwrite(int fd, void *buf, long len, long ms)
{
	long n, oalarm;

	atnotify(mxtimeout, 1);
	oalarm = alarm(ms);
	n = write(fd, buf, len);
	alarm(oalarm);
	atnotify(mxtimeout, 0);
	return n;
}

static int
isloopback(char *ip)
{
	return strcmp(ip, "127.0.0.1") == 0 || strcmp(ip, "::1") == 0;
}

/* Giveup overrides all others */
static void
setmxsts(Mx *mxp, char *sts)
{
	if (sts == nil || sts[0] == '\0' ||
	    mxp->sts && strcmp(mxp->sts, Giveup) == 0)
		return;
	if (mxp->sts && strcmp(mxp->sts, Retry) == 0 &&
	    strcmp(sts, Giveup) == 0)
		mxp->sts = Giveup;
	else if (mxp->sts == nil)
		mxp->sts = sts;
}

/*
 * refuse to honor loopback addresses given by dns.  catch \n too.
 * it's imperfect, as there may be multiple ip addresses per name.
 */
static char *
ismxok(Mx *mxp)
{
	if(strchr(mxp->ip, '\n') != nil){
		if(debug)
			fprint(2, "mxlookup ip contains newline\n");
		werrstr("newline in mail server ip");
		return Giveup;
	} else if(isloopback(mxp->ip)){
		if(debug)
			fprint(2, "mxlookup returned loopback\n");
		werrstr("domain lists %s as mail server", mxp->ip);
		return Giveup;
	} else if (busted(mxp->host)) {
		if (debug)
			fprint(2, "mxdial skipping busted mx %s\n", mxp->host);
		return Retry;
	} else
		return nil;
}

/*
 *  fetch all the mx entries for a domain name, dial them in order of
 *  preference until one succeeds, and return its fd.  on error, *status
 *  is set to Retry or Giveup instead of nil.
 *
 *  minor bug: should keep trying mxs until successful delivery.
 *  the conversation could fail even after making a connection.
 *  one mx could be broken or buggy.
 */
static int
callmx(DS *ds, char *dest, char *domain, char **status)
{
	int mxfd, nmx, oalarm;
	char *sts;
	char addr[Maxstring], netpath[NETPATHLEN];
	Mx *mxp;

	/* get a list of mx entries */
	nmx = mxlookup(ds, domain);
	if(nmx < 0) {		/* dns isn't working? don't just dial dest */
		*status = Retry;
		return -1;
	}
	if(nmx == 0){
		if(debug)
			fprint(2, "mxlookup returns none; trying %s\n", dest);
		mxfd = dial(dest, 0, 0, 0);
		*status = mxfd < 0? Retry: nil;
		return mxfd;
	}

	/* sort by preference */
	if(nmx > 1)
		qsort(mx, nmx, sizeof(Mx), compar);

	/* dial each one in turn by name, not ip, until a connection is made */
	for (mxp = mx; mxp < &mx[nmx]; mxp++) {
		sts = ismxok(mxp);
		setmxsts(mxp, sts);
		if(sts != nil)
			continue;
		snprint(addr, sizeof(addr), "%s/%s!%s!%s", ds->netdir, ds->proto,
			mxp->host, ds->service);
		if(debug)
			fprint(2, "mxdial trying %s\n", addr);
		atnotify(mxtimeout, 1);
		/* this was 10 seconds, but oclsc.org at least needs more. */
		oalarm = alarm(6*alarmscale);
		werrstr("");
		mxfd = dial(addr, 0, netpath, 0);
		alarm(oalarm);
		atnotify(mxtimeout, 0);
		if(mxfd >= 0) {
			/* TODO? read netpath"/local", check for loopback addr */
			*status = nil;
			return mxfd;  /* normal exit: this mx had better work */
		}
		setmxsts(mxp, Retry);	/* default if sts not Giveup */
		if (debug)
			fprint(2, "dial: %r\n");
	}
	for (mxp = mx; mxp < &mx[nmx]; mxp++)
		if (mxp->sts && strcmp(mxp->sts, Giveup) == 0) {
			*status = Giveup;
			return -1;
		}
	*status = Retry;
	return -1;
}

/*
 *  call the dns process and have it try to resolve the mx request
 *
 *  this routine knows about the firewall and tries inside and outside
 *  dns's seperately.
 */
static int
mxlookup(DS *ds, char *domain)
{
	int n;

	/* just in case we find no domain name */
	strcpy(domain, ds->host);	/* assumes domain is Maxdomain bytes */

	if(ds->netdir)
		n = mxlookup1(ds, domain);
	else {
		ds->netdir = "/net";
		n = mxlookup1(ds, domain);
		if(n == 0) {
			ds->netdir = "/net.alt";
			n = mxlookup1(ds, domain);
		}
	}

	return n;
}

static int
askdnsmx(int dns, char *host, char *domain)
{
	int n, nmx;
	char buf[Maxdomain], dnsname[Maxstring];
	char *fields[4];
	Mx *mxp;

	snprint(buf, sizeof buf, "%s mx", host);
	if(debug)
		fprint(2, "sending %s '%s'\n", dnsname, buf);
	/*
	 * don't hang indefinitely in the write to /net/dns.
	 */
	seek(dns, 0, 0);
	n = timedwrite(dns, buf, strlen(buf), 6*alarmscale);
	if(n < 0){
		rerrstr(buf, sizeof buf);
		if(debug)
			fprint(2, "dns: %s\n", buf);
		if(strstr(buf, "dns failure"))
			/* if dns fails for the mx lookup, we have to stop */
			return -1;
		return 0;
	}

	/*
	 *  parse any mx entries
	 *  assumes one record per read from dns
	 */
	seek(dns, 0, 0);
	nmx = 0;
	while(nmx < Nmx && (n = read(dns, buf, sizeof buf-1)) > 0){
		mxp = &mx[nmx];
		buf[n] = 0;
		if(debug)
			fprint(2, "dns mx: %s\n", buf);
		n = getfields(buf, fields, 4, 1, " \t");
		if(n < 4)
			continue;

		if(strchr(domain, '.') == 0)
			strcpy(domain, fields[0]);

		strncpy(mxp->host, fields[3], sizeof mxp->host - 1);
		mxp->host[sizeof mxp->host - 1] = '\0';
		mxp->pref = atoi(fields[2]);
		mxp->sts = nil;
		nmx++;
	}
	if(debug)
		fprint(2, "dns mx: got %d mx servers\n", nmx);
	return nmx;
}

/*
 * lookup up mxp->host in dns and store first ip address in mxp->ip.
 * returns number of answers (0 or 1), or -1 on dns failure.
 */
static int
askdnsmxip(int dns, Mx *mxp, char *iptype)
{
	int n;
	char buf[Maxdomain];		/* query & answer */
	char *fields[4];

	/*
	 * look up first ip address (v4 or v6) of each mx name.
	 * should really look at all addresses.
	 * assumes one record per read from dns.
	 */
	seek(dns, 0, 0);
	snprint(buf, sizeof buf, "%s %s", mxp->host, iptype);
	if (debug)
		fprint(2, "asking dns for mx ip: %s\n", buf);
	mxp->ip[0] = 0;
	/*
	 * don't hang indefinitely in the write to /net/dns.
	 */
	if(timedwrite(dns, buf, strlen(buf), 6*alarmscale) < 0)
		return -1;

	seek(dns, 0, 0);
	if((n = read(dns, buf, sizeof buf-1)) < 0)
		return -1;
	buf[n] = 0;
	if(getfields(buf, fields, 4, 1, " \t") < 3)
		return 0;
	strncpy(mxp->ip, fields[2], sizeof mxp->ip - 1);
	mxp->ip[sizeof mxp->ip - 1] = '\0';
	if (debug)
		fprint(2, "\tgot ip %s\n", mxp->ip);
	return 1;
}

static int
mxlookup1(DS *ds, char *domain)
{
	int n, dns, nmx, gotmx;
	char dnsname[Maxstring];
	Mx *mxp, *endmxp;

	werrstr("");
	snprint(dnsname, sizeof dnsname, "%s/dns", ds->netdir);
	dns = open(dnsname, ORDWR);
	if(dns < 0)
		return -1;

	nmx = askdnsmx(dns, ds->host, domain);
	if (nmx < 0) {
		close(dns);
		return -1;
	}

	/*
	 * no mx record? try name itself.
	 *
	 * BUG? If domain has no dots, then we used to look up ds->host
	 * but return domain instead of ds->host in the list.  Now we return
	 * ds->host.  What will this break?
	 */
	if(nmx == 0){
		gotmx = 0;
		mx[0].pref = 1;
		strncpy(mx[0].host, ds->host, sizeof(mx[0].host));
		nmx++;
	} else
		gotmx = 1;	/* never return 0 below, but -1 w/ errstr */

	/*
	 * look up first ip address (v4 or v6) of each mx name.
	 * should really look at all addresses.
	 * assumes one record per read from dns.
	 *
	 * we now only do this to later reject localhost attempts.
	 * maybe we should do that just before dialing in callmx, not here.
	 */
	endmxp = &mx[nmx];
	for (mxp = mx; mxp < endmxp; ) {
		n = askdnsmxip(dns, mxp, "ip");
		if (n <= 0)
			n = askdnsmxip(dns, mxp, "ipv6");
		if (n <= 0) {		/* no ip from dns? */
			/*
			 * ignore this mx.  if it isn't last mx, replace it
			 * with last mx, go around again.  always drop last mx.
			 */
			--endmxp;
			if (mxp < endmxp)
				*mxp = *endmxp;	/* copy previous last mx */
			else
				/* don't confuse error processing */
				mxp->sts = nil;
		} else
			mxp++;
	}
	close(dns);
	nmx = mxp - mx;
	if (nmx <= 0 && gotmx) {
		/*
		 * mxs exist, even if we couldn't find ip addresses for them,
		 * so report dns failure.  don't try to contact dest directly.
		 */
		werrstr("dns failure");
		return -1;
	}
	return nmx;
}

static int
compar(void *a, void *b)
{
	return ((Mx*)a)->pref - ((Mx*)b)->pref;
}

/* break up a network address into its component parts */
void
dial_string_parse(char *str, DS *ds)
{
	char *p, *p2;

	strncpy(ds->buf, str, sizeof(ds->buf));
	ds->buf[sizeof(ds->buf)-1] = 0;

	p = strchr(ds->buf, '!');
	if(p == 0) {
		ds->netdir = 0;
		ds->proto = "net";
		ds->host = ds->buf;
	} else {
		if(*ds->buf != '/'){
			ds->netdir = 0;
			ds->proto = ds->buf;
		} else {
			ds->netdir = ds->buf;
			for(p2 = p; *p2 != '/'; p2--)
				;
			*p2++ = 0;  /* chop "/net.alt/tcp!host" before "tcp" */
			ds->proto = p2;
		}
		*p = 0;			/* step on "!" after "tcp" */
		ds->host = p + 1;
	}
	ds->service = strchr(ds->host, '!');
	if(ds->service)
		*ds->service++ = 0;
	if(*ds->host == '$')
		expand_meta(ds);
}

static void
expand_meta(DS *ds)
{
	char buf[128], cs[128], *net, *p;
	int fd, n;

	net = ds->netdir;
	if(!net)
		net = "/net";

	if(debug)
		fprint(2, "expanding %s!%s\n", net, ds->host);
	snprint(cs, sizeof(cs), "%s/cs", net);
	if((fd = open(cs, ORDWR)) == -1){
		if(debug)
			fprint(2, "open %s: %r\n", cs);
		syslog(0, "smtp", "cannot open %s: %r", cs);
		return;
	}

	snprint(buf, sizeof buf, "!ipinfo %s", ds->host+1);	// +1 to skip $
	if(write(fd, buf, strlen(buf)) <= 0){
		if(debug)
			fprint(2, "write %s: %r\n", cs);
		syslog(0, "smtp", "%s to %s - write failed: %r", buf, cs);
		close(fd);
		return;
	}

	seek(fd, 0, 0);
	if((n = read(fd, ds->expand, sizeof(ds->expand)-1)) < 0){
		if(debug)
			fprint(2, "read %s: %r\n", cs);
		syslog(0, "smtp", "%s - read failed: %r", cs);
		close(fd);
		return;
	}
	close(fd);

	ds->expand[n] = 0;
	if((p = strchr(ds->expand, '=')) == nil){
		if(debug)
			fprint(2, "response %s: %s\n", cs, ds->expand);
		syslog(0, "smtp", "%q from %s - bad response: %r", ds->expand, cs);
		return;
	}
	ds->host = p+1;

	/* take only first one returned (quasi-bug) */
	if((p = strchr(ds->host, ' ')) != nil)
		*p = 0;
}
