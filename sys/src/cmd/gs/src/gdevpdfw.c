/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfw.c,v 1.12 2001/07/30 15:11:00 lpd Exp $ */
/* Font writing for pdfwrite driver. */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gsalloc.h"		/* for patching font memory */
#include "gsbittab.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfcid.h"
#include "gxfcmap.h"
#include "gxfont.h"
#include "gxfont0.h"
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "gdevpsf.h"
#include "scommon.h"

/* Get the ID of various kinds of resources, with type checking. */
inline private long
pdf_font_id(const pdf_font_t *pdfont)
{
    return pdf_resource_id((const pdf_resource_t *)pdfont);
}
inline private long
pdf_font_descriptor_id(const pdf_font_descriptor_t *pfd)
{
    return pdf_resource_id((const pdf_resource_t *)pfd);
}
inline private long
pdf_char_proc_id(const pdf_char_proc_t *pcp)
{
    return pdf_resource_id((const pdf_resource_t *)pcp);
}

/* Write the Widths for a font. */
private int
pdf_write_Widths(gx_device_pdf *pdev, int first, int last,
		 const int widths[256])
{
    stream *s = pdev->strm;
    int i;

    pprintd2(s, "/FirstChar %d/LastChar %d/Widths[", first, last);
    for (i = first; i <= last; ++i)
	pprintd1(s, (i & 15 ? " %d" : "\n%d"), widths[i]);
    stream_puts(s, "]\n");
    return 0;
}

/* Write a FontBBox dictionary element. */
private int
pdf_write_font_bbox(gx_device_pdf *pdev, const gs_int_rect *pbox)
{
    stream *s = pdev->strm;
    /* AR 4 doesn't like fonts with empty FontBBox which
     * happens when the font contains only space characters.
     * Small bbox causes AR 4 to display a hairline. So we use
     * the full BBox.
     */ 
    int x= pbox->q.x + ((pbox->p.x == pbox->q.x) ? 1000 : 0);
    int y= pbox->q.y + ((pbox->p.y == pbox->q.y) ? 1000 : 0);

    pprintd4(s, "/FontBBox[%d %d %d %d]",
	     pbox->p.x, pbox->p.y, x, y);
    return 0;
}

/* Write a synthesized bitmap font resource. */
private int
pdf_write_synthesized_type3(gx_device_pdf *pdev, const pdf_font_t *pef)
{
    stream *s;
    gs_int_rect bbox;
    int widths[256];

    memset(&bbox, 0, sizeof(bbox));
    memset(widths, 0, sizeof(widths));
    pdf_open_separate(pdev, pdf_font_id(pef));
    s = pdev->strm;
    pprints1(s, "<</Type/Font/Name/%s/Subtype/Type3", pef->rname);
    pprintld1(s, "/Encoding %ld 0 R/CharProcs", pdev->embedded_encoding_id);

    /* Write the CharProcs. */
    {
	const pdf_char_proc_t *pcp;
	int w;

	stream_puts(s, "<<");
	/* Write real characters. */
	for (pcp = pef->char_procs; pcp; pcp = pcp->char_next) {
	    bbox.p.y = min(bbox.p.y, pcp->y_offset);
	    bbox.q.x = max(bbox.q.x, pcp->width);
	    bbox.q.y = max(bbox.q.y, pcp->height + pcp->y_offset);
	    widths[pcp->char_code] = pcp->x_width;
	    pprintld2(s, "/a%ld\n%ld 0 R", (long)pcp->char_code,
		      pdf_char_proc_id(pcp));
	}
	/* Write space characters. */
	for (w = 0; w < countof(pef->spaces); ++w) {
	    byte ch = pef->spaces[w];

	    if (ch) {
		pprintld2(s, "/a%ld\n%ld 0 R", (long)ch,
			  pdev->space_char_ids[w]);
		widths[ch] = w + X_SPACE_MIN;
	    }
	}
	stream_puts(s, ">>");
    }

    pdf_write_font_bbox(pdev, &bbox);
    stream_puts(s, "/FontMatrix[1 0 0 1 0 0]");
    pdf_write_Widths(pdev, 0, pef->num_chars - 1, widths);
    stream_puts(s, ">>\n");
    pdf_end_separate(pdev);
    return 0;
}

