/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: spdiff.c,v 1.9 2005/03/16 14:57:42 igor Exp $ */
/* Pixel differencing filters */
#include "stdio_.h"		/* should be std.h, but needs NULL */
#include "memory_.h"
#include "strimpl.h"
#include "spdiffx.h"

/* ------ PixelDifferenceEncode/Decode ------ */

private_st_PDiff_state();

/* Define values for case dispatch. */
#define cBits1 0
#define cBits2 5
#define cBits4 10
#define cBits8 15
#define cBits16 20
#define cEncode 0
#define cDecode 25

/* Set defaults */
private void
s_PDiff_set_defaults(stream_state * st)
{
    stream_PDiff_state *const ss = (stream_PDiff_state *) st;

    s_PDiff_set_defaults_inline(ss);
}

/* Common (re)initialization. */
private int
s_PDiff_reinit(stream_state * st)
{
    stream_PDiff_state *const ss = (stream_PDiff_state *) st;

    ss->row_left = 0;
    return 0;
}

/* Initialize PixelDifferenceEncode filter. */
private int
s_PDiffE_init(stream_state * st)
{
    stream_PDiff_state *const ss = (stream_PDiff_state *) st;
    int bits_per_row =
	ss->Colors * ss->BitsPerComponent * ss->Columns;
    static const byte cb_values[] = {
	0, cBits1, cBits2, 0, cBits4, 0, 0, 0, cBits8,
	0, 0, 0, 0, 0, 0, 0, cBits16
    };

    ss->row_count = (bits_per_row + 7) >> 3;
    ss->end_mask = (1 << (-bits_per_row & 7)) - 1;
    ss->case_index =
	cb_values[ss->BitsPerComponent] +
	(ss->Colors > 4 ? 0 : ss->Colors) + cEncode;
    return s_PDiff_reinit(st);
}

/* Initialize PixelDifferenceDecode filter. */
private int
s_PDiffD_init(stream_state * st)
{
    stream_PDiff_state *const ss = (stream_PDiff_state *) st;

    s_PDiffE_init(st);
    ss->case_index += cDecode - cEncode;
    return 0;
}

