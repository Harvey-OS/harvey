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
#include "compat.h"

typedef struct Exporter	Exporter;
struct Exporter
{
	int	fd;
	Chan	**roots;
	int	nroots;
};

int
mounter(char *mntpt, int how, int fd, int n)
{
	char buf[32];
	int i, ok, mfd;

	ok = 1;
	for(i = 0; i < n; i++){
		snprint(buf, sizeof buf, "%d", i);
		mfd = dup(fd, -1);
		if(mount(mfd, -1, mntpt, how, buf) == -1){
			close(mfd);
			fprint(2, "can't mount on %s: %r\n", mntpt);
			ok = 0;
			break;
		}
		close(mfd);
		if(how == MREPL)
			how = MAFTER;
	}

	close(fd);

	return ok;
}

static void
extramp(void *v)
{
	Exporter *ex;

	rfork(RFNAMEG);
	ex = v;
	sysexport(ex->fd, ex->roots, ex->nroots);
	shutdown();
	exits(nil);
}

int
exporter(Dev **dt, int *fd, int *sfd)
{
	Chan **roots;
	Exporter ex;
	int p[2], i, n, ed;

	for(n = 0; dt[n] != nil; n++)
		;
	if(!n){
		werrstr("no devices specified");
		return 0;
	}

	ed = errdepth(-1);
	if(waserror()){
		werrstr(up->error);
		return 0;
	}

	roots = smalloc(n * sizeof *roots);
	for(i = 0; i < n; i++){
		(*dt[i]->reset)();
		(*dt[i]->init)();
		roots[i] = (*dt[i]->attach)("");
	}
	poperror();
	errdepth(ed);

	if(pipe(p) < 0){
		werrstr("can't make pipe: %r");
		return 0;
	}

	*sfd = p[0];
	*fd = p[1];

	ex.fd = *sfd;
	ex.roots = roots;
	ex.nroots = n;
	kproc("exporter", extramp, &ex);

	return n;
}
