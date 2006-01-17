/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxpcopy.c,v 1.26 2005/08/30 06:38:44 igor Exp $ */
/* Path copying and flattening */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gconfigv.h"		/* for USE_FPU */
#include "gxfixed.h"
#include "gxfarith.h"
#include "gxistate.h"		/* for access to line params */
#include "gzpath.h"
#include "vdtrace.h"

/* Forward declarations */
private void adjust_point_to_tangent(segment *, const segment *,
				     const gs_fixed_point *);
/* Copy a path, optionally flattening or monotonizing it. */
/* If the copy fails, free the new path. */
int
gx_path_copy_reducing(const gx_path *ppath_old, gx_path *ppath,
		      fixed fixed_flatness, const gs_imager_state *pis,
		      gx_path_copy_options options)
{
    const segment *pseg;
    fixed flat = fixed_flatness;
    gs_fixed_point expansion;
    /*
     * Since we're going to be adding to the path, unshare it
     * before we start.
     */
    int code = gx_path_unshare(ppath);

    if (code < 0)
	return code;
#ifdef DEBUG
    if (gs_debug_c('P'))
	gx_dump_path(ppath_old, "before reducing");
#endif
    if (options & pco_for_stroke) {
	/* Precompute the maximum expansion of the bounding box. */
	double width = pis->line_params.half_width;

	expansion.x =
	    float2fixed((fabs(pis->ctm.xx) + fabs(pis->ctm.yx)) * width) * 2;
	expansion.y =
	    float2fixed((fabs(pis->ctm.xy) + fabs(pis->ctm.yy)) * width) * 2;
    }
    vd_setcolor(RGB(255,255,0));
    pseg = (const segment *)(ppath_old->first_subpath);
    while (pseg) {
	switch (pseg->type) {
	    case s_start:
		code = gx_path_add_point(ppath,
					 pseg->pt.x, pseg->pt.y);
		vd_moveto(pseg->pt.x, pseg->pt.y);
		break;
	    case s_curve:
		{
		    const curve_segment *pc = (const curve_segment *)pseg;

		    if (fixed_flatness == max_fixed) {	/* don't flatten */
			if (options & pco_monotonize)
			    code = gx_curve_monotonize(ppath, pc);
			else
			    code = gx_path_add_curve_notes(ppath,
				     pc->p1.x, pc->p1.y, pc->p2.x, pc->p2.y,
					   pc->pt.x, pc->pt.y, pseg->notes);
		    } else {
			fixed x0 = ppath->position.x;
			fixed y0 = ppath->position.y;
			segment_notes notes = pseg->notes;
			curve_segment cseg;
			int k;

			if (options & pco_for_stroke) {
			    /*
			     * When flattening for stroking, the flatness
			     * must apply to the outside of the resulting
			     * stroked region.  We approximate this by
			     * dividing the flatness by the ratio of the
			     * expanded bounding box to the original
			     * bounding box.  This is crude, but pretty
			     * simple to calculate, and produces reasonably
			     * good results.
			     */
			    fixed min01, max01, min23, max23;
			    fixed ex, ey, flat_x, flat_y;

#define SET_EXTENT(r, c0, c1, c2, c3)\
    BEGIN\
	if (c0 < c1) min01 = c0, max01 = c1;\
	else         min01 = c1, max01 = c0;\
	if (c2 < c3) min23 = c2, max23 = c3;\
	else         min23 = c3, max23 = c2;\
	r = max(max01, max23) - min(min01, min23);\
    END
			    SET_EXTENT(ex, x0, pc->p1.x, pc->p2.x, pc->pt.x);
			    SET_EXTENT(ey, y0, pc->p1.y, pc->p2.y, pc->pt.y);
#undef SET_EXTENT
			    /*
			     * We check for the degenerate case specially
			     * to avoid a division by zero.
			     */
			    if (ex == 0 || ey == 0)
				k = 0;
			    else {
				flat_x =
				    fixed_mult_quo(fixed_flatness, ex,
						   ex + expansion.x);
				flat_y =
				    fixed_mult_quo(fixed_flatness, ey,
						   ey + expansion.y);
				flat = min(flat_x, flat_y);
				k = gx_curve_log2_samples(x0, y0, pc, flat);
			    }
			} else
			    k = gx_curve_log2_samples(x0, y0, pc, flat);
			if (options & pco_accurate) {
			    segment *start;
			    segment *end;

			    /*
			     * Add an extra line, which will become
			     * the tangent segment.
			     */
			    code = gx_path_add_line_notes(ppath, x0, y0,
							  notes);
			    if (code < 0)
				break;
			    vd_lineto(x0, y0);
			    start = ppath->current_subpath->last;
			    notes |= sn_not_first;
			    cseg = *pc;
			    code = gx_subdivide_curve(ppath, k, &cseg, notes);
			    if (code < 0)
				break;
			    /*
			     * Adjust the first and last segments so that
			     * they line up with the tangents.
			     */
			    end = ppath->current_subpath->last;
			    vd_lineto(ppath->position.x, ppath->position.y);
			    if ((code = gx_path_add_line_notes(ppath,
							  ppath->position.x,
							  ppath->position.y,
					    pseg->notes | sn_not_first)) < 0)
				break;
			    if (start->next->pt.x != pc->p1.x || start->next->pt.y != pc->p1.y)
				adjust_point_to_tangent(start, start->next, &pc->p1);
			    else if (start->next->pt.x != pc->p2.x || start->next->pt.y != pc->p2.y)
				adjust_point_to_tangent(start, start->next, &pc->p2);
			    else
				adjust_point_to_tangent(start, start->next, &end->prev->pt);
			    if (end->prev->pt.x != pc->p2.x || end->prev->pt.y != pc->p2.y)
				adjust_point_to_tangent(end, end->prev, &pc->p2);
			    else if (end->prev->pt.x != pc->p1.x || end->prev->pt.y != pc->p1.y)
				adjust_point_to_tangent(end, end->prev, &pc->p1);
			    else
				adjust_point_to_tangent(end, end->prev, &start->pt);
			} else {
			    cseg = *pc;
			    code = gx_subdivide_curve(ppath, k, &cseg, notes);
			}
		    }
		    break;
		}
	    case s_line:
		code = gx_path_add_line_notes(ppath,
				       pseg->pt.x, pseg->pt.y, pseg->notes);
		vd_lineto(pseg->pt.x, pseg->pt.y);
		break;
	    case s_line_close:
		code = gx_path_close_subpath(ppath);
		vd_closepath;
		break;
	    default:		/* can't happen */
		code = gs_note_error(gs_error_unregistered);
	}
	if (code < 0) {
	    gx_path_new(ppath);
	    return code;
	}
	pseg = pseg->next;
    }
    if (path_last_is_moveto(ppath_old))
	gx_path_add_point(ppath, ppath_old->position.x,
			  ppath_old->position.y);
    if (ppath_old->bbox_set) {
	if (ppath->bbox_set) {
	    ppath->bbox.p.x = min(ppath_old->bbox.p.x, ppath->bbox.p.x);
	    ppath->bbox.p.y = min(ppath_old->bbox.p.y, ppath->bbox.p.y);
	    ppath->bbox.q.x = max(ppath_old->bbox.q.x, ppath->bbox.q.x);
	    ppath->bbox.q.y = max(ppath_old->bbox.q.y, ppath->bbox.q.y);
	} else {
	    ppath->bbox_set = true;
	    ppath->bbox = ppath_old->bbox;
	}
    }
#ifdef DEBUG
    if (gs_debug_c('P'))
	gx_dump_path(ppath, "after reducing");
#endif
    return 0;
}

