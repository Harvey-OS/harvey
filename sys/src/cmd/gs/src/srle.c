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

/* $Id: srle.c,v 1.4 2002/02/21 22:24:54 giles Exp $ */
/* RunLengthEncode filter */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "strimpl.h"
#include "srlx.h"

/* ------ RunLengthEncode ------ */

private_st_RLE_state();

/* Set defaults */
private void
s_RLE_set_defaults(stream_state * st)
{
    stream_RLE_state *const ss = (stream_RLE_state *) st;

    s_RLE_set_defaults_inline(ss);
}

/* Initialize */
private int
s_RLE_init(stream_state * st)
{
    stream_RLE_state *const ss = (stream_RLE_state *) st;

    return s_RLE_init_inline(ss);
}

/* Process a buffer */
private int
s_RLE_process(stream_state * st, stream_cursor_read * pr,
	      stream_cursor_write * pw, bool last)
{
    stream_RLE_state *const ss = (stream_RLE_state *) st;
    register const byte *p = pr->ptr;
    register byte *q = pw->ptr;
    const byte *rlimit = pr->limit;
    byte *wlimit = pw->limit;
    int status = 0;
    ulong rleft = ss->record_left;

    /*
     * We thought that the Genoa CET demands that the output from this
     * filter be not just legal, but optimal, so we went to the trouble
     * of ensuring this.  It turned out that this wasn't actually
     * necessary, but we didn't want to change the code back.
     *
     * For optimal output, we can't just break runs at buffer
     * boundaries: unless we hit a record boundary or the end of the
     * input, we have to look ahead far enough to know that we aren't
     * breaking a run prematurely.
     */

    /* Check for leftover output. */
copy:
    if (ss->copy_left) {
	uint rcount = rlimit - p;
	uint wcount = wlimit - q;
	uint count = ss->copy_left;

	if (rcount < count)
	    count = rcount;
	if (wcount < count)
	    count = wcount;
	if (rleft < count)
	    count = rleft;
	memcpy(q + 1, p + 1, count);
	pr->ptr = p += count;
	pw->ptr = q += count;
	if ((ss->record_left = rleft -= count) == 0)
	    ss->record_left = rleft = ss->record_size;
	if ((ss->copy_left -= count) != 0)
	    return (rcount == 0 ? 0 : 1);
    }
    while (p < rlimit) {
	const byte *beg = p;
	const byte *p1;
	uint count = rlimit - p;
	bool end = last;
	byte next;

	if (count > rleft)
	    count = rleft, end = true;
	if (count > 128)
	    count = 128, end = true;
	p1 = p + count - 1;
	if (count < 3) {
	    if (!end || count == 0)
		break;		/* can't look ahead far enough */
	    if (count == 1) {
		if (wlimit - q < 2) {
		    status = 1;
		    break;
		}
		*++q = 0;
	    } else {		/* count == 2 */
		if (p[1] == p[2]) {
		    if (wlimit - q < 2) {
			status = 1;
			break;
		    }
		    *++q = 255;
		} else {
		    if (wlimit - q < 3) {
			status = 1;
			break;
		    }
		    *++q = 1;
		    *++q = p[1];
		}
	    }
	    *++q = p1[1];
	    p = p1 + 1;
	} else if ((next = p[1]) == p[2] && next == p[3]) {
	    if (wlimit - q < 2) {
		status = 1;
		break;
	    }
	    /* Recognize leading repeated byte */
	    do {
		p++;
	    }
	    while (p < p1 && p[2] == next);
	    if (p == p1 && !end) {
		p = beg;	/* need to look ahead further */
		break;
	    }
	    p++;
	    *++q = (byte) (257 - (p - beg));
	    *++q = next;
	} else {
	    p1--;
	    while (p < p1 && (p[2] != p[1] || p[3] != p[1]))
		p++;
	    if (p == p1) {
		if (!end) {
		    p = beg;	/* need to look ahead further */
		    break;
		}
		p += 2;
	    }
	    count = p - beg;
	    if (wlimit - q < count + 1) {
		p = beg;
		if (q >= wlimit) {
		    status = 1;
		    break;
		}
		/* Copy some now and some later. */
		*++q = count - 1;
		ss->copy_left = count;
		goto copy;
	    }
	    *++q = count - 1;
	    memcpy(q + 1, beg + 1, count);
	    q += count;
	}
	rleft -= p - beg;
	if (rleft == 0)
	    rleft = ss->record_size;
    }
    if (last && status == 0 && ss->EndOfData) {
	if (q < wlimit)
	    *++q = 128;
	else
	    status = 1;
    }
    pr->ptr = p;
    pw->ptr = q;
    ss->record_left = rleft;
    return status;
}

/* Stream template */
const stream_template s_RLE_template = {
    &st_RLE_state, s_RLE_init, s_RLE_process, 129, 2, NULL,
    s_RLE_set_defaults, s_RLE_init
};
