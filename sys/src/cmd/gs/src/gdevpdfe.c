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

/*$Id: gdevpdfe.c,v 1.10 2001/07/31 06:32:42 lpd Exp $ */
/* Embedded font writing for pdfwrite driver. */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gxfcid.h"
#include "gxfont.h"
#include "gxfont0.h"
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "gdevpsf.h"
#include "scommon.h"

/* ---------------- Utilities ---------------- */

/* Begin writing FontFile* data. */
private int
pdf_begin_fontfile(gx_device_pdf *pdev, long FontFile_id,
		   const char *entries, long len1, pdf_data_writer_t *pdw)
{
    stream *s;

    pdf_open_separate(pdev, FontFile_id);
    s = pdev->strm;
    stream_puts(s, "<<");
    if (entries)
	stream_puts(pdev->strm, entries);
    if (len1 >= 0)
	pprintld1(pdev->strm, "/Length1 %ld", len1);
    return pdf_begin_data(pdev, pdw);
}

/* Finish writing FontFile* data. */
private int
pdf_end_fontfile(gx_device_pdf *pdev, pdf_data_writer_t *pdw)
{
    return pdf_end_data(pdw);
}

/* ---------------- Individual font types ---------------- */

/* ------ Type 1 family ------ */

/*
 * Acrobat Reader apparently doesn't accept CFF fonts with Type 1
 * CharStrings, so we need to convert them.  Also remove lenIV,
 * so Type 2 fonts will compress better.
 */
#define TYPE2_OPTIONS (WRITE_TYPE2_NO_LENIV | WRITE_TYPE2_CHARSTRINGS)

/* Write the FontFile[3] data for an embedded Type 1, Type 2, or */
/* CIDFontType 0 font. */
private int
pdf_embed_font_as_type1(gx_device_pdf *pdev, gs_font_type1 *font,
			long FontFile_id, gs_glyph subset_glyphs[256],
			uint subset_size, const gs_const_string *pfname)
{
    stream poss;
    int lengths[3];
    int code;
    pdf_data_writer_t writer;
#define MAX_INT_CHARS ((sizeof(int) * 8 + 2) / 3)
    char lengths_str[9 + MAX_INT_CHARS + 11];  /* /Length2 %d/Length3 0\0 */
#undef MAX_INT_CHARS

    swrite_position_only(&poss);
    /*
     * We omit the 512 zeros and the cleartomark, and set Length3 to 0.
     * Note that the interpreter adds them implicitly (per documentation),
     * so we must set MARK so that the encrypted portion pushes a mark on
     * the stack.
     *
     * NOTE: the options were set to 0 in the first checked-in version of
     * this file.  We can't explain this: Acrobat Reader requires eexec
     * encryption, so the code can't possibly have worked.
     *
     * Acrobat Reader 3 allows lenIV = -1 in Type 1 fonts, but Acrobat
     * Reader 4 doesn't.  Therefore, we can't allow it.
     */
#define TYPE1_OPTIONS (WRITE_TYPE1_EEXEC | WRITE_TYPE1_EEXEC_MARK |\
		       WRITE_TYPE1_WITH_LENIV)
    code = psf_write_type1_font(&poss, font, TYPE1_OPTIONS,
				 subset_glyphs, subset_size, pfname, lengths);
    if (code < 0)
	return code;
    sprintf(lengths_str, "/Length2 %d/Length3 0", lengths[1]);
    code = pdf_begin_fontfile(pdev, FontFile_id, lengths_str, lengths[0],
			      &writer);
    if (code < 0)
	return code;
    code = psf_write_type1_font(writer.binary.strm, font, TYPE1_OPTIONS,
				subset_glyphs, subset_size, pfname,
				lengths /*ignored*/);
#undef TYPE1_OPTIONS
    pdf_end_fontfile(pdev, &writer);
    return code;
}

/* Embed a font as Type 2. */
private int
pdf_embed_font_as_type2(gx_device_pdf *pdev, gs_font_type1 *font,
			long FontFile_id, gs_glyph subset_glyphs[256],
			uint subset_size, const gs_const_string *pfname)
{
    int code;
    pdf_data_writer_t writer;
    int options = TYPE2_OPTIONS |
	(pdev->CompatibilityLevel < 1.3 ? WRITE_TYPE2_AR3 : 0);

    code = pdf_begin_fontfile(pdev, FontFile_id, "/Subtype/Type1C", -1L,
			      &writer);
    if (code < 0)
	return code;
    code = psf_write_type2_font(writer.binary.strm, font, options,
				subset_glyphs, subset_size, pfname);
    pdf_end_fontfile(pdev, &writer);
    return code;
}

