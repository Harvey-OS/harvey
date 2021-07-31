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

/* gdevm24.c */
/* 24-bit-per-pixel "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* semi-public definitions */
#include "gdevmem.h"			/* private definitions */

#undef chunk
#define chunk byte

/* Procedures */
declare_mem_procs(mem_true24_copy_mono, mem_true24_copy_color, mem_true24_fill_rectangle);

/* The device descriptor. */
const gx_device_memory far_data mem_true24_color_device =
  mem_device("image(24)", 24, 0,
    gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb,
    mem_true24_copy_mono, mem_true24_copy_color, mem_true24_fill_rectangle);

/* Convert x coordinate to byte offset in scan line. */
#undef x_to_byte
#define x_to_byte(x) ((x) * 3)

/* Unpack a color into its bytes. */
#define declare_unpack_color(r, g, b, color)\
	byte r = (byte)(color >> 16);\
	byte g = (byte)((uint)color >> 8);\
	byte b = (byte)color
#if arch_is_big_endian
#  define declare_pack_color(cword, rgb, r, g, b)\
	bits32 cword = (bits32)(rgb) << 8
#else
#  define declare_pack_color(cword, rgb, r, g, b)\
	bits32 cword =\
	  ((bits32)(b) << 16) | ((bits16)(g) << 8) | (r)
#endif
/* Put a 24-bit color into the bitmap. */
#define put3(ptr, r, g, b)\
	(ptr)[0] = r, (ptr)[1] = g, (ptr)[2] = b
/* Put 4 bytes of color into the bitmap. */
#define putw(ptr, wxyz)\
	*(bits32 *)(ptr) = (wxyz)
/* Load the 3-word 24-bit-color cache. */
/* Free variables: [m]dev, rgbr, gbrg, brgb. */
#if arch_is_big_endian
#  define set_color24_cache(crgb, r, g, b)\
	mdev->color24.rgbr = rgbr = ((bits32)(crgb) << 8) | (r),\
	mdev->color24.gbrg = gbrg = (rgbr << 8) | (g),\
	mdev->color24.brgb = brgb = (gbrg << 8) | (b),\
	mdev->color24.rgb = (crgb)
#else
#  define set_color24_cache(crgb, r, g, b)\
	mdev->color24.rgbr = rgbr =\
		((bits32)(r) << 24) | ((bits32)(b) << 16) |\
		((bits16)(g) << 8) | (r),\
	mdev->color24.brgb = brgb = (rgbr << 8) | (b),\
	mdev->color24.gbrg = gbrg = (brgb << 8) | (g),\
	mdev->color24.rgb = (crgb)
#endif

/* Fill a rectangle with a color. */
private int
mem_true24_fill_rectangle(gx_device *dev,
  int x, int y, int w, int h, gx_color_index color)
{	declare_unpack_color(r, g, b, color);
	declare_scan_ptr(dest);
	fit_fill(dev, x, y, w, h);
	setup_rect(dest);
	if ( w >= 5 )
	  { if ( r == g && r == b)
	      {
#if 1
		/* We think we can do better than the library's memset.... */
		int bcntm7 = w * 3 - 7;
		register bits32 cword = color | (color << 24);
		while ( h-- > 0 )
		{	register byte *pptr = dest;
			byte *limit = pptr + bcntm7;
			/* We want to store full words, but we have to */
			/* guarantee that they are word-aligned. */
			switch ( x & 3 )
			  {
			  case 3: *pptr++ = (byte)cword;
			  case 2: *pptr++ = (byte)cword;
			  case 1: *pptr++ = (byte)cword;
			  case 0: ;
			  }
			/* Even with w = 5, we always store at least */
			/* 3 full words, regardless of the starting x. */
			*(bits32 *)pptr =
			  ((bits32 *)pptr)[1] =
			  ((bits32 *)pptr)[2] = cword;
			pptr += 12;
			while ( pptr < limit )
			  { *(bits32 *)pptr =
			      ((bits32 *)pptr)[1] = cword;
			    pptr += 8;
			  }
			switch ( pptr - limit )
			  {
			  case 0: pptr[6] = (byte)cword;
			  case 1: pptr[5] = (byte)cword;
			  case 2: pptr[4] = (byte)cword;
			  case 3: *(bits32 *)pptr = cword;
			    break;
			  case 4: pptr[2] = (byte)cword;
			  case 5: pptr[1] = (byte)cword;
			  case 6: pptr[0] = (byte)cword;
			  case 7: ;
			  }
			inc_ptr(dest, draster);
		}
#else
		int bcnt = w * 3;
		while ( h-- > 0 )
		{	memset(dest, r, bcnt);
			inc_ptr(dest, draster);
		}
#endif
	      }
	    else
	      {	int x3 = -x & 3, ww = w - x3;
		bits32 rgbr, gbrg, brgb;
		if ( mdev->color24.rgb == color )
		  rgbr = mdev->color24.rgbr,
		  gbrg = mdev->color24.gbrg,
		  brgb = mdev->color24.brgb;
		else
		  set_color24_cache(color, r, g, b);
		while ( h-- > 0 )
		  {	register byte *pptr = dest;
			int w1 = ww;
			switch ( x3 )
			  {
			  case 1:
				put3(pptr, r, g, b);
				pptr += 3; break;
			  case 2:
				pptr[0] = r; pptr[1] = g;
				putw(pptr + 2, brgb);
				pptr += 6; break;
			  case 3:
				pptr[0] = r;
				putw(pptr + 1, gbrg);
				putw(pptr + 5, brgb);
				pptr += 9; break;
			  case 0:
				;
			  }
			while ( w1 >= 4 )
			  {	putw(pptr, rgbr);
				putw(pptr + 4, gbrg);
				putw(pptr + 8, brgb);
				pptr += 12;
				w1 -= 4;
			  }
			switch ( w1 )
			  {
			  case 1:
				put3(pptr, r, g, b);
				break;
			  case 2:
				putw(pptr, rgbr);
				pptr[4] = g; pptr[5] = b;
				break;
			  case 3:
				putw(pptr, rgbr);
				putw(pptr + 4, gbrg);
				pptr[8] = b;
				break;
			  case 0:
				;
			  }
			inc_ptr(dest, draster);
		  }
	      }
	  }
	else			/* w < 5 */
	{	while ( h-- > 0 )
		  {	switch ( w )
			  {
			  case 4: put3(dest + 9, r, g, b);
			  case 3: put3(dest + 6, r, g, b);
			  case 2: put3(dest + 3, r, g, b);
			  case 1: put3(dest, r, g, b);
			  case 0: ;
			  }
			inc_ptr(dest, draster);
		  }
	}
	return 0;
}

