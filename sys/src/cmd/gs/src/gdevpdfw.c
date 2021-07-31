/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfw.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Font writing for pdfwrite driver. */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmalloc.h"		/* for patching font memory */
#include "gsmatrix.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfont.h"
#include "gdevpdfx.h"
#include "gdevpdff.h"
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

/* Begin writing FontFile* data. */
private int
pdf_begin_fontfile(gx_device_pdf *pdev, long *plength_id)
{
    stream *s;

    *plength_id = pdf_obj_ref(pdev);
    s = pdev->strm;
    pputs(s, "<<");
    if (!pdev->binary_ok)
	pputs(s, "/Filter/ASCII85Decode");
    pprintld1(s, "/Length %ld 0 R", *plength_id);
    return 0;
}

/* Finish writing FontFile* data. */
private int
pdf_end_fontfile(gx_device_pdf *pdev, long start, long length_id)
{
    stream *s = pdev->strm;
    long length;

    pputs(s, "\n");
    length = pdf_stell(pdev) - start;
    pputs(s, "endstream\n");
    pdf_end_separate(pdev);
    pdf_open_separate(pdev, length_id);
    pprintld1(pdev->strm, "%ld\n", length);
    pdf_end_separate(pdev);
    return 0;
}

/* Write the FontFile data for an embedded Type 1 font. */
private int
pdf_embed_font_type1(gx_device_pdf *pdev, gs_font_type1 *font,
		     long FontFile_id, gs_glyph subset_glyphs[256],
		     uint subset_size, const gs_const_string *pfname)
{
    stream poss;
    int lengths[3];
    int code;
    long length_id;
    long start;
    psdf_binary_writer writer;

    swrite_position_only(&poss);
    /*
     * We omit the 512 zeros and the cleartomark, and set Length3 to 0.
     * Note that the interpreter adds them implicitly (per documentation),
     * so we must set MARK so that the encrypted portion pushes a mark on
     * the stack.
     */
#define TYPE1_OPTIONS (WRITE_TYPE1_EEXEC | WRITE_TYPE1_EEXEC_MARK)
    code = psdf_write_type1_font(&poss, font, TYPE1_OPTIONS,
				 subset_glyphs, subset_size, pfname, lengths);
    if (code < 0)
	return code;
    pdf_open_separate(pdev, FontFile_id);
    pdf_begin_fontfile(pdev, &length_id);
    pprintd2(pdev->strm, "/Length1 %d/Length2 %d/Length3 0>>stream\n",
	     lengths[0], lengths[1]);
    start = pdf_stell(pdev);
    code = psdf_begin_binary((gx_device_psdf *)pdev, &writer);
    if (code < 0)
	return code;
#ifdef DEBUG
    {
	int check_lengths[3];

	psdf_write_type1_font(writer.strm, font, TYPE1_OPTIONS,
			      subset_glyphs, subset_size, pfname,
			      check_lengths);
	if (writer.strm == pdev->strm &&
	    (check_lengths[0] != lengths[0] ||
	     check_lengths[1] != lengths[1] ||
	     check_lengths[2] != lengths[2])
	    ) {
	    lprintf7("Type 1 font id %ld, lengths mismatch: (%d,%d,%d) != (%d,%d,%d)\n",
		     ((gs_font *)font)->id, lengths[0], lengths[1], lengths[2],
		     check_lengths[0], check_lengths[1], check_lengths[2]);
	}
    }
#else
    psdf_write_type1_font(writer.strm, font, TYPE1_OPTIONS,
			  subset_glyphs, subset_size, pfname,
			  lengths /*ignored*/);
#endif
#undef TYPE1_OPTIONS
    psdf_end_binary(&writer);
    pdf_end_fontfile(pdev, start, length_id);
    return 0;
}

