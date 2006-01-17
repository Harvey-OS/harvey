/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxclipm.c,v 1.13 2004/06/24 05:03:36 dan Exp $ */
/* Mask clipping device */
#include "memory_.h"
#include "gx.h"
#include "gsbittab.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxclipm.h"

/* Device procedures */
private dev_proc_fill_rectangle(mask_clip_fill_rectangle);
private dev_proc_copy_mono(mask_clip_copy_mono);
private dev_proc_copy_color(mask_clip_copy_color);
private dev_proc_copy_alpha(mask_clip_copy_alpha);
private dev_proc_strip_tile_rectangle(mask_clip_strip_tile_rectangle);
private dev_proc_strip_copy_rop(mask_clip_strip_copy_rop);
private dev_proc_get_clipping_box(mask_clip_get_clipping_box);

/* The device descriptor. */
const gx_device_mask_clip gs_mask_clip_device =
{std_device_std_body_open(gx_device_mask_clip, 0, "mask clipper",
			  0, 0, 1, 1),
 {gx_default_open_device,
  gx_forward_get_initial_matrix,
  gx_default_sync_output,
  gx_default_output_page,
  gx_default_close_device,
  gx_forward_map_rgb_color,
  gx_forward_map_color_rgb,
  mask_clip_fill_rectangle,
  gx_default_tile_rectangle,
  mask_clip_copy_mono,
  mask_clip_copy_color,
  gx_default_draw_line,
  gx_forward_get_bits,
  gx_forward_get_params,
  gx_forward_put_params,
  gx_forward_map_cmyk_color,
  gx_forward_get_xfont_procs,
  gx_forward_get_xfont_device,
  gx_forward_map_rgb_alpha_color,
  gx_forward_get_page_device,
  gx_forward_get_alpha_bits,
  mask_clip_copy_alpha,
  gx_forward_get_band,
  gx_default_copy_rop,
  gx_default_fill_path,
  gx_default_stroke_path,
  gx_default_fill_mask,
  gx_default_fill_trapezoid,
  gx_default_fill_parallelogram,
  gx_default_fill_triangle,
  gx_default_draw_thin_line,
  gx_default_begin_image,
  gx_default_image_data,
  gx_default_end_image,
  mask_clip_strip_tile_rectangle,
  mask_clip_strip_copy_rop,
  mask_clip_get_clipping_box,
  gx_default_begin_typed_image,
  gx_forward_get_bits_rectangle,
  gx_forward_map_color_rgb_alpha,
  gx_no_create_compositor,
  gx_forward_get_hardware_params,
  gx_default_text_begin,
  gx_default_finish_copydevice,
  NULL,			/* begin_transparency_group */
  NULL,			/* end_transparency_group */
  NULL,			/* begin_transparency_mask */
  NULL,			/* end_transparency_mask */
  NULL,			/* discard_transparency_layer */
  gx_forward_get_color_mapping_procs,
  gx_forward_get_color_comp_index,
  gx_forward_encode_color,
  gx_forward_decode_color,
  gx_forward_pattern_manage,
  gx_forward_fill_rectangle_hl_color,
  gx_forward_include_color_space,
  gx_forward_fill_linear_color_scanline,
  gx_forward_fill_linear_color_trapezoid,
  gx_forward_fill_linear_color_triangle,
  gx_forward_update_spot_equivalent_colors
 }
};

/* Fill a rectangle by painting through the mask. */
private int
mask_clip_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			 gx_color_index color)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    gx_device *tdev = cdev->target;

    /* Clip the rectangle to the region covered by the mask. */
    int mx0 = x + cdev->phase.x, my0 = y + cdev->phase.y;
    int mx1 = mx0 + w, my1 = my0 + h;

    if (mx0 < 0)
	mx0 = 0;
    if (my0 < 0)
	my0 = 0;
    if (mx1 > cdev->tiles.size.x)
	mx1 = cdev->tiles.size.x;
    if (my1 > cdev->tiles.size.y)
	my1 = cdev->tiles.size.y;
    return (*dev_proc(tdev, copy_mono))
	(tdev, cdev->tiles.data + my0 * cdev->tiles.raster, mx0,
	 cdev->tiles.raster, cdev->tiles.id,
	 mx0 - cdev->phase.x, my0 - cdev->phase.y,
	 mx1 - mx0, my1 - my0, gx_no_color_index, color);
}

