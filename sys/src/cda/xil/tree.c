#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "symbols.h"

#define ONES	0xffffffff
#define AFANIN	2
#define OFANIN	2

Tree *ortree(Term *, int);
Tree *andtree(Term *, int);
Tree *termid(ulong, int);
Tree *mux4hack(Term *, int, ulong);
int nbits(ulong);
int eqcomp(Term *, int, Term *, int);
int eval(Term *, int, ulong);

Tree *
tree(Term *exp, int n)
{
	ulong mask;
	ulong pos=0,neg=0;
	ulong m,v;
	int i,mid;
	Term temp;
	if (n == 0)
		return ZERO;
	if (n == 1)
		return andtree(exp, nbits(exp->mask));
	if (!mflag)
		return ortree(exp, n);
	/* check for easy factors first */
	for (i = 0, mask = ONES; i < n; i++) {
		if ((m = exp[i].mask) == 0)
			return ONE;
		mask &= m;
		v = exp[i].value;
		pos |= v & m;
		neg |= ~v & m;
	}
	if (mask) {
		/* heuristic: factor ands before muxes */
		if (m = mask & ~(mask&pos&neg))
			mask = m;
		mask &= -mask;
		mid = n;
		for (i = 0; i < mid; i++) {
			exp[i].mask &= ~mask;
			if (exp[i].value & mask) {
				mid--;
				temp = exp[mid];
				exp[mid] = exp[i];
				exp[i] = temp;
				i--;
			}
		}
		if (mid == 0)
			return andnode(termid(mask,0), tree(exp,n));
		else if (mid == n)
			return andnode(termid(mask,1), tree(exp,n));
		else if (eqcomp(exp,mid,exp+mid,n-mid))
			return xornode(termid(mask,0), tree(exp,mid));	/* xor */
		else
			return muxnode(termid(mask,0), tree(exp,mid),
				tree(&exp[mid],n-mid));
	}
	/* hack to factor out the almost-muxes, very special case */
	else if (!xflag && mflag && n == 3 && (mask = pos&neg) && nbits(mask) == 2) 	{
		int q=0;
		v = -1;
		for (i = m = 0; i < n; i++)
			if ((exp[i].mask & mask) != mask) {
				q++;				/* count odd terms */
				m |= exp[i].mask & ~mask;	/* have to share values */
				v &= exp[i].mask & ~mask;
			}
/*		if (q == 2 && m == v)
			return mux4hack(exp, n, mask);*/
	}
	return ortree(exp, n);
}

ulong
termmask(Term *exp, int n)
{
	int i;
	ulong m;
	for (i = 0, m = 0; i < n; i++)
		m |= exp[i].mask;
	return m;
}

int
eval(Term *exp, int n, ulong v)
{
	int i;
	ulong m;
	for (i = 0; i < n; i++) {
		m = exp[i].mask;
		if ((v & m) == (exp[i].value & m))
			return 1;
	}
	return 0;
}

int
eqcomp(Term *a, int na, Term *b, int nb)
{
	ulong v,m,ov=-1;
	if ((m = termmask(a, na)) != termmask(b, nb))
		return 0;
	for (v = 0; v <= m; v++) {
		if ((v&m) == ov)
			continue;
		if (eval(a, na, v) == eval(b, nb, v))
			return 0;
		ov = v&m;
	}
	return 1;
}

