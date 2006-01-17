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

/* $Id: gxdtfill.h,v 1.27 2004/08/05 17:02:36 stefan Exp $ */
/* Configurable algorithm for filling a trapezoid */

/*
 * Since we need several statically defined variants of this agorithm,
 * we store it in .h file and include several times into gdevddrw.c and 
 * into gxfill.h . Configuration flags (macros) are :
 * 
 *   GX_FILL_TRAPEZOID - a name of method
 *   CONTIGUOUS_FILL   - prevent dropouts in narrow trapezoids
 *   SWAP_AXES         - assume swapped axes
 *   FILL_DIRECT       - See LOOP_FILL_RECTANGLE_DIRECT.
 *   LINEAR_COLOR      - Fill with a linear color.
 *   EDGE_TYPE	       - a type of edge structure.
 *   FILL_ATTRS        - operation attributes. 
 */

/*
 * Fill a trapezoid.  left.start => left.end and right.start => right.end
 * define the sides; ybot and ytop define the top and bottom.  Requires:
 *      {left,right}->start.y <= ybot <= ytop <= {left,right}->end.y.
 * Lines where left.x >= right.x will not be drawn.  Thanks to Paul Haeberli
 * for an early floating point version of this algorithm.
 */

/*
 * With CONTIGUOUS_FILL is off, 
 * this algorithm paints pixels, which centers fall between
 * the left and the right side of the trapezoid, excluding the 
 * right side (see PLRM3, 7.5. Scan conversion details). 
 * Particularly 0-width trapezoids are not painted.
 *
 * Similarly, it paints pixels, which centers
 * fall between ybot and ytop, excluding ytop.
 * Particularly 0-height trapezoids are not painted.
 *
 * With CONTIGUOUS_FILL is on, it paints a contigous area,
 * adding a minimal number of pixels outside the trapezoid.
 * Particularly it may paint pixels on the right and on the top sides,
 * if they are necessary for the contiguity.
 *
 * With LINEAR_COLOR returns 1 if the gradient arithmetics overflows.. 
 */

/*
We must paint pixels with index i such that

    Xl <= i + 0.5 < Xr

The condition is is equivalent to

    Xl - 0.5 <= i < Xr - 0.5

which is equivalent to

    (is_integer(Xl - 0.5) ? Xl - 0.5 : ceil(Xl - 0.5)) <= i <
    (is_integer(Xr - 0.5) ? Xr - 0.5 : floor(Xr - 0.5) + 1)

(the last '+1" happens due to the strong comparizon '<')
which is equivalent to

    ceil(Xl - 0.5) <= i < ceil(Xr - 0.5)

trap_line represents the intersection coordinate as a rational value :

    Xl = xl + e - fl
    Xr = xr + e - fr

Where 'e' is 'fixed_epsilon', 0.5 is 'fixed_half', and fl == l.fx / l.h, fr == - r.fx / r.h,
e <= fl < 0, e <= fr < 0.
Let 

    xl' := xl + 0.5
    xr' := xr + 0.5

Then 

    xl = xl' - 0.5
    xr = xr' - 0.5

    Xl = xl' - 0.5 + e - fl
    Xr = xr' - 0.5 + e - fr

    ceil(xl' - 0.5 + e - fl - 0.5) <= i < ceil(xr' - 0.5 + e - fr - 0.5)
    
which is equivalent to

    ceil(xl' + e - fl) - 1 <= i < ceil(xr' + e - fr) - 1

which is equivalent to

    (is_integer(xl' + e - fl) ? xl' + e - fl - 1 : ceil(xl' + e - fl) - 1) <= i <
    (is_integer(xr' + e - fr) ? xr' + e - fr - 1 : ceil(xr' + e - fr) - 1)

which is equivalent to

    (is_integer(xl' + e - fl) ? xl' + e - fl - 1 : floor(xl' + e - fl)) <= i <
    (is_integer(xr' + e - fr) ? xr' + e - fr - 1 : floor(xr' + e - fr))

which is equivalent to

    (is_integer(xl') && e == fl ? xl' - 1 : floor(xl' + e - fl)) <= i <
    (is_integer(xr') && e == fr ? xr' - 1 : floor(xr' + e - fr))

Note that e != fl ==> floor(xl' + e - fl) == floor(xl')  due to e - fl < LeastSignificantBit(xl') ;
          e == fl ==> floor(xl' + e - fl) == floor(xl')  due to e - fl == 0;

thus the condition is is equivalent to

    (is_integer(xl') && e == fl ? xl' - 1 : floor(xl')) <= i <
    (is_integer(xr') && e == fr ? xr' - 1 : floor(xr'))

It is computed with the macro 'rational_floor'.

*/

