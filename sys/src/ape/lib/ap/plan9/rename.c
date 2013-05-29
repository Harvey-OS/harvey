#include "lib.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
rename(const char *from, const char *to)
{
	int n;
	char *f, *t;
	Dir *d, nd;

	if(access(to, 0) >= 0){
		if(_REMOVE(to) < 0){
			_syserrno();
			return -1;
		}
	}
	if((d = _dirstat(to)) != nil){
		free(d);
		errno = EEXIST;
		return -1;
	}
	if((d = _dirstat(from)) == nil){
		_syserrno();
		return -1;
	}
	f = strrchr(from, '/');
	t = strrchr(to, '/');
	f = f? f+1 : (char *)from;
	t = t? t+1 : (char *)to;
	n = 0;
	if(f-from==t-to && strncmp(from, to, f-from)==0){
		/* from and to are in same directory (we miss some cases) */
		_nulldir(&nd);
		nd.name = t;
		if(_dirwstat(from, &nd) < 0){
			_syserrno();
			n = -1;
		}
	}else{
		/* different directories: have to copy */
		int ffd, tfd;
		char buf[8192];

		tfd = -1;
		if((ffd = _OPEN(from, 0)) < 0 ||
		   (tfd = _CREATE(to, 1, d->mode)) < 0){
			_CLOSE(ffd);
			_syserrno();
			n = -1;
		}
		while(n>=0 && (n = _READ(ffd, buf, 8192)) > 0)
			if(_WRITE(tfd, buf, n) != n){
				_syserrno();
				n = -1;
			}
		_CLOSE(ffd);
		_CLOSE(tfd);
		if(n>0)
			n = 0;
		if(n == 0) {
			if(_REMOVE(from) < 0){
				_syserrno();
				return -1;
			}
		}
	}
	free(d);
	return n;
}
