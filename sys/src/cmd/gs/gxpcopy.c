/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gxpcopy.c */
/* Path copying and flattening for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"			/* for gscoord.h */
#include "gscoord.h"
#include "gxfixed.h"
#include "gxarith.h"
#include "gzline.h"
#include "gzpath.h"

/* Forward declarations */
private int copy_path(P4(const gx_path *, gx_path *, fixed, bool));
private int flatten_internal(P8(gx_path *,
  fixed, fixed, fixed, fixed, fixed, fixed, fixed));
private int flatten_sample(P8(gx_path *, int,
  fixed, fixed, fixed, fixed, fixed, fixed));

/* Copy a path */
int
gx_path_copy(const gx_path *ppath_old, gx_path *ppath, bool init)
{	return copy_path(ppath_old, ppath, max_fixed, init);
}

/* Flatten a path. */
int
gx_path_flatten(const gx_path *ppath_old, gx_path *ppath, floatp flatness,
  bool in_BuildCharGlyph)
{	return copy_path(ppath_old, ppath,
			 (in_BuildCharGlyph ? fixed_0 : float2fixed(flatness)),
			 true);
}

/* Add a flattened curve to a path. */
int
gx_path_add_flattened_curve(gx_path *ppath,
  fixed x1, fixed y1, fixed x2, fixed y2, fixed x3, fixed y3,
  floatp flatness)
{	return flatten_internal(ppath, x1, y1, x2, y2, x3, y3,
				float2fixed(flatness));
}

/* Copy a path, optionally flattening it. */
/* If the copy fails, free the new path. */
private int
copy_path(const gx_path *ppath_old, gx_path *ppath, fixed fixed_flat,
  bool init)
{	gx_path old;
	const segment *pseg;
	int code;
#ifdef DEBUG
if ( gs_debug_c('p') )
	gx_dump_path(ppath_old, "before copy_path");
#endif
	old = *ppath_old;
	if ( init )
		gx_path_init(ppath, ppath_old->memory);
	pseg = (const segment *)(old.first_subpath);
	while ( pseg )
	   {	switch ( pseg->type )
		   {
		case s_start:
			code = gx_path_add_point(ppath, pseg->pt.x, pseg->pt.y);
			break;
		case s_curve:
		   {	curve_segment *pc = (curve_segment *)pseg;
			if ( fixed_flat == max_fixed )	/* don't flatten */
				code = gx_path_add_curve(ppath,
					pc->p1.x, pc->p1.y,
					pc->p2.x, pc->p2.y,
					pc->pt.x, pc->pt.y);
			else
				code = flatten_internal(ppath,
					pc->p1.x, pc->p1.y,
					pc->p2.x, pc->p2.y,
					pc->pt.x, pc->pt.y,
					fixed_flat);
			break;
		   }
		case s_line:
			code = gx_path_add_line(ppath, pseg->pt.x, pseg->pt.y);
			break;
		case s_line_close:
			code = gx_path_close_subpath(ppath);
			break;
		default:		/* can't happen */
			code = gs_note_error(gs_error_unregistered);
		   }
		if ( code )
		   {	gx_path_release(ppath);
			if ( ppath == ppath_old )
				*ppath = old;
			return code;
		   }
		pseg = pseg->next;
	}
	if ( old.subpath_open < 0 )
		gx_path_add_point(ppath, old.position.x, old.position.y);
#ifdef DEBUG
if ( gs_debug_c('p') )
	gx_dump_path(ppath, "after copy_path");
#endif
	return 0;
}

/*
 * To calculate how many points to sample along a path in order to
 * approximate it to the desired degree of flatness, we define
 *	dist((x,y)) = abs(x) + abs(y);
 * then the number of points we need is
 *	N = 1 + sqrt(3/4 * D / flatness),
 * where
 *	D = max(dist(p0 - 2*p1 + p2), dist(p1 - 2*p2 + p3)).
 * Since we are going to use a power of 2 for the number of intervals,
 * we can avoid the square root by letting
 *	N = 1 + 2^(ceiling(log2(3/4 * D / flatness) / 2)).
 * (Reference: DEC Paris Research Laboratory report #1, May 1989.)
 *
 * We treat two cases specially.  First, if the curve is very
 * short, we halve the flatness, to avoid turning short shallow curves
 * into short straight lines.  Second, if the curve forms part of a
 * character, or the flatness is less than half a pixel, we let
 *	N = 1 + 2 * max(abs(x3-x0), abs(y3-y0)).
 * This is probably too conservative, but it produces good results.
 */
