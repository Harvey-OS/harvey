#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void
abort(void)
{
	kill(getpid(), SIGABRT);
}
