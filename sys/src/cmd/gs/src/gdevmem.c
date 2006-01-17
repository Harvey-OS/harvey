/* Copyright (C) 1989, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevmem.c,v 1.9 2005/03/14 18:08:36 dan Exp $ */
/* Generic "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsrect.h"
#include "gsstruct.h"
#include "gxarith.h"
#include "gxdevice.h"
#include "gxgetbit.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */
#include "gstrans.h"

/* Structure descriptor */
public_st_device_memory();

/* GC procedures */
private 
ENUM_PTRS_WITH(device_memory_enum_ptrs, gx_device_memory *mptr)
{
    return ENUM_USING(st_device_forward, vptr, sizeof(gx_device_forward), index - 3);
}
case 0: ENUM_RETURN((mptr->foreign_bits ? NULL : (void *)mptr->base));
case 1: ENUM_RETURN((mptr->foreign_line_pointers ? NULL : (void *)mptr->line_ptrs));
ENUM_STRING_PTR(2, gx_device_memory, palette);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(device_memory_reloc_ptrs, gx_device_memory *mptr)
{
    if (!mptr->foreign_bits) {
	byte *base_old = mptr->base;
	long reloc;
	int y;

	RELOC_PTR(gx_device_memory, base);
	reloc = base_old - mptr->base;
	for (y = 0; y < mptr->height; y++)
	    mptr->line_ptrs[y] -= reloc;
	/* Relocate line_ptrs, which also points into the data area. */
	mptr->line_ptrs = (byte **) ((byte *) mptr->line_ptrs - reloc);
    } else if (!mptr->foreign_line_pointers) {
	RELOC_PTR(gx_device_memory, line_ptrs);
    }
    RELOC_CONST_STRING_PTR(gx_device_memory, palette);
    RELOC_USING(st_device_forward, vptr, sizeof(gx_device_forward));
}
RELOC_PTRS_END

/* Define the palettes for monobit devices. */
private const byte b_w_palette_string[6] = {
    0xff, 0xff, 0xff, 0, 0, 0
};
const gs_const_string mem_mono_b_w_palette = {
    b_w_palette_string, 6
};
private const byte w_b_palette_string[6] = {
    0, 0, 0, 0xff, 0xff, 0xff
};
const gs_const_string mem_mono_w_b_palette = {
    w_b_palette_string, 6
};

/* ------ Generic code ------ */

/* Return the appropriate memory device for a given */
/* number of bits per pixel (0 if none suitable). */
private const gx_device_memory *const mem_devices[65] = {
    0, &mem_mono_device, &mem_mapped2_device, 0, &mem_mapped4_device,
    0, 0, 0, &mem_mapped8_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true16_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true24_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true32_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true40_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true48_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true56_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true64_device
};
const gx_device_memory *
gdev_mem_device_for_bits(int bits_per_pixel)
{
    return ((uint)bits_per_pixel > 64 ? (const gx_device_memory *)0 :
	    mem_devices[bits_per_pixel]);
}
/* Do the same for a word-oriented device. */
private const gx_device_memory *const mem_word_devices[65] = {
    0, &mem_mono_device, &mem_mapped2_word_device, 0, &mem_mapped4_word_device,
    0, 0, 0, &mem_mapped8_word_device,
    0, 0, 0, 0, 0, 0, 0, 0 /*no 16-bit word device*/,
    0, 0, 0, 0, 0, 0, 0, &mem_true24_word_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true32_word_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true40_word_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true48_word_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true56_word_device,
    0, 0, 0, 0, 0, 0, 0, &mem_true64_word_device
};
const gx_device_memory *
gdev_mem_word_device_for_bits(int bits_per_pixel)
{
    return ((uint)bits_per_pixel > 64 ? (const gx_device_memory *)0 :
	    mem_word_devices[bits_per_pixel]);
}

/* Test whether a device is a memory device */
bool
gs_device_is_memory(const gx_device * dev)
{
    /*
     * We use the draw_thin_line procedure to mark memory devices.
     * See gdevmem.h.
     */
    int bits_per_pixel = dev->color_info.depth;
    const gx_device_memory *mdproto;

    if ((uint)bits_per_pixel > 64)
	return false;
    mdproto = mem_devices[bits_per_pixel];
    if (mdproto != 0 && dev_proc(dev, draw_thin_line) == dev_proc(mdproto, draw_thin_line))
	return true;
    mdproto = mem_word_devices[bits_per_pixel];
    return (mdproto != 0 && dev_proc(dev, draw_thin_line) == dev_proc(mdproto, draw_thin_line));
}

