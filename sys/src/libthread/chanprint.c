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

int
chanprint(Channel *c, char *fmt, ...)
{
	va_list arg;
	char *p;
	int n;

	va_start(arg, fmt);
	p = vsmprint(fmt, arg);
	va_end(arg);
	if(p == nil)
		sysfatal("vsmprint failed: %r");
	n = sendp(c, p);
	yield();	/* let recipient handle message immediately */
	return n;
}
