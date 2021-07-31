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

/* gdevm8.c */
/* 8-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* semi-public definitions */
#include "gdevmem.h"			/* private definitions */

#undef chunk
#define chunk byte

/* Procedures */
declare_mem_procs(mem_mapped8_copy_mono, mem_mapped8_copy_color, mem_mapped8_fill_rectangle);

/* The device descriptor. */
const gx_device_memory far_data mem_mapped8_color_device =
  mem_device("image(8)", 8, 0,
    mem_mapped_map_rgb_color, mem_mapped_map_color_rgb,
    mem_mapped8_copy_mono, mem_mapped8_copy_color, mem_mapped8_fill_rectangle);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) (x)

/* Fill a rectangle with a color. */
private int
mem_mapped8_fill_rectangle(gx_device *dev,
  int x, int y, int w, int h, gx_color_index color)
{	fit_fill(dev, x, y, w, h);
	bytes_fill_rectangle(scan_line_base(mdev, y) + x, mdev->raster,
			     (byte)color, w, h);
	return 0;
}

/* Copy a monochrome bitmap. */
/* We split up this procedure because of limitations in the bcc32 compiler. */
private void mapped8_copy01(P9(chunk *, const byte *, int, int, uint,
  int, int, byte, byte));
private void mapped8_copyN1(P8(chunk *, const byte *, int, int, uint,
  int, int, byte));
private void mapped8_copy0N(P8(chunk *, const byte *, int, int, uint,
  int, int, byte));
private int
mem_mapped8_copy_mono(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{	const byte *line;
	int first_bit;
	declare_scan_ptr(dest);
	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	setup_rect(dest);
	line = base + (sourcex >> 3);
	first_bit = 0x80 >> (sourcex & 7);
#define is_color(c) ((int)(c) != (int)gx_no_color_index)
	if ( is_color(one) )
	{	if ( is_color(zero) )
		  mapped8_copy01(dest, line, first_bit, sraster, draster,
				 w, h, (byte)zero, (byte)one);
		else
		  mapped8_copyN1(dest, line, first_bit, sraster, draster,
				 w, h, (byte)one);
	}
	else if ( is_color(zero) )
	  mapped8_copy0N(dest, line, first_bit, sraster, draster,
			 w, h, (byte)zero);
#undef is_color
	return 0;
}
/* Macros for copy loops */
#define COPY_BEGIN\
	while ( h-- > 0 )\
	{	register byte *pptr = dest;\
		const byte *sptr = line;\
		register int sbyte = *sptr;\
		register uint bit = first_bit;\
		int count = w;\
		do\
		{
#define COPY_END\
			if ( (bit >>= 1) == 0 )\
				bit = 0x80, sbyte = *++sptr;\
			pptr++;\
		}\
		while ( --count > 0 );\
		line += sraster;\
		inc_ptr(dest, draster);\
	}
/* Halftone coloring */
private void
mapped8_copy01(chunk *dest, const byte *line, int first_bit,
  int sraster, uint draster, int w, int h, byte b0, byte b1)
{	COPY_BEGIN
	*pptr = (sbyte & bit ? b1 : b0);
	COPY_END
}
/* Stenciling */
private void
mapped8_copyN1(chunk *dest, const byte *line, int first_bit,
  int sraster, uint draster, int w, int h, byte b1)
{	COPY_BEGIN
	if ( sbyte & bit )
	  *pptr = b1;
	COPY_END
}
/* Reverse stenciling (probably never used) */
private void
mapped8_copy0N(chunk *dest, const byte *line, int first_bit,
  int sraster, uint draster, int w, int h, byte b0)
{	COPY_BEGIN
	if ( !(sbyte & bit) )
	  *pptr = b0;
	COPY_END
}
#undef COPY_BEGIN
#undef COPY_END

/* Copy a color bitmap. */
private int
mem_mapped8_copy_color(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h)
{	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	mem_copy_byte_rect(mdev, base, sourcex, sraster, x, y, w, h);
	return 0;
}
