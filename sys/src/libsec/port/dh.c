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
