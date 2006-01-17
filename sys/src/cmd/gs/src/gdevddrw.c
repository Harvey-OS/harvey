/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevddrw.c,v 1.27 2004/12/08 21:35:13 stefan Exp $ */
/* Default polygon and image drawing device procedures */
#include <assert.h>
#include "math_.h"
#include "memory_.h"
#include "stdint_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsrect.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxiparam.h"
#include "gxistate.h"
#include "gdevddrw.h"
#include "vdtrace.h"
/*
#include "gxdtfill.h" - Do not remove this comment.
                        "gxdtfill.h" is included below.
*/

#define VD_RECT_COLOR RGB(0, 0, 255)

#define SWAP(a, b, t)\
  (t = a, a = b, b = t)

/* ---------------- Polygon and line drawing ---------------- */

/* Define the 'remainder' analogue of fixed_mult_quo. */
private fixed
fixed_mult_rem(fixed a, fixed b, fixed c)
{
    /* All kinds of truncation may happen here, but it's OK. */
    return a * b - fixed_mult_quo(a, b, c) * c;
}

/*
 * The trapezoid fill algorithm uses trap_line structures to keep track of
 * the left and right edges during the Bresenham loop.
 */
typedef struct trap_line_s {
	/*
	 * h is the y extent of the line (edge.end.y - edge.start.y).
	 * We know h > 0.
	 */
    fixed h;
	/*
	 * The dx/dy ratio for the line is di + df/h.
	 * (The quotient refers to the l.s.b. of di, not fixed_1.)
	 * We know 0 <= df < h.
	 */
    int di;
    fixed df;
	/*
	 * The intersection of the line with a scan line is x + xf/h + 1.
	 * (The 1 refers to the least significant bit of x, not fixed_1;
	 * similarly, the quotient refers to the l.s.b. of x.)
	 * We know -h <= xf < 0.
	 *
	 * This rational value preciselly represents the mathematical line
	 * (with no machine arithmetic error).
	 *
	 * Note that the fractional part is negative to simplify
	 * some conditions in the Bresenham algorithm.
	 * Due to that some expressions are inobvious.
	 * We believe that it's a kind of archaic
	 * for the modern hyperthreading architecture,
	 * we still keep it because the code passed a huge testing
	 * on various platforms.
	 */
    fixed x, xf;
	/*
	 * We increment (x,xf) by (ldi,ldf) after each scan line.
	 * (ldi,ldf) is just (di,df) converted to fixed point.
	 * We know 0 <= ldf < h.
	 */
    fixed ldi, ldf;
} trap_line;


/*
 * The linear color trapezoid fill algorithm uses trap_color structures to keep track of
 * the color change during the Bresenham loop.
 */
typedef struct trap_gradient_s {
	frac31 *c; /* integer part of the color in frac32 units. */
	int32_t *f; /* the fraction part numerator */
	int32_t *num; /* the gradient numerator */
	int32_t den; /* color gradient denominator */
} trap_gradient;


/*
 * Compute the di and df members of a trap_line structure.  The x extent
 * (edge.end.x - edge.start.x) is a parameter; the y extent (h member)
 * has already been set.  Also adjust x for the initial y.
 */
inline private void
compute_dx(trap_line *tl, fixed xd, fixed ys)
{
    fixed h = tl->h;
    int di;

    if (xd >= 0) {
	if (xd < h)
	    tl->di = 0, tl->df = xd;
	else {
	    tl->di = di = (int)(xd / h);
	    tl->df = xd - di * h;
	    tl->x += ys * di;
	}
    } else {
	if ((tl->df = xd + h) >= 0 /* xd >= -h */)
	    tl->di = -1, tl->x -= ys;
	else {
	    tl->di = di = (int)-((h - 1 - xd) / h);
	    tl->df = xd - di * h;
	    tl->x += ys * di;
	}
    }
}

#define YMULT_LIMIT (max_fixed / fixed_1)

