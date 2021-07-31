/* Copyright (C) 1991, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevlbp8.c */
/* Canon LBP-8II and LIPS III driver */
#include "gdevprn.h"

#define X_DPI 300
#define Y_DPI 300
#define LINE_SIZE ((X_DPI * 85 / 10 + 7) / 8)	/* bytes per line */

/* The device descriptors */
private dev_proc_print_page(lbp8_print_page);
private dev_proc_print_page(lips3_print_page);

gx_device_printer far_data gs_lbp8_device =
  prn_device(prn_std_procs, "lbp8",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0.16, 0.20, 0.32, 0.20,		/* margins */
	1, lbp8_print_page);

gx_device_printer far_data gs_lips3_device =
  prn_device(prn_std_procs, "lips3",
	82,				/* width_10ths, 8.3" */
	117,				/* height_10ths, 11.7" */
	X_DPI, Y_DPI,
	0.16, 0.27, 0.23, 0.27,		/* margins */
	1, lips3_print_page);

/* ------ Internal routines ------ */

#define ESC 0x1b
#define CSI 0233
#define DCS 0220
#define ST 0234

static const char lbp8_init[] = {
  ESC, ';', ESC, 'c', ESC, ';', /* reset, ISO */
  CSI, '2', '&', 'z',	/* fullpaint mode */
  CSI, '1', '4', 'p',	/* select page type (A4) */
  CSI, '1', '1', 'h',	/* set mode */
  CSI, '7', ' ', 'I',	/* select unit size (300dpi)*/
};

static const char lbp8_end[] = {
};

static const char lips3_init[] = {
  ESC, '<', /* soft reset */
  DCS, '0', 'J', ST, /* JOB END */
  DCS, '3', '1', ';', '3', '0', '0', ';', '2', 'J', ST, /* 300dpi, LIPS3 JOB START */
  ESC, '<',  /* soft reset */
  DCS, '2', 'y', 'P', 'r', 'i', 'n', 't', 'i', 'n', 'g', '(', 'g', 's', ')', ST,  /* Printing (gs) display */
  CSI, '?', '1', 'l',  /* auto cr-lf disable */
  CSI, '?', '2', 'h', /* auto ff disable */
  CSI, '1', '1', 'h', /* set mode */
  CSI, '7', ' ', 'I', /* select unit size (300dpi)*/
  CSI, 'f' /* move to home position */
};

static const char lips3_end[] = {
  DCS, '0', 'J', ST  /* JOB END */
};

/* Send the page to the printer.  */
private int
can_print_page(gx_device_printer *pdev, FILE *prn_stream,
  const char *init, int init_size, const char *end, int end_size)
{	char data[LINE_SIZE*2];
	char *out_data;
	int out_count;

#define CSI_print(str) fprintf(prn_stream, str, CSI)
#define CSI_print_1(str,arg) fprintf(prn_stream, str, CSI, arg)

	fwrite(init, init_size, 1, prn_stream);		/* initialize */

	/* Send each scan line in turn */
	   {	int lnum;
		int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
		byte rmask = (byte)(0xff << (-pdev->width & 7));

		for ( lnum = 0; lnum < pdev->height; lnum++ )
		   {	char *end_data = data + LINE_SIZE;
			gdev_prn_copy_scan_lines(pdev, lnum,
						 (byte *)data, line_size);
		   	/* Mask off 1-bits beyond the line width. */
			end_data[-1] &= rmask;
			/* Remove trailing 0s. */
			while ( end_data > data && end_data[-1] == 0 )
				end_data--;
			if ( end_data != data )
			   {	
				int num_cols = 0;

				out_data = data;
				while(out_data < end_data && *out_data == 0)
                                  {
                                    num_cols += 8;
                                    out_data++;
                                  }
				out_count = end_data - out_data;

				/* move down */
				CSI_print_1("%c%dd", lnum);
				/* move across */
				CSI_print_1("%c%d`", num_cols);

				/* transfer raster graphics */
				fprintf(prn_stream, "%c%d;%d;300;.r",
					CSI, out_count, out_count);

				/* send the row */
				fwrite(out_data, sizeof(char),
                                       out_count, prn_stream);
			   }
		   }
	}

	/* eject page */
	fprintf(prn_stream, "%c=", ESC);

	/* terminate */
	fwrite(end, end_size, 1, prn_stream);

	return 0;
}

/* Print an LBP-8 page. */
private int
lbp8_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	return can_print_page(pdev, prn_stream, lbp8_init, sizeof(lbp8_init),
			      lbp8_end, sizeof(lbp8_end));
}

/* Print a LIPS III page. */
private int
lips3_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	return can_print_page(pdev, prn_stream, lips3_init, sizeof(lips3_init),
			      lips3_end, sizeof(lips3_end));
}
