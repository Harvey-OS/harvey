/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsropc.c,v 1.5 2005/03/14 18:08:36 dan Exp $ */
/* RasterOp-compositing implementation */
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxropc.h"

/* ------ Object definition and creation ------ */

/* Define RasterOp-compositing objects. */
private composite_create_default_compositor_proc(c_rop_create_default_compositor);
private composite_equal_proc(c_rop_equal);
private composite_write_proc(c_rop_write);
private composite_read_proc(c_rop_read);
private const gs_composite_type_t gs_composite_rop_type =
{
    {
	c_rop_create_default_compositor,
	c_rop_equal,
	c_rop_write,
	c_rop_read,
	gx_default_composite_clist_write_update,
	gx_default_composite_clist_read_update
    }
};

private_st_composite_rop();

/* Create a RasterOp-compositing object. */
int
gs_create_composite_rop(gs_composite_t ** ppcte,
		const gs_composite_rop_params_t * params, gs_memory_t * mem)
{
    gs_composite_rop_t *pcte;

    rc_alloc_struct_0(pcte, gs_composite_rop_t, &st_composite_rop,
		      mem, return_error(gs_error_VMerror),
		      "gs_create_composite_rop");
    pcte->type = &gs_composite_rop_type;
    pcte->id = gs_next_ids(1);
    pcte->params = *params;
    *ppcte = (gs_composite_t *) pcte;
    return 0;
}

/* ------ Object implementation ------ */

#define prcte ((const gs_composite_rop_t *)pcte)

private bool
c_rop_equal(const gs_composite_t * pcte, const gs_composite_t * pcte2)
{
    return (pcte2->type == pcte->type &&
#define prcte2 ((const gs_composite_rop_t *)pcte2)
	    prcte2->params.log_op == prcte->params.log_op &&
	    (prcte->params.texture == 0 ? prcte2->params.texture == 0 :
	     prcte2->params.texture != 0 &&
	     gx_device_color_equal(prcte->params.texture,
				   prcte2->params.texture)));
#undef prcte2
}

private int
c_rop_write(const gs_composite_t * pcte, byte * data, uint * psize)
{
/****** NYI ******/
}

private int
c_rop_read(gs_composite_t ** ppcte, const byte * data, uint size,
	   gs_memory_t * mem)
{
/****** NYI ******/
}

/* ---------------- RasterOp-compositing device ---------------- */

/* Define the default RasterOp-compositing device. */
typedef struct gx_device_composite_rop_s {
    gx_device_forward_common;
    gs_composite_rop_params_t params;
} gx_device_composite_rop;

gs_private_st_suffix_add1_final(st_device_composite_rop,
			 gx_device_composite_rop, "gx_device_composite_rop",
	device_c_rop_enum_ptrs, device_c_rop_reloc_ptrs, gx_device_finalize,
				st_device_forward, params.texture);
/* The device descriptor. */
private dev_proc_close_device(dcr_close);
private dev_proc_fill_rectangle(dcr_fill_rectangle);
private dev_proc_copy_mono(dcr_copy_mono);
private dev_proc_copy_color(dcr_copy_color);
private dev_proc_copy_alpha(dcr_copy_alpha);
private const gx_device_composite_rop gs_composite_rop_device =
{std_device_std_body_open(gx_device_composite_rop, 0,
			  "RasterOp compositor", 0, 0, 1, 1),
 {gx_default_open_device,
  gx_forward_get_initial_matrix,
  gx_forward_sync_output,
  gx_forward_output_page,
  dcr_close,
  gx_forward_map_rgb_color,
  gx_forward_map_color_rgb,
  dcr_fill_rectangle,
  gx_default_tile_rectangle,
  dcr_copy_mono,
  dcr_copy_color,
  gx_default_draw_line,
  gx_default_get_bits,
  gx_forward_get_params,
  gx_forward_put_params,
  gx_forward_map_cmyk_color,
  gx_forward_get_xfont_procs,
  gx_forward_get_xfont_device,
  gx_forward_map_rgb_alpha_color,
  gx_forward_get_page_device,
  gx_forward_get_alpha_bits,
  dcr_copy_alpha,
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
  gx_default_strip_tile_rectangle,
  gx_default_strip_copy_rop,
  gx_forward_get_clipping_box,
  gx_default_begin_typed_image,
  gx_forward_get_bits_rectangle,
  gx_forward_map_color_rgb_alpha,
  gx_no_create_compositor
 }
};

