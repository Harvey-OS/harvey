/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxht.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
private dev_color_proc_load(gx_dc_ht_binary_load);
private dev_color_proc_fill_rectangle(gx_dc_ht_binary_fill_rectangle);
private dev_color_proc_equal(gx_dc_ht_binary_equal);
const gx_device_color_type_t
      gx_dc_type_data_ht_binary =
{&st_dc_ht_binary,
 gx_dc_ht_binary_load, gx_dc_ht_binary_fill_rectangle,
 gx_dc_default_fill_masked, gx_dc_ht_binary_equal
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

    ENUM_RETURN(tile - tile->index);
}
ENUM_PTRS_END
private RELOC_PTRS_WITH(dc_ht_binary_reloc_ptrs, gx_device_color *cptr)
{
    uint index = cptr->colors.binary.b_tile->index;

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

/* Make the cache order current, and return whether */
/* there is room for all possible tiles in the cache. */
bool
gx_check_tile_cache(const gs_imager_state * pis)
{
    const gx_ht_order *porder = &pis->dev_ht->order;
    gx_ht_cache *pcache = pis->ht_cache;

    if (pcache == 0 || pis->dev_ht == 0)
	return false;		/* no halftone or cache */
    if (pcache->order.bit_data != porder->bit_data)
	gx_ht_init_cache(pcache, porder);
    if (pcache->tiles_fit >= 0)
	return (bool)pcache->tiles_fit;
    {
	bool fit = false;
	int bits_per_level;

	if (pcache->num_cached < porder->num_levels)
	    DO_NOTHING;
	else if (pcache->levels_per_tile == 1)
	    fit = true;
	/*
	 * All the tiles fit iff bits and levels are exactly N-for-1, where
	 * N is a multiple of levels_per_tile.
	 */
	else if (porder->num_bits % porder->num_levels == 0 &&
		 (bits_per_level = porder->num_bits / porder->num_levels) %
		   pcache->levels_per_tile == 0) {
	    /* Check the N-for-1 property. */
	    const uint *level = porder->levels;
	    int i = porder->num_levels, j = 0;

	    for (; i > 0; --i, j += bits_per_level, ++level)
		if (*level != j)
		    break;
	    fit = i == 0;
	}
	pcache->tiles_fit = (int)fit;
	return fit;
    }
}

/*
 * Determine whether a given (width, y, height) might fit into a single
 * (non-strip) tile. If so, return the byte offset of the appropriate row
 * from the beginning of the tile, and set *ppx to the x phase offset
 * within the tile; if not, return -1.
 */
int
gx_check_tile_size(const gs_imager_state * pis, int w, int y, int h,
		   gs_color_select_t select, int *ppx)
{
    int tsy;
    const gx_strip_bitmap *ptile0;

    if (pis->ht_cache == 0)
	return -1;		/* no halftone cache */
    ptile0 = &pis->ht_cache->ht_tiles[0].tiles;		/* a typical tile */
    if (h > ptile0->rep_height || w > ptile0->rep_width ||
	ptile0->shift != 0
	)
	return -1;
    tsy = (y + imod(-pis->screen_phase[select].y, ptile0->rep_height)) %
	ptile0->rep_height;
    if (tsy + h > ptile0->size.y)
	return -1;
    /* Tile fits in Y, might fit in X. */
    *ppx = imod(-pis->screen_phase[select].x, ptile0->rep_width);
    return tsy * ptile0->raster;
}

/* Render a given level into a halftone cache. */
private int render_ht(P4(gx_ht_tile *, int, const gx_ht_order *,
			 gx_bitmap_id));
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
    gx_ht_cache *pcache =
	(porder->cache == 0 ? pis->ht_cache : porder->cache);

    if (pcache->order.bit_data != porder->bit_data)
	gx_ht_init_cache(pcache, porder);
    /* Expand gx_render_ht inline for speed. */
    {
	int b_level = pdevc->colors.binary.b_level;
	int level = porder->levels[b_level];
	gx_ht_tile *bt = &pcache->ht_tiles[level / pcache->levels_per_tile];

	if (bt->level != level) {
	    int code = render_ht(bt, level, porder,
				 pcache->base_id + b_level);

	    if (code < 0)
		return_error(gs_error_Fatal);
	}
	pdevc->colors.binary.b_tile = bt;
    }
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

/* Initialize the tile cache for a given screen. */
/* Cache as many different levels as will fit. */
void
gx_ht_init_cache(gx_ht_cache * pcache, const gx_ht_order * porder)
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
    pcache->base_id = gs_next_ids(porder->num_levels + 1);
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
