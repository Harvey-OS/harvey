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
runestrecpy(Rune *s1, Rune *es1, Rune *s2)
{
	if(s1 >= es1)
		return s1;

	while(*s1++ = *s2++){
		if(s1 == es1){
			*--s1 = '\0';
			break;
		}
	}
	return s1;
}
