/* Copyright (C) 1994, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevm16.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* 16-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */

#undef chunk
#define chunk byte

/* The 16 bits are divided 5 for red, 6 for green, and 5 for blue. */
/* Note that the bits must always be kept in big-endian order. */

/* Procedures */
declare_mem_map_procs(mem_true16_map_rgb_color, mem_true16_map_color_rgb);
declare_mem_procs(mem_true16_copy_mono, mem_true16_copy_color, mem_true16_fill_rectangle);

/* The device descriptor. */
const gx_device_memory mem_true16_device =
    mem_device("image16", 16, 0,
	       mem_true16_map_rgb_color, mem_true16_map_color_rgb,
	       mem_true16_copy_mono, mem_true16_copy_color,
	       mem_true16_fill_rectangle, mem_default_strip_copy_rop);

/* Map a r-g-b color to a color index. */
private gx_color_index
mem_true16_map_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
			 gx_color_value b)
{
    return ((r >> (gx_color_value_bits - 5)) << 11) +
	((g >> (gx_color_value_bits - 6)) << 5) +
	(b >> (gx_color_value_bits - 5));
}

/* Map a color index to a r-g-b color. */
private int
mem_true16_map_color_rgb(gx_device * dev, gx_color_index color,
			 gx_color_value prgb[3])
{
    ushort value = color >> 11;

    prgb[0] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4))
		      >> (16 - gx_color_value_bits);
    value = (color >> 5) & 0x3f;
    prgb[1] = ((value << 10) + (value << 4) + (value >> 2))
		      >> (16 - gx_color_value_bits);
    value = color & 0x1f;
    prgb[2] = ((value << 11) + (value << 6) + (value << 1) + (value >> 4))
		      >> (16 - gx_color_value_bits);
    return 0;
}

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) << 1)

/* Fill a rectangle with a color. */
private int
mem_true16_fill_rectangle(gx_device * dev,
			  int x, int y, int w, int h, gx_color_index color)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
#if arch_is_big_endian
    const ushort color16 = (ushort)color;
#else
    const ushort color16 = (ushort)((color << 8) | (color >> 8));
#endif
    declare_scan_ptr(dest);

    fit_fill(dev, x, y, w, h);
    setup_rect(dest);
    if (w == 1) {
	while (h-- > 0) {
	    *(ushort *)dest = color16;
	    inc_ptr(dest, draster);
	}
    } else if ((color16 >> 8) == (color16 & 0xff)) {
	bytes_fill_rectangle(scan_line_base(mdev, y) + (x << 1), draster,
			     (byte)color, w << 1, h);
    } else {
	while (h-- > 0) {
	    ushort *pptr = (ushort *) dest;
	    int cnt = w;

	    for (; cnt >= 4; pptr += 4, cnt -= 4)
		pptr[3] = pptr[2] = pptr[1] = pptr[0] = color16;
	    switch (cnt) {
	    case 3: pptr[2] = color16;
	    case 2: pptr[1] = color16;
	    case 1: pptr[0] = color16;
	    case 0: DO_NOTHING;
	    }
	    inc_ptr(dest, draster);
	}
    }
    return 0;
}

/* Copy a monochrome bitmap. */
private int
mem_true16_copy_mono(gx_device * dev,
		     const byte * base, int sourcex, int sraster,
		     gx_bitmap_id id, int x, int y, int w, int h,
		     gx_color_index zero, gx_color_index one)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
#if arch_is_big_endian
    const ushort zero16 = (ushort)zero;
    const ushort one16 = (ushort)one;
#else
    ushort zero16 = ((uint) (byte) zero << 8) + ((ushort) zero >> 8);
    ushort one16 = ((uint) (byte) one << 8) + ((ushort) one >> 8);
#endif
    const byte *line;
    int first_bit;

    declare_scan_ptr(dest);
    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    setup_rect(dest);
    line = base + (sourcex >> 3);
    first_bit = 0x80 >> (sourcex & 7);
    while (h-- > 0) {
	register ushort *pptr = (ushort *) dest;
	const byte *sptr = line;
	register int sbyte = *sptr++;
	register int bit = first_bit;
	int count = w;

	do {
	    if (sbyte & bit) {
		if (one != gx_no_color_index)
		    *pptr = one16;
	    } else {
		if (zero != gx_no_color_index)
		    *pptr = zero16;
	    }
	    if ((bit >>= 1) == 0)
		bit = 0x80, sbyte = *sptr++;
	    pptr++;
	}
	while (--count > 0);
	line += sraster;
	inc_ptr(dest, draster);
    }
    return 0;
}

/* Copy a color bitmap. */
private int
mem_true16_copy_color(gx_device * dev,
	       const byte * base, int sourcex, int sraster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;

    fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
    mem_copy_byte_rect(mdev, base, sourcex, sraster, x, y, w, h);
    return 0;
}
