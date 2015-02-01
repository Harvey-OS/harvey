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
rsapuballoc(void)
{
	RSApub *rsa;

	rsa = mallocz(sizeof(*rsa), 1);
	if(rsa == nil)
		sysfatal("rsapuballoc");
	return rsa;
}

void
rsapubfree(RSApub *rsa)
{
	if(rsa == nil)
		return;
	mpfree(rsa->ek);
	mpfree(rsa->n);
	free(rsa);
}


RSApriv*
rsaprivalloc(void)
{
	RSApriv *rsa;

	rsa = mallocz(sizeof(*rsa), 1);
	if(rsa == nil)
		sysfatal("rsaprivalloc");
	return rsa;
}

void
rsaprivfree(RSApriv *rsa)
{
	if(rsa == nil)
		return;
	mpfree(rsa->pub.ek);
	mpfree(rsa->pub.n);
	mpfree(rsa->dk);
	mpfree(rsa->p);
	mpfree(rsa->q);
	mpfree(rsa->kp);
	mpfree(rsa->kq);
	mpfree(rsa->c2);
	free(rsa);
}
