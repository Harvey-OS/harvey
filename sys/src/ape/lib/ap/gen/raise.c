/* not a posix function, but implemented with posix kill, getpid */

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

int
raise(int sig)
{
	return kill(getpid(), sig);
}
