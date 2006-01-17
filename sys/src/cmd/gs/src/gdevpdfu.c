/* Copyright (C) 1999, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfu.c,v 1.89 2005/10/18 09:05:58 leonardo Exp $ */
/* Output utilities for PDF-writing driver */
#include "memory_.h"
#include "jpeglib_.h"		/* for sdct.h */
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscdefs.h"
#include "gsdsrc.h"
#include "gsfunc.h"
#include "gsfunc3.h"
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "gdevpdfg.h"
#include "gdevpdtd.h"
#include "scanchar.h"
#include "strimpl.h"
#include "sa85x.h"
#include "scfx.h"
#include "sdct.h"
#include "slzwx.h"
#include "spngpx.h"
#include "srlx.h"
#include "sarc4.h"
#include "smd5.h"
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
extern_st(st_pdf_color_space);
extern_st(st_pdf_font_resource);
extern_st(st_pdf_char_proc);
extern_st(st_pdf_font_descriptor);
public_st_pdf_resource();
private_st_pdf_x_object();
private_st_pdf_pattern();

/* ---------------- Utilities ---------------- */

/*
 * Strip whitespace and comments from a line of PostScript code as possible.
 * Return a pointer to any string that remains, or NULL if none.
 * Note that this may store into the string.
 */
/* This function copied from geninit.c . */
private char *
doit(char *line, bool intact)
{
    char *str = line;
    char *from;
    char *to;
    int in_string = 0;

    if (intact)
	return str;
    while (*str == ' ' || *str == '\t')		/* strip leading whitespace */
	++str;
    if (*str == 0)		/* all whitespace */
	return NULL;
    if (!strncmp(str, "%END", 4))	/* keep these for .skipeof */
	return str;
    if (str[0] == '%')    /* comment line */
	return NULL;
    /*
     * Copy the string over itself removing:
     *  - All comments not within string literals;
     *  - Whitespace adjacent to '[' ']' '{' '}';
     *  - Whitespace before '/' '(' '<';
     *  - Whitespace after ')' '>'.
     */
    for (to = from = str; (*to = *from) != 0; ++from, ++to) {
	switch (*from) {
	    case '%':
		if (!in_string)
		    break;
		continue;
	    case ' ':
	    case '\t':
		if (to > str && !in_string && strchr(" \t>[]{})", to[-1]))
		    --to;
		continue;
	    case '(':
	    case '<':
	    case '/':
	    case '[':
	    case ']':
	    case '{':
	    case '}':
		if (to > str && !in_string && strchr(" \t", to[-1]))
		    *--to = *from;
                if (*from == '(')
                    ++in_string;
              	continue;
	    case ')':
		--in_string;
		continue;
	    case '\\':
		if (from[1] == '\\' || from[1] == '(' || from[1] == ')')
		    *++to = *++from;
		continue;
	    default:
		continue;
	}
	break;
    }
    /* Strip trailing whitespace. */
    while (to > str && (to[-1] == ' ' || to[-1] == '\t'))
	--to;
    *to = 0;
    return str;
}


private int
copy_ps_file_stripping(stream *s, const char *fname, bool HaveTrueTypes)
{
    FILE *f;
    char buf[1024], *p, *q  = buf;
    int n, l = 0, m = sizeof(buf) - 1, outl = 0;
    bool skipping = false;

    f = gp_fopen(fname, "rb");
    if (f == NULL)
	return_error(gs_error_undefinedfilename);
    n = fread(buf, 1, m, f);
    buf[n] = 0;
    do {
	if (*q == '\r' || *q == '\n') {
	    q++;
	    continue;
	}
	p = strchr(q, '\r');
	if (p == NULL)
	    p = strchr(q, '\n');
	if (p == NULL) {
	    if (n < m)
		p = buf + n;
	    else {
		strcpy(buf, q);
		l = strlen(buf);
		m = sizeof(buf) - 1 - l;
		if (!m) {
		    eprintf1("The procset %s contains a too long line.", fname);
		    return_error(gs_error_ioerror);
		}
		n = fread(buf + l, 1, m, f);
		n += l;
		m += l;
		buf[n] = 0;
		q = buf;
		continue;
	    }
	}
	*p = 0;
	if (q[0] == '%')
	    l = 0;
	else {
	    q = doit(q, false);
	    if (q == NULL)
		l = 0;
	    else
		l = strlen(q);
	}
	if (l) {
	    if (!HaveTrueTypes && !strcmp("%%beg TrueType", q))
		skipping = true;
	    if (!skipping) {
		outl += l + 1;
		if (outl > 100) {
		    q[l] = '\r';
		    outl = 0;
		} else
		    q[l] = ' ';
		stream_write(s, q, l + 1);
	    }
	    if (!HaveTrueTypes && !strcmp("%%end TrueType", q))
		skipping = false;
	}
	q = p + 1;
    } while (n == m || q < buf + n);
    if (outl)
	stream_write(s, "\r", 1);
    fclose(f);
    return 0;
}

private int
copy_procsets(stream *s, const gs_param_string *path, bool HaveTrueTypes)
{
    char fname[gp_file_name_sizeof];
    const byte *p = path->data, *e = path->data + path->size;
    int l, i = 0, code;
    const char *tt_encs[] = {"gs_agl.ps", "gs_mgl_e.ps"};

    if (p != NULL) {
	for (;; i++) {
	    const byte *c = memchr(p, gp_file_name_list_separator, e - p);
	    int k;

	    if (c == NULL)
		c = e;
	    l = c - p;
	    if (l > 0) {
		if (l > sizeof(fname) - 1)
		    return_error(gs_error_limitcheck);
		memcpy(fname, p, l);
		fname[l] = 0;
		if (!HaveTrueTypes) {
		    for (k = count_of(tt_encs) - 1; k >= 0; k--) {
			int L = strlen(tt_encs[k]);

			if (!strcmp(fname + strlen(fname) - L, tt_encs[k]))
			    break;
		    }
		}
		if (HaveTrueTypes || k < 0) {
		    code = copy_ps_file_stripping(s, fname, HaveTrueTypes);
		    if (code < 0)
			return code;
		}
	    }
	    if (c == e)
		break;
	    p = c + 1;
	}
    }
    if (!i)
	return_error(gs_error_undefinedfilename);
    return 0;
}

