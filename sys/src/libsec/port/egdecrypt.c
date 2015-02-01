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
egdecrypt(EGpriv *priv, mpint *in, mpint *out)
{
	EGpub *pub = &priv->pub;
	mpint *gamma, *delta;
	mpint *p = pub->p;
	int plen = mpsignif(p)+1;
	int shift = ((plen+Dbits-1)/Dbits)*Dbits;

	if(out == nil)
		out = mpnew(0);
	gamma = mpnew(0);
	delta = mpnew(0);
	mpright(in, shift, gamma);
	mpleft(gamma, shift, delta);
	mpsub(in, delta, delta);	
	mpexp(gamma, priv->secret, p, out);
	mpinvert(out, p, gamma);
	mpmul(gamma, delta, out);
	mpmod(out, p, out);
	mpfree(gamma);
	mpfree(delta);
	return out;
}
