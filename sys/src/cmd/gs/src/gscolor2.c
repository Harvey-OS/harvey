/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscolor2.c,v 1.20 2004/04/08 07:59:19 igor Exp $ */
/* Level 2 color operators for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxarith.h"
#include "gxfixed.h"		/* ditto */
#include "gxmatrix.h"		/* for gzstate.h */
#include "gxcspace.h"		/* for gscolor2.h */
#include "gxcolor2.h"
#include "gzstate.h"
#include "gxpcolor.h"
#include "stream.h"

/* ---------------- General colors and color spaces ---------------- */

/* setcolorspace */
int
gs_setcolorspace(gs_state * pgs, const gs_color_space * pcs)
{
    int             code = 0;
    gs_color_space  cs_old = *pgs->color_space;
    gs_client_color cc_old = *pgs->ccolor;

    if (pgs->in_cachedevice)
	return_error(gs_error_undefined);

    if (pcs->id != pgs->color_space->id) {
        pcs->type->adjust_cspace_count(pcs, 1);
        *pgs->color_space = *pcs;
        if ( (code = pcs->type->install_cspace(pcs, pgs)) < 0          ||
              (pgs->overprint && (code = gs_do_set_overprint(pgs)) < 0)  ) {
            *pgs->color_space = cs_old;
            pcs->type->adjust_cspace_count(pcs, -1);
        } else
            cs_old.type->adjust_cspace_count(&cs_old, -1);
    }

    if (code >= 0) {
        cs_full_init_color(pgs->ccolor, pcs);
        cs_old.type->adjust_color_count(&cc_old, &cs_old, -1);
        gx_unset_dev_color(pgs);
    }

    return code;
}

/* currentcolorspace */
const gs_color_space *
gs_currentcolorspace(const gs_state * pgs)
{
    return pgs->color_space;
}

/* setcolor */
int
gs_setcolor(gs_state * pgs, const gs_client_color * pcc)
{
    gs_color_space *    pcs = pgs->color_space;
    gs_client_color     cc_old = *pgs->ccolor;

   if (pgs->in_cachedevice)
	return_error(gs_error_undefined); /* PLRM3 page 215. */
    gx_unset_dev_color(pgs);
    (*pcs->type->adjust_color_count)(pcc, pcs, 1);
    *pgs->ccolor = *pcc;
    (*pcs->type->restrict_color)(pgs->ccolor, pcs);
    (*pcs->type->adjust_color_count)(&cc_old, pcs, -1);

    return 0;
}

/* currentcolor */
const gs_client_color *
gs_currentcolor(const gs_state * pgs)
{
    return pgs->ccolor;
}

/* ------ Internal procedures ------ */

/* GC descriptors */
private_st_indexed_map();

/* Define a lookup_index procedure that just returns the map values. */
int
lookup_indexed_map(const gs_indexed_params * params, int index, float *values)
{
    int m = cs_num_components((const gs_color_space *)&params->base_space);
    const float *pv = &params->lookup.map->values[index * m];

    memcpy(values, pv, sizeof(*values) * m);
    return 0;
}

/* Free an indexed map and its values when the reference count goes to 0. */
void
free_indexed_map(gs_memory_t * pmem, void *pmap, client_name_t cname)
{
    gs_free_object(pmem, ((gs_indexed_map *) pmap)->values, cname);
    gs_free_object(pmem, pmap, cname);
}

/*
 * Allocate an indexed map for an Indexed or Separation color space.
 */
int
alloc_indexed_map(gs_indexed_map ** ppmap, int nvals, gs_memory_t * pmem,
		  client_name_t cname)
{
    gs_indexed_map *pimap;

    rc_alloc_struct_1(pimap, gs_indexed_map, &st_indexed_map, pmem,
		      return_error(gs_error_VMerror), cname);
    if (nvals > 0) {
	pimap->values =
	    (float *)gs_alloc_byte_array(pmem, nvals, sizeof(float), cname);

	if (pimap->values == 0) {
	    gs_free_object(pmem, pimap, cname);
	    return_error(gs_error_VMerror);
	}
    } else
	pimap->values = 0;
    pimap->rc.free = free_indexed_map;
    pimap->proc_data = 0;	/* for GC */
    pimap->num_values = nvals;
    *ppmap = pimap;
    return 0;
}

/* ---------------- Indexed color spaces ---------------- */

gs_private_st_composite(st_color_space_Indexed, gs_paint_color_space,
     "gs_color_space_Indexed", cs_Indexed_enum_ptrs, cs_Indexed_reloc_ptrs);