/* Embed a Type 1 or Type 2 font. */
private int
pdf_embed_font_type1(gx_device_pdf *pdev, gs_font_type1 *font,
		     long FontFile_id, gs_glyph subset_glyphs[256],
		     uint subset_size, const gs_const_string *pfname)
{
    switch (((const gs_font *)font)->FontType) {
    case ft_encrypted:
	if (pdev->CompatibilityLevel < 1.2)
	    return pdf_embed_font_as_type1(pdev, font, FontFile_id,
					   subset_glyphs, subset_size, pfname);
	/* For PDF 1.2 and later, write Type 1 fonts as Type1C. */
    case ft_encrypted2:
	return pdf_embed_font_as_type2(pdev, font, FontFile_id,
				       subset_glyphs, subset_size, pfname);
    default:
	return_error(gs_error_rangecheck);
    }
}

/* Embed a CIDFontType0 font. */
private int
pdf_embed_font_cid0(gx_device_pdf *pdev, gs_font_cid0 *font,
		    long FontFile_id, const byte *subset_cids,
		    uint subset_size, const gs_const_string *pfname)
{
    int code;
    pdf_data_writer_t writer;

    if (pdev->CompatibilityLevel < 1.2)
	return_error(gs_error_rangecheck);
    code = pdf_begin_fontfile(pdev, FontFile_id, "/Subtype/CIDFontType0C", -1L,
			      &writer);
    if (code < 0)
	return code;
    code = psf_write_cid0_font(writer.binary.strm, font, TYPE2_OPTIONS,
			       subset_cids, subset_size, pfname);
    pdf_end_fontfile(pdev, &writer);
    return code;
}

/* ------ TrueType family ------ */

/* Embed a TrueType font. */
private int
pdf_embed_font_type42(gx_device_pdf *pdev, gs_font_type42 *font,
		      long FontFile_id, gs_glyph subset_glyphs[256],
		      uint subset_size, const gs_const_string *pfname)
{
    /* Acrobat Reader 3 doesn't handle cmap format 6 correctly. */
    const int options = WRITE_TRUETYPE_CMAP | WRITE_TRUETYPE_NAME |
	(pdev->CompatibilityLevel <= 1.2 ?
	 WRITE_TRUETYPE_NO_TRIMMED_TABLE : 0);
    stream poss;
    int code;
    pdf_data_writer_t writer;

    swrite_position_only(&poss);
    code = psf_write_truetype_font(&poss, font, options,
				   subset_glyphs, subset_size, pfname);
    if (code < 0)
	return code;
    code = pdf_begin_fontfile(pdev, FontFile_id, NULL, stell(&poss), &writer);
    if (code < 0)
	return code;
    psf_write_truetype_font(writer.binary.strm, font, options,
			    subset_glyphs, subset_size, pfname);
    pdf_end_fontfile(pdev, &writer);
    return 0;
}

/* Embed a CIDFontType2 font. */
private int
pdf_embed_font_cid2(gx_device_pdf *pdev, gs_font_cid2 *font,
		    long FontFile_id, const byte *subset_bits,
		    uint subset_size, const gs_const_string *pfname)
{
    /* CIDFontType 2 fonts don't use cmap, name, OS/2, or post. */
#define OPTIONS 0
    int code;
    pdf_data_writer_t writer;

    code = pdf_begin_fontfile(pdev, FontFile_id, NULL, -1L, &writer);
    if (code < 0)
	return code;
    psf_write_cid2_font(writer.binary.strm, font, OPTIONS,
			subset_bits, subset_size, pfname);
#undef OPTIONS
    pdf_end_fontfile(pdev, &writer);
    return 0;
}

/* ---------------- Entry point ---------------- */

/*
 * Write the FontDescriptor and FontFile* data for an embedded font.
 * Return a rangecheck error if the font can't be embedded.
 */
