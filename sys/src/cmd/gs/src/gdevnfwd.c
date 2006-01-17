/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevnfwd.c,v 1.28 2005/03/14 18:08:36 dan Exp $ */
/* Null and forwarding device implementation */
#include "gx.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gxcmap.h"
#include "memory_.h"

/* ---------------- Forwarding procedures ---------------- */

/* Additional finalization for forwarding devices. */
private void
gx_device_forward_finalize(gx_device *dev)
{
    gx_device *target = ((gx_device_forward *)dev)->target;

    ((gx_device_forward *)dev)->target = 0;
    rc_decrement_only(target, "gx_device_forward_finalize");
}

/* Set the target of a forwarding device. */
void
gx_device_set_target(gx_device_forward *fdev, gx_device *target)
{
    /*
     * ****** HACK: if this device doesn't have special finalization yet,
     * make it decrement the reference count of the target.
     */
    if (target && !fdev->finalize)
	fdev->finalize = gx_device_forward_finalize;
    rc_assign(fdev->target, target, "gx_device_set_target");
}

/* Fill in NULL procedures in a forwarding device procedure record. */
/* We don't fill in: open_device, close_device, or the lowest-level */
/* drawing operations. */
void
gx_device_forward_fill_in_procs(register gx_device_forward * dev)
{
    gx_device_set_procs((gx_device *) dev);
    /* NOT open_device */
    fill_dev_proc(dev, get_initial_matrix, gx_forward_get_initial_matrix);
    fill_dev_proc(dev, sync_output, gx_forward_sync_output);
    fill_dev_proc(dev, output_page, gx_forward_output_page);
    /* NOT close_device */
    fill_dev_proc(dev, map_rgb_color, gx_forward_map_rgb_color);
    fill_dev_proc(dev, map_color_rgb, gx_forward_map_color_rgb);
    /* NOT fill_rectangle */
    /* NOT tile_rectangle */
    /* NOT copy_mono */
    /* NOT copy_color */
    /* NOT draw_line (OBSOLETE) */
    fill_dev_proc(dev, get_bits, gx_forward_get_bits);
    fill_dev_proc(dev, get_params, gx_forward_get_params);
    fill_dev_proc(dev, put_params, gx_forward_put_params);
    fill_dev_proc(dev, map_cmyk_color, gx_forward_map_cmyk_color);
    fill_dev_proc(dev, get_xfont_procs, gx_forward_get_xfont_procs);
    fill_dev_proc(dev, get_xfont_device, gx_forward_get_xfont_device);
    fill_dev_proc(dev, map_rgb_alpha_color, gx_forward_map_rgb_alpha_color);
    fill_dev_proc(dev, get_page_device, gx_forward_get_page_device);
    /* NOT get_alpha_bits (OBSOLETE) */
    /* NOT copy_alpha */
    fill_dev_proc(dev, get_band, gx_forward_get_band);
    fill_dev_proc(dev, copy_rop, gx_forward_copy_rop);
    fill_dev_proc(dev, fill_path, gx_forward_fill_path);
    fill_dev_proc(dev, stroke_path, gx_forward_stroke_path);
    fill_dev_proc(dev, fill_mask, gx_forward_fill_mask);
    fill_dev_proc(dev, fill_trapezoid, gx_forward_fill_trapezoid);
    fill_dev_proc(dev, fill_parallelogram, gx_forward_fill_parallelogram);
    fill_dev_proc(dev, fill_triangle, gx_forward_fill_triangle);
    fill_dev_proc(dev, draw_thin_line, gx_forward_draw_thin_line);
    fill_dev_proc(dev, begin_image, gx_forward_begin_image);
    /* NOT image_data (OBSOLETE) */
    /* NOT end_image (OBSOLETE) */
    /* NOT strip_tile_rectangle */
    fill_dev_proc(dev, strip_copy_rop, gx_forward_strip_copy_rop);
    fill_dev_proc(dev, get_clipping_box, gx_forward_get_clipping_box);
    fill_dev_proc(dev, begin_typed_image, gx_forward_begin_typed_image);
    fill_dev_proc(dev, get_bits_rectangle, gx_forward_get_bits_rectangle);
    fill_dev_proc(dev, map_color_rgb_alpha, gx_forward_map_color_rgb_alpha);
    fill_dev_proc(dev, create_compositor, gx_no_create_compositor);
    fill_dev_proc(dev, get_hardware_params, gx_forward_get_hardware_params);
    fill_dev_proc(dev, text_begin, gx_forward_text_begin);
    fill_dev_proc(dev, get_color_mapping_procs, gx_forward_get_color_mapping_procs);
    fill_dev_proc(dev, get_color_comp_index, gx_forward_get_color_comp_index);
    fill_dev_proc(dev, encode_color, gx_forward_encode_color);
    fill_dev_proc(dev, decode_color, gx_forward_decode_color);
    fill_dev_proc(dev, pattern_manage, gx_forward_pattern_manage);
    fill_dev_proc(dev, fill_rectangle_hl_color, gx_forward_fill_rectangle_hl_color);
    fill_dev_proc(dev, include_color_space, gx_forward_include_color_space);
    fill_dev_proc(dev, fill_linear_color_scanline, gx_forward_fill_linear_color_scanline);
    fill_dev_proc(dev, fill_linear_color_trapezoid, gx_forward_fill_linear_color_trapezoid);
    fill_dev_proc(dev, fill_linear_color_triangle, gx_forward_fill_linear_color_triangle);
    fill_dev_proc(dev, update_spot_equivalent_colors, gx_forward_update_spot_equivalent_colors);
    gx_device_fill_in_procs((gx_device *) dev);
}