/*
 * Adjust one end of a line (the first or last line of a flattened curve)
 * so it falls on the curve tangent.  The closest point on the line from
 * (0,0) to (C,D) to a point (U,V) -- i.e., the point on the line at which
 * a perpendicular line from the point intersects it -- is given by
 *      T = (C*U + D*V) / (C^2 + D^2)
 *      (X,Y) = (C*T,D*T)
 * However, any smaller value of T will also work: the one we actually
 * use is 0.25 * the value we just derived.  We must check that
 * numerical instabilities don't lead to a negative value of T.
 */
private void
adjust_point_to_tangent(segment * pseg, const segment * next,
			const gs_fixed_point * p1)
{
    const fixed x0 = pseg->pt.x, y0 = pseg->pt.y;
    const fixed fC = p1->x - x0, fD = p1->y - y0;

    /*
     * By far the commonest case is that the end of the curve is
     * horizontal or vertical.  Check for this specially, because
     * we can handle it with far less work (and no floating point).
     */
    if (fC == 0) {
	/* Vertical tangent. */
	const fixed DT = arith_rshift(next->pt.y - y0, 2);

	if (fD == 0)
	    return;		/* anomalous case */
	if_debug1('2', "[2]adjusting vertical: DT = %g\n",
		  fixed2float(DT));
	if ((DT ^ fD) > 0)
	    pseg->pt.y = DT + y0;
    } else if (fD == 0) {
	/* Horizontal tangent. */
	const fixed CT = arith_rshift(next->pt.x - x0, 2);

	if_debug1('2', "[2]adjusting horizontal: CT = %g\n",
		  fixed2float(CT));
	if ((CT ^ fC) > 0)
	    pseg->pt.x = CT + x0;
    } else {
	/* General case. */
	const double C = fC, D = fD;
	double T = (C * (next->pt.x - x0) + D * (next->pt.y - y0)) /
	(C * C + D * D);

	if_debug3('2', "[2]adjusting: C = %g, D = %g, T = %g\n",
		  C, D, T);
	if (T > 0) {
	    if (T > 1) {
		/* Don't go outside the curve bounding box. */
		T = 1;
	    }
	    pseg->pt.x = arith_rshift((fixed) (C * T), 2) + x0;
	    pseg->pt.y = arith_rshift((fixed) (D * T), 2) + y0;
	}
    }
}

