#include <windows.h>
#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

#include "devip.h"

#ifdef MSVC
#pragma comment(lib, "wsock32.lib")
#endif

#undef listen
#undef accept
#undef bind

void
osipinit(void)
{
	WSADATA wasdat;
	char buf[1024];

	if(WSAStartup(MAKEWORD(1, 1), &wasdat) != 0)
		panic("no winsock.dll");

	gethostname(buf, sizeof(buf));
	kstrdup(&sysname, buf);
}

int
so_socket(int type)
{
	int fd, one;

	switch(type) {
	default:
		error("bad protocol type");
	case S_TCP:
		type = SOCK_STREAM;
		break;
	case S_UDP:
		type = SOCK_DGRAM;
		break;
	}

	fd = socket(AF_INET, type, 0);
	if(fd < 0)
		oserror();

	one = 1;
	if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(one)) > 0){
		oserrstr();
		print("setsockopt: %s\n", up->errstr);
	}

	return fd;
}


void
so_connect(int fd, unsigned long raddr, unsigned short rport)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, rport);
	hnputl(&sin.sin_addr.s_addr, raddr);

	if(connect(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		oserror();
}

void
so_getsockname(int fd, unsigned long *laddr, unsigned short *lport)
{
	int len;
	struct sockaddr_in sin;

	len = sizeof(sin);
	if(getsockname(fd, (struct sockaddr*)&sin, &len) < 0)
		oserror();

	if(sin.sin_family != AF_INET || len != sizeof(sin))
		error("not AF_INET");

	*laddr = nhgetl(&sin.sin_addr.s_addr);
	*lport = nhgets(&sin.sin_port);
}

void
so_listen(int fd)
{
	if(listen(fd, 5) < 0)
		oserror();
}

int
so_accept(int fd, unsigned long *raddr, unsigned short *rport)
{
	int nfd, len;
	struct sockaddr_in sin;

	len = sizeof(sin);
	nfd = accept(fd, (struct sockaddr*)&sin, &len);
	if(nfd < 0)
		oserror();

	if(sin.sin_family != AF_INET || len != sizeof(sin))
		error("not AF_INET");

	*raddr = nhgetl(&sin.sin_addr.s_addr);
	*rport = nhgets(&sin.sin_port);
	return nfd;
}

void
so_bind(int fd, int su, unsigned short port)
{
	int i, one;
	struct sockaddr_in sin;

	one = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0){
		oserrstr();
		print("setsockopt: %s", up->errstr);
	}

	if(su) {
		for(i = 600; i < 1024; i++) {
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_port = i;

			if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) >= 0)	
				return;
		}
		oserror();
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	hnputs(&sin.sin_port, port);

	if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		oserror();
}

int
so_gethostbyname(char *host, char**hostv, int n)
{
	int i;
	char buf[32];
	unsigned char *p;
	struct hostent *hp;

	hp = gethostbyname(host);
	if(hp == 0)
		return 0;

	for(i = 0; hp->h_addr_list[i] && i < n; i++) {
		p = (unsigned char*)hp->h_addr_list[i];
		sprint(buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
		hostv[i] = strdup(buf);
		if(hostv[i] == 0)
			break;
	}
	return i;
}

char*
hostlookup(char *host)
{
	char buf[100];
	uchar *p;
	HOSTENT *he;

	he = gethostbyname(host);
	if(he != 0 && he->h_addr_list[0]) {
		p = (uchar*)he->h_addr_list[0];
		sprint(buf, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);
	} else
		strcpy(buf, host);

	return strdup(buf);
}

int
so_getservbyname(char *service, char *net, char *port)
{
	struct servent *s;

	s = getservbyname(service, net);
	if(s == 0)
		return -1;

	sprint(port, "%d", nhgets(&s->s_port));
	return 0;
}

int
so_send(int fd, void *d, int n, int f)
{
	return send(fd, d, n, f);
}

int
so_recv(int fd, void *d, int n, int f)
{
	return recv(fd, d, n, f);
}
