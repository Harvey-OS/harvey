/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevcslw.c,v 1.2 2000/09/19 19:00:12 lpd Exp $ */
/* CoStar LabelWriter II, II Plus driver for Ghostscript */
/* Contributed by Mike McCauley mikem@open.com.au        */

#include "gdevprn.h"

/* We round up the LINE_SIZE to a multiple of a ulong for faster scanning. */
typedef ulong word;
#define W sizeof(word)

/* Printer types */
#define LW	0

/* The device descriptors */

private dev_proc_print_page(coslw_print_page);

const gx_device_printer gs_coslw2p_device =
prn_device(prn_std_procs, "coslw2p",
	   200, 400,    /* 2 inches wide */
	   128, 128,    /* 5 dots per mm */
	   0, 0, 0, 0,
	   1, coslw_print_page);

const gx_device_printer gs_coslwxl_device =
prn_device(prn_std_procs, "coslwxl",
	   200, 400,    /* 2 inches wide */
	   204, 204,    /* 8 dots per mm */
	   0, 0, 0, 0,
	   1, coslw_print_page);

/* ------ Internal routines ------ */

/* Send the page to the printer. */
private int
coslw_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    int line_size_words = (line_size + W - 1) / W;
    uint storage_size_words = line_size_words * 8;	/* data, out_row, out_row_alt, prev_row */
    word *storage = (ulong *) gs_malloc(storage_size_words, W,
					"coslw_print_page");

    word *data_words;
#define data ((byte *)data_words)

    byte *out_data;
    int num_rows = dev_print_scan_lines(pdev);
    int bytes_per_line = 0;
    int out_count;
    int code = 0;

    if (storage == 0)		/* can't allocate working area */
	return_error(gs_error_VMerror);
    data_words = storage;

    /* Clear temp storage */
    memset(data, 0, storage_size_words * W);

    /* Initialize printer. */
    if (pdev->PageCount == 0) {
    }

    /* Put out per-page initialization. */

    /* End raster graphics, position cursor at top. */

    /* Send each scan line in turn */
    {
	int lnum;
	int num_blank_lines = 0;
	word rmask = ~(word) 0 << (-pdev->width & (W * 8 - 1));

	/* Transfer raster graphics. */
	for (lnum = 0; lnum < num_rows; lnum++) {
	    register word *end_data =
	    data_words + line_size_words;

	    code = gdev_prn_copy_scan_lines(pdev, lnum,
					    (byte *) data, line_size);
	    if (code < 0)
		break;
	    /* Mask off 1-bits beyond the line width. */
	    end_data[-1] &= rmask;
	    /* Remove trailing 0s. */
	    while (end_data > data_words && end_data[-1] == 0)
		end_data--;
	    if (end_data == data_words) {	/* Blank line */
		num_blank_lines++;
		continue;
	    }

	    /* We've reached a non-blank line. */
	    /* Put out a spacing command if necessary. */
	    while (num_blank_lines > 0)
	    {
		int this_blank = 255;
		if (num_blank_lines < this_blank)
		    this_blank = num_blank_lines;
		fprintf(prn_stream, "\033f\001%c", this_blank);
		num_blank_lines -= this_blank;
	    }

	    /* Perhaps add compression here later? */
	    out_data = data;
	    out_count = (byte *) end_data - data;

	    /* For 2 inch model, max width is 56 bytes */
	    if (out_count > 56)
		out_count = 56;  
	    /* Possible change the bytes per line */
	    if (bytes_per_line != out_count)
	    {
		fprintf(prn_stream, "\033D%c", out_count);
		bytes_per_line = out_count;
	    }

	    /* Transfer the data */
	    fputs("\026", prn_stream);
	    fwrite(out_data, sizeof(byte), out_count, prn_stream);
	}
    }

    /* eject page */
    fputs("\033E", prn_stream);

    /* free temporary storage */
    gs_free((char *)storage, storage_size_words, W, "coslw_print_page");

    return code;
}
