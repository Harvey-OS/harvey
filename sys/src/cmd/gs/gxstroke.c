/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxstroke.c */
/* Path stroking procedures for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxarith.h"
#include "gxmatrix.h"
#include "gscoord.h"
#include "gzstate.h"
#include "gzline.h"
#include "gzpath.h"
#include "gxpaint.h"
#include "gxdraw.h"		/* gx_draw_line_fixed prototype */

/*
 * We don't really know whether it's a good idea to take fill adjustment
 * into account for stroking.  Disregarding it means that strokes
 * come out thinner than fills; observing it produces heavy-looking
 * strokes at low resolutions.  But in any case, we must disregard it
 * when stroking zero-width lines.
 */
#define USE_FILL_ADJUSTMENT

#ifdef USE_FILL_ADJUSTMENT
#  define stroke_adjustment(thin) (thin ? fixed_0 : pgs->fill_adjust)
#else
#  define stroke_adjustment(thin) fixed_0
#endif

/*
 * Structure for a partial line (passed to the drawing routine).
 * Two of these are required to do joins right.
 * Each endpoint includes the two ends of the cap as well,
 * and the deltas for square and round cap computation.
 *
 * The two base values for computing the caps of a partial line are the
 * width and the end cap delta.  The width value is one-half the line
 * width (suitably transformed) at 90 degrees counter-clockwise
 * (in device space, but with "90 degrees" interpreted in *user*
 * coordinates) at the end (as opposed to the origin) of the line.
 * The cdelta value is one-half the transformed line width in the same
 * direction as the line.  From these, we compute two other values at each
 * end of the line: co and ce, which are the ends of the cap.
 * Note that the cdelta values at o are the negatives of the values at e,
 * as are the offsets from p to co and ce.
 *
 * Initially, only o.p, e.p, e.cdelta, width, and thin are set.
 * compute_caps fills in the rest.
 */
typedef gs_fixed_point _ss *p_ptr;
typedef struct endpoint_s {
	gs_fixed_point p;		/* the end of the line */
	gs_fixed_point co, ce;		/* ends of the cap, p +/- width */
	gs_fixed_point cdelta;		/* +/- cap length */
} endpoint;
typedef endpoint _ss *ep_ptr;
typedef struct partial_line_s {
	endpoint o;			/* starting coordinate */
	endpoint e;			/* ending coordinate */
	gs_fixed_point width;		/* one-half line width, see above */
	bool thin;			/* true if minimum-width line */
} partial_line;
typedef partial_line _ss *pl_ptr;

/* Assign a point.  Some compilers would do this with very slow code */
/* if we simply implemented it as an assignment. */
#define assign_point(pp, p)\
  ((pp)->x = (p).x, (pp)->y = (p).y)

/* Procedures that stroke a partial_line (the first pl_ptr argument). */
/* If both partial_lines are non-null, the procedure creates */
/* an appropriate join; otherwise, the procedure creates an */
/* end cap.  If the first int is 0, the procedure also starts with */
/* an appropriate cap. */
private int near stroke_add(P5(gx_path *, int, pl_ptr, pl_ptr, gs_state *));
private int near stroke_fill(P5(gx_path *, int, pl_ptr, pl_ptr, gs_state *));

/* Other forward declarations */
private int near stroke(P4(const gx_path *, gx_path *,
  int near (*)(P5(gx_path *, int, pl_ptr, pl_ptr, gs_state *)),
  gs_state *));
private void near adjust_stroke(P3(pl_ptr, gs_state *, bool));
private void near compute_caps(P1(pl_ptr));
private int near add_capped(P4(gx_path *, gs_line_cap,
  int (*)(P3(gx_path *, fixed, fixed)),
  ep_ptr));

/* Stroke a path for drawing or saving */
int
gx_stroke_fill(const gx_path *ppath, gs_state *pgs)
{	return stroke(ppath, (gx_path *)0, stroke_fill, pgs);
}
int
gx_stroke_add(const gx_path *ppath, gx_path *to_path, gs_state *pgs)
{	int code = stroke(ppath, to_path, stroke_add, pgs);
	/* I don't understand why this code used to be here: */
#if 0
	if ( code < 0 ) return code;
	if ( ppath->subpath_open <= 0 && ppath->position_valid )
		code = gx_path_add_point(to_path, ppath->position.x,
					 ppath->position.y);
#endif
	return code;
}

