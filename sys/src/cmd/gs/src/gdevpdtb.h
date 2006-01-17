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

/* $Id: gdevpdtb.h,v 1.11 2005/04/04 08:53:07 igor Exp $ */
/* BaseFont structure and API for pdfwrite */

#ifndef gdevpdtb_INCLUDED
#  define gdevpdtb_INCLUDED

#include "gdevpdtx.h"

/* ================ Types and structures ================ */

/*
 * A "base font" pdf_base_font_t contains a stable copy of a gs_font.  The
 * only supported font types are Type 1/2, TrueType / Type 42, CIDFontType
 * 0, and CIDFontType 2.
 *
 * At the time a base font is created, we copy the fixed elements of the
 * gs_font (that is, everything except the data for individual glyphs) into
 * stable memory; we then copy individual glyphs as they are used.  If
 * subsetting is not mandatory (that is, if the entire font might be
 * embedded or its Widths written), we also save a separate copy of the
 * entire font, since the subsetting decision cannot be made until the font
 * is written out (normally at the end of the document).
 *
 * In an earlier design, we deferred making the complete copy until the font
 * was about to be freed.  We decided that the substantial extra complexity
 * of this approach did not justify the space that would be saved in the
 * usual (subsetted) case.
 *
 * The term "base font" is used, confusingly, for at least 3 different
 * concepts in Ghostscript.  However, the above meaning is used consistently
 * within the PDF text code (gdevpdt*.[ch]).
 */
/*
 * Font names in PDF files have caused an enormous amount of trouble, so we
 * document specifically how they are handled in each structure.
 *
 * The PDF Reference doesn't place any constraints on the [CID]FontName of
 * base fonts, although it does say that the BaseFont name in the font
 * resource is "usually" derived from the [CID]FontName of the base font.
 * The code in this module allows setting the font name.  It initializes
 * the name to the key_name of the base font, or to the font_name if the
 * base font has no key_name, minus any XXXXXX+ subset prefix; the
 * pdf_do_subset_font procedure adds the XXXXXX+ prefix if the font will
 * be subsetted.
 */

#ifndef pdf_base_font_DEFINED
#  define pdf_base_font_DEFINED
typedef struct pdf_base_font_s pdf_base_font_t;
#endif

/* ================ Procedures ================ */

/*
 * Allocate and initialize a base font structure, making the required
 * stable copy/ies of the gs_font.  Note that this removes any XXXXXX+
 * font name prefix from the copy.  If complete is true, the copy is
 * a complete one, and adding glyphs or Encoding entries is not allowed.
 */
int pdf_base_font_alloc(gx_device_pdf *pdev, pdf_base_font_t **ppbfont,
		    gs_font_base *font, const gs_matrix *orig_matrix, 
		    bool is_standard, bool orig_name);

/*
 * Return a reference to the name of a base font.  This name is guaranteed
 * not to have a XXXXXX+ prefix.  The client may change the name at will,
 * but must not add a XXXXXX+ prefix.
 */
gs_string *pdf_base_font_name(pdf_base_font_t *pbfont);

/*
 * Return the (copied, subset or complete) font associated with a base font.
 * This procedure probably shouldn't exist....
 */
gs_font_base *pdf_base_font_font(const pdf_base_font_t *pbfont, bool complete);

/*
 * Check for subset font.
 */
bool pdf_base_font_is_subset(const pdf_base_font_t *pbfont);

/*
 * Drop the copied complete font associated with a base font.
 */
void pdf_base_font_drop_complete(pdf_base_font_t *pbfont);

/*
 * Copy a glyph (presumably one that was just used) into a saved base
 * font.  Note that it is the client's responsibility to determine that
 * the source font is compatible with the target font.  (Normally they
 * will be the same.)
 */
int pdf_base_font_copy_glyph(pdf_base_font_t *pbfont, gs_glyph glyph,
			     gs_font_base *font);

/*
 * Determine whether a font is a subset font by examining the name.
 */
bool pdf_has_subset_prefix(const byte *str, uint size);

/*
 * Add the XXXXXX+ prefix for a subset font.
 */
int pdf_add_subset_prefix(const gx_device_pdf *pdev, gs_string *pstr, 
			byte *used, int count);

/*
 * Determine whether a copied font should be subsetted.
 */
bool pdf_do_subset_font(gx_device_pdf *pdev, pdf_base_font_t *pbfont, 
			gs_id rid);

/*
 * Write the FontFile entry for an embedded font, /FontFile<n> # # R.
 */
int pdf_write_FontFile_entry(gx_device_pdf *pdev, pdf_base_font_t *pbfont);

/*
 * Write an embedded font, possibly subsetted.
 */
int pdf_write_embedded_font(gx_device_pdf *pdev, pdf_base_font_t *pbfont,
			gs_int_rect *FontBBox, gs_id rid, cos_dict_t **ppcd);

/*
 * Write the CharSet data for a subsetted font, as a PDF string.
 */
int pdf_write_CharSet(gx_device_pdf *pdev, pdf_base_font_t *pbfont);

/*
 * Write the CIDSet object for a subsetted CIDFont.
 */
int pdf_write_CIDSet(gx_device_pdf *pdev, pdf_base_font_t *pbfont,
		     long *pcidset_id);

/*
 * Check whether a base font is standard.
 */
bool pdf_is_standard_font(pdf_base_font_t *bfont);

void pdf_set_FontFile_object(pdf_base_font_t *bfont, cos_dict_t *pcd);
const cos_dict_t * pdf_get_FontFile_object(pdf_base_font_t *bfont);

#endif /* gdevpdtb_INCLUDED */
