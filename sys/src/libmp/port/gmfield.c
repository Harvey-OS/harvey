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
#include "dat.h"

/*
 * fast reduction for generalized mersenne numbers (GM)
 * using a series of additions and subtractions.
 */

enum {
	MAXDIG = 1024/Dbits,
};

typedef struct GMfield GMfield;
struct GMfield
{
	Mfield mfield;

	mpint	m2[1];

	int	nadd;
	int	nsub;
	int	indx[256];
};

static int
gmreduce(Mfield *m, mpint *a, mpint *r)
{
	GMfield *g = (GMfield*)m;
	mpdigit d0, t[MAXDIG];
	int i, j, d, *x;

	if(mpmagcmp(a, g->m2) >= 0)
		return -1;

	if(a != r)
		mpassign(a, r);

	d = g->mfield.mpi.top;
	mpbits(r, (d+1)*Dbits*2);
	memmove(t+d, r->p+d, d*Dbytes);

	r->sign = 1;
	r->top = d;
	r->p[d] = 0;

	if(g->nsub > 0)
		mpvecdigmuladd(g->mfield.mpi.p, d, g->nsub, r->p);

	x = g->indx;
	for(i=0; i<g->nadd; i++){
		t[0] = 0;
		d0 = t[*x++];
		for(j=1; j<d; j++)
			t[j] = t[*x++];
		t[0] = d0;

		mpvecadd(r->p, d+1, t, d, r->p);
	}

	for(i=0; i<g->nsub; i++){
		t[0] = 0;
		d0 = t[*x++];
		for(j=1; j<d; j++)
			t[j] = t[*x++];
		t[0] = d0;

		mpvecsub(r->p, d+1, t, d, r->p);
	}

	mpvecdigmulsub(g->mfield.mpi.p, d, r->p[d], r->p);
	r->p[d] = 0;

	mpvecsub(r->p, d+1, g->mfield.mpi.p, d, r->p+d+1);
	d0 = r->p[2*d+1];
	for(j=0; j<d; j++)
		r->p[j] = (r->p[j] & d0) | (r->p[j+d+1] & ~d0);

	mpnorm(r);

	return 0;
}

Mfield*
gmfield(mpint *N)
{
	int i,j,d, s, *C, *X, *x, *e;
	mpint *M, *T;
	GMfield *g;

	d = N->top;
	if(d <= 2 || d > MAXDIG/2 || (mpsignif(N) % Dbits) != 0)
		return nil;
	g = nil;
	T = mpnew(0);
	M = mpcopy(N);
	C = malloc(sizeof(int)*(d+1));
	X = malloc(sizeof(int)*(d*d));
	if(C == nil || X == nil)
		goto out;

	for(i=0; i<=d; i++){
		if((M->p[i]>>8) != 0 && (~M->p[i]>>8) != 0)
			goto out;
		j = M->p[i];
		C[d - i] = -j;
		itomp(j, T);
		mpleft(T, i*Dbits, T);
		mpsub(M, T, M);
	}
	for(j=0; j<d; j++)
		X[j] = C[d-j];
	for(i=1; i<d; i++){
		X[d*i] = X[d*(i-1) + d-1]*C[d];
		for(j=1; j<d; j++)
			X[d*i + j] = X[d*(i-1) + j-1] + X[d*(i-1) + d-1]*C[d-j];
	}
	g = mallocz(sizeof(GMfield) + (d+1)*sizeof(mpdigit)*2, 1);
	if(g == nil)
		goto out;

	g->m2->p = (mpdigit*)&g[1];
	g->m2->size = d*2+1;
	mpmul(N, N, g->m2);
	mpassign(N, &g->mfield.mpi);
	g->mfield.reduce = gmreduce;
	g->mfield.mpi.flags |= MPfield;

	s = 0;
	x = g->indx;
	e = x + nelem(g->indx) - d;
	for(g->nadd=0; x <= e; x += d, g->nadd++){
		s = 0;
		for(i=0; i<d; i++){
			for(j=0; j<d; j++){
				if(X[d*i+j] > 0 && x[j] == 0){
					X[d*i+j]--;
					x[j] = d+i;
					s = 1;
					break;
				}
			}
		}
		if(s == 0)
			break;
	}
	for(g->nsub=0; x <= e; x += d, g->nsub++){
		s = 0;
		for(i=0; i<d; i++){
			for(j=0; j<d; j++){
				if(X[d*i+j] < 0 && x[j] == 0){
					X[d*i+j]++;
					x[j] = d+i;
					s = 1;
					break;
				}
			}
		}
		if(s == 0)
			break;
	}
	if(s != 0){
		mpfree(&g->mfield.mpi);
		g = nil;
	}
out:
	free(C);
	free(X);
	mpfree(M);
	mpfree(T);
	if (g==nil){
		return nil;
	}
	return &g->mfield;
}
