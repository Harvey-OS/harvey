/* Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevxalt.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Alternative X Windows drivers for help in driver debugging */
#include "gx.h"			/* for gx_bitmap; includes std.h */
#include "math_.h"
#include "memory_.h"
#include "x_.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gsstruct.h"
#include "gxdevice.h"
#include "gsdevice.h"		/* for gs_copydevice */
#include "gdevx.h"

extern const gx_device_X gs_x11_device;

/*
 * Define a forwarding device with a cache for the first 16 colors,
 * which avoids all of the time-consuming color mapping calls for
 * the black-and-white, 2-bit gray, and 1-bit CMYK devices defined here.
 */
typedef struct {
    gx_device_forward_common;
    gx_color_index color_cache[16];
    /*
     * alt_map_color returns a value >= 0 if it maps directly to the final
     * gx_color_index, or < 0 if it only sets RGB values.
     */
    dev_proc_map_color_rgb((*alt_map_color));
} gx_device_X_wrapper;
#define X_WRAPPER_DATA(amc_proc)\
	/* gx_device_forward_common */\
    {0},			/* std_procs */\
    0,				/* target */\
	/* gx_device_X_wrapper */\
    {0},			/* cache */\
    amc_proc
gs_private_st_suffix_add0_final(st_device_X_wrapper, gx_device_X_wrapper,
  "gx_device_X_wrapper", gdevx_wrapper_enum_ptrs, gdevx_wrapper_reloc_ptrs,
  gx_device_finalize, st_device_forward);

/* ---------------- Generic procedures ---------------- */

/* Forward declarations */
private int get_dev_target(P2(gx_device **, gx_device *));

#define set_dev_target(tdev, dev)\
  return_if_error(get_dev_target(&tdev, dev))
private int get_target_info(P1(gx_device *));
private gx_color_index x_alt_map_color(P2(gx_device *, gx_color_index));

/* Clear the color mapping cache. */
private void
x_clear_color_cache(gx_device /*gx_device_X_wrapper */  * dev)
{
    gx_device_X_wrapper *xdev = (gx_device_X_wrapper *) dev;
    int i;

    for (i = 0; i < countof(xdev->color_cache); ++i)
	xdev->color_cache[i] = gx_no_color_index;
    gx_device_decache_colors(dev);
}

/* "Wrappers" for driver procedures */

private int
x_wrap_open(gx_device * dev)
{
    gx_device *tdev;
    int rcode, code;

    set_dev_target(tdev, dev);
    rcode = (*dev_proc(tdev, open_device)) (tdev);
    if (rcode < 0)
	return rcode;
    tdev->is_open = true;
    code = get_target_info(dev);
    return (code < 0 ? code : rcode);
}

private int
x_forward_sync_output(gx_device * dev)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, sync_output)) (tdev);
}

private int
x_forward_output_page(gx_device * dev, int num_copies, int flush)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, output_page)) (tdev, num_copies, flush);
}

private int
x_wrap_close(gx_device * dev)
{
    /*
     * The underlying x11 device will be closed and freed as soon as there
     * are no more pointers to it, which normally occurs in the next
     * statement.
     */
    gx_device_set_target((gx_device_forward *)dev, NULL);
    x_clear_color_cache(dev);
    return 0;
}

private int
x_wrap_map_color_rgb(gx_device * dev, gx_color_index color,
		     gx_color_value prgb[3])
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, map_color_rgb)) (tdev,
					     x_alt_map_color(dev, color),
					     prgb);
}

private int
x_wrap_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		      gx_color_index color)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, fill_rectangle)) (tdev, x, y, w, h,
					      x_alt_map_color(dev, color));
}

private int
x_wrap_copy_mono(gx_device * dev,
		 const byte * base, int sourcex, int raster, gx_bitmap_id id,
		 int x, int y, int w, int h,
		 gx_color_index zero, gx_color_index one)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, copy_mono)) (tdev, base, sourcex, raster, id,
					 x, y, w, h,
					 x_alt_map_color(dev, zero),
					 x_alt_map_color(dev, one));

}

