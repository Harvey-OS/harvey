#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
stat(const char *path, struct stat *buf)
{
	Dir *d;

	if((d = _dirstat(path)) == nil){
		_syserrno();
		return -1;
	}
	_dirtostat(buf, d, 0);
	free(d);

	return 0;
}
