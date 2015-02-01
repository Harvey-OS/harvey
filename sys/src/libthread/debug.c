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
#include <thread.h>
#include "threadimpl.h"

int _threaddebuglevel;

void
_threaddebug(ulong flag, char *fmt, ...)
{
	char buf[128];
	va_list arg;
	Fmt f;
	Proc *p;

	if((_threaddebuglevel&flag) == 0)
		return;

	fmtfdinit(&f, 2, buf, sizeof buf);

	p = _threadgetproc();
	if(p==nil)
		fmtprint(&f, "noproc ");
	else if(p->thread)
		fmtprint(&f, "%d.%d ", p->pid, p->thread->id);
	else
		fmtprint(&f, "%d._ ", p->pid);

	va_start(arg, fmt);
	fmtvprint(&f, fmt, arg);
	va_end(arg);
	fmtprint(&f, "\n");
	fmtfdflush(&f);
}

void
_threadassert(char *s)
{
	char buf[256];
	int n;
	Proc *p;

	p = _threadgetproc();
	if(p && p->thread)
		n = sprint(buf, "%d.%d ", p->pid, p->thread->id);
	else
		n = 0;
	snprint(buf+n, sizeof(buf)-n, "%s: assertion failed\n", s);
	write(2, buf, strlen(buf));
	abort();
}