private int
encode(stream **s, const stream_template *t, gs_memory_t *mem)
{
    stream_state *st = s_alloc_state(mem, t->stype, "pdf_open_document.encode");

    if (st == 0)
	return_error(gs_error_VMerror);
    if (t->set_defaults)
	t->set_defaults(st);
    if (s_add_filter(s, t, st, mem) == 0) {
	gs_free_object(mem, st, "pdf_open_document.encode");
	return_error(gs_error_VMerror);
    }
    return 0;
}

/* ------ Document ------ */

/* Open the document if necessary. */
int
pdf_open_document(gx_device_pdf * pdev)
{
    if (!is_in_page(pdev) && pdf_stell(pdev) == 0) {
	stream *s = pdev->strm;
	int level = (int)(pdev->CompatibilityLevel * 10 + 0.5);

	pdev->binary_ok = !pdev->params.ASCII85EncodePages;
	if (pdev->ForOPDFRead && pdev->OPDFReadProcsetPath.size) {
	    int code, status;
	    
	    stream_write(s, (byte *)"%!PS-Adobe-2.0\r", 15);
	    if (pdev->params.CompressPages || pdev->CompressEntireFile) {
		/*  When CompressEntireFile is true and ASCII85EncodePages is false,
		    the ASCII85Encode filter is applied, rather one may expect the opposite.
		    Keeping it so due to no demand for this mode.
		    A right implementation should compute the length of the compressed procset,
		    write out an invocation of SubFileDecode filter, and write the length to
		    there assuming the output file is positionable. */
		stream_write(s, (byte *)"currentfile /ASCII85Decode filter /LZWDecode filter cvx exec\r", 61);
		code = encode(&s, &s_A85E_template, pdev->pdf_memory);
		if (code < 0)
		    return code;
		code = encode(&s, &s_LZWE_template, pdev->pdf_memory);
		if (code < 0)
		    return code;
	    }
	    code = copy_procsets(s, &pdev->OPDFReadProcsetPath, pdev->HaveTrueTypes);
	    if (code < 0)
		return code;
	    if (!pdev->CompressEntireFile) {
		status = s_close_filters(&s, pdev->strm);
		if (status < 0)
		    return_error(gs_error_ioerror);
	    } else
		pdev->strm = s;
	}
	pprintd2(s, "%%PDF-%d.%d\n", level / 10, level % 10);
	pdev->binary_ok = !pdev->params.ASCII85EncodePages;
	if (pdev->binary_ok)
	    stream_puts(s, "%\307\354\217\242\n");
    }
    /*
     * Determine the compression method.  Currently this does nothing.
     * It also isn't clear whether the compression method can now be
     * changed in the course of the document.
     *
     * Flate compression is available starting in PDF 1.2.  Since we no
     * longer support any older PDF versions, we ignore UseFlateCompression
     * and always use Flate compression.
     */
    if (!pdev->params.CompressPages)
	pdev->compression = pdf_compress_none;
    else
	pdev->compression = pdf_compress_Flate;
    return 0;
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
    stream_puts(pdev->strm, "endobj\n");
    return 0;
}

/* ------ Page contents ------ */

/* Handle transitions between contexts. */
private int
    none_to_stream(gx_device_pdf *), stream_to_text(gx_device_pdf *),
    string_to_text(gx_device_pdf *), text_to_stream(gx_device_pdf *),
    stream_to_none(gx_device_pdf *);
typedef int (*context_proc) (gx_device_pdf *);
private const context_proc context_procs[4][4] =
{
    {0, none_to_stream, none_to_stream, none_to_stream},
    {stream_to_none, 0, stream_to_text, stream_to_text},
    {text_to_stream, text_to_stream, 0, 0},
    {string_to_text, string_to_text, string_to_text, 0}
};

/* Compute an object encryption key. */
private int
pdf_object_key(const gx_device_pdf * pdev, gs_id object_id, byte key[16])
{
    md5_state_t md5;
    md5_byte_t zero[2] = {0, 0}, t;
    int KeySize = pdev->KeyLength / 8;

    md5_init(&md5);
    md5_append(&md5, pdev->EncryptionKey, KeySize);
    t = (byte)(object_id >>  0);  md5_append(&md5, &t, 1);
    t = (byte)(object_id >>  8);  md5_append(&md5, &t, 1);
    t = (byte)(object_id >> 16);  md5_append(&md5, &t, 1);
    md5_append(&md5, zero, 2);
    md5_finish(&md5, key);
    return min(KeySize + 5, 16);
}

/* Initialize encryption. */
int
pdf_encrypt_init(const gx_device_pdf * pdev, gs_id object_id, stream_arcfour_state *psarc4)
{
    byte key[16];

    return s_arcfour_set_key(psarc4, key, pdf_object_key(pdev, object_id, key));
}


/* Add the encryption filter. */
int
pdf_begin_encrypt(gx_device_pdf * pdev, stream **s, gs_id object_id)
{
    gs_memory_t *mem = pdev->v_memory;
    stream_arcfour_state *ss;
    md5_byte_t key[16];
    int code, keylength;

    if (!pdev->KeyLength)
	return 0;
    keylength = pdf_object_key(pdev, object_id, key);
    ss = gs_alloc_struct(mem, stream_arcfour_state, 
		    s_arcfour_template.stype, "psdf_encrypt");
    if (ss == NULL)
	return_error(gs_error_VMerror);
    code = s_arcfour_set_key(ss, key, keylength);
    if (code < 0)
	return code;
    if (s_add_filter(s, &s_arcfour_template, (stream_state *)ss, mem) == 0)
	return_error(gs_error_VMerror);
    return 0;
    /* IMPORTANT NOTE :
       We don't encrypt streams written into temporary files,
       because they can be used for comparizon
       (for example, for merging equal images).
       Instead that the encryption is applied in pdf_copy_data,
       when the stream is copied to the output file.
     */
}

/* Remove the encryption filter. */
void
pdf_end_encrypt(gx_device_pdf * pdev)
{
    if (pdev->KeyLength) {
	stream *s = pdev->strm;
	stream *fs = s->strm;

	sclose(s);
	gs_free_object(pdev->pdf_memory, s->cbuf, "encrypt buffer");
	gs_free_object(pdev->pdf_memory, s, "encrypt stream");
	pdev->strm = fs;
    }
}

