/* Copyright (C) 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevalph.c,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Alpha-channel storage device */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxgetbit.h"

/*
 * Very few devices (actually, none right now) actually store an alpha
 * channel.  Here we provide a device that acts as a "wrapper" around
 * another device, and adds alpha channel storage.
 */

/* Define the device. */
typedef struct gx_device_store_alpha_s {
    gx_device_forward_common;
    bool open_close_target;	/* if true, open/close target when opening/closing */
    int alpha_depth;		/* # of bits of stored alpha */
    gx_device_memory *adev;	/* stores the alpha values -- created when needed */
} gx_device_store_alpha;

gs_private_st_suffix_add1_final(st_device_store_alpha,
			     gx_device_store_alpha, "gx_device_store_alpha",
		device_store_alpha_enum_ptrs, device_store_alpha_reloc_ptrs,
				gx_device_finalize, st_device_forward, adev);
/* The device descriptor. */
private dev_proc_open_device(dsa_open);
private dev_proc_close_device(dsa_close);
private dev_proc_fill_rectangle(dsa_fill_rectangle);
private dev_proc_map_rgb_color(dsa_map_rgb_color);
private dev_proc_map_color_rgb(dsa_map_color_rgb);
private dev_proc_copy_mono(dsa_copy_mono);
private dev_proc_copy_color(dsa_copy_color);
private dev_proc_put_params(dsa_put_params);
private dev_proc_map_rgb_alpha_color(dsa_map_rgb_alpha_color);
private dev_proc_map_color_rgb_alpha(dsa_map_color_rgb_alpha);
private dev_proc_copy_alpha(dsa_copy_alpha);
private dev_proc_fill_path(dsa_fill_path);
private dev_proc_stroke_path(dsa_stroke_path);
private dev_proc_get_bits_rectangle(dsa_get_bits_rectangle);
private const gx_device_store_alpha gs_store_alpha_device =
{std_device_std_body_open(gx_device_store_alpha, 0,
			  "alpha storage", 0, 0, 1, 1),
 {dsa_open,
  gx_forward_get_initial_matrix,
  gx_forward_sync_output,
  gx_forward_output_page,
  dsa_close,
  dsa_map_rgb_color,
  dsa_map_color_rgb,
  dsa_fill_rectangle,
  gx_default_tile_rectangle,
  dsa_copy_mono,
  dsa_copy_color,
  gx_default_draw_line,
  gx_default_get_bits,
  gx_forward_get_params,
  dsa_put_params,
  gx_default_cmyk_map_cmyk_color,	/* only called for CMYK */
  gx_forward_get_xfont_procs,
  gx_forward_get_xfont_device,
  dsa_map_rgb_alpha_color,
  gx_forward_get_page_device,
  gx_forward_get_alpha_bits,
  dsa_copy_alpha,
  gx_forward_get_band,
  gx_default_copy_rop,
  dsa_fill_path,
  dsa_stroke_path,
		/*
		 * We should do the same thing for the "mid-level"
		 * drawing operations that we do for fill_path and
		 * stroke_path, but we don't bother to do this yet.
		 */
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
  dsa_get_bits_rectangle,
  dsa_map_color_rgb_alpha,
  gx_default_create_compositor
 }
};

/* Forward references */
private int alloc_alpha_storage(P1(gx_device_store_alpha * sadev));

#define sadev ((gx_device_store_alpha *)dev)

/* ------ Open/close ------ */

private int
dsa_open(gx_device * dev)
{
    gx_device *tdev = sadev->target;
    int max_value =
    max(tdev->color_info.max_gray, tdev->color_info.max_color);
    int adepth = (max_value <= 3 ? 2 : max_value <= 15 ? 4 : 8);
    int depth = tdev->color_info.depth + adepth;
    int code;

    if (depth > 32)
	return_error(gs_error_rangecheck);
    code =
	(sadev->open_close_target ?
	 (*dev_proc(tdev, open_device)) (tdev) : 0);
    if (code < 0)
	return code;
    sadev->width = tdev->width;
    sadev->height = tdev->height;
    sadev->color_info = tdev->color_info;
    sadev->color_info.depth = (depth <= 4 ? 4 : ROUND_UP(depth, 8));
    sadev->alpha_depth = adepth;
    sadev->adev = 0;
    return 0;
}

