/* Copyright (C) 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevht.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Halftoning device implementation */
#include "gx.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gdevht.h"
#include "gxdcolor.h"
#include "gxdcconv.h"
#include "gxdither.h"

/* The device procedures */
private dev_proc_open_device(ht_open);
private dev_proc_map_rgb_color(ht_map_rgb_color);
private dev_proc_map_color_rgb(ht_map_color_rgb);
private dev_proc_fill_rectangle(ht_fill_rectangle);
private dev_proc_map_cmyk_color(ht_map_cmyk_color);
private dev_proc_map_rgb_alpha_color(ht_map_rgb_alpha_color);
private dev_proc_fill_path(ht_fill_path);
private dev_proc_stroke_path(ht_stroke_path);
private dev_proc_fill_mask(ht_fill_mask);
private dev_proc_fill_trapezoid(ht_fill_trapezoid);
private dev_proc_fill_parallelogram(ht_fill_parallelogram);
private dev_proc_fill_triangle(ht_fill_triangle);
private dev_proc_draw_thin_line(ht_draw_thin_line);
private const gx_device_ht gs_ht_device =
{std_device_dci_body(gx_device_ht, 0, "halftoner",
		     0, 0, 1, 1,
		     1, 8, 255, 0, 0, 0),
 {ht_open,
  gx_forward_get_initial_matrix,
  gx_forward_sync_output,
  gx_forward_output_page,
  gx_default_close_device,
  ht_map_rgb_color,
  ht_map_color_rgb,
  ht_fill_rectangle,
  gx_default_tile_rectangle,
  gx_default_copy_mono,
  gx_default_copy_color,
  gx_default_draw_line,
  gx_default_get_bits,
  gx_forward_get_params,
  gx_forward_put_params,
  ht_map_cmyk_color,
  gx_forward_get_xfont_procs,
  gx_forward_get_xfont_device,
  ht_map_rgb_alpha_color,
  gx_forward_get_page_device,
  gx_forward_get_alpha_bits,
  gx_default_copy_alpha,
  gx_forward_get_band,
  gx_default_copy_rop,
  ht_fill_path,
  ht_stroke_path,
  ht_fill_mask,
  ht_fill_trapezoid,
  ht_fill_parallelogram,
  ht_fill_triangle,
  ht_draw_thin_line,
  gx_default_begin_image,
  gx_default_image_data,
  gx_default_end_image,
  gx_default_strip_tile_rectangle,
  gx_default_strip_copy_rop,
  gx_forward_get_clipping_box,
  gx_default_begin_typed_image,
  gx_no_get_bits_rectangle,
  gx_default_map_color_rgb_alpha,
  gx_no_create_compositor,
  gx_forward_get_hardware_params,
  gx_default_text_begin
 }
};

/*
 * Define the packing of two target colors and a halftone level into
 * a gx_color_index.  Since C doesn't let us cast between a structure
 * and a scalar, we have to use explicit shifting and masking.
 */
#define cx_color0(color) ((color) & htdev->color_mask)
#define cx_color1(color) (((color) >> htdev->color_shift) & htdev->color_mask)
#define cx_level(color) ((color) >> htdev->level_shift)
#define cx_values(c0, c1, lev)\
  ( ((lev) << htdev->level_shift) + ((c1) << htdev->color_shift) + (c0) )

/* ---------------- Non-drawing procedures ---------------- */

/* Open the device.  Right now we just make some error checks. */
private int
ht_open(gx_device * dev)
{
    gx_device_ht *htdev = (gx_device_ht *) dev;
    const gx_device *target = htdev->target;
    const gx_device_halftone *pdht = htdev->dev_ht;
    int target_depth;
    uint num_levels;

    if (target == 0 || pdht == 0 || pdht->num_comp != 0)
	return_error(gs_error_rangecheck);
    /*
     * Make sure we can fit a halftone level and two target colors
     * into a gx_color_index.  If we're lucky, we can pack them into
     * even less space.
     */
    target_depth = target->color_info.depth;
    num_levels = pdht->order.num_levels;
    {
	int depth = target_depth * 2 + num_levels;

	if (depth > sizeof(gx_color_index) * 8)
	    return_error(gs_error_rangecheck);
	/*
	 * If there are too few halftone levels (fewer than 32),
	 * we can't treat this device as contone.
	 */
	if (num_levels < 32)
	    return_error(gs_error_rangecheck);
	/* Set up color information. */
	htdev->color_info.num_components = target->color_info.num_components;
	htdev->color_info.depth = depth;
	htdev->color_info.max_gray = num_levels - 1;
	htdev->color_info.max_color = num_levels - 1;
	htdev->color_info.dither_grays = num_levels;
	htdev->color_info.dither_colors = num_levels;
    }
    htdev->color_shift = target_depth;
    htdev->level_shift = target_depth * 2;
    htdev->color_mask = ((gx_color_index) 1 << target_depth) - 1;
    htdev->phase.x = imod(-htdev->ht_phase.x, pdht->lcm_width);
    htdev->phase.y = imod(-htdev->ht_phase.y, pdht->lcm_height);
    return 0;
}

