/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdft.c,v 1.36 2001/07/30 15:11:00 lpd Exp $ */
/* Text handling for PDF-writing driver. */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxpath.h"		/* for getting current point */
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "gdevpdfg.h"
#include "scommon.h"

/* GC descriptors */
private_st_pdf_text_enum();

/* Define the auxiliary procedures for text processing. */
private int
pdf_text_resync(gs_text_enum_t *pte, const gs_text_enum_t *pfrom)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if ((pte->text.operation ^ pfrom->text.operation) & ~TEXT_FROM_ANY)
	return_error(gs_error_rangecheck);
    if (penum->pte_default) {
	int code = gs_text_resync(penum->pte_default, pfrom);

	if (code < 0)
	    return code;
    }
    pte->text = pfrom->text;
    gs_text_enum_copy_dynamic(pte, pfrom, false);
    return 0;
}
private bool
pdf_text_is_width_only(const gs_text_enum_t *pte)
{
    const pdf_text_enum_t *const penum = (const pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_is_width_only(penum->pte_default);
    return false;
}
private int
pdf_text_current_width(const gs_text_enum_t *pte, gs_point *pwidth)
{
    const pdf_text_enum_t *const penum = (const pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_current_width(penum->pte_default, pwidth);
    return_error(gs_error_rangecheck); /* can't happen */
}
private int
pdf_text_set_cache(gs_text_enum_t *pte, const double *pw,
		   gs_text_cache_control_t control)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_set_cache(penum->pte_default, pw, control);
    return_error(gs_error_rangecheck); /* can't happen */
}
private int
pdf_text_retry(gs_text_enum_t *pte)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_retry(penum->pte_default);
    return_error(gs_error_rangecheck); /* can't happen */
}
private void
pdf_text_release(gs_text_enum_t *pte, client_name_t cname)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if (penum->pte_default) {
	gs_text_release(penum->pte_default, cname);
	penum->pte_default = 0;
    }
    gx_default_text_release(pte, cname);
}

/* Begin processing text. */
extern text_enum_proc_process(pdf_text_process); /* in gdevpdfs.c */
private const gs_text_enum_procs_t pdf_text_procs = {
    pdf_text_resync, pdf_text_process,
    pdf_text_is_width_only, pdf_text_current_width,
    pdf_text_set_cache, pdf_text_retry,
    pdf_text_release
};
int
gdev_pdf_text_begin(gx_device * dev, gs_imager_state * pis,
		    const gs_text_params_t *text, gs_font * font,
		    gx_path * path, const gx_device_color * pdcolor,
		    const gx_clip_path * pcpath,
		    gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)dev;
    pdf_text_enum_t *penum;
    gs_fixed_point cpt;
    int code;

    /* Track the dominant text rotation. */
    {
	gs_matrix tmat;
	int i;

	gs_matrix_multiply(&font->FontMatrix, &ctm_only(pis), &tmat);
	if (is_xxyy(&tmat))
	    i = (tmat.xx >= 0 ? 0 : 2);
	else if (is_xyyx(&tmat))
	    i = (tmat.xy >= 0 ? 1 : 3);
	else
	    i = 4;
	pdf_current_page(pdev)->text_rotation.counts[i] += text->size;
    }

    if (!(text->operation & TEXT_DO_DRAW) || path == 0 ||
	gx_path_current_point(path, &cpt) < 0
	)
	return gx_default_text_begin(dev, pis, text, font, path, pdcolor,
				     pcpath, mem, ppte);

    code = pdf_prepare_fill(pdev, pis);
    if (code < 0)
	return code;

    if (text->operation & TEXT_DO_DRAW) {
	/*
	 * Set the clipping path and drawing color.  We set both the fill
	 * and stroke color, because we don't know whether the fonts will be
	 * filled or stroked, and we can't set a color while we are in text
	 * mode.  (This is a consequence of the implementation, not a
	 * limitation of PDF.)
	 */

	if (pdf_must_put_clip_path(pdev, pcpath)) {
	    int code = pdf_open_page(pdev, PDF_IN_STREAM);

	    if (code < 0)
		return code;
	    pdf_put_clip_path(pdev, pcpath);
	}

	if ((code =
	     pdf_set_drawing_color(pdev, pdcolor, &pdev->stroke_color,
				   &psdf_set_stroke_color_commands)) < 0 ||
	    (code =
	     pdf_set_drawing_color(pdev, pdcolor, &pdev->fill_color,
				   &psdf_set_fill_color_commands)) < 0
	    )
	    return code;
    }

    /* Allocate and initialize the enumerator. */

    rc_alloc_struct_1(penum, pdf_text_enum_t, &st_pdf_text_enum, mem,
		      return_error(gs_error_VMerror), "gdev_pdf_text_begin");
    penum->rc.free = rc_free_text_enum;
    penum->pte_default = 0; 
    code = gs_text_enum_init((gs_text_enum_t *)penum, &pdf_text_procs,
			     dev, pis, text, font, path, pdcolor, pcpath, mem);
    if (code < 0) {
	gs_free_object(mem, penum, "gdev_pdf_text_begin");
	return code;
    }

    *ppte = (gs_text_enum_t *)penum;

    return 0;
}

