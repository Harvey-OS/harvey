/* Copyright (C) 1991, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevbit.c,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* "Plain bits" devices to measure rendering time. */
#include "math_.h"
#include "gdevprn.h"
#include "gsparam.h"
#include "gscrd.h"
#include "gscrdp.h"
#include "gxlum.h"
#include "gdevdcrd.h"

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* The device descriptor */
private dev_proc_map_rgb_color(bit_mono_map_rgb_color);
private dev_proc_map_rgb_color(bit_forcemono_map_rgb_color);
private dev_proc_map_color_rgb(bit_map_color_rgb);
private dev_proc_map_cmyk_color(bit_map_cmyk_color);
private dev_proc_get_params(bit_get_params);
private dev_proc_put_params(bit_put_params);
private dev_proc_print_page(bit_print_page);

#define bit_procs(map_rgb_color, map_cmyk_color)\
{	gdev_prn_open,\
	gx_default_get_initial_matrix,\
	NULL,	/* sync_output */\
	gdev_prn_output_page,\
	gdev_prn_close,\
	map_rgb_color,\
	bit_map_color_rgb,\
	NULL,	/* fill_rectangle */\
	NULL,	/* tile_rectangle */\
	NULL,	/* copy_mono */\
	NULL,	/* copy_color */\
	NULL,	/* draw_line */\
	NULL,	/* get_bits */\
	bit_get_params,\
	bit_put_params,\
	map_cmyk_color,\
	NULL,	/* get_xfont_procs */\
	NULL,	/* get_xfont_device */\
	NULL,	/* map_rgb_alpha_color */\
	gx_page_device_get_page_device	/* get_page_device */\
}

/*
 * The following macro is used in get_params and put_params to determine the
 * num_components for the current device. It works using the device name
 * character after "bit" which is either '\0', 'r', or 'c'. Any new devices
 * that are added to this module must modify this macro to return the
 * correct num_components. This is needed to support the ForceMono
 * parameter, which alters dev->num_components.
 */
#define REAL_NUM_COMPONENTS(dev) (dev->dname[3] == 'c' ? 4 : \
				  dev->dname[3] == 'r' ? 3 : 1)

private const gx_device_procs bitmono_procs =
bit_procs(bit_mono_map_rgb_color, NULL);
const gx_device_printer gs_bit_device =
{prn_device_body(gx_device_printer, bitmono_procs, "bit",
		 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		 X_DPI, Y_DPI,
		 0, 0, 0, 0,    /* margins */
		 1, 1, 1, 0, 2, 1, bit_print_page)
};

private const gx_device_procs bitrgb_procs =
bit_procs(gx_default_rgb_map_rgb_color, NULL);
const gx_device_printer gs_bitrgb_device =
{prn_device_body(gx_device_printer, bitrgb_procs, "bitrgb",
		 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		 X_DPI, Y_DPI,
		 0, 0, 0, 0,	/* margins */
		 3, 4, 1, 1, 2, 2, bit_print_page)
};

private const gx_device_procs bitcmyk_procs =
bit_procs(bit_forcemono_map_rgb_color, bit_map_cmyk_color);
const gx_device_printer gs_bitcmyk_device =
{prn_device_body(gx_device_printer, bitcmyk_procs, "bitcmyk",
		 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		 X_DPI, Y_DPI,
		 0, 0, 0, 0,	/* margins */
		 4, 4, 1, 1, 2, 2, bit_print_page)
};

/* Map gray to color. */
/* Note that 1-bit monochrome is a special case. */
private gx_color_index
bit_mono_map_rgb_color(gx_device * dev, gx_color_value red,
		       gx_color_value green, gx_color_value blue)
{
    int bpc = dev->color_info.depth;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_value gray =
    (red * (unsigned long)lum_red_weight +
     green * (unsigned long)lum_green_weight +
     blue * (unsigned long)lum_blue_weight +
     (lum_all_weights / 2))
    / lum_all_weights;

    return (bpc == 1 ? gx_max_color_value - gray : gray) >> drop;
}

