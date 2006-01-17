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

/* $Id: gdevatx.c,v 1.9 2004/01/29 18:19:41 ray Exp $ */
/* Practical Automation ATX-23, -24, and -38 driver */
#include "math_.h"
#include "gdevprn.h"

/*
 * All of the ATX printers have an unprintable margin of 0.125" at the top
 * and bottom of the page.  They also have unprintable left/right margins:
 *	ATX-23	0.25"
 *	ATX-24	0.193"
 *	ATS-38	0.25"
 * The code below assumes that coordinates refer only to the *printable*
 * part of each page.  This is wrong and must eventually be changed.
 */

/* Define the printer commands. */
#define ATX_SET_PAGE_LENGTH "\033f"  /* + 2-byte length */
#define ATX_VERTICAL_TAB "\033L"  /* + 2-byte count */
#define ATX_UNCOMPRESSED_DATA "\033d"  /* + 2-byte count */
#define ATX_COMPRESSED_DATA "\033x"  /* + 1-byte word count */
#define ATX_END_PAGE "\033e"

/* The device descriptors */
private dev_proc_print_page(atx23_print_page);
private dev_proc_print_page(atx24_print_page);
private dev_proc_print_page(atx38_print_page);

#define ATX_DEVICE(dname, w10, h10, dpi, lrm, btm, print_page)\
  prn_device_margins(prn_std_procs, dname, w10, h10, dpi, dpi, 0, 0,\
		     lrm, btm, lrm, btm, 1, print_page)

const gx_device_printer gs_atx23_device = /* real width = 576 pixels */
ATX_DEVICE("atx23", 28 /* 2.84" */, 35 /* (minimum) */,
	   203, 0.25, 0.125, atx23_print_page);

const gx_device_printer gs_atx24_device = /* real width = 832 pixels */
ATX_DEVICE("atx24", 41 /* 4.1" */, 35 /* (minimum) */,
	   203, 0.193, 0.125, atx24_print_page);

const gx_device_printer gs_atx38_device = /* real width = 2400 pixels */
ATX_DEVICE("atx38", 80 /* 8.0" */, 35 /* (minimum) */,
	   300, 0.25, 0.125, atx38_print_page);

/* Output a printer command with a 2-byte, little-endian numeric argument. */
private void
fput_atx_command(FILE *f, const char *str, int value)
{
    fputs(str, f);
    fputc((byte)value, f);
    fputc((byte)(value >> 8), f);
}

/*
 * Attempt to compress a scan line of data.  in_size and out_size are even.
 * Return -1 if the compressed data would exceed out_size, otherwise the
 * size of the compressed data (always even).
 */
#define MIN_IN_SIZE_TO_COMPRESS 50
#define MAX_COMPRESSED_SEGMENT_PAIRS 127
#define MAX_UNCOMPRESSED_SEGMENT_PAIRS 255
#define COMPRESSED_SEGMENT_COMMAND 0x80	/* + # of repeated pairs */
#define UNCOMPRESSED_SEGMENT_COMMAND 0x7f /* followed by # of pairs */
private int
atx_compress(const byte *in_buf, int in_size, byte *out_buf, int out_size)
{
    const byte *const in_end = in_buf + in_size;
    byte *const out_end = out_buf + out_size;
    const byte *in = in_buf;
    byte *out = out_buf;
    byte *out_command;
    int pair_count;

    if (in_size < MIN_IN_SIZE_TO_COMPRESS)
	return -1;			/* not worth compressing */

    /* Start a new segment. */
 New_Segment:
    if (in == in_end)		/* end of input data */
	return out - out_buf;
    if (out == out_end)		/* output buffer full */
	return -1;
    out_command = out;
    out += 2;
    if (in[1] == in[0]) {		/* start compressed segment */
	/* out[-2] will be compressed segment command */
	out[-1] = in[0];
	pair_count = 1;
	goto Scan_Compressed_Pair;
    } else {			/* start uncompressed segment */
	out[-2] = UNCOMPRESSED_SEGMENT_COMMAND;
	/* out[-1] will be pair count */
	pair_count = 0;
	goto Scan_Uncompressed_Pair;
    }

    /* Scan compressed data. */
 Scan_Compressed:
    if (pair_count == MAX_COMPRESSED_SEGMENT_PAIRS ||
	in == in_end || in[0] != in[-1] || in[1] != in[0]
	) {			/* end the segment */
	out_command[0] = COMPRESSED_SEGMENT_COMMAND + pair_count;
	goto New_Segment;
    }
    ++pair_count;
 Scan_Compressed_Pair:
    in += 2;
    goto Scan_Compressed;

    /* Scan uncompressed data. */
 Scan_Uncompressed:
    if (pair_count == MAX_UNCOMPRESSED_SEGMENT_PAIRS ||
	in == in_end || in[1] == in[0]
	) {			/* end the segment */
	out_command[1] = pair_count;
	goto New_Segment;
    }
 Scan_Uncompressed_Pair:
    if (out == out_end)		/* output buffer full */
	return -1;
    out[0] = in[0], out[1] = in[1];
    in += 2;
    out += 2;
    ++pair_count;
    goto Scan_Uncompressed;
  
}

