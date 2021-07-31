/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxshade1.c,v 1.2 2000/03/17 08:17:55 lpd Exp $ */
/* Rendering for non-mesh shadings */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gspath.h"
#include "gxcspace.h"
#include "gxdcolor.h"
#include "gxfarith.h"
#include "gxfixed.h"
#include "gxistate.h"
#include "gxpath.h"
#include "gxshade.h"

/* ================ Utilities ================ */

/* Check whether 2 colors fall within the smoothness criterion. */
private bool
shade_colors2_converge(const gs_client_color cc[2],
		       const shading_fill_state_t * pfs)
{
    int ci;

    for (ci = pfs->num_components - 1; ci >= 0; --ci)
	if (fabs(cc[1].paint.values[ci] - cc[0].paint.values[ci]) >
	    pfs->cc_max_error[ci]
	    )
	    return false;
    return true;
}

/* Fill a user space rectangle that is also a device space rectangle. */
private int
shade_fill_device_rectangle(const shading_fill_state_t * pfs,
			    const gs_fixed_point * p0,
			    const gs_fixed_point * p1,
			    gx_device_color * pdevc)
{
    gs_imager_state *pis = pfs->pis;
    fixed xmin, ymin, xmax, ymax;
    int x, y;

    if (p0->x < p1->x)
	xmin = p0->x, xmax = p1->x;
    else
	xmin = p1->x, xmax = p0->x;
    if (p0->y < p1->y)
	ymin = p0->y, ymax = p1->y;
    else
	ymin = p1->y, ymax = p0->y;
    /****** NOT QUITE RIGHT FOR PIXROUND ******/
    xmin -= pis->fill_adjust.x;
    xmax += pis->fill_adjust.x;
    ymin -= pis->fill_adjust.y;
    ymax += pis->fill_adjust.y;
    x = fixed2int_var(xmin);
    y = fixed2int_var(ymin);
    return
	gx_fill_rectangle_device_rop(x, y,
				     fixed2int_var(xmax) - x,
				     fixed2int_var(ymax) - y,
				     pdevc, pfs->dev, pis->log_op);
}

/* ================ Specific shadings ================ */

/* ---------------- Function-based shading ---------------- */

#define Fb_max_depth 32		/* 16 bits each in X and Y */
typedef struct Fb_frame_s {	/* recursion frame */
    gs_rect region;
    gs_client_color cc[4];	/* colors at 4 corners */
} Fb_frame_t;

typedef struct Fb_fill_state_s {
    shading_fill_state_common;
    const gs_shading_Fb_t *psh;
    gs_matrix_fixed ptm;	/* parameter space -> device space */
    bool orthogonal;		/* true iff ptm is xxyy or xyyx */
    int depth;			/* 1 <= depth < Fb_max_depth */
    Fb_frame_t frames[Fb_max_depth];
} Fb_fill_state_t;
/****** NEED GC DESCRIPTOR ******/

