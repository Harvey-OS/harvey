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

/*$Id: gxshade6.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Rendering for Coons and tensor patch shadings */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gxcspace.h"
#include "gxdcolor.h"
#include "gxistate.h"
#include "gxshade.h"
#include "gxshade4.h"
#include "gzpath.h"

/* ================ Utilities ================ */

/* Define one segment (vertex and next control points) of a curve. */
typedef struct patch_curve_s {
    mesh_vertex_t vertex;
    gs_fixed_point control[2];
} patch_curve_t;

/* Get colors for patch vertices. */
private int
shade_next_colors(shade_coord_stream_t * cs, patch_curve_t * curves,
		  int num_vertices)
{
    int i, code = 0;

    for (i = 0; i < num_vertices && code >= 0; ++i)
	code = shade_next_color(cs, curves[i].vertex.cc);
    return code;
}

/* Get a Bezier or tensor patch element. */
private int
shade_next_curve(shade_coord_stream_t * cs, patch_curve_t * curve)
{
    int code = shade_next_coords(cs, &curve->vertex.p, 1);

    if (code >= 0)
	code = shade_next_coords(cs, curve->control,
				 countof(curve->control));
    return code;
}

/* Define a color to be used in curve rendering. */
/* This may be a real client color, or a parametric function argument. */
typedef struct patch_color_s {
    float t;			/* parametric value */
    gs_client_color cc;
} patch_color_t;

/*
 * Parse the next patch out of the input stream.  Return 1 if done,
 * 0 if patch, <0 on error.
 */
private int
shade_next_patch(shade_coord_stream_t * cs, int BitsPerFlag,
patch_curve_t curve[4], gs_fixed_point interior[4] /* 0 for Coons patch */ )
{
    int flag = shade_next_flag(cs, BitsPerFlag);
    int num_colors, code;

    if (flag < 0)
	return 1;		/* no more data */
    switch (flag & 3) {
	default:
	    return_error(gs_error_rangecheck);	/* not possible */
	case 0:
	    if ((code = shade_next_curve(cs, &curve[0])) < 0 ||
		(code = shade_next_coords(cs, &curve[1].vertex.p, 1)) < 0
		)
		return code;
	    num_colors = 4;
	    goto vx;
	case 1:
	    curve[0] = curve[1], curve[1].vertex = curve[2].vertex;
	    goto v3;
	case 2:
	    curve[0] = curve[2], curve[1].vertex = curve[3].vertex;
	    goto v3;
	case 3:
	    curve[1].vertex = curve[0].vertex, curve[0] = curve[3];
v3:	    num_colors = 2;
vx:	    if ((code = shade_next_coords(cs, curve[1].control, 2)) < 0 ||
		(code = shade_next_curve(cs, &curve[2])) < 0 ||
		(code = shade_next_curve(cs, &curve[3])) < 0 ||
		(interior != 0 &&
		 (code = shade_next_coords(cs, interior, 4)) < 0) ||
		(code = shade_next_colors(cs, &curve[4 - num_colors],
					  num_colors)) < 0
		)
		return code;
    }
    return 0;
}

/* Define the common state for rendering Coons and tensor patches. */
typedef struct patch_fill_state_s {
    mesh_fill_state_common;
    const gs_function_t *Function;
} patch_fill_state_t;

/*
 * Calculate the interpolated color at a given point.
 * Note that we must do this twice for bilinear interpolation.
 * We use the name ppcr rather than ppc because an Apple compiler defines
 * ppc when compiling on PowerPC systems (!).
 */
private void
patch_interpolate_color(patch_color_t * ppcr, const patch_color_t * ppc0,
       const patch_color_t * ppc1, const patch_fill_state_t * pfs, floatp t)
{
    if (pfs->Function)
	ppcr->t = ppc0->t + t * (ppc1->t - ppc0->t);
    else {
	int ci;

	for (ci = pfs->num_components - 1; ci >= 0; --ci)
	    ppcr->cc.paint.values[ci] =
		ppc0->cc.paint.values[ci] +
		t * (ppc1->cc.paint.values[ci] - ppc0->cc.paint.values[ci]);
    }
}