/* ---------------- Text and font utilities ---------------- */

/* Forward declarations */
private int assign_char_code(P1(gx_device_pdf * pdev));

/*
 * Set the current font and size, writing a Tf command if needed.
 */
int
pdf_set_font_and_size(gx_device_pdf * pdev, pdf_font_t * font, floatp size)
{
    if (font != pdev->text.font || size != pdev->text.size) {
	int code = pdf_open_page(pdev, PDF_IN_TEXT);
	stream *s = pdev->strm;

	if (code < 0)
	    return code;
	pprints1(s, "/%s ", font->rname);
	pprintg1(s, "%g Tf\n", size);
	pdev->text.font = font;
	pdev->text.size = size;
    }
    font->where_used |= pdev->used_mask;
    return 0;
}

/*
 * Set the text matrix for writing text.
 * The translation component of the matrix is the text origin.
 * If the non-translation components of the matrix differ from the
 * current ones, write a Tm command; if there is only a Y translation
 * and it matches the leading, set use_leading so the next text string
 * will be written with ' rather than Tj; otherwise, write either a TL
 * command or a Tj command using space pseudo-characters.
 */
private int
set_text_distance(gs_point *pdist, const gs_point *ppt, const gs_matrix *pmat)
{
    double rounded;

    gs_distance_transform_inverse(pmat->tx - ppt->x, pmat->ty - ppt->y,
				  pmat, pdist);
    /* If the distance is very close to integers, round it. */
    if (fabs(pdist->x - (rounded = floor(pdist->x + 0.5))) < 0.0005)
	pdist->x = rounded;
    if (fabs(pdist->y - (rounded = floor(pdist->y + 0.5))) < 0.0005)
	pdist->y = rounded;
    return 0;
}
int
pdf_set_text_matrix(gx_device_pdf * pdev, const gs_matrix * pmat)
{
    stream *s = pdev->strm;
    double sx = 72.0 / pdev->HWResolution[0],
	sy = 72.0 / pdev->HWResolution[1];
    int code;

    if (pmat->xx == pdev->text.matrix.xx &&
	pmat->xy == pdev->text.matrix.xy &&
	pmat->yx == pdev->text.matrix.yx &&
	pmat->yy == pdev->text.matrix.yy &&
    /*
     * If we aren't already in text context, BT will reset
     * the text matrix.
     */
	(pdev->context == PDF_IN_TEXT || pdev->context == PDF_IN_STRING)
	) {
	/* Use leading, Td or a pseudo-character. */
	gs_point dist;

	set_text_distance(&dist, &pdev->text.current, pmat);
	if (dist.y == 0 && dist.x >= X_SPACE_MIN &&
	    dist.x <= X_SPACE_MAX &&
	    pdev->text.font != 0 &&
	    PDF_FONT_IS_SYNTHESIZED(pdev->text.font)
	    ) {			/* Use a pseudo-character. */
	    int dx = (int)dist.x;
	    int dx_i = dx - X_SPACE_MIN;
	    byte space_char = pdev->text.font->spaces[dx_i];

	    if (space_char == 0) {
		if (pdev->text.font != pdev->open_font)
		    goto not_spaces;
		code = assign_char_code(pdev);
		if (code <= 0)
		    goto not_spaces;
		space_char = pdev->open_font->spaces[dx_i] = (byte)code;
		if (pdev->space_char_ids[dx_i] == 0) {
		    /* Create the space char_proc now. */
		    char spstr[3 + 14 + 1];
		    stream *s;

		    sprintf(spstr, "%d 0 0 0 0 0 d1\n", dx);
		    pdev->space_char_ids[dx_i] = pdf_begin_separate(pdev);
		    s = pdev->strm;
		    pprintd1(s, "<</Length %d>>\nstream\n", strlen(spstr));
		    pprints1(s, "%sendstream\n", spstr);
		    pdf_end_separate(pdev);
		}
	    }
	    pdf_append_chars(pdev, &space_char, 1);
	    pdev->text.current.x += dx * pmat->xx;
	    pdev->text.current.y += dx * pmat->xy;
	    /* Don't change use_leading -- it only affects Y placement. */
	    return 0;
	}
      not_spaces:
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	set_text_distance(&dist, &pdev->text.line_start, pmat);
	if (pdev->text.use_leading) {
	    /* Leading was deferred: take it into account now. */
	    dist.y -= pdev->text.leading;
	}
	if (dist.x == 0 && dist.y < 0) {
	    /* Use TL, if needed, + '. */
	    float dist_y = (float)-dist.y;

	    if (fabs(pdev->text.leading - dist_y) > 0.0005) {
		pprintg1(s, "%g TL\n", dist_y);
		pdev->text.leading = dist_y;
	    }
	    pdev->text.use_leading = true;
	} else {
	    /* Use Td. */
	    pprintg2(s, "%g %g Td\n", dist.x, dist.y);
	    pdev->text.use_leading = false;
	}
    } else {			/* Use Tm. */
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	/*
	 * See stream_to_text in gdevpdf.c for why we need the following
	 * matrix adjustments.
	 */
	pprintg6(pdev->strm, "%g %g %g %g %g %g Tm\n",
		 pmat->xx * sx, pmat->xy * sy,
		 pmat->yx * sx, pmat->yy * sy,
		 pmat->tx * sx, pmat->ty * sy);
	pdev->text.matrix = *pmat;
	pdev->text.use_leading = false;
    }
    pdev->text.line_start.x = pmat->tx;
    pdev->text.line_start.y = pmat->ty;
    pdev->text.current.x = pmat->tx;
    pdev->text.current.y = pmat->ty;
    return 0;
}

