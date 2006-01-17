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

/* $Id: gdevpdtw.c,v 1.44 2005/06/06 15:32:22 igor Exp $ */
/* Font resource writing for pdfwrite text */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfcmap.h"
#include "gxfont.h"
#include "gscencs.h"
#include "gdevpsf.h"
#include "gdevpdfx.h"
#include "gdevpdfo.h"		/* for object->written */
#include "gdevpdtd.h"		/* for writing FontDescriptor */
#include "gdevpdtf.h"
#include "gdevpdti.h"		/* for writing bitmap fonts Encoding */
#include "gdevpdtw.h"
#include "gdevpdtv.h"
#include "sarc4.h"

private const char *const encoding_names[] = {
    KNOWN_REAL_ENCODING_NAMES
};

/* ================ Font resource writing ================ */

/* ---------------- Private ---------------- */

/* Write the Widths for a font. */
private int
pdf_write_Widths(gx_device_pdf *pdev, int first, int last, const double *widths)
{
    stream *s = pdev->strm;
    int i;

    if (first > last)
	first = last = 0;
    pprintd2(s, "/FirstChar %d/LastChar %d/Widths[", first, last);
    for (i = first; i <= last; ++i)
    	pprintg1(s, (i & 15 ? " %g" : "\n%g"), psdf_round(widths[i], 100, 10));
    stream_puts(s, "]\n");
    return 0;
}

/* Check strings equality. */
private bool 
strings_equal(const gs_const_string *str0, const gs_const_string *str1)
{
    return str0->size == str1->size &&
	   !memcmp(str0->data, str1->data, str0->size);
}

/* Check if an encoding element differs from a standard one. */
private int
pdf_different_encoding_element(const pdf_font_resource_t *pdfont, int ch, int encoding_index)
{
    if (pdfont->u.simple.Encoding[ch].is_difference)
	return 1;
    else if (encoding_index != ENCODING_INDEX_UNKNOWN) {
	gs_glyph glyph0 = gs_c_known_encode(ch, encoding_index);
	gs_glyph glyph1 = pdfont->u.simple.Encoding[ch].glyph;
	gs_const_string str;
	int code = gs_c_glyph_name(glyph0, &str);

	if (code < 0)
	    return code; /* Must not happen */
	if (glyph1 != GS_NO_GLYPH)
	    if (!strings_equal(&str, &pdfont->u.simple.Encoding[ch].str))
		return 1;
    }
    return 0;
}

/* Find an index of a different encoding element. */
int
pdf_different_encoding_index(const pdf_font_resource_t *pdfont, int ch0)
{
    gs_encoding_index_t base_encoding = pdfont->u.simple.BaseEncoding;
    int ch, code;

    for (ch = ch0; ch < 256; ++ch) {
	code = pdf_different_encoding_element(pdfont, ch, base_encoding);
	if (code < 0)
	    return code; /* Must not happen */
	if (code)
	    break;
    }
    return ch;
}

/* Check for unknown encode (simple fonts only). */
private bool
pdf_simple_font_needs_ToUnicode(const pdf_font_resource_t *pdfont)
{
    int ch;
    unsigned char mask = (pdfont->FontType == ft_encrypted || pdfont->FontType == ft_encrypted2 
		? GS_C_PDF_GOOD_GLYPH_MASK : GS_C_PDF_GOOD_NON_SYMBOL_MASK);

    if (pdfont->u.simple.Encoding == NULL)
	return true; /* Bitmap Type 3 fonts have no pdfont->u.simple.Encoding . */
    for (ch = 0; ch < 256; ++ch) {
	pdf_encoding_element_t *pet = &pdfont->u.simple.Encoding[ch];
	gs_glyph glyph = pet->glyph;

	if (glyph == GS_NO_GLYPH)
	    continue;
	if (glyph < gs_c_min_std_encoding_glyph || glyph >= GS_MIN_CID_GLYPH) {
	    if (pet->str.size == 0)
		return true;
	    glyph = gs_c_name_glyph(pet->str.data, pet->str.size);
	    if (glyph == GS_NO_GLYPH)
		return true;
	}
        glyph -= gs_c_min_std_encoding_glyph;
        if( glyph > GS_C_PDF_MAX_GOOD_GLYPH ||
           !(gs_c_pdf_glyph_type[glyph >> 2] & (mask << (( glyph & 3 )<<1) ))) 
          return true;
    }
    return false;
}

