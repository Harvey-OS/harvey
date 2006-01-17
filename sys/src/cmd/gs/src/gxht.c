/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxht.c,v 1.17 2005/05/23 22:33:22 dan Exp $ */
/* Halftone rendering for imaging library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsbitops.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxdcolor.h"
#include "gxfixed.h"
#include "gxdevice.h"		/* for gzht.h */
#include "gxistate.h"
#include "gzht.h"
#include "gsserial.h"

/* Define the sizes of the halftone cache. */
#define max_cached_tiles_HUGE 5000	/* not used */
#define max_ht_bits_HUGE 1000000	/* not used */
#define max_cached_tiles_LARGE 577
#define max_ht_bits_LARGE 100000
#define max_cached_tiles_SMALL 25
#define max_ht_bits_SMALL 1000

/* Define the binary halftone device color type. */
/* The type descriptor must be public for Pattern types. */
gs_public_st_composite(st_dc_ht_binary, gx_device_color, "dc_ht_binary",
		       dc_ht_binary_enum_ptrs, dc_ht_binary_reloc_ptrs);
private dev_color_proc_save_dc(gx_dc_ht_binary_save_dc);
private dev_color_proc_get_dev_halftone(gx_dc_ht_binary_get_dev_halftone);
private dev_color_proc_load(gx_dc_ht_binary_load);
private dev_color_proc_fill_rectangle(gx_dc_ht_binary_fill_rectangle);
private dev_color_proc_fill_masked(gx_dc_ht_binary_fill_masked);
private dev_color_proc_equal(gx_dc_ht_binary_equal);
private dev_color_proc_write(gx_dc_ht_binary_write);
private dev_color_proc_read(gx_dc_ht_binary_read);
const gx_device_color_type_t
      gx_dc_type_data_ht_binary =
{&st_dc_ht_binary,
 gx_dc_ht_binary_save_dc, gx_dc_ht_binary_get_dev_halftone,
 gx_dc_ht_get_phase,
 gx_dc_ht_binary_load, gx_dc_ht_binary_fill_rectangle,
 gx_dc_ht_binary_fill_masked, gx_dc_ht_binary_equal,
 gx_dc_ht_binary_write, gx_dc_ht_binary_read,
 gx_dc_ht_binary_get_nonzero_comps
};

#undef gx_dc_type_ht_binary
const gx_device_color_type_t *const gx_dc_type_ht_binary =
&gx_dc_type_data_ht_binary;

#define gx_dc_type_ht_binary (&gx_dc_type_data_ht_binary)
/* GC procedures */
private 
ENUM_PTRS_WITH(dc_ht_binary_enum_ptrs, gx_device_color *cptr) return 0;
ENUM_PTR(0, gx_device_color, colors.binary.b_ht);
case 1:
{
    gx_ht_tile *tile = cptr->colors.binary.b_tile;

    ENUM_RETURN(tile ? tile - tile->index : 0);
}
ENUM_PTRS_END
private RELOC_PTRS_WITH(dc_ht_binary_reloc_ptrs, gx_device_color *cptr)
{
    gx_ht_tile *tile = cptr->colors.binary.b_tile;
    uint index = tile ? tile->index : 0;

    RELOC_PTR(gx_device_color, colors.binary.b_ht);
    RELOC_TYPED_OFFSET_PTR(gx_device_color, colors.binary.b_tile, index);
}
RELOC_PTRS_END
#undef cptr

/* Other GC procedures */
private_st_ht_tiles();
private 
ENUM_PTRS_BEGIN_PROC(ht_tiles_enum_ptrs)
{
    return 0;
}
ENUM_PTRS_END_PROC
private RELOC_PTRS_BEGIN(ht_tiles_reloc_ptrs)
{
    /* Reset the bitmap pointers in the tiles. */
    /* We know the first tile points to the base of the bits. */
    gx_ht_tile *ht_tiles = vptr;
    byte *bits = ht_tiles->tiles.data;
    uint diff;

    if (bits == 0)
	return;
    RELOC_VAR(bits);
    if (size == size_of(gx_ht_tile)) {	/* only 1 tile */
	ht_tiles->tiles.data = bits;
	return;
    }
    diff = ht_tiles[1].tiles.data - ht_tiles[0].tiles.data;
    for (; size; ht_tiles++, size -= size_of(gx_ht_tile), bits += diff) {
	ht_tiles->tiles.data = bits;
    }
}
RELOC_PTRS_END
private_st_ht_cache();