/* Map from RGB or CMYK colors to the packed representation. */
private gx_color_index
ht_finish_map_color(const gx_device_ht * htdev, int code,
		    const gx_device_color * pdevc)
{
    if (code < 0)
	return gx_no_color_index;
    if (gx_dc_is_pure(pdevc))
	return cx_values(pdevc->colors.pure, 0, 0);
    if (gx_dc_is_binary_halftone(pdevc))
	return cx_values(pdevc->colors.binary.color[0],
			 pdevc->colors.binary.color[1],
			 pdevc->colors.binary.b_level);
    lprintf("bad type in ht color mapping!");
    return gx_no_color_index;
}
private gx_color_index
ht_map_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
		 gx_color_value b)
{
    return ht_map_rgb_alpha_color(dev, r, g, b, gx_max_color_value);
}
private gx_color_index
ht_map_cmyk_color(gx_device * dev, gx_color_value c, gx_color_value m,
		  gx_color_value y, gx_color_value k)
{
    gx_device_ht *htdev = (gx_device_ht *) dev;
    gx_device_color devc;
    frac fc = cv2frac(k);
    frac fk = cv2frac(k);
    int code =
    (c == m && m == y ?
     gx_render_device_gray(color_cmyk_to_gray(fc, fc, fc, fk, NULL),
			   gx_max_color_value,
			   &devc, htdev->target, htdev->dev_ht,
			   &htdev->ht_phase) :
     gx_render_device_color(fc, cv2frac(m), cv2frac(y),
			    fk, false, gx_max_color_value,
			    &devc, htdev->target, htdev->dev_ht,
			    &htdev->ht_phase));

    return ht_finish_map_color(htdev, code, &devc);
}
private gx_color_index
ht_map_rgb_alpha_color(gx_device * dev, gx_color_value r,
		   gx_color_value g, gx_color_value b, gx_color_value alpha)
{
    gx_device_ht *htdev = (gx_device_ht *) dev;
    gx_device_color devc;
    int code =
    (r == g && g == b ?
     gx_render_device_gray(cv2frac(r), alpha,
			   &devc, htdev->target, htdev->dev_ht,
			   &htdev->ht_phase) :
     gx_render_device_color(cv2frac(r), cv2frac(g), cv2frac(b),
			    frac_0, false, alpha,
			    &devc, htdev->target, htdev->dev_ht,
			    &htdev->ht_phase));

    return ht_finish_map_color(htdev, code, &devc);
}

/* Map back to an RGB color. */
private int
ht_map_color_rgb(gx_device * dev, gx_color_index color,
		 gx_color_value prgb[3])
{
    gx_device_ht *htdev = (gx_device_ht *) dev;
    gx_device *tdev = htdev->target;
    gx_color_index color0 = cx_color0(color);
    uint level = cx_level(color);

    dev_proc_map_color_rgb((*map)) = dev_proc(tdev, map_color_rgb);

    if (level == 0)
	return (*map) (tdev, color0, prgb);
    {
	gx_color_index color1 = cx_color1(color);
	gx_color_value rgb0[3], rgb1[3];
	uint num_levels = htdev->dev_ht->order.num_levels;
	int i;

	(*map) (tdev, color0, rgb0);
	(*map) (tdev, color1, rgb1);
	for (i = 0; i < 3; ++i)
	    prgb[i] = rgb0[i] +
		(rgb1[i] - rgb0[i]) * (ulong) level / num_levels;
	return 0;
    }
}

/* ---------------- Drawing procedures ---------------- */

/*
 * Map a (pure) contone color into a gx_device_color, either pure or
 * halftoned.  Return 0 if pure, 1 if halftoned, <0 if the original color
 * wasn't pure.  (This is not a driver procedure.)
 */
private int
ht_map_device_color(const gx_device_ht * htdev, gx_device_color * pdevc,
		    const gx_device_color * pcdevc)
{
    if (!gx_dc_is_pure(pcdevc))
	return -1;
    {
	gx_color_index color = pcdevc->colors.pure;
	gx_color_index color0 = cx_color0(color);
	uint level = cx_level(color);

	if (level == 0) {
	    color_set_pure(pdevc, color0);
	    return 0;
	} else {
	    color_set_binary_halftone(pdevc, htdev->dev_ht, color0,
				      cx_color1(color), level);
	    color_set_phase(pdevc, htdev->phase.x, htdev->phase.y);
	    return 1;
	}
    }
}

