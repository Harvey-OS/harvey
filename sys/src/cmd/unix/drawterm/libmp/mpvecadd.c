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

// prereq: alen >= blen, sum has at least blen+1 digits
void
mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
{
	int i, carry;
	mpdigit x, y;

	carry = 0;
	for(i = 0; i < blen; i++){
		x = *a++;
		y = *b++;
		x += carry;
		if(x < carry)
			carry = 1;
		else
			carry = 0;
		x += y;
		if(x < y)
			carry++;
		*sum++ = x;
	}
	for(; i < alen; i++){
		x = *a++ + carry;
		if(x < carry)
			carry = 1;
		else
			carry = 0;
		*sum++ = x;
	}
	*sum = carry;
}
