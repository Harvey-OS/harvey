/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsistate.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Imager state housekeeping */
#include "gx.h"
#include "gserrors.h"
#include "gscspace.h"
#include "gscie.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxbitmap.h"
#include "gxcmap.h"
#include "gxdht.h"
#include "gxistate.h"
#include "gzht.h"
#include "gzline.h"

/******************************************************************************
 * See gsstate.c for a discussion of graphics/imager state memory management. *
 ******************************************************************************/

/* Imported values */
/* The following should include a 'const', but for some reason */
/* the Watcom compiler won't accept it, even though it happily accepts */
/* the same construct everywhere else. */
extern /*const*/ gx_color_map_procs *const cmap_procs_default;

/* Structure descriptors */
private_st_imager_state_shared();

/* GC procedures for gx_line_params */
private
ENUM_PTRS_WITH(line_params_enum_ptrs, gx_line_params *plp) return 0;
    case 0: return ENUM_OBJ((plp->dash.pattern_size == 0 ?
			     NULL : plp->dash.pattern));
ENUM_PTRS_END
private RELOC_PTRS_WITH(line_params_reloc_ptrs, gx_line_params *plp)
{
    if (plp->dash.pattern_size)
	RELOC_VAR(plp->dash.pattern);
} RELOC_PTRS_END
private_st_line_params();

/* GC procedures for gs_imager_state */
public_st_imager_state();
private 
ENUM_PTRS_BEGIN(imager_state_enum_ptrs)
    ENUM_SUPER(gs_imager_state, st_line_params, line_params, st_imager_state_num_ptrs - st_line_params_num_ptrs);
    ENUM_PTR(0, gs_imager_state, shared);
    ENUM_PTR(1, gs_imager_state, client_data);
#define E1(i,elt) ENUM_PTR(i+2,gs_imager_state,elt);
    gs_cr_state_do_ptrs(E1)
#undef E1
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(imager_state_reloc_ptrs)
{
    RELOC_SUPER(gs_imager_state, st_line_params, line_params);
    RELOC_PTR(gs_imager_state, shared);
    RELOC_PTR(gs_imager_state, client_data);
#define R1(i,elt) RELOC_PTR(gs_imager_state,elt);
    gs_cr_state_do_ptrs(R1)
#undef R1
} RELOC_PTRS_END

/* Free device color spaces. */
void
gx_device_color_spaces_free(gx_device_color_spaces_t *pdcs, gs_memory_t *mem,
			    client_name_t cname)
{
    int i;

    for (i = countof(pdcs->indexed); --i >= 0; ) {
	gs_color_space *pcs = pdcs->indexed[i];

	if (pcs) {
	    gs_cspace_release(pcs);
	    gs_free_object(mem, pcs, cname);
	}
    }
}

/* Initialize an imager state, other than the parts covered by */
/* gs_imager_state_initial. */
private float
imager_null_transfer(floatp gray, const gx_transfer_map * pmap)
{
    return gray;
}
private void
rc_free_imager_shared(gs_memory_t * mem, void *data, client_name_t cname)
{
    gs_imager_state_shared_t * const shared =
	(gs_imager_state_shared_t *)data;

    gx_device_color_spaces_free(&shared->device_color_spaces, mem,
				"shared device color space");
    rc_free_struct_only(mem, data, cname);
}

