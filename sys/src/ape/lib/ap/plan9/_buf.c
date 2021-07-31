#define  _BSDTIME_EXTENSION
#define _LOCK_EXTENSION
#include "lib.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <lock.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include "sys9.h"

typedef struct Muxseg {
	Lock	lock;			/* for mutual exclusion access to buffer variables */
	int	curfds;			/* number of fds currently buffered */
	int	selwait;		/* true if selecting process is waiting */
	int	waittime;		/* time for timer process to wait */
	fd_set	rwant;			/* fd's that select wants to read */
	fd_set	ewant;			/* fd's that select wants to know eof info on */
	Muxbuf	bufs[INITBUFS];		/* can grow, via segbrk() */
} Muxseg;

#define MUXADDR ((void*)0x6000000)
static Muxseg *mux = 0;			/* shared memory segment */

/* _muxsid and _killmuxsid are known in libbsd's listen.c */
int _muxsid = -1;			/* group id of copy processes */
static int _mainpid = -1;
static int timerpid = -1;		/* pid of a timer process */

void _killmuxsid(void);
static void _copyproc(int, Muxbuf*);
static void _timerproc(void);
static void _resettimer(void);

static int copynotehandler(void *, char *);

/* assume FD_SETSIZE is 96 */
#define FD_ANYSET(p)	((p)->fds_bits[0] || (p)->fds_bits[1] || (p)->fds_bits[2])

/*
 * Start making fd read-buffered: make the shared segment, if necessary,
 * allocate a slot (index into mux->bufs), and fork a child to read the fd
 * and write into the slot-indexed buffer.
 * Return -1 if we can't do it.
 */
int
_startbuf(int fd)
{
	long i, n, slot;
	int pid, sid;
	Fdinfo *f;
	Muxbuf *b;

	if(mux == 0){
		_RFORK(RFREND);
		mux = (Muxseg*)_SEGATTACH(0, "shared", MUXADDR, sizeof(Muxseg));
		if((long)mux == -1){
			_syserrno();
			return -1;
		}
		/* segattach has returned zeroed memory */
		atexit(_killmuxsid);
	}

	if(fd == -1)
		return 0;

	lock(&mux->lock);
	slot = mux->curfds++;
	if(mux->curfds > INITBUFS) {
		if(_SEGBRK(mux, mux->bufs+mux->curfds) < 0){
			_syserrno();
			unlock(&mux->lock);
			return -1;
		}
	}

	f = &_fdinfo[fd];
	b = &mux->bufs[slot];
	b->n = 0;
	b->putnext = b->data;
	b->getnext = b->data;
	b->eof = 0;
	b->fd = fd;
	if(_mainpid == -1)
		_mainpid = getpid();
	if((pid = _RFORK(RFFDG|RFPROC|RFNOWAIT)) == 0){
		/* copy process ... */
		if(_muxsid == -1) {
			_RFORK(RFNOTEG);
			_muxsid = getpgrp();
		} else
			setpgid(getpid(), _muxsid);
		_NOTIFY(copynotehandler);
		for(i=0; i<OPEN_MAX; i++)
			if(i!=fd && (_fdinfo[i].flags&FD_ISOPEN))
				_CLOSE(i);
		_RENDEZVOUS(0, _muxsid);
		_copyproc(fd, b);
	}

	/* parent process continues ... */
	b->copypid = pid;
	f->buf = b;
	f->flags |= FD_BUFFERED;
	unlock(&mux->lock);
	_muxsid = _RENDEZVOUS(0, 0);
	/* leave fd open in parent so system doesn't reuse it */
	return 0;
}

/*
 * The given buffered fd is being closed.
 * Set the fd field in the shared buffer to -1 to tell copyproc
 * to exit, and kill the copyproc.
 */
void
_closebuf(int fd)
{
	Muxbuf *b;

	b = _fdinfo[fd].buf;
	if(!b)
		return;
	lock(&mux->lock);
	b->fd = -1;
	unlock(&mux->lock);
	kill(b->copypid, SIGKILL);
}

