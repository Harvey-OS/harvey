/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevrops.c,v 1.12 2004/06/24 05:03:36 dan Exp $ */
/* RasterOp source device */
#include "gx.h"
#include "gserrors.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gdevmrop.h"

/* GC procedures */
private_st_device_rop_texture();
private ENUM_PTRS_BEGIN(device_rop_texture_enum_ptrs) {
    if (index < st_device_color_max_ptrs) {
	gs_ptr_type_t ptype =
	    ENUM_SUPER_ELT(gx_device_rop_texture, st_device_color, texture, 0);

	if (ptype)
	    return ptype;
	return ENUM_OBJ(NULL);	/* don't stop early */
    }
    ENUM_PREFIX(st_device_forward, st_device_color_max_ptrs);
} ENUM_PTRS_END
private RELOC_PTRS_BEGIN(device_rop_texture_reloc_ptrs) {
    RELOC_PREFIX(st_device_forward);
    RELOC_SUPER(gx_device_rop_texture, st_device_color, texture);
} RELOC_PTRS_END

/* Device for providing source data for RasterOp. */
private dev_proc_fill_rectangle(rop_texture_fill_rectangle);
private dev_proc_copy_mono(rop_texture_copy_mono);
private dev_proc_copy_color(rop_texture_copy_color);

/* The device descriptor. */
private const gx_device_rop_texture gs_rop_texture_device = {
    std_device_std_body(gx_device_rop_texture, 0, "rop source",
			0, 0, 1, 1),
    {NULL,				/* open_device */
     gx_forward_get_initial_matrix,
     NULL,				/* default_sync_output */
     NULL,				/* output_page */
     NULL,				/* close_device */
     gx_forward_map_rgb_color,
     gx_forward_map_color_rgb,
     rop_texture_fill_rectangle,
     NULL,				/* tile_rectangle */
     rop_texture_copy_mono,
     rop_texture_copy_color,
     NULL,				/* draw_line */
     NULL,				/* get_bits */
     gx_forward_get_params,
     gx_forward_put_params,
     gx_forward_map_cmyk_color,
     gx_forward_get_xfont_procs,
     gx_forward_get_xfont_device,
     gx_forward_map_rgb_alpha_color,
     gx_forward_get_page_device,
     NULL,				/* get_alpha_bits (no alpha) */
     gx_no_copy_alpha,		/* shouldn't be called */
     gx_forward_get_band,
     gx_no_copy_rop,		/* shouldn't be called */
     NULL,				/* fill_path */
     NULL,				/* stroke_path */
     NULL,				/* fill_mask */
     NULL,				/* fill_trapezoid */
     NULL,				/* fill_parallelogram */
     NULL,				/* fill_triangle */
     NULL,				/* draw_thin_line */
     NULL,				/* begin_image */
     NULL,				/* image_data */
     NULL,				/* end_image */
     NULL,				/* strip_tile_rectangle */
     NULL,				/* strip_copy_rop */
     gx_forward_get_clipping_box,
     NULL,				/* begin_typed_image */
     NULL,				/* get_bits_rectangle */
     gx_forward_map_color_rgb_alpha,
     NULL,				/* create_compositor */
     gx_forward_get_hardware_params,
     NULL,				/* text_begin */
     NULL,				/* finish_copydevice */
     NULL,				/* begin_transparency_group */
     NULL,				/* end_transparency_group */
     NULL,				/* begin_transparency_mask */
     NULL,				/* end_transparency_mask */
     NULL,				/* discard_transparency_layer */
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
    },
    0,				/* target */
    lop_default			/* log_op */
    /* */				/* texture */
};

/* Create a RasterOp source device. */
int
gx_alloc_rop_texture_device(gx_device_rop_texture ** prsdev, gs_memory_t * mem,
			    client_name_t cname)
{
    *prsdev = gs_alloc_struct(mem, gx_device_rop_texture,
			      &st_device_rop_texture, cname);
    return (*prsdev == 0 ? gs_note_error(gs_error_VMerror) : 0);
}

/* Initialize a RasterOp source device. */
void
gx_make_rop_texture_device(gx_device_rop_texture * dev, gx_device * target,
	     gs_logical_operation_t log_op, const gx_device_color * texture)
{
    gx_device_init((gx_device *) dev,
		   (const gx_device *)&gs_rop_texture_device,
		   NULL, true);
    gx_device_set_target((gx_device_forward *)dev, target);
    /* Drawing operations are defaulted, non-drawing are forwarded. */
    check_device_separable((gx_device *) dev);
    gx_device_fill_in_procs((gx_device *) dev);
    gx_device_copy_params((gx_device *)dev, target);
    dev->log_op = log_op;
    dev->texture = *texture;
}

/* Fill a rectangle */
private int
rop_texture_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			   gx_color_index color)
{
    gx_device_rop_texture *const rtdev = (gx_device_rop_texture *)dev;
    gx_rop_source_t source;

    source.sdata = NULL;
    source.sourcex = 0;
    source.sraster = 0;
    source.id = gx_no_bitmap_id;
    source.scolors[0] = source.scolors[1] = color;
    source.use_scolors = true;
    return gx_device_color_fill_rectangle(&rtdev->texture,
					  x, y, w, h, rtdev->target,
					  rtdev->log_op, &source);
}

/* Copy a monochrome rectangle */
private int
rop_texture_copy_mono(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		      int x, int y, int w, int h,
		      gx_color_index color0, gx_color_index color1)
{
    gx_device_rop_texture *const rtdev = (gx_device_rop_texture *)dev;
    gx_rop_source_t source;
    gs_logical_operation_t lop = rtdev->log_op;

    source.sdata = data;
    source.sourcex = sourcex;
    source.sraster = raster;
    source.id = id;
    source.scolors[0] = color0;
    source.scolors[1] = color1;
    source.use_scolors = true;
    /* Adjust the logical operation per transparent colors. */
    if (color0 == gx_no_color_index)
	lop = rop3_use_D_when_S_0(lop);
    else if (color1 == gx_no_color_index)
	lop = rop3_use_D_when_S_1(lop);
    return gx_device_color_fill_rectangle(&rtdev->texture,
					  x, y, w, h, rtdev->target,
					  lop, &source);
}

/* Copy a color rectangle */
private int
rop_texture_copy_color(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		       int x, int y, int w, int h)
{
    gx_device_rop_texture *const rtdev = (gx_device_rop_texture *)dev;
    gx_rop_source_t source;

    source.sdata = data;
    source.sourcex = sourcex;
    source.sraster = raster;
    source.id = id;
    source.scolors[0] = source.scolors[1] = gx_no_color_index;
    source.use_scolors = false;
    return gx_device_color_fill_rectangle(&rtdev->texture,
					  x, y, w, h, rtdev->target,
					  rtdev->log_op, &source);
}