/* Forward the color mapping procedures from a device to its target. */
void
gx_device_forward_color_procs(gx_device_forward * dev)
{
    set_dev_proc(dev, map_rgb_color, gx_forward_map_rgb_color);
    set_dev_proc(dev, map_color_rgb, gx_forward_map_color_rgb);
    set_dev_proc(dev, map_cmyk_color, gx_forward_map_cmyk_color);
    set_dev_proc(dev, map_rgb_alpha_color, gx_forward_map_rgb_alpha_color);
    set_dev_proc(dev, get_color_mapping_procs, gx_forward_get_color_mapping_procs);
    set_dev_proc(dev, get_color_comp_index, gx_forward_get_color_comp_index);
    set_dev_proc(dev, encode_color, gx_forward_encode_color);
    set_dev_proc(dev, decode_color, gx_forward_decode_color);
}

int
gx_forward_close_device(gx_device * dev)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0) ? gx_default_close_device(dev)
		       : dev_proc(tdev, close_device)(tdev);
}

void
gx_forward_get_initial_matrix(gx_device * dev, gs_matrix * pmat)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	gx_default_get_initial_matrix(dev, pmat);
    else
	dev_proc(tdev, get_initial_matrix)(tdev, pmat);
}

int
gx_forward_sync_output(gx_device * dev)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_sync_output(dev) :
	    dev_proc(tdev, sync_output)(tdev));
}

int
gx_forward_output_page(gx_device * dev, int num_copies, int flush)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    int code =
	(tdev == 0 ? gx_default_output_page(dev, num_copies, flush) :
	 dev_proc(tdev, output_page)(tdev, num_copies, flush));

    if (code >= 0 && tdev != 0)
	dev->PageCount = tdev->PageCount;
    return code;
}

gx_color_index
gx_forward_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_error_encode_color(dev, cv) :
	    dev_proc(tdev, map_rgb_color)(tdev, cv));
}

int
gx_forward_map_color_rgb(gx_device * dev, gx_color_index color,
			 gx_color_value prgb[3])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_map_color_rgb(dev, color, prgb) :
	    dev_proc(tdev, map_color_rgb)(tdev, color, prgb));
}

int
gx_forward_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			  gx_color_index color)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	return_error(gs_error_Fatal);
    return dev_proc(tdev, fill_rectangle)(tdev, x, y, w, h, color);
}

