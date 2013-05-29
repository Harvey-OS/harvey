#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
fsync(int)
{
	errno = EINVAL;
	return -1;
}
