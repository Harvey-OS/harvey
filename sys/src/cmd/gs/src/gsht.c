/* Copyright (C) 1989, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsht.c,v 1.23 2005/03/14 18:08:36 dan Exp $ */
/* setscreen operator for Ghostscript library */
#include "memory_.h"
#include "string_.h"
#include <stdlib.h>		/* for qsort */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxarith.h"		/* for igcd */
#include "gzstate.h"
#include "gxdevice.h"		/* for gzht.h */
#include "gzht.h"
#include "gswts.h"

/* Forward declarations */
void gx_set_effective_transfer(gs_state *);

/*
 * *HACK ALERT*
 *
 * Value stored in the width field of a well-tempered screen halftone
 * order, to indicate that the wts field of this order points to the
 * same structure as an earlier order. This is used to suppress
 * multiple realeases of shared wts_screen_t orders.
 * 
 * The width field is available for this purpose at it is nominally
 * unused in a well-tempered screening halftone.
 */
private const ushort    ht_wts_suppress_release = (ushort)(-1);


/* Structure types */
public_st_ht_order();
private_st_ht_order_component();
public_st_ht_order_comp_element();
public_st_halftone();
public_st_device_halftone();

/* GC procedures */

private
ENUM_PTRS_WITH(ht_order_enum_ptrs, gx_ht_order *porder) return 0;
case 0: ENUM_RETURN((porder->data_memory ? porder->levels : 0));
case 1: ENUM_RETURN((porder->data_memory ? porder->bit_data : 0));
case 2: ENUM_RETURN(porder->cache);
case 3: ENUM_RETURN(porder->transfer);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(ht_order_reloc_ptrs, gx_ht_order *porder)
{
    if (porder->data_memory) {
	RELOC_VAR(porder->levels);
	RELOC_VAR(porder->bit_data);
    }
    RELOC_VAR(porder->cache);
    RELOC_VAR(porder->transfer);
}
RELOC_PTRS_END

private 
ENUM_PTRS_WITH(halftone_enum_ptrs, gs_halftone *hptr) return 0;
case 0:
switch (hptr->type)
{
    case ht_type_spot:
	ENUM_RETURN((hptr->params.spot.transfer == 0 ?
		     hptr->params.spot.transfer_closure.data :
		     0));
    case ht_type_threshold:
	ENUM_RETURN_CONST_STRING_PTR(gs_halftone, params.threshold.thresholds);
    case ht_type_threshold2:
	return ENUM_CONST_BYTESTRING(&hptr->params.threshold2.thresholds);
    case ht_type_client_order:
	ENUM_RETURN(hptr->params.client_order.client_data);
    case ht_type_multiple:
    case ht_type_multiple_colorscreen:
	ENUM_RETURN(hptr->params.multiple.components);
    case ht_type_none:
    case ht_type_screen:
    case ht_type_colorscreen:
	return 0;
}
case 1:
switch (hptr->type) {
    case ht_type_threshold:
	ENUM_RETURN((hptr->params.threshold.transfer == 0 ?
		     hptr->params.threshold.transfer_closure.data :
		     0));
    case ht_type_threshold2:
	ENUM_RETURN(hptr->params.threshold2.transfer_closure.data);
    case ht_type_client_order:
	ENUM_RETURN(hptr->params.client_order.transfer_closure.data);
    default:
	return 0;
}
ENUM_PTRS_END

private RELOC_PTRS_WITH(halftone_reloc_ptrs, gs_halftone *hptr)
{
    switch (hptr->type) {
	case ht_type_spot:
	    if (hptr->params.spot.transfer == 0)
		RELOC_PTR(gs_halftone, params.spot.transfer_closure.data);
	    break;
	case ht_type_threshold:
	    RELOC_CONST_STRING_PTR(gs_halftone, params.threshold.thresholds);
	    if (hptr->params.threshold.transfer == 0)
		RELOC_PTR(gs_halftone, params.threshold.transfer_closure.data);
	    break;
	case ht_type_threshold2:
	    RELOC_CONST_BYTESTRING_VAR(hptr->params.threshold2.thresholds);
	    RELOC_OBJ_VAR(hptr->params.threshold2.transfer_closure.data);
	    break;
	case ht_type_client_order:
	    RELOC_PTR(gs_halftone, params.client_order.client_data);
	    RELOC_PTR(gs_halftone, params.client_order.transfer_closure.data);
	    break;
	case ht_type_multiple:
	case ht_type_multiple_colorscreen:
	    RELOC_PTR(gs_halftone, params.multiple.components);
	    break;
	case ht_type_none:
	case ht_type_screen:
	case ht_type_colorscreen:
	    break;
    }
}
RELOC_PTRS_END

/* setscreen */
int
gs_setscreen(gs_state * pgs, gs_screen_halftone * phsp)
{
    gs_screen_enum senum;
    int code = gx_ht_process_screen(&senum, pgs, phsp,
				    gs_currentaccuratescreens());

    if (code < 0)
	return code;
    return gs_screen_install(&senum);
}

/* currentscreen */
int
gs_currentscreen(const gs_state * pgs, gs_screen_halftone * phsp)
{
    switch (pgs->halftone->type) {
	case ht_type_screen:
	    *phsp = pgs->halftone->params.screen;
	    return 0;
	case ht_type_colorscreen:
	    *phsp = pgs->halftone->params.colorscreen.screens.colored.gray;
	    return 0;
	default:
	    return_error(gs_error_undefined);
    }
}

/* .currentscreenlevels */
int
gs_currentscreenlevels(const gs_state * pgs)
{
    int gi = 0;

    if (pgs->device != 0)
        gi = pgs->device->color_info.gray_index;
    if (gi != GX_CINFO_COMP_NO_INDEX)
        return pgs->dev_ht->components[gi].corder.num_levels;
    else
        return pgs->dev_ht->components[0].corder.num_levels;
}