/* Return the default sizes of the halftone cache. */
uint
gx_ht_cache_default_tiles(void)
{
#if arch_small_memory
    return max_cached_tiles_SMALL;
#else
    return (gs_debug_c('.') ? max_cached_tiles_SMALL :
	    max_cached_tiles_LARGE);
#endif
}
uint
gx_ht_cache_default_bits(void)
{
#if arch_small_memory
    return max_ht_bits_SMALL;
#else
    return (gs_debug_c('.') ? max_ht_bits_SMALL :
	    max_ht_bits_LARGE);
#endif
}

/* Allocate a halftone cache. */
gx_ht_cache *
gx_ht_alloc_cache(gs_memory_t * mem, uint max_tiles, uint max_bits)
{
    gx_ht_cache *pcache =
    gs_alloc_struct(mem, gx_ht_cache, &st_ht_cache,
		    "alloc_ht_cache(struct)");
    byte *tbits =
	gs_alloc_bytes(mem, max_bits, "alloc_ht_cache(bits)");
    gx_ht_tile *ht_tiles =
	gs_alloc_struct_array(mem, max_tiles, gx_ht_tile, &st_ht_tiles,
			      "alloc_ht_cache(ht_tiles)");

    if (pcache == 0 || tbits == 0 || ht_tiles == 0) {
	gs_free_object(mem, ht_tiles, "alloc_ht_cache(ht_tiles)");
	gs_free_object(mem, tbits, "alloc_ht_cache(bits)");
	gs_free_object(mem, pcache, "alloc_ht_cache(struct)");
	return 0;
    }
    pcache->bits = tbits;
    pcache->bits_size = max_bits;
    pcache->ht_tiles = ht_tiles;
    pcache->num_tiles = max_tiles;
    pcache->order.cache = pcache;
    pcache->order.transfer = 0;
    gx_ht_clear_cache(pcache);
    return pcache;
}

/* Free a halftone cache. */
void
gx_ht_free_cache(gs_memory_t * mem, gx_ht_cache * pcache)
{
    gs_free_object(mem, pcache->ht_tiles, "free_ht_cache(ht_tiles)");
    gs_free_object(mem, pcache->bits, "free_ht_cache(bits)");
    gs_free_object(mem, pcache, "free_ht_cache(struct)");
}

/* Check whether the tile cache corresponds to the current order */
bool
gx_check_tile_cache_current(const gs_imager_state * pis)
{
    /* TO_DO_DEVICEN - this routine is no longer used - delete. */
    return false;
}

/* Make the cache order current, and return whether */
/* there is room for all possible tiles in the cache. */
bool
gx_check_tile_cache(const gs_imager_state * pis)
{
    /* TO_DO_DEVICEN - this routine is no longer used - delete. */
    return false;
}

/*
 * Determine whether a given (width, y, height) might fit into a single
 * (non-strip) tile. If so, return the byte offset of the appropriate row
 * from the beginning of the tile, and set *ppx to the x phase offset
 * within the tile; if not, return -1.
 *
 * This routine cannot be supported in the DeviceN code.
 */
int
gx_check_tile_size(const gs_imager_state * pis, int w, int y, int h,
		   gs_color_select_t select, int *ppx)
{
    /* TO_DO_DEVICEN - this routine is no longer used - delete. */
    return -1;
}

/* Render a given level into a halftone cache. */
private int render_ht(gx_ht_tile *, int, const gx_ht_order *,
		      gx_bitmap_id);
private gx_ht_tile *
gx_render_ht_default(gx_ht_cache * pcache, int b_level)
{
    const gx_ht_order *porder = &pcache->order;
    int level = porder->levels[b_level];
    gx_ht_tile *bt = &pcache->ht_tiles[level / pcache->levels_per_tile];

    if (bt->level != level) {
	int code = render_ht(bt, level, porder, pcache->base_id + b_level);

	if (code < 0)
	    return 0;
    }
    return bt;
}
/* Faster code if num_tiles == 1. */
private gx_ht_tile *
gx_render_ht_1_tile(gx_ht_cache * pcache, int b_level)
{
    const gx_ht_order *porder = &pcache->order;
    int level = porder->levels[b_level];
    gx_ht_tile *bt = &pcache->ht_tiles[0];

    if (bt->level != level) {
	int code = render_ht(bt, level, porder, pcache->base_id + b_level);

	if (code < 0)
	    return 0;
    }
    return bt;
}
/* Faster code if levels_per_tile == 1. */
private gx_ht_tile *
gx_render_ht_1_level(gx_ht_cache * pcache, int b_level)
{
    const gx_ht_order *porder = &pcache->order;
    int level = porder->levels[b_level];
    gx_ht_tile *bt = &pcache->ht_tiles[level];

    if (bt->level != level) {
	int code = render_ht(bt, level, porder, pcache->base_id + b_level);

	if (code < 0)
	    return 0;
    }
    return bt;
}