/* Write a font descriptor. */
int
pdf_write_FontDescriptor(gx_device_pdf *pdev, const pdf_font_descriptor_t *pfd)
{
    gs_font *font = pfd->base_font;
    bool is_subset =
	pdf_has_subset_prefix(pfd->FontName.chars, pfd->FontName.size);
    long cidset_id = 0;		/* pro forma initialization */
    stream *s;
    int code = 0;

    /* If this is a CIDFont subset, write the CIDSet now. */
    if (font && is_subset) {
	switch (pfd->values.FontType) {
	case ft_CID_encrypted:
	case ft_CID_TrueType: {
	    pdf_data_writer_t writer;

	    cidset_id = pdf_begin_separate(pdev);
	    s = pdev->strm;
	    stream_puts(s, "<<");
	    code = pdf_begin_data(pdev, &writer);
	    if (code >= 0) {
		stream_write(writer.binary.strm, pfd->chars_used.data,
		       pfd->chars_used.size);
		code = pdf_end_data(&writer);
	    } else		/* code < 0 */
		pdf_end_separate(pdev);
	}
	default:
	    break;
	}
    }
    pdf_open_separate(pdev, pdf_font_descriptor_id(pfd));
    s = pdev->strm;
    stream_puts(s, "<</Type/FontDescriptor/FontName");
    pdf_put_name(pdev, pfd->FontName.chars, pfd->FontName.size);
    if (font) {		/* not a built-in font */
	param_printer_params_t params;
	printer_param_list_t rlist;
	gs_param_list *const plist = (gs_param_list *)&rlist;

	pdf_write_font_bbox(pdev, &pfd->values.FontBBox);
	params = param_printer_params_default;
	code = s_init_param_printer(&rlist, &params, s);
	if (code >= 0) {
#define DESC_INT(str, memb)\
 {str, gs_param_type_int, offset_of(pdf_font_descriptor_t, values.memb)}
	    static const gs_param_item_t required_items[] = {
		DESC_INT("Ascent", Ascent),
		DESC_INT("CapHeight", CapHeight),
		DESC_INT("Descent", Descent),
		DESC_INT("ItalicAngle", ItalicAngle),
		DESC_INT("StemV", StemV),
		gs_param_item_end
	    };
	    static const gs_param_item_t optional_items[] = {
		DESC_INT("AvgWidth", AvgWidth),
		DESC_INT("Leading", Leading),
		DESC_INT("MaxWidth", MaxWidth),
		DESC_INT("MissingWidth", MissingWidth),
		DESC_INT("StemH", StemH),
		DESC_INT("XHeight", XHeight),
		gs_param_item_end
	    };
#undef DESC_INT
	    int Flags = pfd->values.Flags;
	    pdf_font_descriptor_t defaults;

	    /*
	     * Hack: make all embedded subset TrueType fonts "symbolic" to
	     * work around undocumented assumptions in Acrobat Reader.
	     */
	    if (pfd->values.FontType == ft_TrueType &&
		pdf_has_subset_prefix(pfd->FontName.chars, pfd->FontName.size))
		Flags = (Flags & ~(FONT_IS_ADOBE_ROMAN)) | FONT_IS_SYMBOLIC;
	    param_write_int(plist, "Flags", &Flags);
	    gs_param_write_items(plist, pfd, NULL, required_items);
	    memset(&defaults, 0, sizeof(defaults));
	    gs_param_write_items(plist, pfd, &defaults, optional_items);
	    s_release_param_printer(&rlist);
	}
	if (is_subset) {
	    switch (pfd->values.FontType) {
	    case ft_CID_encrypted:
	    case ft_CID_TrueType:
		pprintld1(s, "/CIDSet %ld 0 R\n", cidset_id);
		break;
	    case ft_composite:
		return_error(gs_error_rangecheck);
	    case ft_encrypted: {
		gs_glyph subset_glyphs[256];
		uint subset_size = psf_subset_glyphs(subset_glyphs, font,
						     pfd->chars_used.data);
		int i;

		stream_puts(s, "/CharSet(");
		for (i = 0; i < subset_size; ++i) {
		    uint len;
		    const char *str = font->procs.callbacks.glyph_name
			(subset_glyphs[i], &len);

		    /* Don't include .notdef. */
		    if (bytes_compare((const byte *)str, len,
				      (const byte *)".notdef", 7))
			pdf_put_name(pdev, (const byte *)str, len);
		}
		stream_puts(s, ")\n");
	    }
	    default:
		break;
	    }
	}
	if (pfd->FontFile_id) {
	    const char *FontFile_key;

	    switch (pfd->values.FontType) {
	    case ft_TrueType:
	    case ft_CID_TrueType:
		FontFile_key = "/FontFile2";
		break;
	    default:
		code = gs_note_error(gs_error_rangecheck);
		/* falls through */
	    case ft_encrypted:
		if (pdev->CompatibilityLevel < 1.2) {
		    FontFile_key = "/FontFile";
		    break;
		}
		/* For PDF 1.2 and later, write Type 1 fonts as Type1C. */
	    case ft_encrypted2:
	    case ft_CID_encrypted:
		FontFile_key = "/FontFile3";
		break;
	    }
	    stream_puts(s, FontFile_key);
	    pprintld1(s, " %ld 0 R", pfd->FontFile_id);
	}
    }
    stream_puts(s, ">>\n");
    pdf_end_separate(pdev);
    return code;
}

