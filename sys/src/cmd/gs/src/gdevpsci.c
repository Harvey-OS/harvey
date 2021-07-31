/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpsci.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* PostScript color image output device */
#include "gdevprn.h"
#include "stream.h"
#include "strimpl.h"
#include "srlx.h"

/*
 * This driver produces plane-separated, run-length-encoded, 24-bit RGB
 * images suitable for a PostScript Level 2 printer.  LZW compression would
 * be better, but Unisys' claim to own the compression algorithm and their
 * demand for licensing and payment even for freely distributed software
 * rule this out.
 */

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 300
#endif
#ifndef Y_DPI
#  define Y_DPI 300
#endif

/* The device descriptor */
private dev_proc_print_page(psrgb_print_page);

private const gx_device_procs psrgb_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);

const gx_device_printer gs_psrgb_device =
{
    prn_device_body(gx_device_printer, psrgb_procs, "psrgb",
		    DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		    X_DPI, Y_DPI,
		    0, 0, 0, 0,	/* margins */
		    3, 24, 255, 255, 256, 256, psrgb_print_page)
};

/*
 * The following setup code gets written to the PostScript file.
 * We would have to break it up anyway because the Watcom compiler has
 * a limit of 512 characters in a single token, so we make a virtue out of
 * necessity and make each line a separate string.
 */
private const char *const psrgb_setup[] =
{
    "%!PS",
    "currentpagedevice /PageSize get aload pop scale",
    "/rgbimage {",		/* <width> <height> rgbimage - */
    "  /h exch def /w exch def save",
    "  /s1 w string def /s2 w string def /s3 w string def",
    "  /f currentfile /RunLengthDecode filter def",
    "  w h 8 [w 0 0 h neg 0 h]",
 "    { f s1 readstring pop} { f s2 readstring pop} { f s3 readstring pop}",
    "    true 3 colorimage restore",
    "} bind def"
};

/* Send the page to the printer. */
private int
psrgb_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    gs_memory_t *mem = pdev->memory;
    int width = pdev->width;
    byte *lbuf = gs_alloc_bytes(mem, width * 3,
				"psrgb_print_page(lbuf)");
    int lnum;
    stream fs, rls;
    stream_RLE_state rlstate;
    byte fsbuf[200];		/* arbitrary, must be >2 */
    byte rlsbuf[200];		/* arbitrary, must be >128 */

    if (lbuf == 0)
	return_error(gs_error_VMerror);
    if (gdev_prn_file_is_new(pdev)) {
	int i;

	for (i = 0; i < countof(psrgb_setup); i++)
	    fprintf(prn_stream, "%s\n", psrgb_setup[i]);
    }
    fprintf(prn_stream, "%d %d rgbimage\n", width, pdev->height);
    swrite_file(&fs, prn_stream, fsbuf, sizeof(fsbuf));
    fs.memory = 0;
    (*s_RLE_template.set_defaults) ((stream_state *) & rlstate);
    s_std_init(&rls, rlsbuf, sizeof(rlsbuf), &s_filter_write_procs,
	       s_mode_write);
    rls.memory = 0;
    rlstate.memory = 0;
    rlstate.template = &s_RLE_template;
    (*s_RLE_template.init) ((stream_state *) & rlstate);
    rls.state = (stream_state *) & rlstate;
    rls.procs.process = s_RLE_template.process;
    rls.strm = &fs;
    for (lnum = 0; lnum < pdev->height; ++lnum) {
	byte *data;
	int i, c;

	gdev_prn_get_bits(pdev, lnum, lbuf, &data);
	for (c = 0; c < 3; ++c) {
	    const byte *p;

	    for (i = 0, p = data + c; i < width; ++i, p += 3)
		sputc(&rls, *p);
	}
    }
    sclose(&rls);
    sflush(&fs);
    fputs("\nshowpage\n", prn_stream);
    gs_free_object(mem, lbuf, "psrgb_print_page(lbuf)");
    return 0;
}
