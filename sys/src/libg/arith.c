#include <u.h>
#include <libg.h>

/* These routines are NOT portable, but they are fast. */

Point
add(Point a, Point b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

Point
sub(Point a, Point b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

Rectangle
inset(Rectangle r, int n)
{
	r.min.x += n;
	r.min.y += n;
	r.max.x -= n;
	r.max.y -= n;
	return r;
}

Point
div(Point a, int b)
{
	a.x /= b;
	a.y /= b;
	return a;
}

Point
mul(Point a, int b)
{
	a.x *= b;
	a.y *= b;
	return a;
}

Rectangle
rsubp(Rectangle r, Point p)
{
	r.min.x -= p.x;
	r.min.y -= p.y;
	r.max.x -= p.x;
	r.max.y -= p.y;
	return r;
}

Rectangle
raddp(Rectangle r, Point p)
{
	r.min.x += p.x;
	r.min.y += p.y;
	r.max.x += p.x;
	r.max.y += p.y;
	return r;
}

Rectangle
rmul(Rectangle r, int a)
{
	if (a != 1) {
		r.min.x *= a;
		r.min.y *= a;
		r.max.x *= a;
		r.max.y *= a;
	}
	return r;
}

Rectangle
rdiv(Rectangle r, int a)
{
	if (a != 1) {
		r.min.x /= a;
		r.min.y /= a;
		r.max.x /= a;
		r.max.y /= a;
	}
	return r;
}

Rectangle
rshift(Rectangle r, int a)
{
	if (a > 0) {
		r.min.x <<= a;
		r.min.y <<= a;
		r.max.x <<= a;
		r.max.y <<= a;
	}
	else if (a < 0) {
		a = -a;
		r.min.x >>= a;
		r.min.y >>= a;
		r.max.x >>= a;
		r.max.y >>= a;
	}
	return r;
}

int
eqpt(Point p, Point q)
{
	return p.x==q.x && p.y==q.y;
}

int
eqrect(Rectangle r, Rectangle s)
{
	return r.min.x==s.min.x && r.max.x==s.max.x &&
	       r.min.y==s.min.y && r.max.y==s.max.y;
}

int
rectXrect(Rectangle r, Rectangle s)
{
	return r.min.x<s.max.x && s.min.x<r.max.x &&
	       r.min.y<s.max.y && s.min.y<r.max.y;
}

int
rectinrect(Rectangle r, Rectangle s)
{
	/* !ptinrect(r.min, s) in line for speed */
	if(!(r.min.x>=s.min.x && r.min.x<s.max.x &&
	    r.min.y>=s.min.y && r.min.y<s.max.y))
		return 0;
	return r.max.x<=s.max.x && r.max.y<=s.max.y;
}

int
ptinrect(Point p, Rectangle r)
{
	return p.x>=r.min.x && p.x<r.max.x &&
	       p.y>=r.min.y && p.y<r.max.y;
}

Rectangle
rcanon(Rectangle r)
{
	int t;
	if (r.max.x < r.min.x) {
		t = r.min.x;
		r.min.x = r.max.x;
		r.max.x = t;
	}
	if (r.max.y < r.min.y) {
		t = r.min.y;
		r.min.y = r.max.y;
		r.max.y = t;
	}
	return r;
}
