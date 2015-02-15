/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdio.h>

void
safecpy(char *to, char *from, int tolen)
{
	int fromlen;
	memset(to, 0, tolen);
	fromlen = from ? strlen(from) : 0;
	if (fromlen > tolen)
		fromlen = tolen;
	memcpy(to, from, fromlen);
}
