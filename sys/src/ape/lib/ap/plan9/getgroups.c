#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
getgroups(int gidsize, gid_t grouplist[])
{
	errno = EINVAL;
	return -1;
}
