/* Copyright (C) 1992, 1995, 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfcache.h,v 1.27 2004/08/19 19:33:09 stefan Exp $ */
/* Font and character cache definitions and procedures */
/* Requires gsfont.h */

#ifndef gxfcache_INCLUDED
#  define gxfcache_INCLUDED

#include "gsccode.h"
#include "gsuid.h"
#include "gsxfont.h"
#include "gxbcache.h"
#include "gxfixed.h"
#include "gxftype.h"

/* ------ Font/matrix pair cache entry ------ */

#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif
#ifndef cached_fm_pair_DEFINED
#  define cached_fm_pair_DEFINED
typedef struct cached_fm_pair_s cached_fm_pair;
#endif
#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

#ifndef ttfFont_DEFINED
#  define ttfFont_DEFINED
typedef struct ttfFont_s ttfFont;
#endif
#ifndef gx_ttfReader_DEFINED
#  define gx_ttfReader_DEFINED
typedef struct gx_ttfReader_s gx_ttfReader;
#endif
#ifndef ttfInterpreter_DEFINED
#  define ttfInterpreter_DEFINED
typedef struct ttfInterpreter_s ttfInterpreter;
#endif
#ifndef gx_device_spot_analyzer_DEFINED
#   define gx_device_spot_analyzer_DEFINED
typedef struct gx_device_spot_analyzer_s gx_device_spot_analyzer;
#endif
#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;
#endif


/*
 * Define the entry for a cached (font,matrix) pair.  If the UID
 * is valid, the font pointer may be 0, since we keep entries even for
 * fonts unloaded by a restore if they have valid UIDs; in this case,
 * we also need the FontType as part of the key.
 * Note that because of the dependency on StrokeWidth, we can't cache
 * fonts with non-zero PaintType.
 * We can't use the address of the pair for the hash value,
 * since the GC may move pairs in storage, so we create a hash
 * when we allocate the pair initially.
 */
struct cached_fm_pair_s {
    gs_font *font;		/* base font */
    gs_uid UID;			/* font UniqueID or XUID */
    font_type FontType;		/* (part of key if UID is valid) */
    uint hash;			/* hash for this pair */
    float mxx, mxy, myx, myy;	/* transformation */
    int num_chars;		/* # of cached chars with this */
    /* f/m pair */
    bool xfont_tried;		/* true if we looked up an xfont */
    gx_xfont *xfont;		/* the xfont (if any) */
    gs_memory_t *memory;	/* the allocator for the xfont */
    uint index;			/* index of this pair in mdata */
    ttfFont *ttf;		/* True Type interpreter data. */
    gx_ttfReader *ttr;		/* True Type interpreter data. */
    bool design_grid;           /* A charpath font face.  */
};

#define private_st_cached_fm_pair() /* in gxccman.c */\
  gs_private_st_ptrs5(st_cached_fm_pair, cached_fm_pair,\
    "cached_fm_pair", fm_pair_enum_ptrs, fm_pair_reloc_ptrs,\
    font, UID.xvalues, xfont, ttf, ttr)
#define private_st_cached_fm_pair_elt()	/* in gxccman.c */\
  gs_private_st_element(st_cached_fm_pair_element, cached_fm_pair,\
    "cached_fm_pair[]", fm_pair_element_enum_ptrs, fm_pair_element_reloc_ptrs,\
    st_cached_fm_pair)
/* If font == 0 and UID is invalid, this is a free entry. */
#define fm_pair_is_free(pair)\
  ((pair)->font == 0 && !uid_is_valid(&(pair)->UID))
#define fm_pair_set_free(pair)\
  ((pair)->font = 0, uid_set_invalid(&(pair)->UID))
#define fm_pair_init(pair)\
  (fm_pair_set_free(pair), (pair)->xfont_tried = false, (pair)->xfont = 0)

/* The font/matrix pair cache itself. */
typedef struct fm_pair_cache_s {
    uint msize, mmax;		/* # of cached font/matrix pairs */
    cached_fm_pair *mdata;
    uint mnext;			/* rover for allocating font/matrix pairs */
} fm_pair_cache;

/* ------ Character cache entry ------- */

/* Define the allocation chunk type. */
typedef gx_bits_cache_chunk char_cache_chunk;

/*
 * This is a subclass of the entry in a general bitmap cache.
 * The character cache contains both used and free blocks.
 * All blocks have a common header; free blocks have ONLY the header.
 */
typedef gx_cached_bits_head cached_char_head;

