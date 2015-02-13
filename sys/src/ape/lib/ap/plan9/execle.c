/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <unistd.h>

int
execle(const int8_t *name, const int8_t *arg0, const int8_t *, ...)
{
	int8_t *p;

	for(p=(int8_t *)(&name)+1; *p; )
		p++;
	return execve(name, &arg0, (int8_t **)p+1);
}