/* ---------------- Monotonic curves ---------------- */

/* Test whether a path is free of non-monotonic curves. */
bool
gx_path__check_curves(const gx_path * ppath, gx_path_copy_options options, fixed fixed_flat)
{
    const segment *pseg = (const segment *)(ppath->first_subpath);
    gs_fixed_point pt0;

    while (pseg) {
	switch (pseg->type) {
	    case s_start:
		{
		    const subpath *psub = (const subpath *)pseg;

		    /* Skip subpaths without curves. */
		    if (!psub->curve_count)
			pseg = psub->last;
		}
		break;
	    case s_curve:
		{
		    const curve_segment *pc = (const curve_segment *)pseg;

		    if (options & pco_monotonize) {
			double t[2];
			int nz = gx_curve_monotonic_points(pt0.y,
					       pc->p1.y, pc->p2.y, pc->pt.y, t);

			if (nz != 0)
			    return false;
			nz = gx_curve_monotonic_points(pt0.x,
					       pc->p1.x, pc->p2.x, pc->pt.x, t);
			if (nz != 0)
			    return false;
		    }
		    if (options & pco_small_curves) {
			fixed ax, bx, cx, ay, by, cy; 
			int k = gx_curve_log2_samples(pt0.x, pt0.y, pc, fixed_flat);

			if(!curve_coeffs_ranged(pt0.x, pc->p1.x, pc->p2.x, pc->pt.x,
				pt0.y, pc->p1.y, pc->p2.y, pc->pt.y,
				&ax, &bx, &cx, &ay, &by, &cy, k))
			    return false;
		    }
		}
		break;
	    default:
		;
	}
	pt0 = pseg->pt;
	pseg = pseg->next;
    }
    return true;
}

