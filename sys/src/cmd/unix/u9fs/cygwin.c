/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* compatability layer for u9fs support on CYGWIN */

#include <unistd.h>
#include <errno.h>

ssize_t
pread(int fd, void *p, size_t n, off_t off)
{
	off_t ooff;
	int oerrno;

	if ((ooff  = lseek(fd, off, SEEK_SET)) == -1)
		return -1;

	n = read(fd, p, n);

	oerrno = errno;
	lseek(fd, ooff, SEEK_SET);
	errno = oerrno;

	return n;
}

ssize_t
pwrite(int fd, const void *p, size_t n, off_t off)
{
	off_t ooff;
	int oerrno;

	if ((ooff  = lseek(fd, off, SEEK_SET)) == -1)
		return -1;

	n = write(fd, p, n);

	oerrno = errno;
	lseek(fd, ooff, SEEK_SET);
	errno = oerrno;

	return n;
}

int
setreuid(int ruid, int euid)
{
	if (ruid != -1)
		if (setuid(ruid) == -1)
			return(-1);
	if (euid != -1)
		if (seteuid(euid) == -1)
			return(-1);
}

int
setregid(int rgid, int egid)
{
	if (rgid != -1)
		if (setgid(rgid) == -1)
			return(-1);
	if (egid != -1)
		if (setegid(egid) == -1)
			return(-1);
}
