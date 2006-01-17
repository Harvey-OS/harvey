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

/* $Id: gsbitcom.c,v 1.3 2002/02/21 22:24:52 giles Exp $ */
/* Oversampled bitmap compression */
#include "std.h"
#include "gstypes.h"
#include "gdebug.h"
#include "gsbitops.h"		/* for prototype */

/*
 * Define a compile-time option to reverse nibble order in alpha maps.
 * Note that this does not reverse bit order within nibbles.
 * This option is here for a very specialized purpose and does not
 * interact well with the rest of the code.
 */
#ifndef ALPHA_LSB_FIRST
#  define ALPHA_LSB_FIRST 0
#endif

/* Count the number of 1-bits in a half-byte. */
static const byte half_byte_1s[16] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/* Count the number of trailing 1s in an up-to-5-bit value, -1. */
static const byte bits5_trailing_1s[32] = {
    0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3,
    0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4
};

/* Count the number of leading 1s in an up-to-5-bit value, -1. */
static const byte bits5_leading_1s[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4
};

/*
 * Compress a value between 0 and 2^M to a value between 0 and 2^N-1.
 * Possible values of M are 1, 2, 3, or 4; of N are 1, 2, and 4.
 * The name of the table is compress_count_M_N.
 * As noted below, we require that N <= M.
 */
static const byte compress_1_1[3] = {
    0, 1, 1
};
static const byte compress_2_1[5] = {
    0, 0, 1, 1, 1
};
static const byte compress_2_2[5] = {
    0, 1, 2, 2, 3
};
static const byte compress_3_1[9] = {
    0, 0, 0, 0, 1, 1, 1, 1, 1
};
static const byte compress_3_2[9] = {
    0, 0, 1, 1, 2, 2, 2, 3, 3
};
static const byte compress_4_1[17] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const byte compress_4_2[17] = {
    0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3
};
static const byte compress_4_4[17] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15
};

/* The table of tables is indexed by log2(N) and then by M-1. */
static const byte *const compress_tables[4][4] = {
    {compress_1_1, compress_2_1, compress_3_1, compress_4_1},
    {0, compress_2_2, compress_3_2, compress_4_2},
    {0, 0, 0, compress_4_4}
};

/*
 * Compress an XxY-oversampled bitmap to Nx1 by counting 1-bits.  The X and
 * Y oversampling factors are 1, 2, or 4, but may be different.  N, the
 * resulting number of (alpha) bits per pixel, may be 1, 2, or 4; we allow
 * compression in place, in which case N must not exceed the X oversampling
 * factor.  Width and height are the source dimensions, and hence reflect
 * the oversampling; both are multiples of the relevant scale factor.  The
 * same is true for srcx.
 */
