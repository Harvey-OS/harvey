/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxfont42.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Type 42 font data definition */

#ifndef gxfont42_INCLUDED
#  define gxfont42_INCLUDED

/* This is the type-specific information for a Type 42 (TrueType) font. */
typedef struct gs_type42_data_s gs_type42_data;
#ifndef gs_font_type42_DEFINED
#  define gs_font_type42_DEFINED
typedef struct gs_font_type42_s gs_font_type42;
#endif
struct gs_type42_data_s {
    /* The following are set by the client. */
    int (*string_proc) (P4(gs_font_type42 *, ulong, uint, const byte **));
    void *proc_data;		/* data for procedures */
    /* The following are initialized by ...font_init, */
    /* but may be reset by the client. */
    int (*get_outline) (P3(gs_font_type42 *, uint, gs_const_string *));
    /* The following are cached values. */
    ulong glyf;			/* offset to glyf table */
    uint unitsPerEm;		/* from head */
    uint indexToLocFormat;	/* from head */
    uint numLongMetrics;	/* from hhea */
    ulong hmtx;			/* offset to hmtx table */
    uint hmtx_length;		/* length of hmtx table */
    ulong loca;			/* offset to loca table */
    uint numGlyphs;		/* from size of loca */
};
struct gs_font_type42_s {
    gs_font_base_common;
    gs_type42_data data;
};

extern_st(st_gs_font_type42);
#define public_st_gs_font_type42()	/* in gstype42.c */\
  gs_public_st_suffix_add1_final(st_gs_font_type42, gs_font_type42,\
    "gs_font_type42", font_type42_enum_ptrs, font_type42_reloc_ptrs,\
    gs_font_finalize, st_gs_font_base, data.proc_data)

/*
 * Because a Type 42 font contains so many cached values,
 * we provide a procedure to initialize them from the font data.
 * Note that this initializes get_outline and the font procedures as well.
 */
int gs_type42_font_init(P1(gs_font_type42 *));

/* Append the outline of a TrueType character to a path. */
int gs_type42_append(P7(uint glyph_index, gs_imager_state * pis,
			gx_path * ppath, const gs_log2_scale_point * pscale,
			bool charpath_flag, int paint_type,
			gs_font_type42 * pfont));

/* Get the metrics of a TrueType character. */
int gs_type42_get_metrics(P3(gs_font_type42 * pfont, uint glyph_index,
			     float psbw[4]));

/* Export the font procedures so they can be called from the interpreter. */
font_proc_enumerate_glyph(gs_type42_enumerate_glyph);
font_proc_glyph_info(gs_type42_glyph_info);
font_proc_glyph_outline(gs_type42_glyph_outline);

#endif /* gxfont42_INCLUDED */
