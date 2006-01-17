/* Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises, 2001 Artifex Software.  All rights reserved.
  
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

/*$Id: gdevm64.c,v 1.4 2005/06/20 08:59:23 igor Exp $ */
/* 64-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */

/* Define debugging statistics. */
#ifdef DEBUG
struct stats_mem64_s {
    long
	fill, fwide, fgray[101], fsetc, fcolor[101], fnarrow[5],
	fprevc[257];
    double ftotal;
} stats_mem64;
static int prev_count = 0;
static gx_color_index prev_colors[256];
# define INCR(v) (++(stats_mem64.v))
#else
# define INCR(v) DO_NOTHING
#endif


/* ================ Standard (byte-oriented) device ================ */

#undef chunk
#define chunk byte
#define PIXEL_SIZE 2

/* Procedures */
declare_mem_procs(mem_true64_copy_mono, mem_true64_copy_color, mem_true64_fill_rectangle);

/* The device descriptor. */
const gx_device_memory mem_true64_device =
mem_full_alpha_device("image64", 64, 0, mem_open,
		 gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb,
     mem_true64_copy_mono, mem_true64_copy_color, mem_true64_fill_rectangle,
		      gx_default_map_cmyk_color, gx_default_copy_alpha,
		 gx_default_strip_tile_rectangle, mem_default_strip_copy_rop,
		      mem_get_bits_rectangle);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) << 3)

/* Put a 64-bit color into the bitmap. */
#define put8(ptr, abcd, efgh)\
	(ptr)[0] = abcd, (ptr)[1] = efgh
/* Free variables: [m]dev, abcd, degh. */
#if arch_is_big_endian
/* Unpack a color into 32 bit chunks. */
#  define declare_unpack_color(abcd, efgh, color)\
	bits32 abcd = (bits32)((color) >> 32);\
	bits32 efgh = (bits32)(color)
#else
/* Unpack a color into 32 bit chunks. */
#  define declare_unpack_color(abcd, efgh, color)\
	bits32 abcd = (bits32)((0x000000ff & ((color) >> 56)) |\
		               (0x0000ff00 & ((color) >> 40)) |\
		               (0x00ff0000 & ((color) >> 24)) |\
		               (0xff000000 & ((color) >> 8)));\
	bits32 efgh = (bits32)((0x000000ff & ((color) >> 24)) |\
		               (0x0000ff00 & ((color) >> 8)) |\
		               (0x00ff0000 & ((color) << 8)) |\
		               (0xff000000 & ((color) << 24)))
#endif
#define dest32 ((bits32 *)dest)

