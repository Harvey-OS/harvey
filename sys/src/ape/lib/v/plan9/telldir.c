#define _POSIX_SOURCE
#define _RESEARCH_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <libv.h>

off_t
telldir(DIR *d)
{
	off_t n;

	n = lseek(d->dd_fd, SEEK_CUR, 0);
	return n+d->dd_loc;
}

void
seekdir(DIR *d, off_t n)
{
	lseek(d->dd_fd, SEEK_SET, n);
}
