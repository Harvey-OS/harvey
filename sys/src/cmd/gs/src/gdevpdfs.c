/* Copyright (C) 1996, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfs.c,v 1.7 2001/08/06 19:36:01 lpd Exp $ */
/* Text string processing for PDF-writing driver. */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfixed.h"		/* for gxfcache.h */
#include "gxfont.h"
#include "gxfont0.h"
#include "gxfont1.h"
#include "gxfont42.h"
#include "gxfcache.h"		/* for orig_fonts list */
#include "gxfcid.h"
#include "gxfcmap.h"
#include "gxpath.h"		/* for getting current point */
#include "gdevpsf.h"
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"		/* only to mark CMap as written */
#include "scommon.h"

/*
 * Continue processing text.  Per the check in pdf_text_begin, we know the
 * operation is not a charpath, but it could be anything else.
 */

/*
 * Define quantities derived from the current font and CTM, used within
 * the text processing loop.
 */
typedef struct pdf_text_process_state_s {
    float chars;		/* scaled character spacing (Tc) */
    float words;		/* scaled word spacing (Tw) */
    float size;			/* font size for Tf */
    gs_matrix text_matrix;	/* normalized FontMatrix * CTM for Tm */
    int mode;			/* render mode (Tr) */
    gs_font *font;
    pdf_font_t *pdfont;
} pdf_text_process_state_t;

/*
 * Declare the procedures for processing different species of text.
 * These procedures may, but need not, copy pte->text into buf
 * (a caller-supplied buffer large enough to hold the string).
 */
#define PROCESS_TEXT_PROC(proc)\
  int proc(P4(gs_text_enum_t *pte, const void *vdata, void *vbuf, uint size))
private PROCESS_TEXT_PROC(process_plain_text);
private PROCESS_TEXT_PROC(process_composite_text);
private PROCESS_TEXT_PROC(process_cmap_text);
private PROCESS_TEXT_PROC(process_cid_text);

/* Other forward declarations */
private int encoding_find_glyph(P3(gs_font_base *bfont, gs_glyph font_glyph,
				   gs_encoding_index_t index));
private int pdf_process_string(P6(pdf_text_enum_t *penum, gs_string *pstr,
				  const gs_matrix *pfmat, bool encoded,
				  pdf_text_process_state_t *pts, int *pindex));
private int pdf_update_text_state(P3(pdf_text_process_state_t *ppts,
				     const pdf_text_enum_t *penum,
				     const gs_matrix *pfmat));
private int pdf_encode_char(P4(gx_device_pdf *pdev, int chr,
			       gs_font_base *bfont, pdf_font_t *ppf));
private int pdf_encode_glyph(P5(gx_device_pdf *pdev, int chr, gs_glyph glyph,
				gs_font_base *bfont, pdf_font_t *ppf));
private int pdf_write_text_process_state(P4(gx_device_pdf *pdev,
			const gs_text_enum_t *pte,	/* for pdcolor, pis */
			const pdf_text_process_state_t *ppts,
			const gs_const_string *pstr));
private int process_text_return_width(P7(const gs_text_enum_t *pte,
					 gs_font *font, pdf_font_t *pdfont,
					 const gs_matrix *pfmat,
					 const gs_const_string *pstr,
					 int *pindex, gs_point *pdpt));
private int process_text_add_width(P7(gs_text_enum_t *pte,
				      gs_font *font, const gs_matrix *pfmat,
				      const pdf_text_process_state_t *ppts,
				      const gs_const_string *pstr,
				      int *pindex, gs_point *pdpt));

/*
 * Continue processing text.  This is the 'process' procedure in the text
 * enumerator.
 */
private int
pdf_default_text_begin(gs_text_enum_t *pte, const gs_text_params_t *text,
		       gs_text_enum_t **ppte)
{
    return gx_default_text_begin(pte->dev, pte->pis, text, pte->current_font,
				 pte->path, pte->pdcolor, pte->pcpath,
				 pte->memory, ppte);
}
int
pdf_text_process(gs_text_enum_t *pte)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;
    uint operation = pte->text.operation;
    const void *vdata;
    uint size = pte->text.size - pte->index;
    gs_text_enum_t *pte_default;
    PROCESS_TEXT_PROC((*process));
    int code = -1;		/* to force default implementation */
#define BUF_SIZE 100		/* arbitrary > 0 */

    /*
     * If we fell back to the default implementation, continue using it.
     */
 top:
    pte_default = penum->pte_default;
    if (pte_default) {
	code = gs_text_process(pte_default);
	gs_text_enum_copy_dynamic(pte, pte_default, true);
	if (code)
	    return code;
	gs_text_release(pte_default, "pdf_text_process");
	penum->pte_default = 0;
	return 0;
    }
    {
	gs_font *font = pte->current_font;

	switch (font->FontType) {
	case ft_CID_encrypted:
	case ft_CID_TrueType:
	    process = process_cid_text;
	    break;
	case ft_encrypted:
	case ft_encrypted2:
	case ft_TrueType:
	    /* The data may be either glyphs or characters. */
	    process = process_plain_text;
	    break;
	case ft_composite:
	    process =
		(((gs_font_type0 *)font)->data.FMapType == fmap_CMap ?
		 process_cmap_text :
		 process_composite_text);
	    break;
	default:
	    goto skip;
	}
    }

    /*
     * We want to process the entire string in a single call, but we may
     * need to modify it.  Copy it to a buffer.  Note that it may consist
     * of bytes, gs_chars, or gs_glyphs.
     */

    if (operation & (TEXT_FROM_STRING | TEXT_FROM_BYTES))
	vdata = pte->text.data.bytes;
    else if (operation & TEXT_FROM_CHARS)
	vdata = pte->text.data.chars, size *= sizeof(gs_char);
    else if (operation & TEXT_FROM_SINGLE_CHAR)
	vdata = &pte->text.data.d_char, size = sizeof(gs_char);
    else if (operation & TEXT_FROM_GLYPHS)
	vdata = pte->text.data.glyphs, size *= sizeof(gs_glyph);
    else if (operation & TEXT_FROM_SINGLE_GLYPH)
	vdata = &pte->text.data.d_glyph, size = sizeof(gs_glyph);
    else
	goto skip;

    if (size <= BUF_SIZE) {
	/* Use a union to ensure alignment. */
	union bu_ {
	    byte bytes[BUF_SIZE];
	    gs_char chars[BUF_SIZE / sizeof(gs_char)];
	    gs_glyph glyphs[BUF_SIZE / sizeof(gs_glyph)];
	} buf;

	code = process(pte, vdata, buf.bytes, size);
    } else {
	byte *buf = gs_alloc_string(pte->memory, size, "pdf_text_process");

	if (buf == 0)
	    return_error(gs_error_VMerror);
	code = process(pte, vdata, buf, size);
	gs_free_string(pte->memory, buf, size, "pdf_text_process");
    }

 skip:
    if (code < 0) {
	/* Fall back to the default implementation. */
	code = pdf_default_text_begin(pte, &pte->text, &pte_default);
	if (code < 0)
	    return code;
	penum->pte_default = pte_default;
	gs_text_enum_copy_dynamic(pte_default, pte, false);
    }
    /* The 'process' procedure might also have set pte_default itself. */
    if (penum->pte_default && !code)
	goto top;
    return code;
}

