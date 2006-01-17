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

/* $Id: gdevdfax.c,v 1.6 2002/02/21 22:24:51 giles Exp $*/
/* DigiBoard fax device. */
/***
 *** Note: this driver is maintained by a user: please contact
 ***       Rick Richardson (rick@digibd.com) if you have questions.
 ***/
#include "gdevprn.h"
#include "strimpl.h"
#include "scfx.h"
#include "gdevfax.h"
#include "gdevtfax.h"

/* Define the device parameters. */
#define X_DPI 204
#define Y_DPI 196

/* The device descriptors */

private dev_proc_open_device(dfax_prn_open);
private dev_proc_print_page(dfax_print_page);

struct gx_device_dfax_s {
	gx_device_common;
	gx_prn_device_common;
	long pageno;
	uint iwidth;		/* width of image data in pixels */
};
typedef struct gx_device_dfax_s gx_device_dfax;

private gx_device_procs dfax_procs =
  prn_procs(dfax_prn_open, gdev_prn_output_page, gdev_prn_close);

gx_device_dfax far_data gs_dfaxlow_device =
{   prn_device_std_body(gx_device_dfax, dfax_procs, "dfaxlow",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI/2,
	0,0,0,0,			/* margins */
	1, dfax_print_page)
};

gx_device_dfax far_data gs_dfaxhigh_device =
{   prn_device_std_body(gx_device_dfax, dfax_procs, "dfaxhigh",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, dfax_print_page)
};

#define dfdev ((gx_device_dfax *)dev)

/* Open the device, adjusting the paper size. */
private int
dfax_prn_open(gx_device *dev)
{	dfdev->pageno = 0;
	return gdev_fax_open(dev);
}

/* Print a DigiFAX page. */
private int
dfax_print_page(gx_device_printer *dev, FILE *prn_stream)
{	stream_CFE_state state;
	static char hdr[64] = "\000PC Research, Inc\000\000\000\000\000\000";
	int code;

	gdev_fax_init_state(&state, (gx_device_fax *)dev);
	state.EndOfLine = true;
	state.EncodedByteAlign = true;

	/* Start a page: write the header */
	hdr[24] = 0; hdr[28] = 1;
	hdr[26] = ++dfdev->pageno; hdr[27] = dfdev->pageno >> 8;
	if (dev->y_pixels_per_inch == Y_DPI)
		{ hdr[45] = 0x40; hdr[29] = 1; }	/* high res */
	else
		{ hdr[45] = hdr[29] = 0; }		/* low res */
	fseek(prn_stream, 0, SEEK_END);
	fwrite(hdr, sizeof(hdr), 1, prn_stream);

	/* Write the page */
	code = gdev_fax_print_page(dev, prn_stream, &state);

	/* Fixup page count */
	fseek(prn_stream, 24L, SEEK_SET);
	hdr[24] = dfdev->pageno; hdr[25] = dfdev->pageno >> 8;
	fwrite(hdr+24, 2, 1, prn_stream);

	return code;
}

#undef dfdev
