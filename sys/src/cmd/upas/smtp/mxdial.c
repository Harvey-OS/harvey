#include "common.h"
#include <ndb.h>
#include <smtp.h>	/* to publish dial_string_parse */

enum
{
	Nmx=		16,
	Maxstring=	256,
	Maxipstr=	8*5,		/* ipv6 */
};

typedef struct Mx	Mx;
struct Mx
{
	char	host[Maxstring];
	char	ip[Maxipstr];		/* this is just the first ip */
	int	pref;
};

char	*bustedmxs[Maxbustedmx];
Ndb *db;

static Mx mx[Nmx];

static int	callmx(DS*, char*, char*);
static int	compar(void*, void*);
static void	expand_meta(DS *ds);
static int	mxlookup(DS*, char*);
static int	mxlookup1(DS*, char*);

int
mxdial(char *addr, char *ddomain, char *gdomain)
{
	int fd;
	DS ds;
	char err[Errlen];

	addr = netmkaddr(addr, 0, "smtp");
	dial_string_parse(addr, &ds);

	/* try connecting to destination or any of it's mail routers */
	fd = callmx(&ds, addr, ddomain);

	/* try our mail gateway */
	rerrstr(err, sizeof(err));
	if(fd < 0 && gdomain && strstr(err, "can't translate") != 0)
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
timeout(void*, char *msg)
{
	if(strstr(msg, "alarm"))
		return 1;
	return 0;
}

long
timedwrite(int fd, void *buf, long len, long ms)
{
	long n, oalarm;

	atnotify(timeout, 1);
	oalarm = alarm(ms);
	n = write(fd, buf, len);
	alarm(oalarm);
	atnotify(timeout, 0);
	return n;
}

static int
isloopback(char *ip)
{
	return strcmp(ip, "127.0.0.1") == 0 || strcmp(ip, "::1") == 0;
}

/*
 *  take an address and return all the mx entries for it,
 *  most preferred first
 */
static int
callmx(DS *ds, char *dest, char *domain)
{
	int fd, i, nmx;
	char *ip;
	char addr[Maxstring];

	/* get a list of mx entries */
	nmx = mxlookup(ds, domain);
	if(nmx < 0){
		/* dns isn't working, don't just dial */
		return -1;
	}
	if(nmx == 0){
		if(debug)
			fprint(2, "mxlookup returns nothing\n");
		return dial(dest, 0, 0, 0);
	}

	/* refuse to honor loopback addresses given by dns.  catch \n too. */
	for(i = 0; i < nmx; i++) {
		ip = mx[i].ip;
		if(strchr(ip, '\n') != nil){
			if(debug)
				fprint(2, "mxlookup ip contains newline\n");
			werrstr("illegal: newline in mail server ip");
			return -1;
		}else if(isloopback(ip)){
			if(debug)
				fprint(2, "mxlookup returns loopback\n");
			werrstr("illegal: domain lists %s as mail server", ip);
			return -1;
		}
	}

	/* sort by preference */
	if(nmx > 1)
		qsort(mx, nmx, sizeof(Mx), compar);

	/* dial each one in turn by name, not ip */
	for(i = 0; i < nmx; i++){
		if (busted(mx[i].host)) {
			if (debug)
				fprint(2, "mxdial skipping busted mx %s\n",
					mx[i].host);
			continue;
		}
		snprint(addr, sizeof(addr), "%s/%s!%s!%s", ds->netdir, ds->proto,
			mx[i].host, ds->service);
		if(debug)
			fprint(2, "mxdial trying %s\n", addr);
		atnotify(timeout, 1);
		/* this was 10 seconds, but oclsc.org at least needs more. */
		alarm(60*1000);
		fd = dial(addr, 0, 0, 0);
		if (debug && fd < 0)
			fprint(2, "dial: %r\n");
		alarm(0);
		atnotify(timeout, 0);
		if(fd >= 0)
			return fd;
	}
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
	strcpy(domain, ds->host);

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
mxlookup1(DS *ds, char *domain)
{
	int i, n, fd, nmx;
	char buf[Maxdomain], dnsname[Maxstring];
	char *fields[4];
	Mx *mxp;

	snprint(dnsname, sizeof dnsname, "%s/dns", ds->netdir);

	fd = open(dnsname, ORDWR);
	if(fd < 0)
		return 0;

	nmx = 0;
	snprint(buf, sizeof buf, "%s mx", ds->host);
	if(debug)
		fprint(2, "sending %s '%s'\n", dnsname, buf);
	/*
	 * don't hang indefinitely in the write to /net/dns.
	 */
	n = timedwrite(fd, buf, strlen(buf), 60*1000);
	if(n < 0){
		rerrstr(buf, sizeof buf);
		if(debug)
			fprint(2, "dns: %s\n", buf);
		if(strstr(buf, "dns failure")){
			/* if dns fails for the mx lookup, we have to stop */
			close(fd);
			return -1;
		}
	} else {
		/*
		 *  get any mx entries
		 *  assumes one record per read
		 */
		seek(fd, 0, 0);
		while(nmx < Nmx && (n = read(fd, buf, sizeof buf-1)) > 0){
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
			nmx++;
		}
		if(debug)
			fprint(2, "dns mx: got %d mx servers\n", nmx);
	}

	/*
	 * no mx record? try name itself.
	 *
	 * BUG? If domain has no dots, then we used to look up ds->host
	 * but return domain instead of ds->host in the list.  Now we return
	 * ds->host.  What will this break?
	 */
	if(nmx == 0){
		mx[0].pref = 1;
		strncpy(mx[0].host, ds->host, sizeof(mx[0].host));
		nmx++;
	}

	/*
	 * look up first ip address of each mx name.
	 * should really look at all addresses.
	 * assumes one record per read.
	 */
	for(i = 0; i < nmx; i++){
		mxp = &mx[i];
		seek(fd, 0, 0);
		snprint(buf, sizeof buf, "%s ip", mxp->host);
		mxp->ip[0] = 0;
		/*
		 * don't hang indefinitely in the write to /net/dns.
		 */
		if(timedwrite(fd, buf, strlen(buf), 60*1000) < 0)
			goto no;
		seek(fd, 0, 0);
		if((n = read(fd, buf, sizeof buf-1)) < 0)
			goto no;
		buf[n] = 0;
		if(getfields(buf, fields, 4, 1, " \t") < 3)
			goto no;
		strncpy(mxp->ip, fields[2], sizeof mxp->ip - 1);
		mxp->ip[sizeof mxp->ip - 1] = '\0';
		continue;

	no:
		/* remove mx[i] and go around again */
		nmx--;
		*mxp = mx[nmx];
		i--;
	}
	close(fd);
	return nmx;
}

static int
compar(void *a, void *b)
{
	return ((Mx*)a)->pref - ((Mx*)b)->pref;
}

/* break up an address to its component parts */
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
			for(p2 = p; *p2 != '/'; p2--)
				;
			*p2++ = 0;
			ds->netdir = ds->buf;
			ds->proto = p2;
		}
		*p = 0;
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
