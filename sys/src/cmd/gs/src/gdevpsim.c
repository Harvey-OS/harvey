/* Copyright (C) 1994, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpsim.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* PostScript image output device */
#include "gdevprn.h"

/*
 * This driver does what the ps2image utility used to do:
 * It produces a bitmap in the form of a PostScript file that can be
 * fed to any PostScript printer.  It uses a run-length compression
 * method that executes quickly (unlike some produced by PostScript
 * drivers!).
 *
 * There are two drivers here, one for 1-bit black-and-white and one
 * for 8-bit gray.  In fact, the same code could also handles 2- and
 * 4-bit gray output.
 */

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 300
#endif
#ifndef Y_DPI
#  define Y_DPI 300
#endif

/* The device descriptor */
private dev_proc_print_page(psmono_print_page);

const gx_device_printer gs_psmono_device =
prn_device(prn_std_procs, "psmono",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   1, psmono_print_page);

private const gx_device_procs psgray_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
	      gx_default_gray_map_rgb_color, gx_default_gray_map_color_rgb);

const gx_device_printer gs_psgray_device =
{
    prn_device_body(gx_device_printer, psgray_procs, "psgray",
		    DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		    X_DPI, Y_DPI,
		    0, 0, 0, 0,	/* margins */
		    1, 8, 255, 0, 256, 1, psmono_print_page)
};

/*
 * The following setup code gets written to the PostScript file.
 * We would have to break it up anyway because the Watcom compiler has
 * a limit of 512 characters in a single token, so we make a virtue out of
 * necessity and make each line a separate string.
 */
private const char *const psmono_setup[] =
{
    "%!PS",
    "  /maxrep 255 def		% max repeat count",
    "		% Initialize the strings for filling runs (lazily).",
    "     /.ImageFill",
    "      { maxrep string dup 0 1 maxrep 1 sub { 3 index put dup } for",
    "	.ImageFills 4 2 roll put",
    "      } bind def",
    "     /.ImageFills [ 0 1 255 {",
    "	/.ImageFill cvx 2 array astore cvx",
    "     } for ] def",
    "		% Initialize the procedure table for input dispatching.",
    "     /.ImageProcs [",
    "		% Stack: <buffer> <file> <xdigits> <previous> <byte>",
    "     32 { { pop .ImageItem } } repeat",
    "     16 { {	% 0x20-0x2f: (N-0x20) data bytes follow",
  "      32 sub 3 -1 roll add 3 index exch 0 exch getinterval 2 index exch",
    "      readhexstring pop exch pop 0 exch dup",
    "     } bind } repeat",
  "     16 { {	% 0x30-0x3f: prefix hex digit (N-0x30) to next count",
    "      48 sub 3 -1 roll add 4 bitshift exch .ImageItem",
    "     } bind } repeat",
    "     32 { {	% 0x40-0x5f: repeat last data byte (N-0x40) times",
    "      64 sub 3 -1 roll add .ImageFills 2 index dup length 1 sub get get exec",
    "      exch 0 exch getinterval 0 3 1 roll",
    "     } bind } repeat",
    "     160 { { pop .ImageItem } } repeat",
    "     ] readonly def",
    "		% Read one item from a compressed image.",
    "		% Stack contents: <buffer> <file> <xdigits> <previous>",
    "  /.ImageItem",
    "   { 2 index read pop dup .ImageProcs exch get exec",
    "   } bind def",
    "		% Read and print an entire compressed image.",
"  /.ImageRead		% <xres> <yres> <width> <height> <bpc> .ImageRead -",
    "   { gsave [",
	/* Stack: xres yres width height bpc -mark- */
    "     6 -2 roll exch 72 div 0 0 4 -1 roll -72 div 0 7 index",
	/* Stack: width height bpc -mark- xres/72 0 0 -yres/72 0 height */
    "     ] { .ImageItem }",
	/* Stack: width height bpc <matrix> <proc> */
    "     4 index 3 index mul 7 add 8 idiv string currentfile 0 ()",
	/* Stack: width height bpc <matrix> <proc> <buffer> <file> 0 () */
    "     9 4 roll",
	/* Stack: <buffer> <file> 0 () width height bpc <matrix> <proc> */
    "     image pop pop pop pop",
    "     grestore showpage",
    "   } def"
};

