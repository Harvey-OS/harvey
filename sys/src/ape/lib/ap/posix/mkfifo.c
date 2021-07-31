#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

int
mkfifo(char *path, mode_t mode)
{
#pragma ref path
#pragma ref mode
	errno = 0;
	return -1;
}
