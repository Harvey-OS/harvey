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

/* gdevm2.c */
/* 2-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* semi-public definitions */
#include "gdevmem.h"			/* private definitions */

#undef chunk
#define chunk byte
#define fpat(byt) mono_fill_make_pattern(byt)

/* Procedures */
declare_mem_procs(mem_mapped2_copy_mono, mem_mapped2_copy_color, mem_mapped2_fill_rectangle);

/* The device descriptor. */
const gx_device_memory far_data mem_mapped2_color_device =
  mem_device("image(2)", 2, 0,
    mem_mapped_map_rgb_color, mem_mapped_map_color_rgb,
    mem_mapped2_copy_mono, mem_mapped2_copy_color, mem_mapped2_fill_rectangle);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) >> 2)

/* Fill a rectangle with a color. */
private int
mem_mapped2_fill_rectangle(gx_device *dev,
  int x, int y, int w, int h, gx_color_index color)
{	static const mono_fill_chunk tile_patterns[4] =
	  {	fpat(0x00), fpat(0x55), fpat(0xaa), fpat(0xff)
	  };
	fit_fill(dev, x, y, w, h);
	bits_fill_rectangle(scan_line_base(mdev, y), x << 1, mdev->raster,
			    tile_patterns[color], w << 1, h);
	return 0;
}

/* Copy a bitmap. */
private int
mem_mapped2_copy_mono(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{	const byte *line;
	int first_bit;
	byte first_mask, b0, b1;
	static byte btab[4] = { 0, 0x55, 0xaa, 0xff };
	static byte bmask[4] = { 0xc0, 0x30, 0xc, 3 };
	declare_scan_ptr(dest);
	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	setup_rect(dest);
	line = base + (sourcex >> 3);
	first_bit = 0x80 >> (sourcex & 7);
	first_mask = bmask[x & 3];
	b0 = btab[zero & 3];
	b1 = btab[one & 3];
	while ( h-- > 0 )
	   {	register byte *pptr = (byte *)dest;
		const byte *sptr = line;
		register int sbyte = *sptr++;
		register int bit = first_bit;
		register byte mask = first_mask;
		int count = w;
		do
		   {	if ( sbyte & bit )
			   {	if ( one != gx_no_color_index )
				  *pptr = (*pptr & ~mask) + (b1 & mask);
			   }
			else
			   {	if ( zero != gx_no_color_index )
				  *pptr = (*pptr & ~mask) + (b0 & mask);
			   }
			if ( (bit >>= 1) == 0 )
				bit = 0x80, sbyte = *sptr++;
			if ( (mask >>= 2) == 0 )
				mask = 0xc0, pptr++;
		   }
		while ( --count > 0 );
		line += sraster;
		inc_ptr(dest, draster);
	   }
	return 0;
}

/* Copy a color bitmap. */
private int
mem_mapped2_copy_color(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h)
{	int code;
	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	/* Use monobit copy_mono. */
	/* Patch the width in the device temporarily. */
	dev->width <<= 1;
	code = (*dev_proc(&mem_mono_device, copy_mono))
	  (dev, base, sourcex << 1, sraster, id,
	   x << 1, y, w << 1, h, (gx_color_index)0, (gx_color_index)1);
	/* Restore the correct width. */
	dev->width >>= 1;
	return code;
}
