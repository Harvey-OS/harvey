/* Copyright (C) 1989, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxchar.h,v 1.14 2003/08/11 19:14:07 igor Exp $ */
/* Internal character definition for Ghostscript library */
/* Requires gsmatrix.h, gxfixed.h */

#ifndef gxchar_INCLUDED
#  define gxchar_INCLUDED

#include "gschar.h"
#include "gxtext.h"

/* The type of cached characters is opaque. */
#ifndef cached_char_DEFINED
#  define cached_char_DEFINED
typedef struct cached_char_s cached_char;
#endif

/* The type of cached font/matrix pairs is opaque. */
#ifndef cached_fm_pair_DEFINED
#  define cached_fm_pair_DEFINED
typedef struct cached_fm_pair_s cached_fm_pair;
#endif

/* The type of font objects is opaque. */
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif

/* The type of text enum objects is opaque. */
#ifndef gs_text_enum_DEFINED
#  define gs_text_enum_DEFINED
typedef struct gs_text_enum_s gs_text_enum_t;
#endif

/* The types of memory and null devices may be opaque. */
#ifndef gx_device_memory_DEFINED
#  define gx_device_memory_DEFINED
typedef struct gx_device_memory_s gx_device_memory;
#endif
#ifndef gx_device_null_DEFINED
#  define gx_device_null_DEFINED
typedef struct gx_device_null_s gx_device_null;
#endif

/* An enumeration object for string display. */
typedef enum {
    sws_none,
    sws_cache,			/* setcachedevice[2] */
    sws_no_cache,		/* setcharwidth */
    sws_cache_width_only,	/* setcharwidth for xfont char */
    sws_retry			/* retry setcachedevice[2] */
} show_width_status;
struct gs_show_enum_s {
    /* Put this first for subclassing. */
    gs_text_enum_common;	/* (procs, text, index) */
    /* Following are set at creation time */
    bool auto_release;		/* true if old API, false if new */
    gs_state *pgs;
    int level;			/* save the level of pgs */
    gs_char_path_mode charpath_flag;
    gs_state *show_gstate;	/* for setting pgs->show_gstate */
				/* at returns/callouts */
    int can_cache;		/* -1 if can't use cache at all, */
				/* 0 if can read but not load, */
				/* 1 if can read and load */
    gs_int_rect ibox;		/* int version of quick-check */
				/* (inner) clipping box */
    gs_int_rect obox;		/* int version of (outer) clip box */
    int ftx, fty;		/* transformed font translation */
    /* Following are updated dynamically */
    gs_glyph (*encode_char)(gs_font *, gs_char, gs_glyph_space_t);  /* copied from font */
    gs_log2_scale_point fapi_log2_scale; /* scaling factors for oversampling with FAPI, -1 = not valid */
    gs_point fapi_glyph_shift;          /* glyph shift for FAPI-handled font */
    gx_device_memory *dev_cache;	/* cache device */
    gx_device_memory *dev_cache2;	/* underlying alpha memory device, */
				/* if dev_cache is an alpha buffer */
    gx_device_null *dev_null;	/* null device for stringwidth */
    /*uint index; */		/* index within string */
    /*uint xy_index;*/		/* index within X/Y widths */
    /*gs_char returned.current_char;*/	/* current char for render or move */
    /*gs_glyph returned.current_glyph;*/	/* current glyph ditto */
    gs_fixed_point wxy;		/* width of current char */
				/* in device coords */
    gs_fixed_point origin;	/* unrounded origin of current char */
				/* in device coords, needed for */
				/* charpath and WMode=1 */
    cached_char *cc;		/* being accumulated */
    /*gs_point returned.total_width;*/		/* total width of string, set at end */
    show_width_status width_status;
    /*gs_log2_scale_point log2_scale;*/
    int (*continue_proc) (gs_show_enum *);	/* continuation procedure */
};
#define gs_show_enum_s_DEFINED
/* The structure descriptor is public for gschar.c. */
#define public_st_gs_show_enum() /* in gxchar.c */\
  gs_public_st_composite(st_gs_show_enum, gs_show_enum, "gs_show_enum",\
    show_enum_enum_ptrs, show_enum_reloc_ptrs)

/* Get the current character code. */
int gx_current_char(const gs_text_enum_t * pte);

/* Cached character procedures (in gxccache.c and gxccman.c) */
#ifndef gs_font_dir_DEFINED
#  define gs_font_dir_DEFINED
typedef struct gs_font_dir_s gs_font_dir;

#endif
cached_char *
            gx_alloc_char_bits(gs_font_dir *, gx_device_memory *, gx_device_memory *, ushort, ushort, const gs_log2_scale_point *, int);
void gx_open_cache_device(gx_device_memory *, cached_char *);
void gx_free_cached_char(gs_font_dir *, cached_char *);
void gx_add_cached_char(gs_font_dir *, gx_device_memory *, cached_char *, cached_fm_pair *, const gs_log2_scale_point *);
void gx_add_char_bits(gs_font_dir *, cached_char *, const gs_log2_scale_point *);
cached_char *
            gx_lookup_cached_char(const gs_font *, const cached_fm_pair *, gs_glyph, int, int, gs_fixed_point *);
cached_char *
            gx_lookup_xfont_char(const gs_state *, cached_fm_pair *, gs_char, gs_glyph, int);
int gx_image_cached_char(gs_show_enum *, cached_char *);
void gx_compute_text_oversampling(const gs_show_enum * penum, const gs_font *pfont,
				  int alpha_bits, gs_log2_scale_point *p_log2_scale);
int set_char_width(gs_show_enum *penum, gs_state *pgs, floatp wx, floatp wy);
int gx_default_text_restore_state(gs_text_enum_t *pte);
int gx_hld_stringwidth_begin(gs_imager_state * pis, gx_path **path);

#endif /* gxchar_INCLUDED */
