/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpcl.h,v 1.4 2000/09/19 19:00:17 lpd Exp $ */
/* Support for PCL-based printer drivers */
/* Requires gdevprn.h */

#ifndef gdevpcl_INCLUDED
#  define gdevpcl_INCLUDED

/*
 * Define the PCL paper size codes.  H-P's documentation and coding for the
 * 11x17 size are inconsistent: some printers seem to accept code 11 as well
 * as code 6, and while the definitions below match the PCL5 reference
 * manual, some documentation calls 11x17 "tabloid" and reserves the name
 * "ledger" for 17x11.
 */
#define PAPER_SIZE_LETTER 2	/* 8.5" x 11" */
#define PAPER_SIZE_LEGAL 3	/* 8.5" x 14" */
#define PAPER_SIZE_LEDGER 6	/* 11" x 17" */
#define PAPER_SIZE_A4 26	/* 21.0 cm x 29.7 cm */
#define PAPER_SIZE_A3 27	/* 29.7 cm x 42.0 cm */
#define PAPER_SIZE_A2 28
#define PAPER_SIZE_A1 29
#define PAPER_SIZE_A0 30

/* Get the paper size code, based on width and height. */
int gdev_pcl_paper_size(P1(gx_device *));

/* Color mapping procedures for 3-bit-per-pixel RGB printers */
dev_proc_map_rgb_color(gdev_pcl_3bit_map_rgb_color);
dev_proc_map_color_rgb(gdev_pcl_3bit_map_color_rgb);

/* Row compression routines */
typedef ulong word;
int
    gdev_pcl_mode2compress(P3(const word * row, const word * end_row, byte * compressed)),
    gdev_pcl_mode2compress_padded(P4(const word * row, const word * end_row, byte * compressed, bool pad)),
    gdev_pcl_mode3compress(P4(int bytecount, const byte * current, byte * previous, byte * compressed)),
    gdev_pcl_mode9compress(P4(int bytecount, const byte * current, const byte * previous, byte * compressed));

#endif /* gdevpcl_INCLUDED */