/* Make a memory device. */
/* Note that the default for monobit devices is white = 0, black = 1. */
void
gs_make_mem_device(gx_device_memory * dev, const gx_device_memory * mdproto,
		   gs_memory_t * mem, int page_device, gx_device * target)
{
    gx_device_init((gx_device *) dev, (const gx_device *)mdproto,
		   mem, true);
    dev->stype = &st_device_memory;
    switch (page_device) {
	case -1:
	    set_dev_proc(dev, get_page_device, gx_default_get_page_device);
	    break;
	case 1:
	    set_dev_proc(dev, get_page_device, gx_page_device_get_page_device);
	    break;
    }
    /* Preload the black and white cache. */
    if (target == 0) {
	if (dev->color_info.depth == 1) {
	    /* The default for black-and-white devices is inverted. */
	    dev->cached_colors.black = 1;
	    dev->cached_colors.white = 0;
	} else {
	    dev->cached_colors.black = 0;
	    dev->cached_colors.white = (1 << dev->color_info.depth) - 1;
	}
    } else {
	gx_device_set_target((gx_device_forward *)dev, target);
	/* Forward the color mapping operations to the target. */
	gx_device_forward_color_procs((gx_device_forward *) dev);
	gx_device_copy_color_procs((gx_device *)dev, target);
	dev->cached_colors = target->cached_colors;
    }
    if (dev->color_info.depth == 1) {
	gdev_mem_mono_set_inverted(dev,
				   (target == 0 || 
                                    dev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE));
    }
    check_device_separable((gx_device *)dev);
    gx_device_fill_in_procs((gx_device *)dev);
}
/* Make a monobit memory device.  This is never a page device. */
/* Note that white=0, black=1. */
void
gs_make_mem_mono_device(gx_device_memory * dev, gs_memory_t * mem,
			gx_device * target)
{
    gx_device_init((gx_device *)dev, (const gx_device *)&mem_mono_device,
		   mem, true);
    set_dev_proc(dev, get_page_device, gx_default_get_page_device);
    gx_device_set_target((gx_device_forward *)dev, target);
    gdev_mem_mono_set_inverted(dev, true);
    check_device_separable((gx_device *)dev);
    gx_device_fill_in_procs((gx_device *)dev);
}

/* Define whether a monobit memory device is inverted (black=1). */
void
gdev_mem_mono_set_inverted(gx_device_memory * dev, bool black_is_1)
{
    if (black_is_1)
	dev->palette = mem_mono_b_w_palette;
    else
	dev->palette = mem_mono_w_b_palette;
}

/*
 * Compute the size of the bitmap storage, including the space for the scan
 * line pointer table.  Note that scan lines are padded to a multiple of
 * align_bitmap_mod bytes, and additional padding may be needed if the
 * pointer table must be aligned to an even larger modulus.
 *
 * The computation for planar devices is a little messier.  Each plane
 * must pad its scan lines, and then we must pad again for the pointer
 * tables (one table per plane).
 */
ulong
gdev_mem_bits_size(const gx_device_memory * dev, int width, int height)
{
    int num_planes = dev->num_planes;
    gx_render_plane_t plane1;
    const gx_render_plane_t *planes;
    ulong size;
    int pi;

    if (num_planes)
	planes = dev->planes;
    else
	planes = &plane1, plane1.depth = dev->color_info.depth, num_planes = 1;
    for (size = 0, pi = 0; pi < num_planes; ++pi)
	size += bitmap_raster(width * planes[pi].depth);
    return ROUND_UP(size * height, ARCH_ALIGN_PTR_MOD);
}
ulong
gdev_mem_line_ptrs_size(const gx_device_memory * dev, int width, int height)
{
    return (ulong)height * sizeof(byte *) * max(dev->num_planes, 1);
}
ulong
gdev_mem_data_size(const gx_device_memory * dev, int width, int height)
{
    return gdev_mem_bits_size(dev, width, height) +
	gdev_mem_line_ptrs_size(dev, width, height);
}
/*
 * Do the inverse computation: given a width (in pixels) and a buffer size,
 * compute the maximum height.
 */
