#include "mothra.h"
#include <bio.h>
#include <ndb.h>
#include <ip.h>

enum {
	Nnetdir=	3*NAMELEN,	/* max length of network directory paths */
	Ndialstr=	64,		/* max length of dial strings */
};

int
tcpconnect(char *dialnet, char *addr, int port, char *net)
{
	char ndir[Nnetdir];
	char service[10];
	char *p;
	int fd;

	snprint(service, sizeof(service), "%d", port);
	fd = dial(netmkaddr(addr, dialnet, service), 0, ndir, 0);
	if(fd < 0)
		return -1;

	if(net) {
		/* remember network for the data connections */
		p = strrchr(ndir, '/');
		if(p == 0){
			fprint(2, "dial is out of date\n");
			return 0;
		}
		*p = 0;
		strcpy(net, ndir);
	}

	return fd;
}

typedef struct Announcement Announcement;
struct Announcement {
	Announcement *next;
	int fd;
	char dir[Nnetdir];
};

static Announcement *annou;

int
tcpannounce(char *net, uchar *addr, int *portp)
{
	char buf[Ndialstr];
	char netdir[Nnetdir];
	char newdir[Nnetdir];
	int fd;
	char *p;
	Announcement *a;

	/* get a channel to listen on, let kernel pick the port number */
	sprint(buf, "%s!*!%d", net, *portp);
	fd = announce(buf, netdir);
	if(fd < 0){
		fprint(2, "can't listen for ftp callback on %s: %r\n", buf);
		return -1;
	}

	/* get the local address and port number */
	sprint(newdir, "%s/local", netdir);
	readfile(newdir, buf, sizeof buf);
	p = strchr(buf, '!')+1;
	parseip(addr, buf);
	*portp = atoi(p);

	/* remember directory under a rock */
	a = emalloc(sizeof(*a));
	strcpy(a->dir, netdir);
	a->fd = fd;
	a->next = annou;
	annou = a;

	return fd;
}

int
tcpaccept(int afd, uchar*, int*)
{
	Announcement *a;
	int cfd, dfd;
	char newdir[Nnetdir];

	for(a = annou; a; a = a->next)
		if(a->fd == afd)
			break;
	if(a == 0){
		werrstr("bad use of fd");
		return -1;
	}

	/* wait for a new call */
	cfd = listen(a->dir, newdir);
	if(cfd < 0){
		fprint(2, "active mode connect failed: %r\n");
		return -1;
	}

	/* open the data connection and close the control connection */
	dfd = accept(cfd, newdir);
	close(cfd);
	return dfd;
}

void
tcpdenounce(int afd)
{
	Announcement *a, **l;

	for(l = &annou; *l; l = &(*l)->next)
		if((*l)->fd == afd)
			break;
	if(*l == 0)
		return;

	a = *l;
	*l = a->next;
	close(afd);
	free(a);
}

char*
sysname(void)
{
	static char sys[Ndbvlen];
	char *p;

	p = readfile("/dev/sysname", sys, sizeof(sys));
	if(p == nil)
		return "unknown";
	return p;
}

char*
domainname(void)
{
	static char domain[Ndbvlen];
	Ndbtuple *t;

	if(*domain)
		return domain;

	t = csgetval(0, "sys", sysname(), "dom", domain);
	if(t){
		ndbfree(t);
		return domain;
	} else
		return sysname();
}

Tm*
p9gmtime(long clock)
{
	return gmtime(clock);
}
