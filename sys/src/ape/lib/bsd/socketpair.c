/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>

int
socketpair(int domain, int type, int protocol, int *sv)
{
	switch(domain){
	case PF_UNIX:
		return pipe(sv);
	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}
