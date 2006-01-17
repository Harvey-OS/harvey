/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscdevn.c,v 1.21 2004/08/04 19:36:12 stefan Exp $ */
/* DeviceN color space and operation definition */

#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscdevn.h"
#include "gsfunc.h"
#include "gsrefct.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gsstruct.h"
#include "gxcspace.h"
#include "gxcdevn.h"
#include "gxfarith.h"
#include "gxfrac.h"
#include "gxcmap.h"
#include "gxistate.h"
#include "gscoord.h"
#include "gzstate.h"
#include "gxdevcli.h"
#include "gsovrc.h"
#include "stream.h"

/* ---------------- Color space ---------------- */

/* GC descriptors */
gs_private_st_composite(st_color_space_DeviceN, gs_paint_color_space,
     "gs_color_space_DeviceN", cs_DeviceN_enum_ptrs, cs_DeviceN_reloc_ptrs);
private_st_device_n_map();

/* Define the DeviceN color space type. */
private cs_proc_num_components(gx_num_components_DeviceN);
private cs_proc_base_space(gx_alt_space_DeviceN);
private cs_proc_init_color(gx_init_DeviceN);
private cs_proc_restrict_color(gx_restrict_DeviceN);
private cs_proc_concrete_space(gx_concrete_space_DeviceN);
private cs_proc_concretize_color(gx_concretize_DeviceN);
private cs_proc_remap_concrete_color(gx_remap_concrete_DeviceN);
private cs_proc_install_cspace(gx_install_DeviceN);
private cs_proc_set_overprint(gx_set_overprint_DeviceN);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_DeviceN);
private cs_proc_serialize(gx_serialize_DeviceN);
const gs_color_space_type gs_color_space_type_DeviceN = {
    gs_color_space_index_DeviceN, true, false,
    &st_color_space_DeviceN, gx_num_components_DeviceN,
    gx_alt_space_DeviceN,
    gx_init_DeviceN, gx_restrict_DeviceN,
    gx_concrete_space_DeviceN,
    gx_concretize_DeviceN, gx_remap_concrete_DeviceN,
    gx_default_remap_color, gx_install_DeviceN,
    gx_set_overprint_DeviceN,
    gx_adjust_cspace_DeviceN, gx_no_adjust_color_count,
    gx_serialize_DeviceN,
    gx_cspace_is_linear_default
};

/* GC procedures */

private 
ENUM_PTRS_WITH(cs_DeviceN_enum_ptrs, gs_color_space *pcs)
{
    return ENUM_USING(*pcs->params.device_n.alt_space.type->stype,
		      &pcs->params.device_n.alt_space,
		      sizeof(pcs->params.device_n.alt_space), index - 2);
}
ENUM_PTR(0, gs_color_space, params.device_n.names);
ENUM_PTR(1, gs_color_space, params.device_n.map);
ENUM_PTRS_END
private RELOC_PTRS_WITH(cs_DeviceN_reloc_ptrs, gs_color_space *pcs)
{
    RELOC_PTR(gs_color_space, params.device_n.names);
    RELOC_PTR(gs_color_space, params.device_n.map);
    RELOC_USING(*pcs->params.device_n.alt_space.type->stype,
		&pcs->params.device_n.alt_space,
		sizeof(gs_base_color_space));
}
RELOC_PTRS_END

/* ------ Public procedures ------ */

/*
 * Build a DeviceN color space.  Not including allocation and
 * initialization of the color space.
 */
int
gs_build_DeviceN(
		gs_color_space *pcspace,
		uint num_components,
		const gs_color_space *palt_cspace,
		gs_memory_t *pmem
		)
{
    gs_device_n_params *pcsdevn = pcsdevn = &pcspace->params.device_n;
    gs_separation_name *pnames = 0;
    int code;

    if (palt_cspace == 0 || !palt_cspace->type->can_be_alt_space)
	return_error(gs_error_rangecheck);

    /* Allocate space for color names list. */
    code = alloc_device_n_map(&pcsdevn->map, pmem, "gs_cspace_build_DeviceN");
    if (code < 0) {
	return code;
    }
    /* Allocate space for color names list. */
    pnames = (gs_separation_name *)
	gs_alloc_byte_array(pmem, num_components, sizeof(gs_separation_name),
			  ".gs_cspace_build_DeviceN(names)");
    if (pnames == 0) {
	gs_free_object(pmem, pcsdevn->map, ".gs_cspace_build_DeviceN(map)");
	return_error(gs_error_VMerror);
    }
    pcsdevn->names = pnames;
    pcsdevn->num_components = num_components;
    return 0;
}

/*
 * Build a DeviceN color space.  Including allocation and initialization
 * of the color space.
 */
