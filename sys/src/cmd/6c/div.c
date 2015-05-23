/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "gc.h"

/*
 * Based on: Granlund, T.; Montgomery, P.L.
 * "Division by Invariant Integers using Multiplication".
 * SIGPLAN Notices, Vol. 29, June 1994, page 61.
 */

#define	TN(n)	((uint64_t)1 << (n))
#define	T31	TN(31)
#define	T32	TN(32)

int
multiplier(uint32_t d, int p, uint64_t *mp)
{
	int l;
	uint64_t mlo, mhi, tlo, thi;

	l = topbit(d - 1) + 1;
	mlo = (((TN(l) - d) << 32) / d) + T32;
	if(l + p == 64)
		mhi = (((TN(l) + 1 - d) << 32) / d) + T32;
	else
		mhi = (TN(32 + l) + TN(32 + l - p)) / d;
	/*assert(mlo < mhi);*/
	while(l > 0) {
		tlo = mlo >> 1;
		thi = mhi >> 1;
		if(tlo == thi)
			break;
		mlo = tlo;
		mhi = thi;
		l--;
	}
	*mp = mhi;
	return l;
}

int
sdiv(uint32_t d, uint32_t *mp, int *sp)
{
	int s;
	uint64_t m;

	s = multiplier(d, 32 - 1, &m);
	*mp = m;
	*sp = s;
	if(m >= T31)
		return 1;
	else
		return 0;
}

int
udiv(uint32_t d, uint32_t *mp, int *sp, int *pp)
{
	int p, s;
	uint64_t m;

	s = multiplier(d, 32, &m);
	p = 0;
	if(m >= T32) {
		while((d & 1) == 0) {
			d >>= 1;
			p++;
		}
		s = multiplier(d, 32 - p, &m);
	}
	*mp = m;
	*pp = p;
	if(m >= T32) {
		/*assert(p == 0);*/
		*sp = s - 1;
		return 1;
	}
	else {
		*sp = s;
		return 0;
	}
}

void
sdivgen(Node *l, Node *r, Node *ax, Node *dx)
{
	int a, s;
	uint32_t m;
	int64_t c;

	c = r->vconst;
	if(c < 0)
		c = -c;
	a = sdiv(c, &m, &s);
//print("a=%d i=%ld s=%d m=%lux\n", a, (long)r->vconst, s, m);
	gins(AMOVL, nodconst(m), ax);
	gins(AIMULL, l, Z);
	gins(AMOVL, l, ax);
	if(a)
		gins(AADDL, ax, dx);
	gins(ASHRL, nodconst(31), ax);
	gins(ASARL, nodconst(s), dx);
	gins(AADDL, ax, dx);
	if(r->vconst < 0)
		gins(ANEGL, Z, dx);
}

void
udivgen(Node *l, Node *r, Node *ax, Node *dx)
{
	int a, s, t;
	uint32_t m;
	Node nod;

	a = udiv(r->vconst, &m, &s, &t);
//print("a=%ud i=%ld p=%d s=%d m=%lux\n", a, (long)r->vconst, t, s, m);
	if(t != 0) {
		gins(AMOVL, l, ax);
		gins(ASHRL, nodconst(t), ax);
		gins(AMOVL, nodconst(m), dx);
		gins(AMULL, dx, Z);
	}
	else if(a) {
		if(l->op != OREGISTER) {
			regalloc(&nod, l, Z);
			gins(AMOVL, l, &nod);
			l = &nod;
		}
		gins(AMOVL, nodconst(m), ax);
		gins(AMULL, l, Z);
		gins(AADDL, l, dx);
		gins(ARCRL, nodconst(1), dx);
		if(l == &nod)
			regfree(l);
	}
	else {
		gins(AMOVL, nodconst(m), ax);
		gins(AMULL, l, Z);
	}
	if(s != 0)
		gins(ASHRL, nodconst(s), dx);
}

void
sext(Node *d, Node *s, Node *l)
{
	if(s->reg == D_AX && !nodreg(d, Z, D_DX)) {
		reg[D_DX]++;
		gins(ACDQ, Z, Z);
	}
	else {
		regalloc(d, l, Z);
		gins(AMOVL, s, d);
		gins(ASARL, nodconst(31), d);
	}
}

void
sdiv2(int32_t c, int v, Node *l, Node *n)
{
	Node nod;

	if(v > 0) {
		if(v > 1) {
			sext(&nod, n, l);
			gins(AANDL, nodconst((1 << v) - 1), &nod);
			gins(AADDL, &nod, n);
			regfree(&nod);
		}
		else {
			gins(ACMPL, n, nodconst(0x80000000));
			gins(ASBBL, nodconst(-1), n);
		}
		gins(ASARL, nodconst(v), n);
	}
	if(c < 0)
		gins(ANEGL, Z, n);
}

void
smod2(int32_t c, int v, Node *l, Node *n)
{
	Node nod;

	if(c == 1) {
		zeroregm(n);
		return;
	}

	sext(&nod, n, l);
	if(v == 0) {
		zeroregm(n);
		gins(AXORL, &nod, n);
		gins(ASUBL, &nod, n);
	}
	else if(v > 1) {
		gins(AANDL, nodconst((1 << v) - 1), &nod);
		gins(AADDL, &nod, n);
		gins(AANDL, nodconst((1 << v) - 1), n);
		gins(ASUBL, &nod, n);
	}
	else {
		gins(AANDL, nodconst(1), n);
		gins(AXORL, &nod, n);
		gins(ASUBL, &nod, n);
	}
	regfree(&nod);
}
