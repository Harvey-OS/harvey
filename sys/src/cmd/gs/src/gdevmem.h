/* Copyright (C) 1991, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevmem.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Private definitions for memory devices. */

#ifndef gdevmem_INCLUDED
#  define gdevmem_INCLUDED

#include "gxbitops.h"

/*
   The representation for a "memory" device is simply a
   contiguous bitmap stored in something like the PostScript
   representation, i.e., each scan line (in left-to-right order), padded
   to a multiple of bitmap_align_mod bytes, followed immediately by
   the next one.

   The representation of strings in the interpreter limits
   the size of a string to 64K-1 bytes, which means we can't simply use
   a string for the contents of a memory device.
   We get around this problem by making the client read out the
   contents of a memory device bitmap in pieces.

   On 80x86 PCs running in 16-bit mode, there may be no way to
   obtain a contiguous block of storage larger than 64K bytes,
   which typically isn't big enough for a full-screen bitmap.
   We take the following compromise position: if the PC is running in
   native mode (pseudo-segmenting), we limit the bitmap to 64K;
   if the PC is running in protected mode (e.g., under MS Windows),
   we assume that blocks larger than 64K have sequential segment numbers,
   and that the client arranges things so that an individual scan line,
   the scan line pointer table, and any single call on a drawing routine
   do not cross a segment boundary.

   Even though the scan lines are stored contiguously, we store a table
   of their base addresses, because indexing into it is faster than
   the multiplication that would otherwise be needed.
 */

/*
 * Macros for scan line access.
 * x_to_byte is different for each number of bits per pixel.
 * Note that these macros depend on the definition of chunk:
 * each procedure that uses the scanning macros should #define
 * (not typedef) chunk as either uint or byte.
 */
#define declare_scan_ptr(ptr)\
	DECLARE_SCAN_PTR_VARS(ptr, chunk *, draster)
#define DECLARE_SCAN_PTR_VARS(ptr, ptype, draster)\
	register ptype ptr;\
	uint draster
#define setup_rect(ptr)\
	SETUP_RECT_VARS(ptr, chunk *, draster)
#define SETUP_RECT_VARS(ptr, ptype, draster)\
	draster = mdev->raster;\
	ptr = (ptype)(scan_line_base(mdev, y) +\
	  (x_to_byte(x) & -chunk_align_bytes))

/* ------ Generic macros ------ */

/* Macro for declaring the essential device procedures. */
dev_proc_get_initial_matrix(mem_get_initial_matrix);
dev_proc_close_device(mem_close);
#define declare_mem_map_procs(map_rgb_color, map_color_rgb)\
  private dev_proc_map_rgb_color(map_rgb_color);\
  private dev_proc_map_color_rgb(map_color_rgb)
#define declare_mem_procs(copy_mono, copy_color, fill_rectangle)\
  private dev_proc_copy_mono(copy_mono);\
  private dev_proc_copy_color(copy_color);\
  private dev_proc_fill_rectangle(fill_rectangle)
/*
 * We define one relatively low-usage drawing procedure that is common to
 * all memory devices so that we have a reliable way to implement
 * gs_is_memory_device.  It is equivalent to gx_default_draw_thin_line.
 */
dev_proc_draw_thin_line(mem_draw_thin_line);

/* The following are used for all except planar or word-oriented devices. */
dev_proc_open_device(mem_open);
dev_proc_get_bits_rectangle(mem_get_bits_rectangle);
/* The following are for word-oriented devices. */
#if arch_is_big_endian
#  define mem_word_get_bits_rectangle mem_get_bits_rectangle
#else
dev_proc_get_bits_rectangle(mem_word_get_bits_rectangle);
#endif
/* The following are used for the non-true-color devices. */
dev_proc_map_rgb_color(mem_mapped_map_rgb_color);
dev_proc_map_color_rgb(mem_mapped_map_color_rgb);
/* Default implementation */
dev_proc_strip_copy_rop(mem_default_strip_copy_rop);

/*
 * Macro for generating the device descriptor.
 * Various compilers have problems with the obvious definition
 * for max_value, namely:
 *      (depth >= 8 ? 255 : (1 << depth) - 1)
 * I tried changing (1 << depth) to (1 << (depth & 15)) to forestall bogus
 * error messages about invalid shift counts, but the H-P compiler chokes
 * on this.  Since the only values of depth we ever plan to support are
 * powers of 2 (and 24), we just go ahead and enumerate them.
 */
#define max_value_gray(rgb_depth, gray_depth)\
  (gray_depth ? (1 << gray_depth) - 1 : max_value_rgb(rgb_depth, 0))
#define max_value_rgb(rgb_depth, gray_depth)\
  (rgb_depth >= 8 ? 255 : rgb_depth == 4 ? 15 : rgb_depth == 2 ? 3 :\
   rgb_depth == 1 ? 1 : (1 << gray_depth) - 1)