/* Map RGB to gray shade. */
/* Only used in CMYK mode when put_params has set ForceMono=1 */
private gx_color_index
bit_forcemono_map_rgb_color(gx_device * dev, gx_color_value red,
		  gx_color_value green, gx_color_value blue)
{
    gx_color_value color;
    int bpc = dev->color_info.depth / 4;	/* This function is used in CMYK mode */
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_value gray = red;

    if ((red != green) || (green != blue))
	gray = (red * (unsigned long)lum_red_weight +
	     green * (unsigned long)lum_green_weight +
	     blue * (unsigned long)lum_blue_weight +
	     (lum_all_weights / 2))
		/ lum_all_weights;

    color = (gx_max_color_value - gray) >> drop;	/* color is in K channel */
    return color;
}

/* Map color to RGB.  This has 3 separate cases, but since it is rarely */
/* used, we do a case test rather than providing 3 separate routines. */
private int
bit_map_color_rgb(gx_device * dev, gx_color_index color, gx_color_value rgb[3])
{
    int depth = dev->color_info.depth;
    int ncomp = REAL_NUM_COMPONENTS(dev);
    int bpc = depth / ncomp;
    uint mask = (1 << bpc) - 1;

#define cvalue(c) ((gx_color_value)((ulong)(c) * gx_max_color_value / mask))

    switch (ncomp) {
	case 1:		/* gray */
	    rgb[0] = rgb[1] = rgb[2] =
		(depth == 1 ? (color ? 0 : gx_max_color_value) :
		 cvalue(color));
	    break;
	case 3:		/* RGB */
	    {
		gx_color_index cshift = color;

		rgb[2] = cvalue(cshift & mask);
		cshift >>= bpc;
		rgb[1] = cvalue(cshift & mask);
		rgb[0] = cvalue(cshift >> bpc);
	    }
	    break;
	case 4:		/* CMYK */
	    /* Map CMYK back to RGB. */
	    {
		gx_color_index cshift = color;
		uint c, m, y, k;

		k = cshift & mask;
		cshift >>= bpc;
		y = cshift & mask;
		cshift >>= bpc;
		m = cshift & mask;
		c = cshift >> bpc;
		/* We use our improved conversion rule.... */
		rgb[0] = cvalue((mask - c) * (mask - k) / mask);
		rgb[1] = cvalue((mask - m) * (mask - k) / mask);
		rgb[2] = cvalue((mask - y) * (mask - k) / mask);
	    }
	    break;
    }
    return 0;
#undef cvalue
}

/* Map CMYK to color. */
private gx_color_index
bit_map_cmyk_color(gx_device * dev, gx_color_value cyan,
	gx_color_value magenta, gx_color_value yellow, gx_color_value black)
{
    int bpc = dev->color_info.depth / 4;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_index color =
    ((((((cyan >> drop) << bpc) +
	(magenta >> drop)) << bpc) +
      (yellow >> drop)) << bpc) +
    (black >> drop);

    return (color == gx_no_color_index ? color ^ 1 : color);
}

/* Get parameters.  We provide a default CRD. */
private int
bit_get_params(gx_device * pdev, gs_param_list * plist)
{
    int code, ecode;
    /*
     * The following is a hack to get the original num_components.
     * See comment above.
     */
    int real_ncomps = REAL_NUM_COMPONENTS(pdev);
    int ncomps = pdev->color_info.num_components;
    int forcemono = (ncomps == real_ncomps ? 0 : 1);

    /*
     * Temporarily set num_components back to the "real" value to avoid
     * confusing those that rely on it.
     */
    pdev->color_info.num_components = real_ncomps;

    ecode = gdev_prn_get_params(pdev, plist);
    code = sample_device_crd_get_params(pdev, plist, "CRDDefault");
	if (code < 0)
	    ecode = code;
    if ((code = param_write_int(plist, "ForceMono", &forcemono)) < 0) {
	ecode = code;
    }

    /* Restore the working num_components */
    pdev->color_info.num_components = ncomps;

    return ecode;
}

