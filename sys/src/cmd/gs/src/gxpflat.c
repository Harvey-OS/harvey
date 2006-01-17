/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxpflat.c,v 1.45 2005/08/10 19:36:11 igor Exp $ */
/* Path flattening algorithms */
#include "string_.h"
#include "gx.h"
#include "gxarith.h"
#include "gxfixed.h"
#include "gzpath.h"
#include "memory_.h"
#include "vdtrace.h"
#include <assert.h>

/* ---------------- Curve flattening ---------------- */

/*
 * To calculate how many points to sample along a path in order to
 * approximate it to the desired degree of flatness, we define
 *      dist((x,y)) = abs(x) + abs(y);
 * then the number of points we need is
 *      N = 1 + sqrt(3/4 * D / flatness),
 * where
 *      D = max(dist(p0 - 2*p1 + p2), dist(p1 - 2*p2 + p3)).
 * Since we are going to use a power of 2 for the number of intervals,
 * we can avoid the square root by letting
 *      N = 1 + 2^(ceiling(log2(3/4 * D / flatness) / 2)).
 * (Reference: DEC Paris Research Laboratory report #1, May 1989.)
 *
 * We treat two cases specially.  First, if the curve is very
 * short, we halve the flatness, to avoid turning short shallow curves
 * into short straight lines.  Second, if the curve forms part of a
 * character (indicated by flatness = 0), we let
 *      N = 1 + 2 * max(abs(x3-x0), abs(y3-y0)).
 * This is probably too conservative, but it produces good results.
 */
int
gx_curve_log2_samples(fixed x0, fixed y0, const curve_segment * pc,
		      fixed fixed_flat)
{
    fixed
	x03 = pc->pt.x - x0,
	y03 = pc->pt.y - y0;
    int k;

    if (x03 < 0)
	x03 = -x03;
    if (y03 < 0)
	y03 = -y03;
    if ((x03 | y03) < int2fixed(16))
	fixed_flat >>= 1;
    if (fixed_flat == 0) {	/* Use the conservative method. */
	fixed m = max(x03, y03);

	for (k = 1; m > fixed_1;)
	    k++, m >>= 1;
    } else {
	const fixed
	      x12 = pc->p1.x - pc->p2.x, y12 = pc->p1.y - pc->p2.y, 
	      dx0 = x0 - pc->p1.x - x12, dy0 = y0 - pc->p1.y - y12,
	      dx1 = x12 - pc->p2.x + pc->pt.x, dy1 = y12 - pc->p2.y + pc->pt.y, 
	      adx0 = any_abs(dx0), ady0 = any_abs(dy0), 
	      adx1 = any_abs(dx1), ady1 = any_abs(dy1);
	fixed
	    d = max(adx0, adx1) + max(ady0, ady1);
	/*
	 * The following statement is split up to work around a
	 * bug in the gcc 2.7.2 optimizer on H-P RISC systems.
	 */
	uint qtmp = d - (d >> 2) /* 3/4 * D */ +fixed_flat - 1;
	uint q = qtmp / fixed_flat;

	if_debug6('2', "[2]d01=%g,%g d12=%g,%g d23=%g,%g\n",
		  fixed2float(pc->p1.x - x0), fixed2float(pc->p1.y - y0),
		  fixed2float(-x12), fixed2float(-y12),
		  fixed2float(pc->pt.x - pc->p2.x), fixed2float(pc->pt.y - pc->p2.y));
	if_debug2('2', "     D=%f, flat=%f,",
		  fixed2float(d), fixed2float(fixed_flat));
	/* Now we want to set k = ceiling(log2(q) / 2). */
	for (k = 0; q > 1;)
	    k++, q = (q + 3) >> 2;
	if_debug1('2', " k=%d\n", k);
    }
    return k;
}

/*
 * Split a curve segment into two pieces at the (parametric) midpoint.
 * Algorithm is from "The Beta2-split: A special case of the Beta-spline
 * Curve and Surface Representation," B. A. Barsky and A. D. DeRose, IEEE,
 * 1985, courtesy of Crispin Goswell.
 */