void
bits_compress_scaled(const byte * src, int srcx, uint width, uint height,
		     uint sraster, byte * dest, uint draster,
		     const gs_log2_scale_point *plog2_scale, int log2_out_bits)
{
    int log2_x = plog2_scale->x, log2_y = plog2_scale->y;
    int xscale = 1 << log2_x;
    int yscale = 1 << log2_y;
    int out_bits = 1 << log2_out_bits;
    /*
     * The following two initializations are only needed (and the variables
     * are only used) if out_bits <= xscale.  We do them in all cases only
     * to suppress bogus "possibly uninitialized variable" warnings from
     * certain compilers.
     */
    int input_byte_out_bits = out_bits << (3 - log2_x);
    byte input_byte_out_mask = (1 << input_byte_out_bits) - 1;
    const byte *table =
	compress_tables[log2_out_bits][log2_x + log2_y - 1];
    uint sskip = sraster << log2_y;
    uint dwidth = (width >> log2_x) << log2_out_bits;
    uint dskip = draster - ((dwidth + 7) >> 3);
    uint mask = (1 << xscale) - 1;
    uint count_max = 1 << (log2_x + log2_y);
    /*
     * For the moment, we don't attempt to take advantage of the fact
     * that the input is aligned.
     */
    const byte *srow = src + (srcx >> 3);
    int in_shift_initial = 8 - xscale - (srcx & 7);
    int in_shift_check = (out_bits <= xscale ? 8 - xscale : -1);
    byte *d = dest;
    uint h;

    for (h = height; h; srow += sskip, h -= yscale) {
	const byte *s = srow;

#if ALPHA_LSB_FIRST
#  define out_shift_initial 0
#  define out_shift_update(out_shift, nbits) ((out_shift += (nbits)) >= 8)
#else
#  define out_shift_initial (8 - out_bits)
#  define out_shift_update(out_shift, nbits) ((out_shift -= (nbits)) < 0)
#endif
	int out_shift = out_shift_initial;
	byte out = 0;
	int in_shift = in_shift_initial;
	int dw = 8 - (srcx & 7);
	int w;

	/* Loop over source bytes. */
	for (w = width; w > 0; w -= dw, dw = 8) {
	    int index;
	    int in_shift_final = (w >= dw ? 0 : dw - w);

	    /*
	     * Check quickly for all-0s or all-1s, but only if each
	     * input byte generates no more than one output byte,
	     * we're at an input byte boundary, and we're processing
	     * an entire input byte (i.e., this isn't a final
	     * partial byte.)
	     */
	    if (in_shift == in_shift_check && in_shift_final == 0)
		switch (*s) {
		    case 0:
			for (index = sraster; index != sskip; index += sraster)
			    if (s[index] != 0)
				goto p;
			if (out_shift_update(out_shift, input_byte_out_bits))
			    *d++ = out, out_shift &= 7, out = 0;
			s++;
			continue;
#if !ALPHA_LSB_FIRST		/* too messy to make it work */
		    case 0xff:
			for (index = sraster; index != sskip; index += sraster)
			    if (s[index] != 0xff)
				goto p;
			{
			    int shift =
				(out_shift -= input_byte_out_bits) + out_bits;

			    if (shift > 0)
				out |= input_byte_out_mask << shift;
			    else {
				out |= input_byte_out_mask >> -shift;
				*d++ = out;
				out_shift += 8;
				out = input_byte_out_mask << (8 + shift);
			    }
			}
			s++;
			continue;
#endif
		    default:
			;
		}
	  p:			/* Loop over source pixels within a byte. */
	    do {
		uint count;

		for (index = 0, count = 0; index != sskip;
		     index += sraster
		    )
		    count += half_byte_1s[(s[index] >> in_shift) & mask];
		if (count != 0 && table[count] == 0) {	/* Look at adjacent cells to help prevent */
		    /* dropouts. */
		    uint orig_count = count;
		    uint shifted_mask = mask << in_shift;
		    byte in;

		    if_debug3('B', "[B]count(%d,%d)=%d\n",
			      (width - w) / xscale,
			      (height - h) / yscale, count);
		    if (yscale > 1) {	/* Look at the next "lower" cell. */
			if (h < height && (in = s[0] & shifted_mask) != 0) {
			    uint lower;

			    for (index = 0, lower = 0;
				 -(index -= sraster) <= sskip &&
				 (in &= s[index]) != 0;
				)
				lower += half_byte_1s[in >> in_shift];
			    if_debug1('B', "[B]  lower adds %d\n",
				      lower);
			    if (lower <= orig_count)
				count += lower;
			}
			/* Look at the next "higher" cell. */
			if (h > yscale && (in = s[sskip - sraster] & shifted_mask) != 0) {
			    uint upper;

			    for (index = sskip, upper = 0;
				 index < sskip << 1 &&
				 (in &= s[index]) != 0;
				 index += sraster
				)
				upper += half_byte_1s[in >> in_shift];
			    if_debug1('B', "[B]  upper adds %d\n",
				      upper);
			    if (upper < orig_count)
				count += upper;
			}
		    }
		    if (xscale > 1) {
			uint mask1 = (mask << 1) + 1;

			/* Look at the next cell to the left. */
			if (w < width) {
			    int lshift = in_shift + xscale - 1;
			    uint left;

			    for (index = 0, left = 0;
				 index < sskip; index += sraster
				) {
				uint bits =
				((s[index - 1] << 8) +
				 s[index]) >> lshift;

				left += bits5_trailing_1s[bits & mask1];
			    }
			    if_debug1('B', "[B]  left adds %d\n",
				      left);
			    if (left < orig_count)
				count += left;
			}
			/* Look at the next cell to the right. */
			if (w > xscale) {
			    int rshift = in_shift - xscale + 8;
			    uint right;

			    for (index = 0, right = 0;
				 index < sskip; index += sraster
				) {
				uint bits =
				((s[index] << 8) +
				 s[index + 1]) >> rshift;

				right += bits5_leading_1s[(bits & mask1) << (4 - xscale)];
			    }
			    if_debug1('B', "[B]  right adds %d\n",
				      right);
			    if (right <= orig_count)
				count += right;
			}
		    }
		    if (count > count_max)
			count = count_max;
		}
		out += table[count] << out_shift;
		if (out_shift_update(out_shift, out_bits))
		    *d++ = out, out_shift &= 7, out = 0;
	    }
	    while ((in_shift -= xscale) >= in_shift_final);
	    s++, in_shift += 8;
	}
	if (out_shift != out_shift_initial)
	    *d++ = out;
	for (w = dskip; w != 0; w--)
	    *d++ = 0;
#undef out_shift_initial
#undef out_shift_update
    }
}