private int
x_wrap_copy_color(gx_device * dev, const byte * base, int sourcex,
		  int raster, gx_bitmap_id id, int x, int y, int w, int h)
{
    gx_device *tdev;

#define mapped_bytes 480	/* must be a multiple of 3 & 4 */
    int depth_bytes, source_bits;
    int block_w, block_h;
    int xblock, yblock;
    byte mapped[mapped_bytes];

    fit_copy(dev, base, sourcex, raster, id, x, y, w, h);
    set_dev_target(tdev, dev);
    /* Device pixels must be an integral number of bytes. */
    if (tdev->color_info.depth & 7)
	return gx_default_copy_color(dev, base, sourcex, raster, id,
				     x, y, w, h);
    depth_bytes = tdev->color_info.depth >> 3;
    source_bits = dev->color_info.depth;
    {
	int mapped_pixels = mapped_bytes / depth_bytes;

	if (w > mapped_pixels >> 1)
	    block_w = min(w, mapped_pixels), block_h = 1;
	else
	    block_w = w, block_h = mapped_pixels / w;
    }
    for (yblock = y; yblock < y + h; yblock += block_h)
	for (xblock = x; xblock < x + w; xblock += block_w) {
	    byte *p = mapped;
	    int xend = min(xblock + block_w, x + w);
	    int yend = min(yblock + block_h, y + h);
	    int xcur, ycur;
	    int code;

	    for (ycur = yblock; ycur < yend; ++ycur)
		for (xcur = xblock; xcur < xend; ++xcur) {
		    int sbit = (xcur - x + sourcex) * source_bits;
		    uint sbyte =
			base[(ycur - y) * raster + (sbit >> 3)];
		    uint spixel =
			((sbyte << (sbit & 7)) & 0xff) >> (8 - source_bits);
		    gx_color_index cindex =
			((gx_device_X_wrapper *) dev)->color_cache[spixel];

		    if (cindex == gx_no_color_index)
			cindex = x_alt_map_color(dev, spixel);
		    switch (depth_bytes) {
			case 4:
			    *p++ = (byte) (cindex >> 24);
			case 3:
			    *p++ = (byte) (cindex >> 16);
			case 2:
			    *p++ = (byte) (cindex >> 8);
			default /*case 1 */ :
			    *p++ = (byte) cindex;
		    }
		}
	    code = (*dev_proc(tdev, copy_color))
		(tdev, mapped, 0, (xend - xblock) * depth_bytes, gx_no_bitmap_id,
		 xblock, yblock, xend - xblock, yend - yblock);
	    if (code < 0)
		return code;
	}
    return 0;
}


private int
x_forward_copy_color(gx_device * dev, const byte * base, int sourcex,
		     int raster, gx_bitmap_id id, int x, int y, int w, int h)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, copy_color)) (tdev, base, sourcex, raster, id,
					  x, y, w, h);
}

private int
x_forward_get_bits(gx_device * dev, int y, byte * str, byte ** actual_data)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);
    return (*dev_proc(tdev, get_bits)) (tdev, y, str, actual_data);
}

private int
x_wrap_get_bits(gx_device * dev, int y, byte * str, byte ** actual_data)
{
    int depth = dev->color_info.depth;
    gx_device *tdev;
    int width;
    int sdepth;
    byte smask;
    uint dsize;
    gs_memory_t *mem = dev->memory;
    byte *row;
    byte *base;
    int code;
    gx_color_index pixel_in = gx_no_color_index;
    /*
     * The following initialization is unnecessary: since no pixel has a
     * value of gx_no_color_index, the test pixel != pixel_in will always
     * succeed the first time through the loop below, so pixel_out will
     * always be set before it is used.  We initialize pixel_out solely to
     * suppress bogus warning messages from certain compilers.
     */
    gx_color_index pixel_out = 0;
    int xi;
    int sbit;

    DECLARE_LINE_ACCUM(str, depth, 0);

    set_dev_target(tdev, dev);
    width = tdev->width;
    sdepth = tdev->color_info.depth;
    smask = (sdepth <= 8 ? (1 << sdepth) - 1 : 0xff);
    dsize = (width * sdepth + 7) / 8;
    row = gs_alloc_bytes(mem, dsize, "x_wrap_get_bits");
    if (row == 0)
	return_error(gs_error_VMerror);
    code = (*dev_proc(tdev, get_bits)) (tdev, y, row, &base);
    if (code < 0)
	goto gx;
    for (sbit = 0, xi = 0; xi < width; sbit += sdepth, ++xi) {
	const byte *sptr = base + (sbit >> 3);
	gx_color_index pixel;
	gx_color_value rgb[3];
	int i;

	if (sdepth <= 8)
	    pixel = (*sptr >> (8 - sdepth - (sbit & 7))) & smask;
	else {
	    pixel = 0;
	    for (i = 0; i < sdepth; i += 8, ++sptr)
		pixel = (pixel << 8) + *sptr;
	}
	if (pixel != pixel_in) {
	    (*dev_proc(tdev, map_color_rgb))(tdev, pixel, rgb);
	    pixel_in = pixel;
	    if (dev->color_info.num_components <= 3)
		pixel_out = (*dev_proc(dev, map_rgb_color))
		    (dev, rgb[0], rgb[1], rgb[2]);
	    else {
		/* Convert RGB to CMYK. */
		gx_color_value c = gx_max_color_value - rgb[0];
		gx_color_value m = gx_max_color_value - rgb[1];
		gx_color_value y = gx_max_color_value - rgb[2];
		gx_color_value k = (c < m ? min(c, y) : min(m, y));

		pixel_out = (*dev_proc(dev, map_cmyk_color))
		    (dev, c - k, m - k, y - k, k);
	    }
	}
	LINE_ACCUM(pixel_out, depth);
    }
    LINE_ACCUM_STORE(depth);
  gx:gs_free_object(mem, row, "x_wrap_get_bits");
    *actual_data = str;
    return code;
}

