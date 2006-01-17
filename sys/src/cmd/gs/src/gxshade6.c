/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxshade6.c,v 1.100 2005/05/25 15:57:58 igor Exp $ */
/* Rendering for Coons and tensor patch shadings */
/*
   A contiguous non-overlapping decomposition 
   of a tensor shading into linear color triangles.
 */
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
#include "gxdevcli.h"
#include "gxarith.h"
#include "gzpath.h"
#include "stdint_.h"
#include "math_.h"
#include "vdtrace.h"
#include <assert.h>

#define VD_TRACE_TENSOR_PATCH 1

#if NOFILL_TEST
static bool dbg_nofill = false;
#endif
#if SKIP_TEST
static int dbg_patch_cnt = 0;
static int dbg_quad_cnt = 0;
static int dbg_triangle_cnt = 0;
static int dbg_wedge_triangle_cnt = 0;
#endif

static int min_linear_grades = 255; /* The minimal number of device color grades,
            required to apply linear color device functions. */

/* ================ Utilities ================ */

/* Get colors for patch vertices. */
private int
shade_next_colors(shade_coord_stream_t * cs, patch_curve_t * curves,
		  int num_vertices)
{
    int i, code = 0;

    for (i = 0; i < num_vertices && code >= 0; ++i) {
        curves[i].vertex.cc[1] = 0; /* safety. (patch_fill may assume 2 arguments) */
	code = shade_next_color(cs, curves[i].vertex.cc);
    }
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

    if (flag < 0) {
	if (!cs->is_eod(cs))
	    return_error(gs_error_rangecheck);
	return 1;		/* no more data */
    }
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

int
init_patch_fill_state(patch_fill_state_t *pfs)
{
    /* Warning : pfs->Function must be set in advance. */
    const gs_color_space *pcs = pfs->direct_space;
    gs_client_color fcc0, fcc1;
    int i, code;

    for (i = 0; i < pfs->num_components; i++) {
	fcc0.paint.values[i] = -1000000;
	fcc1.paint.values[i] = 1000000;
    }
    pcs->type->restrict_color(&fcc0, pcs);
    pcs->type->restrict_color(&fcc1, pcs);
    for (i = 0; i < pfs->num_components; i++)
	pfs->color_domain.paint.values[i] = max(fcc1.paint.values[i] - fcc0.paint.values[i], 1);
    pfs->vectorization = false; /* A stub for a while. Will use with pclwrite. */
    pfs->maybe_self_intersecting = true;
    pfs->monotonic_color = (pfs->Function == NULL);
    pfs->linear_color = false;
    pfs->inside = false;
    pfs->n_color_args = 1;
    pfs->fixed_flat = float2fixed(pfs->pis->flatness);
    pfs->smoothness = pfs->pis->smoothness;
#   if LAZY_WEDGES
	code = wedge_vertex_list_elem_buffer_alloc(pfs);
	if (code < 0)
	    return code;
#   endif
    pfs->max_small_coord = 1 << ((sizeof(int64_t) * 8 - 1/*sign*/) / 3);
return 0;
}

void
term_patch_fill_state(patch_fill_state_t *pfs)
{
#   if LAZY_WEDGES
	wedge_vertex_list_elem_buffer_free(pfs);
#   endif
}

/* Resolve a patch color using the Function if necessary. */
inline private void
patch_resolve_color_inline(patch_color_t * ppcr, const patch_fill_state_t *pfs)
{
    if (pfs->Function) {
	const gs_color_space *pcs = pfs->direct_space;

	gs_function_evaluate(pfs->Function, ppcr->t, ppcr->cc.paint.values);
	pcs->type->restrict_color(&ppcr->cc, pcs);
    }
}

void
patch_resolve_color(patch_color_t * ppcr, const patch_fill_state_t *pfs)
{
    patch_resolve_color_inline(ppcr, pfs);
}


/*
 * Calculate the interpolated color at a given point.
 * Note that we must do this twice for bilinear interpolation.
 * We use the name ppcr rather than ppc because an Apple compiler defines
 * ppc when compiling on PowerPC systems (!).
 */
private void
patch_interpolate_color(patch_color_t * ppcr, const patch_color_t * ppc0,
       const patch_color_t * ppc1, const patch_fill_state_t *pfs, floatp t)
{
    /* The old code gives -IND on Intel. */
    if (pfs->Function) {
	ppcr->t[0] = ppc0->t[0] * (1 - t) + t * ppc1->t[0];
	ppcr->t[1] = ppc0->t[1] * (1 - t) + t * ppc1->t[1];
	patch_resolve_color_inline(ppcr, pfs);
    } else {
	int ci;

	for (ci = pfs->num_components - 1; ci >= 0; --ci)
	    ppcr->cc.paint.values[ci] =
		ppc0->cc.paint.values[ci] * (1 - t) + t * ppc1->cc.paint.values[ci];
    }
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
			     const gs_fixed_rect * rect_clip,
			     gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_Cp_t * const psh = (const gs_shading_Cp_t *)psh0;
    patch_fill_state_t state;
    shade_coord_stream_t cs;
    patch_curve_t curve[4];
    int code;

    code = mesh_init_fill_state((mesh_fill_state_t *) &state,
			 (const gs_shading_mesh_t *)psh0, rect_clip, dev, pis);
    if (code < 0)
	return code;
    state.Function = psh->params.Function;
    code = init_patch_fill_state(&state);
    if(code < 0)
	return code;
    if (VD_TRACE_TENSOR_PATCH && vd_allowed('s')) {
	vd_get_dc('s');
	vd_set_shift(0, 0);
	vd_set_scale(0.01);
	vd_set_origin(0, 0);
	/* vd_erase(RGB(192, 192, 192)); */
    }

    curve[0].straight = curve[1].straight = curve[2].straight = curve[3].straight = false;
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params, pis);
    while ((code = shade_next_patch(&cs, psh->params.BitsPerFlag,
				    curve, NULL)) == 0 &&
	   (code = patch_fill(&state, curve, NULL, Cp_transform)) >= 0
	) {
	DO_NOTHING;
    }
    if (VD_TRACE_TENSOR_PATCH && vd_allowed('s'))
	vd_release_dc;
    term_patch_fill_state(&state);
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
			     const gs_fixed_rect * rect_clip,
			      gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_Tpp_t * const psh = (const gs_shading_Tpp_t *)psh0;
    patch_fill_state_t state;
    shade_coord_stream_t cs;
    patch_curve_t curve[4];
    gs_fixed_point interior[4];
    int code;

    code = mesh_init_fill_state((mesh_fill_state_t *) & state,
			 (const gs_shading_mesh_t *)psh0, rect_clip, dev, pis);
    if (code < 0)
	return code;
    state.Function = psh->params.Function;
    code = init_patch_fill_state(&state);
    if(code < 0)
	return code;
    if (VD_TRACE_TENSOR_PATCH && vd_allowed('s')) {
	vd_get_dc('s');
	vd_set_shift(0, 0);
	vd_set_scale(0.01);
	vd_set_origin(0, 0);
	/* vd_erase(RGB(192, 192, 192)); */
    }
    curve[0].straight = curve[1].straight = curve[2].straight = curve[3].straight = false;
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params, pis);
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
    term_patch_fill_state(&state);
    if (VD_TRACE_TENSOR_PATCH && vd_allowed('s'))
	vd_release_dc;
    return min(code, 0);
}

/*
    This algorithm performs a decomposition of the shading area
    into a set of constant color trapezoids, some of which
    may use the transpozed coordinate system.

    The target device assumes semi-open intrvals by X to be painted
    (See PLRM3, 7.5. Scan conversion details), i.e.
    it doesn't paint pixels which falls exactly to the right side.
    Note that with raster devices the algorithm doesn't paint pixels, 
    whigh are partially covered by the shading area, 
    but which's centers are outside the area.

    Pixels inside a monotonic part of the shading area are painted 
    at once, but some exceptions may happen :

        - While flattening boundaries of a subpatch,
	to keep the plane coverage contiguity we insert wedges 
	between neighbor subpatches, which use a different
	flattening factor. With non-monotonic curves
	those wedges may overlap or be self-overlapping, and a pixel 
	is painted so many times as many wedges cover it. Fortunately
	the area of most wedges is zero or extremily small.

	- Since quazi-horizontal wedges may have a non-constant color,
	they can't decompose into constant color trapezoids with
	keeping the coverage contiguity. To represent them we
	apply the XY plane transposition. But with the transposition 
	a semiopen interval can met a non-transposed one,
	so that some lines are not covered. Therefore we emulate 
	closed intervals with expanding the transposed trapesoids in 
	fixed_epsilon, and pixels at that boundary may be painted twice.

	- A boundary of a monotonic area can't compute in XY 
	preciselly due to high order polynomial equations. 
	Therefore the subdivision near the monotonity boundary 
	may paint some pixels twice within same monotonic part.

    Non-monotonic areas slow down due to a tinny subdivision required.
    
    The target device may be either raster or vector. 
    Vector devices should preciselly pass trapezoids to the output.
    Note that ends of sides of a trapesoid are not necessary 
    the trapezoid's vertices. Converting this thing into
    an exact quadrangle may cause an arithmetic error,
    and the rounding must be done so that the coverage
    contiguity is not lost. 

    When a device passes a trapezoid to it's output, 
    a regular rounding would keep the coverage contiguity, 
    except for the transposed trapesoids. 
    If a transposed trapezoid is being transposed back, 
    it doesn't become a canonic trapezoid, and a further
    decomposition is neccessary. But rounding errors here
    would break the coverage contiguity at boundaries
    of the tansposed part of the area.

    Devices, which have no transposed trapezoids and represent 
    trapezoids only with 8 coordinates of vertices of the quadrangle
    (pclwrite is an example) may apply the backward transposition,
    and a clipping instead the further decomposition.
    Note that many clip regions may appear for all wedges. 
    Note that in some cases the adjustment of the right side to be 
    withdrown before the backward transposition.
 */
 /* We believe that a multiplication of 32-bit integers with a 
    64-bit result is performed by modern platforms performs 
    in hardware level. Therefore we widely use it here, 
    but we minimize the usage of a multiplication of longer integers. 
    
    Unfortunately we do need a multiplication of long integers
    in intersection_of_small_bars, because solving the linear system
    requires tripple multiples of 'fixed'. Therefore we retain
    of it's usage in the algorithm of the main branch.
    Configuration macro QUADRANGLES prevents it.
  */

typedef struct {
    gs_fixed_point pole[4][4]; /* [v][u] */
    patch_color_t c[2][2];     /* [v][u] */
} tensor_patch;

typedef struct {
    const shading_vertex_t *p[2][2]; /* [v][u] */
    wedge_vertex_list_t *l0001, *l0111, *l1110, *l1000;
} quadrangle_patch;

typedef enum {
    interpatch_padding = 1, /* A Padding between patches for poorly designed documents. */
    inpatch_wedge = 2  /* Wedges while a patch decomposition. */
} wedge_type_t;

int
wedge_vertex_list_elem_buffer_alloc(patch_fill_state_t *pfs)
{
    const int max_level = LAZY_WEDGES_MAX_LEVEL;
    gs_memory_t *memory = pfs->pis->memory;

    pfs->wedge_vertex_list_elem_count_max = max_level * (1 << max_level);
    pfs->wedge_vertex_list_elem_buffer = (wedge_vertex_list_elem_t *)gs_alloc_bytes(memory, 
	    sizeof(wedge_vertex_list_elem_t) * pfs->wedge_vertex_list_elem_count_max, 
	    "alloc_wedge_vertex_list_elem_buffer");
    if (pfs->wedge_vertex_list_elem_buffer == NULL)
	return_error(gs_error_VMerror);
    pfs->free_wedge_vertex = NULL;
    pfs->wedge_vertex_list_elem_count = 0;
    return 0;
}

void
wedge_vertex_list_elem_buffer_free(patch_fill_state_t *pfs)
{
    gs_memory_t *memory = pfs->pis->memory;

    gs_free_object(memory, pfs->wedge_vertex_list_elem_buffer, 
		"wedge_vertex_list_elem_buffer_free");
    pfs->wedge_vertex_list_elem_buffer = NULL;
    pfs->free_wedge_vertex = NULL;
}

private inline wedge_vertex_list_elem_t *
wedge_vertex_list_elem_reserve(patch_fill_state_t *pfs)
{
    wedge_vertex_list_elem_t *e = pfs->free_wedge_vertex;

    if (e != NULL) {
	pfs->free_wedge_vertex = e->next;
	return e;
    }
    if (pfs->wedge_vertex_list_elem_count < pfs->wedge_vertex_list_elem_count_max)
	return pfs->wedge_vertex_list_elem_buffer + pfs->wedge_vertex_list_elem_count++;
    return NULL;
}

private inline void
wedge_vertex_list_elem_release(patch_fill_state_t *pfs, wedge_vertex_list_elem_t *e)
{
    e->next = pfs->free_wedge_vertex;
    pfs->free_wedge_vertex = e;
}

private inline void
release_triangle_wedge_vertex_list_elem(patch_fill_state_t *pfs, 
    wedge_vertex_list_elem_t *beg, wedge_vertex_list_elem_t *end)
{
    wedge_vertex_list_elem_t *e = beg->next;

    assert(beg->next->next == end);
    beg->next = end;
    end->prev = beg;
    wedge_vertex_list_elem_release(pfs, e);
}

private inline void
release_wedge_vertex_list_interval(patch_fill_state_t *pfs, 
    wedge_vertex_list_elem_t *beg, wedge_vertex_list_elem_t *end)
{
    wedge_vertex_list_elem_t *e = beg->next, *ee;

    beg->next = end;
    end->prev = beg;
    for (; e != end; e = ee) {
	ee = e->next;
	wedge_vertex_list_elem_release(pfs, e);
    }
}

private inline void
release_wedge_vertex_list(patch_fill_state_t *pfs, wedge_vertex_list_t *ll, int n)
{
    int i;

    for (i = 0; i < n; i++) {
	wedge_vertex_list_t *l = ll + i;

	if (l->beg != NULL) {
	    assert(l->end != NULL);
	    release_wedge_vertex_list_interval(pfs, l->beg, l->end);
	    wedge_vertex_list_elem_release(pfs, l->beg);
	    wedge_vertex_list_elem_release(pfs, l->end);
	    l->beg = l->end = NULL;
	} else
	    assert(l->end == NULL);
    }
}

private inline wedge_vertex_list_elem_t *
wedge_vertex_list_find(wedge_vertex_list_elem_t *beg, const wedge_vertex_list_elem_t *end, 
	    int level)
{
    wedge_vertex_list_elem_t *e = beg;
    
    assert(beg != NULL && end != NULL);
    for (; e != end; e = e->next)
	if (e->level == level)
	    return e;
    return NULL;
}

private inline void
init_wedge_vertex_list(wedge_vertex_list_t *l, int n)
{
    memset(l, 0, sizeof(*l) * n);
}

private void
draw_patch(const tensor_patch *p, bool interior, ulong rgbcolor)
{
#ifdef DEBUG
#if 0 /* Disabled for a better view with a specific purpose. 
	 Feel free to enable fo needed. */
    int i, step = (interior ? 1 : 3);

    for (i = 0; i < 4; i += step) {
	vd_curve(p->pole[i][0].x, p->pole[i][0].y, 
		 p->pole[i][1].x, p->pole[i][1].y, 
		 p->pole[i][2].x, p->pole[i][2].y, 
		 p->pole[i][3].x, p->pole[i][3].y, 
		 0, rgbcolor);
	vd_curve(p->pole[0][i].x, p->pole[0][i].y, 
		 p->pole[1][i].x, p->pole[1][i].y, 
		 p->pole[2][i].x, p->pole[2][i].y, 
		 p->pole[3][i].x, p->pole[3][i].y, 
		 0, rgbcolor);
    }
#endif
#endif
}

private inline void
draw_triangle(const gs_fixed_point *p0, const gs_fixed_point *p1, 
		const gs_fixed_point *p2, ulong rgbcolor)
{
#ifdef DEBUG
    if (!vd_enabled)
	return;
    vd_quad(p0->x, p0->y, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y, 0, rgbcolor);
#endif
}

private inline void
draw_quadrangle(const quadrangle_patch *p, ulong rgbcolor)
{
#ifdef DEBUG
	vd_quad(p->p[0][0]->p.x, p->p[0][0]->p.y, 
	    p->p[0][1]->p.x, p->p[0][1]->p.y,
	    p->p[1][1]->p.x, p->p[1][1]->p.y,
	    p->p[1][0]->p.x, p->p[1][0]->p.y,
	    0, rgbcolor);
#endif
}

