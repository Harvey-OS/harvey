/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.

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

/* $Id: gdevdljm.c,v 1.11 2004/01/29 18:19:41 ray Exp $ */
/* Generic monochrome H-P DeskJet/LaserJet driver */
#include "gdevprn.h"
#include "gdevdljm.h"

/*
 * Thanks for various improvements to:
 *      Jim Mayer (mayer@wrc.xerox.com)
 *      Jan-Mark Wams (jms@cs.vu.nl)
 *      Frans van Hoesel (hoesel@chem.rug.nl)
 *      George Cameron (g.cameron@biomed.abdn.ac.uk)
 *      Nick Duffek (nsd@bbc.com)
 * Thanks for the FS-600 driver to:
 *	Peter Schildmann (peter.schildmann@etechnik.uni-rostock.de)
 * Thanks for the LJIIID duplex capability to:
 *      PDP (Philip) Brown (phil@3soft-uk.com)
 * Thanks for the OCE 9050 driver to:
 *      William Bader (wbader@EECS.Lehigh.Edu)
 * Thanks for the LJ4D duplex capability to:
 *	Les Johnson <les@infolabs.com>
 */

/* See gdevdljm.h for the definitions of the PCL_ features. */

/* The number of blank lines that make it worthwhile to reposition */
/* the cursor. */
#define MIN_SKIP_LINES 7

/* We round up the LINE_SIZE to a multiple of a ulong for faster scanning. */
#define W sizeof(word)