private int
flatten_internal(gx_path *ppath, fixed x1, fixed y1, fixed x2, fixed y2,
  fixed x3, fixed y3, fixed fixed_flat)
{	const fixed
		x0 = ppath->position.x,
		y0 = ppath->position.y;
	fixed
		x03 = x0 - x3,
		y03 = y0 - y3;
	int k;
	if ( x03 < 0 ) x03 = -x03;
	if ( y03 < 0 ) y03 = -y03;
	if ( (x03 | y03) < int2fixed(16) )
		fixed_flat >>= 1;
	if ( fixed_flat < fixed_half )
	{	/* Use the conservative method. */
		fixed m = max(x03, y03);
		for ( k = 1; m > fixed_1; )
			k++, m >>= 1;
	}
	else
	{	const fixed
			x12 = x1 - x2,
			y12 = y1 - y2,
			dx0 = x0 - x1 - x12,
			dy0 = y0 - y1 - y12,
			dx1 = x12 - x2 + x3,
			dy1 = y12 - y2 + y3,
			adx0 = any_abs(dx0),
			ady0 = any_abs(dy0),
			adx1 = any_abs(dx1),
			ady1 = any_abs(dy1);
		fixed
			d = max(adx0, adx1) + max(ady0, ady1);
		fixed q;
		if_debug6('2', "[2]d01=%g,%g d12=%g,%g d23=%g,%g\n",
			  fixed2float(x1 - x0), fixed2float(y1 - y0),
			  fixed2float(-x12), fixed2float(-y12),
			  fixed2float(x3 - x2), fixed2float(y3 - y2));
		if_debug2('2', "     D=%f, flat=%f,",
			  fixed2float(d), fixed2float(fixed_flat));
		d -= d >> 2;			/* 3/4 * D */
		if ( d < (fixed)1 << (sizeof(fixed) * 8 - _fixed_shift - 1) )
			q = (d << _fixed_shift) / fixed_flat;
		else
			q = float2fixed((float)d / fixed_flat);
		/* Now we want to set k = ceiling(log2(q) / 2). */
		for ( k = 0; q > fixed_1; )
			k++, q >>= 2;
		if_debug1('2', " k=%d\n", k);
	}
	return flatten_sample(ppath, k, x1, y1, x2, y2, x3, y3);
}

/* Maximum number of points for sampling if we want accurate rasterizing. */
/* 2^(k_sample_max*3)-1 must fit into a uint with a bit to spare. */
#define k_sample_max ((size_of(int) * 8 - 1) / 3)

