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

/* $Id: gdevpdti.c,v 1.53 2005/10/12 08:16:50 leonardo Exp $ */
/* Bitmap font implementation for pdfwrite */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gxpath.h"
#include "gserrors.h"
#include "gsutil.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdtf.h"
#include "gdevpdti.h"
#include "gdevpdts.h"
#include "gdevpdtw.h"
#include "gdevpdtt.h"
#include "gdevpdfo.h"

/* ---------------- Private ---------------- */

/* Define the structure for a CharProc pseudo-resource. */
/*typedef struct pdf_char_proc_s pdf_char_proc_t;*/  /* gdevpdfx.h */
struct pdf_char_proc_s {
    pdf_resource_common(pdf_char_proc_t);
    pdf_font_resource_t *font;
    pdf_char_proc_t *char_next;	/* next char_proc for same font */
    int y_offset;		/* of character (0,0) */
    gs_char char_code;
    gs_const_string char_name;
    gs_point real_width;        /* Not used with synthesised bitmap fonts. */
    gs_point v;			/* Not used with synthesised bitmap fonts. */
};

/* The descriptor is public for pdf_resource_type_structs. */
gs_public_st_suffix_add2_string1(st_pdf_char_proc, pdf_char_proc_t,
  "pdf_char_proc_t", pdf_char_proc_enum_ptrs, pdf_char_proc_reloc_ptrs,
  st_pdf_resource, font, char_next, char_name);

/* Define the state structure for tracking bitmap fonts. */
/*typedef struct pdf_bitmap_fonts_s pdf_bitmap_fonts_t;*/
struct pdf_bitmap_fonts_s {
    pdf_font_resource_t *open_font;  /* current Type 3 synthesized font */
    bool use_open_font;		/* if false, start new open_font */
    long bitmap_encoding_id;
    int max_embedded_code;	/* max Type 3 code used */
};
gs_private_st_ptrs1(st_pdf_bitmap_fonts, pdf_bitmap_fonts_t,
  "pdf_bitmap_fonts_t", pdf_bitmap_fonts_enum_ptrs,
  pdf_bitmap_fonts_reloc_ptrs, open_font);

inline private long
pdf_char_proc_id(const pdf_char_proc_t *pcp)
{
    return pdf_resource_id((const pdf_resource_t *)pcp);
}

/* Assign a code for a char_proc. */
private int
assign_char_code(gx_device_pdf * pdev, int width)
{
    pdf_bitmap_fonts_t *pbfs = pdev->text->bitmap_fonts;
    pdf_font_resource_t *pdfont = pbfs->open_font; /* Type 3 */
    int c, code;

    if (pbfs->bitmap_encoding_id == 0)
	pbfs->bitmap_encoding_id = pdf_obj_ref(pdev);
    if (pdfont == 0 || pdfont->u.simple.LastChar == 255 ||
	!pbfs->use_open_font
	) {
	/* Start a new synthesized font. */
	char *pc;

	code = pdf_font_type3_alloc(pdev, &pdfont, pdf_write_contents_bitmap);
	if (code < 0)
	    return code;
        pdfont->u.simple.s.type3.bitmap_font = true;
	if (pbfs->open_font == 0)
	    pdfont->rname[0] = 0;
	else
	    strcpy(pdfont->rname, pbfs->open_font->rname);
	pdfont->u.simple.s.type3.FontBBox.p.x = 0;
	pdfont->u.simple.s.type3.FontBBox.p.y = 0;
	pdfont->u.simple.s.type3.FontBBox.q.x = 1000;
	pdfont->u.simple.s.type3.FontBBox.q.y = 1000;
	gs_make_identity(&pdfont->u.simple.s.type3.FontMatrix);
	/*
	 * We "increment" the font name as a radix-26 "number".
	 * This cannot possibly overflow.
	 */
	for (pc = pdfont->rname; *pc == 'Z'; ++pc)
	    *pc = '@';
	if ((*pc)++ == 0)
	    *pc = 'A', pc[1] = 0;
	pbfs->open_font = pdfont;
	pbfs->use_open_font = true;
	pdfont->u.simple.FirstChar = 0;
    }
    c = ++(pdfont->u.simple.LastChar);
    pdfont->Widths[c] = psdf_round(pdev->char_width.x, 100, 10); /* See 
			pdf_write_Widths about rounding. We need to provide 
			a compatible data for Tj. */
    if (c > pbfs->max_embedded_code)
	pbfs->max_embedded_code = c;

    /* Synthezise ToUnicode CMap :*/
    {	gs_text_enum_t *pte = pdev->pte;
        gs_font *font = pte->current_font;

	code = pdf_add_ToUnicode(pdev, font, pdfont, pte->returned.current_glyph, c); 
	if (code < 0)
	    return code;
    }
    return c;
}

