#define _POSIX_SOURCE
/* not a posix function, but implemented with posix kill, getpid */

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

int
raise(int sig)
{
	return kill(getpid(), sig);
}