/* save information about the operand binary halftone color */
private void
gx_dc_ht_binary_save_dc(const gx_device_color * pdevc,
                        gx_device_color_saved * psdc)
{
    psdc->type = pdevc->type;
    psdc->colors.binary.b_color[0] = pdevc->colors.binary.color[0];
    psdc->colors.binary.b_color[1] = pdevc->colors.binary.color[1];
    psdc->colors.binary.b_level = pdevc->colors.binary.b_level;
    psdc->colors.binary.b_index = pdevc->colors.binary.b_index;
    psdc->phase = pdevc->phase;
}

/* get the halftone used for a binary halftone color */
private const gx_device_halftone *
gx_dc_ht_binary_get_dev_halftone(const gx_device_color * pdevc)
{
    return pdevc->colors.binary.b_ht;
}

/* Load the device color into the halftone cache if needed. */
private int
gx_dc_ht_binary_load(gx_device_color * pdevc, const gs_imager_state * pis,
		     gx_device * dev, gs_color_select_t select)
{
    int component_index = pdevc->colors.binary.b_index;
    const gx_ht_order *porder =
	(component_index < 0 ?
	 &pdevc->colors.binary.b_ht->order :
	 &pdevc->colors.binary.b_ht->components[component_index].corder);
    gx_ht_cache *pcache = porder->cache;

    if (pcache->order.bit_data != porder->bit_data)
	gx_ht_init_cache(pis->memory, pcache, porder);
    /*
     * We do not load the cache now.  Instead we wait until we are ready
     * to actually render the color.  This allows multiple colors to be
     * loaded without cache conflicts.  (Cache conflicts can occur when
     * if two device colors use the same cache elements.  This can occur
     * when the tile size is large enough that we do not have a separate
     * tile for each half tone level.)  See gx_dc_ht_binary_load_cache.
     */
    pdevc->colors.binary.b_tile = NULL;
    return 0;
}

/*
 * Load the half tone tile in the halftone cache.
 */
private int
gx_dc_ht_binary_load_cache(const gx_device_color * pdevc)
{
    int component_index = pdevc->colors.binary.b_index;
    const gx_ht_order *porder =
	 &pdevc->colors.binary.b_ht->components[component_index].corder;
    gx_ht_cache *pcache = porder->cache;
    int b_level = pdevc->colors.binary.b_level;
    int level = porder->levels[b_level];
    gx_ht_tile *bt = &pcache->ht_tiles[level / pcache->levels_per_tile];

    if (bt->level != level) {
	int code = render_ht(bt, level, porder, pcache->base_id + b_level);

	if (code < 0)
	    return_error(gs_error_Fatal);
    }
    ((gx_device_color *)pdevc)->colors.binary.b_tile = bt;
    return 0;
}

/* Fill a rectangle with a binary halftone. */
/* Note that we treat this as "texture" for RasterOp. */
private int
gx_dc_ht_binary_fill_rectangle(const gx_device_color * pdevc, int x, int y,
		  int w, int h, gx_device * dev, gs_logical_operation_t lop,
			       const gx_rop_source_t * source)
{
    gx_rop_source_t no_source;

    /* Load the halftone cache for the color */
    gx_dc_ht_binary_load_cache(pdevc);
    /*
     * Observation of H-P devices and documentation yields confusing
     * evidence about whether white pixels in halftones are always
     * opaque.  It appears that for black-and-white devices, these
     * pixels are *not* opaque.
     */
    if (dev->color_info.depth > 1)
	lop &= ~lop_T_transparent;
    if (source == NULL && lop_no_S_is_T(lop))
	return (*dev_proc(dev, strip_tile_rectangle)) (dev,
					&pdevc->colors.binary.b_tile->tiles,
				  x, y, w, h, pdevc->colors.binary.color[0],
					      pdevc->colors.binary.color[1],
					    pdevc->phase.x, pdevc->phase.y);
    /* Adjust the logical operation per transparent colors. */
    if (pdevc->colors.binary.color[0] == gx_no_color_index)
	lop = rop3_use_D_when_T_0(lop);
    if (pdevc->colors.binary.color[1] == gx_no_color_index)
	lop = rop3_use_D_when_T_1(lop);
    if (source == NULL)
	set_rop_no_source(source, no_source, dev);
    return (*dev_proc(dev, strip_copy_rop)) (dev, source->sdata,
			       source->sourcex, source->sraster, source->id,
			     (source->use_scolors ? source->scolors : NULL),
					&pdevc->colors.binary.b_tile->tiles,
					     pdevc->colors.binary.color,
				 x, y, w, h, pdevc->phase.x, pdevc->phase.y,
					     lop);
}