/* Compute ldi, ldf, and xf similarly. */
inline private void
compute_ldx(trap_line *tl, fixed ys)
{
    int di = tl->di;
    fixed df = tl->df;
    fixed h = tl->h;

    if ( df < YMULT_LIMIT ) {
	 if ( df == 0 )		/* vertical edge, worth checking for */
	     tl->ldi = int2fixed(di), tl->ldf = 0, tl->xf = -h;
	 else {
	     tl->ldi = int2fixed(di) + int2fixed(df) / h;
	     tl->ldf = int2fixed(df) % h;
	     tl->xf =
		 (ys < fixed_1 ? ys * df % h : fixed_mult_rem(ys, df, h)) - h;
	 }
    }
    else {
	tl->ldi = int2fixed(di) + fixed_mult_quo(fixed_1, df, h);
	tl->ldf = fixed_mult_rem(fixed_1, df, h);
	tl->xf = fixed_mult_rem(ys, df, h) - h;
    }
}

private inline void
init_gradient(trap_gradient *g, const gs_fill_attributes *fa,
		const gs_linear_color_edge *e, const gs_linear_color_edge *e1,
		const trap_line *l, fixed ybot, int num_components)
{
    int i;
    int64_t c;
    int32_t d;

    if (e->c1 == NULL || e->c0 == NULL)
	g->den = 0; /* A wedge - the color is axial along another edge. */
    else {
	bool ends_from_fa = (e1->c1 == NULL || e1->c0 == NULL);

	if (ends_from_fa)
	    g->den = fa->yend - fa->ystart;
	else {
	    g->den = e->end.y - e->start.y;
	    assert(g->den == l->h);
	}
	for (i = 0; i < num_components; i++) {
	    g->num[i] = e->c1[i] - e->c0[i];
	    c = (int64_t)g->num[i] * (uint32_t)(ybot - 
		    (ends_from_fa ? fa->ystart : e->start.y));
	    d = (int32_t)(c / g->den);
	    g->c[i] = e->c0[i] + d;
	    c -= (int64_t)d * g->den;
	    if (c < 0) {
		g->c[i]--;
		c += g->den;
	    }
	    g->f[i] = (int32_t)c;
	}
    }
}

private inline void
step_gradient(trap_gradient *g, int num_components)
{
    int i;

    if (g->den == 0)
	return;
    for (i = 0; i < num_components; i++) {
	int64_t fc = g->f[i] + (int64_t)g->num[i] * fixed_1;
	int32_t fc32;

	g->c[i] += (int32_t)(fc / g->den);
	fc32 = (int32_t)(fc -  fc / g->den * g->den);
	if (fc32 < 0) {
	    fc32 += g->den;
	    g->c[i]--;
	}
	g->f[i] = fc32;
    }
}

private inline bool
check_gradient_overflow(const gs_linear_color_edge *le, const gs_linear_color_edge *re,
		int num_components)
{
    if (le->c1 == NULL || re->c1 == NULL) {
	/* A wedge doesn't use a gradient by X. */
	return false;
    } else {
	/* Check whether set_x_gradient, fill_linear_color_scanline can overflow. 

	   dev_proc(dev, fill_linear_color_scanline) can perform its computation in 32-bit fractions,
	   so we assume it never overflows. Devices which implement it with no this
	   assumption must implement the check in gx_default_fill_linear_color_trapezoid,
	   gx_default_fill_linear_color_triangle with a function other than this one.

	   Since set_x_gradient perform computations in int64_t, which provides 63 bits
	   while multiplying a 32-bits color value to a coordinate,
	   we must restrict the X span with 63 - 32 = 31 bits.
	 */
	int32_t xl = min(le->start.x, le->end.x);
	int32_t xr = min(re->start.x, re->end.x);
	/* The pixel span boundaries : */
	return arith_rshift_1(xr) - arith_rshift_1(xl) >= 0x3FFFFFFE;
    }
}


