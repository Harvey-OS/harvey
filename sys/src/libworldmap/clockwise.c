#include	<u.h>
#include	<libc.h>
#include	<thread.h>
#include	<draw.h>
#include	<keyboard.h>
#include	<mouse.h>
#include	<worldmap.h>
#include	"mapimpl.h"

double
determinant(Point p1, Point p2, Point p3) {
	double x1, y1, x2, y2, x3, y3;

	x1 = p1.x;	y1 = p1.y;
	x2 = p2.x;	y2 = p2.y;
	x3 = p3.x;	y3 = p3.y;

	return x2*y3 - x3*y2 - x1*y3 + x3*y1 + x1*y2 - x2*y1;
}

int
clockwise(Poly *pol, Rectangle *rr) {
	int i;
	int n, w, s, e;
	Rectangle r;

	if (pol->np < 3) {
		fprint(2, "clockwise: too few points: %d ", pol->np);
		for (i = 0; i < pol->np; i++)
			fprint(2, " (%d, %d)", pol->p[i].x, pol->p[i].y);
		fprint(2, "\n");
		return 1;
	}
	r = Rect(pol->p->x, pol->p->y, pol->p->x, pol->p->y);
	n = w = s = e = 0;
	for(i = 1; i < pol->np-1; i++) {
		Point pt = pol->p[i];

		if (pt.y > r.max.y) {
			n = i; r.max.y = pt.y;
		} else if (pt.y < r.min.y) {
			s = i; r.min.y = pt.y;
		}
		if (pt.x > r.max.x || (pt.x == r.max.x && i != s && i != n)) {
			e = i; r.max.x = pt.x;
		} else if (pt.x < r.min.x || (pt.x == r.min.x && i != s && i != n)) {
			w = i; r.min.x = pt.x;
		}
	}
	if (debug & 0x4)
		fprint(2, "%d points, n e s w %d %d %d %d\n", pol->np, n, e, s, w);
	if ((n == e && s == w) || (n == w && s == e)) {
		Point p1, p2, p3;

		p1 = pol->p[(n==0)?(pol->np-2):(n-1)];
		p2 = pol->p[n];
		p3 = pol->p[(n+1)%(pol->np-1)];

		if (debug & 0x4)
			fprint(2, "tiebreaker needed, %d points, n e s w %d %d %d %d, %P, %P, %P, det = %f\n",
				pol->np, n, e, s, w, p1, p2, p3, determinant(p1, p2, p3));
		return determinant(p1, p2, p3) < 0;
	}
	if ((w -= n) < 0) w += pol->np;
	if ((e -= n) < 0) e += pol->np;
	if ((s -= n) < 0) s += pol->np;
	n = 0;
	if (w == n) w += pol->np;
	if (debug & 0x4)
		fprint(2, "n e s w %d %d %d %d\n", n, e, s, w);
	if (rr) *rr = r;
	return n <= e && e <= s && s <= w;
}