/* Write CIDSystemInfo for a CIDFont or CMap. */
private int
pdf_write_cid_system_info(gx_device_pdf *pdev,
			  const gs_cid_system_info_t *pcidsi)
{
    stream *s = pdev->strm;

    stream_puts(s, "<<\n/Registry");
    s_write_ps_string(s, pcidsi->Registry.data, pcidsi->Registry.size,
		      PRINT_HEX_NOT_OK);
    stream_puts(s, "\n/Ordering");
    s_write_ps_string(s, pcidsi->Ordering.data, pcidsi->Ordering.size,
		      PRINT_HEX_NOT_OK);
    pprintd1(s, "\n/Supplement %d\n>>\n", pcidsi->Supplement);
    return 0;
}
int
pdf_write_CIDFont_system_info(gx_device_pdf *pdev, const gs_font *pcidfont)
{
    const gs_cid_system_info_t *pcidsi;

    switch (pcidfont->FontType) {
    case ft_CID_encrypted:
	pcidsi = &((const gs_font_cid0 *)pcidfont)->
	    cidata.common.CIDSystemInfo;
	break;
    case ft_CID_TrueType:
	pcidsi = &((const gs_font_cid2 *)pcidfont)->
	    cidata.common.CIDSystemInfo;
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    return pdf_write_cid_system_info(pdev, pcidsi);
}
int
pdf_write_CMap_system_info(gx_device_pdf *pdev, const gs_cmap_t *pcmap)
{
    return pdf_write_cid_system_info(pdev, pcmap->CIDSystemInfo);
}

/*
 * Write the [D]W[2] entries for a CIDFont.  *pfont is known to be a
 * CIDFont (type 0 or 2).
 */
private int
pdf_write_CIDFont_widths(gx_device_pdf *pdev, const pdf_font_t *ppf)
{
    /*
     * The values of the CIDFont width keys are as follows:
     *   DW = w (default 0)
     *   W = [{c [w ...] | cfirst clast w}*]
     *   DW2 = [vy w1y] (default [880 -1000])
     *   W2 = [{c [w1y vx vy ...] | cfirst clast w1y vx vy}*]
     * Currently we only write DW and W.
     */
    stream *s = pdev->strm;
    psf_glyph_enum_t genum;
    gs_glyph glyph;
    int dw = 0;

    psf_enumerate_bits_begin(&genum, NULL, ppf->widths_known,
			     ppf->FontDescriptor->chars_count,
			     GLYPH_SPACE_INDEX);

    /* Use the most common width as DW. */

    {
	ushort counts[1001];
	int dw_count = 0, i;

	memset(counts, 0, sizeof(counts));
	while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	    int width = ppf->Widths[glyph - gs_min_cid_glyph];

	    counts[min(width, countof(counts) - 1)]++;
	}
	for (i = 0; i < countof(counts); ++i)
	    if (counts[i] > dw_count)
		dw = i, dw_count = counts[i];
	if (dw != 0)
	    pprintd1(s, "/DW %d\n", dw);
    }

    /*
     * Now write all widths different from DW.  Currently we make no
     * attempt to optimize this: we write every width individually.
     */

    psf_enumerate_glyphs_reset(&genum);
    {
	int prev = -2;

	while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	    int cid = glyph - gs_min_cid_glyph;
	    int width = ppf->Widths[cid];

	    if (cid == prev + 1)
		pprintd1(s, "\n%d", width);
	    else if (width == dw)
		continue;
	    else {
		if (prev >= 0)
		    stream_puts(s, "]\n");
		else
		    stream_puts(s, "/W[");
		pprintd2(s, "%d[%d", cid, width);
	    }
	    prev = cid;
	}
	if (prev >= 0)
	    stream_puts(s, "]]");
    }    

    return 0;
}

