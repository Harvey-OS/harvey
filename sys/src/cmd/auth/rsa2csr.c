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
#include <bio.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>
#include "rsa2any.h"

void
usage(void)
{
	fprint(2, "usage: aux/rsa2csr 'C=US ...CN=xxx' [key]");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int len;
	uchar *cert;
	RSApriv *key;

	fmtinstall('B', mpfmt);
	fmtinstall('H', encodefmt);

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc != 1 && argc != 2)
		usage();

	if((key = getkey(argc-1, argv+1, 1, nil)) == nil)
		sysfatal("%r");

	cert = X509req(key, argv[0], &len);
	if(cert == nil)
		sysfatal("X509req: %r");

	write(1, cert, len);
	exits(0);
}
