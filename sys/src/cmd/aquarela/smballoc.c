/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

#ifndef LEAK
void *
smbemallocz(u32 size, int clear)
{
	void *p = nbemalloc(size);
	if (clear && p)
		memset(p, 0, size);
	return p;
}

void *
smbemalloc(u32 size)
{
	return smbemallocz(size, 0);
}

char *
smbestrdup(char *p)
{
	char *q;
	q = smbemalloc(strlen(p) + 1);
	return strcpy(q, p);
}
#endif

void
smbfree(void **pp)
{
	void *p = *pp;
	if (p) {
		free(p);
		*pp = nil;
	}
}

void
smberealloc(void **pp, u32 size)
{
	*pp = realloc(*pp, size);
	assert(size == 0 || *pp);
}
