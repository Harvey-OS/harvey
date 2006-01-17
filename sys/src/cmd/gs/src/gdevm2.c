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

/* $Id: gdevm2.c,v 1.5 2002/11/05 01:03:14 dan Exp $ */
/* 2-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */

/* ================ Standard (byte-oriented) device ================ */

#undef chunk
#define chunk byte
#define fpat(byt) mono_fill_make_pattern(byt)

/* Procedures */
declare_mem_procs(mem_mapped2_copy_mono, mem_mapped2_copy_color, mem_mapped2_fill_rectangle);

/* The device descriptor. */
const gx_device_memory mem_mapped2_device =
mem_device("image2", 2, 0,
	   mem_mapped_map_rgb_color, mem_mapped_map_color_rgb,
	   mem_mapped2_copy_mono, mem_mapped2_copy_color,
	   mem_mapped2_fill_rectangle, mem_gray_strip_copy_rop);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) >> 2)

/* Define the 2-bit fill patterns. */
static const mono_fill_chunk tile_patterns[4] = {
    fpat(0x00), fpat(0x55), fpat(0xaa), fpat(0xff)
};

/* Fill a rectangle with a color. */
private int
mem_mapped2_fill_rectangle(gx_device * dev,
			   int x, int y, int w, int h, gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;

    fit_fill(dev, x, y, w, h);
    bits_fill_rectangle(scan_line_base(mdev, y), x << 1, mdev->raster,
			tile_patterns[color], w << 1, h);
    return 0;
}

/* Copy a bitmap. */
private int
mem_mapped2_copy_mono(gx_device * dev,
		      const byte * base, int sourcex, int sraster,
		      gx_bitmap_id id, int x, int y, int w, int h,
		      gx_color_index zero, gx_color_index one)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    const byte *line;
    int first_bit;
    byte first_mask, b0, b1, bxor, left_mask, right_mask;
    static const byte btab[4] = {0, 0x55, 0xaa, 0xff};
    static const byte bmask[4] = {0xc0, 0x30, 0xc, 3};
    static const byte lmask[4] = {0, 0xc0, 0xf0, 0xfc};

    declare_scan_ptr(dest);

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    setup_rect(dest);
    line = base + (sourcex >> 3);
    first_bit = 0x80 >> (sourcex & 7);
    first_mask = bmask[x & 3];
    left_mask = lmask[x & 3];
    right_mask = ~lmask[(x + w) & 3];
    if ((x & 3) + w <= 3)
	left_mask = right_mask = left_mask | right_mask;
    b0 = btab[zero & 3];
    b1 = btab[one & 3];
    bxor = b0 ^ b1;
    while (h-- > 0) {
	register byte *pptr = (byte *) dest;
	const byte *sptr = line;
	register int sbyte = *sptr++;
	register int bit = first_bit;
	register byte mask = first_mask;
	int count = w;

	/* We have 4 cases, of which only 2 really matter. */
	if (one != gx_no_color_index) {
	    if (zero != gx_no_color_index) {	/* Copying an opaque bitmap. */
		byte data = (*pptr & left_mask) | (b0 & ~left_mask);

		for ( ; ; ) {
		    if (sbyte & bit)
			data ^= bxor & mask;
		    if ((bit >>= 1) == 0)
			bit = 0x80, sbyte = *sptr++;
		    if ((mask >>= 2) == 0)
			mask = 0xc0, *pptr++ = data, data = b0;
		    if (--count <= 0)
			break;
		}
		if (mask != 0xc0)
		    *pptr =
			(*pptr & right_mask) | (data & ~right_mask);
	    } else {		/* Filling a mask. */
		for ( ; ; ) {
		    if (sbyte & bit)
			*pptr = (*pptr & ~mask) + (b1 & mask);
		    if (--count <= 0)
			break;
		    if ((bit >>= 1) == 0)
			bit = 0x80, sbyte = *sptr++;
		    if ((mask >>= 2) == 0)
			mask = 0xc0, pptr++;
		}
	    }
	} else {		/* Some other case. */
	    for ( ; ; ) {
		if (!(sbyte & bit)) {
		    if (zero != gx_no_color_index)
			*pptr = (*pptr & ~mask) + (b0 & mask);
		}
		if (--count <= 0)
		    break;
		if ((bit >>= 1) == 0)
		    bit = 0x80, sbyte = *sptr++;
		if ((mask >>= 2) == 0)
		    mask = 0xc0, pptr++;
	    }
	}
	line += sraster;
	inc_ptr(dest, draster);
    }
    return 0;
}

