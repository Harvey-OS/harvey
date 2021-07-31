/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevhit.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Hit detection device */
#include "std.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gxdevice.h"

/* Define the value returned for a detected hit. */
const int gs_hit_detected = gs_error_hit_detected;

/*
 * Define a minimal device for insideness testing.
 * It returns e_hit whenever it is asked to actually paint any pixels.
 */
private dev_proc_fill_rectangle(hit_fill_rectangle);
const gx_device gs_hit_device = {
 std_device_std_body(gx_device, 0, "hit detector",
		     0, 0, 1, 1),
 {NULL,				/* open_device */
  NULL,				/* get_initial_matrix */
  NULL,				/* sync_output */
  NULL,				/* output_page */
  NULL,				/* close_device */
  gx_default_map_rgb_color,
  gx_default_map_color_rgb,
  hit_fill_rectangle,
  NULL,				/* tile_rectangle */
  NULL,				/* copy_mono */
  NULL,				/* copy_color */
  gx_default_draw_line,
  NULL,				/* get_bits */
  NULL,				/* get_params */
  NULL,				/* put_params */
  gx_default_map_cmyk_color,
  NULL,				/* get_xfont_procs */
  NULL,				/* get_xfont_device */
  gx_default_map_rgb_alpha_color,
  gx_default_get_page_device,
  gx_default_get_alpha_bits,
  NULL,				/* copy_alpha */
  gx_default_get_band,
  NULL,				/* copy_rop */
  gx_default_fill_path,
  NULL,				/* stroke_path */
  NULL,				/* fill_mask */
  gx_default_fill_trapezoid,
  gx_default_fill_parallelogram,
  gx_default_fill_triangle,
  gx_default_draw_thin_line,
  gx_default_begin_image,
  gx_default_image_data,
  gx_default_end_image,
  gx_default_strip_tile_rectangle,
  gx_default_strip_copy_rop,
  gx_get_largest_clipping_box,
  gx_default_begin_typed_image,
  NULL,				/* get_bits_rectangle */
  gx_default_map_color_rgb_alpha,
  gx_non_imaging_create_compositor,
  NULL				/* get_hardware_params */
 }
};

/* Test for a hit when filling a rectangle. */
private int
hit_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		   gx_color_index color)
{
    if (w > 0 && h > 0)
	return_error(gs_error_hit_detected);
    return 0;
}