/* Enter stream context. */
private int
none_to_stream(gx_device_pdf * pdev)
{
    stream *s;
    int code;

    if (pdev->contents_id != 0)
	return_error(gs_error_Fatal);	/* only 1 contents per page */
    pdev->compression_at_page_start = pdev->compression;
    if (pdev->ResourcesBeforeUsage) {
	pdf_resource_t *pres;

	code = pdf_enter_substream(pdev, resourcePage, gs_no_id, &pres, 
		    true, pdev->params.CompressPages);
	if (code < 0)
	    return code;
	pdev->contents_id = pres->object->id;
	pdev->contents_length_id = gs_no_id; /* inapplicable */
	pdev->contents_pos = -1; /* inapplicable */
	s = pdev->strm;
    } else {
    	pdev->contents_id = pdf_begin_obj(pdev);
	pdev->contents_length_id = pdf_obj_ref(pdev);
	s = pdev->strm;
	pprintld1(s, "<</Length %ld 0 R", pdev->contents_length_id);
	if (pdev->compression == pdf_compress_Flate)
	    pprints1(s, "/Filter /%s", compression_filter_name);
	stream_puts(s, ">>\nstream\n");
	pdev->contents_pos = pdf_stell(pdev);
	code = pdf_begin_encrypt(pdev, &s, pdev->contents_id);
	if (code < 0)
	    return code;
	pdev->strm = s;
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
    }
    /*
     * Scale the coordinate system.  Use an extra level of q/Q for the
     * sake of poorly designed PDF tools that assume that the contents
     * stream restores the CTM.
     */
    pprintg2(s, "q %g 0 0 %g 0 0 cm\n",
	     72.0 / pdev->HWResolution[0], 72.0 / pdev->HWResolution[1]);
    if (pdev->CompatibilityLevel >= 1.3) {
	/* Set the default rendering intent. */
	if (pdev->params.DefaultRenderingIntent != ri_Default) {
	    static const char *const ri_names[] = { psdf_ri_names };

	    pprints1(s, "/%s ri\n",
		     ri_names[(int)pdev->params.DefaultRenderingIntent]);
	}
    }
    pdev->AR4_save_bug = false;
    return PDF_IN_STREAM;
}
/* Enter text context from stream context. */
private int
stream_to_text(gx_device_pdf * pdev)
{
    int code;

    /*
     * Bizarrely enough, Acrobat Reader cares how the final font size is
     * obtained -- the CTM (cm), text matrix (Tm), and font size (Tf)
     * are *not* all equivalent.  In particular, it seems to use the
     * product of the text matrix and font size to decide how to
     * anti-alias characters.  Therefore, we have to temporarily patch
     * the CTM so that the scale factors are unity.  What a nuisance!
     */
    code = pdf_save_viewer_state(pdev, pdev->strm);
    if (code < 0)
	return 0;
    pprintg2(pdev->strm, "%g 0 0 %g 0 0 cm BT\n",
	     pdev->HWResolution[0] / 72.0, pdev->HWResolution[1] / 72.0);
    pdev->procsets |= Text;
    code = pdf_from_stream_to_text(pdev);
    return (code < 0 ? code : PDF_IN_TEXT);
}
/* Exit string context to text context. */
private int
string_to_text(gx_device_pdf * pdev)
{
    int code = pdf_from_string_to_text(pdev);

    return (code < 0 ? code : PDF_IN_TEXT);
}
/* Exit text context to stream context. */
private int
text_to_stream(gx_device_pdf * pdev)
{
    int code;

    stream_puts(pdev->strm, "ET\n");
    code = pdf_restore_viewer_state(pdev, pdev->strm);
    if (code < 0)
	return code;
    pdf_reset_text(pdev);	/* because of Q */
    return PDF_IN_STREAM;
}
/* Exit stream context. */
private int
stream_to_none(gx_device_pdf * pdev)
{
    stream *s = pdev->strm;
    long length;

    if (pdev->ResourcesBeforeUsage) {
	int code = pdf_exit_substream(pdev);

	if (code < 0)
	    return code;
    } else {
	if (pdev->vgstack_depth)
	    pdf_restore_viewer_state(pdev, s);
	if (pdev->compression_at_page_start == pdf_compress_Flate) {	/* Terminate the Flate filter. */
	    stream *fs = s->strm;

	    sclose(s);
	    gs_free_object(pdev->pdf_memory, s->cbuf, "zlib buffer");
	    gs_free_object(pdev->pdf_memory, s, "zlib stream");
	    pdev->strm = s = fs;
	}
	pdf_end_encrypt(pdev);
    	s = pdev->strm;
	length = pdf_stell(pdev) - pdev->contents_pos;
	stream_puts(s, "endstream\n");
	pdf_end_obj(pdev);
	pdf_open_obj(pdev, pdev->contents_length_id);
	pprintld1(s, "%ld\n", length);
	pdf_end_obj(pdev);
    }
    return PDF_IN_NONE;
}

/* Begin a page contents part. */
int
pdf_open_contents(gx_device_pdf * pdev, pdf_context_t context)
{
    int (*proc) (gx_device_pdf *);

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
	int code = pdf_open_contents(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
	stream_puts(pdev->strm, "Q\n");	/* See none_to_stream. */
	pdf_close_text_contents(pdev);
    }
    return pdf_open_contents(pdev, PDF_IN_NONE);
}

/* ------ Resources et al ------ */

/* Define the allocator descriptors for the resource types. */
const char *const pdf_resource_type_names[] = {
    PDF_RESOURCE_TYPE_NAMES
};
const gs_memory_struct_type_t *const pdf_resource_type_structs[] = {
    PDF_RESOURCE_TYPE_STRUCTS
};

/* Cancel a resource (do not write it into PDF). */
int
pdf_cancel_resource(gx_device_pdf * pdev, pdf_resource_t *pres, pdf_resource_type_t rtype)
{
    /* fixme : remove *pres from resource chain. */
    pres->where_used = 0;
    pres->object->written = true;
    if (rtype == resourceXObject || rtype == resourceCharProc || rtype == resourceOther
	) {
	int code = cos_stream_release_pieces((cos_stream_t *)pres->object);

	if (code < 0)
	    return code;
    }
    cos_release(pres->object, "pdf_cancel_resource");
    return 0;
}

