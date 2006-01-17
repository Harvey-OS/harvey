/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsim.c,v 1.14 2004/10/07 05:18:34 ray Exp $ */
/* PostScript image output device */
#include "gdevprn.h"
#include "gdevpsu.h"
#include "stream.h"
#include "strimpl.h"
#include "sa85x.h"
#include "srlx.h"

/*
 * There are two drivers in this file, both of which produce PostScript
 * output consisting of a single bitmap per page.  The psmono/psgray
 * driver produces monochrome Level 1 images using home-grown run length
 * compression; the psrgb driver produces planar RGB Level 2 images
 * using the RunLengthEncode filter.
 */

/* ---------------- Shared code ---------------- */

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 300
#endif
#ifndef Y_DPI
#  define Y_DPI 300
#endif

/* Write the file (if necessary) and page headers. */
private void
ps_image_write_headers(FILE *f, gx_device_printer *pdev,
		       const char *const setup[],
		       gx_device_pswrite_common_t *pdpc)
{
    if (gdev_prn_file_is_new(pdev)) {
	gs_rect bbox;

	bbox.p.x = 0;
	bbox.p.y = 0;
	bbox.q.x = pdev->width / pdev->HWResolution[0] * 72.0;
	bbox.q.y = pdev->height / pdev->HWResolution[1] * 72.0;
	psw_begin_file_header(f, (gx_device *)pdev, &bbox, pdpc, false);
	psw_print_lines(f, setup);
	psw_end_file_header(f);
    }
    {
	byte buf[100];		/* arbitrary */
	stream s;

	s_init(&s, pdev->memory);
	swrite_file(&s, f, buf, sizeof(buf));
	psw_write_page_header(&s, (gx_device *)pdev, pdpc, true, pdev->PageCount + 1, 10);
	sflush(&s);
    }
}

/* ---------------- Level 1 monochrome driver ---------------- */

/*
 * This driver produces a bitmap in the form of a PostScript file that can
 * be fed to any PostScript printer.  It uses a run-length compression
 * method that executes quickly (unlike some produced by PostScript
 * drivers!).
 *
 * There are two devices here, one for 1-bit black-and-white and one
 * for 8-bit gray.  In fact, the same code could also handle 2- and
 * 4-bit gray output.
 */

/* The device descriptor */
private dev_proc_print_page(psmono_print_page);
private dev_proc_close_device(psmono_close);

const gx_device_printer gs_psmono_device =
prn_device(prn_std_procs, "psmono",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   1, psmono_print_page);

private const gx_device_procs psgray_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, psmono_close,
	      gx_default_gray_map_rgb_color, gx_default_gray_map_color_rgb);

const gx_device_printer gs_psgray_device = {
    prn_device_body(gx_device_printer, psgray_procs, "psgray",
		    DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		    X_DPI, Y_DPI,
		    0, 0, 0, 0,	/* margins */
		    1, 8, 255, 0, 256, 1, psmono_print_page)
};