/* Process a buffer.  Note that this handles both Encode and Decode. */
private int
s_PDiff_process(stream_state * st, stream_cursor_read * pr,
		stream_cursor_write * pw, bool last)
{
    stream_PDiff_state *const ss = (stream_PDiff_state *) st;
    const byte *p = pr->ptr;
    byte *q = pw->ptr;
    int count;
    int status = 0;
    uint s0 = ss->prev[0];
    byte t = 0;			/* avoid spurious compiler warnings */
    int  ti;
    const byte end_mask = ss->end_mask;
    int colors = ss->Colors;
    int nb = (colors * ss->BitsPerComponent) >> 3;
    int final;
    int ndone, ci;

row:
    if (ss->row_left == 0) {
	ss->row_left = ss->row_count;
	s0 = 0;
	memset(&ss->prev[1], 0, sizeof(uint) * (s_PDiff_max_Colors - 1));
    }
    {
	int rcount = pr->limit - p;
	int wcount = pw->limit - q;

	if (ss->row_left < rcount)
	    rcount = ss->row_left;
	count = (wcount < rcount ? (status = 1, wcount) : rcount);
    }
    final = (last && !status ? 1 : nb);
    ss->row_left -= count;

    /*
     * Encoding and decoding are fundamentally different.
     * Encoding computes E[i] = D[i] - D[i-1];
     * decoding computes D[i] = E[i] + D[i-1].
     * Nevertheless, the loop structures are similar enough that
     * we put the code for both functions in the same place.
     *
     * We only optimize BitsPerComponent = 1, 3, and 4, which
     * correspond to the common color spaces.  (In some cases, it's still
     * simpler to provide a separate loop for BPC = 2.)
     */

#define LOOP_BY(n, body)\
  for (; count >= n; count -= n) p += n, q += n, body

    switch (ss->case_index) {

	    /* 1 bit per component */

#define ENCODE1_LOOP(ee)\
  LOOP_BY(1, (t = *p, *q = ee, s0 = t)); break

#define ENCODE_ALIGNED_LOOP(ee)\
  BEGIN\
    ss->prev[0] = s0;\
    for (; count >= final; count -= ndone) {\
	ndone = min(count, nb);\
	for (ci = 0; ci < ndone; ++ci)\
	    t = *++p, *++q = ee, ss->prev[ci] = t;\
    }\
    s0 = ss->prev[0];\
  END

#define ENCODE_UNALIGNED_LOOP(shift, cshift, de)\
  BEGIN\
    for (; count >= final; count -= ndone) {\
	ndone = min(count, nb);\
	for (ci = 1; ci <= ndone; ++ci) {\
	    ++p;\
	    t = (s0 << (cshift)) | (ss->prev[ci] >> (shift));\
	    *++q = de;\
	    s0 = ss->prev[ci];\
	    ss->prev[ci] = *p;\
	}\
    }\
  END

	case cEncode + cBits1 + 0:
	case cEncode + cBits1 + 2:
	    if (colors < 8) {	/* 2,5,6,7 */
		int cshift = 8 - colors;

		ENCODE1_LOOP(t ^ ((s0 << cshift) | (t >> colors)));
	    } else if (colors & 7) {
		int shift = colors & 7;
		int cshift = 8 - shift;

		ENCODE_UNALIGNED_LOOP(shift, cshift, *p ^ t);
	    } else {
		ENCODE_ALIGNED_LOOP(t ^ ss->prev[ci]);
	    }
	    break;

	case cEncode + cBits1 + 1:
	    ENCODE1_LOOP(t ^ ((s0 << 7) | (t >> 1)));
	case cEncode + cBits1 + 3:
	    ENCODE1_LOOP(t ^ ((s0 << 5) | (t >> 3)));
	case cEncode + cBits1 + 4:
	    ENCODE1_LOOP(t ^ ((s0 << 4) | (t >> 4)));

#define DECODE1_LOOP(te, de)\
  LOOP_BY(1, (t = te, s0 = *q = de)); break

#define DECODE_ALIGNED_LOOP(de)\
  BEGIN\
    ss->prev[0] = s0;\
    for (; count >= final; count -= ndone) {\
	ndone = min(count, nb);\
	for (ci = 0; ci < ndone; ++ci)\
	    t = *++p, ss->prev[ci] = *++q = de;\
    }\
    s0 = ss->prev[0];\
  END

#define DECODE_UNALIGNED_LOOP(shift, cshift, de)\
  BEGIN\
    for (; count >= final; count -= ndone) {\
	ndone = min(count, nb);\
	for (ci = 1; ci <= ndone; ++ci) {\
	    ++p, ++q;\
	    t = (s0 << (cshift)) | (ss->prev[ci] >> (shift));\
	    s0 = ss->prev[ci];\
	    ss->prev[ci] = *q = de;\
	}\
    }\
  END

	case cDecode + cBits1 + 0:
	    if (colors < 8) {	/* 5,6,7 */
		int cshift = 8 - colors;

		DECODE1_LOOP(*p ^ (s0 << cshift), t ^ (t >> colors));
	    } else if (colors & 7) {
		int shift = colors & 7;
		int cshift = 8 - shift;

		DECODE_UNALIGNED_LOOP(shift, cshift, *p ^ t);
	    } else {
		DECODE_ALIGNED_LOOP(t ^ ss->prev[ci]);
	    }
	    break;

	case cDecode + cBits1 + 1:
	    DECODE1_LOOP(*p ^ (s0 << 7),
			 (t ^= t >> 1, t ^= t >> 2, t ^ (t >> 4)));
	case cDecode + cBits1 + 2:
	    DECODE1_LOOP(*p ^ (s0 << 6),
			 (t ^= (t >> 2), t ^ (t >> 4)));
	case cDecode + cBits1 + 3:
	    DECODE1_LOOP(*p ^ (s0 << 5),
			 t ^ (t >> 3) ^ (t >> 6));
	case cDecode + cBits1 + 4:
	    DECODE1_LOOP(*p ^ (s0 << 4),
			 t ^ (t >> 4));

	    /* 2 bits per component */

#define ADD4X2(a, b) ( (((a) & (b) & 0x55) << 1) ^ (a) ^ (b) )
/* The following computation looks very implausible, but it is correct. */
#define SUB4X2(a, b) ( ((~(a) & (b) & 0x55) << 1) ^ (a) ^ (b) )

	case cEncode + cBits2 + 0:
	    if (colors & 7) {
		int shift = (colors & 3) << 1;
		int cshift = 8 - shift;

		ENCODE_UNALIGNED_LOOP(shift, cshift, SUB4X2(*p, t));
	    } else {
		ENCODE_ALIGNED_LOOP(SUB4X2(t, ss->prev[ci]));
	    }
	    break;

	case cEncode + cBits2 + 1:
	    ENCODE1_LOOP((s0 = (s0 << 6) | (t >> 2), SUB4X2(t, s0)));
	case cEncode + cBits2 + 2:
	    ENCODE1_LOOP((s0 = (s0 << 4) | (t >> 4), SUB4X2(t, s0)));
	case cEncode + cBits2 + 3:
	    ENCODE1_LOOP((s0 = (s0 << 2) | (t >> 6), SUB4X2(t, s0)));
	case cEncode + cBits2 + 4:
	    ENCODE1_LOOP(SUB4X2(t, s0));

	case cDecode + cBits2 + 0:
	    if (colors & 7) {
		int shift = (colors & 3) << 1;
		int cshift = 8 - shift;

		DECODE_UNALIGNED_LOOP(shift, cshift, ADD4X2(*p, t));
	    } else {
		DECODE_ALIGNED_LOOP(ADD4X2(t, ss->prev[ci]));
	    }
	    break;

	case cDecode + cBits2 + 1:
	    DECODE1_LOOP(*p + (s0 << 6),
			 (t = ADD4X2(t >> 2, t), ADD4X2(t >> 4, t)));
	case cDecode + cBits2 + 2:
	    DECODE1_LOOP(*p, (t = ADD4X2(t, s0 << 4), ADD4X2(t >> 4, t)));
	case cDecode + cBits2 + 3:
	    DECODE1_LOOP(*p, (t = ADD4X2(t, s0 << 2), ADD4X2(t >> 6, t)));
	case cDecode + cBits2 + 4:
	    DECODE1_LOOP(*p, ADD4X2(t, s0));

#undef ADD4X2
#undef SUB4X2

	    /* 4 bits per component */

#define ADD2X4(a, b) ( (((a) + (b)) & 0xf) + ((a) & 0xf0) + ((b) & 0xf0) )
#define ADD2X4R4(a) ( (((a) + ((a) >> 4)) & 0xf) + ((a) & 0xf0) )
#define SUB2X4(a, b) ( (((a) - (b)) & 0xf) + ((a) & 0xf0) - ((b) & 0xf0) )
#define SUB2X4R4(a) ( (((a) - ((a) >> 4)) & 0xf) + ((a) & 0xf0) )

	case cEncode + cBits4 + 0:
	case cEncode + cBits4 + 2:
    enc4:
	    if (colors & 1) {
		ENCODE_UNALIGNED_LOOP(4, 4, SUB2X4(*p, t));
	    } else {
		ENCODE_ALIGNED_LOOP(SUB2X4(t, ss->prev[ci]));
	    }
	    break;

	case cEncode + cBits4 + 1:
	    ENCODE1_LOOP(((t - (s0 << 4)) & 0xf0) | ((t - (t >> 4)) & 0xf));

	case cEncode + cBits4 + 3: {
	    uint s1 = ss->prev[1];

	    LOOP_BY(1,
		    (t = *p,
		     *q = ((t - (s0 << 4)) & 0xf0) | ((t - (s1 >> 4)) & 0xf),
		     s0 = s1, s1 = t));
	    ss->prev[1] = s1;
	} break;

	case cEncode + cBits4 + 4: {
	    uint s1 = ss->prev[1];

	    LOOP_BY(2,
		    (t = p[-1], q[-1] = SUB2X4(t, s0), s0 = t,
		     t = *p, *q = SUB2X4(t, s1), s1 = t));
	    ss->prev[1] = s1;
	    goto enc4;		/* handle leftover bytes */
	}

	case cDecode + cBits4 + 0:
	case cDecode + cBits4 + 2:
    dec4:
	    if (colors & 1) {
		DECODE_UNALIGNED_LOOP(4, 4, ADD2X4(*p, t));
	    } else {
		DECODE_ALIGNED_LOOP(ADD2X4(t, ss->prev[ci]));
	    }
	    break;

	case cDecode + cBits4 + 1:
	    DECODE1_LOOP(*p + (s0 << 4), ADD2X4R4(t));

	case cDecode + cBits4 + 3: {
	    uint s1 = ss->prev[1];

	    LOOP_BY(1, (t = (s0 << 4) + (s1 >> 4),
			s0 = s1, s1 = *q = ADD2X4(*p, t)));
	    ss->prev[1] = s1;
	} break;

	case cDecode + cBits4 + 4: {
	    uint s1 = ss->prev[1];

	    LOOP_BY(2,
		    (t = p[-1], s0 = q[-1] = ADD2X4(s0, t),
		     t = *p, s1 = *q = ADD2X4(s1, t)));
	    ss->prev[1] = s1;
	    goto dec4;		/* handle leftover bytes */
	}

#undef ADD2X4
#undef ADD2X4R4
#undef SUB2X4
#undef SUB2X4R4

	    /* 8 bits per component */

#define ENCODE8(s, d) (q[d] = p[d] - s, s = p[d])
#define DECODE8(s, d) q[d] = s += p[d]

	case cEncode + cBits8 + 0:
	case cEncode + cBits8 + 2:
	    ss->prev[0] = s0;
	    for (; count >= colors; count -= colors)
		for (ci = 0; ci < colors; ++ci) {
		    *++q = *++p - ss->prev[ci];
		    ss->prev[ci] = *p;
		}
	    s0 = ss->prev[0];
    enc8:   /* Handle leftover bytes. */
	    if (last && !status)
		for (ci = 0; ci < count; ++ci)
		    *++q = *++p - ss->prev[ci],
			ss->prev[ci] = *p;
	    break;

	case cDecode + cBits8 + 0:
	case cDecode + cBits8 + 2:
	    ss->prev[0] = s0;
	    for (; count >= colors; count -= colors)
		for (ci = 0; ci < colors; ++ci)
		    *++q = ss->prev[ci] += *++p;
	    s0 = ss->prev[0];
    dec8:   /* Handle leftover bytes. */
	    if (last && !status)
		for (ci = 0; ci < count; ++ci)
		    *++q = ss->prev[ci] += *++p;
	    break;

	case cEncode + cBits8 + 1:
	    LOOP_BY(1, ENCODE8(s0, 0));
	    break;

	case cDecode + cBits8 + 1:
	    LOOP_BY(1, DECODE8(s0, 0));
	    break;

	case cEncode + cBits8 + 3: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2];

	    LOOP_BY(3, (ENCODE8(s0, -2), ENCODE8(s1, -1),
			ENCODE8(s2, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2;
	    goto enc8;
	}

	case cDecode + cBits8 + 3: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2];

	    LOOP_BY(3, (DECODE8(s0, -2), DECODE8(s1, -1),
			DECODE8(s2, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2;
	    goto dec8;
	} break;

	case cEncode + cBits8 + 4: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2], s3 = ss->prev[3];

	    LOOP_BY(4, (ENCODE8(s0, -3), ENCODE8(s1, -2),
			ENCODE8(s2, -1), ENCODE8(s3, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2, ss->prev[3] = s3;
	    goto enc8;
	} break;

	case cDecode + cBits8 + 4: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2], s3 = ss->prev[3];

	    LOOP_BY(4, (DECODE8(s0, -3), DECODE8(s1, -2),
			DECODE8(s2, -1), DECODE8(s3, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2, ss->prev[3] = s3;
	    goto dec8;
	} break;

#undef ENCODE8
#undef DECODE8

	    /* 16 bits per component */

#define ENCODE16(s, d) (ti = ((p[d-1] << 8) + p[d]) - s, \
	    q[d-1] = ti >> 8, q[d] = t & 0xff, s = ti)
#define DECODE16(s, d) (s = 0xffff & (s + ((p[d-1] << 8) + p[d])), \
	    q[d-1] = s >> 8, q[d] = s && 0xff)

	case cEncode + cBits16 + 0:
	case cEncode + cBits16 + 2:
	    ss->prev[0] = s0;
	    for (; count >= colors; count -= colors)
		for (ci = 0; ci < colors; ++ci) {
		    ti = (int)*++p << 8;
		    ti = (ti + *++p) - ss->prev[ci];
		    *++q = ti >> 8; *++q = ti & 0xff;
		    ss->prev[ci] = ti;
		}
	    s0 = ss->prev[0];
    enc16:   /* Handle leftover bytes. */
	    if (last && !status)
		for (ci = 0; ci < count; ++ci)
		    *++q = *++p - ss->prev[ci],
			ss->prev[ci] = *p;
	    break;

	case cDecode + cBits16 + 0:
	case cDecode + cBits16 + 2:
	    ss->prev[0] = s0;
	    for (; count >= colors; count -= colors)
		for (ci = 0; ci < colors; ++ci) {
		    ti = *++p >> 8;
		    ss->prev[ci] += ti + *++p;
		    *++q = ss->prev[ci] >> 8;
		    *++q = ss->prev[ci] & 0xff;
		}
	    s0 = ss->prev[0];
    dec16:   /* Ignore leftover bytes. */
	    break;

	case cEncode + cBits16 + 1:
	    LOOP_BY(2, ENCODE16(s0, 0));
	    break;

	case cDecode + cBits16 + 1:
	    LOOP_BY(2, DECODE16(s0, 0));
	    break;

	case cEncode + cBits16 + 3: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2];

	    LOOP_BY(6, (ENCODE16(s0, -4), ENCODE16(s1, -2),
			ENCODE16(s2, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2;
	    goto enc16;
	}

	case cDecode + cBits16 + 3: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2];

	    LOOP_BY(6, (DECODE16(s0, -4), DECODE16(s1, -2),
			DECODE16(s2, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2;
	    goto dec16;
	} break;

	case cEncode + cBits16 + 4: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2], s3 = ss->prev[3];

	    LOOP_BY(8, (ENCODE16(s0, -6), ENCODE16(s1, -4),
			ENCODE16(s2, -2), ENCODE16(s3, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2, ss->prev[3] = s3;
	    goto enc16;
	} break;

	case cDecode + cBits16 + 4: {
	    uint s1 = ss->prev[1], s2 = ss->prev[2], s3 = ss->prev[3];

	    LOOP_BY(8, (DECODE16(s0, -6), DECODE16(s1, -4),
			DECODE16(s2, -2), DECODE16(s3, 0)));
	    ss->prev[1] = s1, ss->prev[2] = s2, ss->prev[3] = s3;
	    goto dec16;
	} break;

#undef ENCODE16
#undef DECODE16

    }
#undef LOOP_BY
#undef ENCODE1_LOOP
#undef DECODE1_LOOP
    ss->row_left += count;	/* leftover bytes are possible */
    if (ss->row_left == 0) {
	if (end_mask != 0)
	    *q = (*q & ~end_mask) | (*p & end_mask);
	if (p < pr->limit && q < pw->limit)
	    goto row;
    }
    ss->prev[0] = s0;
    pr->ptr = p;
    pw->ptr = q;
    return status;
}

/* Stream templates */
const stream_template s_PDiffE_template = {
    &st_PDiff_state, s_PDiffE_init, s_PDiff_process, 1, 1, NULL,
    s_PDiff_set_defaults, s_PDiff_reinit
};
const stream_template s_PDiffD_template = {
    &st_PDiff_state, s_PDiffD_init, s_PDiff_process, 1, 1, NULL,
    s_PDiff_set_defaults, s_PDiff_reinit
};