int
gdev_mem_max_height(const gx_device_memory * dev, int width, ulong size,
		bool page_uses_transparency)
{
    int height;
    ulong max_height;

    if (page_uses_transparency) {
        /*
         * If the device is using PDF 1.4 transparency then we will need to
         * also allocate image buffers for doing the blending operations.
         * We can only estimate the space requirements.  However since it
	 * is only an estimate, we may exceed our desired buffer space while
	 * processing the file.
	 */
        max_height = size / (bitmap_raster(width
		* dev->color_info.depth + ESTIMATED_PDF14_ROW_SPACE(width))
		+ sizeof(byte *) * max(dev->num_planes, 1));
        height = (int)min(max_height, max_int);
    } else {
	/* For non PDF 1.4 transparency, we can do an exact calculation */
        max_height = size /
	    (bitmap_raster(width * dev->color_info.depth) +
	     sizeof(byte *) * max(dev->num_planes, 1));
        height = (int)min(max_height, max_int);
        /*
         * Because of alignment rounding, the just-computed height might
         * be too large by a small amount.  Adjust it the easy way.
         */
        while (gdev_mem_data_size(dev, width, height) > size)
	    --height;
    }
    return height;
}

/* Open a memory device, allocating the data area if appropriate, */
/* and create the scan line table. */
int
mem_open(gx_device * dev)
{
    gx_device_memory *const mdev = (gx_device_memory *)dev;

    /* Check that we aren't trying to open a planar device as chunky. */
    if (mdev->num_planes)
	return_error(gs_error_rangecheck);
    return gdev_mem_open_scan_lines(mdev, dev->height);
}
int
gdev_mem_open_scan_lines(gx_device_memory *mdev, int setup_height)
{
    bool line_pointers_adjacent = true;

    if (setup_height < 0 || setup_height > mdev->height)
	return_error(gs_error_rangecheck);
    if (mdev->bitmap_memory != 0) {
	/* Allocate the data now. */
	ulong size = gdev_mem_bitmap_size(mdev);

	if ((uint) size != size)
	    return_error(gs_error_limitcheck);
	mdev->base = gs_alloc_bytes(mdev->bitmap_memory, (uint)size,
				    "mem_open");
	if (mdev->base == 0)
	    return_error(gs_error_VMerror);
	mdev->foreign_bits = false;
    } else if (mdev->line_pointer_memory != 0) {
	/* Allocate the line pointers now. */

	mdev->line_ptrs = (byte **)
	    gs_alloc_byte_array(mdev->line_pointer_memory, mdev->height,
				sizeof(byte *) * max(mdev->num_planes, 1),
				"gdev_mem_open_scan_lines");
	if (mdev->line_ptrs == 0)
	    return_error(gs_error_VMerror);
	mdev->foreign_line_pointers = false;
	line_pointers_adjacent = false;
    }
    if (line_pointers_adjacent)
	mdev->line_ptrs = (byte **)
	    (mdev->base + gdev_mem_bits_size(mdev, mdev->width, mdev->height));
    mdev->raster = gdev_mem_raster(mdev);
    return gdev_mem_set_line_ptrs(mdev, NULL, 0, NULL, setup_height);
}
/*
 * Set up the scan line pointers of a memory device.
 * See gxdevmem.h for the detailed specification.
 * Sets or uses line_ptrs, base, raster; uses width, color_info.depth,
 * num_planes, plane_depths, plane_depth.
 */
int
gdev_mem_set_line_ptrs(gx_device_memory * mdev, byte * base, int raster,
		       byte **line_ptrs, int setup_height)
{
    int num_planes = mdev->num_planes;
    gx_render_plane_t plane1;
    const gx_render_plane_t *planes;
    byte **pline =
	(line_ptrs ? (mdev->line_ptrs = line_ptrs) : mdev->line_ptrs);
    byte *data =
	(base ? (mdev->raster = raster, mdev->base = base) :
	 (raster = mdev->raster, mdev->base));
    int pi;

    if (num_planes) {
	if (base && !mdev->plane_depth)
	    return_error(gs_error_rangecheck);
	planes = mdev->planes;
    } else {
	planes = &plane1;
	plane1.depth = mdev->color_info.depth;
	num_planes = 1;
    }

    for (pi = 0; pi < num_planes; ++pi) {
	int raster = bitmap_raster(mdev->width * planes[pi].depth);
	byte **pptr = pline;
	byte **pend = pptr + setup_height;
	byte *scan_line = data;

	while (pptr < pend) {
	    *pptr++ = scan_line;
	    scan_line += raster;
	}
	data += raster * mdev->height;
	pline += setup_height;	/* not mdev->height, see gxdevmem.h */
    }

    return 0;
}

