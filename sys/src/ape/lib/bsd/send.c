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
send(int fd, char *a, int n, int flags)
{
	if(flags & MSG_OOB){
		errno = EOPNOTSUPP;
		return -1;
	}
	return write(fd, a, n);
}

int
recv(int fd, char *a, int n, int flags)
{
	if(flags & MSG_OOB){
		errno = EOPNOTSUPP;
		return -1;
	}
	return read(fd, a, n);
}