/*
 * Process a text string in an ordinary font.
 */
private int
process_plain_text(gs_text_enum_t *pte, const void *vdata, void *vbuf,
		   uint size)
{
    byte *const buf = vbuf;
    uint operation = pte->text.operation;
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;
    int index = 0, code;
    gs_string str;
    bool encoded;
    pdf_text_process_state_t text_state;

    if (operation & (TEXT_FROM_STRING | TEXT_FROM_BYTES)) {
	memcpy(buf, (const byte *)vdata + pte->index, size);
	encoded = false;
    } else if (operation & (TEXT_FROM_CHARS | TEXT_FROM_SINGLE_CHAR)) {
	/* Check that all chars fit in a single byte. */
	const gs_char *const cdata = vdata;
	int i;

	size /= sizeof(gs_char);
	for (i = 0; i < size; ++i) {
	    gs_char chr = cdata[pte->index + i];

	    if (chr & ~0xff)
		return_error(gs_error_rangecheck);
	    buf[i] = (byte)chr;
	}
	encoded = false;
    } else if (operation & (TEXT_FROM_GLYPHS | TEXT_FROM_SINGLE_GLYPH)) {
	/*
	 * Reverse-map the glyphs into the encoding.  Eventually, assign
	 * them to empty slots if missing.
	 */
	const gs_glyph *const gdata = vdata;
	gs_font *font = pte->current_font;
	int i;

	size /= sizeof(gs_glyph);
	code = pdf_update_text_state(&text_state, penum, &font->FontMatrix);
	if (code < 0)
	    return code;
	for (i = 0; i < size; ++i) {
	    gs_glyph glyph = gdata[pte->index + i];
	    int chr = pdf_encode_glyph((gx_device_pdf *)pte->dev, -1, glyph,
				       (gs_font_base *)font,
				       text_state.pdfont);

	    if (chr < 0)
		return_error(gs_error_rangecheck);
	    buf[i] = (byte)chr;
	}
	encoded = true;
    } else
	return_error(gs_error_rangecheck);
    str.data = buf;
    if (size > 1 && (pte->text.operation & TEXT_INTERVENE)) {
	/* Just do one character. */
	str.size = 1;
	code = pdf_process_string(penum, &str, NULL, encoded, &text_state,
				  &index);
	if (code >= 0) {
	    pte->returned.current_char = buf[0];
	    code = TEXT_PROCESS_INTERVENE;
	}
    } else {
	str.size = size;
	code = pdf_process_string(penum, &str, NULL, encoded, &text_state,
				  &index);
    }
    pte->index += index;
    return code;
}

/*
 * Process a text string in a composite font with FMapType != 9 (CMap).
 */
private int
process_composite_text(gs_text_enum_t *pte, const void *vdata, void *vbuf,
		       uint size)
{
    byte *const buf = vbuf;
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;
    int index = 0, code;
    gs_string str;
    pdf_text_process_state_t text_state;
    pdf_text_enum_t curr, prev;
    gs_point total_width;

    str.data = buf;
    if (pte->text.operation &
	(TEXT_FROM_ANY - (TEXT_FROM_STRING | TEXT_FROM_BYTES))
	)
	return_error(gs_error_rangecheck);
    if (pte->text.operation & TEXT_INTERVENE) {
	/* Not implemented. (PostScript doesn't even allow this case.) */
	return_error(gs_error_rangecheck);
    }
    total_width.x = total_width.y = 0;
    curr = *penum;
    prev = curr;
    prev.current_font = 0;
    /* Scan runs of characters in the same leaf font. */
    for ( ; ; ) {
	int font_code, buf_index;
	gs_font *new_font;

	for (buf_index = 0; ; ++buf_index) {
	    gs_char chr;
	    gs_glyph glyph;

	    gs_text_enum_copy_dynamic((gs_text_enum_t *)&prev,
				      (gs_text_enum_t *)&curr, false);
	    font_code = pte->orig_font->procs.next_char_glyph
		((gs_text_enum_t *)&curr, &chr, &glyph);
	    /*
	     * We check for a font change by comparing the current
	     * font, rather than testing the return code, because
	     * it makes the control structure a little simpler.
	     */
	    switch (font_code) {
	    case 0:		/* no font change */
	    case 1:		/* font change */
		new_font = curr.fstack.items[curr.fstack.depth].font;
		if (new_font != prev.current_font)
		    break;
		if (chr != (byte)chr)	/* probably can't happen */
		    return_error(gs_error_rangecheck);
		buf[buf_index] = (byte)chr;
		continue;
	    case 2:		/* end of string */
		break;
	    default:	/* error */
		return font_code;
	    }
	    break;
	}
	str.size = buf_index;
	gs_text_enum_copy_dynamic((gs_text_enum_t *)&curr,
				  (gs_text_enum_t *)&prev, false);
	if (buf_index) {
	    /* buf_index == 0 is only possible the very first time. */
	    /*
	     * The FontMatrix of leaf descendant fonts is not updated
	     * by scalefont.  Compute the effective FontMatrix now.
	     */
	    const gs_matrix *psmat =
		&curr.fstack.items[curr.fstack.depth - 1].font->FontMatrix;
	    gs_matrix fmat;

	    gs_matrix_multiply(&curr.current_font->FontMatrix, psmat,
			       &fmat);
	    code = pdf_process_string(&curr, &str, &fmat, false, &text_state,
				      &index);
	    if (code < 0)
		return code;
	    gs_text_enum_copy_dynamic(pte, (gs_text_enum_t *)&curr, true);
	    if (pte->text.operation & TEXT_RETURN_WIDTH) {
		pte->returned.total_width.x = total_width.x +=
		    curr.returned.total_width.x;
		pte->returned.total_width.y = total_width.y +=
		    curr.returned.total_width.y;
	    }
	}
	if (font_code == 2)
	    break;
	curr.current_font = new_font;
    }
    return code;
}

/*
 * Process a text string in a composite font with FMapType == 9 (CMap).
 */