private int
gx_dc_ht_binary_fill_masked(const gx_device_color * pdevc, const byte * data,
	int data_x, int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gx_device * dev, gs_logical_operation_t lop, bool invert)
{
    /*
     * Load the halftone cache for the color.  We do not do it earlier
     * because for small halftone caches, each cache tile may be used for
     * for more than one halftone level.  This can cause conflicts if more
     * than one device color has been set and they use the same cache
     * entry.
     */
    int code = gx_dc_ht_binary_load_cache(pdevc);

    if (code < 0)
	return code;
    return gx_dc_default_fill_masked(pdevc, data, data_x, raster, id,
		    			x, y, w, h, dev, lop, invert);
}

/* Compare two binary halftones for equality. */
private bool
gx_dc_ht_binary_equal(const gx_device_color * pdevc1,
		      const gx_device_color * pdevc2)
{
    return pdevc2->type == pdevc1->type &&
	pdevc1->phase.x == pdevc2->phase.x &&
	pdevc1->phase.y == pdevc2->phase.y &&
	gx_dc_binary_color0(pdevc1) == gx_dc_binary_color0(pdevc2) &&
	gx_dc_binary_color1(pdevc1) == gx_dc_binary_color1(pdevc2) &&
	pdevc1->colors.binary.b_level == pdevc2->colors.binary.b_level;
}


/* 
 * Flags to indicate the pieces of a binary halftone that are included
 * in its string representation. The first byte of the string holds this
 * set of flags.
 *
 * The binary halftone tile is never transmitted as part of the string
 * representation, so there is also no flag bit for it.
 */
private const int   dc_ht_binary_has_color0 = 0x01;
private const int   dc_ht_binary_has_color1 = 0x02;
private const int   dc_ht_binary_has_level = 0x04;
private const int   dc_ht_binary_has_index = 0x08;


/*
 * Serialize a binany halftone device color.
 *
 * Operands:
 *
 *  pdevc       pointer to device color to be serialized
 *
 *  psdc        pointer ot saved version of last serialized color (for
 *              this band)
 *  
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to buffer in which to write the data
 *
 *  psize       pointer to a location that, on entry, contains the size of
 *              the buffer pointed to by pdata; on return, the size of
 *              the data required or actually used will be written here.
 *
 * Returns:
 *  1, with *psize set to 0, if *psdc and *pdevc represent the same color
 *
 *  0, with *psize set to the amount of data written, if everything OK
 *
 *  gs_error_rangecheck, with *psize set to the size of buffer required,
 *  if *psize was not large enough
 *
 *  < 0, != gs_error_rangecheck, in the event of some other error; in this
 *  case *psize is not changed.
 */