/* Flatten a segment of the path by repeated sampling. */
/* 2^k is the number of lines to produce (i.e., the number of points - 1, */
/* including the endpoints); we require k >= 1. */
/* If k or any of the coefficient values are too large, */
/* use recursive subdivision to whittle them down. */
private int
flatten_sample(gx_path *ppath, int k,
  fixed x1, fixed y1, fixed x2, fixed y2, fixed x3, fixed y3)
{	fixed x0, y0;
	fixed cx, bx, ax, cy, by, ay;
	fixed ptx, pty;
	fixed x, y;
	/*
	 * If all the coefficients lie between min_fast and max_fast,
	 * we can do everything in fixed point.  In this case we compute
	 * successive values by finite differences, using the formulas:
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
	 */
	uint i;
	/*
	 * We do exact rational arithmetic to avoid accumulating error.
	 * Each quantity is represented as I+R/M, where I is an "integer"
	 * and the "remainder" R lies in the range 0 <= R < M=2^(3*k).
	 * Note that R may temporarily exceed M; for this reason,
	 * we require that M have at least one free high-order bit.
	 * To reduce the number of variables, we don't actually compute M,
	 * only M-1 (rmask).
	 */
	uint rmask;			/* M-1 */
	fixed idx, idy, id2x, id2y, id3x, id3y;		/* I */
	uint rx, ry, rdx, rdy, rd2x, rd2y, rd3x, rd3y;	/* R */
	gs_fixed_point *ppt;
#define max_points 50			/* arbitrary */
	gs_fixed_point points[max_points + 1];

top:	x0 = ppath->position.x;
	y0 = ppath->position.y;
#ifdef DEBUG
if ( gs_debug_c('3') )
	dprintf4("[3]x0=%f y0=%f x1=%f y1=%f\n",
		 fixed2float(x0), fixed2float(y0),
		 fixed2float(x1), fixed2float(y1)),
	dprintf4("   x2=%f y2=%f x3=%f y3=%f\n",
		 fixed2float(x2), fixed2float(y2),
		 fixed2float(x3), fixed2float(y3));
#endif
	{	/* We spell out some multiplies by 3, */
		/* for the benefit of compilers that don't optimize this. */
		fixed x01, x12, y01, y12;
		x01 = x1 - x0;
		cx = (x01 << 1) + x01;		/* 3*(x1-x0) */
		x12 = x2 - x1;
		bx = (x12 << 1) + x12 - cx;	/* 3*(x2-2*x1+x0) */
		ax = x3 - bx - cx - x0;		/* x3-3*x2+3*x1-x0 */
		y01 = y1 - y0;
		cy = (y01 << 1) + y01;
		y12 = y2 - y1;
		by = (y12 << 1) + y12 - cy;
		ay = y3 - by - cy - y0;
	}

	if_debug6('3', "[3]ax=%f bx=%f cx=%f\n   ay=%f by=%f cy=%f\n",
		  fixed2float(ax), fixed2float(bx), fixed2float(cx),
		  fixed2float(ay), fixed2float(by), fixed2float(cy));
#define max_fast (max_fixed / 6)
#define min_fast (-max_fast)
#define in_range(v) (v < max_fast && v > min_fast)
	if ( k == 0 )
	{	/* The curve is very short, or anomalous in some way. */
		/* Just add a line and exit. */
		return gx_path_add_line(ppath, x3, y3);
	}
	if ( k <= k_sample_max &&
	     in_range(ax) && in_range(ay) &&
	     in_range(bx) && in_range(by) &&
	     in_range(cx) && in_range(cy)
	   )
	{	x = x0, y = y0;
		rx = ry = 0;
		ppt = points;
		/* Fast check for n == 3, a common special case */
		/* for small characters. */
		if ( k == 1 )
		{
#define poly2(a,b,c)\
  arith_rshift_1(arith_rshift_1(arith_rshift_1(a) + b) + c)
			x += poly2(ax, bx, cx);
			y += poly2(ay, by, cy);
#undef poly2
			if_debug2('3', "[3]dx=%f, dy=%f\n",
				  fixed2float(x - x0), fixed2float(y - y0));
			if_debug3('3', "[3]%s x=%g, y=%g\n",
				  (((x ^ x0) | (y ^ y0)) & float2fixed(-0.5) ?
				   "add" : "skip"),
				  fixed2float(x), fixed2float(y));
			if ( ((x ^ x0) | (y ^ y0)) & float2fixed(-0.5) )
			  ppt->x = ptx = x,
			  ppt->y = pty = y,
			  ppt++;
			goto last;
		}
		else
		{	fixed bx2 = bx << 1, by2 = by << 1;
			fixed ax6 = ((ax << 1) + ax) << 1,
			      ay6 = ((ay << 1) + ay) << 1;
#define adjust_rem(r, q)\
  if ( r > rmask ) q ++, r &= rmask
			const int k2 = k << 1;
			const int k3 = k2 + k;
			rmask = (1 << k3) - 1;
			/* We can compute all the remainders as ints, */
			/* because we know they don't exceed M. */
			/* cx/y terms */
			idx = arith_rshift(cx, k),
			  idy = arith_rshift(cy, k);
			rdx = ((uint)cx << k2) & rmask,
			  rdy = ((uint)cy << k2) & rmask;
			/* bx/y terms */
			id2x = arith_rshift(bx2, k2),
			  id2y = arith_rshift(by2, k2);
			rd2x = ((uint)bx2 << k) & rmask,
			  rd2y = ((uint)by2 << k) & rmask;
			idx += arith_rshift_1(id2x),
			  idy += arith_rshift_1(id2y);
			rdx += ((uint)bx << k) & rmask,
			  rdy += ((uint)by << k) & rmask;
			adjust_rem(rdx, idx);
			adjust_rem(rdy, idy);
			/* ax/y terms */
			idx += arith_rshift(ax, k3),
			  idy += arith_rshift(ay, k3);
			rdx += (uint)ax & rmask,
			  rdy += (uint)ay & rmask;
			adjust_rem(rdx, idx);
			adjust_rem(rdy, idy);
			id2x += id3x = arith_rshift(ax6, k3),
			  id2y += id3y = arith_rshift(ay6, k3);
			rd2x += rd3x = (uint)ax6 & rmask,
			  rd2y += rd3y = (uint)ay6 & rmask;
			adjust_rem(rd2x, id2x);
			adjust_rem(rd2y, id2y);
#undef adjust_rem
		}
	}
	else
	{	/* Curve is too long.  Break into two pieces and recur. */
		/* Algorithm is from "The Beta2-split: A special case of */
		/* the Beta-spline Curve and Surface Representation," */
		/* B. A. Barsky and A. D. DeRose, IEEE, 1985, */
		/* courtesy of Crispin Goswell. */

		/* We have to define midpoint carefully to avoid overflow. */
		/* (If it overflows, something really pathological is going */
		/* on, but we could get infinite recursion that way....) */
#define midpoint(a,b)\
  (arith_rshift_1(a) + arith_rshift_1(b) + ((a) & (b) & 1))
		k--;
		{	fixed x0 = ppath->position.x, y0 = ppath->position.y;
			fixed x01 = midpoint(x0, x1), y01 = midpoint(y0, y1);
			fixed x12 = midpoint(x1, x2), y12 = midpoint(y1, y2);
			fixed x02 = midpoint(x01, x12), y02 = midpoint(y01, y12);
			int code;
			/* Update x/y1 and x/y2 now for the second half. */
			x2 = midpoint(x2, x3), y2 = midpoint(y2, y3);
			x1 = midpoint(x12, x2), y1 = midpoint(y12, y2);
			code = flatten_sample(ppath, k, x01, y01, x02, y02,
					midpoint(x02, x1), midpoint(y02, y1));
			if ( code < 0 ) return code;
		}
		goto top;
	}
	if_debug1('2', "[2]sampling k=%d\n", k);
	ptx = x0, pty = y0;
	for ( i = (1 << k) - 1; ; )
	{	int code;
#ifdef DEBUG
if ( gs_debug_c('3') )
		dprintf4("[3]dx=%f+%d, dy=%f+%d\n",
			 fixed2float(idx), rdx,
			 fixed2float(idy), rdy),
		dprintf4("   d2x=%f+%d, d2y=%f+%d\n",
			 fixed2float(id2x), rd2x,
			 fixed2float(id2y), rd2y),
		dprintf4("   d3x=%f+%d, d3y=%f+%d\n",
			 fixed2float(id3x), rd3x,
			 fixed2float(id3y), rd3y);
#endif
#define accum(i, r, di, dr)\
  if ( (r += dr) > rmask ) r &= rmask, i += di + 1;\
  else i += di
		accum(x, rx, idx, rdx);
		accum(y, ry, idy, rdy);
		if_debug3('3', "[3]%s x=%g, y=%g\n",
			  (((x ^ ptx) | (y ^ pty)) & float2fixed(-0.5) ?
			   "add" : "skip"),
			  fixed2float(x), fixed2float(y));
		/* Skip very short segments */
		if ( ((x ^ ptx) | (y ^ pty)) & float2fixed(-0.5) )
		{	if ( ppt == &points[max_points] )
			  {	if ( (code = gx_path_add_lines(ppath, points, max_points)) < 0 )
				  return code;
				ppt = points;
			  }
			ppt->x = ptx = x;
			ppt->y = pty = y;
			ppt++;
		}
		if ( --i == 0 )
			break;		/* don't bother with last accum */
		accum(idx, rdx, id2x, rd2x);
		accum(id2x, rd2x, id3x, rd3x);
		accum(idy, rdy, id2y, rd2y);
		accum(id2y, rd2y, id3y, rd3y);
#undef accum
	}
last:	if_debug2('3', "[3]last x=%g, y=%g\n",
		  fixed2float(x3), fixed2float(y3));
	ppt->x = x3, ppt->y = y3;
	return gx_path_add_lines(ppath, points, (int)(ppt + 1 - points));
}

