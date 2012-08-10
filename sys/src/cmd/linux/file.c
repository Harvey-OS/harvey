#include <u.h>
#include <libc.h>

#include "dat.h"
#include "fns.h"

int linuxstat(char *path, Linuxstat *buf)
{
	Dir *d;
	d = dirstat(path);
	if (d == nil)
		return -1;
	buf->st_mode = (int)d->mode;
	buf->st_size = (u64int)d->length;
	buf->st_blksize = 512; // whatever
	buf->st_blocks = buf->st_size / 512;
	buf->st_atime = d->atime;
	buf->st_mtime = d->mtime;
	buf->st_ctime = d->mtime;
	return 0;
}