private int
gx_dc_ht_binary_write(
    const gx_device_color *         pdevc,
    const gx_device_color_saved *   psdc0,
    const gx_device *               dev,
    byte *                          pdata,
    uint *                          psize )
{
    int                             req_size = 1;   /* flag bits */
    int                             flag_bits = 0;
    uint                            tmp_size;
    byte *                          pdata0 = pdata;
    const gx_device_color_saved *   psdc = psdc0;
    int                             code;

    /* check if operand and saved colors are the same type */
    if (psdc != 0 && psdc->type != pdevc->type)
        psdc = 0;

    /* check for the information that must be transmitted */
    if ( psdc == 0                                                      ||
         pdevc->colors.binary.color[0] != psdc->colors.binary.b_color[0]  ) {
        flag_bits |= dc_ht_binary_has_color0;
        tmp_size = 0;
        (void)gx_dc_write_color( pdevc->colors.binary.color[0],
                                 dev,
                                 pdata,
                                 &tmp_size );
        req_size += tmp_size;
    }
    if ( psdc == 0                                                      ||
         pdevc->colors.binary.color[1] != psdc->colors.binary.b_color[1]  ) {
        flag_bits |= dc_ht_binary_has_color1;
        tmp_size = 0;
        (void)gx_dc_write_color( pdevc->colors.binary.color[1],
                                 dev,
                                 pdata,
                                 &tmp_size );
        req_size += tmp_size;
    }

    if ( psdc == 0                                                  ||
         pdevc->colors.binary.b_level != psdc->colors.binary.b_level  ) {
        flag_bits |= dc_ht_binary_has_level;
        req_size += enc_u_sizew(pdevc->colors.binary.b_level);
    }

    if ( psdc == 0                                                  ||
         pdevc->colors.binary.b_index != psdc->colors.binary.b_index  ) {
        flag_bits |= dc_ht_binary_has_index;
        req_size += 1;
    }

    /* check if there is anything to be done */
    if (flag_bits == 0) {
        *psize = 0;
        return 1;
    }

    /* check if sufficient space has been provided */
    if (req_size > *psize) {
        *psize = req_size;
        return gs_error_rangecheck;
    }

    /* write out the flag byte */
    *pdata++ = (byte)flag_bits;

    /* write out such other parts of the device color as are required */
    if ((flag_bits & dc_ht_binary_has_color0) != 0) {
        tmp_size = req_size - (pdata - pdata0);
        code = gx_dc_write_color( pdevc->colors.binary.color[0],
                                  dev,
                                  pdata,
                                  &tmp_size );
        if (code < 0)
            return code;
        pdata += tmp_size;
    }
    if ((flag_bits & dc_ht_binary_has_color1) != 0) {
        tmp_size = req_size - (pdata - pdata0);
        code = gx_dc_write_color( pdevc->colors.binary.color[1],
                                  dev,
                                  pdata,
                                  &tmp_size );
        if (code < 0)
            return code;
        pdata += tmp_size;
    }
    if ((flag_bits & dc_ht_binary_has_level) != 0)
        enc_u_putw(pdevc->colors.binary.b_level, pdata);
    if ((flag_bits & dc_ht_binary_has_index) != 0)
        *pdata++ = pdevc->colors.binary.b_index;

    *psize = pdata - pdata0;
    return 0;
}

/*
 * Reconstruct a binary halftone device color from its serial representation.
 *
 * Operands:
 *
 *  pdevc       pointer to the location in which to write the
 *              reconstructed device color
 *
 *  pis         pointer to the current imager state (to access the
 *              current halftone)
 *
 *  prior_devc  pointer to the current device color (this is provided
 *              separately because the device color is not part of the
 *              imager state)
 *
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to the buffer to be read
 *
 *  size        size of the buffer to be read; this should be large
 *              enough to hold the entire color description
 *
 *  mem         pointer to the memory to be used for allocations
 *              (ignored here)
 *
 * Returns:
 *
 *  # of bytes read if everthing OK, < 0 in the event of an error
 */
