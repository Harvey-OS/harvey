/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxfont0.h */
/* Type 0 (composite) font data definition */

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
	fmap_shift = 8
} fmap_type;
#define fmap_type_min 2
#define fmap_type_max 8
#define fmap_type_is_modal(fmt)\
  ((fmt) == fmap_escape || (fmt) == fmap_double_escape || (fmt) == fmap_shift)

/* This is the type-specific information for a type 0 (composite) gs_font. */
typedef struct gs_type0_data_s {
	fmap_type FMapType;
	byte EscChar, ShiftIn, ShiftOut;
	gs_const_string SubsVector;
	  uint subs_size;		/* bytes per entry */
	  uint subs_width;		/* # of entries */
	uint *Encoding;
	  uint encoding_size;
	gs_font **FDepVector;
	  uint fdep_size;
} gs_type0_data;

typedef struct gs_font_type0_s {
	gs_font_common;
	gs_type0_data data;
} gs_font_type0;
extern_st(st_gs_font_type0);
#define public_st_gs_font_type0()	/* in gsfont0.c */\
  gs_public_st_composite(st_gs_font_type0, gs_font_type0, "gs_font_type0",\
    font_type0_enum_ptrs, font_type0_reloc_ptrs)
