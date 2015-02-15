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
#include <bio.h>
#include "authcmdlib.h"

void
error(char *fmt, ...)
{
	char buf[8192], *s;
	va_list arg;

	s = buf;
	s += snprint(s, sizeof buf, "%s: ", argv0);
	va_start(arg, fmt);
	s = vseprint(s, buf + sizeof(buf), fmt, arg);
	va_end(arg);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}
