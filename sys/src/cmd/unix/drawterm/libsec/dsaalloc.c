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

DSApub*
dsapuballoc(void)
{
	DSApub *dsa;

	dsa = mallocz(sizeof(*dsa), 1);
	if(dsa == nil)
		sysfatal("dsapuballoc");
	return dsa;
}

void
dsapubfree(DSApub *dsa)
{
	if(dsa == nil)
		return;
	mpfree(dsa->p);
	mpfree(dsa->q);
	mpfree(dsa->alpha);
	mpfree(dsa->key);
}


DSApriv*
dsaprivalloc(void)
{
	DSApriv *dsa;

	dsa = mallocz(sizeof(*dsa), 1);
	if(dsa == nil)
		sysfatal("dsaprivalloc");
	return dsa;
}

void
dsaprivfree(DSApriv *dsa)
{
	if(dsa == nil)
		return;
	mpfree(dsa->pub.p);
	mpfree(dsa->pub.q);
	mpfree(dsa->pub.alpha);
	mpfree(dsa->pub.key);
	mpfree(dsa->secret);
}

DSAsig*
dsasigalloc(void)
{
	DSAsig *dsa;

	dsa = mallocz(sizeof(*dsa), 1);
	if(dsa == nil)
		sysfatal("dsasigalloc");
	return dsa;
}

void
dsasigfree(DSAsig *dsa)
{
	if(dsa == nil)
		return;
	mpfree(dsa->r);
	mpfree(dsa->s);
}