#define mem_full_alpha_device(name, rgb_depth, gray_depth, open, map_rgb_color, map_color_rgb, copy_mono, copy_color, fill_rectangle, map_cmyk_color, copy_alpha, strip_tile_rectangle, strip_copy_rop, get_bits_rectangle)\
{	std_device_dci_body(gx_device_memory, 0, name,\
	  0, 0, 72, 72,\
	  (rgb_depth ? 3 : 0) + (gray_depth ? 1 : 0),	/* num_components */\
	  rgb_depth + gray_depth,	/* depth */\
	  max_value_gray(rgb_depth, gray_depth),	/* max_gray */\
	  max_value_rgb(rgb_depth, gray_depth),	/* max_color */\
	  max_value_gray(rgb_depth, gray_depth) + 1, /* dither_grays */\
	  max_value_rgb(rgb_depth, gray_depth) + 1 /* dither_colors */\
	),\
	{	open,			/* differs */\
		mem_get_initial_matrix,\
		gx_default_sync_output,\
		gx_default_output_page,\
		mem_close,\
		map_rgb_color,		/* differs */\
		map_color_rgb,		/* differs */\
		fill_rectangle,		/* differs */\
		gx_default_tile_rectangle,\
		copy_mono,		/* differs */\
		copy_color,		/* differs */\
		gx_default_draw_line,\
		gx_default_get_bits,\
		gx_default_get_params,\
		gx_default_put_params,\
		map_cmyk_color,		/* differs */\
		gx_forward_get_xfont_procs,\
		gx_forward_get_xfont_device,\
		gx_default_map_rgb_alpha_color,\
		gx_forward_get_page_device,\
		gx_default_get_alpha_bits,	/* default is no alpha */\
		copy_alpha,		/* differs */\
		gx_default_get_band,\
		gx_default_copy_rop,\
		gx_default_fill_path,\
		gx_default_stroke_path,\
		gx_default_fill_mask,\
		gx_default_fill_trapezoid,\
		gx_default_fill_parallelogram,\
		gx_default_fill_triangle,\
		mem_draw_thin_line,	/* see above */\
		gx_default_begin_image,\
		gx_default_image_data,\
		gx_default_end_image,\
		strip_tile_rectangle,	/* differs */\
		strip_copy_rop,		/* differs */\
		gx_default_get_clipping_box,\
		gx_default_begin_typed_image,\
		get_bits_rectangle,	/* differs */\
		gx_default_map_color_rgb_alpha,\
		gx_default_create_compositor,\
		gx_default_get_hardware_params,\
		gx_default_text_begin\
	},\
	0,			/* target */\
	mem_device_init_private	/* see gxdevmem.h */\
}
#define mem_full_device(name, rgb_depth, gray_depth, open, map_rgb_color, map_color_rgb, copy_mono, copy_color, fill_rectangle, map_cmyk_color, strip_tile_rectangle, strip_copy_rop, get_bits_rectangle)\
  mem_full_alpha_device(name, rgb_depth, gray_depth, open, map_rgb_color,\
			map_color_rgb, copy_mono, copy_color, fill_rectangle,\
			map_cmyk_color, gx_default_copy_alpha,\
			strip_tile_rectangle, strip_copy_rop,\
			get_bits_rectangle)
#define mem_device(name, rgb_depth, gray_depth, map_rgb_color, map_color_rgb, copy_mono, copy_color, fill_rectangle, strip_copy_rop)\
  mem_full_device(name, rgb_depth, gray_depth, mem_open, map_rgb_color,\
		  map_color_rgb, copy_mono, copy_color, fill_rectangle,\
		  gx_default_map_cmyk_color, gx_default_strip_tile_rectangle,\
		  strip_copy_rop, mem_get_bits_rectangle)

/* Swap a rectangle of bytes, for converting between word- and */
/* byte-oriented representation. */
void mem_swap_byte_rect(P6(byte *, uint, int, int, int, bool));

/* Copy a rectangle of bytes from a source to a destination. */
#define mem_copy_byte_rect(mdev, base, sourcex, sraster, x, y, w, h)\
  bytes_copy_rectangle(scan_line_base(mdev, y) + x_to_byte(x),\
		       (mdev)->raster,\
		       base + x_to_byte(sourcex), sraster,\
		       x_to_byte(w), h)

/* ------ Implementations ------ */

extern const gx_device_memory mem_mono_device;
extern const gx_device_memory mem_mapped2_device;
extern const gx_device_memory mem_mapped4_device;
extern const gx_device_memory mem_mapped8_device;
extern const gx_device_memory mem_true16_device;
extern const gx_device_memory mem_true24_device;
extern const gx_device_memory mem_true32_device;
extern const gx_device_memory mem_planar_device;
/*
 * We declare the RasterOp implementation procedures here because they are
 * referenced in several implementation modules.
 */
dev_proc_strip_copy_rop(mem_mono_strip_copy_rop);
dev_proc_strip_copy_rop(mem_gray_strip_copy_rop);
dev_proc_strip_copy_rop(mem_gray8_rgb24_strip_copy_rop);

#if arch_is_big_endian
#  define mem_mono_word_device mem_mono_device
#  define mem_mapped2_word_device mem_mapped2_device
#  define mem_mapped4_word_device mem_mapped4_device
#  define mem_mapped8_word_device mem_mapped8_device
#  define mem_true24_word_device mem_true24_device
#  define mem_true32_word_device mem_true32_device
#else
extern const gx_device_memory mem_mono_word_device;
extern const gx_device_memory mem_mapped2_word_device;
extern const gx_device_memory mem_mapped4_word_device;
extern const gx_device_memory mem_mapped8_word_device;
extern const gx_device_memory mem_true24_word_device;
extern const gx_device_memory mem_true32_word_device;

#endif
/* Provide standard palettes for 1-bit devices. */
extern const gs_const_string mem_mono_b_w_palette;	/* black=1, white=0 */
extern const gs_const_string mem_mono_w_b_palette;	/* black=0, white=1 */

#endif /* gdevmem_INCLUDED */
