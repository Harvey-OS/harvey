#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../common/common.h"
#include "tr2post.h"

BOOLEAN drawflag = FALSE;

void
cover(double x, double y) {
}

void
drawspline(Biobufhdr *Bp, int flag) {	/* flag!=1 connect end points */
	int x[100], y[100];
	int i, N;
/*
 *
 * Spline drawing routine for Postscript printers. The complicated stuff is
 * handled by procedure Ds, which should be defined in the library file. I've
 * seen wrong implementations of troff's spline drawing, so fo the record I'll
 * write down the parametric equations and the necessary conversions to Bezier
 * cubic splines (as used in Postscript).
 *
 *
 * Parametric equation (x coordinate only):
 *
 *
 *	    (x2 - 2 * x1 + x0)    2                    (x0 + x1)
 *	x = ------------------ * t   + (x1 - x0) * t + ---------
 *		    2					   2
 *
 *
 * The coefficients in the Bezier cubic are,
 *
 *
 *	A = 0
 *	B = (x2 - 2 * x1 + x0) / 2
 *	C = x1 - x0
 *
 *
 * while the current point is,
 *
 *	current-point = (x0 + x1) / 2
 *
 * Using the relationships given in the Postscript manual (page 121) it's easy to
 * see that the control points are given by,
 *
 *
 *	x0' = (x0 + 5 * x1) / 6
 *	x1' = (x2 + 5 * x1) / 6
 *	x2' = (x1 + x2) / 2
 *
 *
 * where the primed variables are the ones used by curveto. The calculations
 * shown above are done in procedure Ds using the coordinates set up in both
 * the x[] and y[] arrays.
 *
 * A simple test of whether your spline drawing is correct would be to use cip
 * to draw a spline and some tangent lines at appropriate points and then print
 * the file.
 *
 */

	for (N=2; N<sizeof(x)/sizeof(x[0]); N++)
		if (Bgetfield(Bp, 'd', &x[N], 0)<=0 || Bgetfield(Bp, 'd', &y[N], 0)<=0)
			break;

	x[0] = x[1] = hpos;
	y[0] = y[1] = vpos;

	for (i = 1; i < N; i++) {
		x[i+1] += x[i];
		y[i+1] += y[i];
	}

	x[N] = x[N-1];
	y[N] = y[N-1];

	for (i = ((flag!=1)?0:1); i < ((flag!=1)?N-1:N-2); i++) {
		endstring();
		if (pageon())
			Bprint(Bstdout, "%d %d %d %d %d %d Ds\n", x[i], y[i], x[i+1], y[i+1], x[i+2], y[i+2]);
/*		if (dobbox == TRUE) {		/* could be better */
/*	    		cover((double)(x[i] + x[i+1])/2,(double)-(y[i] + y[i+1])/2);
/*	    		cover((double)x[i+1], (double)-y[i+1]);
/*	    		cover((double)(x[i+1] + x[i+2])/2, (double)-(y[i+1] + y[i+2])/2);
/*		}
 */
	}

	hpos = x[N];			/* where troff expects to be */
	vpos = y[N];
}

void
draw(Biobufhdr *Bp) {

	int r, x1, y1, x2, y2, i;
	int d1, d2;

	drawflag = TRUE;
	r = Bgetrune(Bp);
	switch(r) {
	case 'l':
		if (Bgetfield(Bp, 'd', &x1, 0)<=0 || Bgetfield(Bp, 'd', &y1, 0)<=0 || Bgetfield(Bp, 'r', &i, 0)<=0)
			error(FATAL, "draw line function, destination coordinates not found.\n");

		endstring();
		if (pageon())
			Bprint(Bstdout, "%d %d %d %d Dl\n", hpos, vpos, hpos+x1, vpos+y1);
		hpos += x1;
		vpos += y1;
		break;
	case 'c':
		if (Bgetfield(Bp, 'd', &d1, 0)<=0)
			error(FATAL, "draw circle function, diameter coordinates not found.\n");

		endstring();
		if (pageon())
			Bprint(Bstdout, "%d %d %d %d De\n", hpos, vpos, d1, d1);
		hpos += d1;
		break;
	case 'e':
		if (Bgetfield(Bp, 'd', &d1, 0)<=0 || Bgetfield(Bp, 'd', &d2, 0)<=0)
			error(FATAL, "draw ellipse function, diameter coordinates not found.\n");

		endstring();
		if (pageon())
			Bprint(Bstdout, "%d %d %d %d De\n", hpos, vpos, d1, d2);
		hpos += d1;
		break;
	case 'a':
		if (Bgetfield(Bp, 'd', &x1, 0)<=0 || Bgetfield(Bp, 'd', &y1, 0)<=0 || Bgetfield(Bp, 'd', &x2, 0)<=0 || Bgetfield(Bp, 'd', &y2, 0)<=0)
			error(FATAL, "draw arc function, coordinates not found.\n");

		endstring();
		if (pageon())
			Bprint(Bstdout, "%d %d %d %d %d %d Da\n", hpos, vpos, x1, y1, x2, y2);
		hpos += x1 + x2;
		vpos += y1 + y2;
		break;
	case 'q':
		drawspline(Bp, 1);
		break;
	case '~':
		drawspline(Bp, 2);
		break;
	default:
		error(FATAL, "unknown draw function <%c>\n", r);
		break;
	}
}
