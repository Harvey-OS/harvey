/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: ichar1.h,v 1.3 2000/09/19 19:00:42 lpd Exp $ */
/* Type 1 / Type 2 character rendering operator procedures */

#ifndef ichar1_INCLUDED
#  define ichar1_INCLUDED

/* ---------------- Public ---------------- */

/* Render a Type 1 or Type 2 outline. */
/* This is the entire implementation of the .type1/2execchar operators. */
int charstring_execchar(P2(i_ctx_t *i_ctx_p, int font_type_mask));

/* ---------------- Internal ---------------- */

/*
 * Get a Type 1 or Type 2 glyph outline.  This is the glyph_outline
 * procedure for the font.
 */
font_proc_glyph_outline(zchar1_glyph_outline);

/*
 * Get a glyph outline given a CharString.  The glyph_outline procedure
 * for CIDFontType 0 fonts uses this.
 */
int zcharstring_outline(P5(gs_font_type1 *pfont, const ref *pgref,
			   const gs_const_string *pgstr,
			   const gs_matrix *pmat, gx_path *ppath));

#endif /* ichar1_INCLUDED */
