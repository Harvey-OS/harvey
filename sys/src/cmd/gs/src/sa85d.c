/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: sa85d.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
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
private int a85d_finish(P3(int, ulong, stream_cursor_write *));
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
	    if (wlimit - q < 4) {
		p--;
		status = 1;
		break;
	    }
	    word = word * 85 + ccode;
	    if (++ccount == 5) {
		q[1] = (byte) (word >> 24);
		q[2] = (byte) (word >> 16);
		q[3] = (byte) ((uint) word >> 8);
		q[4] = (byte) word;
		q += 4;
		word = 0;
		ccount = 0;
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
	    if (*++p != '>') {
		status = ERRC;
		break;
	    }
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
	    word = word * (85L * 85 * 85) + 0xffffffL;
	    goto o1;
	case 3:		/* 2 odd bytes */
	    word = word * (85L * 85) + 0xffffL;
	    goto o2;
	case 4:		/* 3 odd bytes */
	    word = word * 85 + 0xffL;
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