int
gx_forward_tile_rectangle(gx_device * dev, const gx_tile_bitmap * tile,
			  int x, int y, int w, int h, gx_color_index color0,
			  gx_color_index color1, int px, int py)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_tile_rectangle((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_tile_rectangle) :
	 dev_proc(tdev, tile_rectangle));

    return proc(tdev, tile, x, y, w, h, color0, color1, px, py);
}

int
gx_forward_copy_mono(gx_device * dev, const byte * data,
		     int dx, int raster, gx_bitmap_id id,
		     int x, int y, int w, int h,
		     gx_color_index zero, gx_color_index one)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	return_error(gs_error_Fatal);
    return dev_proc(tdev, copy_mono)
	(tdev, data, dx, raster, id, x, y, w, h, zero, one);
}

int
gx_forward_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
		      gx_color_index color, int depth)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	return_error(gs_error_Fatal);
    return dev_proc(tdev, copy_mono)
	(tdev, data, data_x, raster, id, x, y, width, height, color, depth);
}

int
gx_forward_copy_color(gx_device * dev, const byte * data,
		      int dx, int raster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	return_error(gs_error_Fatal);
    return dev_proc(tdev, copy_color)
	(tdev, data, dx, raster, id, x, y, w, h);
}

int
gx_forward_get_bits(gx_device * dev, int y, byte * data, byte ** actual_data)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_get_bits(dev, y, data, actual_data) :
	    dev_proc(tdev, get_bits)(tdev, y, data, actual_data));
}

int
gx_forward_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_get_params(dev, plist) :
	    dev_proc(tdev, get_params)(tdev, plist));
}

int
gx_forward_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    int code;

    if (tdev == 0)
	return gx_default_put_params(dev, plist);
    code = dev_proc(tdev, put_params)(tdev, plist);
    if (code >= 0)
	gx_device_decache_colors(dev);
    return code;
}

gx_color_index
gx_forward_map_cmyk_color(gx_device * dev, const gx_color_value cv[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_map_cmyk_color(dev, cv) :
	    dev_proc(tdev, map_cmyk_color)(tdev, cv));
}

const gx_xfont_procs *
gx_forward_get_xfont_procs(gx_device * dev)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_get_xfont_procs(dev) :
	    dev_proc(tdev, get_xfont_procs)(tdev));
}

gx_device *
gx_forward_get_xfont_device(gx_device * dev)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_get_xfont_device(dev) :
	    dev_proc(tdev, get_xfont_device)(tdev));
}

gx_color_index
gx_forward_map_rgb_alpha_color(gx_device * dev, gx_color_value r,
			       gx_color_value g, gx_color_value b,
			       gx_color_value alpha)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ?
	    gx_default_map_rgb_alpha_color(dev, r, g, b, alpha) :
	    dev_proc(tdev, map_rgb_alpha_color)(tdev, r, g, b, alpha));
}

gx_device *
gx_forward_get_page_device(gx_device * dev)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    gx_device *pdev;

    if (tdev == 0)
	return gx_default_get_page_device(dev);
    pdev = dev_proc(tdev, get_page_device)(tdev);
    return pdev;
}

int
gx_forward_get_band(gx_device * dev, int y, int *band_start)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ?
	    gx_default_get_band(dev, y, band_start) :
	    dev_proc(tdev, get_band)(tdev, y, band_start));
}

int
gx_forward_copy_rop(gx_device * dev,
		    const byte * sdata, int sourcex, uint sraster,
		    gx_bitmap_id id, const gx_color_index * scolors,
		    const gx_tile_bitmap * texture,
		    const gx_color_index * tcolors,
		    int x, int y, int width, int height,
		    int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_copy_rop((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_copy_rop) :
	 dev_proc(tdev, copy_rop));

    return proc(tdev, sdata, sourcex, sraster, id, scolors,
		texture, tcolors, x, y, width, height,
		phase_x, phase_y, lop);
}

int
gx_forward_fill_path(gx_device * dev, const gs_imager_state * pis,
		     gx_path * ppath, const gx_fill_params * params,
		     const gx_drawing_color * pdcolor,
		     const gx_clip_path * pcpath)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_path((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_path) :
	 dev_proc(tdev, fill_path));

    return proc(tdev, pis, ppath, params, pdcolor, pcpath);
}

