#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

/*
	break up orig into pieces not covered by cover
*/
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

int
fg_rectXrect(Rectangle cover, Rectangle orig, Rectangle *r)
{
	int a, b;
	Rectangle *or = r;

#define	R(A,B,C,D)	r->min.x=A,r->min.y=B,r->max.x=C,r->max.y=D,r++

	if((cover.min.x >= orig.max.x) || (cover.min.y >= orig.max.y)
	|| (cover.max.x <= orig.min.x) || (cover.max.y <= orig.min.y))
		return(-1);
	fg_rectclip(&cover, orig);
	a = orig.min.y;
	b = orig.max.y;
	if(cover.min.y != orig.min.y){
		a = cover.min.y;
		R(orig.min.x, orig.min.y, orig.max.x, cover.min.y);
	}
	if(cover.max.y != orig.max.y){
		b = cover.max.y;
		R(orig.min.x,cover.max.y, orig.max.x, orig.max.y);
	}
	if(cover.min.x != orig.min.x)
		R(orig.min.x, a, cover.min.x, b);
	if(cover.max.x != orig.max.x)
		R(cover.max.x, a, orig.max.x, b);
	return(r-or);
}

int
fg_rectclip(Rectangle *rp, Rectangle r)
{
	register i = 0;

	if(rp->min.x < r.min.x) rp->min.x = r.min.x, i++;
	if(rp->max.x > r.max.x) rp->max.x = r.max.x, i++;
	if(rp->min.y < r.min.y) rp->min.y = r.min.y, i++;
	if(rp->max.y > r.max.y) rp->max.y = r.max.y, i++;
	return(i);
}

Rectangle
fg_raddp(Rectangle r, Point p)
{
	r.min.x += p.x;
	r.min.y += p.y;
	r.max.x += p.x;
	r.max.y += p.y;
	return(r);
}

void
fg_rcanon(Rectangle *r)
{
	Rectangle rr;

	rr = *r;
	r->min.x = MIN(rr.min.x, rr.max.x);
	r->min.y = MIN(rr.min.y, rr.max.y);
	r->max.x = MAX(rr.min.x, rr.max.x);
	r->max.y = MAX(rr.min.y, rr.max.y);
}

void
fg_padd(Point *p, Point q)
{
	p->x += q.x;
	p->y += q.y;
}

void
fg_psub(Point *p, Point q)
{
	p->x -= q.x;
	p->y -= q.y;
}
int
fg_rXr(Rectangle r1, Rectangle r2)
{
	return((r1.min.x < r2.max.x) && (r1.min.y < r2.max.y)
	&& (r1.max.x > r2.min.x) && (r1.max.y > r2.min.y));
}

int
fg_rinr(Rectangle r1, Rectangle r2)
{
	return((r1.min.x >= r2.min.x) && (r1.min.y >= r2.min.y)
	&& (r1.max.x <= r2.max.x) && (r1.max.y <= r2.max.y));
}

int
fg_ptinrect(Point p, Rectangle r)
{
	return((p.x >= r.min.x) && (p.x < r.max.x) && (p.y >= r.min.y) && (p.y < r.max.y));
}
