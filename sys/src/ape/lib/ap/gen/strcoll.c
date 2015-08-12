/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>

int
strcoll(const char* s1, const char* s2)
{
	/* BUG: supposed to pay attention to LC_COLLATE of current locale */
	return strcmp(s1, s2);
}