private int
x_wrap_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device *tdev;
    /* We assume that a get_params call has no side effects.... */
    gx_device_X save_dev;
    int ecode;

    set_dev_target(tdev, dev);
    save_dev = *(gx_device_X *) tdev;
    if (tdev->is_open)
	tdev->color_info = dev->color_info;
    tdev->dname = dev->dname;
    ecode = (*dev_proc(tdev, get_params)) (tdev, plist);
    *(gx_device_X *) tdev = save_dev;
    return ecode;
}

private int
x_wrap_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device *tdev;
    gx_device_color_info cinfo;
    const char *dname;
    int rcode, code;

    set_dev_target(tdev, dev);
    /*
     * put_params will choke if we simply feed it the output of
     * get_params; we have to substitute color_info the same way.
     */
    cinfo = tdev->color_info;
    dname = tdev->dname;
    tdev->color_info = dev->color_info;
    tdev->dname = dev->dname;
    rcode = (*dev_proc(tdev, put_params)) (tdev, plist);
    tdev->color_info = cinfo;
    tdev->dname = dname;
    if (rcode < 0)
	return rcode;
    code = get_target_info(dev);
    return (code < 0 ? code : rcode);
}

/* Internal procedures */

/* Get the target, creating it if necessary. */
private int
get_dev_target(gx_device ** ptdev, gx_device * dev)
{
    gx_device *tdev = ((gx_device_forward *) dev)->target;

    if (tdev == 0) {
	/* Create an X device instance. */
	int code = gs_copydevice(&tdev, (const gx_device *)&gs_x11_device,
				 dev->memory);

	if (code < 0)
	    return 0;
	gx_device_fill_in_procs(tdev);
	gx_device_set_target((gx_device_forward *)dev, tdev);
	x_clear_color_cache(dev);
    }
    *ptdev = tdev;
    return 0;
}

/* Copy parameters back from the target. */
private int
get_target_info(gx_device * dev)
{
    gx_device *tdev;

    set_dev_target(tdev, dev);

#define copy(m) dev->m = tdev->m;
#define copy2(m) copy(m[0]); copy(m[1])
#define copy4(m) copy2(m); copy(m[2]); copy(m[3])

    copy(width);
    copy(height);
    copy2(MediaSize);
    copy4(ImagingBBox);
    copy(ImagingBBox_set);
    copy2(HWResolution);
    copy2(MarginsHWResolution);
    copy2(Margins);
    copy4(HWMargins);
    if (dev->color_info.num_components == 3) {
	/* Leave the anti-aliasing information alone. */
	gx_device_anti_alias_info aa;

	aa = dev->color_info.anti_alias;
	copy(color_info);
	dev->color_info.anti_alias = aa;
    }

#undef copy4
#undef copy2
#undef copy

    x_clear_color_cache(dev);
    return 0;
}

