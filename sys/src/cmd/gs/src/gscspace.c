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

/*$Id: gscspace.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Color space operators and support */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsccolor.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxcmap.h"
#include "gxcspace.h"
#include "gxistate.h"

/*
 * Define the standard color space types.  We include DeviceCMYK in the base
 * build because it's too awkward to omit it, but we don't provide any of
 * the PostScript operator procedures (setcmykcolor, etc.) for dealing with
 * it.
 */
private const gs_color_space_type gs_color_space_type_DeviceGray = {
    gs_color_space_index_DeviceGray, true, true,
    &st_base_color_space, gx_num_components_1,
    gx_no_base_space,
    gx_init_paint_1, gx_restrict01_paint_1,
    gx_same_concrete_space,
    gx_concretize_DeviceGray, gx_remap_concrete_DGray,
    gx_remap_DeviceGray, gx_no_install_cspace,
    gx_no_adjust_cspace_count, gx_no_adjust_color_count
};
private const gs_color_space_type gs_color_space_type_DeviceRGB = {
    gs_color_space_index_DeviceRGB, true, true,
    &st_base_color_space, gx_num_components_3,
    gx_no_base_space,
    gx_init_paint_3, gx_restrict01_paint_3,
    gx_same_concrete_space,
    gx_concretize_DeviceRGB, gx_remap_concrete_DRGB,
    gx_remap_DeviceRGB, gx_no_install_cspace,
    gx_no_adjust_cspace_count, gx_no_adjust_color_count
};
private const gs_color_space_type gs_color_space_type_DeviceCMYK = {
    gs_color_space_index_DeviceCMYK, true, true,
    &st_base_color_space, gx_num_components_4,
    gx_no_base_space,
    gx_init_paint_4, gx_restrict01_paint_4,
    gx_same_concrete_space,
    gx_concretize_DeviceCMYK, gx_remap_concrete_DCMYK,
    gx_remap_DeviceCMYK, gx_no_install_cspace,
    gx_no_adjust_cspace_count, gx_no_adjust_color_count
};

/* Structure descriptors */
public_st_color_space();
public_st_base_color_space();

/* Return the shared instances of the color spaces. */
const gs_color_space *
gs_cspace_DeviceGray(const gs_imager_state * pis)
{
    return pis->shared->device_color_spaces.named.Gray;
}
const gs_color_space *
gs_cspace_DeviceRGB(const gs_imager_state * pis)
{
    return pis->shared->device_color_spaces.named.RGB;
}
const gs_color_space *
gs_cspace_DeviceCMYK(const gs_imager_state * pis)
{
    return pis->shared->device_color_spaces.named.CMYK;
}

/* ------ Create/copy/destroy ------ */

void
gs_cspace_init(gs_color_space *pcs, const gs_color_space_type * pcstype,
	       gs_memory_t *mem)
{
    pcs->type = pcstype;
    pcs->pmem = mem;
    pcs->id = gs_next_ids(1);
}

int
gs_cspace_alloc(gs_color_space ** ppcspace,
		const gs_color_space_type * pcstype,
		gs_memory_t * mem)
{
    gs_color_space *pcspace =
	gs_alloc_struct(mem, gs_color_space, &st_color_space,
			"gs_cspace_alloc");

    if (pcspace == 0)
	return_error(gs_error_VMerror);
    gs_cspace_init(pcspace, pcstype, mem);
    *ppcspace = pcspace;
    return 0;
}

int
gs_cspace_init_DeviceGray(gs_color_space *pcs)
{
    gs_cspace_init(pcs, &gs_color_space_type_DeviceGray, NULL);
    return 0;
}
int
gs_cspace_build_DeviceGray(gs_color_space ** ppcspace, gs_memory_t * pmem)
{
    return gs_cspace_alloc(ppcspace, &gs_color_space_type_DeviceGray, pmem);
}

