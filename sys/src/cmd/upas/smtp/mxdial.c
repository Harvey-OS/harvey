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
	if(nmx == 0){
		if(debug)
			fprint(2, "mxlookup returns nothing\n");
		return dial(dest, 0, 0, 0);
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
		if(n <= 0) {
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
	int n, fd, nmx;
	char **mynames;

	snprint(dnsname, sizeof dnsname, "%s/dns", ds->netdir);

	fd = open(dnsname, ORDWR);
	if(fd < 0)
		return 0;

	nmx = 0;
	snprint(buf, sizeof(buf), "%s mx", ds->host);
	if(debug)
		fprint(2, "sending %s '%s'\n", dnsname, buf);
	if(write(fd, buf, strlen(buf)) >= 0){
		/*
		 *  get any mx entries
		 */
		seek(fd, 0, 0);
		while(nmx < Nmx && (n = read(fd, buf, sizeof(buf)-1)) > 0){
			buf[n] = 0;
			n = getfields(buf, fields, 4, 1, " \t");
			if(n < 4)
				continue;

			if(strchr(domain, '.') == 0)
				strcpy(domain, fields[0]);

			strncpy(mx[nmx].host, fields[3], sizeof(mx[n].host)-1);
			mx[nmx].pref = atoi(fields[2]);
			nmx++;
		}
	}
	close(fd);

	if(nmx){
		/* ignore the mx if we are one of the systems */
		mynames = sysnames_read();
		if(mynames == 0)
			return nmx;
		for(; nmx && *mynames; mynames++)
			for(n = 0; n < nmx; n++)
				if(cistrcmp(*mynames, mx[n].host) == 0)
					nmx = 0;
		if(nmx)
			return nmx;
	}


	/*
	 *  no mx, look to see if it has an ip address
	 */
	fd = open(dnsname, ORDWR);
	if(fd < 0)
		return 0;
	snprint(buf, sizeof(buf), "%s ip", ds->host);
	if(debug)
		fprint(2, "sending %s '%s'\n", dnsname, buf);
	if(write(fd, buf, strlen(buf)) >= 0){
		seek(fd, 0, 0);
	
		if((n = read(fd, buf, sizeof(buf)-1)) > 0){
			buf[n] = 0;
			n = getfields(buf, fields, 4, 1, " \t");
			if(n >= 3){
				if(strchr(domain, '.') == 0)
					strcpy(domain, fields[0]);
	
				strncpy(mx[nmx].host, fields[0], sizeof(mx[n].host)-1);
				mx[nmx].pref = 1;
				nmx++;
			}
		}
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
