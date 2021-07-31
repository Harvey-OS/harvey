#include <u.h>
#include <libc.h>

/* qsort -- qsort interface implemented by faster quicksort */

#define SWAPINIT(a, es) swaptype =                            \
    (a - (char*) 0) % sizeof(long) || es % sizeof(long) ? 2 : \
    es == sizeof(long) ? 0 : 1;
#define swapcode(TYPE, parmi, parmj, n) {         \
    long i = (n) / sizeof(TYPE);                  \
    register TYPE *pi = (TYPE *) (parmi);         \
    register TYPE *pj = (TYPE *) (parmj);         \
    do {                                          \
        register TYPE t = *pi;                    \
        *pi++ = *pj;                              \
        *pj++ = t;                                \
    } while (--i);                                \
}
#define swap(a, b) {                              \
    if (swaptype == 0) {                          \
        long t = * (long *) (a);                  \
        * (long *) (a) = *(long*) (b);            \
        * (long *) (b) = t;                       \
    } else if (swaptype == 1)                     \
        swapcode(long, a, b, es)                  \
    else                                          \
        swapcode(char, a, b, es)                  \
}
#define vecswap(a, b, n) { if (n > 0) {           \
    if (swaptype == 2) swapcode(char, a, b, n*es) \
    else swapcode(long, a, b, n*es)               \
} }

static char *med3func(void *a, void *b, void *c, int (*cmp)(void*, void*))
{	return cmp(a, b) < 0 ?
		  (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ) )
		: (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ) );
}
#define med3(a, b, c) med3func(a, b, c, cmp)

void qsort(char *a, long n, long es, int (*cmp)(void*, void*))
{
	int  na, nb, nc, nd, d, r, swaptype;
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;

	SWAPINIT(a, es);
	if (n < 7) { /* Insertion sort on small arrays */
		for (pm = a + es; pm < a + n*es; pm += es)
			for (pl = pm; pl > a && cmp(pl-es, pl) > 0; pl -= es)
				swap(pl, pl-es);
		return;
	}
	pm = a + (n/2) * es;
	if (n > 7) {
		pl = a;
		pn = a + (n-1) * es;
		if (n > 40) { /* On big arrays, pseudomedian of 9 */
			d = (n/8) * es;
			pl = med3(pl, pl+d, pl+2*d);
			pm = med3(pm-d, pm, pm+d);
			pn = med3(pn-2*d, pn-d, pn);
		}
		pm = med3(pl, pm, pn); /* On medium arrays, median of 3 */
	}
	swap(a, pm); /* On tiny arrays, partition around middle */
	pa = pb = a + es;
	pc = pd = pn = a + (n-1)*es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) { swap(pa, pb); pa += es; }
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) { swap(pc, pd); pd -= es; }
			pc -= es;
		}
		if (pb > pc) break;
		swap(pb, pc);
		pb += es;
		pc -= es;
	}
	na = (pa - a)  / es;
	nb = (pb - pa) / es;
	nc = (pd - pc) / es;
	nd = (pn - pd) / es;
	if (na < nb) { vecswap(a,  a + nb*es,  na); }
	else	     { vecswap(a,  a + na*es,  nb); }
	if (nc < nd) { vecswap(pb, pb + nd*es, nc); }
	else	     { vecswap(pb, pb + nc*es, nd); }
	if (nb > 1) qsort(a, nb, es, cmp);
	if (nc > 1) qsort(a + (n-nc) * es, nc, es, cmp);
}