/* child copy procs execute this until eof */
static void
_copyproc(int fd, Muxbuf *b)
{
	unsigned char *e;
	int n;
	int nzeros;

	e = &b->data[PERFDMAX];
	for(;;) {
		/* make sure there's room */
		lock(&mux->lock);
		if(e - b->putnext < READMAX) {
			if(b->getnext == b->putnext) {
				b->getnext = b->putnext = b->data;
				unlock(&mux->lock);
			} else {
				/* sleep until there's room */
				b->roomwait = 1;
				unlock(&mux->lock);
				_RENDEZVOUS((unsigned long)&b->roomwait, 0);
			}
		} else
			unlock(&mux->lock);
		/*
		 * A Zero-length _READ might mean a zero-length write
		 * happened, or it might mean eof; try several times to
		 * disambiguate (posix read() discards 0-length messages)
		 */
		nzeros = 0;
		do {
			n = _READ(fd, b->putnext, READMAX);
			if(b->fd == -1) {
				_exit(0);		/* we've been closed */
			}
		} while(n == 0 && ++nzeros < 3);
		lock(&mux->lock);
		if(n <= 0) {
			b->eof = 1;
			if(mux->selwait && FD_ISSET(fd, &mux->ewant)) {
				mux->selwait = 0;
				unlock(&mux->lock);
				_RENDEZVOUS((unsigned long)&mux->selwait, fd);
			} else if(b->datawait) {
				b->datawait = 0;
				unlock(&mux->lock);
				_RENDEZVOUS((unsigned long)&b->datawait, 0);
			} else if(mux->selwait && FD_ISSET(fd, &mux->rwant)) {
				mux->selwait = 0;
				unlock(&mux->lock);
				_RENDEZVOUS((unsigned long)&mux->selwait, fd);
			} else
				unlock(&mux->lock);
			_exit(0);
		} else {
			b->putnext += n;
			b->n += n;
			if(b->n > 0) {
				/* parent process cannot be both in datawait and selwait */
				if(b->datawait) {
					b->datawait = 0;
					unlock(&mux->lock);
					/* wake up _bufreading process */
					_RENDEZVOUS((unsigned long)&b->datawait, 0);
				} else if(mux->selwait && FD_ISSET(fd, &mux->rwant)) {
					mux->selwait = 0;
					unlock(&mux->lock);
					/* wake up selecting process */
					_RENDEZVOUS((unsigned long)&mux->selwait, fd);
				} else
					unlock(&mux->lock);
			} else
				unlock(&mux->lock);
		}
	}
}

/* like read(), for a buffered fd; extra arg noblock says don't wait for data if true */
int
_readbuf(int fd, void *addr, int nwant, int noblock)
{
	Muxbuf *b;
	int ngot;

	b = _fdinfo[fd].buf;
	if(b->eof && b->n == 0) {
goteof:
		return 0;
	}
	if(b->n == 0 && noblock) {
		errno = EAGAIN;
		return -1;
	}
	/* make sure there's data */
	lock(&mux->lock);
	ngot = b->putnext - b->getnext;
	if(ngot == 0) {
		/* maybe EOF just happened */
		if(b->eof) {
			unlock(&mux->lock);
			goto goteof;
		}
		/* sleep until there's data */
		b->datawait = 1;
		unlock(&mux->lock);
		_RENDEZVOUS((unsigned long)&b->datawait, 0);
		lock(&mux->lock);
		ngot = b->putnext - b->getnext;
	}
	if(ngot == 0) {
		unlock(&mux->lock);
		goto goteof;
	}
	if(ngot > nwant)
		ngot = nwant;
	memcpy(addr, b->getnext, ngot);
	b->getnext += ngot;
	b->n -= ngot;
	if(b->getnext == b->putnext && b->roomwait) {
		b->getnext = b->putnext = b->data;
		b->roomwait = 0;
		unlock(&mux->lock);
		/* wake up copy process */
		_RENDEZVOUS((unsigned long)&b->roomwait, 0);
	} else
		unlock(&mux->lock);
	return ngot;
}

