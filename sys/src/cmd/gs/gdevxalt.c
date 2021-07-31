/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevxalt.c */
/* Alternative X Windows drivers for help in driver debugging */
#include "gx.h"			/* for gx_bitmap; includes std.h */
#include "math_.h"
#include "memory_.h"
#include "x_.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gxdevice.h"
#include "gdevx.h"

extern gx_device_X gs_x11_device;

/* ---------------- Generic procedures ---------------- */

/* Forward declarations */
private gx_device *dev_target(P1(gx_device *));
private void get_target_info(P1(gx_device *));
private gx_color_index x_alt_map_color(P2(gx_device *, gx_color_index));

/* "Wrappers" for driver procedures */

private int
x_wrap_open(gx_device *dev)
{	gx_device *tdev = dev_target(dev);
	int code = (*dev_proc(tdev, open_device))(tdev);
	if ( code < 0 )
	  return code;
	tdev->is_open = true;
	get_target_info(dev);
	return code;
}

private int
x_forward_sync_output(gx_device *dev)
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, sync_output))(tdev);
}

private int
x_forward_output_page(gx_device *dev, int num_copies, int flush)
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, output_page))(tdev, num_copies, flush);
}

private int
x_wrap_close(gx_device *dev)
{	gx_device *tdev = dev_target(dev);
	/* If Ghostscript is exiting, we might have closed the */
	/* underlying x11 device already.... */
	int code;
	if ( tdev->is_open )
	  {	code = (*dev_proc(tdev, close_device))(tdev);
		if ( code < 0 )
		  return code;
		tdev->is_open = false;
	  }
	else
	  code = 0;
	return code;
}

private int
x_wrap_map_color_rgb(register gx_device *dev, gx_color_index color,
		gx_color_value prgb[3])
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, map_color_rgb))(tdev,
						x_alt_map_color(dev, color),
						prgb);
}

private int
x_wrap_fill_rectangle(register gx_device *dev,
		      int x, int y, int w, int h, gx_color_index color)
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, fill_rectangle))(tdev, x, y, w, h,
						 x_alt_map_color(dev, color));
}

private int
x_wrap_copy_mono(register gx_device *dev,
		 const byte *base, int sourcex, int raster, gx_bitmap_id id,
		 int x, int y, int w, int h,
		 gx_color_index zero, gx_color_index one)
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, copy_mono))(tdev, base, sourcex, raster, id,
					    x, y, w, h,
					    x_alt_map_color(dev, zero),
					    x_alt_map_color(dev, one));

}

private int
x_forward_copy_color(gx_device *dev, const byte *base, int sourcex,
		     int raster, gx_bitmap_id id, int x, int y, int w, int h)
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, copy_color))(tdev, base, sourcex, raster, id,
					     x, y, w, h);
}

private int
x_wrap_draw_line(register gx_device *dev,
		 int x0, int y0, int x1, int y1, gx_color_index color)
{	gx_device *tdev = dev_target(dev);
	return (*dev_proc(tdev, draw_line))(tdev, x0, y0, x1, y1,
					    x_alt_map_color(dev, color));

}

private int
x_wrap_get_params(gx_device *dev, gs_param_list *plist)
{	gx_device *tdev = dev_target(dev);
	/* We assume that a get_params call has no side effects.... */
	gx_device_X save_dev;
	int code;

	save_dev = *(gx_device_X *)tdev;
	if ( tdev->is_open )
	  tdev->color_info = dev->color_info;
	tdev->dname = dev->dname;
	code = (*dev_proc(tdev, get_params))(tdev, plist);
	*(gx_device_X *)tdev = save_dev;
	return code;
}

private int
x_wrap_put_params(gx_device *dev, gs_param_list *plist)
{	gx_device *tdev = dev_target(dev);
	int code = (*dev_proc(tdev, put_params))(tdev, plist);
	if ( code < 0 )
	  return code;
	get_target_info(dev);
	return code;
}

/* Internal procedures */

/* Get the target, creating it if necessary. */
private gx_device *
dev_target(gx_device *dev)
{	gx_device *tdev = ((gx_device_forward *)dev)->target;
	if ( tdev == 0 )
	  {	/* Create or link to an X device instance. */
		if ( dev->memory == 0 )	/* static instance */
		  tdev = (gx_device *)&gs_x11_device;
		else
		  {	tdev = (gx_device *)gs_alloc_bytes(dev->memory,
						(gs_x11_device).params_size,
						"dev_target");
			if ( tdev == 0 )
			  {	/* Punt. */
				exit(1);
			  }
			*(gx_device_X *)tdev = gs_x11_device;
			tdev->memory = dev->memory;
			tdev->is_open = false;
		  }
		gx_device_fill_in_procs(tdev);
		((gx_device_forward *)dev)->target = tdev;
	  }
	return tdev;
}

