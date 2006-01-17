/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevdgbr.c,v 1.14 2005/09/04 05:44:43 dan Exp $ */
/* Default implementation of device get_bits[_rectangle] */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxgetbit.h"
#include "gxlum.h"
#include "gdevmem.h"

int
gx_no_get_bits(gx_device * dev, int y, byte * data, byte ** actual_data)
{
    return_error(gs_error_unknownerror);
}
int
gx_default_get_bits(gx_device * dev, int y, byte * data, byte ** actual_data)
{	/*
	 * Hand off to get_bits_rectangle, being careful to avoid a
	 * possible recursion loop.
	 */
    dev_proc_get_bits((*save_get_bits)) = dev_proc(dev, get_bits);
    gs_int_rect rect;
    gs_get_bits_params_t params;
    int code;

    rect.p.x = 0, rect.p.y = y;
    rect.q.x = dev->width, rect.q.y = y + 1;
    params.options =
	(actual_data ? GB_RETURN_POINTER : 0) | GB_RETURN_COPY |
	(GB_ALIGN_STANDARD | GB_OFFSET_0 | GB_RASTER_STANDARD |
    /* No depth specified, we always use native colors. */
	 GB_PACKING_CHUNKY | GB_COLORS_NATIVE | GB_ALPHA_NONE);
    params.x_offset = 0;
    params.raster = bitmap_raster(dev->width * dev->color_info.depth);
    params.data[0] = data;
    set_dev_proc(dev, get_bits, gx_no_get_bits);
    code = (*dev_proc(dev, get_bits_rectangle))
	(dev, &rect, &params, NULL);
    if (actual_data)
	*actual_data = params.data[0];
    set_dev_proc(dev, get_bits, save_get_bits);
    return code;
}

/*
 * Determine whether we can satisfy a request by simply using the stored
 * representation.  dev is used only for color_info.{num_components, depth}.
 */
private bool
requested_includes_stored(const gx_device *dev,
			  const gs_get_bits_params_t *requested,
			  const gs_get_bits_params_t *stored)
{
    gs_get_bits_options_t both = requested->options & stored->options;

    if (!(both & GB_PACKING_ALL))
	return false;
    if (stored->options & GB_SELECT_PLANES) {
	/*
	 * The device only provides a subset of the planes.
	 * Make sure it provides all the requested ones.
	 */
	int i;
	int n = (stored->options & GB_PACKING_BIT_PLANAR ?
		 dev->color_info.depth : dev->color_info.num_components);

	if (!(requested->options & GB_SELECT_PLANES) ||
	    !(both & (GB_PACKING_PLANAR || GB_PACKING_BIT_PLANAR))
	    )
	    return false;
	for (i = 0; i < n; ++i)
	    if (requested->data[i] && !stored->data[i])
		return false;
    }
    if (both & GB_COLORS_NATIVE)
	return true;
    if (both & GB_COLORS_STANDARD_ALL) {
	if ((both & GB_ALPHA_ALL) && (both & GB_DEPTH_ALL))
	    return true;
    }
    return false;
}

/*
 * Try to implement get_bits_rectangle by returning a pointer.
 * Note that dev is used only for computing the default raster
 * and for color_info.depth.
 * This routine does not check x or h for validity.
 */
