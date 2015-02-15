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

Waitmsg*
system(char *name, char **argv)
{
	char err[ERRMAX];
	Waitmsg *w;
	int pid;

	switch(pid = fork()){	/* assign = */
	case -1:
		return nil;
	case 0:
		exec(name, argv);
		errstr(err, sizeof err);
		_exits(err);
	}
	for(;;){
		w = wait();
		if(w == nil)
			break;
		if(w->pid == pid)
			return w;
		free(w);
	}
	return nil;
}

Waitmsg*
systeml(char *name, ...)
{
	return system(name, &name+1);
}