/* Remove a resource. */
void
pdf_forget_resource(gx_device_pdf * pdev, pdf_resource_t *pres1, pdf_resource_type_t rtype)
{   /* fixme : optimize. */
    pdf_resource_t **pchain = pdev->resources[rtype].chains;
    pdf_resource_t *pres;
    pdf_resource_t **pprev = &pdev->last_resource;
    int i;

    for (; (pres = *pprev) != 0; pprev = &pres->prev)
	if (pres == pres1) {
	    *pprev = pres->prev;
	    break;
	}
    for (i = 0; i < NUM_RESOURCE_CHAINS; i++) {
	pprev = pchain + i;
	for (; (pres = *pprev) != 0; pprev = &pres->next)
	    if (pres == pres1) {
		*pprev = pres->next;
		COS_RELEASE(pres->object, "pdf_forget_resource");
		gs_free_object(pdev->pdf_memory, pres->object, "pdf_forget_resource");
		gs_free_object(pdev->pdf_memory, pres, "pdf_forget_resource");
		break;
	    }
    }
}

private int 
nocheck(gx_device_pdf * pdev, pdf_resource_t *pres0, pdf_resource_t *pres1)
{
    return 1;
}


/* Substitute a resource with a same one. */
int
pdf_substitute_resource(gx_device_pdf *pdev, pdf_resource_t **ppres, 
	pdf_resource_type_t rtype, 
	int (*eq)(gx_device_pdf * pdev, pdf_resource_t *pres0, pdf_resource_t *pres1),
	bool write)
{
    pdf_resource_t *pres1 = *ppres;
    int code;

    code = pdf_find_same_resource(pdev, rtype, ppres, (eq ? eq : nocheck));
    if (code < 0)
	return code;
    if (code != 0) {
	code = pdf_cancel_resource(pdev, (pdf_resource_t *)pres1, rtype);
	if (code < 0)
	    return code;
	pdf_forget_resource(pdev, pres1, rtype);
	return 0;
    } else {
	pdf_reserve_object_id(pdev, pres1, gs_no_id);
	if (write) {
	    code = cos_write_object(pres1->object, pdev);
	    if (code < 0)
		return code;
	    pres1->object->written = 1;
	}
	return 1;
    }
}

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

/* Find resource by resource id. */
pdf_resource_t *
pdf_find_resource_by_resource_id(gx_device_pdf * pdev, pdf_resource_type_t rtype, gs_id id)
{
    pdf_resource_t **pchain = pdev->resources[rtype].chains;
    pdf_resource_t *pres;
    int i;
    
    for (i = 0; i < NUM_RESOURCE_CHAINS; i++) {
	for (pres = pchain[i]; pres != 0; pres = pres->next) {
	    if (pres->object->id == id)
		return pres;
	}
    }
    return 0;
}


/* Find same resource. */
int
pdf_find_same_resource(gx_device_pdf * pdev, pdf_resource_type_t rtype, pdf_resource_t **ppres,
	int (*eq)(gx_device_pdf * pdev, pdf_resource_t *pres0, pdf_resource_t *pres1))
{
    pdf_resource_t **pchain = pdev->resources[rtype].chains;
    pdf_resource_t *pres;
    cos_object_t *pco0 = (*ppres)->object;
    int i;
    
    for (i = 0; i < NUM_RESOURCE_CHAINS; i++) {
	for (pres = pchain[i]; pres != 0; pres = pres->next) {
	    if (!pres->named && *ppres != pres) {
		cos_object_t *pco1 = pres->object;
		int code = pco0->cos_procs->equal(pco0, pco1, pdev);

		if (code < 0)
		    return code;
		if (code > 0) {
		    code = eq(pdev, *ppres, pres);
		    if (code < 0)
			return code;
		    if (code > 0) {
			*ppres = pres;
			return 1;
		    }
		}
	    }
	}
    }
    return 0;
}

/* Drop resources by a condition. */
void
pdf_drop_resources(gx_device_pdf * pdev, pdf_resource_type_t rtype, 
	int (*cond)(gx_device_pdf * pdev, pdf_resource_t *pres))
{
    pdf_resource_t **pchain = pdev->resources[rtype].chains;
    pdf_resource_t **pprev;
    pdf_resource_t *pres;
    int i;

    for (i = 0; i < NUM_RESOURCE_CHAINS; i++) {
	pprev = pchain + i;
	for (; (pres = *pprev) != 0; ) {
	    if (cond(pdev, pres)) {
		*pprev = pres->next;
		pres->next = pres; /* A temporary mark - see below */
	    } else
		pprev = &pres->next;
	}
    }
    pprev = &pdev->last_resource;
    for (; (pres = *pprev) != 0; )
	if (pres->next == pres) {
	    *pprev = pres->prev;
	    COS_RELEASE(pres->object, "pdf_drop_resources");
	    gs_free_object(pdev->pdf_memory, pres->object, "pdf_drop_resources");
	    gs_free_object(pdev->pdf_memory, pres, "pdf_drop_resources");
	} else
	    pprev = &pres->prev;
}

