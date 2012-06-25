#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

void pollwait(Ufile *f, Uwaitq *q, void *t)
{
	Uwait *w, **p;

	if(f == nil || t == nil || q == nil)
		return;

	p = t;
	w = addwaitq(q);
	w->file = getfile(f);
	w->next = *p;
	*p = w;
}

static void
clearpoll(Uwait **p)
{
	Uwait *w;

	while(w = *p){
		*p = w->next;
		delwaitq(w);
	}
}

struct linux_pollfd
{
	int			fd;
	short		events;
	short		revents;
};

int sys_poll(void *p, int nfd, long timeout)
{
	int i, e, err;
	Uwait *tab;
	Ufile *file;
	vlong now, t;
	struct linux_pollfd *fds = p;

	trace("sys_poll(%p, %d, %ld)", p, nfd, timeout);

	if(nfd < 0)
		return -EINVAL;

	t = 0;
	wakeme(1);
	if(timeout > 0){
		now = nsec();
		if(current->restart->syscall){
			t = current->restart->poll.timeout;
		} else {
			t = now + timeout*1000000LL;
		}
		if(now < t){
			current->timeout = t;
			setalarm(t);
		}
	}

	tab = nil;
	for(;;){
		clearpoll(&tab);

		err = 0;
		for(i=0; i<nfd; i++){
			e = 0;
			if(fds[i].fd >= 0){
				e = POLLNVAL;
				if(file = fdgetfile(fds[i].fd)){
					if(devtab[file->dev]->poll == nil){
						e = POLLIN|POLLOUT;
					} else {
						e = devtab[file->dev]->poll(file, (err == 0) ? &tab : nil);
					}
					putfile(file);
					e &= fds[i].events | POLLERR | POLLHUP;
				}
			}
			if(fds[i].revents = e){
				trace("sys_poll(): fd %d is ready with %x", fds[i].fd, fds[i].revents);
				err++;
			}
		}
		if(err > 0)
			break;
		if(timeout >= 0 && current->timeout == 0){
			trace("sys_poll(): timeout");
			break;
		}
		if((err = sleepproc(nil, 1)) < 0){
			trace("sys_poll(): interrupted");
			current->restart->poll.timeout = t;
			break;
		}
	}
	clearpoll(&tab);
	wakeme(0);

	if(timeout > 0)
		current->timeout = 0;

	return err;
}

int sys_select(int nfd, ulong *rfd, ulong *wfd, ulong *efd, void *ptv)
{
	int i, p, e, w, nwrd, nbits, fd, err;
	ulong m;
	Uwait *tab;
	Ufile *file;
	vlong now, t;
	struct linux_timeval *tv = ptv;
	struct {
		int fd;
		int ret;
	} *ardy, astk[16];

	trace("sys_select(%d, %p, %p, %p, %p)", nfd, rfd, wfd, efd, ptv);

	if(nfd < 0)
		return -EINVAL;

	if(tv != nil)
		if(tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000)
			return -EINVAL;

	nwrd = (nfd + (8 * sizeof(m))-1) / (8 * sizeof(m));

	nbits = 0;
	for(w=0; w<nwrd; w++)
		for(m=1; m; m<<=1)
			if((rfd && rfd[w] & m) || (wfd && wfd[w] & m) || (efd && efd[w] & m))
				nbits++;

	if(nbits > nelem(astk)){
		ardy = kmalloc(nbits * sizeof(ardy[0]));
	} else {
		ardy = astk;
	}

	t = 0;
	wakeme(1);
	if(tv != nil){
		now = nsec();
		if(current->restart->syscall){
			t = current->restart->select.timeout;
		} else {
			t = now + tv->tv_sec*1000000000LL + tv->tv_usec*1000;
		}
		if(now < t){
			current->timeout = t;
			setalarm(t);
		}
	}

	tab = nil;
	for(;;){
		clearpoll(&tab);

		fd = 0;
		err = 0;
		for(w=0; w<nwrd; w++){
			for(m=1; m; m<<=1, fd++){
				p = 0;
				if(rfd && rfd[w] & m)
					p |= POLLIN;
				if(wfd && wfd[w] & m)
					p |= POLLOUT;
				if(efd && efd[w] & m)
					p |= POLLERR;
				if(!p || ((file = fdgetfile(fd)) == nil))
					continue;
				if(devtab[file->dev]->poll == nil){
					e = POLLIN|POLLOUT;
				} else {
					e = devtab[file->dev]->poll(file, (err == 0) ? &tab : nil);
				}
				putfile(file);
				if(e &= p) {
					ardy[err].fd = fd;
					ardy[err].ret = e;
					if(++err == nbits)
						break;
				}
			}
		}
		if(err > 0)
			break;
		if(tv != nil && current->timeout == 0){
			trace("sys_select(): timeout");
			break;
		}
		if((err = sleepproc(nil, 1)) < 0){
			trace("sys_select(): interrupted");
			current->restart->select.timeout = t;
			break;
		}
	}
	clearpoll(&tab);
	wakeme(0);

	if(tv != nil){
		current->timeout = 0;
		t -= nsec();
		if(t < 0)
			t = 0;
		tv->tv_sec = (long)(t/1000000000LL);
		tv->tv_usec = (long)((t%1000000000LL)/1000);
	}

	if(err >= 0){
		if(rfd)	memset(rfd, 0, nwrd*sizeof(m));
		if(wfd)	memset(wfd, 0, nwrd*sizeof(m));
		if(efd)	memset(efd, 0, nwrd*sizeof(m));

		nbits = 0;
		for(i=0; i<err; i++){
			e = ardy[i].ret;
			fd = ardy[i].fd;
			w =  fd / (8 * sizeof(m));
			m = 1 << (fd % (8 * sizeof(m)));
			if(rfd && (e & POLLIN)){
				rfd[w] |= m;
				nbits++;
			}
			if(wfd && (e & POLLOUT)){
				wfd[w] |= m;
				nbits++;
			}
			if(efd && (e & POLLERR)){
				efd[w] |= m;
				nbits++;
			}
		}
		err = nbits;
	}

	if(ardy != astk)
		free(ardy);

	return err;
}