/* Monotonize a curve, by splitting it if necessary. */
/* In the worst case, this could split the curve into 9 pieces. */
int
gx_curve_monotonize(gx_path * ppath, const curve_segment * pc)
{
    fixed x0 = ppath->position.x, y0 = ppath->position.y;
    segment_notes notes = pc->notes;
    double t[4], tt = 1, tp;
    int c[4];
    int n0, n1, n, i, j, k = 0;
    fixed ax, bx, cx, ay, by, cy, v01, v12;
    fixed px, py, qx, qy, rx, ry, sx, sy;
    const double delta = 0.0000001;

    /* Roots of the derivative : */
    n0 = gx_curve_monotonic_points(x0, pc->p1.x, pc->p2.x, pc->pt.x, t);
    n1 = gx_curve_monotonic_points(y0, pc->p1.y, pc->p2.y, pc->pt.y, t + n0);
    n = n0 + n1;
    if (n == 0)
	return gx_path_add_curve_notes(ppath, pc->p1.x, pc->p1.y,
		pc->p2.x, pc->p2.y, pc->pt.x, pc->pt.y, notes);
    if (n0 > 0)
	c[0] = 1;
    if (n0 > 1)
	c[1] = 1;
    if (n1 > 0)
	c[n0] = 2;
    if (n1 > 1)
	c[n0 + 1] = 2;
    /* Order roots : */
    for (i = 0; i < n; i++)
	for (j = i + 1; j < n; j++)
	    if (t[i] > t[j]) {
		int w;
		double v = t[i]; t[i] = t[j]; t[j] = v;
		w = c[i]; c[i] = c[j]; c[j] = w;
	    }
    /* Drop roots near zero : */
    for (k = 0; k < n; k++)
	if (t[k] >= delta)
	    break;
    /* Merge close roots, and drop roots at 1 : */
    if (t[n - 1] > 1 - delta)
	n--;
    for (i = k + 1, j = k; i < n && t[k] < 1 - delta; i++)
	if (any_abs(t[i] - t[j]) < delta) {
	    t[j] = (t[j] + t[i]) / 2; /* Unlikely 3 roots are close. */
	    c[j] |= c[i];
	} else {
	    j++;
	    t[j] = t[i];
	    c[j] = c[i];
	}
    n = j + 1;
    /* Do split : */
    curve_points_to_coefficients(x0, pc->p1.x, pc->p2.x, pc->pt.x, ax, bx, cx, v01, v12);
    curve_points_to_coefficients(y0, pc->p1.y, pc->p2.y, pc->pt.y, ay, by, cy, v01, v12);
    ax *= 3, bx *= 2; /* Coefficients of the derivative. */
    ay *= 3, by *= 2;
    px = x0;
    py = y0;
    qx = (fixed)((pc->p1.x - px) * t[0] + 0.5);
    qy = (fixed)((pc->p1.y - py) * t[0] + 0.5);
    tp = 0;
    for (i = k; i < n; i++) {
	double ti = t[i];
	double t2 = ti * ti, t3 = t2 * ti;
	double omt = 1 - ti, omt2 = omt * omt, omt3 = omt2 * omt;
	double x = x0 * omt3 + 3 * pc->p1.x * omt2 * ti + 3 * pc->p2.x * omt * t2 + pc->pt.x * t3;
	double y = y0 * omt3 + 3 * pc->p1.y * omt2 * ti + 3 * pc->p2.y * omt * t2 + pc->pt.y * t3;
	double ddx = (c[i] & 1 ? 0 : ax * t2 + bx * ti + cx); /* Suppress noize. */
	double ddy = (c[i] & 2 ? 0 : ay * t2 + by * ti + cy);
	fixed dx = (fixed)(ddx + 0.5);
	fixed dy = (fixed)(ddy + 0.5);
	int code;

	tt = (i + 1 < n ? t[i + 1] : 1) - ti;
	rx = (fixed)(dx * (t[i] - tp) / 3 + 0.5);
	ry = (fixed)(dy * (t[i] - tp) / 3 + 0.5);
	sx = (fixed)(x + 0.5);
	sy = (fixed)(y + 0.5);
	/* Suppress the derivative sign noize near a beak : */
	if ((double)(sx - px) * qx + (double)(sy - py) * qy < 0)
	    qx = -qx, qy = -qy;
	if ((double)(sx - px) * rx + (double)(sy - py) * ry < 0)
	    rx = -rx, ry = -qy;
	/* Do add : */
	code = gx_path_add_curve_notes(ppath, px + qx, py + qy, sx - rx, sy - ry, sx, sy, notes);
	if (code < 0)
	    return code;
	notes |= sn_not_first;
	px = sx;
	py = sy;
	qx = (fixed)(dx * tt / 3 + 0.5);
	qy = (fixed)(dy * tt / 3 + 0.5);
	tp = t[i];
    }
    sx = pc->pt.x;
    sy = pc->pt.y;
    rx = (fixed)((pc->pt.x - pc->p2.x) * tt + 0.5);
    ry = (fixed)((pc->pt.y - pc->p2.y) * tt + 0.5);
    /* Suppress the derivative sign noize near peaks : */
    if ((double)(sx - px) * qx + (double)(sy - py) * qy < 0)
	qx = -qx, qy = -qy;
    if ((double)(sx - px) * rx + (double)(sy - py) * ry < 0)
	rx = -rx, ry = -qy;
    return gx_path_add_curve_notes(ppath, px + qx, py + qy, sx - rx, sy - ry, sx, sy, notes);
}

