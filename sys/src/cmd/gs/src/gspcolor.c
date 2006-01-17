/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gspcolor.c,v 1.14 2004/08/04 19:36:12 stefan Exp $ */
/* Pattern color operators and procedures for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsrop.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxcoord.h"		/* for gs_concat, gx_tr'_to_fixed */
#include "gxcspace.h"		/* for gscolor2.h */
#include "gxcolor2.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxclip2.h"
#include "gspath.h"
#include "gxpath.h"
#include "gxpcolor.h"
#include "gzstate.h"
#include "gsimage.h"
#include "gsiparm4.h"
#include "stream.h"

/* GC descriptors */
public_st_pattern_template();
public_st_pattern_instance();

/* Define the Pattern color space. */
gs_private_st_composite(st_color_space_Pattern, gs_paint_color_space,
     "gs_color_space_Pattern", cs_Pattern_enum_ptrs, cs_Pattern_reloc_ptrs);
private cs_proc_num_components(gx_num_components_Pattern);
private cs_proc_base_space(gx_base_space_Pattern);
private cs_proc_remap_color(gx_remap_Pattern);
private cs_proc_init_color(gx_init_Pattern);
private cs_proc_restrict_color(gx_restrict_Pattern);
private cs_proc_install_cspace(gx_install_Pattern);
private cs_proc_set_overprint(gx_set_overprint_Pattern);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_Pattern);
private cs_proc_adjust_color_count(gx_adjust_color_Pattern);
private cs_proc_serialize(gx_serialize_Pattern);
const gs_color_space_type gs_color_space_type_Pattern = {
    gs_color_space_index_Pattern, false, false,
    &st_color_space_Pattern, gx_num_components_Pattern,
    gx_base_space_Pattern,
    gx_init_Pattern, gx_restrict_Pattern,
    gx_no_concrete_space,
    gx_no_concretize_color, NULL,
    gx_remap_Pattern, gx_install_Pattern,
    gx_set_overprint_Pattern,
    gx_adjust_cspace_Pattern, gx_adjust_color_Pattern,
    gx_serialize_Pattern,
    gx_cspace_no_linear
};

/* Initialize a generic pattern template. */
void
gs_pattern_common_init(gs_pattern_template_t * ppat,
		       const gs_pattern_type_t *type)
{
    ppat->type = type;
    ppat->PatternType = type->PatternType;
    uid_set_invalid(&ppat->uid);
    ppat->client_data = 0;	/* for GC */
}

/* Generic makepattern */
int
gs_make_pattern(gs_client_color * pcc, const gs_pattern_template_t * pcp,
		const gs_matrix * pmat, gs_state * pgs, gs_memory_t * mem)
{
    return pcp->type->procs.make_pattern(pcc, pcp, pmat, pgs, mem);
}

/*
 * Do the generic work for makepattern: allocate the instance and the
 * saved graphics state, and fill in the common members.
 */
int
gs_make_pattern_common(gs_client_color *pcc,
		       const gs_pattern_template_t *ptemp,
		       const gs_matrix *pmat, gs_state *pgs, gs_memory_t *mem,
		       gs_memory_type_ptr_t pstype)
{
    gs_pattern_instance_t *pinst;
    gs_state *saved;

    if (mem == 0)
	mem = gs_state_memory(pgs);
    rc_alloc_struct_1(pinst, gs_pattern_instance_t, pstype, mem,
		      return_error(gs_error_VMerror),
		      "gs_make_pattern_common");
    pinst->rc.free = rc_free_pattern_instance;
    pinst->type = ptemp->type;
    saved = gs_state_copy(pgs, mem);
    if (saved == 0) {
	gs_free_object(mem, pinst, "gs_make_pattern_common");
	return_error(gs_error_VMerror);
    }
    gs_concat(saved, pmat);
    gs_newpath(saved);
    pinst->saved = saved;
    pcc->pattern = pinst;
    pcc->pattern->pattern_id = gs_next_ids(mem, 1);
    return 0;
}

/* Free the saved gstate when freeing a Pattern instance. */
void
rc_free_pattern_instance(gs_memory_t * mem, void *pinst_void,
			 client_name_t cname)
{
    gs_pattern_instance_t *pinst = pinst_void;

    gs_state_free(pinst->saved);
    rc_free_struct_only(mem, pinst_void, cname);
}

/* setpattern */
int
gs_setpattern(gs_state * pgs, const gs_client_color * pcc)
{
    int code = gs_setpatternspace(pgs);

    if (code < 0)
	return code;
    return gs_setcolor(pgs, pcc);
}

/* setpatternspace */
/* This does all the work of setpattern except for the final setcolor. */
int
gs_setpatternspace(gs_state * pgs)
{
    int code = 0;

    if (pgs->in_cachedevice)
	return_error(gs_error_undefined);
    if (pgs->color_space->type->index != gs_color_space_index_Pattern) {
	gs_color_space cs;

	gs_cspace_init(&cs, &gs_color_space_type_Pattern, pgs->memory, false);
	/**************** base_space SETTING IS WRONG ****************/
	cs.params.pattern.base_space =
	    *(gs_paint_color_space *) pgs->color_space;
	cs.params.pattern.has_base_space = true;
	*pgs->color_space = cs;
	cs_full_init_color(pgs->ccolor, &cs);
	gx_unset_dev_color(pgs);
    }
    return code;
}

/*
 * Adjust the reference count of a pattern. This is intended to support
 * applications (such as PCL) which maintain client colors outside of the
 * graphic state. Since the pattern instance structure is opaque to these
 * applications, they need some way to release or retain the instances as
 * needed.
 */
void
gs_pattern_reference(gs_client_color * pcc, int delta)
{
    if (pcc->pattern != 0)
        rc_adjust(pcc->pattern, delta, "gs_pattern_reference");
}