private void
split_curve_midpoint(fixed x0, fixed y0, const curve_segment * pc,
		     curve_segment * pc1, curve_segment * pc2)
{				/*
				 * We have to define midpoint carefully to avoid overflow.
				 * (If it overflows, something really pathological is going
				 * on, but we could get infinite recursion that way....)
				 */
#define midpoint(a,b)\
  (arith_rshift_1(a) + arith_rshift_1(b) + (((a) | (b)) & 1))
    fixed x12 = midpoint(pc->p1.x, pc->p2.x);
    fixed y12 = midpoint(pc->p1.y, pc->p2.y);

    /*
     * pc1 or pc2 may be the same as pc, so we must be a little careful
     * about the order in which we store the results.
     */
    pc1->p1.x = midpoint(x0, pc->p1.x);
    pc1->p1.y = midpoint(y0, pc->p1.y);
    pc2->p2.x = midpoint(pc->p2.x, pc->pt.x);
    pc2->p2.y = midpoint(pc->p2.y, pc->pt.y);
    pc1->p2.x = midpoint(pc1->p1.x, x12);
    pc1->p2.y = midpoint(pc1->p1.y, y12);
    pc2->p1.x = midpoint(x12, pc2->p2.x);
    pc2->p1.y = midpoint(y12, pc2->p2.y);
    if (pc2 != pc)
	pc2->pt.x = pc->pt.x,
	    pc2->pt.y = pc->pt.y;
    pc1->pt.x = midpoint(pc1->p2.x, pc2->p1.x);
    pc1->pt.y = midpoint(pc1->p2.y, pc2->p1.y);
#undef midpoint
}

private inline void
print_points(const gs_fixed_point *points, int count)
{
#ifdef DEBUG    
    int i;

    if (!gs_debug_c('3'))
	return;
    for (i = 0; i < count; i++)
	if_debug2('3', "[3]out x=%d y=%d\n", points[i].x, points[i].y);
#endif
}


bool
curve_coeffs_ranged(fixed x0, fixed x1, fixed x2, fixed x3, 
		    fixed y0, fixed y1, fixed y2, fixed y3, 
		    fixed *ax, fixed *bx, fixed *cx, 
		    fixed *ay, fixed *by, fixed *cy, 
		    int k)
{
    fixed x01, x12, y01, y12;

    curve_points_to_coefficients(x0, x1, x2, x3, 
				 *ax, *bx, *cx, x01, x12);
    curve_points_to_coefficients(y0, y1, y2, y3, 
				 *ay, *by, *cy, y01, y12);
#   define max_fast (max_fixed / 6)
#   define min_fast (-max_fast)
#   define in_range(v) (v < max_fast && v > min_fast)
    if (k > k_sample_max ||
	!in_range(*ax) || !in_range(*ay) ||
	!in_range(*bx) || !in_range(*by) ||
	!in_range(*cx) || !in_range(*cy)
	)
	return false;
#undef max_fast
#undef min_fast
#undef in_range
    return true;
}

/*  Initialize the iterator. 
    Momotonic curves with non-zero length are only allowed.
 */
