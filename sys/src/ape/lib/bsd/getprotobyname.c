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

extern int h_errno;

enum
{
	Nname= 6,
};

static struct protoent r;

struct protoent *getprotobyname(const char *name) {
	int fd, i, m;
	char *p, *bp;
	int nn, na;
	static char buf[1024], proto[1024];
	static char *nptr[Nname+1];

	/* connect to server */
	fd = open("/net/cs", O_RDWR);
	if(fd < 0){
		_syserrno();
		h_errno = NO_RECOVERY;
		return 0;
	}

	/* construct the query, always expect a protocol# back */
	snprintf(buf, sizeof buf, "!protocol=%s ipv4proto=*", name);

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
		if(strcmp(bp, "protocol") == 0){
			if(!nn)
				r.p_name = p;
			if(nn < Nname)
				nptr[nn++] = p;
		} else if(strcmp(bp, "ipv4proto") == 0){
			r.p_proto = atoi(p);
			na++;
		}
		while(*p && *p != ' ')
			p++;
		if(*p)
			*p++ = 0;
		bp = p;
	}
	nptr[nn] = 0;
	r.p_aliases = nptr;
	if (nn+na == 0)
		return 0;

	return &r;
}
