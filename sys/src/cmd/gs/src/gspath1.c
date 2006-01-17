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

/* $Id: gspath1.c,v 1.10 2004/08/31 13:23:16 igor Exp $ */
/* Additional PostScript Level 1 path routines for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxfarith.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gspath.h"
#include "gzpath.h"
#include "gscoord.h"		/* gs_itransform prototype */

/* ------ Arcs ------ */

/* Conversion parameters */
#define degrees_to_radians (M_PI / 180.0)

typedef enum {
    arc_nothing,
    arc_moveto,
    arc_lineto
} arc_action;

typedef struct arc_curve_params_s {
    /* The following are set once. */
    gx_path *ppath;
    gs_imager_state *pis;
    gs_point center;		/* (not used by arc_add) */
    double radius;
    /* The following may be updated dynamically. */
    arc_action action;
    segment_notes notes;
    gs_point p0, p3, pt;
    gs_sincos_t sincos;		/* (not used by arc_add) */
    fixed angle;		/* (not used by arc_add) */
    int fast_quadrant;		/* 0 = not calculated, -1 = not fast, */
				/* 1 = fast (only used for quadrants) */
    /* The following are set once iff fast_quadrant > 0. */
    fixed scaled_radius;	/* radius * CTM scale */
    fixed quadrant_delta;	/* scaled_radius * quarter_arc_fraction */
} arc_curve_params_t;

/* Forward declarations */
private int arc_add(const arc_curve_params_t *arc, bool is_quadrant);


int
gx_setcurrentpoint_from_path(gs_imager_state *pis, gx_path *path)
{
    gs_point pt;

    pt.x = fixed2float(path->position.x);
    pt.y = fixed2float(path->position.y);
    gx_setcurrentpoint(pis, pt.x, pt.y);
    pis->current_point_valid = true;
    return 0;
}

private inline int
gs_arc_add_inline(gs_state *pgs, bool cw, floatp xc, floatp yc, floatp rad, 
		    floatp a1, floatp a2, bool add)
{
    int code = gs_imager_arc_add(pgs->path, (gs_imager_state *)pgs, cw, xc, yc, rad, a1, a2, add);

    if (code < 0)
	return code;
    return gx_setcurrentpoint_from_path((gs_imager_state *)pgs, pgs->path);
}

int
gs_arc(gs_state * pgs,
       floatp xc, floatp yc, floatp r, floatp ang1, floatp ang2)
{
    return gs_arc_add_inline(pgs, false, xc, yc, r, ang1, ang2, true);
}

int
gs_arcn(gs_state * pgs,
	floatp xc, floatp yc, floatp r, floatp ang1, floatp ang2)
{
    return gs_arc_add_inline(pgs, true, xc, yc, r, ang1, ang2, true);
}

int
gs_arc_add(gs_state * pgs, bool clockwise, floatp axc, floatp ayc,
	   floatp arad, floatp aang1, floatp aang2, bool add_line)
{
    return gs_arc_add_inline(pgs, clockwise, axc, ayc, arad,
			     aang1, aang2, add_line);
}

/* Compute the next curve as part of an arc. */
private int
next_arc_curve(arc_curve_params_t * arc, fixed anext)
{
    double x0 = arc->p0.x = arc->p3.x;
    double y0 = arc->p0.y = arc->p3.y;
    double trad = arc->radius *
	tan(fixed2float(anext - arc->angle) *
	    (degrees_to_radians / 2));

    arc->pt.x = x0 - trad * arc->sincos.sin;
    arc->pt.y = y0 + trad * arc->sincos.cos;
    gs_sincos_degrees(fixed2float(anext), &arc->sincos);
    arc->p3.x = arc->center.x + arc->radius * arc->sincos.cos;
    arc->p3.y = arc->center.y + arc->radius * arc->sincos.sin;
    arc->angle = anext;
    return arc_add(arc, false);
}
/*
 * Use this when both arc.angle and anext are multiples of 90 degrees,
 * and anext = arc.angle +/- 90.
 */
