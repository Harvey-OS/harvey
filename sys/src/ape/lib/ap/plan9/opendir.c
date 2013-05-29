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
	Dir *d9;

	if((d9 = _dirstat(filename)) == nil){
		_syserrno();
		return NULL;
	}
	_dirtostat(&sb, d9, 0);
	free(d9);
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
	d->dd_buf = (char *)d + sizeof(DIR);
	d->dd_fd = f;
	d->dd_loc = 0;
	d->dd_size = 0;
	d->dirs = nil;
	d->dirsize = 0;
	d->dirloc = 0;
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
	free(d->dirs);
	free(d);
	return 0;
}

void
rewinddir(DIR *d)
{
	if(!d)
		return;
	d->dd_loc = 0;
	d->dd_size = 0;
	d->dirsize = 0;
	d->dirloc = 0;
	free(d->dirs);
	d->dirs = nil;
	if(_SEEK(d->dd_fd, 0, 0) < 0){
		_syserrno();
		return;
	}
}

struct dirent *
readdir(DIR *d)
{
	int i;
	struct dirent *dr;
	Dir *dirs;

	if(!d){
		errno = EBADF;
		return NULL;
	}
	if(d->dd_loc >= d->dd_size){
		if(d->dirloc >= d->dirsize){
			free(d->dirs);
			d->dirs = NULL;
			d->dirsize = _dirread(d->dd_fd, &d->dirs);
			d->dirloc = 0;
		}
		if(d->dirsize < 0) {	/* malloc or read failed in _dirread? */
			free(d->dirs);
			d->dirs = NULL;
		}
		if(d->dirs == NULL)
			return NULL;

		dr = (struct dirent *)d->dd_buf;
		dirs = d->dirs;
		for(i=0; i<DBLOCKSIZE && d->dirloc < d->dirsize; i++){
			strncpy(dr[i].d_name, dirs[d->dirloc++].name, MAXNAMLEN);
			dr[i].d_name[MAXNAMLEN] = 0;
		}
		d->dd_loc = 0;
		d->dd_size = i*sizeof(struct dirent);
	}
	dr = (struct dirent*)(d->dd_buf+d->dd_loc);
	d->dd_loc += sizeof(struct dirent);
	return dr;
}
