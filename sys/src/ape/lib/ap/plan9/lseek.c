#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

/*
 * BUG: errno mapping
 */
off_t
lseek(int d, off_t offset, int whence)
{
	int n;

	n = _SEEK(d, offset, whence);
	if(n < 0)
		_syserrno();
	return n;
}