int
gs_cspace_build_DeviceN(
			gs_color_space **ppcspace,
			gs_separation_name *psnames,
			uint num_components,
			const gs_color_space *palt_cspace,
			gs_memory_t *pmem
			)
{
    gs_color_space *pcspace = 0; /* bogus initialization */
    gs_device_n_params *pcsdevn = 0; /* bogus initialization */
    int code;

    code = gs_cspace_alloc(&pcspace, &gs_color_space_type_DeviceN, pmem);
    if (code < 0)
	return code;

    code = gs_build_DeviceN(pcspace, num_components, palt_cspace, pmem);
    if (code < 0) {
	gs_free_object(pmem, pcspace, "gs_cspace_build_DeviceN");
	return code;
    }
    gs_cspace_init_from((gs_color_space *)&pcsdevn->alt_space, palt_cspace);
    *ppcspace = pcspace;
    return 0;
}

/* Allocate and initialize a DeviceN map. */
int
alloc_device_n_map(gs_device_n_map ** ppmap, gs_memory_t * mem,
		   client_name_t cname)
{
    gs_device_n_map *pimap;

    rc_alloc_struct_1(pimap, gs_device_n_map, &st_device_n_map, mem,
		      return_error(gs_error_VMerror), cname);
    pimap->tint_transform = 0;
    pimap->tint_transform_data = 0;
    pimap->cache_valid = false;
    *ppmap = pimap;
    return 0;
}

#if 0 /* Unused; Unsupported by gx_serialize_device_n_map. */
/*
 * Set the DeviceN tint transformation procedure.
 */
int
gs_cspace_set_devn_proc(gs_color_space * pcspace,
			int (*proc)(const float *,
                                    float *,
                                    const gs_imager_state *,
                                    void *
				    ),
			void *proc_data
			)
{
    gs_device_n_map *pimap;

    if (gs_color_space_get_index(pcspace) != gs_color_space_index_DeviceN)
	return_error(gs_error_rangecheck);
    pimap = pcspace->params.device_n.map;
    pimap->tint_transform = proc;
    pimap->tint_transform_data = proc_data;
    pimap->cache_valid = false;
    return 0;
}
#endif

/*
 * Check if we are using the alternate color space.
 */
bool
using_alt_color_space(const gs_state * pgs)
{
    return (pgs->color_component_map.use_alt_cspace);
}

/* Map a DeviceN color using a Function. */
int
map_devn_using_function(const float *in, float *out,
			const gs_imager_state *pis, void *data)

{
    gs_function_t *const pfn = data;

    return gs_function_evaluate(pfn, in, out);
}

/*
 * Set the DeviceN tint transformation procedure to a Function.
 */
int
gs_cspace_set_devn_function(gs_color_space *pcspace, gs_function_t *pfn)
{
    gs_device_n_map *pimap;

    if (gs_color_space_get_index(pcspace) != gs_color_space_index_DeviceN ||
	pfn->params.m != pcspace->params.device_n.num_components ||
	pfn->params.n !=
	  gs_color_space_num_components((gs_color_space *)
					&pcspace->params.device_n.alt_space)
	)
	return_error(gs_error_rangecheck);
    pimap = pcspace->params.device_n.map;
    pimap->tint_transform = map_devn_using_function;
    pimap->tint_transform_data = pfn;
    pimap->cache_valid = false;
    return 0;
}

/*
 * If the DeviceN tint transformation procedure is a Function,
 * return the function object, otherwise return 0.
 */
gs_function_t *
gs_cspace_get_devn_function(const gs_color_space *pcspace)
{
    if (gs_color_space_get_index(pcspace) == gs_color_space_index_DeviceN &&
	pcspace->params.device_n.map->tint_transform ==
	  map_devn_using_function)
	return pcspace->params.device_n.map->tint_transform_data;
    return 0;
}

/* ------ Color space implementation ------ */

/* Return the number of components of a DeviceN space. */
private int
gx_num_components_DeviceN(const gs_color_space * pcs)
{
    return pcs->params.device_n.num_components;
}

/* Return the alternate space of a DeviceN space. */
private const gs_color_space *
gx_alt_space_DeviceN(const gs_color_space * pcs)
{
    return pcs->params.device_n.use_alt_cspace
	   ? (const gs_color_space *)&(pcs->params.device_n.alt_space)
    	   : NULL;
}

/* Initialize a DeviceN color. */
private void
gx_init_DeviceN(gs_client_color * pcc, const gs_color_space * pcs)
{
    uint i;

    for (i = 0; i < pcs->params.device_n.num_components; ++i)
	pcc->paint.values[i] = 1.0;
}

/* Force a DeviceN color into legal range. */
private void
gx_restrict_DeviceN(gs_client_color * pcc, const gs_color_space * pcs)
{
    uint i;

    for (i = 0; i < pcs->params.device_n.num_components; ++i) {
	floatp value = pcc->paint.values[i];

	pcc->paint.values[i] = (value <= 0 ? 0 : value >= 1 ? 1 : value);
    }
}

