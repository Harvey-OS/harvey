#include <u.h>
#include <libc.h>
#include <bio.h>

#include "object.h"
#include "catset.h"

static int debug = 0;

int
catsetneeded(int v)
{
	return (v / 8) + 1;
}

static void
catsetprint(int f, Catset*cs)
{
	int i;
	fprint(2, "(%p %d:", cs->bitpiece, cs->nbitpiece);
	for (i = 0; i < cs->nbitpiece; i++)
		fprint(f, "[%d]=%x", i, cs->bitpiece[i]);
	fprint(2, ")");
}

void
catsetrealloc(Catset *cs, int sz)
{
	if (debug) fprint(2, "catsetrealloc %p %d (%p %d)", cs, sz, cs->bitpiece, cs->nbitpiece);
	if (sz > cs->nbitpiece) {
		cs->bitpiece = realloc(cs->bitpiece, sz*sizeof(uchar));
		memset(cs->bitpiece + cs->nbitpiece, 0, sz - cs->nbitpiece);
		cs->nbitpiece = sz;
	}
	if (debug) fprint(2, " -> %p %d\n", cs->bitpiece, cs->nbitpiece);
}

void
catsetfree(Catset *cs)
{
	free(cs->bitpiece);
	cs->bitpiece = 0;
	cs->nbitpiece = 0;
}

void
catsetinit(Catset*cs, int v)
{
	int n;

	n = catsetneeded(v);
	if (debug) fprint(2, "catsetinit %p %d -> ", cs, v);
	catsetrealloc(cs, n);
	catsetset(cs, v);
	if (debug) catsetprint(2, cs);
	if (debug) fprint(2, "\n");
}

void
catsetcopy(Catset*dst, Catset*src)
{
	if (debug) fprint(2, "catsetcopy %p %p ", dst, src);
	if (debug) catsetprint(2, dst);
	if (debug) fprint(2, " ");
	if (debug) catsetprint(2, src);
	if (dst->nbitpiece < src->nbitpiece)
		catsetrealloc(dst, src->nbitpiece);
	else
		memset(dst->bitpiece, 0, dst->nbitpiece);
	memcpy(dst->bitpiece, src->bitpiece, src->nbitpiece);
	dst->nbitpiece = src->nbitpiece;
	if (debug) fprint(2, "-> ");
	if (debug) catsetprint(2, dst);
	if (debug) fprint(2, "\n");
}

void
catsetset(Catset*cs, int v)
{
	int p = v / 8;
	int b = v % 8;
	if (debug) fprint(2, "catsetset %p %d ", cs, v);
	if (debug) catsetprint(2, cs);
	cs->bitpiece[p] = 1 << b;
	if (debug) fprint(2, "-> ");
	if (debug) catsetprint(2, cs);
	if (debug) fprint(2, "\n");
}

int
catsetisset(Catset*cs)
{
	int i;

	if (debug) fprint(2, "catsetisset %p ", cs);
	if (debug) catsetprint(2, cs);
	if (debug) fprint(2, "\n");
	for (i =0; i < cs->nbitpiece; i++) {
		if (cs->bitpiece[i])
			return 1;
	}
	return 0;
}

void
catsetorset(Catset*dst, Catset*src)
{
	int i;

	if (debug) fprint(2, "catsetorset %p %p ", dst, src);
	if (debug) catsetprint(2, dst);
	if (debug) fprint(2, " ");
	if (debug) catsetprint(2, src);
	if (src->nbitpiece > dst->nbitpiece)
		catsetrealloc(dst, src->nbitpiece);

	for (i =0; i < src->nbitpiece; i++) {
		dst->bitpiece[i] |= src->bitpiece[i];
	}
	if (debug) fprint(2, "-> ");
	if (debug) catsetprint(2, dst);
	if (debug) fprint(2, "\n");
}

int
catseteq(Catset*cs1, Catset*cs2)
{
	int i;
	Catset *css, * csl;

	if (debug) fprint(2, "catseteq %p %p ", cs1, cs2);
	if (debug) catsetprint(2, cs1);
	if (debug) fprint(2, " ");
	if (debug) catsetprint(2, cs2);
	if (debug) fprint(2, "\n");
	if (cs1->nbitpiece > cs2->nbitpiece) {
		csl = cs1;
		css = cs2;
	} else {
		csl = cs2;
		css = cs1;
	}
	for (i =0; i < css->nbitpiece; i++) {
		if (css->bitpiece[i] != csl->bitpiece[i])
			return 0;
	}
	for (i = css->nbitpiece; i < csl->nbitpiece; i++) {
		if (csl->bitpiece[i])
			return 0;
	}
	return 1;
}