/* Create a RasterOp compositor. */
int
c_rop_create_default_compositor(const gs_composite_t * pcte,
	gx_device ** pcdev, gx_device * dev, gs_imager_state * pis,
	gs_memory_t * mem)
{
    gs_logical_operation_t log_op = prcte->params.log_op;
    const gx_device_color *texture = prcte->params.texture;
    gx_device_composite_rop *cdev;

#if 0				/*************** */
    if (<<operation is identity >>) {
	/* Just use the original device. */
	*pcdev = dev;
	return 0;
    }
#endif /*************** */
    cdev =
	gs_alloc_struct_immovable(mem, gx_device_composite_rop,
				  &st_device_composite_rop,
				  "create default RasterOp compositor");
    *pcdev = (gx_device *) cdev;
    if (cdev == 0)
	return_error(gs_error_VMerror);
    gx_device_init((gx_device *)cdev,
		   (const gx_device *)&gs_composite_rop_device, mem, true);
    gx_device_copy_params((gx_device *)cdev, dev);
    /*
     * Check for memory devices, and select the correct RasterOp
     * implementation based on depth and device color space.
     ****** NYI ******
     */
    gx_device_set_target((gx_device_forward *)cdev, dev);
    cdev->params = prcte->params;
    return 0;
}

/* Close the device and free its storage. */
private int
dcr_close(gx_device * dev)
{				/*
				 * Finalization will call close again: avoid a recursion loop.
				 */
    set_dev_proc(dev, close_device, gx_default_close_device);
    gs_free_object(dev->memory, dev, "dcr_close");
    return 0;
}

/* ------ Imaging ------ */

/* Import the existing RasterOp implementations. */
extern dev_proc_strip_copy_rop(gx_default_strip_copy_rop);

private int
dcr_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		   gx_color_index color)
{
    gx_device_composite_rop *rdev = (gx_device_composite_rop *) dev;

    /*
     * This is where all the work gets done right now.
     * Sooner or later, we'll have to do the right thing here....
     */
    gs_logical_operation_t log_op = rdev->params.log_op;
    const gx_device_color *texture = rdev->params.texture;
    gx_color_index colors[2];
    gx_color_index tcolors[2];

    dev_proc_strip_copy_rop((*copy)) = gx_default_strip_copy_rop;

    colors[0] = colors[1] = color;
    if (gs_device_is_memory(dev)) {
/****** SHOULD CHECK FOR STANDARD COLOR REPRESENTATION ******/
	switch (dev->color_info.depth) {
	    case 1:
		copy = mem_mono_strip_copy_rop;
		break;
	    case 2:
	    case 4:
		copy = mem_gray_strip_copy_rop;
		break;
	    case 8:
	    case 24:
		copy = mem_gray8_rgb24_strip_copy_rop;
		break;
	    case 16:
	    case 32:
/****** NOT IMPLEMENTED ******/
	}
    }
    if (texture == 0) {
	return (*copy)
	    (dev, (const byte *)0, 0, 0, gx_no_bitmap_id,
	     (const gx_color_index *)0, (const gx_strip_bitmap *)0, colors,
	     x, y, w, h, 0, 0, log_op);
    }
    /* Apply the texture, whatever it may be. */
    if (gx_dc_is_pure(texture)) {
	tcolors[0] = tcolors[1] = texture->colors.pure;
	return (*copy)
	    (dev, (const byte *)0, 0, 0, gx_no_bitmap_id, colors,
	     (const gx_strip_bitmap *)0, tcolors,
	     x, y, w, h, 0, 0, log_op);
    } else if (gx_dc_is_binary_halftone(texture)) {
	tcolors[0] = texture->colors.binary.color[0];
	tcolors[1] = texture->colors.binary.color[1];
	return (*copy)
	    (dev, (const byte *)0, 0, 0, gx_no_bitmap_id, colors,
	     &texture->colors.binary.b_tile->tiles, tcolors,
	     x, y, w, h, texture->phase.x, texture->phase.y, log_op);
    } else if (gx_dc_is_colored_halftone(texture)) {
/****** NO CAN DO ******/
    } else
/****** WHAT ABOUT PATTERNS? ******/
	return_error(gs_error_rangecheck);
}

private int
dcr_copy_mono(gx_device * dev, const byte * data,
	    int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h,
	      gx_color_index zero, gx_color_index one)
{
/****** TEMPORARY ******/
    return gx_default_copy_mono(dev, data, dx, raster, id, x, y, w, h,
				zero, one);
}

private int
dcr_copy_color(gx_device * dev, const byte * data,
	       int dx, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h)
{
/****** TEMPORARY ******/
    return gx_default_copy_color(dev, data, dx, raster, id, x, y, w, h);
}

private int
dcr_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
	       gx_color_index color, int depth)
{
/****** TEMPORARY ******/
    return gx_default_copy_alpha(dev, data, data_x, raster, id, x, y,
				 width, height, color, depth);
}
