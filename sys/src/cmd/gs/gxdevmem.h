/* Copyright (C) 1989, 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxdevmem.h */
/* "Memory" device structure for Ghostscript library */
/* Requires gxdevice.h */

/*
 * A 'memory' device is essentially a stored bitmap.
 * There are several different kinds: 1-bit black and white,
 * 2-, 4-, and 8-bit mapped color, and 16-, 24-, and 32-bit RGB color.
 * (16-bit uses 5/6/5 bits per color.  32-bit uses a padding byte;
 * 24-bit takes less space, but is slower.)  All use the same structure,
 * since it's so awkward to get the effect of subclasses in C.
 * We should support 32-bit CMYK color, but we don't yet.
 *
 * Regardless of the machine byte order, we store bytes big-endian.
 * This is required if the bits will be used as the source for a
 * rendering operation, and doesn't cost much to maintain.  Since
 * memory devices also are guaranteed to allocate the bitmap consecutively,
 * the bitmap can serve directly as input to copy_mono or copy_color
 * operations.
 */
typedef struct gx_device_memory_s gx_device_memory;
struct gx_device_memory_s {
	gx_device_forward_common;	/* (see gxdevice.h) */
	gs_matrix initial_matrix;	/* the initial transformation */
	uint raster;			/* bytes per scan line, */
					/* filled in by 'open' */
	bool foreign_bits;		/* if true, bits are not in */
					/* GC-able space */
	byte *base;
	byte **line_ptrs;		/* scan line pointers */
#define scan_line_base(dev,y) (dev->line_ptrs[y])
	/* If the bitmap_memory pointer is non-zero, it is used for */
	/* allocating the bitmap when the device is opened, */
	/* and freeing it when the device is closed. */
	gs_memory_t *bitmap_memory;
		/* Following is only needed for monochrome. */
	int invert;			/* 0 if 1=white, -1 if 1=black */
		/* Following is only needed for mapped color. */
	gs_string palette;		/* RGB triples */
		/* Following is only used for 24-bit color. */
	struct _c24 {
		gx_color_index rgb;	/* cache key */
		bits32 rgbr, gbrg, brgb;	/* cache value */
	} color24;
		/* Following are only used for alpha buffers. */
		/* The client initializes those marked with $; */
		/* they don't change after initialization. */
	gs_log2_scale_point log2_scale;	/* $ oversampling scale factors */
	int log2_alpha_bits;	/* $ log2 of # of alpha bits being produced */
	int mapped_x;		/* $ X value mapped to buffer X=0 */
	int mapped_y;		/* lowest Y value mapped to buffer */
	int mapped_height;	/* # of Y values mapped to buffer */
	int mapped_start;	/* local Y value corresponding to mapped_y */
	gx_color_index save_color;	/* last (only) color displayed */
};
extern_st(st_device_memory);
#define public_st_device_memory() /* in gdevmem.c */\
  gs_public_st_composite(st_device_memory, gx_device_memory,\
    "gx_device_memory", device_memory_enum_ptrs, device_memory_reloc_ptrs)
#define st_device_memory_max_ptrs (st_device_forward_max_ptrs + 2)

/*
 * Memory devices may have special setup requirements.
 * In particular, it may not be obvious how much space to allocate
 * for the bitmap.  Here is the routine that computes this
 * from the width and height in the device structure.
 */
ulong gdev_mem_bitmap_size(P1(const gx_device_memory *));
/*
 * Compute the raster (bytes per line) similarly.
 */
#define gdev_mem_raster(mdev)\
  gx_device_raster((const gx_device *)(mdev), 1)

/* Determine the appropriate memory device for a given */
/* number of bits per pixel (0 if none suitable). */
const gx_device_memory *gdev_mem_device_for_bits(P1(int));

/* Make a memory device. */
/* mem is 0 if the device is temporary and local, */
/* or the allocator that was used to allocate it if it is a real object. */
/* page_device is 1 if the device should be a page device, */
/* 0 if it should propagate this property from its target, or */
/* -1 if it should not be a page device. */
void gs_make_mem_mono_device(P3(gx_device_memory *mdev, gs_memory_t *mem,
				gx_device *target));
void gs_make_mem_device(P5(gx_device_memory *mdev,
			   const gx_device_memory *mdproto,
			   gs_memory_t *mem, int page_device,
			   gx_device *target));
void gs_make_mem_abuf_device(P6(gx_device_memory *adev, gs_memory_t *mem,
				gx_device *target,
				const gs_log2_scale_point *pscale,
				int alpha_bits, int mapped_x));

/* Test whether a device is a memory device. */
bool gs_device_is_memory(P1(const gx_device *));
