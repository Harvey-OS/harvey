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

int
utfnlen(int8_t *s, int32_t m)
{
	int c;
	int32_t n;
	Rune rune;
	int8_t *es;

	es = s + m;
	for(n = 0; s < es; n++) {
		c = *(uint8_t*)s;
		if(c < Runeself){
			if(c == '\0')
				break;
			s++;
			continue;
		}
		if(!fullrune(s, es-s))
			break;
		s += chartorune(&rune, s);
	}
	return n;
}