int
gx_get_bits_return_pointer(gx_device * dev, int x, int h,
			   gs_get_bits_params_t *params,
			   const gs_get_bits_params_t *stored,
			   byte * stored_base)
{
    gs_get_bits_options_t options = params->options;
    gs_get_bits_options_t both = options & stored->options;

    if (!(options & GB_RETURN_POINTER) ||
	!requested_includes_stored(dev, params, stored)
	)
	return -1;
    /*
     * See whether we can return the bits in place.  Note that even if
     * OFFSET_ANY isn't set, x_offset and x don't have to be equal: their
     * bit offsets only have to match modulo align_bitmap_mod * 8 (to
     * preserve alignment) if ALIGN_ANY isn't set, or mod 8 (since
     * byte alignment is always required) if ALIGN_ANY is set.
     */
    {
	int depth = dev->color_info.depth;
	/*
	 * For PLANAR devices, we assume that each plane consists of
	 * depth/num_components bits.  This is wrong in general, but if
	 * the device wants something else, it should implement
	 * get_bits_rectangle itself.
	 */
	uint dev_raster =
	    (both & GB_PACKING_CHUNKY ?
	       gx_device_raster(dev, true) :
	     both & GB_PACKING_PLANAR ?
	       bitmap_raster(dev->color_info.depth /
			     dev->color_info.num_components * dev->width) :
	     both & GB_PACKING_BIT_PLANAR ?
	       bitmap_raster(dev->width) :
	     0 /* not possible */);
	uint raster =
	    (options & (GB_RASTER_STANDARD | GB_RASTER_ANY) ? dev_raster :
	     params->raster);
	byte *base;

	if (h <= 1 || raster == dev_raster) {
	    int x_offset =
		(options & GB_OFFSET_ANY ? x :
		 options & GB_OFFSET_0 ? 0 : params->x_offset);

	    if (x_offset == x) {
		base = stored_base;
		params->x_offset = x;
	    } else {
		uint align_mod =
		    (options & GB_ALIGN_ANY ? 8 : align_bitmap_mod * 8);
		int bit_offset = x - x_offset;
		int bytes;

		if (bit_offset & (align_mod - 1))
		    return -1;	/* can't align */
		if (depth & (depth - 1)) {
		    /* step = lcm(depth, align_mod) */
		    int step = depth / igcd(depth, align_mod) * align_mod;

		    bytes = bit_offset / step * step;
		} else {
		    /* Use a faster algorithm if depth is a power of 2. */
		    bytes = bit_offset & (-depth & -(int)align_mod);
		}
		base = stored_base + arith_rshift(bytes, 3);
		params->x_offset = (bit_offset - bytes) / depth;
	    }
	    params->options =
		GB_ALIGN_STANDARD | GB_RETURN_POINTER | GB_RASTER_STANDARD |
		(stored->options & ~GB_PACKING_ALL) /*see below for PACKING*/ |
		(params->x_offset == 0 ? GB_OFFSET_0 : GB_OFFSET_SPECIFIED);
	    if (both & GB_PACKING_CHUNKY) {
		params->options |= GB_PACKING_CHUNKY;
		params->data[0] = base;
	    } else {
		int n =
		    (stored->options & GB_PACKING_BIT_PLANAR ?
		       (params->options |= GB_PACKING_BIT_PLANAR,
			dev->color_info.depth) :
		       (params->options |= GB_PACKING_PLANAR,
			dev->color_info.num_components));
		int i;

		for (i = 0; i < n; ++i)
		    if (!(both & GB_SELECT_PLANES) || stored->data[i] != 0) {
			params->data[i] = base;
			base += dev_raster * dev->height;
		    }
	    }
	    return 0;
	}
    }
    return -1;
}

/*
 * Implement gx_get_bits_copy (see below) for the case of converting
 * 4-bit CMYK to 24-bit RGB with standard mapping, used heavily by PCL.
 */
private void
gx_get_bits_copy_cmyk_1bit(byte *dest_line, uint dest_raster,
			   const byte *src_line, uint src_raster,
			   int src_bit, int w, int h)
{
    for (; h > 0; dest_line += dest_raster, src_line += src_raster, --h) {
	const byte *src = src_line;
	byte *dest = dest_line;
	bool hi = (src_bit & 4) != 0;  /* last nibble processed was hi */
	int i;

	for (i = w; i > 0; dest += 3, --i) {
	    uint pixel =
		((hi = !hi)? *src >> 4 : *src++ & 0xf);

	    if (pixel & 1)
		dest[0] = dest[1] = dest[2] = 0;
	    else {
		dest[0] = (byte)((pixel >> 3) - 1);
		dest[1] = (byte)(((pixel >> 2) & 1) - 1);
		dest[2] = (byte)(((pixel >> 1) & 1) - 1);
	    }
	}
    }
}