/* ------ Color space ------ */

/* Define the Indexed color space type. */
private cs_proc_base_space(gx_base_space_Indexed);
private cs_proc_restrict_color(gx_restrict_Indexed);
private cs_proc_concrete_space(gx_concrete_space_Indexed);
private cs_proc_concretize_color(gx_concretize_Indexed);
private cs_proc_install_cspace(gx_install_Indexed);
private cs_proc_set_overprint(gx_set_overprint_Indexed);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_Indexed);
private cs_proc_serialize(gx_serialize_Indexed);
const gs_color_space_type gs_color_space_type_Indexed = {
    gs_color_space_index_Indexed, false, false,
    &st_color_space_Indexed, gx_num_components_1,
    gx_base_space_Indexed,
    gx_init_paint_1, gx_restrict_Indexed,
    gx_concrete_space_Indexed,
    gx_concretize_Indexed, NULL,
    gx_default_remap_color, gx_install_Indexed,
    gx_set_overprint_Indexed,
    gx_adjust_cspace_Indexed, gx_no_adjust_color_count,
    gx_serialize_Indexed,
    gx_cspace_is_linear_default
};

/* GC procedures. */

private uint
indexed_table_size(const gs_color_space *pcs)
{
    return (pcs->params.indexed.hival + 1) *
	cs_num_components((const gs_color_space *)
			  &pcs->params.indexed.base_space);

}
private 
ENUM_PTRS_WITH(cs_Indexed_enum_ptrs, gs_color_space *pcs)
{
    return ENUM_USING(*pcs->params.indexed.base_space.type->stype,
		      &pcs->params.indexed.base_space,
		      sizeof(pcs->params.indexed.base_space), index - 1);
}
case 0:
if (pcs->params.indexed.use_proc)
    ENUM_RETURN((void *)pcs->params.indexed.lookup.map);
else
    return ENUM_CONST_STRING2(pcs->params.indexed.lookup.table.data,
			      indexed_table_size(pcs));
ENUM_PTRS_END
private RELOC_PTRS_WITH(cs_Indexed_reloc_ptrs, gs_color_space *pcs)
{
    RELOC_USING(*pcs->params.indexed.base_space.type->stype,
		&pcs->params.indexed.base_space,
		sizeof(gs_base_color_space));
    if (pcs->params.indexed.use_proc)
	RELOC_PTR(gs_color_space, params.indexed.lookup.map);
    else {
	gs_const_string table;

	table.data = pcs->params.indexed.lookup.table.data;
	table.size = indexed_table_size(pcs);
	RELOC_CONST_STRING_VAR(table);
	pcs->params.indexed.lookup.table.data = table.data;
    }
}
RELOC_PTRS_END

/* Return the base space of an Indexed color space. */
private const gs_color_space *
gx_base_space_Indexed(const gs_color_space * pcs)
{
    return (const gs_color_space *)&(pcs->params.indexed.base_space);
}


/* Color space installation ditto. */

private int
gx_install_Indexed(const gs_color_space * pcs, gs_state * pgs)
{
    return (*pcs->params.indexed.base_space.type->install_cspace)
	((const gs_color_space *) & pcs->params.indexed.base_space, pgs);
}

/* Color space overprint setting ditto. */

private int
gx_set_overprint_Indexed(const gs_color_space * pcs, gs_state * pgs)
{
    return (*pcs->params.indexed.base_space.type->set_overprint)
	((const gs_color_space *)&pcs->params.indexed.base_space, pgs);
}

/* Color space reference count adjustment ditto. */

private void
gx_adjust_cspace_Indexed(const gs_color_space * pcs, int delta)
{
    if (pcs->params.indexed.use_proc) {
	rc_adjust_const(pcs->params.indexed.lookup.map, delta,
			"gx_adjust_Indexed");
    }
    (*pcs->params.indexed.base_space.type->adjust_cspace_count)
	((const gs_color_space *)&pcs->params.indexed.base_space, delta);
}

/*
 * Default palette mapping functions for indexed color maps. These just
 * return the values already in the palette.
 *
 * For performance reasons, we provide four functions: special cases for 1,
 * 3, and 4 entry palettes, and a general case. Note that these procedures
 * do not range-check their input values.
 */
private int
map_palette_entry_1(const gs_indexed_params * params, int indx, float *values)
{
    values[0] = params->lookup.map->values[indx];
    return 0;
}

