/* Copyright (C) 1989, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxbitmap.h */
/* Definitions for stored bitmaps for Ghostscript */

#ifndef gxbitmap_INCLUDED
#  define gxbitmap_INCLUDED

/*
 * Drivers such as the X driver and the command list (band list) driver
 * benefit greatly by being able to cache bitmaps (tiles and characters)
 * and refer to them later.  To support this, we define a bitmap ID type
 * which the kernel passes to the driver on each copy_ or tile_ operation.
 */
typedef unsigned long gx_bitmap_id;
#define gx_no_bitmap_id 0L

/*
 * Structure for describing stored bitmaps.
 * Bitmaps are stored bit-big-endian (i.e., the 0x80 bit of the first
 * byte corresponds to x=0), as a sequence of bytes (i.e., you can't
 * do word-oriented operations on them if you're on a little-endian
 * platform like the Intel 80x86 or VAX).  Each scan line must start on
 * a `word' (long) boundary, and hence is padded to a word boundary,
 * although this should rarely be of concern, since the raster and width
 * are specified individually.  The first scan line corresponds to y=0
 * in whatever coordinate system is relevant.
 */
/* We assume arch_align_long_mod is 1-4 or 8. */
#if arch_align_long_mod <= 4
#  define log2_align_bitmap_mod 2
#else
#if arch_align_long_mod == 8
#  define log2_align_bitmap_mod 3
#endif
#endif
#define align_bitmap_mod (1 << log2_align_bitmap_mod)
#define bitmap_raster(width_bits)\
  ((uint)(((width_bits + (align_bitmap_mod * 8 - 1))\
    >> (log2_align_bitmap_mod + 3)) << log2_align_bitmap_mod))
/*
 * The basic structure for a bitmap does not specify the depth;
 * this is implicit in the context of use.
 */
#define gx_bitmap_common\
	byte *data;\
	int raster;			/* bytes per scan line */\
	gs_int_point size;		/* width, height */\
	gx_bitmap_id id
typedef struct gx_bitmap_s {
	gx_bitmap_common;
} gx_bitmap;
/*
 * For bitmaps used as halftone tiles, we may replicate the tile in
 * X and/or Y, but it is still valuable to know the true tile dimensions
 * (i.e., the dimensions prior to replication).  Eventually, for rotated
 * tiles, we will include strip and shift values that will allow us to
 * store such tiles in much less space.
 */
typedef struct gx_tile_bitmap_s {
	gx_bitmap_common;
	ushort rep_width, rep_height;	/* true size of tile */
} gx_tile_bitmap;

#endif					/* gxbitmap_INCLUDED */
