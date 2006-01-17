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

/*$Id: gdevbbox.c,v 1.23 2005/03/14 18:08:36 dan Exp $ */
/* Device for tracking bounding box */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gxdevice.h"
#include "gsdevice.h"		/* requires gsmatrix.h */
#include "gdevbbox.h"
#include "gxdcolor.h"		/* for gx_device_black/white */
#include "gxiparam.h"		/* for image source size */
#include "gxistate.h"
#include "gxpaint.h"
#include "gxpath.h"
#include "gxcpath.h"

/* GC descriptor */
public_st_device_bbox();

/* Device procedures */
private dev_proc_open_device(bbox_open_device);
private dev_proc_close_device(bbox_close_device);
private dev_proc_output_page(bbox_output_page);
private dev_proc_fill_rectangle(bbox_fill_rectangle);
private dev_proc_copy_mono(bbox_copy_mono);
private dev_proc_copy_color(bbox_copy_color);
private dev_proc_get_params(bbox_get_params);
private dev_proc_put_params(bbox_put_params);
private dev_proc_copy_alpha(bbox_copy_alpha);
private dev_proc_fill_path(bbox_fill_path);
private dev_proc_stroke_path(bbox_stroke_path);
private dev_proc_fill_mask(bbox_fill_mask);
private dev_proc_fill_trapezoid(bbox_fill_trapezoid);
private dev_proc_fill_parallelogram(bbox_fill_parallelogram);
private dev_proc_fill_triangle(bbox_fill_triangle);
private dev_proc_draw_thin_line(bbox_draw_thin_line);
private dev_proc_strip_tile_rectangle(bbox_strip_tile_rectangle);
private dev_proc_strip_copy_rop(bbox_strip_copy_rop);
private dev_proc_begin_typed_image(bbox_begin_typed_image);
private dev_proc_create_compositor(bbox_create_compositor);
private dev_proc_text_begin(bbox_text_begin);

/* The device prototype */
/*
 * Normally this would be private, but if the device is going to be used
 * stand-alone, it has to be public.
 */
/*private */ const
/*
 * The bbox device sets the resolution to some value R (currently 4000), and
 * the page size in device pixels to slightly smaller than the largest
 * representable values (around 500K), leaving a little room for stroke
 * widths, rounding, etc.  If an input file (or the command line) resets the
 * resolution to a value R' > R, the page size in pixels will get multiplied
 * by R'/R, and will thereby exceed the representable range, causing a
 * limitcheck.  That is why the bbox device must set the resolution to a
 * value larger than that of any real device.  A consequence of this is that
 * the page size in inches is limited to the maximum representable pixel
 * size divided by R, which gives a limit of about 120" in each dimension.
 */
#define MAX_COORD (max_int_in_fixed - 1000)
#define MAX_RESOLUTION 4000
gx_device_bbox gs_bbox_device =
{
    /*
     * Define the device as 8-bit gray scale to avoid computing halftones.
     */
    std_device_dci_body(gx_device_bbox, 0, "bbox",
			MAX_COORD, MAX_COORD,
			MAX_RESOLUTION, MAX_RESOLUTION,
			1, 8, 255, 0, 256, 1),
    {bbox_open_device,
     gx_upright_get_initial_matrix,
     NULL,			/* sync_output */
     bbox_output_page,
     bbox_close_device,
     gx_default_gray_map_rgb_color,
     gx_default_gray_map_color_rgb,
     bbox_fill_rectangle,
     NULL,			/* tile_rectangle */
     bbox_copy_mono,
     bbox_copy_color,
     NULL,			/* draw_line */
     NULL,			/* get_bits */
     bbox_get_params,
     bbox_put_params,
     gx_default_map_cmyk_color,
     NULL,			/* get_xfont_procs */
     NULL,			/* get_xfont_device */
     gx_default_map_rgb_alpha_color,
     gx_page_device_get_page_device,
     NULL,			/* get_alpha_bits */
     bbox_copy_alpha,
     NULL,			/* get_band */
     NULL,			/* copy_rop */
     bbox_fill_path,
     bbox_stroke_path,
     bbox_fill_mask,
     bbox_fill_trapezoid,
     bbox_fill_parallelogram,
     bbox_fill_triangle,
     bbox_draw_thin_line,
     gx_default_begin_image,
     NULL,			/* image_data */
     NULL,			/* end_image */
     bbox_strip_tile_rectangle,
     bbox_strip_copy_rop,
     NULL,			/* get_clipping_box */
     bbox_begin_typed_image,
     NULL,			/* get_bits_rectangle */
     gx_default_map_color_rgb_alpha,
     bbox_create_compositor,
     NULL,			/* get_hardware_params */
     bbox_text_begin,
     NULL,			/* finish_copydevice */
     NULL,			/* begin_transparency_group */
     NULL,			/* end_transparency_group */
     NULL,			/* begin_transparency_mask */
     NULL,			/* end_transparency_mask */
     NULL,			/* discard_transparency_layer */
     NULL,			/* get_color_mapping_procs */
     NULL,			/* get_color_comp_index */
     NULL,			/* encode_color */
     NULL			/* decode_color */
    },
    0,				/* target */
    1,				/*true *//* free_standing */
    1				/*true *//* forward_open_close */
};

#undef MAX_COORD
#undef MAX_RESOLUTION

/* Default box procedures */

