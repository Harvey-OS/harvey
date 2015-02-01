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
#include <libsec.h>
#include "dat.h"

mpint*
mprand(int bits, void (*gen)(uchar*, int), mpint *b)
{
	int n, m;
	mpdigit mask;
	uchar *p;

	n = DIGITS(bits);
	if(b == nil)
		b = mpnew(bits);
	else
		mpbits(b, bits);

	p = malloc(n*Dbytes);
	if(p == nil)
		return nil;
	(*gen)(p, n*Dbytes);
	betomp(p, n*Dbytes, b);
	free(p);

	// make sure we don't give too many bits
	m = bits%Dbits;
	n--;
	if(m > 0){
		mask = 1;
		mask <<= m;
		mask--;
		b->p[n] &= mask;
	}

	for(; n >= 0; n--)
		if(b->p[n] != 0)
			break;
	b->top = n+1;
	b->sign = 1;
	return b;
}