/*
 * Split a curve if necessary into pieces that are monotonic in X or Y as a
 * function of the curve parameter t.  This allows us to rasterize curves
 * directly without pre-flattening.  This takes a fair amount of analysis....
 * Store the values of t of the split points in pst[0] and pst[1].  Return
 * the number of split points (0, 1, or 2).
 */
int
gx_curve_monotonic_points(fixed v0, fixed v1, fixed v2, fixed v3,
			  double pst[2])
{
    /*
       Let
       v(t) = a*t^3 + b*t^2 + c*t + d, 0 <= t <= 1.
       Then
       dv(t) = 3*a*t^2 + 2*b*t + c.
       v(t) has a local minimum or maximum (or inflection point)
       precisely where dv(t) = 0.  Now the roots of dv(t) = 0 (i.e.,
       the zeros of dv(t)) are at
       t =  ( -2*b +/- sqrt(4*b^2 - 12*a*c) ) / 6*a
       =    ( -b +/- sqrt(b^2 - 3*a*c) ) / 3*a
       (Note that real roots exist iff b^2 >= 3*a*c.)
       We want to know if these lie in the range (0..1).
       (The endpoints don't count.)  Call such a root a "valid zero."
       Since computing the roots is expensive, we would like to have
       some cheap tests to filter out cases where they don't exist
       (i.e., where the curve is already monotonic).
     */
    fixed v01, v12, a, b, c, b2, a3;
    fixed dv_end, b2abs, a3abs;

    curve_points_to_coefficients(v0, v1, v2, v3, a, b, c, v01, v12);
    b2 = b << 1;
    a3 = (a << 1) + a;
    /*
       If a = 0, the only possible zero is t = -c / 2*b.
       This zero is valid iff sign(c) != sign(b) and 0 < |c| < 2*|b|.
     */
    if (a == 0) {
	if ((b ^ c) < 0 && any_abs(c) < any_abs(b2) && c != 0) {
	    *pst = (double)(-c) / b2;
	    return 1;
	} else
	    return 0;
    }
    /*
       Iff a curve is horizontal at t = 0, c = 0.  In this case,
       there can be at most one other zero, at -2*b / 3*a.
       This zero is valid iff sign(a) != sign(b) and 0 < 2*|b| < 3*|a|.
     */
    if (c == 0) {
	if ((a ^ b) < 0 && any_abs(b2) < any_abs(a3) && b != 0) {
	    *pst = (double)(-b2) / a3;
	    return 1;
	} else
	    return 0;
    }
    /*
       Similarly, iff a curve is horizontal at t = 1, 3*a + 2*b + c = 0.
       In this case, there can be at most one other zero,
       at -1 - 2*b / 3*a, iff sign(a) != sign(b) and 1 < -2*b / 3*a < 2,
       i.e., 3*|a| < 2*|b| < 6*|a|.
     */
    else if ((dv_end = a3 + b2 + c) == 0) {
	if ((a ^ b) < 0 &&
	    (b2abs = any_abs(b2)) > (a3abs = any_abs(a3)) &&
	    b2abs < a3abs << 1
	    ) {
	    *pst = (double)(-b2 - a3) / a3;
	    return 1;
	} else
	    return 0;
    }
    /*
       If sign(dv_end) != sign(c), at least one valid zero exists,
       since dv(0) and dv(1) have opposite signs and hence
       dv(t) must be zero somewhere in the interval [0..1].
     */
    else if ((dv_end ^ c) < 0);
    /*
       If sign(a) = sign(b), no valid zero exists,
       since dv is monotonic on [0..1] and has the same sign
       at both endpoints.
     */
    else if ((a ^ b) >= 0)
	return 0;
    /*
       Otherwise, dv(t) may be non-monotonic on [0..1]; it has valid zeros
       iff its sign anywhere in this interval is different from its sign
       at the endpoints, which occurs iff it has an extremum in this
       interval and the extremum is of the opposite sign from c.
       To find this out, we look for the local extremum of dv(t)
       by observing
       d2v(t) = 6*a*t + 2*b
       which has a zero only at
       t1 = -b / 3*a
       Now if t1 <= 0 or t1 >= 1, no valid zero exists.
       Note that we just determined that sign(a) != sign(b), so we know t1 > 0.
     */
    else if (any_abs(b) >= any_abs(a3))
	return 0;
    /*
       Otherwise, we just go ahead with the computation of the roots,
       and test them for being in the correct range.  Note that a valid
       zero is an inflection point of v(t) iff d2v(t) = 0; we don't
       bother to check for this case, since it's rare.
     */
    {
	double nbf = (double)(-b);
	double a3f = (double)a3;
	double radicand = nbf * nbf - a3f * c;

	if (radicand < 0) {
	    if_debug1('2', "[2]negative radicand = %g\n", radicand);
	    return 0;
	} {
	    double root = sqrt(radicand);
	    int nzeros = 0;
	    double z = (nbf - root) / a3f;

	    /*
	     * We need to return the zeros in the correct order.
	     * We know that root is non-negative, but a3f may be either
	     * positive or negative, so we need to check the ordering
	     * explicitly.
	     */
	    if_debug2('2', "[2]zeros at %g, %g\n", z, (nbf + root) / a3f);
	    if (z > 0 && z < 1)
		*pst = z, nzeros = 1;
	    if (root != 0) {
		z = (nbf + root) / a3f;
		if (z > 0 && z < 1) {
		    if (nzeros && a3f < 0)	/* order is reversed */
			pst[1] = *pst, *pst = z;
		    else
			pst[nzeros] = z;
		    nzeros++;
		}
	    }
	    return nzeros;
	}
    }
}

