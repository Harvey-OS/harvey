/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevp2up.c,v 1.6 2004/05/26 04:10:58 dan Exp $ */
/* A "2-up" PCX device for testing page objects. */
#include "gdevprn.h"
#include "gdevpccm.h"
#include "gxclpage.h"

extern gx_device_printer gs_pcx256_device;

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

/*
 * Define the size of the rendering buffer.
 */
#define RENDER_BUFFER_SPACE 500000

/* This device only supports SVGA 8-bit color. */

private dev_proc_open_device(pcx2up_open);
private dev_proc_print_page(pcx2up_print_page);

typedef struct gx_device_2up_s {
    gx_device_common;
    gx_prn_device_common;
    bool have_odd_page;
    gx_saved_page odd_page;
} gx_device_2up;

private const gx_device_procs pcx2up_procs =
prn_color_procs(pcx2up_open, gdev_prn_output_page, gdev_prn_close,
		pc_8bit_map_rgb_color, pc_8bit_map_color_rgb);
gx_device_2up gs_pcx2up_device =
{prn_device_body(gx_device_2up, pcx2up_procs, "pcx2up",
		 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		 X_DPI, Y_DPI,
		 0, 0, 0, 0,	/* margins */
		 3, 8, 5, 5, 6, 6, pcx2up_print_page)
};

/* Open the device.  We reimplement this to force banding with */
/* delayed rasterizing. */
private int
pcx2up_open(gx_device * dev)
{
    gx_device_printer *pdev = (gx_device_printer *) dev;
    int code;
    gdev_prn_space_params save_params;

    save_params = pdev->space_params;
    pdev->space_params.MaxBitmap = 0;	/* force banding */
    pdev->space_params.band.BandWidth =
	dev->width * 2 + (int)(dev->HWResolution[0] * 2.0);
    pdev->space_params.band.BandBufferSpace = RENDER_BUFFER_SPACE;
    code = gdev_prn_open(dev);
    pdev->space_params = save_params;
    ((gx_device_2up *) dev)->have_odd_page = false;
    return code;
}

/* Write the page. */
private int
pcx2up_print_page(gx_device_printer * pdev, FILE * file)
{
    gx_device_2up *pdev2 = (gx_device_2up *) pdev;
    const gx_device_printer *prdev_template =
    (const gx_device_printer *)&gs_pcx2up_device;

    if (!pdev2->have_odd_page) {	/* This is the odd page, just save it. */
	pdev2->have_odd_page = true;
	return gdev_prn_save_page(pdev, &pdev2->odd_page, 1);
    } else {			/* This is the even page, do 2-up output. */
	gx_saved_page even_page;
	gx_placed_page pages[2];
	int x_offset = (int)(pdev->HWResolution[0] * 0.5);
	int y_offset = (int)(pdev->HWResolution[1] * 0.5);
	int code = gdev_prn_save_page(pdev, &even_page, 1);
	int prdev_size = prdev_template->params_size;
	gx_device_printer *prdev;

#define rdev ((gx_device *)prdev)

	if (code < 0)
	    return code;
	/* Create the placed page list. */
	pages[0].page = &pdev2->odd_page;
	pages[0].offset.x = x_offset;
	pages[0].offset.y = 0 /*y_offset */ ;
	pages[1].page = &even_page;
	pages[1].offset.x = pdev->width + x_offset * 3;
	pages[1].offset.y = 0 /*y_offset */ ;
	/* Create and open a device for rendering. */
	prdev = (gx_device_printer *)
	    gs_alloc_bytes(pdev->memory, prdev_size,
			   "pcx2up_print_page(device)");
	if (prdev == 0)
	    return_error(gs_error_VMerror);
	memcpy(prdev, prdev_template, prdev_size);
        check_device_separable((gx_device *)rdev);
	gx_device_fill_in_procs(rdev);
	set_dev_proc(prdev, open_device,
		     dev_proc(&gs_pcx256_device, open_device));
	prdev->printer_procs.print_page =
	    gs_pcx256_device.printer_procs.print_page;
	prdev->space_params.band =
	    pages[0].page->info.band_params;	/* either one will do */
	prdev->space_params.MaxBitmap = 0;
	prdev->space_params.BufferSpace =
	    prdev->space_params.band.BandBufferSpace;
	prdev->width = prdev->space_params.band.BandWidth;
	prdev->OpenOutputFile = false;
	code = (*dev_proc(rdev, open_device)) (rdev);
	if (code < 0)
	    return code;
	rdev->is_open = true;
	prdev->file = pdev->file;
	/* Render the pages. */
	code = gdev_prn_render_pages(prdev, pages, 2);
	/* Clean up. */
	if (pdev->file != 0)
	    prdev->file = 0;	/* don't close it */
	gs_closedevice(rdev);
	pdev2->have_odd_page = false;
	return code;
#undef rdev
    }
}