GX_FILL_TRAPEZOID (gx_device * dev, const EDGE_TYPE * left,
    const EDGE_TYPE * right, fixed ybot, fixed ytop, int flags,
    const gx_device_color * pdevc, FILL_ATTRS fa)
{
    const fixed ymin = fixed_pixround(ybot) + fixed_half;
    const fixed ymax = fixed_pixround(ytop);

    if (ymin >= ymax)
	return 0;		/* no scan lines to sample */
    {
	int iy = fixed2int_var(ymin);
	const int iy1 = fixed2int_var(ymax);
	trap_line l, r;
	register int rxl, rxr;
	int ry;
	const fixed
	    x0l = left->start.x, x1l = left->end.x, x0r = right->start.x,
	    x1r = right->end.x, dxl = x1l - x0l, dxr = x1r - x0r;
	const fixed	/* partial pixel offset to first line to sample */
	    ysl = ymin - left->start.y, ysr = ymin - right->start.y;
	fixed fxl;
	int code;
#	if CONTIGUOUS_FILL
	    const bool peak0 = ((flags & 1) != 0);
	    const bool peak1 = ((flags & 2) != 0);
	    int peak_y0 = ybot + fixed_half;
	    int peak_y1 = ytop - fixed_half;
#	endif
#	if LINEAR_COLOR
	    int num_components = dev->color_info.num_components;
	    frac31 lgc[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    int32_t lgf[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    int32_t lgnum[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    frac31 rgc[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    int32_t rgf[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    int32_t rgnum[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    frac31 xgc[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    int32_t xgf[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    int32_t xgnum[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    trap_gradient lg, rg, xg;
#	else
	    gx_color_index cindex = pdevc->colors.pure;
	    dev_proc_fill_rectangle((*fill_rect)) =
		dev_proc(dev, fill_rectangle);
#	endif

	if_debug2('z', "[z]y=[%d,%d]\n", iy, iy1);

	l.h = left->end.y - left->start.y;
	r.h = right->end.y - right->start.y;
	l.x = x0l + (fixed_half - fixed_epsilon);
	r.x = x0r + (fixed_half - fixed_epsilon);
	ry = iy;

/*
 * Free variables of FILL_TRAP_RECT:
 *	SWAP_AXES, pdevc, dev, fa
 * Free variables of FILL_TRAP_RECT_DIRECT:
 *	SWAP_AXES, fill_rect, dev, cindex
 */
#define FILL_TRAP_RECT_INDIRECT(x,y,w,h)\
  (SWAP_AXES ? gx_fill_rectangle_device_rop(y, x, h, w, pdevc, dev, fa) :\
   gx_fill_rectangle_device_rop(x, y, w, h, pdevc, dev, fa))
#define FILL_TRAP_RECT_DIRECT(x,y,w,h)\
  (SWAP_AXES ? (*fill_rect)(dev, y, x, h, w, cindex) :\
   (*fill_rect)(dev, x, y, w, h, cindex))

#if LINEAR_COLOR
#   define FILL_TRAP_RECT(x,y,w,h)\
	(!(w) ? 0 : dev_proc(dev, fill_linear_color_scanline)(dev, fa, x, y, w, xg.c, xg.f, xg.num, xg.den))
#else
#   define FILL_TRAP_RECT(x,y,w,h)\
	(FILL_DIRECT ? FILL_TRAP_RECT_DIRECT(x,y,w,h) : FILL_TRAP_RECT_INDIRECT(x,y,w,h))
#endif

#define VD_RECT_SWAPPED(rxl, ry, rxr, iy)\
    vd_rect(int2fixed(SWAP_AXES ? ry : rxl), int2fixed(SWAP_AXES ? rxl : ry),\
            int2fixed(SWAP_AXES ? iy : rxr), int2fixed(SWAP_AXES ? rxr : iy),\
	    1, VD_RECT_COLOR);

	/* Compute the dx/dy ratios. */

	/*
	 * Compute the x offsets at the first scan line to sample.  We need
	 * to be careful in computing ys# * dx#f {/,%} h# because the
	 * multiplication may overflow.  We know that all the quantities
	 * involved are non-negative, and that ys# is usually less than 1 (as
	 * a fixed, of course); this gives us a cheap conservative check for
	 * overflow in the multiplication.
	 */
#define YMULT_QUO(ys, tl)\
  (ys < fixed_1 && tl.df < YMULT_LIMIT ? ys * tl.df / tl.h :\
   fixed_mult_quo(ys, tl.df, tl.h))

#if CONTIGUOUS_FILL
/*
 * If left and right boundary round to same pixel index,
 * we would not paing the scan and would get a dropout.
 * Check for this case and choose one of two pixels
 * which is closer to the "axis". We need to exclude
 * 'peak' because it would paint an excessive pixel.
 */
#define SET_MINIMAL_WIDTH(ixl, ixr, l, r) \
    if (ixl == ixr) \
	if ((!peak0 || iy >= peak_y0) && (!peak1 || iy <= peak_y1)) {\
	    fixed x = int2fixed(ixl) + fixed_half;\
	    if (x - l.x < r.x - x)\
		++ixr;\
	    else\
		--ixl;\
	}

#define CONNECT_RECTANGLES(ixl, ixr, rxl, rxr, iy, ry, adj1, adj2, fill)\
    if (adj1 < adj2) {\
	if (iy - ry > 1) {\
	    VD_RECT_SWAPPED(rxl, ry, rxr, iy - 1);\
	    code = fill(rxl, ry, rxr - rxl, iy - ry - 1);\
	    if (code < 0)\
		goto xit;\
	    ry = iy - 1;\
	}\
	adj1 = adj2 = (adj2 + adj2) / 2;\
    }

#else
#define SET_MINIMAL_WIDTH(ixl, ixr, l, r) DO_NOTHING
#define CONNECT_RECTANGLES(ixl, ixr, rxl, rxr, iy, ry, adj1, adj2, fill) DO_NOTHING
#endif
	if (fixed_floor(l.x) == fixed_pixround(x1l)) {
	    /* Left edge is vertical, we don't need to increment. */
	    l.di = 0, l.df = 0;
	    fxl = 0;
	} else {
	    compute_dx(&l, dxl, ysl);
	    fxl = YMULT_QUO(ysl, l);
	    l.x += fxl;
	}
	if (fixed_floor(r.x) == fixed_pixround(x1r)) {
	    /* Right edge is vertical.  If both are vertical, */
	    /* we have a rectangle. */
#	    if !LINEAR_COLOR
		if (l.di == 0 && l.df == 0) {
		    rxl = fixed2int_var(l.x);
		    rxr = fixed2int_var(r.x);
		    SET_MINIMAL_WIDTH(rxl, rxr, l, r);
		    VD_RECT_SWAPPED(rxl, ry, rxr, iy1);
		    code = FILL_TRAP_RECT(rxl, ry, rxr - rxl, iy1 - ry);
		    goto xit;
		}
#	    endif
	    r.di = 0, r.df = 0;
	}
	/*
	 * The test for fxl != 0 is required because the right edge might
	 * cross some pixel centers even if the left edge doesn't.
	 */
	else if (dxr == dxl && fxl != 0) {
	    if (l.di == 0)
		r.di = 0, r.df = l.df;
	    else
		compute_dx(&r, dxr, ysr);
	    if (ysr == ysl && r.h == l.h)
		r.x += fxl;
	    else
		r.x += YMULT_QUO(ysr, r);
	} else {
	    compute_dx(&r, dxr, ysr);
	    r.x += YMULT_QUO(ysr, r);
	}
	/* Compute one line's worth of dx/dy. */
	compute_ldx(&l, ysl);
	compute_ldx(&r, ysr);
	/* We subtracted fixed_epsilon from l.x, r.x to simplify rounding
	   when the rational part is zero. Now add it back to get xl', xr' */
	l.x += fixed_epsilon;
	r.x += fixed_epsilon;
#	if LINEAR_COLOR
#	    ifdef DEBUG
		if (check_gradient_overflow(left, right, num_components)) {
		    /* The caller must care of. 
		       Checking it here looses some performance with triangles. */
		    return_error(gs_error_unregistered);
		}
#	    endif
	    lg.c = lgc;
	    lg.f = lgf;
	    lg.num = lgnum;
	    rg.c = rgc;
	    rg.f = rgf;
	    rg.num = rgnum;
	    xg.c = xgc;
	    xg.f = xgf;
	    xg.num = xgnum;
	    init_gradient(&lg, fa, left, right, &l, ymin, num_components);
	    init_gradient(&rg, fa, right, left, &r, ymin, num_components);

#	endif

#define rational_floor(tl)\
  fixed2int_var(fixed_is_int(tl.x) && tl.xf == -tl.h ? tl.x - fixed_1 : tl.x)
#define STEP_LINE(ix, tl)\
  tl.x += tl.ldi;\
  if ( (tl.xf += tl.ldf) >= 0 ) tl.xf -= tl.h, tl.x++;\
  ix = rational_floor(tl)

	rxl = rational_floor(l);
	rxr = rational_floor(r);
	SET_MINIMAL_WIDTH(rxl, rxr, l, r);
	while (LINEAR_COLOR ? 1 : ++iy != iy1) {
#	    if LINEAR_COLOR
		if (rxl != rxr) {
		    code = set_x_gradient(&xg, &lg, &rg, &l, &r, rxl, rxr, num_components);
		    if (code < 0)
			goto xit;
		    /*VD_RECT_SWAPPED(rxl, iy, rxr, iy + 1);*/
		    code = FILL_TRAP_RECT(rxl, iy, rxr - rxl, 1);
		    if (code < 0)
			goto xit;
		}
		if (++iy == iy1)
		    break;
		STEP_LINE(rxl, l);
		STEP_LINE(rxr, r);
		step_gradient(&lg, num_components);
		step_gradient(&rg, num_components);
#	    else
		register int ixl, ixr;

		STEP_LINE(ixl, l);
		STEP_LINE(ixr, r);
		SET_MINIMAL_WIDTH(ixl, ixr, l, r);
		if (ixl != rxl || ixr != rxr) {
		    CONNECT_RECTANGLES(ixl, ixr, rxl, rxr, iy, ry, rxr, ixl, FILL_TRAP_RECT);
		    CONNECT_RECTANGLES(ixl, ixr, rxl, rxr, iy, ry, ixr, rxl, FILL_TRAP_RECT);
		    VD_RECT_SWAPPED(rxl, ry, rxr, iy);
		    code = FILL_TRAP_RECT(rxl, ry, rxr - rxl, iy - ry);
		    if (code < 0)
			goto xit;
		    rxl = ixl, rxr = ixr, ry = iy;
		}
#	    endif
	}
#	if !LINEAR_COLOR
	    VD_RECT_SWAPPED(rxl, ry, rxr, iy);
	    code = FILL_TRAP_RECT(rxl, ry, rxr - rxl, iy - ry);
#	else
	    code = 0;
#	endif
#undef STEP_LINE
#undef SET_MINIMAL_WIDTH
#undef CONNECT_RECTANGLES
#undef FILL_TRAP_RECT
#undef FILL_TRAP_RECT_DIRECT
#undef FILL_TRAP_RECT_INRECT
#undef YMULT_QUO
#undef VD_RECT_SWAPPED
xit:	if (code < 0 && FILL_DIRECT)
	    return_error(code);
	return_if_interrupt(dev->memory);
	return code;
    }
}

#undef GX_FILL_TRAPEZOID
#undef CONTIGUOUS_FILL
#undef SWAP_AXES
#undef FLAGS_TYPE