/* Return the initial transformation matrix */
void
mem_get_initial_matrix(gx_device * dev, gs_matrix * pmat)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;

    pmat->xx = mdev->initial_matrix.xx;
    pmat->xy = mdev->initial_matrix.xy;
    pmat->yx = mdev->initial_matrix.yx;
    pmat->yy = mdev->initial_matrix.yy;
    pmat->tx = mdev->initial_matrix.tx;
    pmat->ty = mdev->initial_matrix.ty;
}

/* Close a memory device, freeing the data area if appropriate. */
int
mem_close(gx_device * dev)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;

    if (mdev->bitmap_memory != 0) {
	gs_free_object(mdev->bitmap_memory, mdev->base, "mem_close");
	/*
	 * The following assignment is strictly for the benefit of one
	 * client that is sloppy about using is_open properly.
	 */
	mdev->base = 0;
    } else if (mdev->line_pointer_memory != 0) {
	gs_free_object(mdev->line_pointer_memory, mdev->line_ptrs,
		       "mem_close");
	mdev->line_ptrs = 0;	/* ibid. */
    }
    return 0;
}

/* Copy bits to a client. */
#undef chunk
#define chunk byte
int
mem_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    gs_get_bits_options_t options = params->options;
    int x = prect->p.x, w = prect->q.x - x, y = prect->p.y, h = prect->q.y - y;

    if (options == 0) {
	params->options =
	    (GB_ALIGN_STANDARD | GB_ALIGN_ANY) |
	    (GB_RETURN_COPY | GB_RETURN_POINTER) |
	    (GB_OFFSET_0 | GB_OFFSET_SPECIFIED | GB_OFFSET_ANY) |
	    (GB_RASTER_STANDARD | GB_RASTER_SPECIFIED | GB_RASTER_ANY) |
	    GB_PACKING_CHUNKY | GB_COLORS_NATIVE | GB_ALPHA_NONE;
	return_error(gs_error_rangecheck);
    }
    if ((w <= 0) | (h <= 0)) {
	if ((w | h) < 0)
	    return_error(gs_error_rangecheck);
	return 0;
    }
    if (x < 0 || w > dev->width - x ||
	y < 0 || h > dev->height - y
	)
	return_error(gs_error_rangecheck);
    {
	gs_get_bits_params_t copy_params;
	byte *base = scan_line_base(mdev, y);
	int code;

	copy_params.options =
	    GB_COLORS_NATIVE | GB_PACKING_CHUNKY | GB_ALPHA_NONE |
	    (mdev->raster ==
	     bitmap_raster(mdev->width * mdev->color_info.depth) ?
	     GB_RASTER_STANDARD : GB_RASTER_SPECIFIED);
	copy_params.raster = mdev->raster;
	code = gx_get_bits_return_pointer(dev, x, h, params,
					  &copy_params, base);
	if (code >= 0)
	    return code;
	return gx_get_bits_copy(dev, x, w, h, params, &copy_params, base,
				gx_device_raster(dev, true));
    }
}

#if !arch_is_big_endian

/*
 * Swap byte order in a rectangular subset of a bitmap.  If store = true,
 * assume the rectangle will be overwritten, so don't swap any bytes where
 * it doesn't matter.  The caller has already done a fit_fill or fit_copy.
 * Note that the coordinates are specified in bits, not in terms of the
 * actual device depth.
 */
void
mem_swap_byte_rect(byte * base, uint raster, int x, int w, int h, bool store)
{
    int xbit = x & 31;

    if (store) {
	if (xbit + w > 64) {	/* Operation spans multiple words. */
	    /* Just swap the words at the left and right edges. */
	    if (xbit != 0)
		mem_swap_byte_rect(base, raster, x, 1, h, false);
	    x += w - 1;
	    xbit = x & 31;
	    if (xbit == 31)
		return;
	    w = 1;
	}
    }
    /* Swap the entire rectangle (or what's left of it). */
    {
	byte *row = base + ((x >> 5) << 2);
	int nw = (xbit + w + 31) >> 5;
	int ny;

	for (ny = h; ny > 0; row += raster, --ny) {
	    int nx = nw;
	    bits32 *pw = (bits32 *) row;

	    do {
		bits32 w = *pw;

		*pw++ = (w >> 24) + ((w >> 8) & 0xff00) +
		    ((w & 0xff00) << 8) + (w << 24);
	    }
	    while (--nx);
	}
    }
}

