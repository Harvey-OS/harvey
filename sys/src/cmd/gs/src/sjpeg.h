/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sjpeg.h,v 1.5 2002/06/16 05:00:54 lpd Exp $ */
/* IJG entry point wrappers */
/* Requires sdct.h, jpeg/jpeglib.h */

#ifndef sjpeg_INCLUDED
#  define sjpeg_INCLUDED

/*
 * Each routine gs_jpeg_xxx is equivalent to the IJG entry point jpeg_xxx,
 * except that
 *   (a) it takes a pointer to stream_DCT_state instead of just the IJG
 *       jpeg_(de)compress_data struct;
 *   (b) it catches any error exit from the IJG code and converts it into
 *       an error return value per Ghostscript custom.  A negative return
 *       value is an error code, except for gs_jpeg_alloc_xxx which return
 *       NULL (indicating e_VMerror).
 */

/* Common to encode/decode */

void gs_jpeg_error_setup(stream_DCT_state * st);
int gs_jpeg_log_error(stream_DCT_state * st);
JQUANT_TBL *gs_jpeg_alloc_quant_table(stream_DCT_state * st);
JHUFF_TBL *gs_jpeg_alloc_huff_table(stream_DCT_state * st);
int gs_jpeg_destroy(stream_DCT_state * st);

/* Encode */

int gs_jpeg_create_compress(stream_DCT_state * st);
int gs_jpeg_set_defaults(stream_DCT_state * st);
int gs_jpeg_set_colorspace(stream_DCT_state * st,
			   J_COLOR_SPACE colorspace);
int gs_jpeg_set_linear_quality(stream_DCT_state * st,
			       int scale_factor, boolean force_baseline);
int gs_jpeg_set_quality(stream_DCT_state * st,
			int quality, boolean force_baseline);
int gs_jpeg_start_compress(stream_DCT_state * st,
			   boolean write_all_tables);
int gs_jpeg_write_scanlines(stream_DCT_state * st,
			    JSAMPARRAY scanlines, int num_lines);
int gs_jpeg_finish_compress(stream_DCT_state * st);

/* Decode */

int gs_jpeg_create_decompress(stream_DCT_state * st);
int gs_jpeg_read_header(stream_DCT_state * st,
			boolean require_image);
int gs_jpeg_start_decompress(stream_DCT_state * st);
int gs_jpeg_read_scanlines(stream_DCT_state * st,
			   JSAMPARRAY scanlines, int max_lines);
int gs_jpeg_finish_decompress(stream_DCT_state * st);

#endif /* sjpeg_INCLUDED */