private inline int
curve_samples(patch_fill_state_t *pfs, 
		const gs_fixed_point *pole, int pole_step, fixed fixed_flat)
{
    curve_segment s;
    int k;

    s.p1.x = pole[pole_step].x;
    s.p1.y = pole[pole_step].y;
    s.p2.x = pole[pole_step * 2].x;
    s.p2.y = pole[pole_step * 2].y;
    s.pt.x = pole[pole_step * 3].x;
    s.pt.y = pole[pole_step * 3].y;
    k = gx_curve_log2_samples(pole[0].x, pole[0].y, &s, fixed_flat);
    {	
#	if LAZY_WEDGES || QUADRANGLES
	    int k1;
	    fixed L = any_abs(pole[1].x - pole[0].x) + any_abs(pole[1].y - pole[0].y) +
		      any_abs(pole[2].x - pole[1].x) + any_abs(pole[2].y - pole[1].y) +
		      any_abs(pole[3].x - pole[2].x) + any_abs(pole[3].y - pole[2].y);
#	endif

#	if LAZY_WEDGES
	    /* Restrict lengths for a reasonable memory consumption : */
	    k1 = ilog2(L / fixed_1 / (1 << (LAZY_WEDGES_MAX_LEVEL - 1)));
	    k = max(k, k1);
#	endif
#	if QUADRANGLES
	    /* Restrict lengths for intersection_of_small_bars : */
	    k = max(k, ilog2(L) - ilog2(pfs->max_small_coord));
#	endif
    }
    return 1 << k;
}

private bool 
intersection_of_small_bars(const gs_fixed_point q[4], int i0, int i1, int i2, int i3, fixed *ry, fixed *ey)
{
    /* This function is only used with QUADRANGLES. */
    fixed dx1 = q[i1].x - q[i0].x, dy1 = q[i1].y - q[i0].y;
    fixed dx2 = q[i2].x - q[i0].x, dy2 = q[i2].y - q[i0].y;
    fixed dx3 = q[i3].x - q[i0].x, dy3 = q[i3].y - q[i0].y;
    int64_t vp2a, vp2b, vp3a, vp3b;
    int s2, s3;

    if (dx1 == 0 && dy1 == 0)
	return false; /* Zero length bars are out of interest. */
    if (dx2 == 0 && dy2 == 0)
	return false; /* Contacting ends are out of interest. */
    if (dx3 == 0 && dy3 == 0)
	return false; /* Contacting ends are out of interest. */
    if (dx2 == dx1 && dy2 == dy1)
	return false; /* Contacting ends are out of interest. */
    if (dx3 == dx1 && dy3 == dy1)
	return false; /* Contacting ends are out of interest. */
    if (dx2 == dx3 && dy2 == dy3)
	return false; /* Zero length bars are out of interest. */
    vp2a = (int64_t)dx1 * dy2;
    vp2b = (int64_t)dy1 * dx2; 
    /* vp2 = vp2a - vp2b; It can overflow int64_t, but we only need the sign. */
    if (vp2a > vp2b)
	s2 = 1;
    else if (vp2a < vp2b)
	s2 = -1;
    else 
	s2 = 0;
    vp3a = (int64_t)dx1 * dy3;
    vp3b = (int64_t)dy1 * dx3; 
    /* vp3 = vp3a - vp3b; It can overflow int64_t, but we only need the sign. */
    if (vp3a > vp3b)
	s3 = 1;
    else if (vp3a < vp3b)
	s3 = -1;
    else 
	s3 = 0;
    if (s2 == 0) {
	if (s3 == 0)
	    return false; /* Collinear bars - out of interest. */
	if (0 <= dx2 && dx2 <= dx1 && 0 <= dy2 && dy2 <= dy1) {
	    /* The start of the bar 2 is in the bar 1. */
	    *ry = q[i2].y;
	    *ey = 0;
	    return true;
	}
    } else if (s3 == 0) {
	if (0 <= dx3 && dx3 <= dx1 && 0 <= dy3 && dy3 <= dy1) {
	    /* The end of the bar 2 is in the bar 1. */
	    *ry = q[i3].y;
	    *ey = 0;
	    return true;
	}
    } else if (s2 * s3 < 0) {
	/* The intersection definitely exists, so the determinant isn't zero.  */
	fixed d23x = dx3 - dx2, d23y = dy3 - dy2;
	int64_t det = (int64_t)dx1 * d23y - (int64_t)dy1 * d23x;
	int64_t mul = (int64_t)dx2 * d23y - (int64_t)dy2 * d23x;
#	define USE_DOUBLE 0
#	define USE_INT64_T (1 || !USE_DOUBLE)
#	if USE_DOUBLE
	{ 
	    /* Assuming big bars. Not a good thing due to 'double'.  */
	    /* The determinant can't compute in double due to 
	       possible loss of all significant bits when subtracting the 
	       trucnated prodicts. But after we subtract in int64_t,
	       it converts to 'double' with a reasonable truncation. */
	    double dy = dy1 * (double)mul / (double)det;
	    fixed iy;

	    if (dy1 > 0 && dy >= dy1)
		return false; /* Outside the bar 1. */
	    if (dy1 < 0 && dy <= dy1)
		return false; /* Outside the bar 1. */
	    if (dy2 < dy3) {
		if (dy <= dy2 || dy >= dy3)
		    return false; /* Outside the bar 2. */
	    } else {
		if (dy >= dy2 || dy <= dy3)
		    return false; /* Outside the bar 2. */
	    }
	    iy = (int)floor(dy);
	    *ry = q[i0].y + iy;
	    *ey = (dy > iy ? 1 : 0);
	}
#	endif
#	if USE_INT64_T
	{
	    /* Assuming small bars : cubes of coordinates must fit into int64_t.
	       curve_samples must provide that.  */
	    int64_t num = dy1 * mul, iiy;
	    fixed iy;
	    fixed pry, pey;

	    {	/* Likely when called form wedge_trap_decompose or constant_color_quadrangle,
		   we always have det > 0 && num >= 0, but we check here for a safety reason. */
		if (det < 0)
		    {num = -num; det = -det;}
		if(num >= 0)
			iiy = num / det;
		else
			iiy = (num - det + 1) / det;
		iy = (fixed)iiy;
		if (iy != iiy) {
		    /* If it is inside the bars, it must fit into fixed. */
		    return false;
		}
	    }
	    if (dy1 > 0 && iy >= dy1)
		return false; /* Outside the bar 1. */
	    if (dy1 < 0 && iy <= dy1)
		return false; /* Outside the bar 1. */
	    if (dy2 < dy3) {
		if (iy <= dy2 || iy >= dy3)
		    return false; /* Outside the bar 2. */
	    } else {
		if (iy >= dy2 || iy <= dy3)
		    return false; /* Outside the bar 2. */
	    }
	    pry = q[i0].y + (fixed)iy;
	    pey = (iy * det < num ? 1 : 0);
#	    if USE_DOUBLE && USE_INT64_T
		assert(*ry == pry);
		assert(*ey == pey);
#	    endif
	    *ry = pry;
	    *ey = pey;
	}
#	endif
	return true;
    }
    return false;
}

private inline void
adjust_swapped_boundary(fixed *b, bool swap_axes)
{
    if (swap_axes) {
	/*  Sinse the rasterizer algorithm assumes semi-open interval
	    when computing pixel coverage, we should expand
	    the right side of the area. Otherwise a dropout can happen :
	    if the left neighbour is painted with !swap_axes,
	    the left side of this area appears to be the left side 
	    of the neighbour area, and the side is not included
	    into both areas.
	 */
	*b += fixed_epsilon;
    }
}

private inline void
make_trapezoid(const gs_fixed_point q[4], 
	int vi0, int vi1, int vi2, int vi3, fixed ybot, fixed ytop, 
	bool swap_axes, bool orient, gs_fixed_edge *le, gs_fixed_edge *re)
{
    if (!orient) {
	le->start = q[vi0];
	le->end = q[vi1];
	re->start = q[vi2];
	re->end = q[vi3];
    } else {
	le->start = q[vi2];
	le->end = q[vi3];
	re->start = q[vi0];
	re->end = q[vi1];
    }
    adjust_swapped_boundary(&re->start.x, swap_axes);
    adjust_swapped_boundary(&re->end.x, swap_axes);
}

private inline int 
gx_shade_trapezoid(patch_fill_state_t *pfs, const gs_fixed_point q[4], 
	int vi0, int vi1, int vi2, int vi3, fixed ybot0, fixed ytop0, 
	bool swap_axes, const gx_device_color *pdevc, bool orient)
{
    gs_fixed_edge le, re;
    int code;
    fixed ybot = max(ybot0, swap_axes ? pfs->rect.p.x : pfs->rect.p.y);
    fixed ytop = min(ytop0, swap_axes ? pfs->rect.q.x : pfs->rect.q.y);
    vd_save;

    if (ybot > ytop)
	return 0;
#   if NOFILL_TEST
	if (dbg_nofill)
	    return 0;
#   endif
    make_trapezoid(q, vi0, vi1, vi2, vi3, ybot, ytop, swap_axes, orient, &le, &re);
    if (!VD_TRACE_DOWN)
	vd_disable;
    code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
	    &le, &re, ybot, ytop, swap_axes, pdevc, pfs->pis->log_op);
    vd_restore;
    return code;
}

private int
patch_color_to_device_color(const patch_fill_state_t *pfs, const patch_color_t *c, gx_device_color *pdevc)
{
    /* A code fragment copied from mesh_fill_triangle. */
    gs_client_color fcc;
    const gs_color_space *pcs = pfs->direct_space;

    memcpy(fcc.paint.values, c->cc.paint.values, 
		sizeof(fcc.paint.values[0]) * pfs->num_components);
    return pcs->type->remap_color(&fcc, pcs, pdevc, pfs->pis,
			      pfs->dev, gs_color_select_texture);
}

private inline double
color_span(const patch_fill_state_t *pfs, const patch_color_t *c0, const patch_color_t *c1)
{
    int n = pfs->num_components, i;
    double m;

    /* Dont want to copy colors, which are big things. */
    m = any_abs(c1->cc.paint.values[0] - c0->cc.paint.values[0]) / pfs->color_domain.paint.values[0];
    for (i = 1; i < n; i++)
	m = max(m, any_abs(c1->cc.paint.values[i] - c0->cc.paint.values[i]) / pfs->color_domain.paint.values[i]);
    return m;
}

private inline void
color_diff(const patch_fill_state_t *pfs, const patch_color_t *c0, const patch_color_t *c1, patch_color_t *d)
{
    int n = pfs->num_components, i;

    for (i = 0; i < n; i++)
	d->cc.paint.values[i] = c1->cc.paint.values[i] - c0->cc.paint.values[i];
}

private inline double
color_norm(const patch_fill_state_t *pfs, const patch_color_t *c)
{
    int n = pfs->num_components, i;
    double m;

    m = any_abs(c->cc.paint.values[0]) / pfs->color_domain.paint.values[0];
    for (i = 1; i < n; i++)
	m = max(m, any_abs(c->cc.paint.values[i]) / pfs->color_domain.paint.values[i]);
    return m;
}

private inline int
isnt_color_monotonic(const patch_fill_state_t *pfs, const patch_color_t *c0, const patch_color_t *c1)
{   /* checks whether the color is monotonic in the n-dimensional interval,
       where n is the number of parameters in c0->t, c1->t.
       returns : 0 = monotonic, 
       bit 0 = not or don't know by t0, 
       bit 1 = not or don't know by t1, 
       <0 = error. */
    /* When pfs->Function is not set, the color is monotonic.
       In this case do not call this function because 
       it doesn't check whether pfs->Function is set. 
       Actually pfs->monotonic_color prevents that.
     */
    uint mask;
    int code = gs_function_is_monotonic(pfs->Function, c0->t, c1->t, &mask);

    if (code >= 0)
	return mask;
    return code;
}

private inline bool
covers_pixel_centers(fixed ybot, fixed ytop)
{
    return fixed_pixround(ybot) < fixed_pixround(ytop);
}

private inline int
constant_color_trapezoid(patch_fill_state_t *pfs, gs_fixed_edge *le, gs_fixed_edge *re, 
	fixed ybot, fixed ytop, bool swap_axes, const patch_color_t *c)
{
    patch_color_t c1 = *c;
    gx_device_color dc;
    int code;
    vd_save;

#   if NOFILL_TEST
	/* if (dbg_nofill)
		return 0; */
#   endif
    code = patch_color_to_device_color(pfs, &c1, &dc);
    if (code < 0)
	return code;
    if (!VD_TRACE_DOWN)
	vd_disable;
    code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
	le, re, ybot, ytop, swap_axes, &dc, pfs->pis->log_op);
    vd_restore;
    return code;
}

private inline void
dc2fc(const patch_fill_state_t *pfs, gx_color_index c, 
	    frac31 fc[GX_DEVICE_COLOR_MAX_COMPONENTS])
{
    int j;
    const gx_device_color_info *cinfo = &pfs->dev->color_info;

    for (j = 0; j < cinfo->num_components; j++) {
	    int shift = cinfo->comp_shift[j];
	    int bits = cinfo->comp_bits[j];

	    fc[j] = ((c >> shift) & ((1 << bits) - 1)) << (sizeof(frac31) * 8 - 1 - bits);
    }
}

private inline float
function_linearity(const patch_fill_state_t *pfs, const patch_color_t *c0, const patch_color_t *c1)
{
    float smoothness = max(pfs->smoothness, 1.0 / min_linear_grades), s = 0;
    /* Restrict the smoothness with 1/min_linear_grades, because cs_is_linear
       can't provide a better precision due to the color
       representation with integers.
     */

    if (pfs->Function != NULL) {
	patch_color_t c;
	const float q[2] = {(float)0.3, (float)0.7};
	int i, j;

	for (j = 0; j < count_of(q); j++) {
	    c.t[0] = c0->t[0] * (1 - q[j]) + c1->t[0] * q[j];
	    c.t[1] = c0->t[1] * (1 - q[j]) + c1->t[1] * q[j];
	    patch_resolve_color_inline(&c, pfs);
	    for (i = 0; i < pfs->num_components; i++) {
		float v = c0->cc.paint.values[i] * (1 - q[j]) + c1->cc.paint.values[i] * q[j];
		float d = v - c.cc.paint.values[i];
		float s1 = any_abs(d) / pfs->color_domain.paint.values[i];

		if (s1 > smoothness)
		    return s1;
		if (s < s1)
		    s = s1;
	    }
	}
    }
    return s;
}

private inline int
is_color_linear(const patch_fill_state_t *pfs, const patch_color_t *c0, const patch_color_t *c1)
{   /* returns : 1 = linear, 0 = unlinear, <0 = error. */
    if (pfs->unlinear)
	return 1; /* Disable this check. */
    else {
	gs_direct_color_space *cs = 
		    (gs_direct_color_space *)pfs->direct_space; /* break 'const'. */
	int code;
	float smoothness = max(pfs->smoothness, 1.0 / min_linear_grades);
	/* Restrict the smoothness with 1/min_linear_grades, because cs_is_linear
	   can't provide a better precision due to the color
	   representation with integers.
	 */
	float s = function_linearity(pfs, c0, c1);

	if (s > smoothness)
	    return 0;
	code = cs_is_linear(cs, pfs->pis, pfs->dev, 
		&c0->cc, &c1->cc, NULL, NULL, smoothness - s);
	if (code <= 0)
	    return code;
	return 1;
    }
}

