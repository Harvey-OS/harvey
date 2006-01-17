/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sbcp.c,v 1.5 2002/02/21 22:24:53 giles Exp $ */
/* BCP and TBCP filters */
#include "stdio_.h"
#include "strimpl.h"
#include "sbcp.h"

#define CtrlA 0x01
#define CtrlC 0x03
#define CtrlD 0x04
#define CtrlE 0x05
#define CtrlQ 0x11
#define CtrlS 0x13
#define CtrlT 0x14
#define ESC 0x1b
#define CtrlBksl 0x1c

/* The following is not used yet. */
/*private const char *TBCP_end_protocol_string = "\033%-12345X"; */

/* ------ BCPEncode and TBCPEncode ------ */

/* Process a buffer */
private int
s_xBCPE_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last, const byte * escaped)
{
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    uint rcount = rlimit - p;
    byte *q = pw->ptr;
    uint wcount = pw->limit - q;
    const byte *end = p + min(rcount, wcount);

    while (p < end) {
	byte ch = *++p;

	if (ch <= 31 && escaped[ch]) {
	    if (p == rlimit) {
		p--;
		break;
	    }
	    *++q = CtrlA;
	    ch ^= 0x40;
	    if (--wcount < rcount)
		end--;
	}
	*++q = ch;
    }
    pr->ptr = p;
    pw->ptr = q;
    return (p == rlimit ? 0 : 1);
}

/* Actual process procedures */
private int
s_BCPE_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    static const byte escaped[32] =
    {
	0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
    };

    return s_xBCPE_process(st, pr, pw, last, escaped);
}
private int
s_TBCPE_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last)
{
    static const byte escaped[32] =
    {
	0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0
    };

    return s_xBCPE_process(st, pr, pw, last, escaped);
}

/* Stream templates */
const stream_template s_BCPE_template =
{&st_stream_state, NULL, s_BCPE_process, 1, 2
};
const stream_template s_TBCPE_template =
{&st_stream_state, NULL, s_TBCPE_process, 1, 2
};

/* ------ BCPDecode and TBCPDecode ------ */

private_st_BCPD_state();

/* Initialize the state */
private int
s_BCPD_init(stream_state * st)
{
    stream_BCPD_state *const ss = (stream_BCPD_state *) st;

    ss->escaped = 0;
    ss->matched = ss->copy_count = 0;
    return 0;
}

/* Process a buffer */
private int
s_xBCPD_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last, bool tagged)
{
    stream_BCPD_state *const ss = (stream_BCPD_state *) st;
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    int copy_count = ss->copy_count;
    int status;
    bool escaped = ss->escaped;

    for (;;) {
	byte ch;

	if (copy_count) {
	    if (q == wlimit) {
		status = (p < rlimit ? 1 : 0);
		break;
	    }
	    *++q = *++(ss->copy_ptr);
	    copy_count--;
	    continue;
	}
	if (p == rlimit) {
	    status = 0;
	    break;
	}
	ch = *++p;
	if (ch <= 31)
	    switch (ch) {
		case CtrlA:
		    if (escaped) {
			status = ERRC;
			goto out;
		    }
		    escaped = true;
		    continue;
		case CtrlC:
		    status = (*ss->signal_interrupt) (st);
		    if (status < 0)
			goto out;
		    continue;
		case CtrlD:
		    if (escaped) {
			status = ERRC;
			goto out;
		    }
		    status = EOFC;
		    goto out;
		case CtrlE:
		    continue;
		case CtrlQ:
		    continue;
		case CtrlS:
		    continue;
		case CtrlT:
		    status = (*ss->request_status) (st);
		    if (status < 0)
			goto out;
		    continue;
		case CtrlBksl:
		    continue;
	    }
	if (q == wlimit) {
	    p--;
	    status = 1;
	    break;
	}
	if (escaped) {
	    escaped = false;
	    switch (ch) {
		case '[':
		    if (!tagged) {
			status = ERRC;
			goto out;
		    }
		    /* falls through */
		case 'A':
		case 'C':
		case 'D':
		case 'E':
		case 'Q':
		case 'S':
		case 'T':
		case '\\':
		    ch ^= 0x40;
		    break;
		case 'M':
		    if (!tagged) {
			status = ERRC;
			goto out;
		    }
		    continue;
		default:
		    status = ERRC;
		    goto out;
	    }
	}
	*++q = ch;
    }
  out:ss->copy_count = copy_count;
    ss->escaped = escaped;
    pr->ptr = p;
    pw->ptr = q;
    return status;
}

/* Actual process procedures */
private int
s_BCPD_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    return s_xBCPD_process(st, pr, pw, last, false);
}
private int
s_TBCPD_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last)
{
    return s_xBCPD_process(st, pr, pw, last, true);
}

/* Stream templates */
const stream_template s_BCPD_template =
{&st_BCPD_state, s_BCPD_init, s_BCPD_process, 1, 1,
 NULL, NULL, s_BCPD_init
};
const stream_template s_TBCPD_template =
{&st_BCPD_state, s_BCPD_init, s_TBCPD_process, 1, 1,
 NULL, NULL, s_BCPD_init
};