/* Copy parameters back from the target. */
private void
get_target_info(gx_device *dev)
{	gx_device *tdev = dev_target(dev);

#define copy(m) dev->m = tdev->m;
#define copy2(m) copy(m[0]); copy(m[1])
#define copy4(m) copy2(m); copy(m[2]); copy(m[3])

	copy(width); copy(height);
	copy2(PageSize);
	copy4(ImagingBBox);
	copy(ImagingBBox_set);
	copy2(HWResolution);
	copy2(Margins_HWResolution);
	copy2(Margins);
	copy4(HWMargins);
	if ( dev->color_info.num_components == 3 )
	  copy(color_info);

#undef copy4
#undef copy2
#undef copy
}

/* Map a fake CMYK or black/white color to a real X color if necessary. */
private gx_color_index
x_alt_map_color(gx_device *dev, gx_color_index color)
{	gx_device *tdev = dev_target(dev);
	gx_color_value r, g, b;

	if ( color == gx_no_color_index )
	  return color;
	switch ( dev->color_info.num_components )
	  {
	  case 3:	/* RGB, this is the real thing */
	    return color;
	  case 4:	/* CMYK */
	    if ( color & 1 )
	      r = g = b = 0;
	    else
	      { r = (color & 8 ? 0 : gx_max_color_value);
		g = (color & 4 ? 0 : gx_max_color_value);
		b = (color & 2 ? 0 : gx_max_color_value);
	      }
	    break;
	  default /*case 1*/:	/* black-and-white */
	    r = g = b = (color ? gx_max_color_value : 0);
	    break;
	  }
	return (*dev_proc(tdev, map_rgb_color))(tdev, r, g, b);
}

/* ---------------- CMYK procedures ---------------- */

/* Device procedures */
private dev_proc_map_rgb_color(x_cmyk_map_rgb_color);
private dev_proc_copy_color(x_cmyk_copy_color);
private dev_proc_map_cmyk_color(x_cmyk_map_cmyk_color);

/* The device descriptor */
private gx_device_procs x_cmyk_procs = {
	x_wrap_open,
	gx_forward_get_initial_matrix,
	x_forward_sync_output,
	x_forward_output_page,
	x_wrap_close,
	x_cmyk_map_rgb_color,
	x_wrap_map_color_rgb,
	x_wrap_fill_rectangle,
	gx_default_tile_rectangle,
	x_wrap_copy_mono,
	x_cmyk_copy_color,
	x_wrap_draw_line,
	gx_default_get_bits,
	x_wrap_get_params,
	x_wrap_put_params,
	x_cmyk_map_cmyk_color,
	gx_forward_get_xfont_procs,
	gx_forward_get_xfont_device,
	NULL,			/* map_rgb_alpha_color */
	gx_forward_get_page_device,
	gx_forward_get_alpha_bits,
	NULL			/* copy_alpha */
};

/* The instance is public. */
gx_device_forward gs_x11cmyk_device = {
	std_device_dci_body(gx_device_forward, &x_cmyk_procs, "x11cmyk",
	  FAKE_RES*85/10, FAKE_RES*11,	/* x and y extent (nominal) */
	  FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	  4, 4, 1, 1, 2, 2),
	{ 0 },			/* std_procs */
	0			/* target */
};

/* Device procedures */

private gx_color_index
x_cmyk_map_rgb_color(register gx_device *dev,
		     gx_color_value r, gx_color_value g, gx_color_value b)
{	return
	  (gx_color_index)
	    ((r | g | b) == 0 ? 1 :
	     ((~r >> (gx_color_value_bits - 4)) & 8) |
	     ((~g >> (gx_color_value_bits - 3)) & 4) |
	     ((~b >> (gx_color_value_bits - 2)) & 2));
}

private int
x_cmyk_copy_color(gx_device *dev, const byte *base, int sourcex,
		  int raster, gx_bitmap_id id, int x, int y, int w, int h)
{	gx_device *tdev = dev_target(dev);
	/* Do this the slow, painful way. */
	int xi, yi;
	const byte *row = base;

	for ( yi = y; yi < y + h; row += raster, ++yi )
	  {	for ( xi = x; xi < x + w; ++xi )
		  {	int sx = sourcex + xi - x;
			byte cmyk = row[sx >> 1];
			gx_color_index color =
			  x_alt_map_color(dev,
					 (sx & 1 ? cmyk & 0xf : cmyk >> 4));
			(*dev_proc(tdev, fill_rectangle))(tdev, xi, yi, 1, 1,
							  color);
		  }
	  }
	return 0;
}

private gx_color_index
x_cmyk_map_cmyk_color(register gx_device *dev,
		      gx_color_value c, gx_color_value m, gx_color_value y,
		      gx_color_value k)
{	return
	  (gx_color_index)
	    (((c >> (gx_color_value_bits - 4)) & 8) |
	     ((m >> (gx_color_value_bits - 3)) & 4) |
	     ((y >> (gx_color_value_bits - 2)) & 2) |
	     ((k >> (gx_color_value_bits - 1)) & 1));
}

/* ---------------- Black-and-white procedures ---------------- */

