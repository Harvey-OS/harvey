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

//
//	divide two digits by one and return quotient
//
void
mpdigdiv(mpdigit *dividend, mpdigit divisor, mpdigit *quotient)
{
	mpdigit hi, lo, q, x, y;
	int i;

	hi = dividend[1];
	lo = dividend[0];

	// return highest digit value if the result >= 2**32
	if(hi >= divisor || divisor == 0){
		divisor = 0;
		*quotient = ~divisor;
		return;
	}

	// at this point we know that hi < divisor
	// just shift and subtract till we're done
	q = 0;
	x = divisor;
	for(i = Dbits-1; hi > 0 && i >= 0; i--){
		x >>= 1;
		if(x > hi)
			continue;
		y = divisor<<i;
		if(x == hi && y > lo)
			continue;
		if(y > lo)
			hi--;
		lo -= y;
		hi -= x;
		q |= 1<<i;
	}
	q += lo/divisor;
	*quotient = q;
}
