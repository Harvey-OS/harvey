#include "lib.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include "sys9.h"
#include "dir.h"

Fdinfo _fdinfo[OPEN_MAX] = {
/* flags     oflags    name      n      eof	buf    next    end     pid */
   FD_ISOPEN,	0,	0,	0,	0,	0,	0,	0,	0,
   FD_ISOPEN,	1,	0,	0,	0,	0,	0,	0,	0,
   FD_ISOPEN,	1,	0,	0,	0,	0,	0,	0,	0,
};


/*
 * For read-buffered fd's, fork a child process that continually
 * reads the fd, and writes it (preceded by a Muxhdr) to a
 * shared pipe.
 */
static int cfd = -1, sfd = -1; /* child and server ends of shared pipe */

typedef struct Muxhdr {
	unsigned short	fd;
	short		count;
} Muxhdr;

/*
 * Start making fd read-buffered: make the pipe, if necessary,
 * and fork the child to write to it.
 * Return -1 if we can't do it.
 */
int
_startbuf(int fd)
{
	int p[2];
	long i, n;
	int pid;
	Muxhdr *h;
	Fdinfo *f;
	char buf[sizeof(Muxhdr) + READMAX];

	if(sfd < 0){
		if(_PIPE(p) < 0){
			errno = EIO;
			return -1;
		}
		cfd = p[0];
		sfd = p[1];
	}
	f = &_fdinfo[fd];
	if((pid = fork()) == 0){
		for(i=0; i<OPEN_MAX; i++)
			if(i!=fd && i!=cfd && (_fdinfo[i].flags&FD_ISOPEN))
				_CLOSE(i);
		_sigclear();
		h = (Muxhdr *)buf;
		h->fd = fd;
		while((n = _READ(fd, buf + sizeof(Muxhdr), READMAX)) >= 0){
			h->count = n;
			i = _WRITE(cfd, buf, sizeof(Muxhdr) + n);
			if(i != sizeof(Muxhdr) + n){
				_exit(0);
			}
		}
		h->count = n;
		_WRITE(cfd, buf, sizeof(Muxhdr));
		_exit(0);
	}
	f->n = 0;
	f->eof = 0;
	f->buf = malloc(2*READMAX);
	f->next = f->buf;
	f->end = f->buf+2*READMAX;
	f->flags |= FD_BUFFERED;
	f->pid = pid;
	return 0;
}

/*
 * When an fd is buffered, the _servebuf routine must be
 * called to read the shared pipe until there is nothing
 * more on it, putting any data found into the buffer in
 * the appropriate Fdinfo structure.
 *
 * _servebuf will return as soon as any data has been read.
 * If noblock, it will return immediately if the shared pipe
 * has nothing on it.
 *
 * returns the number of fds with new data, -1 on error.
 */

/* efficiency hack: know offset of length in stat (see _dirconv) */
#define LOFF (3 * NAMELEN + 5 * sizeof(long))
int
_servebuf(int noblock)
{
	int any;
	long n;
	char cd[DIRLEN];
	Muxhdr *h;
	Fdinfo *f;
	char buf[sizeof(Muxhdr) + READMAX];

	any = 0;
	h = (Muxhdr *)buf;
	for(;;){
		/*
		 * Allowed to block if noblock==0 and any==0.
		 * Otherwise, check use pipe stat to find out
		 * if a read of sfd will return immediately
		 */
		if(noblock || any){
			if(_FSTAT(sfd, cd) < 0){
				errno = EIO;
				return -1;
			}
			if(!(cd[LOFF] || cd[LOFF+1] || cd[LOFF+2] || cd[LOFF+3]))
				return any;
		}
		n = _READ(sfd, buf, sizeof buf);

		/* we expect one read yields data from one write */

		if(n < sizeof(Muxhdr) ||
		   h->fd >= OPEN_MAX || n != h->count + sizeof(Muxhdr)){
			errno = EIO;
			return -1;
		}
		f = &_fdinfo[h->fd];
		n = h->count;
		if(n <= 0)
			f->eof = 1;
		else{
			if(f->end - f->buf - f->n < n){
				f->next -= (long)f->buf;
				f->buf = realloc(f->buf, f->end-f->buf+READMAX);
				f->end = f->buf+(f->end-f->buf)+READMAX;
				f->next += (long)f->buf;
			}
			memcpy(f->buf + f->n, buf + sizeof(Muxhdr), n);
			f->n += n;
		}
		any++;
	}
}
