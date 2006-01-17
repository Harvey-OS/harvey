/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zimage3.c,v 1.7 2004/08/04 19:36:13 stefan Exp $ */
/* LanguageLevel 3 ImageTypes (3 & 4 - masked images) */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gscspace.h"		/* for gscolor2.h */
#include "gscolor2.h"
#include "gsiparm3.h"
#include "gsiparm4.h"
#include "gxiparam.h"		/* for image enumerator */
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "iimage.h"
#include "ialloc.h"

/* <dict> .image3 - */
private int
zimage3(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image3_t image;
    int interleave_type;
    ref *pDataDict;
    ref *pMaskDict;
    image_params ip_data, ip_mask;
    int ignored;
    int code, mcode;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if ((code = dict_int_param(op, "InterleaveType", 1, 3, -1,
			       &interleave_type)) < 0
	)
	return code;
    gs_image3_t_init(&image, NULL, interleave_type);
    if (dict_find_string(op, "DataDict", &pDataDict) <= 0 ||
	dict_find_string(op, "MaskDict", &pMaskDict) <= 0
	)
	return_error(e_rangecheck);
    if ((code = pixel_image_params(i_ctx_p, pDataDict,
				   (gs_pixel_image_t *)&image, &ip_data,
				   12, false)) < 0 ||
	(mcode = code = data_image_params(imemory, pMaskDict, &image.MaskDict,
				   &ip_mask, false, 1, 12, false)) < 0 ||
	(code = dict_int_param(pDataDict, "ImageType", 1, 1, 0, &ignored)) < 0 ||
	(code = dict_int_param(pMaskDict, "ImageType", 1, 1, 0, &ignored)) < 0
	)
	return code;
    /*
     * MaskDict must have a DataSource iff InterleaveType == 3.
     */
    if ((ip_data.MultipleDataSources && interleave_type != 3) ||
	ip_mask.MultipleDataSources ||
	mcode != (image.InterleaveType != 3)
	)
	return_error(e_rangecheck);
    if (image.InterleaveType == 3) {
	/* Insert the mask DataSource before the data DataSources. */
	memmove(&ip_data.DataSource[1], &ip_data.DataSource[0],
		(countof(ip_data.DataSource) - 1) *
		sizeof(ip_data.DataSource[0]));
	ip_data.DataSource[0] = ip_mask.DataSource[0];
    }
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image,
			&ip_data.DataSource[0],
			image.CombineWithColor, 1);
}

/* <dict> .image4 - */
private int
zimage4(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image4_t image;
    image_params ip;
    int num_components =
	gs_color_space_num_components(gs_currentcolorspace(igs));
    int colors[countof(image.MaskColor)];
    int code;
    int i;

    gs_image4_t_init(&image, NULL);
    code = pixel_image_params(i_ctx_p, op, (gs_pixel_image_t *)&image, &ip,
			      12, false);
    if (code < 0)
	return code;
    code = dict_int_array_check_param(op, "MaskColor", num_components * 2,
				      colors, 0, e_rangecheck);
    /* Clamp the color values to the unsigned range. */
    if (code == num_components) {
	image.MaskColor_is_range = false;
	for (i = 0; i < code; ++i)
	    image.MaskColor[i] = (colors[i] < 0 ? ~(uint)0 : colors[i]);
    }
    else if (code == num_components * 2) {
	image.MaskColor_is_range = true;
	for (i = 0; i < code; i += 2) {
	    if (colors[i+1] < 0) /* no match possible */
		image.MaskColor[i] = 1, image.MaskColor[i+1] = 0;
	    else {
		image.MaskColor[i+1] = colors[i+1];
		image.MaskColor[i] = max(colors[i], 0);
	    }
	}
    } else
	return_error(code < 0 ? code : gs_note_error(e_rangecheck));
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image, &ip.DataSource[0],
			image.CombineWithColor, 1);
}

/* ------ Initialization procedure ------ */

const op_def zimage3_op_defs[] =
{
    op_def_begin_ll3(),
    {"1.image3", zimage3},
    {"1.image4", zimage4},
    op_def_end(0)
};
