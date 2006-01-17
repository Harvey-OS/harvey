/* Copyright (C) 1992, 1993, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevbmp.c,v 1.12 2005/10/20 19:42:18 ray Exp $ */
/* .BMP file format output drivers */
#include "gdevprn.h"
#include "gdevpccm.h"
#include "gdevbmp.h"

/* ------ The device descriptors ------ */

private dev_proc_print_page(bmp_print_page);
private dev_proc_print_page(bmp_cmyk_print_page);

/* Monochrome. */

const gx_device_printer gs_bmpmono_device =
prn_device(prn_std_procs, "bmpmono",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   1, bmp_print_page);

/* 8-bit (SuperVGA-style) grayscale . */
/* (Uses a fixed palette of 256 gray levels.) */

private const gx_device_procs bmpgray_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		gx_default_gray_map_rgb_color, gx_default_gray_map_color_rgb);
const gx_device_printer gs_bmpgray_device = {
  prn_device_body(gx_device_printer, bmpgray_procs, "bmpgray",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   1, 8, 255, 0, 256, 0, bmp_print_page)
};

/* 1-bit-per-plane separated CMYK color. */

#define bmp_cmyk_procs(p_map_color_rgb, p_map_cmyk_color)\
    gdev_prn_open, NULL, NULL, gdev_prn_output_page, gdev_prn_close,\
    NULL, p_map_color_rgb, NULL, NULL, NULL, NULL, NULL, NULL,\
    gdev_prn_get_params, gdev_prn_put_params,\
    p_map_cmyk_color, NULL, NULL, NULL, gx_page_device_get_page_device

private const gx_device_procs bmpsep1_procs = {
    bmp_cmyk_procs(cmyk_1bit_map_color_rgb, cmyk_1bit_map_cmyk_color)
};
const gx_device_printer gs_bmpsep1_device = {
  prn_device_body(gx_device_printer, bmpsep1_procs, "bmpsep1",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	4, 4, 1, 1, 2, 2, bmp_cmyk_print_page)
};

/* 8-bit-per-plane separated CMYK color. */

private const gx_device_procs bmpsep8_procs = {
    bmp_cmyk_procs(cmyk_8bit_map_color_rgb, cmyk_8bit_map_cmyk_color)
};
const gx_device_printer gs_bmpsep8_device = {
  prn_device_body(gx_device_printer, bmpsep8_procs, "bmpsep8",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	4, 32, 255, 255, 256, 256, bmp_cmyk_print_page)
};

/* 4-bit planar (EGA/VGA-style) color. */

private const gx_device_procs bmp16_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		pc_4bit_map_rgb_color, pc_4bit_map_color_rgb);
const gx_device_printer gs_bmp16_device = {
  prn_device_body(gx_device_printer, bmp16_procs, "bmp16",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   3, 4, 1, 1, 2, 2, bmp_print_page)
};

/* 8-bit (SuperVGA-style) color. */
/* (Uses a fixed palette of 3,3,2 bits.) */

private const gx_device_procs bmp256_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		pc_8bit_map_rgb_color, pc_8bit_map_color_rgb);
const gx_device_printer gs_bmp256_device = {
  prn_device_body(gx_device_printer, bmp256_procs, "bmp256", 
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   3, 8, 5, 5, 6, 6, bmp_print_page)
};

/* 24-bit color. */

private const gx_device_procs bmp16m_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		bmp_map_16m_rgb_color, bmp_map_16m_color_rgb);
const gx_device_printer gs_bmp16m_device =
prn_device(bmp16m_procs, "bmp16m",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   24, bmp_print_page);

/* 32-bit CMYK color (outside the BMP specification). */

private const gx_device_procs bmp32b_procs = {
    bmp_cmyk_procs(cmyk_8bit_map_color_rgb, gx_default_cmyk_map_cmyk_color)
};
const gx_device_printer gs_bmp32b_device =
prn_device(bmp32b_procs, "bmp32b",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   32, bmp_print_page);

/* ------ Private definitions ------ */

/* Write out a page in BMP format. */
/* This routine is used for all non-separated formats. */
private int
bmp_print_page(gx_device_printer * pdev, FILE * file)
{
    uint raster = gdev_prn_raster(pdev);
    /* BMP scan lines are padded to 32 bits. */
    uint bmp_raster = raster + (-(int)raster & 3);
    byte *row = gs_alloc_bytes(pdev->memory, bmp_raster, "bmp file buffer");
    int y;
    int code;		/* return code */

    if (row == 0)		/* can't allocate row buffer */
	return_error(gs_error_VMerror);
    memset(row+raster, 0, bmp_raster - raster); /* clear the padding bytes */

    /* Write the file header. */

    code = write_bmp_header(pdev, file);
    if (code < 0)
	goto done;

    /* Write the contents of the image. */
    /* BMP files want the image in bottom-to-top order! */

    for (y = pdev->height - 1; y >= 0; y--) {
	gdev_prn_copy_scan_lines(pdev, y, row, raster);
	fwrite((const char *)row, bmp_raster, 1, file);
    }

done:
    gs_free_object(pdev->memory, row, "bmp file buffer");

    return code;
}

/* Write out a page in separated CMYK format. */
/* This routine is used for all formats. */
private int
bmp_cmyk_print_page(gx_device_printer * pdev, FILE * file)
{
    int plane_depth = pdev->color_info.depth / 4;
    uint raster = (pdev->width * plane_depth + 7) >> 3;
    /* BMP scan lines are padded to 32 bits. */
    uint bmp_raster = raster + (-(int)raster & 3);
    byte *row = gs_alloc_bytes(pdev->memory, bmp_raster, "bmp file buffer");
    int y;
    int code = 0;		/* return code */
    int plane;

    if (row == 0)		/* can't allocate row buffer */
	return_error(gs_error_VMerror);
    memset(row+raster, 0, bmp_raster - raster); /* clear the padding bytes */
    
    for (plane = 0; plane <= 3; ++plane) {
	gx_render_plane_t render_plane;

	/* Write the page header. */

    	code = write_bmp_separated_header(pdev, file);
	if (code < 0)
	    break;

	/* Write the contents of the image. */
	/* BMP files want the image in bottom-to-top order! */

	gx_render_plane_init(&render_plane, (gx_device *)pdev, plane);
	for (y = pdev->height - 1; y >= 0; y--) {
	    byte *actual_data;
	    uint actual_raster;

	    code = gdev_prn_get_lines(pdev, y, 1, row, bmp_raster,
				      &actual_data, &actual_raster,
				      &render_plane);
	    if (code < 0)
		goto done;
	    fwrite((const char *)actual_data, bmp_raster, 1, file);
	}
    }

done:
    gs_free_object(pdev->memory, row, "bmp file buffer");

    return code;
}