bool
bbox_default_init_box(void *pdata)
{
    gx_device_bbox *const bdev = (gx_device_bbox *)pdata;
    gs_fixed_rect *const pr = &bdev->bbox;

    pr->p.x = pr->p.y = max_fixed;
    pr->q.x = pr->q.y = min_fixed;
    return bdev->white != bdev->transparent;
}
#define BBOX_INIT_BOX(bdev)\
  bdev->box_procs.init_box(bdev->box_proc_data)

void
bbox_default_get_box(const void *pdata, gs_fixed_rect *pbox)
{
    const gx_device_bbox *const bdev = (const gx_device_bbox *)pdata;

    *pbox = bdev->bbox;
}
#define BBOX_GET_BOX(bdev, pbox)\
    bdev->box_procs.get_box(bdev->box_proc_data, pbox);

void
bbox_default_add_rect(void *pdata, fixed x0, fixed y0, fixed x1, fixed y1)
{
    gx_device_bbox *const bdev = (gx_device_bbox *)pdata;
    gs_fixed_rect *const pr = &bdev->bbox;

    if (x0 < pr->p.x)
	pr->p.x = x0;
    if (y0 < pr->p.y)
	pr->p.y = y0;
    if (x1 > pr->q.x)
	pr->q.x = x1;
    if (y1 > pr->q.y)
	pr->q.y = y1;
}
#define BBOX_ADD_RECT(bdev, x0, y0, x1, y1)\
    bdev->box_procs.add_rect(bdev->box_proc_data, x0, y0, x1, y1)
#define BBOX_ADD_INT_RECT(bdev, x0, y0, x1, y1)\
    BBOX_ADD_RECT(bdev, int2fixed(x0), int2fixed(y0), int2fixed(x1),\
		  int2fixed(y1))

bool
bbox_default_in_rect(const void *pdata, const gs_fixed_rect *pbox)
{
    const gx_device_bbox *const bdev = (const gx_device_bbox *)pdata;

    return rect_within(*pbox, bdev->bbox);
}
#define BBOX_IN_RECT(bdev, pbox)\
    bdev->box_procs.in_rect(bdev->box_proc_data, pbox)

private const gx_device_bbox_procs_t box_procs_default = {
    bbox_default_init_box, bbox_default_get_box, bbox_default_add_rect,
    bbox_default_in_rect
};

#define RECT_IS_PAGE(dev, x, y, w, h)\
  (x <= 0 && y <= 0 && x + w >= dev->width && y + h >= dev->height)

     /* ---------------- Open/close/page ---------------- */

/* Copy device parameters back from the target. */
private void
bbox_copy_params(gx_device_bbox * bdev, bool remap_colors)
{
    gx_device *tdev = bdev->target;

    if (tdev != 0)
	gx_device_copy_params((gx_device *)bdev, tdev);
    if (remap_colors) {
	bdev->black = gx_device_black((gx_device *)bdev);
	bdev->white = gx_device_white((gx_device *)bdev);
	bdev->transparent =
	    (bdev->white_is_opaque ? gx_no_color_index : bdev->white);
    }
}

#define GX_DC_IS_TRANSPARENT(pdevc, bdev)\
  (gx_dc_pure_color(pdevc) == (bdev)->transparent && gx_dc_is_pure(pdevc))

private int
bbox_close_device(gx_device * dev)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gx_device *tdev = bdev->target;

    if (bdev->box_procs.init_box != box_procs_default.init_box) {
	/*
	 * This device was created as a wrapper for a compositor.
	 * Just free the devices.
	 */
	int code = (bdev->forward_open_close ? gs_closedevice(tdev) : 0);

	gs_free_object(dev->memory, dev, "bbox_close_device(composite)");
	return code;
    } else {
	return (tdev && bdev->forward_open_close ? gs_closedevice(tdev) : 0);
    }
}

/* Initialize a bounding box device. */
void
gx_device_bbox_init(gx_device_bbox * dev, gx_device * target, gs_memory_t *mem)
{
    gx_device_init((gx_device *) dev, (const gx_device *)&gs_bbox_device,
		   (target ? target->memory : mem), true);
    if (target) {
        gx_device_forward_fill_in_procs((gx_device_forward *) dev);
	set_dev_proc(dev, get_initial_matrix, gx_forward_get_initial_matrix);
	set_dev_proc(dev, map_rgb_color, gx_forward_map_rgb_color);
	set_dev_proc(dev, map_color_rgb, gx_forward_map_color_rgb);
	set_dev_proc(dev, map_cmyk_color, gx_forward_map_cmyk_color);
	set_dev_proc(dev, map_rgb_alpha_color, gx_forward_map_rgb_alpha_color);
	set_dev_proc(dev, get_color_mapping_procs, gx_forward_get_color_mapping_procs);
	set_dev_proc(dev, get_color_comp_index, gx_forward_get_color_comp_index);
	set_dev_proc(dev, encode_color, gx_forward_encode_color);
	set_dev_proc(dev, decode_color, gx_forward_decode_color);
	set_dev_proc(dev, pattern_manage, gx_forward_pattern_manage);
	set_dev_proc(dev, fill_rectangle_hl_color, gx_forward_fill_rectangle_hl_color);
	set_dev_proc(dev, include_color_space, gx_forward_include_color_space);
	set_dev_proc(dev, update_spot_equivalent_colors,
				gx_forward_update_spot_equivalent_colors);
	set_dev_proc(dev, get_page_device, gx_forward_get_page_device);
	gx_device_set_target((gx_device_forward *)dev, target);
    } else {
	gx_device_fill_in_procs((gx_device *)dev);
        gx_device_forward_fill_in_procs((gx_device_forward *) dev);
    }
    dev->box_procs = box_procs_default;
    dev->box_proc_data = dev;
    bbox_copy_params(dev, false);
    dev->free_standing = false;	/* being used as a component */
}