private int
next_arc_quadrant(arc_curve_params_t * arc, fixed anext)
{
    double x0 = arc->p0.x = arc->p3.x;
    double y0 = arc->p0.y = arc->p3.y;

    if (!arc->fast_quadrant) {
	/*
	 * If the CTM is well-behaved, we can pre-calculate the delta
	 * from the arc points to the control points.
	 */
	const gs_imager_state *pis = arc->pis;
	double scale;

	if (is_fzero2(pis->ctm.xy, pis->ctm.yx) ?
	    (scale = fabs(pis->ctm.xx)) == fabs(pis->ctm.yy) :
	    is_fzero2(pis->ctm.xx, pis->ctm.yy) ?
	    (scale = fabs(pis->ctm.xy)) == fabs(pis->ctm.yx) :
	    0
	    ) {
	    double scaled_radius = arc->radius * scale;

	    arc->scaled_radius = float2fixed(scaled_radius);
	    arc->quadrant_delta =
		float2fixed(scaled_radius * quarter_arc_fraction);
	    arc->fast_quadrant = 1;
	} else {
	    arc->fast_quadrant = -1;
	}
    }
    /*
     * We know that anext is a multiple of 90 (as a fixed); we want
     * (anext / 90) & 3.  The following is much faster than a division.
     */
    switch ((fixed2int(anext) >> 1) & 3) {
    case 0:
	arc->sincos.sin = 0, arc->sincos.cos = 1;
	arc->p3.x = x0 = arc->center.x + arc->radius;
	arc->p3.y = arc->center.y;
	break;
    case 1:
	arc->sincos.sin = 1, arc->sincos.cos = 0;
	arc->p3.x = arc->center.x;
	arc->p3.y = y0 = arc->center.y + arc->radius;
	break;
    case 2:
	arc->sincos.sin = 0, arc->sincos.cos = -1;
	arc->p3.x = x0 = arc->center.x - arc->radius;
	arc->p3.y = arc->center.y;
	break;
    case 3:
	arc->sincos.sin = -1, arc->sincos.cos = 0;
	arc->p3.x = arc->center.x;
	arc->p3.y = y0 = arc->center.y - arc->radius;
	break;
    }
    arc->pt.x = x0, arc->pt.y = y0;
    arc->angle = anext;
    return arc_add(arc, true);
}

int
gs_imager_arc_add(gx_path * ppath, gs_imager_state * pis, bool clockwise,
	    floatp axc, floatp ayc, floatp arad, floatp aang1, floatp aang2,
		  bool add_line)
{
    double ar = arad;
    fixed ang1 = float2fixed(aang1), ang2 = float2fixed(aang2), anext;
    double ang1r;		/* reduced angle */
    arc_curve_params_t arc;
    int code;

    arc.ppath = ppath;
    arc.pis = pis;
    arc.center.x = axc;
    arc.center.y = ayc;
#define fixed_90 int2fixed(90)
#define fixed_180 int2fixed(180)
#define fixed_360 int2fixed(360)
    if (ar < 0) {
	ang1 += fixed_180;
	ang2 += fixed_180;
	ar = -ar;
    }
    arc.radius = ar;
    arc.action = (add_line ? arc_lineto : arc_moveto);
    arc.notes = sn_none;
    arc.fast_quadrant = 0;
    ang1r = fixed2float(ang1 % fixed_360);
    gs_sincos_degrees(ang1r, &arc.sincos);
    arc.p3.x = axc + ar * arc.sincos.cos;
    arc.p3.y = ayc + ar * arc.sincos.sin;
    if (clockwise) {
	while (ang1 < ang2)
	    ang2 -= fixed_360;
	if (ang2 < 0) {
	    fixed adjust = ROUND_UP(-ang2, fixed_360);

	    ang1 += adjust, ang2 += adjust;
	}
	arc.angle = ang1;
	if (ang1 == ang2)
	    goto last;
	/* Do the first part, up to a multiple of 90 degrees. */
	if (!arc.sincos.orthogonal) {
	    anext = ROUND_DOWN(arc.angle - fixed_epsilon, fixed_90);
	    if (anext < ang2)
		goto last;
	    code = next_arc_curve(&arc, anext);
	    if (code < 0)
		return code;
	    arc.action = arc_nothing;
	    arc.notes = sn_not_first;
	}	    
	/* Do multiples of 90 degrees.  Invariant: ang1 >= ang2 >= 0. */
	while ((anext = arc.angle - fixed_90) >= ang2) {
	    code = next_arc_quadrant(&arc, anext);
	    if (code < 0)
		return code;
	    arc.action = arc_nothing;
	    arc.notes = sn_not_first;
	}
    } else {
	while (ang2 < ang1)
	    ang2 += fixed_360;
	if (ang1 < 0) {
	    fixed adjust = ROUND_UP(-ang1, fixed_360);

	    ang1 += adjust, ang2 += adjust;
	}
	arc.angle = ang1;
	if (ang1 == ang2)
	    return next_arc_curve(&arc, ang2);
	/* Do the first part, up to a multiple of 90 degrees. */
	if (!arc.sincos.orthogonal) {
	    anext = ROUND_UP(arc.angle + fixed_epsilon, fixed_90);
	    if (anext > ang2)
		goto last;
	    code = next_arc_curve(&arc, anext);
	    if (code < 0)
		return code;
	    arc.action = arc_nothing;
	    arc.notes = sn_not_first;
	}	    
	/* Do multiples of 90 degrees.  Invariant: 0 <= ang1 <= ang2. */
	while ((anext = arc.angle + fixed_90) <= ang2) {
	    code = next_arc_quadrant(&arc, anext);
	    if (code < 0)
		return code;
	    arc.action = arc_nothing;
	    arc.notes = sn_not_first;
	}
    }
    /*
     * Do the last curve of the arc, if any.
     */
    if (arc.angle == ang2)
	return 0;
last:
    return next_arc_curve(&arc, ang2);
}

