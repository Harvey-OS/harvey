#include "lib.h"
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
utime(const char *path, const struct utimbuf *times)
{
	int n;
	Dir *d;
	time_t curt;

	if((d = _dirstat(path)) == nil){
		_syserrno();
		return -1;
	}
	if(times == 0) {
		curt = time(0);
		d->atime = curt;
		d->mtime = curt;
	} else {
		d->atime = times->actime;
		d->mtime = times->modtime;
	}
	n = _dirwstat(path, d);
	if(n < 0)
		_syserrno();
	free(d);
	return n;
}