/* Set whether a bounding box device propagates open/close to its target. */
void
gx_device_bbox_fwd_open_close(gx_device_bbox * dev, bool forward_open_close)
{
    dev->forward_open_close = forward_open_close;
}

/* Set whether a bounding box device considers white to be opaque. */
void
gx_device_bbox_set_white_opaque(gx_device_bbox *bdev, bool white_is_opaque)
{
    bdev->white_is_opaque = white_is_opaque;
    bdev->transparent =
	(bdev->white_is_opaque ? gx_no_color_index : bdev->white);
}

/* Release a bounding box device. */
void
gx_device_bbox_release(gx_device_bbox *dev)
{
    /* Just release the reference to the target. */
    gx_device_set_target((gx_device_forward *)dev, NULL);
}

/* Read back the bounding box in 1/72" units. */
void
gx_device_bbox_bbox(gx_device_bbox * dev, gs_rect * pbbox)
{
    gs_fixed_rect bbox;

    BBOX_GET_BOX(dev, &bbox);
    if (bbox.p.x > bbox.q.x || bbox.p.y > bbox.q.y) {
	/* Nothing has been written on this page. */
	pbbox->p.x = pbbox->p.y = pbbox->q.x = pbbox->q.y = 0;
    } else {
	gs_rect dbox;
	gs_matrix mat;

	dbox.p.x = fixed2float(bbox.p.x);
	dbox.p.y = fixed2float(bbox.p.y);
	dbox.q.x = fixed2float(bbox.q.x);
	dbox.q.y = fixed2float(bbox.q.y);
	gs_deviceinitialmatrix((gx_device *)dev, &mat);
	gs_bbox_transform_inverse(&dbox, &mat, pbbox);
    }
}

private int
bbox_open_device(gx_device * dev)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;

    if (bdev->free_standing) {
	gx_device_forward_fill_in_procs((gx_device_forward *) dev);
	bdev->box_procs = box_procs_default;
	bdev->box_proc_data = bdev;
    }
    if (bdev->box_procs.init_box == box_procs_default.init_box)
	BBOX_INIT_BOX(bdev);
    /* gx_forward_open_device doesn't exist */
    {
	gx_device *tdev = bdev->target;
	int code =
	    (tdev && bdev->forward_open_close ? gs_opendevice(tdev) : 0);

	bbox_copy_params(bdev, true);
	return code;
    }
}

private int
bbox_output_page(gx_device * dev, int num_copies, int flush)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;

    if (bdev->free_standing) {
	/*
	 * This is a free-standing device.  Print the page bounding box.
	 */
	gs_rect bbox;

	gx_device_bbox_bbox(bdev, &bbox);
	dlprintf4("%%%%BoundingBox: %d %d %d %d\n",
		  (int)floor(bbox.p.x), (int)floor(bbox.p.y),
		  (int)ceil(bbox.q.x), (int)ceil(bbox.q.y));
	dlprintf4("%%%%HiResBoundingBox: %f %f %f %f\n",
		  bbox.p.x, bbox.p.y, bbox.q.x, bbox.q.y);
    }
    return gx_forward_output_page(dev, num_copies, flush);
}

/* ---------------- Low-level drawing ---------------- */

private int
bbox_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		    gx_color_index color)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gx_device *tdev = bdev->target;
    /* gx_forward_fill_rectangle doesn't exist */
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, fill_rectangle)(tdev, x, y, w, h, color));

    /* Check for erasing the entire page. */
    if (RECT_IS_PAGE(dev, x, y, w, h)) {
	if (!BBOX_INIT_BOX(bdev))
	    return code;
    }
    if (color != bdev->transparent)
	BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    return code;
}

private int
bbox_copy_mono(gx_device * dev, const byte * data,
	    int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h,
	       gx_color_index zero, gx_color_index one)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* gx_forward_copy_mono doesn't exist */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, copy_mono)
	 (tdev, data, dx, raster, id, x, y, w, h, zero, one));

    if ((one != gx_no_color_index && one != bdev->transparent) ||
	(zero != gx_no_color_index && zero != bdev->transparent)
	)
	BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    return code;
}

private int
bbox_copy_color(gx_device * dev, const byte * data,
	    int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* gx_forward_copy_color doesn't exist */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, copy_color)
	 (tdev, data, dx, raster, id, x, y, w, h));

    BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    return code;
}

private int
bbox_copy_alpha(gx_device * dev, const byte * data, int data_x,
		int raster, gx_bitmap_id id, int x, int y, int w, int h,
		gx_color_index color, int depth)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* gx_forward_copy_alpha doesn't exist */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, copy_alpha)
	 (tdev, data, data_x, raster, id, x, y, w, h, color, depth));

    BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    return code;
}

