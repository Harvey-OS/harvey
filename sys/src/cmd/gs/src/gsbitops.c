/* Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsbitops.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Bitmap filling, copying, and transforming operations */
#include "stdio_.h"
#include "memory_.h"
#include "gdebug.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsbittab.h"
#include "gxbitops.h"

/*
 * Define a compile-time option to reverse nibble order in alpha maps.
 * Note that this does not reverse bit order within nibbles.
 * This option is here for a very specialized purpose and does not
 * interact well with the rest of the code.
 */
#ifndef ALPHA_LSB_FIRST
#  define ALPHA_LSB_FIRST 0
#endif

/* ---------------- Bit-oriented operations ---------------- */

/* Define masks for little-endian operation. */
/* masks[i] has the first i bits off and the rest on. */
#if !arch_is_big_endian
const bits16 mono_copy_masks[17] = {
    0xffff, 0xff7f, 0xff3f, 0xff1f,
    0xff0f, 0xff07, 0xff03, 0xff01,
    0xff00, 0x7f00, 0x3f00, 0x1f00,
    0x0f00, 0x0700, 0x0300, 0x0100,
    0x0000
};
const bits32 mono_fill_masks[33] = {
#define mask(n)\
  ((~0xff | (0xff >> (n & 7))) << (n & -8))
    mask( 0),mask( 1),mask( 2),mask( 3),mask( 4),mask( 5),mask( 6),mask( 7),
    mask( 8),mask( 9),mask(10),mask(11),mask(12),mask(13),mask(14),mask(15),
    mask(16),mask(17),mask(18),mask(19),mask(20),mask(21),mask(22),mask(23),
    mask(24),mask(25),mask(26),mask(27),mask(28),mask(29),mask(30),mask(31),
    0
#undef mask
};
#endif

/* Fill a rectangle of bits with an 8x1 pattern. */
/* The pattern argument must consist of the pattern in every byte, */
/* e.g., if the desired pattern is 0xaa, the pattern argument must */
/* have the value 0xaaaa (if ints are short) or 0xaaaaaaaa. */
#undef chunk
#define chunk mono_fill_chunk
#undef mono_masks
#define mono_masks mono_fill_masks
void
bits_fill_rectangle(byte * dest, int dest_bit, uint draster,
		    mono_fill_chunk pattern, int width_bits, int height)
{
    uint bit;
    chunk right_mask;
    int line_count = height;
    chunk *ptr;
    int last_bit;

#define FOR_EACH_LINE(stat)\
	do { stat } while ( inc_ptr(ptr, draster), --line_count )

    dest += (dest_bit >> 3) & -chunk_align_bytes;
    ptr = (chunk *) dest;
    bit = dest_bit & chunk_align_bit_mask;
    last_bit = width_bits + bit - (chunk_bits + 1);

    if (last_bit < 0) {		/* <=1 chunk */
	set_mono_thin_mask(right_mask, width_bits, bit);
	switch ((byte) pattern) {
	    case 0:
		FOR_EACH_LINE(*ptr &= ~right_mask;
		    );
		break;
	    case 0xff:
		FOR_EACH_LINE(*ptr |= right_mask;
		    );
		break;
	    default:
		FOR_EACH_LINE(
		       *ptr = (*ptr & ~right_mask) | (pattern & right_mask);
		    );
	}
    } else {
	chunk mask;
	int last = last_bit >> chunk_log2_bits;

	set_mono_left_mask(mask, bit);
	set_mono_right_mask(right_mask, (last_bit & chunk_bit_mask) + 1);
	switch (last) {
	    case 0:		/* 2 chunks */
		switch ((byte) pattern) {
		    case 0:
			FOR_EACH_LINE(*ptr &= ~mask;
				      ptr[1] &= ~right_mask;
			    );
			break;
		    case 0xff:
			FOR_EACH_LINE(*ptr |= mask;
				      ptr[1] |= right_mask;
			    );
			break;
		    default:
			FOR_EACH_LINE(
				   *ptr = (*ptr & ~mask) | (pattern & mask);
					 ptr[1] = (ptr[1] & ~right_mask) | (pattern & right_mask);
			    );
		}
		break;
	    case 1:		/* 3 chunks */
		switch ((byte) pattern) {
		    case 0:
			FOR_EACH_LINE(
					 *ptr &= ~mask;
					 ptr[1] = 0;
					 ptr[2] &= ~right_mask;
			    );
			break;
		    case 0xff:
			FOR_EACH_LINE(
					 *ptr |= mask;
					 ptr[1] = ~(chunk) 0;
					 ptr[2] |= right_mask;
			    );
			break;
		    default:
			FOR_EACH_LINE(
				   *ptr = (*ptr & ~mask) | (pattern & mask);
					 ptr[1] = pattern;
					 ptr[2] = (ptr[2] & ~right_mask) | (pattern & right_mask);
			    );
		}
		break;
	    default:{		/* >3 chunks */
		    uint byte_count = (last_bit >> 3) & -chunk_bytes;

		    switch ((byte) pattern) {
			case 0:
			    FOR_EACH_LINE(
					     *ptr &= ~mask;
					     memset(ptr + 1, 0, byte_count);
					     ptr[last + 1] &= ~right_mask;
				);
			    break;
			case 0xff:
			    FOR_EACH_LINE(
					     *ptr |= mask;
					  memset(ptr + 1, 0xff, byte_count);
					     ptr[last + 1] |= right_mask;
				);
			    break;
			default:
			    FOR_EACH_LINE(
				   *ptr = (*ptr & ~mask) | (pattern & mask);
				memset(ptr + 1, (byte) pattern, byte_count);
					     ptr[last + 1] =
					     (ptr[last + 1] & ~right_mask) |
					     (pattern & right_mask);
				);
		    }
		}
	}
    }
#undef FOR_EACH_LINE
}

