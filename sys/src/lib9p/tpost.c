/*
 * This is in a different file so we don't drag in 
 * libthread (for threadmalloc) unless it's actually
 * getting used.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"

typedef struct Postcrud Postcrud;
struct Postcrud {
	int fd[2];
	Srv *s;
};

static void
threadsrvproc(void *v)
{
	Postcrud *p;

	p = v;
	srv(p->s);
	if(p->s->end)
		p->s->end(p->s);
	threadexits(nil);
}

void
threadpostmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	Postcrud *p;

	p = emalloc9p(sizeof(*p));
	if(s->nopipe){
		if(name || mtpt){
			fprint(2, "threadpostmountsrv: can't post or mount with nopipe\n");
			threadexitsall("nopipe");
		}
	}else{
		if(pipe(p->fd) < 0){
			fprint(2, "pipe: %r\n");
			threadexitsall("pipe");
		}
		if(name)
			if(postfd(name, p->fd[0]) < 0){
				fprint(2, "postfd %s: %r", name);
				threadexitsall("postfd");
			}
		s->infd = p->fd[1];
		s->outfd = p->fd[1];
		s->srvfd = p->fd[0];
	}
	p->s = s;

	if(procrfork(threadsrvproc, p, 32*1024, RFNAMEG) < 0) {
		fprint(2, "procrfork: %r\n");
		threadexitsall("procrfork");
	}

	if(!s->nopipe){
		if(mtpt)
			if(amount(p->fd[0], mtpt, flag, "") < 0)
				fprint(2, "mount at %s fails: %r\n", mtpt);
	}
}