/*
 *	The rest of this file is an analysis that will eventually
 *	allow us to rasterize curves on the fly, by finding points
 *	where Y reaches a local maximum or minimum, which allows us to
 *	divide the curve into locally Y-monotonic sections.
 */

/*
	Let y(t) = a*t^3 + b*t^2 + c*t + d, 0 <= t <= 1.
	Then dy(t) = 3*a*t^2 + 2*b*t + c.
	y(t) has a local minimum or maximum (or inflection point)
	precisely where dy(t) = 0.  Now the roots of dy(t) are
		( -2*b +/- sqrt(4*b^2 - 12*a*c) ) / 6*a
	   =	( -b +/- sqrt(b*2 - 3*a*c) ) / 3*a
	(Note that real roots exist iff b^2 >= 3*a*c.)
	We want to know if these lie in the range (0..1).
	(We don't care about the endpoints.)  Call such a root
	a "valid zero."  Since computing the roots is expensive, we would
	like to have a cheap a priori test as to whether they exist.
	We proceed as follows:
		If sign(3*a + 2*b + c) ~= sign(c), a valid zero exists,
		  since dy(0) and dy(1) have opposite signs and hence
		  dy(t) must be zero somewhere in the interval [0..1].
		If sign(a) = sign(b), no valid zero exists,
		  since dy is monotonic on [0..1] and has the same sign
		  at both endpoints.
	Otherwise, dy(t) may be non-monotonic on [0..1]; it has valid zeros
	  iff there is an extremum in this interval and the extremum
	  is of the opposite sign from c.
	To find this out, we look for the local extremum of dy(t) by observing
		d2y(t) = 6*a*t + 2*b
	which has a zero only at
		t1 = -b / 3*a
	Now if t1 <= 0 or t1 >= 1, no valid zero exists.  Otherwise,
	we compute
		dy(t1) = c - (b^2 / 3*a)
	Then a valid zero exists (at t1) iff sign(dy(t1)) ~= sign(c).
 */

