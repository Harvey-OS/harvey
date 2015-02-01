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
#include <bin.h>
#include <httpd.h>

/*
 * memory allocators:
 * h routines call canalloc; they should be used by everything else
 * note this memory is wiped out at the start of each new request
 * note: these routines probably shouldn't fatal.
 */
char*
hstrdup(HConnect *c, char *s)
{
	char *t;
	int n;

	n = strlen(s) + 1;
	t = binalloc(&c->bin, n, 0);
	if(t == nil)
		sysfatal("out of memory");
	memmove(t, s, n);
	return t;
}

void*
halloc(HConnect *c, ulong n)
{
	void *p;

	p = binalloc(&c->bin, n, 1);
	if(p == nil)
		sysfatal("out of memory");
	return p;
}