private int
bbox_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
   int x, int y, int w, int h, gx_color_index color0, gx_color_index color1,
			  int px, int py)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* Skip the call if there is no target. */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, strip_tile_rectangle)
	 (tdev, tiles, x, y, w, h, color0, color1, px, py));

    if (RECT_IS_PAGE(dev, x, y, w, h)) {
	if (!BBOX_INIT_BOX(bdev))
	    return code;
    }
    BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    return code;
}

private int
bbox_strip_copy_rop(gx_device * dev,
		    const byte * sdata, int sourcex, uint sraster,
		    gx_bitmap_id id,
		    const gx_color_index * scolors,
		    const gx_strip_bitmap * textures,
		    const gx_color_index * tcolors,
		    int x, int y, int w, int h,
		    int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* gx_forward_strip_copy_rop doesn't exist */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, strip_copy_rop)
	 (tdev, sdata, sourcex, sraster, id, scolors,
	  textures, tcolors, x, y, w, h, phase_x, phase_y, lop));

    BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    return code;
}

/* ---------------- Parameters ---------------- */

/* We implement get_params to provide a way to read out the bounding box. */
private int
bbox_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gs_fixed_rect fbox;
    int code = gx_forward_get_params(dev, plist);
    gs_param_float_array bba;
    float bbox[4];

    if (code < 0)
	return code;
    /*
     * We might be calling get_params before the device has been
     * initialized: in this case, box_proc_data = 0.
     */
    if (bdev->box_proc_data == 0)
	fbox = bdev->bbox;
    else
	BBOX_GET_BOX(bdev, &fbox);
    bbox[0] = fixed2float(fbox.p.x);
    bbox[1] = fixed2float(fbox.p.y);
    bbox[2] = fixed2float(fbox.q.x);
    bbox[3] = fixed2float(fbox.q.y);
    bba.data = bbox, bba.size = 4, bba.persistent = false;
    code = param_write_float_array(plist, "PageBoundingBox", &bba);
    if (code < 0)
        return code;
    code = param_write_bool(plist, "WhiteIsOpaque", &bdev->white_is_opaque);
    return code;
}

/* We implement put_params to ensure that we keep the important */
/* device parameters up to date, and to prevent an /undefined error */
/* from PageBoundingBox. */
private int
bbox_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    int code;
    int ecode = 0;
    bool white_is_opaque = bdev->white_is_opaque;
    gs_param_name param_name;
    gs_param_float_array bba;

    code = param_read_float_array(plist, (param_name = "PageBoundingBox"),
				  &bba);
    switch (code) {
	case 0:
	    if (bba.size != 4) {
		ecode = gs_note_error(gs_error_rangecheck);
		goto e;
	    }
	    break;
	default:
	    ecode = code;
	    e:param_signal_error(plist, param_name, ecode);
	case 1:
	    bba.data = 0;
    }

    switch (code = param_read_bool(plist, (param_name = "WhiteIsOpaque"), &white_is_opaque)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
        case 1:
	    break;
    }

    code = gx_forward_put_params(dev, plist);
    if (ecode < 0)
	code = ecode;
    if (code >= 0) {
        if( bba.data != 0) {
	    BBOX_INIT_BOX(bdev);
	    BBOX_ADD_RECT(bdev, float2fixed(bba.data[0]), float2fixed(bba.data[1]),
	    	          float2fixed(bba.data[2]), float2fixed(bba.data[3]));
        }
        bdev->white_is_opaque = white_is_opaque;
    }
    bbox_copy_params(bdev, bdev->is_open);
    return code;
}

/* ---------------- Polygon drawing ---------------- */

private fixed
edge_x_at_y(const gs_fixed_edge * edge, fixed y)
{
    return fixed_mult_quo(edge->end.x - edge->start.x,
			  y - edge->start.y,
			  edge->end.y - edge->start.y) + edge->start.x;
}
private int
bbox_fill_trapezoid(gx_device * dev,
		    const gs_fixed_edge * left, const gs_fixed_edge * right,
		    fixed ybot, fixed ytop, bool swap_axes,
		    const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* Skip the call if there is no target. */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, fill_trapezoid)
	 (tdev, left, right, ybot, ytop, swap_axes, pdevc, lop));

    if (!GX_DC_IS_TRANSPARENT(pdevc, bdev)) {
	fixed x0l =
	    (left->start.y == ybot ? left->start.x :
	     edge_x_at_y(left, ybot));
	fixed x1l =
	    (left->end.y == ytop ? left->end.x :
	     edge_x_at_y(left, ytop));
	fixed x0r =
	    (right->start.y == ybot ? right->start.x :
	     edge_x_at_y(right, ybot));
	fixed x1r =
	    (right->end.y == ytop ? right->end.x :
	     edge_x_at_y(right, ytop));
	fixed xminl = min(x0l, x1l), xmaxl = max(x0l, x1l);
	fixed xminr = min(x0r, x1r), xmaxr = max(x0r, x1r);
	fixed x0 = min(xminl, xminr), x1 = max(xmaxl, xmaxr);

	if (swap_axes)
	    BBOX_ADD_RECT(bdev, ybot, x0, ytop, x1);
	else
	    BBOX_ADD_RECT(bdev, x0, ybot, x1, ytop);
    }
    return code;
}

