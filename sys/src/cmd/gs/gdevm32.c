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

/* gdevm32.c */
/* 32-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* semi-public definitions */
#include "gdevmem.h"			/* private definitions */

#undef chunk
#define chunk byte

/* Procedures */
declare_mem_procs(mem_true32_copy_mono, mem_true32_copy_color, mem_true32_fill_rectangle);

/* The device descriptor. */
const gx_device_memory far_data mem_true32_color_device =
  mem_full_device("image(32)", 24, 8, mem_open,
    gx_default_map_rgb_color, gx_default_map_color_rgb,
    mem_true32_copy_mono, mem_true32_copy_color, mem_true32_fill_rectangle,
    gx_default_tile_rectangle, mem_get_bits, gx_default_cmyk_map_cmyk_color);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) << 2)

/* Swap the bytes of a color if needed. */
#if arch_is_big_endian
#  define arrange_bytes(color) (color)
#else
#  define arrange_bytes(color)\
    (((color) >> 24) + (((color) >> 8) & 0xff00) +\
     (((color) & 0xff00) << 8) + ((color) << 24))
#endif

/* Fill a rectangle with a color. */
private int
mem_true32_fill_rectangle(gx_device *dev,
  int x, int y, int w, int h, gx_color_index color)
{	bits32 a_color = arrange_bytes(color);
	declare_scan_ptr(dest);
	fit_fill(dev, x, y, w, h);
	setup_rect(dest);
	while ( h-- > 0 )
	{	bits32 *pptr = (bits32 *)dest;
		int cnt = w;
		do { *pptr++ = a_color; } while ( --cnt > 0 );
		inc_ptr(dest, draster);
	}
	return 0;
}

/* Copy a monochrome bitmap. */
private int
mem_true32_copy_mono(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{	bits32 a_zero = arrange_bytes(zero);
	bits32 a_one = arrange_bytes(one);
	const byte *line;
	int first_bit;
	declare_scan_ptr(dest);
	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	setup_rect(dest);
	line = base + (sourcex >> 3);
	first_bit = 0x80 >> (sourcex & 7);
	while ( h-- > 0 )
	{	register bits32 *pptr = (bits32 *)dest;
		const byte *sptr = line;
		register int sbyte = *sptr++;
		register int bit = first_bit;
		int count = w;
		do
		{	if ( sbyte & bit )
			{	if ( one != gx_no_color_index )
				  *pptr = a_one;
			}
			else
			{	if ( zero != gx_no_color_index )
				  *pptr = a_zero;
			}
			if ( (bit >>= 1) == 0 )
				bit = 0x80, sbyte = *sptr++;
			pptr++;
		}
		while ( --count > 0 );
		line += sraster;
		inc_ptr(dest, draster);
	}
	return 0;
}

/* Copy a color bitmap. */
private int
mem_true32_copy_color(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h)
{	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	mem_copy_byte_rect(mdev, base, sourcex, sraster, x, y, w, h);
	return 0;
}