int
gs_imager_state_initialize(gs_imager_state * pis, gs_memory_t * mem)
{
    pis->memory = mem;
    pis->client_data = 0;
    /* Preallocate color spaces. */
    {
	int code;
	gs_imager_state_shared_t *shared;

	rc_alloc_struct_1(shared, gs_imager_state_shared_t,
			  &st_imager_state_shared, mem,
			  return_error(gs_error_VMerror),
			  "gs_imager_state_init(shared)");
	shared->device_color_spaces.named.Gray =
	    shared->device_color_spaces.named.RGB =
	    shared->device_color_spaces.named.CMYK = 0; /* in case we bail out */
	shared->rc.free = rc_free_imager_shared;
	if ((code = gs_cspace_build_DeviceGray(&shared->device_color_spaces.named.Gray, mem)) < 0 ||
	    (code = gs_cspace_build_DeviceRGB(&shared->device_color_spaces.named.RGB, mem)) < 0 ||
	    (code = gs_cspace_build_DeviceCMYK(&shared->device_color_spaces.named.CMYK, mem)) < 0
	    ) {
	    rc_free_imager_shared(mem, shared, "gs_imager_state_init(shared)");
	    return code;
	}
	pis->shared = shared;
    }
    pis->halftone = 0;
    {
	int i;

	for (i = 0; i < gs_color_select_count; ++i)
	    pis->screen_phase[i].x = pis->screen_phase[i].y = 0;
    }
    pis->dev_ht = 0;
    pis->ht_cache = 0;
    pis->cie_render = 0;
    pis->black_generation = 0;
    pis->undercolor_removal = 0;
    /* Allocate an initial transfer map. */
    rc_alloc_struct_n(pis->set_transfer.colored.gray,
		      gx_transfer_map, &st_transfer_map,
		      mem, return_error(gs_error_VMerror),
		      "gs_imager_state_init(transfer)", 4);
    pis->set_transfer.colored.gray->proc = imager_null_transfer;
    pis->set_transfer.colored.gray->id = gs_next_ids(1);
    pis->set_transfer.colored.gray->values[0] = frac_0;
    pis->set_transfer.colored.red =
	pis->set_transfer.colored.green =
	pis->set_transfer.colored.blue =
	pis->set_transfer.colored.gray;
    pis->effective_transfer = pis->set_transfer;
    pis->cie_joint_caches = 0;
    pis->cmap_procs = cmap_procs_default;
    pis->pattern_cache = 0;
    return 0;
}

/*
 * Make a temporary copy of a gs_imager_state.  Note that this does not
 * do all the necessary reference counting, etc.
 */
gs_imager_state *
gs_imager_state_copy(const gs_imager_state * pis, gs_memory_t * mem)
{
    gs_imager_state *pis_copy =
	gs_alloc_struct(mem, gs_imager_state, &st_imager_state,
			"gs_imager_state_copy");

    if (pis_copy)
	*pis_copy = *pis;
    return pis_copy;
}

/* Increment reference counts to note that an imager state has been copied. */
void
gs_imager_state_copied(gs_imager_state * pis)
{
    rc_increment(pis->shared);
    rc_increment(pis->halftone);
    rc_increment(pis->dev_ht);
    rc_increment(pis->cie_render);
    rc_increment(pis->black_generation);
    rc_increment(pis->undercolor_removal);
    rc_increment(pis->set_transfer.colored.gray);
    rc_increment(pis->set_transfer.colored.red);
    rc_increment(pis->set_transfer.colored.green);
    rc_increment(pis->set_transfer.colored.blue);
    rc_increment(pis->cie_joint_caches);
}

/* Adjust reference counts before assigning one imager state to another. */
void
gs_imager_state_pre_assign(gs_imager_state *pto, const gs_imager_state *pfrom)
{
    const char *const cname = "gs_imager_state_pre_assign";

#define RCCOPY(element)\
    rc_pre_assign(pto->element, pfrom->element, cname)

    RCCOPY(cie_joint_caches);
    RCCOPY(set_transfer.colored.blue);
    RCCOPY(set_transfer.colored.green);
    RCCOPY(set_transfer.colored.red);
    RCCOPY(set_transfer.colored.gray);
    RCCOPY(undercolor_removal);
    RCCOPY(black_generation);
    RCCOPY(cie_render);
    RCCOPY(dev_ht);
    RCCOPY(halftone);
    RCCOPY(shared);
#undef RCCOPY
}

/* Release an imager state. */
void
gs_imager_state_release(gs_imager_state * pis)
{
    const char *const cname = "gs_imager_state_release";
    gx_device_halftone *pdht = pis->dev_ht;

#define RCDECR(element)\
    rc_decrement(pis->element, cname)

    RCDECR(cie_joint_caches);
    RCDECR(set_transfer.colored.gray);
    RCDECR(set_transfer.colored.blue);
    RCDECR(set_transfer.colored.green);
    RCDECR(set_transfer.colored.red);
    RCDECR(undercolor_removal);
    RCDECR(black_generation);
    RCDECR(cie_render);
    /*
     * If we're going to free the device halftone, make sure we free the
     * dependent structures as well.
     */
    if (pdht != 0 && pdht->rc.ref_count == 1) {
	/* Make sure we don't leave dangling pointers in the cache. */
	gx_ht_cache *pcache = pis->ht_cache;

	if (pcache->order.bit_data == pdht->order.bit_data ||
	    pcache->order.levels == pdht->order.levels
	    )
	    gx_ht_clear_cache(pcache);
	gx_device_halftone_release(pdht, pdht->rc.memory);
    }
    RCDECR(dev_ht);
    RCDECR(halftone);
    RCDECR(shared);
#undef RCDECR
}