/* .setscreenphase */
int
gx_imager_setscreenphase(gs_imager_state * pis, int x, int y,
			 gs_color_select_t select)
{
    if (select == gs_color_select_all) {
	int i;

	for (i = 0; i < gs_color_select_count; ++i)
	    gx_imager_setscreenphase(pis, x, y, (gs_color_select_t) i);
	return 0;
    } else if (select < 0 || select >= gs_color_select_count)
	return_error(gs_error_rangecheck);
    pis->screen_phase[select].x = x;
    pis->screen_phase[select].y = y;
    return 0;
}
int
gs_setscreenphase(gs_state * pgs, int x, int y, gs_color_select_t select)
{
    int code = gx_imager_setscreenphase((gs_imager_state *) pgs, x, y,
					select);

    /*
     * If we're only setting the source phase, we don't need to do
     * unset_dev_color, because the source phase doesn't affect painting
     * with the current color.
     */
    if (code >= 0 && (select == gs_color_select_texture ||
		      select == gs_color_select_all)
	)
	gx_unset_dev_color(pgs);
    return code;
}

int
gs_currentscreenphase_pis(const gs_imager_state * pis, gs_int_point * pphase,
		      gs_color_select_t select)
{
    if (select < 0 || select >= gs_color_select_count)
	return_error(gs_error_rangecheck);
    *pphase = pis->screen_phase[select];
    return 0;
}

/* .currentscreenphase */
int
gs_currentscreenphase(const gs_state * pgs, gs_int_point * pphase,
		      gs_color_select_t select)
{
    return gs_currentscreenphase_pis((const gs_imager_state *)pgs, pphase, select);
}

/* currenthalftone */
int
gs_currenthalftone(gs_state * pgs, gs_halftone * pht)
{
    *pht = *pgs->halftone;
    return 0;
}

/* ------ Internal routines ------ */

/* Process one screen plane. */
int
gx_ht_process_screen_memory(gs_screen_enum * penum, gs_state * pgs,
		gs_screen_halftone * phsp, bool accurate, gs_memory_t * mem)
{
    gs_point pt;
    int code = gs_screen_init_memory(penum, pgs, phsp, accurate, mem);

    if (code < 0)
	return code;
    while ((code = gs_screen_currentpoint(penum, &pt)) == 0)
	if ((code = gs_screen_next(penum, (*phsp->spot_function) (pt.x, pt.y))) < 0)
	    return code;
    return 0;
}

/*
 * Internal procedure to allocate and initialize either an internally
 * generated or a client-defined halftone order.  For spot halftones,
 * the client is responsible for calling gx_compute_cell_values.
 *
 * Note: this function is used for old-style halftones only. WTS
 * halftones are allocated in gs_sethalftone_try_wts().
 */
int
gx_ht_alloc_ht_order(gx_ht_order * porder, uint width, uint height,
		     uint num_levels, uint num_bits, uint strip_shift,
		     const gx_ht_order_procs_t *procs, gs_memory_t * mem)
{
    porder->wse = NULL;
    porder->wts = NULL;
    porder->width = width;
    porder->height = height;
    porder->raster = bitmap_raster(width);
    porder->shift = strip_shift;
    porder->orig_height = porder->height;
    porder->orig_shift = porder->shift;
    porder->full_height = ht_order_full_height(porder);
    porder->num_levels = num_levels;
    porder->num_bits = num_bits;
    porder->procs = procs;
    porder->data_memory = mem;

    if (num_levels > 0) {
        porder->levels =
	    (uint *)gs_alloc_byte_array(mem, porder->num_levels, sizeof(uint),
				        "alloc_ht_order_data(levels)");
	if (porder->levels == 0)
	    return_error(gs_error_VMerror);
    } else
	porder->levels = 0;

    if (num_bits > 0) {
	porder->bit_data =
	    gs_alloc_byte_array(mem, porder->num_bits,
			        porder->procs->bit_data_elt_size,
			        "alloc_ht_order_data(bit_data)");
	if (porder->bit_data == 0) {
	    gs_free_object(mem, porder->levels, "alloc_ht_order_data(levels)");
	    porder->levels = 0;
	    return_error(gs_error_VMerror);
	}
    } else
	porder->bit_data = 0;

    porder->cache = 0;
    porder->transfer = 0;
    return 0;
}

/*
 * Procedure to copy a halftone order.
 */
private int
gx_ht_copy_ht_order(gx_ht_order * pdest, gx_ht_order * psrc, gs_memory_t * mem)
{
    int code;

    *pdest = *psrc;

    code = gx_ht_alloc_ht_order(pdest, psrc->width, psrc->height,
		     psrc->num_levels, psrc->num_bits, psrc->shift,
		     psrc->procs, mem);
    if (code < 0)
	return code;
    if (pdest->levels != 0)
        memcpy(pdest->levels, psrc->levels, psrc->num_levels * sizeof(uint));
    if (pdest->bit_data != 0)
        memcpy(pdest->bit_data, psrc->bit_data,
		psrc->num_bits * psrc->procs->bit_data_elt_size);
    pdest->wse = psrc->wse;
    pdest->transfer = psrc->transfer;
    rc_increment(pdest->transfer);
    return 0;
}

/*
 * Set the destination component to match the source component, and
 * "assume ownership" of all of the refrernced data structures.
 */
private void
gx_ht_move_ht_order(gx_ht_order * pdest, gx_ht_order * psrc)
{
    uint    width = psrc->width, height = psrc->height, shift = psrc->shift;

    pdest->params = psrc->params;
    pdest->wse = psrc->wse;
    pdest->wts = 0;
    pdest->width = width;
    pdest->height = height;
    pdest->raster = bitmap_raster(width);
    pdest->shift = shift;
    pdest->orig_height = height;
    pdest->orig_shift = shift;
    pdest->full_height = ht_order_full_height(pdest);
    pdest->num_levels = psrc->num_levels;
    pdest->num_bits = psrc->num_bits;
    pdest->procs = psrc->procs;
    pdest->data_memory = psrc->data_memory;
    pdest->levels = psrc->levels;
    pdest->bit_data = psrc->bit_data;
    pdest->cache = psrc->cache;    /* should be 0 */
    pdest->transfer = psrc->transfer;
}


