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

/* $Id: gxfillts.h,v 1.5 2004/10/26 04:08:59 giles Exp $ */
/* Configurable algorithm for filling a slanted trapezoid. */

/*
 * Since we need several statically defined variants of this agorithm,
 * we store it in .h file and include it several times into gxfill.c .
 * Configuration macros (template arguments) are :
 * 
 *  FILL_DIRECT - See LOOP_FILL_RECTANGLE_DIRECT.
 *  TEMPLATE_slant_into_trapezoids - the name of the procedure to generate.
*/


private inline int
TEMPLATE_slant_into_trapezoids (const line_list *ll, 
	const active_line *flp, const active_line *alp, fixed y, fixed y1)
{
    /*
     * We want to get the effect of filling an area whose
     * outline is formed by dragging a square of side adj2
     * along the border of the trapezoid.  This is *not*
     * equivalent to simply expanding the corners by
     * adjust: There are 3 cases needing different
     * algorithms, plus rectangles as a fast special case.
     */
    const fill_options * const fo = ll->fo;
    int xli = fixed2int_var_pixround(flp->x_next - fo->adjust_left);
    gs_fixed_edge le, re;
    int code = 0;
    /*
     * Define a faster test for
     *	fixed2int_pixround(y - below) != fixed2int_pixround(y + above)
     * where we know
     *	0 <= below <= _fixed_pixround_v,
     *	0 <= above <= min(fixed_half, fixed_1 - below).
     * Subtracting out the integer parts, this is equivalent to
     *	fixed2int_pixround(fixed_fraction(y) - below) !=
     *	  fixed2int_pixround(fixed_fraction(y) + above)
     * or to
     *	fixed2int(fixed_fraction(y) + _fixed_pixround_v - below) !=
     *	  fixed2int(fixed_fraction(y) + _fixed_pixround_v + above)
     * Letting A = _fixed_pixround_v - below and B = _fixed_pixround_v + above,
     * we can rewrite this as
     *	fixed2int(fixed_fraction(y) + A) != fixed2int(fixed_fraction(y) + B)
     * Because of the range constraints given above, this is true precisely when
     *	fixed_fraction(y) + A < fixed_1 && fixed_fraction(y) + B >= fixed_1
     * or equivalently
     *	fixed_fraction(y + B) < B - A.
     * i.e.
     *	fixed_fraction(y + _fixed_pixround_v + above) < below + above
     */
    fixed y_span_delta = _fixed_pixround_v + fo->adjust_above;
    fixed y_span_limit = fo->adjust_below + fo->adjust_above;
    le.start.x = flp->start.x - fo->adjust_left;
    le.end.x = flp->end.x - fo->adjust_left;
    re.start.x = alp->start.x + fo->adjust_right;
    re.end.x = alp->end.x + fo->adjust_right;

#define ADJUSTED_Y_SPANS_PIXEL(y)\
  (fixed_fraction((y) + y_span_delta) < y_span_limit)

    if (le.end.x <= le.start.x) {
	if (re.end.x >= re.start.x) {	/* Top wider than bottom. */
	    le.start.y = flp->start.y - fo->adjust_below;
	    le.end.y = flp->end.y - fo->adjust_below;
	    re.start.y = alp->start.y - fo->adjust_below;
	    re.end.y = alp->end.y - fo->adjust_below;
	    code = loop_fill_trap_np(ll, &le, &re, y - fo->adjust_below, y1 - fo->adjust_below);
	    if (ADJUSTED_Y_SPANS_PIXEL(y1)) {
		if (code < 0)
		    return code;
		INCR(afill);
		code = LOOP_FILL_RECTANGLE_DIRECT(fo, 
		 xli, fixed2int_pixround(y1 - fo->adjust_below),
		     fixed2int_var_pixround(alp->x_next + fo->adjust_right) - xli, 1);
		vd_rect(flp->x_next - fo->adjust_left, y1 - fo->adjust_below, 
			alp->x_next + fo->adjust_right, y1, 1, VD_TRAP_COLOR);
	    }
	} else {	/* Slanted trapezoid. */
	    code = fill_slant_adjust(ll, flp, alp, y, y1);
	}
    } else {
	if (re.end.x <= re.start.x) {	/* Bottom wider than top. */
	    if (ADJUSTED_Y_SPANS_PIXEL(y)) {
		INCR(afill);
		xli = fixed2int_var_pixround(flp->x_current - fo->adjust_left);
		code = LOOP_FILL_RECTANGLE_DIRECT(fo, 
		  xli, fixed2int_pixround(y - fo->adjust_below),
		     fixed2int_var_pixround(alp->x_current + fo->adjust_right) - xli, 1);
		vd_rect(flp->x_current - fo->adjust_left, y - fo->adjust_below, 
			alp->x_current + fo->adjust_right, y, 1, VD_TRAP_COLOR);
		if (code < 0)
		    return code;
	    }
	    le.start.y = flp->start.y + fo->adjust_above;
	    le.end.y = flp->end.y + fo->adjust_above;
	    re.start.y = alp->start.y + fo->adjust_above;
	    re.end.y = alp->end.y + fo->adjust_above;
	    code = loop_fill_trap_np(ll, &le, &re, y + fo->adjust_above, y1 + fo->adjust_above);
	} else {	/* Slanted trapezoid. */
	    code = fill_slant_adjust(ll, flp, alp, y, y1);
	}
    }
    return code;
}

