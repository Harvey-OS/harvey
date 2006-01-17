/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: spngp.c,v 1.4 2002/02/21 22:24:54 giles Exp $ */
/* PNG pixel prediction filters */
#include "memory_.h"
#include "strimpl.h"
#include "spngpx.h"

/* ------ PNGPredictorEncode/Decode ------ */

private_st_PNGP_state();

/* Define values for case dispatch. */
#define cNone 10
#define cSub 11
#define cUp 12
#define cAverage 13
#define cPaeth 14
#define cOptimum 15
#define cEncode -10
#define cDecode -4
private const byte pngp_case_needs_prev[] = {
    0, 0, 1, 1, 1, 1
};

/* Set defaults */
private void
s_PNGP_set_defaults(stream_state * st)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;

    s_PNGP_set_defaults_inline(ss);
}

/* Common (re)initialization. */
private int
s_PNGP_reinit(stream_state * st)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;

    if (ss->prev_row != 0)
	memset(ss->prev_row + ss->bpp, 0, ss->row_count);
    ss->row_left = 0;
    return 0;
}

/* Common initialization. */
private int
s_pngp_init(stream_state * st, bool need_prev)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;
    int bits_per_pixel = ss->Colors * ss->BitsPerComponent;
    long bits_per_row = (long)bits_per_pixel * ss->Columns;
    byte *prev_row = 0;

#if arch_sizeof_long > arch_sizeof_int
    if (bits_per_row > max_uint * 7L)
	return ERRC;	/****** WRONG ******/
#endif
    ss->row_count = (uint) ((bits_per_row + 7) >> 3);
    ss->end_mask = (1 << (-bits_per_row & 7)) - 1;
    ss->bpp = (bits_per_pixel + 7) >> 3;
    if (need_prev) {
	prev_row = gs_alloc_bytes(st->memory, ss->bpp + ss->row_count,
				  "PNGPredictor prev row");
	if (prev_row == 0)
	    return ERRC;	/****** WRONG ******/
	memset(prev_row, 0, ss->bpp);
    }
    ss->prev_row = prev_row;
    /* case_index is only preset for encoding */
    return s_PNGP_reinit(st);
}

/* Initialize PNGPredictorEncode filter. */
private int
s_PNGPE_init(stream_state * st)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;

    return s_pngp_init(st, pngp_case_needs_prev[ss->Predictor - cNone]);
}

/* Initialize PNGPredictorDecode filter. */
private int
s_PNGPD_init(stream_state * st)
{
    return s_pngp_init(st, true);
}

/* Release a PNGPredictor filter. */
private void
s_PNGP_release(stream_state *st)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;

    if (ss->prev_row)
	gs_free_object(st->memory, ss->prev_row, "PNGPredictor prev row");
}

/*
 * Process a partial buffer.  We pass in current and previous pointers
 * to both the current and preceding scan line.  Note that dprev is
 * p - bpp for encoding, q - bpp for decoding; similarly, the 'up' row
 * is the raw data for encoding, the filtered data for decoding.
 * Note also that the case_index cannot be cOptimum.
 */