/*
 * Write the CIDToGIDMap for a CIDFontType 2 font.  We only need to worry
 * about mapping CIDs that have actually been used.
 */
private int
pdf_write_CIDToGIDMap(gx_device_pdf *pdev, pdf_font_t *ppf,
		      long *pcidmap_id)
{
    stream *s = pdev->strm;
    psf_glyph_enum_t genum;
    gs_glyph glyph;

    psf_enumerate_bits_begin(&genum, NULL, ppf->widths_known,
			     ppf->FontDescriptor->chars_count,
			     GLYPH_SPACE_INDEX);

    /* Check for the identity map. */
    while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	int cid = glyph - gs_min_cid_glyph;
	int gid = ppf->CIDToGIDMap[cid];

	if (gid != cid) {
	    /*
	     * The map isn't Identity.  Assign an object ID for writing the
	     * map later.
	     */
	    *pcidmap_id = pdf_obj_ref(pdev);
	    pprintld1(s, "/CIDToGIDMap %ld 0 R\n", *pcidmap_id);
	    return 1;
	}
    }
    /* All CIDs and GIDs match. */
    stream_puts(s, "/CIDToGIDMap/Identity\n");
    *pcidmap_id = 0;
    return 0;
}
private int
pdf_write_CIDMap(stream *s, pdf_font_t *ppf)
{
    psf_glyph_enum_t genum;
    int next = 0, count = ppf->FontDescriptor->chars_count;
    gs_glyph glyph;

    psf_enumerate_bits_begin(&genum, NULL, ppf->widths_known, count,
			     GLYPH_SPACE_INDEX);
    while (!psf_enumerate_glyphs_next(&genum, &glyph)) {
	int cid = glyph - gs_min_cid_glyph;
	int gid = ppf->CIDToGIDMap[cid];

	for (; next < cid; ++next) {
	    stream_putc(s, 0); stream_putc(s, 0);
	}
	stream_putc(s, (byte)(gid >> 8));
	stream_putc(s, (byte)gid);
	next = cid + 1;
    }
    for (; next < count; ++next) {
	stream_putc(s, 0); stream_putc(s, 0);
    }
    return 0;
}

