#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"


/*
 * BUG: errno mapping
 */

int
unlink(const char *path)
{
	int n, i, fd;
	Dir *db1, *db2;
	Fdinfo *f;
 	char *p, newname[PATH_MAX], newelem[32];

	/* if the file is already open, make it close-on-exec (and rename to qid) */
	if((db1 = _dirstat(path)) == nil) {
		_syserrno();
		return -1;
	}
	fd = -1;
	for(i=0, f = _fdinfo;i < OPEN_MAX; i++, f++) {
		if((f->flags&FD_ISOPEN) && (db2=_dirfstat(i)) != nil) {
			if(db1->qid.path == db2->qid.path &&
			   db1->qid.vers == db2->qid.vers &&
			   db1->type == db2->type &&
			   db1->dev == db2->dev) {
				sprintf(newelem, "%.8lux%.8lux", (ulong)(db2->qid.path>>32), (ulong)db2->qid.path);
				db2->name = newelem;
				if(_dirfwstat(i, db2) < 0)
					p = (char*)path;
				else {
					p = strrchr(path, '/');
					if(p == 0)
						p = newelem; 
					else {
						*p = '\0';
						sprintf(newname, "%s/%.8x", path, newelem);
						p = newname;
					}
				}
				/* reopen remove on close */
				fd = _OPEN(p, 64|(f->oflags)); 
				if(fd < 0){
					free(db2);
					continue;
				}
				n = _OSEEK(i, 0, 1);
				if(n < 0)
					n = 0;
				_OSEEK(fd, n, 0);
				_DUP(fd, i);
				_CLOSE(fd);
				free(db1);
				return 0;
			}
			free(db2);
		}
	}
	if(fd == -1)
		if((n=_REMOVE(path)) < 0)
			_syserrno();
	free(db1);
	return n;
}