/*
 * Append characters to a string being accumulated.
 */
int
pdf_append_chars(gx_device_pdf * pdev, const byte * str, uint size)
{
    const byte *p = str;
    uint left = size;

    while (left)
	if (pdev->text.buffer_count == max_text_buffer) {
	    int code = pdf_open_page(pdev, PDF_IN_TEXT);

	    if (code < 0)
		return code;
	} else {
	    int code = pdf_open_page(pdev, PDF_IN_STRING);
	    uint copy;

	    if (code < 0)
		return code;
	    copy = min(max_text_buffer - pdev->text.buffer_count, left);
	    memcpy(pdev->text.buffer + pdev->text.buffer_count, p, copy);
	    pdev->text.buffer_count += copy;
	    p += copy;
	    left -= copy;
	}
    return 0;
}

/* ---------------- Synthesized fonts ---------------- */

/* Assign a code for a char_proc. */
private int
assign_char_code(gx_device_pdf * pdev)
{
    pdf_font_t *font = pdev->open_font;
    int c;

    if (pdev->embedded_encoding_id == 0)
	pdev->embedded_encoding_id = pdf_obj_ref(pdev);
    if (font == 0 || font->num_chars == 256 || !pdev->use_open_font) {
	/* Start a new synthesized font. */
	int code = pdf_alloc_font(pdev, gs_no_id, &font, NULL, NULL);
	char *pc;

	if (code < 0)
	    return code;
	if (pdev->open_font == 0)
	    font->rname[0] = 0;
	else
	    strcpy(font->rname, pdev->open_font->rname);
	for (pc = font->rname; *pc == 'Z'; ++pc)
	    *pc = '@';
	if ((*pc)++ == 0)
	    *pc = 'A', pc[1] = 0;
	pdev->open_font = font;
	pdev->use_open_font = true;
    }
    c = font->num_chars++;
    if (c > pdev->max_embedded_code)
	pdev->max_embedded_code = c;
    return c;
}

