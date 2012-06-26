#include "lib.h"
#include "sys9.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "dir.h"

int
chown(const char *path, uid_t owner, gid_t group)
{
	int num;
	Dir d;

	_nulldir(&d);

	/* find owner, group */
	d.uid = nil;
	num = owner;
	if(!_getpw(&num, &d.uid, 0)) {
		errno = EINVAL;
		return -1;
	}

	d.gid = nil;
	num = group;
	if(!_getpw(&num, &d.gid, 0)) {
		errno = EINVAL;
		return -1;
	}

	if(_dirwstat(path, &d) < 0){
		_syserrno();
		return -1;
	}
	return 0;
}