private const char *const standard_cmap_names[] = {
    "Identity-H", "Identity-V",

    "GB-EUC-H", "GB-EUC-V",
    "GBpc-EUC-H", "GBpc-EUC-V",
    "GBK-EUC-H", "GBK-EUC-V",
    "UniGB-UCS2-H", "UniGB-UCS2-V",

    "B5pc-H", "B5pc-V",
    "ETen-B5-H", "ETen-B5-V",
    "ETenms-B5-H", "ETenms-B5-V",
    "CNS-EUC-H", "CNS-EUC-V",
    "UniCNS-UCS2-H", "UniCNS-UCS2-V",

    "83pv-RKSJ-H",
    "90ms-RKSJ-H", "90ms-RKSJ-V",
    "90msp-RKSJ-H", "90msp-RKSJ-V",
    "90pv-RKSJ-H",
    "Add-RKSJ-H", "Add-RKSJ-V",
    "EUC-H", "EUC-V",
    "Ext-RKSJ-H", "Ext-RKSJ-V",
    "H", "V",
    "UniJIS-UCS2-H", "UniJIS-UCS2-V",
    "UniJIS-UCS2-HW-H", "UniJIS-UCS2-HW-V",

    "KSC-EUC-H", "KSC-EUC-V",
    "KSCms-UHC-H", "KSCms-UHC-V",
    "KSCms-UHC-HW-H", "KSCms-UHC-HW-V",
    "KSCpc-EUC-H",
    "UniKS-UCS2-H", "UniKS-UCS2-V",

    0
};
/* Mark a glyph and all its pieces as used. */
private int
mark_glyphs_used(gs_font *font, gs_glyph orig_glyph,
		 const pdf_font_descriptor_t *pfd)
{
#define MAX_GLYPH_PIECES 10	/* arbitrary */
    gs_glyph glyphs[MAX_GLYPH_PIECES];
    int count = 1;

    glyphs[0] = orig_glyph;
    while (count > 0) {
	gs_glyph glyph = glyphs[--count];

	if (glyph >= gs_min_cid_glyph &&
	    glyph - gs_min_cid_glyph < pfd->glyphs_count
	    ) {
	    int gid = glyph - gs_min_cid_glyph;
	    int index = gid >> 3, mask = 0x80 >> (gid & 7);

	    if (!(pfd->glyphs_used.data[index] & mask)) {
		uint piece_count = 1;
		uint max_pieces = MAX_GLYPH_PIECES - 1 - count;
		psf_add_subset_pieces(&glyphs[count], &piece_count,
				      max_pieces, max_pieces, font);

		count += piece_count - 1;
		if (count > MAX_GLYPH_PIECES)
		    return_error(gs_error_limitcheck);
		pfd->glyphs_used.data[index] |= mask;
	    }
	}
    }
#undef MAX_GLYPH_PIECES
    return 0;
}
/* Record widths and CID => GID mappings. */
private int
scan_cmap_text(gs_text_enum_t *pte, gs_font *font, pdf_font_t *psubf,
	       gs_font *subfont)
{
    pdf_font_descriptor_t *pfd = psubf->FontDescriptor;

    for ( ; ; ) {
	gs_char chr;
	gs_glyph glyph;
	int code = font->procs.next_char_glyph(pte, &chr, &glyph);

	if (code == 2)		/* end of string */
	    break;
	if (code < 0)
	    return code;
	if (glyph >= gs_min_cid_glyph) {
	    uint cid = glyph - gs_min_cid_glyph;

	    if (cid < pfd->chars_count) {
		int index = cid >> 3, mask = 0x80 >> (cid & 7);

		if (!(pfd->chars_used.data[index] & mask)) {
		    pfd->chars_used.data[index] |= mask;
		    if (psubf->CIDToGIDMap) {
			gs_font_cid2 *const subfont2 =
			    (gs_font_cid2 *)subfont;
			int gid =
			    subfont2->cidata.CIDMap_proc(subfont2, glyph);

			if (gid >= 0) {
			    psubf->CIDToGIDMap[cid] = gid;
			    mark_glyphs_used(subfont, gid + gs_min_cid_glyph,
					     pfd);
			}
		    }
		}
		if (!(psubf->widths_known[index] & mask)) {
		    int width;

		    code = pdf_glyph_width(psubf, glyph, subfont, &width);
		    if (code == 0) {
			psubf->Widths[cid] = width;
			psubf->widths_known[index] |= mask;
		    }
		}
	    }
	}
    }
    return 0;
}
private int
process_cmap_text(gs_text_enum_t *pte, const void *vdata, void *vbuf,
		  uint size)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)pte->dev;
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;
    gs_font *font = pte->current_font;
    gs_font_type0 *const pfont = (gs_font_type0 *)font;
    gs_font *subfont = pfont->data.FDepVector[0];
    const gs_cmap_t *pcmap = pfont->data.CMap;
    const char *const *pcmn = standard_cmap_names;
    pdf_resource_t *pcmres = 0;
    pdf_font_t *ppf;
    pdf_font_t *psubf;
    pdf_text_process_state_t text_state;
    int code;

    if (pte->text.operation &
	(TEXT_FROM_ANY - (TEXT_FROM_STRING | TEXT_FROM_BYTES))
	)
	return_error(gs_error_rangecheck);
    if (pte->text.operation & TEXT_INTERVENE) {
	/* Not implemented.  (PostScript doesn't allow TEXT_INTERVENE.) */
	return_error(gs_error_rangecheck);
    }
    /* Require a single CID-keyed DescendantFont. */
    if (pfont->data.fdep_size != 1)
	return_error(gs_error_rangecheck);
    switch (subfont->FontType) {
    case ft_CID_encrypted:
    case ft_CID_TrueType:
	break;
    default:
	return_error(gs_error_rangecheck);
    }

    /*
     * If the CMap isn't standard, write it out if necessary.
     */

    for (; *pcmn != 0; ++pcmn)
	if (!bytes_compare((const byte *)*pcmn, strlen(*pcmn),
			   pcmap->CMapName.data, pcmap->CMapName.size))
	    break;
    if (*pcmn == 0) {		/* not standard */
	pcmres = pdf_find_resource_by_gs_id(pdev, resourceCMap, pcmap->id);
	if (pcmres == 0) {
	    /*
	     * Create the CMap object, and write the stream.
	     */
	    pdf_data_writer_t writer;
	    stream *s;

	    code = pdf_begin_resource(pdev, resourceCMap, pcmap->id, &pcmres);
	    if (code < 0)
		return code;
	    s = pdev->strm;
	    pprintd1(s, "/WMode %d/CMapName", pcmap->WMode);
	    pdf_put_name(pdev, pcmap->CMapName.data, pcmap->CMapName.size);
	    stream_puts(s, "/CIDSystemInfo");
	    code = pdf_write_CMap_system_info(pdev, pcmap);
	    if (code < 0)
		return code;
	    code = pdf_begin_data_binary(pdev, &writer, false);
	    if (code < 0)
		return code;
	    code = psf_write_cmap(writer.binary.strm, pcmap,
				  pdf_put_name_chars_proc(pdev), NULL);
	    if (code < 0)
		return code;
	    code = pdf_end_data(&writer);
	    if (code < 0)
		return code;
	    pcmres->object->written = true; /* don't try to write at end */
	}
    }

    /* Create pdf_font_t objects for the composite font and the CIDFont. */

    pte->current_font = subfont; /* CIDFont */
    {
	/* See process_composite_text re the following matrix computation. */
	gs_matrix fmat;

	gs_matrix_multiply(&subfont->FontMatrix, &font->FontMatrix, &fmat);
	code = pdf_update_text_state(&text_state, penum, &fmat);
    }
    pte->current_font = font;	/* restore composite font */
    if (code < 0)
	return code;
    psubf = text_state.pdfont;
    /* Make sure we always write glyph 0. */
    psubf->FontDescriptor->chars_used.data[0] |= 0x80;
    if (psubf->FontDescriptor->glyphs_used.data)
	psubf->FontDescriptor->glyphs_used.data[0] |= 0x80;
    code = pdf_update_text_state(&text_state, penum, &font->FontMatrix);
    if (code < 0)
	return code;
    if (code > 0)
	return_error(gs_error_rangecheck);
    ppf = text_state.pdfont;
    ppf->DescendantFont = psubf;
    /* Copy the subfont name and type to the composite font. */
    ppf->fname = psubf->FontDescriptor->FontName;
    ppf->sub_font_type = subfont->FontType;
    if (pcmres) {
	sprintf(ppf->cmapname, "%ld 0 R", pdf_resource_id(pcmres));
    } else {
	strcpy(ppf->cmapname, "/");
	strcat(ppf->cmapname, *pcmn);
    }

    /*
     * Scan the string to mark CIDs as used, record widths, and
     * (for CIDFontType 2 fonts) record CID => GID mappings and mark
     * glyphs as used.
     */

    {
	gs_text_enum_t orig;

	gs_text_enum_copy_dynamic(&orig, pte, false);
	code = scan_cmap_text(pte, font, psubf, subfont);
	gs_text_enum_copy_dynamic(pte, &orig, false);
    }

    /*
     * Let the default implementation do the rest of the work, but don't
     * allow it to draw anything in the output; instead, append the
     * characters to the output now.
     */

    {
	gs_text_params_t text;
	gs_text_enum_t *pte_default;
	gs_const_string str;
	int acode;

	text = pte->text;
	if (text.operation & TEXT_DO_DRAW)
	    text.operation =
		(text.operation - TEXT_DO_DRAW) | TEXT_DO_CHARWIDTH;
	code = pdf_default_text_begin(pte, &text, &pte_default);
	if (code < 0)
	    return code;
	str.data = vdata;
	str.size = size;
	if ((acode = pdf_write_text_process_state(pdev, pte, &text_state, &str)) < 0 ||
	    (acode = pdf_append_chars(pdev, str.data, str.size)) < 0
	    ) {
	    gs_text_release(pte_default, "process_cmap_text");
	    return acode;
	}
	/* Let the caller (pdf_text_process) handle pte_default. */
	gs_text_enum_copy_dynamic(pte_default, pte, false);
	penum->pte_default = pte_default;
	return code;
    }	
}