/* Map a fake CMYK or black/white color to a real X color if necessary. */
private gx_color_index
x_alt_map_color(gx_device * dev, gx_color_index color)
{
    gx_device_X_wrapper *xdev = (gx_device_X_wrapper *) dev;
    gx_device *tdev;
    gx_color_value rgb[3];
    gx_color_index cindex;
    int result;

    if (color == gx_no_color_index)
	return color;
    if (color < 16) {
	cindex = ((gx_device_X_wrapper *) dev)->color_cache[color];
	if (cindex != gx_no_color_index)
	    return cindex;
    }
    set_dev_target(tdev, dev);
    result = xdev->alt_map_color(dev, color, rgb);
    if (result >= 0)
	cindex = result;
    else
	cindex = dev_proc(tdev, map_rgb_color)(tdev, rgb[0], rgb[1], rgb[2]);
    if (color < 16)
	((gx_device_X_wrapper *) dev)->color_cache[color] = cindex;
    return cindex;
}

/* ---------------- CMYK procedures ---------------- */

/* Device procedures */
private dev_proc_open_device(x_cmyk_open);
private dev_proc_put_params(x_cmyk_put_params);
private dev_proc_map_cmyk_color(x_cmyk_map_cmyk_color);
/* Extended device procedures */
private dev_proc_map_color_rgb(x_cmyk_alt_map_color);

/* The device descriptor */
private const gx_device_procs x_cmyk_procs = {
    x_cmyk_open,
    gx_forward_get_initial_matrix,
    x_forward_sync_output,
    x_forward_output_page,
    x_wrap_close,
    NULL,			/* map_rgb_color */
    x_wrap_map_color_rgb,
    x_wrap_fill_rectangle,
    gx_default_tile_rectangle,
    x_wrap_copy_mono,
    x_wrap_copy_color,
    gx_default_draw_line,
    x_wrap_get_bits,
    x_wrap_get_params,
    x_cmyk_put_params,
    x_cmyk_map_cmyk_color,
    gx_forward_get_xfont_procs,
    gx_forward_get_xfont_device,
    NULL,			/* map_rgb_alpha_color */
    gx_forward_get_page_device,
    gx_forward_get_alpha_bits,
    NULL			/* copy_alpha */
};

/* The instances are public. */
const gx_device_X_wrapper gs_x11cmyk_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_cmyk_procs, "x11cmyk",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	4, 4, 1, 1, 2, 2),
    X_WRAPPER_DATA(x_cmyk_alt_map_color)
};
const gx_device_X_wrapper gs_x11cmyk2_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_cmyk_procs, "x11cmyk2",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	4, 8, 3, 3, 4, 4),
    X_WRAPPER_DATA(x_cmyk_alt_map_color)
};
const gx_device_X_wrapper gs_x11cmyk4_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_cmyk_procs, "x11cmyk4",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	4, 16, 15, 15, 16, 16),
    X_WRAPPER_DATA(x_cmyk_alt_map_color)
};
const gx_device_X_wrapper gs_x11cmyk8_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_cmyk_procs, "x11cmyk8",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	4, 32, 255, 255, 256, 256),
    X_WRAPPER_DATA(x_cmyk_alt_map_color)
};

/* Map a fake color to RGB. */
private int
x_cmyk_alt_map_color(gx_device * dev, gx_color_index color,
		     gx_color_value rgb[3])
{
    int shift = dev->color_info.depth >> 2;
    int mask = (1 << shift) - 1;
    /* The following division is guaranteed exact. */
    gx_color_value scale = gx_max_color_value / mask;
    int cw = ~color & mask;
    int cb = cw - ((color >> shift) & mask);
    int cg = cw - ((color >> (shift * 2)) & mask);
    int cr = cw - ((color >> (shift * 3)) & mask);

    rgb[0] = max(cr, 0) * scale;
    rgb[1] = max(cg, 0) * scale;
    rgb[2] = max(cb, 0) * scale;
    return -1;
}

/* Set color mapping procedures */
private void
x_cmyk_set_procs(gx_device *dev)
{
    if (dev->color_info.depth == 4) {
	set_dev_proc(dev, map_cmyk_color, cmyk_1bit_map_cmyk_color);
    } else {
	set_dev_proc(dev, map_cmyk_color, x_cmyk_map_cmyk_color);
    }
}

/* Device procedures */

private int
x_cmyk_open(gx_device *dev)
{
    int code = x_wrap_open(dev);

    if (code >= 0)
	x_cmyk_set_procs(dev);
    return code;
}