int
gs_cspace_init_DeviceRGB(gs_color_space *pcs)
{
    gs_cspace_init(pcs, &gs_color_space_type_DeviceRGB, NULL);
    return 0;
}
int
gs_cspace_build_DeviceRGB(gs_color_space ** ppcspace, gs_memory_t * pmem)
{
    return gs_cspace_alloc(ppcspace, &gs_color_space_type_DeviceRGB, pmem);
}

int
gs_cspace_init_DeviceCMYK(gs_color_space *pcs)
{
    gs_cspace_init(pcs, &gs_color_space_type_DeviceCMYK, NULL);
    return 0;
}
int
gs_cspace_build_DeviceCMYK(gs_color_space ** ppcspace, gs_memory_t * pmem)
{
    return gs_cspace_alloc(ppcspace, &gs_color_space_type_DeviceCMYK, pmem);
}

/*
 * Copy just enough of a color space object.  This will do the right thing
 * for copying color spaces into the base or alternate color space of a
 * compound color space when legal, but it can't check that the operation is
 * actually legal.
 */
inline private void
cs_copy(gs_color_space *pcsto, const gs_color_space *pcsfrom)
{
    memcpy(pcsto, pcsfrom, pcsfrom->type->stype->ssize);
}

/* Copy a color space into one newly allocated by the caller. */
void
gs_cspace_init_from(gs_color_space * pcsto, const gs_color_space * pcsfrom)
{
    cs_copy(pcsto, pcsfrom);
    (*pcsto->type->adjust_cspace_count)(pcsto, 1);
}

/* Assign a color space into a previously initialized one. */
void
gs_cspace_assign(gs_color_space * pdest, const gs_color_space * psrc)
{
    /* check for a = a */
    if (pdest == psrc)
	return;
    (*psrc->type->adjust_cspace_count)(psrc, 1);
    (*pdest->type->adjust_cspace_count)(pdest, -1);
    cs_copy(pdest, psrc);
}


/* Prepare to free a color space. */
void
gs_cspace_release(gs_color_space * pcs)
{
    (*pcs->type->adjust_cspace_count)(pcs, -1);
}

/* ------ Accessors ------ */

/* Get the index of a color space. */
gs_color_space_index
gs_color_space_get_index(const gs_color_space * pcs)
{
    return pcs->type->index;
}

/* Get the number of components in a color space. */
int
gs_color_space_num_components(const gs_color_space * pcs)
{
    return cs_num_components(pcs);
}

int
gx_num_components_1(const gs_color_space * pcs)
{
    return 1;
}
int
gx_num_components_3(const gs_color_space * pcs)
{
    return 3;
}
int
gx_num_components_4(const gs_color_space * pcs)
{
    return 4;
}

/*
 * For color spaces that have a base or alternative color space, return that
 * color space. Otherwise return null.
 */
const gs_color_space *
gs_cspace_base_space(const gs_color_space * pcspace)
{
    return cs_base_space(pcspace);
}

const gs_color_space *
gx_no_base_space(const gs_color_space * pcspace)
{
    return NULL;
}

/* ------ Other implementation procedures ------ */

/* Null color space installation procedure. */
int
gx_no_install_cspace(const gs_color_space * pcs, gs_state * pgs)
{
    return 0;
}

/* Null reference count adjustment procedure. */
void
gx_no_adjust_cspace_count(const gs_color_space * pcs, int delta)
{
}

/* GC procedures */

private 
ENUM_PTRS_BEGIN_PROC(color_space_enum_ptrs)
{
    EV_CONST gs_color_space *pcs = vptr;

    return ENUM_USING(*pcs->type->stype, vptr, size, index);
    ENUM_PTRS_END_PROC
}
private 
RELOC_PTRS_WITH(color_space_reloc_ptrs, gs_color_space *pcs)
{
    RELOC_USING(*pcs->type->stype, vptr, size);
}
RELOC_PTRS_END