#define data_run_code 0x20
#define xdigit_code 0x30
#define max_data_per_line 35
#define repeat_run_code 0x40
#define max_repeat_run_code 31
#define max_repeat_run 255

/* Send the page to the printer. */
private void write_data_run(P4(const byte *, int, FILE *, byte));
private int
psmono_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    int lnum;
    byte *line = gs_alloc_bytes(pdev->memory, line_size, "psmono_print_page");
    byte invert = (pdev->color_info.depth == 1 ? 0xff : 0);

    if (line == 0)
	return_error(gs_error_VMerror);

    /* If this is the first page of the file, */
    /* write the setup code. */
    if (gdev_prn_file_is_new(pdev)) {
	int i;

	for (i = 0; i < countof(psmono_setup); i++)
	    fprintf(prn_stream, "%s\r\n", psmono_setup[i]);
    }
    /* Write the .ImageRead command. */
    fprintf(prn_stream,
	    "%g %g %d %d %d .ImageRead\r\n",
	    pdev->HWResolution[0], pdev->HWResolution[1],
	    pdev->width, pdev->height, pdev->color_info.depth);

    /* Compress each scan line in turn. */
    for (lnum = 0; lnum < pdev->height; lnum++) {
	const byte *p;
	int left = line_size;
	byte *data;

	gdev_prn_get_bits(pdev, lnum, line, &data);
	p = data;
	/* Loop invariant: p + left = data + line_size. */
#define min_repeat_run 10
	while (left >= min_repeat_run) {	/* Detect a maximal run of non-repeated data. */
	    const byte *p1 = p;
	    int left1 = left;
	    byte b;
	    int count, count_left;

	    while (left1 >= min_repeat_run &&
		   ((b = *p1) != p1[1] ||
		    b != p1[2] || b != p1[3] || b != p1[4] ||
		    b != p1[5] || b != p1[6] || b != p1[7] ||
		    b != p1[8] || b != p1[9])
		)
		++p1, --left1;
	    if (left1 < min_repeat_run)
		break;		/* no repeated data left */
	    write_data_run(p, (int)(p1 - p + 1), prn_stream,
			   invert);
	    /* Detect a maximal run of repeated data. */
	    p = ++p1 + (min_repeat_run - 1);
	    left = --left1 - (min_repeat_run - 1);
	    while (left > 0 && *p == b)
		++p, --left;
	    for (count = p - p1; count > 0;
		 count -= count_left
		) {
		count_left = min(count, max_repeat_run);
		if (count_left > max_repeat_run_code)
		    fputc(xdigit_code + (count_left >> 4),
			  prn_stream),
			fputc(repeat_run_code + (count_left & 0xf),
			      prn_stream);
		else
		    putc(repeat_run_code + count_left,
			 prn_stream);
	    }
	}
	/* Write the remaining data, if any. */
	write_data_run(p, left, prn_stream, invert);
    }

    /* Clean up and return. */
    fputs("\r\n", prn_stream);
    gs_free_object(pdev->memory, line, "psmono_print_page");
    return 0;
}

/* Write a run of data on the file. */
private void
write_data_run(const byte * data, int count, FILE * f, byte invert)
{
    const byte *p = data;
    const char *const hex_digits = "0123456789abcdef";
    int left = count;
    char line[sizeof(count) * 2 + max_data_per_line * 2 + 3];
    char *q = line;

    /* Write the count. */

    if (!count)
	return;
    {
	int shift = sizeof(count) * 8;

	while ((shift -= 4) > 0 && (count >> shift) == 0);
	for (; shift > 0; shift -= 4)
	    *q++ = xdigit_code + ((count >> shift) & 0xf);
	*q++ = data_run_code + (count & 0xf);
    }

    /* Write the data. */

    while (left > 0) {
	register int wcount = min(left, max_data_per_line);

	left -= wcount;
	for (; wcount > 0; ++p, --wcount) {
	    byte b = *p ^ invert;

	    *q++ = hex_digits[b >> 4];
	    *q++ = hex_digits[b & 0xf];
	}
	*q++ = '\r';
	*q++ = '\n';
	fwrite(line, 1, q - line, f);
	q = line;
    }

}
