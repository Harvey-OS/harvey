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
vsnprint(char *buf, int len, const char *fmt, va_list args)
{
	Fmt f;

	if(len <= 0)
		return -1;
	f.runes = 0;
	f.start = buf;
	f.to = buf;
	f.stop = buf + len - 1;
	f.flush = nil;
	f.farg = nil;
	f.nfmt = 0;
	//f.args = args;
	va_copy(f.args,args);
	dofmt(&f, fmt);
	va_end(f.args);
	*(char*)f.to = '\0';
	return (char*)f.to - buf;
}
