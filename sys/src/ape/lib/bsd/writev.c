/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>

#include "priv.h"

int
writev(int fd, struct iovec *v, int ent)
{
	int i, n, written;
	char *t, *e, *f;
	char buf[10*1024];

	written = 0;
	t = buf;
	e = buf+sizeof(buf);
	for(;ent ; v++, ent--){
		n = v->iov_len;
		f = v->iov_base;
		while(n > 0){
			i = e-t;
			if(n < i){
				memmove(t, f, n);
				t += n;
				break;
			}
			memmove(t, f, i);
			n -= i;
			f += i;
			i = write(fd, buf, sizeof(buf));
			if(i < 0){
				if(written > 0){
					return written;
				}else{
					_syserrno();
					return -1;
				}
			}
			written += i;
			if(i != sizeof(buf)) {
				return written;
			}
			t = buf;
		}
	}
	i = t - buf;
	if(i > 0){
		n = write(fd, buf, i);
		if(n < 0){
			if(written == 0){
				_syserrno();
				return -1;
			}
		} else
			written += n;
	}
	return written;
}