#define cc_head_is_free(cch) cb_head_is_free(cch)
#define cc_head_set_free(cch) cb_head_set_free(cch)
/*
 * Define the cache entry for an individual character.
 * The bits, if any, immediately follow the structure;
 * characters with only xfont definitions may not have bits.
 * An entry is 'real' if it is not free and if pair != 0.
 * We maintain the invariant that at least one of the following must be true
 * for all real entries:
 *      - cc_has_bits(cc);
 *      - cc->xglyph != gx_no_xglyph && cc_pair(cc)->xfont != 0.
 */
#ifndef cached_char_DEFINED
#  define cached_char_DEFINED
typedef struct cached_char_s cached_char;

#endif
struct cached_char_s {

    /* The code, font/matrix pair, wmode, and depth */
    /* are the 'key' in the cache. */
    /* gx_cached_bits_common includes depth. */

    gx_cached_bits_common;	/* (must be first) */
#define cc_depth(cc) ((cc)->cb_depth)
#define cc_set_depth(cc, d) ((cc)->cb_depth = (d))
    cached_fm_pair *pair;
    bool linked;
#define cc_pair(cc) ((cc)->pair)
#define cc_set_pair_only(cc, p) ((cc)->pair = (p))
    gs_glyph code;		/* glyph code */
    byte wmode;			/* writing mode (0 or 1) */

    /* The following are neither 'key' nor 'value'. */

    char_cache_chunk *chunk;	/* chunk where this char */
    /* is allocated */
    uint loc;			/* relative location in chunk */
    uint pair_index;		/* index of pair in mdata */
    gs_fixed_point subpix_origin; /* glyph origin offset modulo pixel */

    /* The rest of the structure is the 'value'. */
    /* gx_cached_bits_common has width, height, raster, */
    /* shift (not used here), id. */

#define cc_raster(cc) ((cc)->raster)
#define cc_set_raster(cc, r) ((cc)->raster = (r))
    gx_xglyph xglyph;		/* the xglyph for the xfont, if any */
    gs_fixed_point wxy;		/* width in device coords */
    gs_fixed_point offset;	/* (-llx, -lly) in device coords */
};

#define cc_is_free(cc) cc_head_is_free(&(cc)->head)
#define cc_set_free(cc) cc_head_set_free(&(cc)->head)
#define cc_set_pair(cc, p)\
  ((cc)->pair_index = ((cc)->pair = (p))->index)
#define cc_has_bits(cc) ((cc)->id != gx_no_bitmap_id)
/*
 * Memory management for cached_chars is a little unusual.
 * cached_chars are never instantiated on their own; a pointer to
 * a cached_char points into the middle of a cache chunk.
 * Consequently, such pointers can't be traced or relocated
 * in the usual way.  What we do instead is allocate the cache
 * outside garbage-collectable space; we do all the tracing and relocating
 * of pointers *from* the cache (currently only the head.pair pointer)
 * when we trace or relocate the font "directory" that owns the cache.
 *
 * Since cached_chars are (currently) never instantiated on their own,
 * they only have a descriptor so that cached_char_ptr can trace them.
 */
#define private_st_cached_char() /* in gxccman.c */\
  gs_private_st_composite(st_cached_char, cached_char, "cached_char",\
    cached_char_enum_ptrs, cached_char_reloc_ptrs)
#define private_st_cached_char_ptr() /* in gxccman.c */\
  gs_private_st_composite(st_cached_char_ptr, cached_char *,\
    "cached_char *", cc_ptr_enum_ptrs, cc_ptr_reloc_ptrs)
#define private_st_cached_char_ptr_elt() /* in gxccman.c */\
  gs_private_st_element(st_cached_char_ptr_element, cached_char *,\
    "cached_char *[]", cc_ptr_element_enum_ptrs, cc_ptr_element_reloc_ptrs,\
    st_cached_char_ptr)

/*
 * Define the alignment and size of the cache structures.
 */
#define align_cached_char_mod align_cached_bits_mod
#define sizeof_cached_char\
  ROUND_UP(sizeof(cached_char), align_cached_char_mod)
#define cc_bits(cc) ((byte *)(cc) + sizeof_cached_char)
#define cc_const_bits(cc) ((const byte *)(cc) + sizeof_cached_char)

/* Define the hash index for a (glyph, fm_pair) key. */
#define chars_head_index(glyph, pair)\
  ((uint)(glyph) * 59 + (pair)->hash * 73)	/* scramble it a bit */

