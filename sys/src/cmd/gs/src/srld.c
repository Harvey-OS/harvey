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

/* $Id: srld.c,v 1.4 2002/02/21 22:24:54 giles Exp $ */
/* RunLengthDecode filter */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "strimpl.h"
#include "srlx.h"

/* ------ RunLengthDecode ------ */

private_st_RLD_state();

/* Set defaults */
private void
s_RLD_set_defaults(stream_state * st)
{
    stream_RLD_state *const ss = (stream_RLD_state *) st;

    s_RLD_set_defaults_inline(ss);
}

/* Initialize */
private int
s_RLD_init(stream_state * st)
{
    stream_RLD_state *const ss = (stream_RLD_state *) st;

    return s_RLD_init_inline(ss);
}

/* Refill the buffer */
private int
s_RLD_process(stream_state * st, stream_cursor_read * pr,
	      stream_cursor_write * pw, bool last)
{
    stream_RLD_state *const ss = (stream_RLD_state *) st;
    register const byte *p = pr->ptr;
    register byte *q = pw->ptr;
    const byte *rlimit = pr->limit;
    byte *wlimit = pw->limit;
    int left;
    int status = 0;

top:
    if ((left = ss->copy_left) > 0) {
	/*
	 * We suspended because the output buffer was full:;
	 * try again now.
	 */
	uint avail = wlimit - q;
	int copy_status = 1;

	if (left > avail)
	    left = avail;
	if (ss->copy_data >= 0)
	    memset(q + 1, ss->copy_data, left);
	else {
	    avail = rlimit - p;
	    if (left >= avail) {
		copy_status = 0;
		left = avail;
	    }
	    memcpy(q + 1, p + 1, left);
	    p += left;
	}
	q += left;
	if ((ss->copy_left -= left) > 0) {
	    status = copy_status;
	    goto x;
	}
    }
    while (p < rlimit) {
	int b = *++p;

	if (b < 128) {
	    if (++b > rlimit - p || b > wlimit - q) {
		ss->copy_left = b;
		ss->copy_data = -1;
		goto top;
	    }
	    memcpy(q + 1, p + 1, b);
	    p += b;
	    q += b;
	} else if (b == 128) {	/* end of data */
	    if (ss->EndOfData) {
		status = EOFC;
		break;
	    }
	} else if (p == rlimit) {
	    p--;
	    break;
	} else if ((b = 257 - b) > wlimit - q) {
	    ss->copy_left = b;
	    ss->copy_data = *++p;
	    goto top;
	} else {
	    memset(q + 1, *++p, b);
	    q += b;
	}
    }
x:  pr->ptr = p;
    pw->ptr = q;
    return status;
}

/* Stream template */
const stream_template s_RLD_template = {
    &st_RLD_state, s_RLD_init, s_RLD_process, 1, 1, NULL,
    s_RLD_set_defaults
};