/* Allocate and initialize the contents of a halftone order. */
/* The client must have set the defining values in porder->params. */
int
gx_ht_alloc_order(gx_ht_order * porder, uint width, uint height,
		  uint strip_shift, uint num_levels, gs_memory_t * mem)
{
    gx_ht_order order;
    int code;

    order = *porder;
    gx_compute_cell_values(&order.params);
    code = gx_ht_alloc_ht_order(&order, width, height, num_levels,
				width * height, strip_shift,
				&ht_order_procs_default, mem);
    if (code < 0)
	return code;
    *porder = order;
    return 0;
}

/*
 * Allocate and initialize a threshold order, which may use the short
 * representation.
 */
int
gx_ht_alloc_threshold_order(gx_ht_order * porder, uint width, uint height,
			    uint num_levels, gs_memory_t * mem)
{
    gx_ht_order order;
    uint num_bits = width * height;
    const gx_ht_order_procs_t *procs =
	(num_bits > 2000 && num_bits <= max_ushort ?
	 &ht_order_procs_short : &ht_order_procs_default);
    int code;

    order = *porder;
    gx_compute_cell_values(&order.params);
    code = gx_ht_alloc_ht_order(&order, width, height, num_levels,
				width * height, 0, procs, mem);
    if (code < 0)
	return code;
    *porder = order;
    return 0;
}

/* Allocate and initialize the contents of a client-defined halftone order. */
int
gx_ht_alloc_client_order(gx_ht_order * porder, uint width, uint height,
			 uint num_levels, uint num_bits, gs_memory_t * mem)
{
    gx_ht_order order;
    int code;

    order = *porder;
    order.params.M = width, order.params.N = 0;
    order.params.R = 1;
    order.params.M1 = height, order.params.N1 = 0;
    order.params.R1 = 1;
    gx_compute_cell_values(&order.params);
    code = gx_ht_alloc_ht_order(&order, width, height, num_levels,
				num_bits, 0, &ht_order_procs_default, mem);
    if (code < 0)
	return code;
    *porder = order;
    return 0;
}

/* Compare keys ("masks", actually sample values) for qsort. */
private int
compare_samples(const void *p1, const void *p2)
{
    ht_sample_t m1 = ((const gx_ht_bit *)p1)->mask;
    ht_sample_t m2 = ((const gx_ht_bit *)p2)->mask;

    return (m1 < m2 ? -1 : m1 > m2 ? 1 : 0);
}
/* Sort the halftone order by sample value. */
void
gx_sort_ht_order(gx_ht_bit * recs, uint N)
{
    int i;

    /* Tag each sample with its index, for sorting. */
    for (i = 0; i < N; i++)
	recs[i].offset = i;
    qsort((void *)recs, N, sizeof(*recs), compare_samples);
#ifdef DEBUG
    if (gs_debug_c('H')) {
	uint i;

	dlputs("[H]Sorted samples:\n");
	for (i = 0; i < N; i++)
	    dlprintf3("%5u: %5u: %u\n",
		      i, recs[i].offset, recs[i].mask);
    }
#endif
}

/*
 * Construct the halftone order from a sampled spot function.  Only width x
 * strip samples have been filled in; we must replicate the resulting sorted
 * order vertically, shifting it by shift each time.  See gxdht.h regarding
 * the invariants that must be restored.
 */
void
gx_ht_construct_spot_order(gx_ht_order * porder)
{
    uint width = porder->width;
    uint num_levels = porder->num_levels;	/* = width x strip */
    uint strip = num_levels / width;
    gx_ht_bit *bits = (gx_ht_bit *)porder->bit_data;
    uint *levels = porder->levels;
    uint shift = porder->orig_shift;
    uint full_height = porder->full_height;
    uint num_bits = porder->num_bits;
    uint copies = num_bits / (width * strip);
    gx_ht_bit *bp = bits + num_bits - 1;
    uint i;

    gx_sort_ht_order(bits, num_levels);
    if_debug5('h',
	      "[h]spot order: num_levels=%u w=%u h=%u strip=%u shift=%u\n",
	      num_levels, width, porder->orig_height, strip, shift);
    /* Fill in the levels array, replicating the bits vertically */
    /* if needed. */
    for (i = num_levels; i > 0;) {
	uint offset = bits[--i].offset;
	uint x = offset % width;
	uint hy = offset - x;
	uint k;

	levels[i] = i * copies;
	for (k = 0; k < copies;
	     k++, bp--, hy += num_levels, x = (x + width - shift) % width
	    )
	    bp->offset = hy + x;
    }
    /* If we have a complete halftone, restore the invariant. */
    if (num_bits == width * full_height) {
	porder->height = full_height;
	porder->shift = 0;
    }
    gx_ht_construct_bits(porder);
}

/* Construct a single offset/mask. */
void
gx_ht_construct_bit(gx_ht_bit * bit, int width, int bit_num)
{
    uint padding = bitmap_raster(width) * 8 - width;
    int pix = bit_num;
    ht_mask_t mask;
    byte *pb;

    pix += pix / width * padding;
    bit->offset = (pix >> 3) & -size_of(mask);
    mask = (ht_mask_t) 1 << (~pix & (ht_mask_bits - 1));
    /* Replicate the mask bits. */
    pix = ht_mask_bits - width;
    while ((pix -= width) >= 0)
	mask |= mask >> width;
    /* Store the mask, reversing bytes if necessary. */
    bit->mask = 0;
    for (pb = (byte *) & bit->mask + (sizeof(mask) - 1);
	 mask != 0;
	 mask >>= 8, pb--
	)
	*pb = (byte) mask;
}

