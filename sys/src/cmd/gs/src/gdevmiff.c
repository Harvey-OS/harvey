/* Copyright (C) 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevmiff.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* MIFF file format driver */
#include "gdevprn.h"

/* ------ The device descriptor ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

private dev_proc_print_page(miff24_print_page);

private const gx_device_procs miff24_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);
gx_device_printer gs_miff24_device =
prn_device(miff24_procs, "miff24",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   24, miff24_print_page);

/* Print one page in 24-bit RLE direct color format. */
private int
miff24_print_page(gx_device_printer * pdev, FILE * file)
{
    int raster = gx_device_raster((gx_device *) pdev, true);
    byte *line = gs_alloc_bytes(pdev->memory, raster, "miff line buffer");
    int y;
    int code = 0;		/* return code */

    if (line == 0)		/* can't allocate line buffer */
	return_error(gs_error_VMerror);
    fputs("id=ImageMagick\n", file);
    fputs("class=DirectClass\n", file);
    fprintf(file, "columns=%d\n", pdev->width);
    fputs("compression=RunlengthEncoded\n", file);
    fprintf(file, "rows=%d\n", pdev->height);
    fputs(":\n", file);
    for (y = 0; y < pdev->height; ++y) {
	byte *row;
	byte *end;

	code = gdev_prn_get_bits(pdev, y, line, &row);
	if (code < 0)
	    break;
	end = row + pdev->width * 3;
	while (row < end) {
	    int count = 0;

	    while (count < 255 && row < end - 3 &&
		   row[0] == row[3] && row[1] == row[4] &&
		   row[2] == row[5]
		)
		++count, row += 3;
	    putc(row[0], file);
	    putc(row[1], file);
	    putc(row[2], file);
	    putc(count, file);
	    row += 3;
	}
    }
    gs_free_object(pdev->memory, line, "miff line buffer");

    return code;
}
