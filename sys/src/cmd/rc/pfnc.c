/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"

struct{
	void	(*f)(void);
	char	*name;
} fname[] = {
	Xappend, "Xappend",
	Xassign, "Xassign",
	Xasync, "Xasync",
	Xbackq, "Xbackq",
	Xbang, "Xbang",
	Xcase, "Xcase",
	Xclose, "Xclose",
	Xconc, "Xconc",
	Xcount, "Xcount",
	Xdelfn, "Xdelfn",
	Xdelhere, "Xdelhere",
	Xdol, "Xdol",
	Xdup, "Xdup",
	Xeflag, "Xeflag",
	(void (*)(void))Xerror, "Xerror",
	Xexit, "Xexit",
	Xfalse, "Xfalse",
	Xfn, "Xfn",
	Xfor, "Xfor",
	Xglob, "Xglob",
	Xif, "Xif",
	Xifnot, "Xifnot",
	Xjump, "Xjump",
	Xlocal, "Xlocal",
	Xmark, "Xmark",
	Xmatch, "Xmatch",
	Xpipe, "Xpipe",
	Xpipefd, "Xpipefd",
	Xpipewait, "Xpipewait",
	Xpopm, "Xpopm",
	Xpopredir, "Xpopredir",
	Xqdol, "Xqdol",
	Xrdcmds, "Xrdcmds",
	Xrdfn, "Xrdfn",
	Xrdwr, "Xrdwr",
	Xread, "Xread",
	Xreturn, "Xreturn",
	Xsimple, "Xsimple",
	Xsub, "Xsub",
	Xsubshell, "Xsubshell",
	Xtrue, "Xtrue",
	Xunlocal, "Xunlocal",
	Xwastrue, "Xwastrue",
	Xword, "Xword",
	Xwrite, "Xwrite",
	0
};

void
pfnc(io *fd, thread *t)
{
	int i;
	void (*fn)(void) = t->code[t->pc].f;
	list *a;

	pfmt(fd, "pid %d cycle %p %d ", getpid(), t->code, t->pc);
	for (i = 0; fname[i].f; i++) 
		if (fname[i].f == fn) {
			pstr(fd, fname[i].name);
			break;
		}
	if (!fname[i].f)
		pfmt(fd, "%p", fn);
	for (a = t->argv; a; a = a->next) 
		pfmt(fd, " (%v)", a->words);
	pchr(fd, '\n');
	flush(fd);
}
