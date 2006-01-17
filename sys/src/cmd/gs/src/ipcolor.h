/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ipcolor.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Interpreter definitions for Pattern color */

#ifndef ipcolor_INCLUDED
#  define ipcolor_INCLUDED

/*
 * Define the structure for remembering the pattern dictionary.
 * This is the "client data" in the template.
 * See zgstate.c (int_gstate) or zfont2.c (font_data) for information
 * as to why we define this as a structure rather than a ref array.
 */
typedef struct int_pattern_s {
    ref dict;
} int_pattern;

#define private_st_int_pattern()	/* in zpcolor.c */\
  gs_private_st_ref_struct(st_int_pattern, int_pattern, "int_pattern")

/* Create an interpreter pattern structure. */
int int_pattern_alloc(int_pattern **ppdata, const ref *op,
		      gs_memory_t *mem);

#endif /* ipcolor_INCLUDED */
