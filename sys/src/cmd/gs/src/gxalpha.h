/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxalpha.h,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Internal machinery for alpha channel support */

#ifndef gxalpha_INCLUDED
#  define gxalpha_INCLUDED

/*
 * As discussed in the classic Porter & Duff paper on compositing,
 * supporting alpha channel properly involves premultiplying color values
 * that are associated with non-unity alpha values.  After considerable
 * thrashing around trying to read between the lines of the spotty NeXT
 * documentation, we've concluded that the correct approach is to
 * premultiply towards whatever the color value 0 represents in the device's
 * native color space: black for DeviceGray and DeviceRGB (displays and some
 * file formats), white for DeviceCMYK (color printers), with a special hack
 * for monochrome printers TBD.  This makes things very easy internally, at
 * the expense of some inconsistency at the boundaries.
 *
 * For the record, the only places apparently affected by this decision
 * are the following:
 *      - alphaimage, if it doesn't assume premultiplication (see below)
 *      - readimage
 *      - The cmap_rgb_alpha_ procedures in gxcmap.c
 *      - [color]image, if they are supposed to use currentalpha (see below)
 *      - The compositing code in gsalphac.c
 *
 * The NeXT documentation also is very unclear as to how readimage,
 * alphaimage, and [color]image are supposed to work.  Our current
 * interpretation is the following:
 *
 *      - readimage reads pixels exactly as the device stores them
 *      (converted into DeviceGray or DeviceRGB space if the device
 *      uses a palette).  Pixels with non-unity alpha come out
 *      premultiplied, however the device stores them.
 *
 *      - alphaimage assumes the pixels are premultiplied as appropriate
 *      for the relevant color space.  This makes alphaimage and
 *      readimage complementary, i.e., the output of readimage is
 *      suitable as the input of alphaimage.
 *
 *      - [color]image disregard currentalpha, and treat all input as
 * opaque (alpha = 1).  */
/*
 * Just in case we ever change our minds about the direction of
 * premultiplication, uncommenting the following preprocessor definition is
 * supposed to produce premultiplication towards white.
 */
/*#define PREMULTIPLY_TOWARDS_WHITE */

#endif /* gxalpha_INCLUDED */
