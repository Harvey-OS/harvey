/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* sdctd.c */
/* DCT decoding filter stream */
#include "memory_.h"
#include "stdio_.h"
#include "jpeglib.h"
#include "jerror.h"
#include "gdebug.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"

#define ss ((stream_DCT_state *)st)

/* ------ DCTDecode ------ */

/* JPEG source manager procedures */
private void
dctd_init_source(j_decompress_ptr dinfo)
{
}
static const JOCTET fake_eoi[2] = { 0xFF , JPEG_EOI };
private boolean
dctd_fill_input_buffer(j_decompress_ptr dinfo)
{	jpeg_decompress_data *jddp =
	  (jpeg_decompress_data *)((char *)dinfo -
				   offset_of(jpeg_decompress_data, dinfo));
	if (! jddp->input_eod)
		return FALSE;	/* normal case: suspend processing */
	/* Reached end of source data without finding EOI */
	WARNMS(dinfo, JWRN_JPEG_EOF);
	/* Insert a fake EOI marker */
	dinfo->src->next_input_byte = fake_eoi;
	dinfo->src->bytes_in_buffer = 2;
	jddp->faked_eoi = true;	/* so process routine doesn't use next_input_byte */
	return TRUE;
}
private void
dctd_skip_input_data(j_decompress_ptr dinfo, long num_bytes)
{	struct jpeg_source_mgr *src = dinfo->src;
	jpeg_decompress_data *jddp =
	  (jpeg_decompress_data *)((char *)dinfo -
				   offset_of(jpeg_decompress_data, dinfo));
	if ( num_bytes > 0 )
	{	if ( num_bytes > src->bytes_in_buffer )
		{	jddp->skip += num_bytes - src->bytes_in_buffer;
			src->next_input_byte += src->bytes_in_buffer;
			src->bytes_in_buffer = 0;
			return;
		}
		src->next_input_byte += num_bytes;
		src->bytes_in_buffer -= num_bytes;
	}
}
private void
dctd_term_source(j_decompress_ptr dinfo)
{
}

/* Initialize DCTDecode filter */
private int
s_DCTD_init(stream_state *st)
{	struct jpeg_source_mgr *src = &ss->data.decompress->source;
	src->init_source = dctd_init_source;
	src->fill_input_buffer = dctd_fill_input_buffer;
	src->skip_input_data = dctd_skip_input_data;
	src->term_source = dctd_term_source;
	src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	ss->data.decompress->dinfo.src = src;
	ss->data.decompress->skip = 0;
	ss->data.decompress->input_eod = false;
	ss->data.decompress->faked_eoi = false;
	ss->phase = 0;
	return 0;
}

