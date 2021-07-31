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

/* sdcte.c */
/* DCT encoding filter stream */
#include "memory_.h"
#include "stdio_.h"
#include "jpeglib.h"
#include "jerror.h"
#include "gdebug.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"

#define ss ((stream_DCT_state *)st)

/* ------ DCTEncode ------ */

/* JPEG destination manager procedures */
private void
dcte_init_destination(j_compress_ptr cinfo)
{
}
private boolean
dcte_empty_output_buffer(j_compress_ptr cinfo)
{	return FALSE;
}
private void
dcte_term_destination(j_compress_ptr cinfo)
{
}

/* Initialize DCTEncode filter */
private int
s_DCTE_init(stream_state *st)
{	struct jpeg_destination_mgr *dest = &ss->data.compress->destination;
	dest->init_destination = dcte_init_destination;
	dest->empty_output_buffer = dcte_empty_output_buffer;
	dest->term_destination = dcte_term_destination;
	ss->data.compress->cinfo.dest = dest;
	ss->phase = 0;
	return 0;
}

/* Process a buffer */
private int
s_DCTE_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *pw, bool last)
{	jpeg_compress_data *jcdp = ss->data.compress;
	struct jpeg_destination_mgr *dest = jcdp->cinfo.dest;
	dest->next_output_byte = pw->ptr + 1;
	dest->free_in_buffer = pw->limit - pw->ptr;
	switch ( ss->phase )
	{
	case 0:				/* not initialized yet */
		if ( gs_jpeg_start_compress(ss, TRUE) < 0 )
			return ERRC;
		pw->ptr = dest->next_output_byte - 1;
		ss->phase = 1;
		/* falls through */
	case 1:				/* initialized, Markers not written */
		if ( pw->limit - pw->ptr < ss->Markers.size )
			return 1;
		memcpy(pw->ptr + 1, ss->Markers.data, ss->Markers.size);
		pw->ptr += ss->Markers.size;
		ss->phase = 2;
		/* falls through */
	case 2:				/* still need to write Adobe marker */
		if ( ! ss->NoMarker )
		{	static const byte Adobe[] = {
			  0xFF, JPEG_APP0+14, 0, 14, /* parameter length */
			  'A', 'd', 'o', 'b', 'e',
			  0, 100, /* Version */
			  0, 0,	/* Flags0 */
			  0, 0,	/* Flags1 */
			  0	/* ColorTransform */
			};
#define ADOBE_MARKER_LEN sizeof(Adobe)
			if ( pw->limit - pw->ptr < ADOBE_MARKER_LEN )
				return 1;
			memcpy(pw->ptr + 1, Adobe, ADOBE_MARKER_LEN);
			pw->ptr += ADOBE_MARKER_LEN;
			*pw->ptr = ss->ColorTransform;
#undef ADOBE_MARKER_LEN
		}
		dest->next_output_byte = pw->ptr + 1;
		dest->free_in_buffer = pw->limit - pw->ptr;
		ss->phase = 3;
		/* falls through */
	case 3:				/* markers written, processing data */
		while ( jcdp->cinfo.image_height > jcdp->cinfo.next_scanline )
		{	int written;
			/*const*/ byte *samples = (byte *)(pr->ptr + 1);
			if ( (uint)(pr->limit - pr->ptr) < ss->scan_line_size )
				return 0;		/* need more data */
			written = gs_jpeg_write_scanlines(ss, &samples, 1);
			if ( written < 0 )
				return ERRC;
			pw->ptr = dest->next_output_byte - 1;
			if ( !written )
				return 1;		/* output full */
			pr->ptr += ss->scan_line_size;
		}
		ss->phase = 4;
		/* falls through */
	case 4:				/* all data processed, finishing */
		/* jpeg_finish_compress can't suspend, so make sure
		 * it has plenty of room to write the last few bytes.
		 */
		if ( pw->limit - pw->ptr < 100 )
			return 1;
		if ( gs_jpeg_finish_compress(ss) < 0 )
			return ERRC;
		pw->ptr = dest->next_output_byte - 1;
		ss->phase = 5;
		/* falls through */
	case 5:				/* we are DONE */
		return EOFC;
	}
	/* Default case can't happen.... */
	return ERRC;
}

/* Release the stream */
private void
s_DCTE_release(stream_state *st)
{	gs_jpeg_destroy(ss);
	gs_free(ss->data.compress, 1, sizeof(jpeg_compress_data),
		"s_DCTE_release");
	/* Switch the template pointer back in case we still need it. */
	st->template = &s_DCTE_template;
}

/* Stream template */
const stream_template s_DCTE_template =
{	&st_DCT_state, s_DCTE_init, s_DCTE_process, 1000, 4000, s_DCTE_release
};

#undef ss