private int
paeth_predictor(int a, int b, int c)
{
    /* The definitions of ac and bc are correct, not a typo. */
    int ac = b - c, bc = a - c, abcc = ac + bc;
    int pa = (ac < 0 ? -ac : ac), pb = (bc < 0 ? -bc : bc),
	pc = (abcc < 0 ? -abcc : abcc);

    return (pa <= pb && pa <= pc ? a : pb <= pc ? b : c);
}
private void
s_pngp_process(stream_state * st, stream_cursor_write * pw,
	       const byte * dprev, stream_cursor_read * pr,
	       const byte * upprev, const byte * up, uint count)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;
    byte *q = pw->ptr + 1;
    const byte *p = pr->ptr + 1;

    pr->ptr += count;
    pw->ptr += count;
    ss->row_left -= count;
    switch (ss->case_index) {
	case cEncode + cNone:
	case cDecode + cNone:
	    memcpy(q, p, count);
	    break;
	case cEncode + cSub:
	    for (; count; ++q, ++dprev, ++p, --count)
		*q = (byte) (*p - *dprev);
	    break;
	case cDecode + cSub:
	    for (; count; ++q, ++dprev, ++p, --count)
		*q = (byte) (*p + *dprev);
	    break;
	case cEncode + cUp:
	    for (; count; ++q, ++up, ++p, --count)
		*q = (byte) (*p - *up);
	    break;
	case cDecode + cUp:
	    for (; count; ++q, ++up, ++p, --count)
		*q = (byte) (*p + *up);
	    break;
	case cEncode + cAverage:
	    for (; count; ++q, ++dprev, ++up, ++p, --count)
		*q = (byte) (*p - arith_rshift_1((int)*dprev + (int)*up));
	    break;
	case cDecode + cAverage:
	    for (; count; ++q, ++dprev, ++up, ++p, --count)
		*q = (byte) (*p + arith_rshift_1((int)*dprev + (int)*up));
	    break;
	case cEncode + cPaeth:
	    for (; count; ++q, ++dprev, ++up, ++upprev, ++p, --count)
		*q = (byte) (*p - paeth_predictor(*dprev, *up, *upprev));
	    break;
	case cDecode + cPaeth:
	    for (; count; ++q, ++dprev, ++up, ++upprev, ++p, --count)
		*q = (byte) (*p + paeth_predictor(*dprev, *up, *upprev));
	    break;
    }
}

/* Calculate the number of bytes for the next processing step, */
/* the min of (input data, output data, remaining row length). */
private uint
s_pngp_count(const stream_state * st_const, const stream_cursor_read * pr,
	     const stream_cursor_write * pw)
{
    const stream_PNGP_state *const ss_const =
	(const stream_PNGP_state *)st_const;
    uint rcount = pr->limit - pr->ptr;
    uint wcount = pw->limit - pw->ptr;
    uint row_left = ss_const->row_left;

    if (rcount < row_left)
	row_left = rcount;
    if (wcount < row_left)
	row_left = wcount;
    return row_left;
}

/*
 * Encode a buffer.  Let N = ss->row_count, P = N - ss->row_left,
 * and B = ss->bpp.  Consider that bytes [-B .. -1] of every row are zero.
 * Then:
 *      prev_row[0 .. P - 1] contain bytes -B .. P - B - 1
 *        of the current input row.
 *      ss->prev[0 .. B - 1] contain bytes P - B .. P - 1
 *        of the current input row.
 *      prev_row[P .. N + B - 1] contain bytes P - B .. N - 1
 *        of the previous input row.
 */
private int
optimum_predictor(const stream_state * st, const stream_cursor_read * pr)
{
    return cSub;
}
private int
s_PNGPE_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;
    int bpp = ss->bpp;
    int status = 0;

    while (pr->ptr < pr->limit) {
	uint count;

	if (ss->row_left == 0) {
	    /* Beginning of row, write algorithm byte. */
	    int predictor;

	    if (pw->ptr >= pw->limit) {
		status = 1;
		break;
	    }
	    predictor =
		(ss->Predictor == cOptimum ?
		 optimum_predictor(st, pr) :
		 ss->Predictor);
	    *++(pw->ptr) = (byte) predictor - cNone;
	    ss->case_index = predictor + cEncode;
	    ss->row_left = ss->row_count;
	    memset(ss->prev, 0, bpp);
	    continue;
	}
	count = s_pngp_count(st, pr, pw);
	if (count == 0) {
	    /* We know we have input, so output must be full. */
	    status = 1;
	    break;
	} {
	    byte *up = ss->prev_row + bpp + ss->row_count - ss->row_left;
	    uint n = min(count, bpp);

	    /* Process bytes whose predecessors are in prev. */
	    s_pngp_process(st, pw, ss->prev, pr, up - bpp, up, n);
	    if (ss->prev_row)
		memcpy(up - bpp, ss->prev, n);
	    if (ss->row_left == 0)
		continue;
	    if (n < bpp) {
		/*
		 * We didn't have both enough input data and enough output
		 * space to use up all of prev.  Shift more data into prev
		 * and exit.
		 */
		int prev_left = bpp - n;

		memmove(ss->prev, ss->prev + n, prev_left);
		memcpy(ss->prev + prev_left, pr->ptr - (n - 1), n);
		if (pw->ptr >= pw->limit && pr->ptr < pr->limit)
		    status = 1;
		break;
	    }
	    /* Process bytes whose predecessors are in the input. */
	    /* We know we have at least bpp input and output bytes, */
	    /* and that n = bpp. */
	    count -= bpp;
	    s_pngp_process(st, pw, pr->ptr - (bpp - 1), pr,
			   up, up + bpp, count);
	    memcpy(ss->prev, pr->ptr - (bpp - 1), bpp);
	    if (ss->prev_row) {
		memcpy(up, pr->ptr - (bpp + count - 1), count);
		if (ss->row_left == 0)
		    memcpy(up + count, ss->prev, bpp);
	    }
	}
    }
    return status;
}

