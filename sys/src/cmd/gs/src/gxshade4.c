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

/*$Id: gxshade4.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Rendering for Gouraud triangle shadings */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gxcspace.h"
#include "gxdcolor.h"
#include "gxdevcli.h"
#include "gxistate.h"
#include "gxpath.h"
#include "gxshade.h"
#include "gxshade4.h"

/* ---------------- Triangle mesh filling ---------------- */

/* Initialize the fill state for triangle shading. */
void
mesh_init_fill_state(mesh_fill_state_t * pfs, const gs_shading_mesh_t * psh,
		     const gs_rect * rect, gx_device * dev,
		     gs_imager_state * pis)
{
    shade_init_fill_state((shading_fill_state_t *) pfs,
			  (const gs_shading_t *)psh, dev, pis);
    pfs->pshm = psh;
    shade_bbox_transform2fixed(rect, pis, &pfs->rect);
}

/* Initialize the recursion state for filling one triangle. */
void
mesh_init_fill_triangle(mesh_fill_state_t * pfs,
  const mesh_vertex_t *va, const mesh_vertex_t *vb, const mesh_vertex_t *vc,
  bool check_clipping)
{
    pfs->depth = 1;
    pfs->frames[0].va = *va;
    pfs->frames[0].vb = *vb;
    pfs->frames[0].vc = *vc;
    pfs->frames[0].check_clipping = check_clipping;
}

#define SET_MIN_MAX_3(vmin, vmax, a, b, c)\
  if ( a < b ) vmin = a, vmax = b; else vmin = b, vmax = a;\
  if ( c < vmin ) vmin = c; else if ( c > vmax ) vmax = c