/* Resolve a patch color using the Function if necessary. */
private void
patch_resolve_color(patch_color_t * ppcr, const patch_fill_state_t * pfs)
{
    if (pfs->Function)
	gs_function_evaluate(pfs->Function, &ppcr->t, ppcr->cc.paint.values);
}

/* ================ Specific shadings ================ */

/*
 * The curves are stored in a clockwise or counter-clockwise order that maps
 * to the patch definition in a non-intuitive way.  The documentation
 * (pp. 277-281 of the PostScript Language Reference Manual, Third Edition)
 * says that the curves are in the order D1, C2, D2, C1.
 */
/* The starting points of the curves: */
#define D1START 0
#define C2START 1
#define D2START 3
#define C1START 0
/* The control points of the curves (x means reversed order): */
#define D1CTRL 0
#define C2CTRL 1
#define D2XCTRL 2
#define C1XCTRL 3
/* The end points of the curves: */
#define D1END 1
#define C2END 2
#define D2END 2
#define C1END 3

/* ---------------- Common code ---------------- */

/* Evaluate a curve at a given point. */
private void
curve_eval(gs_fixed_point * pt, const gs_fixed_point * p0,
	   const gs_fixed_point * p1, const gs_fixed_point * p2,
	   const gs_fixed_point * p3, floatp t)
{
    fixed a, b, c, d;
    fixed t01, t12;

    d = p0->x;
    curve_points_to_coefficients(d, p1->x, p2->x, p3->x,
				 a, b, c, t01, t12);
    pt->x = (fixed) (((a * t + b) * t + c) * t + d);
    d = p0->y;
    curve_points_to_coefficients(d, p1->y, p2->y, p3->y,
				 a, b, c, t01, t12);
    pt->y = (fixed) (((a * t + b) * t + c) * t + d);
    if_debug3('2', "[2]t=%g => (%g,%g)\n", t, fixed2float(pt->x),
	      fixed2float(pt->y));
}

/*
 * Merge two arrays of splits, sorted in increasing order.
 * Return the number of entries in the result, which might be less than
 * n1 + n2 (if an a1 entry is equal to an a2 entry).
 * a1 or a2 may overlap out as long as a1 - out >= n2 or a2 - out >= n1
 * respectively.
 */
private int
merge_splits(double *out, const double *a1, int n1, const double *a2, int n2)
{
    double *p = out;
    int i1 = 0, i2 = 0;

    /*
     * We would like to write the body of the loop as an assignement
     * with a conditional expression on the right, but gcc 2.7.2.3
     * generates incorrect code if we do this.
     */
    while (i1 < n1 || i2 < n2)
	if (i1 == n1)
	    *p++ = a2[i2++];
	else if (i2 == n2 || a1[i1] < a2[i2])
	    *p++ = a1[i1++];
	else if (a1[i1] > a2[i2])
	    *p++ = a2[i2++];
	else
	    i1++, *p++ = a2[i2++];
    return p - out;
}

/*
 * Split a curve in both X and Y.  Return the number of split points.
 * swap = 0 if the control points are in order, 1 if reversed.
 */
private int
split_xy(double out[4], const gs_fixed_point *p0, const gs_fixed_point *p1,
	 const gs_fixed_point *p2, const gs_fixed_point *p3)
{
    double tx[2], ty[2];

    return merge_splits(out, tx,
			gx_curve_monotonic_points(p0->x, p1->x, p2->x, p3->x,
						  tx),
			ty,
			gx_curve_monotonic_points(p0->y, p1->y, p2->y, p3->y,
						  ty));
}

/*
 * Compute the joint split points of 2 curves.
 * swap = 0 if the control points are in order, 1 if reversed.
 * Return the number of split points.
 */