private const char *const psmono_setup[] = {
		/* Initialize the strings for filling runs. */
    "/.ImageFills [ 0 1 255 {",
    "  256 string dup 0 1 7 { 3 index put dup } for { 8 16 32 64 128 } {",
    "    2 copy 0 exch getinterval putinterval dup",
    "  } forall pop exch pop",
    "} bind for ] def",
		/* Initialize the procedure table for input dispatching. */
    "/.ImageProcs [",
		/* Stack: <buffer> <file> <xdigits> <previous> <byte> */
    "  32 { { pop .ImageItem } } repeat",
    "  16 { {",	/* 0x20-0x2f: (N-0x20) data bytes follow */
    "    32 sub 3 -1 roll add 3 index exch 0 exch getinterval 2 index exch",
    "    readhexstring pop exch pop 0 exch dup",
    "  } bind } repeat",
    "  16 { {",	/* 0x30-0x3f: prefix hex digit (N-0x30) to next count */
    "    48 sub 3 -1 roll add 4 bitshift exch .ImageItem",
    "  } bind } repeat",
    "  32 { {",	/* 0x40-0x5f: repeat last data byte (N-0x40) times */
    "    64 sub 3 -1 roll add .ImageFills 2 index dup length 1 sub get get",
    "    exch 0 exch getinterval 0 3 1 roll",
    "  } bind } repeat",
    "  160 { { pop .ImageItem } } repeat",
    "] readonly def",
		/* Read one item from a compressed image. */
		/* Stack contents: <buffer> <file> <xdigits> <previous> */
    "/.ImageItem {",
    "  2 index read pop dup .ImageProcs exch get exec",
    "} bind def",
		/* Read and print an entire compressed image. */
    "/.ImageRead {"	/* <width> <height> <bpc> .ImageRead - */
    "  gsave [",
      /* Stack: width height bpc -mark- */
    "    1 0 0 -1 0 7 index",
      /* Stack: width height bpc -mark- 1 0 0 -1 0 height */
    "  ] { .ImageItem }",
	/* Stack: width height bpc <matrix> <proc> */
    "  4 index 3 index mul 7 add 8 idiv string currentfile 0 ()",
	/* Stack: width height bpc <matrix> <proc> <buffer> <file> 0 () */
    "  9 4 roll",
	/* Stack: <buffer> <file> 0 () width height bpc <matrix> <proc> */
    "  image pop pop pop pop grestore",
    "} def",
    0
};
static const gx_device_pswrite_common_t psmono_values =
    PSWRITE_COMMON_VALUES(1, 0 /*false*/, 1);

#define data_run_code 0x20
#define xdigit_code 0x30
#define max_data_per_line 35
#define repeat_run_code 0x40
#define max_repeat_run_code 31
#define max_repeat_run 255

/* Send the page to the printer. */
private void write_data_run(const byte *, int, FILE *, byte);
private int
psmono_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    int lnum;
    byte *line = gs_alloc_bytes(pdev->memory, line_size, "psmono_print_page");
    byte invert = (pdev->color_info.depth == 1 ? 0xff : 0);
    gx_device_pswrite_common_t pswrite_common;

    if (line == 0)
	return_error(gs_error_VMerror);
    pswrite_common = psmono_values;

    /* If this is the first page of the file, */
    /* write the setup code. */
    ps_image_write_headers(prn_stream, pdev, psmono_setup, &pswrite_common);

    /* Write the .ImageRead command. */
    fprintf(prn_stream,
	    "%d %d %d .ImageRead\n",
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
            if (ferror(prn_stream))
	        return_error(gs_error_ioerror);
        }
	/* Write the remaining data, if any. */
	write_data_run(p, left, prn_stream, invert);
    }

    /* Clean up and return. */
    fputs("\n", prn_stream);
    psw_write_page_trailer(prn_stream, 1, true);
    gs_free_object(pdev->memory, line, "psmono_print_page");
    if (ferror(prn_stream))
	return_error(gs_error_ioerror);
    return 0;
}

/* Close the file. */
private int
psmono_close(gx_device *dev)
{
    int code = psw_end_file(((gx_device_printer *)dev)->file, dev, 
            &psmono_values, NULL, dev->PageCount);
    
    if (code < 0)
        return code;
    return gdev_prn_close(dev);
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
	*q++ = '\n';
	fwrite(line, 1, q - line, f);
	q = line;
    }

}

/* ---------------- Level 2 RGB driver ---------------- */

/*
 * This driver produces plane-separated, run-length-encoded, 24-bit RGB
 * images suitable for a PostScript Level 2 printer.  LZW compression would
 * be better, but Unisys' claim to own the compression algorithm and their
 * demand for licensing and payment even for freely distributed software
 * rule this out.
 */

/* The device descriptor */
private dev_proc_print_page(psrgb_print_page);
private dev_proc_close_device(psrgb_close);

private const gx_device_procs psrgb_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, psrgb_close,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);

const gx_device_printer gs_psrgb_device = {
    prn_device_body(gx_device_printer, psrgb_procs, "psrgb",
		    DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		    X_DPI, Y_DPI,
		    0, 0, 0, 0,	/* margins */
		    3, 24, 255, 255, 256, 256, psrgb_print_page)
};