int
select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout)
{
	int n, i, tmp, t, slots, fd, err;
	Fdinfo *f;
	Muxbuf *b;

	if(timeout)
		t = timeout->tv_sec*1000 + (timeout->tv_usec+999)/1000;
	else
		t = -1;
	if(!((rfds && FD_ANYSET(rfds)) || (wfds && FD_ANYSET(wfds))
			|| (efds && FD_ANYSET(efds)))) {
		/* no requested fds */
		if(t > 0)
			_SLEEP(t);
		return 0;
	}

	_startbuf(-1);

	/* make sure all requested rfds and efds are buffered */
	if(nfds >= OPEN_MAX)
		nfds = OPEN_MAX;
	for(i = 0; i < nfds; i++)
		if((rfds && FD_ISSET(i, rfds)) || (efds && FD_ISSET(i, efds))){
			f = &_fdinfo[i];
			if(!(f->flags&FD_BUFFERED))
				if(_startbuf(i) != 0) {
					return -1;
				}
			b = f->buf;
			if(rfds && FD_ISSET(i,rfds) && b->eof && b->n == 0)
			if(efds == 0 || !FD_ISSET(i,efds)) {
				errno = EBADF;		/* how X tells a client is gone */
				return -1;
			}
		}

	/* check wfds;  for now, we'll say they are all ready */
	n = 0;
	if(wfds && FD_ANYSET(wfds)){
		for(i = 0; i<nfds; i++)
			if(FD_ISSET(i, wfds)) {
				n++;
			}
	}

	lock(&mux->lock);

	slots = mux->curfds;
	FD_ZERO(&mux->rwant);
	FD_ZERO(&mux->ewant);

	for(i = 0; i<slots; i++) {
		b = &mux->bufs[i];
		fd = b->fd;
		if(fd == -1)
			continue;
		err = 0;
		if(efds && FD_ISSET(fd, efds)) {
			if(b->eof && b->n == 0){
				err = 1;
				n++;
			}else{
				FD_CLR(fd, efds);
				FD_SET(fd, &mux->ewant);
			}
		}
		if(rfds && FD_ISSET(fd, rfds)) {
			if(!err && (b->n > 0 || b->eof))
				n++;
			else{
				FD_CLR(fd, rfds);
				FD_SET(fd, &mux->rwant);
			}
		}
	}
	if(n || !(FD_ANYSET(&mux->rwant) || FD_ANYSET(&mux->ewant)) || t == 0) {
		FD_ZERO(&mux->rwant);
		FD_ZERO(&mux->ewant);
		unlock(&mux->lock);
		return n;
	}

	if(timeout) {
		mux->waittime = t;
		if(timerpid == -1)
			_timerproc();
		else
			_resettimer();
	}
	mux->selwait = 1;
	unlock(&mux->lock);
	fd = _RENDEZVOUS((unsigned long)&mux->selwait, 0);
	if(fd >= 0) {
		b = _fdinfo[fd].buf;
		if(FD_ISSET(fd, &mux->rwant)) {
			FD_SET(fd, rfds);
			n = 1;
		} else if(FD_ISSET(fd, &mux->ewant) && b->eof && b->n == 0) {
			FD_SET(fd, efds);
			n = 1;
		}
	}
	FD_ZERO(&mux->rwant);
	FD_ZERO(&mux->ewant);
	return n;
}

static int timerreset;
static int timerpid;

static void
alarmed(int v)
{
	timerreset = 1;
}

/* a little over an hour */
#define LONGWAIT 4000001

static void
_killtimerproc(void)
{
	if(timerpid > 0)
		kill(timerpid, SIGKILL);
}

static void
_timerproc(void)
{
	int i;

	if((timerpid = _RFORK(RFFDG|RFPROC|RFNOWAIT)) == 0){
		/* timer process */
		setpgid(getpid(), _muxsid);
		signal(SIGALRM, alarmed);
		for(i=0; i<OPEN_MAX; i++)
				_CLOSE(i);
		_RENDEZVOUS(1, 0);
		for(;;) {
			_SLEEP(mux->waittime);
			if(timerreset) {
				timerreset = 0;
			} else {
				lock(&mux->lock);
				if(mux->selwait && mux->waittime != LONGWAIT) {
					mux->selwait = 0;
					mux->waittime = LONGWAIT;
					unlock(&mux->lock);
					_RENDEZVOUS((unsigned long)&mux->selwait, -2);
				} else {
					mux->waittime = LONGWAIT;
					unlock(&mux->lock);
				}
			}
		}
	}
	atexit(_killtimerproc);
	/* parent process continues */
	_RENDEZVOUS(1, 0);
}

static void
_resettimer(void)
{
	kill(timerpid, SIGALRM);
}

void
_killmuxsid(void)
{
	if(_muxsid != -1 && (_mainpid == getpid() || _mainpid == -1))
		kill(-_muxsid,SIGTERM);
}

/* call this on fork(), because reading a BUFFERED fd won't work in child */
void
_detachbuf(void)
{
	int i;
	Fdinfo *f;

	if(mux == 0)
		return;
	_SEGDETACH(mux);
	for(i = 0; i < OPEN_MAX; i++){
		f = &_fdinfo[i];
		if(f->flags&FD_BUFFERED)
			f->flags = (f->flags&~FD_BUFFERED) | FD_BUFFEREDX;
				/* mark 'poisoned' */
	}
	mux = 0;
	_muxsid = -1;
	_mainpid = -1;
	timerpid = -1;
}

static int
copynotehandler(void *u, char *msg)
{
	int i;
	void(*f)(int);

	if(_finishing)
		_finish(0, 0);
	_NOTED(1);
}