/* Write Encoding differencrs. */
int 
pdf_write_encoding(gx_device_pdf *pdev, const pdf_font_resource_t *pdfont, long id, int ch)
{
    /* Note : this truncates extended glyph names to original names. */
    stream *s;
    gs_encoding_index_t base_encoding = pdfont->u.simple.BaseEncoding;
    const int sl = strlen(gx_extendeg_glyph_name_separator);
    int prev = 256, code;

    pdf_open_separate(pdev, id);
    s = pdev->strm;
    stream_puts(s, "<</Type/Encoding");
    if (base_encoding < 0 && pdev->ForOPDFRead)
	base_encoding = ENCODING_INDEX_STANDARD;
    if (base_encoding > 0)
	pprints1(s, "/BaseEncoding/%s", encoding_names[base_encoding]);
    stream_puts(s, "/Differences[");
    for (; ch < 256; ++ch) {
	code = pdf_different_encoding_element(pdfont, ch, base_encoding);
	if (code < 0)
	    return code; /* Must not happen */
	if (code == 0 && pdfont->FontType == ft_user_defined) {
	    /* PDF 1.4 spec Appendix H Note 42 says that
	     * Acrobat 4 can't properly handle Base Encoding. 
	     * Enforce writing differences against that.
	     */
	    if (pdfont->used[ch >> 3] & 0x80 >> (ch & 7))
		if (pdfont->u.simple.Encoding[ch].str.size)
		    code = 1;
	}
	if (code) {
	    const byte *d = pdfont->u.simple.Encoding[ch].str.data;
	    int i, l = pdfont->u.simple.Encoding[ch].str.size;

    	    if (pdev->HavePDFWidths) {
		for (i = 0; i + sl < l; i++)
		    if (!memcmp(d + i, gx_extendeg_glyph_name_separator, sl)) {
			l = i;
			break;
		    }
	    }
	    if (ch != prev + 1)
		pprintd1(s, "\n%d", ch);
	    pdf_put_name(pdev, d, l);
	    prev = ch;
	}
    }
    stream_puts(s, "]>>\n");
    pdf_end_separate(pdev);
    return 0;
}

/* Write Encoding reference. */
int
pdf_write_encoding_ref(gx_device_pdf *pdev,
	  const pdf_font_resource_t *pdfont, long id)
{
    stream *s = pdev->strm;

    if (id != 0)
	pprintld1(s, "/Encoding %ld 0 R", id);
    else if (pdfont->u.simple.BaseEncoding > 0) {
	gs_encoding_index_t base_encoding = pdfont->u.simple.BaseEncoding;
	pprints1(s, "/Encoding/%s", encoding_names[base_encoding]);
    }
    return 0;
}

/* Write the Subtype and Encoding for a simple font. */
private int
pdf_write_simple_contents(gx_device_pdf *pdev,
			  const pdf_font_resource_t *pdfont)
{
    stream *s = pdev->strm;
    long diff_id = 0;
    int ch = (pdfont->u.simple.Encoding ? 0 : 256);
    int code = 0;

    ch = pdf_different_encoding_index(pdfont, ch);
    if (ch < 256)
	diff_id = pdf_obj_ref(pdev);
    code = pdf_write_encoding_ref(pdev, pdfont, diff_id);
    if (code < 0)
	return code;
    pprints1(s, "/Subtype/%s>>\n",
	     (pdfont->FontType == ft_TrueType ? "TrueType" :
	      pdfont->u.simple.s.type1.is_MM_instance ? "MMType1" : "Type1"));
    pdf_end_separate(pdev);
    if (diff_id) {
	code = pdf_write_encoding(pdev, pdfont, diff_id, ch);
	if (code < 0)
	    return code;
    }
    return 0;
}