/* The device descriptor */
private gx_device_procs x_mono_procs = {
	x_wrap_open,
	gx_forward_get_initial_matrix,
	x_forward_sync_output,
	x_forward_output_page,
	x_wrap_close,
	gx_default_map_rgb_color,
	x_wrap_map_color_rgb,
	x_wrap_fill_rectangle,
	gx_default_tile_rectangle,
	x_wrap_copy_mono,
	gx_default_copy_color,
	x_wrap_draw_line,
	gx_default_get_bits,
	x_wrap_get_params,
	x_wrap_put_params,
	gx_default_map_cmyk_color,
	gx_forward_get_xfont_procs,
	gx_forward_get_xfont_device,
	NULL,			/* map_rgb_alpha_color */
	gx_forward_get_page_device,
	gx_forward_get_alpha_bits,
	NULL			/* copy_alpha */
};

/* The instance is public. */
gx_device_forward gs_x11mono_device = {
	std_device_dci_body(gx_device_forward, &x_mono_procs, "x11mono",
	  FAKE_RES*85/10, FAKE_RES*11,	/* x and y extent (nominal) */
	  FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	  1, 1, 1, 0, 2, 0),
	{ 0 },			/* std_procs */
	0			/* target */
};

/* ---------------- Alpha procedures ---------------- */

/* Device procedures */
private dev_proc_get_alpha_bits(x_alpha_get_alpha_bits);
private dev_proc_copy_alpha(x_alpha_copy_alpha);

/* The device descriptor */
private gx_device_procs x_alpha_procs = {
	x_wrap_open,
	gx_forward_get_initial_matrix,
	x_forward_sync_output,
	x_forward_output_page,
	x_wrap_close,
	gx_forward_map_rgb_color,
	gx_forward_map_color_rgb,
	x_wrap_fill_rectangle,
	gx_default_tile_rectangle,
	x_wrap_copy_mono,
	x_forward_copy_color,
	x_wrap_draw_line,
	gx_default_get_bits,
	gx_forward_get_params,
	x_wrap_put_params,
	gx_forward_map_cmyk_color,
	gx_forward_get_xfont_procs,
	gx_forward_get_xfont_device,
	NULL,			/* map_rgb_alpha_color */
	gx_forward_get_page_device,
	x_alpha_get_alpha_bits,
	x_alpha_copy_alpha
};

/* The instance is public. */
gx_device_forward gs_x11alpha_device = {
	std_device_color_body(gx_device_forward, &x_alpha_procs, "x11alpha",
	  (int)(8.5*FAKE_RES), (int)(11*FAKE_RES),	/* x and y extent (nominal) */
	  FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	  24, 255, 255),
	{ 0 },			/* std_procs */
	0			/* target */
};

/* Device procedures */

private int
x_alpha_get_alpha_bits(gx_device *dev, graphics_object_type type)
{	return 4;
}

private int
x_alpha_copy_alpha(gx_device *dev, const unsigned char *base, int sourcex,
		   int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gx_color_index color, int depth)
{	gx_device *tdev = dev_target(dev);
	int xi, yi;
	const byte *row = base;
	/* We fake alpha by interpreting it as saturation, i.e., */
	/* alpha = 0 is white, alpha = 15/15 is the full color. */
	gx_color_value rgb[3];
	gx_color_index shades[16];
	int i;

	(*dev_proc(tdev, map_color_rgb))(tdev, color, rgb);
	for ( i = 0; i < 15; ++i )
	  shades[i] = gx_no_color_index;
	shades[15] = color;
	/* Do the copy operation pixel-by-pixel. */
	for ( yi = y; yi < y + h; row += raster, ++yi )
	  {	for ( xi = x; xi < x + w; ++xi )
		  {	int sx = sourcex + xi - x;
			uint alpha2 = row[sx >> 1];
			uint alpha = (sx & 1 ? alpha2 & 0xf : alpha2 >> 4);
			gx_color_index a_color;

			if ( alpha == 0 )
			  continue;
			while ( (a_color = shades[alpha]) == gx_no_color_index )
			  {	/* Map the color now. */
#define make_shade(v, alpha)\
  (gx_max_color_value -\
   ((gx_max_color_value - (v)) * (alpha) / 15))
				gx_color_index r = make_shade(rgb[0], alpha);
				gx_color_index g = make_shade(rgb[1], alpha);
				gx_color_index b = make_shade(rgb[1], alpha);
#undef make_shade
				a_color = (*dev_proc(tdev, map_rgb_color))(tdev, r, g, b);
				if ( a_color != gx_no_color_index )
				  {	shades[alpha] = a_color;
					break;
				  }
				/* Try a higher saturation.  (We know */
				/* the fully saturated color exists.) */
				alpha += (16 - alpha) >> 1;
			  }
			(*dev_proc(tdev, fill_rectangle))(tdev, xi, yi, 1, 1,
							  a_color);
		  }
	  }
	return 0;
}
