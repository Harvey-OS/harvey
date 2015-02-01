/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

int
close(int d)
{
	int n;
	Fdinfo *f;

	n = -1;
	f = &_fdinfo[d];
	if(d<0 || d>=OPEN_MAX || !(f->flags&FD_ISOPEN))
		errno = EBADF;
	else{
		if(f->flags&(FD_BUFFERED|FD_BUFFEREDX)) {
			if(f->flags&FD_BUFFERED)
				_closebuf(d);
			f->flags &= ~FD_BUFFERED;
		}
		n = _CLOSE(d);
		if(n < 0)
			_syserrno();
		_fdinfo[d].flags = 0;
		_fdinfo[d].oflags = 0;
		if(_fdinfo[d].name){
			free(_fdinfo[d].name);
			_fdinfo[d].name = 0;
		}
	}
	return n;
}
