#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <auth.h>
#include "post.h"

Postcrud*
_post1(Srv *s, char *name, char *mtpt, int flag)
{
	Postcrud *p;

	p = emalloc9p(sizeof *p);
	if(!s->nopipe){
		if(pipe(p->fd) < 0)
			sysfatal("pipe: %r");
		s->infd = s->outfd = p->fd[1];
		s->srvfd = p->fd[0];
	}
	if(name)
		if(postfd(name, s->srvfd) < 0)
			sysfatal("postfd %s: %r", name);
	p->s = s;
	p->mtpt = mtpt;
	p->flag = flag;
	return p;
}

void
_post2(void *v)
{
	Srv *s;

	s = v;
	rfork(RFNOTEG);
	rendezvous((ulong)s, 0);
	close(s->srvfd);
	srv(s);
}

void
_post3(Postcrud *p)
{
	rfork(RFFDG);
	rendezvous((ulong)p->s, 0);
	close(p->s->infd);

	if(p->mtpt){
		if(amount(p->s->srvfd, p->mtpt, p->flag, "") == -1)
			sysfatal("mount %s: %r", p->mtpt);
	}else
		close(p->s->srvfd);
	free(p);
}

