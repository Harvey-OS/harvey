/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gximage4.c,v 1.6 2005/04/13 20:04:46 ray Exp $ */
/* ImageType 4 image implementation */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscspace.h"
#include "gsiparm4.h"
#include "gxiparam.h"
#include "gximage.h"
#include "stream.h"

/* Forward references */
private dev_proc_begin_typed_image(gx_begin_image4);

/* Structure descriptor */
private_st_gs_image4();

/* Define the image type for ImageType 4 images. */
private image_proc_sput(gx_image4_sput);
private image_proc_sget(gx_image4_sget);
private image_proc_release(gx_image4_release);
const gx_image_type_t gs_image_type_4 = {
    &st_gs_image4, gx_begin_image4, gx_data_image_source_size,
    gx_image4_sput, gx_image4_sget, gx_image4_release, 4
};
/*
 * The implementation is shared with ImageType 1, so we don't need our own
 * enum_procs.
 */
/*
  private const gx_image_enum_procs_t image4_enum_procs = {
    gx_image1_plane_data, gx_image1_end_image
  };
*/

/* Initialize an ImageType 4 image. */
void
gs_image4_t_init(gs_image4_t * pim, const gs_color_space * color_space)
{
    gs_pixel_image_t_init((gs_pixel_image_t *) pim, color_space);
    pim->type = &gs_image_type_4;
    pim->MaskColor_is_range = false;
}

/* Start processing an ImageType 4 image. */
private int
gx_begin_image4(gx_device * dev,
		const gs_imager_state * pis, const gs_matrix * pmat,
		const gs_image_common_t * pic, const gs_int_rect * prect,
		const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    gx_image_enum *penum;
    const gs_image4_t *pim = (const gs_image4_t *)pic;
    int code = gx_image_enum_alloc(pic, prect, mem, &penum);

    if (code < 0)
	return code;
    penum->alpha = gs_image_alpha_none;
    penum->masked = false;
    penum->adjust = fixed_0;
    /* Check that MaskColor values are within the valid range. */
    {
	bool opaque = false;
	uint max_value = (1 << pim->BitsPerComponent) - 1;
	int spp = cs_num_components(pim->ColorSpace);
	int i;

	for (i = 0; i < spp * 2; i += 2) {
	    uint c0, c1;

	    if (pim->MaskColor_is_range)
		c0 = pim->MaskColor[i], c1 = pim->MaskColor[i + 1];
	    else
		c0 = c1 = pim->MaskColor[i >> 1];

	    if ((c0 | c1) > max_value) {
		gs_free_object(mem, penum, "gx_begin_image4");
		return_error(gs_error_rangecheck);
	    }
	    if (c0 > c1) {
		opaque = true;	/* pixel can never match mask color */
		break;
	    }
	    penum->mask_color.values[i] = c0;
	    penum->mask_color.values[i + 1] = c1;
	}
	penum->use_mask_color = !opaque;
    }
    code = gx_image_enum_begin(dev, pis, pmat, pic, pdcolor, pcpath, mem,
			       penum);
    if (code >= 0)
	*pinfo = (gx_image_enum_common_t *)penum;
    return code;
}

/* Serialization */

private int
gx_image4_sput(const gs_image_common_t *pic, stream *s,
	       const gs_color_space **ppcs)
{
    const gs_image4_t *pim = (const gs_image4_t *)pic;
    bool is_range = pim->MaskColor_is_range;
    int code = gx_pixel_image_sput((const gs_pixel_image_t *)pim, s, ppcs,
				   is_range);
    int num_values =
	gs_color_space_num_components(pim->ColorSpace) * (is_range ? 2 : 1);
    int i;

    if (code < 0)
	return code;
    for (i = 0; i < num_values; ++i)
	sput_variable_uint(s, pim->MaskColor[i]);
    *ppcs = pim->ColorSpace;
    return 0;
}

private int
gx_image4_sget(gs_image_common_t *pic, stream *s,
	       const gs_color_space *pcs)
{
    gs_image4_t *const pim = (gs_image4_t *)pic;
    int num_values;
    int i;
    int code = gx_pixel_image_sget((gs_pixel_image_t *)pim, s, pcs);

    if (code < 0)
	return code;
    pim->type = &gs_image_type_4;
    pim->MaskColor_is_range = code;
    num_values =
	gs_color_space_num_components(pcs) *
	(pim->MaskColor_is_range ? 2 : 1);
    for (i = 0; i < num_values; ++i)
	sget_variable_uint(s, &pim->MaskColor[i]);
    return 0;
}

private void
gx_image4_release(gs_image_common_t *pic, gs_memory_t *mem)
{
    gx_pixel_image_release((gs_pixel_image_t *)pic, mem);
}