private int
decompose_linear_color(patch_fill_state_t *pfs, gs_fixed_edge *le, gs_fixed_edge *re, 
	fixed ybot, fixed ytop, bool swap_axes, const patch_color_t *c0, 
	const patch_color_t *c1, int level)
{
    /* Assuming a very narrow trapezoid - ignore the transversal color variation. */
    /* Assuming the XY span is restricted with curve_samples. 
       It is important for intersection_of_small_bars to compute faster. */
    int code;
    patch_color_t c;

    if (level > 100)
	return_error(gs_error_unregistered); /* Must not happen. */
    /* Use the recursive decomposition due to isnt_color_monotonic
       based on fn_is_monotonic_proc_t is_monotonic, 
       which applies to intervals. */
    patch_interpolate_color(&c, c0, c1, pfs, 0.5);
    if (ytop - ybot < fixed_1 / 2) /* Prevent an infinite color decomposition. */
	return constant_color_trapezoid(pfs, le, re, ybot, ytop, swap_axes, &c);
    else {  
	bool monotonic_color_save = pfs->monotonic_color;
	bool linear_color_save = pfs->linear_color;

	if (!pfs->monotonic_color) {
	    code = isnt_color_monotonic(pfs, c0, c1);
	    if (code < 0)
		return code;
	    if (!code)
		pfs->monotonic_color = true;
	}
	if (pfs->monotonic_color && !pfs->linear_color) {
	    code = is_color_linear(pfs, c0, c1);
	    if (code < 0)
		return code;
	    if (code > 0)
		pfs->linear_color =  true;
	}
	if (!pfs->unlinear && pfs->linear_color) {
	    gx_device *pdev = pfs->dev;
	    frac31 fc[2][GX_DEVICE_COLOR_MAX_COMPONENTS];
	    gs_fill_attributes fa;
	    gx_device_color dc[2];
	    gs_fixed_rect clip;
	    int code;

	    clip = pfs->rect;
	    if (swap_axes) {
		fixed v;

		v = clip.p.x; clip.p.x = clip.p.y; clip.p.y = v;
		v = clip.q.x; clip.q.x = clip.q.y; clip.q.y = v;
		/* Don't need adjust_swapped_boundary here. */
	    }
	    clip.p.y = max(clip.p.y, ybot);
	    clip.q.y = min(clip.q.y, ytop);
	    fa.clip = &clip; 
	    fa.ht = NULL;
	    fa.swap_axes = swap_axes;
	    fa.lop = 0;
	    fa.ystart = ybot;
	    fa.yend = ytop;
	    code = patch_color_to_device_color(pfs, c0, &dc[0]);
	    if (code < 0)
		return code;
	    if (dc[0].type == &gx_dc_type_data_pure) {
		dc2fc(pfs, dc[0].colors.pure, fc[0]);
		code = patch_color_to_device_color(pfs, c1, &dc[1]);
		if (code < 0)
		    return code;
		dc2fc(pfs, dc[1].colors.pure, fc[1]);
		code = dev_proc(pdev, fill_linear_color_trapezoid)(pdev, &fa, 
				&le->start, &le->end, &re->start, &re->end, 
				fc[0], fc[1], NULL, NULL);
		if (code == 1) {
		    pfs->monotonic_color = monotonic_color_save;
		    pfs->linear_color = linear_color_save;
		    return 0; /* The area is filled. */
		}
		if (code < 0)
		    return code;
		else /* code == 0, the device requested to decompose the area. */ 
		    return_error(gs_error_unregistered); /* Must not happen. */
	    }
	}
	if (!pfs->unlinear || !pfs->linear_color || 
		color_span(pfs, c0, c1) > pfs->smoothness) {
	    fixed y = (ybot + ytop) / 2;

	    code = decompose_linear_color(pfs, le, re, ybot, y, swap_axes, c0, &c, level + 1);
	    if (code >= 0)
		code = decompose_linear_color(pfs, le, re, y, ytop, swap_axes, &c, c1, level + 1);
	} else
	    code = constant_color_trapezoid(pfs, le, re, ybot, ytop, swap_axes, &c);
	pfs->monotonic_color = monotonic_color_save;
	pfs->linear_color = linear_color_save;
	return code;
    }
}

private inline int 
linear_color_trapezoid(patch_fill_state_t *pfs, gs_fixed_point q[4], int i0, int i1, int i2, int i3, 
		fixed ybot, fixed ytop, bool swap_axes, const patch_color_t *c0, const patch_color_t *c1, 
		bool orient)
{
    /* Assuming a very narrow trapezoid - ignore the transversal color change. */
    gs_fixed_edge le, re;

    make_trapezoid(q, i0, i1, i2, i3, ybot, ytop, swap_axes, orient, &le, &re);
    return decompose_linear_color(pfs, &le, &re, ybot, ytop, swap_axes, c0, c1, 0);
}

private int
wedge_trap_decompose(patch_fill_state_t *pfs, gs_fixed_point q[4],
	fixed ybot, fixed ytop, const patch_color_t *c0, const patch_color_t *c1, 
	bool swap_axes, bool self_intersecting)
{
    /* Assuming a very narrow trapezoid - ignore the transversal color change. */
    fixed dx1, dy1, dx2, dy2;
    bool orient;

    if (!pfs->vectorization && !covers_pixel_centers(ybot, ytop))
	return 0;
    if (ybot == ytop)
	return 0;
    dx1 = q[1].x - q[0].x, dy1 = q[1].y - q[0].y;
    dx2 = q[2].x - q[0].x, dy2 = q[2].y - q[0].y;
#if 1
    if (!swap_axes)
	vd_quad(q[0].x, q[0].y, q[1].x, q[1].y, q[3].x, q[3].y, q[2].x, q[2].y, 0, RGB(255, 0, 0));
    else
	vd_quad(q[0].y, q[0].x, q[1].y, q[1].x, q[3].y, q[3].x, q[2].y, q[2].x, 0, RGB(255, 0, 0));
#endif
    if ((int64_t)dx1 * dy2 != (int64_t)dy1 * dx2) {
	orient = ((int64_t)dx1 * dy2 > (int64_t)dy1 * dx2);
	return linear_color_trapezoid(pfs, q, 0, 1, 2, 3, ybot, ytop, swap_axes, c0, c1, orient);
    } else {
	fixed dx3 = q[3].x - q[0].x, dy3 = q[3].y - q[0].y;

	orient = ((int64_t)dx1 * dy3 > (int64_t)dy1 * dx3);
	return linear_color_trapezoid(pfs, q, 0, 1, 2, 3, ybot, ytop, swap_axes, c0, c1, orient);
    }
}

private inline int
fill_wedge_trap(patch_fill_state_t *pfs, const gs_fixed_point *p0, const gs_fixed_point *p1, 
	    const gs_fixed_point *q0, const gs_fixed_point *q1, const patch_color_t *c0, const patch_color_t *c1, 
	    bool swap_axes, bool self_intersecting)
{
    /* We assume that the width of the wedge is close to zero,
       so we can ignore the slope when computing transversal distances. */
    gs_fixed_point p[4];
    const patch_color_t *cc0, *cc1;

    if (p0->y < p1->y) {
	p[2] = *p0;
	p[3] = *p1;
	cc0 = c0;
	cc1 = c1;
    } else {
	p[2] = *p1;
	p[3] = *p0;
	cc0 = c1;
	cc1 = c0;
    }
    p[0] = *q0;
    p[1] = *q1;
    return wedge_trap_decompose(pfs, p, p[2].y, p[3].y, cc0, cc1, swap_axes, self_intersecting);
}

private void
split_curve_s(const gs_fixed_point *pole, gs_fixed_point *q0, gs_fixed_point *q1, int pole_step)
{
    /*	This copies a code fragment from split_curve_midpoint,
        substituting another data type. 
     */				
    /*
     * We have to define midpoint carefully to avoid overflow.
     * (If it overflows, something really pathological is going
     * on, but we could get infinite recursion that way....)
     */
#define midpoint(a,b)\
  (arith_rshift_1(a) + arith_rshift_1(b) + (((a) | (b)) & 1))
    fixed x12 = midpoint(pole[1 * pole_step].x, pole[2 * pole_step].x);
    fixed y12 = midpoint(pole[1 * pole_step].y, pole[2 * pole_step].y);

    /* q[0] and q[1] must not be the same as pole. */
    q0[1 * pole_step].x = midpoint(pole[0 * pole_step].x, pole[1 * pole_step].x);
    q0[1 * pole_step].y = midpoint(pole[0 * pole_step].y, pole[1 * pole_step].y);
    q1[2 * pole_step].x = midpoint(pole[2 * pole_step].x, pole[3 * pole_step].x);
    q1[2 * pole_step].y = midpoint(pole[2 * pole_step].y, pole[3 * pole_step].y);
    q0[2 * pole_step].x = midpoint(q0[1 * pole_step].x, x12);
    q0[2 * pole_step].y = midpoint(q0[1 * pole_step].y, y12);
    q1[1 * pole_step].x = midpoint(x12, q1[2 * pole_step].x);
    q1[1 * pole_step].y = midpoint(y12, q1[2 * pole_step].y);
    q0[0 * pole_step].x = pole[0 * pole_step].x;
    q0[0 * pole_step].y = pole[0 * pole_step].y;
    q0[3 * pole_step].x = q1[0 * pole_step].x = midpoint(q0[2 * pole_step].x, q1[1 * pole_step].x);
    q0[3 * pole_step].y = q1[0 * pole_step].y = midpoint(q0[2 * pole_step].y, q1[1 * pole_step].y);
    q1[3 * pole_step].x = pole[3 * pole_step].x;
    q1[3 * pole_step].y = pole[3 * pole_step].y;
#undef midpoint
}

private void
split_curve(const gs_fixed_point pole[4], gs_fixed_point q0[4], gs_fixed_point q1[4])
{
    split_curve_s(pole, q0, q1, 1);
}


private void
generate_inner_vertices(gs_fixed_point *p, const gs_fixed_point pole[4], int k)
{
    /* Recure to get exactly same points as when devided a patch. */
    /* An iteration can't give them preciselly. */
    if (k > 1) {
	gs_fixed_point q[2][4];

	split_curve(pole, q[0], q[1]);
	p[k / 2] = q[0][3];
	generate_inner_vertices(p, q[0], k / 2);
	generate_inner_vertices(p + k / 2, q[1], k / 2);
    }
}

private inline void
do_swap_axes(gs_fixed_point *p, int k)
{
    int i;

    for (i = 0; i < k; i++) {
	p[i].x ^= p[i].y; p[i].y ^= p[i].x; p[i].x ^= p[i].y;
    }
}

private inline void
y_extreme_vertice(gs_fixed_point *q, const gs_fixed_point *p, int k, int minmax)
{
    int i;
    gs_fixed_point r = *p;

    for (i = 1; i < k; i++)
	if ((p[i].y - r.y) * minmax > 0)
	    r = p[i];
    *q = r;	
}

private inline fixed
span_x(const gs_fixed_point *p, int k)
{
    int i;
    fixed xmin = p[0].x, xmax = p[0].x;

    for (i = 1; i < k; i++) {
	xmin = min(xmin, p[i].x);
	xmax = max(xmax, p[i].x);
    }
    return xmax - xmin;
}

private inline fixed
span_y(const gs_fixed_point *p, int k)
{
    int i;
    fixed ymin = p[0].y, ymax = p[0].y;

    for (i = 1; i < k; i++) {
	ymin = min(ymin, p[i].y);
	ymax = max(ymax, p[i].y);
    }
    return ymax - ymin;
}

private inline void
draw_wedge(const gs_fixed_point *p, int n)
{
#ifdef DEBUG
    int i;

    if (!vd_enabled)
	return;
    vd_setlinewidth(4);
    vd_setcolor(RGB(255, 0, 0));
    vd_beg_path;
    vd_moveto(p[0].x, p[0].y);
    for (i = 1; i < n; i++)
	vd_lineto(p[i].x, p[i].y);
    vd_closepath;
    vd_end_path;
    vd_fill;
    /*vd_stroke;*/
#endif
}

private inline fixed
manhattan_dist(const gs_fixed_point *p0, const gs_fixed_point *p1)
{
    fixed dx = any_abs(p1->x - p0->x), dy = any_abs(p1->y - p0->y);

    return max(dx, dy);
}

private inline void
create_wedge_vertex_list(patch_fill_state_t *pfs, wedge_vertex_list_t *l, 
	const gs_fixed_point *p0, const gs_fixed_point *p1)
{
    assert(l->end == NULL);
    l->beg = wedge_vertex_list_elem_reserve(pfs);
    l->end = wedge_vertex_list_elem_reserve(pfs);
    assert(l->beg != NULL);
    assert(l->end != NULL);
    l->beg->prev = l->end->next = NULL;
    l->beg->next = l->end;
    l->end->prev = l->beg;
    l->beg->p = *p0;
    l->end->p = *p1;
    l->beg->level = l->end->level = 0;
}

private inline wedge_vertex_list_elem_t *
insert_wedge_vertex_list_elem(patch_fill_state_t *pfs, wedge_vertex_list_t *l, const gs_fixed_point *p)
{
    wedge_vertex_list_elem_t *e = wedge_vertex_list_elem_reserve(pfs);

    /* We have got enough free elements due to the preliminary decomposition 
       of curves to LAZY_WEDGES_MAX_LEVEL, see curve_samples. */
    assert(e != NULL); 
    assert(l->beg->next == l->end);
    assert(l->end->prev == l->beg);
    e->next = l->end;
    e->prev = l->beg;
    e->p = *p;
    e->level = max(l->beg->level, l->end->level) + 1;
    e->divide_count = 0;
    l->beg->next = l->end->prev = e;
    {	int sx = l->beg->p.x < l->end->p.x ? 1 : -1;
	int sy = l->beg->p.y < l->end->p.y ? 1 : -1;

	assert((p->x - l->beg->p.x) * sx >= 0);
	assert((p->y - l->beg->p.y) * sy >= 0);
	assert((l->end->p.x - p->x) * sx >= 0);
	assert((l->end->p.y - p->y) * sy >= 0);
    }
    return e;
}

private inline wedge_vertex_list_elem_t *
open_wedge_median(patch_fill_state_t *pfs, wedge_vertex_list_t *l,
	const gs_fixed_point *p0, const gs_fixed_point *p1, const gs_fixed_point *pm)
{
    wedge_vertex_list_elem_t *e;

    if (!l->last_side) {
	if (l->beg == NULL)
	    create_wedge_vertex_list(pfs, l, p0, p1);
	assert(l->beg->p.x == p0->x);
	assert(l->beg->p.y == p0->y);
	assert(l->end->p.x == p1->x);
	assert(l->end->p.y == p1->y);
	e = insert_wedge_vertex_list_elem(pfs, l, pm);
	e->divide_count++;
	return e;
    } else {
	if (l->beg == NULL) {
	    create_wedge_vertex_list(pfs, l, p1, p0);
	    e = insert_wedge_vertex_list_elem(pfs, l, pm);
	    e->divide_count++;
	    return e;
	}
	assert(l->beg->p.x == p1->x);
	assert(l->beg->p.y == p1->y);
	assert(l->end->p.x == p0->x);
	assert(l->end->p.y == p0->y);
	if (l->beg->next == l->end) {
	    e = insert_wedge_vertex_list_elem(pfs, l, pm);
	    e->divide_count++;
	    return e;
	} else {
	    e = wedge_vertex_list_find(l->beg, l->end, 
			max(l->beg->level, l->end->level) + 1);
	    assert(e != NULL);
	    assert(e->p.x == pm->x && e->p.y == pm->y);
    	    e->divide_count++;
	    return e;
	}
    }
}

private inline void
make_wedge_median(patch_fill_state_t *pfs, wedge_vertex_list_t *l, 
	wedge_vertex_list_t *l0, bool forth, 
	const gs_fixed_point *p0, const gs_fixed_point *p1, const gs_fixed_point *pm)
{
    l->last_side = l0->last_side;
    if (!l->last_side ^ !forth) {
	l->end = open_wedge_median(pfs, l0, p0, p1, pm);
	l->beg = l0->beg;
    } else {
	l->beg = open_wedge_median(pfs, l0, p0, p1, pm);
	l->end = l0->end;
    }
}

private int fill_wedge_from_list(patch_fill_state_t *pfs, const wedge_vertex_list_t *l,
	    const patch_color_t *c0, const patch_color_t *c1);

private inline int
close_wedge_median(patch_fill_state_t *pfs, wedge_vertex_list_t *l,
	const patch_color_t *c0, const patch_color_t *c1)
{
    int code;

    if (!l->last_side)
	return 0;
    code = fill_wedge_from_list(pfs, l, c1, c0);
    if (code < 0)
	return code;
    release_wedge_vertex_list_interval(pfs, l->beg, l->end);
    return 0;
}

private inline void
move_wedge(wedge_vertex_list_t *l, const wedge_vertex_list_t *l0, bool forth)
{
    if (!l->last_side ^ !forth) {
	l->beg = l->end;
	l->end = l0->end;
    } else {
	l->end = l->beg;
	l->beg = l0->beg;
    }
}

private inline int 
fill_triangle_wedge_aux(patch_fill_state_t *pfs,
	    const shading_vertex_t *q0, const shading_vertex_t *q1, const shading_vertex_t *q2)
{   int code;
    const gs_fixed_point *p0, *p1, *p2;
    gs_fixed_point qq0, qq1, qq2;
    fixed dx = any_abs(q0->p.x - q1->p.x), dy = any_abs(q0->p.y - q1->p.y);
    bool swap_axes;

#   if SKIP_TEST
	dbg_wedge_triangle_cnt++;
#   endif
    if (dx > dy) {
	swap_axes = true;
	qq0.x = q0->p.y;
	qq0.y = q0->p.x;
	qq1.x = q1->p.y;
	qq1.y = q1->p.x;
	qq2.x = q2->p.y;
	qq2.y = q2->p.x;
	p0 = &qq0;
	p1 = &qq1;
	p2 = &qq2;
    } else {
	swap_axes = false;
	p0 = &q0->p;
	p1 = &q1->p;
	p2 = &q2->p;
    }
    /* We decompose the thin triangle into 2 thin trapezoids.
       An optimization with decomposing into 2 triangles
       appears low useful, because the self_intersecting argument
       with inline expansion does that job perfectly. */
    if (p0->y < p1->y) {
	code = fill_wedge_trap(pfs, p0, p2, p0, p1, &q0->c, &q2->c, swap_axes, false);
	if (code < 0)
	    return code;
	return fill_wedge_trap(pfs, p2, p1, p0, p1, &q2->c, &q1->c, swap_axes, false);
    } else {
	code = fill_wedge_trap(pfs, p0, p2, p1, p0, &q0->c, &q2->c, swap_axes, false);
	if (code < 0)
	    return code;
	return fill_wedge_trap(pfs, p2, p1, p1, p0, &q2->c, &q1->c, swap_axes, false);
    }
}

