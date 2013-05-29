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
	long long nn;
	Dir *db1, *db2, nd;
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
				sprintf(newelem, "%8.8lx%8.8lx", (ulong)(db2->qid.path>>32), (ulong)db2->qid.path);
				_nulldir(&nd);
				nd.name = newelem;
				if(_dirfwstat(i, &nd) < 0)
					p = (char*)path;
				else {
					p = strrchr(path, '/');
					if(p == 0)
						p = newelem; 
					else {
						memmove(newname, path, p-path);
						newname[p-path] = '/';
						strcpy(newname+(p-path)+1, newelem);
						p = newname;
					}
				}
				/* reopen remove on close */
				fd = _OPEN(p, 64|(f->oflags)); 
				if(fd < 0){
					free(db2);
					continue;
				}
				nn = _SEEK(i, 0, 1);
				if(nn < 0)
					nn = 0;
				_SEEK(fd, nn, 0);
				_DUP(fd, i);
				_CLOSE(fd);
				free(db1);
				return 0;
			}
			free(db2);
		}
	}
	n = 0;
	if(fd == -1)
		if((n=_REMOVE(path)) < 0)
			_syserrno();
	free(db1);
	return n;
}