int
pdf_write_embedded_font(gx_device_pdf *pdev, pdf_font_descriptor_t *pfd)
{
    gs_font *font = pfd->base_font;
    gs_const_string font_name;
    byte *fnchars = pfd->FontName.chars;
    uint fnsize = pfd->FontName.size;
    bool do_subset;
    long FontFile_id = pfd->FontFile_id;
    gs_glyph subset_glyphs[256];
    gs_glyph *subset_list = 0;	/* for non-CID fonts */
    byte *subset_bits = 0;	/* for CID fonts */
    uint subset_size = 0;
    gs_matrix save_mat;
    int code;

    /* Determine whether to subset the font. */
    switch (pfd->do_subset) {
    case FONT_SUBSET_OK:
	do_subset = pdev->params.SubsetFonts && pdev->params.MaxSubsetPct > 0;
	break;
    case FONT_SUBSET_YES:
	do_subset = true;
	break;
    case FONT_SUBSET_NO:
	do_subset = false;
	break;
    }
    if (do_subset) {
	int max_pct = pdev->params.MaxSubsetPct;
	int used, i;

	for (i = 0, used = 0; i < pfd->chars_used.size; ++i)
	    used += byte_count_bits[pfd->chars_used.data[i]];
	/*
	 * We want to subset iff used / total <= MaxSubsetPct / 100, i.e.,
	 * iff total >= used * 100 / MaxSubsetPct.  This observation
	 * allows us to stop the enumeration loop early.
	 */
	if (max_pct < 100) {
	    do_subset = false;
	    if (max_pct > 0) {
		int max_total = used * 100 / max_pct;
		int index, total;
		gs_glyph ignore_glyph;

		for (index = 0, total = 0;
		     (font->procs.enumerate_glyph(font, &index,
						  GLYPH_SPACE_INDEX,
						  &ignore_glyph), index != 0);
		     )
		    if (++total >= max_total) {
			do_subset = true;
			break;
		    }
	    }
	}
    }

    /* Generate an appropriate font name. */
    if (pdf_has_subset_prefix(fnchars, fnsize)) {
	/* Strip off any existing subset prefix. */
	fnsize -= SUBSET_PREFIX_SIZE;
	memmove(fnchars, fnchars + SUBSET_PREFIX_SIZE, fnsize);
    }
    if (do_subset) {
	memmove(fnchars + SUBSET_PREFIX_SIZE, fnchars, fnsize);
	pdf_make_subset_prefix(pdev, fnchars, FontFile_id);
	fnsize += SUBSET_PREFIX_SIZE;
    }
    font_name.data = fnchars;
    font_name.size = pfd->FontName.size = fnsize;
    code = pdf_write_FontDescriptor(pdev, pfd);
    if (code >= 0) {
	pfd->written = true;

	/*
	 * Finally, write the font (or subset), using the original
	 * (unscaled) FontMatrix.
	 */
	save_mat = font->FontMatrix;
	font->FontMatrix = pfd->orig_matrix;
	switch (font->FontType) {
	case ft_composite:
	    /* Nothing to embed -- the descendant fonts do it all. */
	    break;
	case ft_encrypted:
	case ft_encrypted2:
	    if (do_subset) {
		subset_size = psf_subset_glyphs(subset_glyphs, font,
						pfd->chars_used.data);
		subset_list = subset_glyphs;
	    }
	    code = pdf_embed_font_type1(pdev, (gs_font_type1 *)font,
					FontFile_id, subset_list,
					subset_size, &font_name);
	    break;
	case ft_TrueType:
	    if (do_subset) {
		subset_size = psf_subset_glyphs(subset_glyphs, font,
						pfd->chars_used.data);
		subset_list = subset_glyphs;
	    }
	    code = pdf_embed_font_type42(pdev, (gs_font_type42 *)font,
					 FontFile_id, subset_list,
					 subset_size, &font_name);
	    break;
	case ft_CID_encrypted:
	    if (do_subset) {
		subset_size = pfd->chars_used.size << 3;
		subset_bits = pfd->chars_used.data;
	    }
	    code = pdf_embed_font_cid0(pdev, (gs_font_cid0 *)font,
				       FontFile_id, subset_bits,
				       subset_size, &font_name);
	    break;
	case ft_CID_TrueType:
	    if (do_subset) {
		/*
		 * Note that the subset mask for CIDFontType 2 fonts is
		 * indexed by the TrueType glyph number, not the CID.
		 */
		subset_size = pfd->glyphs_used.size << 3;
		subset_bits = pfd->glyphs_used.data;
	    }
	    code = pdf_embed_font_cid2(pdev, (gs_font_cid2 *)font,
				       FontFile_id, subset_bits,
				       subset_size, &font_name);
	    break;
	default:
	    code = gs_note_error(gs_error_rangecheck);
	}
	font->FontMatrix = save_mat;
    }
    return code;
}