private int
Fb_fill_region(Fb_fill_state_t * pfs)
{
    const gs_shading_Fb_t * const psh = pfs->psh;
    gs_imager_state *pis = pfs->pis;
    Fb_frame_t *fp = &pfs->frames[pfs->depth - 1];

    for (;;) {
	const double
	    x0 = fp->region.p.x, y0 = fp->region.p.y,
	    x1 = fp->region.q.x, y1 = fp->region.q.y;

	if (!shade_colors4_converge(fp->cc,
				    (const shading_fill_state_t *)pfs) &&
	    fp < &pfs->frames[countof(pfs->frames) - 1]
	    ) {
	    /*
	     * The colors don't converge.  Does the region color more than
	     * a single pixel?
	     */
	    gs_rect region;

	    gs_bbox_transform(&fp->region, (const gs_matrix *)&pfs->ptm,
			      &region);
	    if (region.q.x - region.p.x > 1 || region.q.y - region.p.y > 1)
		goto recur;
	    {
		/*
		 * More precisely, does the bounding box of the region,
		 * taking fill adjustment into account, span more than 1
		 * pixel center in either X or Y?
		 */
		fixed ax = pis->fill_adjust.x;
		int nx =
		    fixed2int_pixround(float2fixed(region.q.x) + ax) -
		    fixed2int_pixround(float2fixed(region.p.x) - ax);
		fixed ay = pis->fill_adjust.y;
		int ny =
		    fixed2int_pixround(float2fixed(region.q.y) + ay) -
		    fixed2int_pixround(float2fixed(region.p.y) - ay);

		if ((nx > 1 && ny != 0) || (ny > 1 && nx != 0))
		    goto recur;
	    }
	    /* We could do the 1-pixel case a lot faster! */
	}
	/* Fill the region with the color. */
	{
	    gx_device_color dev_color;
	    const gs_color_space *pcs = psh->params.ColorSpace;
	    gs_client_color cc;
	    gs_fixed_point pts[4];
	    int code;

	    if_debug0('|', "[|]... filling region\n");
	    cc = fp->cc[0];
	    (*pcs->type->restrict_color)(&cc, pcs);
	    (*pcs->type->remap_color)(&cc, pcs, &dev_color, pis,
				      pfs->dev, gs_color_select_texture);
	    gs_point_transform2fixed(&pfs->ptm, x0, y0, &pts[0]);
	    gs_point_transform2fixed(&pfs->ptm, x1, y1, &pts[2]);
	    if (pfs->orthogonal) {
		code =
		    shade_fill_device_rectangle((const shading_fill_state_t *)pfs,
						&pts[0], &pts[2], &dev_color);
	    } else {
		gx_path *ppath = gx_path_alloc(pis->memory, "Fb_fill");

		gs_point_transform2fixed(&pfs->ptm, x1, y0, &pts[1]);
		gs_point_transform2fixed(&pfs->ptm, x0, y1, &pts[3]);
		gx_path_add_point(ppath, pts[0].x, pts[0].y);
		gx_path_add_lines(ppath, pts + 1, 3);
		code = shade_fill_path((const shading_fill_state_t *)pfs,
				       ppath, &dev_color);
		gx_path_free(ppath, "Fb_fill");
	    }
	    if (code < 0 || fp == &pfs->frames[0])
		return code;
	    --fp;
	    continue;
	}

	/*
	 * No luck.  Subdivide the region and recur.
	 *
	 * We should subdivide on the axis that has the largest color
	 * discrepancy, but for now we subdivide on the axis with the
	 * largest coordinate difference.
	 */
    recur:
	{
	    gs_function_t *pfn = psh->params.Function;
	    float v[2];
	    int code;

	    if (fabs(y1 - y0) > fabs(x1 - x0)) {
		/* Subdivide in Y. */
		double ym = (y0 + y1) * 0.5;

		if_debug1('|', "[|]dividing at y=%g\n", ym);
		v[1] = ym;
		v[0] = x0;
		code = gs_function_evaluate(pfn, v, fp[1].cc[2].paint.values);
		if (code < 0)
		    return code;
		v[0] = x1;
		code = gs_function_evaluate(pfn, v, fp[1].cc[3].paint.values);
		fp[1].region.q.x = x1;
		fp[1].region.q.y = fp->region.p.y = ym;
		fp[1].cc[0].paint = fp->cc[0].paint;
		fp[1].cc[1].paint = fp->cc[1].paint;
		fp->cc[0].paint = fp[1].cc[2].paint;
		fp->cc[1].paint = fp[1].cc[3].paint;
	    } else {
		/* Subdivide in X. */
		double xm = (x0 + x1) * 0.5;

		if_debug1('|', "[|]dividing at x=%g\n", xm);
		v[0] = xm;
		v[1] = y0;
		code = gs_function_evaluate(pfn, v, fp[1].cc[1].paint.values);
		if (code < 0)
		    return code;
		v[1] = y1;
		code = gs_function_evaluate(pfn, v, fp[1].cc[3].paint.values);
		fp[1].region.q.x = fp->region.p.x = xm;
		fp[1].region.q.y = y1;
		fp[1].cc[0].paint = fp->cc[0].paint;
		fp[1].cc[2].paint = fp->cc[2].paint;
		fp->cc[0].paint = fp[1].cc[1].paint;
		fp->cc[2].paint = fp[1].cc[3].paint;
	    }
	    if (code < 0)
		return code;
	    fp[1].region.p.x = x0, fp[1].region.p.y = y0;
	    ++fp;
	}
    }
}

