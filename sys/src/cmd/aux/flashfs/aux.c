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
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"

void*
emalloc9p(ulong sz)
{
	void *v;

	if((v = malloc(sz)) == nil) {
		fprint(2, "out of memory allocating %lud\n", sz);
		exits("mem");
	}
	memset(v, 0, sz);
	setmalloctag(v, getcallerpc(&sz));
	return v;
}

void*
erealloc9p(void *v, ulong sz)
{
	if((v = realloc(v, sz)) == nil) {
		fprint(2, "out of memory allocating %lud\n", sz);
		exits("mem");
	}
	setrealloctag(v, getcallerpc(&v));
	return v;
}

char*
estrdup9p(char *s)
{
	char *t;

	if((t = strdup(s)) == nil) {
		fprint(2, "out of memory in strdup(%.10s)\n", s);
		exits("mem");
	}
	setmalloctag(t, getcallerpc(&s));
	return t;
}

