/* Copyright (C) 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ifont42.h,v 1.9 2004/08/04 19:36:12 stefan Exp $ */
/* Procedure for building a Type 42 or CIDFontType 2 font */

#ifndef ifont42_INCLUDED
#  define ifont42_INCLUDED

/* Build a type 11 (TrueType CID-keyed) or 42 (TrueType) font. */
int build_gs_TrueType_font(i_ctx_t *, os_ptr, gs_font_type42 **, font_type,
			   gs_memory_type_ptr_t, const char *, const char *,
			   build_font_options_t);

/*
 * Check a parameter for being an array of strings.  Return the parameter
 * value even if it is of the wrong type.
 */
int font_string_array_param(const gs_memory_t *mem, os_ptr, const char *, ref *);

/*
 * Get a GlyphDirectory if present.  Return 0 if present, 1 if absent,
 * or an error code.
 */
int font_GlyphDirectory_param(os_ptr, ref *);

/*
 * Get a glyph outline from GlyphDirectory.  Return an empty string if
 * the glyph is missing or out of range.
 */
int font_gdir_get_outline(const gs_memory_t *mem, const ref *, long, gs_glyph_data_t *);

/*
 * Access a given byte offset and length in an array of strings.
 * This is used for sfnts and for CIDMap.  The int argument is 2 for sfnts
 * (because of the strange behavior of odd-length strings), 1 for CIDMap.
 * Return code : 0 - success, <0 - error, 
 *               >0 - number of accessible bytes (client must cycle).
 */
int string_array_access_proc(const gs_memory_t *mem, const ref *, int, ulong, uint, const byte **);

#endif /* ifont42_INCLUDED */
