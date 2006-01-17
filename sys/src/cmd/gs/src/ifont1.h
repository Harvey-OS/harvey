/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ifont1.h,v 1.7 2004/08/04 19:36:12 stefan Exp $ */
/* Type 1 font utilities shared with Type 2 */

#ifndef ifont1_INCLUDED
#  define ifont1_INCLUDED

/*
 * Define the temporary structure for holding pointers to substructures of a
 * CharString-based font.  This is used for parsing Type 1, 2, and 4 fonts.
 */
typedef struct charstring_font_refs_s {
    ref *Private;
    ref no_subrs;
    ref *OtherSubrs;
    ref *Subrs;
    ref *GlobalSubrs;
} charstring_font_refs_t;

/* Define the default lenIV value for a Type 1 font. */
#define DEFAULT_LENIV_1 4

/*
 * Parse the substructures of a CharString-based font.
 */
int charstring_font_get_refs(const_os_ptr op, charstring_font_refs_t *pfr);

/*
 * Get the parameters of a CharString-based font or a FDArray entry for a
 * CIDFontType 0 font.  The client has filled in pdata1->interpret,
 * subroutineNumberBias, lenIV, and (if applicable) the Type 2 elements.
 */
int charstring_font_params(const gs_memory_t *mem, 
			   const_os_ptr op, charstring_font_refs_t *pfr,
			   gs_type1_data *pdata1);

/*
 * Fill in a newly built CharString-based font or FDArray entry.
 */
int charstring_font_init(gs_font_type1 *pfont,
			 const charstring_font_refs_t *pfr,
			 const gs_type1_data *pdata1);

/*
 * Finish building a CharString-based font.  The client has filled in the
 * same elements as for charstring_font_params.
 */
int build_charstring_font(i_ctx_t *i_ctx_p, os_ptr op,
			  build_proc_refs * pbuild, font_type ftype,
			  charstring_font_refs_t *pfr,
			  gs_type1_data *pdata1,
			  build_font_options_t options);

#endif /* ifont1_INCLUDED */
