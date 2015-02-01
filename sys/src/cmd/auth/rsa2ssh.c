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
#include <auth.h>
#include <mp.h>
#include <libsec.h>
#include "rsa2any.h"

void
usage(void)
{
	fprint(2, "usage: auth/rsa2ssh [file]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	RSApriv *k;

	fmtinstall('B', mpfmt);

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();

	if((k = getkey(argc, argv, 0, nil)) == nil)
		sysfatal("%r");

	print("%d %.10B %.10B\n", mpsignif(k->pub.n), k->pub.ek, k->pub.n);
	exits(nil);
}
