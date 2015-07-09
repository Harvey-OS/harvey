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

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
char*
strstr(char *s1, char *s2)
{
	char *p, *pa, *pb;
	int c0, c;

	c0 = *s2;
	if(c0 == 0)
		return s1;
	s2++;
	for(p=strchr(s1, c0); p; p=strchr(p+1, c0)) {
		pa = p;
		for(pb=s2;; pb++) {
			c = *pb;
			if(c == 0)
				return p;
			if(c != *++pa)
				break;
		}
	}
	return 0;
}
