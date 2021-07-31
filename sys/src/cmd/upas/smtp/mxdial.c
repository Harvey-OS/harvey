#include "common.h"
#include <ndb.h>

enum
{
	Nmx=	16,
	Maxstring=	256,
};

typedef struct Mx	Mx;
struct Mx
{
	char host[256];
	char ip[24];
	int pref;
};
static Mx mx[Nmx];

typedef struct DS	DS;
struct DS {
	/* dist string */
	char	buf[128];
	char	*netdir;
	char	*proto;
	char	*host;
	char	*service;
};

Ndb *db;
extern int debug;

static int	mxlookup(DS*, char*);
static int	mxlookup1(DS*, char*);
static int	compar(void*, void*);
static int	callmx(DS*, char*, char*);
static void	dial_string_parse(char*, DS*);
extern int	cistrcmp(char*, char*);

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

/*
 *  take an address and return all the mx entries for it,
 *  most preferred first
 */
static int
callmx(DS *ds, char *dest, char *domain)
{
	int fd, i, nmx;
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

	/* refuse to honor loopback addresses given by dns */
	for(i = 0; i < nmx; i++){
		if(strcmp(mx[i].ip, "127.0.0.1") == 0){
			if(debug)
				fprint(2, "mxlookup returns loopback\n");
			werrstr("illegal: domain lists 127.0.0.1 as mail server");
			return -1;
		}
	}

	/* sort by preference */
	if(nmx > 1)
		qsort(mx, nmx, sizeof(Mx), compar);

	/* dial each one in turn */
	for(i = 0; i < nmx; i++){
		snprint(addr, sizeof(addr), "%s/%s!%s!%s", ds->netdir, ds->proto,
			mx[i].host, ds->service);
		if(debug)
			fprint(2, "mxdial trying %s\n", addr);
		fd = dial(addr, 0, 0, 0);
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

	if(ds->netdir){
		n = mxlookup1(ds, domain);
	} else {
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
	char buf[1024];
	char dnsname[Maxstring];
	char *fields[4];
	int i, n, fd, nmx;

	snprint(dnsname, sizeof dnsname, "%s/dns", ds->netdir);

	fd = open(dnsname, ORDWR);
	if(fd < 0)
		return 0;

	nmx = 0;
	snprint(buf, sizeof(buf), "%s mx", ds->host);
	if(debug)
		fprint(2, "sending %s '%s'\n", dnsname, buf);
	n = write(fd, buf, strlen(buf));
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
		 */
		seek(fd, 0, 0);
		while(nmx < Nmx && (n = read(fd, buf, sizeof(buf)-1)) > 0){
			buf[n] = 0;
			if(debug)
				fprint(2, "dns mx: %s\n", buf);
			n = getfields(buf, fields, 4, 1, " \t");
			if(n < 4)
				continue;

			if(strchr(domain, '.') == 0)
				strcpy(domain, fields[0]);

			strncpy(mx[nmx].host, fields[3], sizeof(mx[n].host)-1);
			mx[nmx].pref = atoi(fields[2]);
			nmx++;
		}
		if(debug)
			fprint(2, "dns mx; got %d entries\n", nmx);
	}

	/*
	 * no mx record? try name itself.
	 */
	/*
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
	 * look up all ip addresses
	 */
	for(i = 0; i < nmx; i++){
		seek(fd, 0, 0);
		snprint(buf, sizeof buf, "%s ip", mx[i].host);
		mx[i].ip[0] = 0;
		if(write(fd, buf, strlen(buf)) < 0)
			goto no;
		seek(fd, 0, 0);
		if((n = read(fd, buf, sizeof buf-1)) < 0)
			goto no;
		buf[n] = 0;
		if(getfields(buf, fields, 4, 1, " \t") < 3)
			goto no;
		strncpy(mx[i].ip, fields[2], sizeof(mx[i].ip)-1);
		continue;

	no:
		/* remove mx[i] and go around again */
		nmx--;
		mx[i] = mx[nmx];
		i--;
	}
	return nmx;		
}

static int
compar(void *a, void *b)
{
	return ((Mx*)a)->pref - ((Mx*)b)->pref;
}

/* break up an address to its component parts */
static void
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
}
