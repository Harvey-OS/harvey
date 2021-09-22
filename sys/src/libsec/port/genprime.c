#include "os.h"
#include <mp.h>
#include <libsec.h>

//  generate a probable prime.  accuracy is the miller-rabin interations
void
genprime(mpint *p, int n, int accuracy)
{
	int loops;
	mpdigit x;

	// generate n random bits with high and low bits set
	mpbits(p, n);
	genrandom((uchar*)p->p, (n+7)/8);
	p->top = (n+Dbits-1)/Dbits;
	x = 1;
	x <<= ((n-1)%Dbits);
	p->p[p->top-1] &= (x-1);
	p->p[p->top-1] |= x;
	p->p[0] |= 1;

	// keep incrementing till it looks prime (or we fail).
	// largest known prime gap is 1113106 says wikipedia on 9 aug 2021.
	loops = 1113106/2 + 1;
	for(;;){
		if(probably_prime(p, accuracy))
			break;
		if(--loops <= 0) {
			fprint(2, "genprime %d bits: failed\n", n);
			*p = *mpcopy(mptwo);
			break;
		}
		mpadd(p, mptwo, p);
	}
}