/* Replicate a bitmap horizontally in place. */
void
bits_replicate_horizontally(byte * data, uint width, uint height,
		 uint raster, uint replicated_width, uint replicated_raster)
{
    /* The current algorithm is extremely inefficient! */
    const byte *orig_row = data + (height - 1) * raster;
    byte *tile_row = data + (height - 1) * replicated_raster;
    uint y;

    if (!(width & 7)) {
	uint src_bytes = width >> 3;
	uint dest_bytes = replicated_width >> 3;

	for (y = height; y-- > 0;
	     orig_row -= raster, tile_row -= replicated_raster
	     ) {
	    uint move = src_bytes;
	    const byte *from = orig_row;
	    byte *to = tile_row + dest_bytes - src_bytes;

	    memmove(to, from, move);
	    while (to - tile_row >= move) {
		from = to;
		to -= move;
		memmove(to, from, move);
		move <<= 1;
	    }
	    if (to != tile_row)
		memmove(tile_row, to, to - tile_row);
	}
    } else {
	/*
	 * This algorithm is inefficient, but probably not worth improving.
	 */
	uint bit_count = width & -width;  /* lowest bit: 1, 2, or 4 */
	uint left_mask = (0xff00 >> bit_count) & 0xff;

	for (y = height; y-- > 0;
	     orig_row -= raster, tile_row -= replicated_raster
	     ) {
	    uint sx;

	    for (sx = width; sx > 0;) {
		uint bits, dx;

		sx -= bit_count;
		bits = (orig_row[sx >> 3] << (sx & 7)) & left_mask;
		for (dx = sx + replicated_width; dx >= width;) {
		    byte *dp;
		    int dbit;

		    dx -= width;
		    dbit = dx & 7;
		    dp = tile_row + (dx >> 3);
		    *dp = (*dp & ~(left_mask >> dbit)) | (bits >> dbit);
		}
	    }
	}
    }
}

/* Replicate a bitmap vertically in place. */
void
bits_replicate_vertically(byte * data, uint height, uint raster,
			  uint replicated_height)
{
    byte *dest = data;
    uint h = replicated_height;
    uint size = raster * height;

    while (h > height) {
	memcpy(dest + size, dest, size);
	dest += size;
	h -= height;
    }
}