private inline int
set_x_gradient_nowedge(trap_gradient *xg, const trap_gradient *lg, const trap_gradient *rg, 
	     const trap_line *l, const trap_line *r, int il, int ir, int num_components)
{
    /* Ignoring the ending coordinats fractions, 
       so the gridient is slightly shifted to the left (in <1 'fixed' unit). */
    int32_t xl = l->x - (l->xf == -l->h ? 1 : 0) - fixed_half; /* Revert the GX_FILL_TRAPEZOID shift. */
    int32_t xr = r->x - (r->xf == -r->h ? 1 : 0) - fixed_half; /* Revert the GX_FILL_TRAPEZOID shift. */
    /* The pixel span boundaries : */
    int32_t x0 = int2fixed(il) + fixed_half; /* Shift to the pixel center. */
    int32_t x1 = int2fixed(ir) + fixed_half; /* Shift to the pixel center. */
    int i;

#   ifdef DEBUG
	if (arith_rshift_1(xr) - arith_rshift_1(xl) >= 0x3FFFFFFE) /* Can overflow ? */
	    return_error(gs_error_unregistered); /* Must not happen. */
#   endif
    xg->den = fixed2int(x1 - x0);
    for (i = 0; i < num_components; i++) {
	/* Ignoring the ending colors fractions, 
	   so the color gets a slightly smaller value
	   (in <1 'frac31' unit), but it's not important due to 
	   the further conversion to [0, 1 << cinfo->comp_bits[j]], 
	   which drops the fraction anyway. */
	int32_t cl = lg->c[i];
	int32_t cr = rg->c[i];
	int32_t c0 = (int32_t)(cl + ((int64_t)cr - cl) * (x0 - xl) / (xr - xl));
	int32_t c1 = (int32_t)(cl + ((int64_t)cr - cl) * (x1 - xl) / (xr - xl));

	xg->c[i] = c0;
	xg->f[i] = 0; /* Insufficient bits to compute it better. 
	                 The color so the color gets a slightly smaller value
			 (in <1 'frac31' unit), but it's not important due to 
			 the further conversion to [0, 1 << cinfo->comp_bits[j]], 
			 which drops the fraction anyway. 
			 So setting 0 appears pretty good and fast. */
	xg->num[i] = c1 - c0;
    }
    return 0;
}

private inline int
set_x_gradient(trap_gradient *xg, const trap_gradient *lg, const trap_gradient *rg, 
	     const trap_line *l, const trap_line *r, int il, int ir, int num_components)
{
    if (lg->den == 0 || rg->den == 0) {
	/* A wedge doesn't use a gradient by X. */
	int i;

	xg->den = 1;
	for (i = 0; i < num_components; i++) {
	    xg->c[i] = (lg->den == 0 ? rg->c[i] : lg->c[i]);
	    xg->f[i] = 0; /* Compatible to set_x_gradient_nowedge. */
	    xg->num[i] = 0;
	}
	return 0;
    } else
	return set_x_gradient_nowedge(xg, lg, rg, l, r, il, ir, num_components);
}

/*
 * Fill a trapezoid.
 * Since we need several statically defined variants of this algorithm,
 * we stored it in gxdtfill.h and include it configuring with
 * macros defined here.
 */
#define LINEAR_COLOR 0 /* Common for shading variants. */
#define EDGE_TYPE gs_fixed_edge  /* Common for non-shading variants. */
#define FILL_ATTRS gs_logical_operation_t  /* Common for non-shading variants. */

#define GX_FILL_TRAPEZOID private int gx_fill_trapezoid_as_fd
#define CONTIGUOUS_FILL 0
#define SWAP_AXES 1
#define FILL_DIRECT 1
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#define GX_FILL_TRAPEZOID private int gx_fill_trapezoid_as_nd
#define CONTIGUOUS_FILL 0
#define SWAP_AXES 1
#define FILL_DIRECT 0
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#define GX_FILL_TRAPEZOID private int gx_fill_trapezoid_ns_fd
#define CONTIGUOUS_FILL 0
#define SWAP_AXES 0
#define FILL_DIRECT 1
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#define GX_FILL_TRAPEZOID private int gx_fill_trapezoid_ns_nd
#define CONTIGUOUS_FILL 0
#define SWAP_AXES 0
#define FILL_DIRECT 0
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#define GX_FILL_TRAPEZOID int gx_fill_trapezoid_cf_fd
#define CONTIGUOUS_FILL 1
#define SWAP_AXES 0
#define FILL_DIRECT 1
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#define GX_FILL_TRAPEZOID int gx_fill_trapezoid_cf_nd
#define CONTIGUOUS_FILL 1
#define SWAP_AXES 0
#define FILL_DIRECT 0
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#undef EDGE_TYPE
#undef LINEAR_COLOR
#undef FILL_ATTRS