inline private int
split2_xy(double out[8], const gs_fixed_point *p10, const gs_fixed_point *p11,
	  const gs_fixed_point *p12, const gs_fixed_point *p13,
	  const gs_fixed_point *p20, const gs_fixed_point *p21,
	  const gs_fixed_point *p22, const gs_fixed_point *p23)
{
    double t1[4], t2[4];

    return merge_splits(out, t1, split_xy(t1, p10, p11, p12, p13),
			t2, split_xy(t2, p20, p21, p22, p23));
}

private int
patch_fill(patch_fill_state_t * pfs, const patch_curve_t curve[4],
	   const gs_fixed_point interior[4],
	   void (*transform) (P5(gs_fixed_point *, const patch_curve_t[4],
				 const gs_fixed_point[4], floatp, floatp)))
{	/*
	 * The specification says the output must appear to be produced in
	 * order of increasing values of v, and for equal v, in order of
	 * increasing u.  However, all we actually have to do is follow this
	 * order with respect to sub-patches that might overlap, which can
	 * only occur as a result of non-monotonic curves; we can render
	 * each monotonic sub-patch in any order we want.  Therefore, we
	 * begin by breaking up the patch into pieces that are monotonic
	 * with respect to all 4 edges.  Since each edge has no more than
	 * 2 X and 2 Y split points (for a total of 4), taking both edges
	 * together we have a maximum of 8 split points for each axis.
	 */
    double su[9], sv[9];
    int nu = split2_xy(su, &curve[C1START].vertex.p,&curve[C1XCTRL].control[1],
		       &curve[C1XCTRL].control[0], &curve[C1END].vertex.p,
		       &curve[C2START].vertex.p, &curve[C2CTRL].control[0],
		       &curve[C2CTRL].control[1], &curve[C2END].vertex.p);
    int nv = split2_xy(sv, &curve[D1START].vertex.p, &curve[D1CTRL].control[0],
		       &curve[D1CTRL].control[1], &curve[D1END].vertex.p,
		       &curve[D2START].vertex.p, &curve[D2XCTRL].control[1],
		       &curve[D2XCTRL].control[0], &curve[D2END].vertex.p);
    int iu, iv, ju, jv, ku, kv;
    double du, dv;
    double v0, v1, vn, u0, u1, un;
    patch_color_t c00, c01, c10, c11;
    /*
     * At some future time, we should set check = false if the curves
     * fall entirely within the bounding rectangle.  (Only a small
     * performance optimization, to avoid making this check for each
     * triangle.)
     */
    bool check = true;

#ifdef DEBUG
    if (gs_debug_c('2')) {
	int k;

	dlputs("[2]patch curves:\n");
	for (k = 0; k < 4; ++k)
	    dprintf6("        (%g,%g) (%g,%g)(%g,%g)\n",
		     fixed2float(curve[k].vertex.p.x),
		     fixed2float(curve[k].vertex.p.y),
		     fixed2float(curve[k].control[0].x),
		     fixed2float(curve[k].control[0].y),
		     fixed2float(curve[k].control[1].x),
		     fixed2float(curve[k].control[1].y));
	if (nu > 1) {
	    dlputs("[2]Splitting u");
	    for (k = 0; k < nu; ++k)
		dprintf1(", %g", su[k]);
	    dputs("\n");
	}
	if (nv > 1) {
	    dlputs("[2]Splitting v");
	    for (k = 0; k < nv; ++k)
		dprintf1(", %g", sv[k]);
	    dputs("\n");
	}
    }
#endif
    /* Add boundary values to simplify the iteration. */
    su[nu] = 1;
    sv[nv] = 1;

    /*
     * We're going to fill the curves by flattening them and then filling
     * the resulting triangles.  Start by computing the number of
     * segments required for flattening each side of the patch.
     */
    {
	fixed flatness = float2fixed(pfs->pis->flatness);
	int i;
	int log2_k[4];

	for (i = 0; i < 4; ++i) {
	    curve_segment cseg;

	    cseg.p1 = curve[i].control[0];
	    cseg.p2 = curve[i].control[1];
	    cseg.pt = curve[(i + 1) & 3].vertex.p;
	    log2_k[i] =
		gx_curve_log2_samples(curve[i].vertex.p.x, curve[i].vertex.p.y,
				      &cseg, flatness);
	}
	ku = 1 << max(log2_k[1], log2_k[3]);
	kv = 1 << max(log2_k[0], log2_k[2]);
    }

    /* Precompute the colors at the corners. */

#define PATCH_SET_COLOR(c, v)\
  if ( pfs->Function ) c.t = v.cc[0];\
  else memcpy(c.cc.paint.values, v.cc, sizeof(c.cc.paint.values))

    PATCH_SET_COLOR(c00, curve[D1START].vertex); /* = C1START */
    PATCH_SET_COLOR(c01, curve[D1END].vertex); /* = C2START */
    PATCH_SET_COLOR(c11, curve[C2END].vertex); /* = D2END */
    PATCH_SET_COLOR(c10, curve[C1END].vertex); /* = D2START */

#undef PATCH_SET_COLOR

    /*
     * Since ku and kv are powers of 2, and since log2(k) is surely less
     * than the number of bits in the mantissa of a double, 1/k ...
     * (k-1)/k can all be represented exactly as doubles.
     */
    du = 1.0 / ku;
    dv = 1.0 / kv;

    /* Now iterate over the sub-patches. */
    for (iv = 0, jv = 0, v0 = 0, v1 = vn = dv; jv < kv; v0 = v1, v1 = vn) {
	patch_color_t c0v0, c0v1, c1v0, c1v1;

	/* Subdivide the interval if it crosses a split point. */

#define CHECK_SPLIT(ix, jx, x1, xn, dx, ax)\
  if (x1 > ax[ix])\
      x1 = ax[ix++];\
  else {\
      xn += dx;\
      jx++;\
      if (x1 == ax[ix])\
	  ix++;\
  }

	CHECK_SPLIT(iv, jv, v1, vn, dv, sv);

	patch_interpolate_color(&c0v0, &c00, &c01, pfs, v0);
	patch_interpolate_color(&c0v1, &c00, &c01, pfs, v1);
	patch_interpolate_color(&c1v0, &c10, &c11, pfs, v0);
	patch_interpolate_color(&c1v1, &c10, &c11, pfs, v1);

	for (iu = 0, ju = 0, u0 = 0, u1 = un = du; ju < ku; u0 = u1, u1 = un) {
	    patch_color_t cu0v0, cu1v0, cu0v1, cu1v1;
	    int code;

	    CHECK_SPLIT(iu, ju, u1, un, du, su);

#undef CHECK_SPLIT

	    patch_interpolate_color(&cu0v0, &c0v0, &c1v0, pfs, u0);
	    patch_resolve_color(&cu0v0, pfs);
	    patch_interpolate_color(&cu1v0, &c0v0, &c1v0, pfs, u1);
	    patch_resolve_color(&cu1v0, pfs);
	    patch_interpolate_color(&cu0v1, &c0v1, &c1v1, pfs, u0);
	    patch_resolve_color(&cu0v1, pfs);
	    patch_interpolate_color(&cu1v1, &c0v1, &c1v1, pfs, u1);
	    patch_resolve_color(&cu1v1, pfs);
	    if_debug6('2', "[2]u[%d]=[%g .. %g], v[%d]=[%g .. %g]\n",
		      iu, u0, u1, iv, v0, v1);

	    /* Fill the sub-patch given by ((u0,v0),(u1,v1)). */
	    {
		mesh_vertex_t mu0v0, mu1v0, mu1v1, mu0v1;

		(*transform)(&mu0v0.p, curve, interior, u0, v0);
		(*transform)(&mu1v0.p, curve, interior, u1, v0);
		(*transform)(&mu1v1.p, curve, interior, u1, v1);
		(*transform)(&mu0v1.p, curve, interior, u0, v1);
		if_debug4('2', "[2]  => (%g,%g), (%g,%g),\n",
			  fixed2float(mu0v0.p.x), fixed2float(mu0v0.p.y),
			  fixed2float(mu1v0.p.x), fixed2float(mu1v0.p.y));
		if_debug4('2', "[2]     (%g,%g), (%g,%g)\n",
			  fixed2float(mu1v1.p.x), fixed2float(mu1v1.p.y),
			  fixed2float(mu0v1.p.x), fixed2float(mu0v1.p.y));
		memcpy(mu0v0.cc, cu0v0.cc.paint.values, sizeof(mu0v0.cc));
		memcpy(mu1v0.cc, cu1v0.cc.paint.values, sizeof(mu1v0.cc));
		memcpy(mu1v1.cc, cu1v1.cc.paint.values, sizeof(mu1v1.cc));
		memcpy(mu0v1.cc, cu0v1.cc.paint.values, sizeof(mu0v1.cc));
		mesh_init_fill_triangle((mesh_fill_state_t *)pfs,
					&mu0v0, &mu1v1, &mu1v0, check);
		code = mesh_fill_triangle((mesh_fill_state_t *)pfs);
		if (code < 0)
		    return code;
		mesh_init_fill_triangle((mesh_fill_state_t *)pfs,
					&mu0v0, &mu1v1, &mu0v1, check);
		code = mesh_fill_triangle((mesh_fill_state_t *)pfs);
		if (code < 0)
		    return code;
	    }
	}
    }
    return 0;
}