private inline int
try_device_linear_color(patch_fill_state_t *pfs, bool wedge,
	const shading_vertex_t *p0, const shading_vertex_t *p1, 
	const shading_vertex_t *p2)
{
    /*	Returns :
	<0 - error;
	0 - success;
	1 - decompose to linear color areas;
	2 - decompose to constant color areas;
     */
    int code;

    if (pfs->unlinear)
	return 2;
    if (!wedge) {
	gs_direct_color_space *cs = 
		(gs_direct_color_space *)pfs->direct_space; /* break 'const'. */
	float smoothness = max(pfs->smoothness, 1.0 / min_linear_grades);
	/* Restrict the smoothness with 1/min_linear_grades, because cs_is_linear
	   can't provide a better precision due to the color
	   representation with integers.
	 */
	float s0, s1, s2, s01, s012;

	s0 = function_linearity(pfs, &p0->c, &p1->c);
	if (s0 > smoothness)
	    return 1;
	s1 = function_linearity(pfs, &p1->c, &p2->c);
	if (s1 > smoothness)
	    return 1;
	s2 = function_linearity(pfs, &p2->c, &p0->c);
	if (s2 > smoothness)
	    return 1;
	/* fixme: check an inner color ? */
	s01 = max(s0, s1);
	s012 = max(s01, s2);
	code = cs_is_linear(cs, pfs->pis, pfs->dev, 
			    &p0->c.cc, &p1->c.cc, &p2->c.cc, NULL, smoothness - s012);
	if (code < 0)
	    return code;
	if (code == 0)
	    return 1;
    }
    {   gx_device *pdev = pfs->dev;
	frac31 fc[3][GX_DEVICE_COLOR_MAX_COMPONENTS];
	gs_fill_attributes fa;
	gx_device_color dc[3];
	vd_save;

	fa.clip = &pfs->rect;
	fa.ht = NULL;
	fa.swap_axes = false;
	fa.lop = 0;
	code = patch_color_to_device_color(pfs, &p0->c, &dc[0]);
	if (code < 0)
	    return code;
	if (dc[0].type != &gx_dc_type_data_pure)
	    return 2;
	dc2fc(pfs, dc[0].colors.pure, fc[0]);
	if (!wedge) {
	    code = patch_color_to_device_color(pfs, &p1->c, &dc[1]);
	    if (code < 0)
		return code;
	    dc2fc(pfs, dc[1].colors.pure, fc[1]);
	}
	code = patch_color_to_device_color(pfs, &p2->c, &dc[2]);
	if (code < 0)
	    return code;
	dc2fc(pfs, dc[2].colors.pure, fc[2]);
	draw_triangle(&p0->p, &p1->p, &p2->p, RGB(255, 0, 0));
	if (!VD_TRACE_DOWN)
	    vd_disable;
	code = dev_proc(pdev, fill_linear_color_triangle)(pdev, &fa, 
			&p0->p, &p1->p, &p2->p, 
			fc[0], (wedge ? NULL : fc[1]), fc[2]);
	vd_restore;
	if (code == 1)
	    return 0; /* The area is filled. */
	if (code < 0)
	    return code;
	else /* code == 0, the device requested to decompose the area. */ 
	    return 1;
    }
}

private inline int 
fill_triangle_wedge(patch_fill_state_t *pfs,
	    const shading_vertex_t *q0, const shading_vertex_t *q1, const shading_vertex_t *q2)
{
    if ((int64_t)(q1->p.x - q0->p.x) * (q2->p.y - q0->p.y) == 
	(int64_t)(q1->p.y - q0->p.y) * (q2->p.x - q0->p.x))
	return 0; /* Zero area. */
    draw_triangle(&q0->p, &q1->p, &q2->p, RGB(255, 255, 0));
    /*
	Can't apply try_device_linear_color here
	because didn't check is_color_linear.
	Maybe need a decomposition.
	Do same as for 'unlinear', and branch later.
     */
    return fill_triangle_wedge_aux(pfs, q0, q1, q2);
}

private inline int
fill_triangle_wedge_from_list(patch_fill_state_t *pfs, 
    const wedge_vertex_list_elem_t *beg, const wedge_vertex_list_elem_t *end, 
    const wedge_vertex_list_elem_t *mid,
    const patch_color_t *c0, const patch_color_t *c1)
{
    shading_vertex_t p[3];

    p[0].p = beg->p;
    p[0].c = *c0; /* fixme : unhappy copying colors. */
    p[1].p = end->p;
    p[1].c = *c1;
    p[2].p = mid->p;
    patch_interpolate_color(&p[2].c, c0, c1, pfs, 0.5);
    return fill_triangle_wedge(pfs, &p[0], &p[1], &p[2]);
}

private int 
fill_wedge_from_list_rec(patch_fill_state_t *pfs, 
	    wedge_vertex_list_elem_t *beg, const wedge_vertex_list_elem_t *end, 
	    int level, const patch_color_t *c0, const patch_color_t *c1)
{
    if (beg->next == end)
	return 0;
    else if (beg->next->next == end) {
	assert(beg->next->divide_count == 1 || beg->next->divide_count == 2);
	if (beg->next->divide_count != 1)
	    return 0;
	return fill_triangle_wedge_from_list(pfs, beg, end, beg->next, c0, c1);
    } else {
	gs_fixed_point p;
	wedge_vertex_list_elem_t *e;
	patch_color_t c;
	int code;

	p.x = (beg->p.x + end->p.x) / 2;
	p.y = (beg->p.y + end->p.y) / 2;
	e = wedge_vertex_list_find(beg, end, level + 1);
	assert(e != NULL);
	assert(e->p.x == p.x && e->p.y == p.y);
	patch_interpolate_color(&c, c0, c1, pfs, 0.5);
	code = fill_wedge_from_list_rec(pfs, beg, e, level + 1, c0, &c);
	if (code < 0)
	    return code;
	code = fill_wedge_from_list_rec(pfs, e, end, level + 1, &c, c1);
	if (code < 0)
	    return code;
	assert(e->divide_count == 1 || e->divide_count == 2);
	if (e->divide_count != 1)
	    return 0;
	return fill_triangle_wedge_from_list(pfs, beg, end, e, c0, c1);
    }
}

private int 
fill_wedge_from_list(patch_fill_state_t *pfs, const wedge_vertex_list_t *l,
	    const patch_color_t *c0, const patch_color_t *c1)
{
    return fill_wedge_from_list_rec(pfs, l->beg, l->end, 
		    max(l->beg->level, l->end->level), c0, c1);
}

private inline int
terminate_wedge_vertex_list(patch_fill_state_t *pfs, wedge_vertex_list_t *l,
	const patch_color_t *c0, const patch_color_t *c1)
{
    if (l->beg != NULL) {
	int code = fill_wedge_from_list(pfs, l, c0, c1);

	if (code < 0)
	    return code;
	release_wedge_vertex_list(pfs, l, 1);
    }
    return 0;
}

private int
wedge_by_triangles(patch_fill_state_t *pfs, int ka, 
	const gs_fixed_point pole[4], const patch_color_t *c0, const patch_color_t *c1)
{   /* Assuming ka >= 2, see fill_wedges. */
    gs_fixed_point q[2][4];
    shading_vertex_t p[3];
    int code;

    split_curve(pole, q[0], q[1]);
    p[0].p = pole[0];
    p[0].c = *c0; /* fixme : unhappy copying colors. */
    p[1].p = pole[3];
    p[1].c = *c1;
    p[2].p = q[0][3];
    patch_interpolate_color(&p[2].c, c0, c1, pfs, 0.5);
    code = fill_triangle_wedge(pfs, &p[0], &p[1], &p[2]);
    if (code < 0)
	return code;
    if (ka == 2)
	return 0;
    code = wedge_by_triangles(pfs, ka / 2, q[0], c0, &p[2].c);
    if (code < 0)
	return code;
    return wedge_by_triangles(pfs, ka / 2, q[1], &p[2].c, c1);
}

private inline bool
is_linear_color_applicable(const patch_fill_state_t *pfs)
{
    if (!USE_LINEAR_COLOR_PROCS)
	return false;
    if (pfs->dev->color_info.separable_and_linear != GX_CINFO_SEP_LIN)
	return false;
    if (gx_get_cmap_procs(pfs->pis, pfs->dev)->is_halftoned(pfs->pis, pfs->dev))
	return false;
    return true;
}

int
mesh_padding(patch_fill_state_t *pfs, const gs_fixed_point *p0, const gs_fixed_point *p1, 
	    const patch_color_t *c0, const patch_color_t *c1)
{
    gs_fixed_point q0, q1;
    const patch_color_t *cc0, *cc1;
    fixed dx = p1->x - p0->x;
    fixed dy = p1->y - p0->y;
    bool swap_axes = (any_abs(dx) > any_abs(dy));
    gs_fixed_edge le, re;
    const fixed adjust = INTERPATCH_PADDING;

    pfs->unlinear = !is_linear_color_applicable(pfs);
    if (swap_axes) {
	if (p0->x < p1->x) {
	    q0.x = p0->y;
	    q0.y = p0->x;
	    q1.x = p1->y;
	    q1.y = p1->x;
	    cc0 = c0;
	    cc1 = c1;
	} else {
	    q0.x = p1->y;
	    q0.y = p1->x;
	    q1.x = p0->y;
	    q1.y = p0->x;
	    cc0 = c1;
	    cc1 = c0;
	}
    } else if (p0->y < p1->y) {
	q0 = *p0;
	q1 = *p1;
	cc0 = c0;
	cc1 = c1;
    } else {
	q0 = *p1;
	q1 = *p0;
	cc0 = c1;
	cc1 = c0;
    }
    le.start.x = q0.x - adjust;
    re.start.x = q0.x + adjust;
    le.start.y = re.start.y = q0.y - adjust;
    le.end.x = q1.x - adjust;
    re.end.x = q1.x + adjust;
    le.end.y = re.end.y = q1.y + adjust;
    adjust_swapped_boundary(&re.start.x, swap_axes);
    adjust_swapped_boundary(&re.end.x, swap_axes);
    return decompose_linear_color(pfs, &le, &re, le.start.y, le.end.y, swap_axes, cc0, cc1, 0);
    /* fixme : for a better performance and quality, we would like to 
       consider the bar as an oriented one and to know at what side of it the spot resides.
       If we know that, we could expand only to outside the spot.
       Note that if the boundary has a self-intersection,
       we still need to expand to both directions.
     */
}

private int
fill_wedges_aux(patch_fill_state_t *pfs, int k, int ka, 
	const gs_fixed_point pole[4], const patch_color_t *c0, const patch_color_t *c1,
	int wedge_type)
{
    int code;

    if (k > 1) {
	gs_fixed_point q[2][4];
	patch_color_t c;

	patch_interpolate_color(&c, c0, c1, pfs, 0.5);
	split_curve(pole, q[0], q[1]);
	code = fill_wedges_aux(pfs, k / 2, ka, q[0], c0, &c, wedge_type);
	if (code < 0)
	    return code;
	return fill_wedges_aux(pfs, k / 2, ka, q[1], &c, c1, wedge_type);
    } else {
	if (INTERPATCH_PADDING && (wedge_type & interpatch_padding)) {
	    vd_bar(pole[0].x, pole[0].y, pole[3].x, pole[3].y, 0, RGB(255, 0, 0));
	    code = mesh_padding(pfs, &pole[0], &pole[3], c0, c1);
	    if (code < 0)
		return code;
	}
	if (ka >= 2 && (wedge_type & inpatch_wedge))
	    return wedge_by_triangles(pfs, ka, pole, c0, c1);
	return 0;
    }
}

private int
fill_wedges(patch_fill_state_t *pfs, int k0, int k1, 
	const gs_fixed_point *pole, int pole_step, 
	const patch_color_t *c0, const patch_color_t *c1, int wedge_type)
{
    /* Generate wedges between 2 variants of a curve flattening. */
    /* k0, k1 is a power of 2. */
    gs_fixed_point p[4];

    if (!(wedge_type & interpatch_padding) && k0 == k1)
	return 0; /* Wedges are zero area. */
    if (k0 > k1) {
	k0 ^= k1; k1 ^= k0; k0 ^= k1;
    }
    p[0] = pole[0];
    p[1] = pole[pole_step];
    p[2] = pole[pole_step * 2];
    p[3] = pole[pole_step * 3];
    return fill_wedges_aux(pfs, k0, k1 / k0, p, c0, c1, wedge_type);
}

private inline void
make_vertices(gs_fixed_point q[4], const quadrangle_patch *p)
{
    q[0] = p->p[0][0]->p;
    q[1] = p->p[0][1]->p;
    q[2] = p->p[1][1]->p;
    q[3] = p->p[1][0]->p;
}

private inline void
wrap_vertices_by_y(gs_fixed_point q[4], const gs_fixed_point s[4])
{
    fixed y = s[0].y;
    int i = 0;

    if (y > s[1].y)
	i = 1, y = s[1].y;
    if (y > s[2].y)
	i = 2, y = s[2].y;
    if (y > s[3].y)
	i = 3, y = s[3].y;
    q[0] = s[(i + 0) % 4];
    q[1] = s[(i + 1) % 4];
    q[2] = s[(i + 2) % 4];
    q[3] = s[(i + 3) % 4];
}

private int 
ordered_triangle(patch_fill_state_t *pfs, gs_fixed_edge *le, gs_fixed_edge *re, patch_color_t *c)
{
    gs_fixed_edge ue;
    int code;
    gx_device_color dc;
    vd_save;

#   if NOFILL_TEST
	if (dbg_nofill)
	    return 0;
#   endif
    if (!VD_TRACE_DOWN)
        vd_disable;
    code = patch_color_to_device_color(pfs, c, &dc);
    if (code < 0)
	return code;
    if (le->end.y < re->end.y) {
	code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
	    le, re, le->start.y, le->end.y, false, &dc, pfs->pis->log_op);
	if (code >= 0) {
	    ue.start = le->end;
	    ue.end = re->end;
	    code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
		&ue, re, le->end.y, re->end.y, false, &dc, pfs->pis->log_op);
	}
    } else if (le->end.y > re->end.y) {
	code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
	    le, re, le->start.y, re->end.y, false, &dc, pfs->pis->log_op);
	if (code >= 0) {
	    ue.start = re->end;
	    ue.end = le->end;
	    code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
		le, &ue, re->end.y, le->end.y, false, &dc, pfs->pis->log_op);
	}
    } else
	code = dev_proc(pfs->dev, fill_trapezoid)(pfs->dev,
	    le, re, le->start.y, le->end.y, false, &dc, pfs->pis->log_op);
    vd_restore;
    return code;
}

private int 
constant_color_triangle(patch_fill_state_t *pfs,
	const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2)
{
    patch_color_t c, cc;
    gs_fixed_edge le, re;
    fixed dx0, dy0, dx1, dy1;
    const shading_vertex_t *pp;
    int i;

    draw_triangle(&p0->p, &p1->p, &p2->p, RGB(255, 0, 0));
    patch_interpolate_color(&c, &p0->c, &p1->c, pfs, 0.5);
    patch_interpolate_color(&cc, &p2->c, &c, pfs, 0.5);
    for (i = 0; i < 3; i++) {
	/* fixme : does optimizer compiler expand this cycle ? */
	if (p0->p.y <= p1->p.y && p0->p.y <= p2->p.y) {
	    le.start = re.start = p0->p;
	    le.end = p1->p; 
	    re.end = p2->p;

	    dx0 = le.end.x - le.start.x;
	    dy0 = le.end.y - le.start.y;
	    dx1 = re.end.x - re.start.x;
	    dy1 = re.end.y - re.start.y;
	    if ((int64_t)dx0 * dy1 < (int64_t)dy0 * dx1)
    		return ordered_triangle(pfs, &le, &re, &c);
	    else
    		return ordered_triangle(pfs, &re, &le, &c);
	}
	pp = p0; p0 = p1; p1 = p2; p2 = pp;
    }
    return 0;
}


