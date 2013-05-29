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