/* ---------------- Path optimization for the filling algorithm. ---------------- */

private bool
find_contacting_segments(const subpath *sp0, segment *sp0last, 
			 const subpath *sp1, segment *sp1last, 
			 segment **sc0, segment **sc1)
{
    segment *s0, *s1;
    const segment *s0s, *s1s;
    int count0, count1, search_limit = 50;
    int min_length = fixed_1 * 1;

    /* This is a simplified algorithm, which only checks for quazi-colinear vertical lines.
       "Quazi-vertical" means dx <= 1 && dy >= min_length . */
    /* To avoid a big unuseful expence of the processor time,
       we search the first subpath from the end
       (assuming that it was recently merged near the end), 
       and restrict the search with search_limit segments 
       against a quadratic scanning of two long subpaths. 
       Thus algorithm is not necessary finds anything contacting.
       Instead it either quickly finds something, or maybe not. */
    for (s0 = sp0last, count0 = 0; count0 < search_limit && s0 != (segment *)sp0; s0 = s0->prev, count0++) {
	s0s = s0->prev;
	if (s0->type == s_line && (s0s->pt.x == s0->pt.x ||
	    (any_abs(s0s->pt.x - s0->pt.x) == 1 && any_abs(s0s->pt.y - s0->pt.y) > min_length))) {
	    for (s1 = sp1last, count1 = 0; count1 < search_limit && s1 != (segment *)sp1; s1 = s1->prev, count1++) {
		s1s = s1->prev;
		if (s1->type == s_line && (s1s->pt.x == s1->pt.x || 
		    (any_abs(s1s->pt.x - s1->pt.x) == 1 && any_abs(s1s->pt.y - s1->pt.y) > min_length))) {
		    if (s0s->pt.x == s1s->pt.x || s0->pt.x == s1->pt.x || s0->pt.x == s1s->pt.x || s0s->pt.x == s1->pt.x) {
			if (s0s->pt.y < s0->pt.y && s1s->pt.y > s1->pt.y) {
			    fixed y0 = max(s0s->pt.y, s1->pt.y);
			    fixed y1 = min(s0->pt.y, s1s->pt.y);

			    if (y0 <= y1) {
				*sc0 = s0;
				*sc1 = s1;
				return true;
			    }
			}
			if (s0s->pt.y > s0->pt.y && s1s->pt.y < s1->pt.y) {
			    fixed y0 = max(s0->pt.y, s1s->pt.y);
			    fixed y1 = min(s0s->pt.y, s1->pt.y);

			    if (y0 <= y1) {
				*sc0 = s0;
				*sc1 = s1;
				return true;
			    }
			}
		    }
		}
	    }
	}
    }
    return false;
}

