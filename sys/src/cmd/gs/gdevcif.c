/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevcif.c */
/* The `Fake bitmapped device to estimate rendering time' 
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
gx_device_printer far_data gs_cif_device =
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
	byte *in = (byte *)gs_malloc(line_size, 1, "cif_print_page(in)");
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
	s = (char *)gs_malloc(length, sizeof(char), "cif_print_page(s)");

	strncpy(s, pdev->fname, length);
	*(s + length) = '\0';
	fprintf(prn_stream, "DS1 25 1;\n9 %s;\nLCP;\n", s);
	gs_free(s, length, 1, "cif_print_page(s)");

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
	gs_free(in, line_size, 1, "cif_print_page(in)");
	return 0;
}