/*
 * Convert pixels between representations, primarily for get_bits_rectangle.
 * stored indicates how the data are actually stored, and includes:
 *      - one option from the GB_PACKING group;
 *      - if h > 1, one option from the GB_RASTER group;
 *      - optionally (and normally), GB_COLORS_NATIVE;
 *      - optionally, one option each from the GB_COLORS_STANDARD, GB_DEPTH,
 *      and GB_ALPHA groups.
 * Note that dev is used only for color mapping.  This routine assumes that
 * the stored data are aligned.
 *
 * Note: this routine does not check x, w, h for validity.
 *
 * The code for converting between standard and native colors has been
 * factored out into single-use procedures strictly for readability.
 * A good optimizing compiler would compile them in-line.
 */
private int
    gx_get_bits_std_to_native(gx_device * dev, int x, int w, int h,
				  gs_get_bits_params_t * params,
			      const gs_get_bits_params_t *stored,
			      const byte * src_base, uint dev_raster,
			      int x_offset, uint raster),
    gx_get_bits_native_to_std(gx_device * dev, int x, int w, int h,
			      gs_get_bits_params_t * params,
			      const gs_get_bits_params_t *stored,
			      const byte * src_base, uint dev_raster,
			      int x_offset, uint raster, uint std_raster);
int
gx_get_bits_copy(gx_device * dev, int x, int w, int h,
		 gs_get_bits_params_t * params,
		 const gs_get_bits_params_t *stored,
		 const byte * src_base, uint dev_raster)
{
    gs_get_bits_options_t options = params->options;
    gs_get_bits_options_t stored_options = stored->options;
    int x_offset = (options & GB_OFFSET_0 ? 0 : params->x_offset);
    int depth = dev->color_info.depth;
    int bit_x = x * depth;
    const byte *src = src_base;
    /*
     * If the stored representation matches a requested representation,
     * we can copy the data without any transformations.
     */
    bool direct_copy = requested_includes_stored(dev, params, stored);
    int code = 0;

    /*
     * The request must include either GB_PACKING_CHUNKY or
     * GB_PACKING_PLANAR + GB_SELECT_PLANES, GB_RETURN_COPY,
     * and an offset and raster specification.  In the planar case,
     * the request must include GB_ALIGN_STANDARD, the stored
     * representation must include GB_PACKING_CHUNKY, and both must
     * include GB_COLORS_NATIVE.
     */
    if ((~options & GB_RETURN_COPY) ||
	!(options & (GB_OFFSET_0 | GB_OFFSET_SPECIFIED)) ||
	!(options & (GB_RASTER_STANDARD | GB_RASTER_SPECIFIED))
	)
	return_error(gs_error_rangecheck);
    if (options & GB_PACKING_CHUNKY) {
	byte *data = params->data[0];
	int end_bit = (x_offset + w) * depth;
	uint std_raster =
	    (options & GB_ALIGN_STANDARD ? bitmap_raster(end_bit) :
	     (end_bit + 7) >> 3);
	uint raster =
	    (options & GB_RASTER_STANDARD ? std_raster : params->raster);
	int dest_bit_x = x_offset * depth;
	int skew = bit_x - dest_bit_x;

	/*
	 * If the bit positions line up, use bytes_copy_rectangle.
	 * Since bytes_copy_rectangle doesn't require alignment,
	 * the bit positions only have to match within a byte,
	 * not within align_bitmap_mod bytes.
	 */
	if (!(skew & 7) && direct_copy) {
	    int bit_w = w * depth;

	    bytes_copy_rectangle(data + (dest_bit_x >> 3), raster,
				 src + (bit_x >> 3), dev_raster,
			      ((bit_x + bit_w + 7) >> 3) - (bit_x >> 3), h);
	} else if (direct_copy) {
	    /*
	     * Use the logic already in mem_mono_copy_mono to copy the
	     * bits to the destination.  We do this one line at a time,
	     * to avoid having to allocate a line pointer table.
	     */
	    gx_device_memory tdev;
	    byte *line_ptr = data;
	    int bit_w = w * depth;

	    tdev.line_ptrs = &tdev.base;
	    for (; h > 0; line_ptr += raster, src += dev_raster, --h) {
		/* Make sure the destination is aligned. */
		int align = ALIGNMENT_MOD(line_ptr, align_bitmap_mod);

		tdev.base = line_ptr - align;
		/* set up parameters required by copy_mono's fit_copy */
		tdev.width = dest_bit_x + (align << 3) + bit_w;
		tdev.height = 1;
		(*dev_proc(&mem_mono_device, copy_mono))
		    ((gx_device *) & tdev, src, bit_x, dev_raster, gx_no_bitmap_id,
		     dest_bit_x + (align << 3), 0, bit_w, 1,
		     (gx_color_index) 0, (gx_color_index) 1);
	    }
	} else if (options & ~stored_options & GB_COLORS_NATIVE) {
	    /* Convert standard colors to native. */
	    code = gx_get_bits_std_to_native(dev, x, w, h, params, stored,
					     src_base, dev_raster,
					     x_offset, raster);
	    options = params->options;
	} else {
	    /* Convert native colors to standard. */
	    code = gx_get_bits_native_to_std(dev, x, w, h, params, stored,
					     src_base, dev_raster,
					     x_offset, raster, std_raster);
	    options = params->options;
	}
	params->options =
	    (options & (GB_COLORS_ALL | GB_ALPHA_ALL)) | GB_PACKING_CHUNKY |
	    (options & GB_COLORS_NATIVE ? 0 : options & GB_DEPTH_ALL) |
	    (options & GB_ALIGN_STANDARD ? GB_ALIGN_STANDARD : GB_ALIGN_ANY) |
	    GB_RETURN_COPY |
	    (x_offset == 0 ? GB_OFFSET_0 : GB_OFFSET_SPECIFIED) |
	    (raster == std_raster ? GB_RASTER_STANDARD : GB_RASTER_SPECIFIED);
    } else if (!(~options &
		 (GB_PACKING_PLANAR | GB_SELECT_PLANES | GB_ALIGN_STANDARD)) &&
	       (stored_options & GB_PACKING_CHUNKY) &&
	       ((options & stored_options) & GB_COLORS_NATIVE)
	       ) {
	int num_planes = dev->color_info.num_components;
	int dest_depth = depth / num_planes;
	bits_plane_t source, dest;
	int plane = -1;
	int i;

	/* Make sure only one plane is being requested. */
	for (i = 0; i < num_planes; ++i)
	    if (params->data[i] != 0) {
		if (plane >= 0)
		    return_error(gs_error_rangecheck); /* > 1 plane */
		plane = i;
	    }
	source.data.read = src_base;
	source.raster = dev_raster;
	source.depth = depth;
	source.x = x;
	dest.data.write = params->data[plane];
	dest.raster =
	    (options & GB_RASTER_STANDARD ?
	     bitmap_raster((x_offset + w) * dest_depth) : params->raster);
	dest.depth = dest_depth;
	dest.x = x_offset;
	return bits_extract_plane(&dest, &source,
				  (num_planes - 1 - plane) * dest_depth,
				  w, h);
    } else
	return_error(gs_error_rangecheck);
    return code;
}

