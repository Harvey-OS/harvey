#include <u.h>
#include <libc.h>
#include <thread.h>
#include "worker.h"

typedef struct Work Work;
typedef struct Workproc Workproc;

struct Work
{
	Worker	work;
	void*	arg;
	Channel*rc;	/* of char* */
};

struct Workproc
{
	Workproc* next;
	Channel* wc;
	void*	aux;	/* per-worker storage */
	int id;
	Work;
};

static Channel *workerc;  	 /* of Work[N] */
static Channel *workerdonec;	 /* of Workproc* */
static Channel *workeridc;	 /* of ulong */
static int debug;

typedef int (*Forker)(void(*)(void*), void*, uint);

Forker workerthreadcreate = threadcreate;

static void
workproc(void *a)
{
	Workproc *w;
	int id;
	char *r;

	w = a;
	threadsetname("worker");
	id = threadid();
	if(debug)
		fprint(2, "worker %d: started\n", id);
	sendul(w->wc, threadid());
	for(;;){
		if(recvul(w->wc) == ~0)
			break;
		if(debug)
			fprint(2, "worker %d: work %p\n", id, w->work);
		r = w->work(w->arg, &w->aux);
		if(debug && r != nil)
			fprint(2, "worker %d: work %p: %s\n", id, w->work, r);
		if(w->rc != nil)
			sendp(w->rc, r);
		w->work = nil;
		w->arg = nil;
		w->rc = nil;
		if(debug)
			fprint(2, "worker %d: idle\n", id);
		sendp(workerdonec, w);
	}
	if(debug)
		fprint(2, "worker %d: exiting\n", id);
	threadexits(nil);
}

static void
ctlproc(void*)
{
	Work w;
	Workproc *wp, *wl;
	Alt a[] = {
		{workerc, &w, CHANRCV},
		{workerdonec, &wp, CHANRCV},
		{nil, nil, CHANEND}
	};

	threadsetname("workctl");
	wl = nil;
	for(;;){
		switch(alt(a)){
		case 0:
			if(wl == nil){
				wl = mallocz(sizeof *wl, 1);
				if(wl == nil)
					sysfatal("ctlproc: no memory");
				wl->wc = chancreate(sizeof(ulong), 0);
				if(wl->wc == nil)
					sysfatal("chancreate");
				if(workerthreadcreate(workproc, wl, mainstacksize) < 0)
					sysfatal("threadcreate");
				wl->id = recvul(wl->wc);
			}
			wp = wl;
			wl = wl->next;
			wp->Work = w;
			sendul(wp->wc, 0);
			sendul(workeridc, wp->id);
			break;
		case 1:
			wp->next = wl;
			wl = wp;
			break;
		default:
			sysfatal("alt");
		}
	}	
}

static void
init(void)
{
	if(workerc != nil)
		return;
	workerc = chancreate(sizeof(Work), 0);
	workeridc = chancreate(sizeof(ulong), 1);
	workerdonec = chancreate(sizeof(Workproc*), 0);
	if(workerc == nil || workeridc == nil || workerdonec == nil)
		sysfatal("chancreate");
	if(workerthreadcreate(ctlproc, nil, mainstacksize) < 0)
		sysfatal("threadcreate");
}

int
getworker(Worker work, void *arg, Channel *rc)
{
	Work w;

	init();
	w.work = work;
	w.arg = arg;
	w.rc = rc;
	send(workerc, &w);
	return recvul(workeridc);
}

void
workerdebug(int on)
{
	debug = on;
}
