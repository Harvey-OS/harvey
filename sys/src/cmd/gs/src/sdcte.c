/* Copyright (C) 1994, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sdcte.c,v 1.7 2002/02/21 22:24:53 giles Exp $ */
/* DCT encoding filter stream */
#include "memory_.h"
#include "stdio_.h"
#include "jpeglib_.h"
#include "jerror_.h"
#include "gdebug.h"
#include "gsmemory.h"		/* for gsmalloc.h */
#include "gsmalloc.h"		/* for gs_memory_default */
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"

public_st_jpeg_compress_data();

/* ------ DCTEncode ------ */

/* JPEG destination manager procedures */
private void
dcte_init_destination(j_compress_ptr cinfo)
{
}
private boolean
dcte_empty_output_buffer(j_compress_ptr cinfo)
{
    return FALSE;
}
private void
dcte_term_destination(j_compress_ptr cinfo)
{
}

/* Set the defaults for the DCTEncode filter. */
private void
s_DCTE_set_defaults(stream_state * st)
{
    stream_DCT_state *const ss = (stream_DCT_state *) st;

    s_DCT_set_defaults(st);
    ss->QFactor = 1.0;
    ss->ColorTransform = -1;
    ss->Markers.data = 0;
    ss->Markers.size = 0;
    ss->NoMarker = true;
}

/* Initialize DCTEncode filter */
private int
s_DCTE_init(stream_state * st)
{
    stream_DCT_state *const ss = (stream_DCT_state *) st;
    struct jpeg_destination_mgr *dest = &ss->data.compress->destination;

    dest->init_destination = dcte_init_destination;
    dest->empty_output_buffer = dcte_empty_output_buffer;
    dest->term_destination = dcte_term_destination;
    ss->data.common->memory = ss->jpeg_memory;
    ss->data.compress->cinfo.dest = dest;
    ss->phase = 0;
    return 0;
}

/* Process a buffer */
private int
s_DCTE_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    stream_DCT_state *const ss = (stream_DCT_state *) st;
    jpeg_compress_data *jcdp = ss->data.compress;
    struct jpeg_destination_mgr *dest = jcdp->cinfo.dest;

    if_debug2('w', "[wde]process avail=%u, last=%d\n",
	      (uint) (pr->limit - pr->ptr), last);
    dest->next_output_byte = pw->ptr + 1;
    dest->free_in_buffer = pw->limit - pw->ptr;
    switch (ss->phase) {
	case 0:		/* not initialized yet */
	    if (gs_jpeg_start_compress(ss, TRUE) < 0)
		return ERRC;
	    if_debug4('w', "[wde]width=%u, height=%u, components=%d, scan_line_size=%u\n",
		      jcdp->cinfo.image_width,
		      jcdp->cinfo.image_height,
		      jcdp->cinfo.input_components,
		      ss->scan_line_size);
	    pw->ptr = dest->next_output_byte - 1;
	    ss->phase = 1;
	    /* falls through */
	case 1:		/* initialized, Markers not written */
	    if (pw->limit - pw->ptr < ss->Markers.size)
		return 1;
	    memcpy(pw->ptr + 1, ss->Markers.data, ss->Markers.size);
	    pw->ptr += ss->Markers.size;
	    ss->phase = 2;
	    /* falls through */
	case 2:		/* still need to write Adobe marker */
	    if (!ss->NoMarker) {
		static const byte Adobe[] =
		{
		    0xFF, JPEG_APP0 + 14, 0, 14,	/* parameter length */
		    'A', 'd', 'o', 'b', 'e',
		    0, 100,	/* Version */
		    0, 0,	/* Flags0 */
		    0, 0,	/* Flags1 */
		    0		/* ColorTransform */
		};

#define ADOBE_MARKER_LEN sizeof(Adobe)
		if (pw->limit - pw->ptr < ADOBE_MARKER_LEN)
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
	case 3:		/* markers written, processing data */
	    while (jcdp->cinfo.image_height > jcdp->cinfo.next_scanline) {
		int written;

		/*
		 * The data argument for jpeg_write_scanlines is
		 * declared as a JSAMPARRAY.  There is no corresponding
		 * const type, so we must remove const from the
		 * argument that we are passing here.  (Tom Lane of IJG
		 * judges that providing const analogues of the
		 * interface types wouldn't be worth the trouble.)
		 */
		/*const */ byte *samples = (byte *) (pr->ptr + 1);

		if_debug1('w', "[wde]next_scanline=%u\n",
			  jcdp->cinfo.next_scanline);
		if ((uint) (pr->limit - pr->ptr) < ss->scan_line_size) {
		    if (last)
			return ERRC;	/* premature EOD */
		    return 0;	/* need more data */
		}
		written = gs_jpeg_write_scanlines(ss, &samples, 1);
		if (written < 0)
		    return ERRC;
		if_debug3('w', "[wde]write returns %d, used=%u, written=%u\n",
			  written,
			  (uint) (samples - 1 - pr->ptr),
			  (uint) (dest->next_output_byte - 1 - pw->ptr));
		pw->ptr = dest->next_output_byte - 1;
		if (!written)
		    return 1;	/* output full */
		pr->ptr += ss->scan_line_size;
	    }
	    ss->phase = 4;
	    /* falls through */
	case 4:		/* all data processed, finishing */
	    /* jpeg_finish_compress can't suspend, so write its output
	     * to a fixed-size internal buffer.
	     */
	    dest->next_output_byte = jcdp->finish_compress_buf;
	    dest->free_in_buffer = sizeof(jcdp->finish_compress_buf);
	    if (gs_jpeg_finish_compress(ss) < 0)
		return ERRC;
	    jcdp->fcb_size =
		dest->next_output_byte - jcdp->finish_compress_buf;
	    jcdp->fcb_pos = 0;
	    ss->phase = 5;
	    /* falls through */
	case 5:		/* copy the final data to the output */
	    if (jcdp->fcb_pos < jcdp->fcb_size) {
		int count = min(jcdp->fcb_size - jcdp->fcb_pos,
				pw->limit - pw->ptr);

		if_debug1('w', "[wde]copying final %d\n", count);
		memcpy(pw->ptr + 1, jcdp->finish_compress_buf + jcdp->fcb_pos,
		       count);
		jcdp->fcb_pos += count;
		pw->ptr += count;
		if (jcdp->fcb_pos < jcdp->fcb_size)
		    return 1;
	    }
	    return EOFC;
    }
    /* Default case can't happen.... */
    return ERRC;
}

/* Release the stream */
private void
s_DCTE_release(stream_state * st)
{
    stream_DCT_state *const ss = (stream_DCT_state *) st;

    gs_jpeg_destroy(ss);
    gs_free_object(ss->data.common->memory, ss->data.compress,
		   "s_DCTE_release");
    /* Switch the template pointer back in case we still need it. */
    st->template = &s_DCTE_template;
}

/* Stream template */
const stream_template s_DCTE_template =
{&st_DCT_state, s_DCTE_init, s_DCTE_process, 1000, 4000, s_DCTE_release,
 s_DCTE_set_defaults
};
