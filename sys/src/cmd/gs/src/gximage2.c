/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gximage2.c,v 1.5 2002/08/22 07:12:29 henrys Exp $ */
/* ImageType 2 image implementation */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gscspace.h"
#include "gscpixel.h"
#include "gsdevice.h"
#include "gsiparm2.h"
#include "gxgetbit.h"
#include "gxiparam.h"
#include "gxpath.h"
#include "gscolor2.h"

/* Forward references */
private dev_proc_begin_typed_image(gx_begin_image2);
private image_proc_source_size(gx_image2_source_size);

/* Structure descriptor */
private_st_gs_image2();

/* Define the image type for ImageType 2 images. */
const gx_image_type_t gs_image_type_2 = {
    &st_gs_image2, gx_begin_image2, gx_image2_source_size,
    gx_image_no_sput, gx_image_no_sget, gx_image_default_release, 2
};

/* Initialize an ImageType 2 image. */
void
gs_image2_t_init(gs_image2_t * pim)
{
    pim->type = &gs_image_type_2;
    pim->UnpaintedPath = 0;
    pim->PixelCopy = false;
}

/*
 * Compute the device space coordinates and source data size for an
 * ImageType 2 image.  This procedure fills in
 * image.{Width,Height,ImageMatrix}.
 */
typedef struct image2_data_s {
    gs_point origin;
    gs_int_rect bbox;
    gs_image1_t image;
} image2_data_t;
private int
image2_set_data(const gs_image2_t * pim, image2_data_t * pid)
{
    gs_state *pgs = pim->DataSource;
    gs_matrix smat;
    gs_rect sbox, dbox;

    gs_transform(pgs, pim->XOrigin, pim->YOrigin, &pid->origin);
    sbox.q.x = (sbox.p.x = pim->XOrigin) + pim->Width;
    sbox.q.y = (sbox.p.y = pim->YOrigin) + pim->Height;
    gs_currentmatrix(pgs, &smat);
    gs_bbox_transform(&sbox, &smat, &dbox);
    pid->bbox.p.x = (int)floor(dbox.p.x);
    pid->bbox.p.y = (int)floor(dbox.p.y);
    pid->bbox.q.x = (int)ceil(dbox.q.x);
    pid->bbox.q.y = (int)ceil(dbox.q.y);
    pid->image.Width = pid->bbox.q.x - pid->bbox.p.x;
    pid->image.Height = pid->bbox.q.y - pid->bbox.p.y;
    pid->image.ImageMatrix = pim->ImageMatrix;
    return 0;
}

/* Compute the source size of an ImageType 2 image. */
private int
gx_image2_source_size(const gs_imager_state * pis, const gs_image_common_t * pim,
		      gs_int_point * psize)
{
    image2_data_t idata;

    image2_set_data((const gs_image2_t *)pim, &idata);
    psize->x = idata.image.Width;
    psize->y = idata.image.Height;
    return 0;
}