int
gx_path_merge_contacting_contours(gx_path *ppath)
{
    /* Now this is a simplified algorithm,
       which merge only contours by a common quazi-vertical line. */
    int window = 5/* max spot holes */ * 6/* segments per subpath */;
    subpath *sp0 = ppath->segments->contents.subpath_first;

    for (; sp0 != NULL; sp0 = (subpath *)sp0->last->next) {
	segment *sp0last = sp0->last;
	subpath *sp1 = (subpath *)sp0last->next, *spnext;
	subpath *sp1p = sp0;
	int count;
	
	for (count = 0; sp1 != NULL && count < window; sp1 = spnext, count++) {
	    segment *sp1last = sp1->last;
	    segment *sc0, *sc1;
		
	    spnext = (subpath *)sp1last->next;
	    if (find_contacting_segments(sp0, sp0last, sp1, sp1last, &sc0, &sc1)) {
		/* Detach the subpath 1 from the path: */
		sp1->prev->next = sp1last->next;
		if (sp1last->next != NULL)
		    sp1last->next->prev = sp1->prev;
		sp1->prev = 0;
		sp1last->next = 0;
		/* Change 'closepath' of the subpath 1 to a line (maybe degenerate) : */
		if (sp1last->type == s_line_close)
		    sp1last->type = s_line;
		/* Rotate the subpath 1 to sc1 : */
		{   segment *old_first = sp1->next;

		    /* Detach s_start and make a loop : */
		    sp1last->next = old_first;
		    old_first->prev = sp1last;
		    /* Unlink before sc1 : */
		    sp1last = sc1->prev;
		    sc1->prev->next = 0;
		    sc1->prev = 0; /* Safety. */
		    /* sp1 is not longer in use. Free it : */
		    if (ppath->segments->contents.subpath_current == sp1) {
			ppath->segments->contents.subpath_current = sp1p;
		    }
		    gs_free_object(ppath->memory, sp1, "gx_path_merge_contacting_contours");
		    sp1 = 0; /* Safety. */
		}
		/* Insert the subpath 1 into the subpath 0 before sc0 :*/
		sc0->prev->next = sc1;
		sc1->prev = sc0->prev;
		sp1last->next = sc0;
		sc0->prev = sp1last;
		/* Remove degenearte "bridge" segments : (fixme: Not done due to low importance). */
		/* Edit the subpath count : */
		ppath->subpath_count--;
	    } else
		sp1p = sp1;
	}
    }
    return 0;
}

