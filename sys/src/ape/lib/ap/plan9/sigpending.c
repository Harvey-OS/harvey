#include <signal.h>

/*
 * BUG: don't keep track of these
 */
int
sigpending(sigset_t *set)
{
	*set = 0;
	return 0;
}
