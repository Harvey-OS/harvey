/* Copyright (C) 1995, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxctable.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Interface to color table lookup and interpolation */

#ifndef gxctable_INCLUDED
#  define gxctable_INCLUDED

#include "gxfixed.h"
#include "gxfrac.h"

/*
 * Define a 3- or 4-D color lookup table.
 * n is the number of dimensions (input indices), 3 or 4.
 * dims[0..n-1] are the table dimensions.
 * m is the number of output values, typically 3 (RGB) or 4 (CMYK).
 * For n = 3:
 *   table[i], 0 <= i < dims[0], point to strings of length
 *     dims[1] x dims[2] x m.
 * For n = 4:
 *   table[i], 0 <= i < dims[0] x dims[1], points to strings of length
 *     dims[2] x dims[3] x m.
 * It isn't really necessary to store the size of each string, since
 * they're all the same size, but it makes things a lot easier for the GC.
 */
typedef struct gx_color_lookup_table_s {
    int n;
    int dims[4];		/* [ndims] */
    int m;
    const gs_const_string *table;
} gx_color_lookup_table;

/*
 * Interpolate in a 3- or 4-D color lookup table.
 * pi[0..n-1] are the table indices, guaranteed to be in the ranges
 * [0..dims[n]-1] respectively.
 * Return interpolated values in pv[0..m-1].
 */

/* Return the nearest value without interpolation. */
void gx_color_interpolate_nearest(P3(const fixed * pi,
			    const gx_color_lookup_table * pclt, frac * pv));

/* Use trilinear interpolation. */
void gx_color_interpolate_linear(P3(const fixed * pi,
			    const gx_color_lookup_table * pclt, frac * pv));

#endif /* gxctable_INCLUDED */
