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

Rune*
runestrncat(Rune *s1, const Rune *s2, int32_t n)
{
	Rune *os1;

	os1 = s1;
	s1 = runestrchr(s1, 0);
	while((*s1++ = *s2++) != 0)
		if(--n < 0) {
			s1[-1] = 0;
			break;
		}
	return os1;
}
