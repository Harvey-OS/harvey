/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: smd5.c,v 1.6 2004/01/13 14:03:30 igor Exp $ */
/* MD5Encode filter */
#include "memory_.h"
#include "strimpl.h"
#include "stream.h"
#include "smd5.h"

/* ------ MD5Encode ------ */

private_st_MD5E_state();

/* Initialize the state. */
private int
s_MD5E_init(stream_state * st)
{
    stream_MD5E_state *const ss = (stream_MD5E_state *) st;

    md5_init(&ss->md5);
    return 0;
}

/* Process a buffer. */
private int
s_MD5E_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    stream_MD5E_state *const ss = (stream_MD5E_state *) st;
    int status = 0;

    if (pr->ptr < pr->limit) {
	md5_append(&ss->md5, pr->ptr + 1, pr->limit - pr->ptr);
	pr->ptr = pr->limit;
    }
    if (last) {
	if (pw->limit - pw->ptr >= 16) {
	    md5_finish(&ss->md5, pw->ptr + 1);
	    pw->ptr += 16;
	    status = EOFC;
	} else
	    status = 1;
    }
    return status;
}

/* Stream template */
const stream_template s_MD5E_template = {
    &st_MD5E_state, s_MD5E_init, s_MD5E_process, 1, 16
};

stream *
s_MD5E_make_stream(gs_memory_t *mem, byte *digest, int digest_size)
{
    stream *s = s_alloc(mem, "s_MD5E_make_stream");
    stream_state *ss = s_alloc_state(mem, s_MD5E_template.stype, "s_MD5E_make_stream");

    if (ss == NULL || s == NULL)
	goto err;
    ss->template = &s_MD5E_template;
    if (s_init_filter(s, ss, digest, digest_size, NULL) < 0)
	goto err;
    s->strm = s;
    return s;
err:
    gs_free_object(mem, ss, "s_MD5E_make_stream");
    gs_free_object(mem, s, "s_MD5E_make_stream");
    return NULL;
}
