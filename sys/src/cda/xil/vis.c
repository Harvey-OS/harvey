#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "symbols.h"
#include "dat.h"

typedef struct Vis {
	int j;		/* which bit we're working on */
	int n;		/* number of visible edges */
	ulong mask;	/* set of primary inputs */
	int cost;	/* accumulated number of edges */
} Vis;

Vis
addvis(Vis a, Vis b)
{
	a.j = b.j;		/* return most recent counter value */
	a.n += b.n;		/* add edges */
	a.mask |= b.mask;	/* union primary inputs */
	a.cost += b.cost;	/* add module costs */
	return a;
}

Vis
nvis(Tree *t)
{
	Vis v;
	switch (t->op) {
	case C0:
	case C1:
		return (Vis) {0, 0, 0, 0};
	case ID:
		return (Vis) {0, 0, t->mask, 0};
	case and:
	case or:
	case xor:
		v = nvis(t->right);
		v = addvis(v, nvis(t->left));
		goto envis;
	case not:
	default:
		v = nvis(t->left);
envis:
		if (t->vis)
			v.n++;
		return v;
	}
}

Vis
walk(Tree *t, int j, ulong p)
{
	Vis v;
	switch (t->op) {
	case C0:
	case C1:
		return (Vis) {j, 0, 0, 0};
	case ID:
		return (Vis) {j, 0, t->mask, 0};
	case and:
	case or:
	case xor:
		if (aflag && !t->vis)
			return (Vis) {j, 0, t->mask, 0};
		v = walk(t->right, j, p);
		if (v.n + nbits(v.mask) > K)
			return v;
		v = addvis(v, walk(t->left, v.j, p));
		goto ewalk;
	case not:
	default:
		if (aflag && !t->vis)
			return (Vis) {j, 0, t->mask, 0};
		v = walk(t->left, j, p);
ewalk:
		if (v.n + nbits(v.mask) > K)
			return v;
		if ((p >> (v.j--)) & 1)
			return (Vis) {v.j, 1, 0, v.cost+1};
		else
			return v;
	}
}

int
wipe(Tree *t, int j, ulong p)
{
	switch (t->op) {
	case C0:
	case C1:
	case ID:
		return j;
	case and:
	case or:
	case xor:
		j = wipe(t->right, j, p);
	case not:
	default:
		j = wipe(t->left, j, p);
		if (!t->vis)
			return j;
		if (((p >> j) & 1) == 0)
			t->vis = 0;
		return j-1;
	}
}

void
markvis(Tree *t)
{
	Vis v,vmin;
	int m;
	ulong p, pmin, dp;
	v = nvis(t);
	m = v.n;
	vmin.cost = 1000;
	pmin = 0;
	if (m + nbits(v.mask) > K) {
		for (p = 0; p < (1<<m);) {
			v = walk(t, m-1, p);
			if (v.n + nbits(v.mask) > K) {
				dp = 1 << v.j;
				p += dp;
			}
			else {
				if (v.cost < vmin.cost) {
					vmin = v;
					pmin = p;
				}
				p++;
			}
		}
		wipe(t, m-1, pmin);
	}
	t->vis = 0;

}
