#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
ftruncate(int fd, off_t length)
{
	errno = EINVAL;
	return -1;
}
