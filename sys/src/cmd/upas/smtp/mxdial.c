#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

enum
{
	Nmx=	16,
};

typedef struct Mx	Mx;
struct Mx
{
	char host[256];
	int pref;
};
static Mx mx[Nmx];
Ndb *db;

static int	mygetfields(char*, char**, int, char);
static int	mxlookup(char*);
static int	compar(void*, void*);
static int	dnlookup(char*, char*);
/*
 *  take an address and return all the mx entries for it,
 *  most preferred first
 */
int
mxdial(char *dest, int *diff, char *domain)
{
	char *net;
	char *service;
	char *host;
	int fd, i, nmx;
	char buf[1024];

	/* parse address */
	strncpy(buf, dest, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;
	net = buf;
	host = strchr(buf, '!');
	if(host){
		*host++ = 0;
		service = strchr(host, '!');
		if(service)
			*service++ = 0;
	} else {
		host = buf;
		net = 0;
		service = 0;
	}

	/* get domain name if this isn't one */
	if(dnlookup(host, domain) < 0){
		strcpy(domain, host);
		*diff = 0;
		return dial(dest, 0, 0, 0);
	}

	/* get a list of mx entries */
	nmx = mxlookup(domain);
	if(nmx == 0){
		strcpy(domain, host);
		*diff = 0;
		return dial(dest, 0, 0, 0);
	}

	/* sort by preference */
	if(nmx > 1)
		qsort(mx, nmx, sizeof(Mx), compar);

	/* dial each one in turn */
	for(i = 0; i < nmx; i++){
		fd = dial(netmkaddr(mx[i].host, net, service), 0, 0, 0);
		if(fd >= 0){
			if(strcmp(mx[i].host, domain))
				*diff = 1;
			return fd;
		}
	}
	return -1;
}

/*
 *  lookup a domain name for host
 */
static int
dnlookup(char *host, char *domain)
{
	char *attr;
	Ndbtuple *t;
	Ndbs s;

	attr = ipattr(host);
	if(strcmp(attr, "dom") == 0){
		strcpy(domain, host);
		return 0;
	}
	if(db == 0)
		db = ndbopen(0);
	if(db == 0)
		return -1;
	t = ndbgetval(db, &s, attr, host, "dom", domain);
	if(t == 0)
		return -1;
	ndbfree(t);
	return 0;
}

/*
 *  call the dns process and have it try to resolve the mx request
 */
static int
mxlookup(char *host)
{
	int fd, n, nmx;
	char buf[1024];
	char *fields[4];

	fd = open("/net/dns", ORDWR);
	if(fd < 0)
		return 0;

	nmx = 0;
	sprint(buf, "%.*s mx", sizeof(buf)-4, host);
	if(write(fd, buf, strlen(buf)) >= 0){
		seek(fd, 0, 0);
		while(nmx < Nmx && (n = read(fd, buf, sizeof(buf)-1)) > 0){
			buf[n] = 0;
			n = mygetfields(buf, fields, 4, ' ');
			if(n < 3)
				continue;
			strncpy(mx[nmx].host, fields[3], sizeof(mx[n].host)-1);
			mx[nmx].pref = atoi(fields[2]);
			nmx++;
		}
	}
	close(fd);

	return nmx;
}

static int
mygetfields(char *lp, char **fields, int n, char sep)
{
	int i;
	char sep2=0;

	if(sep == ' ')
		sep2 = '\t';
	for(i=0; lp && *lp && i<n; i++){
		if(*lp==sep || *lp==sep2)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && *lp!=sep && *lp!=sep2)
			lp++;
	}
	return i;
}

static int
compar(void *a, void *b)
{
	return ((Mx*)a)->pref - ((Mx*)b)->pref;
}