/* ------ Character cache ------ */

/*
 * So that we can find all the entries in the cache without
 * following chains of pointers, we use open hashing rather than
 * chained hashing for the lookup table.
 */
typedef struct char_cache_s {
    /* gx_bits_cache_common provides chunks, cnext, */
    /* bsize, csize. */
    gx_bits_cache_common;
    gs_memory_t *struct_memory;
    gs_memory_t *bits_memory;
    cached_char **table;	/* hash table */
    uint table_mask;		/* (a power of 2 -1) */
    uint bmax;			/* max bsize */
    uint cmax;			/* max csize */
    uint bspace;		/* space allocated for chunks */
    uint lower;			/* min size at which cached chars */
    /* should be stored compressed */
    uint upper;			/* max size of a single cached char */
    gs_glyph_mark_proc_t mark_glyph;
    void *mark_glyph_data;	/* closure data */
} char_cache;

/* ------ Font/character cache ------ */

/* A font "directory" (font/character cache manager). */
#ifndef gs_font_dir_DEFINED
#  define gs_font_dir_DEFINED
typedef struct gs_font_dir_s gs_font_dir;
#endif
struct gs_font_dir_s {

    /* Original (unscaled) fonts */

    gs_font *orig_fonts;

    /* Scaled font cache */

    gs_font *scaled_fonts;	/* list of recently scaled fonts */
    uint ssize, smax;

    /* Font/matrix pair cache */

    fm_pair_cache fmcache;

    /* Character cache */

    char_cache ccache;
    /* Scanning cache for GC */
    uint enum_index;		/* index (N) */
    uint enum_offset;		/* ccache.table[offset] is N'th non-zero entry */

    /* User parameter AlignToPixels. */
    bool align_to_pixels;

    /* A table for converting glyphs to Unicode */
    void *glyph_to_unicode_table; /* closure data */

    /* An allocator for extension structures */
    gs_memory_t *memory;
    ttfInterpreter *tti;
    /* User parameter GridFitTT. */
    uint grid_fit_tt;
    gx_device_spot_analyzer *san;
    int (*global_glyph_code)(const gs_memory_t *mem, gs_const_string *gstr, gs_glyph *pglyph);
};

#define private_st_font_dir()	/* in gsfont.c */\
  gs_private_st_composite(st_font_dir, gs_font_dir, "gs_font_dir",\
    font_dir_enum_ptrs, font_dir_reloc_ptrs)

/* Enumerate the pointers in a font directory, except for orig_fonts. */
#define font_dir_do_ptrs(m)\
  /*m(-,orig_fonts)*/ m(0,scaled_fonts) m(1,fmcache.mdata)\
  m(2,ccache.table) m(3,ccache.mark_glyph_data)\
  m(4,glyph_to_unicode_table) m(5,tti) m(6,san)
#define st_font_dir_max_ptrs 7

/* Character cache procedures (in gxccache.c and gxccman.c) */
int gx_char_cache_alloc(gs_memory_t * struct_mem, gs_memory_t * bits_mem,
			gs_font_dir * pdir, uint bmax, uint mmax,
			uint cmax, uint upper);
void gx_char_cache_init(gs_font_dir *);
void gx_purge_selected_cached_chars(gs_font_dir *, 
				    bool(*)(const gs_memory_t *, cached_char *, void *), void *);
void gx_compute_char_matrix(const gs_matrix *char_tm, const gs_log2_scale_point *log2_scale, 
    float *mxx, float *mxy, float *myx, float *myy);
void gx_compute_ccache_key(gs_font * pfont, const gs_matrix *char_tm, 
    const gs_log2_scale_point *log2_scale, bool design_grid,
    float *mxx, float *mxy, float *myx, float *myy);
int gx_lookup_fm_pair(gs_font * pfont, const gs_matrix *char_tm, 
    const gs_log2_scale_point *log2_scale, bool design_grid, cached_fm_pair **ppair);
int gx_add_fm_pair(register gs_font_dir * dir, gs_font * font, const gs_uid * puid,
	       const gs_matrix * char_tm, const gs_log2_scale_point *log2_scale,
	       bool design_grid, cached_fm_pair **ppair);
void gx_lookup_xfont(const gs_state *, cached_fm_pair *, int);
void gs_purge_fm_pair(gs_font_dir *, cached_fm_pair *, int);
void gs_purge_font_from_char_caches(gs_font_dir *, const gs_font *);

#endif /* gxfcache_INCLUDED */