bool
gx_flattened_iterator__init(gx_flattened_iterator *this, 
	    fixed x0, fixed y0, const curve_segment *pc, int k)
{
    /* Note : Immediately after the ininialization it keeps an invalid (zero length) segment. */
    fixed x1, y1, x2, y2;
    const int k2 = k << 1, k3 = k2 + k;
    fixed bx2, by2, ax6, ay6;

    x1 = pc->p1.x;
    y1 = pc->p1.y;
    x2 = pc->p2.x;
    y2 = pc->p2.y;
    this->x0 = this->lx0 = this->lx1 = x0;
    this->y0 = this->ly0 = this->ly1 = y0;
    this->x3 = pc->pt.x;
    this->y3 = pc->pt.y;
    if (!curve_coeffs_ranged(this->x0, x1, x2, this->x3,
			     this->y0, y1, y2, this->y3, 
			     &this->ax, &this->bx, &this->cx, 
			     &this->ay, &this->by, &this->cy, k))
	return false;
    this->curve = true;
    vd_curve(this->x0, this->y0, x1, y1, x2, y2, this->x3, this->y3, 0, RGB(255, 255, 255));
    this->k = k;
#   ifdef DEBUG
	if (gs_debug_c('3')) {
	    dlprintf4("[3]x0=%f y0=%f x1=%f y1=%f\n",
		      fixed2float(this->x0), fixed2float(this->y0),
		      fixed2float(x1), fixed2float(y1));
	    dlprintf5("   x2=%f y2=%f x3=%f y3=%f  k=%d\n",
		      fixed2float(x2), fixed2float(y2),
		      fixed2float(this->x3), fixed2float(this->y3), this->k);
	}
#   endif
    if (k == -1) {
	/* A special hook for gx_subdivide_curve_rec.
	   Only checked the range. 
	   Returning with no initialization. */
	return true;
    }
    this->rmask = (1 << k3) - 1;
    this->i = (1 << k);
    this->rx = this->ry = 0;
    if_debug6('3', "[3]ax=%f bx=%f cx=%f\n   ay=%f by=%f cy=%f\n",
	      fixed2float(this->ax), fixed2float(this->bx), fixed2float(this->cx),
	      fixed2float(this->ay), fixed2float(this->by), fixed2float(this->cy));
    bx2 = this->bx << 1;
    by2 = this->by << 1;
    ax6 = ((this->ax << 1) + this->ax) << 1;
    ay6 = ((this->ay << 1) + this->ay) << 1;
    this->idx = arith_rshift(this->cx, this->k);
    this->idy = arith_rshift(this->cy, this->k);
    this->rdx = ((uint)this->cx << k2) & this->rmask;
    this->rdy = ((uint)this->cy << k2) & this->rmask;
    /* bx/y terms */
    this->id2x = arith_rshift(bx2, k2);
    this->id2y = arith_rshift(by2, k2);
    this->rd2x = ((uint)bx2 << this->k) & this->rmask;
    this->rd2y = ((uint)by2 << this->k) & this->rmask;
#   define adjust_rem(r, q, rmask) if ( r > rmask ) q ++, r &= rmask
    /* We can compute all the remainders as ints, */
    /* because we know they don't exceed M. */
    /* cx/y terms */
    this->idx += arith_rshift_1(this->id2x);
    this->idy += arith_rshift_1(this->id2y);
    this->rdx += ((uint)this->bx << this->k) & this->rmask,
    this->rdy += ((uint)this->by << this->k) & this->rmask;
    adjust_rem(this->rdx, this->idx, this->rmask);
    adjust_rem(this->rdy, this->idy, this->rmask);
    /* ax/y terms */
    this->idx += arith_rshift(this->ax, k3);
    this->idy += arith_rshift(this->ay, k3);
    this->rdx += (uint)this->ax & this->rmask;
    this->rdy += (uint)this->ay & this->rmask;
    adjust_rem(this->rdx, this->idx, this->rmask);
    adjust_rem(this->rdy, this->idy, this->rmask);
    this->id2x += this->id3x = arith_rshift(ax6, k3);
    this->id2y += this->id3y = arith_rshift(ay6, k3);
    this->rd2x += this->rd3x = (uint)ax6 & this->rmask,
    this->rd2y += this->rd3y = (uint)ay6 & this->rmask;
    adjust_rem(this->rd2x, this->id2x, this->rmask);
    adjust_rem(this->rd2y, this->id2y, this->rmask);
#   undef adjust_rem
    return true;
}

private inline bool 
check_diff_overflow(fixed v0, fixed v1)
{
    if (v0 < v1) {
	if (v1 - v0 < 0)
	    return true;
    } else {
	if (v0 - v1 < 0)
	    return true;
    }
    return false;
}

/*  Initialize the iterator with a line. */
bool
gx_flattened_iterator__init_line(gx_flattened_iterator *this, 
	    fixed x0, fixed y0, fixed x1, fixed y1)
{
    bool ox = check_diff_overflow(x0, x1);
    bool oy = check_diff_overflow(y0, y1);

    this->x0 = this->lx0 = this->lx1 = x0;
    this->y0 = this->ly0 = this->ly1 = y0;
    this->x3 = x1;
    this->y3 = y1;
    if (ox || oy) {
	/* Subdivide a long line into 2 segments, because the filling algorithm 
	   and the stroking algorithm need to compute differences 
	   of coordinates of end points. */
	/* Note : the result of subdivision may be not strongly colinear. */
	this->ax = this->bx = 0;
	this->ay = this->by = 0;
	this->cx = (ox ? (x1 >> 1) - (x0 >> 1) : (x1 - x0) / 2);
	this->cy = (oy ? (y1 >> 1) - (y0 >> 1) : (y1 - y0) / 2);
	this->k = 1;
	this->i = 2;
    } else {
	this->k = 0;
	this->i = 1;
    }
    this->curve = false;
    return true;
}

