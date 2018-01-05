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
#include <bio.h>

void
main(void)
{
	int n;
	int64_t start;
	char *p;
	uint8_t buf[4096];
	Biobuf b;
	RSApriv *rsa;
	mpint *clr, *enc, *clr2;

	fmtinstall('B', mpfmt);

	rsa = rsagen(1024, 16, 0);
	if(rsa == nil)
		sysfatal("rsagen");
	Binit(&b, 0, OREAD);
	clr = mpnew(0);
	clr2 = mpnew(0);
	enc = mpnew(0);

	strtomp("123456789abcdef123456789abcdef123456789abcdef123456789abcdef", nil, 16, clr);
	rsaencrypt(&rsa->pub, clr, enc);

	start = nsec();
	for(n = 0; n < 10; n++)
		rsadecrypt(rsa, enc, clr);
	print("%lld\n", nsec()-start);

	start = nsec();
	for(n = 0; n < 10; n++)
		mpexp(enc, rsa->dk, rsa->pub.n, clr2);
	print("%lld\n", nsec()-start);

	if(mpcmp(clr, clr2) != 0)
		print("%B != %B\n", clr, clr2);

	print("> ");
	while(p = Brdline(&b, '\n')){
		n = Blinelen(&b);
		letomp((uint8_t*)p, n, clr);
		print("clr %B\n", clr);
		rsaencrypt(&rsa->pub, clr, enc);
		print("enc %B\n", enc);
		rsadecrypt(rsa, enc, clr);
		print("clr %B\n", clr);
		n = mptole(clr, buf, sizeof(buf), nil);
		write(1, buf, n);
		print("> ");
	}
}