/* Fill a partial stroked path. */
/* Free variables: code, to_path, ppath, stroke_path_body, pgs, exit(label). */
#define fill_stroke_path()\
if(to_path==&stroke_path_body && !gx_path_is_void_inline(&stroke_path_body))\
{ code = gx_fill_path(to_path, pgs->dev_color, pgs, gx_rule_winding_number,\
    stroke_adjustment(always_thin));\
  gx_path_release(to_path);\
  if ( code < 0 ) goto exit;\
  gx_path_init(to_path, ppath->memory);\
}

/* Stroke a path.  Call line_proc (stroke_add or stroke_fill) */
/* for each line segment. */
private int near
stroke(const gx_path *ppath, gx_path *to_path,
  int near (*line_proc)(P5(gx_path *, int, pl_ptr, pl_ptr, gs_state *)),
  gs_state *pgs)
{	int code = 0;
	const gx_line_params *lp = pgs->line_params;
	int dash_count = lp->dash.pattern_size;
	gx_path fpath, dpath;
	gx_path stroke_path_body;
	const gx_path *spath;
	float xx = pgs->ctm.xx, xy = pgs->ctm.xy;
	float yx = pgs->ctm.yx, yy = pgs->ctm.yy;
	bool skewed = !is_fzero2(xy, yx);
	int uniform = (skewed ? 0 : xx == yy ? 1 : xx == -yy ? -1 : 0);
	/*
	 * We are dealing with a reflected coordinate system
	 * if transform(1,0) is counter-clockwise from transform(0,1).
	 * See the note in stroke_add for the algorithm.
	 */	
	bool reflected =
	  (uniform ? uniform < 0 :
	   skewed ? xy * yx > xx * yy :
	   (xx < 0) != (yy < 0));
	float line_width = lp->width;	/* this is *half* the line width! */
	bool always_thin;
	double line_width_and_scale, line_width_scale_xx;
	const segment *pseg;
#ifdef DEBUG
if ( gs_debug_c('o') )
   {	int count = lp->dash.pattern_size;
	int i;
	dprintf3("[o]half_width=%f, cap=%d, join=%d,\n",
		 lp->width, (int)lp->cap, (int)lp->join);
	dprintf2("   miter_limit=%f, miter_check=%f,\n",
		 lp->miter_limit, lp->miter_check);
	dprintf1("   dash pattern=%d", count);
	for ( i = 0; i < count; i++ )
		dprintf1(",%f", lp->dash.pattern[i]);
	dprintf4(",\n	offset=%f, init(ink_on=%d, index=%d, dist_left=%f)\n",
		 lp->dash.offset, lp->dash.init_ink_on, lp->dash.init_index,
		 lp->dash.init_dist_left);
   }
#endif
	if ( line_width < 0 ) line_width = -line_width;
	if ( is_fzero(line_width) )
		always_thin = true;
	else if ( !skewed )
	   {	float xxa = xx, yya = yy;
		if ( xxa < 0 ) xxa = -xxa;
		if ( yya < 0 ) yya = -yya;
		always_thin = (max(xxa, yya) * line_width < 0.5);
	   }
	else
	   {	/* The check is more complicated, but it's worth it. */
		float xsq = xx * xx + xy * xy;
		float ysq = yx * yx + yy * yy;
		float cross = xx * yx + xy * yy;
		if ( cross < 0 ) cross = 0;
		always_thin =
		  ((max(xsq, ysq) + cross) * line_width * line_width < 0.5);
	   }
	line_width_and_scale = line_width * (double)int2fixed(1);
	if ( !always_thin && uniform )
	{	/* Precompute a value we'll need later. */
		line_width_scale_xx = line_width_and_scale * xx;
		if ( line_width_scale_xx < 0 )
		  line_width_scale_xx = -line_width_scale_xx;
	}
	if_debug5('o', "[o]ctm=(%g,%g,%g,%g) thin=%d\n",
		  xx, xy, yx, yy, always_thin);
	/* Start by flattening the path.  We should do this on-the-fly.... */
	if ( !ppath->curve_count )	/* don't need to flatten */
	   {	if ( !ppath->first_subpath )
		  return 0;
		spath = ppath;
	   }
	else
	   {	if ( (code = gx_path_flatten(ppath, &fpath, pgs->flatness,
					     pgs->in_cachedevice > 1)) < 0
		   )
		   return code;
		spath = &fpath;
	   }
	if ( dash_count )
	  {	code = gx_path_expand_dashes(spath, &dpath, pgs);
		if ( code < 0 )
		  goto exf;
		spath = &dpath;
	  }
	if ( to_path == 0 )
	  {	/* We might try to defer this if it's expensive.... */
		to_path = &stroke_path_body;
		gx_path_init(to_path, ppath->memory);
	  }
	for ( pseg = (const segment *)spath->first_subpath; pseg != 0; )
	  { int first = 0;
	    int index = 0;
	    fixed x = pseg->pt.x;
	    fixed y = pseg->pt.y;
	    bool is_closed = ((const subpath *)pseg)->is_closed;
	    partial_line pl, pl_prev, pl_first;
	    while ( (pseg = pseg->next) != 0 && pseg->type != s_start )
	      {	/* Compute the width parameters in device space. */
		/* We work with unscaled values, for speed. */
		fixed sx = pseg->pt.x, udx = sx - x;
		fixed sy = pseg->pt.y, udy = sy - y;
		pl.o.p.x = x, pl.o.p.y = y;
		pl.e.p.x = sx, pl.e.p.y = sy;
		if ( !(udx | udy) )	/* degenerate */
		  { /* Only consider a degenerate segment */
		    /* if the entire subpath is degenerate and */
		    /* we are using round caps or joins. */
		    if ( index != 0 || (pseg->next != 0 &&
					pseg->next->type != s_start) ||
			 (lp->cap != gs_cap_round &&
			  lp->join != gs_join_round)
		       )
		      goto nd;
		    /* Pick an arbitrary orientation. */
		    udx = int2fixed(1);
		  }
		if ( !always_thin )
		   {	if ( uniform != 0 )
			   {	/* We can save a lot of work in this case. */
				float dpx = udx, dpy = udy;
 				float wl = line_width_scale_xx /
					hypot(dpx, dpy);
				pl.e.cdelta.x = (fixed)(dpx * wl);
				pl.e.cdelta.y = (fixed)(dpy * wl);
				pl.width.x = -pl.e.cdelta.y;
				pl.width.y = pl.e.cdelta.x;
				pl.thin = false; /* if not always_thin, */
						/* then never thin. */
			   }
			else
			   {	gs_point dpt;	/* unscaled */
				float wl;
				gs_idtransform(pgs, (float)udx, (float)udy,
					       &dpt);
				wl = line_width_and_scale /
					hypot(dpt.x, dpt.y);
				/* Construct the width vector in */
				/* user space, still unscaled. */
				dpt.x *= wl;
				dpt.y *= wl;
				/*
				 * We now compute both perpendicular
				 * and (optionally) parallel half-widths,
				 * as deltas in device space.  We use
				 * a fixed-point, unscaled version of
				 * gs_dtransform.  The second computation
				 * folds in a 90-degree rotation (in user
				 * space, before transforming) in the
				 * direction that corresponds to counter-
				 * clockwise in device space.
				 */
				pl.e.cdelta.x = (fixed)(dpt.x * xx);
				pl.e.cdelta.y = (fixed)(dpt.y * yy);
				if ( skewed )
				  pl.e.cdelta.x += (fixed)(dpt.y * yx),
				  pl.e.cdelta.y += (fixed)(dpt.x * xy);
				if ( !reflected )
				  dpt.x = -dpt.x, dpt.y = -dpt.y;
				pl.width.x = (fixed)(dpt.y * xx),
				pl.width.y = -(fixed)(dpt.x * yy);
				if ( skewed )
				  pl.width.x -= (fixed)(dpt.x * yx),
				  pl.width.y += (fixed)(dpt.y * xy);
				pl.thin =
				  any_abs(pl.width.x) + any_abs(pl.width.y) <
				    float2fixed(0.75);
			   }
			if ( !pl.thin )
			{	adjust_stroke(&pl, pgs, false);
				compute_caps(&pl);
			}
		   }
		else			/* always_thin */
			pl.e.cdelta.x = pl.e.cdelta.y = 0,
			pl.width.x = pl.width.y = 0,
			pl.thin = true;
		if ( first++ == 0 ) pl_first = pl;
		if ( index++ )
		{	code = (*line_proc)(to_path,
					    (is_closed ? 1 : index - 2),
					    &pl_prev, &pl, pgs);
			if ( code < 0 )
			  goto exit;
			fill_stroke_path();
		}
		pl_prev = pl;
		x = sx, y = sy;
nd:		;
	      }
	    if ( index )
	      {	/* If closed, join back to start, else cap. */
		/* For some reason, the Borland compiler requires the cast */
		/* in the following line. */
		pl_ptr lptr = (is_closed ? (pl_ptr)&pl_first : (pl_ptr)0);
		code = (*line_proc)(to_path, index - 1, &pl_prev, lptr, pgs);
		if ( code < 0 )
		  goto exit;
		fill_stroke_path();
	      }
	 }
exit:	if ( to_path == &stroke_path_body )
	  gx_path_release(to_path);	/* (only needed if error) */
	if ( dash_count )
	  gx_path_release(&dpath);
exf:	if ( ppath->curve_count )
	  gx_path_release(&fpath);
	return code;
}