/*
 * Clip the rectangle for a copy operation.
 * Sets m{x,y}{0,1} to the region in the mask coordinate system;
 * subtract cdev->phase.{x,y} to get target coordinates.
 * Sets sdata, sx to adjusted values of data, sourcex.
 * References cdev, data, sourcex, raster, x, y, w, h.
 */
#define DECLARE_MASK_COPY\
	const byte *sdata;\
	int sx, mx0, my0, mx1, my1
#define FIT_MASK_COPY(data, sourcex, raster, vx, vy, vw, vh)\
	BEGIN\
	  sdata = data, sx = sourcex;\
	  mx0 = vx + cdev->phase.x, my0 = vy + cdev->phase.y;\
	  mx1 = mx0 + vw, my1 = my0 + vh;\
	  if ( mx0 < 0 )\
	    sx -= mx0, mx0 = 0;\
	  if ( my0 < 0 )\
	    sdata -= my0 * raster, my0 = 0;\
	  if ( mx1 > cdev->tiles.size.x )\
	    mx1 = cdev->tiles.size.x;\
	  if ( my1 > cdev->tiles.size.y )\
	    my1 = cdev->tiles.size.y;\
	END

/* Copy a monochrome bitmap by playing Boolean games. */
private int
mask_clip_copy_mono(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		    int x, int y, int w, int h,
		    gx_color_index color0, gx_color_index color1)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    gx_device *tdev = cdev->target;
    gx_color_index color, mcolor0, mcolor1;

    DECLARE_MASK_COPY;
    int cy, ny;
    int code;

    setup_mask_copy_mono(cdev, color, mcolor0, mcolor1);
    FIT_MASK_COPY(data, sourcex, raster, x, y, w, h);
    for (cy = my0; cy < my1; cy += ny) {
	int ty = cy - cdev->phase.y;
	int cx, nx;

	ny = my1 - cy;
	if (ny > cdev->mdev.height)
	    ny = cdev->mdev.height;
	for (cx = mx0; cx < mx1; cx += nx) {
	    int tx = cx - cdev->phase.x;

	    nx = mx1 - cx;	/* also should be min */
	    /* Copy a tile slice to the memory device buffer. */
	    memcpy(cdev->buffer.bytes,
		   cdev->tiles.data + cy * cdev->tiles.raster,
		   cdev->tiles.raster * ny);
	    /* Intersect the tile with the source data. */
	    /* mcolor0 and mcolor1 invert the data if needed. */
	    /* This call can't fail. */
	    (*dev_proc(&cdev->mdev, copy_mono)) ((gx_device *) & cdev->mdev,
				     sdata + (ty - y) * raster, sx + tx - x,
						 raster, gx_no_bitmap_id,
					   cx, 0, nx, ny, mcolor0, mcolor1);
	    /* Now copy the color through the double mask. */
	    code = (*dev_proc(tdev, copy_mono)) (cdev->target,
				 cdev->buffer.bytes, cx, cdev->tiles.raster,
						 gx_no_bitmap_id,
				  tx, ty, nx, ny, gx_no_color_index, color);
	    if (code < 0)
		return code;
	}
    }
    return 0;
}

/*
 * Define the run enumerator for the other copying operations.  We can't use
 * the BitBlt tricks: we have to scan for runs of 1s.  There are obvious
 * ways to speed this up; we'll implement some if we need to.
 */
