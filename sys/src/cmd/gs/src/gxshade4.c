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

/* $Id: gxshade4.c,v 1.30 2005/04/19 12:22:08 igor Exp $ */
/* Rendering for Gouraud triangle shadings */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gsptype2.h"
#include "gxcspace.h"
#include "gxdcolor.h"
#include "gxdevcli.h"
#include "gxistate.h"
#include "gxpath.h"
#include "gxshade.h"
#include "gxshade4.h"
#include "vdtrace.h"

#define VD_TRACE_TRIANGLE_PATCH 1

/* Initialize the fill state for triangle shading. */
int
mesh_init_fill_state(mesh_fill_state_t * pfs, const gs_shading_mesh_t * psh,
		     const gs_fixed_rect * rect_clip, gx_device * dev,
		     gs_imager_state * pis)
{
    shade_init_fill_state((shading_fill_state_t *) pfs,
			  (const gs_shading_t *)psh, dev, pis);
    pfs->pshm = psh;
    pfs->rect = *rect_clip;
    return 0;
}

/* ---------------- Gouraud triangle shadings ---------------- */

private int
Gt_next_vertex(const gs_shading_mesh_t * psh, shade_coord_stream_t * cs,
	       shading_vertex_t * vertex)
{
    int code = shade_next_vertex(cs, vertex);

    if (code >= 0 && psh->params.Function) {
	vertex->c.t[0] = vertex->c.cc.paint.values[0];
	vertex->c.t[1] = 0;
	/* Decode the color with the function. */
	code = gs_function_evaluate(psh->params.Function, vertex->c.t,
				    vertex->c.cc.paint.values);
    }
    return code;
}

inline private int
Gt_fill_triangle(mesh_fill_state_t * pfs, const shading_vertex_t * va,
		 const shading_vertex_t * vb, const shading_vertex_t * vc)
{
    patch_fill_state_t pfs1;
    int code = 0;

    memcpy(&pfs1, (shading_fill_state_t *)pfs, sizeof(shading_fill_state_t));
    pfs1.Function = pfs->pshm->params.Function;
    pfs1.rect = pfs->rect;
    code = init_patch_fill_state(&pfs1);
    if (code < 0)
	return code;
    if (INTERPATCH_PADDING) {
	code = mesh_padding(&pfs1, &va->p, &vb->p, &va->c, &vb->c);
	if (code >= 0)
	    code = mesh_padding(&pfs1, &vb->p, &vc->p, &vb->c, &vc->c);
	if (code >= 0)
	    code = mesh_padding(&pfs1, &vc->p, &va->p, &vc->c, &va->c);
    }
    if (code >= 0)
	code = mesh_triangle(&pfs1, va, vb, vc);
    term_patch_fill_state(&pfs1);
    return code;
}

int
gs_shading_FfGt_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			       const gs_fixed_rect * rect_clip,
			       gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_FfGt_t * const psh = (const gs_shading_FfGt_t *)psh0;
    mesh_fill_state_t state;
    shade_coord_stream_t cs;
    int num_bits = psh->params.BitsPerFlag;
    int flag;
    shading_vertex_t va, vb, vc;
    int code;

    if (VD_TRACE_TRIANGLE_PATCH && vd_allowed('s')) {
	vd_get_dc('s');
	vd_set_shift(0, 0);
	vd_set_scale(0.01);
	vd_set_origin(0, 0);
    }
    code = mesh_init_fill_state(&state, (const gs_shading_mesh_t *)psh, rect_clip,
			 dev, pis);
    if (code < 0)
	return code;
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params,
		    pis);
    while ((flag = shade_next_flag(&cs, num_bits)) >= 0) {
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
v2:		if ((code = Gt_next_vertex(state.pshm, &cs, &vc)) < 0)
		    return code;
		if ((code = Gt_fill_triangle(&state, &va, &vb, &vc)) < 0)
		    return code;
	}
    }
    if (VD_TRACE_TRIANGLE_PATCH && vd_allowed('s'))
	vd_release_dc;
    if (!cs.is_eod(&cs))
	return_error(gs_error_rangecheck);
    return 0;
}

int
gs_shading_LfGt_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
			       const gs_fixed_rect * rect_clip,
			       gx_device * dev, gs_imager_state * pis)
{
    const gs_shading_LfGt_t * const psh = (const gs_shading_LfGt_t *)psh0;
    mesh_fill_state_t state;
    shade_coord_stream_t cs;
    shading_vertex_t *vertex;
    shading_vertex_t next;
    int per_row = psh->params.VerticesPerRow;
    int i, code;

    if (VD_TRACE_TRIANGLE_PATCH && vd_allowed('s')) {
	vd_get_dc('s');
	vd_set_shift(0, 0);
	vd_set_scale(0.01);
	vd_set_origin(0, 0);
    }
    code = mesh_init_fill_state(&state, (const gs_shading_mesh_t *)psh, rect_clip,
			 dev, pis);
    if (code < 0)
	return code;
    shade_next_init(&cs, (const gs_shading_mesh_params_t *)&psh->params,
		    pis);
    vertex = (shading_vertex_t *)
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
    if (VD_TRACE_TRIANGLE_PATCH && vd_allowed('s'))
	vd_release_dc;
    gs_free_object(pis->memory, vertex, "gs_shading_LfGt_render");
    return code;
}
