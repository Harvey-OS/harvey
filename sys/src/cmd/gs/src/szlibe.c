/* Copyright (C) 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: szlibe.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* zlib encoding (compression) filter stream */
#include "std.h"
#include "gsmemory.h"
#include "gsmalloc.h"		/* for gs_memory_default */
#include "strimpl.h"
#include "szlibxx.h"

/* Initialize the filter. */
private int
s_zlibE_init(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;
    int code = s_zlib_alloc_dynamic_state(ss);

    if (code < 0)
	return ERRC;	/****** WRONG ******/
    if (deflateInit2(&ss->dynamic->zstate, ss->level, ss->method,
		     (ss->no_wrapper ? -ss->windowBits : ss->windowBits),
		     ss->memLevel, ss->strategy) != Z_OK)
	return ERRC;	/****** WRONG ******/
    return 0;
}

/* Reinitialize the filter. */
private int
s_zlibE_reset(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;

    if (deflateReset(&ss->dynamic->zstate) != Z_OK)
	return ERRC;	/****** WRONG ******/
    return 0;
}

/* Process a buffer */
private int
s_zlibE_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;
    z_stream *zs = &ss->dynamic->zstate;
    const byte *p = pr->ptr;
    int status;

    /* Detect no input or full output so that we don't get */
    /* a Z_BUF_ERROR return. */
    if (pw->ptr == pw->limit)
	return 1;
    if (p == pr->limit && !last)
	return 0;
    zs->next_in = (Bytef *)p + 1;
    zs->avail_in = pr->limit - p;
    zs->next_out = pw->ptr + 1;
    zs->avail_out = pw->limit - pw->ptr;
    status = deflate(zs, (last ? Z_FINISH : Z_NO_FLUSH));
    pr->ptr = zs->next_in - 1;
    pw->ptr = zs->next_out - 1;
    switch (status) {
	case Z_OK:
	    return (pw->ptr == pw->limit ? 1 : pr->ptr > p && !last ? 0 : 1);
	case Z_STREAM_END:
	    return (last && pr->ptr == pr->limit ? 0 : ERRC);
	default:
	    return ERRC;
    }
}

/* Release the stream */
private void
s_zlibE_release(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;

    deflateEnd(&ss->dynamic->zstate);
    s_zlib_free_dynamic_state(ss);
}

/* Stream template */
const stream_template s_zlibE_template = {
    &st_zlib_state, s_zlibE_init, s_zlibE_process, 1, 1, s_zlibE_release,
    s_zlib_set_defaults, s_zlibE_reset
};
