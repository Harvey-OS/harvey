/* Copyright (C) 1996, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfont42.h,v 1.21 2005/02/27 05:56:59 ray Exp $ */
/* Type 42 font data definition */

#ifndef gxfont42_INCLUDED
#  define gxfont42_INCLUDED

#ifndef gs_glyph_cache_DEFINED
#  define gs_glyph_cache_DEFINED
typedef struct gs_glyph_cache_s gs_glyph_cache;
#endif

#ifndef cached_fm_pair_DEFINED
#  define cached_fm_pair_DEFINED
typedef struct cached_fm_pair_s cached_fm_pair;
#endif

/* This is the type-specific information for a Type 42 (TrueType) font. */
#ifndef gs_type42_data_DEFINED
#define gs_type42_data_DEFINED
typedef struct gs_type42_data_s gs_type42_data;
#endif
#ifndef gs_font_type42_DEFINED
#  define gs_font_type42_DEFINED
typedef struct gs_font_type42_s gs_font_type42;
#endif
typedef struct gs_type42_mtx_s {
    uint numMetrics;		/* num*Metrics from [hv]hea */
    ulong offset;		/* offset to [hv]mtx table */
    uint length;		/* length of [hv]mtx table */
} gs_type42_mtx_t;
struct gs_type42_data_s {
    /* The following are set by the client. */
    int (*string_proc) (gs_font_type42 *, ulong, uint, const byte **);
    void *proc_data;		/* data for procedures */
    /*
     * The following are initialized by ...font_init, but may be reset by
     * the client.
     */
    uint (*get_glyph_index)(gs_font_type42 *pfont, gs_glyph glyph);
    int (*get_outline)(gs_font_type42 *pfont, uint glyph_index,
		       gs_glyph_data_t *pgd);
    int (*get_metrics)(gs_font_type42 *pfont, uint glyph_index, int wmode,
		       float sbw[4]);
    /* The following are cached values. */
    ulong cmap;			/* offset to cmap table (not used by */
				/* renderer, only here for clients) */
    ulong glyf;			/* offset to glyf table */
    uint unitsPerEm;		/* from head */
    uint indexToLocFormat;	/* from head */
    gs_type42_mtx_t metrics[2];	/* hhea/hmtx, vhea/vmtx (indexed by WMode) */
    ulong loca;			/* offset to loca table */
    /*
     * TrueType fonts specify the number of glyphs in two different ways:
     * the size of the loca table, and an explicit value in maxp.  Currently
     * the value of numGlyphs in this structure is computed from the size of
     * loca.  This is wrong: incrementally downloaded TrueType (or
     * CIDFontType 2) fonts will have no loca table, but will have a
     * reasonable glyph count in maxp.  Unfortunately, a significant amount
     * of code now depends on the incorrect definition of numGlyphs.
     * Therefore, rather than run the risk of introducing bugs by changing
     * the definition and/or by changing the name of the data member, we add
     * another member trueNumGlyphs to hold the value from maxp.
     */
    uint numGlyphs;		/* from size of loca */
    uint trueNumGlyphs;		/* from maxp */
    uint *len_glyphs;		/* built from the loca table */
    gs_glyph_cache *gdcache;
    bool warning_patented;
    bool warning_bad_instruction;
};
#define gs_font_type42_common\
    gs_font_base_common;\
    gs_type42_data data
struct gs_font_type42_s {
    gs_font_type42_common;
};

extern_st(st_gs_font_type42);
#define public_st_gs_font_type42()	/* in gstype42.c */\
  gs_public_st_suffix_add3_final(st_gs_font_type42, gs_font_type42,\
    "gs_font_type42", font_type42_enum_ptrs, font_type42_reloc_ptrs,\
    gs_font_finalize, st_gs_font_base, data.proc_data, data.len_glyphs, \
    data.gdcache)

/*
 * Because a Type 42 font contains so many cached values,
 * we provide a procedure to initialize them from the font data.
 * Note that this initializes the type42_data procedures other than
 * string_proc, and the font procedures as well.
 */
int gs_type42_font_init(gs_font_type42 *);

/* Append the outline of a TrueType character to a path. */
int gs_type42_append(uint glyph_index, gs_imager_state * pis,
		 gx_path * ppath, const gs_log2_scale_point * pscale,
		 bool charpath_flag, int paint_type, cached_fm_pair *pair);

/* Get the metrics of a TrueType character. */
int gs_type42_get_metrics(gs_font_type42 * pfont, uint glyph_index,
			  float psbw[4]);
int gs_type42_wmode_metrics(gs_font_type42 * pfont, uint glyph_index,
			    int wmode, float psbw[4]);
/* Export the default get_metrics procedure. */
int gs_type42_default_get_metrics(gs_font_type42 *pfont, uint glyph_index,
				  int wmode, float sbw[4]);

int gs_type42_get_outline_from_TT_file(gs_font_type42 * pfont, stream *s, uint glyph_index,
		gs_glyph_data_t *pgd);

/* Export the font procedures so they can be called from the interpreter. */
font_proc_enumerate_glyph(gs_type42_enumerate_glyph);
font_proc_glyph_info(gs_type42_glyph_info);
font_proc_glyph_outline(gs_type42_glyph_outline);

/* Get glyph info by glyph index. */
int gs_type42_glyph_info_by_gid(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		     int members, gs_glyph_info_t *info, uint glyph_index);

#endif /* gxfont42_INCLUDED */
