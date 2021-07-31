#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/*
 * Bug: can't chown in plan9
 */
int
chown(const char *path, uid_t owner, gid_t group)
{
	errno = EPERM;
	return -1;
}