/* getpattern */
/* This is only intended for the benefit of pattern PaintProcs. */
const gs_pattern_template_t *
gs_get_pattern(const gs_client_color * pcc)
{
    const gs_pattern_instance_t *pinst = pcc->pattern;

    return (pinst == 0 ? 0 : pinst->type->procs.get_pattern(pinst));
}

/*
 * Get the number of components in a Pattern color.
 * For backward compatibility, and to distinguish Pattern color spaces
 * from all others, we negate the result.
 */
private int
gx_num_components_Pattern(const gs_color_space * pcs)
{
    return
	(pcs->params.pattern.has_base_space ?
	 -1 - cs_num_components((const gs_color_space *)
				&(pcs->params.pattern.base_space)) :
	 -1 /* Pattern dictionary only */ );
}

/* Get the base space of a Pattern color space. */
private const gs_color_space *
gx_base_space_Pattern(const gs_color_space * pcs)
{
    return
	(pcs->params.pattern.has_base_space ?
	 (const gs_color_space *)&(pcs->params.pattern.base_space) :
	 NULL);
}

/* Remap a Pattern color. */
private int
gx_remap_Pattern(const gs_client_color * pc, const gs_color_space * pcs,
		 gx_device_color * pdc, const gs_imager_state * pis,
		 gx_device * dev, gs_color_select_t select)
{
    if (pc->pattern == 0) {
        pdc->ccolor_valid = false;
	color_set_null_pattern(pdc);
	return 0;
    }
    return
	pc->pattern->type->procs.remap_color(pc, pcs, pdc, pis, dev, select);
}

/* Initialize a Pattern color. */
private void
gx_init_Pattern(gs_client_color * pcc, const gs_color_space * pcs)
{
    if (pcs->params.pattern.has_base_space) {
	const gs_color_space *pbcs =
	(const gs_color_space *)&pcs->params.pattern.base_space;

	cs_init_color(pcc, pbcs);
    }
    /*pcc->pattern = 0; *//* cs_full_init_color handles this */
}

/* Force a Pattern color into legal range. */
/* Note that if the pattern is uncolored (PaintType = 2), */
/* the color space must have a base space: we check this here only */
/* to prevent accessing uninitialized data, but if there is no base space, */
/* it is an error that we count on being detected elsewhere. */
private void
gx_restrict_Pattern(gs_client_color * pcc, const gs_color_space * pcs)
{
    /* We need a special check for the null pattern. */
    if (pcc->pattern &&
	pcc->pattern->type->procs.uses_base_space(gs_get_pattern(pcc)) &&
	pcs->params.pattern.has_base_space
	) {
	const gs_color_space *pbcs =
	    (const gs_color_space *)&pcs->params.pattern.base_space;

	(*pbcs->type->restrict_color) (pcc, pbcs);
    }
}

/* Install a Pattern color space. */
private int
gx_install_Pattern(const gs_color_space * pcs, gs_state * pgs)
{
    if (!pcs->params.pattern.has_base_space)
	return 0;
    return (*pcs->params.pattern.base_space.type->install_cspace)
	((const gs_color_space *) & pcs->params.pattern.base_space, pgs);
}

/*
 * Set the overprint compositor for a Pattern color space. This does nothing;
 * for patterns the overprint compositor is set at set_device_color time.
*/
private int
gx_set_overprint_Pattern(const gs_color_space * pcs, gs_state * pgs)
{
    return 0;
}

/* Adjust the reference counts for Pattern color spaces or colors. */
private void
gx_adjust_cspace_Pattern(const gs_color_space * pcs, int delta)
{
    if (pcs->params.pattern.has_base_space)
	(*pcs->params.pattern.base_space.type->adjust_cspace_count)
	    ((const gs_color_space *)&pcs->params.pattern.base_space, delta);
}

private void
gx_adjust_color_Pattern(const gs_client_color * pcc,
			const gs_color_space * pcs, int delta)
{
    gs_pattern_instance_t *pinst = pcc->pattern;

    rc_adjust_only(pinst, delta, "gx_adjust_color_Pattern");
    if (pcs && pcs->params.pattern.has_base_space)
	(*pcs->params.pattern.base_space.type->adjust_color_count)
	    (pcc, (const gs_color_space *)&pcs->params.pattern.base_space,
	     delta);
}

/* GC procedures */

private 
ENUM_PTRS_BEGIN_PROC(cs_Pattern_enum_ptrs)
{
    EV_CONST gs_color_space *const pcs = vptr;

    if (!pcs->params.pattern.has_base_space)
	return 0;
    return ENUM_USING(*pcs->params.pattern.base_space.type->stype,
		      &pcs->params.pattern.base_space,
		      sizeof(pcs->params.pattern.base_space), index);
}
ENUM_PTRS_END_PROC
private RELOC_PTRS_WITH(cs_Pattern_reloc_ptrs, gs_color_space *pcs)
{
    if (!pcs->params.pattern.has_base_space)
	return;
    RELOC_USING(*pcs->params.pattern.base_space.type->stype,
		&pcs->params.pattern.base_space,
		sizeof(gs_paint_color_space));
}
RELOC_PTRS_END

/* ---------------- Serialization. -------------------------------- */

private int 
gx_serialize_Pattern(const gs_color_space * pcs, stream * s)
{
    const gs_pattern_params * p = &pcs->params.pattern;
    uint n;
    int code = gx_serialize_cspace_type(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (byte *)&p->has_base_space, sizeof(p->has_base_space), &n);
    if (code < 0)
	return code;
    if (!p->has_base_space)
	return 0;
    return cs_serialize((const gs_color_space *)&p->base_space, s);
}
