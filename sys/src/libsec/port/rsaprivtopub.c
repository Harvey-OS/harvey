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

RSApub*
rsaprivtopub(RSApriv *priv)
{
	RSApub *pub;

	pub = rsapuballoc();
	if(pub == nil)
		return nil;
	pub->n = mpcopy(priv->pub.n);
	pub->ek = mpcopy(priv->pub.ek);
	return pub;
}