private int 
constant_color_quadrangle(patch_fill_state_t *pfs, const quadrangle_patch *p, bool self_intersecting)
{
    /* Assuming the XY span is restricted with curve_samples. 
       It is important for intersection_of_small_bars to compute faster. */
    gs_fixed_point q[4];
    fixed ry, ey;
    int code;
    bool swap_axes = false;
    gx_device_color dc;
    patch_color_t c1, c2, c;
    bool orient;

    draw_quadrangle(p, RGB(0, 255, 0));
    patch_interpolate_color(&c1, &p->p[0][0]->c, &p->p[0][1]->c, pfs, 0.5);
    patch_interpolate_color(&c2, &p->p[1][0]->c, &p->p[1][1]->c, pfs, 0.5);
    patch_interpolate_color(&c, &c1, &c2, pfs, 0.5);
    code = patch_color_to_device_color(pfs, &c, &dc);
    if (code < 0)
	return code;
    {	gs_fixed_point qq[4];

	make_vertices(qq, p);
#	if 0 /* Swapping axes may improve the precision, 
		but slows down due to the area expantion needed
		in gx_shade_trapezoid. */
	    dx = span_x(qq, 4);
	    dy = span_y(qq, 4);
	    if (dy < dx) {
		do_swap_axes(qq, 4);
		swap_axes = true;
	    }
#	endif
	wrap_vertices_by_y(q, qq);
    }
    {	fixed dx1 = q[1].x - q[0].x, dy1 = q[1].y - q[0].y;
	fixed dx3 = q[3].x - q[0].x, dy3 = q[3].y - q[0].y;
	int64_t g13 = (int64_t)dx1 * dy3, h13 = (int64_t)dy1 * dx3;

	if (g13 == h13) {
	    fixed dx2 = q[2].x - q[0].x, dy2 = q[2].y - q[0].y;
	    int64_t g23 = (int64_t)dx2 * dy3, h23 = (int64_t)dy2 * dx3;

	    if (dx1 == 0 && dy1 == 0 && g23 == h23)
		return 0;
	    if (g23 != h23) {
		orient = (g23 > h23);
		if (q[2].y <= q[3].y) {
		    if ((code = gx_shade_trapezoid(pfs, q, 1, 2, 0, 3, q[1].y, q[2].y, swap_axes, &dc, orient)) < 0)
			return code;
		    return gx_shade_trapezoid(pfs, q, 2, 3, 0, 3, q[2].y, q[3].y, swap_axes, &dc, orient);
		} else {
		    if ((code = gx_shade_trapezoid(pfs, q, 1, 2, 0, 3, q[1].y, q[3].y, swap_axes, &dc, orient)) < 0)
			return code;
		    return gx_shade_trapezoid(pfs, q, 1, 2, 3, 2, q[3].y, q[2].y, swap_axes, &dc, orient);
		}
	    } else {
		int64_t g12 = (int64_t)dx1 * dy2, h12 = (int64_t)dy1 * dx2;

		if (dx3 == 0 && dy3 == 0 && g12 == h12)
		    return 0;
		orient = (g12 > h12);
		if (q[1].y <= q[2].y) {
		    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 3, 2, q[0].y, q[1].y, swap_axes, &dc, orient)) < 0)
			return code;
		    return gx_shade_trapezoid(pfs, q, 1, 2, 3, 2, q[1].y, q[2].y, swap_axes, &dc, orient);
		} else {
		    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 3, 2, q[0].y, q[2].y, swap_axes, &dc, orient)) < 0)
			return code;
		    return gx_shade_trapezoid(pfs, q, 0, 1, 2, 1, q[2].y, q[1].y, swap_axes, &dc, orient);
		}
	    }
	}
	orient = ((int64_t)dx1 * dy3 > (int64_t)dy1 * dx3);
    }
    if (q[1].y <= q[2].y && q[2].y <= q[3].y) {
	if (self_intersecting && intersection_of_small_bars(q, 0, 3, 1, 2, &ry, &ey)) {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 3, 1, 2, q[1].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 1, 2, 0, 3, ry, q[2].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 0, 3, 2, 3, q[2].y, q[3].y, swap_axes, &dc, orient);
	} else {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 1, 2, 0, 3, q[1].y, q[2].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 2, 3, 0, 3, q[2].y, q[3].y, swap_axes, &dc, orient);
	}
    } else if (q[1].y <= q[3].y && q[3].y <= q[2].y) {
	if (self_intersecting && intersection_of_small_bars(q, 0, 3, 1, 2, &ry, &ey)) {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 1, 2, 0, 3, q[1].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 3, 1, 2, ry, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 3, 2, 1, 2, q[3].y, q[2].y, swap_axes, &dc, orient);
	} else {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 1, 2, 0, 3, q[1].y, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 1, 2, 3, 2, q[3].y, q[2].y, swap_axes, &dc, orient);
	}
    } else if (q[2].y <= q[1].y && q[1].y <= q[3].y) {
	if (self_intersecting && intersection_of_small_bars(q, 0, 1, 2, 3, &ry, &ey)) {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 1, 2, 3, q[2].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 1, 0, 1, ry, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 2, 3, 0, 3, ry, q[3].y, swap_axes, &dc, orient);
	} else if (self_intersecting && intersection_of_small_bars(q, 0, 3, 1, 2, &ry, &ey)) {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 1, 2, 3, q[2].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 2, 1, ry, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 0, 3, 2, 3, ry, q[3].y, swap_axes, &dc, orient);
	} else {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 3, 2, 1, q[2].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 2, 3, 0, 3, q[1].y, q[3].y, swap_axes, &dc, orient);
	}
    } else if (q[2].y <= q[3].y && q[3].y <= q[1].y) {
	if (self_intersecting && intersection_of_small_bars(q, 0, 1, 2, 3, &ry, &ey)) {
	    /* Same code as someone above. */
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 1, 2, 3, q[2].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 1, 0, 1, ry, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 2, 3, 0, 3, ry, q[3].y, swap_axes, &dc, orient);
	} else if (self_intersecting && intersection_of_small_bars(q, 0, 3, 2, 1, &ry, &ey)) {
	    /* Same code as someone above. */
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 1, 2, 3, q[2].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 2, 1, ry, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 0, 3, 2, 3, ry, q[3].y, swap_axes, &dc, orient);
	} else {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[2].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 2, 3, 0, 3, q[2].y, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 0, 1, 2, 1, q[2].y, q[1].y, swap_axes, &dc, orient);
	}
    } else if (q[3].y <= q[1].y && q[1].y <= q[2].y) {
	if (self_intersecting && intersection_of_small_bars(q, 0, 1, 3, 2, &ry, &ey)) {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 3, 2, q[3].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 3, 2, 0, 1, ry, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 3, 2, 1, 2, q[1].y, q[2].y, swap_axes, &dc, orient);
	} else {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 3, 2, q[3].y, q[1].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 1, 2, 3, 2, q[1].y, q[2].y, swap_axes, &dc, orient);
	}
    } else if (q[3].y <= q[2].y && q[2].y <= q[1].y) {
	if (self_intersecting && intersection_of_small_bars(q, 0, 1, 2, 3, &ry, &ey)) {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 3, 2, q[3].y, ry + ey, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 3, 2, 0, 1, ry, q[2].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 2, 1, 0, 1, q[2].y, q[1].y, swap_axes, &dc, orient);
	} else {
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 0, 3, q[0].y, q[3].y, swap_axes, &dc, orient)) < 0)
		return code;
	    if ((code = gx_shade_trapezoid(pfs, q, 0, 1, 3, 2, q[3].y, q[2].y, swap_axes, &dc, orient)) < 0)
		return code;
	    return gx_shade_trapezoid(pfs, q, 0, 1, 2, 1, q[2].y, q[1].y, swap_axes, &dc, orient);
	}
    } else {
	/* Impossible. */
	return_error(gs_error_unregistered);
    }
}

private inline void
divide_quadrangle_by_v(patch_fill_state_t *pfs, quadrangle_patch *s0, quadrangle_patch *s1, 
	    shading_vertex_t q[2], const quadrangle_patch *p)
{
    q[0].p.x = (p->p[0][0]->p.x + p->p[1][0]->p.x) / 2;
    q[1].p.x = (p->p[0][1]->p.x + p->p[1][1]->p.x) / 2;
    q[0].p.y = (p->p[0][0]->p.y + p->p[1][0]->p.y) / 2;
    q[1].p.y = (p->p[0][1]->p.y + p->p[1][1]->p.y) / 2;
    patch_interpolate_color(&q[0].c, &p->p[0][0]->c, &p->p[1][0]->c, pfs, 0.5);
    patch_interpolate_color(&q[1].c, &p->p[0][1]->c, &p->p[1][1]->c, pfs, 0.5);
    s0->p[0][0] = p->p[0][0];
    s0->p[0][1] = p->p[0][1];
    s0->p[1][0] = s1->p[0][0] = &q[0];
    s0->p[1][1] = s1->p[0][1] = &q[1];
    s1->p[1][0] = p->p[1][0];
    s1->p[1][1] = p->p[1][1];
}

private inline void
divide_quadrangle_by_u(patch_fill_state_t *pfs, quadrangle_patch *s0, quadrangle_patch *s1, 
	    shading_vertex_t q[2], const quadrangle_patch *p)
{
    q[0].p.x = (p->p[0][0]->p.x + p->p[0][1]->p.x) / 2;
    q[1].p.x = (p->p[1][0]->p.x + p->p[1][1]->p.x) / 2;
    q[0].p.y = (p->p[0][0]->p.y + p->p[0][1]->p.y) / 2;
    q[1].p.y = (p->p[1][0]->p.y + p->p[1][1]->p.y) / 2;
    patch_interpolate_color(&q[0].c, &p->p[0][0]->c, &p->p[0][1]->c, pfs, 0.5);
    patch_interpolate_color(&q[1].c, &p->p[1][0]->c, &p->p[1][1]->c, pfs, 0.5);
    s0->p[0][0] = p->p[0][0];
    s0->p[1][0] = p->p[1][0];
    s0->p[0][1] = s1->p[0][0] = &q[0];
    s0->p[1][1] = s1->p[1][0] = &q[1];
    s1->p[0][1] = p->p[0][1];
    s1->p[1][1] = p->p[1][1];
}

private inline int
is_quadrangle_color_monotonic(const patch_fill_state_t *pfs, const quadrangle_patch *p, 
			      bool *not_monotonic_by_u, bool *not_monotonic_by_v) 
{   /* returns : 1 = monotonic, 0 = don't know, <0 = error. */
    int code;

    code = isnt_color_monotonic(pfs, &p->p[0][0]->c, &p->p[1][1]->c);
    if (code <= 0)
	return code;
    if (code & 1)
	*not_monotonic_by_v = true;
    if (code & 2)
	*not_monotonic_by_u = true;
    return !code;
}

private inline bool
quadrangle_bbox_covers_pixel_centers(const quadrangle_patch *p)
{
    fixed xbot, xtop, ybot, ytop;
    
    xbot = min(min(p->p[0][0]->p.x, p->p[0][1]->p.x),
	       min(p->p[1][0]->p.x, p->p[1][1]->p.x));
    xtop = max(max(p->p[0][0]->p.x, p->p[0][1]->p.x),
	       max(p->p[1][0]->p.x, p->p[1][1]->p.x));
    if (covers_pixel_centers(xbot, xtop))
	return true;
    ybot = min(min(p->p[0][0]->p.y, p->p[0][1]->p.y),
	       min(p->p[1][0]->p.y, p->p[1][1]->p.y));
    ytop = max(max(p->p[0][0]->p.y, p->p[0][1]->p.y),
	       max(p->p[1][0]->p.y, p->p[1][1]->p.y));
    if (covers_pixel_centers(ybot, ytop))
	return true;
    return false;
}

private inline void
divide_bar(patch_fill_state_t *pfs, 
	const shading_vertex_t *p0, const shading_vertex_t *p1, int radix, shading_vertex_t *p)
{
    p->p.x = (fixed)((int64_t)p0->p.x * (radix - 1) + p1->p.x) / radix;
    p->p.y = (fixed)((int64_t)p0->p.y * (radix - 1) + p1->p.y) / radix;
    patch_interpolate_color(&p->c, &p0->c, &p1->c, pfs, (double)(radix - 1) / radix);
}

private inline void
bbox_of_points(gs_fixed_rect *r, 
	const gs_fixed_point *p0, const gs_fixed_point *p1,
	const gs_fixed_point *p2, const gs_fixed_point *p3)
{
    r->p.x = r->q.x = p0->x;
    r->p.y = r->q.y = p0->y;

    if (r->p.x > p1->x)
	r->p.x = p1->x;
    if (r->q.x < p1->x)
	r->q.x = p1->x;
    if (r->p.y > p1->y)
	r->p.y = p1->y;
    if (r->q.y < p1->y)
	r->q.y = p1->y;

    if (r->p.x > p2->x)
	r->p.x = p2->x;
    if (r->q.x < p2->x)
	r->q.x = p2->x;
    if (r->p.y > p2->y)
	r->p.y = p2->y;
    if (r->q.y < p2->y)
	r->q.y = p2->y;

    if (p3 == NULL)
	return;

    if (r->p.x > p3->x)
	r->p.x = p3->x;
    if (r->q.x < p3->x)
	r->q.x = p3->x;
    if (r->p.y > p3->y)
	r->p.y = p3->y;
    if (r->q.y < p3->y)
	r->q.y = p3->y;
}

private int 
triangle_by_4(patch_fill_state_t *pfs, 
	const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2, 
	wedge_vertex_list_t *l01, wedge_vertex_list_t *l12, wedge_vertex_list_t *l20,
	double cd, fixed sd)
{
    shading_vertex_t p01, p12, p20;
    wedge_vertex_list_t L01, L12, L20, L[3];
    bool inside_save = pfs->inside;
    gs_fixed_rect r, r1;
    int code;
    
    if (!pfs->inside) {
	bbox_of_points(&r, &p0->p, &p1->p, &p2->p, NULL);
	r1 = r;
	rect_intersect(r, pfs->rect);
	if (r.q.x <= r.p.x || r.q.y <= r.p.y)
	    return 0; /* Outside. */
    }
    code = try_device_linear_color(pfs, false, p0, p1, p2);
    switch(code) {
	case 0: /* The area is filled. */
	    return 0;
	case 2: /* decompose to constant color areas */
	    if (sd < fixed_1 * 4)
		return constant_color_triangle(pfs, p2, p0, p1);
	    if (pfs->Function != NULL) {
		double d01 = color_span(pfs, &p1->c, &p0->c);
		double d12 = color_span(pfs, &p2->c, &p1->c);
		double d20 = color_span(pfs, &p0->c, &p2->c);

		if (d01 <= pfs->smoothness / COLOR_CONTIGUITY && 
		    d12 <= pfs->smoothness / COLOR_CONTIGUITY && 
		    d20 <= pfs->smoothness / COLOR_CONTIGUITY)
		    return constant_color_triangle(pfs, p2, p0, p1);
	    } else if (cd <= pfs->smoothness / COLOR_CONTIGUITY)
		return constant_color_triangle(pfs, p2, p0, p1);
	    break;
	case 1: /* decompose to linear color areas */
	    if (sd < fixed_1)
		return constant_color_triangle(pfs, p2, p0, p1);
	    break;
	default: /* Error. */
	    return code;
    }
    if (!pfs->inside) {
	if (r.p.x == r1.p.x && r.p.y == r1.p.y && 
	    r.q.x == r1.q.x && r.q.y == r1.q.y)
	    pfs->inside = true;
    }
    divide_bar(pfs, p0, p1, 2, &p01);
    divide_bar(pfs, p1, p2, 2, &p12);
    divide_bar(pfs, p2, p0, 2, &p20);
    if (LAZY_WEDGES) {
	init_wedge_vertex_list(L, count_of(L));
	make_wedge_median(pfs, &L01, l01, true,  &p0->p, &p1->p, &p01.p);
	make_wedge_median(pfs, &L12, l12, true,  &p1->p, &p2->p, &p12.p);
	make_wedge_median(pfs, &L20, l20, false, &p2->p, &p0->p, &p20.p);
    } else {
	code = fill_triangle_wedge(pfs, p0, p1, &p01);
	if (code < 0)
	    return code;
	code = fill_triangle_wedge(pfs, p1, p2, &p12);
	if (code < 0)
	    return code;
	code = fill_triangle_wedge(pfs, p2, p0, &p20);
	if (code < 0)
	    return code;
    }
    code = triangle_by_4(pfs, p0, &p01, &p20, &L01, &L[0], &L20, cd / 2, sd / 2);
    if (code < 0)
	return code;
    if (LAZY_WEDGES) {
	move_wedge(&L01, l01, true);
	move_wedge(&L20, l20, false);
    }
    code = triangle_by_4(pfs, p1, &p12, &p01, &L12, &L[1], &L01, cd / 2, sd / 2);
    if (code < 0)
	return code;
    if (LAZY_WEDGES)
	move_wedge(&L12, l12, true);
    code = triangle_by_4(pfs, p2, &p20, &p12, &L20, &L[2], &L12, cd / 2, sd / 2);
    if (code < 0)
	return code;
    L[0].last_side = L[1].last_side = L[2].last_side = true;
    code = triangle_by_4(pfs, &p01, &p12, &p20, &L[1], &L[2], &L[0], cd / 2, sd / 2);
    if (code < 0)
	return code;
    if (LAZY_WEDGES) {
	code = close_wedge_median(pfs, l01, &p0->c, &p1->c);
	if (code < 0)
	    return code;
	code = close_wedge_median(pfs, l12, &p1->c, &p2->c);
	if (code < 0)
	    return code;
	code = close_wedge_median(pfs, l20, &p2->c, &p0->c);
	if (code < 0)
	    return code;
	code = terminate_wedge_vertex_list(pfs, &L[0], &p01.c, &p20.c);
	if (code < 0)
	    return code;
	code = terminate_wedge_vertex_list(pfs, &L[1], &p12.c, &p01.c);
	if (code < 0)
	    return code;
	code = terminate_wedge_vertex_list(pfs, &L[2], &p20.c, &p12.c);
	if (code < 0)
	    return code;
    }
    pfs->inside = inside_save;
    return 0;
}

