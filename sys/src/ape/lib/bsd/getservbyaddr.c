#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

struct servent*
getservbyport(int port, char *proto)
{
	char buf[32];

	snprintf(buf, sizeof buf, "%d", port);
	return getservbyname(buf, proto);
}