/*
 * Process a text string in a CIDFont.  (Only glyphshow is supported.)
 */
private int
process_cid_text(gs_text_enum_t *pte, const void *vdata, void *vbuf,
		 uint size)
{
    uint operation = pte->text.operation;
    gs_text_enum_t save;
    gs_font *scaled_font = pte->current_font;
    gs_font *font;		/* unscaled font */
    gx_device_pdf *pdev = (gx_device_pdf *)pte->dev;
    const gs_glyph *glyphs = (const gs_glyph *)vdata;
    gs_matrix scale_matrix;
    pdf_font_t *ppf;
    int code;

    if (!(operation & (TEXT_FROM_GLYPHS | TEXT_FROM_SINGLE_GLYPH)))
	return_error(gs_error_rangecheck);

    /*
     * PDF doesn't support glyphshow directly: we need to create a Type 0
     * font with an Identity CMap.  Make sure all the glyph numbers fit
     * into 16 bits.  (Eventually we should support wider glyphs too.)
     */
    {
	int i;
	byte *pchars = vbuf;

	for (i = 0; i * sizeof(gs_glyph) < size; ++i) {
	    ulong gnum = glyphs[i] - gs_min_cid_glyph;

	    if (gnum & ~0xffffL)
		return_error(gs_error_rangecheck);
	    *pchars++ = (byte)(gnum >> 8);
	    *pchars++ = (byte)gnum;
	}
    }

    /* Find the original (unscaled) version of this font. */

    for (font = scaled_font; font->base != font; )
	font = font->base;
    /* Compute the scaling matrix. */
    gs_matrix_invert(&font->FontMatrix, &scale_matrix);
    gs_matrix_multiply(&scale_matrix, &scaled_font->FontMatrix, &scale_matrix);

    /* Find or create the CIDFont resource. */

    ppf = (pdf_font_t *)
	pdf_find_resource_by_gs_id(pdev, resourceCIDFont, font->id);
    /* Check for the possibility that the base font has been freed. */
    if (ppf && ppf->FontDescriptor->written)
	ppf = 0;
    if (ppf == 0 || ppf->font == 0) {
	code = pdf_create_pdf_font(pdev, font, &font->FontMatrix, &ppf);
	if (code < 0)
	    return code;
    }

    /* Create the CMap and Type 0 font if they don't exist already. */

    if (!ppf->glyphshow_font) {
	gs_memory_t *mem = font->memory;
	gs_font_type0 *font0 = (gs_font_type0 *)
	    gs_font_alloc(mem, &st_gs_font_type0,
			  &gs_font_procs_default, NULL, "process_cid_text");
	/* We allocate Encoding dynamically only for the sake of the GC. */
	uint *encoding = (uint *)
	    gs_alloc_bytes(mem, sizeof(uint), "process_cid_text");
	gs_font **fdep = gs_alloc_struct_array(mem, 1, gs_font *,
					       &st_gs_font_ptr_element,
					       "process_cid_text");
	gs_cmap_t *pcmap;
	pdf_font_t *pgsf;

	if (font0 == 0 || encoding == 0 || fdep == 0)
	    return_error(gs_error_VMerror);
	code = gs_cmap_create_identity(&pcmap, 2, font->WMode, mem);
	if (code < 0)
	    return code;

	font0->FontMatrix = scale_matrix;
	font0->FontType = ft_composite;
	font0->procs.init_fstack = gs_type0_init_fstack;
	font0->procs.define_font = 0; /* not called */
	font0->procs.make_font = 0; /* not called */
	font0->procs.next_char_glyph = gs_type0_next_char_glyph;
	font0->key_name = font->key_name;
	font0->font_name = font->font_name;
	font0->data.FMapType = fmap_CMap;
	encoding[0] = 0;
	font0->data.Encoding = encoding;
	font0->data.encoding_size = 1;
	fdep[0] = font;
	font0->data.FDepVector = fdep;
	font0->data.fdep_size = 1;
	font0->data.CMap = pcmap;

	code = pdf_create_pdf_font(pdev, (gs_font *)font0, &font0->FontMatrix,
				   &pgsf);
	if (code < 0)
	    return code;
	pgsf->DescendantFont = ppf;
	ppf->glyphshow_font = pgsf;
    }

    /* Now handle the glyphshow as a show in the Type 0 font. */

    save = *pte;
    pte->current_font = ppf->glyphshow_font->font;
    /* Patch the operation temporarily for init_fstack. */
    pte->text.operation = (operation & ~TEXT_FROM_ANY) | TEXT_FROM_BYTES;
    /* Patch the data for process_cmap_text. */
    pte->text.data.bytes = vbuf;
    pte->text.size = size / sizeof(gs_glyph) * 2;
    pte->index = 0;
    gs_type0_init_fstack(pte, pte->current_font);
    code = process_cmap_text(pte, vbuf, vbuf, pte->text.size);
    pte->current_font = font;
    pte->text = save.text;
    pte->index = save.index + pte->index / 2;
    pte->fstack = save.fstack;
    return code;
}