private inline int 
fill_triangle(patch_fill_state_t *pfs, 
	const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2,
	wedge_vertex_list_t *l01, wedge_vertex_list_t *l12, wedge_vertex_list_t *l20)
{
    fixed sd01 = max(any_abs(p1->p.x - p0->p.x), any_abs(p1->p.y - p0->p.y));
    fixed sd12 = max(any_abs(p2->p.x - p1->p.x), any_abs(p2->p.y - p1->p.y));
    fixed sd20 = max(any_abs(p0->p.x - p2->p.x), any_abs(p0->p.y - p2->p.y));
    fixed sd1 = max(sd01, sd12);
    fixed sd = max(sd1, sd20);
    double cd = 0;

#   if SKIP_TEST
	dbg_triangle_cnt++;
#   endif
    if (pfs->Function == NULL) {
    	double d01 = color_span(pfs, &p1->c, &p0->c);
	double d12 = color_span(pfs, &p2->c, &p1->c);
	double d20 = color_span(pfs, &p0->c, &p2->c);
	double cd1 = max(d01, d12);
	
	cd = max(cd1, d20);
    }
    return triangle_by_4(pfs, p0, p1, p2, l01, l12, l20, cd, sd);
}

private int 
small_mesh_triangle(patch_fill_state_t *pfs, 
	const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2)
{
    int code;
    wedge_vertex_list_t l[3];

    init_wedge_vertex_list(l, count_of(l));
    code = fill_triangle(pfs, p0, p1, p2, &l[0], &l[1], &l[2]);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l[0], &p0->c, &p1->c);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l[1], &p1->c, &p2->c);
    if (code < 0)
	return code;
    return terminate_wedge_vertex_list(pfs, &l[2], &p2->c, &p0->c);
}

private int
mesh_triangle_rec(patch_fill_state_t *pfs, 
	const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2)
{
    pfs->unlinear = !is_linear_color_applicable(pfs);
    if (manhattan_dist(&p0->p, &p1->p) < pfs->max_small_coord &&
	manhattan_dist(&p1->p, &p2->p) < pfs->max_small_coord &&
	manhattan_dist(&p2->p, &p0->p) < pfs->max_small_coord)
	return small_mesh_triangle(pfs, p0, p1, p2);
    else {
	/* Subdivide into 4 triangles with 3 triangle non-lazy wedges.
	   Doing so against the wedge_vertex_list_elem_buffer overflow.
	   We could apply a smarter method, dividing long sides 
	   with no wedges and short sides with lazy wedges.
	   This needs to start wedges dynamically when 
	   a side becomes short. We don't do so because the 
	   number of checks per call significantly increases
	   and the logics is complicated, but the performance 
	   advantage appears small due to big meshes are rare.
	 */
	shading_vertex_t p01, p12, p20;
	int code;

	divide_bar(pfs, p0, p1, 2, &p01);
	divide_bar(pfs, p1, p2, 2, &p12);
	divide_bar(pfs, p2, p0, 2, &p20);
	code = fill_triangle_wedge(pfs, p0, p1, &p01);
	if (code < 0)
	    return code;
	code = fill_triangle_wedge(pfs, p1, p2, &p12);
	if (code < 0)
	    return code;
	code = fill_triangle_wedge(pfs, p2, p0, &p20);
	if (code < 0)
	    return code;
	code = mesh_triangle_rec(pfs, p0, &p01, &p20);
	if (code < 0)
	    return code;
	code = mesh_triangle_rec(pfs, p1, &p12, &p01);
	if (code < 0)
	    return code;
	code = mesh_triangle_rec(pfs, p2, &p20, &p12);
	if (code < 0)
	    return code;
	return mesh_triangle_rec(pfs, &p01, &p12, &p20);
    }
}

int
mesh_triangle(patch_fill_state_t *pfs, 
	const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2)
{
    if ((*dev_proc(pfs->dev, pattern_manage))(pfs->dev, 
	    gs_no_id, NULL, pattern_manage__shading_area) > 0) {
	/* Inform the device with the shading coverage area. 
	   First compute the sign of the area, because
	   all areas to be clipped in same direction. */
	gx_device *pdev = pfs->dev;
	gx_path path;
	int code;
	fixed d01x = p1->p.x - p0->p.x, d01y = p1->p.y - p0->p.y;
	fixed d12x = p2->p.x - p1->p.x, d12y = p2->p.y - p1->p.y;
	int64_t s1 = (int64_t)d01x * d12y - (int64_t)d01y * d12x;

	gx_path_init_local(&path, pdev->memory);
	code = gx_path_add_point(&path, p0->p.x, p0->p.y);
	if (code >= 0 && s1 >= 0)
	    code = gx_path_add_line(&path, p1->p.x, p1->p.y);
	if (code >= 0)
	    code = gx_path_add_line(&path, p2->p.x, p2->p.y);
	if (code >= 0 && s1 < 0)
	    code = gx_path_add_line(&path, p1->p.x, p1->p.y);
	if (code >= 0)
	    code = gx_path_close_subpath(&path);
	if (code >= 0)
	    code = (*dev_proc(pfs->dev, fill_path))(pdev, NULL, &path, NULL, NULL, NULL);
	gx_path_free(&path, "mesh_triangle");
	if (code < 0)
	    return code;
    }
    return mesh_triangle_rec(pfs, p0, p1, p2);
}

private inline int 
triangles4(patch_fill_state_t *pfs, const quadrangle_patch *p, bool dummy_argument)
{
    shading_vertex_t p0001, p1011, q;
    wedge_vertex_list_t l[4];
    int code;

    init_wedge_vertex_list(l, count_of(l));
    divide_bar(pfs, p->p[0][0], p->p[0][1], 2, &p0001);
    divide_bar(pfs, p->p[1][0], p->p[1][1], 2, &p1011);
    divide_bar(pfs, &p0001, &p1011, 2, &q);
    code = fill_triangle(pfs, p->p[0][0], p->p[0][1], &q, p->l0001, &l[0], &l[3]);
    if (code < 0)
	return code;
    l[0].last_side = true;
    l[3].last_side = true;
    code = fill_triangle(pfs, p->p[0][1], p->p[1][1], &q, p->l0111, &l[1], &l[0]);
    if (code < 0)
	return code;
    l[1].last_side = true;
    code = fill_triangle(pfs, p->p[1][1], p->p[1][0], &q, p->l1110, &l[2], &l[1]);
    if (code < 0)
	return code;
    l[2].last_side = true;
    code = fill_triangle(pfs, p->p[1][0], p->p[0][0], &q, p->l1000, &l[3], &l[2]);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l[0], &p->p[0][1]->c, &q.c);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l[1], &p->p[1][1]->c, &q.c);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l[2], &p->p[1][0]->c, &q.c);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l[3], &q.c, &p->p[0][0]->c);
    if (code < 0)
	return code;
    return 0;
}

private inline int 
triangles2(patch_fill_state_t *pfs, const quadrangle_patch *p, bool dummy_argument)
{
    wedge_vertex_list_t l;
    int code;

    init_wedge_vertex_list(&l, 1);
    code = fill_triangle(pfs, p->p[0][0], p->p[0][1], p->p[1][1], p->l0001, p->l0111, &l);
    if (code < 0)
	return code;
    l.last_side = true;
    code = fill_triangle(pfs, p->p[1][1], p->p[1][0], p->p[0][0], p->l1110, p->l1000, &l);
    if (code < 0)
	return code;
    code = terminate_wedge_vertex_list(pfs, &l, &p->p[1][1]->c, &p->p[0][0]->c);
    if (code < 0)
	return code;
    return 0;
}

private inline void 
make_quadrangle(const tensor_patch *p, shading_vertex_t qq[2][2], 
	wedge_vertex_list_t l[4], quadrangle_patch *q)
{
    qq[0][0].p = p->pole[0][0];
    qq[0][1].p = p->pole[0][3];
    qq[1][0].p = p->pole[3][0];
    qq[1][1].p = p->pole[3][3];
    qq[0][0].c = p->c[0][0];
    qq[0][1].c = p->c[0][1];
    qq[1][0].c = p->c[1][0];
    qq[1][1].c = p->c[1][1];
    q->p[0][0] = &qq[0][0];
    q->p[0][1] = &qq[0][1];
    q->p[1][0] = &qq[1][0];
    q->p[1][1] = &qq[1][1];
    q->l0001 = &l[0];
    q->l0111 = &l[1];
    q->l1110 = &l[2];
    q->l1000 = &l[3];
}

private inline int
is_quadrangle_color_linear_by_u(const patch_fill_state_t *pfs, const quadrangle_patch *p)
{   /* returns : 1 = linear, 0 = unlinear, <0 = error. */
    int code;

    code = is_color_linear(pfs, &p->p[0][0]->c, &p->p[0][1]->c);
    if (code <= 0)
	return code;
    return is_color_linear(pfs, &p->p[1][0]->c, &p->p[1][1]->c);
}

private inline int
is_quadrangle_color_linear_by_v(const patch_fill_state_t *pfs, const quadrangle_patch *p)
{   /* returns : 1 = linear, 0 = unlinear, <0 = error. */
    int code;

    code = is_color_linear(pfs, &p->p[0][0]->c, &p->p[1][0]->c);
    if (code <= 0)
	return code;
    return is_color_linear(pfs, &p->p[0][1]->c, &p->p[1][1]->c);
}

private inline int
is_quadrangle_color_linear_by_diagonals(const patch_fill_state_t *pfs, const quadrangle_patch *p)
{   /* returns : 1 = linear, 0 = unlinear, <0 = error. */
    int code;

    code = is_color_linear(pfs, &p->p[0][0]->c, &p->p[1][1]->c);
    if (code <= 0)
	return code;
    return is_color_linear(pfs, &p->p[0][1]->c, &p->p[1][0]->c);
}

typedef enum {
    color_change_small,
    color_change_gradient,
    color_change_linear,
    color_change_bilinear,
    color_change_general
} color_change_type_t;

private inline color_change_type_t
quadrangle_color_change(const patch_fill_state_t *pfs, const quadrangle_patch *p, 
			bool is_big_u, bool is_big_v, bool *divide_u, bool *divide_v)
{
    patch_color_t d0001, d1011, d;
    double D, D0001, D1011, D0010, D0111, D0011, D0110;
    double Du, Dv;

    color_diff(pfs, &p->p[0][0]->c, &p->p[0][1]->c, &d0001);
    color_diff(pfs, &p->p[1][0]->c, &p->p[1][1]->c, &d1011);
    D0001 = color_norm(pfs, &d0001);
    D1011 = color_norm(pfs, &d1011);
    D0010 = color_span(pfs, &p->p[0][0]->c, &p->p[1][0]->c);
    D0111 = color_span(pfs, &p->p[0][1]->c, &p->p[1][1]->c);
    D0011 = color_span(pfs, &p->p[0][0]->c, &p->p[1][1]->c);
    D0110 = color_span(pfs, &p->p[0][1]->c, &p->p[1][0]->c);
    if (pfs->unlinear) {
	if (D0001 <= pfs->smoothness && D1011 <= pfs->smoothness &&
	    D0010 <= pfs->smoothness && D0111 <= pfs->smoothness &&
	    D0011 <= pfs->smoothness && D0110 <= pfs->smoothness)
	    return color_change_small;
	if (D0001 <= pfs->smoothness && D1011 <= pfs->smoothness) {
	    if (!is_big_v) {
		/* The color function looks uncontiguous. */
		return color_change_small;
	    }
	    *divide_v = true;
	    return color_change_gradient;
	}
	if (D0010 <= pfs->smoothness && D0111 <= pfs->smoothness) {
	    if (!is_big_u) {
		/* The color function looks uncontiguous. */
		return color_change_small;
	    }
	    *divide_u = true;
	    return color_change_gradient;
	}
    }
    color_diff(pfs, &d0001, &d1011, &d);
    Du = max(D0001, D1011);
    Dv = max(D0010, D0111);
    if (Du <= pfs->smoothness / 8 && Dv <= pfs->smoothness / 8)
	return color_change_small;
    if (Du <= pfs->smoothness / 8)
	return color_change_linear;
    if (Dv <= pfs->smoothness / 8)
	return color_change_linear;
    D = color_norm(pfs, &d);
    if (D <= pfs->smoothness)
	return color_change_bilinear;
#if 0 /* Disabled due to a 0.5% slowdown with the test file of the Bug 687948. */
    if (Du > Dv && is_big_u)
	*divide_u = true;
    else if (Du < Dv && is_big_v)
	*divide_v = true;
    else if (is_big_u)
	*divide_u = true;
    else if (is_big_v)
	*divide_v = true;
    else {
	/* The color function looks uncontiguous. */
	return color_change_small;
    }
#else
    if (Du > Dv)
	*divide_u = true;
    else
	*divide_v = true;
#endif
    return color_change_general;
}