#define LINEAR_COLOR 1 /* Common for shading variants. */
#define EDGE_TYPE gs_linear_color_edge /* Common for shading variants. */
#define FILL_ATTRS const gs_fill_attributes *  /* Common for non-shading variants. */

#define GX_FILL_TRAPEZOID private int gx_fill_trapezoid_ns_lc
#define CONTIGUOUS_FILL 0
#define SWAP_AXES 0
#define FILL_DIRECT 1
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#define GX_FILL_TRAPEZOID private int gx_fill_trapezoid_as_lc
#define CONTIGUOUS_FILL 0
#define SWAP_AXES 1
#define FILL_DIRECT 1
#include "gxdtfill.h"
#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL 
#undef SWAP_AXES
#undef FILL_DIRECT

#undef EDGE_TYPE
#undef LINEAR_COLOR
#undef FILL_ATTRS


int
gx_default_fill_trapezoid(gx_device * dev, const gs_fixed_edge * left,
    const gs_fixed_edge * right, fixed ybot, fixed ytop, bool swap_axes,
    const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    bool fill_direct = color_writes_pure(pdevc, lop);

    if (swap_axes) {
	if (fill_direct)	
	    return gx_fill_trapezoid_as_fd(dev, left, right, ybot, ytop, 0, pdevc, lop);
	else
	    return gx_fill_trapezoid_as_nd(dev, left, right, ybot, ytop, 0, pdevc, lop);
    } else {
	if (fill_direct)	
	    return gx_fill_trapezoid_ns_fd(dev, left, right, ybot, ytop, 0, pdevc, lop);
	else
	    return gx_fill_trapezoid_ns_nd(dev, left, right, ybot, ytop, 0, pdevc, lop);
    }
}

private inline void
middle_frac31_color(frac31 *c, const frac31 *c0, const frac31 *c2, int num_components)
{
    /* Assuming non-negative values. */
    int i;

    for (i = 0; i < num_components; i++)
	c[i] = (int32_t)(((uint32_t)c0[i] + (uint32_t)c2[i]) >> 1);
}

private inline int
fill_linear_color_trapezoid_nocheck(gx_device *dev, const gs_fill_attributes *fa,
	const gs_linear_color_edge *le, const gs_linear_color_edge *re)
{
    fixed y02 = max(le->start.y, re->start.y), ymin = max(y02, fa->clip->p.y);
    fixed y13 = min(le->end.y, re->end.y), ymax = min(y13, fa->clip->q.y);
    int code;

    code = (fa->swap_axes ? gx_fill_trapezoid_as_lc : gx_fill_trapezoid_ns_lc)(dev, 
	    le, re, ymin, ymax, 0, NULL, fa);
    if (code < 0)
	return code;
    return !code;
}


/*  Fill a trapezoid with a linear color.
    [p0 : p1] - left edge, from bottom to top.
    [p2 : p3] - right edge, from bottom to top.
    The filled area is within Y-spans of both edges.

    This implemetation actually handles a bilinear color,
    in which the generatrix keeps a parallelizm to the X axis.
    In general a bilinear function doesn't keep the generatrix parallelizm, 
    so the caller must decompose/approximate such functions.

    Return values : 
    1 - success;
    0 - Too big. The area isn't filled. The client must decompose the area.
    <0 - error.
 */
