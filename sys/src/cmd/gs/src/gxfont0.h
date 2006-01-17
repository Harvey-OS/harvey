/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfont0.h,v 1.6 2002/06/24 17:27:30 lpd Exp $ */
/* Type 0 (composite) font data definition */

#ifndef gxfont0_INCLUDED
#  define gxfont0_INCLUDED

/* Define the composite font mapping types. */
/* These numbers must be the same as the values of FMapType */
/* in type 0 font dictionaries. */
typedef enum {
    fmap_8_8 = 2,
    fmap_escape = 3,
    fmap_1_7 = 4,
    fmap_9_7 = 5,
    fmap_SubsVector = 6,
    fmap_double_escape = 7,
    fmap_shift = 8,
    fmap_CMap = 9
} fmap_type;

#define fmap_type_min 2
#define fmap_type_max 9
#define fmap_type_is_modal(fmt)\
  ((fmt) == fmap_escape || (fmt) == fmap_double_escape || (fmt) == fmap_shift)

/* This is the type-specific information for a type 0 (composite) gs_font. */
#ifndef gs_cmap_DEFINED
#  define gs_cmap_DEFINED
typedef struct gs_cmap_s gs_cmap_t;
#endif
typedef struct gs_type0_data_s {
    fmap_type FMapType;
    byte EscChar, ShiftIn, ShiftOut;
    gs_const_string SubsVector;	/* fmap_SubsVector only */
    uint subs_size;		/* bytes per entry */
    uint subs_width;		/* # of entries */
    uint *Encoding;
    uint encoding_size;
    gs_font **FDepVector;
    uint fdep_size;
    const gs_cmap_t *CMap;	/* fmap_CMap only */
} gs_type0_data;

#define gs_type0_data_max_ptrs 3

#ifndef gs_font_type0_DEFINED
#  define gs_font_type0_DEFINED
typedef struct gs_font_type0_s gs_font_type0;
#endif

struct gs_font_type0_s {
    gs_font_common;
    gs_type0_data data;
};

extern_st(st_gs_font_type0);
#define public_st_gs_font_type0()	/* in gsfont0.c */\
  gs_public_st_complex_only(st_gs_font_type0, gs_font_type0, "gs_font_type0",\
    0, font_type0_enum_ptrs, font_type0_reloc_ptrs, gs_font_finalize)

/* Define the Type 0 font procedures. */
font_proc_define_font(gs_type0_define_font);
font_proc_make_font(gs_type0_make_font);
font_proc_init_fstack(gs_type0_init_fstack);
font_proc_next_char_glyph(gs_type0_next_char_glyph);

#endif /* gxfont0_INCLUDED */
