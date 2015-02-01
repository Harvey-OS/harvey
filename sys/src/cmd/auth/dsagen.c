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
#include <mp.h>
#include <libsec.h>

void
usage(void)
{
	fprint(2, "usage: auth/dsagen [-t 'attr=value attr=value ...']\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *s, *tag;
	DSApriv *key;

	tag = nil;
	fmtinstall('B', mpfmt);

	ARGBEGIN{
	case 't':
		tag = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	key = dsagen(nil);

	s = smprint("key proto=dsa %s%sp=%B q=%B alpha=%B key=%B !secret=%B\n",
		tag ? tag : "", tag ? " " : "",
		key->pub.p, key->pub.q, key->pub.alpha, key->pub.key,
		key->secret);
	if(s == nil)
		sysfatal("smprint: %r");

	if(write(1, s, strlen(s)) != strlen(s))
		sysfatal("write: %r");
	
	exits(nil);
}
