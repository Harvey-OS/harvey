#include "lib.h"
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include "sys9.h"
#include "dir.h"

int
utime(const char *path, const struct utimbuf *times)
{
	int n;
	char cd[DIRLEN];
	Dir dir;
	time_t curt;

	if(_STAT(path, cd) < 0){
		_syserrno();
		return -1;
	}
	convM2D(cd, &dir);
	if(times == 0) {
		curt = time(0);
		dir.atime = curt;
		dir.mtime = curt;
	} else {
		dir.atime = times->actime;
		dir.mtime = times->modtime;
	}
	convD2M(&dir, cd);
	n = _WSTAT(path, cd);
	if(n < 0)
		_syserrno();
	return n;
}