private int
clip_runs_enumerate(gx_device_mask_clip * cdev,
		    int (*process) (clip_callback_data_t * pccd, int xc, int yc, int xec, int yec),
		    clip_callback_data_t * pccd)
{
    DECLARE_MASK_COPY;
    int cy;
    const byte *tile_row;
    gs_int_rect prev;
    int code;

    FIT_MASK_COPY(pccd->data, pccd->sourcex, pccd->raster,
		  pccd->x, pccd->y, pccd->w, pccd->h);
    tile_row = cdev->tiles.data + my0 * cdev->tiles.raster + (mx0 >> 3);
    prev.p.x = 0;	/* arbitrary */
    prev.q.x = prev.p.x - 1;	/* an impossible rectangle */
    prev.p.y = prev.q.y = -1;	/* arbitrary */
    for (cy = my0; cy < my1; cy++) {
	int cx = mx0;
	const byte *tp = tile_row;

	if_debug1('B', "[B]clip runs y=%d:", cy - cdev->phase.y);
	while (cx < mx1) {
	    int len;
	    int tx1, tx, ty;

	    /* Skip a run of 0s. */
	    len = byte_bit_run_length[cx & 7][*tp ^ 0xff];
	    if (len < 8) {
		cx += len;
		if (cx >= mx1)
		    break;
	    } else {
		cx += len - 8;
		tp++;
		while (cx < mx1 && *tp == 0)
		    cx += 8, tp++;
		if (cx >= mx1)
		    break;
		cx += byte_bit_run_length_0[*tp ^ 0xff];
		if (cx >= mx1)
		    break;
	    }
	    tx1 = cx - cdev->phase.x;
	    /* Scan a run of 1s. */
	    len = byte_bit_run_length[cx & 7][*tp];
	    if (len < 8) {
		cx += len;
		if (cx > mx1)
		    cx = mx1;
	    } else {
		cx += len - 8;
		tp++;
		while (cx < mx1 && *tp == 0xff)
		    cx += 8, tp++;
		if (cx > mx1)
		    cx = mx1;
		else {
		    cx += byte_bit_run_length_0[*tp];
		    if (cx > mx1)
			cx = mx1;
		}
	    }
	    tx = cx - cdev->phase.x;
	    if_debug2('B', " %d-%d,", tx1, tx);
	    ty = cy - cdev->phase.y;
	    /* Detect vertical rectangles. */
	    if (prev.p.x == tx1 && prev.q.x == tx && prev.q.y == ty)
		prev.q.y = ty + 1;
	    else {
		if (prev.q.y > prev.p.y) {
		    code = (*process)(pccd, tx1, ty, tx, ty + 1);
		    if (code < 0)
			return code;
		}
		prev.p.x = tx1;
		prev.p.y = ty;
		prev.q.x = tx;
		prev.q.y = ty + 1;
	    }
	}
	if_debug0('B', "\n");
	tile_row += cdev->tiles.raster;
    }
    if (prev.q.y > prev.p.y) {
	code = (*process)(pccd, prev.p.x, prev.p.y, prev.q.x, prev.q.y);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* Copy a color rectangle */
private int
mask_clip_copy_color(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		     int x, int y, int w, int h)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.tdev = cdev->target;
    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.x = x, ccdata.y = y, ccdata.w = w, ccdata.h = h;
    return clip_runs_enumerate(cdev, clip_call_copy_color, &ccdata);
}

/* Copy a rectangle with alpha */
private int
mask_clip_copy_alpha(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		int x, int y, int w, int h, gx_color_index color, int depth)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.tdev = cdev->target;
    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.x = x, ccdata.y = y, ccdata.w = w, ccdata.h = h;
    ccdata.color[0] = color, ccdata.depth = depth;
    return clip_runs_enumerate(cdev, clip_call_copy_alpha, &ccdata);
}

private int
mask_clip_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
			       int x, int y, int w, int h,
			       gx_color_index color0, gx_color_index color1,
			       int phase_x, int phase_y)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.tdev = cdev->target;
    ccdata.x = x, ccdata.y = y, ccdata.w = w, ccdata.h = h;
    ccdata.tiles = tiles;
    ccdata.color[0] = color0, ccdata.color[1] = color1;
    ccdata.phase.x = phase_x, ccdata.phase.y = phase_y;
    return clip_runs_enumerate(cdev, clip_call_strip_tile_rectangle, &ccdata);
}

private int
mask_clip_strip_copy_rop(gx_device * dev,
	       const byte * data, int sourcex, uint raster, gx_bitmap_id id,
			 const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			 int x, int y, int w, int h,
		       int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.tdev = cdev->target;
    ccdata.x = x, ccdata.y = y, ccdata.w = w, ccdata.h = h;
    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.scolors = scolors, ccdata.textures = textures,
	ccdata.tcolors = tcolors;
    ccdata.phase.x = phase_x, ccdata.phase.y = phase_y, ccdata.lop = lop;
    return clip_runs_enumerate(cdev, clip_call_strip_copy_rop, &ccdata);
}

private void
mask_clip_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    gx_device_mask_clip *cdev = (gx_device_mask_clip *) dev;
    gx_device *tdev = cdev->target;
    gs_fixed_rect tbox;

    (*dev_proc(tdev, get_clipping_box)) (tdev, &tbox);
    pbox->p.x = tbox.p.x - cdev->phase.x;
    pbox->p.y = tbox.p.y - cdev->phase.y;
    pbox->q.x = tbox.q.x - cdev->phase.x;
    pbox->q.y = tbox.q.y - cdev->phase.y;
}