private int
dsa_close(gx_device * dev)
{
    gx_device *adev = (gx_device *) sadev->adev;

    if (adev) {
	(*dev_proc(adev, close_device)) (adev);
	gs_free_object(dev->memory, adev, "dsa_close(adev)");
	sadev->adev = 0;
    }
    if (sadev->open_close_target) {
	gx_device *tdev = sadev->target;

	return (*dev_proc(tdev, close_device)) (tdev);
    }
    return 0;
}

/* ------ Color mapping ------ */

private gx_color_index
dsa_map_rgb_color(gx_device * dev,
	      gx_color_value red, gx_color_value green, gx_color_value blue)
{
    return dsa_map_rgb_alpha_color(dev, red, green, blue,
				   gx_max_color_value);
}

private int
dsa_map_color_rgb(gx_device * dev,
		  gx_color_index color, gx_color_value rgb[3])
{
    gx_device *tdev = sadev->target;

    return (*dev_proc(tdev, map_color_rgb))
	(tdev, color >> sadev->alpha_depth, rgb);
}

private gx_color_index
dsa_map_rgb_alpha_color(gx_device * dev,
	      gx_color_value red, gx_color_value green, gx_color_value blue,
			gx_color_value alpha)
{
    gx_device *tdev = sadev->target;
    int adepth = sadev->alpha_depth;

    return
	((*dev_proc(tdev, map_rgb_color)) (tdev, red, green, blue)
	 << adepth) + (alpha * ((1 << adepth) - 1) / gx_max_color_value);
}

private int
dsa_map_color_rgb_alpha(gx_device * dev,
			gx_color_index color, gx_color_value rgba[4])
{
    int code = dsa_map_color_rgb(dev, color, rgba);
    int amask = (1 << sadev->alpha_depth) - 1;

    if (code < 0)
	return code;
    rgba[3] = (color & amask) * gx_max_color_value / amask;
    return code;
}

/* ------ Low-level drawing ------ */

private int
dsa_fill_rectangle(gx_device * dev,
		   int x, int y, int width, int height, gx_color_index color)
{
    gx_device *tdev = sadev->target;
    gx_device *adev = (gx_device *) sadev->adev;
    int adepth = sadev->alpha_depth;
    uint amask = (1 << adepth) - 1;

    if ((~color & amask) && !adev) {
	int code = alloc_alpha_storage(sadev);

	if (code < 0)
	    return code;
	adev = (gx_device *) sadev->adev;
    }
    if (adev)
	(*dev_proc(adev, fill_rectangle))
	    (adev, x, y, width, height, color & amask);
    return (*dev_proc(tdev, fill_rectangle))
	(tdev, x, y, width, height, color >> adepth);
}

private int
dsa_copy_mono(gx_device * dev,
	      const byte * data, int data_x, int raster, gx_bitmap_id id,
	      int x, int y, int width, int height,
	      gx_color_index color0, gx_color_index color1)
{
    gx_device *tdev = sadev->target;
    gx_device *adev = (gx_device *) sadev->adev;
    int adepth = sadev->alpha_depth;
    uint amask = (1 << adepth) - 1;

    if ((((color0 != gx_no_color_index) && (~color0 & amask)) ||
	 ((color1 != gx_no_color_index) && (~color1 & amask))) && !adev
	) {
	int code = alloc_alpha_storage(sadev);

	if (code < 0)
	    return code;
	adev = (gx_device *) sadev->adev;
    }
    if (adev)
	(*dev_proc(adev, copy_mono))
	    (adev, data, data_x, raster, id, x, y, width, height,
	     (color0 == gx_no_color_index ? color0 : color0 & amask),
	     (color1 == gx_no_color_index ? color1 : color1 & amask));
    return (*dev_proc(tdev, copy_mono))
	(tdev, data, data_x, raster, id, x, y, width, height,
	 (color0 == gx_no_color_index ? color0 : color0 >> adepth),
	 (color1 == gx_no_color_index ? color1 : color1 >> adepth));
}

