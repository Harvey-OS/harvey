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
wstrutflen(Rune *s)
{
	int n;
	
	for(n=0; *s; n+=runelen(*s),s++)
		;
	return n;
}

int
wstrtoutf(char *s, Rune *t, int n)
{
	int i;
	char *s0;

	s0 = s;
	if(n <= 0)
		return wstrutflen(t)+1;
	while(*t) {
		if(n < UTFmax+1 && n < runelen(*t)+1) {
			*s = 0;
			return i+wstrutflen(t)+1;
		}
		i = runetochar(s, t);
		s += i;
		n -= i;
		t++;
	}
	*s = 0;
	return s-s0;
}