int
gs_shading_Fb_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			     gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_Fb_t * const psh = (const gs_shading_Fb_t *)psh0;
    gs_matrix save_ctm;
    int xi, yi;
    float x[2], y[2];
    Fb_fill_state_t state;

    shade_init_fill_state((shading_fill_state_t *) & state, psh0, dev, pis);
    state.psh = psh;
    /****** HACK FOR FIXED-POINT MATRIX MULTIPLY ******/
    gs_currentmatrix((gs_state *) pis, &save_ctm);
    gs_concat((gs_state *) pis, &psh->params.Matrix);
    state.ptm = pis->ctm;
    gs_setmatrix((gs_state *) pis, &save_ctm);
    state.orthogonal = is_xxyy(&state.ptm) || is_xyyx(&state.ptm);
    /* Compute the parameter X and Y ranges. */
    {
	gs_rect pbox;

	gs_bbox_transform_inverse(rect, &psh->params.Matrix, &pbox);
	x[0] = max(pbox.p.x, psh->params.Domain[0]);
	x[1] = min(pbox.q.x, psh->params.Domain[1]);
	y[0] = max(pbox.p.y, psh->params.Domain[2]);
	y[1] = min(pbox.q.y, psh->params.Domain[3]);
    }
    for (xi = 0; xi < 2; ++xi)
	for (yi = 0; yi < 2; ++yi) {
	    float v[2];

	    v[0] = x[xi], v[1] = y[yi];
	    gs_function_evaluate(psh->params.Function, v,
				 state.frames[0].cc[yi * 2 + xi].paint.values);
	}
    state.frames[0].region.p.x = x[0];
    state.frames[0].region.p.y = y[0];
    state.frames[0].region.q.x = x[1];
    state.frames[0].region.q.y = y[1];
    state.depth = 1;
    return Fb_fill_region(&state);
}

/* ---------------- Axial shading ---------------- */

/*
 * Note that the max recursion depth and the frame structure are shared
 * with radial shading.
 */
#define AR_max_depth 16
typedef struct AR_frame_s {	/* recursion frame */
    double t0, t1;
    gs_client_color cc[2];	/* color at t0, t1 */
} AR_frame_t;

#define A_max_depth AR_max_depth
typedef AR_frame_t A_frame_t;

typedef struct A_fill_state_s {
    shading_fill_state_common;
    const gs_shading_A_t *psh;
    bool orthogonal;		/* true iff ctm is xxyy or xyyx */
    gs_rect rect;
    gs_point delta;
    double length, dd;
    int depth;
    A_frame_t frames[A_max_depth];
} A_fill_state_t;
/****** NEED GC DESCRIPTOR ******/

/* Note t0 and t1 vary over [0..1], not the Domain. */

private int
A_fill_stripe(const A_fill_state_t * pfs, gs_client_color *pcc,
	      floatp t0, floatp t1)
{
    const gs_shading_A_t * const psh = pfs->psh;
    gx_device_color dev_color;
    const gs_color_space *pcs = psh->params.ColorSpace;
    gs_imager_state *pis = pfs->pis;
    double
	x0 = psh->params.Coords[0] + pfs->delta.x * t0,
	y0 = psh->params.Coords[1] + pfs->delta.y * t0;
    double
	x1 = psh->params.Coords[0] + pfs->delta.x * t1,
	y1 = psh->params.Coords[1] + pfs->delta.y * t1;
    gs_fixed_point pts[4];
    int code;

    (*pcs->type->restrict_color)(pcc, pcs);
    (*pcs->type->remap_color)(pcc, pcs, &dev_color, pis,
			      pfs->dev, gs_color_select_texture);
    if (x0 == x1 && pfs->orthogonal) {
	/* Stripe is horizontal in both user and device space. */
	x0 = pfs->rect.p.x;
	x1 = pfs->rect.q.x;
    } else if (y0 == y1 && pfs->orthogonal) {
	/* Stripe is vertical in both user and device space space. */
	y0 = pfs->rect.p.y;
	y1 = pfs->rect.q.y;
    } else {
	/*
	 * Stripe is neither horizontal nor vertical.
	 * Extend it to the edges of the rectangle.
	 */
	gx_path *ppath = gx_path_alloc(pis->memory, "A_fill");
	double dist = max(pfs->rect.q.x - pfs->rect.p.x,
			  pfs->rect.q.y - pfs->rect.p.y);
	double denom = hypot(pfs->delta.x, pfs->delta.y);
	double dx = dist * pfs->delta.y / denom,
	    dy = -dist * pfs->delta.x / denom;

	if_debug6('|', "[|]p0=(%g,%g), p1=(%g,%g), dxy=(%g,%g)\n",
		  x0, y0, x1, y1, dx, dy);
	gs_point_transform2fixed(&pis->ctm, x0 - dx, y0 - dy, &pts[0]);
	gs_point_transform2fixed(&pis->ctm, x0 + dx, y0 + dy, &pts[1]);
	gs_point_transform2fixed(&pis->ctm, x1 + dx, y1 + dy, &pts[2]);
	gs_point_transform2fixed(&pis->ctm, x1 - dx, y1 - dy, &pts[3]);
	gx_path_add_point(ppath, pts[0].x, pts[0].y);
	gx_path_add_lines(ppath, pts + 1, 3);
	code = shade_fill_path((const shading_fill_state_t *)pfs,
			       ppath, &dev_color);
	gx_path_free(ppath, "A_fill");
	return code;
    }
    /* Stripe is horizontal or vertical in both user and device space. */
    gs_point_transform2fixed(&pis->ctm, x0, y0, &pts[0]);
    gs_point_transform2fixed(&pis->ctm, x1, y1, &pts[1]);
    return
	shade_fill_device_rectangle((const shading_fill_state_t *)pfs,
				    &pts[0], &pts[1], &dev_color);
}