/*
 * Internal procedure to process a string in a non-composite font.
 * Doesn't use or set pte->{data,size,index}; may use/set pte->xy_index;
 * may set pte->returned.total_width.
 */
private int
pdf_process_string(pdf_text_enum_t *penum, gs_string *pstr,
		   const gs_matrix *pfmat, bool encoded,
		   pdf_text_process_state_t *pts, int *pindex)
{
    gs_text_enum_t *const pte = (gs_text_enum_t *)penum;
    gx_device_pdf *const pdev = (gx_device_pdf *)penum->dev;
    gs_font *font;
    const gs_text_params_t *text = &pte->text;
    int i;
    int code = 0, mask;

    font = penum->current_font;
    if (pfmat == 0)
	pfmat = &font->FontMatrix;
    if (text->operation & TEXT_RETURN_WIDTH)
	gx_path_current_point(pte->path, &penum->origin);

    switch (font->FontType) {
    case ft_TrueType:
    case ft_encrypted:
    case ft_encrypted2:
	break;
    default:
	return_error(gs_error_rangecheck);
    }

    code = pdf_update_text_state(pts, penum, pfmat);
    if (code > 0) {
	/* Try not to emulate ADD_TO_WIDTH if we don't have to. */
	if (code & TEXT_ADD_TO_SPACE_WIDTH) {
	    if (!memchr(pstr->data, pte->text.space.s_char, pstr->size))
		code &= ~TEXT_ADD_TO_SPACE_WIDTH;
	}
    }
    if (code < 0)
	return code;
    mask = code;

    /* Check that all characters can be encoded. */

    for (i = 0; i < pstr->size; ++i) {
	int chr = pstr->data[i];
	pdf_font_t *pdfont = pts->pdfont;
	int code =
	    (encoded ? chr :
	     pdf_encode_char(pdev, chr, (gs_font_base *)font, pdfont));

	if (code < 0)
	    return code;
	/*
	 * For incrementally loaded fonts, expand FirstChar..LastChar
	 * if needed.
	 */
	if (code < pdfont->FirstChar)
	    pdfont->FirstChar = code;
	if (code > pdfont->LastChar)
	    pdfont->LastChar = code;
	pstr->data[i] = (byte)code;
    }

    /* Bring the text-related parameters in the output up to date. */
    code = pdf_write_text_process_state(pdev, pte, pts,
					(gs_const_string *)pstr);
    if (code < 0)
	return code;

    if (text->operation & TEXT_REPLACE_WIDTHS) {
	gs_point w;
	gs_matrix tmat;

	w.x = w.y = 0;
	tmat = pts->text_matrix;
	for (i = 0; i < pstr->size; *pindex = ++i, pte->xy_index++) {
	    gs_point d, dpt;

	    if (text->operation & TEXT_DO_DRAW) {
		code = pdf_append_chars(pdev, pstr->data + i, 1);
		if (code < 0)
		    return code;
	    }
	    gs_text_replaced_width(&pte->text, pte->xy_index, &d);
	    w.x += d.x, w.y += d.y;
	    gs_distance_transform(d.x, d.y, &ctm_only(pte->pis), &dpt);
	    tmat.tx += dpt.x;
	    tmat.ty += dpt.y;
	    if (i + 1 < pstr->size) {
		code = pdf_set_text_matrix(pdev, &tmat);
		if (code < 0)
		    return code;
	    }
	}
	pte->returned.total_width = w;
	if (text->operation & TEXT_RETURN_WIDTH)
	    code = gx_path_add_point(pte->path, float2fixed(tmat.tx),
				     float2fixed(tmat.ty));
	return code;
    }

    /* Write out the characters, unless we have to emulate ADD_TO_WIDTH. */
    if (mask == 0 && (text->operation & TEXT_DO_DRAW)) {
	code = pdf_append_chars(pdev, pstr->data, pstr->size);
	if (code < 0)
	    return code;
    }

    /*
     * If we don't need the total width, and don't have to emulate
     * ADD_TO_WIDTH, return now.  If the widths are available directly from
     * the font, place the characters and/or compute and return the total
     * width now.  Otherwise, call the default implementation.
     */
    {
	gs_point dpt;

	if (mask) {
	    /*
	     * Cancel the word and character spacing, since we're going
	     * to emulate them.
	     */
	    pts->words = pts->chars = 0;
	    code = pdf_write_text_process_state(pdev, pte, pts,
						(gs_const_string *)pstr);
	    if (code < 0)
		return code;
	    code = process_text_add_width(pte, font, pfmat, pts,
					  (gs_const_string *)pstr,
					  pindex, &dpt);
	    if (code < 0)
		return code;
	    if (!(text->operation & TEXT_RETURN_WIDTH))
		return 0;
	} else if (!(text->operation & TEXT_RETURN_WIDTH))
	    return 0;
	else {
	    code = process_text_return_width(pte, font, pts->pdfont, pfmat,
					     (gs_const_string *)pstr,
					     pindex, &dpt);
	    if (code < 0)
		return code;
	}
	pte->returned.total_width = dpt;
	gs_distance_transform(dpt.x, dpt.y, &ctm_only(pte->pis), &dpt);
	return gx_path_add_point(pte->path,
				 penum->origin.x + float2fixed(dpt.x),
				 penum->origin.y + float2fixed(dpt.y));
    }
}

/*
 * Compute the cached values in the text processing state from the text
 * parameters, current_font, and pis->ctm.  Return either an error code (<
 * 0) or a mask of operation attributes that the caller must emulate.
 * Currently the only such attributes are TEXT_ADD_TO_ALL_WIDTHS and
 * TEXT_ADD_TO_SPACE_WIDTH.
 */