/* Find the bounding box of a bitmap. */
/* Assume bits beyond the width are zero. */
void
bits_bounding_box(const byte * data, uint height, uint raster,
		  gs_int_rect * pbox)
{
    register const ulong *lp;
    static const byte first_1[16] = {
	4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static const byte last_1[16] = {
	0, 4, 3, 4, 2, 4, 3, 4, 1, 4, 3, 4, 2, 4, 3, 4
    };

    /* Count trailing blank rows. */
    /* Since the raster is a multiple of sizeof(long), */
    /* we don't need to scan by bytes, only by longs. */

    lp = (const ulong *)(data + raster * height);
    while ((const byte *)lp > data && !lp[-1])
	--lp;
    if ((const byte *)lp == data) {
	pbox->p.x = pbox->q.x = pbox->p.y = pbox->q.y = 0;
	return;
    }
    pbox->q.y = height = ((const byte *)lp - data + raster - 1) / raster;

    /* Count leading blank rows. */

    lp = (const ulong *)data;
    while (!*lp)
	++lp;
    {
	uint n = ((const byte *)lp - data) / raster;

	pbox->p.y = n;
	if (n)
	    height -= n, data += n * raster;
    }

    /* Find the left and right edges. */
    /* We know that the first and last rows are non-blank. */

    {
	uint raster_longs = raster >> arch_log2_sizeof_long;
	uint left = raster_longs - 1, right = 0;
	ulong llong = 0, rlong = 0;
	const byte *q;
	uint h, n;

	for (q = data, h = height; h-- > 0; q += raster) {	/* Work from the left edge by longs. */
	    for (lp = (const ulong *)q, n = 0;
		 n < left && !*lp; lp++, n++
		);
	    if (n < left)
		left = n, llong = *lp;
	    else
		llong |= *lp;
	    /* Work from the right edge by longs. */
	    for (lp = (const ulong *)(q + raster - sizeof(long)),
		 n = raster_longs - 1;

		 n > right && !*lp; lp--, n--
		);
	    if (n > right)
		right = n, rlong = *lp;
	    else
		rlong |= *lp;
	}

	/* Do binary subdivision on edge longs.  We assume that */
	/* sizeof(long) = 4 or 8. */
#if arch_sizeof_long > 8
	Error_longs_are_too_large();
#endif

#if arch_is_big_endian
#  define last_bits(n) ((1L << (n)) - 1)
#  define shift_out_last(x,n) ((x) >>= (n))
#  define right_justify_last(x,n) DO_NOTHING
#else
#  define last_bits(n) (-1L << ((arch_sizeof_long * 8) - (n)))
#  define shift_out_last(x,n) ((x) <<= (n))
#  define right_justify_last(x,n) (x) >>= ((arch_sizeof_long * 8) - (n))
#endif

	left <<= arch_log2_sizeof_long + 3;
#if arch_sizeof_long == 8
	if (llong & ~last_bits(32))
	    shift_out_last(llong, 32);
	else
	    left += 32;
#endif
	if (llong & ~last_bits(16))
	    shift_out_last(llong, 16);
	else
	    left += 16;
	if (llong & ~last_bits(8))
	    shift_out_last(llong, 8);
	else
	    left += 8;
	right_justify_last(llong, 8);
	if (llong & 0xf0)
	    left += first_1[(byte) llong >> 4];
	else
	    left += first_1[(byte) llong] + 4;

	right <<= arch_log2_sizeof_long + 3;
#if arch_sizeof_long == 8
	if (!(rlong & last_bits(32)))
	    shift_out_last(rlong, 32);
	else
	    right += 32;
#endif
	if (!(rlong & last_bits(16)))
	    shift_out_last(rlong, 16);
	else
	    right += 16;
	if (!(rlong & last_bits(8)))
	    shift_out_last(rlong, 8);
	else
	    right += 8;
	right_justify_last(rlong, 8);
	if (!(rlong & 0xf))
	    right += last_1[(byte) rlong >> 4];
	else
	    right += last_1[(uint) rlong & 0xf] + 4;

	pbox->p.x = left;
	pbox->q.x = right;
    }
}

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
		 const gs_log2_scale_point * plog2_scale, int log2_out_bits)
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
     * For right now, we don't attempt to take advantage of the fact
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
	    int in_shift_final =
	    (w >= dw ? 0 : dw - w);

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

/* Extract a plane from a pixmap. */
int
bits_extract_plane(const bits_plane_t *dest /*write*/,
    const bits_plane_t *source /*read*/, int shift, int width, int height)
{
    int source_depth = source->depth;
    int source_bit = source->x * source_depth;
    const byte *source_row = source->data.read + (source_bit >> 3);
    int dest_depth = dest->depth;
    uint plane_mask = (1 << dest_depth) - 1;
    int dest_bit = dest->x * dest_depth;
    byte *dest_row = dest->data.write + (dest_bit >> 3);
    enum {
	EXTRACT_SLOW = 0,
	EXTRACT_4_TO_1,
	EXTRACT_32_TO_8
    } loop_case = EXTRACT_SLOW;
    int y;

    source_bit &= 7;
    dest_bit &= 7;
    /* Check for the fast CMYK cases. */
    if (!(source_bit | dest_bit)) {
	switch (source_depth) {
	case 4:
	    loop_case =
		(dest_depth == 1 && !(source->raster & 3) &&
		 !(source->x & 1) ? EXTRACT_4_TO_1 :
		 EXTRACT_SLOW);
	    break;
	case 32:
	    if (dest_depth == 8 && !(shift & 7)) {
		loop_case = EXTRACT_32_TO_8;
		source_row += 3 - (shift >> 3);
	    }
	    break;
	}
    }
    for (y = 0; y < height;
	 ++y, source_row += source->raster, dest_row += dest->raster
	) {
	int x;

	switch (loop_case) {
	case EXTRACT_4_TO_1: {
	    const byte *source = source_row;
	    byte *dest = dest_row;

	    /* Do groups of 8 pixels. */
	    for (x = width; x >= 8; source += 4, x -= 8) {
		bits32 sword =
		    (*(const bits32 *)source >> shift) & 0x11111111;

		*dest++ =
		    byte_acegbdfh_to_abcdefgh[(
#if arch_is_big_endian
		    (sword >> 21) | (sword >> 14) | (sword >> 7) | sword
#else
		    (sword << 3) | (sword >> 6) | (sword >> 15) | (sword >> 24)
#endif
					) & 0xff];
	    }
	    if (x) {
		/* Do the final 1-7 pixels. */
		uint test = 0x10 << shift, store = 0x80;

		do {
		    *dest = (*source & test ? *dest | store : *dest & ~store);
		    if (test >= 0x10)
			test >>= 4;
		    else
			test <<= 4, ++source;
		    store >>= 1;
		} while (--x > 0);
	    }
	    break;
	}
	case EXTRACT_32_TO_8: {
	    const byte *source = source_row;
	    byte *dest = dest_row;

	    for (x = width; x > 0; source += 4, --x)
		*dest++ = *source;
	    break;
	}
	default: {
	    sample_load_declare_setup(sptr, sbit, source_row, source_bit,
				      source_depth);
	    sample_store_declare_setup(dptr, dbit, dbbyte, dest_row, dest_bit,
				       dest_depth);

	    sample_store_preload(dbbyte, dptr, dbit, dest_depth);
	    for (x = width; x > 0; --x) {
		bits32 color;
		uint pixel;

		sample_load_next32(color, sptr, sbit, source_depth);
		pixel = (color >> shift) & plane_mask;
		sample_store_next8(pixel, dptr, dbit, dest_depth, dbbyte);
	    }
	    sample_store_flush(dptr, dbit, dest_depth, dbbyte);
	}
	}
    }
    return 0;
}

