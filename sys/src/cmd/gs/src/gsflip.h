/* Copyright (C) 1996, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsflip.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Interface to routines for "flipping" image data */

#ifndef gsflip_INCLUDED
#  define gsflip_INCLUDED

/*
 * Convert planar (MultipleDataSource) input to chunky format.  The input
 * data starts at planes[0] + offset ... planes[num_planes-1] + offset; the
 * output is stored at buffer.  This procedure assumes that the input
 * consists of an integral number of pixels; in particular, for 12-bit
 * input, nbytes is rounded up to a multiple of 3.  num_planes must be >=0;
 * bits_per_sample must be 1, 2, 4, 8, or 12.  Returns -1 if num_planes or
 * bits_per_sample is invalid, otherwise 0.
 */
extern int image_flip_planes(P6(byte * buffer, const byte ** planes,
				int offset, int nbytes,
				int num_planes, int bits_per_sample));

#endif /* gsflip_INCLUDED */
