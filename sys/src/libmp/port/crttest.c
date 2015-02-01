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

void
testcrt(mpint **p)
{
	CRTpre *crt;
	CRTres *res;
	mpint *m, *x, *y;

	fmtinstall('B', mpfmt);

	// get a modulus and a test number
	m = mpnew(1024+160);
	mpmul(p[0], p[1], m);
	x = mpnew(1024+160);
	mpadd(m, mpone, x);

	// do the precomputation for crt conversion
	crt = crtpre(2, p);

	// convert x to residues
	res = crtin(crt, x);

	// convert back
	y = mpnew(1024+160);
	crtout(crt, res, y);
	print("x %B\ny %B\n", x, y);
	mpfree(m);
	mpfree(x);
	mpfree(y);
}

void
main(void)
{
	int i;
	mpint *p[2];
	long start;

	start = time(0);
	for(i = 0; i < 10; i++){
		p[0] = mpnew(1024);
		p[1] = mpnew(1024);
		DSAprimes(p[0], p[1], nil);
		testcrt(p);
		mpfree(p[0]);
		mpfree(p[1]);
	}
	print("%ld secs with more\n", time(0)-start);
	exits(0);
}
