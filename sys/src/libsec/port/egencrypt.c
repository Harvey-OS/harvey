#include "os.h"
#include <mp.h>
#include <libsec.h>

mpint*
egencrypt(EGpub *pub, mpint *in, mpint *out)
{
	mpint *m, *k, *gamma, *delta;
	mpint *p = pub->p, *alpha = pub->alpha;
	int plen = mpsignif(p)+1;
	int shift = ((plen+Dbits-1)/Dbits)*Dbits;
	// in libcrypt version, (int)(LENGTH(pub->p)*sizeof(NumType)*CHARBITS);

	if(out == nil)
		out = mpnew(0);
	m = mpnew(0);
	gamma = mpnew(0);
	delta = mpnew(0);
	mpmod(in, p, m);
	k = mprand(plen/2, genrandom, nil);
	if(mplowbits0(k)>0)
		mpadd(k, mpone, k);
	mpexp(alpha, k, p, gamma);
	mpexp(pub->key, k, p, delta);
	mpmul(m, delta, delta);
	mpmod(delta, p, delta);
	mpleft(gamma, shift, out);
	mpadd(delta, out, out);
	mpfree(m);
	mpfree(k);
	mpfree(gamma);
	mpfree(delta);
	return out;
}