/* Remap a DeviceN color. */
private const gs_color_space *
gx_concrete_space_DeviceN(const gs_color_space * pcs,
			  const gs_imager_state * pis)
{
#ifdef DEBUG
    /* 
     * Verify that the color space and imager state info match.
     */
    if (pcs->id != pis->color_component_map.cspace_id)
	dprintf("gx_concrete_space_DeviceN: color space id mismatch");
#endif

    /*
     * Check if we are using the alternate color space.
     */
    if (pis->color_component_map.use_alt_cspace) {
        const gs_color_space *pacs =
	    (const gs_color_space *)&pcs->params.device_n.alt_space;

        return cs_concrete_space(pacs, pis);
    }
    /*
     * DeviceN color spaces are concrete (when not using alt. color space).
     */
    return pcs;
}


private int
gx_concretize_DeviceN(const gs_client_color * pc, const gs_color_space * pcs,
		      frac * pconc, const gs_imager_state * pis)
{
    int code, tcode = 0;
    gs_client_color cc;
    const gs_color_space *pacs =
	(const gs_color_space *)&pcs->params.device_n.alt_space;
    gs_device_n_map *map = pcs->params.device_n.map;

#ifdef DEBUG
    /* 
     * Verify that the color space and imager state info match.
     */
    if (pcs->id != pis->color_component_map.cspace_id)
	dprintf("gx_concretize_DeviceN: color space id mismatch");
#endif

    /*
     * Check if we need to map into the alternate color space.
     * We must preserve tcode for implementing a semi-hack in the interpreter.
     */
    if (pis->color_component_map.use_alt_cspace) {
	    /* Check the 1-element cache first. */
	if (map->cache_valid) {
	    int i;

	    for (i = pcs->params.device_n.num_components; --i >= 0;) {
		if (map->tint[i] != pc->paint.values[i])
		    break;
	    }
	    if (i < 0) {
		int num_out = gs_color_space_num_components(pacs);

		for (i = 0; i < num_out; ++i)
		    pconc[i] = map->conc[i];
		return 0;
	    }
	}
        tcode = (*pcs->params.device_n.map->tint_transform)
	     (pc->paint.values, &cc.paint.values[0],
	     pis, pcs->params.device_n.map->tint_transform_data);
        if (tcode < 0)
	    return tcode;
	code = cs_concretize_color(&cc, pacs, pconc, pis);
    }
    else {
	float ftemp;
	int i;

	for (i = pcs->params.device_n.num_components; --i >= 0;)
	    pconc[i] = unit_frac(pc->paint.values[i], ftemp);
	return 0;
    }
    return (code < 0 || tcode == 0 ? code : tcode);
}

private int
gx_remap_concrete_DeviceN(const frac * pconc, const gs_color_space * pcs,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
			  gs_color_select_t select)
{
#ifdef DEBUG
    /* 
     * Verify that the color space and imager state info match.
     */
    if (pcs->id != pis->color_component_map.cspace_id)
	dprintf("gx_remap_concrete_DeviceN: color space id mismatch");
#endif

    if (pis->color_component_map.use_alt_cspace) {
        const gs_color_space *pacs =
	    (const gs_color_space *)&pcs->params.device_n.alt_space;

	return (*pacs->type->remap_concrete_color)
				(pconc, pacs, pdc, pis, dev, select);
    }
    else {
	gx_remap_concrete_devicen(pconc, pdc, pis, dev, select);
	return 0;
    }
}

/*
 * Check that the color component names for a DeviceN color space
 * match the device colorant names.  Also build a gs_devicen_color_map
 * structure.
 */
