#include <alef.h>
#include <libg.h>

/*	Fill a disc of radius r centered at x1,y1
 *	The boundary is a sequence of vertically, horizontally,
 *	or diagonally adjacent points that minimize 
 *	abs(x^2+y^2-r^2).
 *
 *	The circle is guaranteed to be symmetric about
 *	the horizontal, vertical, and diagonal axes
 *
 *	If the disc is large, use bitblt to draw the lines, as it's faster.
 */

void
disc(Bitmap *b, Point p, int r, int v, Fcode f)
{
	int x1, y1;
	int x0, y0;
	Bitmap *bp;
	int eps, exy;	
	int dxsq, dysq;

	x1 = p.x;
	y1 = p.y;
	eps = 0;
	dxsq = 1;
	dysq = 1 - 2*r;
	x0 = x1;
	y0 = y1 - r;

	x1++;		/* to offset half-open lines*/
	y1 += r;
	if(r >= 16){
		bp = balloc(Rect(p.x-r, 0, p.x+r+1, 1), b->ldepth);
		if(bp == nil)
			berror("can't allocate bitmap for disc");
		segment(bp, bp->r.min, Pt(p.x+r+1, 0), v, S);	/* load the value */
		while(y1 > y0) {
			exy = eps + dxsq + dysq;
			if(-exy <= eps+dxsq) {
				bitblt(b, Pt(x0, y0), bp, Rect(x0, 0, x1, 1), f);
				bitblt(b, Pt(x0, y1), bp, Rect(x0, 0, x1, 1), f);
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
		bitblt(b, Pt(x0, y0), bp, Rect(x0, 0, x1, 1), f);
		bfree(bp);
	}else{
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
}