/* Begin an ImageType 2 image. */
/* Note that since ImageType 2 images don't have any source data, */
/* this procedure does all the work. */
private int
gx_begin_image2(gx_device * dev,
		const gs_imager_state * pis, const gs_matrix * pmat,
		const gs_image_common_t * pic, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    const gs_image2_t *pim = (const gs_image2_t *)pic;
    gs_state *pgs = pim->DataSource;
    gx_device *sdev = gs_currentdevice(pgs);
    int depth = sdev->color_info.depth;
    bool pixel_copy = pim->PixelCopy;
    bool has_alpha;
    bool direct_copy = false;
    image2_data_t idata;
    byte *row;
    uint row_size, source_size;
    gx_image_enum_common_t *info;
    gs_matrix smat, dmat;
    int code;

    /* verify that color models are the same for PixelCopy */
    if ( pixel_copy                            &&
         memcmp( &dev->color_info,
                 &sdev->color_info,
                 sizeof(dev->color_info) ) != 0  )
        return_error(gs_error_typecheck);

/****** ONLY HANDLE depth <= 8 FOR PixelCopy ******/
    if (pixel_copy && depth <= 8)
        return_error(gs_error_unregistered);

    gs_image_t_init(&idata.image, gs_currentcolorspace((const gs_state *)pis));

    /* Add Decode entries for K and alpha */
    idata.image.Decode[6] = idata.image.Decode[8] = 0.0;
    idata.image.Decode[7] = idata.image.Decode[9] = 1.0;
    if (pmat == 0) {
	gs_currentmatrix((const gs_state *)pis, &dmat);
	pmat = &dmat;
    } else
	dmat = *pmat;
    gs_currentmatrix(pgs, &smat);
    code = image2_set_data(pim, &idata);
    if (code < 0)
	return code;
/****** ONLY HANDLE SIMPLE CASES FOR NOW ******/
    if (idata.bbox.p.x != floor(idata.origin.x))
	return_error(gs_error_rangecheck);
    if (!(idata.bbox.p.y == floor(idata.origin.y) ||
	  idata.bbox.q.y == ceil(idata.origin.y))
	)
	return_error(gs_error_rangecheck);
    source_size = (idata.image.Width * depth + 7) >> 3;
    row_size = max(3 * idata.image.Width, source_size);
    row = gs_alloc_bytes(mem, row_size, "gx_begin_image2");
    if (row == 0)
	return_error(gs_error_VMerror);
    if (pixel_copy) {
	idata.image.BitsPerComponent = depth;
	has_alpha = false;	/* no separate alpha channel */

	if ( pcpath == NULL ||
	     gx_cpath_includes_rectangle(pcpath,
				     int2fixed(idata.bbox.p.x),
				     int2fixed(idata.bbox.p.y),
				     int2fixed(idata.bbox.q.x),
				     int2fixed(idata.bbox.q.y)) ) {
	    gs_matrix mat;


	    /*
	     * Figure 7.2 of the Adobe 3010 Supplement says that we should
	     * compute CTM x ImageMatrix here, but I'm almost certain it
	     * should be the other way around.  Also see gdevx.c.
	     */
	    gs_matrix_multiply(&idata.image.ImageMatrix, &smat, &mat);
	    direct_copy =
	        (is_xxyy(&dmat) || is_xyyx(&dmat)) &&
#define eqe(e) mat.e == dmat.e
	        eqe(xx) && eqe(xy) && eqe(yx) && eqe(yy);
#undef eqe
        }
    } else {
	idata.image.BitsPerComponent = 8;

	/* Always use RGB source color for now.
         *
	 * The source device has alpha if the same RGB values with
	 * different alphas map to different pixel values.
	 ****** THIS IS NOT GOOD ENOUGH: WE WANT TO SKIP TRANSFERRING
	 ****** ALPHA IF THE SOURCE IS CAPABLE OF HAVING ALPHA BUT
	 ****** DOESN'T CURRENTLY HAVE ANY ACTUAL ALPHA VALUES DIFFERENT
	 ****** FROM 1.
	 */
	/*
	 * Since the default implementation of map_rgb_alpha_color
	 * premultiplies the color towards white, we can't just test
	 * whether changing alpha has an effect on the color.
	 */
	{
	    gx_color_index trans_black =
	    (*dev_proc(sdev, map_rgb_alpha_color))
	    (sdev, (gx_color_value) 0, (gx_color_value) 0,
	     (gx_color_value) 0, (gx_color_value) 0);

	    has_alpha =
		trans_black != (*dev_proc(sdev, map_rgb_alpha_color))
		(sdev, (gx_color_value) 0, (gx_color_value) 0,
		 (gx_color_value) 0, gx_max_color_value) &&
		trans_black != (*dev_proc(sdev, map_rgb_alpha_color))
		(sdev, gx_max_color_value, gx_max_color_value,
		 gx_max_color_value, gx_max_color_value);
	}
    }
    idata.image.Alpha =
	(has_alpha ? gs_image_alpha_last : gs_image_alpha_none);
    if (smat.yy < 0) {
	/*
	 * The source Y axis is reflected.  Reflect the mapping from
	 * user space to source data.
	 */
	idata.image.ImageMatrix.ty += idata.image.Height *
	    idata.image.ImageMatrix.yy;
	idata.image.ImageMatrix.xy = -idata.image.ImageMatrix.xy;
	idata.image.ImageMatrix.yy = -idata.image.ImageMatrix.yy;
    }
    if (!direct_copy)
	code = (*dev_proc(dev, begin_typed_image))
	    (dev, pis, pmat, (const gs_image_common_t *)&idata.image, NULL,
	     pdcolor, pcpath, mem, &info);
    if (code >= 0) {
	int y;
	gs_int_rect rect;
	gs_get_bits_params_t params;
	const byte *data;
	uint offset = row_size - source_size;

	rect = idata.bbox;
	for (y = 0; code >= 0 && y < idata.image.Height; ++y) {
	    gs_int_rect *unread = 0;
	    int num_unread;

/****** y COMPUTATION IS ROUNDED -- WRONG ******/
	    rect.q.y = rect.p.y + 1;
	    /* Insist on x_offset = 0 to simplify the conversion loop. */
	    params.options =
		GB_ALIGN_ANY | (GB_RETURN_COPY | GB_RETURN_POINTER) |
		GB_OFFSET_0 | (GB_RASTER_STANDARD | GB_RASTER_ANY) |
		GB_PACKING_CHUNKY;
	    if (pixel_copy) {
		params.options |= GB_COLORS_NATIVE;
		params.data[0] = row + offset;
		code = (*dev_proc(sdev, get_bits_rectangle))
		    (sdev, &rect, &params, &unread);
		if (code < 0)
		    break;
		num_unread = code;
		data = params.data[0];
		if (direct_copy) {
		    /*
		     * Copy the pixels directly to the destination.
		     * We know that the transformation is only a translation,
		     * but we must handle an inverted destination Y axis.
		     */
		    code = (*dev_proc(dev, copy_color))
			(dev, data, 0, row_size, gx_no_bitmap_id,
			 (int)(dmat.tx - idata.image.ImageMatrix.tx),
			 (int)(dmat.ty - idata.image.ImageMatrix.ty +
			       (dmat.yy < 0 ? ~y : y)),
			 idata.image.Width, 1);
		    continue;
		}
	    } else {
		/*
		 * Convert the pixels to pure colors.  This may be very
		 * slow and painful.  Eventually we will use indexed color for
		 * narrow pixels.
		 */
		/* Always use RGB source color for now. */
		params.options |=
		    GB_COLORS_RGB | GB_DEPTH_8 |
		    (has_alpha ? GB_ALPHA_LAST : GB_ALPHA_NONE);
		params.data[0] = row;
		code = (*dev_proc(sdev, get_bits_rectangle))
		    (sdev, &rect, &params, &unread);
		if (code < 0)
		    break;
		num_unread = code;
		data = params.data[0];
	    }
	    if (num_unread > 0 && pim->UnpaintedPath) {
		/* Add the rectangle(s) to the unpainted path. */
		int i;

		for (i = 0; code >= 0 && i < num_unread; ++i)
		    code = gx_path_add_rectangle(pim->UnpaintedPath,
						 int2fixed(unread[i].p.x),
						 int2fixed(unread[i].p.y),
						 int2fixed(unread[i].q.x),
						 int2fixed(unread[i].q.y));
		gs_free_object(dev->memory, unread, "UnpaintedPath unread");
	    }
	    code = gx_image_data(info, &data, 0, row_size, 1);
	    rect.p.y = rect.q.y;
	}
	if (!direct_copy) {
	    if (code >= 0)
		code = gx_image_end(info, true);
	    else
		discard(gx_image_end(info, false));
	}
    }
    gs_free_object(mem, row, "gx_begin_image2");
    return (code < 0 ? code : 1);
}
