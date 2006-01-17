/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscsepr.c,v 1.27 2004/08/04 19:36:12 stefan Exp $ */
/* Separation color space and operation definition */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsfunc.h"
#include "gsrefct.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gscsepr.h"
#include "gxcspace.h"
#include "gxfixed.h"		/* for gxcolor2.h */
#include "gxcolor2.h"		/* for gs_indexed_map */
#include "gzstate.h"		/* for pgs->overprint */
#include "gscdevn.h"		/* for alloc_device_n_map */
#include "gxcdevn.h"		/* for gs_device_n_map_s */
#include "gxcmap.h"
#include "gxdevcli.h"
#include "gsovrc.h"
#include "stream.h"

/* ---------------- Color space ---------------- */

gs_private_st_composite(st_color_space_Separation, gs_paint_color_space,
			"gs_color_space_Separation",
			cs_Separation_enum_ptrs, cs_Separation_reloc_ptrs);

/* Define the Separation color space type. */
private cs_proc_base_space(gx_alt_space_Separation);
private cs_proc_init_color(gx_init_Separation);
private cs_proc_concrete_space(gx_concrete_space_Separation);
private cs_proc_concretize_color(gx_concretize_Separation);
private cs_proc_remap_concrete_color(gx_remap_concrete_Separation);
private cs_proc_remap_color(gx_remap_Separation);
private cs_proc_install_cspace(gx_install_Separation);
private cs_proc_set_overprint(gx_set_overprint_Separation);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_Separation);
private cs_proc_serialize(gx_serialize_Separation);
const gs_color_space_type gs_color_space_type_Separation = {
    gs_color_space_index_Separation, true, false,
    &st_color_space_Separation, gx_num_components_1,
    gx_alt_space_Separation,
    gx_init_Separation, gx_restrict01_paint_1,
    gx_concrete_space_Separation,
    gx_concretize_Separation, gx_remap_concrete_Separation,
    gx_remap_Separation, gx_install_Separation,
    gx_set_overprint_Separation,
    gx_adjust_cspace_Separation, gx_no_adjust_color_count,
    gx_serialize_Separation,
    gx_cspace_is_linear_default
};

/* GC procedures */

private 
ENUM_PTRS_WITH(cs_Separation_enum_ptrs, gs_color_space *pcs)
{
    return ENUM_USING(*pcs->params.separation.alt_space.type->stype,
		      &pcs->params.separation.alt_space,
		      sizeof(pcs->params.separation.alt_space), index - 1);
}
ENUM_PTR(0, gs_color_space, params.separation.map);
ENUM_PTRS_END
private RELOC_PTRS_WITH(cs_Separation_reloc_ptrs, gs_color_space *pcs)
{
    RELOC_PTR(gs_color_space, params.separation.map);
    RELOC_USING(*pcs->params.separation.alt_space.type->stype,
		&pcs->params.separation.alt_space,
		sizeof(gs_base_color_space));
}
RELOC_PTRS_END

/* Get the alternate space for a Separation space. */
private const gs_color_space *
gx_alt_space_Separation(const gs_color_space * pcs)
{
    return pcs->params.separation.use_alt_cspace
	   ? (const gs_color_space *)&(pcs->params.separation.alt_space)
    	   : NULL;
}

/* Get the concrete space for a Separation space. */
private const gs_color_space *
gx_concrete_space_Separation(const gs_color_space * pcs,
			     const gs_imager_state * pis)
{
#ifdef DEBUG
    /* 
     * Verify that the color space and imager state info match.
     */
    if (pcs->id != pis->color_component_map.cspace_id)
	dprintf("gx_concretze_space_Separation: color space id mismatch");
#endif

    /*
     * Check if we are using the alternate color space.
     */
    if (pis->color_component_map.use_alt_cspace) {
        const gs_color_space *pacs =
	    (const gs_color_space *)&pcs->params.separation.alt_space;

        return cs_concrete_space(pacs, pis);
    }
    /*
     * Separation color spaces are concrete (when not using alt. color space).
     */
    return pcs;
}

private int
check_Separation_component_name(const gs_color_space * pcs, gs_state * pgs);

