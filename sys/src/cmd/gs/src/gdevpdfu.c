/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfu.c,v 1.2 2000/03/16 01:21:24 lpd Exp $ */
/* Output utilities for PDF-writing driver */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "time_.h"
#include "gx.h"
#include "gp.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gxistate.h"
#include "gxpaint.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "scanchar.h"
#include "strimpl.h"		/* for short-sighted compilers */
#include "scfx.h"		/* s_CFE_template is default */
#include "sstring.h"
#include "szlibx.h"

/* Define the size of internal stream buffers. */
/* (This is not a limitation, it only affects performance.) */
#define sbuf_size 512

/* Optionally substitute other filters for FlateEncode for debugging. */
#if 1
#  define compression_filter_name "FlateDecode"
#  define compression_filter_template s_zlibE_template
#  define compression_filter_state stream_zlib_state
#else
#  include "slzwx.h"
#  define compression_filter_name "LZWDecode"
#  define compression_filter_template s_LZWE_template
#  define compression_filter_state stream_LZW_state
#endif

/* GC descriptors */
extern_st(st_pdf_font);
extern_st(st_pdf_char_proc);
extern_st(st_pdf_font_descriptor);
public_st_pdf_resource();
private_st_pdf_x_object();

/* ---------------- Utilities ---------------- */

/* ------ Document ------ */

/* Open the document if necessary. */
void
pdf_open_document(gx_device_pdf * pdev)
{
    if (!is_in_page(pdev) && pdf_stell(pdev) == 0) {
	stream *s = pdev->strm;
	int level = (int)(pdev->CompatibilityLevel * 10 + 0.5);

	pprintd2(s, "%%PDF-%d.%d\n", level / 10, level % 10);
	pdev->binary_ok = !pdev->params.ASCII85EncodePages;
	if (pdev->binary_ok)
	    pputs(s, "%\307\354\217\242\n");
    }
    /*
     * Determine the compression method.  Currently this does nothing.
     * It also isn't clear whether the compression method can now be
     * changed in the course of the document.
     *
     * The following algorithm is per an update to TN # 5151 by
     * Adobe Developer Support.
     */
    if (!pdev->params.CompressPages)
	pdev->compression = pdf_compress_none;
    else if (pdev->CompatibilityLevel < 1.2)
	pdev->compression = pdf_compress_LZW;
    else if (pdev->params.UseFlateCompression)
	pdev->compression = pdf_compress_Flate;
    else
	pdev->compression = pdf_compress_LZW;
}

/* ------ Objects ------ */

/* Allocate an object ID. */
private long
pdf_next_id(gx_device_pdf * pdev)
{
    return (pdev->next_id)++;
}

/*
 * Return the current position in the output.  Note that this may be in the
 * main output file, the asides file, or the pictures file.  If the current
 * file is the pictures file, positions returned by pdf_stell must only be
 * used locally (for computing lengths or patching), since there is no way
 * to map them later to the eventual position in the output file.
 */
long
pdf_stell(gx_device_pdf * pdev)
{
    stream *s = pdev->strm;
    long pos = stell(s);

    if (s == pdev->asides.strm)
	pos += ASIDES_BASE_POSITION;
    return pos;
}

/* Allocate an ID for a future object. */
long
pdf_obj_ref(gx_device_pdf * pdev)
{
    long id = pdf_next_id(pdev);
    long pos = pdf_stell(pdev);

    fwrite(&pos, sizeof(pos), 1, pdev->xref.file);
    return id;
}

/* Begin an object, optionally allocating an ID. */
long
pdf_open_obj(gx_device_pdf * pdev, long id)
{
    stream *s = pdev->strm;

    if (id <= 0) {
	id = pdf_obj_ref(pdev);
    } else {
	long pos = pdf_stell(pdev);
	FILE *tfile = pdev->xref.file;
	long tpos = ftell(tfile);

	fseek(tfile, (id - pdev->FirstObjectNumber) * sizeof(pos),
	      SEEK_SET);
	fwrite(&pos, sizeof(pos), 1, tfile);
	fseek(tfile, tpos, SEEK_SET);
    }
    pprintld1(s, "%ld 0 obj\n", id);
    return id;
}
long
pdf_begin_obj(gx_device_pdf * pdev)
{
    return pdf_open_obj(pdev, 0L);
}

/* End an object. */
int
pdf_end_obj(gx_device_pdf * pdev)
{
    pputs(pdev->strm, "endobj\n");
    return 0;
}

/* ------ Graphics ------ */