/* Expand a dashed path into explicit segments. */
/* The path contains no curves. */
private int subpath_expand_dashes(P3(const subpath *, gx_path *, gs_state *));
int
gx_path_expand_dashes(const gx_path *ppath_old, gx_path *ppath, gs_state *pgs)
{	const subpath *psub;
	if ( gs_currentdash_length(pgs) == 0 )
	  return gx_path_copy(ppath_old, ppath, true);
	gx_path_init(ppath, ppath_old->memory);
	for ( psub = ppath_old->first_subpath; psub != 0;
	      psub = (const subpath *)psub->last->next
	    )
	  {	int code = subpath_expand_dashes(psub, ppath, pgs);
		if ( code < 0 )
		  {	gx_path_release(ppath);
			return code;
		  }
	  }
	return 0;
}
private int
subpath_expand_dashes(const subpath *psub, gx_path *ppath, gs_state *pgs)
{	const gx_dash_params *dash = &gs_currentlineparams(pgs)->dash;
	const float *pattern = dash->pattern;
	int count, ink_on, index;
	float dist_left;
	fixed x0 = psub->pt.x, y0 = psub->pt.y;
	fixed x, y;
	const segment *pseg;
	int wrap = (dash->init_ink_on && psub->is_closed ? -1 : 0);
	int drawing = wrap;
	int code;
	if ( (code = gx_path_add_point(ppath, x0, y0)) < 0 )
		return code;
	/* To do the right thing at the beginning of a closed path, */
	/* we have to skip any initial line, and then redo it at */
	/* the end of the path.  Drawing = -1 while skipping, */
	/* 0 while drawing normally, and 1 on the second round. */
top:	count = dash->pattern_size;
	ink_on = dash->init_ink_on;
	index = dash->init_index;
	dist_left = dash->init_dist_left;
	x = x0, y = y0;
	pseg = (const segment *)psub;
	while ( (pseg = pseg->next) != 0 && pseg->type != s_start )
	   {	fixed sx = pseg->pt.x, sy = pseg->pt.y;
		fixed udx = sx - x, udy = sy - y;
		float length, dx, dy;
		float dist;
		if ( !(udx | udy) )	/* degenerate */
			dx = 0, dy = 0, length = 0;
		else
		   {	gs_point d;
			dx = udx, dy = udy;	/* scaled as fixed */
			gs_idtransform(pgs, dx, dy, &d);
			length = hypot(d.x, d.y) * (1 / (float)int2fixed(1));
		   }
		dist = length;
		while ( dist > dist_left )
		   {	/* We are using up the dash element */
			float fraction = dist_left / length;
			fixed nx = x + (fixed)(dx * fraction);
			fixed ny = y + (fixed)(dy * fraction);
			if ( ink_on )
			   {	if ( drawing >= 0 )
				  code = gx_path_add_line(ppath, nx, ny);
			   }
			else
			   {	if ( drawing > 0 ) return 0;	/* done */
				code = gx_path_add_point(ppath, nx, ny);
				drawing = 0;
			   }
			if ( code < 0 ) return code;
			dist -= dist_left;
			ink_on = !ink_on;
			if ( ++index == count ) index = 0;
			dist_left = pattern[index];
			x = nx, y = ny;
		   }
		dist_left -= dist;
		/* Handle the last dash of a segment. */
		if ( ink_on )
		   {	if ( drawing >= 0 )
			  code =
			    (pseg->type == s_line_close && drawing > 0 ?
			     gx_path_close_subpath(ppath) :
			     gx_path_add_line(ppath, sx, sy));
		   }
		else
		   {	if ( drawing > 0 ) return 0;	/* done */
			code = gx_path_add_point(ppath, sx, sy);
			drawing = 0;
		   }
		if ( code < 0 ) return code;
		x = sx, y = sy;
	   }
	/* Check for wraparound. */
	if ( wrap && drawing <= 0 )
	   {	/* We skipped some initial lines. */
		/* Go back and do them now. */
		drawing = 1;
		goto top;
	   }
	return 0;
}
