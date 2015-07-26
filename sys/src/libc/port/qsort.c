/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * qsort -- simple quicksort
 */

#include <u.h>

typedef
struct
{
	int	(*cmp)(const void*, const void*);
	void	(*swap)(char*, char*, int32_t);
	int32_t	es;
} Sort;

static	void
swapb(char *i, char *j, int32_t es)
{
	char c;

	do {
		c = *i;
		*i++ = *j;
		*j++ = c;
		es--;
	} while(es != 0);

}

static	void
swapi(char *ii, char *ij, int32_t es)
{
	int32_t *i, *j, c;

	i = (int32_t*)ii;
	j = (int32_t*)ij;
	do {
		c = *i;
		*i++ = *j;
		*j++ = c;
		es -= sizeof(int32_t);
	} while(es != 0);
}

static	char*
pivot(char *a, int32_t n, Sort *p)
{
	int32_t j;
	char *pi, *pj, *pk;

	j = n/6 * p->es;
	pi = a + j;	/* 1/6 */
	j += j;
	pj = pi + j;	/* 1/2 */
	pk = pj + j;	/* 5/6 */
	if(p->cmp(pi, pj) < 0) {
		if(p->cmp(pi, pk) < 0) {
			if(p->cmp(pj, pk) < 0)
				return pj;
			return pk;
		}
		return pi;
	}
	if(p->cmp(pj, pk) < 0) {
		if(p->cmp(pi, pk) < 0)
			return pi;
		return pk;
	}
	return pj;
}

static	void
qsorts(char *a, int32_t n, Sort *p)
{
	int32_t j, es;
	char *pi, *pj, *pn;

	es = p->es;
	while(n > 1) {
		if(n > 10) {
			pi = pivot(a, n, p);
		} else
			pi = a + (n>>1)*es;

		p->swap(a, pi, es);
		pi = a;
		pn = a + n*es;
		pj = pn;
		for(;;) {
			do
				pi += es;
			while(pi < pn && p->cmp(pi, a) < 0);
			do
				pj -= es;
			while(pj > a && p->cmp(pj, a) > 0);
			if(pj < pi)
				break;
			p->swap(pi, pj, es);
		}
		p->swap(a, pj, es);
		j = (pj - a) / es;

		n = n-j-1;
		if(j >= n) {
			qsorts(a, j, p);
			a += (j+1)*es;
		} else {
			qsorts(a + (j+1)*es, n, p);
			n = j;
		}
	}
}

void
qsort(void *va, int32_t n, int32_t es, int (*cmp)(const void*, const void*))
{
	Sort s;

	s.cmp = cmp;
	s.es = es;
	s.swap = swapi;
	if(((uintptr)va | es) % sizeof(int32_t))
		s.swap = swapb;
	qsorts((char*)va, n, &s);
}
