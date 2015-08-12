/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>

void *
calloc(size_t nmemb, size_t size)
{
	void *mp;

	nmemb = nmemb * size;
	if(mp = malloc(nmemb))
		memset(mp, 0, nmemb);
	return (mp);
}