/* ------ Internal routines ------ */

/* Adjust the endpoints and width of a stroke segment */
/* to achieve more uniform rendering. */
/* Only o.p, e.p, e.cdelta, and width have been set. */
private void near
adjust_stroke(pl_ptr plp, gs_state *pgs, bool thin)
{	fixed _ss *pw;
	fixed _ss *pov;
	fixed _ss *pev;
	fixed w, w2;
	fixed adj2;
	if ( !pgs->stroke_adjust && plp->width.x != 0 && plp->width.y != 0 )
		return;		/* don't adjust */
	if ( any_abs(plp->width.x) < any_abs(plp->width.y) )
	{	/* More horizontal stroke */
		pw = &plp->width.y, pov = &plp->o.p.y, pev = &plp->e.p.y;
	}
	else
	{	/* More vertical stroke */
		pw = &plp->width.x, pov = &plp->o.p.x, pev = &plp->e.p.x;
	}
	/* Round the larger component of the width up or down, */
	/* whichever way produces a result closer to the correct width. */
	/* Note that just rounding the larger component */
	/* may not produce the correct result. */
	adj2 = stroke_adjustment(thin) << 1;
	w = *pw;
	w2 = fixed_rounded(w << 1);		/* full line width */
	if ( w2 == 0 && *pw != 0 )
	{	/* Make sure thin lines don't disappear. */
		w2 = (*pw < 0 ? -fixed_1 + adj2 : fixed_1 - adj2);
		*pw = arith_rshift_1(w2);
	}
	/* Only adjust the endpoints if the line is horizontal or vertical. */
	if ( *pov == *pev )
	{	/* We're going to round the endpoint coordinates, so */
		/* take the fill adjustment into account now. */
		if ( w >= 0 ) w2 += adj2;
		else w2 -= adj2;
		if ( w2 & fixed_1 )	/* odd width, move to half-pixel */
		{	*pov = *pev = fixed_floor(*pov) + fixed_half;
		}
		else			/* even width, move to pixel */
		{	*pov = *pev = fixed_rounded(*pov);
		}
	}
}