private int
gx_dc_ht_binary_read(
    gx_device_color *       pdevc,
    const gs_imager_state * pis,
    const gx_device_color * prior_devc,
    const gx_device *       dev,        /* ignored */
    const byte *            pdata,
    uint                    size,
    gs_memory_t *           mem )       /* ignored */
{
    gx_device_color         devc;
    const byte *            pdata0 = pdata;
    int                     code, flag_bits;

    /* if prior information is available, use it */
    if (prior_devc != 0 && prior_devc->type == gx_dc_type_ht_binary)
        devc = *prior_devc;
    else
        memset(&devc, 0, sizeof(devc));   /* clear pointers */
    devc.type = gx_dc_type_ht_binary;

    /* the halftone is always taken from the imager state */
    devc.colors.binary.b_ht = pis->dev_ht;

    /* cache is not porvided until the device color is used */
    devc.colors.binary.b_tile = 0;

    /* verify the minimum amount of information */
    if (size == 0)
        return_error(gs_error_rangecheck);
    size --;
    flag_bits = *pdata++;

    /* read the other information provided */
    if ((flag_bits & dc_ht_binary_has_color0) != 0) {
        code = gx_dc_read_color( &devc.colors.binary.color[0],
                                 dev,
                                 pdata,
                                 size );
        if (code < 0)
            return code;
        size -= code;
        pdata += code;
    }
    if ((flag_bits & dc_ht_binary_has_color1) != 0) {
        code = gx_dc_read_color( &devc.colors.binary.color[1],
                                 dev,
                                 pdata,
                                 size );
        if (code < 0)
            return code;
        size -= code;
        pdata += code;
    }
    if ((flag_bits & dc_ht_binary_has_level) != 0) {
        const byte *    pdata_start = pdata;

        if (size < 1)
            return_error(gs_error_rangecheck);
        enc_u_getw(devc.colors.binary.b_level, pdata);
        size -= pdata - pdata_start;
    }
    if ((flag_bits & dc_ht_binary_has_index) != 0) {
        if (size == 0)
            return_error(gs_error_rangecheck);
	--size;
        devc.colors.binary.b_index = *pdata++;
    }

    /* set the phase as required (select value is arbitrary) */
    /* set the phase as required (select value is arbitrary) */
    color_set_phase_mod( &devc,
                         pis->screen_phase[0].x,
                         pis->screen_phase[0].y,
                         pis->dev_ht->lcm_width,
                         pis->dev_ht->lcm_height );

    /* everything looks good */
    *pdevc = devc;
    return pdata - pdata0;
}


/*
 * Get the nonzero components of a binary halftone. This is used to
 * distinguish components that are given zero intensity due to halftoning
 * from those for which the original color intensity was in fact zero.
 *
 * Since this device color type involves only a single halftone component,
 * we can reasonably assume that b_level != 0. Hence, we need to check
 * for components with identical intensities in color[0] and color[1].
 */
int
gx_dc_ht_binary_get_nonzero_comps(
    const gx_device_color * pdevc,
    const gx_device *       dev,
    gx_color_index *        pcomp_bits )
{
    int                     code;
    gx_color_value          cvals_0[GX_DEVICE_COLOR_MAX_COMPONENTS],
                            cvals_1[GX_DEVICE_COLOR_MAX_COMPONENTS];

    if ( (code = dev_proc(dev, decode_color)( (gx_device *)dev,
                                              pdevc->colors.binary.color[0],
                                              cvals_0 )) >= 0 &&
         (code = dev_proc(dev, decode_color)( (gx_device *)dev,
                                              pdevc->colors.binary.color[1],
                                              cvals_1 )) >= 0   ) {
        int     i, ncomps = dev->color_info.num_components;
        int     mask = 0x1, comp_bits = 0;

        for (i = 0; i < ncomps; i++, mask <<= 1) {
            if (cvals_0[i] != 0 || cvals_1[i] != 0)
                comp_bits |= mask;
        }
        *pcomp_bits = comp_bits;
        code = 0;
    }

    return code;
}


/* Initialize the tile cache for a given screen. */
/* Cache as many different levels as will fit. */
void
gx_ht_init_cache(const gs_memory_t *mem, gx_ht_cache * pcache, const gx_ht_order * porder)
{
    uint width = porder->width;
    uint height = porder->height;
    uint size = width * height + 1;
    int width_unit =
    (width <= ht_mask_bits / 2 ? ht_mask_bits / width * width :
     width);
    int height_unit = height;
    uint raster = porder->raster;
    uint tile_bytes = raster * height;
    uint shift = porder->shift;
    int num_cached;
    int i;
    byte *tbits = pcache->bits;

    /* Non-monotonic halftones may have more bits than size. */
    if (porder->num_bits >= size)
	size = porder->num_bits + 1;
    /* Make sure num_cached is within bounds */
    num_cached = pcache->bits_size / tile_bytes;
    if (num_cached > size)
	num_cached = size;
    if (num_cached > pcache->num_tiles)
	num_cached = pcache->num_tiles;
    if (num_cached == size &&
	tile_bytes * num_cached <= pcache->bits_size / 2
	) {
	/*
	 * We can afford to replicate every tile in the cache,
	 * which will reduce breakage when tiling.  Since
	 * horizontal breakage is more expensive than vertical,
	 * and since wide shallow fills are more common than
	 * narrow deep fills, we replicate the tile horizontally.
	 * We do have to be careful not to replicate the tile
	 * to an absurdly large size, however.
	 */
	uint rep_raster =
	((pcache->bits_size / num_cached) / height) &
	~(align_bitmap_mod - 1);
	uint rep_count = rep_raster * 8 / width;

	/*
	 * There's no real value in replicating the tile
	 * beyond the point where the byte width of the replicated
	 * tile is a multiple of a long.
	 */
	if (rep_count > sizeof(ulong) * 8)
	    rep_count = sizeof(ulong) * 8;
	width_unit = width * rep_count;
	raster = bitmap_raster(width_unit);
	tile_bytes = raster * height;
    }
    pcache->base_id = gs_next_ids(mem, porder->num_levels + 1);
    pcache->order = *porder;
    /* The transfer function is irrelevant, and might become dangling. */
    pcache->order.transfer = 0;
    pcache->num_cached = num_cached;
    pcache->levels_per_tile = (size + num_cached - 1) / num_cached;
    pcache->tiles_fit = -1;
    memset(tbits, 0, pcache->bits_size);
    for (i = 0; i < num_cached; i++, tbits += tile_bytes) {
	register gx_ht_tile *bt = &pcache->ht_tiles[i];

	bt->level = 0;
	bt->index = i;
	bt->tiles.data = tbits;
	bt->tiles.raster = raster;
	bt->tiles.size.x = width_unit;
	bt->tiles.size.y = height_unit;
	bt->tiles.rep_width = width;
	bt->tiles.rep_height = height;
	bt->tiles.shift = bt->tiles.rep_shift = shift;
    }
    pcache->render_ht =
	(pcache->num_tiles == 1 ? gx_render_ht_1_tile :
	 pcache->levels_per_tile == 1 ? gx_render_ht_1_level :
	 gx_render_ht_default);
}

