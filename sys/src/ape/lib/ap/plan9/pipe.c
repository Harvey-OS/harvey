#include <errno.h>
#include "lib.h"
#include "sys9.h"

int
pipe(int fildes[2])
{
	if(!fildes){
		errno = EFAULT;
		return -1;
	}
	if(_PIPE(fildes) < 0)
		_syserrno();
	else if(fildes[0] < 0 || fildes[0]>=OPEN_MAX || fildes[1] < 0 || fildes[1]>=OPEN_MAX){
		errno = EMFILE;
		return -1;
	}
	_fdinfo[fildes[0]].flags = FD_ISPIPE|FD_ISOPEN;
	_fdinfo[fildes[1]].flags = FD_ISPIPE|FD_ISOPEN;
	_fdinfo[fildes[0]].oflags = O_RDONLY;
	_fdinfo[fildes[1]].oflags = O_WRONLY;
	return 0;
}
