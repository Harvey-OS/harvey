#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Bufproc Bufproc;
typedef struct Bufq Bufq;

struct Bufq
{
	Bufq		*next;

	uchar	*start;
	uchar	*end;

	uchar	data[8*1024];
};

struct Bufproc
{
	Ref;
	QLock;

	int		fd;
	int		error;
	int		notefd;

	Bufq		*qf;
	Bufq		*qh;
	Bufq		**qt;

	int		wr;
	Uwaitq	wq;
};

static int
queuesize(Bufq *q)
{
	int n;

	n = 0;
	while(q){
		n += (q->end - q->start);
		q = q->next;
	}
	return n;
}

void
freebufproc(void *bp)
{
	Bufproc *b = bp;
	Bufq *q;

	if(b == nil)
		return;
	qlock(b);
	b->fd = -1;
	if(decref(b)){
		if(b->wr){
			b->wr = 0;
			while(rendezvous(&b->wr, 0) == (void*)~0)
				;
		} else {
			write(b->notefd, "interrupt", 9);
		}
		qunlock(b);
		return;
	}
	qunlock(b);

	*b->qt = b->qf;
	while(q = b->qh){
		b->qh = q->next;
		free(q);
	}
	close(b->notefd);
	free(b);
}

static void
bufproc(void *aux)
{
	Bufproc *b = aux;
	Bufq *q;
	int ret;
	int fd;

	setprocname("bufproc()");

	q = nil;
	qlock(b);
	for(;;){
		while((b->fd >= 0) && (queuesize(b->qh) >= 64*1024)){
			b->wr = 1;
			qunlock(b);
			while(rendezvous(&b->wr, 0) == (void*)~0)
				;
			qlock(b);
		}
		if((fd = b->fd) < 0)
			break;
		if((q == nil) && (q = b->qf))
			b->qf = q->next;
		qunlock(b);

		if(q == nil)
			q = kmalloc(sizeof(*q));
		q->next = nil;
		q->end = q->start = &q->data[0];
		ret = read(fd, q->start, sizeof(q->data));

		qlock(b);
		if(ret < 0){
			ret = mkerror();
			if(ret == -EINTR || ret == -ERESTART)
				continue;
			b->error = ret;
			b->fd = -1;
			break;
		}
		q->end = q->start + ret;
		*b->qt = q;
		b->qt = &q->next;
		q = nil;
		wakeq(&b->wq, MAXPROC);
	}
	if(q){
		q->next = b->qf;
		b->qf = q;
	}
	wakeq(&b->wq, MAXPROC);
	qunlock(b);
	freebufproc(b);
}

void*
newbufproc(int fd)
{
	char buf[80];
	Bufproc *b;
	int pid;

	b = kmallocz(sizeof(*b), 1);
	b->ref = 2;
	b->fd = fd;
	b->qt = &b->qh;
	if((pid = procfork(bufproc, b, 0)) < 0)
		panic("unable to fork bufproc: %r");
	snprint(buf, sizeof(buf), "/proc/%d/note", pid);
	b->notefd = open(buf, OWRITE);

	return b;
}

int readbufproc(void *bp, void *data, int len, int peek, int noblock)
{
	Bufproc *b = bp;
	uchar *p;
	Bufq *q;
	int ret;

	qlock(b);
	while((q = b->qh) == nil){
		if(noblock){
			ret = -EAGAIN;
			goto out;
		}
		if(peek){
			ret = 0;
			goto out;
		}
		if(b->fd < 0){
			if((ret = b->error) == 0)
				ret = -EIO;
			goto out;
		}
		if((ret = sleepq(&b->wq, b, 1)) < 0){
			qunlock(b);
			return ret;
		}
	}

	p = data;
	ret = 0;
	while(q != nil){
		int n;

		n = q->end - q->start;
		if(n == 0)
			break;
		if(n > len - ret)
			n = len - ret;
		memmove(p, q->start, n);
		p += n;
		ret += n;
		if(q->start+n >= q->end){
			if(!peek){
				Bufq *t;

				t = q->next;
				if((b->qh = q->next) == nil)
					b->qt = &b->qh;
				q->next = b->qf;
				b->qf = q;
				q = t;
			} else {
				q = q->next;
			}
		} else {
			if(!peek)
				q->start += n;
			break;
		}
	}

	if(b->wr && !peek){
		b->wr = 0;
		while(rendezvous(&b->wr, 0) == (void*)~0)
			;
		qunlock(b);

		return ret;
	}
out:
	qunlock(b);

	return ret;
}

int pollbufproc(void *bp, Ufile *file, void *tab)
{
	Bufproc *b = bp;
	int ret;

	ret = 0;

	qlock(b);
	pollwait(file, &b->wq, tab);
	if(b->fd >= 0){
		ret |= POLLOUT;
	} else if(b->error < 0)
		ret |= POLLERR;
	if(b->qh)
		ret |= POLLIN;
	qunlock(b);

	return ret;
}

int nreadablebufproc(void *bp)
{
	Bufproc *b = bp;
	int ret;

	qlock(b);
	ret = queuesize(b->qh);
	qunlock(b);

	return ret;
}