/* Reset the graphics state parameters to initial values. */
void
pdf_reset_graphics(gx_device_pdf * pdev)
{
    color_set_pure(&pdev->fill_color, 0);	/* black */
    color_set_pure(&pdev->stroke_color, 0);	/* ditto */
    pdev->state.flatness = -1;
    {
	static const gx_line_params lp_initial = {
	    gx_line_params_initial
	};

	pdev->state.line_params = lp_initial;
    }
    pdev->text.character_spacing = 0;
    pdev->text.font = NULL;
    pdev->text.size = 0;
    pdev->text.word_spacing = 0;
}

/* Set the fill or stroke color. */
int
pdf_set_color(gx_device_pdf * pdev, gx_color_index color,
	      gx_drawing_color * pdcolor,
	      const psdf_set_color_commands_t *ppscc)
{
    if (gx_dc_pure_color(pdcolor) != color) {
	int code;

	/*
	 * In principle, we can set colors in either stream or text
	 * context.  However, since we currently enclose all text
	 * strings inside a gsave/grestore, this causes us to lose
	 * track of the color when we leave text context.  Therefore,
	 * we require stream context for setting colors.
	 */
#if 0
	switch (pdev->context) {
	    case PDF_IN_STREAM:
	    case PDF_IN_TEXT:
		break;
	    case PDF_IN_NONE:
		code = pdf_open_page(pdev, PDF_IN_STREAM);
		goto open;
	    case PDF_IN_STRING:
		code = pdf_open_page(pdev, PDF_IN_TEXT);
	      open:if (code < 0)
		    return code;
	}
#else
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
#endif
	color_set_pure(pdcolor, color);
	psdf_set_color((gx_device_vector *) pdev, pdcolor, ppscc);
    }
    return 0;
}

/* Write matrix values. */
void
pdf_put_matrix(gx_device_pdf * pdev, const char *before,
	       const gs_matrix * pmat, const char *after)
{
    stream *s = pdev->strm;

    if (before)
	pputs(s, before);
    pprintg6(s, "%g %g %g %g %g %g ",
	     pmat->xx, pmat->xy, pmat->yx, pmat->yy, pmat->tx, pmat->ty);
    if (after)
	pputs(s, after);
}

/*
 * Write a name, with escapes for unusual characters.  In PDF 1.1, we have
 * no choice but to replace these characters with '?'; in PDF 1.2, we can
 * use an escape sequence for anything except a null <00>.
 */
void
pdf_put_name_escaped(stream *s, const byte *nstr, uint size, bool escape)
{
    uint i;

    pputc(s, '/');
    for (i = 0; i < size; ++i) {
	uint c = nstr[i];
	char hex[4];

	switch (c) {
	    case '#':
		/* These are valid in 1.1, but must be escaped in 1.2. */
		if (escape) {
		    sprintf(hex, "#%02x", c);
		    pputs(s, hex);
		    break;
		}
		/* falls through */
	    default:
		if (c >= 0x21 && c <= 0x7e) {
		    /* These are always valid. */
		    pputc(s, c);
		    break;
		}
		/* falls through */
	    case '%':
	    case '(': case ')':
	    case '<': case '>':
	    case '[': case ']':
	    case '{': case '}':
	    case '/':
		/* These characters are invalid in both 1.1 and 1.2, */
		/* but can be escaped in 1.2. */
		if (escape) {
		    sprintf(hex, "#%02x", c);
		    pputs(s, hex);
		    break;
		}
		/* falls through */
	    case 0:
		/* This is invalid in 1.1 and 1.2, and cannot be escaped. */
		pputc(s, '?');
	}
    }
}
void
pdf_put_name(const gx_device_pdf *pdev, const byte *nstr, uint size)
{
    pdf_put_name_escaped(pdev->strm, nstr, size,
			 pdev->CompatibilityLevel >= 1.2);
}

/*
 * Write a string in its shortest form ( () or <> ).  Note that
 * this form is different depending on whether binary data are allowed.
 * We wish PDF supported ASCII85 strings ( <~ ~> ), but it doesn't.
 */
void
pdf_put_string(const gx_device_pdf * pdev, const byte * str, uint size)
{
    psdf_write_string(pdev->strm, str, size,
		      (pdev->binary_ok ? PRINT_BINARY_OK : 0));
}

/* Write a value, treating names specially. */
void
pdf_write_value(const gx_device_pdf * pdev, const byte * vstr, uint size)
{
    if (size > 0 && vstr[0] == '/')
	pdf_put_name(pdev, vstr + 1, size - 1);
    else
	pwrite(pdev->strm, vstr, size);
}

