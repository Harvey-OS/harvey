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

/* $Id: gximage3.h,v 1.8 2002/06/16 08:55:53 lpd Exp $ */
/* ImageType 3 internal API */

#ifndef gximage3_INCLUDED
#  define gximage3_INCLUDED

#include "gsiparm3.h"
#include "gxiparam.h"

/*
 * The machinery for splitting an ImageType3 image into pixel and mask data
 * is used both for imaging per se and for writing high-level output.
 * We implement this by making the procedures for setting up the mask image
 * and clipping devices virtual.
 */

/*
 * Make the mask image device -- the device that processes mask bits.
 * The device is closed and freed at the end of processing the image.
 */
#define IMAGE3_MAKE_MID_PROC(proc)\
  int proc(gx_device **pmidev, gx_device *dev, int width, int height,\
	   gs_memory_t *mem)
typedef IMAGE3_MAKE_MID_PROC((*image3_make_mid_proc_t));

/*
 * Make the mask clip device -- the device that uses the mask image to
 * clip the (opaque) image data -- and its enumerator.
 * The device is closed and freed at the end of processing the image.
 */
#define IMAGE3_MAKE_MCDE_PROC(proc)\
  int proc(/* The initial arguments are those of begin_typed_image. */\
	       gx_device *dev,\
	   const gs_imager_state *pis,\
	   const gs_matrix *pmat,\
	   const gs_image_common_t *pic,\
	   const gs_int_rect *prect,\
	   const gx_drawing_color *pdcolor,\
	   const gx_clip_path *pcpath, gs_memory_t *mem,\
	   gx_image_enum_common_t **pinfo,\
	   /* The following arguments are added. */\
	   gx_device **pmcdev, gx_device *midev,\
	   gx_image_enum_common_t *pminfo,\
	   const gs_int_point *origin)
typedef IMAGE3_MAKE_MCDE_PROC((*image3_make_mcde_proc_t));

/*
 * Begin processing an ImageType 3 image, with the mask device creation
 * procedures as additional parameters.
 */
int gx_begin_image3_generic(gx_device * dev,
			    const gs_imager_state *pis,
			    const gs_matrix *pmat,
			    const gs_image_common_t *pic,
			    const gs_int_rect *prect,
			    const gx_drawing_color *pdcolor,
			    const gx_clip_path *pcpath, gs_memory_t *mem,
			    IMAGE3_MAKE_MID_PROC((*make_mid)),
			    IMAGE3_MAKE_MCDE_PROC((*make_mcde)),
			    gx_image_enum_common_t **pinfo);

#endif /* gximage3_INCLUDED */