/* Print resource statistics. */
void
pdf_print_resource_statistics(gx_device_pdf * pdev)
{

    int rtype;

    for (rtype = 0; rtype < NUM_RESOURCE_TYPES; rtype++) {
	pdf_resource_t **pchain = pdev->resources[rtype].chains;
	pdf_resource_t *pres;
	const char *name = pdf_resource_type_names[rtype];
	int i, n = 0;
    
	for (i = 0; i < NUM_RESOURCE_CHAINS; i++) {
	    for (pres = pchain[i]; pres != 0; pres = pres->next, n++);
	}
	dprintf3("Resource type %d (%s) has %d instances.\n", rtype, 
		(name ? name : ""), n);
    }
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

void
pdf_reserve_object_id(gx_device_pdf * pdev, pdf_resource_t *pres, long id)
{
    pres->object->id = (id == 0 ? pdf_obj_ref(pdev) : id);
    sprintf(pres->rname, "R%ld", pres->object->id);
}

/* Begin an aside (resource, annotation, ...). */
int
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
    if (pres == 0)
	return_error(gs_error_VMerror);
    object = cos_object_alloc(pdev, "pdf_alloc_aside(object)");
    if (object == 0)
	return_error(gs_error_VMerror);
    memset(pres + 1, 0, pst->ssize - sizeof(*pres));
    pres->object = object;
    if (id < 0) {
	object->id = -1L;
	pres->rname[0] = 0;
    } else
	pdf_reserve_object_id(pdev, pres, id);
    pres->next = *plist;
    *plist = pres;
    pres->prev = pdev->last_resource;
    pdev->last_resource = pres;
    pres->named = false;
    pres->global = false;
    pres->where_used = pdev->used_mask;
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
			       pdf_resource_type_structs[rtype], ppres);

    if (code >= 0)
	(*ppres)->rid = rid;
    return code;
}
int
pdf_begin_resource(gx_device_pdf * pdev, pdf_resource_type_t rtype, gs_id rid,
		   pdf_resource_t ** ppres)
{
    int code = pdf_begin_resource_body(pdev, rtype, rid, ppres);

    if (code >= 0 && pdf_resource_type_names[rtype] != 0) {
	stream *s = pdev->strm;

	pprints1(s, "<</Type%s", pdf_resource_type_names[rtype]);
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
			       pdf_resource_type_structs[rtype], ppres, id);

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

/*
 * Write the Cos objects for resources local to a content stream.  Formerly,
 * this procedure also freed such objects, but this doesn't work, because
 * resources of one type might refer to resources of another type.
 */
int
pdf_write_resource_objects(gx_device_pdf *pdev, pdf_resource_type_t rtype)
{
    int j, code = 0;

    for (j = 0; j < NUM_RESOURCE_CHAINS && code >= 0; ++j) {
	pdf_resource_t *pres = pdev->resources[rtype].chains[j];

	for (; pres != 0; pres = pres->next)
	    if ((!pres->named || pdev->ForOPDFRead) 
		&& !pres->object->written)
		code = cos_write_object(pres->object, pdev);

    }
    return code;
}

/*
 * Reverse resource chains.
 * ps2write uses it with page resources.
 * Assuming only the 0th chain contauns something.
 */
void
pdf_reverse_resource_chain(gx_device_pdf *pdev, pdf_resource_type_t rtype)
{
    pdf_resource_t *pres = pdev->resources[rtype].chains[0];
    pdf_resource_t *pres1, *pres0 = pres, *pres2;

    if (pres == NULL)
	return;
    pres1 = pres->next;
    for (;;) {
	if (pres1 == NULL)
	    break;
	pres2 = pres1->next;
	pres1->next = pres;
	pres = pres1;
	pres1 = pres2;
    }
    pres0->next = NULL;
    pdev->resources[rtype].chains[0] = pres;
}


/*
 * Free unnamed Cos objects for resources local to a content stream,
 * since they can't be used again.
 */
int
pdf_free_resource_objects(gx_device_pdf *pdev, pdf_resource_type_t rtype)
{
    int j;

    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j) {
	pdf_resource_t **prev = &pdev->resources[rtype].chains[j];
	pdf_resource_t *pres;

	while ((pres = *prev) != 0) {
	    if (pres->named) {	/* named, don't free */
		prev = &pres->next;
	    } else {
		cos_free(pres->object, "pdf_free_resource_objects");
		pres->object = 0;
		*prev = pres->next;
	    }
	}
    }
    return 0;
}

/* Write and free all resource objects. */

int
pdf_write_and_free_all_resource_objects(gx_device_pdf *pdev)
{
    int i, code = 0, code1;

    for (i = 0; i < NUM_RESOURCE_TYPES; ++i) {
	code1 = pdf_write_resource_objects(pdev, i);
	if (code >= 0)
	    code = code1;
    }
    code1 = pdf_finish_font_descriptors(pdev, pdf_release_FontDescriptor_components);
    if (code >= 0)
	code = code1;
    for (i = 0; i < NUM_RESOURCE_TYPES; ++i) {
	code1 = pdf_free_resource_objects(pdev, i);
	if (code >= 0)
	    code = code1;
    }
    return code;
}

/*
 * Store the resource sets for a content stream (page or XObject).
 * Sets page->{procsets, resource_ids[]}.
 */
int
pdf_store_page_resources(gx_device_pdf *pdev, pdf_page_t *page)
{
    int i;

    /* Write any resource dictionaries. */

    for (i = 0; i <= resourceFont; ++i) {
	stream *s = 0;
	int j;

	page->resource_ids[i] = 0;
	for (j = 0; j < NUM_RESOURCE_CHAINS; ++j) {
	    pdf_resource_t *pres = pdev->resources[i].chains[j];

	    for (; pres != 0; pres = pres->next) {
		if (pres->where_used & pdev->used_mask) {
		    long id = pres->object->id;

		    if (s == 0) {
			page->resource_ids[i] = pdf_begin_separate(pdev);
			s = pdev->strm;
			stream_puts(s, "<<");
		    }
		    pprints1(s, "/%s\n", pres->rname);
		    pprintld1(s, "%ld 0 R", id);
		    pres->where_used -= pdev->used_mask;
		}
	    }
	}
	if (s) {
	    stream_puts(s, ">>\n");
	    pdf_end_separate(pdev);
	    if (i != resourceFont)
		pdf_write_resource_objects(pdev, i);
	}
    }
    page->procsets = pdev->procsets;
    return 0;
}

/* Copy data from a temporary file to a stream. */
void
pdf_copy_data(stream *s, FILE *file, long count, stream_arcfour_state *ss)
{
    long left = count;
    byte buf[sbuf_size];
    
    while (left > 0) {
	uint copy = min(left, sbuf_size);

	fread(buf, 1, copy, file);
	if (ss)
	    s_arcfour_process_buffer(ss, buf, copy);
	stream_write(s, buf, copy);
	left -= copy;
    }
}


/* Copy data from a temporary file to a stream, 
   which may be targetted to the same file. */
