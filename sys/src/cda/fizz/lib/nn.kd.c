#include	<cda/fizz.h>
/*
 Point set
	k = 2 # dimension
	n = num of pts
	x[ptnum "," dim] [1..n,1..k]
	bounding box: bbl[1..k], bbu[1..k]
 k-d tree
	tp[1..n] = permutation vector
	troot = root
	tnext = next available node number
	tcutoff = 2 # Max bucket size
	For each node p: tbucket[p]==1 if a bucket, else 0
	For internal nodes: tloson[], thison[], tdim[], tval[]
	For buckets: tloson[], thison[]
*/

#define		HUGE		999999L
#define		TCUTOFF		2

#define	DIST(a,b)	(abs(a.x-b.x)+abs(a.y-b.y))
static Pin *pb;
static np;
static long nalloc;
static bbl[2], bbu[2];
static short *tp;
static troot, tnext;
static char *tbucket;
static short *tloson, *thison, *tdim, *tval;
static short nnbest;
static long nnrad;

#define	DIM(d, i)	(d? pb[i].p.y : pb[i].p.x)

nnprep(pins, npins)
	Pin *pins;
{
	int i;

	if(npins > nalloc){
		nalloc = npins;
		tp = (short *)lmalloc(nalloc*sizeof(short));
		tloson = (short *)lmalloc(nalloc*sizeof(short));
		thison = (short *)lmalloc(nalloc*sizeof(short));
		tdim = (short *)lmalloc(nalloc*sizeof(short));
		tval = (short *)lmalloc(nalloc*sizeof(short));
		tbucket = lmalloc(nalloc);
	}
	pb = pins;
	np = npins;
	for(i = 0; i < np; i++)
		tp[i] = i;
	tnext = 0;
	troot = rbuild(0, np-1);
}

rbuild(l, u)
{
	short p, m, maxdim, savev, d, v;

	p = tnext++;
	if((u-l) <= TCUTOFF) { /* Bucket */
		tbucket[p] = 1;
		tloson[p] = l; thison[p] = u;
	} else { /* Internal node */
		tbucket[p] = 0;
		maxdim = spread(l, u);
		m = (l+u)/2;
		partition(l, u, m, maxdim);
		d = tdim[p] = maxdim;
		v = tval[p] = DIM(maxdim, tp[m]);
		savev = bbu[d]; bbu[d] = v;
		tloson[p] = rbuild(l, m);
		bbu[d] = savev;
		savev = bbl[d]; bbl[d] = v;
		thison[p] = rbuild(m+1, u);
		bbl[d] = savev;
	}
	return(p);
}

spread(l, u)
{
	short i;
	register Point *p;
	Point min, max;

	min.x = min.y = HUGE;
	max.x = max.y = -HUGE;
	for(i = l; i <= u; i++){
		p = &pb[tp[i]].p;
		if(p->x < min.x) min.x = p->x;
		if(p->x > max.x) max.x = p->x;
		if(p->y < min.y) min.y = p->y;
		if(p->y > max.y) max.y = p->y;
	}
	max.x -= min.x;
	max.y -= min.y;
	return(max.y > max.x);
}

partition(l, u, k, d)
{
	short t, m, i;

	if(l < u){
		swap(l, l+nrand(u+1-l));
		t = DIM(d, tp[l]);
		m = l;
		for(i = l+1; i <= u; i++)
			if(DIM(d, tp[i]) < t) swap(++m, i);
		swap(l, m);
		if      (m < k)	partition(m+1, u, k, d);
		else if (m > k)	partition(l, m-1, k, d);
	}
}

swap(i, j)
{
	register t=tp[i];
	tp[i]=tp[j]; tp[j]=t;
}

nn(p, bdist, bpt)
	Point p;
	long *bdist;
	short *bpt;
{
	nnrad = HUGE;
	rnn(troot, p);
	*bpt = nnbest;
	*bdist = nnrad;
}

rnn(p, pt)
	Point pt;
{
	short i, d, q, v, pval;
	register k, dist;

	if(tbucket[p]){	/* Bucket */
		for(i = tloson[p]; i <= thison[p]; i++){
			q = tp[i];
			dist = pt.x-pb[q].p.x;
			if(dist < 0) dist = -dist;
			k = pt.x-pb[q].p.x;
			dist += (k<0)? -k:k;
			if(dist < nnrad) { nnrad = dist; nnbest = q; }
		}
	} else { /* Internal node */
		d = tdim[p]; v = tval[p]; pval = d? pt.y:pt.x;
		if (pval < v) {
			rnn(tloson[p], pt); if(pval+nnrad > v) rnn(thison[p], pt);
		} else {
			rnn(thison[p], pt); if(pval-nnrad < v) rnn(tloson[p], pt);
		}
	}
}
