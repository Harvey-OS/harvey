#include <libg.h>

/*	Fill a disc of radius r centered at x1,y1
 *	The boundary is a sequence of vertically, horizontally,
 *	or diagonally adjacent points that minimize 
 *	abs(x^2+y^2-r^2).
 *
 *	The circle is guaranteed to be symmetric about
 *	the horizontal, vertical, and diagonal axes
 */

void
disc(Bitmap *b, Point p, int r, int v, Fcode f)
{
	int x1=p.x, y1=p.y;
	int eps = 0;		/* x^2 + y^2 - r^2 */
	int dxsq = 1;		/* (x+dx)^2-x^2*/
	int dysq = 1 - 2*r;
	int exy;
	register x0 = x1;
	register y0 = y1 - r;

	x1++;		/* to offset half-open lines*/
	y1 += r;
	while(y1 > y0) {
		exy = eps + dxsq + dysq;
		if(-exy <= eps+dxsq) {
			segment(b, Pt(x0, y0), Pt(x1, y0), v, f);
			segment(b, Pt(x0, y1), Pt(x1, y1), v, f);
			y1--;
			y0++;
			eps += dysq;
			dysq += 2;
		}
		if(exy <= -eps) {
			x1++;
			x0--;
			eps += dxsq;
			dxsq += 2;
		}
	}
	segment(b, Pt(x0, y0), Pt(x1, y0), v, f);
}