private int
bbox_fill_parallelogram(gx_device * dev,
			fixed px, fixed py, fixed ax, fixed ay,
			fixed bx, fixed by, const gx_device_color * pdevc,
			gs_logical_operation_t lop)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* Skip the call if there is no target. */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, fill_parallelogram)
	 (tdev, px, py, ax, ay, bx, by, pdevc, lop));

    if (!GX_DC_IS_TRANSPARENT(pdevc, bdev)) {
	fixed xmin, ymin, xmax, ymax;

	/* bbox_add_rect requires points in correct order. */
#define SET_MIN_MAX(vmin, vmax, av, bv)\
  BEGIN\
    if (av <= 0) {\
	if (bv <= 0)\
	    vmin = av + bv, vmax = 0;\
	else\
	    vmin = av, vmax = bv;\
    } else if (bv <= 0)\
	vmin = bv, vmax = av;\
    else\
	vmin = 0, vmax = av + bv;\
  END
	SET_MIN_MAX(xmin, xmax, ax, bx);
	SET_MIN_MAX(ymin, ymax, ay, by);
#undef SET_MIN_MAX
	BBOX_ADD_RECT(bdev, px + xmin, py + ymin, px + xmax, py + ymax);
    }
    return code;
}

private int
bbox_fill_triangle(gx_device * dev,
		   fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		   const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* Skip the call if there is no target. */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, fill_triangle)
	 (tdev, px, py, ax, ay, bx, by, pdevc, lop));

    if (!GX_DC_IS_TRANSPARENT(pdevc, bdev)) {
	fixed xmin, ymin, xmax, ymax;

	/* bbox_add_rect requires points in correct order. */
#define SET_MIN_MAX(vmin, vmax, av, bv)\
  BEGIN\
    if (av <= 0) {\
	if (bv <= 0)\
	    vmin = min(av, bv), vmax = 0;\
	else\
	    vmin = av, vmax = bv;\
    } else if (bv <= 0)\
	vmin = bv, vmax = av;\
    else\
	vmin = 0, vmax = max(av, bv);\
  END
	SET_MIN_MAX(xmin, xmax, ax, bx);
	SET_MIN_MAX(ymin, ymax, ay, by);
#undef SET_MIN_MAX
	BBOX_ADD_RECT(bdev, px + xmin, py + ymin, px + xmax, py + ymax);
    }
    return code;
}

private int
bbox_draw_thin_line(gx_device * dev,
		    fixed fx0, fixed fy0, fixed fx1, fixed fy1,
		    const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    /* Skip the call if there is no target. */
    gx_device *tdev = bdev->target;
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, draw_thin_line)
	 (tdev, fx0, fy0, fx1, fy0, pdevc, lop));

    if (!GX_DC_IS_TRANSPARENT(pdevc, bdev)) {
	fixed xmin, ymin, xmax, ymax;

	/* bbox_add_rect requires points in correct order. */
#define SET_MIN_MAX(vmin, vmax, av, bv)\
  BEGIN\
    if (av < bv)\
	vmin = av, vmax = bv;\
    else\
	vmin = bv, vmax = av;\
  END
	SET_MIN_MAX(xmin, xmax, fx0, fx1);
	SET_MIN_MAX(ymin, ymax, fy0, fy1);
#undef SET_MIN_MAX
	BBOX_ADD_RECT(bdev, xmin, ymin, xmax, ymax);
    }
    return code;
}

/* ---------------- High-level drawing ---------------- */

#define adjust_box(pbox, adj)\
((pbox)->p.x -= (adj).x, (pbox)->p.y -= (adj).y,\
 (pbox)->q.x += (adj).x, (pbox)->q.y += (adj).y)

private int
bbox_fill_path(gx_device * dev, const gs_imager_state * pis, gx_path * ppath,
	       const gx_fill_params * params, const gx_device_color * pdevc,
	       const gx_clip_path * pcpath)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gx_device *tdev = bdev->target;
    dev_proc_fill_path((*fill_path)) =
	(tdev == 0 ? dev_proc(&gs_null_device, fill_path) :
	 dev_proc(tdev, fill_path));
    int code;

    if (!GX_DC_IS_TRANSPARENT(pdevc, bdev) && !gx_path_is_void(ppath)) {
	gs_fixed_rect ibox;
	gs_fixed_point adjust;

	if (gx_path_bbox(ppath, &ibox) < 0)
	    return 0;
	adjust = params->adjust;
	if (params->fill_zero_width)
	    gx_adjust_if_empty(&ibox, &adjust);
	adjust_box(&ibox, adjust);
	/*
	 * If the path lies within the already accumulated box, just draw
	 * on the target.
	 */
	if (BBOX_IN_RECT(bdev, &ibox))
	    return fill_path(tdev, pis, ppath, params, pdevc, pcpath);
	/*
	 * If the target uses the default algorithm, just draw on the
	 * bbox device.
	 */
	if (tdev != 0 && fill_path == gx_default_fill_path)
	    return fill_path(dev, pis, ppath, params, pdevc, pcpath);
	/* Draw on the target now. */
	code = fill_path(tdev, pis, ppath, params, pdevc, pcpath);
	if (code < 0)
	    return code;
	if (pcpath != NULL &&
	    !gx_cpath_includes_rectangle(pcpath, ibox.p.x, ibox.p.y,
					 ibox.q.x, ibox.q.y)
	    ) {
	    /*
	     * Let the target do the drawing, but break down the
	     * fill path into pieces for computing the bounding box.
	     */
	    gx_drawing_color devc;

	    set_nonclient_dev_color(&devc, bdev->black);  /* any non-white color will do */
	    bdev->target = NULL;
	    code = gx_default_fill_path(dev, pis, ppath, params, &devc, pcpath);
	    bdev->target = tdev;
	} else {		/* Just use the path bounding box. */
	    BBOX_ADD_RECT(bdev, ibox.p.x, ibox.p.y, ibox.q.x, ibox.q.y);
	}
	return code;
    } else
	return fill_path(tdev, pis, ppath, params, pdevc, pcpath);
}

