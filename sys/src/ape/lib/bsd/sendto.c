/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>

#include "priv.h"

int
sendto(int fd, void *a, int n, int flags,
	void *to, int tolen)
{
	/* actually, should do connect if not done already */
	return send(fd, a, n, flags);
}

int
recvfrom(int fd, void *a, int n, int flags,
	void *from, int *fromlen)
{
	if(getsockname(fd, from, fromlen) < 0)
		return -1;
	return recv(fd, a, n, flags);
}