/* Write the FontFile2 data for an embedded TrueType font. */
private int
pdf_embed_font_type42(gx_device_pdf *pdev, gs_font_type42 *font,
		      long FontFile_id, gs_glyph subset_glyphs[256],
		      uint subset_size, const gs_const_string *pfname)
{
    stream poss;
    int length;
    int code;
    long length_id;
    long start;
    psdf_binary_writer writer;
    /* Acrobat Reader 3 doesn't handle cmap format 6 correctly. */
    const int options = WRITE_TRUETYPE_CMAP | WRITE_TRUETYPE_NAME |
	(pdev->CompatibilityLevel <= 1.2 ?
	 WRITE_TRUETYPE_NO_TRIMMED_TABLE : 0);

    swrite_position_only(&poss);
    code = psdf_write_truetype_font(&poss, font, options,
				    subset_glyphs, subset_size, pfname);
    if (code < 0)
	return code;
    length = stell(&poss);
    pdf_open_separate(pdev, FontFile_id);
    pdf_begin_fontfile(pdev, &length_id);
    pprintd1(pdev->strm, "/Length1 %d>>stream\n", length);
    start = pdf_stell(pdev);
    code = psdf_begin_binary((gx_device_psdf *)pdev, &writer);
    if (code < 0)
	return code;
    psdf_write_truetype_font(writer.strm, font, options,
			     subset_glyphs, subset_size, pfname);
    psdf_end_binary(&writer);
    pdf_end_fontfile(pdev, start, length_id);
    return 0;
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
    pputs(s, "]\n");
    return 0;
}

/* Write a FontBBox dictionary element. */
private int
pdf_write_font_bbox(gx_device_pdf *pdev, const gs_int_rect *pbox)
{
    stream *s = pdev->strm;

    pprintd4(s, "/FontBBox[%d %d %d %d]",
	     pbox->p.x, pbox->p.y, pbox->q.x, pbox->q.y);
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
    pprints1(s, "<</Type/Font/Name/%s/Subtype/Type3", pef->frname);
    pprintld1(s, "/Encoding %ld 0 R/CharProcs", pdev->embedded_encoding_id);

    /* Write the CharProcs. */
    {
	const pdf_char_proc_t *pcp;
	int w;

	pputs(s, "<<");
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
	pputs(s, ">>");
    }

    pdf_write_font_bbox(pdev, &bbox);
    pputs(s, "/FontMatrix[1 0 0 1 0 0]");
    pdf_write_Widths(pdev, 0, pef->num_chars - 1, widths);
    pputs(s, ">>\n");
    pdf_end_separate(pdev);
    return 0;
}

/* Write a font descriptor. */
private int
pdf_write_FontDescriptor(gx_device_pdf *pdev,
			 const pdf_font_descriptor_t *pfd)
{
    stream *s;
    int code = 0;

    pdf_open_separate(pdev, pdf_font_descriptor_id(pfd));
    s = pdev->strm;
    pputs(s, "<</Type/FontDescriptor/FontName");
    pdf_put_name(pdev, pfd->FontName.chars, pfd->FontName.size);
    if (pfd->base_font) {	/* not a built-in font */
	param_printer_params_t params;
	static const param_printer_params_t ppp_defaults = {
	    param_printer_params_default_values
	};
	printer_param_list_t rlist;
	gs_param_list *const plist = (gs_param_list *)&rlist;

	pdf_write_font_bbox(pdev, &pfd->values.FontBBox);
	params = ppp_defaults;
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
	if (pdf_has_subset_prefix(pfd->FontName.chars, pfd->FontName.size)) {
	    gs_font *font = pfd->base_font;
	    gs_glyph subset_glyphs[256];
	    uint subset_size = psdf_subset_glyphs(subset_glyphs, font,
						  pfd->chars_used);
	    int i;

	    pputs(s, "/CharSet(");
	    for (i = 0; i < subset_size; ++i) {
		uint len;
		const char *str = font->procs.callbacks.glyph_name
		    (subset_glyphs[i], &len);

		/* Don't include .notdef. */
		if (bytes_compare((const byte *)str, len,
				  (const byte *)".notdef", 7))
		    pdf_put_name(pdev, (const byte *)str, len);
	    }
	    pputs(s, ")\n");
	}
	if (pfd->FontFile_id) {
	    const char *FontFile_key;

	    switch (pfd->values.FontType) {
	    case ft_encrypted:
		FontFile_key = "/FontFile";
		break;
	    case ft_TrueType:
	    case ft_CID_TrueType:
		FontFile_key = "/FontFile2";
		break;
	    default:
		code = gs_note_error(gs_error_rangecheck);
		/* falls through */
	    case ft_encrypted2:
	    case ft_CID_encrypted:
		FontFile_key = "/FontFile3";
		break;
	    }
	    pputs(s, FontFile_key);
	    pprintld1(s, " %ld 0 R", pfd->FontFile_id);
	}
    }
    pputs(s, ">>\n");
    pdf_end_separate(pdev);
    return code;
}

