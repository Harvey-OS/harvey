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

/*$Id: gdevbmp.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* .BMP file format definitions and utility interfaces */

#ifndef gdevbmp_INCLUDED
#  define gdevbmp_INCLUDED

/* Define the default X and Y resolution. */
#define X_DPI 72
#define Y_DPI 72

/* Write the BMP file header.  This procedure is used for all formats. */
int write_bmp_header(P2(gx_device_printer *pdev, FILE *file));

/* Write a BMP header for separated CMYK output. */
int write_bmp_separated_header(P2(gx_device_printer *pdev, FILE *file));

/* 24-bit color mappers */
dev_proc_map_rgb_color(bmp_map_16m_rgb_color);
dev_proc_map_color_rgb(bmp_map_16m_color_rgb);

#endif				/* gdevbmp_INCLUDED */