/* Install a Separation color space. */
private int
gx_install_Separation(const gs_color_space * pcs, gs_state * pgs)
{
    int code = check_Separation_component_name(pcs, pgs);

    if (code < 0)
       return code;
    pgs->color_space->params.separation.use_alt_cspace =
	using_alt_color_space(pgs);
    if (pgs->color_space->params.separation.use_alt_cspace)
        code = (*pcs->params.separation.alt_space.type->install_cspace)
	((const gs_color_space *) & pcs->params.separation.alt_space, pgs);
    /*
     * Give the device an opportunity to capture equivalent colors for any
     * spot colors which might be present in the color space.
     */
    if (code >= 0)
        code = dev_proc(pgs->device, update_spot_equivalent_colors)
							(pgs->device, pgs);
    return code;
}

/* Set the overprint information appropriate to a separation color space */
private int
gx_set_overprint_Separation(const gs_color_space * pcs, gs_state * pgs)
{
    gs_devicen_color_map *  pcmap = &pgs->color_component_map;

    if (pcmap->use_alt_cspace)
        return gx_spot_colors_set_overprint( 
                   (const gs_color_space *)&pcs->params.separation.alt_space,
                   pgs );
    else {
        gs_overprint_params_t   params;

        params.retain_any_comps = pgs->overprint &&
                                  pcs->params.separation.sep_type != SEP_ALL;
        if (params.retain_any_comps) {
            params.retain_spot_comps = false;
            params.drawn_comps = 0;
            if (pcs->params.separation.sep_type != SEP_NONE) {
                int     mcomp = pcmap->color_map[0];

                if (mcomp >= 0)
		    gs_overprint_set_drawn_comp( params.drawn_comps, mcomp);
            }
        }

        pgs->effective_overprint_mode = 0;
        return gs_state_update_overprint(pgs, &params);
    }
}

/* Adjust the reference count of a Separation color space. */
private void
gx_adjust_cspace_Separation(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.separation.map, delta,
		    "gx_adjust_Separation");
    (*pcs->params.separation.alt_space.type->adjust_cspace_count)
	((const gs_color_space *)&pcs->params.separation.alt_space, delta);
}

/* ------ Constructors/accessors ------ */

/*
 * Build a separation color space.
 */
int
gs_build_Separation(
		    gs_color_space * pcspace,
		    const gs_color_space * palt_cspace,
		    gs_memory_t * pmem
)
{
    gs_separation_params * pcssepr = &pcspace->params.separation;
    int code;

    if (palt_cspace == 0 || !palt_cspace->type->can_be_alt_space)
	return_error(gs_error_rangecheck);

    code = alloc_device_n_map(&pcssepr->map, pmem, "gs_cspace_build_Separation");
    if (pcssepr->map == NULL) {
	gs_free_object(pmem, pcspace, "gs_cspace_build_Separation");
	return_error(gs_error_VMerror);
    }
    return 0;
}

/*
 * Build a separation color space.
 *
 * The values array provided with separation color spaces is actually cached
 * information, but filled in by the client. The alternative space is the
 * color space in which the tint procedure will provide alternative colors.
 */
int
gs_cspace_build_Separation(
			      gs_color_space ** ppcspace,
			      gs_separation_name sname,
			      const gs_color_space * palt_cspace,
			      int cache_size,
			      gs_memory_t * pmem
)
{
    gs_color_space *pcspace = NULL;
    gs_separation_params *pcssepr = NULL;
    int code;

    if (palt_cspace == 0 || !palt_cspace->type->can_be_alt_space)
	return_error(gs_error_rangecheck);

    code = gs_cspace_alloc(&pcspace, &gs_color_space_type_Separation, pmem);
    if (code < 0)
	return code;
 
    code = gs_build_Separation(pcspace, palt_cspace, pmem);
    if (code < 0) {
	gs_free_object(pmem, pcspace, "gs_cspace_build_Separation");
	return code;
    }
    pcssepr->sep_name = sname;
    gs_cspace_init_from((gs_color_space *) & pcssepr->alt_space, palt_cspace);
    *ppcspace = pcspace;
    return 0;
}

#if 0 /* Unused; Unsupported by gx_serialize_device_n_map. */
/*
 * Set the tint transformation procedure used by a Separation color space.
 */
