#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include "sys9.h"
#include "dir.h"

int
fstat(int fd, struct stat *buf)
{
	Fdinfo *f;

	char cd[DIRLEN];

	if(_FSTAT(fd, cd) < 0){
		_syserrno();
		return -1;
	}
	_dirtostat(buf, cd);
	f = &_fdinfo[fd];
	if(f->flags&FD_BUFFERED)
		buf->st_size = f->n;
	return 0;
}
