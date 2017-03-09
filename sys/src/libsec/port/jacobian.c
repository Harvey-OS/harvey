/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */
 
#include "os.h"
#include <mp.h>

void
jacobian_new(mpint *x, mpint *y, mpint *z, mpint *X, mpint *Y, mpint *Z){
  mpassign(x, X);
  mpassign(y, Y);
  mpassign(z, Z);
}

void
jacobian_inf(mpint *X, mpint *Y, mpint *Z){
  jacobian_new(mpzero, mpone, mpzero, X, Y, Z);
}

void
jacobian_affine(mpint *p, mpint *X, mpint *Y, mpint *Z){
  mpint *ZZZ = mpnew(0);
  mpint *ZZ = mpnew(0);
  if (mpcmp(Z, mpzero)!=0){
    mpmodmul(Z, Z, p, ZZ);
    mpmodmul(ZZ, Z, p, ZZZ);
    mpint *tmp1 = mpnew(0);
    mpinvert(ZZ, p, tmp1);
    mpmodmul(X, tmp1, p, X);
    mpfree(tmp1);
    tmp1 = mpnew(0);
    mpinvert(ZZZ, p, tmp1);
    mpmodmul(Y, tmp1, p, Y);
    mpfree(tmp1);
    mpassign(mpone, Z);
  }
  mpfree(ZZ);
  mpfree(ZZZ);
}

void
jacobian_dbl(mpint *p, mpint *a, mpint *X1, mpint *Y1, mpint *Z1, mpint *X3, mpint *Y3, mpint *Z3){
	mpint *M = mpnew(0);
	mpint *S = mpnew(0);
	mpint *ZZ = mpnew(0);
	mpint *YYYY = mpnew(0);
	mpint *YY = mpnew(0);
	mpint *XX = mpnew(0);
	if(mpcmp(Y1, mpzero) == 0){
		jacobian_inf(X3, Y3, Z3);
		}else{
		mpmodmul(X1, X1, p, XX);
		mpmodmul(Y1, Y1, p, YY);
		mpmodmul(YY, YY, p, YYYY);
		mpmodmul(Z1, Z1, p, ZZ);
		mpint *tmp1 = mpnew(0);
		mpmodadd(X1, YY, p, tmp1);
		mpmodmul(tmp1, tmp1, p, tmp1);
		mpmodsub(tmp1, XX, p, tmp1);
		mpmodsub(tmp1, YYYY, p, tmp1);
		mpmodadd(tmp1, tmp1, p, S); // 2*tmp1
		mpfree(tmp1);
		tmp1 = mpnew(0);
		uitomp(3UL, tmp1);
		mpmodmul(tmp1, XX, p, M);
		mpfree(tmp1);
		tmp1 = mpnew(0);
		mpint *tmp2 = mpnew(0);
		mpmodmul(ZZ, ZZ, p, tmp2);
		mpmodmul(a, tmp2, p, tmp1);
		mpfree(tmp2);
		mpmodadd(M, tmp1, p, M);
		mpfree(tmp1);
		mpmodadd(Y1, Z1, p, Z3);
		mpmodmul(Z3, Z3, p, Z3);
		mpmodsub(Z3, YY, p, Z3);
		mpmodsub(Z3, ZZ, p, Z3);
		mpmodmul(M, M, p, X3);
		tmp1 = mpnew(0);
		mpmodadd(S, S, p, tmp1); // 2*S
		mpmodsub(X3, tmp1, p, X3);
		mpfree(tmp1);
		tmp1 = mpnew(0);
		mpmodsub(S, X3, p, tmp1);
		mpmodmul(M, tmp1, p, Y3);
		mpfree(tmp1);
		tmp1 = mpnew(0);
		tmp2 = mpnew(0);
		uitomp(8UL, tmp2);
		mpmodmul(tmp2, YYYY, p, tmp1);
		mpfree(tmp2);
		mpmodsub(Y3, tmp1, p, Y3);
		mpfree(tmp1);
		}
	mpfree(M);
	mpfree(S);
	mpfree(ZZ);
	mpfree(YYYY);
	mpfree(YY);
	mpfree(XX);
}

void
jacobian_add(mpint *p, mpint *a, mpint *X1, mpint *Y1, mpint *Z1, mpint *X2, mpint *Y2, mpint *Z2, mpint *X3, mpint *Y3, mpint *Z3){
	mpint *V = mpnew(0);
	mpint *r = mpnew(0);
	mpint *J = mpnew(0);
	mpint *I = mpnew(0);
	mpint *H = mpnew(0);
	mpint *S2 = mpnew(0);
	mpint *S1 = mpnew(0);
	mpint *U2 = mpnew(0);
	mpint *U1 = mpnew(0);
	mpint *Z2Z2 = mpnew(0);
	mpint *Z1Z1 = mpnew(0);
	mpmodmul(Z1, Z1, p, Z1Z1);
	mpmodmul(Z2, Z2, p, Z2Z2);
	mpmodmul(X1, Z2Z2, p, U1);
	mpmodmul(X2, Z1Z1, p, U2);
	mpint *tmp1 = mpnew(0);
	mpmodmul(Y1, Z2, p, tmp1);
	mpmodmul(tmp1, Z2Z2, p, S1);
	mpfree(tmp1);
	tmp1 = mpnew(0);
	mpmodmul(Y2, Z1, p, tmp1);
	mpmodmul(tmp1, Z1Z1, p, S2);
	mpfree(tmp1);
	if(mpcmp(U1, U2) == 0){
		if(mpcmp(S1, S2) != 0){
			jacobian_inf(X3, Y3, Z3);
			}else{
			jacobian_dbl(p, a, X1, Y1, Z1, X3, Y3, Z3);
			}
		}else{
		mpmodsub(U2, U1, p, H);
		mpmodadd(H, H, p, I); // 2*H
		mpmodmul(I, I, p, I);
		mpmodmul(H, I, p, J);
		mpint *tmp2 = mpnew(0);
		mpmodsub(S2, S1, p, tmp2);
		mpmodadd(tmp2, tmp2, p, r); // 2*tmp2
		mpfree(tmp2);
		mpmodmul(U1, I, p, V);
		mpmodmul(r, r, p, X3);
		mpmodsub(X3, J, p, X3);
		tmp2 = mpnew(0);
		mpmodadd(V, V, p, tmp2); // 2*V
		mpmodsub(X3, tmp2, p, X3);
		mpfree(tmp2);
		tmp2 = mpnew(0);
		mpmodsub(V, X3, p, tmp2);
		mpmodmul(r, tmp2, p, Y3);
		mpfree(tmp2);
		tmp2 = mpnew(0);
		mpint *tmp3 = mpnew(0);
		mpmodadd(S1, S1, p, tmp3); // 2*S1
		mpmodmul(tmp3, J, p, tmp2);
		mpfree(tmp3);
		mpmodsub(Y3, tmp2, p, Y3);
		mpfree(tmp2);
		tmp2 = mpnew(0);
		mpmodadd(Z1, Z2, p, tmp2);
		mpmodmul(tmp2, tmp2, p, tmp2);
		mpmodsub(tmp2, Z1Z1, p, tmp2);
		mpmodsub(tmp2, Z2Z2, p, tmp2);
		mpmodmul(tmp2, H, p, Z3);
		mpfree(tmp2);
		}
	mpfree(V);
	mpfree(r);
	mpfree(J);
	mpfree(I);
	mpfree(H);
	mpfree(S2);
	mpfree(S1);
	mpfree(U2);
	mpfree(U1);
	mpfree(Z2Z2);
	mpfree(Z1Z1);
}
