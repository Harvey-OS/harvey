#include <alef.h>
/*
 * qsort -- simple quicksort
 */

aggr Sort
{
	int	(*cmp)(void*, void*);
	void	(*swap)(byte*, byte*, int);
	int	es;	
};

intern	void
swapb(byte *i, byte *j, int es)
{
	byte c;

	do {
		c = *i;
		*i++ = *j;
		*j++ = c;
		es--;
	} while(es != 0);

}

intern	void
swapi(byte *ii, byte *ij, int es)
{
	int *i, *j, c;

	i = (int*)ii;
	j = (int*)ij;
	do {
		c = *i;
		*i++ = *j;
		*j++ = c;
		es -= sizeof(int);
	} while(es != 0);
}

intern	void
qsorts(byte *a, int n, Sort *p)
{
	int j, es;
	byte *pi, *pj, *pn;

	es = p->es;
	while(n > 1) {
		pi = a + (n>>1) * es;
		(*p->swap)(a, pi, es);
		pi = a;
		pn = a + n*es;
		pj = pn;
		for(;;) {
			do
				pi += es;
			while(pi < pn && (*p->cmp)(pi, a) < 0);
			do
				pj -= es;
			while(pj > a && (*p->cmp)(pj, a) > 0);
			if(pj < pi)
				break;
			(*p->swap)(pi, pj, es);
		}
		(*p->swap)(a, pj, es);
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
qsort(void *va, int n, int es, int (*cmp)(void*, void*))
{
	Sort s;

	s = (Sort)(cmp, swapi, es);
	if(((int)va ^ es) % sizeof(int))
		s.swap = swapb;
	qsorts((byte*)va, n, &s);
}
