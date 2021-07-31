#include <sys/types.h>
#include <utime.h>
#include <errno.h>
#include "sys9.h"

/*
 * Bug: can't change times in plan 9
 */

int
utime(const char *path, const struct utimbuf *times)
{
	errno = EPERM;
	return -1;
}