Tree *
ortree(Term *exp, int n)
{
	int d,m,i,j,mid,count[32];
	ulong mask,bit;
	Tree *t;
	Term temp;
	if (n == 1)
		return andtree(exp, nbits(exp->mask));
/*	if (mflag && n == 2)
		return muxnode(andtree(exp, nbits(exp->mask)),
			andtree(exp+1, nbits(exp[1].mask)), ONE);*/
	if (!kflag)
		goto skippit;
	/*
	 * karplus hack
	 * try to turn it into a + b*c */
	for (i = 0, mask = 0; i < n; i++)
		mask |= exp[i].mask;
	for (i = 0; i < 32; i++) {
		count[i] = 0;
		bit = 1 << i;
		if ((bit & mask) == 0)
			continue;
		for (j = 0; j < n; j++)
			if (bit & exp[j].mask)
				count[i]++;
	}
	j = 0;
	for (i = 1; i < 32; i++)
		if (count[i] > count[j])
			j = i;
	if (count[j] < 2)
		goto skippit;
	bit = 1 << j;
	mid = n;
	for (i = 0; i < mid; i++)
		if (bit & exp[i].mask) {
			mid--;
			temp = exp[mid];
			exp[mid] = exp[i];
			exp[i] = temp;
			i--;
		}
	return ornode(tree(exp, mid), tree(exp+mid, n-mid));
skippit:
	for (d = 1; mflag && d*OFANIN < n; d *= OFANIN)
		;
	m = ((n-1)/d)*d;
	t = ortree(exp+m, n-m);
	for (m -= d; m >= 0; m -= d)
		t = ornode(tree(exp+m, d), t);	/* tree -> aggressive */
	return t;
}

log2(ulong u)
{
	int i;
	for (i = -1; u; i++, u >>= 1)
		;
	return i;
}

Tree *
termid(ulong m, int i)
{
	Tree *t;
	t = idnode(dep[log2(m)], i);
	if (i&1)
		t->left->mask = m;
	t->mask = m;
	return t;
}

Tree *
andtree(Term *exp, int n)
{
	int m,d;
	ulong u;
	Tree *t;
	if (n == 1) {
		/* do bubbles first */
		if ((u = exp->mask & exp->value) == 0)
			u = exp->mask;
		u = u & -u;
		exp->mask &= ~u;
		return termid(u, (exp->value&u) ? 0 : 1);
	}
	/* height balance if mflag */
	for (d = 1; mflag && d*AFANIN < n; d *= AFANIN)
		;
	m = ((n-1)/d)*d;
	t = andtree(exp, n-m);
	for (m -= d; m >= 0; m -= d)
		t = andnode(andtree(exp, d), t);
	return t;
}

#define NB	12
#define BMASK	0xfff
int nbit[1<<NB];

int
nbits(ulong m)
{
	int n;
	for (n = 0; m; m >>= NB)
		n += nbit[m&BMASK];
	return n;
}

void
terminit(void)
{
	int i,m;
	nbit[0] = 0;
	for (m = 1, i = 1; i < (1<<NB); i++)
		if (i == m) {
			nbit[i] = 1;
			m <<= 1;
		}
		else
			nbit[i] = 1 + nbit[i & ~(m>>1)];
}

ulong
revbit(ulong u, int n)
{
	ulong m;
	int i;
	for (i = 0, m = 0; i < n; i++, u >>= 1)
		m = (m<<1) | (u & 1);
	return m;
}

void
bitreverse(Term *ex, int ne, Sym *s[], int ns)
{
	int i;
	Sym *ts;
	for (i = 0; i < ne; i++) {
		ex[i].value = revbit(ex[i].value,ns);
		ex[i].mask = revbit(ex[i].mask,ns);
	}
	for (i = 0, ns--; i < ns; i++, ns--) {
		ts = s[i];
		s[i] = s[ns];
		s[ns] = ts;
	}
}

void
setbits(ulong *u, ulong m, ulong v)
{
	*u = ~m & *u | m & v;
}

Tree *
xortree(Sym *s[], int i, int n)
{
	Tree *t;
	int n2 = n>>1;
	if (n == 1) {
		t = idnode(s[0], 0);
		t->mask = 1 << i;
	}
	else {
		t = xornode(xortree(s, i, n2), xortree(s+n2, i+n2, n-n2));
		if (t->left->vis && t->right->vis)	/* sleaze */
			t->vis = 0;
	}
	return t;
}
