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

/*$Id: gdevmpla.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Interface to planar memory devices. */

#ifndef gdevmpla_INCLUDED
#  define gdevmpla_INCLUDED

/*
 * Planar memory devices store the bits by planes instead of by chunks.
 * The plane corresponding to the least significant bits of the color index
 * is stored first.  Each plane may store a different number of bits,
 * but the depth of each plane must be an allowable one for a memory
 * device and not greater than 16 (currently, 1, 2, 4, 8, or 16), and the
 * total must not exceed the size of gx_color_index.
 *
 * Planar devices store the data for each plane contiguously, as though
 * each plane were a separate device.  There is an array of line pointers
 * for each plane (num_planes arrays in all).
 */

/*
 * Set up a planar memory device, after calling gs_make_mem_device but
 * before opening the device.  The pre-existing device provides the color
 * mapping procedures, but not the drawing procedures.  Requires: num_planes
 * > 0, plane_depths[0 ..  num_planes - 1] > 0, sum of plane_depths <=
 * mdev->color_info.depth.
 */
int gdev_mem_set_planar(P3(gx_device_memory * mdev, int num_planes,
			   const gx_render_plane_t *planes /*[num_planes]*/));

#endif /* gdevmpla_INCLUDED */
