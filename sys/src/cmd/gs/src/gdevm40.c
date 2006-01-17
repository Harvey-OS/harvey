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

/*$Id: gdevm40.c,v 1.3 2005/06/20 08:59:23 igor Exp $ */
/* 40-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */

/* Define debugging statistics. */
#ifdef DEBUG
struct stats_mem40_s {
    long
	fill, fwide, fgray[101], fsetc, fcolor[101], fnarrow[5],
	fprevc[257];
    double ftotal;
} stats_mem40;
static int prev_count = 0;
static gx_color_index prev_colors[256];
# define INCR(v) (++(stats_mem40.v))
#else
# define INCR(v) DO_NOTHING
#endif


/* ================ Standard (byte-oriented) device ================ */

#undef chunk
#define chunk byte
#define PIXEL_SIZE 5

/* Procedures */
declare_mem_procs(mem_true40_copy_mono, mem_true40_copy_color, mem_true40_fill_rectangle);

/* The device descriptor. */
const gx_device_memory mem_true40_device =
mem_full_alpha_device("image40", 40, 0, mem_open,
		 gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb,
     mem_true40_copy_mono, mem_true40_copy_color, mem_true40_fill_rectangle,
		      gx_default_map_cmyk_color, gx_default_copy_alpha,
		 gx_default_strip_tile_rectangle, mem_default_strip_copy_rop,
		      mem_get_bits_rectangle);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) * PIXEL_SIZE)

/* Unpack a color into its bytes. */
#define declare_unpack_color(a, b, c, d, e, color)\
	byte a = (byte)(color >> 32);\
	byte b = (byte)((uint)color >> 24);\
	byte c = (byte)((uint)color >> 16);\
	byte d = (byte)((uint)color >> 8);\
	byte e = (byte)color
/* Put a 40-bit color into the bitmap. */
#define put5(ptr, a, b, c, d, e)\
	(ptr)[0] = a, (ptr)[1] = b, (ptr)[2] = c, (ptr)[3] = d, (ptr)[4] = e
/* Put 4 bytes of color into the bitmap. */
#define putw(ptr, wxyz)\
	*(bits32 *)(ptr) = (wxyz)
/* Load the 5-word 40-bit-color cache. */
/* Free variables: [m]dev, abcd, bcde, cdea, deab, earc. */
#if arch_is_big_endian
#  define set_color40_cache(color, a, b, c, d, e)\
	mdev->color40.abcd = abcd = (color) >> 8, \
	mdev->color40.bcde = bcde = (abcd << 8) | (e),\
	mdev->color40.cdea = cdea = (bcde << 8) | (a),\
	mdev->color40.deab = deab = (cdea << 8) | (b),\
	mdev->color40.eabc = eabc = (deab << 8) | (c),\
	mdev->color40.abcde = (color)
#else
#  define set_color40_cache(color, a, b, c, d, e)\
	mdev->color40.abcd = abcd =\
		((bits32)(d) << 24) | ((bits32)(c) << 16) |\
		((bits16)(b) << 8) | (a),\
	mdev->color40.eabc = eabc = (abcd << 8) | (e),\
	mdev->color40.deab = deab = (eabc << 8) | (d),\
	mdev->color40.cdea = cdea = (deab << 8) | (c),\
	mdev->color40.bcde = bcde = (cdea << 8) | (b),\
	mdev->color40.abcde = (color)
#endif

