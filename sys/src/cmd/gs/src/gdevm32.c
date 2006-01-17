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

/* $Id: gdevm32.c,v 1.5 2003/05/24 17:19:58 dan Exp $ */
/* 32-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */

/* ================ Standard (byte-oriented) device ================ */

#undef chunk
#define chunk byte

/* Procedures */
declare_mem_procs(mem_true32_copy_mono, mem_true32_copy_color, mem_true32_fill_rectangle);

/* The device descriptor. */
const gx_device_memory mem_true32_device =
mem_full_device("image32", 24, 8, mem_open,
		gx_default_map_rgb_color, gx_default_map_color_rgb,
     mem_true32_copy_mono, mem_true32_copy_color, mem_true32_fill_rectangle,
	    gx_default_cmyk_map_cmyk_color, gx_default_strip_tile_rectangle,
		mem_default_strip_copy_rop, mem_get_bits_rectangle);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) << 2)

/* Swap the bytes of a color if needed. */
#define color_swap_bytes(color)\
  ((((color) >> 24) & 0xff) + (((color) >> 8) & 0xff00) +\
   (((color) & 0xff00) << 8) + ((color) << 24))
#if arch_is_big_endian
#  define arrange_bytes(color) (color)
#else
#  define arrange_bytes(color) color_swap_bytes(color)
#endif

/* Fill a rectangle with a color. */
private int
mem_true32_fill_rectangle(gx_device * dev,
			  int x, int y, int w, int h, gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    bits32 a_color;

    declare_scan_ptr(dest);

    fit_fill(dev, x, y, w, h);
    a_color = arrange_bytes(color);
    setup_rect(dest);
    if (w <= 4)
	switch (w) {
		/*case 0: *//* not possible */
#define dest32 ((bits32 *)dest)
	    case 1:
		do {
		    dest32[0] = a_color;
		    inc_ptr(dest, draster);
		}
		while (--h > 0);
		break;
	    case 2:
		do {
		    dest32[1] = dest32[0] = a_color;
		    inc_ptr(dest, draster);
		}
		while (--h > 0);
		break;
	    case 3:
		do {
		    dest32[2] = dest32[1] = dest32[0] = a_color;
		    inc_ptr(dest, draster);
		}
		while (--h > 0);
		break;
	    case 4:
		do {
		    dest32[3] = dest32[2] = dest32[1] = dest32[0] = a_color;
		    inc_ptr(dest, draster);
		}
		while (--h > 0);
		break;
	    default:		/* not possible */
		;
    } else if (a_color == 0)
	do {
	    memset(dest, 0, w << 2);
	    inc_ptr(dest, draster);
	}
	while (--h > 0);
    else
	do {
	    bits32 *pptr = dest32;
	    int cnt = w;

	    do {
		pptr[3] = pptr[2] = pptr[1] = pptr[0] = a_color;
		pptr += 4;
	    }
	    while ((cnt -= 4) > 4);
	    do {
		*pptr++ = a_color;
	    } while (--cnt > 0);
	    inc_ptr(dest, draster);
	}
	while (--h > 0);
#undef dest32
    return 0;
}

