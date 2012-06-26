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
	long long n;
	int flags;

	flags = _fdinfo[d].flags;
	if(flags&(FD_BUFFERED|FD_BUFFEREDX|FD_ISTTY)) {
		errno = ESPIPE;
		return -1;
	}
	n = _SEEK(d, offset, whence);
	if(n < 0)
		_syserrno();
	return n;
}