void
pdf_copy_data_safe(stream *s, FILE *file, long position, long count)
{   
    long left = count;

    while (left > 0) {
	byte buf[sbuf_size];
	long copy = min(left, (long)sbuf_size);
	long end_pos = ftell(file);

	fseek(file, position + count - left, SEEK_SET);
	fread(buf, 1, copy, file);
	fseek(file, end_pos, SEEK_SET);
	stream_write(s, buf, copy);
	sflush(s);
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
	stream_write(pdev->strm, pstr->data, pstr->size);
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
	int code;

	if (pdf_page_id(pdev, pdev->next_page + 1) == 0)
	    return_error(gs_error_VMerror);
	code = pdf_open_document(pdev);
	if (code < 0)
	    return code;
    }
    /* Note that context may be PDF_IN_NONE here. */
    return pdf_open_contents(pdev, context);
}


/*  Go to the unclipped stream context. */
int
pdf_unclip(gx_device_pdf * pdev)
{
    const int bottom = (pdev->ResourcesBeforeUsage ? 1 : 0);
    /* When ResourcesBeforeUsage != 0, one sbstack element 
       appears from the page contents stream. */

    if (pdev->sbstack_depth <= bottom) {
	int code = pdf_open_page(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
    }
    if (pdev->context > PDF_IN_STREAM) {
	int code = pdf_open_contents(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
    }
    if (pdev->vgstack_depth > pdev->vgstack_bottom) {
	int code = pdf_restore_viewer_state(pdev, pdev->strm);

	if (code < 0)
	    return code;
	code = pdf_remember_clip_path(pdev, NULL);
	if (code < 0)
	    return code;
	pdev->clip_path_id = pdev->no_clip_path_id;
    }
    return 0;
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
	stream_puts(s, before);
    pprintg6(s, "%g %g %g %g %g %g ",
	     pmat->xx, pmat->xy, pmat->yx, pmat->yy, pmat->tx, pmat->ty);
    if (after)
	stream_puts(s, after);
}

/*
 * Write a name, with escapes for unusual characters.  Since we only support
 * PDF 1.2 and above, we can use an escape sequence for anything except a
 * null <00>, and the machinery for selecting the put_name_chars procedure
 * depending on CompatibilityLevel is no longer needed.
 */
private int
pdf_put_name_chars_1_2(stream *s, const byte *nstr, uint size)
{
    uint i;

    for (i = 0; i < size; ++i) {
	uint c = nstr[i];
	char hex[4];

	switch (c) {
	    default:
		if (c >= 0x21 && c <= 0x7e) {
		    stream_putc(s, (byte)c);
		    break;
		}
		/* falls through */
	    case '#':
	    case '%':
	    case '(': case ')':
	    case '<': case '>':
	    case '[': case ']':
	    case '{': case '}':
	    case '/':
		sprintf(hex, "#%02x", c);
		stream_puts(s, hex);
		break;
	    case 0:
		stream_puts(s, "BnZr"); /* arbitrary */
	}
    }
    return 0;
}
pdf_put_name_chars_proc_t
pdf_put_name_chars_proc(const gx_device_pdf *pdev)
{
    return &pdf_put_name_chars_1_2;
}
int
pdf_put_name_chars(const gx_device_pdf *pdev, const byte *nstr, uint size)
{
    return pdf_put_name_chars_proc(pdev)(pdev->strm, nstr, size);
}
int
pdf_put_name(const gx_device_pdf *pdev, const byte *nstr, uint size)
{
    stream_putc(pdev->strm, '/');
    return pdf_put_name_chars(pdev, nstr, size);
}

/* Write an encoded string with encryption. */
private int
pdf_encrypt_encoded_string(const gx_device_pdf *pdev, const byte *str, uint size, gs_id object_id)
{
    stream sinp, sstr, sout;
    stream_PSSD_state st;
    stream_state so;
    byte buf[100], bufo[100];
    stream_arcfour_state sarc4;

    if (pdf_encrypt_init(pdev, object_id, &sarc4) < 0) {
	/* The interface can't pass an error. */
	stream_write(pdev->strm, str, size);
	return size;
    }
    sread_string(&sinp, str + 1, size);
    s_init(&sstr, NULL);
    sstr.close_at_eod = false;
    s_init_state((stream_state *)&st, &s_PSSD_template, NULL);
    s_init_filter(&sstr, (stream_state *)&st, buf, sizeof(buf), &sinp);
    s_init(&sout, NULL);
    s_init_state(&so, &s_PSSE_template, NULL);
    s_init_filter(&sout, &so, bufo, sizeof(bufo), pdev->strm);
    stream_putc(pdev->strm, '(');
    for (;;) {
	uint n;
	int code = sgets(&sstr, buf, sizeof(buf), &n);

	if (n > 0) {
	    s_arcfour_process_buffer(&sarc4, buf, n);
	    stream_write(&sout, buf, n);
	}
	if (code == EOFC)
	    break;
	if (code < 0 || n < sizeof(buf)) {
	    /* The interface can't pass an error. */
	    break;
	}
    }
    sclose(&sout); /* Writes ')'. */
    return stell(&sinp) + 1;
}

/* Write an encoded string with possible encryption. */
private int
pdf_put_encoded_string(const gx_device_pdf *pdev, const byte *str, uint size, gs_id object_id)
{
    if (!pdev->KeyLength || object_id == (gs_id)-1) {
	stream_write(pdev->strm, str, size);
	return 0;
    } else
	return pdf_encrypt_encoded_string(pdev, str, size, object_id);
}
/* Write an encoded hexadecimal string with possible encryption. */
private int
pdf_put_encoded_hex_string(const gx_device_pdf *pdev, const byte *str, uint size, gs_id object_id)
{
    eprintf("Unimplemented function : pdf_put_encoded_hex_string\n");
    stream_write(pdev->strm, str, size);
    return_error(gs_error_unregistered);
}
/*  Scan an item in a serialized array or dictionary.
    This is a very simplified Postscript lexical scanner.
    It assumes the serialization with pdf===only defined in gs/lib/gs_pdfwr.ps .
    We only need to select strings and encrypt them.
    Other items are passed identically.
    Note we don't reconstruct the nesting of arrays|dictionaries.
*/
private int
pdf_scan_item(const gx_device_pdf * pdev, const byte * p, uint l, gs_id object_id)
{
    const byte *q = p;
    int n = l; 

    if (*q == ' ' || *q == 't' || *q == '\r' || *q == '\n')
	return (l > 0 ? 1 : 0);
    for (q++, n--; n; q++, n--) {
	if (*q == ' ' || *q == 't' || *q == '\r' || *q == '\n')
	    return q - p;
	if (*q == '/' || *q == '[' || *q == ']' || *q == '{' || *q == '}' || *q == '(' || *q == '<')
	    return q - p;
	/* Note : immediate names are not allowed in PDF. */
    }
    return l;
}

/* Write a serialized array or dictionary with possible encryption. */
private int
pdf_put_composite(const gx_device_pdf * pdev, const byte * vstr, uint size, gs_id object_id)
{
    if (!pdev->KeyLength || object_id == (gs_id)-1) {
	stream_write(pdev->strm, vstr, size);
    } else {
	const byte *p = vstr;
	int l = size, n;

	for (;l > 0 ;) {
	    if (*p == '(')
		n = pdf_encrypt_encoded_string(pdev, p, l, object_id);
	    else {
		n = pdf_scan_item(pdev, p, l, object_id);
		stream_write(pdev->strm, p, n);
	    }
	    l -= n;
	    p += n;
	}
    }
    return 0;
}

/*
 * Write a string in its shortest form ( () or <> ).  Note that
 * this form is different depending on whether binary data are allowed.
 * We wish PDF supported ASCII85 strings ( <~ ~> ), but it doesn't.
 */
int
pdf_put_string(const gx_device_pdf * pdev, const byte * str, uint size)
{
    psdf_write_string(pdev->strm, str, size,
		      (pdev->binary_ok ? PRINT_BINARY_OK : 0));
    return 0;
}

/* Write a value, treating names specially. */
int
pdf_write_value(const gx_device_pdf * pdev, const byte * vstr, uint size, gs_id object_id)
{
    if (size > 0 && vstr[0] == '/')
	return pdf_put_name(pdev, vstr + 1, size - 1);
    else if (size > 3 && vstr[0] == 0 && vstr[1] == 0 && vstr[size - 1] == 0)
	return pdf_put_name(pdev, vstr + 3, size - 4);
    else if (size > 1 && (vstr[0] == '[' || vstr[0] == '{'))
	return pdf_put_composite(pdev, vstr, size, object_id);
    else if (size > 2 && vstr[0] == '<' && vstr[1] == '<')
	return pdf_put_composite(pdev, vstr, size, object_id);
    else if (size > 1 && vstr[0] == '(')
	return pdf_put_encoded_string(pdev, vstr, size, object_id);
    else if (size > 1 && vstr[0] == '<')
	return pdf_put_encoded_hex_string(pdev, vstr, size, object_id);
    stream_write(pdev->strm, vstr, size);
    return 0;
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
	     * If EndOfBlock is true, we mustn't write a Rows value.
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
    const stream_template *template = (pdev->CompatibilityLevel < 1.3 ? 
		    &s_LZWE_template : &s_zlibE_template);
    stream_state *st = s_alloc_state(pdev->pdf_memory, template->stype,
				     "pdf_write_function");

    if (st == 0)
	return_error(gs_error_VMerror);
    if (template->set_defaults)
	template->set_defaults(st);
    return psdf_encode_binary(pbw, template, st);
}

/*
 * Begin a data stream.  The client has opened the object and written
 * the << and any desired dictionary keys.
 */
int
pdf_begin_data(gx_device_pdf *pdev, pdf_data_writer_t *pdw)
{
    return pdf_begin_data_stream(pdev, pdw,
				 DATA_STREAM_BINARY | DATA_STREAM_COMPRESS, 0);
}

int
pdf_append_data_stream_filters(gx_device_pdf *pdev, pdf_data_writer_t *pdw,
		      int orig_options, gs_id object_id)
{
    stream *s = pdev->strm;
    int options = orig_options;
#define USE_ASCII85 1
#define USE_FLATE 2
    static const char *const fnames[4] = {
	"", "/Filter/ASCII85Decode", "/Filter/FlateDecode",
	"/Filter[/ASCII85Decode/FlateDecode]"
    };
    static const char *const fnames1_2[4] = {
	"", "/Filter/ASCII85Decode", "/Filter/LZWDecode",
	"/Filter[/ASCII85Decode/LZWDecode]"
    };
    int filters = 0;
    int code;

    if (options & DATA_STREAM_COMPRESS) {
	filters |= USE_FLATE;
	options |= DATA_STREAM_BINARY;
    }
    if ((options & DATA_STREAM_BINARY) && !pdev->binary_ok)
	filters |= USE_ASCII85;
    if (!(options & DATA_STREAM_NOLENGTH)) {
	stream_puts(s, (pdev->CompatibilityLevel < 1.3 ? 
	    fnames1_2[filters] : fnames[filters]));
	if (pdev->ResourcesBeforeUsage) {
	    pdw->length_pos = stell(s) + 8;
	    stream_puts(s, "/Length             >>stream\n");
	    pdw->length_id = -1;
	} else {
	    pdw->length_pos = -1;		
	    pdw->length_id = pdf_obj_ref(pdev);
	    pprintld1(s, "/Length %ld 0 R>>stream\n", pdw->length_id);
	}
    }
    if (options & DATA_STREAM_ENCRYPT) {
	code = pdf_begin_encrypt(pdev, &s, object_id);
	if (code < 0)
	    return code;
	pdev->strm = s;
	pdw->encrypted = true;
    } else
    	pdw->encrypted = false;
    if (options & DATA_STREAM_BINARY) {
	code = psdf_begin_binary((gx_device_psdf *)pdev, &pdw->binary);
	if (code < 0)
	    return code;
    } else {
	code = 0;
	pdw->binary.target = pdev->strm;
	pdw->binary.dev = (gx_device_psdf *)pdev;
	pdw->binary.strm = pdev->strm;
    }
    pdw->start = stell(s);
    if (filters & USE_FLATE)
	code = pdf_flate_binary(pdev, &pdw->binary);
    return code;
#undef USE_ASCII85
#undef USE_FLATE
}

int
pdf_begin_data_stream(gx_device_pdf *pdev, pdf_data_writer_t *pdw,
		      int options, gs_id object_id)
{   int code;
    /* object_id is an unused rudiment from the old code,
       when the encription was applied when creating the stream.
       The new code encrypts than copying stream from the temporary file. */
    pdw->pdev = pdev;  /* temporary for backward compatibility of pdf_end_data prototype. */
    pdw->binary.target = pdev->strm;
    pdw->binary.dev = (gx_device_psdf *)pdev;
    pdw->binary.strm = 0;		/* for GC in case of failure */
    code = pdf_open_aside(pdev, resourceOther, gs_no_id, &pdw->pres, !object_id, 
		options);
    if (object_id != 0)
	pdf_reserve_object_id(pdev, pdw->pres, object_id);
    pdw->binary.strm = pdev->strm;
    return code;
}

/* End a data stream. */
int
pdf_end_data(pdf_data_writer_t *pdw)
{   int code;

    code = pdf_close_aside(pdw->pdev);
    if (code < 0)
	return code;
    code = COS_WRITE_OBJECT(pdw->pres->object, pdw->pdev);
    if (code < 0)
	return code;
    return 0;
}

/* Create a Function object. */
private int pdf_function_array(gx_device_pdf *pdev, cos_array_t *pca,
			       const gs_function_info_t *pinfo);
int
pdf_function_scaled(gx_device_pdf *pdev, const gs_function_t *pfn,
		    const gs_range_t *pranges, cos_value_t *pvalue)
{
    if (pranges == NULL)
	return pdf_function(pdev, pfn, pvalue);
    {
	/*
	 * Create a temporary scaled function.  Note that the ranges
	 * represent the inverse scaling from what gs_function_make_scaled
	 * expects.
	 */
	gs_memory_t *mem = pdev->pdf_memory;
	gs_function_t *psfn;
	gs_range_t *ranges = (gs_range_t *)
	    gs_alloc_byte_array(mem, pfn->params.n, sizeof(gs_range_t),
				"pdf_function_scaled");
	int i, code;

	if (ranges == 0)
	    return_error(gs_error_VMerror);
	for (i = 0; i < pfn->params.n; ++i) {
	    double rbase = pranges[i].rmin;
	    double rdiff = pranges[i].rmax - rbase;
	    double invbase = -rbase / rdiff;

	    ranges[i].rmin = invbase;
	    ranges[i].rmax = invbase + 1.0 / rdiff;
	}
	code = gs_function_make_scaled(pfn, &psfn, ranges, mem);
	if (code >= 0) {
	    code = pdf_function(pdev, psfn, pvalue);
	    gs_function_free(psfn, true, mem);
	}
	gs_free_object(mem, ranges, "pdf_function_scaled");
	return code;
    }
}
private int
pdf_function_aux(gx_device_pdf *pdev, const gs_function_t *pfn,
	     pdf_resource_t **ppres)
{
    gs_function_info_t info;
    cos_param_list_writer_t rlist;
    pdf_resource_t *pres;
    cos_object_t *pcfn;
    cos_dict_t *pcd;
    int code = pdf_alloc_resource(pdev, resourceFunction, gs_no_id, &pres, -1);

    if (code < 0) {
	*ppres = 0;
	return code;
    }
    *ppres = pres;
    pcfn = pres->object;
    gs_function_get_info(pfn, &info);
    if (FunctionType(pfn) == function_type_ArrayedOutput) {
	/*
	 * Arrayed Output Functions are used internally to represent
	 * Shading Function entries that are arrays of Functions.
	 * They require special handling.
	 */
	cos_array_t *pca;

	cos_become(pcfn, cos_type_array);
	pca = (cos_array_t *)pcfn;
	return pdf_function_array(pdev, pca, &info);
    }
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
	if (code >= 0 && info.data_size > 30	/* 30 is arbitrary */
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
		stream_write(writer.strm, ptr, count);
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
	cos_array_t *functions =
	    cos_array_alloc(pdev, "pdf_function(Functions)");
	cos_value_t v;

	if (functions == 0)
	    return_error(gs_error_VMerror);
	if ((code = pdf_function_array(pdev, functions, &info)) < 0 ||
	    (code = cos_dict_put_c_key(pcd, "/Functions",
				       COS_OBJECT_VALUE(&v, functions))) < 0
	    ) {
	    COS_FREE(functions, "pdf_function(Functions)");
	    return code;
	}
    }
    code = cos_param_list_writer_init(&rlist, pcd, PRINT_BINARY_OK);
    if (code < 0)
	return code;
    return gs_function_get_params(pfn, (gs_param_list *)&rlist);
}
private int 
functions_equal(gx_device_pdf * pdev, pdf_resource_t *pres0, pdf_resource_t *pres1)
{
    return true;
}
int
pdf_function(gx_device_pdf *pdev, const gs_function_t *pfn, cos_value_t *pvalue)
{
    pdf_resource_t *pres;
    int code = pdf_function_aux(pdev, pfn, &pres);

    if (code < 0)
	return code;
    code = pdf_substitute_resource(pdev, &pres, resourceFunction, functions_equal, false);
    if (code < 0)
	return code;
    COS_OBJECT_VALUE(pvalue, pres->object);
    return 0;
}
private int pdf_function_array(gx_device_pdf *pdev, cos_array_t *pca,
			       const gs_function_info_t *pinfo)
{
    int i, code = 0;
    cos_value_t v;

    for (i = 0; i < pinfo->num_Functions; ++i) {
	if ((code = pdf_function(pdev, pinfo->Functions[i], &v)) < 0 ||
	    (code = cos_array_add(pca, &v)) < 0
	    ) {
	    break;
	}
    }
    return code;
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

/* Write a FontBBox dictionary element. */
int
pdf_write_font_bbox(gx_device_pdf *pdev, const gs_int_rect *pbox)
{
    stream *s = pdev->strm;
    /*
     * AR 4 doesn't like fonts with empty FontBBox, which
     * happens when the font contains only space characters.
     * Small bbox causes AR 4 to display a hairline. So we use
     * the full BBox.
     */ 
    int x = pbox->q.x + ((pbox->p.x == pbox->q.x) ? 1000 : 0);
    int y = pbox->q.y + ((pbox->p.y == pbox->q.y) ? 1000 : 0);

    pprintd4(s, "/FontBBox[%d %d %d %d]",
	     pbox->p.x, pbox->p.y, x, y);
    return 0;
}
