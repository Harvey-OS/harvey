/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gzht.h,v 1.13 2004/08/04 19:36:12 stefan Exp $ */
/* Internal procedures for halftones */
/* Requires gxdevice.h, gxdcolor.h */

#ifndef gzht_INCLUDED
#  define gzht_INCLUDED

#include "gscsel.h"
#include "gxht.h"
#include "gxfmap.h"
#include "gxdht.h"
#include "gxhttile.h"

/* Sort a sampled halftone order by sample value. */
void gx_sort_ht_order(gx_ht_bit *, uint);

/* (Internal) procedures for constructing halftone orders. */
int gx_ht_alloc_ht_order(gx_ht_order * porder, uint width, uint height,
			 uint num_levels, uint num_bits, uint strip_shift,
			 const gx_ht_order_procs_t *procs,
			 gs_memory_t * mem);
int gx_ht_alloc_order(gx_ht_order * porder, uint width, uint height,
		      uint strip_shift, uint num_levels, gs_memory_t *mem);
int gx_ht_alloc_threshold_order(gx_ht_order * porder, uint width,
				uint height, uint num_levels,
				gs_memory_t * mem);
int gx_ht_alloc_client_order(gx_ht_order * porder, uint width, uint height,
			     uint num_levels, uint num_bits, gs_memory_t * mem);
void gx_ht_construct_spot_order(gx_ht_order *);
int gx_ht_construct_threshold_order(gx_ht_order *, const byte *);
void gx_ht_construct_bit(gx_ht_bit * bit, int width, int bit_num);
void gx_ht_construct_bits(gx_ht_order *);

/* Halftone enumeration structure */
struct gs_screen_enum_s {
    gs_halftone halftone;	/* supplied by client */
    gx_ht_order order;
    gs_matrix mat;		/* for mapping device x,y to rotated cell */
    gs_matrix mat_inv;		/* the inversion of mat */
    int x, y;
    int strip, shift;
    gs_state *pgs;
};

#define private_st_gs_screen_enum() /* in gshtscr.c */\
  gs_private_st_composite(st_gs_screen_enum, gs_screen_enum,\
    "gs_screen_enum", screen_enum_enum_ptrs, screen_enum_reloc_ptrs)
/*    order.levels, order.bits, pgs) */

/* Prepare a device halftone for installation, but don't install it. */
int gs_sethalftone_prepare(gs_state *, gs_halftone *,
			   gx_device_halftone *);

/* Allocate and initialize a spot screen. */
/* This is the first half of gs_screen_init_accurate/memory. */
int gs_screen_order_alloc(gx_ht_order *, gs_memory_t *);
int gs_screen_order_init_memory(gx_ht_order *, const gs_state *,
				gs_screen_halftone *, bool, gs_memory_t *);

#define gs_screen_order_init(porder, pgs, phsp, accurate)\
  gs_screen_order_init_memory(porder, pgs, phsp, accurate, pgs->memory)

/* Prepare to sample a spot screen. */
/* This is the second half of gs_screen_init_accurate/memory. */
int gs_screen_enum_init_memory(gs_screen_enum *, const gx_ht_order *,
			       gs_state *, const gs_screen_halftone *,
			       gs_memory_t *);

#define gs_screen_enum_init(penum, porder, pgs, phsp)\
  gs_screen_enum_init_memory(penum, porder, pgs, phsp, pgs->memory)

/* Process an entire screen plane. */
int gx_ht_process_screen_memory(gs_screen_enum * penum, gs_state * pgs,
				gs_screen_halftone * phsp, bool accurate,
				gs_memory_t * mem);

#define gx_ht_process_screen(penum, pgs, phsp, accurate)\
  gx_ht_process_screen_memory(penum, pgs, phsp, accurate, pgs->memory)

/*
 * We don't want to remember all the values of the halftone screen,
 * because they would take up space proportional to P^3, where P is
 * the number of pixels in a cell.  Instead, we pick some number N of
 * patterns to cache.  Each cache slot covers a range of (P+1)/N
 * different gray levels: we "slide" the contents of the slot back and
 * forth within this range by incrementally adding and dropping 1-bits.
 * N>=0 (obviously); N<=P+1 (likewise); also, so that we can simplify things
 * by preallocating the bookkeeping information for the cache, we define
 * a constant max_cached_tiles which is an a priori maximum value for N.
 *
 * Note that the raster for each tile must be a multiple of bitmap_align_mod,
 * to satisfy the copy_mono device routine, even though a multiple of
 * sizeof(ht_mask_t) would otherwise be sufficient.
 */

struct gx_ht_cache_s {
    /* The following are set when the cache is created. */
    byte *bits;			/* the base of the bits */
    uint bits_size;		/* the space available for bits */
    gx_ht_tile *ht_tiles;	/* the base of the tiles */
    uint num_tiles;		/* the number of tiles allocated */
    /* The following are reset each time the cache is initialized */
    /* for a new screen. */
    gx_ht_order order;		/* the cached order vector */
    int num_cached;		/* actual # of cached tiles */
    int levels_per_tile;	/* # of levels per cached tile */
    int tiles_fit;		/* -1 if not determined, 0 if no fit, */
				/* 1 if fit */
    gx_bitmap_id base_id;	/* the base id, to which */
				/* we add the halftone level */
    gx_ht_tile *(*render_ht)(gx_ht_cache *, int); /* rendering procedure */
};

