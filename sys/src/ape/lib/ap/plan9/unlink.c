#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sys9.h"
#include "dir.h"


/*
 * BUG: errno mapping
 */

int
unlink(const char *path)
{
	int n, i, fd;
	Dir db1, db2;
	Fdinfo *f;
 	char *p, cd[DIRLEN], newname[PATH_MAX];

	/* if the file is already open, make it close-on-exec (and rename to qid) */
	if(_STAT(path, cd) < 0){
		_syserrno();
		return -1;
	}
	convM2D(cd, &db1);
	fd = -1;
	for(i=0, f = _fdinfo;i < OPEN_MAX; i++, f++) {
		if((f->flags&FD_ISOPEN) && _FSTAT(i, cd) >= 0) {
			convM2D(cd, &db2);
			if(db1.qid.path == db2.qid.path &&
			   db1.qid.vers == db2.qid.vers &&
			   db1.type == db2.type &&
			   db1.dev == db2.dev) {
				memset(db2.name, 0, NAMELEN);
				sprintf(db2.name, "%.8x", db2.qid.path);
				convD2M(&db2, cd);
				if(_FWSTAT(i, cd) < 0)
					p = (char*)path;
				else {
					p = strrchr(path, '/');
					if(p == 0)
						p = db2.name; 
					else {
						*p = '\0';
						sprintf(newname, "%s/%.8x", path, db2.qid.path);
						p = newname;
					}
				}
				/* reopen remove on close */
				fd = _OPEN(p, 64|(f->oflags)); 
				if(fd < 0)
					continue;
				n = _OSEEK(i, 0, 1);
				if(n < 0)
					n = 0;
				_OSEEK(fd, n, 0);
				_DUP(fd, i);
				_CLOSE(fd);
				return 0;
			}
		}
	}
	if(fd == -1)
		if((n=_REMOVE(path)) < 0)
			_syserrno();
	return n;
}
