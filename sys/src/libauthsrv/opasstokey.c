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
#include <authsrv.h>

int
opasstokey(char *key, char *p)
{
	uchar t[10];
	int c, n;

	n = strlen(p);
	memset(t, ' ', sizeof t);
	if(n < 5)
		return 0;
	if(n > 10)
		n = 10;
	strncpy((char*)t, p, n);
	if(n >= 9){
		c = p[8] & 0xf;
		if(n == 10)
			c += p[9] << 4;
		for(n = 0; n < 8; n++)
			if(c & (1 << n))
				t[n] -= ' ';
	}
	for(n = 0; n < 7; n++)
		key[n] = (t[n] >> n) + (t[n+1] << (8 - (n+1)));
	return 1;
}
