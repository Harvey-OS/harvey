#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include "sys9.h"
#include "dir.h"

int
stat(const char *path, struct stat *buf)
{
	char cd[DIRLEN];

	if(_STAT(path, cd) < 0){
		_syserrno();
		return -1;
	}
	_dirtostat(buf, cd);
	return 0;
}
