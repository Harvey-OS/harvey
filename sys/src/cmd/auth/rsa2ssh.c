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