private int
transform_delta_inverse(const gs_point *pdelta, const gs_matrix *pmat,
			gs_point *ppt)
{
    int code = gs_distance_transform_inverse(pdelta->x, pdelta->y, pmat, ppt);
    gs_point delta;

    if (code < 0)
	return code;
    if (ppt->y == 0)
	return 0;
    /* Check for numerical fuzz. */
    code = gs_distance_transform(ppt->x, 0.0, pmat, &delta);
    if (code < 0)
	return 0;		/* punt */
    if (fabs(delta.x - pdelta->x) < 0.01 && fabs(delta.y - pdelta->y) < 0.01) {
	/* Close enough to y == 0: device space error < 0.01 pixel. */
	ppt->y = 0;
    }
    return 0;
}
private int
pdf_update_text_state(pdf_text_process_state_t *ppts,
		      const pdf_text_enum_t *penum, const gs_matrix *pfmat)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)penum->dev;
    gs_font *font = penum->current_font;
    pdf_font_t *ppf;
    gs_fixed_point cpt;
    gs_matrix orig_matrix, smat, tmat;
    double
	sx = pdev->HWResolution[0] / 72.0,
	sy = pdev->HWResolution[1] / 72.0;
    float chars = 0, words = 0, size;
    int mask = 0;
    int code = gx_path_current_point(penum->path, &cpt);

    if (code < 0)
	return code;

    pdf_font_orig_matrix(font, &orig_matrix);
    DISCARD(pdf_find_orig_font(pdev, font, &orig_matrix));

    /* Compute the scaling matrix and combined matrix. */

    gs_matrix_invert(&orig_matrix, &smat);
    gs_matrix_multiply(&smat, pfmat, &smat);
    tmat = ctm_only(penum->pis);
    tmat.tx = tmat.ty = 0;
    gs_matrix_multiply(&smat, &tmat, &tmat);

    /* Try to find a reasonable size value.  This isn't necessary, */
    /* but it's worth a little effort. */

    size = fabs(tmat.yy) / sy;
    if (size < 0.01)
	size = fabs(tmat.xx) / sx;
    if (size < 0.01)
	size = 1;

    /* Check for spacing parameters we can handle, and transform them. */

    if (penum->text.operation & TEXT_ADD_TO_ALL_WIDTHS) {
	gs_point pt;

	code = transform_delta_inverse(&penum->text.delta_all, &smat, &pt);
	if (code >= 0 && pt.y == 0)
	    chars = pt.x * size;
	else
	    mask |= TEXT_ADD_TO_ALL_WIDTHS;
    }

    if (penum->text.operation & TEXT_ADD_TO_SPACE_WIDTH) {
	gs_point pt;

	code = transform_delta_inverse(&penum->text.delta_space, &smat, &pt);
	if (code >= 0 && pt.y == 0 && penum->text.space.s_char == 32)
	    words = pt.x * size;
	else
	    mask |= TEXT_ADD_TO_SPACE_WIDTH;
    }

    /* Find or create the font resource. */

    ppf = (pdf_font_t *)
	pdf_find_resource_by_gs_id(pdev, pdf_font_resource_type(font),
				   font->id);
    /* Check for the possibility that the base font has been freed. */
    if (ppf && ppf->FontDescriptor && ppf->FontDescriptor->written)
	ppf = 0;
    if (ppf == 0 || ppf->font == 0) {
	code = pdf_create_pdf_font(pdev, font, &orig_matrix, &ppf);
	if (code < 0)
	    return code;
    }

    /* Store the updated values. */

    tmat.xx /= size;
    tmat.xy /= size;
    tmat.yx /= size;
    tmat.yy /= size;
    tmat.tx += fixed2float(cpt.x);
    tmat.ty += fixed2float(cpt.y);

    ppts->chars = chars;
    ppts->words = words;
    ppts->size = size;
    ppts->text_matrix = tmat;
    ppts->mode = (font->PaintType == 0 ? 0 : 1);
    ppts->font = font;
    ppts->pdfont = ppf;

    return mask;
}

/*
 * For a given character, check whether the encoding of bfont (the current
 * font) is compatible with that of the underlying unscaled, possibly
 * standard, base font, and if not, whether we can re-encode the character
 * using the base font's encoding.  Return the (possibly re-encoded)
 * character if successful.
 *
 * We have to handle re-encoded Type 1 fonts, because TeX output makes
 * constant use of them.  We have several alternatives for how to handle
 * characters whose encoding doesn't match their encoding in the base font's
 * built-in encoding.  If a character's glyph doesn't match the character's
 * glyph in the encoding built up so far, we check if the font has that
 * glyph at all; if not, we fall back to a bitmap.  Otherwise, we use one or
 * both of the following algorithms:
 *
 *      1. If this is the first time a character at this position has been
 *      seen, assign its glyph to that position in the encoding.
 *      We do this step if the device parameter ReAssignCharacters is true.
 *      (This is the default.)
 *
 *      2. If the glyph is present in the encoding at some other position,
 *      substitute that position for the character; otherwise, assign the
 *      glyph to an unoccupied (.notdef) position.
 *      We do this step if the device parameter ReEncodeCharacters is true.
 *      (This is the default.)
 *
 *      3. Finally, fall back to using a bitmap.
 *
 * If it is essential that all strings in the output contain exactly the
 * same character codes as the input, set ReEncodeCharacters to false.  If
 * it is important that strings be searchable, but some non-searchable
 * strings can be tolerated, the defaults are appropriate.  If searchability
 * is not important, set ReAssignCharacters to false.
 * 
 * Since the chars_used bit vector keeps track of characters by their
 * position in the Encoding, not by their glyph identifier, it can't record
 * use of unencoded characters.  Therefore, if we ever use an unencoded
 * character in an embedded font, we force writing the entire font rather
 * than a subset.  This is inelegant but (for the moment) too awkward to
 * fix.
 *
 * Acrobat Reader 3's Print function has a bug that make re-encoded
 * characters print as blank if the font is substituted (not embedded or one
 * of the base 14).  To work around this bug, when CompatibilityLevel <= 1.2,
 * for non-embedded non-base fonts, no substitutions or re-encodings are
 * allowed.
 */
inline private void
record_used(pdf_font_descriptor_t *pfd, int c)
{
    pfd->chars_used.data[c >> 3] |= 0x80 >> (c & 7);
}
inline private gs_encoding_index_t
base_encoding_index(const pdf_font_t *ppf)
{
    return
	(ppf->BaseEncoding != ENCODING_INDEX_UNKNOWN ? ppf->BaseEncoding :
	 ppf->index >= 0 ? pdf_standard_fonts[ppf->index].base_encoding :
	 ENCODING_INDEX_UNKNOWN);
}

