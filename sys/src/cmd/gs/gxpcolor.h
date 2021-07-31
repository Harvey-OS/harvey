/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxpcolor.h */
/* Internal definitions for Pattern colors */
/* Requires gxdevice.h, gxdevmem.h, gxcolor2.h, gxdcolor.h */

/*
 * Define the Pattern device color types.  There is one type for
 * colored patterns, and one uncolored pattern type for each non-Pattern
 * device color type.
 */
extern const gx_device_color_procs
	gx_dc_pattern, gx_dc_pure_masked,
	gx_dc_binary_masked, gx_dc_colored_masked;

/*
 * Define a color tile, an entry in the rendered Pattern cache (and
 * eventually in the colored halftone cache).  Note that the depth is
 * not sufficient to ensure that the rendering matches a given device;
 * however, we don't currently have an object that represents the
 * abstraction of a 'color representation'.
 */
struct gx_color_tile_s {
		/* The following are the 'key' in the cache. */
	gx_bitmap_id id;
	int depth;
		/* The following are the 'value'. */
	int tiling_type;		/* TilingType */
	gs_fixed_point xstep, ystep;	/* in device coordinates; */
		/* xstep.x > 0, ystep.y > xstep.y >= 0, ystep.x = 0 */
	gx_tile_bitmap bits;		/* data = 0 if uncolored */
	gx_tile_bitmap mask;		/* data = 0 if no mask */
					/* (i.e., the mask is all 1's) */
	bool is_simple;			/* true if xstep/ystep = tile size */
		/* The following is neither key nor value. */
	uint index;			/* the index of the tile within */
					/* the cache (for GC) */
};
#define private_st_color_tile()	/* in gxpcmap.c */\
  gs_private_st_ptrs2(st_color_tile, gx_color_tile, "gx_color_tile",\
    color_tile_enum_ptrs, color_tile_reloc_ptrs, bits.data, mask.data)
#define private_st_color_tile_element()	/* in gxpcmap.c */\
  gs_private_st_element(st_color_tile_element, gx_color_tile,\
    "gx_color_tile[]", color_tile_elt_enum_ptrs, color_tile_elt_reloc_ptrs,\
    st_color_tile)

/*
 * Define a cache for rendered Patterns.  This is currently an open
 * hash table with linear reprobing and round-robin replacement.
 * Obviously, we can do better in both areas.
 */
typedef struct gx_pattern_cache_s {
	gs_memory_t *memory;
	gx_color_tile *tiles;
	uint num_tiles;
	uint tiles_used;
	uint next;		/* round-robin index */
	ulong bits_used;
	ulong max_bits;
} gx_pattern_cache;
#define private_st_pattern_cache() /* in gxpcmap.c */\
  gs_private_st_ptrs1(st_pattern_cache, gx_pattern_cache,\
    "gx_pattern_cache", pattern_cache_enum, pattern_cache_reloc, tiles)

/* Allocate a Pattern cache. */
extern const uint pattern_cache_default_max_tiles;
extern const ulong pattern_cache_default_max_bits;
gx_pattern_cache *gx_pattern_alloc_cache(P3(gs_memory_t *, uint, ulong));

/* Get or set the Pattern cache in a gstate. */
gx_pattern_cache *gstate_pattern_cache(P1(gs_state *));
void gstate_set_pattern_cache(P2(gs_state *, gx_pattern_cache *));

/*
 * Define a device for accumulating the rendering of a Pattern.
 * This is actually a wrapper for two other devices: one that accumulates
 * the actual pattern image (if this is a colored pattern), and one that
 * accumulates a mask defining which pixels in the image are set.
 */
typedef struct gx_device_pattern_accum_s {
	gx_device_forward_common;
		/* Client sets these before opening */
	gs_memory_t *bitmap_memory;
	const gs_pattern_instance *instance;
		/* open sets these */
	gx_device_memory *bits;		/* target also points to bits */
	gx_device_memory *mask;
} gx_device_pattern_accum;
#define private_st_device_pattern_accum() /* in gxpcmap.c */\
  gs_private_st_suffix_add3(st_device_pattern_accum, gx_device_pattern_accum,\
    "pattern accumulator", pattern_accum_enum, pattern_accum_reloc,\
    st_device_forward, instance, bits, mask)

/* Allocate a pattern accumulator. */
gx_device_pattern_accum *gx_pattern_accum_alloc(P2(gs_memory_t *memory, client_name_t));

/* Add an accumulated pattern to the cache. */
int gx_pattern_cache_add_entry(P3(gs_state *, gx_device_pattern_accum *,
  gx_color_tile **));

/* Look up a pattern color in the cache. */
bool gx_pattern_cache_lookup(P2(gx_device_color *, const gs_state *));
