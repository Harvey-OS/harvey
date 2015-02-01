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

EGpriv*
eggen(int nlen, int rounds)
{
	EGpub *pub;
	EGpriv *priv;

	priv = egprivalloc();
	pub = &priv->pub;
	pub->p = mpnew(0);
	pub->alpha = mpnew(0);
	pub->key = mpnew(0);
	priv->secret = mpnew(0);
	gensafeprime(pub->p, pub->alpha, nlen, rounds);
	mprand(nlen-1, genrandom, priv->secret);
	mpexp(pub->alpha, priv->secret, pub->p, pub->key);
	return priv;
}
