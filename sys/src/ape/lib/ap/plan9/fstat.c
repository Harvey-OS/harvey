#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
fstat(int fd, struct stat *buf)
{
	Dir *d;

	if((d = _dirfstat(fd)) == nil){
		_syserrno();
		return -1;
	}
	_dirtostat(buf, d, &_fdinfo[fd]);
	free(d);
	return 0;
}
