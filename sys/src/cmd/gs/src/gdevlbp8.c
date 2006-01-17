/* Copyright (C) 1991, 1994, 1996, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevlbp8.c,v 1.6 2002/10/07 08:28:56 ghostgum Exp $*/
/* Canon LBP-8II and LIPS III driver */
#include "gdevprn.h"

/* 
  Modifications:
    2.2.97  Lauri Paatero
            Changed CSI command into ESC [. DCS commands may still need to be changed
            (to ESC P).
    4.9.96  Lauri Paatero
	    Corrected LBP-8II margins again. Real problem was that (0,0) is NOT 
                in upper left corner.
	    Now using relative addressing for vertical addressing. This avoids
problems
                when printing to paper with wrong size.
    18.6.96 Lauri Paatero, lauri.paatero@paatero.pp.fi
            Corrected LBP-8II margins.
            Added logic to recognize (and optimize away) long strings of 00's in data.
            For LBP-8II removed use of 8-bit CSI (this does not work if 8-bit character
                set has been configured in LBP-8II. (Perhaps this should also be done
                for LBP-8III?)
  Original versions:
    LBP8 driver: Tom Quinn (trq@prg.oxford.ac.uk)
    LIPS III driver: Kenji Okamoto (okamoto@okamoto.cias.osakafu-u.ac.jp)
*/


#define X_DPI 300
#define Y_DPI 300
#define LINE_SIZE ((X_DPI * 85 / 10 + 7) / 8)	/* bytes per line */

/* The device descriptors */
private dev_proc_print_page(lbp8_print_page);
private dev_proc_print_page(lips3_print_page);

const gx_device_printer far_data gs_lbp8_device =
  prn_device(prn_std_procs, "lbp8",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0.16, 0.2, 0.32, 0.21,		/* margins: left, bottom, right, top */
	1, lbp8_print_page);

const gx_device_printer far_data gs_lips3_device =
  prn_device(prn_std_procs, "lips3",
	82,				/* width_10ths, 8.3" */
	117,				/* height_10ths, 11.7" */
	X_DPI, Y_DPI,
	0.16, 0.27, 0.23, 0.27,		/* margins */
	1, lips3_print_page);

/* ------ Internal routines ------ */

#define ESC (char)0x1b
#define CSI '\233'
#define DCS '\220'
#define ST '\234'

static const char lbp8_init[] = {
  ESC, ';', ESC, 'c', ESC, ';', /* reset, ISO */
  ESC, '[', '2', '&', 'z',	/* fullpaint mode */
  ESC, '[', '1', '4', 'p',	/* select page type (A4) */
  ESC, '[', '1', '1', 'h',	/* set mode */
  ESC, '[', '7', ' ', 'I',	/* select unit size (300dpi)*/
  ESC, '[', '6', '3', 'k', 	/* Move 63 dots up (to top of printable area) */ 
};

static const char *lbp8_end = NULL;

static const char lips3_init[] = {
  ESC, '<', /* soft reset */
  DCS, '0', 'J', ST, /* JOB END */
  DCS, '3', '1', ';', '3', '0', '0', ';', '2', 'J', ST, /* 300dpi, LIPS3 JOB START */
  ESC, '<',  /* soft reset */
  DCS, '2', 'y', 'P', 'r', 'i', 'n', 't', 'i', 'n', 'g', '(', 'g', 's', ')', ST,  /* Printing (gs) display */
  ESC, '[', '?', '1', 'l',  /* auto cr-lf disable */
  ESC, '[', '?', '2', 'h', /* auto ff disable */
  ESC, '[', '1', '1', 'h', /* set mode */
  ESC, '[', '7', ' ', 'I', /* select unit size (300dpi)*/
  ESC, '[', 'f' /* move to home position */
};

static const char lips3_end[] = {
  DCS, '0', 'J', ST  /* JOB END */
};

/* Send the page to the printer.  */
private int
can_print_page(gx_device_printer *pdev, FILE *prn_stream,
  const char *init, int init_size, const char *end, int end_size)
{	
	char data[LINE_SIZE*2];
	char *out_data;
	int last_line_nro = 0;

	fwrite(init, init_size, 1, prn_stream);		/* initialize */

	/* Send each scan line in turn */
	{	
	    int lnum;
	    int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	    byte rmask = (byte)(0xff << (-pdev->width & 7));

	    for ( lnum = 0; lnum < pdev->height; lnum++ ) {
    		char *end_data = data + LINE_SIZE;
		gdev_prn_copy_scan_lines(pdev, lnum,
					 (byte *)data, line_size);
	   	/* Mask off 1-bits beyond the line width. */
		end_data[-1] &= rmask;
		/* Remove trailing 0s. */
		while ( end_data > data && end_data[-1] == 0 )
			end_data--;
		if ( end_data != data ) {
		    int num_cols = 0;
		    int out_count;
		    int zero_count;
		    out_data = data;

		    /* move down */
		    fprintf(prn_stream, "%c[%de", 
			    ESC, lnum-last_line_nro );
		    last_line_nro = lnum;

		    while (out_data < end_data) {
			/* Remove leading 0s*/
			while(out_data < end_data && *out_data == 0) {	
		            num_cols += 8;
                            out_data++;
                        }

			out_count = end_data - out_data;
			zero_count = 0;
			
			/* if there is a lot data, find if there is sequence of zeros */
			if (out_count>22) {

				out_count = 1;

				while(out_data+out_count+zero_count < end_data) {
					if (out_data[zero_count+out_count] != 0) {
						out_count += 1+zero_count;
						zero_count = 0;
					}
					else {
						zero_count++;
						if (zero_count>20)
							break;
					}
				}

			}
	
			if (out_count==0)
				break;

			/* move down and across*/
			fprintf(prn_stream, "%c[%d`", 
				ESC, num_cols );
			/* transfer raster graphic command */
			fprintf(prn_stream, "%c[%d;%d;300;.r",
				ESC, out_count, out_count);

			/* send the row */
			fwrite(out_data, sizeof(char),
                               out_count, prn_stream);

			out_data += out_count+zero_count;
               	        num_cols += 8*(out_count+zero_count);
		    }
		}
	    }
	}

	/* eject page */
	fprintf(prn_stream, "%c=", ESC);

	/* terminate */
	if (end != NULL)
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