/* Write the contents of a Type 3 bitmap or vector font resource. */
int
pdf_write_contents_bitmap(gx_device_pdf *pdev, pdf_font_resource_t *pdfont)
{
    stream *s = pdev->strm;
    const pdf_char_proc_t *pcp;
    long diff_id = 0;
    int code;

    if (pdfont->u.simple.s.type3.bitmap_font)
	diff_id = pdev->text->bitmap_fonts->bitmap_encoding_id;
    else {
	/* See comment in pdf_write_encoding. */
        diff_id = pdf_obj_ref(pdev);
    }
    code = pdf_write_encoding_ref(pdev, pdfont, diff_id);
    if (code < 0)
	return code;
    stream_puts(s, "/CharProcs <<");
    /* Write real characters. */
    for (pcp = pdfont->u.simple.s.type3.char_procs; pcp;
	 pcp = pcp->char_next
	 ) {
	if (pdfont->u.simple.s.type3.bitmap_font)
	    pprintld2(s, "/a%ld %ld 0 R\n", (long)pcp->char_code,
		      pdf_char_proc_id(pcp));
	else {
	    pdf_put_name(pdev, pcp->char_name.data, pcp->char_name.size);
	    pprintld1(s, " %ld 0 R\n", pdf_char_proc_id(pcp));
	}
    }
    stream_puts(s, ">>");
    pprintg6(s, "/FontMatrix[%g %g %g %g %g %g]", 
	    (float)pdfont->u.simple.s.type3.FontMatrix.xx,
	    (float)pdfont->u.simple.s.type3.FontMatrix.xy,
	    (float)pdfont->u.simple.s.type3.FontMatrix.yx,
	    (float)pdfont->u.simple.s.type3.FontMatrix.yy,
	    (float)pdfont->u.simple.s.type3.FontMatrix.tx,
	    (float)pdfont->u.simple.s.type3.FontMatrix.ty);
    code = pdf_finish_write_contents_type3(pdev, pdfont);
    if (code < 0)
	return code;
    s = pdev->strm; /* pdf_finish_write_contents_type3 changes pdev->strm . */
    if (!pdfont->u.simple.s.type3.bitmap_font && diff_id > 0) {
	code = pdf_write_encoding(pdev, pdfont, diff_id, 0);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* ---------------- Public ---------------- */

/*
 * Allocate and initialize bookkeeping for bitmap fonts.
 */
pdf_bitmap_fonts_t *
pdf_bitmap_fonts_alloc(gs_memory_t *mem)
{
    pdf_bitmap_fonts_t *pbfs =
	gs_alloc_struct(mem, pdf_bitmap_fonts_t, &st_pdf_bitmap_fonts,
			"pdf_bitmap_fonts_alloc");

    if (pbfs == 0)
	return 0;
    memset(pbfs, 0, sizeof(*pbfs));
    pbfs->max_embedded_code = -1;
    return pbfs;
}

/*
 * Update text state at the end of a page.
 */
void
pdf_close_text_page(gx_device_pdf *pdev)
{
    /*
     * When Acrobat Reader 3 prints a file containing a Type 3 font with a
     * non-standard Encoding, it apparently only emits the subset of the
     * font actually used on the page.  Thus, if the "Download Fonts Once"
     * option is selected, characters not used on the page where the font
     * first appears will not be defined, and hence will print as blank if
     * used on subsequent pages.  Thus, we can't allow a Type 3 font to
     * add additional characters on subsequent pages.
     */
    if (pdev->CompatibilityLevel <= 1.2)
	pdev->text->bitmap_fonts->use_open_font = false;
}

/* Return the Y offset for a bitmap character image. */
int
pdf_char_image_y_offset(const gx_device_pdf *pdev, int x, int y, int h)
{
    const pdf_text_data_t *const ptd = pdev->text;
    gs_point pt;
    int max_off, off;

    pdf_text_position(pdev, &pt);
    if (x < pt.x)
	return 0;
    max_off = (ptd->bitmap_fonts->open_font == 0 ? 0 :
	       ptd->bitmap_fonts->open_font->u.simple.s.type3.max_y_offset);
    off = (y + h) - (int)(pt.y + 0.5);
    if (off < -max_off || off > max_off)
	off = 0;
    return off;
}

/* Begin a CharProc for a synthesized font. */
private int
pdf_begin_char_proc_generic(gx_device_pdf * pdev, pdf_font_resource_t *pdfont,
		    gs_id id, gs_char char_code, 
		    pdf_char_proc_t ** ppcp, pdf_stream_position_t * ppos)
{
    pdf_resource_t *pres;
    pdf_char_proc_t *pcp;
    int code;

    code = pdf_begin_resource(pdev, resourceCharProc, id, &pres);
    if (code < 0)
	return code;
    pcp = (pdf_char_proc_t *) pres;
    pcp->font = pdfont;
    pcp->char_next = pdfont->u.simple.s.type3.char_procs;
    pdfont->u.simple.s.type3.char_procs = pcp;
    pcp->char_code = char_code;
    pres->object->written = true;
    pcp->char_name.data = 0; 
    pcp->char_name.size = 0;

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
    code = pdf_begin_encrypt(pdev, &pdev->strm, pres->object->id);
    if (code < 0)
	return code;
    *ppcp = pcp;
    return 0;
}

/* Begin a CharProc for a synthesized (bitmap) font. */
int
pdf_begin_char_proc(gx_device_pdf * pdev, int w, int h, int x_width,
		    int y_offset, gs_id id, pdf_char_proc_t ** ppcp,
		    pdf_stream_position_t * ppos)
{
    int char_code = assign_char_code(pdev, x_width);
    pdf_bitmap_fonts_t *const pbfs = pdev->text->bitmap_fonts; 
    pdf_font_resource_t *font = pbfs->open_font; /* Type 3 */
    int code = pdf_begin_char_proc_generic(pdev, font, id, char_code, ppcp, ppos);
    
    if (code < 0)
	return code;
    (*ppcp)->y_offset = y_offset;
    font->u.simple.s.type3.FontBBox.p.y =
	min(font->u.simple.s.type3.FontBBox.p.y, y_offset);
    font->u.simple.s.type3.FontBBox.q.x =
	max(font->u.simple.s.type3.FontBBox.q.x, w);
    font->u.simple.s.type3.FontBBox.q.y =
	max(font->u.simple.s.type3.FontBBox.q.y, y_offset + h);
    font->u.simple.s.type3.max_y_offset =
	max(font->u.simple.s.type3.max_y_offset, h + (h >> 2));
    return 0;
}

/* End a CharProc. */
int
pdf_end_char_proc(gx_device_pdf * pdev, pdf_stream_position_t * ppos)
{
    stream *s;
    long start_pos, end_pos, length;

    pdf_end_encrypt(pdev);
    s = pdev->strm;
    start_pos = ppos->start_pos;
    end_pos = stell(s);
    length = end_pos - start_pos;
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
    pdf_font_resource_t *pdfont = pcp->font;
    byte ch = pcp->char_code;
    pdf_text_state_values_t values;

    values.character_spacing = 0;
    values.pdfont = pdfont;
    values.size = 1;
    values.matrix = *pimat;
    values.matrix.ty -= pcp->y_offset;
    values.render_mode = 0;
    values.word_spacing = 0;
    pdf_set_text_state_values(pdev, &values);
    pdf_append_chars(pdev, &ch, 1, pdfont->Widths[ch] * pimat->xx, 0.0, false);
    return 0;
}

/*
 * Write the Encoding for bitmap fonts, if needed.
 */
int
pdf_write_bitmap_fonts_Encoding(gx_device_pdf *pdev)
{
    pdf_bitmap_fonts_t *pbfs = pdev->text->bitmap_fonts;

    if (pbfs->bitmap_encoding_id) {
	stream *s;
	int i;

	pdf_open_separate(pdev, pbfs->bitmap_encoding_id);
	s = pdev->strm;
	/*
	 * Even though the PDF reference documentation says that a
	 * BaseEncoding key is required unless the encoding is
	 * "based on the base font's encoding" (and there is no base
	 * font in this case), Acrobat 2.1 gives an error if the
	 * BaseEncoding key is present.
	 */
	stream_puts(s, "<</Type/Encoding/Differences[0");
	for (i = 0; i <= pbfs->max_embedded_code; ++i) {
	    if (!(i & 15))
		stream_puts(s, "\n");
	    pprintd1(s, "/a%d", i);
	}
	stream_puts(s, "\n] >>\n");
	pdf_end_separate(pdev);
	pbfs->bitmap_encoding_id = 0;
    }
    return 0;
}

/*
 * Start charproc accumulation for a Type 3 font.
 */
int
pdf_start_charproc_accum(gx_device_pdf *pdev)
{
    pdf_char_proc_t *pcp;
    pdf_resource_t *pres;
    int code = pdf_enter_substream(pdev, resourceCharProc, gs_next_ids(pdev->memory, 1), 
				   &pres, false, pdev->CompressFonts);

    if (code < 0)
       return code;
    pcp = (pdf_char_proc_t *)pres;
    pcp->char_next = NULL;
    pcp->font = NULL;
    pcp->char_code = GS_NO_CHAR;
    pcp->char_name.data = NULL;
    pcp->char_name.size = 0;
    return 0;
}

/*
 * Install charproc accumulator for a Type 3 font.
 */
int
pdf_set_charproc_attrs(gx_device_pdf *pdev, gs_font *font, const double *pw, int narg,
		gs_text_cache_control_t control, gs_char ch, gs_const_string *gnstr)
{
    pdf_font_resource_t *pdfont;
    pdf_resource_t *pres = pdev->accumulating_substream_resource;
    pdf_char_proc_t *pcp;
    int code;

    code = pdf_attached_font_resource(pdev, font, &pdfont, NULL, NULL, NULL, NULL);
    if (code < 0)
	return code;
    pcp = (pdf_char_proc_t *)pres;
    pcp->char_next = NULL;
    pcp->font = pdfont;
    pcp->char_code = ch;
    pcp->char_name = *gnstr;
    pcp->real_width.x = pw[font->WMode && narg > 6 ? 6 : 0];
    pcp->real_width.y = pw[font->WMode && narg > 6 ? 7 : 1];
    pcp->v.x = (narg > 8 ? pw[8] : 0);
    pcp->v.y = (narg > 8 ? pw[9] : 0);
    if (control == TEXT_SET_CHAR_WIDTH) {
	/* PLRM 5.7.1 "BuildGlyph" reads : "Normally, it is unnecessary and 
	undesirable to initialize the current color parameter, because show 
	is defined to paint glyphs with the current color."
	However comparefiles/Bug687044.ps doesn't follow that. */
	pdev->skip_colors = false; 
	pprintg2(pdev->strm, "%g %g d0\n", (float)pw[0], (float)pw[1]);
    } else {
	pdev->skip_colors = true;
	pprintg6(pdev->strm, "%g %g %g %g %g %g d1\n", 
	    (float)pw[0], (float)pw[1], (float)pw[2], 
	    (float)pw[3], (float)pw[4], (float)pw[5]);
	pdfont->u.simple.s.type3.cached[ch >> 3] |= 0x80 >> (ch & 7);
    }
    pdfont->used[ch >> 3] |= 0x80 >> (ch & 7);
    pdev->font3 = (pdf_resource_t *)pdfont;
    return 0;
}

/*
 * Open a stream object in the temporary file.
 */

int
pdf_open_aside(gx_device_pdf *pdev, pdf_resource_type_t rtype, 
	gs_id id, pdf_resource_t **ppres, bool reserve_object_id, int options) 
{
    int code;
    pdf_resource_t *pres;
    stream *s, *save_strm = pdev->strm;
    pdf_data_writer_t writer;
    static const pdf_filter_names_t fnames = {
	PDF_FILTER_NAMES
    };

    pdev->streams.save_strm = pdev->strm;
    code = pdf_alloc_aside(pdev, PDF_RESOURCE_CHAIN(pdev, rtype, id),
		pdf_resource_type_structs[rtype], &pres, reserve_object_id ? 0 : -1);
    if (code < 0)
	return code;
    cos_become(pres->object, cos_type_stream);
    s = cos_write_stream_alloc((cos_stream_t *)pres->object, pdev, "pdf_enter_substream");
    if (s == 0)
	return_error(gs_error_VMerror);
    pdev->strm = s;
    code = pdf_append_data_stream_filters(pdev, &writer,
			     options | DATA_STREAM_NOLENGTH, pres->object->id);
    if (code < 0) {
	pdev->strm = save_strm;
	return code;
    }
    code = pdf_put_filters((cos_dict_t *)pres->object, pdev, writer.binary.strm, &fnames);
    if (code < 0) {
	pdev->strm = save_strm;
	return code;
    }
    pdev->strm = writer.binary.strm;
    *ppres = pres;
    return 0;
}

/*
 * Close a stream object in the temporary file.
 */
int
pdf_close_aside(gx_device_pdf *pdev) 
{
    /* We should call pdf_end_data here, but we don't want to put pdf_data_writer_t
       into pdf_substream_save stack to simplify garbager descriptors. 
       Use a lower level functions instead that. */
    stream *s = pdev->strm;
    int status = s_close_filters(&s, cos_write_stream_from_pipeline(s));
    cos_stream_t *pcs = cos_stream_from_pipeline(s);
    int code = 0;

    if (status < 0)
	 code = gs_note_error(gs_error_ioerror);
    pcs->is_open = false;
    sclose(s);
    pdev->strm = pdev->streams.save_strm;
    return code;
}

/*
 * Enter the substream accumulation mode.
 */
int
pdf_enter_substream(gx_device_pdf *pdev, pdf_resource_type_t rtype, 
	gs_id id, pdf_resource_t **ppres, bool reserve_object_id, bool compress) 
{
    int sbstack_ptr = pdev->sbstack_depth;
    pdf_resource_t *pres;
    stream *save_strm = pdev->strm;
    int code;

    if (pdev->sbstack_depth >= pdev->sbstack_size)
	return_error(gs_error_unregistered); /* Must not happen. */
    if (pdev->sbstack[sbstack_ptr].text_state == 0) {
	pdev->sbstack[sbstack_ptr].text_state = pdf_text_state_alloc(pdev->pdf_memory);
	if (pdev->sbstack[sbstack_ptr].text_state == 0)
	    return_error(gs_error_VMerror);
    }
    code = pdf_open_aside(pdev, rtype, id, &pres, reserve_object_id, 
		    (compress ? DATA_STREAM_COMPRESS : 0));
    if (code < 0)
	return code;
    code = pdf_save_viewer_state(pdev, NULL);
    if (code < 0) {
	pdev->strm = save_strm;
	return code;
    }
    pdev->sbstack[sbstack_ptr].context = pdev->context;
    pdf_text_state_copy(pdev->sbstack[sbstack_ptr].text_state, pdev->text->text_state);
    pdf_set_text_state_default(pdev->text->text_state);
    pdev->sbstack[sbstack_ptr].clip_path = pdev->clip_path;
    pdev->clip_path = 0;
    pdev->sbstack[sbstack_ptr].clip_path_id = pdev->clip_path_id;
    pdev->clip_path_id = pdev->no_clip_path_id;
    pdev->sbstack[sbstack_ptr].vgstack_bottom = pdev->vgstack_bottom;
    pdev->vgstack_bottom = pdev->vgstack_depth;
    pdev->sbstack[sbstack_ptr].strm = save_strm;
    pdev->sbstack[sbstack_ptr].procsets = pdev->procsets;
    pdev->sbstack[sbstack_ptr].substream_Resources = pdev->substream_Resources;
    pdev->sbstack[sbstack_ptr].skip_colors = pdev->skip_colors;
    pdev->sbstack[sbstack_ptr].font3 = pdev->font3;
    pdev->sbstack[sbstack_ptr].accumulating_substream_resource = pdev->accumulating_substream_resource;
    pdev->sbstack[sbstack_ptr].charproc_just_accumulated = pdev->charproc_just_accumulated;
    pdev->sbstack[sbstack_ptr].accumulating_a_global_object = pdev->accumulating_a_global_object;
    pdev->sbstack[sbstack_ptr].pres_soft_mask_dict = pdev->pres_soft_mask_dict;
    pdev->sbstack[sbstack_ptr].objname = pdev->objname;
    pdev->skip_colors = false;
    pdev->charproc_just_accumulated = false;
    pdev->pres_soft_mask_dict = NULL;
    pdev->objname.data = NULL;
    pdev->objname.size = 0;
    /* Do not reset pdev->accumulating_a_global_object - it inherits. */
    pdev->sbstack_depth++;
    pdev->procsets = 0;
    pdev->font3 = 0;
    pdev->context = PDF_IN_STREAM;
    pdev->accumulating_substream_resource = pres;
    pdf_reset_graphics(pdev);
    *ppres = pres;
    return 0;
}

/*
 * Exit the substream accumulation mode.
 */
int
pdf_exit_substream(gx_device_pdf *pdev) 
{
    int code, code1;
    int sbstack_ptr;

    if (pdev->sbstack_depth <= 0)
	return_error(gs_error_unregistered); /* Must not happen. */
    code = pdf_open_contents(pdev, PDF_IN_STREAM);
    sbstack_ptr = pdev->sbstack_depth - 1;
    while (pdev->vgstack_depth > pdev->vgstack_bottom) {
	code1 = pdf_restore_viewer_state(pdev, pdev->strm);
	if (code >= 0)
	    code = code1;
    }
    if (pdev->clip_path != 0)
	gx_path_free(pdev->clip_path, "pdf_end_charproc_accum");
    code1 = pdf_close_aside(pdev);
    if (code1 < 0 && code >= 0)
	code = code1;
    pdev->context = pdev->sbstack[sbstack_ptr].context;
    pdf_text_state_copy(pdev->text->text_state, pdev->sbstack[sbstack_ptr].text_state);
    pdev->clip_path = pdev->sbstack[sbstack_ptr].clip_path;
    pdev->sbstack[sbstack_ptr].clip_path = 0;
    pdev->clip_path_id = pdev->sbstack[sbstack_ptr].clip_path_id;
    pdev->vgstack_bottom = pdev->sbstack[sbstack_ptr].vgstack_bottom;
    pdev->strm = pdev->sbstack[sbstack_ptr].strm;
    pdev->sbstack[sbstack_ptr].strm = 0;
    pdev->procsets = pdev->sbstack[sbstack_ptr].procsets;
    pdev->substream_Resources = pdev->sbstack[sbstack_ptr].substream_Resources;
    pdev->sbstack[sbstack_ptr].substream_Resources = 0;
    pdev->skip_colors = pdev->sbstack[sbstack_ptr].skip_colors;
    pdev->font3 = pdev->sbstack[sbstack_ptr].font3;
    pdev->sbstack[sbstack_ptr].font3 = 0;
    pdev->accumulating_substream_resource = pdev->sbstack[sbstack_ptr].accumulating_substream_resource;
    pdev->sbstack[sbstack_ptr].accumulating_substream_resource = 0;
    pdev->charproc_just_accumulated = pdev->sbstack[sbstack_ptr].charproc_just_accumulated;
    pdev->accumulating_a_global_object = pdev->sbstack[sbstack_ptr].accumulating_a_global_object;
    pdev->pres_soft_mask_dict = pdev->sbstack[sbstack_ptr].pres_soft_mask_dict;
    pdev->objname = pdev->sbstack[sbstack_ptr].objname;
    pdev->sbstack_depth = sbstack_ptr;
    code1 = pdf_restore_viewer_state(pdev, NULL);
    if (code1 < 0 && code >= 0)
	code = code1;
    return code;
}

private bool 
pdf_is_same_charproc1(gx_device_pdf * pdev, pdf_char_proc_t *pcp0, pdf_char_proc_t *pcp1)
{
    if (pcp0->char_code != pcp1->char_code)
	return false; /* We need same encoding. */
    if (pcp0->font->u.simple.Encoding[pcp0->char_code].glyph !=
	pcp1->font->u.simple.Encoding[pcp1->char_code].glyph)
	return false; /* We need same encoding. */
    if (bytes_compare(pcp0->char_name.data, pcp0->char_name.size, 
		      pcp1->char_name.data, pcp1->char_name.size))
	return false; /* We need same encoding. */
    if (pcp0->real_width.x != pcp1->real_width.x)
	return false;
    if (pcp0->real_width.y != pcp1->real_width.y)
	return false;
    if (pcp0->v.x != pcp1->v.x)
	return false;
    if (pcp0->v.y != pcp1->v.y)
	return false;
    if (pcp0->font->u.simple.s.type3.bitmap_font != pcp1->font->u.simple.s.type3.bitmap_font)
	return false;
    if (memcmp(&pcp0->font->u.simple.s.type3.FontMatrix, &pcp1->font->u.simple.s.type3.FontMatrix,
		sizeof(pcp0->font->u.simple.s.type3.FontMatrix)))
	return false;
    return pdf_check_encoding_compatibility(pcp1->font, pdev->cgp->s, pdev->cgp->num_all_chars);
}

private int 
pdf_is_same_charproc(gx_device_pdf * pdev, pdf_resource_t *pres0, pdf_resource_t *pres1)
{
    return pdf_is_same_charproc1(pdev, (pdf_char_proc_t *)pres0, (pdf_char_proc_t *)pres1);
}

private int 
pdf_find_same_charproc(gx_device_pdf *pdev, 
	    pdf_font_resource_t *pdfont, const pdf_char_glyph_pairs_t *cgp, 
	    pdf_char_proc_t **ppcp)
{
    pdf_char_proc_t *pcp;
    int code;

    pdev->cgp = cgp;
    for (pcp = pdfont->u.simple.s.type3.char_procs; pcp != NULL; pcp = pcp->char_next) {
	if (*ppcp != pcp && pdf_is_same_charproc1(pdev, *ppcp, pcp)) {
    	    cos_object_t *pco0 = pcp->object;
    	    cos_object_t *pco1 = (*ppcp)->object;

	    code = pco0->cos_procs->equal(pco0, pco1, pdev);
	    if (code < 0)
		return code;
	    if (code) {
		*ppcp = pcp;
		pdev->cgp = NULL;
		return 1;
	    }
	}
    }
    pcp = *ppcp;
    code = pdf_find_same_resource(pdev, resourceCharProc, (pdf_resource_t **)ppcp, pdf_is_same_charproc);
    pdev->cgp = NULL;
    if (code <= 0)
	return code;
    /* fixme : do we need more checks here ? */
    return 1;
}

private bool
pdf_is_charproc_defined(gx_device_pdf *pdev, pdf_font_resource_t *pdfont, gs_char ch)
{
    pdf_char_proc_t *pcp;

    for (pcp = pdfont->u.simple.s.type3.char_procs; pcp != NULL; pcp = pcp->char_next) {
	if (pcp->char_code == ch) {
	    return true;
	}
    }
    return false;
}

/*
 * Complete charproc accumulation for a Type 3 font.
 */
int
pdf_end_charproc_accum(gx_device_pdf *pdev, gs_font *font, const pdf_char_glyph_pairs_t *cgp) 
{
    int code;
    pdf_resource_t *pres = (pdf_resource_t *)pdev->accumulating_substream_resource;
    /* We could use pdfont->u.simple.s.type3.char_procs insted the thing above
       unless the font is defined recursively.
       But we don't want such assumption. */
    pdf_char_proc_t *pcp = (pdf_char_proc_t *)pres;
    pdf_font_resource_t *pdfont;
    gs_char ch = pcp->char_code;
    double *real_widths;
    byte *glyph_usage;
    int char_cache_size, width_cache_size;
    gs_glyph glyph0;
    bool checking_glyph_variation = false;
    int i;

    code = pdf_attached_font_resource(pdev, font, &pdfont, NULL, NULL, NULL, NULL);
    if (code < 0)
	return code;
    if (pdfont != (pdf_font_resource_t *)pdev->font3)
	return_error(gs_error_unregistered); /* Must not happen. */
    if (ch == GS_NO_CHAR)
	return_error(gs_error_unregistered); /* Must not happen. */
    if (ch >= 256)
	return_error(gs_error_unregistered); /* Must not happen. */
    code = pdf_exit_substream(pdev);
    if (code < 0)
	return code;
    if (pdfont->used[ch >> 3] & (0x80 >> (ch & 7))) {
	if (!(pdfont->u.simple.s.type3.cached[ch >> 3] & (0x80 >> (ch & 7)))) {
	    checking_glyph_variation = true;
	    code = pdf_find_same_charproc(pdev, pdfont, cgp, &pcp);
	    if (code < 0)
		return code;
	    if (code != 0) {
		code = pdf_cancel_resource(pdev, pres, resourceCharProc);
		if (code < 0)
		    return code;
		pdf_forget_resource(pdev, pres, resourceCharProc);
		if (pcp->font != pdfont) {
		    code = pdf_attach_font_resource(pdev, font, pcp->font);
		    if (code < 0)
			return code;
		}
		pdev->charproc_just_accumulated = true;
		return 0;
	    }
	    if (pdf_is_charproc_defined(pdev, pdfont, ch)) {
		gs_font *base_font = font, *below;

		while ((below = base_font->base) != base_font &&
			base_font->procs.same_font(base_font, below, FONT_SAME_OUTLINES))
		    base_font = below;
		code = pdf_make_font3_resource(pdev, base_font, &pdfont);
		if (code < 0)
		    return code;
		code = pdf_attach_font_resource(pdev, font, pdfont);
		if (code < 0)
		    return code;
	    }
	}
    } 
    pdf_reserve_object_id(pdev, pres, 0);
    code = pdf_attached_font_resource(pdev, font, &pdfont,
		&glyph_usage, &real_widths, &char_cache_size, &width_cache_size);
    if (code < 0)
	return code;
    if (ch >= char_cache_size || ch >= width_cache_size)
	return_error(gs_error_unregistered); /* Must not happen. */
    if (checking_glyph_variation)
	pdev->charproc_just_accumulated = true;
    glyph0 = font->procs.encode_char(font, ch, GLYPH_SPACE_NAME);
    pcp->char_next = pdfont->u.simple.s.type3.char_procs;
    pdfont->u.simple.s.type3.char_procs = pcp;
    pcp->font = pdfont;
    pdfont->Widths[ch] = pcp->real_width.x;
    real_widths[ch * 2    ] = pcp->real_width.x;
    real_widths[ch * 2 + 1] = pcp->real_width.y;
    glyph_usage[ch / 8] |= 0x80 >> (ch & 7);
    pdfont->used[ch >> 3] |= 0x80 >> (ch & 7);
    if (pdfont->u.simple.v != NULL && font->WMode) {
	pdfont->u.simple.v[ch].x = pcp->v.x;
	pdfont->u.simple.v[ch].y = pcp->v.x;
    }
    for (i = 0; i < 256; i++) {
	gs_glyph glyph = font->procs.encode_char(font, i, 
		    font->FontType == ft_user_defined ? GLYPH_SPACE_NOGEN
						      : GLYPH_SPACE_NAME);

	if (glyph == glyph0) {
	    real_widths[i * 2    ] = real_widths[ch * 2    ];
	    real_widths[i * 2 + 1] = real_widths[ch * 2 + 1];
	    glyph_usage[i / 8] |= 0x80 >> (i & 7);
	    pdfont->used[i >> 3] |= 0x80 >> (i & 7);
	    pdfont->u.simple.v[i] = pdfont->u.simple.v[ch];
	    pdfont->Widths[i] = pdfont->Widths[ch];
	}
    }
    return 0;
}

/* Add procsets to substream Resources. */
int
pdf_add_procsets(cos_dict_t *pcd, pdf_procset_t procsets)
{
    char str[5 + 7 + 7 + 7 + 5 + 2];
    cos_value_t v;

    strcpy(str, "[/PDF");
    if (procsets & ImageB)
	strcat(str, "/ImageB");
    if (procsets & ImageC)
	strcat(str, "/ImageC");
    if (procsets & ImageI)
	strcat(str, "/ImageI");
    if (procsets & Text)
	strcat(str, "/Text");
    strcat(str, "]");
    cos_string_value(&v, (byte *)str, strlen(str));
    return cos_dict_put_c_key(pcd, "/ProcSet", &v);
}

/* Add a resource to substream Resources. */
int
pdf_add_resource(gx_device_pdf *pdev, cos_dict_t *pcd, const char *key, pdf_resource_t *pres)
{
    if (pcd != 0) {
	const cos_value_t *v = cos_dict_find(pcd, (const byte *)key, strlen(key));
	cos_dict_t *list;
	int code;
	char buf[10 + (sizeof(long) * 8 / 3 + 1)], buf1[sizeof(pres->rname) + 1];

	if (pdev->ForOPDFRead && !pres->global && pdev->accumulating_a_global_object) {
	    pres->global = true;
	    code = cos_dict_put_c_key_bool((cos_dict_t *)pres->object, "/.Global", true);
	    if (code < 0)
		return code;
	}
	sprintf(buf, "%ld 0 R\n", pres->object->id);
	if (v != NULL) {
	    if (v->value_type != COS_VALUE_OBJECT && 
		v->value_type != COS_VALUE_RESOURCE)
		return_error(gs_error_unregistered); /* Must not happen. */
	    list = (cos_dict_t *)v->contents.object;	
	    if (list->cos_procs != &cos_dict_procs)
		return_error(gs_error_unregistered); /* Must not happen. */
	} else {
	    list = cos_dict_alloc(pdev, "pdf_add_resource");
	    if (list == NULL)
		return_error(gs_error_VMerror);
	    code = cos_dict_put_c_key_object((cos_dict_t *)pcd, key, (cos_object_t *)list);
	    if (code < 0)
		return code;
	}
	buf1[0] = '/';
	strcpy(buf1 + 1, pres->rname);
	return cos_dict_put_string(list, (const byte *)buf1, strlen(buf1),
			(const byte *)buf, strlen(buf));
    }
    return 0;
}

