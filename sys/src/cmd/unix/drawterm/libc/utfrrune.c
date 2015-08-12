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

char*
utfrrune(char* s, int32_t c)
{
	int32_t c1;
	Rune r;
	char* s1;

	if(c < Runesync) /* not part of utf sequence */
		return strrchr(s, c);

	s1 = 0;
	for(;;) {
		c1 = *(uint8_t*)s;
		if(c1 < Runeself) { /* one byte rune */
			if(c1 == 0)
				return s1;
			if(c1 == c)
				s1 = s;
			s++;
			continue;
		}
		c1 = chartorune(&r, s);
		if(r == c)
			s1 = s;
		s += c1;
	}
	return 0;
}