/* Set parameters.  We allow setting the number of bits per component. */
/* Also, ForceMono=1 forces monochrome output from RGB/CMYK devices. */
private int
bit_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_color_info save_info;
    int ncomps = pdev->color_info.num_components;
    int real_ncomps = REAL_NUM_COMPONENTS(pdev);
    int bpc = pdev->color_info.depth / real_ncomps;
    int v;
    int ecode = 0;
    int code;
    static const byte depths[4][8] = {
	{1, 2, 0, 4, 8, 0, 0, 8},
	{0},
	{4, 8, 0, 16, 16, 0, 0, 24},
	{4, 8, 0, 16, 32, 0, 0, 32}
    };
    const char *vname;

    /*
     * Temporarily set num_components back to the "real" value to avoid
     * confusing those that rely on it.
     */
    pdev->color_info.num_components = real_ncomps;

    if ((code = param_read_int(plist, (vname = "GrayValues"), &v)) != 1 ||
	(code = param_read_int(plist, (vname = "RedValues"), &v)) != 1 ||
	(code = param_read_int(plist, (vname = "GreenValues"), &v)) != 1 ||
	(code = param_read_int(plist, (vname = "BlueValues"), &v)) != 1
	) {
	if (code < 0)
	    ecode = code;
	else
	    switch (v) {
		case   2: bpc = 1; break;
		case   4: bpc = 2; break;
		case  16: bpc = 4; break;
		case  32: bpc = 5; break;
		case 256: bpc = 8; break;
		default:
		    param_signal_error(plist, vname,
				       ecode = gs_error_rangecheck);
	    }
    }

    switch (code = param_read_int(plist, (vname = "ForceMono"), &v)) {
    case 0:
	if (v == 1) {
	    ncomps = 1;
	    break;
	}
	else if (v == 0) {
	    ncomps = real_ncomps;
	    break;
	}
	code = gs_error_rangecheck;
    default:
	ecode = code;
	param_signal_error(plist, vname, ecode);
    case 1:
	break;
    }
    if (ecode < 0)
	return ecode;

    /*
     * Save the color_info in case gdev_prn_put_params fails, and for
     * comparison.  Note that depth is computed from real_ncomps.
     */
    save_info = pdev->color_info;
    pdev->color_info.depth = depths[real_ncomps - 1][bpc - 1];
    pdev->color_info.max_gray = pdev->color_info.max_color =
	(pdev->color_info.dither_grays =
	 pdev->color_info.dither_colors =
	 (1 << bpc)) - 1;
    ecode = gdev_prn_put_params(pdev, plist);
    if (ecode < 0) {
	pdev->color_info = save_info;
	return ecode;
    }
    /* Now restore/change num_components. This is done after other	*/
    /* processing since it is used in gx_default_put_params		*/
    pdev->color_info.num_components = ncomps;
    if (pdev->color_info.depth != save_info.depth ||
	pdev->color_info.num_components != save_info.num_components
	) {
	gs_closedevice(pdev);
    }
    /* Reset the map_cmyk_color procedure if appropriate. */
    if (dev_proc(pdev, map_cmyk_color) == cmyk_1bit_map_cmyk_color ||
	dev_proc(pdev, map_cmyk_color) == cmyk_8bit_map_cmyk_color ||
	dev_proc(pdev, map_cmyk_color) == bit_map_cmyk_color) {
	set_dev_proc(pdev, map_cmyk_color,
		     pdev->color_info.depth == 4 ? cmyk_1bit_map_cmyk_color :
		     pdev->color_info.depth == 32 ? cmyk_8bit_map_cmyk_color :
		     bit_map_cmyk_color);
    }
    return 0;
}

/* Send the page to the printer. */
private int
bit_print_page(gx_device_printer * pdev, FILE * prn_stream)
{				/* Just dump the bits on the file. */
    /* If the file is 'nul', don't even do the writes. */
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    byte *in = gs_alloc_bytes(pdev->memory, line_size, "bit_print_page(in)");
    byte *data;
    int nul = !strcmp(pdev->fname, "nul");
    int lnum = 0, bottom = pdev->height;

    if (in == 0)
	return_error(gs_error_VMerror);
    for (; lnum < bottom; ++lnum) {
	gdev_prn_get_bits(pdev, lnum, in, &data);
	if (!nul)
	    fwrite(data, 1, line_size, prn_stream);
    }
    gs_free_object(pdev->memory, in, "bit_print_page(in)");
    return 0;
}