private int 
fill_quadrangle(patch_fill_state_t *pfs, const quadrangle_patch *p, bool big, int level)
{
    /* The quadrangle is flattened enough by V and U, so ignore inner poles. */
    /* Assuming the XY span is restricted with curve_samples. 
       It is important for intersection_of_small_bars to compute faster. */
    quadrangle_patch s0, s1;
    wedge_vertex_list_t l0, l1, l2;
    int code;
    bool divide_u = false, divide_v = false, big1 = big;
    shading_vertex_t q[2];
    bool monotonic_color_save = pfs->monotonic_color;
    bool linear_color_save = pfs->linear_color;
    bool inside_save = pfs->inside;
    gs_fixed_rect r, r1;
    /* Warning : pfs->monotonic_color is not restored on error. */

    if (level > 100)
	return_error(gs_error_unregistered); /* Safety. */
    if (!pfs->inside) {
	bbox_of_points(&r, &p->p[0][0]->p, &p->p[0][1]->p, &p->p[1][0]->p, &p->p[1][1]->p);
	r1 = r;
	rect_intersect(r, pfs->rect);
	if (r.q.x <= r.p.x || r.q.y <= r.p.y)
	    return 0; /* Outside. */
    }
    if (big) {
	/* Likely 'big' is an unuseful rudiment due to curve_samples
	   restricts lengthes. We keep it for a while because its implementation 
	   isn't obvious and its time consumption is invisibly small.
	 */
	fixed size_u = max(max(any_abs(p->p[0][0]->p.x - p->p[0][1]->p.x), 
			       any_abs(p->p[1][0]->p.x - p->p[1][1]->p.x)),
			   max(any_abs(p->p[0][0]->p.y - p->p[0][1]->p.y),
			       any_abs(p->p[1][0]->p.y - p->p[1][1]->p.y)));
	fixed size_v = max(max(any_abs(p->p[0][0]->p.x - p->p[1][0]->p.x),
			       any_abs(p->p[0][1]->p.x - p->p[1][1]->p.x)),
			   max(any_abs(p->p[0][0]->p.y - p->p[1][0]->p.y),
			       any_abs(p->p[0][1]->p.y - p->p[1][1]->p.y)));

	if (QUADRANGLES && pfs->maybe_self_intersecting) {
	    if (size_v > pfs->max_small_coord) {
		/* constant_color_quadrangle can't handle big self-intersecting areas
		   because we don't want int64_t in it. */
		divide_v = true;
	    } else if (size_u > pfs->max_small_coord) {
		/* constant_color_quadrangle can't handle big self-intersecting areas, 
		   because we don't want int64_t in it. */
		divide_u = true;
	    } else
		big1 = false;
	} else
	    big1 = false;
    }
    if (!big1) {
	bool is_big_u = false, is_big_v = false;
	double d0001x = any_abs(p->p[0][0]->p.x - p->p[0][1]->p.x);
	double d1011x = any_abs(p->p[1][0]->p.x - p->p[1][1]->p.x);
	double d0001y = any_abs(p->p[0][0]->p.y - p->p[0][1]->p.y);
	double d1011y = any_abs(p->p[1][0]->p.y - p->p[1][1]->p.y);
	double d0010x = any_abs(p->p[0][0]->p.x - p->p[1][0]->p.x);
	double d0111x = any_abs(p->p[0][1]->p.x - p->p[1][1]->p.x);
	double d0010y = any_abs(p->p[0][0]->p.y - p->p[1][0]->p.y);
	double d0111y = any_abs(p->p[0][1]->p.y - p->p[1][1]->p.y);

	if (d0001x > fixed_1 || d1011x > fixed_1 || d0001y > fixed_1 || d1011y > fixed_1)
	    is_big_u = true;
	if (d0010x > fixed_1 || d0111x > fixed_1 || d0010y > fixed_1 || d0111y > fixed_1)
	    is_big_v = true;
	else if (!is_big_u)
	    return (QUADRANGLES || !pfs->maybe_self_intersecting ? 
			constant_color_quadrangle : triangles4)(pfs, p, 
			    pfs->maybe_self_intersecting);
	if (!pfs->monotonic_color) {
	    bool not_monotonic_by_u = false, not_monotonic_by_v = false;

	    code = is_quadrangle_color_monotonic(pfs, p, &not_monotonic_by_u, &not_monotonic_by_v);
	    if (code < 0)
		return code;
	    if (is_big_u)
		divide_u = not_monotonic_by_u;
	    if (is_big_v)
		divide_v = not_monotonic_by_v;
	    if (!divide_u && !divide_v)
		pfs->monotonic_color = true;
	}
	if (pfs->monotonic_color && !pfs->linear_color) {
	    if (divide_v && divide_u) {
		if (d0001x + d1011x + d0001y + d1011y > d0010x + d0111x + d0010y + d0111y)
		    divide_v = false;
		else
		    divide_u = false;
	    } else if (!divide_u && !divide_v && !pfs->unlinear) {
		if (is_big_u) {
		    code = is_quadrangle_color_linear_by_u(pfs, p);
		    if (code < 0)
			return code;
		    divide_u = !code;
		}
		if (is_big_v) {
		    code = is_quadrangle_color_linear_by_v(pfs, p);
		    if (code < 0)
			return code;
		    divide_v = !code;
		}
		if (is_big_u && is_big_v) {
		    code = is_quadrangle_color_linear_by_diagonals(pfs, p);
		    if (code < 0)
			return code;
		    if (!code) {
			if (d0001x + d1011x + d0001y + d1011y > d0010x + d0111x + d0010y + d0111y) {
			    divide_u = true;
			    divide_v = false;
			} else {
			    divide_v = true;
			    divide_u = false;
			}
		    }
		}
	    }
	    if (!divide_u && !divide_v)
		pfs->linear_color = true;
	}
	if (!pfs->linear_color) {
	    /* go to divide. */
	} else switch(quadrangle_color_change(pfs, p, is_big_u, is_big_v, &divide_u, &divide_v)) {
	    case color_change_small: 
		code = (QUADRANGLES || !pfs->maybe_self_intersecting ? 
			    constant_color_quadrangle : triangles4)(pfs, p, 
				pfs->maybe_self_intersecting);
		pfs->monotonic_color = monotonic_color_save;
		pfs->linear_color = linear_color_save;
		return code;
	    case color_change_bilinear:
		if (!QUADRANGLES) {
		    code = triangles4(pfs, p, true);
		    pfs->monotonic_color = monotonic_color_save;
		    pfs->linear_color = linear_color_save;
		    return code;
		}
	    case color_change_linear:
		if (!QUADRANGLES) {
		    code = triangles2(pfs, p, true);
		    pfs->monotonic_color = monotonic_color_save;
		    pfs->linear_color = linear_color_save;
		    return code;
		}
	    case color_change_gradient:
	    case color_change_general:
		; /* goto divide. */
	}
    }
    if (!pfs->inside) {
	if (r.p.x == r1.p.x && r.p.y == r1.p.y && 
	    r.q.x == r1.q.x && r.q.y == r1.q.y)
	    pfs->inside = true;
    }
    if (LAZY_WEDGES)
	init_wedge_vertex_list(&l0, 1);
    if (divide_v) {
	divide_quadrangle_by_v(pfs, &s0, &s1, q, p);
	if (LAZY_WEDGES) {
	    make_wedge_median(pfs, &l1, p->l0111, true,  &p->p[0][1]->p, &p->p[1][1]->p, &s0.p[1][1]->p);
	    make_wedge_median(pfs, &l2, p->l1000, false, &p->p[1][0]->p, &p->p[0][0]->p, &s0.p[1][0]->p);
	    s0.l1110 = s1.l0001 = &l0;
	    s0.l0111 = s1.l0111 = &l1;
	    s0.l1000 = s1.l1000 = &l2;
	    s0.l0001 = p->l0001;
	    s1.l1110 = p->l1110;
	} else {
	    code = fill_triangle_wedge(pfs, s0.p[0][0], s1.p[1][0], s0.p[1][0]);
	    if (code < 0)
		return code;
	    code = fill_triangle_wedge(pfs, s0.p[0][1], s1.p[1][1], s0.p[1][1]);
	    if (code < 0)
		return code;
	}
	code = fill_quadrangle(pfs, &s0, big, level + 1);
	if (code < 0)
	    return code;
	if (LAZY_WEDGES) {
	    l0.last_side = true;
	    move_wedge(&l1, p->l0111, true);
	    move_wedge(&l2, p->l1000, false);
	}
	code = fill_quadrangle(pfs, &s1, big1, level + 1);
	if (LAZY_WEDGES) {
	    if (code < 0)
		return code;
	    code = close_wedge_median(pfs, p->l0111, &p->p[0][1]->c, &p->p[1][1]->c);
	    if (code < 0)
		return code;
	    code = close_wedge_median(pfs, p->l1000, &p->p[1][0]->c, &p->p[0][0]->c);
	    if (code < 0)
		return code;
	    code = terminate_wedge_vertex_list(pfs, &l0, &s0.p[1][0]->c, &s0.p[1][1]->c);
	}
    } else if (divide_u) {
	divide_quadrangle_by_u(pfs, &s0, &s1, q, p);
	if (LAZY_WEDGES) {
	    make_wedge_median(pfs, &l1, p->l0001, true,  &p->p[0][0]->p, &p->p[0][1]->p, &s0.p[0][1]->p);
	    make_wedge_median(pfs, &l2, p->l1110, false, &p->p[1][1]->p, &p->p[1][0]->p, &s0.p[1][1]->p);
	    s0.l0111 = s1.l1000 = &l0;
	    s0.l0001 = s1.l0001 = &l1;
	    s0.l1110 = s1.l1110 = &l2;
	    s0.l1000 = p->l1000;
	    s1.l0111 = p->l0111;
	} else {
	    code = fill_triangle_wedge(pfs, s0.p[0][0], s1.p[0][1], s0.p[0][1]);
	    if (code < 0)
		return code;
	    code = fill_triangle_wedge(pfs, s0.p[1][0], s1.p[1][1], s0.p[1][1]);
	    if (code < 0)
		return code;
	}
	code = fill_quadrangle(pfs, &s0, big1, level + 1);
	if (code < 0)
	    return code;
	if (LAZY_WEDGES) {
	    l0.last_side = true;
	    move_wedge(&l1, p->l0001, true);
	    move_wedge(&l2, p->l1110, false);
	}
	code = fill_quadrangle(pfs, &s1, big1, level + 1);
	if (LAZY_WEDGES) {
	    if (code < 0)
		return code;
	    code = close_wedge_median(pfs, p->l0001, &p->p[0][0]->c, &p->p[0][1]->c);
	    if (code < 0)
		return code;
	    code = close_wedge_median(pfs, p->l1110, &p->p[1][1]->c, &p->p[1][0]->c);
	    if (code < 0)
		return code;
	    code = terminate_wedge_vertex_list(pfs, &l0, &s0.p[0][1]->c, &s0.p[1][1]->c);
	}
    } else 
	code = (QUADRANGLES || !pfs->maybe_self_intersecting ? 
		    constant_color_quadrangle : triangles4)(pfs, p, 
			pfs->maybe_self_intersecting);
    pfs->monotonic_color = monotonic_color_save;
    pfs->linear_color = linear_color_save;
    pfs->inside = inside_save;
    return code;
}



private inline void
split_stripe(patch_fill_state_t *pfs, tensor_patch *s0, tensor_patch *s1, const tensor_patch *p)
{
    split_curve_s(p->pole[0], s0->pole[0], s1->pole[0], 1);
    split_curve_s(p->pole[1], s0->pole[1], s1->pole[1], 1);
    split_curve_s(p->pole[2], s0->pole[2], s1->pole[2], 1);
    split_curve_s(p->pole[3], s0->pole[3], s1->pole[3], 1);
    s0->c[0][0] = p->c[0][0];
    s0->c[1][0] = p->c[1][0];
    patch_interpolate_color(&s0->c[0][1], &p->c[0][0], &p->c[0][1], pfs, 0.5);
    patch_interpolate_color(&s0->c[1][1], &p->c[1][0], &p->c[1][1], pfs, 0.5);
    s1->c[0][0] = s0->c[0][1];
    s1->c[1][0] = s0->c[1][1];
    s1->c[0][1] = p->c[0][1];
    s1->c[1][1] = p->c[1][1];
}

private inline void
split_patch(patch_fill_state_t *pfs, tensor_patch *s0, tensor_patch *s1, const tensor_patch *p)
{
    split_curve_s(&p->pole[0][0], &s0->pole[0][0], &s1->pole[0][0], 4);
    split_curve_s(&p->pole[0][1], &s0->pole[0][1], &s1->pole[0][1], 4);
    split_curve_s(&p->pole[0][2], &s0->pole[0][2], &s1->pole[0][2], 4);
    split_curve_s(&p->pole[0][3], &s0->pole[0][3], &s1->pole[0][3], 4);
    s0->c[0][0] = p->c[0][0];
    s0->c[0][1] = p->c[0][1];
    patch_interpolate_color(&s0->c[1][0], &p->c[0][0], &p->c[1][0], pfs, 0.5);
    patch_interpolate_color(&s0->c[1][1], &p->c[0][1], &p->c[1][1], pfs, 0.5);
    s1->c[0][0] = s0->c[1][0];
    s1->c[0][1] = s0->c[1][1];
    s1->c[1][0] = p->c[1][0];
    s1->c[1][1] = p->c[1][1];
}

private int 
decompose_stripe(patch_fill_state_t *pfs, const tensor_patch *p, int ku)
{
    if (ku > 1) {
	tensor_patch s0, s1;
	int code;

	split_stripe(pfs, &s0, &s1, p);
	if (0) { /* Debug purpose only. */
	    draw_patch(&s0, true, RGB(0, 128, 128));
	    draw_patch(&s1, true, RGB(0, 128, 128));
	}
	code = decompose_stripe(pfs, &s0, ku / 2);
	if (code < 0)
	    return code;
	return decompose_stripe(pfs, &s1, ku / 2);
    } else {
	quadrangle_patch q;
	shading_vertex_t qq[2][2];
	wedge_vertex_list_t l[4];
	int code;

	init_wedge_vertex_list(l, count_of(l));
	make_quadrangle(p, qq, l, &q);
#	if SKIP_TEST
	    dbg_quad_cnt++;
#	endif
	code = fill_quadrangle(pfs, &q, true, 0);
	if (LAZY_WEDGES) {
	    code = terminate_wedge_vertex_list(pfs, &l[0], &q.p[0][0]->c, &q.p[0][1]->c);
	    if (code < 0)
		return code;
	    code = terminate_wedge_vertex_list(pfs, &l[1], &q.p[0][1]->c, &q.p[1][1]->c);
	    if (code < 0)
		return code;
	    code = terminate_wedge_vertex_list(pfs, &l[2], &q.p[1][1]->c, &q.p[1][0]->c);
	    if (code < 0)
		return code;
	    code = terminate_wedge_vertex_list(pfs, &l[3], &q.p[1][0]->c, &q.p[0][1]->c);
	    if (code < 0)
		return code;
	}
	return code;
    }
}

private int 
fill_stripe(patch_fill_state_t *pfs, const tensor_patch *p)
{
    /* The stripe is flattened enough by V, so ignore inner poles. */
    int ku[4], kum, code;

    /* We would like to apply iterations for enumerating the kum curve parts,
       but the roundinmg errors would be too complicated due to
       the dependence on the direction. Note that neigbour
       patches may use the opposite direction for same bounding curve.
       We apply the recursive dichotomy, in which 
       the rounding errors do not depend on the direction. */
    ku[0] = curve_samples(pfs, p->pole[0], 1, pfs->fixed_flat);
    ku[3] = curve_samples(pfs, p->pole[3], 1, pfs->fixed_flat);
    kum = max(ku[0], ku[3]);
    code = fill_wedges(pfs, ku[0], kum, p->pole[0], 1, &p->c[0][0], &p->c[0][1], inpatch_wedge);
    if (code < 0)
	return code;
    if (INTERPATCH_PADDING) {
	vd_bar(p->pole[0][0].x, p->pole[0][0].y, p->pole[3][0].x, p->pole[3][0].y, 0, RGB(255, 0, 0));
	code = mesh_padding(pfs, &p->pole[0][0], &p->pole[3][0], &p->c[0][0], &p->c[1][0]);
	if (code < 0)
	    return code;
	vd_bar(p->pole[0][3].x, p->pole[0][3].y, p->pole[3][3].x, p->pole[3][3].y, 0, RGB(255, 0, 0));
	code = mesh_padding(pfs, &p->pole[0][3], &p->pole[3][3], &p->c[0][1], &p->c[1][1]);
	if (code < 0)
	    return code;
    }
    code = decompose_stripe(pfs, p, kum);
    if (code < 0)
	return code;
    return fill_wedges(pfs, ku[3], kum, p->pole[3], 1, &p->c[1][0], &p->c[1][1], inpatch_wedge);
}

private inline bool
is_curve_x_monotonic(const gs_fixed_point *pole, int pole_step)
{   /* true = monotonic, false = don't know. */
    return (pole[0 * pole_step].x <= pole[1 * pole_step].x && 
	    pole[1 * pole_step].x <= pole[2 * pole_step].x &&
	    pole[2 * pole_step].x <= pole[3 * pole_step].x) ||
	   (pole[0 * pole_step].x >= pole[1 * pole_step].x && 
	    pole[1 * pole_step].x >= pole[2 * pole_step].x &&
	    pole[2 * pole_step].x >= pole[3 * pole_step].x);
}

private inline bool
is_curve_y_monotonic(const gs_fixed_point *pole, int pole_step)
{   /* true = monotonic, false = don't know. */
    return (pole[0 * pole_step].y <= pole[1 * pole_step].y && 
	    pole[1 * pole_step].y <= pole[2 * pole_step].y &&
	    pole[2 * pole_step].y <= pole[3 * pole_step].y) ||
	   (pole[0 * pole_step].y >= pole[1 * pole_step].y && 
	    pole[1 * pole_step].y >= pole[2 * pole_step].y &&
	    pole[2 * pole_step].y >= pole[3 * pole_step].y);
}

private inline bool neqs(int *a, int b)
{   /* Unequal signs. Assuming -1, 0, 1 only. */
    if (*a * b < 0)
	return true;
    if (!*a)
	*a = b;
    return false;
}

private inline int
vector_pair_orientation(const gs_fixed_point *p0, const gs_fixed_point *p1, const gs_fixed_point *p2)
{   fixed dx1 = p1->x - p0->x, dy1 = p1->y - p0->y;
    fixed dx2 = p2->x - p0->x, dy2 = p2->y - p0->y;
    int64_t vp = (int64_t)dx1 * dy2 - (int64_t)dy1 * dx2;

    return (vp > 0 ? 1 : vp < 0 ? -1 : 0);
}

private inline bool
is_x_bended(const tensor_patch *p)
{   
    int sign = vector_pair_orientation(&p->pole[0][0], &p->pole[0][1], &p->pole[1][0]);

    if (neqs(&sign, vector_pair_orientation(&p->pole[0][1], &p->pole[0][2], &p->pole[1][1])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[0][2], &p->pole[0][3], &p->pole[1][2])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[0][3], &p->pole[0][2], &p->pole[1][3])))
	return true;

    if (neqs(&sign, vector_pair_orientation(&p->pole[1][1], &p->pole[1][2], &p->pole[2][1])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[1][1], &p->pole[1][2], &p->pole[2][1])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[1][2], &p->pole[1][3], &p->pole[2][2])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[1][3], &p->pole[1][2], &p->pole[2][3])))
	return true;

    if (neqs(&sign, vector_pair_orientation(&p->pole[2][1], &p->pole[2][2], &p->pole[3][1])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[2][1], &p->pole[2][2], &p->pole[3][1])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[2][2], &p->pole[2][3], &p->pole[3][2])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[2][3], &p->pole[2][2], &p->pole[3][3])))
	return true;

    if (neqs(&sign, -vector_pair_orientation(&p->pole[3][1], &p->pole[3][2], &p->pole[2][1])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[3][1], &p->pole[3][2], &p->pole[2][1])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[3][2], &p->pole[3][3], &p->pole[2][2])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[3][3], &p->pole[3][2], &p->pole[2][3])))
	return true;
    return false;
}