int 
gx_default_fill_linear_color_trapezoid(gx_device *dev, const gs_fill_attributes *fa,
	const gs_fixed_point *p0, const gs_fixed_point *p1,
	const gs_fixed_point *p2, const gs_fixed_point *p3,
	const frac31 *c0, const frac31 *c1,
	const frac31 *c2, const frac31 *c3)
{
    gs_linear_color_edge le, re;
    int num_components = dev->color_info.num_components;

    le.start = *p0;
    le.end = *p1;
    le.c0 = c0;
    le.c1 = c1;
    le.clip_x = fa->clip->p.x;
    re.start = *p2;
    re.end = *p3;
    re.c0 = c2;
    re.c1 = c3;
    re.clip_x = fa->clip->q.x;
    if (check_gradient_overflow(&le, &re, num_components))
        return 0;
    return fill_linear_color_trapezoid_nocheck(dev, fa, &le, &re);
}

private inline int 
fill_linear_color_triangle(gx_device *dev, const gs_fill_attributes *fa,
	const gs_fixed_point *p0, const gs_fixed_point *p1,
	const gs_fixed_point *p2,
	const frac31 *c0, const frac31 *c1, const frac31 *c2)
{   /* p0 must be the lowest vertex. */
    int code;
    gs_linear_color_edge e0, e1, e2;
    int num_components = dev->color_info.num_components;

    if (p0->y == p1->y)
	return gx_default_fill_linear_color_trapezoid(dev, fa, p0, p2, p1, p2, c0, c2, c1, c2);
    if (p1->y == p2->y)
	return gx_default_fill_linear_color_trapezoid(dev, fa, p0, p2, p0, p1, c0, c2, c0, c1);
    e0.start = *p0;
    e0.end = *p2;
    e0.c0 = c0;
    e0.c1 = c2;
    e0.clip_x = fa->clip->p.x;
    e1.start = *p0;
    e1.end = *p1;
    e1.c0 = c0;
    e1.c1 = c1;
    e1.clip_x = fa->clip->q.x;
    if (p0->y < p1->y && p1->y < p2->y) {
	e2.start = *p1;
	e2.end = *p2;
	e2.c0 = c1;
	e2.c1 = c2;
	e2.clip_x = fa->clip->q.x;
	if (check_gradient_overflow(&e0, &e1, num_components))
	    return 0;
	if (check_gradient_overflow(&e0, &e2, num_components))
	    return 0;
	code = fill_linear_color_trapezoid_nocheck(dev, fa, &e0, &e1);
	if (code <= 0) /* Sic! */
	    return code;
	return fill_linear_color_trapezoid_nocheck(dev, fa, &e0, &e2);
    } else { /* p0->y < p2->y && p2->y < p1->y */
	e2.start = *p2;
	e2.end = *p1;
	e2.c0 = c2;
	e2.c1 = c1;
	e2.clip_x = fa->clip->q.x;
	if (check_gradient_overflow(&e0, &e1, num_components))
	    return 0;
	if (check_gradient_overflow(&e2, &e1, num_components))
	    return 0;
	code = fill_linear_color_trapezoid_nocheck(dev, fa, &e0, &e1);
	if (code <= 0) /* Sic! */
	    return code;
	return fill_linear_color_trapezoid_nocheck(dev, fa, &e2, &e1);
    }
}

/*  Fill a triangle with a linear color. */
int 
gx_default_fill_linear_color_triangle(gx_device *dev, const gs_fill_attributes *fa,
	const gs_fixed_point *p0, const gs_fixed_point *p1,
	const gs_fixed_point *p2,
	const frac31 *c0, const frac31 *c1, const frac31 *c2)
{
    fixed dx1 = p1->x - p0->x, dy1 = p1->y - p0->y;
    fixed dx2 = p2->x - p0->x, dy2 = p2->y - p0->y;
    
    if ((int64_t)dx1 * dy2 < (int64_t)dx2 * dy1) {
	const gs_fixed_point *p = p1;
	const frac31 *c = c1;
	
	p1 = p2; 
	p2 = p;
	c1 = c2;
	c2 = c;
    }
    if (p0->y <= p1->y && p0->y <= p2->y)
	return fill_linear_color_triangle(dev, fa, p0, p1, p2, c0, c1, c2);
    if (p1->y <= p0->y && p1->y <= p2->y)
	return fill_linear_color_triangle(dev, fa, p1, p2, p0, c1, c2, c0);
    else
	return fill_linear_color_triangle(dev, fa, p2, p0, p1, c2, c0, c1);
}

