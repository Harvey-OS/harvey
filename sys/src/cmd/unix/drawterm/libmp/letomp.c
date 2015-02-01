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

// convert a little endian byte array (least significant byte first) to an mpint
mpint*
letomp(uchar *s, uint n, mpint *b)
{
	int i=0, m = 0;
	mpdigit x=0;

	if(b == nil)
		b = mpnew(0);
	mpbits(b, 8*n);
	b->top = DIGITS(8*n);
	for(; n > 0; n--){
		x |= ((mpdigit)(*s++)) << i;
		i += 8;
		if(i == Dbits){
			b->p[m++] = x;
			i = 0;
			x = 0;
		}
	}
	if(i > 0)
		b->p[m++] = x;
	b->top = m;
	return b;
}