/*
 * Decode a buffer.  Let N = ss->row_count, P = N - ss->row_left,
 * and B = ss->bpp.  Consider that bytes [-B .. -1] of every row are zero.
 * Then:
 *      prev_row[0 .. P - 1] contain bytes -B .. P - B - 1
 *        of the current output row.
 *      ss->prev[0 .. B - 1] contain bytes P - B .. P - 1
 *        of the current output row.
 *      prev_row[P .. N + B - 1] contain bytes P - B .. N - 1
 *        of the previous output row.
 */
private int
s_PNGPD_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last)
{
    stream_PNGP_state *const ss = (stream_PNGP_state *) st;
    int bpp = ss->bpp;
    int status = 0;

    while (pr->ptr < pr->limit) {
	uint count;

	if (ss->row_left == 0) {
	    /* Beginning of row, read algorithm byte. */
	    int predictor = pr->ptr[1];

	    if (predictor >= cOptimum - cNone) {
		status = ERRC;
		break;
	    }
	    pr->ptr++;
	    ss->case_index = predictor + cNone + cDecode;
	    ss->row_left = ss->row_count;
	    memset(ss->prev, 0, bpp);
	    continue;
	}
	count = s_pngp_count(st, pr, pw);
	if (count == 0) {
	    /* We know we have input, so output must be full. */
	    status = 1;
	    break;
	} {
	    byte *up = ss->prev_row + bpp + ss->row_count - ss->row_left;
	    uint n = min(count, bpp);

	    /* Process bytes whose predecessors are in prev. */
	    s_pngp_process(st, pw, ss->prev, pr, up - bpp, up, n);
	    if (ss->prev_row)
		memcpy(up - bpp, ss->prev, n);
	    if (ss->row_left == 0)
		continue;
	    if (n < bpp) {
		/*
		 * We didn't have both enough input data and enough output
		 * space to use up all of prev.  Shift more data into prev
		 * and exit.
		 */
		int prev_left = bpp - n;

		memmove(ss->prev, ss->prev + n, prev_left);
		memcpy(ss->prev + prev_left, pw->ptr - (n - 1), n);
		if (pw->ptr >= pw->limit && pr->ptr < pr->limit)
		    status = 1;
		break;
	    }
	    /* Process bytes whose predecessors are in the output. */
	    /* We know we have at least bpp input and output bytes, */
	    /* and that n = bpp. */
	    count -= bpp;
	    s_pngp_process(st, pw, pw->ptr - (bpp - 1), pr,
			   up, up + bpp, count);
	    memcpy(ss->prev, pw->ptr - (bpp - 1), bpp);
	    if (ss->prev_row) {
		memcpy(up, pw->ptr - (bpp + count - 1), count);
		if (ss->row_left == 0)
		    memcpy(up + count, ss->prev, bpp);
	    }
	}
    }
    return status;
}

/* Stream templates */
const stream_template s_PNGPE_template = {
    &st_PNGP_state, s_PNGPE_init, s_PNGPE_process, 1, 1, s_PNGP_release,
    s_PNGP_set_defaults, s_PNGP_reinit
};
const stream_template s_PNGPD_template = {
    &st_PNGP_state, s_PNGPD_init, s_PNGPD_process, 1, 1, s_PNGP_release,
    s_PNGP_set_defaults, s_PNGP_reinit
};