/*
 * Write the W[2] entries for a CIDFont.  *pdfont is known to be a
 * CIDFont (type 0 or 2).
 */
private bool
pdf_compute_CIDFont_default_widths(const pdf_font_resource_t *pdfont, int wmode, int *pdw, int *pdv)
{
    psf_glyph_enum_t genum;
    gs_glyph glyph;
    ushort counts[1500]; /* Some CID fonts use vertical widths 1026 .*/
    int dw_count = 0, i, dwi = 0, neg_count = 0, pos_count = 0;
    double *w = (wmode ? pdfont->u.cidfont.Widths2 : pdfont->Widths);

    /* We don't wont to scan for both negative and positive widths,
     * to save the C stack space.
     * Doubtly they both are used in same font.
     * So just count positive and negative widths separately
     * and use the corresponding sign.
     * fixme : implement 2 hystograms.
     */
    psf_enumerate_bits_begin(&genum, NULL, 
			     wmode ? pdfont->u.cidfont.used2 : pdfont->used, 
			     pdfont->count, GLYPH_SPACE_INDEX);
    memset(counts, 0, sizeof(counts));
    while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
        int i = glyph - GS_MIN_CID_GLYPH;

	if ( i < pdfont->count) { /* safety */
	    int width = (int)(w[i] + 0.5);

	    counts[min(any_abs(width), countof(counts) - 1)]++;
	    if (width > 0)
		pos_count++;
	    else if (width < 0)
		neg_count++;
	}
    }
    for (i = 1; i < countof(counts); ++i)
	if (counts[i] > dw_count)
	    dwi = i, dw_count = counts[i];
    *pdw = (neg_count > pos_count ? -dwi : dwi);
    *pdv = 0;
    if (wmode) {
	psf_enumerate_glyphs_reset(&genum);
	while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	    int i = glyph - GS_MIN_CID_GLYPH;

	    if ( i < pdfont->count) { /* safety */
		int width = (int)(w[i] + 0.5);

		if (min(any_abs(width), countof(counts) - 1) == any_abs(dwi)) {
		    *pdv = (int)(pdfont->u.cidfont.v[i * 2 + 1] + 0.5);
		    break;
		}
	    }
	}
    }
    return (dw_count + counts[0] > 0);
}

/*
 * Write the [D]W[2] entries for a CIDFont.  *pdfont is known to be a
 * CIDFont (type 0 or 2).
 */