#ifdef DEBUG
private inline void
gx_flattened_iterator__print_state(gx_flattened_iterator *this)
{
    if (!gs_debug_c('3'))
	return;
    dlprintf4("[3]dx=%f+%d, dy=%f+%d\n",
	      fixed2float(this->idx), this->rdx,
	      fixed2float(this->idy), this->rdy);
    dlprintf4("   d2x=%f+%d, d2y=%f+%d\n",
	      fixed2float(this->id2x), this->rd2x,
	      fixed2float(this->id2y), this->rd2y);
    dlprintf4("   d3x=%f+%d, d3y=%f+%d\n",
	      fixed2float(this->id3x), this->rd3x,
	      fixed2float(this->id3y), this->rd3y);
}
#endif

/* Move to the next segment and store it to this->lx0, this->ly0, this->lx1, this->ly1 .
 * Return true iff there exist more segments.
 */
bool
gx_flattened_iterator__next(gx_flattened_iterator *this)
{
    /*
     * We can compute successive values by finite differences,
     * using the formulas:
     x(t) =
     a*t^3 + b*t^2 + c*t + d =>
     dx(t) = x(t+e)-x(t) =
     a*(3*t^2*e + 3*t*e^2 + e^3) + b*(2*t*e + e^2) + c*e =
     (3*a*e)*t^2 + (3*a*e^2 + 2*b*e)*t + (a*e^3 + b*e^2 + c*e) =>
     d2x(t) = dx(t+e)-dx(t) =
     (3*a*e)*(2*t*e + e^2) + (3*a*e^2 + 2*b*e)*e =
     (6*a*e^2)*t + (6*a*e^3 + 2*b*e^2) =>
     d3x(t) = d2x(t+e)-d2x(t) =
     6*a*e^3;
     x(0) = d, dx(0) = (a*e^3 + b*e^2 + c*e),
     d2x(0) = 6*a*e^3 + 2*b*e^2;
     * In these formulas, e = 1/2^k; of course, there are separate
     * computations for the x and y values.
     *
     * There is a tradeoff in doing the above computation in fixed
     * point.  If we separate out the constant term (d) and require that
     * all the other values fit in a long, then on a 32-bit machine with
     * 12 bits of fraction in a fixed, k = 4 implies a maximum curve
     * size of 128 pixels; anything larger requires subdividing the
     * curve.  On the other hand, doing the computations in explicit
     * double precision slows down the loop by a factor of 3 or so.  We

     * found to our surprise that the latter is actually faster, because
     * the additional subdivisions cost more than the slower loop.
     *
     * We represent each quantity as I+R/M, where I is an "integer" and
     * the "remainder" R lies in the range 0 <= R < M=2^(3*k).  Note
     * that R may temporarily exceed M; for this reason, we require that
     * M have at least one free high-order bit.  To reduce the number of
     * variables, we don't actually compute M, only M-1 (rmask).  */
    fixed x = this->lx1, y = this->ly1;

    this->lx0 = this->lx1;
    this->ly0 = this->ly1;
    /* Fast check for N == 3, a common special case for small characters. */
    if (this->k <= 1) {
	--this->i;
	if (this->i == 0)
	    goto last;
#	define poly2(a,b,c) arith_rshift_1(arith_rshift_1(arith_rshift_1(a) + b) + c)
	x += poly2(this->ax, this->bx, this->cx);
	y += poly2(this->ay, this->by, this->cy);
#	undef poly2
	if_debug2('3', "[3]dx=%f, dy=%f\n",
		  fixed2float(x - this->x0), fixed2float(y - this->y0));
	if_debug5('3', "[3]%s x=%g, y=%g x=%d y=%d\n",
		  (((x ^ this->x0) | (y ^ this->y0)) & float2fixed(-0.5) ?
		   "add" : "skip"),
		  fixed2float(x), fixed2float(y), x, y);
	this->lx1 = x, this->ly1 = y;
	vd_bar(this->lx0, this->ly0, this->lx1, this->ly1, 1, RGB(0, 255, 0));
	return true;
    } else {
	--this->i;
	if (this->i == 0)
	    goto last; /* don't bother with last accum */
#	ifdef DEBUG
	    gx_flattened_iterator__print_state(this);
#	endif
#	define accum(i, r, di, dr, rmask)\
			if ( (r += dr) > rmask ) r &= rmask, i += di + 1;\
			else i += di
	accum(x, this->rx, this->idx, this->rdx, this->rmask);
	accum(y, this->ry, this->idy, this->rdy, this->rmask);
	accum(this->idx, this->rdx, this->id2x, this->rd2x, this->rmask);
	accum(this->idy, this->rdy, this->id2y, this->rd2y, this->rmask);
	accum(this->id2x, this->rd2x, this->id3x, this->rd3x, this->rmask);
	accum(this->id2y, this->rd2y, this->id3y, this->rd3y, this->rmask);
	if_debug5('3', "[3]%s x=%g, y=%g x=%d y=%d\n",
		  (((x ^ this->lx0) | (y ^ this->ly0)) & float2fixed(-0.5) ?
		   "add" : "skip"),
		  fixed2float(x), fixed2float(y), x, y);
#	undef accum
	this->lx1 = this->x = x;
	this->ly1 = this->y = y;
	vd_bar(this->lx0, this->ly0, this->lx1, this->ly1, 1, RGB(0, 255, 0));
	return true;
    }
last:
    this->lx1 = this->x3;
    this->ly1 = this->y3;
    if_debug4('3', "[3]last x=%g, y=%g x=%d y=%d\n",
	      fixed2float(this->lx1), fixed2float(this->ly1), this->lx1, this->ly1);
    vd_bar(this->lx0, this->ly0, this->lx1, this->ly1, 1, RGB(0, 255, 0));
    return false;
}