int
mesh_fill_triangle(mesh_fill_state_t * pfs)
{
    const gs_shading_mesh_t *psh = pfs->pshm;
    gs_imager_state *pis = pfs->pis;
    mesh_frame_t *fp = &pfs->frames[pfs->depth - 1];
    int ci;

    for (;;) {
	bool check = fp->check_clipping;

	/*
	 * Fill the triangle with vertices at fp->va.p, fp->vb.p, and
	 * fp->vc.p with color fp->va.cc.  If check is true, check for
	 * whether the triangle is entirely inside the rectangle, entirely
	 * outside, or partly inside; if check is false, assume the triangle
	 * is entirely inside.
	 */
	if (check) {
	    fixed xmin, ymin, xmax, ymax;

	    SET_MIN_MAX_3(xmin, xmax, fp->va.p.x, fp->vb.p.x, fp->vc.p.x);
	    SET_MIN_MAX_3(ymin, ymax, fp->va.p.y, fp->vb.p.y, fp->vc.p.y);
	    if (xmin >= pfs->rect.p.x && xmax <= pfs->rect.q.x &&
		ymin >= pfs->rect.p.y && ymax <= pfs->rect.q.y
		) {
		/* The triangle is entirely inside the rectangle. */
		check = false;
	    } else if (xmin >= pfs->rect.q.x || xmax <= pfs->rect.p.x ||
		       ymin >= pfs->rect.q.y || ymax <= pfs->rect.p.y
		       ) {
		/* The triangle is entirely outside the rectangle. */
		goto next;
	    }
	}
	if (fp < &pfs->frames[countof(pfs->frames) - 3]) {
	/* Check whether the colors fall within the smoothness criterion. */
	    for (ci = 0; ci < pfs->num_components; ++ci) {
		float
		    c0 = fp->va.cc[ci], c1 = fp->vb.cc[ci], c2 = fp->vc.cc[ci];
		float cmin, cmax;

		SET_MIN_MAX_3(cmin, cmax, c0, c1, c2);
		if (cmax - cmin > pfs->cc_max_error[ci])
		    goto nofill;
	    }
	}
    fill:
	/* Fill the triangle with the color. */
	{
	    gx_device_color dev_color;
	    const gs_color_space *pcs = psh->params.ColorSpace;
	    gs_client_color fcc;
	    int code;

	    memcpy(&fcc.paint, fp->va.cc, sizeof(fcc.paint));
	    (*pcs->type->restrict_color)(&fcc, pcs);
	    (*pcs->type->remap_color)(&fcc, pcs, &dev_color, pis,
				      pfs->dev, gs_color_select_texture);
	    /****** SHOULD ADD adjust ON ANY OUTSIDE EDGES ******/
#if 0
	    {
		gx_path *ppath = gx_path_alloc(pis->memory, "Gt_fill");

		gx_path_add_point(ppath, fp->va.p.x, fp->va.p.y);
		gx_path_add_line(ppath, fp->vb.p.x, fp->vb.p.y);
		gx_path_add_line(ppath, fp->vc.p.x, fp->vc.p.y);
		code = shade_fill_path((const shading_fill_state_t *)pfs,
				       ppath, &dev_color);
		gx_path_free(ppath, "Gt_fill");
	    }
#else
	    code = (*dev_proc(pfs->dev, fill_triangle))
		(pfs->dev, fp->va.p.x, fp->va.p.y,
		 fp->vb.p.x - fp->va.p.x, fp->vb.p.y - fp->va.p.y,
		 fp->vc.p.x - fp->va.p.x, fp->vc.p.y - fp->va.p.y,
		 &dev_color, pis->log_op);
#endif
	    if (code < 0)
		return code;
	}
    next:
	if (fp == &pfs->frames[0])
	    return 0;
	--fp;
	continue;
    nofill:
	/*
	 * The colors don't converge.  Does the region color more than
	 * a single pixel?
	 */
	{
	    gs_fixed_rect region;

	    SET_MIN_MAX_3(region.p.x, region.q.x,
			  fp->va.p.x, fp->vb.p.x, fp->vc.p.x);
	    SET_MIN_MAX_3(region.p.y, region.q.y,
			  fp->va.p.y, fp->vb.p.y, fp->vc.p.y);
	    if (region.q.x - region.p.x <= fixed_1 &&
		region.q.y - region.p.y <= fixed_1) {
		/*
		 * More precisely, does the bounding box of the region,
		 * taking fill adjustment into account, span more than 1
		 * pixel center in either X or Y?
		 */
		fixed ax = pis->fill_adjust.x;
		int nx =
		    fixed2int_pixround(region.q.x + ax) -
		    fixed2int_pixround(region.p.x - ax);
		fixed ay = pis->fill_adjust.y;
		int ny =
		    fixed2int_pixround(region.q.y + ay) -
		    fixed2int_pixround(region.p.y - ay);

		if (!(nx > 1 && ny != 0) || (ny > 1 && nx != 0))
		    goto fill;
	    }
	}
	/*
	 * Subdivide the triangle and recur.  The only subdivision method
	 * that doesn't seem to create anomalous shapes divides the
	 * triangle in 4, using the midpoints of each side.
	 *
	 * If the original vertices are A, B, C, we fill the sub-triangles
	 * in the following order:
	 *	(A, AB, AC) - fp[3]
	 *	(AB, AC, BC) - fp[2]
	 *	(AC, BC, C) - fp[1]
	 *	(AB, B, BC) - fp[0]
	 */
	{
#define VAB fp[3].vb
#define VAC fp[2].vb
#define VBC fp[1].vb
	    int i;

#define MIDPOINT_FAST(a,b) arith_rshift_1((a) + (b) + 1)
	    VAB.p.x = MIDPOINT_FAST(fp->va.p.x, fp->vb.p.x);
	    VAB.p.y = MIDPOINT_FAST(fp->va.p.y, fp->vb.p.y);
	    VAC.p.x = MIDPOINT_FAST(fp->va.p.x, fp->vc.p.x);
	    VAC.p.y = MIDPOINT_FAST(fp->va.p.y, fp->vc.p.y);
	    VBC.p.x = MIDPOINT_FAST(fp->vb.p.x, fp->vc.p.x);
	    VBC.p.y = MIDPOINT_FAST(fp->vb.p.y, fp->vc.p.y);
#undef MIDPOINT_FAST
	    for (i = 0; i < pfs->num_components; ++i) {
		float ta = fp->va.cc[i], tb = fp->vb.cc[i], tc = fp->vc.cc[i];

		VAB.cc[i] = (ta + tb) * 0.5;
		VAC.cc[i] = (ta + tc) * 0.5;
		VBC.cc[i] = (tb + tc) * 0.5;
	    }
	    /* Fill in the rest of the triangles. */
	    fp[3].va = fp->va;
	    fp[3].vc = VAC;
	    fp[2].va = VAB;
	    fp[2].vc = VBC;
	    fp[1].va = VAC;
	    fp[1].vc = fp->vc;
	    fp->va = VAB;
	    fp->vc = VBC;
	    fp[3].check_clipping = fp[2].check_clipping =
		fp[1].check_clipping = fp->check_clipping = check;
#undef VAB
#undef VAC
#undef VBC
	    fp += 3;
	}
    }
}

