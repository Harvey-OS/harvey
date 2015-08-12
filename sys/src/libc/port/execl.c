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

int
execl(char *f, ...)
{
	va_list va, va2;
	char *arg;
	int n;

	va_start(va, f);
	va_copy(va2, va);

	n = 0;
	while((arg = va_arg(va, char *)) != nil)
		n++;

	char *args[n + 1];

	n = 0;
	while((arg = va_arg(va2, char *)) != nil)
		args[n++] = arg;
	args[n] = nil;

	va_end(va);
	va_end(va2);

	return exec(f, args);
}