/*
 * Convert standard colors to native.  Note that the source
 * may have depths other than 8 bits per component.
 */
private int
gx_get_bits_std_to_native(gx_device * dev, int x, int w, int h,
			  gs_get_bits_params_t * params,
			  const gs_get_bits_params_t *stored,
			  const byte * src_base, uint dev_raster,
			  int x_offset, uint raster)
{
    int depth = dev->color_info.depth;
    int dest_bit_offset = x_offset * depth;
    byte *dest_line = params->data[0] + (dest_bit_offset >> 3);
    int ncolors =
	(stored->options & GB_COLORS_RGB ? 3 :
	 stored->options & GB_COLORS_CMYK ? 4 :
	 stored->options & GB_COLORS_GRAY ? 1 : -1);
    int ncomp = ncolors +
	((stored->options & (GB_ALPHA_FIRST | GB_ALPHA_LAST)) != 0);
    int src_depth = GB_OPTIONS_DEPTH(stored->options);
    int src_bit_offset = x * src_depth * ncomp;
    const byte *src_line = src_base + (src_bit_offset >> 3);
    gx_color_value src_max = (1 << src_depth) - 1;
#define v2cv(value) ((ulong)(value) * gx_max_color_value / src_max)
    gx_color_value alpha_default = src_max;

    params->options &= ~GB_COLORS_ALL | GB_COLORS_NATIVE;
    for (; h > 0; dest_line += raster, src_line += dev_raster, --h) {
	int i;

	sample_load_declare_setup(src, sbit, src_line,
				  src_bit_offset & 7, src_depth);
	sample_store_declare_setup(dest, dbit, dbyte, dest_line,
				   dest_bit_offset & 7, depth);

#define v2frac(value) ((long)(value) * frac_1 / src_max)

        for (i = 0; i < w; ++i) {
            int j;
            frac sc[4], dc[GX_DEVICE_COLOR_MAX_COMPONENTS];
            gx_color_value v[GX_DEVICE_COLOR_MAX_COMPONENTS], va = alpha_default;
            gx_color_index pixel;
            bool do_alpha = false;
            const gx_cm_color_map_procs * map_procs;

            map_procs = dev_proc(dev, get_color_mapping_procs)(dev);

            /* Fetch the source data. */
            if (stored->options & GB_ALPHA_FIRST) {
                sample_load_next16(va, src, sbit, src_depth);
                va = v2cv(va);
                do_alpha = true;
            }
            for (j = 0; j < ncolors; ++j) {
                gx_color_value vj;

                sample_load_next16(vj, src, sbit, src_depth);
                sc[j] = v2frac(vj);
            }
            if (stored->options & GB_ALPHA_LAST) {
                sample_load_next16(va, src, sbit, src_depth);
                va = v2cv(va);
                do_alpha = true;
            }

            /* Convert and store the pixel value. */
            if (do_alpha) {
                for (j = 0; j < ncolors; j++)
                    v[j] = frac2cv(sc[j]);
                if (ncolors == 1)
                    v[2] = v[1] = v[0];
                pixel = dev_proc(dev, map_rgb_alpha_color)
                    (dev, v[0], v[1], v[2], va);
            } else {

                switch (ncolors) {
                case 1:
                    map_procs->map_gray(dev, sc[0], dc);
                    break;
                case 3:
                    map_procs->map_rgb(dev, 0, sc[0], sc[1], sc[2], dc);
                    break;
                case 4:
                    map_procs->map_cmyk(dev, sc[0], sc[1], sc[2], sc[3], dc);
                    break;
                default:
                    return_error(gs_error_rangecheck);
                }

                for (j = 0; j < dev->color_info.num_components; j++)
                    v[j] = frac2cv(dc[j]);

                pixel = dev_proc(dev, encode_color)(dev, v);
            }
            sample_store_next_any(pixel, dest, dbit, depth, dbyte);
        }
	sample_store_flush(dest, dbit, depth, dbyte);
    }
    return 0;
}

