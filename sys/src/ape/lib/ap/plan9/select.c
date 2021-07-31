#define  _SELECT_EXTENSION
#define  _BSDTIME_EXTENSION
#include <sys/time.h>
#include <select.h>
#include <errno.h>
#include "lib.h"
#include "sys9.h"

#include <stdio.h>
/* assume FD_SETSIZE is 96 */

#define FD_ANYSET(p)	((p)->fds_bits[0] || (p)->fds_bits[1] || (p)->fds_bits[2])

int
select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout)
{
	int n, i, tmp, t;
	Fdinfo *f;
	fd_set rwant, ewant;

	n = 0;
	if(wfds && FD_ANYSET(wfds)){
		/* count how many are set: they'll all be returned as ready */
		for(i = 0; i<nfds; i++)
			if(FD_ISSET(i, wfds)) {
				if(_fdinfo[i].eof) {
					errno = EBADF;
					return -1;
				}
				n++;
			}
	}
	FD_ZERO(&rwant);
	if(rfds && FD_ANYSET(rfds)){
		for(i = 0; i< nfds; i++)
			if(FD_ISSET(i, rfds)){
				f = &_fdinfo[i];
				if(f->eof) {
					errno = EBADF;
					return -1;
				}
				if(!(f->flags&(FD_BUFFERED|FD_BUFFEREDX)))
					if((f->flags&FD_BUFFEREDX) || _startbuf(i) != 0) {
						errno = EIO;
						return -1;
					}
				if(f->n > 0)
					n++;
				else{
					FD_CLR(i, rfds);
					FD_SET(i, &rwant);
				}
			}
	}
	FD_ZERO(&ewant);
	if(efds && FD_ANYSET(efds)){
		for(i = 0; i< nfds; i++)
			if(FD_ISSET(i, efds)){
				f = &_fdinfo[i];
				if(f->eof) {
					errno = EBADF;
					return -1;
				}
				if((f->flags&FD_BUFFERED) && f->n == 0 && f->eof)
					n++;
				else{
					FD_CLR(i, efds);
					FD_SET(i, &ewant);
				}
			}
	}
	if(n == 0 && (FD_ANYSET(&rwant) || FD_ANYSET(&ewant))){
		if(timeout)
			t = timeout->tv_sec*1000 + (timeout->tv_usec+999)/1000;
		for(;;){
			tmp = _servebuf(timeout!=0); /* no block if timeout */
			if(tmp < 0){
				n = -1;
				errno = -tmp;
				break;
			}else if(tmp > 0){
				/*
				 * check for action on wanted fds
				 */
				for(i=0; i<nfds; i++){
					tmp = _fdinfo[i].n;
					if(FD_ISSET(i, &rwant) && tmp != 0){
						FD_SET(i, rfds);
						n++;
					}
					if(FD_ISSET(i, &ewant) && tmp == 0 &&
					   _fdinfo[i].eof){
						FD_SET(i, efds);
						n++;
					}
				}
				if(n)
					break;
			}
			if(timeout){
				if(t <= 0)
					break;
				_SLEEP(100); /* milliseconds */
				t -= 100;
			}
		}
	}
	return n;
}
