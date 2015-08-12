/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1991, 1996 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdev4081.c,v 1.6 2004/08/04 23:33:29 stefan Exp $*/
/* Ricoh 4081 laser printer driver */
#include "gdevprn.h"

#define X_DPI 300 /* pixels per inch */
#define Y_DPI 300 /* pixels per inch */

/* The device descriptor */
private
dev_proc_print_page(r4081_print_page);
const gx_device_printer far_data gs_r4081_device =
    prn_device(prn_std_procs, "r4081",
	       85,  /* width_10ths, 8.5" */
	       110, /* height_10ths, 11" */
	       X_DPI, Y_DPI,
	       0.25, 0.16, 0.25, 0.16, /* margins */
	       1, r4081_print_page);

/* ------ Internal routines ------ */

/* Send the page to the printer. */
private
int
r4081_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	int out_size = ((pdev->width + 7) & -8);
	byte *out = (byte *)gs_malloc(pdev->memory, out_size, 1, "r4081_print_page(out)");
	int lnum = 0;
	int last = pdev->height;

	/* Check allocations */
	if(out == 0) {
		if(out)
			gs_free(pdev->memory, (char *)out, out_size, 1,
				"r4081_print_page(out)");
		return -1;
	}

	/* find the first line which has something to print */
	while(lnum < last) {
		gdev_prn_copy_scan_lines(pdev, lnum, (byte *)out, line_size);
		if(out[0] != 0 ||
		   memcmp((char *)out, (char *)out + 1, line_size - 1))
			break;
		lnum++;
	}

	/* find the last line which has something to print */
	while(last > lnum) {
		gdev_prn_copy_scan_lines(pdev, last - 1, (byte *)out, line_size);
		if(out[0] != 0 ||
		   memcmp((char *)out, (char *)out + 1, line_size - 1))
			break;
		last--;
	}

	/* Initialize the printer and set the starting position. */
	fprintf(prn_stream, "\033\rP\033\022YB2 \033\022G3,%d,%d,1,1,1,%d@",
		out_size, last - lnum, (lnum + 1) * 720 / Y_DPI);

	/* Print lines of graphics */
	while(lnum < last) {
		gdev_prn_copy_scan_lines(pdev, lnum, (byte *)out, line_size);
		fwrite(out, sizeof(char), line_size, prn_stream);
		lnum++;
	}

	/* Eject the page and reinitialize the printer */
	fputs("\f\033\rP", prn_stream);

	gs_free(pdev->memory, (char *)out, out_size, 1, "r4081_print_page(out)");
	return 0;
}