/* Copy a color bitmap. */
private int
mem_mapped2_copy_color(gx_device * dev,
		       const byte * base, int sourcex, int sraster,
		       gx_bitmap_id id, int x, int y, int w, int h)
{
    int code;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    /* Use monobit copy_mono. */
    /* Patch the width in the device temporarily. */
    dev->width <<= 1;
    code = (*dev_proc(&mem_mono_device, copy_mono))
	(dev, base, sourcex << 1, sraster, id,
	 x << 1, y, w << 1, h, (gx_color_index) 0, (gx_color_index) 1);
    /* Restore the correct width. */
    dev->width >>= 1;
    return code;
}

/* ================ "Word"-oriented device ================ */

/* Note that on a big-endian machine, this is the same as the */
/* standard byte-oriented-device. */

#if !arch_is_big_endian

/* Procedures */
declare_mem_procs(mem2_word_copy_mono, mem2_word_copy_color, mem2_word_fill_rectangle);

/* Here is the device descriptor. */
const gx_device_memory mem_mapped2_word_device =
mem_full_device("image2w", 2, 0, mem_open,
		mem_mapped_map_rgb_color, mem_mapped_map_color_rgb,
		mem2_word_copy_mono, mem2_word_copy_color,
		mem2_word_fill_rectangle, gx_default_map_cmyk_color,
		gx_default_strip_tile_rectangle, gx_no_strip_copy_rop,
		mem_word_get_bits_rectangle);

/* Fill a rectangle with a color. */
private int
mem2_word_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			 gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *base;
    uint raster;

    fit_fill(dev, x, y, w, h);
    base = scan_line_base(mdev, y);
    raster = mdev->raster;
    mem_swap_byte_rect(base, raster, x << 1, w << 1, h, true);
    bits_fill_rectangle(base, x << 1, raster,
			tile_patterns[color], w << 1, h);
    mem_swap_byte_rect(base, raster, x << 1, w << 1, h, true);
    return 0;
}

/* Copy a bitmap. */
private int
mem2_word_copy_mono(gx_device * dev,
		    const byte * base, int sourcex, int sraster,
		    gx_bitmap_id id, int x, int y, int w, int h,
		    gx_color_index zero, gx_color_index one)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *row;
    uint raster;
    bool store;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    row = scan_line_base(mdev, y);
    raster = mdev->raster;
    store = (zero != gx_no_color_index && one != gx_no_color_index);
    mem_swap_byte_rect(row, raster, x << 1, w << 1, h, store);
    mem_mapped2_copy_mono(dev, base, sourcex, sraster, id,
			  x, y, w, h, zero, one);
    mem_swap_byte_rect(row, raster, x << 1, w << 1, h, false);
    return 0;
}

/* Copy a color bitmap. */
private int
mem2_word_copy_color(gx_device * dev,
		     const byte * base, int sourcex, int sraster,
		     gx_bitmap_id id, int x, int y, int w, int h)
{
    int code;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    /* Use monobit copy_mono. */
    /* Patch the width in the device temporarily. */
    dev->width <<= 1;
    code = (*dev_proc(&mem_mono_word_device, copy_mono))
	(dev, base, sourcex << 1, sraster, id,
	 x << 1, y, w << 1, h, (gx_color_index) 0, (gx_color_index) 1);
    /* Restore the correct width. */
    dev->width >>= 1;
    return code;
}

#endif /* !arch_is_big_endian */
