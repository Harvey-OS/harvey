/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	Fdinfo *fi, *fans;
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
			if(fi->flags&(FD_BUFFERED|FD_BUFFEREDX)){
				err = EGREG;	/* dup of buffered fd not implemented */
				break;
			}
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
					fans = &_fdinfo[ans];
					fans->flags = fi->flags&~FD_CLOEXEC;
					fans->oflags = oflags;
					fans->uid = fi->uid;
					fans->gid = fi->gid;
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