private int
check_DeviceN_component_names(const gs_color_space * pcs, gs_state * pgs)
{
    const gs_separation_name *names = pcs->params.device_n.names;
    int num_comp = pcs->params.device_n.num_components;
    int i, j;
    int colorant_number;
    byte * pname;
    uint name_size;
    gs_devicen_color_map * pcolor_component_map
	= &pgs->color_component_map;
    gx_device * dev = pgs->device;
    const char none_str[] = "None";
    const uint none_size = strlen(none_str);
    bool non_match = false;

    pcolor_component_map->num_components = num_comp;
    pcolor_component_map->cspace_id = pcs->id;
    pcolor_component_map->num_colorants = dev->color_info.num_components;
    pcolor_component_map->sep_type = SEP_OTHER;
    /*
     * Always use the alternate color space if the current device is
     * using an additive color model.
     */
    if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE) {
	pcolor_component_map->use_alt_cspace = true;
	return 0;
    }
    /*
     * Now check the names of the color components.
     */
    non_match = false;
    for(i = 0; i < num_comp; i++ ) {
	/*
	 * Get the character string and length for the component name.
	 */
	pcs->params.device_n.get_colorname_string(dev->memory, names[i], &pname, &name_size);
	/*
         * Postscript does not accept /None as a color component but it is
         * allowed in PDF so we accept it.  It is also accepted as a
	 * separation name.
         */
	if (name_size == none_size &&
	        (strncmp(none_str, (const char *)pname, name_size) == 0)) {
	    pcolor_component_map->color_map[i] = -1;
	}
	else {
 	    /*
	     * Check for duplicated names.  Except for /None, no components
	     * are allowed to have duplicated names.
	     */
	    for (j = 0; j < i; j++) {
	        if (names[i] == names[j])
		    return_error(gs_error_rangecheck);
            }
	    /*
	     * Compare the colorant name to the device's.  If the device's
	     * compare routine returns GX_DEVICE_COLOR_MAX_COMPONENTS then the
	     * colorant is in the SeparationNames list but not in the
	     * SeparationOrder list.
	     */
	    colorant_number = (*dev_proc(dev, get_color_comp_index))
		    (dev, (const char *)pname, name_size, SEPARATION_NAME);
	    if (colorant_number >= 0) {		/* If valid colorant name */
		pcolor_component_map->color_map[i] =
		    (colorant_number == GX_DEVICE_COLOR_MAX_COMPONENTS) ? -1
		    					   : colorant_number;
	    }
	    else
		non_match = true;
        }	
    }
    pcolor_component_map->use_alt_cspace = non_match;
    return 0;
}

/* Install a DeviceN color space. */
private int
gx_install_DeviceN(const gs_color_space * pcs, gs_state * pgs)
{
    int code = check_DeviceN_component_names(pcs, pgs);

    if (code < 0)
       return code;
    pgs->color_space->params.device_n.use_alt_cspace =
	using_alt_color_space(pgs);
    if (pgs->color_space->params.device_n.use_alt_cspace)
        code = (*pcs->params.device_n.alt_space.type->install_cspace)
	((const gs_color_space *) & pcs->params.device_n.alt_space, pgs);
    /*
     * Give the device an opportunity to capture equivalent colors for any
     * spot colors which might be present in the color space.
     */
    if (code >= 0)
        code = dev_proc(pgs->device, update_spot_equivalent_colors)
							(pgs->device, pgs);
    return code;
}

/* Set overprint information for a DeviceN color space */
private int
gx_set_overprint_DeviceN(const gs_color_space * pcs, gs_state * pgs)
{
    gs_devicen_color_map *  pcmap = &pgs->color_component_map;

    if (pcmap->use_alt_cspace)
        return gx_spot_colors_set_overprint( 
                   (const gs_color_space *)&pcs->params.device_n.alt_space,
                   pgs );
    else {
        gs_overprint_params_t   params;

        if ((params.retain_any_comps = pgs->overprint)) {
            int     i, ncomps = pcs->params.device_n.num_components;

            params.retain_spot_comps = false;
            params.drawn_comps = 0;
            for (i = 0; i < ncomps; i++) {
                int     mcomp = pcmap->color_map[i];

                if (mcomp >= 0)
		    gs_overprint_set_drawn_comp( params.drawn_comps, mcomp);
            }
        }

        pgs->effective_overprint_mode = 0;
        return gs_state_update_overprint(pgs, &params);
    }
}

/* Adjust the reference count of a DeviceN color space. */
private void
gx_adjust_cspace_DeviceN(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.device_n.map, delta, "gx_adjust_DeviceN");
    (*pcs->params.device_n.alt_space.type->adjust_cspace_count)
	((const gs_color_space *)&pcs->params.device_n.alt_space, delta);
}

/* ---------------- Serialization. -------------------------------- */

int 
gx_serialize_device_n_map(const gs_color_space * pcs, gs_device_n_map * m, stream * s)
{
    const gs_function_t *pfn;

    if (m->tint_transform != map_devn_using_function)
	return_error(gs_error_unregistered); /* Unimplemented. */
    pfn = (const gs_function_t *)m->tint_transform_data;
    return gs_function_serialize(pfn, s);
}

private int 
gx_serialize_DeviceN(const gs_color_space * pcs, stream * s)
{
    const gs_device_n_params * p = &pcs->params.device_n;
    uint n;
    int code = gx_serialize_cspace_type(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->num_components, sizeof(p->num_components), &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->names[0], sizeof(p->names[0]) * p->num_components, &n);
    if (code < 0)
	return code;
    code = cs_serialize((const gs_color_space *)&p->alt_space, s);
    if (code < 0)
	return code;
    return gx_serialize_device_n_map(pcs, p->map, s);
    /* p->use_alt_cspace isn't a property of the space. */
}