private int
dsa_copy_color(gx_device * dev,
	       const byte * data, int data_x, int raster, gx_bitmap_id id,
	       int x, int y, int width, int height)
{
    int sdepth = dev->color_info.depth;
    gx_device *tdev = sadev->target;
    int tdepth = tdev->color_info.depth;
    gx_device *adev = (gx_device *) sadev->adev;
    int adepth = sadev->alpha_depth;	/* = adev->color_info.depth */
    uint amask = (1 << adepth) - 1;
    gs_memory_t *mem = dev->memory;
    byte *color_row;
    byte *alpha_row;
    int code = 0;
    int xbit, yi;

    fit_copy(dev, data, data_x, raster, id, x, y, width, height);
    color_row =
	gs_alloc_bytes(mem, bitmap_raster(width * tdepth),
		       "dsa_copy_color(color)");
    /*
     * If we aren't already storing alpha, alpha_row remains 0 until we
     * encounter a source value with a non-unity alpha.
     */
    alpha_row =
	(adev ?
	 gs_alloc_bytes(mem, bitmap_raster(width * adepth),
			"dsa_copy_color(color)") : 0);
    if (color_row == 0 || (adev && alpha_row == 0)) {
	code = gs_note_error(gs_error_VMerror);
	goto out;
    }
    xbit = data_x * dev->color_info.depth;
    for (yi = y; yi < y + height; ++yi) {
	const byte *source_row = data + (yi - y) * raster;

	sample_load_declare_setup(sptr, sbit, source_row + (xbit >> 3),
				  xbit & 7, sdepth);
	sample_store_declare_setup(dcptr, dcbit, dcbyte,
				   color_row, 0, tdepth);
	sample_store_declare_setup(daptr, dabit, dabyte,
				   alpha_row, 0, adepth);
	int xi;

	for (xi = 0; xi < width; ++xi) {
	    gx_color_index source;

	    sample_load_next32(source, sptr, sbit, sdepth);
	    {
		gx_color_index dest_color = source >> adepth;

		sample_store_next32(dest_color, dcptr, dcbit, tdepth, dcbyte);
	    }
	    {
		uint dest_alpha = source & amask;

		if (dest_alpha != amask && !alpha_row) {
		    /*
		     * Create an alpha row (and, if necessary, alpha storage).
		     */
		    int axbit = (xi - x) * adepth;

		    if (!adev) {
			code = alloc_alpha_storage(sadev);
			if (code < 0)
			    return code;
			adev = (gx_device *) sadev->adev;
		    }
		    alpha_row =
			gs_alloc_bytes(mem, bitmap_raster(width * adepth),
				       "dsa_copy_color(color)");
		    if (alpha_row == 0) {
			code = gs_note_error(gs_error_VMerror);
			goto out;
		    }
		    /*
		     * All the alpha values up to this point are unity, so we
		     * can fill them into the alpha row quickly.
		     */
		    memset(alpha_row, 0xff, (axbit + 7) >> 3);
		    daptr = alpha_row + (axbit >> 3);
		    sample_store_setup(dabit, axbit & 7, adepth);
		    sample_store_preload(dabyte, daptr, dabit, adepth);
		}
		if (alpha_row)
		    sample_store_next12(dest_alpha, daptr, dabit, adepth, dabyte);
	    }
	}
	if (alpha_row) {
	    sample_store_flush(daptr, dabit, adepth, dabyte);
	    (*dev_proc(adev, copy_color))
		(adev, alpha_row, 0, 0 /* irrelevant */ , gx_no_bitmap_id,
		 x, yi, width, 1);
	}
	sample_store_flush(dcptr, dcbit, adepth, dcbyte);
	(*dev_proc(tdev, copy_color))
	    (tdev, color_row, 0, 0 /* irrelevant */ , gx_no_bitmap_id,
	     x, yi, width, 1);
    }
  out:gs_free_object(mem, alpha_row, "dsa_copy_color(alpha)");
    gs_free_object(mem, color_row, "dsa_copy_color(color)");
    return code;
}

