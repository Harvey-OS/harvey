#include "lib.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "sys9.h"
#include "dir.h"

int
rename(const char *from, const char *to)
{
	int n, i;
	char *f, *t;
	Dir d;
	long mode;
	char cd[DIRLEN];

	if(access(to, 0) >= 0){
		if(_REMOVE(to) < 0){
			_syserrno();
			return -1;
		}
	}
	if(_STAT(to, cd) >= 0){
		errno = EEXIST;
		return -1;
	}
	if(_STAT(from, cd) < 0){
		_syserrno();
		return -1;
	}
	convM2D(cd, &d);
	f = strrchr(from, '/');
	t = strrchr(to, '/');
	f = f? f+1 : from;
	t = t? t+1 : to;
	n = 0;
	if(f-from==t-to && strncmp(from, to, f-from)==0){
		/* from and to are in same directory (we miss some cases) */
		i = strlen(t);
		if(i > NAMELEN){
			errno = EINVAL;
			n = -1;
		}else{
			strcpy(d.name, t);
			convD2M(&d, cd);
			if(_WSTAT(from, cd) < 0){
				_syserrno();
				n = -1;
			}
		}
	}else{
		/* different directories: have to copy */
		int ffd, tfd;
		char buf[8192];

		if((ffd = _OPEN(from, 0)) < 0 ||
		   (tfd = _CREATE(to, 1, d.mode)) < 0){
			_CLOSE(ffd);
			_syserrno();
			n = -1;
		}
		while(n>=0 && (n = _READ(ffd, buf, 8192) > 0))
			if(_WRITE(tfd, buf, n) != n){
				_syserrno();
				n = -1;
			}
		_CLOSE(ffd);
		_CLOSE(tfd);
		if(n>0)
			n = 0;
		
	}
	return n;
}
