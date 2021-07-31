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
#include "impl.h"

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
	close(p->fd[0]);

	srv(p->s, p->fd[1]);
	if(endsrv)
		endsrv(nil);
	threadexitsall(nil);
}

void
threadpostmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	Postcrud *p;

	p = emalloc(sizeof(*p));
	if(pipe(p->fd) < 0)
		return;

	p->s = s;

	procrfork(threadsrvproc, p, 16384, RFNAMEG|RFFDG);
	close(p->fd[1]);

	if(name)
		_postfd(name, p->fd[0]);

	if(mtpt)
		if(mount(p->fd[0], mtpt, flag, "") < 0)
			threadprint(2, "mount at %s fails: %r\n", mtpt);

	close(p->fd[0]);
}