/*
 * Write a Type 1 or TrueType font resource, including Widths and/or
 * Encoding if relevant, but not the FontDescriptor.  Note that the
 * font itself may not be available.
 */
private int
pdf_write_font_resource(gx_device_pdf *pdev, const pdf_font_t *pef,
			const gs_const_string *pfname,
			const gs_const_string *pbfname)
{
    stream *s;
    const pdf_font_descriptor_t *pfd = pef->FontDescriptor;
    static const char *const encoding_names[] = {
	KNOWN_REAL_ENCODING_NAMES
    };

    pdf_open_separate(pdev, pdf_font_id(pef));
    s = pdev->strm;
    switch (pef->FontType) {
    case ft_encrypted: {
	/*
	 * If the font is a Multiple Master instance, it needs to be
	 * identified as such.
	 */
	if (pef->is_MM_instance) {
	    pputs(s, "<</Subtype/MMType1/BaseFont");
	    /****** NAME IS WRONG ******/
	    pdf_put_name(pdev, pbfname->data, pbfname->size);
	} else {
	    pputs(s, "<</Subtype/Type1/BaseFont");
	    pdf_put_name(pdev, pbfname->data, pbfname->size);
	}
    }
	break;
    case ft_TrueType:
	pputs(s, "<</Subtype/TrueType/BaseFont");
	/****** WHAT ABOUT STYLE INFO? ******/
	pdf_put_name(pdev, pbfname->data, pbfname->size);
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    pprintld1(s, "/Type/Font/Name/R%ld", pdf_font_id(pef));
    if (pef->index < 0 || pfd->base_font || pfd->FontFile_id)
	pprintld1(s, "/FontDescriptor %ld 0 R", pdf_font_descriptor_id(pfd));
    if (pef->write_Widths)
	pdf_write_Widths(pdev, pef->FirstChar, pef->LastChar, pef->Widths);
    if (pef->Differences) {
	long diff_id = pdf_obj_ref(pdev);
	int prev = 256;
	int i;

	pprintld1(s, "/Encoding %ld 0 R>>\n", diff_id);
	pdf_end_separate(pdev);
	pdf_open_separate(pdev, diff_id);
	s = pdev->strm;
	pputs(s, "<</Type/Encoding");
	if (pef->BaseEncoding != ENCODING_INDEX_UNKNOWN)
	    pprints1(s, "/BaseEncoding/%s", encoding_names[pef->BaseEncoding]);
	pputs(s, "/Differences[");
	for (i = 0; i < 256; ++i)
	    if (pef->Differences[i].str.data != 0) {
		if (i != prev + 1)
		    pprintd1(s, "\n%d", i);
		pdf_put_name(pdev, pef->Differences[i].str.data,
			     pef->Differences[i].str.size);
		prev = i;
	    }
	pputs(s, "]");
    } else if (pef->BaseEncoding != ENCODING_INDEX_UNKNOWN) {
	pprints1(s, "/Encoding/%s", encoding_names[pef->BaseEncoding]);
    }
    pputs(s, ">>\n");
    return pdf_end_separate(pdev);
}

/*
 * Write the FontDescriptor and FontFile* data for an embedded font.
 * Return a rangecheck error if the font can't be embedded.
 */
private int
pdf_write_embedded_font(gx_device_pdf *pdev, pdf_font_descriptor_t *pfd)
{
    gs_font *font = pfd->base_font;
    gs_const_string font_name;
    byte *fnchars = pfd->FontName.chars;
    uint fnsize = pfd->FontName.size;
    bool do_subset = pfd->subset_ok && pdev->params.SubsetFonts &&
	pdev->params.MaxSubsetPct > 0;
    long FontFile_id = pfd->FontFile_id;
    gs_glyph subset_glyphs[256];
    gs_glyph *glyph_subset = 0;
    uint subset_size = 0;
    gs_matrix save_mat;
    int code;

    /* Determine whether to subset the font. */
    if (do_subset) {
	int used, i, total, index;
	gs_glyph ignore_glyph;

	for (i = 0, used = 0; i < 256/8; ++i)
	    used += byte_count_bits[pfd->chars_used[i]];
	for (index = 0, total = 0;
	     (font->procs.enumerate_glyph(font, &index, GLYPH_SPACE_INDEX,
					  &ignore_glyph), index != 0);
	     )
	    ++total;
	if ((double)used / total > pdev->params.MaxSubsetPct / 100.0)
	    do_subset = false;
	else {
	    subset_size = psdf_subset_glyphs(subset_glyphs, font,
					     pfd->chars_used);
	    glyph_subset = subset_glyphs;
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
	pdf_make_subset_prefix(fnchars, FontFile_id);
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
	case ft_encrypted:
	    code = pdf_embed_font_type1(pdev, (gs_font_type1 *)font,
					FontFile_id, glyph_subset,
					subset_size, &font_name);
	    break;
	case ft_TrueType:
	    code = pdf_embed_font_type42(pdev, (gs_font_type42 *)font,
					 FontFile_id, glyph_subset,
					 subset_size, &font_name);
	    break;
	default:
	    code = gs_note_error(gs_error_rangecheck);
	}
	font->FontMatrix = save_mat;
    }
    return code;
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
    gs_font *base_font = pfd->base_font;
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
	gs_memory_t *save_memory = font->memory;

	if_debug4('_',
		  "[_]  notify 0x%lx: pdf_font_descriptor_t 0x%lx => 0x%lx, id %ld\n",
		  (ulong)pfn, (ulong)pfd, (ulong)font, font->id);
	gs_font_notify_unregister(font, pdf_font_notify_proc, vpfn);
	/*
	 * HACK: temporarily patch the font's memory to one that we know is
	 * available even during GC or restore.  (Eventually we need to fix
	 * this by prohibiting implementations of font_info and glyph_info
	 * from doing any allocations.)
	 */
	font->memory = &gs_memory_default;
	code = pdf_finalize_font_descriptor(pdev, pfd);
	font->memory = save_memory;
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
int
pdf_write_font_resources(gx_device_pdf *pdev)
{
    int j;

    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j) {
	pdf_font_t *ppf;
	pdf_font_descriptor_t *pfd;

	for (ppf = (pdf_font_t *)pdev->resources[resourceFont].chains[j];
	     ppf != 0; ppf = ppf->next
	     ) {
	    if (PDF_FONT_IS_SYNTHESIZED(ppf))
		pdf_write_synthesized_type3(pdev, ppf);
	    else if (!ppf->skip) {
		gs_const_string font_name;

		pfd = ppf->FontDescriptor;
		font_name.data = pfd->FontName.chars;
		font_name.size = pfd->FontName.size;
		/*
		 * If the font is based on one of the built-in fonts,
		 * we already ensured it uses the built-in name.
		 * If it is a subset, we must write the Widths even if
		 * it is a built-in font.
		 */
		if (pdf_has_subset_prefix(font_name.data, font_name.size))
		    ppf->write_Widths = true;
		pdf_write_font_resource(pdev, ppf, &font_name, &font_name);
		if (ppf->font)
		    gs_notify_unregister_calling(&ppf->font->notify_list,
						 pdf_font_notify_proc, NULL,
						 pdf_font_unreg_proc);
	    }
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

    /* Unregister the standard fonts. */

    pdf_unregister_fonts(pdev);

    return 0;
}
