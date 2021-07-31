#define  _BSDTIME_EXTENSION
#include "lib.h"
#include <sys/stat.h>
#include <stdlib.h>

Fdinfo _fdinfo[OPEN_MAX];

/*
   called from _envsetup, either with the value of the environment
   variable _fdinfo (from s to se-1), or with s==0 if there was no _fdinfo
*/
void
_fdinit(char *s, char *se)
{
	int i;
	Fdinfo *fi;
	unsigned long fd, fl, ofl;
	char *e;
	struct stat sbuf;

	if(s){
		while(s < se){
			fd = strtoul(s, &e, 10);
			if(s == e)
				break;
			s = e;
			fl = strtoul(s, &e, 10);
			if(s == e)
				break;
			s = e;
			ofl = strtoul(s, &e, 10);
			if(s == e)
				break;
			s = e;
			if(fd < OPEN_MAX){
				fi = &_fdinfo[fd];
				fi->flags = fl;
				fi->oflags = ofl;
/* hack, until all posix apps recompiled: */
				if(_isatty(fd))
					fi->flags |= FD_ISTTY;
			}
		}
	} else {
		for(i = 0; i <= 2; i++) {
			fi = &_fdinfo[i];
			fi->flags = FD_ISOPEN;
			fi->oflags = (i == 0)? O_RDONLY : O_WRONLY;
			if(_isatty(i))
				fi->flags |= FD_ISTTY;
		}
	}
	for(i = 0; i < OPEN_MAX; i++) {
		fi = &_fdinfo[i];
		if(fi->flags&FD_ISOPEN){
			if(fstat(i, &sbuf) >= 0) {
				fi->uid = sbuf.st_uid;
				fi->gid = sbuf.st_gid;
			}
		}
	}
}

