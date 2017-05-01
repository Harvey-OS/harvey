/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>
#include "dat.h"

// convert an mpint into a big endian byte array (most significant byte first)
//   return number of bytes converted
//   if p == nil, allocate and result array
int
mptobe(mpint *b, uint8_t *p, uint n, uint8_t **pp)
{
	int i, j, suppress;
	mpdigit x;
	uint8_t *e, *s, c;

	if(p == nil){
		n = (b->top+1)*Dbytes;
		p = malloc(n);
		setmalloctag(p, getcallerpc());
	}
	if(p == nil)
		return -1;
	if(pp != nil)
		*pp = p;
	memset(p, 0, n);

	// special case 0
	if(b->top == 0){
		if(n < 1)
			return -1;
		else
			return 1;
	}

	s = p;
	e = s+n;
	suppress = 1;
	for(i = b->top-1; i >= 0; i--){
		x = b->p[i];
		for(j = Dbits-8; j >= 0; j -= 8){
			c = x>>j;
			if(c == 0 && suppress)
				continue;
			if(p >= e)
				return -1;
			*p++ = c;
			suppress = 0;
		}
	}

	// guarantee at least one byte
	if(s == p){
		if(p >= e)
			return -1;
		*p++ = 0;
	}

	return p - s;
}
