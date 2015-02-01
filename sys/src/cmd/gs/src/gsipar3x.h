/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsipar3x.h,v 1.7 2002/06/16 08:45:42 lpd Exp $ */
/* Extended ImageType 3 ("3x") image parameter definition */

#ifndef gsipar3x_INCLUDED
#  define gsipar3x_INCLUDED

#include "gsiparam.h"
#include "gsiparm3.h"		/* for interleave types */

/*
 * An ImageType 3x image is the transparency-capable extension of an
 * ImageType 3 image.  Instead of a MaskDict, it has an OpacityMaskDict
 * and/or a ShapeMaskDict, whose depths may be greater than 1.  It also
 * has an optional Matte member, defining a pre-mixed background color.
 *
 * Since ImageTypes must be numeric, we assign the number 103 for this
 * type of image; however, this is defined in only one place, namely here.
 */
#define IMAGE3X_IMAGETYPE 103

/*
 * If InterleaveType is 3, the data source(s) for the mask(s) *precede* the
 * data sources for the pixel data, with opacity preceding shape.  For
 * InterleaveType 3, the client is responsible for always providing mask
 * data before the pixel data that it masks.  (The implementation does not
 * currently check this, but it should.)  For this type of image,
 * InterleaveType 2 (interleaved scan lines) is not allowed.
 */
typedef struct gs_image3x_mask_s {
    int InterleaveType;
    float Matte[GS_CLIENT_COLOR_MAX_COMPONENTS];
    bool has_Matte;
    /*
     * Note that the ColorSpaces in the MaskDicts are ignored.
     * Note also that MaskDict.BitsPerComponent may be zero, which
     * indicates that the given mask is not supplied.
     */
    gs_data_image_t MaskDict;
} gs_image3x_mask_t;
typedef struct gs_image3x_s {
    gs_pixel_image_common;	/* DataDict */
    gs_image3x_mask_t Opacity, Shape; /* ...MaskDict */
} gs_image3x_t;

/* As noted above, the ColorSpaces in the MaskDicts are ignored. */
#define private_st_gs_image3x()	/* in gximag3x.c */\
  gs_private_st_suffix_add0(st_gs_image3x, gs_image3x_t, "gs_image3x_t",\
    image3x_enum_ptrs, image3x_reloc_ptrs, st_gs_pixel_image)

/*
 * Initialize an ImageType 3x image.
 */
void gs_image3x_t_init(gs_image3x_t *pim, const gs_color_space *color_space);

#endif /* gsipar3x_INCLUDED */