/* Expand a plane into a pixmap. */
int
bits_expand_plane(const bits_plane_t *dest /*write*/,
    const bits_plane_t *source /*read*/, int shift, int width, int height)
{
    /*
     * Eventually we will optimize this just like bits_extract_plane.
     */
    int source_depth = source->depth;
    int source_bit = source->x * source_depth;
    const byte *source_row = source->data.read + (source_bit >> 3);
    int dest_depth = dest->depth;
    int dest_bit = dest->x * dest_depth;
    byte *dest_row = dest->data.write + (dest_bit >> 3);
    enum {
	EXPAND_SLOW = 0,
	EXPAND_1_TO_4,
	EXPAND_8_TO_32
    } loop_case = EXPAND_SLOW;
    int y;

    source_bit &= 7;
    /* Check for the fast CMYK cases. */
    if (!(source_bit || (dest_bit & 31) || (dest->raster & 3))) {
	switch (dest_depth) {
	case 4:
	    if (source_depth == 1)
		loop_case = EXPAND_1_TO_4;
	    break;
	case 32:
	    if (source_depth == 8 && !(shift & 7))
		loop_case = EXPAND_8_TO_32;
	    break;
	}
    }
    dest_bit &= 7;
    switch (loop_case) {

    case EXPAND_8_TO_32: {
#if arch_is_big_endian
#  define word_shift (shift)
#else
	int word_shift = 24 - shift;
#endif
	for (y = 0; y < height;
	     ++y, source_row += source->raster, dest_row += dest->raster
	     ) {
	    int x;
	    const byte *source = source_row;
	    bits32 *dest = (bits32 *)dest_row;

	    for (x = width; x > 0; --x)
		*dest++ = (bits32)(*source++) << word_shift;
	}
#undef word_shift
    }
	break;

    case EXPAND_1_TO_4:
    default:
	for (y = 0; y < height;
	     ++y, source_row += source->raster, dest_row += dest->raster
	     ) {
	    int x;
	    sample_load_declare_setup(sptr, sbit, source_row, source_bit,
				      source_depth);
	    sample_store_declare_setup(dptr, dbit, dbbyte, dest_row, dest_bit,
				       dest_depth);

	    sample_store_preload(dbbyte, dptr, dbit, dest_depth);
	    for (x = width; x > 0; --x) {
		uint color;
		bits32 pixel;

		sample_load_next8(color, sptr, sbit, source_depth);
		pixel = color << shift;
		sample_store_next32(pixel, dptr, dbit, dest_depth, dbbyte);
	    }
	    sample_store_flush(dptr, dbit, dest_depth, dbbyte);
	}
	break;

    }
    return 0;
}

/* ---------------- Byte-oriented operations ---------------- */

/* Fill a rectangle of bytes. */
void
bytes_fill_rectangle(byte * dest, uint raster,
		     byte value, int width_bytes, int height)
{
    while (height-- > 0) {
	memset(dest, value, width_bytes);
	dest += raster;
    }
}

/* Copy a rectangle of bytes. */
void
bytes_copy_rectangle(byte * dest, uint dest_raster,
	     const byte * src, uint src_raster, int width_bytes, int height)
{
    while (height-- > 0) {
	memcpy(dest, src, width_bytes);
	src += src_raster;
	dest += dest_raster;
    }
}