private int
pdf_encode_char(gx_device_pdf *pdev, int chr, gs_font_base *bfont,
		pdf_font_t *ppf)
{
    pdf_font_descriptor_t *const pfd = ppf->FontDescriptor;
    /*
     * bfont is the current font in which the text is being shown.
     * ei is its encoding_index.
     */
    gs_encoding_index_t ei = bfont->encoding_index;
    /*
     * base_font is the font that underlies this PDF font (i.e., this PDF
     * font is base_font plus some possible Encoding and Widths differences,
     * and possibly a different FontMatrix).  base_font is 0 iff this PDF
     * font is one of the standard 14 (i.e., ppf->index >= 0).  bei is the
     * index of the BaseEncoding (explicit or, for the standard fonts,
     * implicit) that will be written in the PDF file: it is not necessarily
     * the same as base_font->encoding_index, or even
     * base_font->nearest_encoding_index.
     */
    gs_font *base_font = pfd->base_font;
    bool have_font = base_font != 0 && base_font->FontType != ft_composite;
    bool is_standard = ppf->index >= 0;
    gs_encoding_index_t bei = base_encoding_index(ppf);
    pdf_encoding_element_t *pdiff = ppf->Differences;
    /*
     * If set, font_glyph is the glyph currently associated with chr in
     * base_font + bei + diffs; glyph is the glyph corresponding to chr in
     * bfont.
     */
    gs_glyph font_glyph, glyph;
#define IS_USED(c)\
  (((pfd)->chars_used.data[(c) >> 3] & (0x80 >> ((c) & 7))) != 0)

    if (ei == bei && ei != ENCODING_INDEX_UNKNOWN && pdiff == 0) {
	/*
	 * Just note that the character has been used with its original
	 * encoding.
	 */
	record_used(pfd, chr);
	return chr;
    }
    if (!is_standard && !have_font)
	return_error(gs_error_undefined); /* can't encode */

#define ENCODE_NO_DIFF(ch)\
   (bei != ENCODING_INDEX_UNKNOWN ?\
    bfont->procs.callbacks.known_encode((gs_char)(ch), bei) :\
    /* have_font */ bfont->procs.encode_char(base_font, ch, GLYPH_SPACE_NAME))
#define HAS_DIFF(ch) (pdiff != 0 && pdiff[ch].str.data != 0)
#define ENCODE_DIFF(ch) (pdiff[ch].glyph)
#define ENCODE(ch)\
  (HAS_DIFF(ch) ? ENCODE_DIFF(ch) : ENCODE_NO_DIFF(ch))

    font_glyph = ENCODE(chr);
    glyph =
	(ei == ENCODING_INDEX_UNKNOWN ?
	 bfont->procs.encode_char((gs_font *)bfont, chr, GLYPH_SPACE_NAME) :
	 bfont->procs.callbacks.known_encode(chr, ei));
    if (glyph == font_glyph) {
	record_used(pfd, chr);
	return chr;
    }

    return pdf_encode_glyph(pdev, chr, glyph, bfont, ppf);
}
/*
 * Encode a glyph that doesn't appear in the default place in an encoding.
 * chr will be -1 if we are encoding a glyph for glyphshow.
 */
private int
pdf_encode_glyph(gx_device_pdf *pdev, int chr, gs_glyph glyph,
		 gs_font_base *bfont, pdf_font_t *ppf)
{
    pdf_font_descriptor_t *const pfd = ppf->FontDescriptor;
    pdf_encoding_element_t *pdiff = ppf->Differences;
    gs_font *base_font = pfd->base_font;
    gs_encoding_index_t bei = base_encoding_index(ppf);

    if (ppf->index < 0 && pfd->FontFile_id == 0 &&
	pdev->CompatibilityLevel <= 1.2
	) {
	/*
	 * Work around the bug in Acrobat Reader 3's Print function
	 * that makes re-encoded characters in substituted fonts
	 * print as blank.
	 */
	return_error(gs_error_undefined);
    }

    /*
     * TrueType fonts don't allow Differences in the encoding, but we might
     * be able to re-encode the character if it appears elsewhere in the
     * encoding.
     */
    if (bfont->FontType == ft_TrueType) {
	if (pdev->ReEncodeCharacters) {
	    int c;

	    for (c = 0; c < 256; ++c)
		if (ENCODE_NO_DIFF(c) == glyph) {
		    record_used(pfd, c);
		    return c;
		}
	}
	return_error(gs_error_undefined);
    }

    /*
     * If the font isn't going to be embedded, check whether this glyph is
     * available in the base font's glyph set at all.
     */
    if (pfd->FontFile_id == 0) {
	switch (bei) {
	case ENCODING_INDEX_STANDARD:
	case ENCODING_INDEX_ISOLATIN1:
	case ENCODING_INDEX_WINANSI:
	case ENCODING_INDEX_MACROMAN:
	    if (encoding_find_glyph(bfont, glyph, ENCODING_INDEX_ALOGLYPH) < 0
	    /*
	     * One would expect that the standard Latin character set in PDF
	     * 1.3, which was released after PostScript 3, would be the full
	     * 315-character PostScript 3 set.  However, the PDF 1.3
	     * reference manual clearly specifies that the PDF 1.3 Latin
	     * character set is the smaller PostScript Level 2 set,
	     * and we have verified that this is the case in Acrobat 4.
	     * Therefore, we have commented out the second part of the
	     * conditional below.
	     */
#if 0
		&&
		(pdev->CompatibilityLevel < 1.3 ||
		 encoding_find_glyph(bfont, glyph, ENCODING_INDEX_ALXGLYPH) < 0)
#endif
		)
		return_error(gs_error_undefined);
	default:
	    break;
	}
    }

    if (pdev->ReAssignCharacters && chr >= 0) {
	/*
	 * If this is the first time we've seen this character,
	 * assign the glyph to its position in the encoding.
	 */
	if (!HAS_DIFF(chr) && !IS_USED(chr)) {
	    int code =
		pdf_add_encoding_difference(pdev, ppf, chr, bfont, glyph);

	    if (code >= 0) {
		/*
		 * As noted in the comments at the beginning of this file,
		 * we don't have any way to record, for the purpose of
		 * subsetting, the fact that a particular unencoded glyph
		 * was used.  If this glyph doesn't appear anywhere else in
		 * the base encoding, fall back to writing the entire font.
		 */
		int c;

		for (c = 0; c < 256; ++c)
		    if (ENCODE_NO_DIFF(c) == glyph)
			break;
		if (c < 256)	/* found */
		    record_used(pfd, c);
		else {		/* not found */
		    if (pfd->do_subset == FONT_SUBSET_YES)
			return_error(gs_error_undefined);
		    pfd->do_subset = FONT_SUBSET_NO;
		}
		return chr;
	    }
	}
    }

    if (pdev->ReEncodeCharacters || chr < 0) {
	/*
	 * Look for the glyph at some other position in the encoding.
	 */
	int c, code;

	for (c = 0; c < 256; ++c) {
	    if (HAS_DIFF(c)) {
		if (ENCODE_DIFF(c) == glyph)
		    return c;
	    } else if (ENCODE_NO_DIFF(c) == glyph) {
		record_used(pfd, c);
		return c;
	    }
	}
	/*
	 * The glyph isn't encoded anywhere.  Look for a
	 * never-referenced .notdef position where we can put it.
	 */
	for (c = 0; c < 256; ++c) {
	    gs_glyph font_glyph;

	    if (HAS_DIFF(c) || IS_USED(c))
		continue; /* slot already referenced */
	    font_glyph = ENCODE_NO_DIFF(c);
	    if (font_glyph == gs_no_glyph ||
		gs_font_glyph_is_notdef(bfont, font_glyph)
		)
		break;
	}
	if (c == 256)	/* no .notdef positions left */
	    return_error(gs_error_undefined);
	code = pdf_add_encoding_difference(pdev, ppf, c, bfont, glyph);
	if (code < 0)
	    return code;
	/* See under ReAssignCharacters above regarding the following: */
	if (pfd->do_subset == FONT_SUBSET_YES)
	    return_error(gs_error_undefined);
	pfd->do_subset = FONT_SUBSET_NO;
	return c;
    }

    return_error(gs_error_undefined);

#undef IS_USED
#undef ENCODE_NO_DIFF
#undef HAS_DIFF
#undef ENCODE_DIFF
#undef ENCODE
}

/*
 * Write out commands to make the output state match the processing state.
 */