/* Process a buffer */
private int
s_DCTD_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	jpeg_decompress_data *jddp = ss->data.decompress;
	struct jpeg_source_mgr *src = jddp->dinfo.src;
	int code;
	if ( jddp->skip != 0 )
	{	long avail = pr->limit - pr->ptr;
		if ( avail < jddp->skip )
		{	jddp->skip -= avail;
			pr->ptr = pr->limit;
			if (! last)
				return 0;	/* need more data */
			jddp->skip = 0;		/* don't skip past input EOD */
		}
		pr->ptr += jddp->skip;
		jddp->skip = 0;
	}
	src->next_input_byte = pr->ptr + 1;
	src->bytes_in_buffer = pr->limit - pr->ptr;
	jddp->input_eod = last;
	switch ( ss->phase )
	{
	case 0:				/* not initialized yet */
		if ( (code = gs_jpeg_read_header(ss, TRUE)) < 0 )
			return ERRC;
		pr->ptr = jddp->faked_eoi ? pr->limit : src->next_input_byte-1;
		switch ( code )
		{
		case JPEG_SUSPENDED:
			return 0;
		/*case JPEG_HEADER_OK: */
		}
		/* If we have a ColorTransform parameter, and it's not
		 * overridden by an Adobe marker in the data, set colorspace.
		 */
		if ( ss->ColorTransform >= 0 &&
		    ! jddp->dinfo.saw_Adobe_marker )
		{	switch ( jddp->dinfo.num_components )
			{
			case 3:
				jddp->dinfo.jpeg_color_space =
				  ss->ColorTransform ? JCS_YCbCr : JCS_RGB;
				/* out_color_space will default to JCS_RGB */
				break;
			case 4:
				jddp->dinfo.jpeg_color_space =
				  ss->ColorTransform ? JCS_YCCK : JCS_CMYK;
				/* out_color_space will default to JCS_CMYK */
				break;
			}
		}
		if ( gs_jpeg_start_decompress(ss) < 0 )
			return ERRC;
		ss->scan_line_size = 
		  jddp->dinfo.output_width * jddp->dinfo.output_components;
		if ( ss->scan_line_size > (uint) jddp->template.min_out_size )
		{
			/* Create a spare buffer for oversize scanline */
			jddp->scanline_buffer = (byte *)
			  gs_malloc(ss->scan_line_size, sizeof(byte),
				    "s_DCTD_process(scanline_buffer)");
			if ( jddp->scanline_buffer == NULL )
				return ERRC;
		}
		jddp->bytes_in_scanline = 0;
		ss->phase = 1;
		/* falls through */
	case 1:				/* reading data */
	dumpbuffer:
		if ( jddp->bytes_in_scanline != 0 )
		{	uint avail = pw->limit - pw->ptr;
			uint tomove = min(jddp->bytes_in_scanline,
					  avail);
			memcpy(pw->ptr + 1, jddp->scanline_buffer +
			       (ss->scan_line_size - jddp->bytes_in_scanline),
			       tomove);
			pw->ptr += tomove;
			jddp->bytes_in_scanline -= tomove;
			if ( jddp->bytes_in_scanline != 0 )
				return 1;		/* need more room */
		}
		while ( jddp->dinfo.output_height > jddp->dinfo.output_scanline )
		{	int read;
			byte *samples;
			if ( jddp->scanline_buffer != NULL )
				samples = jddp->scanline_buffer;
			else
			{	if ( (uint)(pw->limit - pw->ptr) < ss->scan_line_size )
					return 1;	/* need more room */
				samples = (byte *)(pw->ptr + 1);
			}
			read = gs_jpeg_read_scanlines(ss, &samples, 1);
			if ( read < 0 )
				return ERRC;
			pr->ptr = jddp->faked_eoi ? pr->limit : src->next_input_byte-1;
			if ( !read )
				return 0;		/* need more data */
			if ( jddp->scanline_buffer != NULL )
			{	jddp->bytes_in_scanline = ss->scan_line_size;
				goto dumpbuffer;
			}
			pw->ptr += ss->scan_line_size;
		}
		ss->phase = 2;
		/* falls through */
	case 2:				/* end of image; scan for EOI */
		if ( (code = gs_jpeg_finish_decompress(ss)) < 0 )
			return ERRC;
		pr->ptr = jddp->faked_eoi ? pr->limit : src->next_input_byte-1;
		if ( code == 0 )
			return 0;
		ss->phase = 3;
		/* falls through */
	case 3:				/* we are DONE */
		return EOFC;
	}
	/* Default case can't happen.... */
	return ERRC;
}

/* Release the stream */
private void
s_DCTD_release(stream_state *st)
{	gs_jpeg_destroy(ss);
	if ( ss->data.decompress->scanline_buffer != NULL )
	{
		gs_free(ss->data.decompress->scanline_buffer,
			ss->scan_line_size, sizeof(byte),
			"s_DCTD_release(scanline_buffer)");
	}
	gs_free(ss->data.decompress, 1, sizeof(jpeg_decompress_data),
		"s_DCTD_release");
	/* Switch the template pointer back in case we still need it. */
	st->template = &s_DCTD_template;
}

/* Stream template */
const stream_template s_DCTD_template =
{	&st_DCT_state, s_DCTD_init, s_DCTD_process, 2000, 4000, s_DCTD_release
};

#undef ss