int
gx_forward_stroke_path(gx_device * dev, const gs_imager_state * pis,
		       gx_path * ppath, const gx_stroke_params * params,
		       const gx_drawing_color * pdcolor,
		       const gx_clip_path * pcpath)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_stroke_path((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_stroke_path) :
	 dev_proc(tdev, stroke_path));

    return proc(tdev, pis, ppath, params, pdcolor, pcpath);
}

int
gx_forward_fill_mask(gx_device * dev,
		     const byte * data, int dx, int raster, gx_bitmap_id id,
		     int x, int y, int w, int h,
		     const gx_drawing_color * pdcolor, int depth,
		     gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_mask((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_mask) :
	 dev_proc(tdev, fill_mask));

    return proc(tdev, data, dx, raster, id, x, y, w, h, pdcolor, depth,
		lop, pcpath);
}

int
gx_forward_fill_trapezoid(gx_device * dev,
			  const gs_fixed_edge * left,
			  const gs_fixed_edge * right,
			  fixed ybot, fixed ytop, bool swap_axes,
			  const gx_drawing_color * pdcolor,
			  gs_logical_operation_t lop)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_trapezoid((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_trapezoid) :
	 dev_proc(tdev, fill_trapezoid));

    return proc(tdev, left, right, ybot, ytop, swap_axes, pdcolor, lop);
}

int
gx_forward_fill_parallelogram(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_parallelogram((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_parallelogram) :
	 dev_proc(tdev, fill_parallelogram));

    return proc(tdev, px, py, ax, ay, bx, by, pdcolor, lop);
}

int
gx_forward_fill_triangle(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_triangle((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_triangle) :
	 dev_proc(tdev, fill_triangle));

    return proc(tdev, px, py, ax, ay, bx, by, pdcolor, lop);
}

int
gx_forward_draw_thin_line(gx_device * dev,
			  fixed fx0, fixed fy0, fixed fx1, fixed fy1,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_draw_thin_line((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_draw_thin_line) :
	 dev_proc(tdev, draw_thin_line));

    return proc(tdev, fx0, fy0, fx1, fy1, pdcolor, lop);
}

int
gx_forward_begin_image(gx_device * dev,
		       const gs_imager_state * pis, const gs_image_t * pim,
		       gs_image_format_t format, const gs_int_rect * prect,
		       const gx_drawing_color * pdcolor,
		       const gx_clip_path * pcpath,
		       gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_begin_image((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_begin_image) :
	 dev_proc(tdev, begin_image));

    return proc(tdev, pis, pim, format, prect, pdcolor, pcpath,
		memory, pinfo);
}

int
gx_forward_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
   int x, int y, int w, int h, gx_color_index color0, gx_color_index color1,
				int px, int py)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_strip_tile_rectangle((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_strip_tile_rectangle) :
	 dev_proc(tdev, strip_tile_rectangle));

    return proc(tdev, tiles, x, y, w, h, color0, color1, px, py);
}

int
gx_forward_strip_copy_rop(gx_device * dev, const byte * sdata, int sourcex,
			  uint sraster, gx_bitmap_id id,
			  const gx_color_index * scolors,
			  const gx_strip_bitmap * textures,
			  const gx_color_index * tcolors,
			  int x, int y, int width, int height,
			  int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_strip_copy_rop((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_strip_copy_rop) :
	 dev_proc(tdev, strip_copy_rop));

    return proc(tdev, sdata, sourcex, sraster, id, scolors,
		textures, tcolors, x, y, width, height,
		phase_x, phase_y, lop);
}

void
gx_forward_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	gx_default_get_clipping_box(dev, pbox);
    else
	dev_proc(tdev, get_clipping_box)(tdev, pbox);
}

int
gx_forward_begin_typed_image(gx_device * dev, const gs_imager_state * pis,
			     const gs_matrix * pmat,
			     const gs_image_common_t * pim,
			     const gs_int_rect * prect,
			     const gx_drawing_color * pdcolor,
			     const gx_clip_path * pcpath,
			     gs_memory_t * memory,
			     gx_image_enum_common_t ** pinfo)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_begin_typed_image((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_begin_typed_image) :
	 dev_proc(tdev, begin_typed_image));

    return proc(tdev, pis, pmat, pim, prect, pdcolor, pcpath,
		memory, pinfo);
}