private int
bbox_stroke_path(gx_device * dev, const gs_imager_state * pis, gx_path * ppath,
		 const gx_stroke_params * params,
		 const gx_drawing_color * pdevc, const gx_clip_path * pcpath)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gx_device *tdev = bdev->target;
    /* Skip the call if there is no target. */
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, stroke_path)(tdev, pis, ppath, params, pdevc, pcpath));

    if (!GX_DC_IS_TRANSPARENT(pdevc, bdev)) {
	gs_fixed_rect ibox;
	gs_fixed_point expand;

	if (gx_stroke_path_expansion(pis, ppath, &expand) == 0 &&
	    gx_path_bbox(ppath, &ibox) >= 0
	    ) {
	    /* The fast result is exact. */
	    adjust_box(&ibox, expand);
	} else {
	    /*
	     * The result is not exact.  Compute an exact result using
	     * strokepath.
	     */
	    gx_path *spath = gx_path_alloc(pis->memory, "bbox_stroke_path");
	    int code = 0;

	    if (spath)
		code = gx_imager_stroke_add(ppath, spath, dev, pis);
	    else
		code = -1;
	    if (code >= 0)
		code = gx_path_bbox(spath, &ibox);
	    if (code < 0) {
		ibox.p.x = ibox.p.y = min_fixed;
		ibox.q.x = ibox.q.y = max_fixed;
	    }
	    if (spath)
		gx_path_free(spath, "bbox_stroke_path");
	}
	if (pcpath != NULL &&
	    !gx_cpath_includes_rectangle(pcpath, ibox.p.x, ibox.p.y,
					 ibox.q.x, ibox.q.y)
	    ) {
	    /* Let the target do the drawing, but break down the */
	    /* fill path into pieces for computing the bounding box. */
	    gx_drawing_color devc;

	    set_nonclient_dev_color(&devc, bdev->black);  /* any non-white color will do */
	    bdev->target = NULL;
	    gx_default_stroke_path(dev, pis, ppath, params, &devc, pcpath);
	    bdev->target = tdev;
	} else {
	    /* Just use the path bounding box. */
	    BBOX_ADD_RECT(bdev, ibox.p.x, ibox.p.y, ibox.q.x, ibox.q.y);
	}
    }
    return code;
}

private int
bbox_fill_mask(gx_device * dev,
	       const byte * data, int dx, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h,
	       const gx_drawing_color * pdcolor, int depth,
	       gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gx_device *tdev = bdev->target;
    /* Skip the call if there is no target. */
    int code =
	(tdev == 0 ? 0 :
	 dev_proc(tdev, fill_mask)
	 (tdev, data, dx, raster, id, x, y, w, h,
	  pdcolor, depth, lop, pcpath));

    if (pcpath != NULL &&
	!gx_cpath_includes_rectangle(pcpath, int2fixed(x), int2fixed(y),
				     int2fixed(x + w),
				     int2fixed(y + h))
	) {
	/* Let the target do the drawing, but break down the */
	/* image into pieces for computing the bounding box. */
	bdev->target = NULL;
	gx_default_fill_mask(dev, data, dx, raster, id, x, y, w, h,
			     pdcolor, depth, lop, pcpath);
	bdev->target = tdev;
    } else {
	/* Just use the mask bounding box. */
	BBOX_ADD_INT_RECT(bdev, x, y, x + w, y + h);
    }
    return code;
}

/* ------ Bitmap imaging ------ */

typedef struct bbox_image_enum_s {
    gx_image_enum_common;
    gs_memory_t *memory;
    gs_matrix matrix;		/* map from image space to device space */
    const gx_clip_path *pcpath;
    gx_image_enum_common_t *target_info;
    bool params_are_const;
    int x0, x1;
    int y, height;
} bbox_image_enum;

gs_private_st_suffix_add2(st_bbox_image_enum, bbox_image_enum,
  "bbox_image_enum", bbox_image_enum_enum_ptrs, bbox_image_enum_reloc_ptrs,
  st_gx_image_enum_common, pcpath, target_info);

private image_enum_proc_plane_data(bbox_image_plane_data);
private image_enum_proc_end_image(bbox_image_end_image);
private image_enum_proc_flush(bbox_image_flush);
private image_enum_proc_planes_wanted(bbox_image_planes_wanted);
private const gx_image_enum_procs_t bbox_image_enum_procs = {
    bbox_image_plane_data, bbox_image_end_image,
    bbox_image_flush, bbox_image_planes_wanted
};