/* Compute the intersection of two lines.  This is a messy algorithm */
/* that somehow ought to be useful in more places than just here.... */
/* If the lines are (nearly) parallel, return -1 without setting *pi; */
/* otherwise, return 0 if the intersection is beyond *pp1 and *pp2 in */
/* the direction determined by *pd1 and *pd2, and 1 otherwise. */
private int
line_intersect(
    p_ptr pp1,				/* point on 1st line */
    p_ptr pd1,				/* slope of 1st line (dx,dy) */
    p_ptr pp2,				/* point on 2nd line */
    p_ptr pd2,				/* slope of 2nd line */
    p_ptr pi)				/* return intersection here */
{	/* We don't have to do any scaling, the factors all work out right. */
	float u1 = pd1->x, v1 = pd1->y;
	float u2 = pd2->x, v2 = pd2->y;
	double denom = u1 * v2 - u2 * v1;
	float xdiff = pp2->x - pp1->x;
	float ydiff = pp2->y - pp1->y;
	double f1;
	double max_result = any_abs(denom) * (double)max_fixed;
#ifdef DEBUG
if ( gs_debug_c('O') )
   {	dprintf4("[o]Intersect %f,%f(%f/%f)",
		 fixed2float(pp1->x), fixed2float(pp1->y),
		 fixed2float(pd1->x), fixed2float(pd1->y));
	dprintf4(" & %f,%f(%f/%f),\n",
		 fixed2float(pp2->x), fixed2float(pp2->y),
		 fixed2float(pd2->x), fixed2float(pd2->y));
	dprintf3("\txdiff=%f ydiff=%f denom=%f ->\n",
		 xdiff, ydiff, denom);
   }
#endif
	/* Check for degenerate result. */
	if ( any_abs(xdiff) >= max_result || any_abs(ydiff) >= max_result )
	   {	/* The lines are nearly parallel, */
		/* or one of them has zero length.  Punt. */
		if_debug0('O', "\tdegenerate!\n");
		return -1;
	   }
	f1 = (v2 * xdiff - u2 * ydiff) / denom;
	pi->x = pp1->x + (fixed)(f1 * u1);
	pi->y = pp1->y + (fixed)(f1 * v1);
	if_debug2('O', "\t%f,%f\n",
		  fixed2float(pi->x), fixed2float(pi->y));
	return (f1 >= 0 && (v1 * xdiff >= u1 * ydiff ? denom >= 0 : denom < 0) ? 0 : 1);
}

