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

static long totalmalloc;

void*
_threadmalloc(long size, int z)
{
	void *m;

	m = malloc(size);
	if (m == nil)
		sysfatal("Malloc of size %ld failed: %r", size);
	setmalloctag(m, getcallerpc(&size));
	totalmalloc += size;
	if (size > 100000000) {
		fprint(2, "Malloc of size %ld, total %ld\n", size, totalmalloc);
		abort();
	}
	if (z)
		memset(m, 0, size);
	return m;
}

void
_threadsysfatal(char *fmt, va_list arg)
{
	char buf[1024];	/* size doesn't matter; we're about to exit */

	vseprint(buf, buf+sizeof(buf), fmt, arg);
	if(argv0)
		fprint(2, "%s: %s\n", argv0, buf);
	else
		fprint(2, "%s\n", buf);
	threadexitsall(buf);
}
