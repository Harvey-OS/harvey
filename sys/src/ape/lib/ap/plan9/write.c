#include <errno.h>
#include <unistd.h>
#include "lib.h"
#include "sys9.h"

ssize_t
write(int d, const void *buf, size_t nbytes)
{
	int n;

	if(d<0 || d>=OPEN_MAX || !(_fdinfo[d].flags&FD_ISOPEN)){
		errno = EBADF;
		return -1;
	}
	if(_fdinfo[d].oflags&O_APPEND)
		_SEEK(d, 0, 2);
	n = _WRITE(d, buf, nbytes);
	if(n < 0)
		_syserrno();
	return n;
}
