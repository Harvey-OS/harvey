/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: ifont42.h,v 1.3 2000/09/19 19:00:43 lpd Exp $ */
/* Procedure for building a Type 42 or CIDFontType 2 font */

#ifndef ifont42_INCLUDED
#  define ifont42_INCLUDED

/* Build a type 11 (TrueType CID-keyed) or 42 (TrueType) font. */
int build_gs_TrueType_font(P8(i_ctx_t *, os_ptr, gs_font_type42 **, font_type,
			      gs_memory_type_ptr_t, const char *, const char *,
			      build_font_options_t));

/*
 * Check a parameter for being an array of strings.  Return the parameter
 * value even if it is of the wrong type.
 */
int font_string_array_param(P3(os_ptr, const char *, ref *));

/*
 * Get a GlyphDirectory if present.  Return 0 if present, 1 if absent,
 * or an error code.
 */
int font_GlyphDirectory_param(P2(os_ptr, ref *));

/*
 * Get a glyph outline from GlyphDirectory.  Return an empty string if
 * the glyph is missing or out of range.
 */
int font_gdir_get_outline(P3(const ref *, long, gs_const_string *));

/*
 * Access a given byte offset and length in an array of strings.
 * This is used for sfnts and for CIDMap.  The int argument is 2 for sfnts
 * (because of the strange behavior of odd-length strings), 1 for CIDMap.
 */
int string_array_access_proc(P5(const ref *, int, ulong, uint, const byte **));

#endif /* ifont42_INCLUDED */