int
gs_arcto(gs_state * pgs,
floatp ax1, floatp ay1, floatp ax2, floatp ay2, floatp arad, float retxy[4])
{
    double xt0, yt0, xt2, yt2;
    gs_point up0;

#define ax0 up0.x
#define ay0 up0.y
    /* Transform the current point back into user coordinates. */
    int code = gs_currentpoint(pgs, &up0);

    if (code < 0)
	return code;
    {				/* Now we have to compute the tangent points. */
	/* Basically, the idea is to compute the tangent */
	/* of the bisector by using tan(x+y) and tan(z/2) */
	/* formulas, without ever using any trig. */
	double dx0 = ax0 - ax1, dy0 = ay0 - ay1;
	double dx2 = ax2 - ax1, dy2 = ay2 - ay1;

	/* Compute the squared lengths from p1 to p0 and p2. */
	double sql0 = dx0 * dx0 + dy0 * dy0;
	double sql2 = dx2 * dx2 + dy2 * dy2;

	/* Compute the distance from p1 to the tangent points. */
	/* This is the only messy part. */
	double num = dy0 * dx2 - dy2 * dx0;
	double denom = sqrt(sql0 * sql2) - (dx0 * dx2 + dy0 * dy2);

	/* Check for collinear points. */
	if (denom == 0) {
	    code = gs_lineto(pgs, ax1, ay1);
	    xt0 = xt2 = ax1;
	    yt0 = yt2 = ay1;
	} else {		/* not collinear */
	    double dist = fabs(arad * num / denom);
	    double l0 = dist / sqrt(sql0), l2 = dist / sqrt(sql2);
	    arc_curve_params_t arc;

	    arc.ppath = pgs->path;
	    arc.pis = (gs_imager_state *) pgs;
	    arc.radius = arad;
	    arc.action = arc_lineto;
	    arc.notes = sn_none;
	    if (arad < 0)
		l0 = -l0, l2 = -l2;
	    arc.p0.x = xt0 = ax1 + dx0 * l0;
	    arc.p0.y = yt0 = ay1 + dy0 * l0;
	    arc.p3.x = xt2 = ax1 + dx2 * l2;
	    arc.p3.y = yt2 = ay1 + dy2 * l2;
	    arc.pt.x = ax1;
	    arc.pt.y = ay1;
	    code = arc_add(&arc, false);
	    if (code == 0)
		code = gx_setcurrentpoint_from_path((gs_imager_state *)pgs, pgs->path);
	}
    }
    if (retxy != 0) {
	retxy[0] = xt0;
	retxy[1] = yt0;
	retxy[2] = xt2;
	retxy[3] = yt2;
    }
    return code;
}