/* Define the sizes of the halftone cache. */
#define max_cached_tiles_HUGE 5000	/* not used */
#define max_ht_bits_HUGE 1000000	/* not used */
#define max_cached_tiles_LARGE 577
#define max_ht_bits_LARGE 100000
#define max_cached_tiles_SMALL 25
#define max_ht_bits_SMALL 1000

/* Define the size of the halftone tile cache. */
#define max_tile_bytes_LARGE 4096
#define max_tile_bytes_SMALL 512
#if arch_small_memory
#  define max_tile_cache_bytes max_tile_bytes_SMALL
#else
#  define max_tile_cache_bytes\
     (gs_debug_c('.') ? max_tile_bytes_SMALL : max_tile_bytes_LARGE)
#endif

/* We don't mark from the tiles pointer, and we relocate the tiles en masse. */
#define private_st_ht_tiles()	/* in gxht.c */\
  gs_private_st_composite(st_ht_tiles, gx_ht_tile, "ht tiles",\
    ht_tiles_enum_ptrs, ht_tiles_reloc_ptrs)
#define private_st_ht_cache()	/* in gxht.c */\
  gs_private_st_ptrs_add2(st_ht_cache, gx_ht_cache, "ht cache",\
    ht_cache_enum_ptrs, ht_cache_reloc_ptrs,\
    st_ht_order, order, bits, ht_tiles)

/* Compute a fractional color for dithering, the correctly rounded */
/* quotient f * max_gx_color_value / maxv. */
#define frac_color_(f, maxv)\
  (gx_color_value)(((f) * (0xffffL * 2) + maxv) / (maxv * 2))
extern const gx_color_value *const fc_color_quo[8];

#define fractional_color(f, maxv)\
  ((maxv) <= 7 ? fc_color_quo[maxv][f] : frac_color_(f, maxv))

/* ------ Halftone cache procedures ------ */

/* Allocate/free a halftone cache. */
uint gx_ht_cache_default_tiles(void);
uint gx_ht_cache_default_bits(void);
gx_ht_cache *gx_ht_alloc_cache(gs_memory_t *, uint, uint);
void gx_ht_free_cache(gs_memory_t *, gx_ht_cache *);

/* Clear a halftone cache. */
#define gx_ht_clear_cache(pcache)\
  ((pcache)->order.levels = 0, (pcache)->order.bit_data = 0,\
   (pcache)->ht_tiles[0].tiles.data = 0)

/* Initialize a halftone cache with a given order. */
void gx_ht_init_cache(const gs_memory_t *mem, gx_ht_cache *, const gx_ht_order *);

/* Check whether the tile cache corresponds to the current order */
bool gx_check_tile_cache_current(const gs_imager_state * pis);

/* Make the cache order current, and return whether */
/* there is room for all possible tiles in the cache. */
bool gx_check_tile_cache(const gs_imager_state *);

/* Determine whether a given (width, y, height) might fit into a */
/* single tile. If so, return the byte offset of the appropriate row */
/* from the beginning of the tile, and set *ppx to the x phase offset */
/* within the tile; if not, return -1. */
int gx_check_tile_size(const gs_imager_state * pis, int w, int y, int h,
		       gs_color_select_t select, int *ppx);

/* Make a given level current in a halftone cache. */
#define gx_render_ht(pcache, b_level)\
  ((pcache)->render_ht(pcache, b_level))

/* ------ Device halftone management ------ */

/* Release a gx_ht_order by freeing its components. */
/* (Don't free the gx_device_halftone itself.) */
void gx_ht_order_release(gx_ht_order * porder, gs_memory_t * mem, bool free_cache);

/*
 * Install a device halftone in an imager state.  Note that this does not
 * read or update the client halftone.
 */
int gx_imager_dev_ht_install(gs_imager_state * pis,
			     gx_device_halftone * pdht,
			     gs_halftone_type type,
			     const gx_device * dev);

/*
 * Install a new halftone in the graphics state.  Note that we copy the top
 * level of the gs_halftone and the gx_device_halftone, and take ownership
 * of any substructures.
 */
int gx_ht_install(gs_state *, const gs_halftone *, gx_device_halftone *);

/* Reestablish the effective transfer functions, taking into account */
/* any overrides from halftone dictionaries. */
/* Some compilers object to names longer than 31 characters.... */
void gx_imager_set_effective_xfer(gs_imager_state * pis);
void gx_set_effective_transfer(gs_state * pgs);

/*
 * This routine will take a color name (defined by a ptr and size) and
 * check if this is a valid colorant name for the current device.  If
 * so then the device's colorant number is returned.
 *
 * Two other checks are also made.  If the name is "Default" then a value
 * of GX_DEVICE_COLOR_MAX_COMPONENTS is returned.  This is done to
 * simplify the handling of default halftones.
 *
 * If the halftone type is colorscreen or multiple colorscreen, then we
 * also check for Red/Cyan, Green/Magenta, Blue/Yellow, and Gray/Black
 * component name pairs.  This is done since the setcolorscreen and
 * sethalftone types 2 and 4 imply the dual name sets.
 *
 * A negative value is returned if the color name is not found.
 */
int gs_color_name_component_number(gx_device * dev, const char * pname,
				int name_size, int halftonetype);
/*
 * See gs_color_name_component_number for main description.
 *
 * This version converts a name index value into a string and size and
 * then call gs_color_name_component_number.
 */
int gs_cname_to_colorant_number(gs_state * pgs, byte * pname, uint name_size,
				 int halftonetype);
#endif /* gzht_INCLUDED */
