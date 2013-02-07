#include "lib.h"
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

static int
seterrno(void)
{
	_syserrno();
	return -1;
}

int
chmod(const char *path, mode_t mode)
{
	Dir d, *dir;

	dir = _dirstat(path);
	if(dir == nil)
		return seterrno();
	_nulldir(&d);
	d.mode = (dir->mode & ~0777) | (mode & 0777);
	free(dir);
	if(_dirwstat(path, &d) < 0)
		return seterrno();
	return 0;
}

int
fchmod(int fd, mode_t mode)
{
	Dir d, *dir;

	dir = _dirfstat(fd);
	if(dir == nil)
		return seterrno();
	_nulldir(&d);
	d.mode = (dir->mode & ~0777) | (mode & 0777);
	free(dir);
	if(_dirfwstat(fd, &d) < 0)
		return seterrno();
	return 0;
}
