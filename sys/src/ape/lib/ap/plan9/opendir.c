#include "lib.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "sys9.h"
#include "dir.h"

#define DBLOCKSIZE 20

DIR *
opendir(const char *filename)
{
	int f;
	DIR *d;

	f = __open(filename, O_RDONLY);
	if(f < 0){
		_syserrno();
		return NULL;
	}
	_fdinfo[f].flags |= FD_CLOEXEC;
	d = (DIR *)malloc(sizeof(DIR) + DBLOCKSIZE*sizeof(struct dirent));
	if(!d){
		errno = ENOMEM;
		return NULL;
	}
	d->dd_buf = ((char *)d) + sizeof(DIR);
	d->dd_fd = f;
	d->dd_loc = 0;
	d->dd_size = 0;
	return d;
}

int
closedir(DIR *d)
{
	if(!d){
		errno = EBADF;
		return -1;
	}
	if(close(d->dd_fd) < 0)
		return -1;
	free(d);
	return 0;
}

void
rewinddir(DIR *d)
{
	if(!d){
		return;
	}
	lseek(d->dd_fd, SEEK_SET, 0);
	d->dd_loc = 0;
	d->dd_size = 0;
}

struct dirent *
readdir(DIR *d)
{
	int i, n;
	struct dirent *dr;
	Dir dirs[DBLOCKSIZE];
	Dir td;

	if(!d){
		errno = EBADF;
		return NULL;
	}
	if(d->dd_loc >= d->dd_size){
		n = read(d->dd_fd, (char *)dirs, sizeof dirs);
		if(n <= 0)
			return NULL;
		n = n/sizeof(Dir);
		dr = (struct dirent *)(d->dd_buf);
		for(i=0; i<n; i++, dr++){
			convM2D((char *)&dirs[i], &td);
			strncpy(dr->d_name, td.name, MAXNAMLEN);
			dr->d_name[MAXNAMLEN] = 0;
		}
		d->dd_loc = 0;
		d->dd_size = n*sizeof(struct dirent);
	}
	dr = (struct dirent*)(d->dd_buf+d->dd_loc);
	d->dd_loc += sizeof(struct dirent);
	return dr;
}
