/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsipar3x.h,v 1.3 2000/09/19 19:00:29 lpd Exp $ */
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
    /* Note that the ColorSpaces in the MaskDicts are ignored. */
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
void gs_image3x_t_init(P2(gs_image3x_t *pim,
			  const gs_color_space *color_space));

#endif /* gsipar3x_INCLUDED */
