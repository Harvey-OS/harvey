/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gzht.h */
/* Private halftone representation for Ghostscript */
/* Requires gxdevice.h, gxdcolor.h */
#include "gxht.h"
#include "gxfmap.h"

/*
 * The whitening order is represented by a pair of arrays.
 * The levels array contains an integer (an index into the bits array)
 * for each distinct halftone level, indicating how many pixels should be
 * whitened for that level; levels[0] = 0, levels[i] <= levels[i+1], and
 * levels[num_levels-1] <= num_bits.
 * The bits array contains an (offset,mask) pair for each pixel in the tile.
 * bits[i].offset is the (properly aligned) byte index of a pixel
 * in the tile; bits[i].mask is the mask to be or'ed into this byte and
 * following ones.  This is arranged so it will work properly on
 * either big- or little-endian machines, and with different mask widths.
 */
/* The mask width must be at least as wide as uint, */
/* and must not be wider than the width implied by align_bitmap_mod. */
typedef uint ht_mask_t;
#define ht_mask_bits (sizeof(ht_mask_t) * 8)
typedef struct gx_ht_bit_s {
	uint offset;
	ht_mask_t mask;
} gx_ht_bit;
/* During sampling, bits[i].mask is used to hold a normalized sample value. */
typedef ht_mask_t ht_sample_t;
/* The following awkward expression avoids integer overflow. */
#define max_ht_sample (ht_sample_t)(((1 << (ht_mask_bits - 2)) - 1) * 2 + 1)

/* Sort a sampled halftone order by sample value. */
void gx_sort_ht_order(P2(gx_ht_bit *, uint));

/* Define the internal representation of a halftone order. */
/* Note that it includes a cached phase value, computed from */
/* the halftone phase in the graphics state, */
/* and possibly a cached transfer function. */
typedef struct gx_ht_cache_s gx_ht_cache;
typedef struct gx_ht_order_s {
	ushort width;
	ushort height;
	ushort raster;
	ushort shift;			/* shift between strips, */
					/* see gshtscr.c */
	uint num_levels;		/* = levels size */
	uint num_bits;			/* = width * height = bits size */
	uint *levels;
	gx_ht_bit *bits;
	gs_int_point phase;		/* negated gstate phase mod */
					/* tile width/height */
	gx_ht_cache *cache;		/* cache to use, 0 means pgs->ht_cache */
	gx_transfer_map *transfer;	/* TransferFunction or 0 */
} gx_ht_order;
/* We only export st_ht_order for use in st_screen_enum. */
extern_st(st_ht_order);
#define public_st_ht_order()	/* in gsht.c */\
  gs_public_st_ptrs4(st_ht_order, gx_ht_order, "gx_ht_order",\
    ht_order_enum_ptrs, ht_order_reloc_ptrs, levels, bits, cache, transfer)
#define st_ht_order_max_ptrs 4

/* Procedures for constructing halftone orders */
int	gx_ht_alloc_order(P5(gx_ht_order *, uint, uint, uint, gs_memory_t *));
void	gx_ht_construct_spot_order(P1(gx_ht_order *));
void	gx_ht_construct_threshold_order(P2(gx_ht_order *, const byte *));
void	gx_ht_construct_bits(P1(gx_ht_order *));

/*
 * Define a device halftone.  This consists of one or more orders.
 * If components = 0, then order is the only current halftone screen
 * (set by setscreen, Type 1 sethalftone, Type 3 sethalftone, or
 * Type 5 sethalftone with only a Default).  Otherwise, order is the
 * gray or black screen (for gray/RGB or CMYK devices respectively),
 * and components is an array of gx_ht_order_components parallel to
 * the components of the client halftone (set by setcolorscreen or
 * Type 5 sethalftone).
 */
typedef struct gx_ht_order_component_s {
	gx_ht_order corder;
	gs_ht_separation_name cname;
} gx_ht_order_component;
#define private_st_ht_order_component()	/* in gsht1.c */\
  gs_private_st_ptrs_add0(st_ht_order_component, gx_ht_order_component,\
    "gx_ht_order_component", ht_order_component_enum_ptrs,\
     ht_order_component_reloc_ptrs, st_ht_order, corder)
#define st_ht_order_component_max_ptrs st_ht_order_max_ptrs
#define private_st_ht_order_comp_element() /* in gsht1.c */\
  gs_private_st_element(st_ht_order_component_element, gx_ht_order_component,\
    "gx_ht_order_component[]", ht_order_element_enum_ptrs,\
    ht_order_element_reloc_ptrs, st_ht_order_component)

#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

/*
 * color_indices is a cache that gives the indices in components of
 * the screens for the 1, 3, or 4 primary color(s).  These indices are
 * always in the same order, i.e.:
 *	-,-,-,W(gray)
 *	R,G,B,-
 *	C,M,Y,K
 */