private double
font_matrix_scaling(const gs_font *font)
{
    return fabs((font->FontMatrix.yy != 0 ? font->FontMatrix.yy :
		 font->FontMatrix.yx));
}
private int
pdf_write_text_process_state(gx_device_pdf *pdev,
			     const gs_text_enum_t *pte,	/* for pdcolor, pis */
			     const pdf_text_process_state_t *ppts,
			     const gs_const_string *pstr)
{
    int code;

    pdf_set_font_and_size(pdev, ppts->pdfont, ppts->size);
    code = pdf_set_text_matrix(pdev, &ppts->text_matrix);
    if (code < 0)
	return code;

    if (pdev->text.character_spacing != ppts->chars &&
	pstr->size + pdev->text.buffer_count > 1
	) {
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	pprintg1(pdev->strm, "%g Tc\n", ppts->chars);
	pdev->text.character_spacing = ppts->chars;
    }

    if (pdev->text.word_spacing != ppts->words &&
	(memchr(pstr->data, 32, pstr->size) ||
	 memchr(pdev->text.buffer, 32, pdev->text.buffer_count))
	) {
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	pprintg1(pdev->strm, "%g Tw\n", ppts->words);
	pdev->text.word_spacing = ppts->words;
    }

    if (pdev->text.render_mode != ppts->mode) {
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	pprintd1(pdev->strm, "%d Tr\n", ppts->mode);
	if (ppts->mode) {
	    /* Also write all the parameters for stroking. */
	    gs_imager_state *pis = pte->pis;
	    float save_width = pis->line_params.half_width;
	    const gs_font *font = ppts->font;
	    const gs_font *bfont = font;
	    double scaled_width = font->StrokeWidth;

	    /*
	     * The font's StrokeWidth is in the character coordinate system,
	     * which means that it should be scaled by the inverse of any
	     * scaling in the FontMatrix.  Do the best we can with this.
	     */
	    while (bfont != bfont->base)
		bfont = bfont->base;
	    scaled_width *= font_matrix_scaling(bfont) /
		font_matrix_scaling(font);
	    pis->line_params.half_width = scaled_width / 2;
	    code = pdf_prepare_stroke(pdev, pis);
	    if (code >= 0) {
		/*
		 * See stream_to_text in gdevpdfu.c re the computation of
		 * the scaling value.
		 */
		double scale = 72.0 / pdev->HWResolution[1];

		code = gdev_vector_prepare_stroke((gx_device_vector *)pdev,
						  pis, NULL, NULL, scale);
	    }
	    pis->line_params.half_width = save_width;
	    if (code < 0)
		return code;
	}
	pdev->text.render_mode = ppts->mode;
    }

    return 0;
}

/* Compute the total text width (in user space). */
private int
process_text_return_width(const gs_text_enum_t *pte, gs_font *font,
			  pdf_font_t *pdfont, const gs_matrix *pfmat,
			  const gs_const_string *pstr,
			  int *pindex, gs_point *pdpt)
{
    int i, w;
    double scale = (font->FontType == ft_TrueType ? 0.001 : 1.0);
    gs_point dpt;
    int num_spaces = 0;
    int space_char =
	(pte->text.operation & TEXT_ADD_TO_SPACE_WIDTH ?
	 pte->text.space.s_char : -1);

    for (i = *pindex, w = 0; i < pstr->size; ++i) {
	int cw;
	int code = pdf_char_width(pdfont, pstr->data[i], font, &cw);

	if (code < 0)
	    return code;
	w += cw;
	if (pstr->data[i] == space_char)
	    ++num_spaces;
    }
    gs_distance_transform(w * scale, 0.0, pfmat, &dpt);
    if (pte->text.operation & TEXT_ADD_TO_ALL_WIDTHS) {
	int num_chars = pstr->size - pte->index;

	dpt.x += pte->text.delta_all.x * num_chars;
	dpt.y += pte->text.delta_all.y * num_chars;
    }
    if (pte->text.operation & TEXT_ADD_TO_SPACE_WIDTH) {
	dpt.x += pte->text.delta_space.x * num_spaces;
	dpt.y += pte->text.delta_space.y * num_spaces;
    }
    *pindex = i;
    *pdpt = dpt;
    return 0;
}
/*
 * Emulate TEXT_ADD_TO_ALL_WIDTHS and/or TEXT_ADD_TO_SPACE_WIDTH.
 * We know that the Tw and Tc values are zero.
 */
private int
process_text_add_width(gs_text_enum_t *pte, gs_font *font,
		       const gs_matrix *pfmat,
		       const pdf_text_process_state_t *ppts,
		       const gs_const_string *pstr,
		       int *pindex, gs_point *pdpt)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)pte->dev;
    int i, w;
    double scale = (font->FontType == ft_TrueType ? 0.001 : 1.0);
    gs_point dpt;
	gs_matrix tmat;
    int space_char =
	(pte->text.operation & TEXT_ADD_TO_SPACE_WIDTH ?
	 pte->text.space.s_char : -1);
    int code = 0;
    bool move = false;

    dpt.x = dpt.y = 0;
    tmat = ppts->text_matrix;
    for (i = *pindex, w = 0; i < pstr->size; ++i) {
	int cw;
	int code = pdf_char_width(ppts->pdfont, pstr->data[i], font, &cw);
	gs_point wpt;

	if (code < 0)
	    break;
	if (move) {
	    gs_point mpt;

	    gs_distance_transform(dpt.x, dpt.y, &ctm_only(pte->pis), &mpt);
	    tmat.tx = ppts->text_matrix.tx + mpt.x;
	    tmat.ty = ppts->text_matrix.ty + mpt.y;
	    code = pdf_set_text_matrix(pdev, &tmat);
	    if (code < 0)
		break;
	    move = false;
	}
	if (pte->text.operation & TEXT_DO_DRAW) {
	    code = pdf_append_chars(pdev, &pstr->data[i], 1);
	    if (code < 0)
		break;
	}
	gs_distance_transform(cw * scale, 0.0, pfmat, &wpt);
	dpt.x += wpt.x, dpt.y += wpt.y;
	if (pte->text.operation & TEXT_ADD_TO_ALL_WIDTHS) {
	    dpt.x += pte->text.delta_all.x;
	    dpt.y += pte->text.delta_all.y;
	    move = true;
	}
	if (pstr->data[i] == space_char) {
	    dpt.x += pte->text.delta_space.x;
	    dpt.y += pte->text.delta_space.y;
	    move = true;
	}
    }
    *pindex = i;		/* only do the part we haven't done yet */
    *pdpt = dpt;
    return code;
}

/* ---------------- Text and font utilities ---------------- */

/*
 * Try to find a glyph in a (pseudo-)encoding.  If present, return the
 * index (character code); if absent, return -1.
 */
private int
encoding_find_glyph(gs_font_base *bfont, gs_glyph font_glyph,
		    gs_encoding_index_t index)
{
    int ch;
    gs_glyph glyph;

    for (ch = 0;
	 (glyph = bfont->procs.callbacks.known_encode((gs_char)ch, index)) !=
	     gs_no_glyph;
	 ++ch)
	if (glyph == font_glyph)
	    return ch;
    return -1;
}