int
gx_forward_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_get_bits_rectangle((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_get_bits_rectangle) :
	 dev_proc(tdev, get_bits_rectangle));

    return proc(tdev, prect, params, unread);
}

int
gx_forward_map_color_rgb_alpha(gx_device * dev, gx_color_index color,
			       gx_color_value prgba[4])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_map_color_rgb_alpha(dev, color, prgba) :
	    dev_proc(tdev, map_color_rgb_alpha)(tdev, color, prgba));
}

int
gx_forward_get_hardware_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_default_get_hardware_params(dev, plist) :
	    dev_proc(tdev, get_hardware_params)(tdev, plist));
}

int
gx_forward_text_begin(gx_device * dev, gs_imager_state * pis,
		      const gs_text_params_t * text, gs_font * font,
		      gx_path * path, const gx_device_color * pdcolor,
		      const gx_clip_path * pcpath, gs_memory_t * memory,
		      gs_text_enum_t ** ppenum)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_text_begin((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_text_begin) :
	 dev_proc(tdev, text_begin));

    return proc(tdev, pis, text, font, path, pdcolor, pcpath,
		memory, ppenum);
}

/* Forwarding device color mapping procs. */

/*
 * We need to forward the color mapping to the target device.
 */
static void
fwd_map_gray_cs(gx_device * dev, frac gray, frac out[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device * const tdev = fdev->target;
    const gx_cm_color_map_procs * pprocs;

    /* Verify that all of the pointers and procs are set */
    /* If not then use a default routine.  This case should be an error */
    if (tdev == 0 || dev_proc(tdev, get_color_mapping_procs) == 0 ||
          (pprocs = dev_proc(tdev, get_color_mapping_procs(tdev))) == 0 ||
          pprocs->map_gray == 0)
        gray_cs_to_gray_cm(tdev, gray, out);   /* if all else fails */
    else
        pprocs->map_gray(tdev, gray, out);
}

/*
 * We need to forward the color mapping to the target device.
 */
static void
fwd_map_rgb_cs(gx_device * dev, const gs_imager_state *pis,
				   frac r, frac g, frac b, frac out[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device * const tdev = fdev->target;
    const gx_cm_color_map_procs * pprocs;

    /* Verify that all of the pointers and procs are set */
    /* If not then use a default routine.  This case should be an error */
    if (tdev == 0 || dev_proc(tdev, get_color_mapping_procs) == 0 ||
          (pprocs = dev_proc(tdev, get_color_mapping_procs(tdev))) == 0 ||
          pprocs->map_rgb == 0)
        rgb_cs_to_rgb_cm(tdev, pis, r, g, b, out);   /* if all else fails */
    else
        pprocs->map_rgb(tdev, pis, r, g, b, out);
}

/*
 * We need to forward the color mapping to the target device.
 */
static void
fwd_map_cmyk_cs(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device * const tdev = fdev->target;
    const gx_cm_color_map_procs * pprocs;

    /* Verify that all of the pointers and procs are set */
    /* If not then use a default routine.  This case should be an error */
    if (tdev == 0 || dev_proc(tdev, get_color_mapping_procs) == 0 ||
          (pprocs = dev_proc(tdev, get_color_mapping_procs(tdev))) == 0 ||
          pprocs->map_cmyk == 0)
        cmyk_cs_to_cmyk_cm(tdev, c, m, y, k, out);   /* if all else fails */
    else
        pprocs->map_cmyk(tdev, c, m, y, k, out);
}

static const gx_cm_color_map_procs FwdDevice_cm_map_procs = {
    fwd_map_gray_cs, fwd_map_rgb_cs, fwd_map_cmyk_cs
};
/*
 * Instead of returning the target device's mapping procs, we need
 * to return a list of forwarding wrapper procs for the color mapping
 * procs.  This is required because the target device mapping procs
 * may also need the target device pointer (instead of the forwarding
 * device pointer).
 */
const gx_cm_color_map_procs *
gx_forward_get_color_mapping_procs(const gx_device * dev)
{
    const gx_device_forward * fdev = (const gx_device_forward *)dev;
    gx_device * tdev = fdev->target;

    return (tdev == 0 || dev_proc(tdev, get_color_mapping_procs) == 0
	? gx_default_DevGray_get_color_mapping_procs(dev)
	: &FwdDevice_cm_map_procs);
}

int
gx_forward_get_color_comp_index(gx_device * dev, const char * pname,
					int name_size, int component_type)
{
    const gx_device_forward * fdev = (const gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 
	? gx_error_get_color_comp_index(dev, pname,
				name_size, component_type)
	: dev_proc(tdev, get_color_comp_index)(tdev, pname,
				name_size, component_type));
}

gx_color_index
gx_forward_encode_color(gx_device * dev, const gx_color_value colors[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    return (tdev == 0 ? gx_error_encode_color(dev, colors)
		      : dev_proc(tdev, encode_color)(tdev, colors));
}

int
gx_forward_decode_color(gx_device * dev, gx_color_index cindex, gx_color_value colors[])
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)	/* If no device - just clear the color values */
	memset(colors, 0, sizeof(gx_color_value[GX_DEVICE_COLOR_MAX_COMPONENTS]));
    else
	dev_proc(tdev, decode_color)(tdev, cindex, colors);
    return 0;
}

int
gx_forward_pattern_manage(gx_device * dev, gx_bitmap_id id,
		gs_pattern1_instance_t *pinst, pattern_manage_t function)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    /* Note that clist sets fdev->target == fdev, 
       so this function is unapplicable to clist. */
    if (tdev == 0)
	return 0;
    else
	return dev_proc(tdev, pattern_manage)(tdev, id, pinst, function);
}

