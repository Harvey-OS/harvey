/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <auth.h>

static void postproc(void*);

void
_postmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	int fd[2];

	if(!s->nopipe){
		if(pipe(fd) < 0)
			sysfatal("pipe: %r");
		s->infd = s->outfd = fd[1];
		s->srvfd = fd[0];
	}
	if(name)
		if(postfd(name, s->srvfd) < 0)
			sysfatal("postfd %s: %r", name);

	if(_forker == nil)
		sysfatal("no forker");
	_forker(postproc, s, RFNAMEG);

	/*
	 * Normally the server is posting as the last thing it does
	 * before exiting, so the correct thing to do is drop into
	 * a different fd space and close the 9P server half of the
	 * pipe before trying to mount the kernel half.  This way,
	 * if the file server dies, we don't have a ref to the 9P server
	 * half of the pipe.  Then killing the other procs will drop
	 * all the refs on the 9P server half, and the mount will fail.
	 * Otherwise the mount hangs forever.
	 *
	 * Libthread in general and acme win in particular make
	 * it hard to make this fd bookkeeping work out properly,
	 * so leaveinfdopen is a flag that win sets to opt out of this
	 * safety net.
	 */
	if(!s->leavefdsopen){
		rfork(RFFDG);
		rendezvous(0, 0);
		close(s->infd);
		if(s->infd != s->outfd)
			close(s->outfd);
	}

	if(mtpt){
		if(amount(s->srvfd, mtpt, flag, "") == -1)
			sysfatal("mount %s: %r", mtpt);
	}else
		close(s->srvfd);
}

static void
postproc(void *v)
{
	Srv *s;

	s = v;
	if(!s->leavefdsopen){
		rfork(RFNOTEG);
		rendezvous(0, 0);
		close(s->srvfd);
	}
	srv(s);
}
