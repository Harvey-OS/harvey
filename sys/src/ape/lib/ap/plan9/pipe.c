#include <errno.h>
#include "lib.h"
#include "sys9.h"

int
pipe(int fildes[2])
{
	Fdinfo *fi;
	int i;

	if(!fildes){
		errno = EFAULT;
		return -1;
	}
	if(_PIPE(fildes) < 0)
		_syserrno();
	else
	if(fildes[0] < 0 || fildes[0]>=OPEN_MAX ||
	   fildes[1] < 0 || fildes[1]>=OPEN_MAX) {
		errno = EMFILE;
		return -1;
	}
	for(i = 0; i <=1; i++) {
		fi = &_fdinfo[fildes[i]];
		fi->flags = FD_ISOPEN;
		fi->oflags = O_RDWR;
		fi->uid = 0;	/* none */
		fi->gid = 0;
	}
	return 0;
}