private int
map_palette_entry_3(const gs_indexed_params * params, int indx, float *values)
{
    const float *pv = &(params->lookup.map->values[3 * indx]);

    values[0] = pv[0];
    values[1] = pv[1];
    values[2] = pv[2];
    return 0;
}

private int
map_palette_entry_4(const gs_indexed_params * params, int indx, float *values)
{
    const float *pv = &(params->lookup.map->values[4 * indx]);

    values[0] = pv[0];
    values[1] = pv[1];
    values[2] = pv[2];
    values[3] = pv[3];
    return 0;
}

private int
map_palette_entry_n(const gs_indexed_params * params, int indx, float *values)
{
    int m = cs_num_components((const gs_color_space *)&params->base_space);

    memcpy((void *)values,
	   (const void *)(params->lookup.map->values + indx * m),
	   m * sizeof(float)
    );

    return 0;
}

/*
 * Allocate an indexed map to be used as a palette for indexed color space.
 */
private gs_indexed_map *
alloc_indexed_palette(
			 const gs_color_space * pbase_cspace,
			 int nvals,
			 gs_memory_t * pmem
)
{
    int num_comps = gs_color_space_num_components(pbase_cspace);
    gs_indexed_map *pimap;
    int code =
    alloc_indexed_map(&pimap, nvals * num_comps, pmem,
		      "alloc_indexed_palette");

    if (code < 0)
	return 0;
    if (num_comps == 1)
	pimap->proc.lookup_index = map_palette_entry_1;
    else if (num_comps == 3)
	pimap->proc.lookup_index = map_palette_entry_3;
    else if (num_comps == 4)
	pimap->proc.lookup_index = map_palette_entry_4;
    else
	pimap->proc.lookup_index = map_palette_entry_n;
    return pimap;
}

/*
 * Build an indexed color space.
 */
int
gs_cspace_build_Indexed(
			   gs_color_space ** ppcspace,
			   const gs_color_space * pbase_cspace,
			   uint num_entries,
			   const gs_const_string * ptbl,
			   gs_memory_t * pmem
)
{
    gs_color_space *pcspace = 0;
    gs_indexed_params *pindexed = 0;
    int code;

    if ((pbase_cspace == 0) || !pbase_cspace->type->can_be_base_space)
	return_error(gs_error_rangecheck);

    code = gs_cspace_alloc(&pcspace, &gs_color_space_type_Indexed, pmem);
    if (code < 0)
	return code;
    pindexed = &(pcspace->params.indexed);
    if (ptbl == 0) {
	pindexed->lookup.map =
	    alloc_indexed_palette(pbase_cspace, num_entries, pmem);
	if (pindexed->lookup.map == 0) {
	    gs_free_object(pmem, pcspace, "gs_cspace_build_Indexed");
	    return_error(gs_error_VMerror);
	}
	pindexed->use_proc = true;
    } else {
	pindexed->lookup.table = *ptbl;
	pindexed->use_proc = false;
    }
    gs_cspace_init_from((gs_color_space *) & (pindexed->base_space),
			pbase_cspace);
    pindexed->hival = num_entries - 1;
    *ppcspace = pcspace;
    return 0;
}

/*
 * Return the number of entries in an indexed color space.
 */
int
gs_cspace_indexed_num_entries(const gs_color_space * pcspace)
{
    if (gs_color_space_get_index(pcspace) != gs_color_space_index_Indexed)
	return 0;
    return pcspace->params.indexed.hival + 1;
}

/*
 * Get the palette for an indexed color space. This will return a null
 * pointer if the color space is not an indexed color space or if the
 * color space does not use the mapped index palette.
 */
float *
gs_cspace_indexed_value_array(const gs_color_space * pcspace)
{
    if ((gs_color_space_get_index(pcspace) != gs_color_space_index_Indexed) ||
	pcspace->params.indexed.use_proc
	)
	return 0;
    return pcspace->params.indexed.lookup.map->values;
}

/*
 * Set the lookup procedure to be used with an indexed color space.
 */
int
gs_cspace_indexed_set_proc(
			   gs_color_space * pcspace,
			   int (*proc)(const gs_indexed_params *, int, float *)
)
{
    if ((gs_color_space_get_index(pcspace) != gs_color_space_index_Indexed) ||
	!pcspace->params.indexed.use_proc
	)
	return_error(gs_error_rangecheck);
    pcspace->params.indexed.lookup.map->proc.lookup_index = proc;
    return 0;
}

/* ------ Colors ------ */

