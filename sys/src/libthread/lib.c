#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

static vlong totalmalloc;

void*
_threadmalloc(uintptr size, int z)
{
	void *m;

	m = malloc(size);
	if (m == nil)
		sysfatal("Malloc of size %lld failed: %r", (vlong)size);
	setmalloctag(m, getcallerpc(&size));
	totalmalloc += size;
	if (1 || size > 100000000) {	// TODO: restore
		static int log = -1;

		if (log < 0)
			log = open("/sys/log/plumber", OWRITE);
		fprint(log, "Malloc of size %lld, total %lld\n",
			(vlong)size, totalmalloc);
//		abort();
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
