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

/*$Id: gdevpdfu.c,v 1.12 2000/09/19 19:00:17 lpd Exp $ */
/* Output utilities for PDF-writing driver */
#include "memory_.h"
#include "jpeglib_.h"		/* for sdct.h */
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscdefs.h"
#include "gsdsrc.h"
#include "gsfunc.h"
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "scanchar.h"
#include "strimpl.h"
#include "sa85x.h"
#include "scfx.h"
#include "sdct.h"
#include "slzwx.h"
#include "spngpx.h"
#include "srlx.h"
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

/* Import procedures for writing filter parameters. */
extern stream_state_proc_get_params(s_DCTE_get_params, stream_DCT_state);
extern stream_state_proc_get_params(s_CF_get_params, stream_CF_state);

#define CHECK(expr)\
  BEGIN if ((code = (expr)) < 0) return code; END

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
    if (pdev->CompatibilityLevel >= 1.3) {
	/* Set the default rendering intent. */
	if (pdev->params.DefaultRenderingIntent != ri_Default) {
	    static const char *const ri_names[] = { psdf_ri_names };

	    pprints1(s, "/%s ri\n",
		     ri_names[(int)pdev->params.DefaultRenderingIntent]);
	}
    }
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
    pputs(pdev->strm, (pdev->text.use_leading ? "'\n" : "Tj\n"));
    pdev->text.use_leading = false;
    pdev->text.buffer_count = 0;
    return PDF_IN_TEXT;
}
/* Exit text context to stream context. */
private int
text_to_stream(gx_device_pdf * pdev)
{
    pputs(pdev->strm, "ET Q\n");
    pdf_reset_text(pdev);	/* because of Q */
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
    object = cos_object_alloc(pdev, "pdf_alloc_aside(object)");
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
	    cos_dict_alloc(pdev, "pdf_page_id");
	Page->id = pdf_obj_ref(pdev);
    }
    return Page->id;
}

