/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

void
bcopy(void *f, void *t, size_t n)
{
	memmove(t, f, n);
}

int
bcmp(void *a, void *b, size_t n)
{
	return memcmp(a, b, n);
}

void
bzero(void *a, size_t n)
{
	memset(a, 0, n);
}