/*
 * Compute and save the rendering of a given gray level
 * with the current halftone.  The cache holds multiple tiles,
 * where each tile covers a range of possible levels.
 * We adjust the tile whose range includes the desired level incrementally;
 * this saves a lot of time for the average image, where gray levels
 * don't change abruptly.  Note that the "level" is the number of bits,
 * not the index in the levels vector.
 */
private int
render_ht(gx_ht_tile * pbt, int level /* [1..num_bits-1] */ ,
	  const gx_ht_order * porder, gx_bitmap_id new_id)
{
    byte *data = pbt->tiles.data;
    int code;

    if_debug7('H', "[H]Halftone cache slot 0x%lx: old=%d, new=%d, w=%d(%d), h=%d(%d):\n",
	      (ulong) data, pbt->level, level,
	      pbt->tiles.size.x, porder->width,
	      pbt->tiles.size.y, porder->num_bits / porder->width);
#ifdef DEBUG
    if (level < 0 || level > porder->num_bits) {
	lprintf3("Error in render_ht: level=%d, old level=%d, num_bits=%d\n",
		 level, pbt->level, porder->num_bits);
	return_error(gs_error_Fatal);
    }
#endif
    code = porder->procs->render(pbt, level, porder);
    if (code < 0)
	return code;
    pbt->level = level;
    pbt->tiles.id = new_id;
    /*
     * Check whether we want to replicate the tile in the cache.
     * Since we only do this when all the renderings will fit
     * in the cache, we only do it once per level, and it doesn't
     * have to be very efficient.
     */
	/****** TEST IS WRONG if width > rep_width but tile.raster ==
	 ****** order raster.
	 ******/
    if (pbt->tiles.raster > porder->raster)
	bits_replicate_horizontally(data, pbt->tiles.rep_width,
				    pbt->tiles.rep_height, porder->raster,
				    pbt->tiles.size.x, pbt->tiles.raster);
    if (pbt->tiles.size.y > pbt->tiles.rep_height &&
	pbt->tiles.shift == 0
	)
	bits_replicate_vertically(data, pbt->tiles.rep_height,
				  pbt->tiles.raster, pbt->tiles.size.y);
#ifdef DEBUG
    if (gs_debug_c('H')) {
	const byte *p = pbt->tiles.data;
	int wb = pbt->tiles.raster;
	const byte *ptr = p + wb * pbt->tiles.size.y;

	while (p < ptr) {
	    dprintf8(" %d%d%d%d%d%d%d%d",
		     *p >> 7, (*p >> 6) & 1, (*p >> 5) & 1,
		     (*p >> 4) & 1, (*p >> 3) & 1, (*p >> 2) & 1,
		     (*p >> 1) & 1, *p & 1);
	    if ((++p - data) % wb == 0)
		dputc('\n');
	}
    }
#endif
    return 0;
}