/* Fill a parallelogram whose points are p, p+a, p+b, and p+a+b. */
/* We should swap axes to get best accuracy, but we don't. */
/* We must be very careful to follow the center-of-pixel rule in all cases. */
int
gx_default_fill_parallelogram(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		  const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    fixed t;
    fixed qx, qy, ym;
    dev_proc_fill_trapezoid((*fill_trapezoid));
    gs_fixed_edge left, right;
    int code;

    /* Make a special fast check for rectangles. */
    if (PARALLELOGRAM_IS_RECT(ax, ay, bx, by)) {
	gs_int_rect r;

	INT_RECT_FROM_PARALLELOGRAM(&r, px, py, ax, ay, bx, by);
	return gx_fill_rectangle_device_rop(r.p.x, r.p.y, r.q.x - r.p.x,
					    r.q.y - r.p.y, pdevc, dev, lop);
    }
    /*
     * Not a rectangle.  Ensure that the 'a' line is to the left of
     * the 'b' line.  Testing ax <= bx is neither sufficient nor
     * necessary: in general, we need to compare the slopes.
     */
    /* Ensure ay >= 0, by >= 0. */
    if (ay < 0)
	px += ax, py += ay, ax = -ax, ay = -ay;
    if (by < 0)
	px += bx, py += by, bx = -bx, by = -by;
    qx = px + ax + bx;
    if ((ax ^ bx) < 0) {	/* In this case, the test ax <= bx is sufficient. */
	if (ax > bx)
	    SWAP(ax, bx, t), SWAP(ay, by, t);
    } else {			/*
				 * Compare the slopes.  We know that ay >= 0, by >= 0,
				 * and ax and bx have the same sign; the lines are in the
				 * correct order iff
				 *          ay/ax >= by/bx, or
				 *          ay*bx >= by*ax
				 * Eventually we can probably find a better way to test this,
				 * without using floating point.
				 */
	if ((double)ay * bx < (double)by * ax)
	    SWAP(ax, bx, t), SWAP(ay, by, t);
    }
    fill_trapezoid = dev_proc(dev, fill_trapezoid);
    qy = py + ay + by;
    left.start.x = right.start.x = px;
    left.start.y = right.start.y = py;
    left.end.x = px + ax;
    left.end.y = py + ay;
    right.end.x = px + bx;
    right.end.y = py + by;
#define ROUNDED_SAME(p1, p2)\
  (fixed_pixround(p1) == fixed_pixround(p2))
    if (ay < by) {
	if (!ROUNDED_SAME(py, left.end.y)) {
	    code = (*fill_trapezoid) (dev, &left, &right, py, left.end.y,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	left.start = left.end;
	left.end.x = qx, left.end.y = qy;
	ym = right.end.y;
	if (!ROUNDED_SAME(left.start.y, ym)) {
	    code = (*fill_trapezoid) (dev, &left, &right, left.start.y, ym,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	right.start = right.end;
	right.end.x = qx, right.end.y = qy;
    } else {
	if (!ROUNDED_SAME(py, right.end.y)) {
	    code = (*fill_trapezoid) (dev, &left, &right, py, right.end.y,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	right.start = right.end;
	right.end.x = qx, right.end.y = qy;
	ym = left.end.y;
	if (!ROUNDED_SAME(right.start.y, ym)) {
	    code = (*fill_trapezoid) (dev, &left, &right, right.start.y, ym,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	left.start = left.end;
	left.end.x = qx, left.end.y = qy;
    }
    if (!ROUNDED_SAME(ym, qy))
	return (*fill_trapezoid) (dev, &left, &right, ym, qy,
				  false, pdevc, lop);
    else
	return 0;
#undef ROUNDED_SAME
}

/* Fill a triangle whose points are p, p+a, and p+b. */
/* We should swap axes to get best accuracy, but we don't. */
int
gx_default_fill_triangle(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		  const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    fixed t;
    fixed ym;

    dev_proc_fill_trapezoid((*fill_trapezoid)) =
	dev_proc(dev, fill_trapezoid);
    gs_fixed_edge left, right;
    int code;

    /* Ensure ay >= 0, by >= 0. */
    if (ay < 0)
	px += ax, py += ay, bx -= ax, by -= ay, ax = -ax, ay = -ay;
    if (by < 0)
	px += bx, py += by, ax -= bx, ay -= by, bx = -bx, by = -by;
    /* Ensure ay <= by. */
    if (ay > by)
	SWAP(ax, bx, t), SWAP(ay, by, t);
    /*
     * Make a special check for a flat bottom or top,
     * which we can handle with a single call on fill_trapezoid.
     */
    left.start.x = right.start.x = px;
    left.start.y = right.start.y = py;
    if (ay == 0) {
	/* Flat top */
	if (ax < 0)
	    left.start.x = px + ax;
	else
	    right.start.x = px + ax;
	left.end.x = right.end.x = px + bx;
	left.end.y = right.end.y = py + by;
	ym = py;
    } else if (ay == by) {
	/* Flat bottom */
	if (ax < bx)
	    left.end.x = px + ax, right.end.x = px + bx;
	else
	    left.end.x = px + bx, right.end.x = px + ax;
	left.end.y = right.end.y = py + by;
	ym = py;
    } else {
	ym = py + ay;
	if (fixed_mult_quo(bx, ay, by) < ax) {
	    /* The 'b' line is to the left of the 'a' line. */
	    left.end.x = px + bx, left.end.y = py + by;
	    right.end.x = px + ax, right.end.y = py + ay;
	    code = (*fill_trapezoid) (dev, &left, &right, py, ym,
				      false, pdevc, lop);
	    right.start = right.end;
	    right.end = left.end;
	} else {
	    /* The 'a' line is to the left of the 'b' line. */
	    left.end.x = px + ax, left.end.y = py + ay;
	    right.end.x = px + bx, right.end.y = py + by;
	    code = (*fill_trapezoid) (dev, &left, &right, py, ym,
				      false, pdevc, lop);
	    left.start = left.end;
	    left.end = right.end;
	}
	if (code < 0)
	    return code;
    }
    return (*fill_trapezoid) (dev, &left, &right, ym, right.end.y,
			      false, pdevc, lop);
}

/* Draw a one-pixel-wide line. */
int
gx_default_draw_thin_line(gx_device * dev,
			  fixed fx0, fixed fy0, fixed fx1, fixed fy1,
		  const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    int ix = fixed2int_var(fx0);
    int iy = fixed2int_var(fy0);
    int itox = fixed2int_var(fx1);
    int itoy = fixed2int_var(fy1);

    return_if_interrupt(dev->memory);
    if (itoy == iy) {		/* horizontal line */
	return (ix <= itox ?
		gx_fill_rectangle_device_rop(ix, iy, itox - ix + 1, 1,
					     pdevc, dev, lop) :
		gx_fill_rectangle_device_rop(itox, iy, ix - itox + 1, 1,
					     pdevc, dev, lop)
	    );
    }
    if (itox == ix) {		/* vertical line */
	return (iy <= itoy ?
		gx_fill_rectangle_device_rop(ix, iy, 1, itoy - iy + 1,
					     pdevc, dev, lop) :
		gx_fill_rectangle_device_rop(ix, itoy, 1, iy - itoy + 1,
					     pdevc, dev, lop)
	    );
    } {
	fixed h = fy1 - fy0;
	fixed w = fx1 - fx0;
	fixed tf;
	bool swap_axes;
	gs_fixed_edge left, right;

	if ((w < 0 ? -w : w) <= (h < 0 ? -h : h)) {
	    if (h < 0)
		SWAP(fx0, fx1, tf), SWAP(fy0, fy1, tf),
		    h = -h;
	    right.start.x = (left.start.x = fx0 - fixed_half) + fixed_1;
	    right.end.x = (left.end.x = fx1 - fixed_half) + fixed_1;
	    left.start.y = right.start.y = fy0;
	    left.end.y = right.end.y = fy1;
	    swap_axes = false;
	} else {
	    if (w < 0)
		SWAP(fx0, fx1, tf), SWAP(fy0, fy1, tf),
		    w = -w;
	    right.start.x = (left.start.x = fy0 - fixed_half) + fixed_1;
	    right.end.x = (left.end.x = fy1 - fixed_half) + fixed_1;
	    left.start.y = right.start.y = fx0;
	    left.end.y = right.end.y = fx1;
	    swap_axes = true;
	}
	return (*dev_proc(dev, fill_trapezoid)) (dev, &left, &right,
						 left.start.y, left.end.y,
						 swap_axes, pdevc, lop);
    }
}

/* Stub out the obsolete procedure. */
int
gx_default_draw_line(gx_device * dev,
		     int x0, int y0, int x1, int y1, gx_color_index color)
{
    return -1;
}

/* ---------------- Image drawing ---------------- */

/* GC structures for image enumerator */
public_st_gx_image_enum_common();

private 
ENUM_PTRS_WITH(image_enum_common_enum_ptrs, gx_image_enum_common_t *eptr)
    return 0;
case 0: return ENUM_OBJ(gx_device_enum_ptr(eptr->dev));
ENUM_PTRS_END

private RELOC_PTRS_WITH(image_enum_common_reloc_ptrs, gx_image_enum_common_t *eptr)
{
    eptr->dev = gx_device_reloc_ptr(eptr->dev, gcst);
}
RELOC_PTRS_END

/*
 * gx_default_begin_image is only invoked for ImageType 1 images.  However,
 * the argument types are different, and if the device provides a
 * begin_typed_image procedure, we should use it.  See gxdevice.h.
 */
private int
gx_no_begin_image(gx_device * dev,
		  const gs_imager_state * pis, const gs_image_t * pim,
		  gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		  gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    return -1;
}
int
gx_default_begin_image(gx_device * dev,
		       const gs_imager_state * pis, const gs_image_t * pim,
		       gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		       gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    /*
     * Hand off to begin_typed_image, being careful to avoid a
     * possible recursion loop.
     */
    dev_proc_begin_image((*save_begin_image)) = dev_proc(dev, begin_image);
    gs_image_t image;
    const gs_image_t *ptim;
    int code;

    set_dev_proc(dev, begin_image, gx_no_begin_image);
    if (pim->format == format)
	ptim = pim;
    else {
	image = *pim;
	image.format = format;
	ptim = &image;
    }
    code = (*dev_proc(dev, begin_typed_image))
	(dev, pis, NULL, (const gs_image_common_t *)ptim, prect, pdcolor,
	 pcpath, memory, pinfo);
    set_dev_proc(dev, begin_image, save_begin_image);
    return code;
}

int
gx_default_begin_typed_image(gx_device * dev,
			const gs_imager_state * pis, const gs_matrix * pmat,
		   const gs_image_common_t * pic, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		      gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{	/*
	 * If this is an ImageType 1 image using the imager's CTM,
	 * defer to begin_image.
	 */
    if (pic->type->begin_typed_image == gx_begin_image1) {
	const gs_image_t *pim = (const gs_image_t *)pic;

	if (pmat == 0 ||
	    (pis != 0 && !memcmp(pmat, &ctm_only(pis), sizeof(*pmat)))
	    ) {
	    int code = (*dev_proc(dev, begin_image))
	    (dev, pis, pim, pim->format, prect, pdcolor,
	     pcpath, memory, pinfo);

	    if (code >= 0)
		return code;
	}
    }
    return (*pic->type->begin_typed_image)
	(dev, pis, pmat, pic, prect, pdcolor, pcpath, memory, pinfo);
}

/* Backward compatibility for obsolete driver procedures. */

int
gx_default_image_data(gx_device *dev, gx_image_enum_common_t * info,
		      const byte ** plane_data,
		      int data_x, uint raster, int height)
{
    return gx_image_data(info, plane_data, data_x, raster, height);
}

int
gx_default_end_image(gx_device *dev, gx_image_enum_common_t * info,
		     bool draw_last)
{
    return gx_image_end(info, draw_last);
}
