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

// prereq: a >= b, alen >= blen, diff has at least alen digits
void
mpvecsub(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *diff)
{
	int i, borrow;
	mpdigit x, y;

	borrow = 0;
	for(i = 0; i < blen; i++){
		x = *a++;
		y = *b++;
		y += borrow;
		if(y < borrow)
			borrow = 1;
		else
			borrow = 0;
		if(x < y)
			borrow++;
		*diff++ = x - y;
	}
	for(; i < alen; i++){
		x = *a++;
		y = x - borrow;
		if(y > x)
			borrow = 1;
		else
			borrow = 0;
		*diff++ = y;
	}
}
