#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "playlist.h"

static Channel *reqs;

Req*
reqalloc(void)
{
	Req *r;

	if(reqs == nil)
		reqs = chancreate(sizeof(Req*), 256);
	if(r = nbrecvp(reqs))
		return r;
	r = malloc(sizeof(Req));
	return r;
}

void
reqfree(Req *r)
{
	if(!nbsendp(reqs, r))
		free(r);
}

Wmsg
waitmsg(Worker *w, Channel *q)
{
	Wmsg m;

	sendp(q, w);
	recv(w->eventc, &m);
	return m;
}

int
sendmsg(Channel *q, Wmsg *m)
{
	Worker *w;

	if(w = nbrecvp(q))
		send(w->eventc, m);
	return w != nil;
}

void
bcastmsg(Channel *q, Wmsg *m)
{
	Worker *w;
	void *a;

	a = m->arg;
	while(w = nbrecvp(q)){
		if(a) m->arg = strdup(a);
		send(w->eventc, m);
	}
	free(a);
	m->arg = nil;
}

void
readbuf(Req *r, void *s, long n)
{
	r->ofcall.count = r->ifcall.count;
	if(r->ifcall.offset >= n){
		r->ofcall.count = 0;
		return;
	}
	if(r->ifcall.offset+r->ofcall.count > n)
		r->ofcall.count = n - r->ifcall.offset;
	memmove(r->ofcall.data, (char*)s+r->ifcall.offset, r->ofcall.count);
}

void
readstr(Req *r, char *s)
{
	readbuf(r, s, strlen(s));
}
