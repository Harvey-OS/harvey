#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

/*
 * BUG: errno mapping
 */

int
__unlink(const char *path)
{
	int n;

	if((n=_REMOVE(path)) < 0)
		_syserrno();
	return n;
}
