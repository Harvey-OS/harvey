/*
 * qsort --
 *	qsort interface implemented by faster quicksort
 */
#include	<u.h>
#include	<libc.h>

#define	swapcode(TYPE, parmi, parmj, n) {\
	long i = (n) / (int) sizeof(TYPE);\
	TYPE *pi = (TYPE *) (parmi);\
	TYPE *pj = (TYPE *) (parmj);\
	do {\
		TYPE t = *pi;\
		*pi++ = *pj;\
		*pj++ = t;\
	} while(--i > 0);\
}

#define swap(a, b)\
{\
	if(swaptype == 0) {\
		long t = *(long*)(a);\
		*(long*)(a) = *(long*)(b);\
		*(long*)(b) = t;\
	} else {\
		if(swaptype <= 1)\
			swapcode(long, a, b, es)\
		else\
			swapcode(char, a, b, es)\
	}\
}

#define	vecswap(a, b, n)\
{\
	if(n > 0)\
		if(swaptype <= 1)\
			swapcode(long, a, b, n*es)\
		else\
			swapcode(char, a, b, n*es)\
}

static
char*
med3func(char *a, char *b, char *c, int (*cmp)(void*, void*))
{
	return cmp(a, b) < 0?
		(cmp(b, c) < 0?
			b: (cmp(a, c) < 0? c: a)):
		(cmp(b, c) > 0?
			b: (cmp(a, c) < 0? a: c));
}

static
void
qsort1(char *a, ulong n, ulong es, int swaptype, int (*cmp)(void*, void*))
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int r, na, nb, nc, nd, d;

loop:
	if(n < 7) {	/* Insertion sort on small arrays */
		for(pm = a + es; pm < a + n*es; pm += es)
			for(pl = pm; pl > a && cmp(pl-es, pl) > 0; pl -= es)
				swap(pl, pl-es);
		return;
	}
	pm = a + (n/2) * es;
	if(n > 7) {
		pl = a;
		pn = a + (n-1) * es;
		if(n > 40) {	/* On big arrays, pseudomedian of 9 */
			d = (n/8) * es;
			pl = med3func(pl, pl+d, pl+2*d, cmp);
			pm = med3func(pm-d, pm, pm+d, cmp);
			pn = med3func(pn-2*d, pn-d, pn, cmp);
		}
		pm = med3func(pl, pm, pn, cmp); /* On medium arrays, median of 3 */
	}
	swap(a, pm);	/* On tiny arrays, partition around middle */
	pa = pb = a + es;
	pc = pd = pn = a + (n-1)*es;
	for(;;) {
		while(pb <= pc && (r = cmp(pb, a)) <= 0) {
			if(r == 0) {
				swap(pa, pb); pa += es;
			}
			pb += es;
		}
		while(pb <= pc && (r = cmp(pc, a)) >= 0) {
			if(r == 0) {
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if(pb > pc)
			break;
		swap(pb, pc);
		pb += es;
		pc -= es;
	}
	na = (pa - a) / es;
	nb = (pb - pa) / es;
	nc = (pd - pc) / es;
	nd = (pn - pd) / es;
	if(na < nb) {
		vecswap(a, a + nb*es, na);
	} else {
		vecswap(a, a + na*es, nb);
	}
	if(nc < nd) {
		vecswap(pb, pb + nd*es, nc);
	} else {
		vecswap(pb, pb + nc*es, nd);
	}
	if(nb > nc) {
		qsort1(a, nb, es, swaptype, cmp);
		a += (n-nc)*es;
		n = nc;
		goto loop;
	}
	qsort1(a + (n-nc)*es, nc, es, swaptype, cmp);
	n = nb;
	goto loop;
}

void
qsort(void *a, long n, long es, int (*cmp)(void*, void*))
{
	int swaptype;

	swaptype = 1;
	if((ulong)a % sizeof(long) || (ulong)es % sizeof(long))
		swaptype = 2;
	else
	if(es == sizeof(long))
		swaptype = 0;
	qsort1(a, n, es, swaptype, cmp);
}