private int
pdf_write_CIDFont_widths(gx_device_pdf *pdev,
			 const pdf_font_resource_t *pdfont, int wmode)
{
    /*
     * The values of the CIDFont width keys are as follows:
     *   DW = w (default 0)
     *   W = [{c [w ...] | cfirst clast w}*]
     *   DW2 = [vy w1y] (default [880 -1000])
     *   W2 = [{c [w1y vx vy ...] | cfirst clast w1y vx vy}*]
     */
    stream *s = pdev->strm;
    psf_glyph_enum_t genum;
    gs_glyph glyph;
    int dw = 0, dv = 0, prev = -2;
    const char *Widths_key = (wmode ? "/W2" : "/W");
    double *w = (wmode ? pdfont->u.cidfont.Widths2 : pdfont->Widths);

    /* Compute and write default width : */
    if (pdf_compute_CIDFont_default_widths(pdfont, wmode, &dw, &dv)) {
	if (wmode) {
	    pprintd2(s, "/DW2 [%d %d]\n", dv, dw);
	} else
	    pprintd1(s, "/DW %d\n", dw);
    }

    /*
     * Now write all widths different from the default one.  Currently we make no
     * attempt to optimize this: we write every width individually.
     */
    psf_enumerate_bits_begin(&genum, NULL, 
			     wmode ? pdfont->u.cidfont.used2 : pdfont->used, 
			     pdfont->count, GLYPH_SPACE_INDEX);
    {
	while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	    int cid = glyph - GS_MIN_CID_GLYPH;
	    int width = (int)(w[cid] + 0.5);

#if 0 /* Must write zero widths - see test file of the bug Bug 687681.
	 We don't enumerate unused glyphs here due to pdfont->used. */
	    if (width == 0)
		continue; /* Don't write for unused glyphs. */
#else
	    {	/* Check whether copied font really have this glyph.
	           debugged with 401-01.ps, which uses undefined CIDs. */
		gs_font_base *pfont = pdf_font_resource_font(pdfont, false);
		gs_glyph_info_t info;

		if (pdfont != NULL) {
		    if (pfont->procs.glyph_info((gs_font *)pfont, glyph, NULL, 0, &info) < 0)
			continue;
		}
	    }
#endif
	    if (cid == prev + 1) {
		if (wmode) {
		    int vx = (int)(pdfont->u.cidfont.v[cid * 2 + 0] + 0.5);
		    int vy = (int)(pdfont->u.cidfont.v[cid * 2 + 1] + 0.5);

		    pprintd3(s, "\n%d %d %d", width, vx, vy);
		} else
		    pprintd1(s, "\n%d", width);
	    } else if (width == dw && 
		    (!wmode || (int)(pdfont->u.cidfont.v[cid * 2 + 0] + 0.5) == 
				(int)(pdfont->Widths[cid] / 2 + 0.5)) &&
		    (!wmode || (int)(pdfont->u.cidfont.v[cid * 2 + 1] + 0.5) == dv))
		continue;
	    else {
		if (prev >= 0)
		    stream_puts(s, "]\n");
		else {
		    stream_puts(s, Widths_key);
		    stream_puts(s, "[");
		}
		if (wmode) {
		    int vx = (int)(pdfont->u.cidfont.v[cid * 2 + 0] + 0.5);
		    int vy = (int)(pdfont->u.cidfont.v[cid * 2 + 1] + 0.5);

		    pprintd4(s, "%d[%d %d %d", cid, width, vx, vy);
		} else
		    pprintd2(s, "%d[%d", cid, width);
	    }
	    prev = cid;
	}
	if (prev >= 0)
	    stream_puts(s, "]]\n");
    }    

    return 0;
}

/* ---------------- Specific FontTypes ---------------- */

/* Write the contents of a Type 0 font resource. */
int
pdf_write_contents_type0(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    stream *s = pdev->strm;

    /*
     * The Encoding name might be missing if an error occurred when
     * creating the font resource.
     */
    if (pdfont->u.type0.Encoding_name[0])
	pprints1(s, "/Encoding %s", pdfont->u.type0.Encoding_name);
    pprintld1(s, "/DescendantFonts[%ld 0 R]",
	      pdf_font_id(pdfont->u.type0.DescendantFont));
    stream_puts(s, "/Subtype/Type0>>\n");
    pdf_end_separate(pdev);
    return 0;
}

/*
 * Finish writing the contents of a Type 3 font resource (FontBBox, Widths,
 * Subtype).
 */
int
pdf_finish_write_contents_type3(gx_device_pdf *pdev,
				pdf_font_resource_t *pdfont)
{
    stream *s = pdev->strm;

    pdf_write_font_bbox(pdev, &pdfont->u.simple.s.type3.FontBBox);
    pdf_write_Widths(pdev, pdfont->u.simple.FirstChar, 
		    pdfont->u.simple.LastChar, pdfont->Widths);
    stream_puts(s, "/Subtype/Type3>>\n");
    pdf_end_separate(pdev);
    return 0;
}

/* Write the contents of a standard (base 14) font resource. */
int
pdf_write_contents_std(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    return pdf_write_simple_contents(pdev, pdfont);
}

