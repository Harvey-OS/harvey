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

/*$Id: gximag3x.h,v 1.3 2000/09/19 19:00:38 lpd Exp $ */
/* ImageType 3x internal API */

#ifndef gximag3x_INCLUDED
#  define gximag3x_INCLUDED

#include "gsipar3x.h"
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
#define IMAGE3X_MAKE_MID_PROC(proc)\
  int proc(P6(gx_device **pmidev, gx_device *dev, int width, int height,\
	      int depth, gs_memory_t *mem))
typedef IMAGE3X_MAKE_MID_PROC((*image3x_make_mid_proc_t));

/*
 * Make the mask clip device -- the device that uses the mask image to
 * clip the (opaque) image data -- and its enumerator.
 * The device is closed and freed at the end of processing the image.
 */
#define IMAGE3X_MAKE_MCDE_PROC(proc)\
  int proc(P14(/* The initial arguments are those of begin_typed_image. */\
	       gx_device *dev,\
	       const gs_imager_state *pis,\
	       const gs_matrix *pmat,\
	       const gs_image_common_t *pic,\
	       const gs_int_rect *prect,\
	       const gx_drawing_color *pdcolor,\
	       const gx_clip_path *pcpath, gs_memory_t *mem,\
	       gx_image_enum_common_t **pinfo,\
	       /* The following arguments are added. */\
	       gx_device **pmcdev, gx_device *midev[2],\
	       gx_image_enum_common_t *pminfo[2],\
	       const gs_int_point origin[2],\
	       const gs_image3x_t *pim))
typedef IMAGE3X_MAKE_MCDE_PROC((*image3x_make_mcde_proc_t));

/*
 * Begin processing an ImageType 3x image, with the mask device creation
 * procedures as additional parameters.
 */
int gx_begin_image3x_generic(P11(gx_device * dev,
				 const gs_imager_state *pis,
				 const gs_matrix *pmat,
				 const gs_image_common_t *pic,
				 const gs_int_rect *prect,
				 const gx_drawing_color *pdcolor,
				 const gx_clip_path *pcpath, gs_memory_t *mem,
				 IMAGE3X_MAKE_MID_PROC((*make_mid)),
				 IMAGE3X_MAKE_MCDE_PROC((*make_mcde)),
				 gx_image_enum_common_t **pinfo));

#endif /* gximag3x_INCLUDED */
