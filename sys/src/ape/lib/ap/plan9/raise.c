#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int raise(int sig)
{
	kill(getpid(), sig);
	return 0;
}