/* Send a page to the printer. */
int
dljet_mono_print_page(gx_device_printer * pdev, FILE * prn_stream,
		      int dots_per_inch, int features, const char *page_init)
{
    return dljet_mono_print_page_copies(pdev, prn_stream, 1, dots_per_inch,
					features, page_init);
}
int
dljet_mono_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			     int num_copies, int dots_per_inch, int features,
			     const char *page_init)
{
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    int line_size_words = (line_size + W - 1) / W;
    uint storage_size_words = line_size_words * 8;	/* data, out_row, out_row_alt, prev_row */
    word *storage;
    word
	*data_words,
	*out_row_words,
	*out_row_alt_words,
	*prev_row_words;
#define data ((byte *)data_words)
#define out_row ((byte *)out_row_words)
#define out_row_alt ((byte *)out_row_alt_words)
#define prev_row ((byte *)prev_row_words)
    byte *out_data;
    int x_dpi = (int)pdev->x_pixels_per_inch;
    int y_dpi = (int)pdev->y_pixels_per_inch;
    int y_dots_per_pixel = dots_per_inch / y_dpi;
    int num_rows = dev_print_scan_lines(pdev);

    int out_count;
    int compression = -1;
    static const char *const from2to3 = "\033*b3M";
    static const char *const from3to2 = "\033*b2M";
    int penalty_from2to3 = strlen(from2to3);
    int penalty_from3to2 = strlen(from3to2);
    int paper_size = gdev_pcl_paper_size((gx_device *) pdev);
    int code = 0;
    bool dup = pdev->Duplex;
    bool dupset = pdev->Duplex_set >= 0;

    if (num_copies != 1 && !(features & PCL_CAN_PRINT_COPIES))
	return gx_default_print_page_copies(pdev, prn_stream, num_copies);
    storage =
	(ulong *)gs_alloc_byte_array(pdev->memory, storage_size_words, W,
				     "hpjet_print_page");
    if (storage == 0)		/* can't allocate working area */
	return_error(gs_error_VMerror);
    data_words = storage;
    out_row_words = data_words + (line_size_words * 2);
    out_row_alt_words = out_row_words + (line_size_words * 2);
    prev_row_words = out_row_alt_words + (line_size_words * 2);
    /* Clear temp storage */
    memset(data, 0, storage_size_words * W);

    /* Initialize printer. */
    if (pdev->PageCount == 0) {
	fputs("\033E", prn_stream);	/* reset printer */
	/* If the printer supports it, set the paper size */
	/* based on the actual requested size. */
	if (features & PCL_CAN_SET_PAPER_SIZE) {
	    fprintf(prn_stream, "\033&l%dA", paper_size);
	}
	/* If printer can duplex, set duplex mode appropriately. */
	if (features & PCL_HAS_DUPLEX) {
	    if (dupset && dup)
		fputs("\033&l1S", prn_stream);
	    else if (dupset && !dup)
		fputs("\033&l0S", prn_stream);
	    else		/* default to duplex for this printer */
		fputs("\033&l1S", prn_stream);
	}
    }
    /* Put out per-page initialization. */
    if (features & PCL_CAN_SET_PAPER_SIZE){ 
        fprintf(prn_stream, "\033&l%dA", paper_size); 
    } 
    fputs("\033&l0o0l0E", prn_stream);
    fputs(page_init, prn_stream);
    fprintf(prn_stream, "\033&l%dX", num_copies);	/* # of copies */

    /* End raster graphics, position cursor at top. */
    fputs("\033*rB\033*p0x0Y", prn_stream);

    /* The DeskJet and DeskJet Plus reset everything upon */
    /* receiving \033*rB, so we must reinitialize graphics mode. */
    if (features & PCL_END_GRAPHICS_DOES_RESET) {
	fputs(page_init, prn_stream);
	fprintf(prn_stream, "\033&l%dX", num_copies);	/* # of copies */
    }

    /* Set resolution. */
    fprintf(prn_stream, "\033*t%dR", x_dpi);

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
	    if (num_blank_lines == lnum) {
		/* We're at the top of a page. */
		if (features & PCL_ANY_SPACING) {
		    if (num_blank_lines > 0)
			fprintf(prn_stream, "\033*p+%dY",
				num_blank_lines * y_dots_per_pixel);
		    /* Start raster graphics. */
		    fputs("\033*r1A", prn_stream);
		} else if (features & PCL_MODE_3_COMPRESSION) {
		    /* Start raster graphics. */
		    fputs("\033*r1A", prn_stream);
#if 1				/* don't waste paper */
		    if (num_blank_lines > 0)
			fputs("\033*b0W", prn_stream);
		    num_blank_lines = 0;
#else
		    for (; num_blank_lines; num_blank_lines--)
			fputs("\033*b0W", prn_stream);
#endif
		} else {
		    /* Start raster graphics. */
		    fputs("\033*r1A", prn_stream);
		    for (; num_blank_lines; num_blank_lines--)
			fputs("\033*bW", prn_stream);
		}
	    }
	    /* Skip blank lines if any */
	    else if (num_blank_lines != 0) {
		/*
		 * Moving down from current position causes head motion
		 * on the DeskJet, so if the number of lines is small,
		 * we're better off printing blanks.
		 */
		/*
		 * For Canon LBP4i and some others, <ESC>*b<n>Y doesn't
		 * properly clear the seed row if we are in compression mode
		 * 3.
		 */
		if ((num_blank_lines < MIN_SKIP_LINES && compression != 3) ||
		    !(features & PCL_ANY_SPACING)
		    ) {
		    bool mode_3ns =
			(features & PCL_MODE_3_COMPRESSION) &&
			!(features & PCL_ANY_SPACING);

		    if (mode_3ns && compression != 2) {
			/* Switch to mode 2 */
			fputs(from3to2, prn_stream);
			compression = 2;
		    }
		    if (features & PCL_MODE_3_COMPRESSION) {
			/* Must clear the seed row. */
			fputs("\033*b1Y", prn_stream);
			num_blank_lines--;
		    }
		    if (mode_3ns) {
			for (; num_blank_lines; num_blank_lines--)
			    fputs("\033*b0W", prn_stream);
		    } else {
			for (; num_blank_lines; num_blank_lines--)
			    fputs("\033*bW", prn_stream);
		    }
		} else if (features & PCL3_SPACING) {
		    fprintf(prn_stream, "\033*p+%dY",
			    num_blank_lines * y_dots_per_pixel);
		} else {
		    fprintf(prn_stream, "\033*b%dY",
			    num_blank_lines);
		}
		/* Clear the seed row (only matters for */
		/* mode 3 compression). */
		memset(prev_row, 0, line_size);
	    }
	    num_blank_lines = 0;

	    /* Choose the best compression mode */
	    /* for this particular line. */
	    if (features & PCL_MODE_3_COMPRESSION) {
		/* Compression modes 2 and 3 are both */
		/* available.  Try both and see which one */
		/* produces the least output data. */
		int count3 = gdev_pcl_mode3compress(line_size, data,
						    prev_row, out_row);
		int count2 = gdev_pcl_mode2compress(data_words, end_data,
						    out_row_alt);
		int penalty3 =
		    (compression == 3 ? 0 : penalty_from2to3);
		int penalty2 =
		    (compression == 2 ? 0 : penalty_from3to2);

		if (count3 + penalty3 < count2 + penalty2) {
		    if (compression != 3)
			fputs(from2to3, prn_stream);
		    compression = 3;
		    out_data = out_row;
		    out_count = count3;
		} else {
		    if (compression != 2)
			fputs(from3to2, prn_stream);
		    compression = 2;
		    out_data = out_row_alt;
		    out_count = count2;
		}
	    } else if (features & PCL_MODE_2_COMPRESSION) {
		out_data = out_row;
		out_count = gdev_pcl_mode2compress(data_words, end_data,
						   out_row);
	    } else {
		out_data = data;
		out_count = (byte *) end_data - data;
	    }

	    /* Transfer the data */
	    fprintf(prn_stream, "\033*b%dW", out_count);
	    fwrite(out_data, sizeof(byte), out_count,
		   prn_stream);
	}
    }

    /* end raster graphics and eject page */
    fputs("\033*rB\f", prn_stream);

    /* free temporary storage */
    gs_free_object(pdev->memory, storage, "hpjet_print_page");

    return code;
}