/* Fill a rectangle by tiling with a halftone. */
private int
ht_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		  gx_color_index color)
{
    gx_device_ht *htdev = (gx_device_ht *) dev;
    gx_device *tdev = htdev->target;
    gx_color_index color0 = cx_color0(color);
    uint level = cx_level(color);

    if (level == 0)
	return (*dev_proc(tdev, fill_rectangle))
	    (tdev, x, y, w, h, color0);
    {
	const gx_ht_order *porder = &htdev->dev_ht->order;
	gx_ht_cache *pcache = porder->cache;
	gx_ht_tile *tile;

	/* Ensure that the tile cache is current. */
	if (pcache->order.bits != porder->bits)
	    gx_ht_init_cache(pcache, porder);
	/* Ensure that the tile we want is cached. */
	tile = gx_render_ht(pcache, level);
	if (tile == 0)
	    return_error(gs_error_Fatal);
	/* Fill the rectangle with the tile. */
	return (*dev_proc(tdev, strip_tile_rectangle))
	    (tdev, &tile->tiles, x, y, w, h, color0, cx_color1(color),
	     htdev->phase.x, htdev->phase.y);
    }
}

/*
 * Create a halftoned color if necessary for the high-level drawing
 * operations.
 */

#define MAP_DRAWING_COLOR(proc, default_proc)\
	gx_device_ht *htdev = (gx_device_ht *)dev;\
	gx_device_color dcolor;\
	gx_device *tdev;\
	const gx_device_color *tdcolor;\
\
	if ( ht_map_device_color(htdev, &dcolor, pdcolor) < 0 )\
	  tdev = dev, tdcolor = pdcolor, proc = default_proc;\
	else\
	  tdev = htdev->target, tdcolor = &dcolor,\
	    proc = dev_proc(tdev, proc)

private int
ht_fill_path(gx_device * dev,
	     const gs_imager_state * pis, gx_path * ppath,
	     const gx_fill_params * params,
	     const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    dev_proc_fill_path((*fill_path));
    MAP_DRAWING_COLOR(fill_path, gx_default_fill_path);
    return (*fill_path) (tdev, pis, ppath, params, tdcolor, pcpath);
}

private int
ht_stroke_path(gx_device * dev,
	       const gs_imager_state * pis, gx_path * ppath,
	       const gx_stroke_params * params,
	       const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    dev_proc_stroke_path((*stroke_path));
    MAP_DRAWING_COLOR(stroke_path, gx_default_stroke_path);
    return (*stroke_path) (tdev, pis, ppath, params, tdcolor, pcpath);
}

private int
ht_fill_mask(gx_device * dev,
	     const byte * data, int data_x, int raster, gx_bitmap_id id,
	     int x, int y, int width, int height,
	     const gx_drawing_color * pdcolor, int depth,
	     gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    dev_proc_fill_mask((*fill_mask));
    MAP_DRAWING_COLOR(fill_mask, gx_default_fill_mask);
    return (*fill_mask) (tdev, data, data_x, raster, id, x, y,
			 width, height, tdcolor, depth, lop, pcpath);
}

private int
ht_fill_trapezoid(gx_device * dev,
		  const gs_fixed_edge * left, const gs_fixed_edge * right,
		  fixed ybot, fixed ytop, bool swap_axes,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    dev_proc_fill_trapezoid((*fill_trapezoid));
    MAP_DRAWING_COLOR(fill_trapezoid, gx_default_fill_trapezoid);
    return (*fill_trapezoid) (tdev, left, right, ybot, ytop, swap_axes,
			      tdcolor, lop);
}

private int
ht_fill_parallelogram(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    dev_proc_fill_parallelogram((*fill_parallelogram));
    MAP_DRAWING_COLOR(fill_parallelogram, gx_default_fill_parallelogram);
    return (*fill_parallelogram) (tdev, px, py, ax, ay, bx, by,
				  tdcolor, lop);
}

private int
ht_fill_triangle(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    dev_proc_fill_triangle((*fill_triangle));
    MAP_DRAWING_COLOR(fill_triangle, gx_default_fill_triangle);
    return (*fill_triangle) (tdev, px, py, ax, ay, bx, by,
			     tdcolor, lop);
}

private int
ht_draw_thin_line(gx_device * dev,
		  fixed fx0, fixed fy0, fixed fx1, fixed fy1,
	       const gx_drawing_color * pdcolor, gs_logical_operation_t lop)
{
    dev_proc_draw_thin_line((*draw_thin_line));
    MAP_DRAWING_COLOR(draw_thin_line, gx_default_draw_thin_line);
    return (*draw_thin_line) (tdev, fx0, fy0, fx1, fy1, tdcolor, lop);
}
