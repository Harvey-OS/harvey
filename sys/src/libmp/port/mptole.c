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


// convert an mpint into a little endian byte array (least significant byte first)

//   return number of bytes converted
//   if p == nil, allocate and result array
int
mptole(mpint *b, uint8_t *p, uint n, uint8_t **pp)
{
	int i, j, res;
	mpdigit x;
	uint8_t *e, *s;
	int free_p = 0;

	if(p == nil){
		n = (b->top+1)*Dbytes;
		p = (uint8_t *)malloc(n);
		free_p = 1;
	}
	if(pp != nil)
		*pp = p;
	if(p == nil)
		return -1;
	memset(p, 0, n);

	// special case 0
	if(b->top == 0){
		if (free_p)
			free(p);
		if(n < 1)
			return -1;
		else
			return 0;
	}
	s = p;
	e = s+n;
	for(i = 0; i < b->top-1; i++){
		x = b->p[i];
		for(j = 0; j < Dbytes; j++){
			if(p >= e) {
				if (free_p)
					free(p);
				return -1;
			}

			*p++ = x;
			x >>= 8;
		}
	}
	x = b->p[i];
	while(x > 0){
		if(p >= e) {
			if (free_p)
				free(p);
			return -1;
		}
		*p++ = x;
		x >>= 8;
	}
	res = p - s;
	if (free_p)
		free(p);
	return res;
}
