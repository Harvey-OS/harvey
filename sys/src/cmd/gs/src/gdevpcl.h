/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpcl.h,v 1.8 2002/06/16 07:25:26 lpd Exp $ */
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
#define PAPER_SIZE_EXECUTIVE 1
#define PAPER_SIZE_LETTER 2	/* 8.5" x 11" */
#define PAPER_SIZE_LEGAL 3	/* 8.5" x 14" */
#define PAPER_SIZE_LEDGER 6	/* 11" x 17" */
#define PAPER_SIZE_A4 26	/* 21.0 cm x 29.7 cm */
#define PAPER_SIZE_A3 27	/* 29.7 cm x 42.0 cm */
#define PAPER_SIZE_A2 28
#define PAPER_SIZE_A1 29
#define PAPER_SIZE_A0 30
#define PAPER_SIZE_JIS_B5 45
#define PAPER_SIZE_JIS_B4 46
#define PAPER_SIZE_JPOST 71
#define PAPER_SIZE_JPOSTD 72
#define PAPER_SIZE_MONARCH 80
#define PAPER_SIZE_COM10 81
#define PAPER_SIZE_DL 90
#define PAPER_SIZE_C5 91
#define PAPER_SIZE_B5 100

/* Get the paper size code, based on width and height. */
int gdev_pcl_paper_size(gx_device *);

/* Color mapping procedures for 3-bit-per-pixel RGB printers */
dev_proc_map_rgb_color(gdev_pcl_3bit_map_rgb_color);
dev_proc_map_color_rgb(gdev_pcl_3bit_map_color_rgb);

/* Row compression routines */
typedef ulong word;
int
    gdev_pcl_mode2compress(const word * row, const word * end_row, byte * compressed),
    gdev_pcl_mode2compress_padded(const word * row, const word * end_row, byte * compressed, bool pad),
    gdev_pcl_mode3compress(int bytecount, const byte * current, byte * previous, byte * compressed),
    gdev_pcl_mode9compress(int bytecount, const byte * current, const byte * previous, byte * compressed);

#endif /* gdevpcl_INCLUDED */
