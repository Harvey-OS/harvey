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
#include <sunrpc.h>

void*
emalloc(ulong n)
{
	void *v;

	v = mallocz(n, 1);
	if(v == nil)
{
abort();
		sysfatal("out of memory");
}
	setmalloctag(v, getcallerpc(&n));
	return v;
}

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil)
{
abort();
		sysfatal("out of memory");
}
	setrealloctag(v, getcallerpc(&n));
	return v;
}