/* Construct offset/masks from the whitening order. */
/* porder->bits[i].offset contains the index of the bit position */
/* that is i'th in the whitening order. */
void
gx_ht_construct_bits(gx_ht_order * porder)
{
    uint i;
    gx_ht_bit *phb;

    for (i = 0, phb = (gx_ht_bit *)porder->bit_data;
	 i < porder->num_bits;
	 i++, phb++)
	gx_ht_construct_bit(phb, porder->width, phb->offset);
#ifdef DEBUG
    if (gs_debug_c('H')) {
	dlprintf1("[H]Halftone order bits 0x%lx:\n", (ulong)porder->bit_data);
	for (i = 0, phb = (gx_ht_bit *)porder->bit_data;
	     i < porder->num_bits;
	     i++, phb++)
	    dlprintf3("%4d: %u:0x%lx\n", i, phb->offset,
		      (ulong) phb->mask);
    }
#endif
}

/* Release a gx_device_halftone by freeing its components. */
/* (Don't free the gx_device_halftone itself.) */
void
gx_ht_order_release(gx_ht_order * porder, gs_memory_t * mem, bool free_cache)
{
    /* "free cache" is a proxy for "differs from default" */
    if (free_cache) {
        if (porder->cache != 0)
            gx_ht_free_cache(mem, porder->cache);
        else if (porder->wse != 0)
            gs_wts_free_enum(porder->wse);
    }
    porder->cache = 0;
    if (porder->wts != 0 && porder->width != ht_wts_suppress_release)
        gs_wts_free_screen(porder->wts);
    porder->wts = 0;
    rc_decrement(porder->transfer, "gx_ht_order_release(transfer)");
    porder->transfer = 0;
    if (porder->data_memory != 0) {
	gs_free_object(porder->data_memory, porder->bit_data,
		       "gx_ht_order_release(bit_data)");
	gs_free_object(porder->data_memory, porder->levels,
		       "gx_ht_order_release(levels)");
    }
    porder->levels = 0;
    porder->bit_data = 0;
}

void
gx_device_halftone_release(gx_device_halftone * pdht, gs_memory_t * mem)
{
    if (pdht->components) {
	int i;

	/* One of the components might be the same as the default */
	/* order, so check that we don't free it twice. */
	for (i = 0; i < pdht->num_comp; ++i)
	    if (pdht->components[i].corder.bit_data !=
		pdht->order.bit_data
		) {		/* Currently, all orders except the default one */
		/* own their caches. */
		gx_ht_order_release(&pdht->components[i].corder, mem, true);
	    }
	gs_free_object(mem, pdht->components,
		       "gx_dev_ht_release(components)");
	pdht->components = 0;
	pdht->num_comp = 0;
    }
    gx_ht_order_release(&pdht->order, mem, false);
}

/*
 * This routine will take a color name (defined by a ptr and size) and
 * check if this is a valid colorant name for the current device.  If
 * so then the device's colorant number is returned.
 *
 * Two other checks are also made.  If the name is "Default" then a value
 * of GX_DEVICE_COLOR_MAX_COMPONENTS is returned.  This is done to
 * simplify the handling of default halftones.  Note:  The device also
 * uses GX_DEVICE_COLOR_MAX_COMPONENTS to indicate colorants which are
 * known but not being used due to the SeparationOrder parameter.  In this
 * case we return -1 since the colorant is not currently being used by the
 * device.
 *
 * If the halftone type is colorscreen or multiple colorscreen, then we
 * also check for Red/Cyan, Green/Magenta, Blue/Yellow, and Gray/Black
 * component name pairs.  This is done since the setcolorscreen and
 * sethalftone types 2 and 4 imply the dual name sets.
 *
 * A negative value is returned if the color name is not found.
 */
int
gs_color_name_component_number(gx_device * dev, const char * pname,
				int name_size, int halftonetype)
{
    int num_colorant;

#define check_colorant_name(dev, name) \
    ((*dev_proc(dev, get_color_comp_index)) (dev, name, strlen(name), NO_COMP_NAME_TYPE))

#define check_colorant_name_length(dev, name, length) \
    ((*dev_proc(dev, get_color_comp_index)) (dev, name, length, NO_COMP_NAME_TYPE))

#define check_name(str, pname, length) \
    ((strlen(str) == length) && (strncmp(pname, str, length) == 0))

    /*
     * Check if this is a device colorant.
     */
    num_colorant = check_colorant_name_length(dev, pname, name_size);
    if (num_colorant >= 0) {
	/*
	 * The device will return GX_DEVICE_COLOR_MAX_COMPONENTS if the
	 * colorant is logically present in the device but not being used
	 * because a SeparationOrder parameter is specified.  Since we are
	 * using this value to indicate 'Default', we use -1 to indicate
	 * that the colorant is not really being used.
	 */
	if (num_colorant == GX_DEVICE_COLOR_MAX_COMPONENTS)
	    num_colorant = -1;
	return num_colorant;
    }

    /*
     * Check if this is the default component
     */
    if (check_name("Default", pname, name_size))
	return GX_DEVICE_COLOR_MAX_COMPONENTS;

    /* Halftones set by setcolorscreen, and (we think) */
    /* Type 2 and Type 4 halftones, are supposed to work */
    /* for both RGB and CMYK, so we need a special check here. */
    if (halftonetype == ht_type_colorscreen ||
	halftonetype == ht_type_multiple_colorscreen) {
	if (check_name("Red", pname, name_size))
	    num_colorant = check_colorant_name(dev, "Cyan");
	else if (check_name("Green", pname, name_size))
	    num_colorant = check_colorant_name(dev, "Magenta");
	else if (check_name("Blue", pname, name_size))
	    num_colorant = check_colorant_name(dev, "Yellow");
	else if (check_name("Gray", pname, name_size))
	    num_colorant = check_colorant_name(dev, "Black");

#undef check_colorant_name
#undef check_colorant_name_length
#undef check_name

    }
    return num_colorant;
}