int
gx_forward_fill_rectangle_hl_color(gx_device *dev, 
    const gs_fixed_rect *rect, 
    const gs_imager_state *pis, const gx_drawing_color *pdcolor,
    const gx_clip_path *pcpath)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    /* Note that clist sets fdev->target == fdev, 
       so this function is unapplicable to clist. */
    if (tdev == 0)
	return_error(gs_error_rangecheck);
    else
	return dev_proc(tdev, fill_rectangle_hl_color)(tdev, rect, 
						pis, pdcolor, NULL);
}

int
gx_forward_include_color_space(gx_device *dev, gs_color_space *cspace, 
	    const byte *res_name, int name_length)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    /* Note that clist sets fdev->target == fdev, 
       so this function is unapplicable to clist. */
    if (tdev == 0)
	return 0;
    else
	return dev_proc(tdev, include_color_space)(tdev, cspace, res_name, name_length);
}

int 
gx_forward_fill_linear_color_scanline(gx_device *dev, const gs_fill_attributes *fa,
	int i, int j, int w,
	const frac31 *c, const int32_t *addx, const int32_t *mulx, int32_t divx)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_linear_color_scanline((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_linear_color_scanline) :
	 dev_proc(tdev, fill_linear_color_scanline));
    return proc(tdev, fa, i, j, w, c, addx, mulx, divx);
}

int 
gx_forward_fill_linear_color_trapezoid(gx_device *dev, const gs_fill_attributes *fa,
	const gs_fixed_point *p0, const gs_fixed_point *p1,
	const gs_fixed_point *p2, const gs_fixed_point *p3,
	const frac31 *c0, const frac31 *c1,
	const frac31 *c2, const frac31 *c3)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_linear_color_trapezoid((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_linear_color_trapezoid) :
	 dev_proc(tdev, fill_linear_color_trapezoid));
    return proc(tdev, fa, p0, p1, p2, p3, c0, c1, c2, c3);
}

