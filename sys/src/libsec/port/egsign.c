/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>

EGsig*
egsign(EGpriv *priv, mpint *m)
{
	EGpub *pub = &priv->pub;
	EGsig *sig;
	mpint *pm1, *k, *kinv, *r, *s;
	mpint *p = pub->p, *alpha = pub->alpha;
	int plen = mpsignif(p);

	pm1 = mpnew(0);
	kinv = mpnew(0);
	r = mpnew(0);
	s = mpnew(0);
	k = mpnew(0);
	mpsub(p, mpone, pm1);
	while(1){
		mprand(plen, genrandom, k);
		if((mpcmp(mpone, k) > 0) || (mpcmp(k, pm1) >= 0))
			continue;
		mpextendedgcd(k, pm1, r, kinv, s);
		if(mpcmp(r, mpone) != 0)
			continue;
		break;
	}
	mpmod(kinv, pm1, kinv);  // make kinv positive
	mpexp(alpha, k, p, r);
	mpmul(priv->secret, r, s);
	mpmod(s, pm1, s);
	mpsub(m, s, s);
	mpmul(kinv, s, s);
	mpmod(s, pm1, s);
	sig = egsigalloc();
	sig->r = r;
	sig->s = s;
	mpfree(pm1);
	mpfree(k);
	mpfree(kinv);
	return sig;
}