private int
bbox_image_begin(const gs_imager_state * pis, const gs_matrix * pmat,
		 const gs_image_common_t * pic, const gs_int_rect * prect,
		 const gx_clip_path * pcpath, gs_memory_t * memory,
		 bbox_image_enum ** ppbe)
{
    int code;
    gs_matrix mat;
    bbox_image_enum *pbe;

    if (pmat == 0)
	pmat = &ctm_only(pis);
    if ((code = gs_matrix_invert(&pic->ImageMatrix, &mat)) < 0 ||
	(code = gs_matrix_multiply(&mat, pmat, &mat)) < 0
	)
	return code;
    pbe = gs_alloc_struct(memory, bbox_image_enum, &st_bbox_image_enum,
			  "bbox_image_begin");
    if (pbe == 0)
	return_error(gs_error_VMerror);
    pbe->memory = memory;
    pbe->matrix = mat;
    pbe->pcpath = pcpath;
    pbe->target_info = 0;	/* in case no target */
    pbe->params_are_const = false;	/* check the first time */
    if (prect) {
	pbe->x0 = prect->p.x, pbe->x1 = prect->q.x;
	pbe->y = prect->p.y, pbe->height = prect->q.y - prect->p.y;
    } else {
	gs_int_point size;
	int code = (*pic->type->source_size) (pis, pic, &size);

	if (code < 0) {
	    gs_free_object(memory, pbe, "bbox_image_begin");
	    return code;
	}
	pbe->x0 = 0, pbe->x1 = size.x;
	pbe->y = 0, pbe->height = size.y;
    }
    *ppbe = pbe;
    return 0;
}

private void
bbox_image_copy_target_info(bbox_image_enum * pbe)
{
    const gx_image_enum_common_t *target_info = pbe->target_info;

    pbe->num_planes = target_info->num_planes;
    memcpy(pbe->plane_depths, target_info->plane_depths,
	   pbe->num_planes * sizeof(pbe->plane_depths[0]));
    memcpy(pbe->plane_widths, target_info->plane_widths,
	   pbe->num_planes * sizeof(pbe->plane_widths[0]));
}

private int
bbox_begin_typed_image(gx_device * dev,
		       const gs_imager_state * pis, const gs_matrix * pmat,
		   const gs_image_common_t * pic, const gs_int_rect * prect,
		       const gx_drawing_color * pdcolor,
		       const gx_clip_path * pcpath,
		       gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    bbox_image_enum *pbe;
    int code =
	bbox_image_begin(pis, pmat, pic, prect, pcpath, memory, &pbe);

    if (code < 0)
	return code;
    /*
     * If there is no target, we still have to call default_begin_image
     * to get the correct num_planes and plane_depths.
     */
    {
	gx_device_bbox *const bdev = (gx_device_bbox *) dev;
	gx_device *tdev = bdev->target;
	dev_proc_begin_typed_image((*begin_typed_image));
	byte wanted[GS_IMAGE_MAX_PLANES];

	if (tdev == 0) {
	    tdev = dev;
	    begin_typed_image = gx_default_begin_typed_image;
	} else {
	    begin_typed_image = dev_proc(tdev, begin_typed_image);
	}
	code = (*begin_typed_image)
	    (tdev, pis, pmat, pic, prect, pdcolor, pcpath, memory,
	     &pbe->target_info);
	if (code) {
	    bbox_image_end_image((gx_image_enum_common_t *)pbe, false);
	    return code;
	}
	/*
	 * We fill in num_planes and plane_depths later.  format is
	 * irrelevant.  NOTE: we assume that if begin_typed_image returned
	 * 0, the image is a data image.
	 */
	code = gx_image_enum_common_init((gx_image_enum_common_t *) pbe,
					 (const gs_data_image_t *)pic,
					 &bbox_image_enum_procs, dev,
					 0, gs_image_format_chunky);
	if (code < 0)
	    return code;
	bbox_image_copy_target_info(pbe);
	pbe->params_are_const =
	    gx_image_planes_wanted(pbe->target_info, wanted);
    }
    *pinfo = (gx_image_enum_common_t *) pbe;
    return 0;
}

private int
bbox_image_plane_data(gx_image_enum_common_t * info,
		      const gx_image_plane_t * planes, int height,
		      int *rows_used)
{
    gx_device *dev = info->dev;
    gx_device_bbox *const bdev = (gx_device_bbox *)dev;
    gx_device *tdev = bdev->target;
    bbox_image_enum *pbe = (bbox_image_enum *) info;
    const gx_clip_path *pcpath = pbe->pcpath;
    gs_rect sbox, dbox;
    gs_point corners[4];
    gs_fixed_rect ibox;
    int code;

    code = gx_image_plane_data_rows(pbe->target_info, planes, height,
				    rows_used);
    if (code != 1 && !pbe->params_are_const)
	bbox_image_copy_target_info(pbe);
    sbox.p.x = pbe->x0;
    sbox.p.y = pbe->y;
    sbox.q.x = pbe->x1;
    sbox.q.y = pbe->y = min(pbe->y + height, pbe->height);
    gs_bbox_transform_only(&sbox, &pbe->matrix, corners);
    gs_points_bbox(corners, &dbox);
    ibox.p.x = float2fixed(dbox.p.x);
    ibox.p.y = float2fixed(dbox.p.y);
    ibox.q.x = float2fixed(dbox.q.x);
    ibox.q.y = float2fixed(dbox.q.y);
    if (pcpath != NULL &&
	!gx_cpath_includes_rectangle(pcpath, ibox.p.x, ibox.p.y,
				     ibox.q.x, ibox.q.y)
	) {
	/* Let the target do the drawing, but drive two triangles */
	/* through the clipping path to get an accurate bounding box. */
	gx_device_clip cdev;
	gx_drawing_color devc;
	fixed x0 = float2fixed(corners[0].x), y0 = float2fixed(corners[0].y);
	fixed bx2 = float2fixed(corners[2].x) - x0, by2 = float2fixed(corners[2].y) - y0;

	gx_make_clip_path_device(&cdev, pcpath);
	cdev.target = dev;
	(*dev_proc(&cdev, open_device)) ((gx_device *) & cdev);
	set_nonclient_dev_color(&devc, bdev->black);  /* any non-white color will do */
	bdev->target = NULL;
	gx_default_fill_triangle((gx_device *) & cdev, x0, y0,
				 float2fixed(corners[1].x) - x0,
				 float2fixed(corners[1].y) - y0,
				 bx2, by2, &devc, lop_default);
	gx_default_fill_triangle((gx_device *) & cdev, x0, y0,
				 float2fixed(corners[3].x) - x0,
				 float2fixed(corners[3].y) - y0,
				 bx2, by2, &devc, lop_default);
	bdev->target = tdev;
    } else {
	/* Just use the bounding box. */
	BBOX_ADD_RECT(bdev, ibox.p.x, ibox.p.y, ibox.q.x, ibox.q.y);
    }
    return code;
}

