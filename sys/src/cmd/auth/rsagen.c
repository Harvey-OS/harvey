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
	fprint(2, "usage: auth/rsagen [-b bits] [-t 'attr=value attr=value ...']\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *s;
	int bits;
	char *tag;
	RSApriv *key;

	bits = 1024;
	tag = nil;
	key = nil;
	fmtinstall('B', mpfmt);

	ARGBEGIN{
	case 'b':
		bits = atoi(EARGF(usage()));
		if(bits == 0)
			usage();
		break;
	case 't':
		tag = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	do{
		if(key)
			rsaprivfree(key);
		key = rsagen(bits, 6, 0);
	}while(mpsignif(key->pub.n) != bits);

	s = smprint("key proto=rsa %s%ssize=%d ek=%B !dk=%B n=%B !p=%B !q=%B !kp=%B !kq=%B !c2=%B\n",
		tag ? tag : "", tag ? " " : "",
		mpsignif(key->pub.n), key->pub.ek,
		key->dk, key->pub.n, key->p, key->q,
		key->kp, key->kq, key->c2);
	if(s == nil)
		sysfatal("smprint: %r");

	if(write(1, s, strlen(s)) != strlen(s))
		sysfatal("write: %r");
	
	exits(nil);
}