private int
dsa_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
	       gx_color_index color, int depth)
{
    gx_device *tdev = sadev->target;
    gx_device *adev = (gx_device *) sadev->adev;
    int adepth = sadev->alpha_depth;
    uint amask = (1 << adepth) - 1;

    if ((~color & amask) && !adev) {
	int code = alloc_alpha_storage(sadev);

	if (code < 0)
	    return code;
	adev = (gx_device *) sadev->adev;
    }
    if (adev)
	(*dev_proc(adev, copy_alpha))
	    (adev, data, data_x, raster, id, x, y, width, height,
	     color & amask, depth);
    return (*dev_proc(tdev, copy_alpha))
	(tdev, data, data_x, raster, id, x, y, width, height,
	 color >> adepth, depth);
}

/* ------ High-level drawing ------ */

private int
dsa_fill_path(gx_device * dev, const gs_imager_state * pis, gx_path * ppath,
	    const gx_fill_params * params, const gx_drawing_color * pdcolor,
	      const gx_clip_path * pcpath)
{
    if (!color_is_pure(pdcolor))
	return gx_default_fill_path(dev, pis, ppath, params, pdcolor,
				    pcpath);

    {
	gx_color_index color = gx_dc_pure_color(pdcolor);
	gx_drawing_color dcolor;
	gx_device *tdev = sadev->target;
	gx_device *adev = (gx_device *) sadev->adev;
	int adepth = sadev->alpha_depth;
	uint amask = (1 << adepth) - 1;
	int code;

	if ((~color & amask) && !adev) {
	    code = alloc_alpha_storage(sadev);
	    if (code < 0)
		return code;
	    adev = (gx_device *) sadev->adev;
	}
	if (adev) {
	    color_set_pure(&dcolor, color & amask);
	    code = (*dev_proc(adev, fill_path))
		(adev, pis, ppath, params, &dcolor, pcpath);
	    if (code < 0)
		return code;
	}
	color_set_pure(&dcolor, color >> adepth);
	return (*dev_proc(tdev, fill_path))
	    (tdev, pis, ppath, params, &dcolor, pcpath);
    }
}

private int
dsa_stroke_path(gx_device * dev, const gs_imager_state * pis, gx_path * ppath,
	  const gx_stroke_params * params, const gx_drawing_color * pdcolor,
		const gx_clip_path * pcpath)
{
    if (!color_is_pure(pdcolor))
	return gx_default_stroke_path(dev, pis, ppath, params, pdcolor,
				      pcpath);

    {
	gx_color_index color = gx_dc_pure_color(pdcolor);
	gx_drawing_color dcolor;
	gx_device *tdev = sadev->target;
	gx_device *adev = (gx_device *) sadev->adev;
	int adepth = sadev->alpha_depth;
	uint amask = (1 << adepth) - 1;
	int code;

	if ((~color & amask) && !adev) {
	    code = alloc_alpha_storage(sadev);
	    if (code < 0)
		return code;
	    adev = (gx_device *) sadev->adev;
	}
	if (adev) {
	    color_set_pure(&dcolor, color & amask);
	    code = (*dev_proc(adev, stroke_path))
		(adev, pis, ppath, params, &dcolor, pcpath);
	    if (code < 0)
		return code;
	}
	color_set_pure(&dcolor, color >> adepth);
	return (*dev_proc(tdev, stroke_path))
	    (tdev, pis, ppath, params, &dcolor, pcpath);
    }
}

/* ------ Reading bits ------ */

