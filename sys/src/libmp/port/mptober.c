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

void
mptober(mpint *b, uint8_t *p, int n)
{
	int i, j, m;
	mpdigit x;

	memset(p, 0, n);

	p += n;
	m = b->top*Dbytes;
	if(m < n)
		n = m;

	i = 0;
	while(n >= Dbytes){
		n -= Dbytes;
		x = b->p[i++];
		for(j = 0; j < Dbytes; j++){
			*--p = x;
			x >>= 8;
		}
	}
	if(n > 0){
		x = b->p[i];
		for(j = 0; j < n; j++){
			*--p = x;
			x >>= 8;
		}
	}
}