/*
 * Convert native colors to standard.  Only GB_DEPTH_8 is supported.
 */
private int
gx_get_bits_native_to_std(gx_device * dev, int x, int w, int h,
			  gs_get_bits_params_t * params,
			  const gs_get_bits_params_t *stored,
			  const byte * src_base, uint dev_raster,
			  int x_offset, uint raster, uint std_raster)
{
    int depth = dev->color_info.depth;
    int src_bit_offset = x * depth;
    const byte *src_line = src_base + (src_bit_offset >> 3);
    gs_get_bits_options_t options = params->options;
    int ncomp =
	(options & (GB_ALPHA_FIRST | GB_ALPHA_LAST) ? 4 : 3);
    byte *dest_line = params->data[0] + x_offset * ncomp;
    byte *mapped[16];
    int dest_bytes;
    int i;

    if (!(options & GB_DEPTH_8)) {
	/*
	 * We don't support general depths yet, or conversion between
	 * different formats.  Punt.
	 */
	return_error(gs_error_rangecheck);
    }

    /* Pick the representation that's most likely to be useful. */
    if (options & GB_COLORS_RGB)
	params->options = options &= ~GB_COLORS_STANDARD_ALL | GB_COLORS_RGB,
	    dest_bytes = 3;
    else if (options & GB_COLORS_CMYK)
	params->options = options &= ~GB_COLORS_STANDARD_ALL | GB_COLORS_CMYK,
	    dest_bytes = 4;
    else if (options & GB_COLORS_GRAY)
	params->options = options &= ~GB_COLORS_STANDARD_ALL | GB_COLORS_GRAY,
	    dest_bytes = 1;
    else
	return_error(gs_error_rangecheck);
    /* Recompute the destination raster based on the color space. */
    if (options & GB_RASTER_STANDARD) {
	uint end_byte = (x_offset + w) * dest_bytes;

	raster = std_raster =
	    (options & GB_ALIGN_STANDARD ?
	     bitmap_raster(end_byte << 3) : end_byte);
    }
    /* Check for the one special case we care about. */
    if (((options & (GB_COLORS_RGB | GB_ALPHA_FIRST | GB_ALPHA_LAST))
	   == GB_COLORS_RGB) &&
	dev_proc(dev, map_color_rgb) == cmyk_1bit_map_color_rgb) {
	gx_get_bits_copy_cmyk_1bit(dest_line, raster,
				   src_line, dev_raster,
				   src_bit_offset & 7, w, h);
	return 0;
    }
    if (options & (GB_ALPHA_FIRST | GB_ALPHA_LAST))
	++dest_bytes;
    /* Clear the color translation cache. */
    for (i = (depth > 4 ? 16 : 1 << depth); --i >= 0; )
	mapped[i] = 0;
    for (; h > 0; dest_line += raster, src_line += dev_raster, --h) {
	sample_load_declare_setup(src, bit, src_line,
				  src_bit_offset & 7, depth);
	byte *dest = dest_line;

	for (i = 0; i < w; ++i) {
	    gx_color_index pixel = 0;
	    gx_color_value rgba[4];

	    sample_load_next_any(pixel, src, bit, depth);
	    if (pixel < 16) {
		if (mapped[pixel]) {
		    /* Use the value from the cache. */
		    memcpy(dest, mapped[pixel], dest_bytes);
		    dest += dest_bytes;
		    continue;
		}
		mapped[pixel] = dest;
	    }
	    (*dev_proc(dev, map_color_rgb_alpha)) (dev, pixel, rgba);
	    if (options & GB_ALPHA_FIRST)
		*dest++ = gx_color_value_to_byte(rgba[3]);
	    /* Convert to the requested color space. */
	    if (options & GB_COLORS_RGB) {
		dest[0] = gx_color_value_to_byte(rgba[0]);
		dest[1] = gx_color_value_to_byte(rgba[1]);
		dest[2] = gx_color_value_to_byte(rgba[2]);
		dest += 3;
	    } else if (options & GB_COLORS_CMYK) {
		/* Use the standard RGB to CMYK algorithm, */
		/* with maximum black generation and undercolor removal. */
		gx_color_value white = max(rgba[0], max(rgba[1], rgba[2]));

		dest[0] = gx_color_value_to_byte(white - rgba[0]);
		dest[1] = gx_color_value_to_byte(white - rgba[1]);
		dest[2] = gx_color_value_to_byte(white - rgba[2]);
		dest[3] = gx_color_value_to_byte(gx_max_color_value - white);
		dest += 4;
	    } else {	/* GB_COLORS_GRAY */
		/* Use the standard RGB to Gray algorithm. */
		*dest++ = gx_color_value_to_byte(
				((rgba[0] * (ulong) lum_red_weight) +
				 (rgba[1] * (ulong) lum_green_weight) +
				 (rgba[2] * (ulong) lum_blue_weight) +
				   (lum_all_weights / 2))
				/ lum_all_weights);
	    }
	    if (options & GB_ALPHA_LAST)
		*dest++ = gx_color_value_to_byte(rgba[3]);
	}
    }
    return 0;
}

