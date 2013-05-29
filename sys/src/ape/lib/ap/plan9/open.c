#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lib.h"
#include <sys/stat.h>
#include "sys9.h"

/*
 * O_NOCTTY has no effect
 */
int
open(const char *path, int flags, ...)
{
	int n;
	long f;
	int mode;
	Fdinfo *fi;
	va_list va;

	f = flags&O_ACCMODE;
	if(flags&O_CREAT){
		if(access(path, 0) >= 0){
			if(flags&O_EXCL){
				errno = EEXIST;
				return -1;
			}else{
				if((flags&O_TRUNC)&&(flags&(O_RDWR|O_WRONLY)))
					f |= 16;
				n = _OPEN(path, f);
			}
		}else{
			va_start(va, flags);
			mode = va_arg(va, int);
			va_end(va);
			n = _CREATE(path, f, mode&0777);
		}
		if(n < 0)
			_syserrno();
	}else{
		if((flags&O_TRUNC)&&(flags&(O_RDWR|O_WRONLY)))
			f |= 16;
		n = _OPEN(path, f);
		if(n < 0)
			_syserrno();
	}
	if(n >= 0){
		fi = &_fdinfo[n];
		fi->flags = FD_ISOPEN;
		fi->oflags = flags&(O_ACCMODE|O_NONBLOCK|O_APPEND);
		fi->uid = -2;
		fi->gid = -2;
		fi->name = malloc(strlen(path)+1);
		if(fi->name)
			strcpy(fi->name, path);
		if(fi->oflags&O_APPEND)
			_SEEK(n, 0, 2);
	}
	return n;
}