/*
 * See gs_color_name_component_number for main description.
 *
 * This version converts a name index value into a string and size and
 * then call gs_color_name_component_number.
 */
int
gs_cname_to_colorant_number(gs_state * pgs, byte * pname, uint name_size,
		int halftonetype)
{
    gx_device * dev = pgs->device;

    return gs_color_name_component_number(dev, (char *)pname, name_size,
		    halftonetype);
}

/*
 * Install a device halftone into the imager state.
 *
 * To allow halftones to be shared between graphic states, the imager
 * state contains a pointer to a device halftone structure. Thus, when 
 * we say a halftone is "in" the imager state, we are only claiming 
 * that the halftone pointer in the imager state points to that halftone.
 *
 * Though the operand halftone uses the same structure as the halftone
 * "in" the imager state, not all of its fields are filled in, and the
 * organization of components differs. Specifically, the following fields
 * are not filled in:
 *
 *  rc          The operand device halftone has only a transient existence,
 *              its reference count information is not initialized. In many
 *              cases, the operand device halftone structure is allocated
 *              on the stack by clients.
 *
 *  id          A halftone is not considered to have an identity until it
 *              is installed in the imager state. This is a design error
 *              which reflects the PostScript origins of this code. In
 *              PostScript, it is impossible to check if two halftone
 *              specifications (sets of operands to setscreen/setcolorscreen
 *              or halftone dictionaries) are the same. Hence, the only way
 *              a halftone could be identified was by the graphic state in
 *              which it was included. In PCL it is possible to directly
 *              identify a halftone specification, but currently there is
 *              no way to use this knowledge in the graphic library.
 *
 *              (An altogether more reasonable approach would be to apply
 *              id's to halftone orders.)
 *
 *  type        This is filled in by the type operand. It is used by
 *              PostScript's currentscreen/currentcolorscreen operators to
 *              determine if a sampling procedure or a halftone dictionary
 *              should be pushed onto the stack. More importantly, it is
 *              also used to determine if specific halftone components can
 *              be used for either the additive or subtractive version of
 *              that component in the process color model. For example, a
 *              RedThreshold in a HalftoneType 4 dictionary can be applied
 *              to either the component "Red" or the component "Cyan", but
 *              the value of the key "Red" in a HalftoneType 5 dictionary
 *              can only be used for a "Red" component (not a "Cyan"
 *              component).
 *
 *  num_comp    For the operand halftone, this is the number of halftone
 *              components included in the specification. For the device
 *              halftone in the imager state, this is always the same as
 *              the number of color model components (see num_dev_comp).
 *
 *  num_dev_comp The number of components in the device process color model
 *		when the operand halftone was created.  With some compositor
 *		devices (for example PDF 1.4) we can have differences in the
 *		process color model of the compositor versus the output device.
 *		These compositor devices do not halftone.
 *
 *  components  For the operand halftone, this field is non-null only if
 *              multiple halftones are provided. In that case, the size
 *              of the array pointed is the same as the number of
 *              components provided. One of these components will usually
 *              be the same as that identified by the "order" field.
 *
 *              For the device halftone in the imager state, this field is
 *              always non-null, and the size of the array pointed to will
 *              be the same as the number of components in the process
 *              color model.
 *
 *  lcm_width,  These fields provide the least common multiple of the
 *  lcm_height  halftone dimensions of the individual component orders.
 *              They represent the dimensions of the smallest tile that
 *              repeats for all color components (this is of interest
 *              because Ghostscript uses a "chunky" raster format for all
 *              drawing procedures). These fields cannot be set in the
 *              operand device halftone as we do not yet know which of
 *              the halftone components will actually be used.
 *
 * Conversely, the "order" field is significant only in the operand device
 * halftone. There it represents the default halftone component, which will
 * be used for all device color components for which a named halftone is
 * not available. It is ignored (filled with 0's) in the device halftone
 * in the imager state.
 *
 * The ordering of entries and the set of fields initialized in the
 * components array also vary between the operand device halftone and
 * the device halftone in the imager state.
 *
 * If the components array is present in the operand device halftone, the
 * cname field in each entry of the array will contain a name index
 * identifying the colorant name, and the comp_number field will provide the
 * index of the corresponding component in the process color model. The
 * order of entries in the components array is essentially arbitrary,
 * but in some common cases will reflect the order in which the halftone
 * specification is provided. By convention, if no explicit default order
 * is provided (i.e.: via a HalftoneType 5 dictionary), the first
 * entry of the array will be the same as the "order" (default) field.
 *
 * For the device halftone in the imager state, the components array is
 * always present, but the cname and comp_number fields of individual
 * entries are ignored. The order of the entries in the array always
 * matches the order of components in the device color model.
 *
 * The distinction between the operand device halftone and the one in
 * the graphic state extends even to the individual fields of the
 * gx_ht_order structure incorporated in the order field of the halftone
 * and the corder field of the elements of the components array. The
 * fields of this structure that are handled differently in the operand 
 * and imager state device halftones are:
 *
 *  params          Provides a set of parameters that are required for
 *                  converting a halftone specification to a single
 *                  component order. This field is used only in the
 *                  operand device halftone; it is not set in the device
 *                  halftone in the imager state.
 *
 *  wse            Points to an "enumerator" instance, used to construct
 *                 a well-tempered screen. This is only required while
 *                 the well-tempered screen is being constructed. This
 *                 field is always a null pointer in the device halftone
 *                 in the imager state.
 *
 *  wts            Points to the "constructed" form of a well-tempered
 *                 screen. The "construction" operation occurs as part
 *                 of the installation process. Hence, this should
 *                 always be a null pointer in the operand device
 *                 halftone.
 *
 *  orig_height,   The height and shift values of the halftone cell,
 *  orig_shift     prior to any replication. These fields are currently
 *                 unused, and will always be the same as the height
 *                 and width fields in the device halftone in the
 *                 imager state.
 *
 *  full_height    The height of the smallest replicated tile whose shift
 *                 value is 0. This is calculated as part of the
 *                 installation process; it may be set in the operand
 *                 device halftone, but its value is ignored.
 *
 *  
 *  data_memory    Points to the memory structure used to allocate the
 *                 levels and bit_data arrays. The handling of this field
 *                 is a bit complicated. For orders that are "taken over"
 *                 by the installation process, this field will have the
 *                 same value in the operand device halftone and the
 *                 device halftone in the imager state. For halftones
 *                 that are copied by the installation process, this
 *                 field will have the same value as the memory field in
 *                 the imager state (the two are usually the same).
 *
 *  cache          Pointer to a cache of tiles representing various
 *                 levels of the halftone. This may or may not be
 *                 provided in the operand device halftone (in principle
 *                 this should always be a null pointer in the operand
 *                 device halftone, but this is not the manner in which
 *                 the cache was handled historically).
 *
 *  screen_params  This structure contains transformation information
 *                 that is required when reading the sample data for a
 *                 screen. It is no longer required once the halftone
 *                 order has been constructed.
 *
 * In addition to what is noted above, this procedure is made somewhat
 * more complex than expected due to memory management considerations. To
 * clarify this, it is necessary to consider the properties of the pieces
 * that constitute a device halftone.
 *
 *  The gx_device_halftone structure itself is shareable and uses
 *  reference counts.
 *
 *  The gx_ht_order_component array (components array entry) is in
 *  principle shareable, though it does not provide any reference
 *  counting mechanism. Hence any sharing needs to be done with
 *  caution.
 *
 *  Individual component orders are not shareable, as they are part of
 *  the gx_ht_order_commponent structure (a major design error).
 *
 *  The levels, bit_data, and cache structures referenced by the
 *  gx_ht_order structure are in principle shareable, but they also do
 *  not provide any reference counting mechanism. Traditionally, one set
 *  of two component orders could share these structures, using the
 *  halftone's "order" field and various scattered bits of special case
 *  code. This practice has been ended because it did not extend to
 *  sharing amongst more than two components.
 *
 *  The gx_transfer_map structure referenced by the gx_ht_order structure
 *  is shareable, and uses reference counts. Traditionally this structure
 *  was not shared, but this is no longer the case.
 *
 * As noted, the rc field of the operand halftone is not initialized, so
 * this procedure cannot simply take ownership of the operand device
 * halftone structure (i.e.: an ostensibly shareable structure is not
 * shareable). Hence, this procedure will always create a new copy of the
 * gx_device_halftone structure, either by allocating a new structure or
 * re-using the structure already referenced by the imager state. This
 * feature must be retained, as in several cases the calling code will
 * allocate the operand device halftone structure on the stack.
 *
 * Traditionally, this procedure took ownership of all of the structures
 * referenced by the operand device halftone structure. This implied
 * that all structures referenced by the gx_device_halftone structure
 * needed to be allocated on the heap, and should not be released once
 * the call to gx_imager_dev_ht_install completes.
 *
 * There were two problems with this approach:
 *
 *  1. In the event of an error, the calling code most likely would have
 *     to release referenced components, as the imager state had not yet
 *     take ownership of them. In many cases, the code did not do this.
 *
 *  2. When the structures referenced by a single order needed to be
 *     shared amongst more than one component, there was no easy way to
 *     discover this sharing when the imager state's device halftone
 *     subsequently needed to be released. Hence, objects would be
 *     released multiple times.
 *
 * Subsequently, the code in this routine was changed to copy most of
 * the referenced structures (everything except the transfer functions).
 * Unfortunately, the calling code was not changed, which caused memory
 * leaks.
 *
 * The approach now taken uses a mixture of the two approaches.
 * Ownership to structures referenced by the operand device halftone is
 * assumed by the device halftone in the imager state where this is
 * possible. In these cases, the corresponding references are removed in
 * the operand device halftone (hence, this operand is no longer
 * qualified as const). When a structure is required but ownership cannot
 * be assumed, a copy is made and the reference in the operand device
 * halftone is left undisturbed. The calling code has also been modified
 * to release any remaining referenced structures when this routine
 * returns, whether or not an error is indicated.
 */
