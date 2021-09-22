#include "os.h"
#include <mp.h>
#include <libsec.h>

// NIST algorithm for generating DSA primes
// Menezes et al (1997) Handbook of Applied Cryptography, p.151
// q is a 160-bit prime;  p is a 1024-bit prime;  q divides p-1.
//
// FIPS 186-4 calls 160 N and 1024 L, and recommends one of these
// pairs: (L=1024, N=160), (L=2048, N=224), (L=2048, N=256),
// (L=3072, N=256).

enum {
	L = 1024,		/* bits in p */
	N = 160,		/* bits in q */

	NBY = N / 8,		/* = SHA1dlen for N=160 */
};

// arithmetic on unsigned ints mod 2**N, represented
//    as NBY-byte, little-endian uchar array

static void
Hrand(uchar *s)
{
	int i;
	ulong *u = (ulong*)s;

	for (i = NBY/sizeof *u; i-- > 0; )
		*u++ = fastrand();
}

static void
Hincr(uchar *s)
{
	int i;
	for(i=0; i < NBY; i++)
		if(++s[i]!=0)
			break;
}

// this can run for quite a while;  be patient
void
DSAprimes(mpint *q, mpint *p, uchar seed[NBY])
{
	int i, j, k, n = 6, b = 63;
	uchar s[NBY], Hs[NBY], Hs1[NBY], sj[NBY], sjk[NBY];
	mpint *twolm1, *mb, *Vk, *W, *X, *q2;

	twolm1 = mpnew(L);
	mpleft(mpone, L-1, twolm1);
	mb = mpnew(0);
	mpleft(mpone, b, mb);
	W = mpnew(L);
	Vk = mpnew(L);
	X = mpnew(0);
	q2 = mpnew(0);
forever:
	do{
		Hrand(s);
		memcpy(sj, s, NBY);
		sha1(s, NBY, Hs, 0);
		Hincr(sj);
		sha1(sj, NBY, Hs1, 0);
		for(i=0; i<NBY; i++)
			Hs[i] ^= Hs1[i];
		Hs[0] |= 1;
		Hs[NBY-1] |= 0x80;
		letomp(Hs, NBY, q);
	}while(!probably_prime(q, 18));
	if(seed != nil)	// allow skeptics to confirm computation
		memmove(seed, s, NBY);
	j = 2;
	Hincr(sj);
	mpleft(q, 1, q2);
	for(i = 0; i<4096; i++){
		memcpy(sjk, sj, NBY);
		for(k=0; k <= n; k++){
			sha1(sjk, NBY, Hs, 0);
			letomp(Hs, NBY, Vk);
			if(k == n)
				mpmod(Vk, mb, Vk);
			mpleft(Vk, N*k, Vk);
			mpadd(W, Vk, W);
			Hincr(sjk);
		}
		mpadd(W, twolm1, X);
		mpmod(X, q2, W);
		mpsub(W, mpone, W);
		mpsub(X, W, p);
		if(mpcmp(p, twolm1)>=0 && probably_prime(p, 5))
			goto done;
		j += n+1;
		for(k=0; k<n+1; k++)
			Hincr(sj);
	}
	goto forever;
done:
	mpfree(q2);
	mpfree(X);
	mpfree(Vk);
	mpfree(W);
	mpfree(mb);
	mpfree(twolm1);
}