struct gx_device_halftone_s {
	gx_ht_order order;
	uint color_indices[4];
	gx_ht_order_component *components;
	uint num_comp;
		/* The following are computed from the above. */
	int lcm_width, lcm_height;	/* LCM of primary color tile sizes, */
					/* max_int if overflowed */
};
extern_st(st_device_halftone);
#define public_st_device_halftone() /* in gsht.c */\
  gs_public_st_ptrs_add1(st_device_halftone, gx_device_halftone,\
    "gx_device_halftone", device_halftone_enum_ptrs,\
    device_halftone_reloc_ptrs, st_ht_order, order, components)
#define st_device_halftone_max_ptrs (st_ht_order_max_ptrs + 1)

/* Halftone enumeration structure */
struct gs_screen_enum_s {
	gs_halftone halftone;	/* supplied by client */
	gx_ht_order order;
	gs_matrix mat;		/* for mapping device x,y to rotated cell */
	int x, y;
	int strip;
	gs_state *pgs;
};
#define private_st_gs_screen_enum() /* in gshtscr.c */\
  gs_private_st_composite(st_gs_screen_enum, gs_screen_enum,\
    "gs_screen_enum", screen_enum_enum_ptrs, screen_enum_reloc_ptrs)
/*    order.levels, order.bits, pgs)*/

/* Prepare a device halftone for installation, but don't install it. */
int	gs_sethalftone_prepare(P3(gs_state *, gs_halftone *,
				  gx_device_halftone *));

/* Allocate and initialize a spot screen. */
/* This is the first half of gs_screen_init_accurate. */
int	gs_screen_order_init(P4(gx_ht_order *, const gs_state *,
				gs_screen_halftone *, bool));

/* Prepare to sample a spot screen. */
/* This is the second half of gs_screen_init_accurate. */
int	gs_screen_enum_init(P4(gs_screen_enum *, const gx_ht_order *,
			       gs_state *, gs_screen_halftone *));

/* Update the phase cache in the graphics state */
void	gx_ht_set_phase(P1(gs_state *));

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

#ifndef gx_ht_tile_DEFINED
#  define gx_ht_tile_DEFINED
typedef struct gx_ht_tile_s gx_ht_tile;
#endif

struct gx_ht_tile_s {
	gx_tile_bitmap tile;		/* the currently rendered tile */
	int level;			/* the cached gray level, i.e. */
					/* the number of spots whitened, */
					/* or -1 if the cache is empty */
	uint index;			/* the index of the tile within */
					/* the cache (for GC) */
};

struct gx_ht_cache_s {
	/* The following are set when the cache is created. */
	byte *bits;			/* the base of the bits */
	uint bits_size;			/* the space available for bits */
	gx_ht_tile *tiles;		/* the base of the tiles */
	uint num_tiles;			/* the number of tiles allocated */
	/* The following are reset each time the cache is initialized */
	/* for a new screen. */
	gx_ht_order order;		/* the cached order vector */
	int num_cached;			/* actual # of cached tiles */
	int levels_per_tile;		/* # of levels per cached tile */
	gx_bitmap_id base_id;		/* the base id, to which */
					/* we add the halftone level */
};
#define private_st_ht_cache()	/* in gxht.c */\
  gs_private_st_ptrs4(st_ht_cache, gx_ht_cache, "ht cache",\
    ht_cache_enum_ptrs, ht_cache_reloc_ptrs,\
    bits, tiles, order.levels, order.bits)

/* Compute a fractional color for dithering, the correctly rounded */
/* quotient f * max_gx_color_value / maxv. */
#define frac_color_(f, maxv)\
  (gx_color_value)(((f) * (0xffffL * 2) + maxv) / (maxv * 2))
extern const gx_color_value _ds *fc_color_quo[8];
#define fractional_color(f, maxv)\
  ((maxv) <= 7 ? fc_color_quo[maxv][f] : frac_color_(f, maxv))

/* ------ Halftone cache procedures ------ */

/* Allocate a halftone cache. */
extern const uint
	ht_cache_default_max_tiles,
	ht_cache_default_max_bits;
gx_ht_cache *gx_ht_alloc_cache(P3(gs_memory_t *, uint, uint));

/* Clear a halftone cache. */
#define gx_ht_clear_cache(pcache)\
  ((pcache)->order.levels = 0, (pcache)->order.bits = 0)

/* Initialize a halftone cache with a given order. */
void gx_ht_init_cache(P2(gx_ht_cache *, const gx_ht_order *));

/* Install a halftone in the graphics state. */
int gx_ht_install(P3(gs_state *,
		     const gs_halftone *, const gx_device_halftone *));

/* Make the cache order current, and return whether */
/* there is room for all possible tiles in the cache. */
bool gx_check_tile_cache(P1(gs_state *));

/* Determine whether a given (width, y, height) might fit into a */
/* single tile. If so, return the byte offset of the appropriate row */
/* from the beginning of the tile; if not, return -1. */
int gx_check_tile_size(P4(gs_state *pgs, int w, int y, int h));
