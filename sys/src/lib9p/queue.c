#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>

static void
_reqqueueproc(void *v)
{
	Reqqueue *q;
	Req *r;
	void (*f)(Req *);
	int fd;
	char *buf;

	q = v;
	rfork(RFNOTEG);

	buf = smprint("/proc/%lud/ctl", (ulong)getpid());
	fd = open(buf, OWRITE|OCEXEC);
	free(buf);
	
	for(;;){
		qlock(q);
		q->flush = 0;
		if(fd >= 0)
			write(fd, "nointerrupt", 11);
		q->cur = nil;
		while(q->next == q)
			rsleep(q);
		r = (Req*)(((char*)q->next) - ((char*)&((Req*)0)->qu));
		r->qu.next->prev = r->qu.prev;
		r->qu.prev->next = r->qu.next;
		f = r->qu.f;
		memset(&r->qu, 0, sizeof(r->qu));
		q->cur = r;
		qunlock(q);
		if(f == nil)
			break;
		f(r);
	}

	if(fd >= 0)
		close(fd);
	free(r);
	free(q);
	threadexits(nil);
}

Reqqueue *
reqqueuecreate(void)
{
	Reqqueue *q;

	q = emalloc9p(sizeof(*q));
	memset(q, 0, sizeof(*q));
	q->l = q;
	q->next = q->prev = q;
	q->pid = proccreate(_reqqueueproc, q, mainstacksize);
	return q;
}

void
reqqueuepush(Reqqueue *q, Req *r, void (*f)(Req *))
{
	qlock(q);
	r->qu.f = f;
	r->qu.next = q;
	r->qu.prev = q->prev;
	q->prev->next = &r->qu;
	q->prev = &r->qu;
	rwakeup(q);
	qunlock(q);
}

void
reqqueueflush(Reqqueue *q, Req *r)
{
	qlock(q);
	if(q->cur == r){
		threadint(q->pid);
		q->flush++;
		qunlock(q);
	}else{
		if(r->qu.next != nil){
			r->qu.next->prev = r->qu.prev;
			r->qu.prev->next = r->qu.next;
		}
		memset(&r->qu, 0, sizeof(r->qu));
		qunlock(q);
		respond(r, "interrupted");
	}
}

void
reqqueuefree(Reqqueue *q)
{
	Req *r;

	if(q == nil)
		return;
	r = emalloc9p(sizeof(Req));
	reqqueuepush(q, r, nil);
}