/* Begin a CharProc for a synthesized (bitmap) font. */
int
pdf_begin_char_proc(gx_device_pdf * pdev, int w, int h, int x_width,
  int y_offset, gs_id id, pdf_char_proc_t ** ppcp, pdf_stream_position_t * ppos)
{
    pdf_resource_t *pres;
    pdf_char_proc_t *pcp;
    int char_code = assign_char_code(pdev);
    pdf_font_t *font = pdev->open_font;
    int code;

    if (char_code < 0)
	return char_code;
    code = pdf_begin_resource(pdev, resourceCharProc, id, &pres);
    if (code < 0)
	return code;
    pcp = (pdf_char_proc_t *) pres;
    pcp->font = font;
    pcp->char_next = font->char_procs;
    font->char_procs = pcp;
    pcp->char_code = char_code;
    pcp->width = w;
    pcp->height = h;
    pcp->x_width = x_width;
    pcp->y_offset = y_offset;
    font->max_y_offset = max(font->max_y_offset, h + (h >> 2));
    *ppcp = pcp;
    {
	stream *s = pdev->strm;

	/*
	 * The resource file is positionable, so rather than use an
	 * object reference for the length, we'll go back and fill it in
	 * at the end of the definition.  Take 1M as the longest
	 * definition we can handle.  (This used to be 10K, but there was
	 * a real file that exceeded this limit.)
	 */
	stream_puts(s, "<</Length       >>stream\n");
	ppos->start_pos = stell(s);
    }
    return 0;
}

/* End a CharProc. */
int
pdf_end_char_proc(gx_device_pdf * pdev, pdf_stream_position_t * ppos)
{
    stream *s = pdev->strm;
    long start_pos = ppos->start_pos;
    long end_pos = stell(s);
    long length = end_pos - start_pos;

    if (length > 999999)
	return_error(gs_error_limitcheck);
    sseek(s, start_pos - 15);
    pprintd1(s, "%d", length);
    sseek(s, end_pos);
    stream_puts(s, "endstream\n");
    pdf_end_separate(pdev);
    return 0;
}

/* Put out a reference to an image as a character in a synthesized font. */
int
pdf_do_char_image(gx_device_pdf * pdev, const pdf_char_proc_t * pcp,
		  const gs_matrix * pimat)
{
    pdf_set_font_and_size(pdev, pcp->font, 1.0);
    {
	gs_matrix tmat;

	tmat = *pimat;
	tmat.ty -= pcp->y_offset;
	pdf_set_text_matrix(pdev, &tmat);
    }
    pdf_append_chars(pdev, &pcp->char_code, 1);
    pdev->text.current.x += pcp->x_width * pdev->text.matrix.xx;
    return 0;
}
