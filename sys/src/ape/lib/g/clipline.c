#include <stdlib.h>
#include <libg.h>

#define	XYswap(p)	t=(p)->x, (p)->x=(p)->y, (p)->y=t
#define	Swap(x, y)	t=x, x=y, y=t

static long
lfloor(long x, long y)	/* first integer <= x/y */
{
	if(y <= 0){
		if(y == 0)
			return x;
		y = -y;
		x = -x;
	}
	if(x < 0){	/* be careful; C div. is undefined */
		x = -x;
		x += y-1;
		return -(x/y);
	}
	return x/y;
}

static long
lceil(long x, long y)	/* first integer >= x/y */
{
	if(y <= 0){
		if(y == 0)
			return x;
		y = -y;
		x = -x;
	}
	if(x < 0){
		x = -x;
		return -(x/y);
	}
	x += y-1;
	return x/y;
}

int
_gminor(long x, Linedesc *l)
{
	long y;

	y = 2*(x-l->x0)*l->dminor + l->dmajor;
	y = lfloor(y, 2*l->dmajor) + l->y0;
	return l->slopeneg? -y : y;
}

int
_gmajor(long y, Linedesc *l)
{
	long x;

	x = 2*((l->slopeneg? -y : y)-l->y0)*l->dmajor - l->dminor;
	x = lceil(x, 2*l->dminor) + l->x0;
	if(l->dminor)
		while(_gminor(x-1, l) == y)
			x--;
	return x;
}

static void
gsetline(Point *pp0, Point *pp1, Linedesc *l)
{
	long dx, dy, t;
	int swapped;
	Point p0, p1;

	swapped = 0;
	p0 = *pp0;
	p1 = *pp1;
	l->xmajor = 1;
	l->slopeneg = 0;
	dx = p1.x - p0.x;
	dy = p1.y - p0.y;
	if(abs(dy) > abs(dx)){	/* Steep */
		l->xmajor = 0;
		XYswap(&p0);
		XYswap(&p1);
		Swap(dx, dy);
	}
	if(dx < 0){
		swapped++;
		Swap(p0.x, p1.x);
		Swap(p0.y, p1.y);
		dx = -dx;
		dy = -dy;
	}
	if(dy < 0){
		l->slopeneg = 1;
		dy = -dy;
		p0.y = -p0.y;
	}
	l->dminor = dy;
	l->dmajor = dx;
	l->x0 = p0.x;
	l->y0 = p0.y;
	p1.x = swapped? p0.x+1 : p1.x-1;
	p1.y = _gminor(p1.x, l);
	if(l->xmajor == 0){
		XYswap(&p0);
		XYswap(&p1);
	}
	if(pp0->x > pp1->x){
		*pp1 = *pp0;
		*pp0 = p1;
	}else
		*pp1 = p1;
}

/*
 * Modified clip-to-rectangle algorithm
 *	works in bitmaps with half-open lines
 *
 *	Newman & Sproull 124 (1st edition)
 */

static
code(Point *p, Rectangle *r)
{
	return( (p->x<r->min.x? 1 : p->x>=r->max.x? 2 : 0) |
		(p->y<r->min.y? 4 : p->y>=r->max.y? 8 : 0));
}

int
clipline(Rectangle r, Point *p0, Point *p1)
{
	Linedesc l;

	return _clipline(r, p0, p1, &l);
}

int
_clipline(Rectangle r, Point *p0, Point *p1, Linedesc *l)
{
	int c0, c1, n;
	long t, ret;
	Point temp;
	int swapped;

	if(p0->x==p1->x && p0->y==p1->y)
		return 0;
	gsetline(p0, p1, l);
	/* line is now closed */
	if(l->xmajor == 0){
		XYswap(p0);
		XYswap(p1);
		XYswap(&r.min);
		XYswap(&r.max);
	}
	c0 = code(p0, &r);
	c1 = code(p1, &r);
	ret = 1;
	swapped = 0;
	n = 0;
	while(c0 | c1){
		if(c0 & c1){	/* no point of line in r */
			ret = 0;
			goto Return;
		}
		if(++n > 10){	/* horrible points; overflow etc. etc. */
			ret = 0;
			goto Return;
		}
		if(c0 == 0){	/* swap points */
			temp = *p0;
			*p0 = *p1;
			*p1 = temp;
			Swap(c0, c1);
			swapped ^= 1;
		}
		if(c0 == 0)
			break;
		if(c0 & 1){		/* push towards left edge */
			p0->x = r.min.x;
			p0->y = _gminor(p0->x, l);
		}else if(c0 & 2){	/* push towards right edge */
			p0->x = r.max.x-1;
			p0->y = _gminor(p0->x, l);
		}else if(c0 & 4){	/* push towards top edge */
			p0->y = r.min.y;
			if(l->slopeneg)
				p0->x = _gmajor(p0->y-1, l)-1;
			else
				p0->x = _gmajor(p0->y, l);
		}else if(c0 & 8){	/* push towards bottom edge */
			p0->y = r.max.y-1;
			if(l->slopeneg)
				p0->x = _gmajor(p0->y, l);
			else
				p0->x = _gmajor(p0->y+1, l)-1;
		}
		c0 = code(p0, &r);
	}

    Return:
	if(l->xmajor == 0){
		XYswap(p0);
		XYswap(p1);
	}
	if(swapped){
		temp = *p0;
		*p0 = *p1;
		*p1 = temp;
	}
	return ret;
}
