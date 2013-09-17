/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "priv.h"

int h_errno;

enum
{
	Nname= 6,
};

/*
 *  for inet addresses only
 */
struct hostent*
gethostbyname(const char *name)
{
	int i, t, fd, m;
	char *p, *bp;
	int nn, na;
	unsigned long x;
	static struct hostent h;
	static char buf[1024];
	static char *nptr[Nname+1];
	static char *aptr[Nname+1];
	static char addr[Nname][4];

	h.h_name = 0;
	t = _sock_ipattr(name);

	/* connect to server */
	fd = open("/net/cs", O_RDWR);
	if(fd < 0){
		_syserrno();
		h_errno = NO_RECOVERY;
		return 0;
	}

	/* construct the query, always expect an ip# back */
	switch(t){
	case Tsys:
		snprintf(buf, sizeof buf, "!sys=%s ip=*", name);
		break;
	case Tdom:
		snprintf(buf, sizeof buf, "!dom=%s ip=*", name);
		break;
	case Tip:
		snprintf(buf, sizeof buf, "!ip=%s", name);
		break;
	}

	/* query the server */
	if(write(fd, buf, strlen(buf)) < 0){
		_syserrno();
		h_errno = TRY_AGAIN;
		return 0;
	}
	lseek(fd, 0, 0);
	for(i = 0; i < sizeof(buf)-1; i += m){
		m = read(fd, buf+i, sizeof(buf) - 1 - i);
		if(m <= 0)
			break;
		buf[i+m++] = ' ';
	}
	close(fd);
	buf[i] = 0;

	/* parse the reply */
	nn = na = 0;
	for(bp = buf;;){
		p = strchr(bp, '=');
		if(p == 0)
			break;
		*p++ = 0;
		if(strcmp(bp, "dom") == 0){
			if(h.h_name == 0)
				h.h_name = p;
			if(nn < Nname)
				nptr[nn++] = p;
		} else if(strcmp(bp, "sys") == 0){
			if(nn < Nname)
				nptr[nn++] = p;
		} else if(strcmp(bp, "ip") == 0){
			x = inet_addr(p);
			x = ntohl(x);
			if(na < Nname){
				addr[na][0] = x>>24;
				addr[na][1] = x>>16;
				addr[na][2] = x>>8;
				addr[na][3] = x;
				aptr[na] = addr[na];
				na++;
			}
		}
		while(*p && *p != ' ')
			p++;
		if(*p)
			*p++ = 0;
		bp = p;
	}
	if(nn+na == 0){
		h_errno = HOST_NOT_FOUND;
		return 0;
	}

	nptr[nn] = 0;
	aptr[na] = 0;
	h.h_aliases = nptr;
	h.h_addr_list = aptr;
	h.h_length = 4;
	h.h_addrtype = AF_INET;
	if(h.h_name == 0)
		h.h_name = nptr[0];
	if(h.h_name == 0)
		h.h_name = aptr[0];

	return &h;
}
