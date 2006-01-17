/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevcif.c,v 1.6 2004/08/10 13:02:36 stefan Exp $*/
/*
  CIF output driver

   The `Fake bitmapped device to estimate rendering time' 
   slightly modified to produce CIF files from PostScript.
   So anyone can put a nice logo free on its chip!
   Frederic Petrot, petrot@masi.ibp.fr */

#include "gdevprn.h"

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* The device descriptor */
private dev_proc_print_page(cif_print_page);
const gx_device_printer far_data gs_cif_device =
  prn_device(prn_std_procs, "cif",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,
	1, cif_print_page);

/* Send the page to the output. */
private int
cif_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	int lnum;
	byte *in = (byte *)gs_malloc(pdev->memory, line_size, 1, "cif_print_page(in)");
	char *s;
	int scanline, scanbyte;
	int length, start; /* length is the number of successive 1 bits, */
			   /* start is the set of 1 bit start position */

	if (in == 0)
		return_error(gs_error_VMerror);

	if ((s = strchr(pdev->fname, '.')) == NULL)
		length = strlen(pdev->fname) + 1;
	else
		length = s - pdev->fname;
	s = (char *)gs_malloc(pdev->memory, length, sizeof(char), "cif_print_page(s)");

	strncpy(s, pdev->fname, length);
	*(s + length) = '\0';
	fprintf(prn_stream, "DS1 25 1;\n9 %s;\nLCP;\n", s);
	gs_free(pdev->memory, s, length, 1, "cif_print_page(s)");

   for (lnum = 0; lnum < pdev->height; lnum++) {   
      gdev_prn_copy_scan_lines(pdev, lnum, in, line_size);
      length = 0;
      for (scanline = 0; scanline < line_size; scanline++)
#ifdef TILE			/* original, simple, inefficient algorithm */
         for (scanbyte = 0; scanbyte < 8; scanbyte++)
            if (((in[scanline] >> scanbyte) & 1) != 0)
               fprintf(prn_stream, "B4 4 %d %d;\n",
                  (scanline * 8 + (7 - scanbyte)) * 4,
                  (pdev->height - lnum) * 4);
#else				/* better algorithm */
         for (scanbyte = 7; scanbyte >= 0; scanbyte--)
            /* cheap linear reduction of rectangles in lines */
            if (((in[scanline] >> scanbyte) & 1) != 0) {
               if (length == 0)
                  start = (scanline * 8 + (7 - scanbyte));
               length++;
            } else {
               if (length != 0)
                  fprintf(prn_stream, "B%d 4 %d %d;\n", length * 4,
                           start * 4 + length * 2,
                           (pdev->height - lnum) * 4);
               length = 0;
            }
#endif
   }
	fprintf(prn_stream, "DF;\nC1;\nE\n");
	gs_free(pdev->memory, in, line_size, 1, "cif_print_page(in)");
	return 0;
}