int 
gx_forward_fill_linear_color_triangle(gx_device *dev, const gs_fill_attributes *fa,
	const gs_fixed_point *p0, const gs_fixed_point *p1,
	const gs_fixed_point *p2,
	const frac31 *c0, const frac31 *c1, const frac31 *c2)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    dev_proc_fill_linear_color_triangle((*proc)) =
	(tdev == 0 ? (tdev = dev, gx_default_fill_linear_color_triangle) :
	 dev_proc(tdev, fill_linear_color_triangle));
    return proc(tdev, fa, p0, p1, p2, c0, c1, c2);
}

int 
gx_forward_update_spot_equivalent_colors(gx_device *dev, const gs_state * pgs)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;
    int code = 0;

    if (tdev != NULL)
	code = dev_proc(tdev, update_spot_equivalent_colors)(tdev, pgs);
    return code;
}


/* ---------------- The null device(s) ---------------- */

private dev_proc_get_initial_matrix(gx_forward_upright_get_initial_matrix);
private dev_proc_fill_rectangle(null_fill_rectangle);
private dev_proc_copy_mono(null_copy_mono);
private dev_proc_copy_color(null_copy_color);
private dev_proc_put_params(null_put_params);
private dev_proc_copy_alpha(null_copy_alpha);
private dev_proc_copy_rop(null_copy_rop);
private dev_proc_fill_path(null_fill_path);
private dev_proc_stroke_path(null_stroke_path);
private dev_proc_fill_trapezoid(null_fill_trapezoid);
private dev_proc_fill_parallelogram(null_fill_parallelogram);
private dev_proc_fill_triangle(null_fill_triangle);
private dev_proc_draw_thin_line(null_draw_thin_line);
private dev_proc_decode_color(null_decode_color);
/* We would like to have null implementations of begin/data/end image, */
/* but we can't do this, because image_data must keep track of the */
/* Y position so it can return 1 when done. */
private dev_proc_strip_copy_rop(null_strip_copy_rop);

#define null_procs(get_initial_matrix, get_page_device) {\
	gx_default_open_device,\
	get_initial_matrix, /* differs */\
	gx_default_sync_output,\
	gx_default_output_page,\
	gx_default_close_device,\
	gx_forward_map_rgb_color,\
	gx_forward_map_color_rgb,\
	null_fill_rectangle,\
	gx_default_tile_rectangle,\
	null_copy_mono,\
	null_copy_color,\
	gx_default_draw_line,\
	gx_default_get_bits,\
	gx_forward_get_params,\
	null_put_params,\
	gx_forward_map_cmyk_color,\
	gx_forward_get_xfont_procs,\
	gx_forward_get_xfont_device,\
	gx_forward_map_rgb_alpha_color,\
	get_page_device,	/* differs */\
	gx_default_get_alpha_bits,\
	null_copy_alpha,\
	gx_forward_get_band,\
	null_copy_rop,\
	null_fill_path,\
	null_stroke_path,\
	gx_default_fill_mask,\
	null_fill_trapezoid,\
	null_fill_parallelogram,\
	null_fill_triangle,\
	null_draw_thin_line,\
	gx_default_begin_image,\
	gx_default_image_data,\
	gx_default_end_image,\
	gx_default_strip_tile_rectangle,\
	null_strip_copy_rop,\
	gx_default_get_clipping_box,\
	gx_default_begin_typed_image,\
	gx_default_get_bits_rectangle,\
	gx_forward_map_color_rgb_alpha,\
	gx_non_imaging_create_compositor,\
	gx_forward_get_hardware_params,\
	gx_default_text_begin,\
	gx_default_finish_copydevice,\
	NULL,				/* begin_transparency_group */\
	NULL,				/* end_transparency_group */\
	NULL,				/* begin_transparency_mask */\
	NULL,				/* end_transparency_mask */\
	NULL,				/* discard_transparency_layer */\
	gx_default_DevGray_get_color_mapping_procs,	/* get_color_mapping_procs */\
	gx_default_DevGray_get_color_comp_index,/* get_color_comp_index */\
	gx_default_gray_fast_encode,		/* encode_color */\
	null_decode_color,		/* decode_color */\
	gx_default_pattern_manage,\
	gx_default_fill_rectangle_hl_color,\
	gx_default_include_color_space\
}

