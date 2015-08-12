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

char* sfactors[] = {
    "3", "5", "17", "257", "641", "65537", "274177", "2424833", "6700417",
    "45592577", "6487031809", "67280421310721", "1238926361552897",
    "59649589127497217", "5704689200685129054721",
    "4659775785220018543264560743076778192897",
    "7455602825647884208337395736200454918783366342657",
    "93461639715357977769163558199606896584051237541638188580280321",

    "74164006262753080152478714190193747405994078109751902390582131614441575950"
    "4705008092818711693940737",

    "13043987440548818972748476879650990394660853084161189218689529577683241625"
    "14718635741402279775731048958987839288429238448311490329137987290886016179"
    "46094119449010595906710130531906171018354491609619193912488538116080712299"
    "672322806217820753127014424577"};

int32_t start;

void
printmp(mpint* b, char* tag)
{
	int n;
	char* p;

	print("%s (%d) ", tag, b->top);
	p = mptoa(b, 10, nil, 0);
	write(1, p, strlen(p));
	free(p);
	print("\n");
}

int
timing(void)
{
	int32_t now, span;

	now = time(0);
	span = now - start;
	start = now;

	return span;
}

int expdebug;

void
main(int argc, char** argv)
{
	mpint* p, *k, *d, *b, *e, *x, *r;
	int i;

	start = time(0);
	fmtinstall('B', mpconv);
	mpsetminbits(2 * Dbits);

	x = mpnew(0);
	e = mpnew(0);
	r = mpnew(0);
	p = mpnew(0);

	// b = 2^32
	b = mpcopy(mpone);
	mpleft(b, 32, b);

	// 2^29440
	p = mpcopy(mpone);
	mpleft(p, 29440, p);
	// 2^27392
	k = mpcopy(mpone);
	mpleft(k, 27392, k);
	// k = 2^29440 - 2^27392
	mpsub(p, k, k);
	// p = 2^29440 - 2^27392 + 1
	mpadd(k, mpone, p);

	//	if(!probably_prime(p, 18)){
	//		print("not a prime\n");
	//		exits(0);
	//	}
	//	print("probably prime\n");

	mpright(k, 10, k);
	printmp(k, "k =");

	expdebug = 1;
	mpexp(b, k, p, x);
	printmp(x, "x =");
	print("timing %d\n", timing());

	for(i = 0; i < nelem(sfactors); i++) {
		d = strtomp(sfactors[i], nil, 10, nil);
		// e = k/d
		mpdiv(k, d, e, r);
		printmp(r, "r =");

		// x = b^e mod p
		mpexp(b, e, p, x);
		printmp(x, "x =");
		print("timing %d\n", timing());
	}
}