int
gs_cspace_set_sepr_proc(gs_color_space * pcspace,
			int (*proc)(const float *,
                                    float *,
                                    const gs_imager_state *,
                                    void *
				    ),
			void *proc_data
			)
{
    gs_device_n_map *pimap;

    if (gs_color_space_get_index(pcspace) != gs_color_space_index_Separation)
	return_error(gs_error_rangecheck);
    pimap = pcspace->params.separation.map;
    pimap->tint_transform = proc;
    pimap->tint_transform_data = proc_data;
    pimap->cache_valid = false;

    return 0;
}
#endif

/*
 * Set the Separation tint transformation procedure to a Function.
 */
int
gs_cspace_set_sepr_function(const gs_color_space *pcspace, gs_function_t *pfn)
{
    gs_device_n_map *pimap;

    if (gs_color_space_get_index(pcspace) != gs_color_space_index_Separation ||
	pfn->params.m != 1 || pfn->params.n !=
	  gs_color_space_num_components((const gs_color_space *)
					&pcspace->params.separation.alt_space)
	)
	return_error(gs_error_rangecheck);
    pimap = pcspace->params.separation.map;
    pimap->tint_transform = map_devn_using_function;
    pimap->tint_transform_data = pfn;
    pimap->cache_valid = false;
    return 0;
}

/*
 * If the Separation tint transformation procedure is a Function,
 * return the function object, otherwise return 0.
 */
gs_function_t *
gs_cspace_get_sepr_function(const gs_color_space *pcspace)
{
    if (gs_color_space_get_index(pcspace) == gs_color_space_index_Separation &&
	pcspace->params.separation.map->tint_transform ==
	  map_devn_using_function)
	return pcspace->params.separation.map->tint_transform_data;
    return 0;
}

/* ------ Internal procedures ------ */

/* Initialize a Separation color. */
private void
gx_init_Separation(gs_client_color * pcc, const gs_color_space * pcs)
{
    pcc->paint.values[0] = 1.0;
}

/* Remap a Separation color. */

private int
gx_remap_Separation(const gs_client_color * pcc, const gs_color_space * pcs,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
		       gs_color_select_t select)
{
    int code = 0;

    if (pcs->params.separation.sep_type != SEP_NONE)
	code = gx_default_remap_color(pcc, pcs, pdc, pis, dev, select);
    else {
        color_set_null(pdc);
    }
    /* Save original color space and color info into dev color */
    pdc->ccolor.paint.values[0] = pcc->paint.values[0];
    pdc->ccolor_valid = true;
    return code;
}

private int
gx_concretize_Separation(const gs_client_color *pc, const gs_color_space *pcs,
			 frac *pconc, const gs_imager_state *pis)
{
    float ftemp;
    int code;
    gs_client_color cc;
    const gs_color_space *pacs =
	(const gs_color_space *)&pcs->params.separation.alt_space;
    
    if (pcs->params.separation.sep_type == SEP_OTHER &&
        pcs->params.separation.use_alt_cspace) {
        gs_device_n_map *map = pcs->params.separation.map;

	/* Check the 1-element cache first. */
	if (map->cache_valid && map->tint[0] == pc->paint.values[0]) {
	    int i, num_out = gs_color_space_num_components(pacs);

	    for (i = 0; i < num_out; ++i)
		pconc[i] = map->conc[i];
	    return 0;
	}
        code = (*pcs->params.separation.map->tint_transform)
	    (pc->paint.values, &cc.paint.values[0],
	     pis, pcs->params.separation.map->tint_transform_data);
        if (code < 0)
	    return code;
	return cs_concretize_color(&cc, pacs, pconc, pis);
    }
    else {
    	pconc[0] = unit_frac(pc->paint.values[0], ftemp);
    }
    return 0;
}

private int
gx_remap_concrete_Separation(const frac * pconc,  const gs_color_space * pcs,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
			     gs_color_select_t select)
{
#ifdef DEBUG
    /* 
     * Verify that the color space and imager state info match.
     */
    if (pcs->id != pis->color_component_map.cspace_id)
	dprintf("gx_remap_concrete_Separation: color space id mismatch");
#endif

    if (pis->color_component_map.use_alt_cspace) {
        const gs_color_space *pacs =
	    (const gs_color_space *)&pcs->params.separation.alt_space;

	return (*pacs->type->remap_concrete_color)
				(pconc, pacs, pdc, pis, dev, select);
    }
    else {
        gx_remap_concrete_separation(pconc[0], pdc, pis, dev, select);
        return 0;
    }
}

