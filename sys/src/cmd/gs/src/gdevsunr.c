/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevsunr.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Sun raster file driver */
#include "gdevprn.h"

/*
 * Currently, the only variety of this format supported in this file is
 * Harlequin's 1-bit "SUN_RAS" with no colormap and odd "};\n" at tail.
 */

#define	RAS_MAGIC	0x59a66a95
#define RT_STANDARD	1	/* Raw pixrect image in 68000 byte order */
#define RMT_NONE	0	/* ras_maplength is expected to be 0 */
typedef struct sun_rasterfile_s {
	int	ras_magic;		/* magic number */
	int	ras_width;		/* width (pixels) of image */
	int	ras_height;		/* height (pixels) of image */
	int	ras_depth;		/* depth (1, 8, or 24 bits) of pixel */
	int	ras_length;		/* length (bytes) of image */
	int	ras_type;		/* type of file; see RT_* below */
	int	ras_maptype;		/* type of colormap; see RMT_* below */
	int	ras_maplength;		/* length (bytes) of following map */
} sun_rasterfile_t;

#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

private dev_proc_print_page(sunhmono_print_page);

gx_device_printer gs_sunhmono_device =
    prn_device(prn_std_procs, "sunhmono",
	       DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	       X_DPI, Y_DPI,
	       0, 0, 0, 0,	/* margins */
	       1, sunhmono_print_page);

private int
sunhmono_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    int gsLineBytes = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    /* Output bytes have to be padded to 16 bits. */
    int rasLineBytes = ROUND_UP(gsLineBytes, 2);
    int lineCnt;
    char *lineStorage; /* Allocated for passing storage to gdev_prn_get_bits() */
    byte *data;
    sun_rasterfile_t ras;
    int code = 0;

    /*
      fprintf(stderr,"pdev->width:%d (%d/%d) gsLineBytes:%d rasLineBytes:%d\n",
      pdev->width, pdev->width/8, pdev->width%8,gsLineBytes,rasLineBytes);
    */
    lineStorage = gs_malloc(gsLineBytes, 1, "rasterfile_print_page(in)");
    if (lineStorage == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto out;
    }
    /* Setup values in header */
    ras.ras_magic = RAS_MAGIC;
    ras.ras_width = pdev->width;
    ras.ras_height = pdev->height;
    ras.ras_depth = 1;
    ras.ras_length = (rasLineBytes * pdev->height);
    ras.ras_type = RT_STANDARD;
    ras.ras_maptype = RMT_NONE;
    ras.ras_maplength = 0;
    /* Write header */
    fwrite(&ras, 1, sizeof(ras), prn_stream);
    /* For each raster line */
    for (lineCnt = 0; lineCnt < pdev->height; ++lineCnt) {
	gdev_prn_get_bits(pdev, lineCnt, lineStorage, &data);
	fwrite(data, 1, gsLineBytes, prn_stream);
	if (gsLineBytes % 2)
	    fputc(0, prn_stream); /* pad to even # of bytes with a 0 */
    }
    /* The weird file terminator */
    fwrite("};\n", 1, 3, prn_stream);
out:
    /* Clean up... */
    gs_free(lineStorage, gsLineBytes, 1, "rasterfile_print_page(in)");
    return code;
}
