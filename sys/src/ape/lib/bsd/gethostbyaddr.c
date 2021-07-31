/* posix */
#include <sys/types.h>
#include <unistd.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int h_errno;

struct hostent*
gethostbyaddr(char *addr, int len, int type)
{
	struct in_addr x;

	if(type != AF_INET){
		h_errno = NO_RECOVERY;
		return 0;
	}

	x.s_addr = (addr[0]<<24)|(addr[1]<<16)|(addr[2]<<8)|addr[3];

	return gethostbyname(inet_ntoa(x));
}