/*
 * Check that the color component name for a Separation color space
 * matches the device colorant names.  Also build a gs_devicen_color_map
 * structure.
 */
private int
check_Separation_component_name(const gs_color_space * pcs, gs_state * pgs)
{
    const gs_separation_name name = pcs->params.separation.sep_name;
    int colorant_number;
    byte * pname;
    uint name_size;
    gs_devicen_color_map * pcolor_component_map
	= &pgs->color_component_map;
    gx_device * dev = pgs->device;

    pcolor_component_map->num_components = 1;
    pcolor_component_map->cspace_id = pcs->id;
    pcolor_component_map->num_colorants = dev->color_info.num_components;
    pcolor_component_map->sep_type = pcs->params.separation.sep_type;
    /*
     * If this is a None or All separation then we do not need to
     * use the alternate color space.
     */
    if (pcs->params.separation.sep_type != SEP_OTHER) {
	pcolor_component_map->use_alt_cspace = false;
	return 0;
    }
    /*
     * Always use the alternate color space if the current device is
     * using an additive color model.  Separations are only for use
     * with a subtractive color model.
     */
    if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE) {
	pcolor_component_map->use_alt_cspace = true;
	return 0;
    }
    /*
     * Get the character string and length for the component name.
     */
    pcs->params.separation.get_colorname_string(dev->memory, name, &pname, &name_size);
    /*
     * Compare the colorant name to the device's.  If the device's
     * compare routine returns GX_DEVICE_COLOR_MAX_COMPONENTS then the
     * colorant is in the SeparationNames list but not in the
     * SeparationOrder list.
     */
    colorant_number = (*dev_proc(dev, get_color_comp_index))
		(dev, (const char *)pname, name_size, SEPARATION_NAME);
    if (colorant_number >= 0) {		/* If valid colorant name */
	pcolor_component_map->color_map[0] =
		    (colorant_number == GX_DEVICE_COLOR_MAX_COMPONENTS) ? -1
		    					   : colorant_number;
	pcolor_component_map->use_alt_cspace = false;
    }
    else
	pcolor_component_map->use_alt_cspace = true;
    return 0;
}


/* ---------------- Notes on real Separation colors ---------------- */

typedef ulong gs_separation;	/* BOGUS */

#define gs_no_separation ((gs_separation)(-1L))

#define dev_proc_lookup_separation(proc)\
  gs_separation proc(gx_device *dev, const byte *sname, uint len,\
    gx_color_value *num_levels)

#define dev_proc_map_tint_color(proc)\
  gx_color_index proc(gx_device *dev, gs_separation sepr, bool overprint,\
    gx_color_value tint)

/*
 * This next comment is outdated since the Separation color space no longer
 * has the multi element cache (lookup table) however the remainder is
 * still appropriate.
 *
 * In principle, setting a Separation color space, or setting the device
 * when the current color space is a Separation space, calls the
 * lookup_separation device procedure to obtain the separation ID and
 * the number of achievable levels.  Currently, the only hooks for doing
 * this are unsuitable: gx_set_cmap_procs isn't called when the color
 * space changes, and doing it in gx_remap_Separation is inefficient.
 * Probably the best approach is to call gx_set_cmap_procs whenever the
 * color space changes.  In fact, if we do this, we can probably short-cut
 * two levels of procedure call in color remapping (gx_remap_color, by
 * turning it into a macro, and gx_remap_DeviceXXX, by calling the
 * cmap_proc procedure directly).  Some care will be required for the
 * implicit temporary resetting of the color space in [color]image.
 */

/* ---------------- Serialization. -------------------------------- */

private int 
gx_serialize_Separation(const gs_color_space * pcs, stream * s)
{
    const gs_separation_params * p = &pcs->params.separation;
    uint n;
    int code = gx_serialize_cspace_type(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->sep_name, sizeof(p->sep_name), &n);
    if (code < 0)
	return code;
    code = cs_serialize((const gs_color_space *)&p->alt_space, s);
    if (code < 0)
	return code;
    code = gx_serialize_device_n_map(pcs, p->map, s);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&p->sep_type, sizeof(p->sep_type), &n);
    /* p->use_alt_cspace isn't a property of the space. */
}
