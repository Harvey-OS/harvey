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
	struct stat sb;
	char cd[DIRLEN];

	if(_STAT(filename, cd) < 0){
		_syserrno();
		return NULL;
	}
	_dirtostat(&sb, cd, 0);
	if(S_ISDIR(sb.st_mode) == 0) {
		errno = ENOTDIR;
		return NULL;
	}

	f = open(filename, O_RDONLY);
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
	int f;
	char dname[300];

	d->dd_loc = 0;
	d->dd_size = 0;
	if(!d){
		return;
	}
	/* seeks aren't allowed on directories, so reopen */
	strncpy(dname, _fdinfo[d->dd_fd].name, sizeof(dname));
	close(d->dd_fd);
	 f = open(dname, O_RDONLY);
	if (f < 0) {
		_syserrno();
		return;
	}
	_fdinfo[f].flags |= FD_CLOEXEC;
	d->dd_fd = f;
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