int
gx_imager_dev_ht_install(
    gs_imager_state *       pis,
    gx_device_halftone *    pdht,
    gs_halftone_type        type,
    const gx_device *       dev )
{
    gx_device_halftone      dht;
    int                     num_comps = pdht->num_dev_comp;
    int                     i, code = 0;
    bool                    used_default = false;
    int                     lcm_width = 1, lcm_height = 1;
    gs_wts_screen_enum_t *  wse0 = pdht->order.wse;
    wts_screen_t *          wts0 = 0;
    bool                    mem_diff = pdht->rc.memory != pis->memory;

    /* construct the new device halftone structure */
    memset(&dht.order, 0, sizeof(dht.order));
    /* the rc field is filled in later */
    dht.id = gs_next_ids(pis->memory, 1);
    dht.type = type;
    dht.components =  gs_alloc_struct_array(
                          pis->memory,
                          num_comps,
                          gx_ht_order_component,
                          &st_ht_order_component_element,
                          "gx_imager_dev_ht_install(components)" );
    if (dht.components == NULL)
	return_error(gs_error_VMerror);
    dht.num_comp = dht.num_dev_comp = num_comps;
    /* lcm_width, lcm_height are filled in later */

    /* initialize the components array */
    memset(dht.components, 0, num_comps * sizeof(dht.components[0]));
    for (i = 0; i < num_comps; i++)
        dht.components[i].comp_number = -1;

    /*
     * Duplicate any of the non-default components, but do not create copies
     * of the levels or bit_data arrays. If all goes according to plan, the
     * imager state's device halftone will assume ownership of these arrays
     * by clearing the corresponding pointers in the operand halftone's
     * orders.
     */
    if (pdht->components != 0) {
        int     input_ncomps = pdht->num_comp;

        for (i = 0; i < input_ncomps && code >= 0; i++) {
            gx_ht_order_component * p_s_comp = &pdht->components[i];
            gx_ht_order *           p_s_order = &p_s_comp->corder;
            int                     comp_num = p_s_comp->comp_number;

	    if (comp_num >= 0 && comp_num < GX_DEVICE_COLOR_MAX_COMPONENTS) {
                gx_ht_order *   p_d_order = &dht.components[comp_num].corder;

                /* indicate that this order has been filled in */
                dht.components[comp_num].comp_number = comp_num;

                /*
                 * The component can be used only if it is from the
                 * proper memory
                 */
                if (mem_diff)
                    code = gx_ht_copy_ht_order( p_d_order,
                                                p_s_order,
                                                pis->memory );
                else {
                    /* check if this is also the default component */
                    used_default = used_default ||
                                   p_s_order->bit_data == pdht->order.bit_data;

                    gx_ht_move_ht_order(p_d_order, p_s_order);
                }
            }
        }
    }

    /*
     * Copy the default order to any remaining components.
     *
     * For well-tempered screens, generate the wts_screen_t structure
     * for each component that corresponds to the sample information
     * that has been gathered.
     *
     * Some caution is necessary here, as multiple component orders may
     * have wse fields pointing to the same gs_wts_creeen_enum_t
     * structure. This structure should only be released once. If
     * multiple components have such a wse value, it will be the same as
     * pdht->order.wse pointer, so we can just release that pointer once
     * when done.
     *
     * If serveral component orders have the same wse value, this code
     * will create just one wts_screen_t structure. In a somewhat ugly
     * hack, the width field (which is otherwise unused) will be set to
     * 0xffff for all components other than the first component that
     * makes use of a give wts_screen_t structure. gx_ht_order_release
     * will check this field to see if it should release the structure
     * pointed to by the wts field of a component order.
     *
     * Components that are not well-tempered screens require a cache.
     * In practice, either all or non of the components will be well-
     * tempered screens, but we ignore that fact here.
     *
     * While engaged in all of these other activities, also calculate
     * the lcm_width and lcm_heigth values (only for non-well-tempered
     * components).
     */
    for (i = 0; i < num_comps && code >= 0; i++) {
        gx_ht_order *           porder = &dht.components[i].corder;
        gs_wts_screen_enum_t *  wse;

        if (dht.components[i].comp_number != i) {
            if (used_default || mem_diff)
                code = gx_ht_copy_ht_order(porder, &pdht->order, pis->memory);
            else {
                gx_ht_move_ht_order(porder, &pdht->order);
                used_default = true;
            }
            dht.components[i].comp_number = i;
        }
        if ((wse = porder->wse) != 0) {
            wts_screen_t *  wts = 0;

            porder->width = 0;
            porder->wse = 0;
            if (wse != wse0)
                wts = wts_screen_from_enum(wse);
            else {
                if (wts0 == 0)
                    wts0 = wts_screen_from_enum(wse);
                else
                    porder->width = ht_wts_suppress_release;
                wts = wts0;
            }
            if (wts == 0)
                code = gs_error_VMerror;
            else
                porder->wts = wts;
        } else {
            uint   w = porder->width, h = porder->full_height;
            int    dw = igcd(lcm_width, w), dh = igcd(lcm_height, h);

            lcm_width /= dw;
            lcm_height /= dh;
            lcm_width = (w > max_int / lcm_width ? max_int : lcm_width * w);
            lcm_height = (h > max_int / lcm_height ? max_int : lcm_height * h);

            if (porder->cache == 0) {
                uint            tile_bytes, num_tiles;
                gx_ht_cache *   pcache;

                tile_bytes = porder->raster
                              * (porder->num_bits / porder->width);
                num_tiles = 1 + max_tile_cache_bytes / tile_bytes;
                pcache = gx_ht_alloc_cache( pis->memory,
                                            num_tiles,
                                            tile_bytes * num_tiles );
                if (pcache == NULL) 
                    code = gs_error_VMerror;
                else {
                    porder->cache = pcache;
                    gx_ht_init_cache(pis->memory, pcache, porder);
                }
            }
        }
    }
    dht.lcm_width = lcm_width;
    dht.lcm_height = lcm_height;

    /*
     * If everything is OK so far, allocate a unique copy of the device
     * halftone reference by the imager state.
     *
     * This code requires a special check for the case in which the
     * deivce halftone referenced by the imager state is already unique.
     * In this case, we must explicitly release just the components array
     * (and any structures it refers to) of the existing halftone. This
     * cannot be done automatically, as the rc_unshare_struct macro only
     * ensures that a unique instance of the top-level structure is
     * created, not that any substructure references are updated.
     *
     * Though this is scheduled to be changed, for the time being the
     * command list renderer may invoke this code with pdht == psi->dev_ht
     * (in which case we know pis->dev_ht.rc.ref_count == 1). Special
     * handling is required in that case, to avoid releasing structures
     * we still need.
     */
    if (code >= 0) {
        gx_device_halftone *    pisdht = pis->dev_ht;
        rc_header               tmp_rc;

        if (pisdht != 0 && pisdht->rc.ref_count == 1) {
            if (pdht != pisdht)
                gx_device_halftone_release(pisdht, pisdht->rc.memory);
        } else {
            rc_unshare_struct( pis->dev_ht,
                               gx_device_halftone,
                               &st_device_halftone,
                               pis->memory,
                               BEGIN code = gs_error_VMerror; goto err; END,
                               "gx_imager_dev_ht_install" );
            pisdht = pis->dev_ht;
        }

        /*
         * Everything worked. "Assume ownership" of the appropriate
         * portions of the source device halftone by clearing the
         * associated references. This includes explicitly releasing
         * any gs_wts_screen_enum_t structures. Since we might have
         * pdht == pis->dev_ht, this must done before updating pis->dev_ht.
         *
         * If the default order has been used for a device component, and
         * any of the source component orders share their levels or bit_data
         * arrays with the default order, clear the pointers in those orders
         * now. This is necessary because the default order's pointers will
         * be cleared immediately below, so subsequently it will not be
         * possible to tell if that this information is being shared.
         */
        if (pdht->components != 0) {
            int     input_ncomps = pdht->num_comp;

            for (i = 0; i < input_ncomps; i++) {
                gx_ht_order_component * p_s_comp = &pdht->components[i];
                gx_ht_order *           p_s_order = &p_s_comp->corder;
                int                     comp_num = p_s_comp->comp_number;

                if ( comp_num >= 0                            &&
                     comp_num < GX_DEVICE_COLOR_MAX_COMPONENTS  ) {
                    if (p_s_order->wse != 0)
                        gs_wts_free_enum(p_s_order->wse);
                    memset(p_s_order, 0, sizeof(*p_s_order));
                } else if ( comp_num == GX_DEVICE_COLOR_MAX_COMPONENTS &&
                            used_default                                 )
                    memset(p_s_order, 0, sizeof(*p_s_order));
            }
        }
        if (used_default) {
            if (wse0 != 0)
                gs_wts_free_enum(wse0);
            memset(&pdht->order, 0, sizeof(pdht->order));
        }

        tmp_rc = pisdht->rc;
        *pisdht = dht;
        pisdht->rc = tmp_rc;

        /* update the effective transfer function array */
        gx_imager_set_effective_xfer(pis);

        return 0;
    }

    /* something went amiss; release all copied components */
  err:
    for (i = 0; i < num_comps; i++) {
        gx_ht_order_component * pcomp = &dht.components[i];
        gx_ht_order *           porder = &pcomp->corder;

        if (pcomp->comp_number == -1)
            gx_ht_order_release(porder, pis->memory, true);
    }
    gs_free_object(pis->memory, dht.components, "gx_imager_dev_ht_install");

    return code;
}