/* Fill a rectangle with a color. */
private int
mem_true64_fill_rectangle(gx_device * dev,
			  int x, int y, int w, int h, gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    declare_scan_ptr(dest);
    declare_unpack_color(abcd, efgh, color);

    /*
     * In order to avoid testing w > 0 and h > 0 twice, we defer
     * executing setup_rect, and use fit_fill_xywh instead of
     * fit_fill.
     */
    fit_fill_xywh(dev, x, y, w, h);
    INCR(fill);
#ifdef DEBUG
    stats_mem64.ftotal += w;
#endif
    if (h <= 0)
	return 0;
    if (w >= 5) {
	INCR(fwide);
	setup_rect(dest);
#ifdef DEBUG
	{
	    int ci;
	    for (ci = 0; ci < prev_count; ++ci)
		if (prev_colors[ci] == color)
	    	    break;
	    INCR(fprevc[ci]);
	    if (ci == prev_count) {
		if (ci < countof(prev_colors))
	    	    ++prev_count;
		else
	    	    --ci;
	    }
	    if (ci) {
		memmove(&prev_colors[1], &prev_colors[0],
			ci * sizeof(prev_colors[0]));
		prev_colors[0] = color;
	    }
	}
#endif
	INCR(fcolor[min(w, 100)]);
	while (h-- > 0) {
	    register bits32 *pptr = dest32;
	    int w1 = w;

	    while (w1 >= 4) {
		put8(pptr, abcd, efgh);
		put8(pptr + 2, abcd, efgh);
		put8(pptr + 4, abcd, efgh);
		put8(pptr + 6, abcd, efgh);
		pptr += 4 * PIXEL_SIZE;
		w1 -= 4;
	    }
	    switch (w1) {
		case 1:
		    put8(pptr, abcd, efgh);
		    break;
		case 2:
		    put8(pptr, abcd, efgh);
		    put8(pptr + 2, abcd, efgh);
		    break;
		case 3:
		    put8(pptr, abcd, efgh);
		    put8(pptr + 2, abcd, efgh);
		    put8(pptr + 4, abcd, efgh);
		    break;
		case 0:
		    ;
	    }
	    inc_ptr(dest, draster);
	}
    } else {		/* w < 5 */
	INCR(fnarrow[max(w, 0)]);
	setup_rect(dest);
	switch (w) {
	    case 4:
		do {
		    put8(dest32, abcd, efgh);
		    put8(dest32 + 2, abcd, efgh);
		    put8(dest32 + 4, abcd, efgh);
		    put8(dest32 + 6, abcd, efgh);
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 3:
		do {
		    put8(dest32, abcd, efgh);
		    put8(dest32 + 2, abcd, efgh);
		    put8(dest32 + 4, abcd, efgh);
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 2:
		do {
		    put8(dest32, abcd, efgh);
		    put8(dest32 + 2, abcd, efgh);
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 1:
		do {
		    put8(dest32, abcd, efgh);
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 0:
	    default:
		;
	}
    }
    return 0;
}

/* Copy a monochrome bitmap. */
private int
mem_true64_copy_mono(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
	int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    const byte *line;
    int sbit;
    int first_bit;

    declare_scan_ptr(dest);

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    setup_rect(dest);
    line = base + (sourcex >> 3);
    sbit = sourcex & 7;
    first_bit = 0x80 >> sbit;
    if (zero != gx_no_color_index) {	/* Loop for halftones or inverted masks */
	/* (never used). */
	declare_unpack_color(abcd0, efgh0, zero);
	declare_unpack_color(abcd1, efgh1, one);
	while (h-- > 0) {
	    register bits32 *pptr = dest32;
	    const byte *sptr = line;
	    register int sbyte = *sptr++;
	    register int bit = first_bit;
	    int count = w;

	    do {
		if (sbyte & bit) {
		    if (one != gx_no_color_index)
			put8(pptr, abcd1, efgh1);
		} else
		    put8(pptr, abcd0, efgh0);
		pptr += PIXEL_SIZE;
		if ((bit >>= 1) == 0)
		    bit = 0x80, sbyte = *sptr++;
	    }
	    while (--count > 0);
	    line += sraster;
	    inc_ptr(dest, draster);
	}
    } else if (one != gx_no_color_index) {	/* Loop for character and pattern masks. */
	/* This is used heavily. */
	declare_unpack_color(abcd1, efgh1, one);
	int first_mask = first_bit << 1;
	int first_count, first_skip;

	if (sbit + w > 8)
	    first_mask -= 1,
		first_count = 8 - sbit;
	else
	    first_mask -= first_mask >> w,
		first_count = w;
	first_skip = first_count * PIXEL_SIZE;
	while (h-- > 0) {
	    register bits32 *pptr = dest32;
	    const byte *sptr = line;
	    register int sbyte = *sptr++ & first_mask;
	    int count = w - first_count;

	    if (sbyte) {
		register int bit = first_bit;

		do {
		    if (sbyte & bit)
			put8(pptr, abcd1, efgh1);
		    pptr += PIXEL_SIZE;
		}
		while ((bit >>= 1) & first_mask);
	    } else
		pptr += first_skip;
	    while (count >= 8) {
		sbyte = *sptr++;
		if (sbyte & 0xf0) {
		    if (sbyte & 0x80)
			put8(pptr, abcd1, efgh1);
		    if (sbyte & 0x40)
			put8(pptr + 2, abcd1, efgh1);
		    if (sbyte & 0x20)
			put8(pptr + 4, abcd1, efgh1);
		    if (sbyte & 0x10)
			put8(pptr + 6, abcd1, efgh1);
		}
		if (sbyte & 0xf) {
		    if (sbyte & 8)
			put8(pptr + 8, abcd1, efgh1);
		    if (sbyte & 4)
			put8(pptr + 10, abcd1, efgh1);
		    if (sbyte & 2)
			put8(pptr + 12, abcd1, efgh1);
		    if (sbyte & 1)
			put8(pptr + 14, abcd1, efgh1);
		}
		pptr += 8 * PIXEL_SIZE;
		count -= 8;
	    }
	    if (count > 0) {
		register int bit = 0x80;

		sbyte = *sptr++;
		do {
		    if (sbyte & bit)
			put8(pptr, abcd1, efgh1);
		    pptr += PIXEL_SIZE;
		    bit >>= 1;
		}
		while (--count > 0);
	    }
	    line += sraster;
	    inc_ptr(dest, draster);
	}
    }
    return 0;
}

/* Copy a color bitmap. */
private int
mem_true64_copy_color(gx_device * dev,
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
declare_mem_procs(mem64_word_copy_mono, mem64_word_copy_color, mem64_word_fill_rectangle);

/* Here is the device descriptor. */
const gx_device_memory mem_true64_word_device =
mem_full_device("image64w", 64, 0, mem_open,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb,
     mem64_word_copy_mono, mem64_word_copy_color, mem64_word_fill_rectangle,
		gx_default_map_cmyk_color, gx_default_strip_tile_rectangle,
		gx_no_strip_copy_rop, mem_word_get_bits_rectangle);

/* Fill a rectangle with a color. */
private int
mem64_word_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			  gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *base;
    uint raster;

    fit_fill(dev, x, y, w, h);
    base = scan_line_base(mdev, y);
    raster = mdev->raster;
    mem_swap_byte_rect(base, raster, x * 64, w * 64, h, true);
    mem_true64_fill_rectangle(dev, x, y, w, h, color);
    mem_swap_byte_rect(base, raster, x * 64, w * 64, h, false);
    return 0;
}

/* Copy a bitmap. */
private int
mem64_word_copy_mono(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
	int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *row;
    uint raster;
    bool store;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    row = scan_line_base(mdev, y);
    raster = mdev->raster;
    store = (zero != gx_no_color_index && one != gx_no_color_index);
    mem_swap_byte_rect(row, raster, x * 64, w * 64, h, store);
    mem_true64_copy_mono(dev, base, sourcex, sraster, id,
			 x, y, w, h, zero, one);
    mem_swap_byte_rect(row, raster, x * 64, w * 64, h, false);
    return 0;
}

/* Copy a color bitmap. */
private int
mem64_word_copy_color(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *row;
    uint raster;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    row = scan_line_base(mdev, y);
    raster = mdev->raster;
    mem_swap_byte_rect(row, raster, x * 64, w * 64, h, true);
    bytes_copy_rectangle(row + x * PIXEL_SIZE, raster, base + sourcex * PIXEL_SIZE,
    				sraster, w * PIXEL_SIZE, h);
    mem_swap_byte_rect(row, raster, x * 64, w * 64, h, false);
    return 0;
}

#endif /* !arch_is_big_endian */