/* Copy a monochrome bitmap. */
private int
mem_true32_copy_mono(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
	int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    bits32 a_zero = arrange_bytes(zero);
    bits32 a_one = arrange_bytes(one);
    const byte *line;

    declare_scan_ptr(dest);
    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    setup_rect(dest);
    line = base + (sourcex >> 3);
    if (zero == gx_no_color_index) {
	int first_bit = sourcex & 7;
	int w_first = min(w, 8 - first_bit);
	int w_rest = w - w_first;

	if (one == gx_no_color_index)
	    return 0;
	/*
	 * There are no halftones, so this case -- characters --
	 * is the only common one.
	 */
	while (h-- > 0) {
	    bits32 *pptr = (bits32 *) dest;
	    const byte *sptr = line;
	    int sbyte = (*sptr++ << first_bit) & 0xff;
	    int count = w_first;

	    if (sbyte)
		do {
		    if (sbyte & 0x80)
			*pptr = a_one;
		    sbyte <<= 1;
		    pptr++;
		}
		while (--count > 0);
	    else
		pptr += count;
	    for (count = w_rest; count >= 8; count -= 8, pptr += 8) {
		sbyte = *sptr++;
		if (sbyte) {
		    if (sbyte & 0x80) pptr[0] = a_one;
		    if (sbyte & 0x40) pptr[1] = a_one;
		    if (sbyte & 0x20) pptr[2] = a_one;
		    if (sbyte & 0x10) pptr[3] = a_one;
		    if (sbyte & 0x08) pptr[4] = a_one;
		    if (sbyte & 0x04) pptr[5] = a_one;
		    if (sbyte & 0x02) pptr[6] = a_one;
		    if (sbyte & 0x01) pptr[7] = a_one;
		}
	    }
	    if (count) {
		sbyte = *sptr;
		do {
		    if (sbyte & 0x80)
			*pptr = a_one;
		    sbyte <<= 1;
		    pptr++;
		}
		while (--count > 0);
	    }
	    line += sraster;
	    inc_ptr(dest, draster);
	}
    } else {			/* zero != gx_no_color_index */
	int first_bit = 0x80 >> (sourcex & 7);

	while (h-- > 0) {
	    bits32 *pptr = (bits32 *) dest;
	    const byte *sptr = line;
	    int sbyte = *sptr++;
	    int bit = first_bit;
	    int count = w;

	    do {
		if (sbyte & bit) {
		    if (one != gx_no_color_index)
			*pptr = a_one;
		} else
		    *pptr = a_zero;
		if ((bit >>= 1) == 0)
		    bit = 0x80, sbyte = *sptr++;
		pptr++;
	    }
	    while (--count > 0);
	    line += sraster;
	    inc_ptr(dest, draster);
	}
    }
    return 0;
}

/* Copy a color bitmap. */
private int
mem_true32_copy_color(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    mem_copy_byte_rect(mdev, base, sourcex, sraster, x, y, w, h);
    return 0;
}

/* ================ "Word"-oriented device ================ */

/* Note that on a big-endian machine, this is the same as the */
/* standard byte-oriented-device. */

#if !arch_is_big_endian

/* Procedures */
declare_mem_procs(mem32_word_copy_mono, mem32_word_copy_color, mem32_word_fill_rectangle);

/* Here is the device descriptor. */
const gx_device_memory mem_true32_word_device =
mem_full_device("image32w", 24, 8, mem_open,
		gx_default_map_rgb_color, gx_default_map_color_rgb,
     mem32_word_copy_mono, mem32_word_copy_color, mem32_word_fill_rectangle,
	    gx_default_cmyk_map_cmyk_color, gx_default_strip_tile_rectangle,
		gx_no_strip_copy_rop, mem_word_get_bits_rectangle);

/* Fill a rectangle with a color. */
private int
mem32_word_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			  gx_color_index color)
{
    return mem_true32_fill_rectangle(dev, x, y, w, h,
				     color_swap_bytes(color));
}

/* Copy a bitmap. */
private int
mem32_word_copy_mono(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
	int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{
    return mem_true32_copy_mono(dev, base, sourcex, sraster, id,
				x, y, w, h, color_swap_bytes(zero),
				color_swap_bytes(one));
}

/* Copy a color bitmap. */
private int
mem32_word_copy_color(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *row;
    uint raster;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    row = scan_line_base(mdev, y);
    raster = mdev->raster;
    bytes_copy_rectangle(row + (x << 2), raster, base + (sourcex << 2),
			 sraster, w << 2, h);
    mem_swap_byte_rect(row, raster, x << 5, w << 5, h, false);
    return 0;
}

#endif /* !arch_is_big_endian */
