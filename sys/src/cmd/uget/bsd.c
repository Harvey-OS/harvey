#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void
hnputl(void *p, unsigned long v)
{
	unsigned char *a;

	a = p;
	a[0] = v>>24;
	a[1] = v>>16;
	a[2] = v>>8;
	a[3] = v;
}

void
hnputs(void *p, unsigned short v)
{
	unsigned char *a;

	a = p;
	a[0] = v>>8;
	a[1] = v;
}

unsigned long
nhgetl(void *p)
{
	unsigned char *a;
	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

unsigned short
nhgets(void *p)
{
	unsigned char *a;
	a = p;
	return (a[0]<<8)|(a[1]<<0);
}


int
tcpconnect(char *dialnet, char *addr, int port, char *net)
{
	int s;
	struct sockaddr_in sin;
	struct hostent *hp;

        hp = gethostbyname(addr);
        if(hp == 0)
                return 0;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		return -1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, port);
	memmove(&sin.sin_addr.s_addr, hp->h_addr_list[0], sizeof(sin.sin_addr.s_addr));

        if(connect(s, &sin, sizeof(sin)) < 0){
		close(s);
                return -1;
	}

	return s;
}

int
tcpannounce(char *net, unsigned char *addr, int *portp)
{
	int s;
	struct sockaddr_in sin;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		return -1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, *portp);

	if(bind(s, &sin, sizeof(sin)) < 0){
		close(s);
		return -1;
	}
	if(listen(s, 1) < 0){
		close(s);
		return -1;
	}

	return s;
}

int
tcpaccept(int afd, unsigned char *raddr, int *rport)
{
	int len, nfd;
	struct sockaddr_in sin;

	len = sizeof(sin);
	nfd = accept(afd, &sin, &len);
	if(nfd < 0)
		return -1;

	if(sin.sin_family != AF_INET || len != sizeof(sin)){
		close(nfd);
		return -1;
	}

	if(raddr)
		memmove(raddr, &sin.sin_addr.s_addr, sizeof(sin.sin_addr.s_addr));
	if(rport)
		*rport = nhgets(&sin.sin_port);

	return nfd;
}

void
tcpdenounce(int afd)
{
	close(afd);
}

char*
sysname(void)
{
	static char sys[64];
	char *p;

	gethostname(sys, sizeof(sys));
	p = strchr(sys, '.');
	if(p)
		*p = 0;
	return sys;
}

char*
domainname(void)
{
	static char sys[64];

	gethostname(sys, sizeof(sys));
	return sys;
}
