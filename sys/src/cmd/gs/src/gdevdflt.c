/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevdflt.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Default device implementation */
#include "gx.h"
#include "gserrors.h"
#include "gsropt.h"
#include "gxcomp.h"
#include "gxdevice.h"

/* ---------------- Default device procedures ---------------- */

/* Fill in NULL procedures in a device procedure record. */
void
gx_device_fill_in_procs(register gx_device * dev)
{
    gx_device_set_procs(dev);
    fill_dev_proc(dev, open_device, gx_default_open_device);
    fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
    fill_dev_proc(dev, sync_output, gx_default_sync_output);
    fill_dev_proc(dev, output_page, gx_default_output_page);
    fill_dev_proc(dev, close_device, gx_default_close_device);
    fill_dev_proc(dev, map_rgb_color, gx_default_map_rgb_color);
    fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);
    /* NOT fill_rectangle */
    fill_dev_proc(dev, tile_rectangle, gx_default_tile_rectangle);
    fill_dev_proc(dev, copy_mono, gx_default_copy_mono);
    fill_dev_proc(dev, copy_color, gx_default_copy_color);
    fill_dev_proc(dev, obsolete_draw_line, gx_default_draw_line);
    fill_dev_proc(dev, get_bits, gx_default_get_bits);
    fill_dev_proc(dev, get_params, gx_default_get_params);
    fill_dev_proc(dev, put_params, gx_default_put_params);
    fill_dev_proc(dev, map_cmyk_color, gx_default_map_cmyk_color);
    fill_dev_proc(dev, get_xfont_procs, gx_default_get_xfont_procs);
    fill_dev_proc(dev, get_xfont_device, gx_default_get_xfont_device);
    fill_dev_proc(dev, map_rgb_alpha_color, gx_default_map_rgb_alpha_color);
    fill_dev_proc(dev, get_page_device, gx_default_get_page_device);
    fill_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    fill_dev_proc(dev, copy_alpha, gx_default_copy_alpha);
    fill_dev_proc(dev, get_band, gx_default_get_band);
    fill_dev_proc(dev, copy_rop, gx_default_copy_rop);
    fill_dev_proc(dev, fill_path, gx_default_fill_path);
    fill_dev_proc(dev, stroke_path, gx_default_stroke_path);
    fill_dev_proc(dev, fill_mask, gx_default_fill_mask);
    fill_dev_proc(dev, fill_trapezoid, gx_default_fill_trapezoid);
    fill_dev_proc(dev, fill_parallelogram, gx_default_fill_parallelogram);
    fill_dev_proc(dev, fill_triangle, gx_default_fill_triangle);
    fill_dev_proc(dev, draw_thin_line, gx_default_draw_thin_line);
    fill_dev_proc(dev, begin_image, gx_default_begin_image);
    /*
     * We always replace get_alpha_bits, image_data, and end_image with the
     * new procedures, and, if in a DEBUG configuration, print a warning if
     * the definitions aren't the default ones.
     */
#ifdef DEBUG
#  define CHECK_NON_DEFAULT(proc, default, procname)\
    BEGIN\
	if ( dev_proc(dev, proc) != NULL && dev_proc(dev, proc) != default )\
	    dprintf2("**** Warning: device %s implements obsolete procedure %s\n",\
		     dev->dname, procname);\
    END
#else
#  define CHECK_NON_DEFAULT(proc, default, procname)\
    DO_NOTHING
#endif
    CHECK_NON_DEFAULT(get_alpha_bits, gx_default_get_alpha_bits,
		      "get_alpha_bits");
    set_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    CHECK_NON_DEFAULT(image_data, gx_default_image_data, "image_data");
    set_dev_proc(dev, image_data, gx_default_image_data);
    CHECK_NON_DEFAULT(end_image, gx_default_end_image, "end_image");
    set_dev_proc(dev, end_image, gx_default_end_image);
