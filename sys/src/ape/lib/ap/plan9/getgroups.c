#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
getgroups(int, gid_t [])
{
	errno = EINVAL;
	return -1;
}
