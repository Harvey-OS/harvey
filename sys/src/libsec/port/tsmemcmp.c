/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <libsec.h>

/*
 * timing safe memcmp()
 */
int
tsmemcmp(void *a1, void *a2, uint32_t n)
{
	int lt, gt, c1, c2, r, m;
	uint8_t *s1, *s2;

	r = m = 0;
	s1 = a1;
	s2 = a2;
	while(n--){
		c1 = *s1++;
		c2 = *s2++;
		lt = (c1 - c2) >> 8;
		gt = (c2 - c1) >> 8;
		r |= (lt - gt) & ~m;
		m |= lt | gt;
	}
	return r;
}
