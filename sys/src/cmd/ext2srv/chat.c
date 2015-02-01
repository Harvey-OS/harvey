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
#include "dat.h"
#include "fns.h"

#define	SIZE	1024
#define	DOTDOT	(&fmt+1)

int	chatty;

void
chat(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg;

	if (!chatty)
		return;

	va_start(arg, fmt);
	out = vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	write(2, buf, (long)(out-buf));
}

void
mchat(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg;

	va_start(arg, fmt);
	out = vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	write(2, buf, (long)(out-buf));
}
void
panic(char *fmt, ...)
{
	char buf[SIZE];
	va_list arg;
	int n;

	n = sprint(buf, "%s %d: panic ", argv0, getpid());
	va_start(arg, fmt);
	vseprint(buf+n, buf+sizeof(buf)-n, fmt, arg);
	va_end(arg);
	fprint(2, "%s: %r\n", buf);
	exits("panic");
}