/* Write the contents of a simple (Type 1 or Type 42) font resource. */
int
pdf_write_contents_simple(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    pdf_write_Widths(pdev, pdfont->u.simple.FirstChar,
		     pdfont->u.simple.LastChar, pdfont->Widths);
    return pdf_write_simple_contents(pdev, pdfont);
}

/* Write the contents of a CIDFont resource. */
private int
write_contents_cid_common(gx_device_pdf *pdev, pdf_font_resource_t *pdfont,
			  int subtype)
{
    /* Write [D]W[2], CIDSystemInfo, and Subtype, and close the object. */
    stream *s = pdev->strm;
    int code;

    if (pdfont->Widths != 0) {
	code = pdf_write_CIDFont_widths(pdev, pdfont, 0);
	if (code < 0)
	    return code;
    } else {
	/* With a vertical font, the viewer uses /DW 
	   to determine glyph width to compute its v-vector. */
	stream_puts(s, "/DW 0\n");
    }
    if (pdfont->u.cidfont.Widths2 != 0) {
	code = pdf_write_CIDFont_widths(pdev, pdfont, 1);
	if (code < 0)
	    return code;
    }
    if (pdfont->u.cidfont.CIDSystemInfo_id)
	pprintld1(s, "/CIDSystemInfo %ld 0 R",
		  pdfont->u.cidfont.CIDSystemInfo_id);
    pprintd1(s, "/Subtype/CIDFontType%d>>\n", subtype);
    pdf_end_separate(pdev);
    return 0;
}
int
pdf_write_contents_cid0(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    return write_contents_cid_common(pdev, pdfont, 0);
}
int
pdf_write_contents_cid2(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    int count = pdfont->count;
    long map_id = 0;
    psf_glyph_enum_t genum;
    gs_glyph glyph;
    int code;

    /* Check for the identity CIDMap. */
    psf_enumerate_bits_begin(&genum, NULL, pdfont->used, count,
			     GLYPH_SPACE_INDEX);
    while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	int cid = glyph - GS_MIN_CID_GLYPH;
	int gid = pdfont->u.cidfont.CIDToGIDMap[cid];

	if (gid != cid) {	/* non-identity map */
	    map_id = pdf_obj_ref(pdev);
	    pprintld1(pdev->strm, "/CIDToGIDMap %ld 0 R\n", map_id);
	    break;
	}
    }

    code = write_contents_cid_common(pdev, pdfont, 2);
    if (code < 0)
	return code;

    if (map_id) {
	pdf_data_writer_t writer;
	int i;

	pdf_begin_data_stream(pdev, &writer,
	    DATA_STREAM_BINARY | DATA_STREAM_COMPRESS,
		    /* Don't set DATA_STREAM_ENCRYPT since we write to a temporary file.
		       See comment in pdf_begin_encrypt. */
		    map_id);
	for (i = 0; i < count; ++i) {
	    uint gid = pdfont->u.cidfont.CIDToGIDMap[i];

	    stream_putc(writer.binary.strm, (byte)(gid >> 8));
	    stream_putc(writer.binary.strm, (byte)(gid));
	}
	code = pdf_end_data(&writer);
    }
    return code;
}

/* ---------------- External entries ---------------- */