private int
A_fill_region(A_fill_state_t * pfs)
{
    const gs_shading_A_t * const psh = pfs->psh;
    gs_function_t * const pfn = psh->params.Function;
    A_frame_t *fp = &pfs->frames[pfs->depth - 1];

    for (;;) {
	double t0 = fp->t0, t1 = fp->t1;
	float ft0, ft1;

	if ((!(pfn->head.is_monotonic > 0 ||
	       (ft0 = (float)t0, ft1 = (float)t1,
		gs_function_is_monotonic(pfn, &ft0, &ft1, EFFORT_MODERATE) > 0)) ||
	     !shade_colors2_converge(fp->cc,
				     (const shading_fill_state_t *)pfs)) &&
	     /*
	      * The function isn't monotonic, or the colors don't converge.
	      * Is the stripe less than 1 pixel wide?
	      */
	    pfs->length * (t1 - t0) > 1 &&
	    fp < &pfs->frames[countof(pfs->frames) - 1]
	    ) {
	    /* Subdivide the interval and recur.  */
	    double tm = (t0 + t1) * 0.5;
	    float dm = tm * pfs->dd + psh->params.Domain[0];

	    gs_function_evaluate(pfn, &dm, fp[1].cc[1].paint.values);
	    fp[1].cc[0].paint = fp->cc[0].paint;
	    fp[1].t0 = t0;
	    fp[1].t1 = fp->t0 = tm;
	    fp->cc[0].paint = fp[1].cc[1].paint;
	    ++fp;
	} else {
	    /* Fill the region with the color. */
	    int code = A_fill_stripe(pfs, &fp->cc[0], t0, t1);

	    if (code < 0 || fp == &pfs->frames[0])
		return code;
	    --fp;
	}
    }
}

