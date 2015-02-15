/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdarg.h>
#include <errno.h>
#include "fmt.h"

extern char _plan9err[128];

void
werrstr(const char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	snprint(_plan9err, sizeof _plan9err, fmt, arg);
	va_end(arg);
	errno = EPLAN9;
}
