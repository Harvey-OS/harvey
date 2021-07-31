#include <alef.h>
#include <libg.h>

intern void
mark(Bitmap *b, Point c, int x, int y, int v, Fcode f)
{
	point(b, Pt(c.x+x, c.y+y), v, f);

	if(x!=0 || y!=0)
		point(b, Pt(c.x-x, c.y-y), v, f);

	if(x!=0 && y!=0) {
		point(b, Pt(c.x-x, c.y+y), v, f);
		point(b, Pt(c.x+x, c.y-y), v, f);
	}
}

void
ellipse(Bitmap *bp, Point c, int a, int b, int v, Fcode f)
{			/* e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */
	int t;
	int dxt, dyt;
	int d2xt, d2yt;
	int x, y, a2, b2;
	int xcrit, ycrit;

	x = 0;
	y = b;
	a2 = a*a;
	b2 = b*b;
	xcrit = 3*a2/4 + 1;
	ycrit = 3*b2/4 + 1;
	t = b2 + a2 -2*a2*b;	/* t = e(x+1,y-1) */
	dxt = b2*(2*x+3);
	dyt = a2*(-2*y+3);
	d2xt = 2*b2;
	d2yt = 2*a2;
	
	while(y > 0){
		mark(bp, c, x, y, v, f);
		if(t + a2*y < xcrit){	/* e(x+1,y-1/2) <= 0 */
			x += 1;
			t += dxt;
			dxt += d2xt;
		}else if(t - b2*x >= ycrit){ /* e(x+1/2,y-1) > 0 */
			y -= 1;
			t += dyt;
			dyt += d2yt;
		}else{
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;
		}
	}
	while(x <= a){
		mark(bp, c, x, y, v, f);
		x++;
	}
}