private int
x_cmyk_put_params(gx_device * dev, gs_param_list * plist)
{
    int code = x_wrap_put_params(dev, plist);

    if (code >= 0)
	x_cmyk_set_procs(dev);
    return code;
}

private gx_color_index
x_cmyk_map_cmyk_color(gx_device * dev,
		      gx_color_value c, gx_color_value m, gx_color_value y,
		      gx_color_value k)
{
    int shift = dev->color_info.depth >> 2;
    gx_color_index pixel = c >> (gx_color_value_bits - shift);

    pixel = (pixel << shift) | (m >> (gx_color_value_bits - shift));
    pixel = (pixel << shift) | (y >> (gx_color_value_bits - shift));
    return (pixel << shift) | (k >> (gx_color_value_bits - shift));
}

/* ---------------- Black-and-white procedures ---------------- */

/* Extended device procedures */
private dev_proc_map_color_rgb(x_mono_alt_map_color);

/* The device descriptor */
private const gx_device_procs x_mono_procs = {
    x_wrap_open,
    gx_forward_get_initial_matrix,
    x_forward_sync_output,
    x_forward_output_page,
    x_wrap_close,
    gx_default_b_w_map_rgb_color,
    x_wrap_map_color_rgb,
    x_wrap_fill_rectangle,
    gx_default_tile_rectangle,
    x_wrap_copy_mono,
    gx_default_copy_color,	/* this is fast for the 1-bit case */
    gx_default_draw_line,
    x_wrap_get_bits,
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
const gx_device_X_wrapper gs_x11mono_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_mono_procs, "x11mono",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	1, 1, 1, 0, 2, 0),
    X_WRAPPER_DATA(x_mono_alt_map_color)
};

/* Map a fake color to RGB. */
private int
x_mono_alt_map_color(gx_device * dev, gx_color_index color,
		     gx_color_value rgb[3])
{
    rgb[0] = rgb[1] = rgb[2] = (color ? 0 : gx_max_color_value);
    return -1;
}

/* ---------------- 2- and 4-bit gray-scale procedures ---------------- */

/* Extended device procedures */
private dev_proc_map_color_rgb(x_gray_alt_map_color);

/* The device descriptor */
private const gx_device_procs x_gray_procs = {
    x_wrap_open,
    gx_forward_get_initial_matrix,
    x_forward_sync_output,
    x_forward_output_page,
    x_wrap_close,
    gx_default_gray_map_rgb_color,
    x_wrap_map_color_rgb,
    x_wrap_fill_rectangle,
    gx_default_tile_rectangle,
    x_wrap_copy_mono,
    x_wrap_copy_color,
    gx_default_draw_line,
    x_wrap_get_bits,
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

/* The instances are public. */
const gx_device_X_wrapper gs_x11gray2_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_gray_procs, "x11gray2",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	1, 2, 3, 0, 4, 0),
    X_WRAPPER_DATA(x_gray_alt_map_color)
};

const gx_device_X_wrapper gs_x11gray4_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_gray_procs, "x11gray4",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	1, 4, 15, 0, 16, 0),
    X_WRAPPER_DATA(x_gray_alt_map_color)
};

/* Map a fake color to RGB. */
private int
x_gray_alt_map_color(gx_device * dev, gx_color_index color,
		     gx_color_value rgb[3])
{
    rgb[0] = rgb[1] = rgb[2] =
	color * gx_max_color_value / dev->color_info.max_gray;
    return -1;
}

/* ---------------- Alpha procedures ---------------- */

/* Device procedures */
private dev_proc_map_color_rgb(x_alpha_map_color_rgb);
private dev_proc_map_rgb_alpha_color(x_alpha_map_rgb_alpha_color);
private dev_proc_copy_alpha(x_alpha_copy_alpha);
/* Extended device procedures */
private dev_proc_map_color_rgb(x_alpha_alt_map_color);

/* The device descriptor */
private const gx_device_procs x_alpha_procs = {
    x_wrap_open,
    gx_forward_get_initial_matrix,
    x_forward_sync_output,
    x_forward_output_page,
    x_wrap_close,
    gx_forward_map_rgb_color,
    x_alpha_map_color_rgb,
    x_wrap_fill_rectangle,
    gx_default_tile_rectangle,
    x_wrap_copy_mono,
    x_forward_copy_color,
    gx_default_draw_line,
    x_forward_get_bits,
    x_wrap_get_params,
    x_wrap_put_params,
    gx_forward_map_cmyk_color,
    gx_forward_get_xfont_procs,
    gx_forward_get_xfont_device,
    x_alpha_map_rgb_alpha_color,
    gx_forward_get_page_device,
    gx_default_get_alpha_bits,
	/*gx_default_copy_alpha */ x_alpha_copy_alpha
};