/*
 * Write a font resource, including Widths and/or Encoding if relevant,
 * but not the FontDescriptor or embedded font data.  Note that the
 * font itself may not be available.
 *
 * If the font is a subset, we must write the Widths even if it is a
 * built-in font.
 */
private int
pdf_write_font_resource(gx_device_pdf *pdev, pdf_font_t *pef,
			const gs_const_string *pfname)
{
    stream *s;
    const pdf_font_descriptor_t *pfd = pef->FontDescriptor;
    static const char *const encoding_names[] = {
	KNOWN_REAL_ENCODING_NAMES
    };
    /* write_Widths = 0 if not writing, 1 if Widths, -1 if [D]W[2]. */
    int write_Widths =
	(pef->write_Widths ||
	 pdf_has_subset_prefix(pfname->data, pfname->size) ? 1 : 0);
    long cidmap_id = 0;
    gs_const_string fname;
    byte fnchars[MAX_PDF_FONT_NAME];

    fname = *pfname;
    pdf_open_separate(pdev, pdf_font_id(pef));
    s = pdev->strm;
    switch (pef->FontType) {
    case ft_composite: {
	byte *chars = pef->fname.chars;
	uint size = pef->fname.size;

	stream_puts(s, "<</Type/Font/Subtype/Type0/BaseFont");
	if (pdf_has_subset_prefix(chars, size))
	    chars += SUBSET_PREFIX_SIZE, size -= SUBSET_PREFIX_SIZE;
	pdf_put_name(pdev, chars, size);
	if (pef->sub_font_type == ft_CID_encrypted &&
	    pef->cmapname[0] == '/'
	    ) {
	    stream_putc(s, '-');
	    pdf_put_name_chars(pdev, (const byte*) (pef->cmapname + 1),
			       strlen(pef->cmapname + 1));
	}
	pprints1(s, "/Encoding %s", pef->cmapname);
	pprintld1(s, "/DescendantFonts[%ld 0 R]",
		pdf_resource_id((const pdf_resource_t *)pef->DescendantFont));
	write_Widths = 0;
	goto out;
    }
    case ft_encrypted:
    case ft_encrypted2:
	/*
	 * Multiple Master instances must be identified as such iff they
	 * are not embedded.
	 */
	if (pef->is_MM_instance && !pfd->FontFile_id) {
	    /* Replace spaces in the name by underscores. */
	    uint i;

	    stream_puts(s, "<</Subtype/MMType1");
	    if (fname.size > sizeof(fnchars))
		return_error(gs_error_rangecheck);
	    for (i = 0; i < fname.size; ++i)
		fnchars[i] = (fname.data[i] == ' ' ? '_' : fname.data[i]);
	    fname.data = fnchars;
	} else {
	    stream_puts(s, "<</Subtype/Type1");
	}
    bfname:
	stream_puts(s, "/BaseFont");
	pdf_put_name(pdev, fname.data, fname.size);
	break;
    case ft_CID_encrypted:
	pprintld1(s, "<</Subtype/CIDFontType0/CIDSystemInfo %ld 0 R",
		  pef->CIDSystemInfo_id);
	write_Widths = -write_Widths;
	goto bfname;
    case ft_CID_TrueType:
	pprintld1(s, "<</Subtype/CIDFontType2/CIDSystemInfo %ld 0 R",
		  pef->CIDSystemInfo_id);
	write_Widths = -write_Widths;
	goto ttname;
    case ft_TrueType:
	stream_puts(s, "<</Subtype/TrueType");
    ttname:
	stream_puts(s, "/BaseFont");
	pdf_put_name(pdev, fname.data, fname.size);
	/****** WHAT ABOUT STYLE INFO? ******/
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    pprintld1(s, "/Type/Font/Name/R%ld", pdf_font_id(pef));
    if (pef->index < 0 || pfd->base_font || pfd->FontFile_id)
	pprintld1(s, "/FontDescriptor %ld 0 R", pdf_font_descriptor_id(pfd));
    switch (write_Widths) {
    case 0:
	break;
    case 1:
	pdf_write_Widths(pdev, pef->FirstChar, pef->LastChar, pef->Widths);
	break;
    case -1:
	pdf_write_CIDFont_widths(pdev, pef);
	if (pef->FontType == ft_CID_TrueType)
	    pdf_write_CIDToGIDMap(pdev, pef, &cidmap_id);
	break;
    }
    if (pef->Differences) {
	long diff_id = pdf_obj_ref(pdev);
	int prev = 256;
	int i;

	pprintld1(s, "/Encoding %ld 0 R>>\n", diff_id);
	pdf_end_separate(pdev);
	pdf_open_separate(pdev, diff_id);
	s = pdev->strm;
	stream_puts(s, "<</Type/Encoding");
	if (pef->BaseEncoding != ENCODING_INDEX_UNKNOWN)
	    pprints1(s, "/BaseEncoding/%s", encoding_names[pef->BaseEncoding]);
	stream_puts(s, "/Differences[");
	for (i = 0; i < 256; ++i)
	    if (pef->Differences[i].str.data != 0) {
		if (i != prev + 1)
		    pprintd1(s, "\n%d", i);
		pdf_put_name(pdev, pef->Differences[i].str.data,
			     pef->Differences[i].str.size);
		prev = i;
	    }
	stream_puts(s, "]");
    } else if (pef->BaseEncoding != ENCODING_INDEX_UNKNOWN) {
	pprints1(s, "/Encoding/%s", encoding_names[pef->BaseEncoding]);
    }
    if (cidmap_id) {
	pdf_data_writer_t writer;

	stream_puts(pdev->strm, ">>\n");
	pdf_end_separate(pdev);
	pdf_open_separate(pdev, cidmap_id);
	stream_puts(pdev->strm, "<<");
	pdf_begin_data(pdev, &writer);
	pdf_write_CIDMap(writer.binary.strm, pef);
	return pdf_end_data(&writer);
    }
 out:
    stream_puts(s, ">>\n");
    return pdf_end_separate(pdev);
}

/* Register a font for eventual writing (embedded or not). */
private GS_NOTIFY_PROC(pdf_font_notify_proc);
typedef struct pdf_font_notify_s {
    gs_memory_t *memory;	/* used to allocate this object */
    gx_device_pdf *pdev;
    pdf_font_t *pdfont;		/* 0 if only for base_font */
    pdf_font_descriptor_t *pfd;	/* 0 if not for base_font */
} pdf_font_notify_t;
gs_private_st_ptrs3(st_pdf_font_notify, pdf_font_notify_t, "pdf_font_notify_t",
		    pdf_font_notify_enum_ptrs, pdf_font_notify_reloc_ptrs,
		    pdev, pdfont, pfd);
int
pdf_register_font(gx_device_pdf *pdev, gs_font *font, pdf_font_t *ppf)
{
    /*
     * Note that we do this allocation in stable font->memory, not
     * pdev->pdf_memory.
     */
    pdf_font_descriptor_t *pfd = ppf->FontDescriptor;
    gs_font *base_font = (pfd ? pfd->base_font : NULL);
    gs_memory_t *fn_memory = gs_memory_stable(font->memory);
    pdf_font_notify_t *pfn =
	gs_alloc_struct(fn_memory, pdf_font_notify_t,
			&st_pdf_font_notify, "pdf_register_font");
    int code;

    if (pfn == 0)
	return_error(gs_error_VMerror);
    pfn->memory = fn_memory;
    pfn->pdev = pdev;
    pfn->pfd = pfd;
    if (base_font == 0 || pfd->notified)
	pfn->pfd = 0;
    else if (base_font != font) {
	/*
	 * We need two notifications: one for ppf->font, one for
	 * base_font.  Create the latter first.
	 */
	pfn->pdfont = 0;
	if_debug4('_', "[_]register 0x%lx: pdf_font_descriptor_t 0x%lx => 0x%lx, id %ld\n",
		  (ulong)pfn, (ulong)pfd, (ulong)base_font, base_font->id);
	code = gs_font_notify_register(base_font, pdf_font_notify_proc, pfn);
	if (code < 0)
	    return 0;
	pfn = gs_alloc_struct(fn_memory, pdf_font_notify_t,
			      &st_pdf_font_notify, "pdf_register_font");
	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->memory = fn_memory;
	pfn->pdev = pdev;
	pfn->pfd = 0;
    }
    if (pfd)
	pfd->notified = true;
    pfn->pdfont = ppf;
#ifdef DEBUG
    if_debug4('_', "[_]register 0x%lx: pdf_font_t 0x%lx => 0x%lx, id %ld\n",
	      (ulong)pfn, (ulong)ppf, (ulong)font, font->id);
    if (pfn->pfd)
	if_debug3('_',
		  "                pdf_font_descriptor_t 0x%lx => 0x%lx, id %ld\n",
		  (ulong)pfd, (ulong)pfd->base_font, pfd->base_font->id);
#endif
    ppf->font = font;
    return gs_font_notify_register(font, pdf_font_notify_proc, pfn);
}
private int
pdf_finalize_font_descriptor(gx_device_pdf *pdev, pdf_font_descriptor_t *pfd)
{
    gs_font *font = pfd->base_font;
    int code =
	(font ? pdf_compute_font_descriptor(pdev, pfd, font, NULL) : 0);

    if (code >= 0) {
	if (pfd->FontFile_id)
	    code = pdf_write_embedded_font(pdev, pfd);
	else
	    code = pdf_write_FontDescriptor(pdev, pfd);
	pfd->written = true;
    }
    pfd->base_font = 0;		/* clean up for GC */
    return code;
}
private int
pdf_font_notify_proc(void *vpfn /*proc_data*/, void *event_data)
{
    pdf_font_notify_t *const pfn = vpfn;
    gx_device_pdf *const pdev = pfn->pdev;
    pdf_font_t *const ppf = pfn->pdfont;	/* may be 0 */
    pdf_font_descriptor_t *pfd = pfn->pfd;	/* may be 0 */
    int code = 0;

    if (event_data)
	return 0;		/* unknown event */

    if (ppf) {
	/*
	 * The font of this pdf_font is about to be freed.  Just clear the
	 * pointer.
	 */
	gs_font *const font = ppf->font;

	if_debug4('_',
		  "[_]  notify 0x%lx: pdf_font_t 0x%lx => 0x%lx, id %ld\n",
		  (ulong)pfn, (ulong)ppf, (ulong)font, font->id);
	gs_font_notify_unregister(font, pdf_font_notify_proc, vpfn);
	ppf->font = 0;
    }

    if (pfd) {
	/*
	 * The base_font of this FontDescriptor is about to be freed.
	 * Write the FontDescriptor and, if relevant, the FontFile.
	 */
	gs_font *const font = pfd->base_font;

	if_debug4('_',
		  "[_]  notify 0x%lx: pdf_font_descriptor_t 0x%lx => 0x%lx, id %ld\n",
		  (ulong)pfn, (ulong)pfd, (ulong)font, font->id);
	gs_font_notify_unregister(font, pdf_font_notify_proc, vpfn);
	/*
	 * HACK: temporarily patch the font's memory to one that we know is
	 * available even during GC or restore.  (Eventually we need to fix
	 * this by prohibiting implementations of font_info and glyph_info
	 * from doing any allocations.)  However, don't use gs_memory_default
	 * directly, because some malloc/free implementations (such as the
	 * default one in some versions of Linux) don't recognize LIFO
	 * behavior and can wind up allocation huge amounts of address space.
	 */
	{
	    gs_memory_t *save_memory = font->memory;
	    gs_ref_memory_t *fmem =
		ialloc_alloc_state((gs_raw_memory_t *)&gs_memory_default,
				   5000);

	    if (fmem == 0)
		return_error(gs_error_VMerror);
	    font->memory = (gs_memory_t *)fmem;
	    code = pdf_finalize_font_descriptor(pdev, pfd);
	    gs_memory_free_all((gs_memory_t *)fmem, FREE_ALL_EVERYTHING,
			       "pdf_font_notify_proc");
	    font->memory = save_memory;
	}
    }
    gs_free_object(pfn->memory, vpfn, "pdf_font_notify_proc");
    return code;
}

/* Write out the font resources when wrapping up the output. */
private void
pdf_font_unreg_proc(void *vpfn /*proc_data*/)
{
    pdf_font_notify_t *const pfn = vpfn;

    gs_free_object(pfn->memory, vpfn, "pdf_font_unreg_proc");
}
/* Write out the font resources when wrapping up the output. */
int
pdf_write_font_resources(gx_device_pdf *pdev)
{
    int j;

    /* Write the fonts and descriptors. */

    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j) {
	pdf_font_t *ppf;
	pdf_font_descriptor_t *pfd;

	for (ppf = (pdf_font_t *)pdev->resources[resourceFont].chains[j];
	     ppf != 0; ppf = ppf->next
	     ) {
	    if (PDF_FONT_IS_SYNTHESIZED(ppf))
		pdf_write_synthesized_type3(pdev, ppf);
	    else {
		gs_const_string font_name;

		pfd = ppf->FontDescriptor;
		if (pfd) {
		    font_name.data = pfd->FontName.chars;
		    font_name.size = pfd->FontName.size;
		} else {	/* must be composite */
		    font_name.data = 0;
		    font_name.size = 0;
		}
		pdf_write_font_resource(pdev, ppf, &font_name);
		if (ppf->font)
		    gs_notify_unregister_calling(&ppf->font->notify_list,
						 pdf_font_notify_proc, NULL,
						 pdf_font_unreg_proc);
	    }
	}

	for (ppf = (pdf_font_t *)pdev->resources[resourceCIDFont].chains[j];
	     ppf != 0; ppf = ppf->next
	     ) {
	    gs_const_string font_name;

	    pfd = ppf->FontDescriptor;
	    font_name.data = pfd->FontName.chars;
	    font_name.size = pfd->FontName.size;
	    pdf_write_font_resource(pdev, ppf, &font_name);
	    if (ppf->font)
		gs_notify_unregister_calling(&ppf->font->notify_list,
					     pdf_font_notify_proc, NULL,
					     pdf_font_unreg_proc);
	}

	for (pfd = (pdf_font_descriptor_t *)pdev->resources[resourceFontDescriptor].chains[j];
	     pfd != 0; pfd = pfd->next
	     ) {
	    if (!pfd->written) {
		pdf_finalize_font_descriptor(pdev, pfd);
		if (pfd->base_font)
		    gs_notify_unregister_calling(&pfd->base_font->notify_list,
						 pdf_font_notify_proc, NULL,
						 pdf_font_unreg_proc);
	    }
	}

    }

    /* If required, write the Encoding for Type 3 bitmap fonts. */

    if (pdev->embedded_encoding_id) {
	stream *s;
	int i;

	pdf_open_separate(pdev, pdev->embedded_encoding_id);
	s = pdev->strm;
	/*
	 * Even though the PDF reference documentation says that a
	 * BaseEncoding key is required unless the encoding is
	 * "based on the base font's encoding" (and there is no base
	 * font in this case), Acrobat 2.1 gives an error if the
	 * BaseEncoding key is present.
	 */
	stream_puts(s, "<</Type/Encoding/Differences[0");
	for (i = 0; i <= pdev->max_embedded_code; ++i) {
	    if (!(i & 15))
		stream_puts(s, "\n");
	    pprintd1(s, "/a%d", i);
	}
	stream_puts(s, "\n] >>\n");
	pdf_end_separate(pdev);
    }

    return 0;
}