private int
dsa_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    int ddepth = dev->color_info.depth;
    gx_device *tdev = sadev->target;
    int tdepth = tdev->color_info.depth;
    gx_device *adev = (gx_device *) sadev->adev;
    int adepth = sadev->alpha_depth;	/* = adev->color_info.depth */
    uint alpha = (1 << adepth) - 1;
    gs_get_bits_options_t options = params->options;
    int x = prect->p.x, w = prect->q.x - x, y = prect->p.y, h = prect->q.y - y;
    int yi;
    uint raster =
    (options & gb_raster_standard ? bitmap_raster(w * ddepth) :
     params->raster);

    if ((~options &
	 (gb_colors_native | gb_alpha_last | gb_packing_chunky |
	  gb_return_copy | gb_offset_0)) ||
	!(options & (gb_raster_standard | gb_raster_specified))
	) {
	if (options == 0) {
	    params->options =
		gb_colors_native | gb_alpha_last | gb_packing_chunky |
		gb_return_copy | gb_offset_0 |
		(gb_raster_standard | gb_raster_specified);
	    return_error(gs_error_rangecheck);
	}
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
    for (yi = y; yi < y + h; ++yi) {
	sample_load_declare(scptr, scbit);
	sample_load_declare(saptr, sabit);
	sample_store_declare(dptr, dbit, dbbyte);
	byte *data = params->data[0] + (yi - y) * raster;
	gs_get_bits_params_t color_params;
	gs_int_rect rect;
	int code, xi;

	rect.p.x = x, rect.p.y = yi;
	rect.q.x = x + w, rect.q.y = yi + 1;
	color_params.options =
	    gb_colors_native | gb_alpha_none | gb_packing_chunky |
	    (gb_return_copy | gb_return_pointer) | gb_offset_specified |
	    gb_raster_all /*irrelevant */  | gb_align_all /*irrelevant */ ;
	color_params.x_offset = w;
	code = (*dev_proc(tdev, get_bits_rectangle))
	    (tdev, &rect, &color_params, NULL);
	if (code < 0)
	    return code;
	dptr = data;
	sample_store_setup(dbit, 0, ddepth);
	sample_store_preload(dbbyte, dptr, dbit, ddepth);
	{
	    int cxbit = (color_params.data[0] == data ? w * tdepth : 0);

	    scptr = data + (cxbit >> 3);
	    sample_load_setup(scbit, cxbit & 7, tdepth);
	}
	if (adev) {
	    int axbit = x * adepth;

	    saptr = data + (axbit >> 3);
	    sample_load_setup(sabit, axbit & 7, adepth);
	}
	for (xi = 0; xi < w; ++xi) {
	    gx_color_index colors;

	    sample_load_next32(colors, scptr, scbit, tdepth);
	    if (adev)
		sample_load_next12(alpha, saptr, sabit, adepth);
	    colors = (colors << adepth) + alpha;
	    sample_store_next32(colors, dptr, dbit, ddepth, dbbyte);
	}
	sample_store_flush(dptr, dbit, ddepth, dbbyte);
    }
    return 0;
}

/* ------ Setting parameters ------ */

private int
dsa_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device *tdev = sadev->target;
    int code = (*dev_proc(tdev, put_params)) (tdev, plist);

    if (code < 0)
	return code;
    gx_device_copy_params(dev, tdev);
    return code;
}

#undef sadev

/* ------ Internal utilities ------ */

/* Allocate the alpha storage memory device. */
private int
alloc_alpha_storage(gx_device_store_alpha * sadev)
{
    int adepth = sadev->alpha_depth;
    const gx_device_memory *mdproto = gdev_mem_device_for_bits(adepth);
    gs_memory_t *mem = sadev->memory;
    gx_device_memory *mdev =
    gs_alloc_struct(mem, gx_device_memory,
		    &st_device_memory, "alloc_alpha_storage(adev)");
    int code;

    if (mdev == 0)
	return_error(gs_error_VMerror);
    gs_make_mem_device(mdev, mdproto, mem, 0, NULL);
    mdev->width = sadev->width;
    mdev->height = sadev->height;
    mdev->bitmap_memory = mem;
    code = (*dev_proc(mdev, open_device)) ((gx_device *) mdev);
    if (code < 0) {
	gs_free_object(mem, mdev, "alloc_alpha_storage(adev)");
	return_error(gs_error_VMerror);
    }
    sadev->adev = mdev;
    return (*dev_proc(mdev, fill_rectangle))
	((gx_device *) mdev, 0, 0, mdev->width, mdev->height,
	 (gx_color_index) ((1 << adepth) - 1));
}
