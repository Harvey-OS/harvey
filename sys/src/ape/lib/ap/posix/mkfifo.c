#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

int
mkfifo(char *, mode_t)
{
	errno = 0;
	return -1;
}