/*
 * Install a new halftone in the graphics state.  Note that we copy the top
 * level of the gs_halftone and the gx_device_halftone, and take ownership
 * of any substructures.
 */
int
gx_ht_install(gs_state * pgs, const gs_halftone * pht,
	      gx_device_halftone * pdht)
{
    gs_memory_t *mem = pht->rc.memory;
    gs_halftone *old_ht = pgs->halftone;
    gs_halftone *new_ht;
    int code;

    pdht->num_dev_comp = pgs->device->color_info.num_components;
    if (old_ht != 0 && old_ht->rc.memory == mem &&
	old_ht->rc.ref_count == 1
	)
	new_ht = old_ht;
    else
	rc_alloc_struct_1(new_ht, gs_halftone, &st_halftone,
			  mem, return_error(gs_error_VMerror),
			  "gx_ht_install(new halftone)");
    code = gx_imager_dev_ht_install((gs_imager_state *) pgs,
			     pdht, pht->type, gs_currentdevice_inline(pgs));
    if (code < 0) {
	if (new_ht != old_ht)
	    gs_free_object(mem, new_ht, "gx_ht_install(new halftone)");
	return code;
    }

    /*
     * Discard and unused components and the components array of the
     * operand device halftone
     */
    gx_device_halftone_release(pdht, pdht->rc.memory);

    if (new_ht != old_ht)
	rc_decrement(old_ht, "gx_ht_install(old halftone)");
    {
	rc_header rc;

	rc = new_ht->rc;
	*new_ht = *pht;
	new_ht->rc = rc;
    }
    pgs->halftone = new_ht;
    gx_unset_dev_color(pgs);
    return 0;
}

