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
	fprint(2, "usage: auth/dsa2pub [file]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	DSApriv *key;
	Attr *a;
	char *s;

	fmtinstall('A', _attrfmt);
	fmtinstall('B', mpfmt);
	quotefmtinstall();

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();

	if((key = getdsakey(argc, argv, 0, &a)) == nil)
		sysfatal("%r");

	s = smprint("key %A p=%B q=%B alpha=%B key=%B\n",
		a, 
		key->pub.p, key->pub.q, key->pub.alpha, key->pub.key);
	if(s == nil)
		sysfatal("smprint: %r");
	write(1, s, strlen(s));
	exits(nil);
}
