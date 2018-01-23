/*
 * Copyright 2014 - 2017 cinap_lenrek <cinap_lenrek@felloff.net>
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

#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>

mpint*
dh_new(DHstate *dh, mpint *p, mpint *q, mpint *g)
{
	mpint *pm1;
	int n;

	memset(dh, 0, sizeof(*dh));
	if(mpcmp(g, mpone) <= 0)
		return nil;

	n = mpsignif(p);
	pm1 = mpnew(n);
	mpsub(p, mpone, pm1);
	dh->p = mpcopy(p);
	dh->g = mpcopy(g);
	dh->q = mpcopy(q != nil ? q : pm1);
	dh->x = mpnew(mpsignif(dh->q));
	dh->y = mpnew(n);
	for(;;){
		mpnrand(dh->q, genrandom, dh->x);
		mpexp(dh->g, dh->x, dh->p, dh->y);
		if(mpcmp(dh->y, mpone) > 0 && mpcmp(dh->y, pm1) < 0)
			break;
	}
	mpfree(pm1);

	return dh->y;
}

mpint*
dh_finish(DHstate *dh, mpint *y)
{
	mpint *k = nil;

	if(y == nil || dh->x == nil || dh->p == nil || dh->q == nil)
		goto Out;

	/* y > 1 */
	if(mpcmp(y, mpone) <= 0)
		goto Out;

	k = mpnew(mpsignif(dh->p));

	/* y < p-1 */
	mpsub(dh->p, mpone, k);
	if(mpcmp(y, k) >= 0){
Bad:
		mpfree(k);
		k = nil;
		goto Out;
	}

	/* y**q % p == 1 if q < p-1 */
	if(mpcmp(dh->q, k) < 0){
		mpexp(y, dh->q, dh->p, k);
		if(mpcmp(k, mpone) != 0)
			goto Bad;
	}

	mpexp(y, dh->x, dh->p, k);

Out:
	mpfree(dh->p);
	mpfree(dh->q);
	mpfree(dh->g);
	mpfree(dh->x);
	mpfree(dh->y);
	memset(dh, 0, sizeof(*dh));
	return k;
}