/*
 * This macro will determine the colorant number of a given color name.
 * A value of -1 indicates that the name is not valid.
 */
#define check_colorant_name(name, dev) \
   ((*dev_proc(dev, get_color_comp_index)) (dev, name, strlen(name), NO_NAME_TYPE))

/* Reestablish the effective transfer functions, taking into account */
/* any overrides from halftone dictionaries. */
void
gx_imager_set_effective_xfer(gs_imager_state * pis)
{
    const gx_device_halftone *pdht = pis->dev_ht;
    gx_transfer_map *pmap;
    int i, component_num;

    for (i = 0; i < GX_DEVICE_COLOR_MAX_COMPONENTS; i++)
	pis->effective_transfer[i] = pis->set_transfer.gray;	/* default */

    /* Check if we have a transfer functions from setcolortransfer */
    if (pis->set_transfer.red) {
	component_num = pis->set_transfer.red_component_num;
	if (component_num >= 0)
	    pis->effective_transfer[component_num] = pis->set_transfer.red;;
    }
    if (pis->set_transfer.green) {
	component_num = pis->set_transfer.green_component_num;
	if (component_num >= 0)
	    pis->effective_transfer[component_num] = pis->set_transfer.green;
    }
    if (pis->set_transfer.blue) {
	component_num = pis->set_transfer.blue_component_num;
	if (component_num >= 0)
	    pis->effective_transfer[component_num] = pis->set_transfer.blue;
    }

    if (pdht == NULL)
	return;			/* not initialized yet */

    for (i = 0; i < pdht->num_comp; i++) {
	pmap = pdht->components[i].corder.transfer;
	if (pmap != NULL)
	    pis->effective_transfer[i] = pmap;
    }
}

void
gx_set_effective_transfer(gs_state * pgs)
{
    gx_imager_set_effective_xfer((gs_imager_state *) pgs);
}