/* Write a font resource. */
private int
pdf_write_font_resource(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    stream *s;

    if (pdfont->cmap_ToUnicode != NULL && pdfont->res_ToUnicode == NULL)
	if (((pdfont->FontType == ft_composite) && 
		!gs_cmap_is_identity(pdfont->cmap_ToUnicode, -1)) ||
	    ((pdfont->FontType == ft_encrypted || pdfont->FontType == ft_encrypted2 || 
		pdfont->FontType == ft_TrueType || pdfont->FontType == ft_user_defined) && 
		pdf_simple_font_needs_ToUnicode(pdfont))
	   ) {
	    pdf_resource_t *prcmap;
	    int code = pdf_cmap_alloc(pdev, pdfont->cmap_ToUnicode, &prcmap, -1);

	    if (code < 0)
		return code;
	    pdfont->res_ToUnicode = prcmap;
	}
    pdf_open_separate(pdev, pdf_font_id(pdfont));
    s = pdev->strm;
    stream_puts(s, "<<");
    if (pdfont->BaseFont.size > 0) {
	stream_puts(s, "/BaseFont");
	pdf_put_name(pdev, pdfont->BaseFont.data, pdfont->BaseFont.size);
    }
    if (pdfont->FontDescriptor)
	pprintld1(s, "/FontDescriptor %ld 0 R",
		  pdf_font_descriptor_id(pdfont->FontDescriptor));
    if (pdfont->res_ToUnicode)
	pprintld1(s, "/ToUnicode %ld 0 R",
		  pdf_resource_id((const pdf_resource_t *)pdfont->res_ToUnicode));
    if (pdev->CompatibilityLevel > 1.0)
	stream_puts(s, "/Type/Font\n");
    else
	pprintld1(s, "/Type/Font/Name/R%ld\n", pdf_font_id(pdfont));
    if (pdev->ForOPDFRead && pdfont->global)
	stream_puts(s, "/.Global true\n");
    return pdfont->write_contents(pdev, pdfont);
}

/*
 * Close the text-related parts of a document, including writing out font
 * and related resources.
 */
private int
write_font_resources(gx_device_pdf *pdev, pdf_resource_list_t *prlist)
{
    int j;
    pdf_resource_t *pres;

    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j)
	for (pres = prlist->chains[j]; pres != 0; pres = pres->next) {
	    pdf_font_resource_t *const pdfont = (pdf_font_resource_t *)pres;
	    int code = pdf_compute_BaseFont(pdev, pdfont, true);

	    if (code < 0)
		return code;
	    code = pdf_write_font_resource(pdev, pdfont);
	    if (code < 0)
		return code;
	    pdfont->object->written = true;
	}
    return 0;
}
int
pdf_finish_font_descriptors(gx_device_pdf *pdev,
			int (*finish_proc)(gx_device_pdf *,
					   pdf_font_descriptor_t *))
{
    int j;
    pdf_resource_t *pres;

    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j)
	for (pres = pdev->resources[resourceFontDescriptor].chains[j];
	     pres != 0; pres = pres->next
	     ) {
	    int code = finish_proc(pdev, (pdf_font_descriptor_t *)pres);

	    if (code < 0)
		return code;
	}
    return 0;
}
int
pdf_close_text_document(gx_device_pdf *pdev)
{
    int code;

    /*
     * Finish the descriptors and write any embedded fonts, but don't
     * write the descriptors yet; then write the fonts; finally write
     * the descriptors.
     */

    pdf_clean_standard_fonts(pdev);
    if ((code = pdf_free_font_cache(pdev)) < 0 ||
	(code = pdf_write_resource_objects(pdev, resourceCharProc)) < 0 ||
 	(code = pdf_finish_font_descriptors(pdev, pdf_finish_FontDescriptor)) < 0 ||
  	(code = write_font_resources(pdev, &pdev->resources[resourceCIDFont])) < 0 ||
	(code = write_font_resources(pdev, &pdev->resources[resourceFont])) < 0 ||
	(code = pdf_finish_font_descriptors(pdev, pdf_write_FontDescriptor)) < 0
	)
	return code;

    /* If required, write the Encoding for Type 3 bitmap fonts. */

    return pdf_write_bitmap_fonts_Encoding(pdev);
}

/* ================ CMap resource writing ================ */

/*
 * Write the CIDSystemInfo for a CIDFont or a CMap.
 */
