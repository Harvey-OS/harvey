/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfcopy.h,v 1.11 2004/10/15 18:18:32 igor Exp $ */
/* Font copying for high-level output */

#ifndef gxfcopy_INCLUDED
#  define gxfcopy_INCLUDED

#include "gsccode.h"

#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif

#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

/*
 * Copy a font, aside from its glyphs.  Note that PostScript-specific data
 * -- that is, data that do not appear in the C structure that is the
 * relevant subclass of gs_font -- are NOT copied.  In particular,
 * Metrics[2], CDevProc, and FontInfo are not copied, except for the
 * information returned by font->procs.font_info (see the definition of
 * gs_font_info_t in gxfont.h).
 *
 * Note that in some cases the font must have a definition for the "not
 * defined" glyph, as noted below, or a rangecheck error occurs.
 *
 * The following FontTypes are supported:
 *
 *	Type 1, Type 2 - Encoding and CharStrings are not copied.  Subrs and
 *	GlobalSubrs (for CFFs) are copied; OtherSubrs are not copied.  The
 *	font must have a glyph named .notdef; its definition is copied.
 *
 *	Type 42 (TrueType) - Encoding and CharStrings are not copied.  The
 *	TrueType glyf and loca tables are not copied, nor are the bogus
 *	Adobe gdir, glyx, and locx "tables".  If the font has a definition
 *	for a glyph named .notdef (in the CharStrings dictionary), the
 *	definition is copied.
 *
 *	CIDFontType 0 (Type 1/2-based CIDFonts) - the glyph data are not
 *	copied.  The Type 1/2 subfonts *are* copied, as are the Subrs (both
 *	local and global).
 *
 *	CIDFontType 2 (TrueType-based CIDFonts) - the glyph data and CIDMap
 *	are not copied.
 *
 * The resulting font supports querying (font_info, glyph_info, etc.) and
 * rendering (glyph_outline, etc.), but it does not support make_font.
 */
int gs_copy_font(gs_font *font, const gs_matrix *orig_matrix, 
		    gs_memory_t *mem, gs_font **pfont_new);

/*
 * Copy a glyph, including any sub-glyphs.  The destination font ("copied"
 * argument) must be a font created by gs_copy_font.  The source font
 * ("font" argument) must have the same FontType as the destination, and in
 * addition must be "compatible" with it as described under the individual
 * FontTypes just below; however, gs_copy_glyph does not check
 * compatibility.
 *
 * If any glyph has already been copied but does not have the same
 * definition as the one being copied now, gs_copy_glyph returns an
 * invalidaccess error.  If the top-level glyph has already been copied
 * (with the same definition), gs_copy_glyph returns 1.  Otherwise,
 * gs_copy_glyph returns 0.
 *
 *	Type 1, Type 2 - the destination and source must have the same
 *	Subrs.  glyph must be a name (not an integer).  Copies the
 *	CharString entry.  Note that the Type 1/2 'seac' operator requires
 *	copying not only the sub-glyphs but their Encoding entries as well.
 *
 *	Type 42 - the destination and source must have compatible tables
 *	other than glyf and loca.  In practice this means that the source
 *	must be the same font that was passed to gs_copy_font.  If glyph is
 *	an integer, it is interpreted as a GID; if glyph is a name, both
 *	the CharString entry and the glyph data are copied.
 *
 *	CIDFontType 0 - the destination and source must have the same Subrs,
 *	and the Type 1/2 subfont(s) referenced by the glyph(s) being copied
 *	must be compatible.  glyph must be a CID.  Copies the CharString.
 *
 *	CIDFontType 2 - compatibility is as for Type 42.  glyph must be a
 *	CID (integer), not a GID.  Copies the glyph data and the CIDMap
 *	entry.
 *
 * Metrics[2] and CDevProc in the source font are ignored.  Currently,
 * for CIDFontType 2 fonts with MetricsCount != 0, the metrics attached to
 * the individual glyph outlines are also ignored (not copied).
 */
int gs_copy_glyph(gs_font *font, gs_glyph glyph, gs_font *copied);

/*
 * Copy a glyph with additional checking options.  If options includes
 * COPY_GLYPH_NO_OLD, then if the top-level glyph has already been copied,
 * return an invalidaccess error rather than 1.  If options includes
 * COPY_GLYPH_NO_NEW, then if the top-level glyph has *not* already been
 * copied, return an undefined error rather than 0.
 */
#define COPY_GLYPH_NO_OLD 1
#define COPY_GLYPH_NO_NEW 2
#define COPY_GLYPH_BY_INDEX 4
int gs_copy_glyph_options(gs_font *font, gs_glyph glyph, gs_font *copied,
			  int options);

/*
 * Add an encoding entry to a copied font.  If the given encoding entry is
 * not empty and is not the same as the new value, gs_copied_font_encode
 * returns an invalidaccess error.
 *
 * The action depends on FontType as follows:
 *
 *	Type 1, Type 2 - glyph must be a name, not a CID (integer).  Makes
 *	an entry in the font's Encoding.  A glyph by that name must exist
 *	in the copied font.
 *
 *	Type 42 - same as Type 1.
 *
 *	CIDFontType 0 - gives an error.
 *
 *	CIDFontType 2 - gives an error.
 */
int gs_copied_font_add_encoding(gs_font *copied, gs_char chr, gs_glyph glyph);

/*
 * Copy all the glyphs and, if relevant, Encoding entries from a font.  This
 * is equivalent to copying the glyphs and Encoding entries individually,
 * and returns errors under the same conditions.
 */
int gs_copy_font_complete(gs_font *font, gs_font *copied);


/*
 * Check whether specified glyphs can be copied from another font.
 * It means that (1) fonts have same hinting parameters and 
 * (2) font subsets for the specified glyph set don't include different 
 * outlines or metrics. Possible returned values : 
 * 0 (incompatible), 1 (compatible), < 0 (error)
 */
int gs_copied_can_copy_glyphs(const gs_font *cfont, const gs_font *ofont, 
		    gs_glyph *glyphs, int num_glyphs, int glyphs_step, 
		    bool check_hinting);

/* Extension glyphs may be added to a font to resolve 
   glyph name conflicts while conwerting a PDF Widths into Metrics.
   This function drops them before writing out an embedded font. */
int copied_drop_extension_glyphs(gs_font *cfont);


#endif /* gxfcopy_INCLUDED */