/* ---------------- Coons patch shading ---------------- */

/* Calculate the device-space coordinate corresponding to (u,v). */
private void
Cp_transform(gs_fixed_point * pt, const patch_curve_t curve[4],
	     const gs_fixed_point ignore_interior[4], floatp u, floatp v)
{
    double co_u = 1.0 - u, co_v = 1.0 - v;
    gs_fixed_point c1u, d1v, c2u, d2v;

    curve_eval(&c1u, &curve[C1START].vertex.p,
	       &curve[C1XCTRL].control[1], &curve[C1XCTRL].control[0],
	       &curve[C1END].vertex.p, u);
    curve_eval(&d1v, &curve[D1START].vertex.p,
	       &curve[D1CTRL].control[0], &curve[D1CTRL].control[1],
	       &curve[D1END].vertex.p, v);
    curve_eval(&c2u, &curve[C2START].vertex.p,
	       &curve[C2CTRL].control[0], &curve[C2CTRL].control[1],
	       &curve[C2END].vertex.p, u);
    curve_eval(&d2v, &curve[D2START].vertex.p,
	       &curve[D2XCTRL].control[1], &curve[D2XCTRL].control[0],
	       &curve[D2END].vertex.p, v);
#define COMPUTE_COORD(xy)\
    pt->xy = (fixed)\
	((co_v * c1u.xy + v * c2u.xy) + (co_u * d1v.xy + u * d2v.xy) -\
	 (co_v * (co_u * curve[C1START].vertex.p.xy +\
		  u * curve[C1END].vertex.p.xy) +\
	  v * (co_u * curve[C2START].vertex.p.xy +\
	       u * curve[C2END].vertex.p.xy)))
    COMPUTE_COORD(x);
    COMPUTE_COORD(y);
#undef COMPUTE_COORD
    if_debug4('2', "[2](u=%g,v=%g) => (%g,%g)\n",
	      u, v, fixed2float(pt->x), fixed2float(pt->y));
}

