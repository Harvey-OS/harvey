/*
 * Drawing routines used by dpic
 *	Dl x1 y1 x y
 *		Draws a line from the current point (x, y) to (x1, y1).
 *	De x y a b
 *		Draws an ellipse that has its left side at the current point (x, y)
 *		and horizontal and vertical axes lengths given by a and b respectively.
 *	Da x y dx1 dy1 dx2 dy2
 *		Draws a circular arc from the current point (x,y) to (x+dx1+dx2,y+dy1+dy2).
 *		The center of the circle is at (x+dx1, y+dy1). Arcs always go
 *		counter-clockwise from the starting point to the end point.
 *	Ds x0 y0 x1 y1 x2 y2
 *		Draws a quadratic B-spline connecting ((x0+x1)/2, (y0+y1)/2) to
 *		((x1+x2)/2, (y1+y2)/2).
 */
#include "ext.h"
/*
 * Draws a line from (hpos, vpos) to (hpos+dx, vpos+dy), and leaves the current
 * position at the endpoint.
 */
void drawline(int dx, int dy){
	if(dx==0 && dy==0)
		drawcirc(1);
	else if(pagenum>=0)
		setline(hpos, vpos, hpos+dx, vpos+dy);
	hgoto(hpos+dx);
	vgoto(vpos+dy);
}
void drawcirc(int d){
    drawellip(d, d);
}
/*
 * Draws an ellipse having axes lengths horizontally and vertically of a and
 * b. The left side of the ellipse is at the current point. After we're done
 * drawing the path we move the current position to the right side.
 */
void drawellip(int a, int b){
	if(a==0 && b==0) return;
	if(pagenum>=0)
		setellipse(hpos, vpos, a, b);
	hgoto(hpos+a);
	vgoto(vpos);
}
/*
 * Draw a counter-clockwise arc from the current point
 * (hpos, vpos) to (hpos+dx1+dx2, vpos+dy1+dy2). The center of the circle is the
 * point (hpos+dx1, vpos+dy1).
 *
 */
void drawarc(int dx1, int dy1, int dx2, int dy2){
	if(pagenum>=0 && (dx1!=0 || dy1!=0) && (dx2!=0 || dy2!=0))
		setarc(hpos, vpos, hpos+dx1+dx2, vpos+dy1+dy2, hpos+dx1, vpos+dy1);
	hgoto(hpos+dx1+dx2);
	vgoto(vpos+dy1+dy2);
}
/*
 * Spline drawing routine for Postscript printers. The complicated stuff is
 * handled by procedure Ds, which should be defined in the library file. I've
 * seen wrong implementations of troff's spline drawing, so fo the record I'll
 * write down the parametric equations and the necessary conversions to Bezier
 * cubic splines (as used in Postscript).
 *
 * Parametric equation (x coordinate only):
 *
 *
 *	    (x2 - 2 * x1 + x0)    2                    (x0 + x1)
 *	x = ------------------ * t   + (x1 - x0) * t + ---------
 *		    2					   2
 *
 * The coefficients in the Bezier cubic are,
 *
 *	A = 0
 *	B = (x2 - 2 * x1 + x0) / 2
 *	C = x1 - x0
 *
 * while the current point is,
 *	current-point = (x0 + x1) / 2
 * Using the relationships given in the Postscript manual (page 121) it's easy to
 * see that the control points are given by,
 *
 *	x0' = (x0 + 5 * x1) / 6
 *	x1' = (x2 + 5 * x1) / 6
 *	x2' = (x1 + x2) / 2
 *
 * where the primed variables are the ones used by curveto. The calculations
 * shown above are done in procedure Ds using the coordinates set up in both
 * the x[] and y[] arrays.
 *
 * A simple test of whether your spline drawing is correct would be to use cip
 * to draw a spline and some tangent lines at appropriate points and then print
 * the file.
 */
void drawspline(FILE *fp, int flag){
	int x[100], y[100];
	int i, N;
	for(N=2;N<sizeof(x)/sizeof(x[0]);N++)
		if(fscanf(fp, "%d %d", &x[N], &y[N])!=2)
			break;
	x[0]=x[1]=hpos;
	y[0]=y[1]=vpos;
	for(i=2;i<N;i++){
		x[i]+=x[i-1];
		y[i]+=y[i-1];
	}
	x[N]=x[N-1];
	y[N]=y[N-1];
	if(pagenum>=0){
		if(flag==1){
			i=1;
			N-=2;
		}
		else{
			i=0;
			--N;
		}
		for(;i<N;i++)
			setspline(x[i], y[i], x[i+1], y[i+1], x[i+2], y[i+2]);
	}
	hgoto(x[N]);
	vgoto(y[N]);
}
