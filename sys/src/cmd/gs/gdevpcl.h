/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevpcl.h */
/* Interface to PCL utilities for printer drivers */
/* Requires gdevprn.h */

/* Define the PCL paper size codes. */
#define PAPER_SIZE_LETTER 2
#define PAPER_SIZE_LEGAL 3
#define PAPER_SIZE_A4 26
#define PAPER_SIZE_A3 27
#define PAPER_SIZE_A2 28
#define PAPER_SIZE_A1 29
#define PAPER_SIZE_A0 30

/* Get the paper size code, based on width and height. */
int	gdev_pcl_paper_size(P1(gx_device *));

/* Color mapping procedures for 3-bit-per-pixel RGB printers */
dev_proc_map_rgb_color(gdev_pcl_3bit_map_rgb_color);
dev_proc_map_color_rgb(gdev_pcl_3bit_map_color_rgb);

/* Row compression routines */
typedef ulong word;
int	gdev_pcl_mode2compress(P3(const word *row, const word *end_row, byte *compressed)),
	gdev_pcl_mode3compress(P4(int bytecount, const byte *current, byte *previous, byte *compressed));
