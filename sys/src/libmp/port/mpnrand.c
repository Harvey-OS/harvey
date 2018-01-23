/*
 * Copyright 2015 - 2017 cinap_lenrek <cinap_lenrek@felloff.net>
 * Copyright 2016 mischief <mischief@offblast.org>
 * Copyright 2017 HarveyOS
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "os.h"
#include <mp.h>
#include "dat.h"

/* return uniform random [0..n-1] */
mpint*
mpnrand(mpint *n, void (*gen)(uint8_t*, int), mpint *b)
{
	int bits;

	bits = mpsignif(n);
	if(bits == 0)
		abort();
	if(b == nil){
		b = mpnew(bits);
		setmalloctag(b, getcallerpc());
	}
	do {
		mprand(bits, gen, b);
	} while(mpmagcmp(b, n) >= 0);

	return b;
}
