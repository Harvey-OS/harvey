#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
fsync(int fd)
{
	errno = EINVAL;
	return -1;
}
