/* Copyright (C) 1994, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: sjpege.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Interface routines for IJG encoding code. */
#include "stdio_.h"
#include "string_.h"
#include "jpeglib_.h"
#include "jerror_.h"
#include "gx.h"
#include "gserrors.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"

/*
 * Interface routines.  This layer of routines exists solely to limit
 * side-effects from using setjmp.
 */

int
gs_jpeg_create_compress(stream_DCT_state * st)
{				/* Initialize error handling */
    gs_jpeg_error_setup(st);
    /* Establish the setjmp return context for gs_jpeg_error_exit to use. */
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));

    jpeg_create_compress(&st->data.compress->cinfo);
    jpeg_stream_data_common_init(st->data.compress);
    return 0;
}

int
gs_jpeg_set_defaults(stream_DCT_state * st)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_set_defaults(&st->data.compress->cinfo);
    return 0;
}

int
gs_jpeg_set_colorspace(stream_DCT_state * st,
		       J_COLOR_SPACE colorspace)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_set_colorspace(&st->data.compress->cinfo, colorspace);
    return 0;
}

int
gs_jpeg_set_linear_quality(stream_DCT_state * st,
			   int scale_factor, boolean force_baseline)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_set_linear_quality(&st->data.compress->cinfo,
			    scale_factor, force_baseline);
    return 0;
}

int
gs_jpeg_set_quality(stream_DCT_state * st,
		    int quality, boolean force_baseline)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_set_quality(&st->data.compress->cinfo,
		     quality, force_baseline);
    return 0;
}

int
gs_jpeg_start_compress(stream_DCT_state * st,
		       boolean write_all_tables)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_start_compress(&st->data.compress->cinfo, write_all_tables);
    return 0;
}

int
gs_jpeg_write_scanlines(stream_DCT_state * st,
			JSAMPARRAY scanlines,
			int num_lines)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    return (int)jpeg_write_scanlines(&st->data.compress->cinfo,
				     scanlines, (JDIMENSION) num_lines);
}

int
gs_jpeg_finish_compress(stream_DCT_state * st)
{
    if (setjmp(st->data.common->exit_jmpbuf))
	return_error(gs_jpeg_log_error(st));
    jpeg_finish_compress(&st->data.compress->cinfo);
    return 0;
}
