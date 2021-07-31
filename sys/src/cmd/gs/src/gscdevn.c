/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscdevn.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* DeviceN color space and operation definition */
#include "gx.h"
#include "gserrors.h"
#include "gsrefct.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gsstruct.h"
#include "gxcspace.h"
#include "gxcdevn.h"

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
private cs_proc_adjust_cspace_count(gx_adjust_cspace_DeviceN);
const gs_color_space_type gs_color_space_type_DeviceN = {
    gs_color_space_index_DeviceN, true, false,
    &st_color_space_DeviceN, gx_num_components_DeviceN,
    gx_alt_space_DeviceN,
    gx_init_DeviceN, gx_restrict_DeviceN,
    gx_concrete_space_DeviceN,
    gx_concretize_DeviceN, gx_remap_concrete_DeviceN,
    gx_default_remap_color, gx_install_DeviceN,
    gx_adjust_cspace_DeviceN, gx_no_adjust_color_count
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
    return (const gs_color_space *)&(pcs->params.device_n.alt_space);
}

/* Initialize a DeviceN color. */
private void
gx_init_DeviceN(gs_client_color * pcc, const gs_color_space * pcs)
{
    int i;

    for (i = 0; i < pcs->params.device_n.num_components; ++i)
	pcc->paint.values[i] = 1.0;
}

/* Force a DeviceN color into legal range. */
private void
gx_restrict_DeviceN(gs_client_color * pcc, const gs_color_space * pcs)
{
    int i;

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
    /* We don't support concrete DeviceN spaces yet. */
    const gs_color_space *pacs =
	(const gs_color_space *)&pcs->params.device_n.alt_space;

    return cs_concrete_space(pacs, pis);
}

private int
gx_concretize_DeviceN(const gs_client_color * pc, const gs_color_space * pcs,
		      frac * pconc, const gs_imager_state * pis)
{
    int code, tcode;
    gs_client_color cc;
    const gs_color_space *pacs =
	(const gs_color_space *)&pcs->params.device_n.alt_space;
    gs_device_n_map *map = pcs->params.device_n.map;

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
    /*
     * We always map into the alternate color space.  We must preserve
     * tcode for implementing a semi-hack in the interpreter.
     */
    tcode = (*pcs->params.device_n.map->tint_transform)
	(&pcs->params.device_n, pc->paint.values, &cc.paint.values[0],
	 pis, pcs->params.device_n.map->tint_transform_data);
    if (tcode < 0)
	return tcode;
    code = (*pacs->type->concretize_color) (&cc, pacs, pconc, pis);
    return (code < 0 || tcode == 0 ? code : tcode);
}

private int
gx_remap_concrete_DeviceN(const frac * pconc,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
			  gs_color_select_t select)
{
    /* We don't support concrete DeviceN colors yet. */
    return_error(gs_error_rangecheck);
}

/* Install a DeviceN color space. */
private int
gx_install_DeviceN(const gs_color_space * pcs, gs_state * pgs)
{
    /*
     * Give an error if any of the separation names are duplicated.
     * We can't check this any earlier.
     */
    const gs_separation_name *names = pcs->params.device_n.names;
    uint i, j;

    for (i = 1; i < pcs->params.device_n.num_components; ++i)
	for (j = 0; j < i; ++j)
	    if (names[i] == names[j])
		return_error(gs_error_rangecheck);
    return (*pcs->params.device_n.alt_space.type->install_cspace)
	((const gs_color_space *) & pcs->params.device_n.alt_space, pgs);
}

/* Adjust the reference count of a DeviceN color space. */
private void
gx_adjust_cspace_DeviceN(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.device_n.map, delta, "gx_adjust_DeviceN");
    (*pcs->params.device_n.alt_space.type->adjust_cspace_count)
	((const gs_color_space *)&pcs->params.device_n.alt_space, delta);
}

#undef pcs
