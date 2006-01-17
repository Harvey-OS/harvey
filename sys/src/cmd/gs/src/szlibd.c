/* Copyright (C) 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: szlibd.c,v 1.7 2004/03/11 14:50:50 igor Exp $ */
/* zlib decoding (decompression) filter stream */
#include "memory_.h"
#include "std.h"
#include "gsmemory.h"
#include "gsmalloc.h"	    /* for gs_memory_default */
#include "strimpl.h"
#include "szlibxx.h"

/* Initialize the filter. */
private int
s_zlibD_init(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;
    int code = s_zlib_alloc_dynamic_state(ss);

    if (code < 0)
	return ERRC;	/****** WRONG ******/
    if (inflateInit2(&ss->dynamic->zstate,
		     (ss->no_wrapper ? -ss->windowBits : ss->windowBits))
	!= Z_OK
	) {
	s_zlib_free_dynamic_state(ss);
	return ERRC;	/****** WRONG ******/
    }
    st->min_left=1;
    return 0;
}

/* Reinitialize the filter. */
private int
s_zlibD_reset(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;

    if (inflateReset(&ss->dynamic->zstate) != Z_OK)
	return ERRC;	/****** WRONG ******/
    return 0;
}

/* Process a buffer */
private int
s_zlibD_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool ignore_last)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;
    z_stream *zs = &ss->dynamic->zstate;
    const byte *p = pr->ptr;
    int status;
    static const unsigned char jaws_empty[] = {0x58, 0x85, 1, 0, 0, 0, 0, 0, 1, 0x0A};

    /* Detect no input or full output so that we don't get */
    /* a Z_BUF_ERROR return. */
    if (pw->ptr == pw->limit)
	return 1;
    if (pr->ptr == pr->limit)
	return 0;
    zs->next_in = (Bytef *)p + 1;
    zs->avail_in = pr->limit - p;
    zs->next_out = pw->ptr + 1;
    zs->avail_out = pw->limit - pw->ptr;
    if (zs->total_in == 0 && zs->avail_in >= 10 && !memcmp(zs->next_in, jaws_empty, 10)) {
        /* JAWS PDF generator encodes empty stream as jaws_empty[].
         * The stream declares that the data block length is zero
         * but zlib routines regard a zero length data block to be an error. 
         */
        pr->ptr += 10;
        return EOFC;
    }
    status = inflate(zs, Z_PARTIAL_FLUSH);
    pr->ptr = zs->next_in - 1;
    pw->ptr = zs->next_out - 1;
    switch (status) {
	case Z_OK:
	    return (pw->ptr == pw->limit ? 1 : pr->ptr > p ? 0 : 1);
	case Z_STREAM_END:
	    return EOFC;
	default:
	    return ERRC;
    }
}

/* Release the stream */
private void
s_zlibD_release(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;

    inflateEnd(&ss->dynamic->zstate);
    s_zlib_free_dynamic_state(ss);
}

/* Stream template */
const stream_template s_zlibD_template = {
    &st_zlib_state, s_zlibD_init, s_zlibD_process, 1, 1, s_zlibD_release,
    s_zlib_set_defaults, s_zlibD_reset
};
