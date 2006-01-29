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

	while(w = nbrecvp(q)){
		/* Test for markerdom (see bcastmsg) */
		if(w->eventc){
			send(w->eventc, m);
			return 1;
		}
		sendp(q, w);	/* put back */
	}
	return 0;
}

void
bcastmsg(Channel *q, Wmsg *m)
{
	Worker *w, marker;
	void *a;

	a = m->arg;
	/*
	 * Use a marker to mark the end of the queue.
	 * This prevents workers from getting the
	 * broadcast and putting themselves back on the
	 * queue before the broadcast has finished
	 */
	marker.eventc = nil;	/* Only markers have eventc == nil */
	sendp(q, &marker);
	while((w = recvp(q)) != &marker){
		if(w->eventc == nil){
			/* somebody else's marker, put it back */
			sendp(q, w);
		}else{
			if(a) m->arg = strdup(a);
			send(w->eventc, m);
		}
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
