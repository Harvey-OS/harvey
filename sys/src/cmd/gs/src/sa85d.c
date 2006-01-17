/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sa85d.c,v 1.9 2003/04/24 19:42:13 ray Exp $ */
/* ASCII85Decode filter */
#include "std.h"
#include "strimpl.h"
#include "sa85d.h"
#include "scanchar.h"

/* ------ ASCII85Decode ------ */

private_st_A85D_state();

/* Initialize the state */
private int
s_A85D_init(stream_state * st)
{
    stream_A85D_state *const ss = (stream_A85D_state *) st;

    return s_A85D_init_inline(ss);
}

/* Process a buffer */
private int a85d_finish(int, ulong, stream_cursor_write *);
private int
s_A85D_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    stream_A85D_state *const ss = (stream_A85D_state *) st;
    register const byte *p = pr->ptr;
    register byte *q = pw->ptr;
    const byte *rlimit = pr->limit;
    byte *wlimit = pw->limit;
    int ccount = ss->odd;
    ulong word = ss->word;
    int status = 0;

    while (p < rlimit) {
	int ch = *++p;
	uint ccode = ch - '!';

	if (ccode < 85) {	/* catches ch < '!' as well */
	    if (ccount == 4) {
		/*
		 * We've completed a 32-bit group.  Make sure we have
		 * room for it in the output.
		 */
		if (wlimit - q < 4) {
		    p--;
		    status = 1;
		    break;
		}
		/* Check for overflow condition, throw ioerror if so */
		if (word >= 0x03030303 && ccode > 0) {
		    status = ERRC;
	            break;
	        }
		word = word * 85 + ccode;
		q[1] = (byte) (word >> 24);
		q[2] = (byte) (word >> 16);
		q[3] = (byte) ((uint) word >> 8);
		q[4] = (byte) word;
		q += 4;
		word = 0;
		ccount = 0;
	    } else {
		word = word * 85 + ccode;
		++ccount;
	    }
	} else if (ch == 'z' && ccount == 0) {
	    if (wlimit - q < 4) {
		p--;
		status = 1;
		break;
	    }
	    q[1] = q[2] = q[3] = q[4] = 0,
		q += 4;
	} else if (scan_char_decoder[ch] == ctype_space)
	    DO_NOTHING;
	else if (ch == '~') {
	    int i = 1;

	    /* Handle odd bytes. */
	    if (p == rlimit) {
		if (last)
		    status = ERRC;
		else
		    p--;
		break;
	    }
	    if ((int)(wlimit - q) < ccount - 1) {
		status = 1;
		p--;
		break;
	    }

	    /* According to PLRM 3rd, if the A85 filter encounters '~',
	     * the next character must be '>'.
	     * And any other characters should raise an ioerror.
	     * But Adobe Acrobat allows CR/LF between ~ and >.
	     * So we allow CR/LF between them. */
	    while ((p[i] == 13 || p[i] == 10) && (p+i <= rlimit)) 
		i++;
	    if (p[i] != '>') {
		if (p+i == rlimit) {
		    if (last)
			status = ERRC;
		    else
			p--;	/* we'll see the '~' after filling the buffer */
		}
		break;
	    }
	    p += i;		/* advance to the '>' */
	    pw->ptr = q;
	    status = a85d_finish(ccount, word, pw);
	    q = pw->ptr;
	    break;
	} else {		/* syntax error or exception */
	    status = ERRC;
	    break;
	}
    }
    pw->ptr = q;
    if (status == 0 && last) {
	if ((int)(wlimit - q) < ccount - 1)
	    status = 1;
	else
	    status = a85d_finish(ccount, word, pw);
    }
    pr->ptr = p;
    ss->odd = ccount;
    ss->word = word;
    return status;
}
/* Handle the end of input data. */
private int
a85d_finish(int ccount, ulong word, stream_cursor_write * pw)
{
    /* Assume there is enough room in the output buffer! */
    byte *q = pw->ptr;
    int status = EOFC;

    switch (ccount) {
	case 0:
	    break;
	case 1:		/* syntax error */
	    status = ERRC;
	    break;
	case 2:		/* 1 odd byte */
	    word = word * (85L * 85 * 85) + 85L * 85 * 85 - 1L;
	    goto o1;
	case 3:		/* 2 odd bytes */
	    word = word * (85L * 85) + 85L * 85L - 1L;
	    goto o2;
	case 4:		/* 3 odd bytes */
	    word = word * 85L + 84L;
	    q[3] = (byte) (word >> 8);
o2:	    q[2] = (byte) (word >> 16);
o1:	    q[1] = (byte) (word >> 24);
	    q += ccount - 1;
	    pw->ptr = q;
    }
    return status;
}

/* Stream template */
const stream_template s_A85D_template = {
    &st_A85D_state, s_A85D_init, s_A85D_process, 2, 4
};
