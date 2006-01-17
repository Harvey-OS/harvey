/* Copyright (C) 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevcljc.c,v 1.7 2004/05/26 04:10:58 dan Exp $ */
/*
 * H-P Color LaserJet 5/5M contone device; based on the gdevclj.c.
 */
#include "math_.h"
#include "gdevprn.h"
#include "gdevpcl.h"

/* X_DPI and Y_DPI must be the same */
#define X_DPI 300
#define Y_DPI 300

/* Send the page to the printer.  Compress each scan line.  NB - the
 * render mode as well as color parameters - bpp etc. are all
 * hardwired.
 */
private int
cljc_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    gs_memory_t *mem = pdev->memory;
    uint raster = gx_device_raster((gx_device *)pdev, false);
    int i;
    int worst_case_comp_size = raster + (raster / 8) + 1;
    byte *data = 0;
    byte *cdata = 0;
    byte *prow = 0;
    int code = 0;

    /* allocate memory for the raw data and compressed data.  */
    if (((data = gs_alloc_bytes(mem, raster, "cljc_print_page(data)")) == 0) ||
	((cdata = gs_alloc_bytes(mem, worst_case_comp_size, "cljc_print_page(cdata)")) == 0) ||
	((prow = gs_alloc_bytes(mem, worst_case_comp_size, "cljc_print_page(prow)")) == 0)) {
	code = gs_note_error(gs_error_VMerror);
	goto out;
    }
    /* send a reset and the the paper definition */
    fprintf(prn_stream, "\033E\033&u300D\033&l%dA",
	    gdev_pcl_paper_size((gx_device *) pdev));
    /* turn off source and pattern transparency */
    fprintf(prn_stream, "\033*v1N\033*v1O");
    /* set color render mode and the requested resolution */
    fprintf(prn_stream, "\033*t4J\033*t%dR", (int)(pdev->HWResolution[0]));
    /* set up the color model - NB currently hardwired to direct by
       pixel which requires 8 bits per component.  See PCL color
       technical reference manual for other possible encodings. */
    fprintf(prn_stream, "\033*v6W%c%c%c%c%c%c", 0, 3, 0, 8, 8, 8);
    /* set up raster width and height, compression mode 3 */
    fprintf(prn_stream, "\033&l0e-180u36Z\033*p0x0Y\033*r1A\033*b3M");
    /* initialize the seed row */
    memset(prow, 0, worst_case_comp_size);
    /* process each scanline */
    for (i = 0; i < pdev->height; i++) {
	int compressed_size;

	code = gdev_prn_copy_scan_lines(pdev, i, (byte *) data, raster);
	if (code < 0)
	    break;
	compressed_size = gdev_pcl_mode3compress(raster, data, prow, cdata);
	fprintf(prn_stream, "\033*b%dW", compressed_size);
	fwrite(cdata, sizeof(byte), compressed_size, prn_stream);
    }
    /* PCL will take care of blank lines at the end */
    fputs("\033*rC\f", prn_stream);
out:
    gs_free_object(mem, prow, "cljc_print_page(prow)");
    gs_free_object(mem, cdata, "cljc_print_page(cdata)");
    gs_free_object(mem, data, "cljc_print_page(data)");
    return code;
}

/* CLJ device methods */
private gx_device_procs cljc_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);

/* the CLJ device */
const gx_device_printer gs_cljet5c_device =
{
    prn_device_body(gx_device_printer, cljc_procs, "cljet5c",
		    85, 110, X_DPI, Y_DPI,
		    0.167, 0.167,
		    0.167, 0.167,
		    3, 24, 255, 255, 256, 256,
		    cljc_print_page)
};