/* Internal routine for adding an arc to the path. */
private int
arc_add(const arc_curve_params_t * arc, bool is_quadrant)
{
    gx_path *path = arc->ppath;
    gs_imager_state *pis = arc->pis;
    double x0 = arc->p0.x, y0 = arc->p0.y;
    double xt = arc->pt.x, yt = arc->pt.y;
    floatp fraction;
    gs_fixed_point p0, p2, p3, pt;
    int code;

    if ((arc->action != arc_nothing &&
#if !PRECISE_CURRENTPOINT
	 (code = gs_point_transform2fixed(&pis->ctm, x0, y0, &p0)) < 0) ||
	(code = gs_point_transform2fixed(&pis->ctm, xt, yt, &pt)) < 0 ||
	(code = gs_point_transform2fixed(&pis->ctm, arc->p3.x, arc->p3.y, &p3)) < 0 ||
#else
	 (code = gs_point_transform2fixed_rounding(&pis->ctm, x0, y0, &p0)) < 0) ||
	(code = gs_point_transform2fixed_rounding(&pis->ctm, xt, yt, &pt)) < 0 ||
	(code = gs_point_transform2fixed_rounding(&pis->ctm, arc->p3.x, arc->p3.y, &p3)) < 0 ||
#endif
	(code =
	 (arc->action == arc_nothing ?
	  (p0.x = path->position.x, p0.y = path->position.y, 0) :
	  arc->action == arc_lineto && path_position_valid(path) ?
	  gx_path_add_line(path, p0.x, p0.y) :
	  /* action == arc_moveto, or lineto with no current point */
	  gx_path_add_point(path, p0.x, p0.y))) < 0
	)
	return code;
    /* Compute the fraction coefficient for the curve. */
    /* See gx_path_add_partial_arc for details. */
    if (is_quadrant) {
	/* one of |dx| and |dy| is r, the other is zero */
	fraction = quarter_arc_fraction;
	if (arc->fast_quadrant > 0) {
	    /*
	     * The CTM is well-behaved, and we have pre-calculated the delta
	     * from the circumference points to the control points.
	     */
	    fixed delta = arc->quadrant_delta;

	    if (pt.x != p0.x)
		p0.x = (pt.x > p0.x ? p0.x + delta : p0.x - delta);
	    if (pt.y != p0.y)
		p0.y = (pt.y > p0.y ? p0.y + delta : p0.y - delta);
	    p2.x = (pt.x == p3.x ? p3.x :
		    pt.x > p3.x ? p3.x + delta : p3.x - delta);
	    p2.y = (pt.y == p3.y ? p3.y :
		    pt.y > p3.y ? p3.y + delta : p3.y - delta);
	    goto add;
	}
    } else {
	double r = arc->radius;
	floatp dx = xt - x0, dy = yt - y0;
	double dist = dx * dx + dy * dy;
	double r2 = r * r;

	if (dist >= r2 * 1.0e8)	/* almost zero radius; */
	    /* the >= catches dist == r == 0 */
	    fraction = 0.0;
	else
	    fraction = (4.0 / 3.0) / (1 + sqrt(1 + dist / r2));
    }
    p0.x += (fixed)((pt.x - p0.x) * fraction);
    p0.y += (fixed)((pt.y - p0.y) * fraction);
    p2.x = p3.x + (fixed)((pt.x - p3.x) * fraction);
    p2.y = p3.y + (fixed)((pt.y - p3.y) * fraction);
add:
    if_debug8('r',
	      "[r]Arc f=%f p0=(%f,%f) pt=(%f,%f) p3=(%f,%f) action=%d\n",
	      fraction, x0, y0, xt, yt, arc->p3.x, arc->p3.y,
	      (int)arc->action);
    /* Open-code gx_path_add_partial_arc_notes */
    return gx_path_add_curve_notes(path, p0.x, p0.y, p2.x, p2.y, p3.x, p3.y,
				   arc->notes | sn_from_arc);
}

void
make_quadrant_arc(gs_point *p, const gs_point *c, 
	const gs_point *p0, const gs_point *p1, double r)
{
    p[0].x = c->x + p0->x * r;
    p[0].y = c->y + p0->y * r;
    p[1].x = c->x + p0->x * r + p1->x * r * quarter_arc_fraction;
    p[1].y = c->y + p0->y * r + p1->y * r * quarter_arc_fraction;
    p[2].x = c->x + p0->x * r * quarter_arc_fraction + p1->x * r;
    p[2].y = c->y + p0->y * r * quarter_arc_fraction + p1->y * r;
    p[3].x = c->x + p1->x * r;
    p[3].y = c->y + p1->y * r;
}


/* ------ Path transformers ------ */

int
gs_dashpath(gs_state * pgs)
{
    gx_path *ppath;
    gx_path fpath;
    int code;

    if (gs_currentdash_length(pgs) == 0)
	return 0;		/* no dash pattern */
    code = gs_flattenpath(pgs);
    if (code < 0)
	return code;
    ppath = pgs->path;
    gx_path_init_local(&fpath, ppath->memory);
    code = gx_path_add_dash_expansion(ppath, &fpath, (gs_imager_state *)pgs);
    if (code < 0) {
	gx_path_free(&fpath, "gs_dashpath");
	return code;
    }
    gx_path_assign_free(pgs->path, &fpath);
    return 0;
}

int
gs_flattenpath(gs_state * pgs)
{
    gx_path *ppath = pgs->path;
    gx_path fpath;
    int code;

    if (!gx_path_has_curves(ppath))
	return 0;		/* nothing to do */
    gx_path_init_local(&fpath, ppath->memory);
    code = gx_path_add_flattened_accurate(ppath, &fpath, pgs->flatness,
					  pgs->accurate_curves);
    if (code < 0) {
	gx_path_free(&fpath, "gs_flattenpath");
	return code;
    }
    gx_path_assign_free(ppath, &fpath);
    return 0;
}