#define lix plp->o.p.x
#define liy plp->o.p.y
#define litox plp->e.p.x
#define litoy plp->e.p.y
#define trsign(v, c) ((v) >= 0 ? (c) : -(c))

/* Set up the width and delta parameters for a thin line. */
/* We only approximate the width and height. */
private void near
set_thin_widths(register pl_ptr plp)
{	fixed dx = litox - lix, dy = litoy - liy;
	if ( any_abs(dx) > any_abs(dy) )
	{	plp->width.x = plp->e.cdelta.y = 0;
		plp->width.y = plp->e.cdelta.x = trsign(dx, fixed_half);
	}
	else
	{	plp->width.y = plp->e.cdelta.x = 0;
		plp->width.x = -(plp->e.cdelta.y = trsign(dy, fixed_half));
	}
}

/* Draw a line on the device. */
private int near
stroke_fill(gx_path *ppath, int first, register pl_ptr plp, pl_ptr nplp,
  gs_state *pgs)
{	if ( plp->thin )
	   {	/* Minimum-width line, don't have to be careful. */
		/* We do have to check for the entire line being */
		/* within the clipping rectangle, allowing for some */
		/* slop at the ends. */
		fixed dx = litox - lix, dy = litoy - liy;
#define slop int2fixed(2)
		fixed xslop = trsign(dx, slop);
		fixed yslop = trsign(dy, slop);
		if ( gx_cpath_includes_rectangle(pgs->clip_path,
				lix - xslop, liy - yslop,
				litox + xslop, litoy + yslop) )
			return gx_draw_line_fixed(lix, liy, litox, litoy,
				pgs->dev_color, pgs);
#undef slop
#undef trsign
		/* We didn't set up the endpoint parameters before, */
		/* because the line was thin.  stroke_add will do this. */
	   }
	/* General case: construct a path for the fill algorithm. */
	return stroke_add(ppath, first, plp, nplp, pgs);
}

#undef lix
#undef liy
#undef litox
#undef litoy

