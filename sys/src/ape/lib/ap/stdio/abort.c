#include <signal.h>

void
abort(void)
{
	kill(getpid(), SIGABRT);
}
