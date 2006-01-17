/* Copyright (C) 1992, 1993, 1994, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevvglb.c,v 1.5 2002/02/21 22:24:52 giles Exp $ */
/*
 * This is a driver for 386 PCs using vgalib for graphics on the console
 * display.  Note that this driver only works with 16-color modes.
 *
 * Written by Sigfrid Lundberg, siglun@euler.teorekol.lu.se.
 * Modified by Erik Talvola, talvola@gnu.ai.mit.edu
 * Updated 9/28/96 by L. Peter Deutsch, ghost@aladdin.com: allow setting
 *   the display mode as a device parameter.
 * Updated 2/13/97 by ghost@aladdin.com: make the device identify itself
 *   as a page device.
 * Updated 5/2/97 by ghost@aladdin.com: copy_mono computed some parameters
 *   before doing fit_copy.
 * Update 1997-06-28 by ghost@aladdin.com: get_bits wasn't implemented.
 */

#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gxdevice.h"
#include "gdevpccm.h"

#include <errno.h>
#include <vga.h>

typedef struct gx_device_vgalib {
    gx_device_common;
    int display_mode;
} gx_device_vgalib;

#define vga_dev ((gx_device_vgalib *)dev)

#define XDPI   60		/* to get a more-or-less square aspect ratio */
#define YDPI   60

#ifndef A4			/*Letter size */
#define YSIZE (20.0 * YDPI / 2.5)
#define XSIZE (8.5 / 11)*YSIZE	/* 8.5 x 11 inch page, by default */
#else /* A4 paper */
#define XSIZE 8.27
#define YSIZE 11.69
#endif

private dev_proc_open_device(vgalib_open);
private dev_proc_close_device(vgalib_close);
private dev_proc_map_rgb_color(vgalib_map_rgb_color);
private dev_proc_map_color_rgb(vgalib_map_color_rgb);
private dev_proc_fill_rectangle(vgalib_fill_rectangle);
private dev_proc_tile_rectangle(vgalib_tile_rectangle);
private dev_proc_copy_mono(vgalib_copy_mono);
private dev_proc_copy_color(vgalib_copy_color);
private dev_proc_get_bits(vgalib_get_bits);
private dev_proc_get_params(vgalib_get_params);
private dev_proc_put_params(vgalib_put_params);

const gx_device_vgalib gs_vgalib_device =
{
    std_device_std_body(gx_device_vgalib, 0, "vgalib",
			0, 0, 1, 1),
    {vgalib_open,
     NULL,			/* get_initial_matrix */
     NULL,			/* sync_output */
     NULL,			/* output_page */
     vgalib_close,
     vgalib_map_rgb_color,
     vgalib_map_color_rgb,
     vgalib_fill_rectangle,
     vgalib_tile_rectangle,
     vgalib_copy_mono,
     vgalib_copy_color,
     NULL,			/* draw_line (obsolete) */
     vgalib_get_bits,
     vgalib_get_params,
     vgalib_put_params,
     NULL,			/* map_cmyk_color */
     NULL,			/* get_xfont_procs */
     NULL,			/* get_xfont_device */
     NULL,			/* map_rgb_alpha_color */
     gx_page_device_get_page_device
    },
    -1				/* display_mode */
};

private int
vgalib_open(gx_device * dev)
{
    int VGAMODE = vga_dev->display_mode;
    int width = dev->width, height = dev->height;

    if (VGAMODE == -1)
	VGAMODE = vga_getdefaultmode();
    if (VGAMODE == -1)
	vga_setmode(G640x480x16);
    else
	vga_setmode(VGAMODE);
    vga_clear();
    if (width == 0)
	width = vga_getxdim() + 1;
    if (height == 0)
	height = vga_getydim() + 1;

    /*vgalib provides no facilities for finding out aspect ratios */
    if (dev->y_pixels_per_inch == 1) {
	dev->y_pixels_per_inch = height / 11.0;
	dev->x_pixels_per_inch = dev->y_pixels_per_inch;
    }
    gx_device_set_width_height(dev, width, height);

    /* Find out if the device supports color */
    /* (default initialization is monochrome). */
    /* We only recognize 16-color devices right now. */
    if (vga_getcolors() > 1) {
	int index;

	static const gx_device_color_info vgalib_16color = dci_pc_4bit;

	dev->color_info = vgalib_16color;

	for (index = 0; index < 16; ++index) {
	    gx_color_value rgb[3];

	    (*dev_proc(dev, map_color_rgb)) (dev, (gx_color_index) index, rgb);
#define cv2pv(cv) ((cv) >> (gx_color_value_bits - 8))
	    vga_setpalette(index, cv2pv(rgb[0]), cv2pv(rgb[1]), cv2pv(rgb[2]));
#undef cv2pv
	}
    }
    return 0;
}

private int
vgalib_close(gx_device * dev)
{
    vga_setmode(TEXT);
    return 0;
}

private gx_color_index
vgalib_map_rgb_color(gx_device * dev, gx_color_value red,
		     gx_color_value green, gx_color_value blue)
{
    return pc_4bit_map_rgb_color(dev, red, green, blue);
}

private int
vgalib_map_color_rgb(gx_device * dev, gx_color_index index,
		     unsigned short rgb[3])
{
    return pc_4bit_map_color_rgb(dev, index, rgb);
}

private int
vgalib_tile_rectangle(gx_device * dev, const gx_tile_bitmap * tile,
		      int x, int y, int w, int h, gx_color_index czero,
		      gx_color_index cone, int px, int py)
{
    if (czero != gx_no_color_index && cone != gx_no_color_index) {
	vgalib_fill_rectangle(dev, x, y, w, h, czero);
	czero = gx_no_color_index;
    }
    return gx_default_tile_rectangle(dev, tile, x, y, w, h, czero, cone, px,
				     py);
}

