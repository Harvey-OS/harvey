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
	unsigned long x;

	x = in.s_addr;
	sprintf(s, "%d.%d.%d.%d", (x>>24)&0xff, (x>>16)&0xff, (x>>8)&0xff, x&0xff);
	return s;
}
