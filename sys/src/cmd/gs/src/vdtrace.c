/* Copyright (C) 2002 artofcode LLC. All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: vdtrace.c,v 1.10 2004/12/10 00:34:12 giles Exp $ */
/* Visual tracer service */

#include "math_.h"
#include "gxfixed.h"
#include "vdtrace.h"

/* Global data for all instances : */
vd_trace_interface * vd_trace0 = NULL, * vd_trace1 = NULL;
char vd_flags[128] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                     "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                     "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                     "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

private double px, py;

#define NullRET if(vd_trace1 == NULL) return

private inline double scale_x(vd_trace_interface *I, double x)
{ return (x - I->orig_x) * I->scale_x + I->shift_x;
}

private inline double scale_y(vd_trace_interface *I, double y)
{ return (y - I->orig_y) * I->scale_y + I->shift_y;
}

#define SX(x) scale_x(vd_trace1, x)
#define SY(y) scale_y(vd_trace1, y)

private inline double bezier_point(double p0, double p1, double p2, double p3, double t)
{   double s = 1-t;
    return p0*s*s*s + 3*p1*s*s*t + 3*p2*s*t*t + p3*t*t*t;
}

private void vd_flatten(double p0x, double p0y, double p1x, double p1y, double p2x, double p2y, double p3x, double p3y)
{   
#ifdef DEBUG
    double flat = 0.5;
    double d2x0 = (p0x - 2 * p1x + p2x), d2y0 = (p0y - 2 * p1y + p2y);
    double d2x1 = (p1x - 2 * p2x + p3x), d2y1 = (p1y - 2 * p2y + p3y);
    double d2norm0 = hypot(d2x0, d2y0);
    double d2norm1 = hypot(d2x1, d2y1);
    double D = max(d2norm0, d2norm1); /* This is half of maximum norm of 2nd derivative of the curve by parameter t. */
    int NN = (int)ceil(sqrt(D * 3 / 4 / flat)); /* Number of output segments. */
    int i;
    int N = max(NN, 1); /* safety (if the curve degenerates to line) */
    double e = 0.5 / N;

    for (i = 0; i < N; i++) {
	double t = (double)i / N + e;
	double px = bezier_point(p0x, p1x, p2x, p3x, t);
	double py = bezier_point(p0y, p1y, p2y, p3y, t);

	vd_lineto(px, py);
    }
    vd_lineto(p3x, p3y);
#endif
}

void vd_impl_moveto(double x, double y)
{   NullRET;
    px = SX(x), py = SY(y);
    vd_trace1->moveto(vd_trace1, px, py);
}

void vd_impl_lineto(double x, double y)
{   NullRET;
    px = SX(x), py = SY(y);
    vd_trace1->lineto(vd_trace1, px, py);
}

void vd_impl_lineto_multi(const struct gs_fixed_point_s *p, int n)
{   int i;
    NullRET;
    for (i = 0; i < n; i++) {
        px = SX(p[i].x), py = SY(p[i].y);
        vd_trace1->lineto(vd_trace1, px, py);
    }
}

void vd_impl_curveto(double x1, double y1, double x2, double y2, double x3, double y3)
{   double p1x, p1y, p2x, p2y, p3x, p3y;

    NullRET;
    p1x = SX(x1), p1y = SY(y1);
    p2x = SX(x2), p2y = SY(y2);
    p3x = SX(x3), p3y = SY(y3);
    if (vd_trace1->curveto != NULL)
        vd_trace1->curveto(vd_trace1, p1x, p1y, p2x, p2y, p3x, p3y);
    else
        vd_flatten(px, py, p1x, p1y, p2x, p2y, p3x, p3y);
    px = p3x, py = p3y;
}

void vd_impl_bar(double x0, double y0, double x1, double y1, int w, unsigned long c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, w);
    vd_trace1->beg_path(vd_trace1);
    vd_trace1->moveto(vd_trace1, SX(x0), SY(y0));
    vd_trace1->lineto(vd_trace1, SX(x1), SY(y1));
    vd_trace1->end_path(vd_trace1);
    vd_trace1->stroke(vd_trace1);
}

void vd_impl_square(double x, double y, int w, unsigned int c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, 1);
    vd_trace1->beg_path(vd_trace1);
    vd_trace1->moveto(vd_trace1, SX(x) - w, SY(y) - w);
    vd_trace1->lineto(vd_trace1, SX(x) + w, SY(y) - w);
    vd_trace1->lineto(vd_trace1, SX(x) + w, SY(y) + w);
    vd_trace1->lineto(vd_trace1, SX(x) - w, SY(y) + w);
    vd_trace1->lineto(vd_trace1, SX(x) - w, SY(y) - w);
    vd_trace1->end_path(vd_trace1);
    vd_trace1->stroke(vd_trace1);
}

void vd_impl_rect(double x0, double y0, double x1, double y1, int w, unsigned int c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, w);
    vd_trace1->beg_path(vd_trace1);
    vd_trace1->moveto(vd_trace1, SX(x0), SY(y0));
    vd_trace1->lineto(vd_trace1, SX(x0), SY(y1));
    vd_trace1->lineto(vd_trace1, SX(x1), SY(y1));
    vd_trace1->lineto(vd_trace1, SX(x1), SY(y0));
    vd_trace1->lineto(vd_trace1, SX(x0), SY(y0));
    vd_trace1->end_path(vd_trace1);
    vd_trace1->stroke(vd_trace1);
}

void vd_impl_quad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int w, unsigned int c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, w);
    vd_trace1->beg_path(vd_trace1);
    vd_trace1->moveto(vd_trace1, SX(x0), SY(y0));
    vd_trace1->lineto(vd_trace1, SX(x1), SY(y1));
    vd_trace1->lineto(vd_trace1, SX(x2), SY(y2));
    vd_trace1->lineto(vd_trace1, SX(x3), SY(y3));
    vd_trace1->lineto(vd_trace1, SX(x0), SY(y0));
    vd_trace1->end_path(vd_trace1);
    vd_trace1->stroke(vd_trace1);
}

void vd_impl_curve(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int w, unsigned long c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, w);
    vd_trace1->beg_path(vd_trace1);
    vd_trace1->beg_path(vd_trace1);
    vd_trace1->moveto(vd_trace1, SX(x0), SY(y0));
    vd_impl_curveto(x1, y1, x2, y2, x3, y3);
    vd_trace1->end_path(vd_trace1);
    vd_trace1->stroke(vd_trace1);
}

void vd_impl_circle(double x, double y, int r, unsigned long c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, 1);
    vd_trace1->circle(vd_trace1, SX(x), SY(y), r);
}

void vd_impl_round(double x, double y, int r, unsigned long c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->setlinewidth(vd_trace1, 1);
    vd_trace1->round(vd_trace1, SX(x), SY(y), r);
}

void vd_impl_text(double x, double y, char *s, unsigned long c)
{   NullRET;
    vd_trace1->setcolor(vd_trace1, c);
    vd_trace1->text(vd_trace1, SX(x), SY(y), s);
}

void vd_setflag(char f, char v)
{   vd_flags[f & 127] = v;
}
