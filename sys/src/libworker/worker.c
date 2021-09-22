#include <u.h>
#include <libc.h>
#include <thread.h>
#include <error.h>
#include <worker.h>

static Channel	*workerthreads;
static Channel	*requests;
static Channel	*dispatchc;

static Request*
reqalloc(void)
{
	Request *r;

	if((r = nbrecvp(requests)) == nil)
		r = mallocz(sizeof(Request), 0);
	assert(r);
	return r;
}

static void
reqfree(Request *r)
{
	assert(r);
	if(nbsendp(requests, r) == 0)
		free(r);
}

static void
worker(void *arg)
{
	Worker *w;

	w = arg;
	while(w->r = recvp(w->chan)){
		snprint(w->name, sizeof(w->name),
			"worker %p.%lud", w, w->version);
		threadsetname(w->name);
		if(!waserror()){
			w->arg = w->r->arg;
			w->r->func(w, w->r->arg);
			poperror();
		}
		w->arg = nil;
		w->version++;	/* Next iteration is a new worker */
		reqfree(w->r);
		w->r = nil;
		if(nbsendp(workerthreads, w) == 0)
			break;
	}
	chanfree(w->chan);
	free(w);
	threadexits(nil);
}

static void
allocwork(Request *r)
{
	Worker *w;

	if((w = nbrecvp(workerthreads)) == nil){
		/* No worker ready to accept request, allocate one */
		w = mallocz(sizeof(Worker), 1);
		assert(w);
		w->chan = chancreate(sizeof(void*), 1);	/* new work */
		w->event = chancreate(sizeof(void*), 0); /* event */
		threadcreate(worker, w, 8*1024);
	}
	sendp(w->chan, r);
}

static void
srve(void*)
{
	void *r;
	Worker *w;
	/*
	 * This is the proc with all the action.
	 * When a file request comes in, it is dispatched to this proc
	 * for processing.
	 * By keeping all the action in this proc, we won't need locks
	 */

	threadsetname("srv");
	
	while(r = recvp(dispatchc))
		allocwork(r);
	while(w = nbrecvp(workerthreads))
		sendp(w->chan, nil);
	threadexits(nil);
}

void
workerdispatch(void (*func)(Worker*,void*), void *arg)
{
	Request *req;
	static QLock q;

	if(dispatchc == nil){
		qlock(&q);
		if(dispatchc == nil){	
			requests = chancreate(sizeof(Request *), Nworker);
			workerthreads = chancreate(sizeof(Worker *), Nworker);
			dispatchc = chancreate(sizeof(void*), 4);
			proccreate(srve, nil, 4*1024);
		}
		qunlock(&q);
	}
	req = reqalloc();
	req->func = func;
	req->arg = arg;
	sendp(dispatchc, req);
}