int
gs_shading_A_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			    gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_A_t *const psh = (const gs_shading_A_t *)psh0;
    gs_matrix cmat;
    gs_rect t_rect;
    A_fill_state_t state;
    gs_client_color rcc[2];
    float d0 = psh->params.Domain[0], d1 = psh->params.Domain[1];
    float dd = d1 - d0;
    float t0, t1;
    float t[2];
    gs_point dist;
    int i;
    int code;

    shade_init_fill_state((shading_fill_state_t *)&state, psh0, dev, pis);
    state.psh = psh;
    state.orthogonal = is_xxyy(&pis->ctm) || is_xyyx(&pis->ctm);
    state.rect = *rect;
    /*
     * Compute the parameter range.  We construct a matrix in which
     * (0,0) corresponds to t = 0 and (0,1) corresponds to t = 1,
     * and use it to inverse-map the rectangle to be filled.
     */
    cmat.tx = psh->params.Coords[0];
    cmat.ty = psh->params.Coords[1];
    state.delta.x = psh->params.Coords[2] - psh->params.Coords[0];
    state.delta.y = psh->params.Coords[3] - psh->params.Coords[1];
    cmat.yx = state.delta.x;
    cmat.yy = state.delta.y;
    cmat.xx = cmat.yy;
    cmat.xy = -cmat.yx;
    gs_bbox_transform_inverse(rect, &cmat, &t_rect);
    state.frames[0].t0 = t0 = max(t_rect.p.y, 0);
    t[0] = t0 * dd + d0;
    state.frames[0].t1 = t1 = min(t_rect.q.y, 1);
    t[1] = t1 * dd + d0;
    for (i = 0; i < 2; ++i) {
	gs_function_evaluate(psh->params.Function, &t[i],
			     rcc[i].paint.values);
    }
    memcpy(state.frames[0].cc, rcc, sizeof(rcc[0]) * 2);
    gs_distance_transform(state.delta.x, state.delta.y, &ctm_only(pis),
			  &dist);
    state.length = hypot(dist.x, dist.y);	/* device space line length */
    state.dd = dd;
    state.depth = 1;
    code = A_fill_region(&state);
    if (psh->params.Extend[0] && t0 > t_rect.p.y) {
	if (code < 0)
	    return code;
	code = A_fill_stripe(&state, &rcc[0], t_rect.p.y, t0);
    }
    if (psh->params.Extend[1] && t1 < t_rect.q.y) {
	if (code < 0)
	    return code;
	code = A_fill_stripe(&state, &rcc[1], t1, t_rect.q.y);
    }
    return code;
}

/* ---------------- Radial shading ---------------- */

#define R_max_depth AR_max_depth
typedef AR_frame_t R_frame_t;

typedef struct R_fill_state_s {
    shading_fill_state_common;
    const gs_shading_R_t *psh;
    gs_rect rect;
    gs_point delta;
    double dr, width, dd;
    int depth;
    R_frame_t frames[R_max_depth];
} R_fill_state_t;
/****** NEED GC DESCRIPTOR ******/

/* Note t0 and t1 vary over [0..1], not the Domain. */

private int
R_fill_annulus(const R_fill_state_t * pfs, gs_client_color *pcc,
	       floatp t0, floatp t1, floatp r0, floatp r1)
{
    const gs_shading_R_t * const psh = pfs->psh;
    gx_device_color dev_color;
    const gs_color_space *pcs = psh->params.ColorSpace;
    gs_imager_state *pis = pfs->pis;
    double
	x0 = psh->params.Coords[0] + pfs->delta.x * t0,
	y0 = psh->params.Coords[1] + pfs->delta.y * t0;
    double
	x1 = psh->params.Coords[0] + pfs->delta.x * t1,
	y1 = psh->params.Coords[1] + pfs->delta.y * t1;
    gx_path *ppath = gx_path_alloc(pis->memory, "R_fill");
    int code;

    (*pcs->type->restrict_color)(pcc, pcs);
    (*pcs->type->remap_color)(pcc, pcs, &dev_color, pis,
			      pfs->dev, gs_color_select_texture);
    if ((code = gs_imager_arc_add(ppath, pis, false, x0, y0, r0,
				  0.0, 360.0, false)) >= 0 &&
	(code = gs_imager_arc_add(ppath, pis, true, x1, y1, r1,
				  360.0, 0.0, false)) >= 0
	) {
	code = shade_fill_path((const shading_fill_state_t *)pfs,
			       ppath, &dev_color);
    }
    gx_path_free(ppath, "R_fill");
    return code;
}