#undef CHECK_NON_DEFAULT
    fill_dev_proc(dev, strip_tile_rectangle, gx_default_strip_tile_rectangle);
    fill_dev_proc(dev, strip_copy_rop, gx_default_strip_copy_rop);
    fill_dev_proc(dev, get_clipping_box, gx_default_get_clipping_box);
    fill_dev_proc(dev, begin_typed_image, gx_default_begin_typed_image);
    fill_dev_proc(dev, get_bits_rectangle, gx_default_get_bits_rectangle);
    fill_dev_proc(dev, map_color_rgb_alpha, gx_default_map_color_rgb_alpha);
    fill_dev_proc(dev, create_compositor, gx_default_create_compositor);
    fill_dev_proc(dev, get_hardware_params, gx_default_get_hardware_params);
    fill_dev_proc(dev, text_begin, gx_default_text_begin);
}

int
gx_default_open_device(gx_device * dev)
{
    return 0;
}

/* Get the initial matrix for a device with inverted Y. */
/* This includes essentially all printers and displays. */
void
gx_default_get_initial_matrix(gx_device * dev, register gs_matrix * pmat)
{
    pmat->xx = dev->HWResolution[0] / 72.0;	/* x_pixels_per_inch */
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = dev->HWResolution[1] / -72.0;	/* y_pixels_per_inch */
    /****** tx/y is WRONG for devices with ******/
    /****** arbitrary initial matrix ******/
    pmat->tx = 0;
    pmat->ty = dev->height;
}
/* Get the initial matrix for a device with upright Y. */
/* This includes just a few printers and window systems. */
void
gx_upright_get_initial_matrix(gx_device * dev, register gs_matrix * pmat)
{
    pmat->xx = dev->HWResolution[0] / 72.0;	/* x_pixels_per_inch */
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = dev->HWResolution[1] / 72.0;	/* y_pixels_per_inch */
    /****** tx/y is WRONG for devices with ******/
    /****** arbitrary initial matrix ******/
    pmat->tx = 0;
    pmat->ty = 0;
}

int
gx_default_sync_output(gx_device * dev)
{
    return 0;
}

int
gx_default_output_page(gx_device * dev, int num_copies, int flush)
{
    int code = dev_proc(dev, sync_output)(dev);

    if (code >= 0)
	code = gx_finish_output_page(dev, num_copies, flush);
    return code;
}

int
gx_default_close_device(gx_device * dev)
{
    return 0;
}

const gx_xfont_procs *
gx_default_get_xfont_procs(gx_device * dev)
{
    return NULL;
}

gx_device *
gx_default_get_xfont_device(gx_device * dev)
{
    return dev;
}

gx_device *
gx_default_get_page_device(gx_device * dev)
{
    return NULL;
}
gx_device *
gx_page_device_get_page_device(gx_device * dev)
{
    return dev;
}

int
gx_default_get_alpha_bits(gx_device * dev, graphics_object_type type)
{
    return (type == go_text ? dev->color_info.anti_alias.text_bits :
	    dev->color_info.anti_alias.graphics_bits);
}

int
gx_default_get_band(gx_device * dev, int y, int *band_start)
{
    return 0;
}

void
gx_default_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = 0;
    pbox->p.y = 0;
    pbox->q.x = int2fixed(dev->width);
    pbox->q.y = int2fixed(dev->height);
}
void
gx_get_largest_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = min_fixed;
    pbox->p.y = min_fixed;
    pbox->q.x = max_fixed;
    pbox->q.y = max_fixed;
}

int
gx_no_create_compositor(gx_device * dev, gx_device ** pcdev,
			const gs_composite_t * pcte,
			const gs_imager_state * pis, gs_memory_t * memory)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}
int
gx_default_create_compositor(gx_device * dev, gx_device ** pcdev,
			     const gs_composite_t * pcte,
			     const gs_imager_state * pis, gs_memory_t * memory)
{
    return pcte->type->procs.create_default_compositor
	(pcte, pcdev, dev, pis, memory);
}
int
gx_null_create_compositor(gx_device * dev, gx_device ** pcdev,
			  const gs_composite_t * pcte,
			  const gs_imager_state * pis, gs_memory_t * memory)
{
    *pcdev = dev;
    return 0;
}

/* ---------------- Default per-instance procedures ---------------- */

int
gx_default_install(gx_device * dev, gs_state * pgs)
{
    return 0;
}

int
gx_default_begin_page(gx_device * dev, gs_state * pgs)
{
    return 0;
}

int
gx_default_end_page(gx_device * dev, int reason, gs_state * pgs)
{
    return (reason != 2 ? 1 : 0);
}
