/* Copyright (C) 1994, 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: sjpegc.c,v 1.4 2001/04/07 00:33:22 rayjj Exp $ */
/* Interface routines for IJG code, common to encode/decode. */
#include "stdio_.h"
#include "string_.h"
#include "jpeglib_.h"
#include "jerror_.h"
#include "jmemsys.h"		/* for prototypes */
#include "gx.h"
#include "gserrors.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"

private_st_jpeg_block();

/*
 * Error handling routines (these replace corresponding IJG routines from
 * jpeg/jerror.c).  These are used for both compression and decompression.
 * We assume
 * offset_of(jpeg_compress_data, cinfo)==offset_of(jpeg_decompress_data, dinfo)
 */

private void
gs_jpeg_error_exit(j_common_ptr cinfo)
{
    jpeg_stream_data *jcomdp =
    (jpeg_stream_data *) ((char *)cinfo -
			  offset_of(jpeg_compress_data, cinfo));

    longjmp(jcomdp->exit_jmpbuf, 1);
}

private void
gs_jpeg_emit_message(j_common_ptr cinfo, int msg_level)
{
    if (msg_level < 0) {	/* GS policy is to ignore IJG warnings when Picky=0,
				 * treat them as errors when Picky=1.
				 */
	jpeg_stream_data *jcomdp =
	(jpeg_stream_data *) ((char *)cinfo -
			      offset_of(jpeg_compress_data, cinfo));

	if (jcomdp->Picky)
	    gs_jpeg_error_exit(cinfo);
    }
    /* Trace messages are always ignored. */
}

/*
 * This routine initializes the error manager fields in the JPEG object.
 * It is based on jpeg_std_error from jpeg/jerror.c.
 */

void
gs_jpeg_error_setup(stream_DCT_state * st)
{
    struct jpeg_error_mgr *err = &st->data.common->err;

    /* Initialize the JPEG compression object with default error handling */
    jpeg_std_error(err);

    /* Replace some methods with our own versions */
    err->error_exit = gs_jpeg_error_exit;
    err->emit_message = gs_jpeg_emit_message;

    st->data.compress->cinfo.err = err;		/* works for decompress case too */
}

/* Stuff the IJG error message into errorinfo after an error exit. */

int
gs_jpeg_log_error(stream_DCT_state * st)
{
    j_common_ptr cinfo = (j_common_ptr) & st->data.compress->cinfo;
    char buffer[JMSG_LENGTH_MAX];

    /* Format the error message */
    (*cinfo->err->format_message) (cinfo, buffer);
    (*st->report_error) ((stream_state *) st, buffer);
    return gs_error_ioerror;	/* caller will do return_error() */
}


/*
 * Interface routines.  This layer of routines exists solely to limit
 * side-effects from using setjmp.
 */


JQUANT_TBL *
gs_jpeg_alloc_quant_table(stream_DCT_state * st)
{
    if (setjmp(st->data.common->exit_jmpbuf)) {
	gs_jpeg_log_error(st);
	return NULL;
    }
    return jpeg_alloc_quant_table((j_common_ptr)
				  & st->data.compress->cinfo);
}

JHUFF_TBL *
gs_jpeg_alloc_huff_table(stream_DCT_state * st)
{
    if (setjmp(st->data.common->exit_jmpbuf)) {
	gs_jpeg_log_error(st);
	return NULL;
    }
    return jpeg_alloc_huff_table((j_common_ptr)
				 & st->data.compress->cinfo);
}

int
gs_jpeg_destroy(stream_DCT_state * st)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_destroy((j_common_ptr) & st->data.compress->cinfo);
    return 0;
}


/*
 * These routines replace the low-level memory manager of the IJG library.
 * They pass malloc/free calls to the Ghostscript memory manager.
 * Note we do not need these to be declared in any GS header file.
 */

private inline jpeg_compress_data *
cinfo2jcd(j_common_ptr cinfo)
{   /* We use the offset of cinfo in jpeg_compress data here, but we */
    /* could equally well have used jpeg_decompress_data.            */
    return (jpeg_compress_data *)
      ((byte *)cinfo - offset_of(jpeg_compress_data, cinfo));
}

private void *
jpeg_alloc(j_common_ptr cinfo, size_t size, const char *info)
{
    jpeg_compress_data *jcd = cinfo2jcd(cinfo);
    gs_memory_t *mem = jcd->memory;
    
    jpeg_block_t *p = gs_alloc_struct_immovable(mem, jpeg_block_t,
    			&st_jpeg_block, "jpeg_alloc(block)");
    void *data = gs_alloc_bytes_immovable(mem, size, info);

    if (p == 0 || data == 0) {
	gs_free_object(mem, data, info);
	gs_free_object(mem, p, "jpeg_alloc(block)");
	return 0;
    }
    p->data = data;
    p->next = jcd->blocks;
    jcd->blocks = p;
    return data;
}

private void
jpeg_free(j_common_ptr cinfo, void *data, const char *info)
{
    jpeg_compress_data *jcd = cinfo2jcd(cinfo);
    gs_memory_t *mem = jcd->memory;
    jpeg_block_t  *p  =  jcd->blocks;
    jpeg_block_t **pp = &jcd->blocks;

    gs_free_object(mem, data, info);
    while(p && p->data != data)
      { pp = &p->next;
        p = p->next;
      }
    if(p == 0)
      lprintf1("Freeing unrecorded JPEG data 0x%lx!\n", (ulong)data);
    else
      *pp = p->next;
    gs_free_object(mem, p, "jpeg_free(block)");
}

void *
jpeg_get_small(j_common_ptr cinfo, size_t size)
{
    return jpeg_alloc(cinfo, size, "JPEG small internal data allocation");
}

void
jpeg_free_small(j_common_ptr cinfo, void *object, size_t size)
{
    jpeg_free(cinfo, object, "Freeing JPEG small internal data");
}

void FAR *
jpeg_get_large(j_common_ptr cinfo, size_t size)
{
    return jpeg_alloc(cinfo, size, "JPEG large internal data allocation");
}

void
jpeg_free_large(j_common_ptr cinfo, void FAR * object, size_t size)
{
    jpeg_free(cinfo, object, "Freeing JPEG large internal data");
}

long
jpeg_mem_available(j_common_ptr cinfo, long min_bytes_needed,
		   long max_bytes_needed, long already_allocated)
{
    return max_bytes_needed;
}

void
jpeg_open_backing_store(j_common_ptr cinfo, backing_store_ptr info,
			long total_bytes_needed)
{
    ERREXIT(cinfo, JERR_NO_BACKING_STORE);
}

long
jpeg_mem_init(j_common_ptr cinfo)
{
    return 0;			/* just set max_memory_to_use to 0 */
}

void
jpeg_mem_term(j_common_ptr cinfo)
{
    /* no work */
}
