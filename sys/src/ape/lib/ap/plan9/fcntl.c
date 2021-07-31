#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include "sys9.h"

/*
 * BUG: advisory locking not implemented
 */

#define OFL (O_ACCMODE|O_NONBLOCK|O_APPEND)

int
fcntl(int fd, int cmd, ...)
{
	int arg, i, ans, err;
	Fdinfo *fi;
	va_list va;
	unsigned long oflags;

	err = 0;
	ans = 0;
	va_start(va, cmd);
	arg = va_arg(va, int);
	va_end(va);
	fi = &_fdinfo[fd];
	if(fd<0 || fd>=OPEN_MAX || !(fi->flags&FD_ISOPEN))
		err = EBADF;
	else switch(cmd){
		case F_DUPFD:
			oflags = fi->oflags;
			for(i = (arg>0)? arg : 0; i<OPEN_MAX; i++)
				if(!(_fdinfo[i].flags&FD_ISOPEN))
					break;
			if(i == OPEN_MAX)
				err = EMFILE;
			else {
				ans = _DUP(fd, i);
				if(ans != i){
					if(ans < 0){
						_syserrno();
						err = errno;
					}else
						err = EBADF;
				}else{
					_fdinfo[ans].flags = fi->flags&~FD_CLOEXEC;
					_fdinfo[ans].oflags = oflags;
				}
			}
			break;
		case F_GETFD:
			ans = fi->flags&FD_CLOEXEC;
			break;
		case F_SETFD:
			fi->flags = (fi->flags&~FD_CLOEXEC)|(arg&FD_CLOEXEC);
			break;
		case F_GETFL:
			ans = fi->oflags&OFL;
			break;
		case F_SETFL:
			fi->oflags = (fi->oflags&~OFL)|(arg&OFL);
			break;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			err = EINVAL;
			break;
		}
	if(err){
		errno = err;
		ans = -1;
	}
	return ans;
}
