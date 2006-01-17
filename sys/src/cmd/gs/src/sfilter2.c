/* Copyright (C) 1991, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sfilter2.c,v 1.5 2002/02/21 22:24:54 giles Exp $ */
/* Simple Level 2 filters */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "gdebug.h"
#include "strimpl.h"
#include "sa85x.h"
#include "sbtx.h"
#include "scanchar.h"

/* ------ ASCII85Encode ------ */

private_st_A85E_state();

/* Initialize the state */
private int
s_A85E_init(stream_state * st)
{
    stream_A85E_state *const ss = (stream_A85E_state *) st;

    return s_A85E_init_inline(ss);
}

/* Process a buffer */
#define LINE_LIMIT 79		/* not 80, to satisfy Genoa FTS */
private int
s_A85E_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    stream_A85E_state *const ss = (stream_A85E_state *) st;
    register const byte *p = pr->ptr;
    register byte *q = pw->ptr;
    byte *qn = q + (LINE_LIMIT - ss->count); /* value of q before next EOL */
    const byte *rlimit = pr->limit;
    byte *wlimit = pw->limit;
    int status = 0;
    int prev = ss->last_char;
    int count;

    if_debug3('w', "[w85]initial ss->count = %d, rcount = %d, wcount = %d\n",
	      ss->count, (int)(rlimit - p), (int)(wlimit - q));
    for (; (count = rlimit - p) >= 4; p += 4) {
	ulong word =
	    ((ulong) (((uint) p[1] << 8) + p[2]) << 16) +
	    (((uint) p[3] << 8) + p[4]);

	if (word == 0) {
	    if (q >= qn) {
		if (wlimit - q < 2) {
		    status = 1;
		    break;
		}
		*++q = prev = '\n';
		qn = q + LINE_LIMIT;
		if_debug1('w', "[w85]EOL at %d bytes written\n",
			  (int)(q - pw->ptr));
	    } else {
		if (q >= wlimit) {
		    status = 1;
		    break;
		}
	    }
	    *++q = prev = 'z';
	} else {
	    ulong v4 = word / 85;	/* max 85^4 */
	    ulong v3 = v4 / 85;	/* max 85^3 */
	    uint v2 = v3 / 85;	/* max 85^2 */
	    uint v1 = v2 / 85;	/* max 85 */

put:	    if (q + 5 > qn) {
		if (q >= wlimit) {
		    status = 1;
		    break;
		}
		*++q = prev = '\n';
		qn = q + LINE_LIMIT;
		if_debug1('w', "[w85]EOL at %d bytes written\n",
			  (int)(q - pw->ptr));
		goto put;
	    }
	    if (wlimit - q < 5) {
		status = 1;
		break;
	    }
	    q[1] = (byte) v1 + '!';
	    q[2] = (byte) (v2 - v1 * 85) + '!';
	    q[3] = (byte) ((uint) v3 - v2 * 85) + '!';
	    q[4] = (byte) ((uint) v4 - (uint) v3 * 85) + '!';
	    q[5] = (byte) ((uint) word - (uint) v4 * 85) + '!';
	    /*
	     * '%%' or '%!' at the beginning of the line will confuse some
	     * document managers: insert (an) EOL(s) if necessary to prevent
	     * this.
	     */
	    if (q[1] == '%') {
		if (prev == '%') {
		    if (qn - q == LINE_LIMIT - 1) {
			/* A line would begin with %%. */
			*++q = prev = '\n';
			qn = q + LINE_LIMIT;
			if_debug1('w',
				  "[w85]EOL for %%%% at %d bytes written\n",
				  (int)(q - pw->ptr));
			goto put;
		    }
		} else if (prev == '\n' && (q[2] == '%' || q[2] == '!')) {
		    /*
		     * We may have to insert more than one EOL if
		     * there are more than two %s in a row.
		     */
		    int extra =
			(q[2] == '!' ? 1 : /* else q[2] == '%' */
			 q[3] == '!' ? 2 :
			 q[3] != '%' ? 1 :
			 q[4] == '!' ? 3 :
			 q[4] != '%' ? 2 :
			 q[5] == '!' ? 4 :
			 q[5] != '%' ? 3 : 4);

		    if (wlimit - q < 5 + extra) {
			status = 1;
			break;
		    }
		    if_debug6('w', "[w]%c%c%c%c%c extra = %d\n",
			      q[1], q[2], q[3], q[4], q[5], extra);
		    switch (extra) {
			case 4:
			    q[9] = q[5], q[8] = '\n';
			    goto e3;
			case 3:
			    q[8] = q[5];
			  e3:q[7] = q[4], q[6] = '\n';
			    goto e2;
			case 2:
			    q[7] = q[5], q[6] = q[4];
			  e2:q[5] = q[3], q[4] = '\n';
			    goto e1;
			case 1:
			    q[6] = q[5], q[5] = q[4], q[4] = q[3];
			  e1:q[3] = q[2], q[2] = '\n';
		    }
		    if_debug1('w', "[w85]EOL at %d bytes written\n",
			      (int)(q + 2 * extra - pw->ptr));
		    qn = q + 2 * extra + LINE_LIMIT;
		    q += extra;
		}
	    } else if (q[1] == '!' && prev == '%' &&
		       qn - q == LINE_LIMIT - 1
		       ) {
		/* A line would begin with %!. */
		*++q = prev = '\n';
		qn = q + LINE_LIMIT;
		if_debug1('w', "[w85]EOL for %%! at %d bytes written\n",
			  (int)(q - pw->ptr));
		goto put;
	    }
	    prev = *(q += 5);
	}
    }
 end:
    ss->count = LINE_LIMIT - (qn - q);
    /* Check for final partial word. */
    if (last && status == 0 && count < 4) {
	int nchars = (count == 0 ? 2 : count + 3);

	if (wlimit - q < nchars)
	    status = 1;
	else if (q + nchars > qn) {
	    *++q = '\n';
	    qn = q + LINE_LIMIT;
	    goto end;
	}
	else {
	    ulong word = 0;
	    ulong divisor = 85L * 85 * 85 * 85;

	    switch (count) {
		case 3:
		    word += (uint) p[3] << 8;
		case 2:
		    word += (ulong) p[2] << 16;
		case 1:
		    word += (ulong) p[1] << 24;
		    p += count;
		    while (count-- >= 0) {
			ulong v = word / divisor;  /* actually only a byte */

			*++q = (byte) v + '!';
			word -= v * divisor;
			divisor /= 85;
		    }
		    /*case 0: */
	    }
	    *++q = '~';
	    *++q = '>';
	}
    }
    if_debug3('w', "[w85]final ss->count = %d, %d bytes read, %d written\n",
	      ss->count, (int)(p - pr->ptr), (int)(q - pw->ptr));
    pr->ptr = p;
    if (q > pw->ptr)
	ss->last_char = *q;
    pw->ptr = q;
    return status;
}
#undef LINE_LIMIT

/* Stream template */
const stream_template s_A85E_template = {
    &st_A85E_state, s_A85E_init, s_A85E_process, 4, 6
};

/* ------ ByteTranslateEncode/Decode ------ */

private_st_BT_state();

/* Process a buffer.  Note that the same code serves for both streams. */
private int
s_BT_process(stream_state * st, stream_cursor_read * pr,
	     stream_cursor_write * pw, bool last)
{
    stream_BT_state *const ss = (stream_BT_state *) st;
    const byte *p = pr->ptr;
    byte *q = pw->ptr;
    uint rcount = pr->limit - p;
    uint wcount = pw->limit - q;
    uint count;
    int status;

    if (rcount <= wcount)
	count = rcount, status = 0;
    else
	count = wcount, status = 1;
    while (count--)
	*++q = ss->table[*++p];
    pr->ptr = p;
    pw->ptr = q;
    return status;
}

/* Stream template */
const stream_template s_BTE_template = {
    &st_BT_state, NULL, s_BT_process, 1, 1
};
const stream_template s_BTD_template = {
    &st_BT_state, NULL, s_BT_process, 1, 1
};
