#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include "sys9.h"
#include "dir.h"

void
_dirtostat(struct stat *s, char *cd)
{
	Dir *d, db;
	int num;
	char *nam, *mem;

	convM2D(cd, &db);
	d = &db;
	s->st_dev = (d->type<<8)|(d->dev&0xFF);
	s->st_ino = d->qid.path;
	s->st_mode = d->mode&0777 |
	   ((d->mode&0x80000000) ? 0040000 : 
	    	((d->type != 'M' && d->type != 'R') ? 0020000: 0100000));
	s->st_nlink = 1;
	s->st_uid = 1;
	s->st_gid = 1;
	s->st_size = d->length;
	s->st_atime = d->atime;
	s->st_mtime = d->mtime;
	s->st_ctime = d->mtime;
	mem = 0;
	nam = db.uid;
	if(_getpw(&num, &nam, &mem))
		s->st_uid = num;
	nam = db.gid;
	if(_getpw(&num, &nam, &mem))
		s->st_gid = num;
}