/* Get the page structure for the current page. */
pdf_page_t *
pdf_current_page(gx_device_pdf *pdev)
{
    return &pdev->pages[pdev->next_page];
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

/* ------ Miscellaneous output ------ */

/* Generate the default Producer string. */
void
pdf_store_default_Producer(char buf[PDF_MAX_PRODUCER])
{
    sprintf(buf, ((gs_revision % 100) == 0 ? "(%s %1.1f)" : "(%s %1.2f)"),
	    gs_product, gs_revision / 100.0);
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

/* Store filters for a stream. */
/* Currently this only saves parameters for CCITTFaxDecode. */
int
pdf_put_filters(cos_dict_t *pcd, gx_device_pdf *pdev, stream *s,
		const pdf_filter_names_t *pfn)
{
    const char *filter_name = 0;
    bool binary_ok = true;
    stream *fs = s;
    cos_dict_t *decode_parms = 0;
    int code;

    for (; fs != 0; fs = fs->strm) {
	const stream_state *st = fs->state;
	const stream_template *template = st->template;

#define TEMPLATE_IS(atemp)\
  (template->process == (atemp).process)
	if (TEMPLATE_IS(s_A85E_template))
	    binary_ok = false;
	else if (TEMPLATE_IS(s_CFE_template)) {
	    cos_param_list_writer_t writer;
	    stream_CF_state cfs;

	    decode_parms =
		cos_dict_alloc(pdev, "pdf_put_image_filters(decode_parms)");
	    if (decode_parms == 0)
		return_error(gs_error_VMerror);
	    CHECK(cos_param_list_writer_init(&writer, decode_parms, 0));
	    /*
	     * If EndOfBlock is true, we mustn't write out a Rows value.
	     * This is a hack....
	     */
	    cfs = *(const stream_CF_state *)st;
	    if (cfs.EndOfBlock)
		cfs.Rows = 0;
	    CHECK(s_CF_get_params((gs_param_list *)&writer, &cfs, false));
	    filter_name = pfn->CCITTFaxDecode;
	} else if (TEMPLATE_IS(s_DCTE_template))
	    filter_name = pfn->DCTDecode;
	else if (TEMPLATE_IS(s_zlibE_template))
	    filter_name = pfn->FlateDecode;
	else if (TEMPLATE_IS(s_LZWE_template))
	    filter_name = pfn->LZWDecode;
	else if (TEMPLATE_IS(s_PNGPE_template)) {
	    /* This is a predictor for FlateDecode or LZWEncode. */
	    const stream_PNGP_state *const ss =
		(const stream_PNGP_state *)st;

	    decode_parms =
		cos_dict_alloc(pdev, "pdf_put_image_filters(decode_parms)");
	    if (decode_parms == 0)
		return_error(gs_error_VMerror);
	    CHECK(cos_dict_put_c_key_int(decode_parms, "/Predictor",
					 ss->Predictor));
	    CHECK(cos_dict_put_c_key_int(decode_parms, "/Columns",
					 ss->Columns));
	    if (ss->Colors != 1)
		CHECK(cos_dict_put_c_key_int(decode_parms, "/Colors",
					     ss->Colors));
	    if (ss->BitsPerComponent != 8)
		CHECK(cos_dict_put_c_key_int(decode_parms,
					     "/BitsPerComponent",
					     ss->BitsPerComponent));
	} else if (TEMPLATE_IS(s_RLE_template))
	    filter_name = pfn->RunLengthDecode;
#undef TEMPLATE_IS
    }
    if (filter_name) {
	if (binary_ok) {
	    CHECK(cos_dict_put_c_strings(pcd, pfn->Filter, filter_name));
	    if (decode_parms)
		CHECK(cos_dict_put_c_key_object(pcd, pfn->DecodeParms,
						COS_OBJECT(decode_parms)));
	} else {
	    cos_array_t *pca =
		cos_array_alloc(pdev, "pdf_put_image_filters(Filters)");

	    if (pca == 0)
		return_error(gs_error_VMerror);
	    CHECK(cos_array_add_c_string(pca, pfn->ASCII85Decode));
	    CHECK(cos_array_add_c_string(pca, filter_name));
	    CHECK(cos_dict_put_c_key_object(pcd, pfn->Filter,
					    COS_OBJECT(pca)));
	    if (decode_parms) {
		pca = cos_array_alloc(pdev,
				      "pdf_put_image_filters(DecodeParms)");
		if (pca == 0)
		    return_error(gs_error_VMerror);
		CHECK(cos_array_add_c_string(pca, "null"));
		CHECK(cos_array_add_object(pca, COS_OBJECT(decode_parms)));
		CHECK(cos_dict_put_c_key_object(pcd, pfn->DecodeParms,
						COS_OBJECT(pca)));
	    }
	}
    } else if (!binary_ok)
	CHECK(cos_dict_put_c_strings(pcd, pfn->Filter, pfn->ASCII85Decode));
    return 0;
}

/* Add a Flate compression filter to a binary writer. */
private int
pdf_flate_binary(gx_device_pdf *pdev, psdf_binary_writer *pbw)
{
    const stream_template *template = &s_zlibE_template;
    stream_state *st = s_alloc_state(pdev->pdf_memory, template->stype,
				     "pdf_write_function");

    if (st == 0)
	return_error(gs_error_VMerror);
    if (template->set_defaults)
	template->set_defaults(st);
    return psdf_encode_binary(pbw, template, st);
}

/*
 * Begin a Function or halftone data stream.  The client has opened the
 * object and written the << and any desired dictionary keys.
 */
int
pdf_begin_data(gx_device_pdf *pdev, pdf_data_writer_t *pdw)
{
    long length_id = pdf_obj_ref(pdev);
    stream *s = pdev->strm;
#define USE_ASCII85 1
#define USE_FLATE 2
    static const char *const fnames[4] = {
	"", "/Filter/ASCII85Decode", "/Filter/FlateDecode",
	"/Filter[/ASCII85Decode/FlateDecode]"
    };
    int filters = 0;
    int code;

    if (!pdev->binary_ok)
	filters |= USE_ASCII85;
    if (pdev->CompatibilityLevel >= 1.2)
	filters |= USE_FLATE;
    pputs(s, fnames[filters]);
    pprintld1(s, "/Length %ld 0 R>>stream\n", length_id);
    code = psdf_begin_binary((gx_device_psdf *)pdev, &pdw->binary);
    if (code < 0)
	return code;
    pdw->start = stell(s);
    pdw->length_id = length_id;
    if (filters & USE_FLATE)
	code = pdf_flate_binary(pdev, &pdw->binary);
    return code;
#undef USE_ASCII85
#undef USE_FLATE
}

/* End a data stream. */
int
pdf_end_data(pdf_data_writer_t *pdw)
{
    gx_device_pdf *pdev = (gx_device_pdf *)pdw->binary.dev;
    int code = psdf_end_binary(&pdw->binary);
    long length = stell(pdev->strm) - pdw->start;

    if (code < 0)
	return code;
    pputs(pdev->strm, "\nendstream\n");
    pdf_end_separate(pdev);
    pdf_open_separate(pdev, pdw->length_id);
    pprintld1(pdev->strm, "%ld\n", length);
    return pdf_end_separate(pdev);
}

/* Create a Function object. */
int
pdf_function(gx_device_pdf *pdev, const gs_function_t *pfn,
	     cos_value_t *pvalue)
{
    gs_function_info_t info;
    cos_param_list_writer_t rlist;
    pdf_resource_t *pres;
    cos_object_t *pcfn;
    cos_dict_t *pcd;
    cos_value_t v;
    int code = pdf_alloc_resource(pdev, resourceFunction, gs_no_id, &pres, 0L);

    if (code < 0)
	return code;
    pcfn = pres->object;
    gs_function_get_info(pfn, &info);
    if (info.DataSource != 0) {
	psdf_binary_writer writer;
	stream *save = pdev->strm;
	cos_stream_t *pcos;
	stream *s;

	cos_become(pcfn, cos_type_stream);
	pcos = (cos_stream_t *)pcfn;
	pcd = cos_stream_dict(pcos);
	s = cos_write_stream_alloc(pcos, pdev, "pdf_function");
	if (s == 0)
	    return_error(gs_error_VMerror);
	pdev->strm = s;
	code = psdf_begin_binary((gx_device_psdf *)pdev, &writer);
	if (code >= 0 && info.data_size > 30 &&	/* 30 is arbitrary */
	    pdev->CompatibilityLevel >= 1.2
	    )
	    code = pdf_flate_binary(pdev, &writer);
	if (code >= 0) {
	    static const pdf_filter_names_t fnames = {
		PDF_FILTER_NAMES
	    };

	    code = pdf_put_filters(pcd, pdev, writer.strm, &fnames);
	}
	if (code >= 0) {
	    byte buf[100];		/* arbitrary */
	    ulong pos;
	    uint count;
	    const byte *ptr;

	    for (pos = 0; pos < info.data_size; pos += count) {
		count = min(sizeof(buf), info.data_size - pos);
		data_source_access_only(info.DataSource, pos, count, buf,
					&ptr);
		pwrite(writer.strm, ptr, count);
	    }
	    code = psdf_end_binary(&writer);
	    sclose(s);
	}
	pdev->strm = save;
	if (code < 0)
	    return code;
    } else {
	cos_become(pcfn, cos_type_dict);
	pcd = (cos_dict_t *)pcfn;
    }
    if (info.Functions != 0) {
	int i;
	cos_array_t *functions =
	    cos_array_alloc(pdev, "pdf_function(Functions)");

	if (functions == 0)
	    return_error(gs_error_VMerror);
	for (i = 0; i < info.num_Functions; ++i) {
	    if ((code = pdf_function(pdev, info.Functions[i], &v)) < 0 ||
		(code = cos_array_add(functions, &v)) < 0
		) {
		COS_FREE(functions, "pdf_function(Functions)");
		return code;
	    }
	}
	code = cos_dict_put_c_key(pcd, "/Functions",
				  COS_OBJECT_VALUE(&v, functions));
	if (code < 0) {
	    COS_FREE(functions, "pdf_function(Functions)");
	    return code;
	}
    }
    code = cos_param_list_writer_init(&rlist, pcd, PRINT_BINARY_OK);
    if (code < 0)
	return code;
    code = gs_function_get_params(pfn, (gs_param_list *)&rlist);
    if (code < 0)
	return code;
    COS_OBJECT_VALUE(pvalue, pcd);
    return 0;
}

/* Write a Function object. */
int
pdf_write_function(gx_device_pdf *pdev, const gs_function_t *pfn, long *pid)
{
    cos_value_t value;
    int code = pdf_function(pdev, pfn, &value);

    if (code < 0)
	return code;
    *pid = value.contents.object->id;
    return 0;
}