int
gs_reversepath(gs_state * pgs)
{
    gx_path *ppath = pgs->path;
    gx_path rpath;
    int code;

    gx_path_init_local(&rpath, ppath->memory);
    code = gx_path_copy_reversed(ppath, &rpath);
    if (code < 0) {
	gx_path_free(&rpath, "gs_reversepath");
	return code;
    }
    if (pgs->current_point_valid) {
	/* Not empty. */
	gx_setcurrentpoint(pgs, fixed2float(rpath.position.x), 
				fixed2float(rpath.position.y));
	pgs->subpath_start.x = fixed2float(rpath.segments->contents.subpath_current->pt.x);
	pgs->subpath_start.y = fixed2float(rpath.segments->contents.subpath_current->pt.y);
    }
    gx_path_assign_free(ppath, &rpath);
    return 0;
}

/* ------ Accessors ------ */

int
gs_upathbbox(gs_state * pgs, gs_rect * pbox, bool include_moveto)
{
    gs_fixed_rect fbox;		/* box in device coordinates */
    gs_rect dbox;
    int code = gx_path_bbox_set(pgs->path, &fbox);

    if (code < 0)
	return code;
    /* If the path ends with a moveto and include_moveto is true, */
    /* include the moveto in the bounding box. */
    if (path_last_is_moveto(pgs->path) && include_moveto) {
	gs_fixed_point pt;

	gx_path_current_point_inline(pgs->path, &pt);
	if (pt.x < fbox.p.x)
	    fbox.p.x = pt.x;
	if (pt.y < fbox.p.y)
	    fbox.p.y = pt.y;
	if (pt.x > fbox.q.x)
	    fbox.q.x = pt.x;
	if (pt.y > fbox.q.y)
	    fbox.q.y = pt.y;
    }
    /* Transform the result back to user coordinates. */
    dbox.p.x = fixed2float(fbox.p.x);
    dbox.p.y = fixed2float(fbox.p.y);
    dbox.q.x = fixed2float(fbox.q.x);
    dbox.q.y = fixed2float(fbox.q.y);
    return gs_bbox_transform_inverse(&dbox, &ctm_only(pgs), pbox);
}

/* ------ Enumerators ------ */

/* Start enumerating a path */
int
gs_path_enum_copy_init(gs_path_enum * penum, const gs_state * pgs, bool copy)
{
    gs_memory_t *mem = pgs->memory;

    if (copy) {
	gx_path *copied_path =
	gx_path_alloc(mem, "gs_path_enum_init");
	int code;

	if (copied_path == 0)
	    return_error(gs_error_VMerror);
	code = gx_path_copy(pgs->path, copied_path);
	if (code < 0) {
	    gx_path_free(copied_path, "gs_path_enum_init");
	    return code;
	}
	gx_path_enum_init(penum, copied_path);
	penum->copied_path = copied_path;
    } else {
	gx_path_enum_init(penum, pgs->path);
    }
    penum->memory = mem;
    gs_currentmatrix(pgs, &penum->mat);
    return 0;
}

/* Enumerate the next element of a path. */
/* If the path is finished, return 0; */
/* otherwise, return the element type. */
int
gs_path_enum_next(gs_path_enum * penum, gs_point ppts[3])
{
    gs_fixed_point fpts[3];
    int pe_op = gx_path_enum_next(penum, fpts);
    int code;

    switch (pe_op) {
	case 0:		/* all done */
	case gs_pe_closepath:
	    break;
	case gs_pe_curveto:
	    if ((code = gs_point_transform_inverse(
						      fixed2float(fpts[1].x),
						      fixed2float(fpts[1].y),
					      &penum->mat, &ppts[1])) < 0 ||
		(code = gs_point_transform_inverse(
						      fixed2float(fpts[2].x),
						      fixed2float(fpts[2].y),
						&penum->mat, &ppts[2])) < 0)
		return code;
	    /* falls through */
	case gs_pe_moveto:
	case gs_pe_lineto:
	    if ((code = gs_point_transform_inverse(
						      fixed2float(fpts[0].x),
						      fixed2float(fpts[0].y),
						&penum->mat, &ppts[0])) < 0)
		return code;
	default:		/* error */
	    break;
    }
    return pe_op;
}

/* Clean up after a pathforall. */
void
gs_path_enum_cleanup(gs_path_enum * penum)
{
    if (penum->copied_path != 0) {
	gx_path_free(penum->copied_path, "gs_path_enum_cleanup");
	penum->path = 0;
	penum->copied_path = 0;
    }
}
