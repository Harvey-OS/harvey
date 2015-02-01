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

EGpub*
egprivtopub(EGpriv *priv)
{
	EGpub *pub;

	pub = egpuballoc();
	if(pub == nil)
		return nil;
	pub->p = mpcopy(priv->pub.p);
	pub->alpha = mpcopy(priv->pub.alpha);
	pub->key = mpcopy(priv->pub.key);
	return pub;
}