/* ---------------- Gouraud triangle shadings ---------------- */

private int
Gt_next_vertex(const gs_shading_mesh_t * psh, shade_coord_stream_t * cs,
	       mesh_vertex_t * vertex)
{
    int code = shade_next_vertex(cs, vertex);

    if (code >= 0 && psh->params.Function) {
	/* Decode the color with the function. */
	code = gs_function_evaluate(psh->params.Function, vertex->cc,
				    vertex->cc);
    }
    return code;
}

inline private int
Gt_fill_triangle(mesh_fill_state_t * pfs, const mesh_vertex_t * va,
		 const mesh_vertex_t * vb, const mesh_vertex_t * vc)
{
    mesh_init_fill_triangle(pfs, va, vb, vc, true);
    return mesh_fill_triangle(pfs);
}

int
gs_shading_FfGt_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			       gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_FfGt_t * const psh = (const gs_shading_FfGt_t *)psh0;
    mesh_fill_state_t state;
    shade_coord_stream_t cs;
    int num_bits = psh->params.BitsPerFlag;
    int flag;
    mesh_vertex_t va, vb, vc;

    mesh_init_fill_state(&state, (const gs_shading_mesh_t *)psh, rect,
			 dev, pis);
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params,
		    pis);
    while ((flag = shade_next_flag(&cs, num_bits)) >= 0) {
	int code;

	switch (flag) {
	    default:
		return_error(gs_error_rangecheck);
	    case 0:
		if ((code = Gt_next_vertex(state.pshm, &cs, &va)) < 0 ||
		    (code = shade_next_flag(&cs, num_bits)) < 0 ||
		    (code = Gt_next_vertex(state.pshm, &cs, &vb)) < 0 ||
		    (code = shade_next_flag(&cs, num_bits)) < 0
		    )
		    return code;
		goto v2;
	    case 1:
		va = vb;
	    case 2:
		vb = vc;
v2:		if ((code = Gt_next_vertex(state.pshm, &cs, &vc)) < 0 ||
		    (code = Gt_fill_triangle(&state, &va, &vb, &vc)) < 0
		    )
		    return code;
	}
    }
    return 0;
}

int
gs_shading_LfGt_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			       gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_LfGt_t * const psh = (const gs_shading_LfGt_t *)psh0;
    mesh_fill_state_t state;
    shade_coord_stream_t cs;
    mesh_vertex_t *vertex;
    mesh_vertex_t next;
    int per_row = psh->params.VerticesPerRow;
    int i, code = 0;

    mesh_init_fill_state(&state, (const gs_shading_mesh_t *)psh, rect,
			 dev, pis);
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params,
		    pis);
    vertex = (mesh_vertex_t *)
	gs_alloc_byte_array(pis->memory, per_row, sizeof(*vertex),
			    "gs_shading_LfGt_render");
    if (vertex == 0)
	return_error(gs_error_VMerror);
    for (i = 0; i < per_row; ++i)
	if ((code = Gt_next_vertex(state.pshm, &cs, &vertex[i])) < 0)
	    goto out;
    while (!seofp(cs.s)) {
	code = Gt_next_vertex(state.pshm, &cs, &next);
	if (code < 0)
	    goto out;
	for (i = 1; i < per_row; ++i) {
	    code = Gt_fill_triangle(&state, &vertex[i - 1], &vertex[i], &next);
	    if (code < 0)
		goto out;
	    vertex[i - 1] = next;
	    code = Gt_next_vertex(state.pshm, &cs, &next);
	    if (code < 0)
		goto out;
	    code = Gt_fill_triangle(&state, &vertex[i], &vertex[i - 1], &next);
	    if (code < 0)
		goto out;
	}
	vertex[per_row - 1] = next;
    }
out:
    gs_free_object(pis->memory, vertex, "gs_shading_LfGt_render");
    return code;
}