const gx_device_null gs_null_device = {
    std_device_std_body_type_open(gx_device_null, 0, "null", &st_device_null,
				  0, 0, 72, 72),
    null_procs(gx_forward_upright_get_initial_matrix, /* upright matrix */
               gx_default_get_page_device     /* not a page device */ ),
    0				/* target */
};

const gx_device_null gs_nullpage_device = {
std_device_std_body_type_open(gx_device_null, 0, "nullpage", &st_device_null,
			      72 /*nominal */ , 72 /*nominal */ , 72, 72),
    null_procs( gx_forward_get_initial_matrix, /* default matrix */
                gx_page_device_get_page_device /* a page device */ ),
    0				/* target */
};

private void
gx_forward_upright_get_initial_matrix(gx_device * dev, gs_matrix * pmat)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    if (tdev == 0)
	gx_upright_get_initial_matrix(dev, pmat);
    else
	dev_proc(tdev, get_initial_matrix)(tdev, pmat);
}

private int
null_decode_color(gx_device * dev, gx_color_index cindex, gx_color_value colors[])
{
    colors[0] = (cindex & 1) ?  gx_max_color_value : 0;
    return 0;
}

private int
null_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		    gx_color_index color)
{
    return 0;
}
private int
null_copy_mono(gx_device * dev, const byte * data, int dx, int raster,
	       gx_bitmap_id id, int x, int y, int w, int h,
	       gx_color_index zero, gx_color_index one)
{
    return 0;
}
private int
null_copy_color(gx_device * dev, const byte * data,
		int data_x, int raster, gx_bitmap_id id,
		int x, int y, int width, int height)
{
    return 0;
}
private int
null_put_params(gx_device * dev, gs_param_list * plist)
{
    /*
     * If this is not a page device, we must defeat attempts to reset
     * the size; otherwise this is equivalent to gx_forward_put_params.
     */
    int code = gx_forward_put_params(dev, plist);

    if (code < 0 || dev_proc(dev, get_page_device)(dev) == dev)
	return code;
    dev->width = dev->height = 0;
    return code;
}
private int
null_copy_alpha(gx_device * dev, const byte * data, int data_x, int raster,
		gx_bitmap_id id, int x, int y, int width, int height,
		gx_color_index color, int depth)
{
    return 0;
}
private int
null_copy_rop(gx_device * dev,
	      const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
	      const gx_color_index * scolors,
	      const gx_tile_bitmap * texture, const gx_color_index * tcolors,
	      int x, int y, int width, int height,
	      int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return 0;
}
private int
null_fill_path(gx_device * dev, const gs_imager_state * pis,
	       gx_path * ppath, const gx_fill_params * params,
	       const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    return 0;
}
private int
null_stroke_path(gx_device * dev, const gs_imager_state * pis,
		 gx_path * ppath, const gx_stroke_params * params,
		 const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    return 0;
}
private int
null_fill_trapezoid(gx_device * dev,
		    const gs_fixed_edge * left, const gs_fixed_edge * right,
		    fixed ybot, fixed ytop, bool swap_axes,
		    const gx_drawing_color * pdcolor,
		    gs_logical_operation_t lop)
{
    return 0;
}
private int
null_fill_parallelogram(gx_device * dev, fixed px, fixed py,
			fixed ax, fixed ay, fixed bx, fixed by,
			const gx_drawing_color * pdcolor,
			gs_logical_operation_t lop)
{
    return 0;
}
private int
null_fill_triangle(gx_device * dev,
		   fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		   const gx_drawing_color * pdcolor,
		   gs_logical_operation_t lop)
{
    return 0;
}
private int
null_draw_thin_line(gx_device * dev,
		    fixed fx0, fixed fy0, fixed fx1, fixed fy1,
		    const gx_drawing_color * pdcolor,
		    gs_logical_operation_t lop)
{
    return 0;
}
private int
null_strip_copy_rop(gx_device * dev, const byte * sdata, int sourcex,
		    uint sraster, gx_bitmap_id id,
		    const gx_color_index * scolors,
		    const gx_strip_bitmap * textures,
		    const gx_color_index * tcolors,
		    int x, int y, int width, int height,
		    int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return 0;
}