private inline bool
is_y_bended(const tensor_patch *p)
{   
    int sign = vector_pair_orientation(&p->pole[0][0], &p->pole[1][0], &p->pole[0][1]);

    if (neqs(&sign, vector_pair_orientation(&p->pole[1][0], &p->pole[2][0], &p->pole[1][1])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[2][0], &p->pole[3][0], &p->pole[2][1])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[3][0], &p->pole[2][0], &p->pole[3][1])))
	return true;

    if (neqs(&sign, vector_pair_orientation(&p->pole[1][1], &p->pole[2][1], &p->pole[1][2])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[1][1], &p->pole[2][1], &p->pole[1][2])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[2][1], &p->pole[3][1], &p->pole[2][2])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[3][1], &p->pole[2][1], &p->pole[3][2])))
	return true;

    if (neqs(&sign, vector_pair_orientation(&p->pole[1][2], &p->pole[2][2], &p->pole[1][3])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[1][2], &p->pole[2][2], &p->pole[1][3])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[2][2], &p->pole[3][2], &p->pole[2][3])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[3][2], &p->pole[2][2], &p->pole[3][3])))
	return true;

    if (neqs(&sign, -vector_pair_orientation(&p->pole[1][3], &p->pole[2][3], &p->pole[1][2])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[1][3], &p->pole[2][3], &p->pole[1][2])))
	return true;
    if (neqs(&sign, -vector_pair_orientation(&p->pole[2][3], &p->pole[3][3], &p->pole[2][2])))
	return true;
    if (neqs(&sign, vector_pair_orientation(&p->pole[3][3], &p->pole[2][3], &p->pole[3][2])))
	return true;
    return false;
}

private inline bool
is_curve_x_small(const gs_fixed_point *pole, int pole_step, fixed fixed_flat)
{   /* Is curve within a single pixel, or smaller than half pixel ? */
    fixed xmin0 = min(pole[0 * pole_step].x, pole[1 * pole_step].x);
    fixed xmin1 = min(pole[2 * pole_step].x, pole[3 * pole_step].x);
    fixed xmin =  min(xmin0, xmin1);
    fixed xmax0 = max(pole[0 * pole_step].x, pole[1 * pole_step].x);
    fixed xmax1 = max(pole[2 * pole_step].x, pole[3 * pole_step].x);
    fixed xmax =  max(xmax0, xmax1);

    if(xmax - xmin <= fixed_1)
	return true;
    return false;	
}

private inline bool
is_curve_y_small(const gs_fixed_point *pole, int pole_step, fixed fixed_flat)
{   /* Is curve within a single pixel, or smaller than half pixel ? */
    fixed ymin0 = min(pole[0 * pole_step].y, pole[1 * pole_step].y);
    fixed ymin1 = min(pole[2 * pole_step].y, pole[3 * pole_step].y);
    fixed ymin =  min(ymin0, ymin1);
    fixed ymax0 = max(pole[0 * pole_step].y, pole[1 * pole_step].y);
    fixed ymax1 = max(pole[2 * pole_step].y, pole[3 * pole_step].y);
    fixed ymax =  max(ymax0, ymax1);

    if (ymax - ymin <= fixed_1)
	return true;
    return false;	
}

private inline bool
is_patch_narrow(const patch_fill_state_t *pfs, const tensor_patch *p)
{
    if (!is_curve_x_small(&p->pole[0][0], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_x_small(&p->pole[0][1], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_x_small(&p->pole[0][2], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_x_small(&p->pole[0][3], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_y_small(&p->pole[0][0], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_y_small(&p->pole[0][1], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_y_small(&p->pole[0][2], 4, pfs->fixed_flat))
	return false;
    if (!is_curve_y_small(&p->pole[0][3], 4, pfs->fixed_flat))
	return false;
    return true;
}

private int 
fill_patch(patch_fill_state_t *pfs, const tensor_patch *p, int kv, int kv0, int kv1)
{
    if (kv <= 1) {
	if (is_patch_narrow(pfs, p))
	    return fill_stripe(pfs, p);
	if (!is_x_bended(p))
	    return fill_stripe(pfs, p);
    }
    {	tensor_patch s0, s1;
        shading_vertex_t q0, q1, q2;
	int code;

	split_patch(pfs, &s0, &s1, p);
	if (kv0 <= 1) {
	    q0.p = s0.pole[0][0];
	    q0.c = s0.c[0][0];
	    q1.p = s1.pole[3][0];
	    q1.c = s1.c[1][0];
	    q2.p = s0.pole[3][0];
	    q2.c = s0.c[1][0];
	    code = fill_triangle_wedge(pfs, &q0, &q1, &q2);
	    if (code < 0)
		return code;
	}
	if (kv1 <= 1) {
	    q0.p = s0.pole[0][3];
	    q0.c = s0.c[0][1];
	    q1.p = s1.pole[3][3];
	    q1.c = s1.c[1][1];
	    q2.p = s0.pole[3][3];
	    q2.c = s0.c[1][1];
	    code = fill_triangle_wedge(pfs, &q0, &q1, &q2);
	    if (code < 0)
		return code;
	}
	code = fill_patch(pfs, &s0, kv / 2, kv0 / 2, kv1 / 2);
	if (code < 0)
	    return code;
	return fill_patch(pfs, &s1, kv / 2, kv0 / 2, kv1 / 2);
	/* fixme : To privide the precise filling order, we must
	   decompose left and right wedges into pieces by intersections
	   with stripes, and fill each piece with its stripe.
	   A lazy wedge list would be fine for storing 
	   the necessary information.

	   If the patch is created from a radial shading,
	   the wedge color appears a constant, so the filling order
	   isn't important. The order is important for other 
	   self-overlapping patches, but the visible effect is 
	   just a slight norrowing the patch (as its lower layer appears
	   visible through the upper layer near the side). 
	   This kind of dropout isn't harmful, because
	   contacring self-overlapping patches are painted 
	   one after one by definition, so that a side coverage break
	   appears unavoidable by definition.

	   Delaying this improvement because it is low important.
	 */
    }
}

private inline fixed
lcp1(fixed p0, fixed p3)
{   /* Computing the 1st pole of a 3d order besier, which applears a line. */
    return (p0 + p0 + p3) / 3;
}
private inline fixed
lcp2(fixed p0, fixed p3)
{   /* Computing the 2nd pole of a 3d order besier, which applears a line. */
    return (p0 + p3 + p3) / 3;
}

private void
patch_set_color(const patch_fill_state_t *pfs, patch_color_t *c, const float *cc)
{
    if (pfs->Function) {
	c->t[0] = cc[0];
	c->t[1] = cc[1];
    } else 
	memcpy(c->cc.paint.values, cc, sizeof(c->cc.paint.values[0]) * pfs->num_components);
}

private void
make_tensor_patch(const patch_fill_state_t *pfs, tensor_patch *p, const patch_curve_t curve[4],
	   const gs_fixed_point interior[4])
{
    const gs_color_space *pcs = pfs->direct_space;

    p->pole[0][0] = curve[0].vertex.p;
    p->pole[1][0] = curve[0].control[0];
    p->pole[2][0] = curve[0].control[1];
    p->pole[3][0] = curve[1].vertex.p;
    p->pole[3][1] = curve[1].control[0];
    p->pole[3][2] = curve[1].control[1];
    p->pole[3][3] = curve[2].vertex.p;
    p->pole[2][3] = curve[2].control[0];
    p->pole[1][3] = curve[2].control[1];
    p->pole[0][3] = curve[3].vertex.p;
    p->pole[0][2] = curve[3].control[0];
    p->pole[0][1] = curve[3].control[1];
    if (interior != NULL) {
	p->pole[1][1] = interior[0];
	p->pole[1][2] = interior[1];
	p->pole[2][2] = interior[2];
	p->pole[2][1] = interior[3];
    } else {
	p->pole[1][1].x = lcp1(p->pole[0][1].x, p->pole[3][1].x) +
			  lcp1(p->pole[1][0].x, p->pole[1][3].x) -
			  lcp1(lcp1(p->pole[0][0].x, p->pole[0][3].x),
			       lcp1(p->pole[3][0].x, p->pole[3][3].x));
	p->pole[1][2].x = lcp1(p->pole[0][2].x, p->pole[3][2].x) +
			  lcp2(p->pole[1][0].x, p->pole[1][3].x) -
			  lcp1(lcp2(p->pole[0][0].x, p->pole[0][3].x),
			       lcp2(p->pole[3][0].x, p->pole[3][3].x));
	p->pole[2][1].x = lcp2(p->pole[0][1].x, p->pole[3][1].x) +
			  lcp1(p->pole[2][0].x, p->pole[2][3].x) -
			  lcp2(lcp1(p->pole[0][0].x, p->pole[0][3].x),
			       lcp1(p->pole[3][0].x, p->pole[3][3].x));
	p->pole[2][2].x = lcp2(p->pole[0][2].x, p->pole[3][2].x) +
			  lcp2(p->pole[2][0].x, p->pole[2][3].x) -
			  lcp2(lcp2(p->pole[0][0].x, p->pole[0][3].x),
			       lcp2(p->pole[3][0].x, p->pole[3][3].x));

	p->pole[1][1].y = lcp1(p->pole[0][1].y, p->pole[3][1].y) +
			  lcp1(p->pole[1][0].y, p->pole[1][3].y) -
			  lcp1(lcp1(p->pole[0][0].y, p->pole[0][3].y),
			       lcp1(p->pole[3][0].y, p->pole[3][3].y));
	p->pole[1][2].y = lcp1(p->pole[0][2].y, p->pole[3][2].y) +
			  lcp2(p->pole[1][0].y, p->pole[1][3].y) -
			  lcp1(lcp2(p->pole[0][0].y, p->pole[0][3].y),
			       lcp2(p->pole[3][0].y, p->pole[3][3].y));
	p->pole[2][1].y = lcp2(p->pole[0][1].y, p->pole[3][1].y) +
			  lcp1(p->pole[2][0].y, p->pole[2][3].y) -
			  lcp2(lcp1(p->pole[0][0].y, p->pole[0][3].y),
			       lcp1(p->pole[3][0].y, p->pole[3][3].y));
	p->pole[2][2].y = lcp2(p->pole[0][2].y, p->pole[3][2].y) +
			  lcp2(p->pole[2][0].y, p->pole[2][3].y) -
			  lcp2(lcp2(p->pole[0][0].y, p->pole[0][3].y),
			       lcp2(p->pole[3][0].y, p->pole[3][3].y));
    }
    patch_set_color(pfs, &p->c[0][0], curve[0].vertex.cc);
    patch_set_color(pfs, &p->c[1][0], curve[1].vertex.cc);
    patch_set_color(pfs, &p->c[1][1], curve[2].vertex.cc);
    patch_set_color(pfs, &p->c[0][1], curve[3].vertex.cc);
    patch_resolve_color_inline(&p->c[0][0], pfs);
    patch_resolve_color_inline(&p->c[0][1], pfs);
    patch_resolve_color_inline(&p->c[1][0], pfs);
    patch_resolve_color_inline(&p->c[1][1], pfs);
    if (!pfs->Function) {
	pcs->type->restrict_color(&p->c[0][0].cc, pcs);
	pcs->type->restrict_color(&p->c[0][1].cc, pcs);
	pcs->type->restrict_color(&p->c[1][0].cc, pcs);
	pcs->type->restrict_color(&p->c[1][1].cc, pcs);
    }
}

int 
gx_shade_background(gx_device *pdev, const gs_fixed_rect *rect, 
	const gx_device_color *pdevc, gs_logical_operation_t log_op)
{
    gs_fixed_edge le, re;

    le.start.x = rect->p.x - INTERPATCH_PADDING;
    le.start.y = rect->p.y - INTERPATCH_PADDING;
    le.end.x = rect->p.x - INTERPATCH_PADDING;
    le.end.y = rect->q.y + INTERPATCH_PADDING;
    re.start.x = rect->q.x + INTERPATCH_PADDING;
    re.start.y = rect->p.y - INTERPATCH_PADDING;
    re.end.x = rect->q.x + INTERPATCH_PADDING;
    re.end.y = rect->q.y + INTERPATCH_PADDING;
    return dev_proc(pdev, fill_trapezoid)(pdev,
	    &le, &re, le.start.y, le.end.y, false, pdevc, log_op);
}


int
patch_fill(patch_fill_state_t *pfs, const patch_curve_t curve[4],
	   const gs_fixed_point interior[4],
	   void (*transform) (gs_fixed_point *, const patch_curve_t[4],
			      const gs_fixed_point[4], floatp, floatp))
{
    tensor_patch p;
    int kv[4], kvm, ku[4], kum, km;
    int code = 0;

#if SKIP_TEST
    dbg_patch_cnt++;
    /*if (dbg_patch_cnt != 67 && dbg_patch_cnt != 78)
	return 0;*/
#endif
    /* We decompose the patch into tiny quadrangles,
       possibly inserting wedges between them against a dropout. */
    make_tensor_patch(pfs, &p, curve, interior);
    pfs->unlinear = !is_linear_color_applicable(pfs);
    pfs->linear_color = false;
    if ((*dev_proc(pfs->dev, pattern_manage))(pfs->dev, 
	    gs_no_id, NULL, pattern_manage__shading_area) > 0) {
	/* Inform the device with the shading coverage area. 
	   First compute the sign of the area, because
	   all areas to be clipped in same direction. */
	gx_device *pdev = pfs->dev;
	gx_path path;
	fixed d01x = (curve[1].vertex.p.x - curve[0].vertex.p.x) >> 1;
	fixed d01y = (curve[1].vertex.p.y - curve[0].vertex.p.y) >> 1;
	fixed d12x = (curve[2].vertex.p.x - curve[1].vertex.p.x) >> 1;
	fixed d12y = (curve[2].vertex.p.y - curve[1].vertex.p.y) >> 1;
	fixed d23x = (curve[3].vertex.p.x - curve[2].vertex.p.x) >> 1;
	fixed d23y = (curve[3].vertex.p.y - curve[2].vertex.p.y) >> 1;
	fixed d30x = (curve[0].vertex.p.x - curve[3].vertex.p.x) >> 1;
	fixed d30y = (curve[0].vertex.p.y - curve[3].vertex.p.y) >> 1;
	int64_t s1 = (int64_t)d01x * d12y - (int64_t)d01y * d12x;
	int64_t s2 = (int64_t)d23x * d30y - (int64_t)d23y * d30x;
	int s = (s1 + s2 > 0 ? 1 : 3), i, j, k, jj, l = (s == 1 ? 0 : 1);

	gx_path_init_local(&path, pdev->memory);
	if (is_x_bended(&p) || is_y_bended(&p)) {
	    /* The patch possibly is self-overlapping,
	       so the patch coverage may fall outside the patch outline.
	       In this case we pass an empty path,
	       and the device must use a bitmap mask instead clipping. */
	} else {
    	    code = gx_path_add_point(&path, curve[0].vertex.p.x, curve[0].vertex.p.y);
	    for (i = k = 0; k < 4 && code >= 0; i = j, k++) {
		j = (i + s) % 4, jj = (s == 1 ? i : j);
		if (curve[jj].straight)
		    code = gx_path_add_line(&path, curve[j].vertex.p.x, 
						curve[j].vertex.p.y);
		else
		    code = gx_path_add_curve(&path, curve[jj].control[l].x, curve[jj].control[l].y, 
						    curve[jj].control[(l + 1) & 1].x, curve[jj].control[(l + 1) & 1].y,
						    curve[j].vertex.p.x, 
						    curve[j].vertex.p.y);
	    }
	    if (code >= 0)
		code = gx_path_close_subpath(&path);
	}
	if (code >= 0)
	    code = (*dev_proc(pfs->dev, fill_path))(pdev, NULL, &path, NULL, NULL, NULL);
	gx_path_free(&path, "patch_fill");
	if (code < 0)
	    return code;
    }
    /* draw_patch(&p, true, RGB(0, 0, 0)); */
    kv[0] = curve_samples(pfs, &p.pole[0][0], 4, pfs->fixed_flat);
    kv[1] = curve_samples(pfs, &p.pole[0][1], 4, pfs->fixed_flat);
    kv[2] = curve_samples(pfs, &p.pole[0][2], 4, pfs->fixed_flat);
    kv[3] = curve_samples(pfs, &p.pole[0][3], 4, pfs->fixed_flat);
    kvm = max(max(kv[0], kv[1]), max(kv[2], kv[3]));
    ku[0] = curve_samples(pfs, p.pole[0], 1, pfs->fixed_flat);
    ku[3] = curve_samples(pfs, p.pole[3], 1, pfs->fixed_flat);
    kum = max(ku[0], ku[3]);
    km = max(kvm, kum);
#   if NOFILL_TEST
	dbg_nofill = false;
#   endif
    code = fill_wedges(pfs, ku[0], kum, p.pole[0], 1, &p.c[0][0], &p.c[0][1], 
		interpatch_padding | inpatch_wedge);
    if (code >= 0) {
	/* We would like to apply iterations for enumerating the kvm curve parts,
	   but the roundinmg errors would be too complicated due to
	   the dependence on the direction. Note that neigbour
	   patches may use the opposite direction for same bounding curve.
	   We apply the recursive dichotomy, in which 
	   the rounding errors do not depend on the direction. */
#	if NOFILL_TEST
	    dbg_nofill = false;
	    code = fill_patch(pfs, &p, kvm, kv[0], kv[3]);
	    dbg_nofill = true;
#	endif
	code = fill_patch(pfs, &p, kvm, kv[0], kv[3]);
    }
    if (code >= 0)
	code = fill_wedges(pfs, ku[3], kum, p.pole[3], 1, &p.c[1][0], &p.c[1][1], 
		interpatch_padding | inpatch_wedge);
    return code;
}
