#include <signal.h>
#include <errno.h>

/*
 * BUG: doesn't work
 */

int
sigsuspend(sigset_t *)
{
	errno = EINVAL;
	return -1;
}
