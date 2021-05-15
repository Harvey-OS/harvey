/*
 * Copyright © 2021 Plan 9 Foundation
 * Copyright © 2021 9front authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

	buf = smprint("/proc/%lu/ctl", (u32)getpid());
	fd = open(buf, OWRITE|OCEXEC);
	free(buf);
	
	for(;;){
		qlock(&q->lock);
		q->flush = 0;
		if(fd >= 0)
			write(fd, "nointerrupt", 11);
		q->cur = nil;
		while((Reqqueue*)q->queue.next == q)
			rsleep(&q->rendez);
		r = (Req*)(((char*)q->queue.next) - ((char*)&((Req*)0)->qu));
		r->qu.next->prev = r->qu.prev;
		r->qu.prev->next = r->qu.next;
		f = r->qu.f;
		memset(&r->qu, 0, sizeof(r->qu));
		q->cur = r;
		qunlock(&q->lock);
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
	q->rendez.l = &q->lock;
	q->queue.next = q->queue.prev = (Queueelem*)q;
	q->pid = proccreate(_reqqueueproc, q, mainstacksize);
	return q;
}

void
reqqueuepush(Reqqueue *q, Req *r, void (*f)(Req *))
{
	qlock(&q->lock);
	r->qu.f = f;
	r->qu.next = (Queueelem*)q;
	r->qu.prev = q->queue.prev;
	q->queue.prev->next = &r->qu;
	q->queue.prev = &r->qu;
	rwakeup(&q->rendez);
	qunlock(&q->lock);
}

void
reqqueueflush(Reqqueue *q, Req *r)
{
	qlock(&q->lock);
	if(q->cur == r){
		threadint(q->pid);
		q->flush++;
		qunlock(&q->lock);
	}else{
		if(r->qu.next != nil){
			r->qu.next->prev = r->qu.prev;
			r->qu.prev->next = r->qu.next;
		}
		memset(&r->qu, 0, sizeof(r->qu));
		qunlock(&q->lock);
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
