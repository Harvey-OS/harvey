/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "cc.h"
#include "compat"

/*
 * fake mallocs
 */
void *
malloc(uint32_t n)
{
	return alloc(n);
}

void *
calloc(uint32_t m, uint32_t n)
{
	return alloc(m * n);
}

void *
realloc(void *, uint32_t)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
free(void *)
{
}

/* needed when profiling */
void *
mallocz(uint32_t size, int clr)
{
	void *v;

	v = alloc(size);
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void
setmalloctag(void *, uint32_t)
{
}
