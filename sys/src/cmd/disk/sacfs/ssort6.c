#include <u.h>
#include <libc.h>
#include "ssort.h"

#define pred(i, h) ((t=(i)-(h))<0?  t+n: t)
#define succ(i, h) ((t=(i)+(h))>=n? t-n: t)

enum
{
	BUCK = ~(~0u>>1),	/* high bit */
	MAXI = ~0u>>1,		/* biggest int */
};

typedef int	Elem;
static	void	qsort2(Elem*, Elem*, int n);
static	int	ssortit(int a[], int p[], int s[], int q[], int n, int h, int *pe, int nbuck);
static  void	lift(int si, int q[], int i);
int	sharedlen(int i, int j, int s[], int q[]);

int
ssort(int a[], int s[], int n)
{
	int i, l;
	int c, cc, ncc, lab, cum, nbuck;
	int k;
	int *p = 0;
	int result = 0;
	int *q = 0;
	int *al;
	int *pl;

#	define finish(r)  if(1){result=r; goto out;}else

	for(k=0,i=0; i<n; i++)	
		if(a[i] > k)
			k = a[i];	/* max element */
	k++;
	if(k>n)
		finish(2);

	nbuck = 0;
	p = malloc(n*sizeof(int));
	if(p == 0)
		finish(1);

	if(s) {					/* shared lengths */
		q = malloc(((n+1)>>1)*sizeof(int));
		if(q == 0)
			finish(1);
		for(i=0; i<n; i++)
			s[i] = q[i>>1] = MAXI;
		q[i>>1] = MAXI;
	}

	pl = p + n - k;
	al = a;
	memset(pl, -1, k*sizeof(int));

	for(i=0; i<n; i++) {		/* (1) link */
		l = a[i];
		al[i] = pl[l];
		pl[l] = i;
	}

	for(i=0; i<k; i++)		/* check input - no holes */
		if(pl[i]<0)
			finish(2);

	
	lab = 0;			/* (2) create p and label a */
	cum = 0;
	i = 0;
	for(c = 0; c < k; c++){	
		for(cc = pl[c]; cc != -1; cc = ncc){
			ncc = al[cc];
			al[cc] = lab;
			cum++;
			p[i++] = cc;
		}
		if(lab + 1 == cum) {
			i--;
		} else {
			p[i-1] |= BUCK;
			nbuck++;
		}
		if(s) {
			s[lab] = 0;
			lift(0, q, lab);
		}
		lab = cum;
	}

	ssortit(a, p, s, q, n, 1, p+i, nbuck);
	memcpy(a, p, n*sizeof(int));
	
out:
	free(p);
	free(q);
	return result;
}

/*
 * calculate the suffix array for the bytes in buf,
 * terminated by a unique end marker less than any character in buf
 * returns the index of the identity permutation,
 * and -1 if there was an error.
 */
int
ssortbyte(uchar buf[], int p[], int s[], int n)
{
	int *a, *q, buckets[256*256];
	int i, last, lastc, cum, c, cc, ncc, lab, id, nbuck;

	a = malloc((n+1)*sizeof(int));
	if(a == nil)
		return -1;

	q = nil;
	if(s) {					/* shared lengths */
		q = malloc(((n+2)>>1)*sizeof(int));
		if(q == nil){
			free(a);
			return -1;
		}
		for(i=0; i<n+1; i++)
			s[i] = q[i>>1] = MAXI;
		q[i>>1] = MAXI;
	}

	memset(buckets, -1, sizeof(buckets));
	c = buf[n-1] << 8;
	last = c;
	for(i = n - 2; i >= 0; i--){
		c = (buf[i] << 8) | (c >> 8);
		a[i] = buckets[c];
		buckets[c] = i;
	}

	/*
	 * end of string comes before anything else
	 */
	a[n] = 0;
	if(s) {
		s[0] = 0;
		lift(0, q, 0);
	}

	lab = 1;
	cum = 1;
	i = 0;
	lastc = 1;		/* won't match c & 0xff00 for any c */
	nbuck = 0;
	for(c = 0; c < 256*256; c++) {
		/*
		 * last character is followed by unique end of string
		 */
		if(c == last) {
			a[n-1] = lab;
			if(s) {
				s[lab] = 0;
				lift(0, q, lab);
			}
			cum++;
			lab++;
			lastc = c & 0xff00;
		}

		for(cc = buckets[c]; cc != -1; cc = ncc) {
			ncc = a[cc];
			a[cc] = lab;
			cum++;
			p[i++] = cc;
		}
		if(lab == cum)
			continue;
		if(lab + 1 == cum)
			i--;
		else {
			p[i - 1] |= BUCK;
			nbuck++;
		}
		if(s) {
			cc = (c & 0xff00) == lastc;
			s[lab] = cc;
			lift(cc, q, lab);
		}
		lastc = c & 0xff00;
		lab = cum;
	}

	id = ssortit(a, p, s, q, n+1, 2, p+i, nbuck);
	free(a);
	free(q);
	return id;
}

/*
 * can get an interval for the shared lengths from [h,3h) by recording h
 * rather than h + sharedlen(..) when relabelling.  if so, no calls to lift are needed.
 */