/* The instance is public. */
const gx_device_X_wrapper gs_x11alpha_device = {
    std_device_dci_alpha_type_body(gx_device_X_wrapper, &x_alpha_procs,
	"x11alpha", &st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	3, 32, 255, 255, 256, 256, 4, 4),
    X_WRAPPER_DATA(x_alpha_alt_map_color)
};

/* Map a fake color to RGB. */
private int
x_alpha_alt_map_color(gx_device * dev, gx_color_index color,
		      gx_color_value rgb[3])
{
    return color & 0xffffff;	/* just remove alpha */
}

/* Device procedures */

/* We encode a complemented alpha value in the top 8 bits of the */
/* device color. */
private int
x_alpha_map_color_rgb(gx_device * dev, gx_color_index color,
		      gx_color_value prgb[3])
{
    return gx_forward_map_color_rgb(dev, color & 0xffffff, prgb);
}
private gx_color_index
x_alpha_map_rgb_alpha_color(gx_device * dev,
 gx_color_value r, gx_color_value g, gx_color_value b, gx_color_value alpha)
{
    gx_color_index color = gx_forward_map_rgb_color(dev, r, g, b);
    byte abyte = alpha >> (gx_color_value_bits - 8);

    return (abyte == 0 ? (gx_color_index)0xff << 24 :
	    ((gx_color_index) (abyte ^ 0xff) << 24) + color);
}

private int
x_alpha_copy_alpha(gx_device * dev, const unsigned char *base, int sourcex,
		   int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gx_color_index color, int depth)
{
    gx_device *tdev;
    int xi, yi;
    const byte *row = base;
    gx_color_index base_color = color & 0xffffff;

    /* We fake alpha by interpreting it as saturation, i.e., */
    /* alpha = 0 is white, alpha = 15/15 is the full color. */
    gx_color_value rgb[3];
    gx_color_index shades[16];
    int i;

    set_dev_target(tdev, dev);
    for (i = 0; i < 15; ++i)
	shades[i] = gx_no_color_index;
    shades[15] = base_color;
    (*dev_proc(tdev, map_color_rgb)) (tdev, base_color, rgb);
    /* Do the copy operation pixel-by-pixel. */
    /* For the moment, if the base color has alpha in it, we ignore it. */
    for (yi = y; yi < y + h; row += raster, ++yi) {
	int prev_x = x;
	gx_color_index prev_color = gx_no_color_index;
	uint prev_alpha = 0x10;	/* not a possible value */

	for (xi = x; xi < x + w; ++xi) {
	    int sx = sourcex + xi - x;
	    uint alpha2 = row[sx >> 1];
	    uint alpha = (sx & 1 ? alpha2 & 0xf : alpha2 >> 4);
	    gx_color_index a_color;

	    if (alpha == prev_alpha)
		continue;
	    prev_alpha = alpha;
	    if (alpha == 0)
		a_color = gx_no_color_index;
	    else
		while ((a_color = shades[alpha]) == gx_no_color_index) {	/* Map the color now. */
#define make_shade(v, alpha)\
  (gx_max_color_value -\
   ((gx_max_color_value - (v)) * (alpha) / 15))
		    gx_color_value r = make_shade(rgb[0], alpha);
		    gx_color_value g = make_shade(rgb[1], alpha);
		    gx_color_value b = make_shade(rgb[2], alpha);

#undef make_shade
		    a_color = (*dev_proc(tdev, map_rgb_color)) (tdev, r, g, b);
		    if (a_color != gx_no_color_index) {
			shades[alpha] = a_color;
			break;
		    }
		    /* Try a higher saturation.  (We know */
		    /* the fully saturated color exists.) */
		    alpha += (16 - alpha) >> 1;
		}
	    if (a_color != prev_color) {
		if (prev_color != gx_no_color_index)
		    (*dev_proc(tdev, fill_rectangle)) (tdev,
						 prev_x, yi, xi - prev_x, 1,
						       prev_color);
		prev_x = xi;
		prev_color = a_color;
	    }
	}
	if (prev_color != gx_no_color_index)
	    (*dev_proc(tdev, fill_rectangle)) (tdev,
					       prev_x, yi, x + w - prev_x, 1,
					       prev_color);
    }
    return 0;
}

