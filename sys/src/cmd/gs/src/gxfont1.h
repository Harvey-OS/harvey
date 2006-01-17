/* Copyright (C) 1994, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfont1.h,v 1.13 2004/09/22 13:52:33 igor Exp $ */
/* Type 1 / Type 2 font data definition */

#ifndef gxfont1_INCLUDED
#  define gxfont1_INCLUDED

#include "gstype1.h"		/* for charstring_interpret_proc */
#include "gxfixed.h"

/*
 * This is the type-specific information for Adobe Type 1 fonts.
 * It also includes the information for Type 2 fonts, because
 * there isn't very much of it and it's less trouble to include here.
 */

#ifndef gs_font_type1_DEFINED
#  define gs_font_type1_DEFINED
typedef struct gs_font_type1_s gs_font_type1;
#endif

/*
 * The zone_table values should be ints, according to the Adobe
 * specification, but some fonts have arbitrary floats here.
 */
#define zone_table(size)\
	struct {\
		int count;\
		float values[(size)*2];\
	}
#define float_array(size)\
	struct {\
		int count;\
		float values[size];\
	}
#define stem_table(size)\
	float_array(size)

#ifndef gs_type1_data_DEFINED
#define gs_type1_data_DEFINED
typedef struct gs_type1_data_s gs_type1_data;
#endif

typedef struct gs_type1_data_procs_s {

    /* Get the data for any glyph.  Return >= 0 or < 0 as usual. */

    int (*glyph_data)(gs_font_type1 * pfont, gs_glyph glyph,
		      gs_glyph_data_t *pgd);

    /* Get the data for a Subr.  Return like glyph_data. */

    int (*subr_data)(gs_font_type1 * pfont, int subr_num, bool global,
		     gs_glyph_data_t *pgd);

    /*
     * Get the data for a seac character, including the glyph and/or the
     * outline data.  Any of the pointers for the return values may be 0,
     * indicating that the corresponding value is not needed.
     * Return like glyph_data.
     */

    int (*seac_data)(gs_font_type1 * pfont, int ccode,
		     gs_glyph * pglyph, gs_const_string *gstr, gs_glyph_data_t *pgd);

    /*
     * Push (a) value(s) onto the client ('PostScript') stack during
     * interpretation.  Note that this procedure and the next one take a
     * closure pointer, not the font pointer, as the first argument.
     */

    int (*push_values)(void *callback_data, const fixed *values,
		       int count);

    /* Pop a value from the client stack. */

    int (*pop_value)(void *callback_data, fixed *value);

} gs_type1_data_procs_t;

/*
 * The garbage collector really doesn't want the client data pointer
 * from a gs_type1_state to point to the gs_type1_data in the middle of
 * a gs_font_type1, so we make the client data pointer (which is passed
 * to the callback procedures) point to the gs_font_type1 itself.
 */
struct gs_type1_data_s {
    /*int PaintType; *//* in gs_font_common */
    gs_type1_data_procs_t procs;
    charstring_interpret_proc((*interpret));
    void *proc_data;		/* data for procs */
    gs_font_base *parent;	/* the type 9 font, if this font is is a type 9 descendent. */
    int lenIV;			/* -1 means no encryption */
				/* (undocumented feature!) */
    uint subroutineNumberBias;	/* added to operand of callsubr */
				/* (undocumented feature!) */
	/* Type 2 additions */
    uint gsubrNumberBias;	/* added to operand of callgsubr */
    long initialRandomSeed;
    fixed defaultWidthX;
    fixed nominalWidthX;
	/* End of Type 2 additions */
    /* For a description of the following hint information, */
    /* see chapter 5 of the "Adobe Type 1 Font Format" book. */
    int BlueFuzz;
    float BlueScale;
    float BlueShift;
#define max_BlueValues 7
          zone_table(max_BlueValues) BlueValues;
    float ExpansionFactor;
    bool ForceBold;
#define max_FamilyBlues 7
    zone_table(max_FamilyBlues) FamilyBlues;
#define max_FamilyOtherBlues 5
    zone_table(max_FamilyOtherBlues) FamilyOtherBlues;
    int LanguageGroup;
#define max_OtherBlues 5
    zone_table(max_OtherBlues) OtherBlues;
    bool RndStemUp;
    stem_table(1) StdHW;
    stem_table(1) StdVW;
#define max_StemSnap 12
    stem_table(max_StemSnap) StemSnapH;
    stem_table(max_StemSnap) StemSnapV;
    /* Additional information for Multiple Master fonts */
#define max_WeightVector 16
    float_array(max_WeightVector) WeightVector;
};

#define gs_type1_data_s_DEFINED

struct gs_font_type1_s {
    gs_font_base_common;
    gs_type1_data data;
};

extern_st(st_gs_font_type1);
#define public_st_gs_font_type1()	/* in gstype1.c */\
  gs_public_st_suffix_add2_final(st_gs_font_type1, gs_font_type1,\
    "gs_font_type1", font_type1_enum_ptrs, font_type1_reloc_ptrs,\
    gs_font_finalize, st_gs_font_base, data.parent, data.proc_data)

/* Export font procedures so they can be called from the interpreter. */
font_proc_glyph_info(gs_type1_glyph_info);

/*
 * If a Type 1 character is defined with 'seac', store the character codes
 * in chars[0] and chars[1] and return 1; otherwise, return 0 or <0.
 * This is exported only for the benefit of font copying.
 */
int gs_type1_piece_codes(/*const*/ gs_font_type1 *pfont,
			 const gs_glyph_data_t *pgd, gs_char *chars);

#endif /* gxfont1_INCLUDED */
