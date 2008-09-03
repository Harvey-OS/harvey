/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

char*
inet_ntoa(struct in_addr in)
{
	static char s[18];
	unsigned char *p;	

	p = (unsigned char*)&in.s_addr;
	snprintf(s, sizeof s, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return s;
}
