/*
 * Copyright 2015 - 2017 cinap_lenrek <cinap_lenrek@felloff.net>
 * Copyright 2017 HarveyOS
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "os.h"
#include <mp.h>
#include "dat.h"

/*
 * fast reduction for crandall numbers of the form: 2^n - c
 */

enum {
	MAXDIG = 1024 / Dbits,
};

typedef struct CNfield CNfield;
struct CNfield
{
	Mfield mfield;

	mpint	m[1];

	int	s;
	mpdigit	c;
};

static int
cnreduce(Mfield *m, mpint *a, mpint *r)
{
	mpdigit q[MAXDIG-1], t[MAXDIG], d;
	CNfield *f = (CNfield*)m;
	int qn, tn, k;

	k = f->mfield.mpi.top;
	if((a->top - k) >= MAXDIG)
		return -1;

	mpleft(a, f->s, r);
	if(r->top <= k)
		mpbits(r, (k+1)*Dbits);

	/* q = hi(r) */
	qn = r->top - k;
	memmove(q, r->p+k, qn*Dbytes);

	/* r = lo(r) */
	r->top = k;
	r->sign = 1;

	do {
		/* t = q*c */
		tn = qn+1;
		memset(t, 0, tn*Dbytes);
		mpvecdigmuladd(q, qn, f->c, t);

		/* q = hi(t) */
		qn = tn - k;
		if(qn <= 0) qn = 0;
		else memmove(q, t+k, qn*Dbytes);

		/* r += lo(t) */
		if(tn > k)
			tn = k;
		mpvecadd(r->p, k, t, tn, r->p);

		/* if(r >= m) r -= m */
		mpvecsub(r->p, k+1, f->m->p, k, t);
		d = t[k];
		for(tn = 0; tn < k; tn++)
			r->p[tn] = (r->p[tn] & d) | (t[tn] & ~d);
	} while(qn > 0);

	if(f->s != 0)
		mpright(r, f->s, r);
	mpnorm(r);

	return 0;
}

Mfield*
cnfield(mpint *N)
{
	mpint *M, *C;
	CNfield *f;
	mpdigit d;
	int s;

	if(N->top <= 2 || N->top >= MAXDIG)
		return nil;
	f = nil;
	d = N->p[N->top-1];
	for(s = 0; (d & (mpdigit)1<<(Dbits-1)) == 0; s++)
		d <<= 1;
	C = mpnew(0);
	M = mpcopy(N);
	mpleft(N, s, M);
	mpleft(mpone, M->top*Dbits, C);
	mpsub(C, M, C);
	if(C->top != 1)
		goto out;
	f = mallocz(sizeof(CNfield) + M->top*sizeof(mpdigit), 1);
	if(f == nil)
		goto out;
	f->s = s;
	f->c = C->p[0];
	f->m->size = M->top;
	f->m->p = (mpdigit*)&f[1];
	mpassign(M, f->m);
	mpassign(N, &f->mfield.mpi);
	f->mfield.reduce = cnreduce;
	f->mfield.mpi.flags |= MPfield;
out:
	mpfree(M);
	mpfree(C);

	return &f->mfield;
}