/* Force an Indexed color into legal range. */
private void
gx_restrict_Indexed(gs_client_color * pcc, const gs_color_space * pcs)
{
    float value = pcc->paint.values[0];

    pcc->paint.values[0] =
	(is_fneg(value) ? 0 :
	 value >= pcs->params.indexed.hival ? pcs->params.indexed.hival :
	 value);
}

/* Color remapping for Indexed color spaces. */
private const gs_color_space *
gx_concrete_space_Indexed(const gs_color_space * pcs,
			  const gs_imager_state * pis)
{
    const gs_color_space *pbcs =
    (const gs_color_space *)&pcs->params.indexed.base_space;

    return cs_concrete_space(pbcs, pis);
}

private int
gx_concretize_Indexed(const gs_client_color * pc, const gs_color_space * pcs,
		      frac * pconc, const gs_imager_state * pis)
{
    float value = pc->paint.values[0];
    int index =
	(is_fneg(value) ? 0 :
	 value >= pcs->params.indexed.hival ? pcs->params.indexed.hival :
	 (int)value);
    const gs_color_space *pbcs =
	(const gs_color_space *)&pcs->params.indexed.base_space;
    gs_client_color cc;
    int code = gs_cspace_indexed_lookup(&pcs->params.indexed, index, &cc);

    if (code < 0)
	return code;
    return (*pbcs->type->concretize_color) (&cc, pbcs, pconc, pis);
}

/* Look up an index in an Indexed color space. */
int
gs_cspace_indexed_lookup(const gs_indexed_params *pip, int index,
			 gs_client_color *pcc)
{
    if (pip->use_proc) {
	return pip->lookup.map->proc.lookup_index
	    (pip, index, &pcc->paint.values[0]);
    } else {
	const gs_color_space *pbcs = (const gs_color_space *)&pip->base_space;
	int m = cs_num_components(pbcs);
	const byte *pcomp = pip->lookup.table.data + m * index;

	switch (m) {
	    default: {		/* DeviceN */
		int i;

		for (i = 0; i < m; ++i)
		    pcc->paint.values[i] = pcomp[i] * (1.0 / 255.0);
	    }
		break;
	    case 4:
		pcc->paint.values[3] = pcomp[3] * (1.0 / 255.0);
	    case 3:
		pcc->paint.values[2] = pcomp[2] * (1.0 / 255.0);
	    case 2:
		pcc->paint.values[1] = pcomp[1] * (1.0 / 255.0);
	    case 1:
		pcc->paint.values[0] = pcomp[0] * (1.0 / 255.0);
	}
	return 0;
    }
}

/* ---------------- Serialization. -------------------------------- */

private int 
gx_serialize_Indexed(const gs_color_space * pcs, stream * s)
{
    const gs_indexed_params * p = &pcs->params.indexed;
    uint n;
    int code = gx_serialize_cspace_type(pcs, s);

    if (code < 0)
	return code;
    code = cs_serialize((const gs_color_space *)&p->base_space, s);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->hival, sizeof(p->hival), &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->use_proc, sizeof(p->use_proc), &n);
    if (code < 0)
	return code;
    if (p->use_proc) {
	code = sputs(s, (const byte *)&p->lookup.map->num_values, 
		sizeof(p->lookup.map->num_values), &n);
	if (code < 0)
	    return code;
	code = sputs(s, (const byte *)&p->lookup.map->values[0], 
		sizeof(p->lookup.map->values[0]) * p->lookup.map->num_values, &n);
    } else {
	code = sputs(s, (const byte *)&p->lookup.table.size, 
			sizeof(p->lookup.table.size), &n);
	if (code < 0)
	    return code;
	code = sputs(s, p->lookup.table.data, p->lookup.table.size, &n);
    }
    return code;
}

/* ---------------- High level device support -------------------------------- */

/*
 * This special function forces a device to include the current
 * color space into the output. Returns 'rangecheck' if the device can't handle it.
 * The primary reason is to include DefaultGray, DefaultRGB, DefaultCMYK into PDF.
 * Should be called for each page that requires the resource.
 * Redundant calls per page with same cspace id are allowed.
 * Redundant calls per page with different cspace id are are allowed but 
 * highly undesirable.
 * No need to call it with color spaces explicitly referred by the document,
 * because they are included automatically.
 * res_name and name_length passes the resource name.
 */
int
gs_includecolorspace(gs_state * pgs, const byte *res_name, int name_length)
{
    return (*dev_proc(pgs->device, include_color_space))(pgs->device, pgs->color_space, res_name, name_length);
}
