/* Copyright (C) 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevlj56.c,v 1.3 2001/08/01 00:48:23 stefan911 Exp $ */
/* H-P LaserJet 5 & 6 drivers for Ghostscript */
#include "gdevprn.h"
#include "stream.h"
#include "gdevpcl.h"
#include "gdevpxat.h"
#include "gdevpxen.h"
#include "gdevpxop.h"
#include "gdevpxut.h"

/* Define the default resolution. */
#ifndef X_DPI
#  define X_DPI 600
#endif
#ifndef Y_DPI
#  define Y_DPI 600
#endif

/* Define the number of blank lines that make it worthwhile to */
/* start a new image. */
#define MIN_SKIP_LINES 2

/* We round up the LINE_SIZE to a multiple of a ulong for faster scanning. */
#define W sizeof(word)

private dev_proc_open_device(ljet5_open);
private dev_proc_close_device(ljet5_close);
private dev_proc_print_page(ljet5_print_page);

private const gx_device_procs ljet5_procs =
prn_procs(ljet5_open, gdev_prn_output_page, ljet5_close);

const gx_device_printer gs_lj5mono_device =
prn_device(ljet5_procs, "lj5mono",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,
	   1, ljet5_print_page);

private const gx_device_procs lj5gray_procs =
prn_color_procs(ljet5_open, gdev_prn_output_page, ljet5_close,
		gx_default_gray_map_rgb_color,
		gx_default_gray_map_color_rgb);

const gx_device_printer gs_lj5gray_device = {
    prn_device_body(gx_device_printer, lj5gray_procs, "lj5gray",
		    DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		    X_DPI, Y_DPI,
		    0, 0, 0, 0,
		    1, 8, 255, 0, 256, 1, ljet5_print_page)
};

/* Open the printer, writing the stream header. */
private int
ljet5_open(gx_device * pdev)
{
    int code = gdev_prn_open(pdev);

    if (code < 0)
	return code;
    code = gdev_prn_open_printer(pdev, true);
    if (code < 0)
	return code;
    {
	gx_device_printer *const ppdev = (gx_device_printer *)pdev;
	stream fs;
	stream *const s = &fs;
	byte buf[50];		/* arbitrary */

	swrite_file(s, ppdev->file, buf, sizeof(buf));
	px_write_file_header(s, pdev);
	sflush(s);		/* don't close */
    }
    return 0;
}

/* Close the printer, writing the stream trailer. */
private int
ljet5_close(gx_device * pdev)
{
    gx_device_printer *const ppdev = (gx_device_printer *)pdev;
    int code = gdev_prn_open_printer(pdev, true);

    if (code < 0)
	return code;
    px_write_file_trailer(ppdev->file);
    return gdev_prn_close(pdev);
}

/* Send the page to the printer.  For now, just send the whole image. */
private int
ljet5_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    gs_memory_t *mem = pdev->memory;
    uint line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    uint line_size_words = (line_size + W - 1) / W;
    uint out_size = line_size + (line_size / 127) + 1;
    word *line = (word *)gs_alloc_byte_array(mem, line_size_words, W, "ljet5(line)");
    byte *out = gs_alloc_bytes(mem, out_size, "ljet5(out)");
    int code = 0;
    int lnum;
    stream fs;
    stream *const s = &fs;
    byte buf[200];		/* arbitrary */

    if (line == 0 || out == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    swrite_file(s, prn_stream, buf, sizeof(buf));

    /* Write the page header. */
    {
	static const byte page_header[] = {
	    pxtBeginPage,
	    DUSP(0, 0), DA(pxaPoint),
	    pxtSetCursor
	};
	static const byte mono_header[] = {
	    DUB(eGray), DA(pxaColorSpace),
	    DUB(e8Bit), DA(pxaPaletteDepth),
	    pxt_ubyte_array, pxt_ubyte, 2, 0xff, 0x00, DA(pxaPaletteData),
	    pxtSetColorSpace
	};
	static const byte gray_header[] = {
	    DUB(eGray), DA(pxaColorSpace),
	    pxtSetColorSpace
	};

	px_write_page_header(s, (gx_device *)pdev);
	px_write_select_media(s, (gx_device *)pdev, NULL);
	PX_PUT_LIT(s, page_header);
	if (pdev->color_info.depth == 1)
	    PX_PUT_LIT(s, mono_header);
	else
	    PX_PUT_LIT(s, gray_header);
    }

    /* Write the image header. */
    {
	static const byte mono_image_header[] = {
	    DA(pxaDestinationSize),
	    DUB(eIndexedPixel), DA(pxaColorMapping),
	    DUB(e1Bit), DA(pxaColorDepth),
	    pxtBeginImage
	};
	static const byte gray_image_header[] = {
	    DA(pxaDestinationSize),
	    DUB(eDirectPixel), DA(pxaColorMapping),
	    DUB(e8Bit), DA(pxaColorDepth),
	    pxtBeginImage
	};

	px_put_us(s, pdev->width);
	px_put_a(s, pxaSourceWidth);
	px_put_us(s, pdev->height);
	px_put_a(s, pxaSourceHeight);
	px_put_usp(s, pdev->width, pdev->height);
	if (pdev->color_info.depth == 1)
	    PX_PUT_LIT(s, mono_image_header);
	else
	    PX_PUT_LIT(s, gray_image_header);
    }

    /* Write the image data, compressing each line. */
    for (lnum = 0; lnum < pdev->height; ++lnum) {
	int ncompr;
	static const byte line_header[] = {
	    DA(pxaStartLine),
	    DUS(1), DA(pxaBlockHeight),
	    DUB(eRLECompression), DA(pxaCompressMode),
	    pxtReadImage
	};

	code = gdev_prn_copy_scan_lines(pdev, lnum, (byte *) line, line_size);
	if (code < 0)
	    goto fin;
	px_put_us(s, lnum);
	PX_PUT_LIT(s, line_header);
	ncompr = gdev_pcl_mode2compress_padded(line, line + line_size_words,
					       out, true);
	px_put_data_length(s, ncompr);
	px_put_bytes(s, out, ncompr);
    }

    /* Finish up. */
  fin:
    spputc(s, pxtEndImage);
    spputc(s, pxtEndPage);
    sflush(s);
  done:
    gs_free_object(mem, out, "ljet5(out)");
    gs_free_object(mem, line, "ljet5(line)");
    return code;
}