/* Copy a word-oriented rectangle to the client, swapping bytes as needed. */
int
mem_word_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte *src;
    uint dev_raster = gx_device_raster(dev, 1);
    int x = prect->p.x;
    int w = prect->q.x - x;
    int y = prect->p.y;
    int h = prect->q.y - y;
    int bit_x, bit_w;
    int code;

    fit_fill_xywh(dev, x, y, w, h);
    if (w <= 0 || h <= 0) {
	/*
	 * It's easiest to just keep going with an empty rectangle.
	 * We pass the original rectangle to mem_get_bits_rectangle,
	 * so unread will be filled in correctly.
	 */
	x = y = w = h = 0;
    }
    bit_x = x * dev->color_info.depth;
    bit_w = w * dev->color_info.depth;
    src = scan_line_base(mdev, y);
    mem_swap_byte_rect(src, dev_raster, bit_x, bit_w, h, false);
    code = mem_get_bits_rectangle(dev, prect, params, unread);
    mem_swap_byte_rect(src, dev_raster, bit_x, bit_w, h, false);
    return code;
}

#endif /* !arch_is_big_endian */

/* Map a r-g-b color to a color index for a mapped color memory device */
/* (2, 4, or 8 bits per pixel.) */
/* This requires searching the palette. */
gx_color_index
mem_mapped_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    byte br = gx_color_value_to_byte(cv[0]);
    
    register const byte *pptr = mdev->palette.data;
    int cnt = mdev->palette.size;
    const byte *which = 0;	/* initialized only to pacify gcc */
    int best = 256 * 3;

    if (mdev->color_info.num_components != 1) {
	/* not 1 component, assume three */
	/* The comparison is rather simplistic, treating differences in	*/
	/* all components as equal. Better choices would be 'distance'	*/
	/* in HLS space or other, but these would be much slower.	*/
	/* At least exact matches will be found.			*/
	byte bg = gx_color_value_to_byte(cv[1]);
	byte bb = gx_color_value_to_byte(cv[2]);

	while ((cnt -= 3) >= 0) {
	    register int diff = *pptr - br;

	    if (diff < 0)
		diff = -diff;
	    if (diff < best) {	/* quick rejection */
		    int dg = pptr[1] - bg;

		if (dg < 0)
		    dg = -dg;
		if ((diff += dg) < best) {	/* quick rejection */
		    int db = pptr[2] - bb;

		    if (db < 0)
			db = -db;
		    if ((diff += db) < best)
			which = pptr, best = diff;
		}
	    }
	    if (diff == 0)	/* can't get any better than 0 diff */
		break;
	    pptr += 3;
	}
    } else {
	/* Gray scale conversion. The palette is made of three equal	*/
	/* components, so this case is simpler.				*/
	while ((cnt -= 3) >= 0) {
	    register int diff = *pptr - br;

	    if (diff < 0)
		diff = -diff;
	    if (diff < best) {	/* quick rejection */
		which = pptr, best = diff;
	    }
	    if (diff == 0)
		break;
	    pptr += 3;
	}
    }
    return (gx_color_index) ((which - mdev->palette.data) / 3);
}

/* Map a color index to a r-g-b color for a mapped color memory device. */
int
mem_mapped_map_color_rgb(gx_device * dev, gx_color_index color,
			 gx_color_value prgb[3])
{
    gx_device_memory * const mdev = (gx_device_memory *)dev;
    const byte *pptr = mdev->palette.data + (int)color * 3;

    prgb[0] = gx_color_value_from_byte(pptr[0]);
    prgb[1] = gx_color_value_from_byte(pptr[1]);
    prgb[2] = gx_color_value_from_byte(pptr[2]);
    return 0;
}

/*
 * Implement draw_thin_line using a distinguished procedure that serves
 * as the common marker for all memory devices.
 */
int
mem_draw_thin_line(gx_device *dev, fixed fx0, fixed fy0, fixed fx1, fixed fy1,
		   const gx_drawing_color *pdcolor,
		   gs_logical_operation_t lop)
{
    return gx_default_draw_thin_line(dev, fx0, fy0, fx1, fy1, pdcolor, lop);
}