private int
pdf_write_cid_system_info_to_stream(gx_device_pdf *pdev, stream *s,
			  const gs_cid_system_info_t *pcidsi, gs_id object_id)
{
    byte Registry[32], Ordering[32];

    if (pcidsi->Registry.size > sizeof(Registry))
	return_error(gs_error_limitcheck);
    if (pcidsi->Ordering.size > sizeof(Ordering))
	return_error(gs_error_limitcheck);
    memcpy(Registry, pcidsi->Registry.data, pcidsi->Registry.size);
    memcpy(Ordering, pcidsi->Ordering.data, pcidsi->Ordering.size);
    if (pdev->KeyLength && object_id != 0) {
	stream_arcfour_state sarc4;
	int code; 

	code = pdf_encrypt_init(pdev, object_id, &sarc4);
	if (code < 0)
	    return code;
	s_arcfour_process_buffer(&sarc4, Registry, pcidsi->Registry.size);
	code = pdf_encrypt_init(pdev, object_id, &sarc4);
	if (code < 0)
	    return code;
	s_arcfour_process_buffer(&sarc4, Ordering, pcidsi->Ordering.size);
    }
    stream_puts(s, "<<\n/Registry");
    s_write_ps_string(s, Registry, pcidsi->Registry.size, PRINT_HEX_NOT_OK);
    stream_puts(s, "\n/Ordering");
    s_write_ps_string(s, Ordering, pcidsi->Ordering.size, PRINT_HEX_NOT_OK);
    pprintd1(s, "\n/Supplement %d\n>>\n", pcidsi->Supplement);
    return 0;
}

int
pdf_write_cid_system_info(gx_device_pdf *pdev,
			  const gs_cid_system_info_t *pcidsi, gs_id object_id)
{
    return pdf_write_cid_system_info_to_stream(pdev, pdev->strm, pcidsi, object_id);
}


/*
 * Write a CMap resource.  We pass the CMap object as well as the resource,
 * because we write CMaps when they are created.
 */
int
pdf_write_cmap(gx_device_pdf *pdev, const gs_cmap_t *pcmap,
	       pdf_resource_t **ppres /*CMap*/, int font_index_only)
{
    int code;
    pdf_data_writer_t writer;

    code = pdf_begin_data_stream(pdev, &writer,
				 DATA_STREAM_NOT_BINARY |
			    /* Don't set DATA_STREAM_ENCRYPT since we write to a temporary file.
			       See comment in pdf_begin_encrypt. */
				 (pdev->CompressFonts ? 
				  DATA_STREAM_COMPRESS : 0), gs_no_id);
    if (code < 0)
	return code;
    *ppres = writer.pres;
    writer.pres->where_used = 0; /* CMap isn't a PDF resource. */
    if (!pcmap->ToUnicode) {
	byte buf[200];
	cos_dict_t *pcd = (cos_dict_t *)writer.pres->object;
	stream s;

	code = cos_dict_put_c_key_int(pcd, "/WMode", pcmap->WMode);
	if (code < 0)
	    return code;
	buf[0] = '/';
	memcpy(buf + 1, pcmap->CMapName.data, pcmap->CMapName.size);
	code = cos_dict_put_c_key_string(pcd, "/CMapName", 
			buf, pcmap->CMapName.size + 1);
	if (code < 0)
	    return code;
	s_init(&s, pdev->memory);
	swrite_string(&s, buf, sizeof(buf));
	code = pdf_write_cid_system_info_to_stream(pdev, &s, pcmap->CIDSystemInfo, 0);
	if (code < 0)
	    return code;
	code = cos_dict_put_c_key_string(pcd, "/CIDSystemInfo", 
			buf, stell(&s));
	if (code < 0)
	    return code;
	code = cos_dict_put_string_copy(pcd, "/Type", "/CMap");
	if (code < 0)
	    return code;
    }
    code = psf_write_cmap(pdev->memory, writer.binary.strm, pcmap,
			  pdf_put_name_chars_proc(pdev), NULL, font_index_only);
    if (code < 0)
	return code;
    code = pdf_end_data(&writer);
    if (code < 0)
	return code;
    return code;
}
