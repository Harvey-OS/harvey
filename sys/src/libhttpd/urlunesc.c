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
#include <bin.h>
#include <httpd.h>

/* go from url with escaped utf to utf */
char *
hurlunesc(HConnect *cc, char *s)
{
	char *t, *v, *u;
	Rune r;
	int c, n;

	/* unescape */
	u = halloc(cc, strlen(s)+1);
	for(t = u; c = *s; s++){
		if(c == '%'){
			n = s[1];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			r = n;
			n = s[2];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			s += 2;
			c = (r<<4)+n;
		}
		*t++ = c;
	}
	*t = '\0';

	/* convert to valid utf */
	v = halloc(cc, UTFmax*strlen(u) + 1);
	s = u;
	t = v;
	while(*s){
		/* in decoding error, assume latin1 */
		if((n=chartorune(&r, s)) == 1 && r == Runeerror)
			r = (uchar)*s;
		s += n;
		t += runetochar(t, &r);
	}
	*t = '\0';

	return v;
}
