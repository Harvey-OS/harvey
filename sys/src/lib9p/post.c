#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

void
postmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	int fd[2];

	if(s->nopipe){
		if(name || mtpt)
			sysfatal("postmountsrv: can't post or mount with nopipe");
	}else{
		if(pipe(fd) < 0)
			sysfatal("pipe: %r");
		if(name)
			if(postfd(name, fd[0]) < 0)
				sysfatal("postfd %s: %r", name);
		s->infd = fd[1];
		s->outfd = fd[1];
	}

	switch(rfork(RFPROC|RFFDG|RFNOTEG|RFNAMEG|RFMEM)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		if(!s->nopipe)
			close(fd[0]);
		srv(s);
		if(s->end)
			s->end(s);
		_exits(0);
	default:
		if(!s->nopipe){
			if(mtpt)
				if(amount(fd[0], mtpt, flag, "") == -1)
					sysfatal("mount %s: %r\n", mtpt);
			close(fd[0]);
		}
		break;
	}
}