private int
R_fill_region(R_fill_state_t * pfs)
{
    const gs_shading_R_t * const psh = pfs->psh;
    gs_function_t *pfn = psh->params.Function;
    R_frame_t *fp = &pfs->frames[pfs->depth - 1];

    for (;;) {
	double t0 = fp->t0, t1 = fp->t1;
	float ft0, ft1;

	if ((!(pfn->head.is_monotonic > 0 ||
	       (ft0 = (float)t0, ft1 = (float)t1,
		gs_function_is_monotonic(pfn, &ft0, &ft1, EFFORT_MODERATE) > 0)) ||
	     !shade_colors2_converge(fp->cc,
				     (const shading_fill_state_t *)pfs)) &&
	    /*
	     * The function isn't monotonic, or the colors don't converge.
	     * Is the annulus less than 1 pixel wide?
	     */
	    pfs->width * (t1 - t0) > 1 &&
	    fp < &pfs->frames[countof(pfs->frames) - 1]
	   ) {
	    /* Subdivide the interval and recur.  */
	    float tm = (t0 + t1) * 0.5;
	    float dm = tm * pfs->dd + psh->params.Domain[0];

	    gs_function_evaluate(pfn, &dm, fp[1].cc[1].paint.values);
	    fp[1].cc[0].paint = fp->cc[0].paint;
	    fp[1].t0 = t0;
	    fp[1].t1 = fp->t0 = tm;
	    fp->cc[0].paint = fp[1].cc[1].paint;
	    ++fp;
	} else {
	    /* Fill the region with the color. */
	    int code = R_fill_annulus(pfs, &fp->cc[0], t0, t1,
				      psh->params.Coords[2] + pfs->dr * t0,
				      psh->params.Coords[2] + pfs->dr * t1);

	    if (code < 0 || fp == &pfs->frames[0])
		return code;
	    --fp;
	}
    }
}

private double
R_compute_radius(floatp x, floatp y, const gs_rect *rect)
{
    double x0 = rect->p.x - x, y0 = rect->p.y - y,
	x1 = rect->q.x - x, y1 = rect->q.y - y;
    double r00 = hypot(x0, y0), r01 = hypot(x0, y1),
	r10 = hypot(x1, y0), r11 = hypot(x1, y1);
    double rm0 = max(r00, r01), rm1 = max(r10, r11);

    return max(rm0, rm1);
}

int
gs_shading_R_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			    gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_R_t *const psh = (const gs_shading_R_t *)psh0;
    R_fill_state_t state;
    gs_client_color rcc[2];
    float d0 = psh->params.Domain[0], d1 = psh->params.Domain[1];
    float dd = d1 - d0;
    float x0 = psh->params.Coords[0], y0 = psh->params.Coords[1];
    floatp r0 = psh->params.Coords[2];
    float x1 = psh->params.Coords[3], y1 = psh->params.Coords[4];
    floatp r1 = psh->params.Coords[5];
    float t[2];
    int i;
    int code;

    shade_init_fill_state((shading_fill_state_t *)&state, psh0, dev, pis);
    state.psh = psh;
    state.rect = *rect;
    /* Compute the parameter range. */
    t[0] = d0;
    t[1] = d1;
    for (i = 0; i < 2; ++i)
	gs_function_evaluate(psh->params.Function, &t[i],
			     rcc[i].paint.values);
    memcpy(state.frames[0].cc, rcc, sizeof(rcc[0]) * 2);
    state.delta.x = x1 - x0;
    state.delta.y = y1 - y0;
    state.dr = r1 - r0;
    /*
     * Compute the annulus width in its thickest direction.  This is
     * only used for a conservative check, so it can be pretty crude
     * (and it is!).
     */
    state.width =
	(fabs(pis->ctm.xx) + fabs(pis->ctm.xy) + fabs(pis->ctm.yx) +
	 fabs(pis->ctm.yy)) * fabs(state.dr);
    state.dd = dd;
    if (psh->params.Extend[0]) {
	if (r0 < r1)
	    code = R_fill_annulus(&state, &rcc[0], 0.0, 0.0, 0.0, r0);
	else
	    code = R_fill_annulus(&state, &rcc[0], 0.0, 0.0, r0,
				  R_compute_radius(x0, y0, rect));
	if (code < 0)
	    return code;
    }
    state.depth = 1;
    state.frames[0].t0 = (t[0] - d0) / dd;
    state.frames[0].t1 = (t[1] - d0) / dd;
    code = R_fill_region(&state);
    if (psh->params.Extend[1]) {
	if (code < 0)
	    return code;
	if (r0 < r1)
	    code = R_fill_annulus(&state, &rcc[1], 1.0, 1.0, r1,
				  R_compute_radius(x1, y1, rect));
	else
	    code = R_fill_annulus(&state, &rcc[1], 1.0, 1.0, 0.0, r1);
    }
    return code;
}