static int
ssortit(int a[], int p[], int shared[], int q[], int n, int h, int *pe, int nbuck)
{
	int *s, *ss, *packing, *sorting;
	int v, sv, vv, packed, lab, t, i;

	for(; h < n && p < pe; h=2*h) {
		packing = p;
		nbuck = 0;

		for(sorting = p; sorting < pe; sorting = s){
			/*
			 * find length of stuff to sort
			 */
			lab = a[*sorting];
			for(s = sorting; ; s++) {
				sv = *s;
				v = a[succ(sv & ~BUCK, h)];
				if(v & BUCK)
					v = lab;
				a[sv & ~BUCK] = v | BUCK;
				if(sv & BUCK)
					break;
			}
			*s++ &= ~BUCK;
			nbuck++;

			qsort2(sorting, a, s - sorting);

			v = a[*sorting];
			a[*sorting] = lab;
			packed = 0;
			for(ss = sorting + 1; ss < s; ss++) {
				sv = *ss;
				vv = a[sv];
				if(vv == v) {
					*packing++ = ss[-1];
					packed++;
				} else {
					if(packed) {
						*packing++ = ss[-1] | BUCK;
					}
					lab += packed + 1;
					if(shared) {
						v = h + sharedlen(v&~BUCK, vv&~BUCK, shared, q);
						shared[lab] = v;
						lift(v, q, lab);
					}
					packed = 0;
					v = vv;
				}
				a[sv] = lab;
			}
			if(packed) {
				*packing++ = ss[-1] | BUCK;
			}
		}
		pe = packing;
	}

	/*
	 * reconstuct the permutation matrix
	 * return index of the entire string
	 */
	v = a[0];
	for(i = 0; i < n; i++)
		p[a[i]] = i;

	return v;
}

/* Propagate a new entry s[i], with value si, into q[]. */
static void
lift(int si, int q[], int i)
{
	for(i >>= 1; q[i] > si; i &= ~-i)		/* squash the low 1-bit */
		q[i] = si;
}

/*
 * Find in s[] the minimum value over the interval i<=k<=j, using
 * tree q[] to do logarithmic, rather than linear search
 */
int
sharedlen(int i, int j, int s[], int q[])
{
	int k, v;
	int min = MAXI;
	int max = 0;

	if(i > j) {		/* swap i & j */
		i ^= j;
		j ^= i;
		i ^= j;
	}
	i++;
	while(i <= j && min > max) {
		k = i & -i;
		if(i & 1)
			v = s[i];
		else
			v = q[i>>1];
		if(i+k > j+1) {
			if(v > max)
				max = v;
			if(s[i] < min)
				min = s[i];
			i++;
		} else {
			if(v < min)
				min = v;
			i += k;
		}
	}
	return min;
}

/*
 * qsort specialized for sorting permutations based on successors
 */
static void
vecswap2(Elem *a, Elem *b, int n)
{
	while (n-- > 0) {
        	Elem t = *a;
		*a++ = *b;
		*b++ = t;
	}
}

#define swap2(a, b) { t = *(a); *(a) = *(b); *(b) = t; }
#define ptr2char(i, asucc) (asucc[*(i)])

static Elem*
med3(Elem *a, Elem *b, Elem *c, Elem *asucc)
{
	Elem va, vb, vc;

	if ((va=ptr2char(a, asucc)) == (vb=ptr2char(b, asucc)))
		return a;
	if ((vc=ptr2char(c, asucc)) == va || vc == vb)
		return c;	   
	return va < vb ?
		  (vb < vc ? b : (va < vc ? c : a))
		: (vb > vc ? b : (va < vc ? a : c));
}

static void
inssort(Elem *a, Elem *asucc, int n)
{
	Elem *pi, *pj, t;

	for (pi = a + 1; --n > 0; pi++)
		for (pj = pi; pj > a; pj--) {
			if(ptr2char(pj-1, asucc) <= ptr2char(pj, asucc))
				break;
			swap2(pj, pj-1);
		}
}

static void
qsort2(Elem *a, Elem *asucc, int n)
{
	Elem d, r, partval;
	Elem *pa, *pb, *pc, *pd, *pl, *pm, *pn, t;

	if (n < 15) {
		inssort(a, asucc, n);
		return;
	}
	pl = a;
	pm = a + (n >> 1);
	pn = a + (n-1);
	if (n > 30) { /* On big arrays, pseudomedian of 9 */
		d = (n >> 3);
		pl = med3(pl, pl+d, pl+2*d, asucc);
		pm = med3(pm-d, pm, pm+d, asucc);
		pn = med3(pn-2*d, pn-d, pn, asucc);
	}
	pm = med3(pl, pm, pn, asucc);
	swap2(a, pm);
	partval = ptr2char(a, asucc);
	pa = pb = a + 1;
	pc = pd = a + n-1;
	for (;;) {
		while (pb <= pc && (r = ptr2char(pb, asucc)-partval) <= 0) {
			if (r == 0) {
				swap2(pa, pb);
				pa++;
			}
			pb++;
		}
		while (pb <= pc && (r = ptr2char(pc, asucc)-partval) >= 0) {
			if (r == 0) {
				swap2(pc, pd);
				pd--;
			}
			pc--;
		}
		if (pb > pc)
			break;
		swap2(pb, pc);
		pb++;
		pc--;
	}
	pn = a + n;
	r = pa-a;
	if(pb-pa < r)
		r = pb-pa;
	vecswap2(a, pb-r, r);
	r = pn-pd-1;
	if(pd-pc < r)
		r = pd-pc;
	vecswap2(pb, pn-r, r);
	if ((r = pb-pa) > 1)
		qsort2(a, asucc, r);
	if ((r = pd-pc) > 1)
		qsort2(a + n-r, asucc, r);
}
