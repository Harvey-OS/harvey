/* Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsbitops.c,v 1.8 2002/10/07 08:28:56 ghostgum Exp $ */
/* Bitmap filling, copying, and transforming operations */
#include "stdio_.h"
#include "memory_.h"
#include "gdebug.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsbittab.h"
#include "gxbitops.h"
#include "gxcindex.h"

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
	if (pattern == 0)
	    FOR_EACH_LINE(*ptr &= ~right_mask;);
	else if (pattern == (mono_fill_chunk)(-1))
	    FOR_EACH_LINE(*ptr |= right_mask;);
	else
	    FOR_EACH_LINE(
		*ptr = (*ptr & ~right_mask) | (pattern & right_mask); );
    } else {
	chunk mask;
	int last = last_bit >> chunk_log2_bits;

	set_mono_left_mask(mask, bit);
	set_mono_right_mask(right_mask, (last_bit & chunk_bit_mask) + 1);
	switch (last) {
	    case 0:		/* 2 chunks */
		if (pattern == 0)
		    FOR_EACH_LINE(*ptr &= ~mask; ptr[1] &= ~right_mask;);
		else if (pattern == (mono_fill_chunk)(-1))
		    FOR_EACH_LINE(*ptr |= mask; ptr[1] |= right_mask;);
		else
		    FOR_EACH_LINE(
		        *ptr = (*ptr & ~mask) | (pattern & mask);
			ptr[1] = (ptr[1] & ~right_mask) | (pattern & right_mask); );
		break;
	    case 1:		/* 3 chunks */
		if (pattern == 0)
		    FOR_EACH_LINE( *ptr &= ~mask;
				   ptr[1] = 0;
				   ptr[2] &= ~right_mask; );
		else if (pattern == (mono_fill_chunk)(-1))
		    FOR_EACH_LINE( *ptr |= mask;
				   ptr[1] = ~(chunk) 0;
				   ptr[2] |= right_mask; );
		else
		    FOR_EACH_LINE( *ptr = (*ptr & ~mask) | (pattern & mask);
				    ptr[1] = pattern;
				    ptr[2] = (ptr[2] & ~right_mask) | (pattern & right_mask); );
		break;
	    default:{		/* >3 chunks */
		    uint byte_count = (last_bit >> 3) & -chunk_bytes;

		    if (pattern == 0)
			FOR_EACH_LINE( *ptr &= ~mask;
				       memset(ptr + 1, 0, byte_count);
				       ptr[last + 1] &= ~right_mask; );
		    else if (pattern == (mono_fill_chunk)(-1))
			FOR_EACH_LINE( *ptr |= mask;
				       memset(ptr + 1, 0xff, byte_count);
				       ptr[last + 1] |= right_mask; );
		    else
			FOR_EACH_LINE(
				*ptr = (*ptr & ~mask) | (pattern & mask);
				memset(ptr + 1, (byte) pattern, byte_count);
				ptr[last + 1] = (ptr[last + 1] & ~right_mask) |
					        (pattern & right_mask); 	);
		}
	}
    }
#undef FOR_EACH_LINE
}

/*
 * Similar to bits_fill_rectangle, but with an additional source mask.
 * The src_mask variable is 1 for those bits of the original that are
 * to be retained. The mask argument must consist of the requisite value
 * in every byte, in the same manner as the pattern.
 */
void
bits_fill_rectangle_masked(byte * dest, int dest_bit, uint draster,
		    mono_fill_chunk pattern, mono_fill_chunk src_mask,
		    int width_bits, int height)
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
	right_mask &= ~src_mask;
	if (pattern == 0)
	    FOR_EACH_LINE(*ptr &= ~right_mask;);
	else if (pattern == (mono_fill_chunk)(-1))
	    FOR_EACH_LINE(*ptr |= right_mask;);
	else
	    FOR_EACH_LINE(
		*ptr = (*ptr & ~right_mask) | (pattern & right_mask); );
    } else {
	chunk mask;
	int last = last_bit >> chunk_log2_bits;

	set_mono_left_mask(mask, bit);
	set_mono_right_mask(right_mask, (last_bit & chunk_bit_mask) + 1);
	mask &= ~src_mask;
	right_mask &= ~src_mask;
	switch (last) {
	    case 0:		/* 2 chunks */
		if (pattern == 0)
		    FOR_EACH_LINE(*ptr &= ~mask; ptr[1] &= ~right_mask;);
		else if (pattern == (mono_fill_chunk)(-1))
		    FOR_EACH_LINE(*ptr |= mask; ptr[1] |= right_mask;);
		else
		    FOR_EACH_LINE(
		        *ptr = (*ptr & ~mask) | (pattern & mask);
			ptr[1] = (ptr[1] & ~right_mask) | (pattern & right_mask); );
		break;
	    case 1:		/* 3 chunks */
		if (pattern == 0)
		    FOR_EACH_LINE( *ptr &= ~mask;
				   ptr[1] &= src_mask;
				   ptr[2] &= ~right_mask; );
		else if (pattern == (mono_fill_chunk)(-1))
		    FOR_EACH_LINE( *ptr |= mask;
				   ptr[1] |= ~src_mask;
				   ptr[2] |= right_mask; );
		else
		    FOR_EACH_LINE( *ptr = (*ptr & ~mask) | (pattern & mask);
				    ptr[1] =(ptr[1] & src_mask) | pattern;
				    ptr[2] = (ptr[2] & ~right_mask) | (pattern & right_mask); );
		break;
	    default:{		/* >3 chunks */
                    int     i;

		    if (pattern == 0)
			FOR_EACH_LINE( *ptr++ &= ~mask;
				       for (i = 0; i < last; i++)
					   *ptr++ &= src_mask;
				       *ptr &= ~right_mask; );
		    else if (pattern == (mono_fill_chunk)(-1))
			FOR_EACH_LINE( *ptr++ |= mask;
				       for (i = 0; i < last; i++)
					   *ptr++ |= ~src_mask;
					*ptr |= right_mask; );
		    else
			FOR_EACH_LINE(
			    /* note: we know (pattern & ~src_mask) == pattern */
			    *ptr = (*ptr & ~mask) | (pattern & mask);
			    ++ptr;
			    for (i = 0; i < last; i++, ptr++)
                                *ptr = (*ptr & src_mask) | pattern;
				*ptr = (*ptr & ~right_mask) | (pattern & right_mask); );
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
	uint bit_count = width & (uint)(-(int)width);  /* lowest bit: 1, 2, or 4 */
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
		gx_color_index color;
		uint pixel;

		sample_load_next_any(color, sptr, sbit, source_depth);
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
		gx_color_index pixel;

		sample_load_next8(color, sptr, sbit, source_depth);
		pixel = color << shift;
		sample_store_next_any(pixel, dptr, dbit, dest_depth, dbbyte);
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