/* Copy a monochrome bitmap. */
private int
mem_true24_copy_mono(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h, gx_color_index zero, gx_color_index one)
{	const byte *line;
	int sbit;
	int first_bit;
	declare_scan_ptr(dest);
	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	setup_rect(dest);
	line = base + (sourcex >> 3);
	sbit = sourcex & 7;
	first_bit = 0x80 >> sbit;
	if ( zero != gx_no_color_index )
	  {	/* Loop for halftones or inverted masks */
		/* (never used). */
		declare_unpack_color(r0, g0, b0, zero);
		declare_unpack_color(r1, g1, b1, one);
		while ( h-- > 0 )
		   {	register byte *pptr = dest;
			const byte *sptr = line;
			register int sbyte = *sptr++;
			register int bit = first_bit;
			int count = w;
			do
			  {	if ( sbyte & bit )
				  { if ( one != gx_no_color_index )
				      put3(pptr, r1, g1, b1);
				  }
				else
				  put3(pptr, r0, g0, b0);
				pptr += 3;
				if ( (bit >>= 1) == 0 )
				  bit = 0x80, sbyte = *sptr++;
			  }
			while ( --count > 0 );
			line += sraster;
			inc_ptr(dest, draster);
		   }
	  }
	else if ( one != gx_no_color_index )
	  {	/* Loop for character and pattern masks. */
		/* This is used heavily. */
		declare_unpack_color(r1, g1, b1, one);
		int first_mask = first_bit << 1;
		int first_count, first_skip;
		if ( sbit + w > 8 )
		  first_mask -= 1,
		  first_count = 8 - sbit;
		else
		  first_mask -= first_mask >> w,
		  first_count = w;
		first_skip = first_count * 3;
		while ( h-- > 0 )
		   {	register byte *pptr = dest;
			const byte *sptr = line;
			register int sbyte = *sptr++ & first_mask;
			int count = w - first_count;
			if ( sbyte )
			  {	register int bit = first_bit;
				do
				  {	if ( sbyte & bit )
					  put3(pptr, r1, g1, b1);
					pptr += 3;
				  }
				while ( (bit >>= 1) & first_mask );
			  }
			else
			  pptr += first_skip;
			while ( count >= 8 )
			  {	sbyte = *sptr++;
				if ( sbyte & 0xf0 )
				  { if ( sbyte & 0x80 )
				      put3(pptr, r1, g1, b1);
				    if ( sbyte & 0x40 )
				      put3(pptr + 3, r1, g1, b1);
				    if ( sbyte & 0x20 )
				      put3(pptr + 6, r1, g1, b1);
				    if ( sbyte & 0x10 )
				      put3(pptr + 9, r1, g1, b1);
				  }
				if ( sbyte & 0xf )
				  { if ( sbyte & 8 )
				      put3(pptr + 12, r1, g1, b1);
				    if ( sbyte & 4 )
				      put3(pptr + 15, r1, g1, b1);
				    if ( sbyte & 2 )
				      put3(pptr + 18, r1, g1, b1);
				    if ( sbyte & 1 )
				      put3(pptr + 21, r1, g1, b1);
				  }
				pptr += 24;
				count -= 8;
			  }
			if ( count > 0 )
			  {	register int bit = 0x80;
				sbyte = *sptr++;
				do
				  {	if ( sbyte & bit )
					  put3(pptr, r1, g1, b1);
					pptr += 3;
					bit >>= 1;
				  }
				while ( --count > 0 );
			  }
			line += sraster;
			inc_ptr(dest, draster);
		   }
	  }
	return 0;
}

/* Copy a color bitmap. */
private int
mem_true24_copy_color(gx_device *dev,
  const byte *base, int sourcex, int sraster, gx_bitmap_id id,
  int x, int y, int w, int h)
{	fit_copy(dev, base, sourcex, sraster, id, x, y, w, h);
	mem_copy_byte_rect(mdev, base, sourcex, sraster, x, y, w, h);
	return 0;
}