int
gs_shading_Cp_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			     gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_Cp_t * const psh = (const gs_shading_Cp_t *)psh0;
    patch_fill_state_t state;
    shade_coord_stream_t cs;
    patch_curve_t curve[4];
    int code;

    mesh_init_fill_state((mesh_fill_state_t *) & state,
			 (const gs_shading_mesh_t *)psh0, rect, dev, pis);
    state.Function = psh->params.Function;
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params,
		    pis);
    while ((code = shade_next_patch(&cs, psh->params.BitsPerFlag,
				    curve, NULL)) == 0 &&
	   (code = patch_fill(&state, curve, NULL, Cp_transform)) >= 0
	)
	DO_NOTHING;
    return min(code, 0);
}

/* ---------------- Tensor product patch shading ---------------- */

/* Calculate the device-space coordinate corresponding to (u,v). */
private void
Tpp_transform(gs_fixed_point * pt, const patch_curve_t curve[4],
	      const gs_fixed_point interior[4], floatp u, floatp v)
{
    double Bu[4], Bv[4];
    gs_fixed_point pts[4][4];
    int i, j;
    double x = 0, y = 0;

    /* Compute the Bernstein polynomials of u and v. */
    {
	double u2 = u * u, co_u = 1.0 - u, co_u2 = co_u * co_u;
	double v2 = v * v, co_v = 1.0 - v, co_v2 = co_v * co_v;

	Bu[0] = co_u * co_u2, Bu[1] = 3 * u * co_u2,
	    Bu[2] = 3 * u2 * co_u, Bu[3] = u * u2;
	Bv[0] = co_v * co_v2, Bv[1] = 3 * v * co_v2,
	    Bv[2] = 3 * v2 * co_v, Bv[3] = v * v2;
    }

    /* Arrange the points into an indexable order. */
    pts[0][0] = curve[0].vertex.p;
    pts[0][1] = curve[0].control[0];
    pts[0][2] = curve[0].control[1];
    pts[0][3] = curve[1].vertex.p;
    pts[1][3] = curve[1].control[0];
    pts[2][3] = curve[1].control[1];
    pts[3][3] = curve[2].vertex.p;
    pts[3][2] = curve[2].control[0];
    pts[3][1] = curve[2].control[1];
    pts[3][0] = curve[3].vertex.p;
    pts[2][0] = curve[3].control[0];
    pts[1][0] = curve[3].control[1];
    pts[1][1] = interior[0];
    pts[2][1] = interior[1];
    pts[2][2] = interior[2];
    pts[1][2] = interior[3];

    /* Now compute the actual point. */
    for (i = 0; i < 4; ++i)
	for (j = 0; j < 4; ++j) {
	    double coeff = Bu[i] * Bv[j];

	    x += pts[i][j].x * coeff, y += pts[i][j].y * coeff;
	}
    pt->x = (fixed)x, pt->y = (fixed)y;
}

int
gs_shading_Tpp_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			      gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_Tpp_t * const psh = (const gs_shading_Tpp_t *)psh0;
    patch_fill_state_t state;
    shade_coord_stream_t cs;
    patch_curve_t curve[4];
    gs_fixed_point interior[4];
    int code;

    mesh_init_fill_state((mesh_fill_state_t *) & state,
			 (const gs_shading_mesh_t *)psh0, rect, dev, pis);
    state.Function = psh->params.Function;
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params,
		    pis);
    while ((code = shade_next_patch(&cs, psh->params.BitsPerFlag,
				    curve, interior)) == 0) {
	/*
	 * The order of points appears to be consistent with that for Coons
	 * patches, which is different from that documented in Red Book 3.
	 */
	gs_fixed_point swapped_interior[4];

	swapped_interior[0] = interior[0];
	swapped_interior[1] = interior[3];
	swapped_interior[2] = interior[2];
	swapped_interior[3] = interior[1];
	code = patch_fill(&state, curve, swapped_interior, Tpp_transform);
	if (code < 0)
	    break;
    }
    return min(code, 0);
}