/* ------ Page contents ------ */

/* Handle transitions between contexts. */
private int
    none_to_stream(P1(gx_device_pdf *)), stream_to_text(P1(gx_device_pdf *)),
    string_to_text(P1(gx_device_pdf *)), text_to_stream(P1(gx_device_pdf *)),
    stream_to_none(P1(gx_device_pdf *));
typedef int (*context_proc) (P1(gx_device_pdf *));
private const context_proc context_procs[4][4] =
{
    {0, none_to_stream, none_to_stream, none_to_stream},
    {stream_to_none, 0, stream_to_text, stream_to_text},
    {text_to_stream, text_to_stream, 0, 0},
    {string_to_text, string_to_text, string_to_text, 0}
};

/* Enter stream context. */
private int
none_to_stream(gx_device_pdf * pdev)
{
    stream *s;

    if (pdev->contents_id != 0)
	return_error(gs_error_Fatal);	/* only 1 contents per page */
    pdev->contents_id = pdf_begin_obj(pdev);
    pdev->contents_length_id = pdf_obj_ref(pdev);
    s = pdev->strm;
    pprintld1(s, "<</Length %ld 0 R", pdev->contents_length_id);
    if (pdev->compression == pdf_compress_Flate)
	pprints1(s, "/Filter /%s", compression_filter_name);
    pputs(s, ">>\nstream\n");
    pdev->contents_pos = pdf_stell(pdev);
    if (pdev->compression == pdf_compress_Flate) {	/* Set up the Flate filter. */
	const stream_template *template = &compression_filter_template;
	stream *es = s_alloc(pdev->pdf_memory, "PDF compression stream");
	byte *buf = gs_alloc_bytes(pdev->pdf_memory, sbuf_size,
				   "PDF compression buffer");
	compression_filter_state *st =
	    gs_alloc_struct(pdev->pdf_memory, compression_filter_state,
			    template->stype, "PDF compression state");

	if (es == 0 || st == 0 || buf == 0)
	    return_error(gs_error_VMerror);
	s_std_init(es, buf, sbuf_size, &s_filter_write_procs,
		   s_mode_write);
	st->memory = pdev->pdf_memory;
	st->template = template;
	es->state = (stream_state *) st;
	es->procs.process = template->process;
	es->strm = s;
	(*template->set_defaults) ((stream_state *) st);
	(*template->init) ((stream_state *) st);
	pdev->strm = s = es;
    }
    /* Scale the coordinate system. */
    pprintg2(s, "%g 0 0 %g 0 0 cm\n",
	     72.0 / pdev->HWResolution[0], 72.0 / pdev->HWResolution[1]);
    /* Do a level of gsave for the clipping path. */
    pputs(s, "q\n");
    return PDF_IN_STREAM;
}
/* Enter text context from stream context. */
private int
stream_to_text(gx_device_pdf * pdev)
{
    /*
     * Bizarrely enough, Acrobat Reader cares how the final font size is
     * obtained -- the CTM (cm), text matrix (Tm), and font size (Tf)
     * are *not* all equivalent.  In particular, it seems to use the
     * product of the text matrix and font size to decide how to
     * anti-alias characters.  Therefore, we have to temporarily patch
     * the CTM so that the scale factors are unity.  What a nuisance!
     */
    pprintg2(pdev->strm, "q %g 0 0 %g 0 0 cm BT\n",
	     pdev->HWResolution[0] / 72.0, pdev->HWResolution[1] / 72.0);
    pdev->procsets |= Text;
    gs_make_identity(&pdev->text.matrix);
    pdev->text.line_start.x = pdev->text.line_start.y = 0;
    pdev->text.buffer_count = 0;
    return PDF_IN_TEXT;
}
/* Exit string context to text context. */
private int
string_to_text(gx_device_pdf * pdev)
{
    pdf_put_string(pdev, pdev->text.buffer, pdev->text.buffer_count);
    pputs(pdev->strm, "Tj\n");
    pdev->text.buffer_count = 0;
    return PDF_IN_TEXT;
}
/* Exit text context to stream context. */
private int
text_to_stream(gx_device_pdf * pdev)
{
    pputs(pdev->strm, "ET Q\n");
    pdev->text.font = 0;	/* because of Q */
    return PDF_IN_STREAM;
}
/* Exit stream context. */
private int
stream_to_none(gx_device_pdf * pdev)
{
    stream *s = pdev->strm;
    long length;

    if (pdev->compression == pdf_compress_Flate) {	/* Terminate the Flate filter. */
	stream *fs = s->strm;

	sclose(s);
	gs_free_object(pdev->pdf_memory, s->cbuf, "zlib buffer");
	gs_free_object(pdev->pdf_memory, s, "zlib stream");
	pdev->strm = s = fs;
    }
    length = pdf_stell(pdev) - pdev->contents_pos;
    pputs(s, "endstream\n");
    pdf_end_obj(pdev);
    pdf_open_obj(pdev, pdev->contents_length_id);
    pprintld1(s, "%ld\n", length);
    pdf_end_obj(pdev);
    return PDF_IN_NONE;
}