/* Add a segment to the path.  This handles all the complex cases. */
private int near add_capped(P4(gx_path *, gs_line_cap, int (*)(P3(gx_path *, fixed, fixed)), ep_ptr));
private int near
stroke_add(gx_path *ppath, int first, register pl_ptr plp, pl_ptr nplp,
  gs_state *pgs)
{	int code;
	if ( plp->thin )
	   {	/* We didn't set up the endpoint parameters before, */
		/* because the line was thin.  Do it now. */
		set_thin_widths(plp);
		adjust_stroke(plp, pgs, true);
		compute_caps(plp);
	   }
	if ( (code = add_capped(ppath, (first == 0 ? pgs->line_params->cap : gs_cap_butt), gx_path_add_point, &plp->o)) < 0 )
		return code;
	if ( nplp == 0 )
	   {	code = add_capped(ppath, pgs->line_params->cap, gx_path_add_line, &plp->e);
	   }
	else if ( pgs->line_params->join == gs_join_round )
	   {	code = add_capped(ppath, gs_cap_round, gx_path_add_line, &plp->e);
	   }
	else if ( nplp->thin )		/* no join */
	  {	code = add_capped(ppath, gs_cap_butt, gx_path_add_line, &plp->e);
	  }
	else				/* join_bevel or join_miter */
	   {	gs_fixed_point join_points[4];
#define jp1 join_points[0]
#define np1 join_points[1]
#define np2 join_points[2]
#define jp2 join_points[3]
		/*
		 * Set np to whichever of nplp->o.co or .ce is outside
		 * the current line.  We observe that the point (x2,y2)
		 * is counter-clockwise from (x1,y1), relative to the origin,
		 * iff
		 *	(arctan(y2/x2) - arctan(y1/x1)) mod 2*pi < pi,
		 * taking the signs of xi and yi into account to determine
		 * the quadrants of the results.  It turns out that
		 * even though arctan is monotonic only in the 4th/1st
		 * quadrants and the 2nd/3rd quadrants, case analysis on
		 * the signs of xi and yi demonstrates that this test
		 * is equivalent to the much less expensive test
		 *	x1 * y2 > x2 * y1
		 * in all cases.
		 *
		 * In the present instance, x1,y1 are plp->width,
		 * x2,y2 are nplp->width, and the origin is
		 * their common point (plp->e.p, nplp->o.p).
		 */
		/* We make the test using double arithmetic only because */
		/* the !@#&^*% C language doesn't give us access to */
		/* the double-width-result multiplication operation */
		/* that almost all CPUs provide! */
		bool ccw =
		  (double)(plp->width.x) *	/* x1 */
		    (nplp->width.y) >		/* y2 */
		  (double)(nplp->width.x) *	/* x2 */
		    (plp->width.y);
		p_ptr outp, np;
		/* Initialize for a bevel join. */
		assign_point(&jp1, plp->e.co);
		assign_point(&jp2, plp->e.ce);
		if ( !ccw )
		  {	outp = &jp2;
			assign_point(&np2, nplp->o.co);
			assign_point(&np1, plp->e.p);
			np = &np2;
		  }
		else
		  {	outp = &jp1;
			assign_point(&np1, nplp->o.ce);
			assign_point(&np2, plp->e.p);
			np = &np1;
		  }
		if_debug1('O', "[o]use %s\n", (ccw ? "co (ccw)" : "ce (cw)"));
		/* Don't bother with the miter check if the two */
		/* points to be joined are very close together, */
		/* namely, in the same square half-pixel. */
		if ( pgs->line_params->join == gs_join_miter &&
		     !(fixed2long(outp->x << 1) == fixed2long(np->x << 1) &&
		       fixed2long(outp->y << 1) == fixed2long(np->y << 1))
		   )
		  { /*
		     * Check whether a miter join is appropriate.
		     * Let a, b be the angles of the two lines.
		     * We check tan(a-b) against the miter_check
		     * by using the following formula:
		     * If tan(a)=u1/v1 and tan(b)=u2/v2, then
		     * tan(a-b) = (u1*v2 - u2*v1) / (u1*u2 + v1*v2).
		     * We can do all the computations unscaled,
		     * because we're only concerned with ratios.
		     */
		    float u1 = plp->e.cdelta.y, v1 = plp->e.cdelta.x;
		    float u2 = nplp->o.cdelta.y, v2 = nplp->o.cdelta.x;
		    double num = u1 * v2 - u2 * v1;
		    double denom = u1 * u2 + v1 * v2;
		    float check = pgs->line_params->miter_check;
		    /*
		     * We will want either tan(a-b) or tan(b-a)
		     * depending on the orientations of the lines.
		     * Fortunately we know the relative orientations already.
		     */
		    if ( !ccw )		/* have plp - nplp, want vice versa */
			num = -num;
#ifdef DEBUG
if ( gs_debug_c('O') )
                   {	dprintf4("[o]Miter check: u1/v1=%f/%f, u2/v2=%f/%f,\n",
				 u1, v1, u2, v2);
			dprintf3("        num=%f, denom=%f, check=%f\n",
				 num, denom, check);
                   }
#endif
		    /*
		     * If we define T = num / denom, then we want to use
		     * a miter join iff arctan(T) >= arctan(check).
		     * We know that both of these angles are in the 1st
		     * or 2nd quadrant, and since arctan is monotonic
		     * within each quadrant, we can do the comparisons
		     * on T and check directly, taking signs into account
		     * as follows:
		     *		sign(T)	sign(check)	atan(T) >= atan(check)
		     *		-------	-----------	----------------------
		     *		+	+		T >= check
		     *		-	+		true
		     *		+	-		false
		     *		-	-		T >= check
		     */
		    if ( denom < 0 )
		      num = -num, denom = -denom;
		    /* Now denom >= 0, so sign(num) = sign(T). */
		    if ( check > 0 ?
			(num < 0 || num >= denom * check) :
			(num < 0 && num >= denom * check)
		       )
		    {	/* OK to use a miter join. */
			gs_fixed_point mpt;
			if_debug0('O', "	... passes.\n");
			/* Compute the intersection of */
			/* the extended edge lines. */
			if ( line_intersect(outp, &plp->e.cdelta, np,
					    &nplp->o.cdelta, &mpt) == 0
			   )
			  assign_point(outp, mpt);
		    }
		   }
		if ( (code = gx_path_add_lines(ppath, join_points, 4)) < 0 )
			return code;
	   }
	if ( code < 0 || (code = gx_path_close_subpath(ppath)) < 0 )
		return code;
	return 0;
}