private inline void
gx_flattened_iterator__unaccum(gx_flattened_iterator *this)
{
#   define unaccum(i, r, di, dr, rmask)\
		    if ( r < dr ) r += rmask + 1 - dr, i -= di + 1;\
		    else r -= dr, i -= di
    unaccum(this->id2x, this->rd2x, this->id3x, this->rd3x, this->rmask);
    unaccum(this->id2y, this->rd2y, this->id3y, this->rd3y, this->rmask);
    unaccum(this->idx, this->rdx, this->id2x, this->rd2x, this->rmask);
    unaccum(this->idy, this->rdy, this->id2y, this->rd2y, this->rmask);
    unaccum(this->x, this->rx, this->idx, this->rdx, this->rmask);
    unaccum(this->y, this->ry, this->idy, this->rdy, this->rmask);
#   undef unaccum
}

/* Move back to the previous segment and store it to this->lx0, this->ly0, this->lx1, this->ly1 .
 * This only works for states reached with gx_flattened_iterator__next.
 * Return true iff there exist more segments.
 */
bool
gx_flattened_iterator__prev(gx_flattened_iterator *this)
{
    bool last; /* i.e. the first one in the forth order. */

    assert(this->i < 1 << this->k);
    this->lx1 = this->lx0;
    this->ly1 = this->ly0;
    if (this->k <= 1) {
	/* If k==0, we have a single segment, return it.
	   If k==1 && i < 2, return the last segment.
	   Otherwise must not pass here.
	   We caould allow to pass here with this->i == 1 << this->k,
	   but we want to check the assertion about the last segment below.
	 */
	this->i++;
	this->lx0 = this->x0;
	this->ly0 = this->y0;
	vd_bar(this->lx0, this->ly0, this->lx1, this->ly1, 1, RGB(0, 0, 255));
	return false;
    }
    gx_flattened_iterator__unaccum(this);
    this->i++;
#   ifdef DEBUG
    if_debug5('3', "[3]%s x=%g, y=%g x=%d y=%d\n",
	      (((this->x ^ this->lx1) | (this->y ^ this->ly1)) & float2fixed(-0.5) ?
	       "add" : "skip"),
	      fixed2float(this->x), fixed2float(this->y), this->x, this->y);
    gx_flattened_iterator__print_state(this);
#   endif
    last = (this->i == (1 << this->k) - 1);
    this->lx0 = this->x;
    this->ly0 = this->y;
    vd_bar(this->lx0, this->ly0, this->lx1, this->ly1, 1, RGB(0, 0, 255));
    if (last)
	assert(this->lx0 == this->x0 && this->ly0 == this->y0);
    return !last;
}

