#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "lib.h"
#include "sys9.h"

#include <stdio.h>

ssize_t
read(int d, void *buf, size_t nbytes)
{
	int n, noblock, isbuf;
	Fdinfo *f;

	if(d<0 || d>=OPEN_MAX || !((f = &_fdinfo[d])->flags&FD_ISOPEN)){
		errno = EBADF;
		return -1;
	}
	if(nbytes == 0)
		return 0;
	if(buf == 0){
		errno = EFAULT;
		return -1;
	}
	f = &_fdinfo[d];
	noblock = f->oflags&O_NONBLOCK;
	isbuf = f->flags&FD_BUFFERED;
	if(noblock || isbuf){
		if(!isbuf)
			if(_startbuf(d) != 0) {
				errno = EIO;
				return -1;
			}
		while(f->n == 0 && !f->eof){
			if(_servebuf(noblock) <= 0 && noblock){
				errno = EAGAIN;
				return -1;
			}
		}
		if(f->n == 0 && f->eof){
			errno = EPIPE;  /* EOF, really */
			return -1;
		}
		n = (f->n > nbytes)? nbytes : f->n;
		memcpy(buf, f->next, n);
		if((f->n -= n) <= 0)
			f->next = f->buf;
		else
			f->next += n;
	}else{
		n = _READ(d,  buf, nbytes);
		if(n < 0)
			_syserrno();
	}
	return n;
}