/* ------ Default implementations of get_bits_rectangle ------ */

int
gx_no_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    return_error(gs_error_unknownerror);
}

int
gx_default_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    dev_proc_get_bits_rectangle((*save_get_bits_rectangle)) =
	dev_proc(dev, get_bits_rectangle);
    int depth = dev->color_info.depth;
    uint min_raster = (dev->width * depth + 7) >> 3;
    gs_get_bits_options_t options = params->options;
    int code;

    /* Avoid a recursion loop. */
    set_dev_proc(dev, get_bits_rectangle, gx_no_get_bits_rectangle);
    /*
     * If the parameters are right, try to call get_bits directly.  Note
     * that this may fail if a device only implements get_bits_rectangle
     * (not get_bits) for a limited set of options.  Note also that this
     * must handle the case of the recursive call from within
     * get_bits_rectangle (see below): because of this, and only because
     * of this, it must handle partial scan lines.
     */
    if (prect->q.y == prect->p.y + 1 &&
	!(~options &
	  (GB_RETURN_COPY | GB_PACKING_CHUNKY | GB_COLORS_NATIVE)) &&
	(options & (GB_ALIGN_STANDARD | GB_ALIGN_ANY)) &&
	((options & (GB_OFFSET_0 | GB_OFFSET_ANY)) ||
	 ((options & GB_OFFSET_SPECIFIED) && params->x_offset == 0)) &&
	((options & (GB_RASTER_STANDARD | GB_RASTER_ANY)) ||
	 ((options & GB_RASTER_SPECIFIED) &&
	  params->raster >= min_raster)) &&
	unread == NULL
	) {
	byte *data = params->data[0];
	byte *row = data;

	if (!(prect->p.x == 0 && prect->q.x == dev->width)) {
	    /* Allocate an intermediate row buffer. */
	    row = gs_alloc_bytes(dev->memory, min_raster,
				 "gx_default_get_bits_rectangle");

	    if (row == 0) {
		code = gs_note_error(gs_error_VMerror);
		goto ret;
	    }
	}
	code = (*dev_proc(dev, get_bits)) (dev, prect->p.y, row,
		(params->options & GB_RETURN_POINTER) ? &params->data[0]
						      : NULL );
	if (code >= 0) {
	    if (row != data) {
		if (prect->p.x == 0 && params->data[0] != row
		    && params->options & GB_RETURN_POINTER) {
		    /*
		     * get_bits returned an appropriate pointer: we can
		     * avoid doing any copying.
		     */
		    DO_NOTHING;
		} else {
		    /* Copy the partial row into the supplied buffer. */
		    int width_bits = (prect->q.x - prect->p.x) * depth;
		    gx_device_memory tdev;

		    tdev.width = width_bits;
		    tdev.height = 1;
		    tdev.line_ptrs = &tdev.base;
		    tdev.base = data;
		    code = (*dev_proc(&mem_mono_device, copy_mono))
			((gx_device *) & tdev, row, prect->p.x * depth,
			 min_raster, gx_no_bitmap_id, 0, 0, width_bits, 1,
			 (gx_color_index) 0, (gx_color_index) 1);
		    params->data[0] = data;
		}
		gs_free_object(dev->memory, row,
			       "gx_default_get_bits_rectangle");
	    }
	    params->options =
		GB_ALIGN_STANDARD | GB_OFFSET_0 | GB_PACKING_CHUNKY |
		GB_ALPHA_NONE | GB_COLORS_NATIVE | GB_RASTER_STANDARD |
		(params->data[0] == data ? GB_RETURN_COPY : GB_RETURN_POINTER);
	    goto ret;
	}
    } {
	/* Do the transfer row-by-row using a buffer. */
	int x = prect->p.x, w = prect->q.x - x;
	int bits_per_pixel = depth;
	byte *row;

	if (options & GB_COLORS_STANDARD_ALL) {
	    /*
	     * Make sure the row buffer can hold the standard color
	     * representation, in case the device decides to use it.
	     */
	    int bpc = GB_OPTIONS_MAX_DEPTH(options);
	    int nc =
	    (options & GB_COLORS_CMYK ? 4 :
	     options & GB_COLORS_RGB ? 3 : 1) +
	    (options & (GB_ALPHA_ALL - GB_ALPHA_NONE) ? 1 : 0);
	    int bpp = bpc * nc;

	    if (bpp > bits_per_pixel)
		bits_per_pixel = bpp;
	}
	row = gs_alloc_bytes(dev->memory, (bits_per_pixel * w + 7) >> 3,
			     "gx_default_get_bits_rectangle");
	if (row == 0) {
	    code = gs_note_error(gs_error_VMerror);
	} else {
	    uint dev_raster = gx_device_raster(dev, true);
	    uint raster =
	    (options & GB_RASTER_SPECIFIED ? params->raster :
	     options & GB_ALIGN_STANDARD ? bitmap_raster(depth * w) :
	     (depth * w + 7) >> 3);
	    gs_int_rect rect;
	    gs_get_bits_params_t copy_params;
	    gs_get_bits_options_t copy_options =
		(GB_ALIGN_STANDARD | GB_ALIGN_ANY) |
		(GB_RETURN_COPY | GB_RETURN_POINTER) |
		(GB_OFFSET_0 | GB_OFFSET_ANY) |
		(GB_RASTER_STANDARD | GB_RASTER_ANY) | GB_PACKING_CHUNKY |
		GB_COLORS_NATIVE | (options & (GB_DEPTH_ALL | GB_COLORS_ALL)) |
		GB_ALPHA_ALL;
	    byte *dest = params->data[0];
	    int y;

	    rect.p.x = x, rect.q.x = x + w;
	    code = 0;
	    for (y = prect->p.y; y < prect->q.y; ++y) {
		rect.p.y = y, rect.q.y = y + 1;
		copy_params.options = copy_options;
		copy_params.data[0] = row;
		code = (*save_get_bits_rectangle)
		    (dev, &rect, &copy_params, NULL);
		if (code < 0)
		    break;
		if (copy_params.options & GB_OFFSET_0)
		    copy_params.x_offset = 0;
		params->data[0] = dest + (y - prect->p.y) * raster;
		code = gx_get_bits_copy(dev, copy_params.x_offset, w, 1,
					params, &copy_params,
					copy_params.data[0], dev_raster);
		if (code < 0)
		    break;
	    }
	    gs_free_object(dev->memory, row, "gx_default_get_bits_rectangle");
	    params->data[0] = dest;
	}
    }
  ret:set_dev_proc(dev, get_bits_rectangle, save_get_bits_rectangle);
    return (code < 0 ? code : 0);
}