/* Switching from the forward scanning to the backward scanning for the filtered1. */
void
gx_flattened_iterator__switch_to_backscan(gx_flattened_iterator *this, bool not_first)
{
    /*	When scanning forth, the accumulator stands on the end of a segment,
	except for the last segment.
	When scanning back, the accumulator should stand on the beginning of a segment.
	Asuuming that at least one forward step is done.
    */
    if (not_first)
	if (this->i > 0 && this->k != 1 /* This case doesn't use the accumulator. */)
	    gx_flattened_iterator__unaccum(this);
}

#define max_points 50		/* arbitrary */

private int
generate_segments(gx_path * ppath, const gs_fixed_point *points, 
		    int count, segment_notes notes)
{
    /* vd_moveto(ppath->position.x, ppath->position.y); */
    if (notes & sn_not_first) {
	/* vd_lineto_multi(points, count); */
	print_points(points, count);
	return gx_path_add_lines_notes(ppath, points, count, notes);
    } else {
	int code;

	/* vd_lineto(points[0].x, points[0].y); */
	print_points(points, 1);
	code = gx_path_add_line_notes(ppath, points[0].x, points[0].y, notes);
	if (code < 0)
	    return code;
	/* vd_lineto_multi(points + 1, count - 1); */
	print_points(points + 1, count - 1);
	return gx_path_add_lines_notes(ppath, points + 1, count - 1, notes | sn_not_first);
    }
}

private int
gx_subdivide_curve_rec(gx_flattened_iterator *this, 
		  gx_path * ppath, int k, curve_segment * pc,
		  segment_notes notes, gs_fixed_point *points)
{
    int code;

top :
    if (!gx_flattened_iterator__init(this, 
		ppath->position.x, ppath->position.y, pc, k)) {
	/* Curve is too long.  Break into two pieces and recur. */
	curve_segment cseg;

	k--;
	split_curve_midpoint(ppath->position.x, ppath->position.y, pc, &cseg, pc);
	code = gx_subdivide_curve_rec(this, ppath, k, &cseg, notes, points);
	if (code < 0)
	    return code;
	notes |= sn_not_first;
	goto top;
    } else if (k == -1) {
	/* fixme : Don't need to init the iterator. Just wanted to check in_range. */
	return gx_path_add_curve_notes(ppath, pc->p1.x, pc->p1.y, pc->p2.x, pc->p2.y, 
			pc->pt.x, pc->pt.y, notes);
    } else {
	gs_fixed_point *ppt = points;
	bool more;

	for(;;) {
	    more = gx_flattened_iterator__next(this);
	    ppt->x = this->lx1;
	    ppt->y = this->ly1;
	    ppt++;
	    if (ppt == &points[max_points] || !more) {
		gs_fixed_point *pe = (more ?  ppt - 2 : ppt);

		code = generate_segments(ppath, points, pe - points, notes);
		if (code < 0)
		    return code;
		if (!more)
		    return 0;
		notes |= sn_not_first;
		memcpy(points, pe, (char *)ppt - (char *)pe);
		ppt = points + (ppt - pe);
	    }
	}
    }
}

#undef coord_near

/*
 * Flatten a segment of the path by repeated sampling.
 * 2^k is the number of lines to produce (i.e., the number of points - 1,
 * including the endpoints); we require k >= 1.
 * If k or any of the coefficient values are too large,
 * use recursive subdivision to whittle them down.
 */

int
gx_subdivide_curve(gx_path * ppath, int k, curve_segment * pc, segment_notes notes)
{
    gs_fixed_point points[max_points + 1];
    gx_flattened_iterator iter;

    return gx_subdivide_curve_rec(&iter, ppath, k, pc, notes, points);
}

#undef max_points