/* Routines for cap computations */

/* Compute the endpoints of the two caps of a segment. */
private void near
compute_caps(register pl_ptr plp)
{	fixed wx2 = plp->width.x;
	fixed wy2 = plp->width.y;
	plp->o.co.x = plp->o.p.x + wx2, plp->o.co.y = plp->o.p.y + wy2;
	plp->o.cdelta.x = -plp->e.cdelta.x,
	  plp->o.cdelta.y = -plp->e.cdelta.y;
	plp->o.ce.x = plp->o.p.x - wx2, plp->o.ce.y = plp->o.p.y - wy2;
	plp->e.co.x = plp->e.p.x - wx2, plp->e.co.y = plp->e.p.y - wy2;
	plp->e.ce.x = plp->e.p.x + wx2, plp->e.ce.y = plp->e.p.y + wy2;
#ifdef DEBUG
if ( gs_debug_c('O') )
	dprintf4("[o]Stroke o=(%f,%f) e=(%f,%f)\n",
		 fixed2float(plp->o.p.x), fixed2float(plp->o.p.y),
		 fixed2float(plp->e.p.x), fixed2float(plp->e.p.y)),
	dprintf4("\twxy=(%f,%f) lxy=(%f,%f)\n",
		 fixed2float(wx2), fixed2float(wy2),
		 fixed2float(plp->e.cdelta.x), fixed2float(plp->e.cdelta.y));
#endif
}

/* Add a properly capped line endpoint to the path. */
/* The first point may require either moveto or lineto. */
private int near
add_capped(gx_path *ppath, gs_line_cap type,
  int (*add_proc)(P3(gx_path *, fixed, fixed)),	/* gx_path_add_point/line */
  register ep_ptr endp)
{	int code;
#define px endp->p.x
#define py endp->p.y
#define xo endp->co.x
#define yo endp->co.y
#define xe endp->ce.x
#define ye endp->ce.y
#define cdx endp->cdelta.x
#define cdy endp->cdelta.y
#ifdef DEBUG
if ( gs_debug_c('O') )
	dprintf4("[o]cap: p=(%g,%g), co=(%g,%g),\n",
		 fixed2float(px), fixed2float(py),
		 fixed2float(xo), fixed2float(yo)),
	dprintf4("[o]\tce=(%g,%g), cd=(%g,%g)\n",
		 fixed2float(xe), fixed2float(ye),
		 fixed2float(cdx), fixed2float(cdy));
#endif
	switch ( type )
	   {
	case gs_cap_round:
	   {	fixed xm = px + cdx;
		fixed ym = py + cdy;
		if (	(code = (*add_proc)(ppath, xo, yo)) < 0 ||
			(code = gx_path_add_arc(ppath, xo, yo, xm, ym,
				xo + cdx, yo + cdy, quarter_arc_fraction)) < 0 ||
			(code = gx_path_add_arc(ppath, xm, ym, xe, ye,
				xe + cdx, ye + cdy, quarter_arc_fraction)) < 0
		   ) return code;
	   }
		break;
	case gs_cap_square:
		if (	(code = (*add_proc)(ppath, xo + cdx, yo + cdy)) < 0 ||
			(code = gx_path_add_line(ppath, xe + cdx, ye + cdy)) < 0
		   ) return code;
		break;
	case gs_cap_butt:
		if (	(code = (*add_proc)(ppath, xo, yo)) < 0 ||
			(code = gx_path_add_line(ppath, xe, ye)) < 0
		   ) return code;
		break;
	default:		/* can't happen */
		return_error(gs_error_unregistered);
	   }
	return code;
}