/* Send the page to the printer. */
private int
atx_print_page(gx_device_printer *pdev, FILE *f, int max_width_bytes)
{
    /*
     * The page length command uses 16 bits to represent the length in
     * units of 0.01", so the maximum representable page length is 
     * 655.35", including the unprintable top and bottom margins.
     * Compute the maximum height of the printable area in pixels.
     */
    float top_bottom_skip = (pdev->HWMargins[1] + pdev->HWMargins[3]) / 72.0;
    int max_height = (int)(pdev->HWResolution[1] * 655 - top_bottom_skip);
    int height = min(pdev->height, max_height);
    int page_length_100ths =
	(int)ceil((height / pdev->HWResolution[1] + top_bottom_skip) * 100);
    gs_memory_t *mem = pdev->memory;
    int raster = gx_device_raster((gx_device *)pdev, true);
    byte *buf;
    /*
     * ATX_COMPRESSED_DATA only takes a 1-byte (word) count.
     * Thus no compressed scan line can take more than 510 bytes.
     */
    int compressed_raster = min(raster / 2, 510); /* require 50% compression */
    byte *compressed;
    int blank_lines, lnum;
    int code = 0;

    /* Enforce a minimum 3" page length. */
    if (page_length_100ths < 300)
	page_length_100ths = 300;
    buf = gs_alloc_bytes(mem, raster, "atx_print_page(buf)");
    compressed = gs_alloc_bytes(mem, compressed_raster,
				"atx_print_page(compressed)");
    if (buf == 0 || compressed == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    fput_atx_command(f, ATX_SET_PAGE_LENGTH, page_length_100ths);
    for (blank_lines = 0, lnum = 0; lnum < height; ++lnum) {
	byte *row;
	byte *end;
	int count;

	gdev_prn_get_bits(pdev, lnum, buf, &row);
	/* Find the end of the non-blank data. */
	for (end = row + raster; end > row && end[-1] == 0 && end[-2] == 0; )
	    end -= 2;
	if (end == row) {		/* blank line */
	    ++blank_lines;
	    continue;
	}
	if (blank_lines) {		/* skip vertically */
	    fput_atx_command(f, ATX_VERTICAL_TAB, blank_lines + 1);
	    blank_lines = 0;
	}
	/* Truncate the line to the maximum printable width. */
	if (end - row > max_width_bytes)
	    end = row + max_width_bytes;
	count = atx_compress(row, end - row, compressed, compressed_raster);
	if (count >= 0) {		/* compressed line */
	    /*
	     * Note that since compressed_raster can't exceed 510, count
	     * can't exceed 510 either.
	     */
	    fputs(ATX_COMPRESSED_DATA, f);
	    fputc(count / 2, f);
	    fwrite(compressed, 1, count, f);
	} else {			/* uncompressed line */
	    int num_bytes = end - row;

	    fput_atx_command(f, ATX_UNCOMPRESSED_DATA, num_bytes);
	    fwrite(row, 1, num_bytes, f);
	}
    }

#if 0	/**************** MAY NOT BE NEEDED ****************/
    /* Enforce the minimum page length, and skip any final blank lines. */
    {
	int paper_length = (int)(pdev->HWResolution[1] * 3 + 0.5);
	int printed_length = height - blank_lines;

	if (height > paper_length)
	    paper_length = height;
	if (printed_length < paper_length)
	    fput_atx_command(f, ATX_VERTICAL_TAB,
			     paper_length - printed_length + 1);
    }
#endif

    /* End the page. */
    fputs(ATX_END_PAGE, f);

 done:
    gs_free_object(mem, compressed, "atx_print_page(compressed)");
    gs_free_object(mem, buf, "atx_print_page(buf)");
    return code;
}

/* Print pages with specified maximum pixel widths. */
private int
atx23_print_page(gx_device_printer *pdev, FILE *f)
{
    return atx_print_page(pdev, f, 576 / 8);
}
private int
atx24_print_page(gx_device_printer *pdev, FILE *f)
{
    return atx_print_page(pdev, f, 832 / 8);
}
private int
atx38_print_page(gx_device_printer *pdev, FILE *f)
{
    return atx_print_page(pdev, f, 2400 / 8);
}