/* ---------------- Permuted RGB16/32 procedures ---------------- */

/* Device procedures */
private dev_proc_map_rgb_color(x_rg16x_map_rgb_color);
private dev_proc_map_rgb_color(x_rg32x_map_rgb_color);
/* Extended device procedures */
private dev_proc_map_color_rgb(x_rg16x_alt_map_color);
private dev_proc_map_color_rgb(x_rg32x_alt_map_color);

/* The device descriptor */
#define RGBX_PROCS(map_rgb_proc)\
    x_wrap_open,\
    gx_forward_get_initial_matrix,\
    x_forward_sync_output,\
    x_forward_output_page,\
    x_wrap_close,\
    map_rgb_proc,		/* differs */\
    x_wrap_map_color_rgb,\
    x_wrap_fill_rectangle,\
    gx_default_tile_rectangle,\
    x_wrap_copy_mono,\
    x_forward_copy_color,\
    gx_default_draw_line,\
    x_forward_get_bits,\
    x_wrap_get_params,\
    x_wrap_put_params,\
    gx_forward_map_cmyk_color,\
    gx_forward_get_xfont_procs,\
    gx_forward_get_xfont_device,\
    x_alpha_map_rgb_alpha_color,\
    gx_forward_get_page_device,\
    gx_default_get_alpha_bits,\
    gx_default_copy_alpha

private const gx_device_procs x_rg16x_procs = {
    RGBX_PROCS(x_rg16x_map_rgb_color)
};
const gx_device_X_wrapper gs_x11rg16x_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_rg16x_procs, "x11rg16x",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	3, 16, 31, 31, 32, 32),
    X_WRAPPER_DATA(x_rg16x_alt_map_color)
};

private const gx_device_procs x_rg32x_procs = {
    RGBX_PROCS(x_rg32x_map_rgb_color)
};
const gx_device_X_wrapper gs_x11rg32x_device = {
    std_device_dci_type_body(gx_device_X_wrapper, &x_rg32x_procs, "x11rg32x",
	&st_device_X_wrapper,
	FAKE_RES * 85 / 10, FAKE_RES * 11,	/* x and y extent (nominal) */
	FAKE_RES, FAKE_RES,	/* x and y density (nominal) */
	3, 32, 1023, 1023, 1024, 1024),
    X_WRAPPER_DATA(x_rg32x_alt_map_color)
};

/* Map RGB to a fake color. */
private gx_color_index
x_rg16x_map_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
		      gx_color_value b)
{
    /* Permute the colors to G5/B5/R6. */
    return (r >> (gx_color_value_bits - 6)) +
	((g >> (gx_color_value_bits - 5)) << 11) +
	((b >> (gx_color_value_bits - 5)) << 6);
}
private gx_color_index
x_rg32x_map_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
		      gx_color_value b)
{
    /* Permute the colors to G11/B10/R11. */
    return (r >> (gx_color_value_bits - 11)) +
	((gx_color_index)(g >> (gx_color_value_bits - 11)) << 21) +
	((gx_color_index)(b >> (gx_color_value_bits - 10)) << 11);
}

/* Map a fake color to RGB. */
private int
x_rg16x_alt_map_color(gx_device * dev, gx_color_index color,
		      gx_color_value rgb[3])
{
    rgb[0] = (color & 0x3f) * gx_max_color_value / 0x3f;
    rgb[1] = ((color >> 11) & 0x1f) * gx_max_color_value / 0x1f;
    rgb[2] = ((color >> 6) & 0x1f) * gx_max_color_value / 0x1f;
    return -1;
}
private int
x_rg32x_alt_map_color(gx_device * dev, gx_color_index color,
		      gx_color_value rgb[3])
{
    rgb[0] = (color & 0x7ff) * gx_max_color_value / 0x7ff;
    rgb[1] = ((color >> 21) & 0x7ff) * gx_max_color_value / 0x7ff;
    rgb[2] = ((color >> 11) & 0x3ff) * gx_max_color_value / 0x3ff;
    return -1;
}
