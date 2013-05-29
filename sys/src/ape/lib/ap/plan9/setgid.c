#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/*
 * BUG: never works
 */

int
setgid(gid_t)
{
	errno = EPERM;
	return -1;
}
