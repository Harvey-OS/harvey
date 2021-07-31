#include "lib.h"
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
chmod(const char *path, mode_t mode)
{
	Dir d;

	_nulldir(&d);
	d.mode = mode & 0777;
	if(_dirwstat(path, &d) < 0){
		_syserrno();	
		return -1;
	}
	return 0;
}

int
fchmod(int fd, mode_t mode)
{
	Dir d;

	_nulldir(&d);
	d.mode = mode & 0777;
	if(_dirfwstat(fd, &d) < 0){
		_syserrno();	
		return -1;
	}
	return 0;
}
