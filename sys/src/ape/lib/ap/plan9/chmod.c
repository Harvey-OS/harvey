#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include "sys9.h"
#include "dir.h"

int
chmod(const char *path, mode_t mode)
{
	Dir d;
	int n;
	char cd[DIRLEN];

	n = -1;
	if(_STAT(path, cd) < 0)
		_syserrno();
	else{
		convM2D(cd, &d);
		d.mode = mode&0777;
		convD2M(&d, cd);
		if(_WSTAT(path, cd) < 0)
			_syserrno();
		else
			n = 0;
	}
	return n;
}

int
fchmod(int fd, mode_t mode)
{
	Dir d;
	int n;
	char cd[DIRLEN];

	n = -1;
	if(_FSTAT(fd, cd) < 0)
		_syserrno();
	else{
		convM2D(cd, &d);
		d.mode = mode&0777;
		convD2M(&d, cd);
		if(_FWSTAT(fd, cd) < 0)
			_syserrno();
		else
			n = 0;
	}
	return n;
}