/* Fill a rectangle with a color. */
private int
mem_true40_fill_rectangle(gx_device * dev,
			  int x, int y, int w, int h, gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    declare_unpack_color(a, b, c, d, e, color);
    declare_scan_ptr(dest);

    /*
     * In order to avoid testing w > 0 and h > 0 twice, we defer
     * executing setup_rect, and use fit_fill_xywh instead of
     * fit_fill.
     */
    fit_fill_xywh(dev, x, y, w, h);
    INCR(fill);
#ifdef DEBUG
    stats_mem40.ftotal += w;
#endif
    if (w >= 5) {
	if (h <= 0)
	    return 0;
	INCR(fwide);
	setup_rect(dest);
	if (a == b && b == c && c == d && d == e) {
	    int bcnt = w * PIXEL_SIZE;

	    INCR(fgray[min(w, 100)]);
	    while (h-- > 0) {
		memset(dest, a, bcnt);
		inc_ptr(dest, draster);
	    }
	} else {
	    int x3 = -x & 3, ww = w - x3;	/* we know ww >= 2 */
	    bits32 abcd, bcde, cdea, deab, eabc;

	    if (mdev->color40.abcde == color) {
		abcd = mdev->color40.abcd;
		bcde = mdev->color40.bcde;
		cdea = mdev->color40.cdea;
		deab = mdev->color40.deab;
		eabc = mdev->color40.eabc;
	    } else {
		INCR(fsetc);
		set_color40_cache(color, a, b, c, d, e);
	    }
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
		register byte *pptr = dest;
		int w1 = ww;

		switch (x3) {
		    case 1:
			pptr[0] = a;
			putw(pptr + 1, bcde);
			pptr += PIXEL_SIZE;
			break;
		    case 2:
			pptr[0] = a;
			pptr[1] = b;
			putw(pptr + 2, cdea);
			putw(pptr + 6, bcde);
			pptr += 2 * PIXEL_SIZE;
			break;
		    case 3:
			pptr[0] = a;
			pptr[1] = b;
			pptr[2] = c;
			putw(pptr + 3, deab);
			putw(pptr + 7, cdea);
			putw(pptr + 11, bcde);
			pptr += 3 * PIXEL_SIZE;
			break;
		    case 0:
			;
		}
		while (w1 >= 4) {
		    putw(pptr, abcd);
		    putw(pptr + 4, eabc);
		    putw(pptr + 8, deab);
		    putw(pptr + 12, cdea);
		    putw(pptr + 16, bcde);
		    pptr += 4 * PIXEL_SIZE;
		    w1 -= 4;
		}
		switch (w1) {
		    case 1:
			putw(pptr, abcd);
			pptr[4] = e;
			break;
		    case 2:
			putw(pptr, abcd);
			putw(pptr + 4, eabc);
			pptr[8] = d;
			pptr[9] = e;
			break;
		    case 3:
			putw(pptr, abcd);
			putw(pptr + 4, eabc);
			putw(pptr + 8, deab);
			pptr[12] = c;
			pptr[13] = d;
			pptr[14] = e;
			break;
		    case 0:
			;
		}
		inc_ptr(dest, draster);
	    }
	}
    } else if (h > 0) {		/* w < 5 */
	INCR(fnarrow[max(w, 0)]);
	setup_rect(dest);
	switch (w) {
	    case 4:
		do {
		    dest[15] = dest[10] = dest[5] = dest[0] = a;
		    dest[16] = dest[11] = dest[6] = dest[1] = b;
		    dest[17] = dest[12] = dest[7] = dest[2] = c;
		    dest[18] = dest[13] = dest[8] = dest[3] = d;
		    dest[19] = dest[14] = dest[9] = dest[4] = e;
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 3:
		do {
		    dest[10] = dest[5] = dest[0] = a;
		    dest[11] = dest[6] = dest[1] = b;
		    dest[12] = dest[7] = dest[2] = c;
		    dest[13] = dest[8] = dest[3] = d;
		    dest[14] = dest[9] = dest[4] = e;
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 2:
		do {
		    dest[5] = dest[0] = a;
		    dest[6] = dest[1] = b;
		    dest[7] = dest[2] = c;
		    dest[8] = dest[3] = d;
		    dest[9] = dest[4] = e;
		    inc_ptr(dest, draster);
		}
		while (--h);
		break;
	    case 1:
		do {
		    dest[0] = a; dest[1] = b; dest[2] = c; dest[3] = d; dest[4] = e;
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
mem_true40_copy_mono(gx_device * dev,
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
	declare_unpack_color(a0, b0, c0, d0, e0, zero);
	declare_unpack_color(a1, b1, c1, d1, e1, one);
	while (h-- > 0) {
	    register byte *pptr = dest;
	    const byte *sptr = line;
	    register int sbyte = *sptr++;
	    register int bit = first_bit;
	    int count = w;

	    do {
		if (sbyte & bit) {
		    if (one != gx_no_color_index)
			put5(pptr, a1, b1, c1, d1, e1);
		} else
		    put5(pptr, a0, b0, c0, d0, e0);
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
	declare_unpack_color(a1, b1, c1, d1, e1, one);
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
	    register byte *pptr = dest;
	    const byte *sptr = line;
	    register int sbyte = *sptr++ & first_mask;
	    int count = w - first_count;

	    if (sbyte) {
		register int bit = first_bit;

		do {
		    if (sbyte & bit)
			put5(pptr, a1, b1, c1, d1, e1);
		    pptr += PIXEL_SIZE;
		}
		while ((bit >>= 1) & first_mask);
	    } else
		pptr += first_skip;
	    while (count >= 8) {
		sbyte = *sptr++;
		if (sbyte & 0xf0) {
		    if (sbyte & 0x80)
			put5(pptr, a1, b1, c1, d1, e1);
		    if (sbyte & 0x40)
			put5(pptr + 5, a1, b1, c1, d1, e1);
		    if (sbyte & 0x20)
			put5(pptr + 10, a1, b1, c1, d1, e1);
		    if (sbyte & 0x10)
			put5(pptr + 15, a1, b1, c1, d1, e1);
		}
		if (sbyte & 0xf) {
		    if (sbyte & 8)
			put5(pptr + 20, a1, b1, c1, d1, e1);
		    if (sbyte & 4)
			put5(pptr + 25, a1, b1, c1, d1, e1);
		    if (sbyte & 2)
			put5(pptr + 30, a1, b1, c1, d1, e1);
		    if (sbyte & 1)
			put5(pptr + 35, a1, b1, c1, d1, e1);
		}
		pptr += 8 * PIXEL_SIZE;
		count -= 8;
	    }
	    if (count > 0) {
		register int bit = 0x80;

		sbyte = *sptr++;
		do {
		    if (sbyte & bit)
			put5(pptr, a1, b1, c1, d1, e1);
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
mem_true40_copy_color(gx_device * dev,
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
declare_mem_procs(mem40_word_copy_mono, mem40_word_copy_color, mem40_word_fill_rectangle);

/* Here is the device descriptor. */
const gx_device_memory mem_true40_word_device =
mem_full_device("image40w", 40, 0, mem_open,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb,
     mem40_word_copy_mono, mem40_word_copy_color, mem40_word_fill_rectangle,
		gx_default_map_cmyk_color, gx_default_strip_tile_rectangle,
		gx_no_strip_copy_rop, mem_word_get_bits_rectangle);

/* Fill a rectangle with a color. */
private int
mem40_word_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			  gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *base;
    uint raster;

    fit_fill(dev, x, y, w, h);
    base = scan_line_base(mdev, y);
    raster = mdev->raster;
    mem_swap_byte_rect(base, raster, x * 40, w * 40, h, true);
    mem_true40_fill_rectangle(dev, x, y, w, h, color);
    mem_swap_byte_rect(base, raster, x * 40, w * 40, h, false);
    return 0;
}

/* Copy a bitmap. */
private int
mem40_word_copy_mono(gx_device * dev,
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
    mem_swap_byte_rect(row, raster, x * 40, w * 40, h, store);
    mem_true40_copy_mono(dev, base, sourcex, sraster, id,
			 x, y, w, h, zero, one);
    mem_swap_byte_rect(row, raster, x * 40, w * 40, h, false);
    return 0;
}

/* Copy a color bitmap. */
private int
mem40_word_copy_color(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *row;
    uint raster;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    row = scan_line_base(mdev, y);
    raster = mdev->raster;
    mem_swap_byte_rect(row, raster, x * 40, w * 40, h, true);
    bytes_copy_rectangle(row + x * PIXEL_SIZE, raster, base + sourcex * PIXEL_SIZE,
    				sraster, w * PIXEL_SIZE, h);
    mem_swap_byte_rect(row, raster, x * 40, w * 40, h, false);
    return 0;
}

#endif /* !arch_is_big_endian */
