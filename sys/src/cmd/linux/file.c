#include <u.h>
#include <libc.h>

#include "dat.h"
#include "fns.h"

/* Values for the second argument to `fcntl'.  */
#define F_DUPFD         0       /* Duplicate file descriptor.  */
#define F_GETFD         1       /* Get file descriptor flags.  */
#define F_SETFD         2       /* Set file descriptor flags.  */
#define F_GETFL         3       /* Get file status flags.  */
#define F_SETFL         4       /* Set file status flags.  */
#define F_GETLK        5       /* Get record locking info.  */
#define F_SETLK        6       /* Set record locking info (non-blocking).  */
#define F_SETLKW       7       /* Set record locking info (blocking).  */
/* Not necessary, we always have 64-bit offsets.  */
#define F_GETLK64      5       /* Get record locking info.  */
#define F_SETLK64      6       /* Set record locking info (non-blocking).  */
#define F_SETLKW64     7       /* Set record locking info (blocking).  */


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

int linuxfcntl(int fd, int cmd, void* arg)
{
	int ret;

	switch (cmd) {
	case F_SETLKW:
		ret = 0;
		break;
	}
	return ret;
}
