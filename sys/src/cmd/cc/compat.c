/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"cc.h"
#include	"compat"

/*
 * fake mallocs
 */
void*
malloc(ulong n)
{
	return alloc(n);
}

void*
calloc(ulong m, ulong n)
{
	return alloc(m*n);
}

void*
realloc(void*, ulong)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
free(void*)
{
}

/* needed when profiling */
void*
mallocz(ulong size, int clr)
{
	void *v;

	v = alloc(size);
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void
setmalloctag(void*, ulong)
{
}
