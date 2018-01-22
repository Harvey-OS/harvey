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
	fprint(2, "usage: auth/rsafill [file]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	RSApriv *key;
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

	if((key = getkey(argc, argv, 1, &a)) == nil)
		sysfatal("%r");

	s = smprint("key %A size=%d ek=%B !dk=%B n=%B !p=%B !q=%B !kp=%B !kq=%B !c2=%B\n",
		a,
		mpsignif(key->pub.n), key->pub.ek,
		key->dk, key->pub.n, key->p, key->q,
		key->kp, key->kq, key->c2);
	if(s == nil)
		sysfatal("smprint: %r");
	write(1, s, strlen(s));
	exits(nil);
}
