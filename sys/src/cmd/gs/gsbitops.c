/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsbitops.c */
/* Bitmap filling, copying, and transforming operations */
#include "stdio_.h"
#include "memory_.h"
#include "gstypes.h"
#include "gsbitops.h"

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
#  if arch_sizeof_int > 2
const bits32 mono_fill_masks[33] = {
	0xffffffff, 0xffffff7f, 0xffffff3f, 0xffffff1f,
	0xffffff0f, 0xffffff07, 0xffffff03, 0xffffff01,
	0xffffff00, 0xffff7f00, 0xffff3f00, 0xffff1f00,
	0xffff0f00, 0xffff0700, 0xffff0300, 0xffff0100,
	0xffff0000, 0xff7f0000, 0xff3f0000, 0xff1f0000,
	0xff0f0000, 0xff070000, 0xff030000, 0xff010000,
	0xff000000, 0x7f000000, 0x3f000000, 0x1f000000,
	0x0f000000, 0x07000000, 0x03000000, 0x01000000,
	0x00000000
};
#  endif
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
bits_fill_rectangle(byte *dest, int dest_bit, uint draster,
  mono_fill_chunk pattern, int width_bits, int height)
{	uint bit;
	chunk right_mask;

#define write_loop(stat)\
 { int line_count = height;\
   chunk *ptr = (chunk *)dest;\
   do { stat; inc_ptr(ptr, draster); }\
   while ( --line_count );\
 }

#define write_partial(msk)\
  switch ( (byte)pattern )\
  { case 0: write_loop(*ptr &= ~msk); break;\
    case 0xff: write_loop(*ptr |= msk); break;\
    default: write_loop(*ptr = (*ptr & ~msk) | (pattern & msk));\
  }

	dest += (dest_bit >> 3) & -chunk_align_bytes;
	bit = dest_bit & chunk_align_bit_mask;
	if ( bit + width_bits <= chunk_bits )
	   {	/* Only one word. */
		set_mono_thin_mask(right_mask, width_bits, bit);
	   }
	else
	   {	int byte_count;
		if ( bit )
		   {	chunk mask;
			set_mono_left_mask(mask, bit);
			write_partial(mask);
			inc_ptr(dest, chunk_bytes);
			width_bits += bit - chunk_bits;
		   }
		set_mono_right_mask(right_mask, width_bits & chunk_bit_mask);
		if ( width_bits >= chunk_bits )
		  switch ( (byte_count = (width_bits >> 3) & -chunk_bytes) )
		{
		case chunk_bytes:
			write_loop(*ptr = pattern);
			inc_ptr(dest, chunk_bytes);
			break;
		case chunk_bytes * 2:
			write_loop(ptr[1] = *ptr = pattern);
			inc_ptr(dest, chunk_bytes * 2);
			break;
		default:
			write_loop(memset(ptr, (byte)pattern, byte_count));
			inc_ptr(dest, byte_count);
			break;
		}
	   }
	if ( right_mask )
		write_partial(right_mask);
}

/* Count the number of 1-bits in a half-byte. */
static const byte half_byte_1s[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};

/*
 * Compress a value between 0 and 2^M to a value between 0 and 2^N-1.
 * Possible values of M are 1, 2, 3, or 4; of N are 1, 2, and 4.
 * The name of the table is compress_count_M_N.
 * As noted below, we require that N <= M.
 */
static const byte compress_1_1[3] =
	{ 0, 1, 1 };
static const byte compress_2_1[5] =
	{ 0, 0, 1, 1, 1 };
static const byte compress_2_2[5] =
	{ 0, 1, 2, 2, 3 };
static const byte compress_3_1[9] =
	{ 0, 0, 0, 0, 1, 1, 1, 1, 1 };
static const byte compress_3_2[9] =
	{ 0, 0, 1, 1, 2, 2, 2, 3, 3 };
static const byte compress_4_1[17] =
	{ 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static const byte compress_4_2[17] =
	{ 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3 };
static const byte compress_4_4[17] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9,10,11,12,13,14,15 };
/* The table of tables is indexed by log2(N) and then by M-1. */
static const byte _ds *compress_tables[4][4] =
{	{ compress_1_1, compress_2_1, compress_3_1, compress_4_1 },
	{ 0, compress_2_2, compress_3_2, compress_4_2 },
	{ 0, 0, 0, compress_4_4 }
};

/*
 * Compress a 1/2/4x1/2/4-oversampled bitmap to Nx1 by counting 1-bits.
 * The X and Y oversampling factors are 1, 2, or 4, but may be different;
 * N, the resulting number of (alpha) bits per pixel, may be 1, 2, or 4;
 * we allow compression in place, in which case N must not exceed
 * the X oversampling factor.  Width and height are the source dimensions,
 * and hence reflect the oversampling; both are multiples of
 * the relevant scale factor.
 */
void
bits_compress_scaled(const byte *src, uint width, uint height, uint sraster,
  byte *dest, uint draster,
  const gs_log2_scale_point *plog2_scale, int log2_out_bits)
{	int xscale = 1 << plog2_scale->x;
	int yscale = 1 << plog2_scale->y;
	int out_bits = 1 << log2_out_bits;
	const byte _ds *table =
	  compress_tables[log2_out_bits][plog2_scale->x + plog2_scale->y - 1];
	uint sskip = sraster * yscale;
	uint dwidth = width / xscale * out_bits;
	uint dskip = draster - ((dwidth + 7) >> 3);
	uint mask = (1 << xscale) - 1;
	const byte *srow = src;
	byte *d = dest;
	uint h;

	for ( h = height; h; h -= yscale )
	{	const byte *s = srow;
		int out_shift = 8 - out_bits;
		byte out = 0;
		int in_shift = 8 - xscale;
		uint w;

		for ( w = width; w; w -= xscale )
		{	uint count = 0;
			uint index;
			for ( index = 0; index != sskip; index += sraster )
			  count += half_byte_1s[(s[index] >> in_shift) & mask];
			out += table[count] << out_shift;
			if ( (in_shift -= xscale) < 0 )
			  s++, in_shift += 8;
			if ( (out_shift -= out_bits) < 0 )
			  *d++ = out, out_shift += 8, out = 0;
		}
		if ( out_shift != 8 - out_bits )
			*d++ = out;
		for ( w = dskip; w != 0; w-- )
			*d++ = 0;
		srow += sskip;
	}
}

/* ---------------- Byte-oriented operations ---------------- */

/* Fill a rectangle of bytes. */
void
bytes_fill_rectangle(byte *dest, uint raster,
  byte value, int width_bytes, int height)
{	while ( height-- > 0 )
	  {	memset(dest, value, width_bytes);
		dest += raster;
	  }
}

/* Copy a rectangle of bytes. */
void
bytes_copy_rectangle(byte *dest, uint dest_raster,
  const byte *src, uint src_raster, int width_bytes, int height)
{	while ( height-- > 0 )
	  {	memcpy(dest, src, width_bytes);
		src += src_raster;
		dest += dest_raster;
	  }
}
