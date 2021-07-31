/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ifont1.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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

/*
 * Parse the substructures of a CharString-based font.
 */
int charstring_font_get_refs(P2(os_ptr op, charstring_font_refs_t *pfr));

/*
 * Finish building a CharString-based font.  The client has filled in
 * pdata1->interpret, subroutineNumberBias, lenIV, and (if applicable)
 * the Type 2 elements.
 */
int build_charstring_font(P7(i_ctx_t *i_ctx_p, os_ptr op,
			     build_proc_refs * pbuild, font_type ftype,
			     charstring_font_refs_t *pfr,
			     gs_type1_data *pdata1,
			     build_font_options_t options));

#endif /* ifont1_INCLUDED */