/* Begin a page contents part. */
int
pdf_open_contents(gx_device_pdf * pdev, pdf_context_t context)
{
    int (*proc) (P1(gx_device_pdf *));

    while ((proc = context_procs[pdev->context][context]) != 0) {
	int code = (*proc) (pdev);

	if (code < 0)
	    return code;
	pdev->context = (pdf_context_t) code;
    }
    pdev->context = context;
    return 0;
}

/* Close the current contents part if we are in one. */
int
pdf_close_contents(gx_device_pdf * pdev, bool last)
{
    if (pdev->context == PDF_IN_NONE)
	return 0;
    if (last) {			/* Exit from the clipping path gsave. */
	pdf_open_contents(pdev, PDF_IN_STREAM);
	pputs(pdev->strm, "Q\n");
	pdev->text.font = 0;
    }
    return pdf_open_contents(pdev, PDF_IN_NONE);
}

/* ------ Resources et al ------ */

/* Define the allocator descriptors for the resource types. */
private const char *const resource_names[] = {
    pdf_resource_type_names
};
private const gs_memory_struct_type_t *const resource_structs[] = {
    pdf_resource_type_structs
};

/* Find a resource of a given type by gs_id. */
pdf_resource_t *
pdf_find_resource_by_gs_id(gx_device_pdf * pdev, pdf_resource_type_t rtype,
			   gs_id rid)
{
    pdf_resource_t **pchain = PDF_RESOURCE_CHAIN(pdev, rtype, rid);
    pdf_resource_t **pprev = pchain;
    pdf_resource_t *pres;

    for (; (pres = *pprev) != 0; pprev = &pres->next)
	if (pres->rid == rid) {
	    if (pprev != pchain) {
		*pprev = pres->next;
		pres->next = *pchain;
		*pchain = pres;
	    }
	    return pres;
	}
    return 0;
}

/* Begin an object logically separate from the contents. */
long
pdf_open_separate(gx_device_pdf * pdev, long id)
{
    pdf_open_document(pdev);
    pdev->asides.save_strm = pdev->strm;
    pdev->strm = pdev->asides.strm;
    return pdf_open_obj(pdev, id);
}
long
pdf_begin_separate(gx_device_pdf * pdev)
{
    return pdf_open_separate(pdev, 0L);
}

/* Begin an aside (resource, annotation, ...). */
private int
pdf_alloc_aside(gx_device_pdf * pdev, pdf_resource_t ** plist,
		const gs_memory_struct_type_t * pst, pdf_resource_t **ppres,
		long id)
{
    pdf_resource_t *pres;
    cos_object_t *object;

    if (pst == NULL)
	pst = &st_pdf_resource;
    pres = gs_alloc_struct(pdev->pdf_memory, pdf_resource_t, pst,
			   "pdf_alloc_aside(resource)");
    object = cos_object_alloc(pdev->pdf_memory, "pdf_alloc_aside(object)");
    if (pres == 0 || object == 0) {
	return_error(gs_error_VMerror);
    }
    object->id = (id < 0 ? -1L : id == 0 ? pdf_obj_ref(pdev) : id);
    pres->next = *plist;
    *plist = pres;
    pres->prev = pdev->last_resource;
    pdev->last_resource = pres;
    pres->named = false;
    pres->used_on_page = true;
    pres->object = object;
    *ppres = pres;
    return 0;
}
int
pdf_begin_aside(gx_device_pdf * pdev, pdf_resource_t ** plist,
		const gs_memory_struct_type_t * pst, pdf_resource_t ** ppres)
{
    long id = pdf_begin_separate(pdev);

    if (id < 0)
	return (int)id;
    return pdf_alloc_aside(pdev, plist, pst, ppres, id);
}