private int
bbox_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    bbox_image_enum *pbe = (bbox_image_enum *) info;
    int code = gx_image_end(pbe->target_info, draw_last);

    gs_free_object(pbe->memory, pbe, "bbox_end_image");
    return code;
}

private int
bbox_image_flush(gx_image_enum_common_t * info)
{
    bbox_image_enum *pbe = (bbox_image_enum *) info;
    gx_image_enum_common_t *target_info = pbe->target_info;

    return (target_info ? gx_image_flush(target_info) : 0);
}

private bool
bbox_image_planes_wanted(const gx_image_enum_common_t * info, byte *wanted)
{
    /* This is only used if target_info != 0. */
    const bbox_image_enum *pbe = (const bbox_image_enum *)info;
    
    return gx_image_planes_wanted(pbe->target_info, wanted);
}

/* Compositing */

private bool
bbox_forward_init_box(void *pdata)
{
    gx_device_bbox *const bdev = (gx_device_bbox *)pdata;

    return BBOX_INIT_BOX(bdev);
}
private void
bbox_forward_get_box(const void *pdata, gs_fixed_rect *pbox)
{
    const gx_device_bbox *const bdev = (const gx_device_bbox *)pdata;

    BBOX_GET_BOX(bdev, pbox);
}
private void
bbox_forward_add_rect(void *pdata, fixed x0, fixed y0, fixed x1, fixed y1)
{
    gx_device_bbox *const bdev = (gx_device_bbox *)pdata;

    BBOX_ADD_RECT(bdev, x0, y0, x1, y1);
}
private bool
bbox_forward_in_rect(const void *pdata, const gs_fixed_rect *pbox)
{
    const gx_device_bbox *const bdev = (const gx_device_bbox *)pdata;

    return BBOX_IN_RECT(bdev, pbox);
}
private const gx_device_bbox_procs_t box_procs_forward = {
    bbox_forward_init_box, bbox_forward_get_box, bbox_forward_add_rect,
    bbox_forward_in_rect
};

private int
bbox_create_compositor(gx_device * dev,
		       gx_device ** pcdev, const gs_composite_t * pcte,
		       gs_imager_state * pis, gs_memory_t * memory)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    gx_device *target = bdev->target;

    /*
     * If there isn't a target, all we care about is the bounding box,
     * so don't bother with actually compositing.
     */
    if (target == 0) {
	*pcdev = dev;
	return 0;
    }
    /*
     * Create a compositor for the target, and then wrap another
     * bbox device around it, but still accumulating the bounding
     * box in the same place.
     */
    {
	gx_device *cdev;
	gx_device_bbox *bbcdev;
	int code = (*dev_proc(target, create_compositor))
	    (target, &cdev, pcte, pis, memory);

	/* If the target did not create a new compositor then we are done. */
	if (code < 0 || target == cdev) {
	    *pcdev = dev;
	    return code;
	}
	bbcdev = gs_alloc_struct_immovable(memory, gx_device_bbox,
					   &st_device_bbox,
					   "bbox_create_compositor");
	if (bbcdev == 0) {
	    (*dev_proc(cdev, close_device)) (cdev);
	    return_error(gs_error_VMerror);
	}
	gx_device_bbox_init(bbcdev, target, memory);
	gx_device_set_target((gx_device_forward *)bbcdev, cdev);
	bbcdev->box_procs = box_procs_forward;
	bbcdev->box_proc_data = bdev;
	*pcdev = (gx_device *) bbcdev;
	return 0;
    }
}

/* ------ Text imaging ------ */

private int
bbox_text_begin(gx_device * dev, gs_imager_state * pis,
		const gs_text_params_t * text, gs_font * font,
		gx_path * path, const gx_device_color * pdcolor,
		const gx_clip_path * pcpath,
		gs_memory_t * memory, gs_text_enum_t ** ppenum)
{
    gx_device_bbox *const bdev = (gx_device_bbox *) dev;
    int code = gx_default_text_begin(dev, pis, text, font, path, pdcolor,
				     pcpath, memory, ppenum);

    if (bdev->target != NULL) {
        /* See note on imaging_dev in gxtext.h */
        rc_assign((*ppenum)->imaging_dev, dev, "bbox_text_begin");
    }

    return code;
}