private const char *const psrgb_setup[] = {
    "/rgbimage {",		/* <width> <height> rgbimage - */
    "  gsave 2 copy scale /h exch def /w exch def",
    "  /s1 w string def /s2 w string def /s3 w string def",
    "  /f currentfile /ASCII85Decode filter /RunLengthDecode filter def",
    "  w h 8 [w 0 0 h neg 0 h]",
    "  {f s1 readstring pop} {f s2 readstring pop} {f s3 readstring pop}",
    "  true 3 colorimage grestore",
    "} bind def",
    0
};
static const gx_device_pswrite_common_t psrgb_values =
    PSWRITE_COMMON_VALUES(2, 0 /*false*/, 1);

/* Send the page to the printer. */
private int
psrgb_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    gs_memory_t *mem = pdev->memory;
    int width = pdev->width;
    byte *lbuf = gs_alloc_bytes(mem, width * 3,
				"psrgb_print_page(lbuf)");
    int lnum;
    stream fs, a85s, rls;
    stream_A85E_state a85state;
    stream_RLE_state rlstate;
    byte fsbuf[200];		/* arbitrary, must be >2 */
    byte a85sbuf[100];		/* arbitrary, must be >=6 */
    byte rlsbuf[200];		/* arbitrary, must be >128 */
    gx_device_pswrite_common_t pswrite_common;
    pswrite_common = psrgb_values;

    if (lbuf == 0)
	return_error(gs_error_VMerror);
    ps_image_write_headers(prn_stream, pdev, psrgb_setup, &pswrite_common);
    fprintf(prn_stream, "%d %d rgbimage\n", width, pdev->height);
    s_init(&fs, mem);
    swrite_file(&fs, prn_stream, fsbuf, sizeof(fsbuf));
    fs.memory = 0;

    if (s_A85E_template.set_defaults)
	(*s_A85E_template.set_defaults) ((stream_state *) & a85state);
    s_init(&a85s, mem);
    s_std_init(&a85s, a85sbuf, sizeof(a85sbuf), &s_filter_write_procs,
	       s_mode_write);
    a85s.memory = 0;
    a85state.memory = 0;
    a85state.template = &s_A85E_template;
    (*s_A85E_template.init) ((stream_state *) & a85state);
    a85s.state = (stream_state *) & a85state;
    a85s.procs.process = s_A85E_template.process;
    a85s.strm = &fs;

    (*s_RLE_template.set_defaults) ((stream_state *) & rlstate);
    s_init(&rls, mem);
    s_std_init(&rls, rlsbuf, sizeof(rlsbuf), &s_filter_write_procs,
	       s_mode_write);
    rls.memory = 0;
    rlstate.memory = 0;
    rlstate.template = &s_RLE_template;
    (*s_RLE_template.init) ((stream_state *) & rlstate);
    rls.state = (stream_state *) & rlstate;
    rls.procs.process = s_RLE_template.process;
    rls.strm = &a85s;

    for (lnum = 0; lnum < pdev->height; ++lnum) {
	byte *data;
	int i, c;

	gdev_prn_get_bits(pdev, lnum, lbuf, &data);
	for (c = 0; c < 3; ++c) {
	    const byte *p;

	    for (i = 0, p = data + c; i < width; ++i, p += 3)
		sputc(&rls, *p);
            if (rls.end_status == ERRC)
              return_error(gs_error_ioerror);
	}
    }
    sclose(&rls);
    sclose(&a85s);
    sflush(&fs);
    fputs("\n", prn_stream);
    psw_write_page_trailer(prn_stream, 1, true);
    gs_free_object(mem, lbuf, "psrgb_print_page(lbuf)");
    if (ferror(prn_stream))
        return_error(gs_error_ioerror);
    return 0;
}

/* Close the file. */
private int
psrgb_close(gx_device *dev)
{
    int code = psw_end_file(((gx_device_printer *)dev)->file, dev,
            &psrgb_values, NULL, dev->PageCount);
    
    if (code < 0)
        return code;
    return gdev_prn_close(dev);
}