/* Begin a resource of a given type. */
int
pdf_begin_resource_body(gx_device_pdf * pdev, pdf_resource_type_t rtype,
			gs_id rid, pdf_resource_t ** ppres)
{
    int code = pdf_begin_aside(pdev, PDF_RESOURCE_CHAIN(pdev, rtype, rid),
			       resource_structs[rtype], ppres);

    if (code >= 0)
	(*ppres)->rid = rid;
    return code;
}
int
pdf_begin_resource(gx_device_pdf * pdev, pdf_resource_type_t rtype, gs_id rid,
		   pdf_resource_t ** ppres)
{
    int code = pdf_begin_resource_body(pdev, rtype, rid, ppres);

    if (code >= 0 && resource_names[rtype] != 0) {
	stream *s = pdev->strm;

	pprints1(s, "<</Type/%s", resource_names[rtype]);
	pprintld1(s, "/Name/R%ld", (*ppres)->object->id);
    }
    return code;
}

/* Allocate a resource, but don't open the stream. */
int
pdf_alloc_resource(gx_device_pdf * pdev, pdf_resource_type_t rtype, gs_id rid,
		   pdf_resource_t ** ppres, long id)
{
    int code = pdf_alloc_aside(pdev, PDF_RESOURCE_CHAIN(pdev, rtype, rid),
			       resource_structs[rtype], ppres, id);

    if (code >= 0)
	(*ppres)->rid = rid;
    return code;
}

/* Get the object id of a resource. */
long
pdf_resource_id(const pdf_resource_t *pres)
{
    return pres->object->id;
}

/* End an aside or other separate object. */
int
pdf_end_separate(gx_device_pdf * pdev)
{
    int code = pdf_end_obj(pdev);

    pdev->strm = pdev->asides.save_strm;
    pdev->asides.save_strm = 0;
    return code;
}
int
pdf_end_aside(gx_device_pdf * pdev)
{
    return pdf_end_separate(pdev);
}

/* End a resource. */
int
pdf_end_resource(gx_device_pdf * pdev)
{
    return pdf_end_aside(pdev);
}

/* Copy data from a temporary file to a stream. */
void
pdf_copy_data(stream *s, FILE *file, long count)
{
    long left = count;
    byte buf[sbuf_size];

    while (left > 0) {
	uint copy = min(left, sbuf_size);

	fread(buf, 1, sbuf_size, file);
	pwrite(s, buf, copy);
	left -= copy;
    }
}

/* ------ Pages ------ */

/* Get or assign the ID for a page. */
/* Returns 0 if the page number is out of range. */
long
pdf_page_id(gx_device_pdf * pdev, int page_num)
{
    cos_dict_t *Page;

    if (page_num < 1)
	return 0;
    if (page_num >= pdev->num_pages) {	/* Grow the pages array. */
	uint new_num_pages =
	    max(page_num + 10, pdev->num_pages << 1);
	pdf_page_t *new_pages =
	    gs_resize_object(pdev->pdf_memory, pdev->pages, new_num_pages,
			     "pdf_page_id(resize pages)");

	if (new_pages == 0)
	    return 0;
	memset(&new_pages[pdev->num_pages], 0,
	       (new_num_pages - pdev->num_pages) * sizeof(pdf_page_t));
	pdev->pages = new_pages;
	pdev->num_pages = new_num_pages;
    }
    if ((Page = pdev->pages[page_num - 1].Page) == 0) {
	pdev->pages[page_num - 1].Page = Page =
	    cos_dict_alloc(pdev->pdf_memory, "pdf_page_id");
	Page->id = pdf_obj_ref(pdev);
    }
    return Page->id;
}

/* Get the dictionary object for the current page. */
cos_dict_t *
pdf_current_page_dict(gx_device_pdf *pdev)
{
    if (pdf_page_id(pdev, pdev->next_page + 1) <= 0)
	return 0;
    return pdev->pages[pdev->next_page].Page;
}

/* Write saved page- or document-level information. */
int
pdf_write_saved_string(gx_device_pdf * pdev, gs_string * pstr)
{
    if (pstr->data != 0) {
	pwrite(pdev->strm, pstr->data, pstr->size);
	gs_free_string(pdev->pdf_memory, pstr->data, pstr->size,
		       "pdf_write_saved_string");
	pstr->data = 0;
    }
    return 0;
}

/* Open a page for writing. */
int
pdf_open_page(gx_device_pdf * pdev, pdf_context_t context)
{
    if (!is_in_page(pdev)) {
	if (pdf_page_id(pdev, pdev->next_page + 1) == 0)
	    return_error(gs_error_VMerror);
	pdf_open_document(pdev);
    }
    /* Note that context may be PDF_IN_NONE here. */
    return pdf_open_contents(pdev, context);
}
