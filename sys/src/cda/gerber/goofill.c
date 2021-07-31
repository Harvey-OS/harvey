#include <u.h>
#include <libc.h>
#include <libg.h>

typedef struct Segment	Segment;

struct Segment{
	Point	top;
	Point	bottom;
	Point	delta;
	long	crossp;
};

static void	fillseg(Bitmap*, Segment*, Segment*, Fcode);
static Segment*	makseg(Point, Point, Segment*);
static void	qsegsort(Segment*, Segment*);
static void	bubble(int*, int*);

void
polyfill(Bitmap *bp, Point *pp, int n, Fcode f)
{
	Segment *sstart, *sfin;

	if(n < 3)
		return;
	sstart = malloc(n*sizeof(Segment));
	if(sstart == 0)
		return;
	sfin = makseg(pp[n-1], pp[0], sstart);
	while(--n > 0){
		sfin = makseg(pp[0], pp[1], sfin);
		++pp;
	}
	if(sfin > sstart)
		fillseg(bp, sstart, sfin-1, f);
	free(sstart);
}

static void
fillseg(Bitmap *bp, Segment *sstart, Segment *sfin, Fcode f)
{
	Segment *st, *s, *su;
	int yscan;
	int *t;
	int ybottom, *xp;
	static int ncuts, *cuts, *cutf;

	if(ncuts == 0){
		ncuts = 16;
		if((cuts = malloc(sizeof(int)*ncuts)) == 0)
			return;
		cutf = cuts + ncuts;
	}
	qsegsort(sstart, sfin);

	ybottom = (s = sstart)->bottom.y;
	while(++s <= sfin)
		if ((yscan = s->bottom.y) > ybottom)
			ybottom = yscan;

	st = su = sstart;
	for(yscan = sstart->top.y; yscan < ybottom; yscan++){
		while(st < su && yscan >= st->bottom.y)
			st++;
		while(su <= sfin && yscan >= su->top.y)
			su++;
		if(st >= su)
			continue;
		xp = cuts;
		for(s = st; s < su; s++){
			if (yscan >= s->bottom.y)
				continue;
			if(xp >= cutf){
				cuts = realloc(cuts, sizeof(int)*(ncuts+4));
				if(cuts == 0)
					return;
				xp = cuts + ncuts;
				ncuts += 4;
				cutf = cuts + ncuts;
			}
			*xp++ = (yscan*s->delta.x + s->crossp)/s->delta.y;
		}
		bubble(cuts, --xp);
		for(t = cuts; t < xp; t += 2)
			segment(bp, Pt(t[0], yscan), Pt(t[1], yscan), ~0, f);
	}
}

#define SWAPX(p, q)	(z = (p).x, (p).x = (q).x, (q).x = z)
#define SWAPY(p, q)	(z = (p).y, (p).y = (q).y, (q).y = z)
#define SWAP(p, q)	(SWAPX(p, q), SWAPY(p, q))

static Segment *
makseg(Point p, Point q, Segment *sp)
{
	long z;

	if(p.y == q.y)
		return sp;
	if(p.y > q.y)
		SWAP(p, q);
	sp->top = p;
	sp->bottom = q;
	sp->delta = sub(q, p);
	sp->crossp = (long)q.y * p.x - (long)p.y * q.x;
	return sp+1;
}

static void
qsegsort(Segment *l, Segment *m)
{
	Segment *i, *j; Segment t;
	int y;

	while(l < m){
		i = l; j = m;
		y = i[(j-i)/2].top.y;
		while(i <= j){
			while(i->top.y < y)
				i++;
			while(y < j->top.y)
				j--;
			if(i <= j){
				t = *i;
				*i++ = *j;
				*j-- = t;
			}
		}
		if(j-l <= m-i){
			qsegsort(l, j);
			l = i;
		}else{
			qsegsort(i, m);
			m = j;
		}
	}
}

static void
bubble(int *p, int *q)
{
	int *i, t;

	do{
		t = 0;
		for(i = p; i < q; i++) 
			if (i[0] > i[1]){
				t = i[0];
				i[0] = i[1];
				i[1] = t;
				t = 1;
			}
	}while (t);
}