private int
vgalib_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		      gx_color_index color)
{
    int i, j;

    fit_fill(dev, x, y, w, h);
    vga_setcolor((int)color);
    if ((w | h) > 3) {		/* Draw larger rectangles as lines. */
	if (w > h)
	    for (i = y; i < y + h; ++i)
		vga_drawline(x, i, x + w - 1, i);
	else
	    for (j = x; j < x + w; ++j)
		vga_drawline(j, y, j, y + h - 1);
    } else {			/* Draw small rectangles point-by-point. */
	for (i = y; i < y + h; i++)
	    for (j = x; j < x + w; j++)
		vga_drawpixel(j, i);
    }
    return 0;
}

private int
vgalib_copy_mono(gx_device * dev, const byte * base, int sourcex,
		 int raster, gx_bitmap_id id, int x, int y, int width,
		 int height, gx_color_index zero, gx_color_index one)
{
    const byte *ptr_line;
    int left_bit, dest_y, end_x;
    int invert = 0;
    int color;

    fit_copy(dev, base, sourcex, raster, id, x, y, width, height);
    ptr_line = base + (sourcex >> 3);
    left_bit = 0x80 >> (sourcex & 7);
    dest_y = y, end_x = x + width;

    if (zero == gx_no_color_index) {
	if (one == gx_no_color_index)
	    return 0;
	color = (int)one;
    } else {
	if (one == gx_no_color_index) {
	    color = (int)zero;
	    invert = -1;
	} else {		/* Pre-clear the rectangle to zero */
	    vgalib_fill_rectangle(dev, x, y, width, height, zero);
	    color = (int)one;
	}
    }

    vga_setcolor(color);
    while (height--) {		/* for each line */
	const byte *ptr_source = ptr_line;
	register int dest_x = x;
	register int bit = left_bit;

	while (dest_x < end_x) {	/* for each bit in the line */
	    if ((*ptr_source ^ invert) & bit)
		vga_drawpixel(dest_x, dest_y);
	    dest_x++;
	    if ((bit >>= 1) == 0)
		bit = 0x80, ptr_source++;
	}

	dest_y++;
	ptr_line += raster;
    }
    return 0;
}


/* Copy a color pixel map.  This is just like a bitmap, except that */
/* each pixel takes 4 bits instead of 1 when device driver has color. */
private int
vgalib_copy_color(gx_device * dev, const byte * base, int sourcex,
		  int raster, gx_bitmap_id id, int x, int y,
		  int width, int height)
{

    fit_copy(dev, base, sourcex, raster, id, x, y, width, height);

    if (gx_device_has_color(dev)) {	/* color device, four bits per pixel */
	const byte *line = base + (sourcex >> 1);
	int dest_y = y, end_x = x + width;

	if (width <= 0)
	    return 0;
	while (height--) {	/* for each line */
	    const byte *source = line;
	    register int dest_x = x;

	    if (sourcex & 1) {	/* odd nibble first */
		int color = *source++ & 0xf;

		vga_setcolor(color);
		vga_drawpixel(dest_x, dest_y);
		dest_x++;
	    }
	    /* Now do full bytes */
	    while (dest_x < end_x) {
		int color = *source >> 4;

		vga_setcolor(color);
		vga_drawpixel(dest_x, dest_y);
		dest_x++;

		if (dest_x < end_x) {
		    color = *source++ & 0xf;
		    vga_setcolor(color);
		    vga_drawpixel(dest_x, dest_y);
		    dest_x++;
		}
	    }

	    dest_y++;
	    line += raster;
	}
    } else {			/* monochrome device: one bit per pixel */
	/* bitmap is the same as bgi_copy_mono: one bit per pixel */
	vgalib_copy_mono(dev, base, sourcex, raster, id, x, y, width, height,
			 (gx_color_index) 0, (gx_color_index) 7);
    }

    return 0;
}

/* Read bits back from the device. */
private int
vgalib_get_bits(gx_device * dev, int y, byte * data, byte ** actual_data)
{
    int x;
    byte *dest = data;
    int b = 0;
    int depth = dev->color_info.depth;	/* 1 or 4 */
    int mask = (1 << depth) - 1;
    int left = 8;

    if (actual_data)
	*actual_data = data;
    for (x = 0; x < dev->width; ++x) {
	int color = vga_getpixel(x, y);

	if ((left -= depth) < 0)
	    *dest++ = b, b = 0, left += 8;
	b += (color & mask) << left;
    }
    if (left < 8)
	*dest = b;
    return 0;
}

/* Get/put the display mode parameter. */
private int
vgalib_get_params(gx_device * dev, gs_param_list * plist)
{
    int code = gx_default_get_params(dev, plist);

    if (code < 0)
	return code;
    return param_write_int(plist, "DisplayMode", &vga_dev->display_mode);
}
private int
vgalib_put_params(gx_device * dev, gs_param_list * plist)
{
    int ecode = 0;
    int code;
    int imode = vga_dev->display_mode;
    const char *param_name;

    switch (code = param_read_int(plist, (param_name = "DisplayMode"), &imode)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }

    if (ecode < 0)
	return ecode;
    code = gx_default_put_params(dev, plist);
    if (code < 0)
	return code;

    if (imode != vga_dev->display_mode) {
	if (dev->is_open)
	    gs_closedevice(dev);
	vga_dev->display_mode = imode;
    }
    return 0;
}
