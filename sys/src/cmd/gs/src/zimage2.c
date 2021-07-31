/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: zimage2.c,v 1.3 2000/09/19 19:00:54 lpd Exp $ */
/* image operator extensions for Level 2 PostScript */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gscolor.h"
#include "gscspace.h"
#include "gscolor2.h"
#include "gsmatrix.h"
#include "gsimage.h"
#include "gxfixed.h"
#include "idict.h"
#include "idparam.h"
#include "iimage.h"
#include "iimage2.h"
#include "ilevel.h"
#include "igstate.h"		/* for igs */

/* Extract and check the parameters for a gs_data_image_t. */
int
data_image_params(const ref *op, gs_data_image_t *pim,
		  image_params *pip, bool require_DataSource,
		  int num_components, int max_bits_per_component)
{
    int code;
    int decode_size;
    ref *pds;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if ((code = dict_int_param(op, "Width", 0, max_int_in_fixed / 2,
			       -1, &pim->Width)) < 0 ||
	(code = dict_int_param(op, "Height", 0, max_int_in_fixed / 2,
			       -1, &pim->Height)) < 0 ||
	(code = dict_matrix_param(op, "ImageMatrix",
				  &pim->ImageMatrix)) < 0 ||
	(code = dict_bool_param(op, "MultipleDataSources", false,
				&pip->MultipleDataSources)) < 0 ||
	(code = dict_int_param(op, "BitsPerComponent", 1,
			       max_bits_per_component, -1,
			       &pim->BitsPerComponent)) < 0 ||
	(code = decode_size = dict_floats_param(op, "Decode",
						num_components * 2,
						&pim->Decode[0], NULL)) < 0 ||
	(code = dict_bool_param(op, "Interpolate", false,
				&pim->Interpolate)) < 0
	)
	return code;
    pip->pDecode = &pim->Decode[0];
    /* Extract and check the data sources. */
    if ((code = dict_find_string(op, "DataSource", &pds)) <= 0) {
	if (require_DataSource)
	    return (code < 0 ? code : gs_note_error(e_rangecheck));
	return 1;		/* no data source */
    }
    if (pip->MultipleDataSources) {
	check_type_only(*pds, t_array);
	if (r_size(pds) != num_components)
	    return_error(e_rangecheck);
	memcpy(&pip->DataSource[0], pds->value.refs,
	       sizeof(ref) * num_components);
    } else
	pip->DataSource[0] = *pds;
    return 0;
}

/* Extract and check the parameters for a gs_pixel_image_t. */
int
pixel_image_params(i_ctx_t *i_ctx_p, const ref *op, gs_pixel_image_t *pim,
		   image_params *pip, int max_bits_per_component)
{
    int num_components =
	gs_color_space_num_components(gs_currentcolorspace(igs));
    int code;

    if (num_components < 1)
	return_error(e_rangecheck);	/* Pattern space not allowed */
    pim->ColorSpace = gs_currentcolorspace(igs);
    code = data_image_params(op, (gs_data_image_t *) pim, pip, true,
			     num_components, max_bits_per_component);
    if (code < 0)
	return code;
    pim->format =
	(pip->MultipleDataSources ? gs_image_format_component_planar :
	 gs_image_format_chunky);
    return dict_bool_param(op, "CombineWithColor", false,
			   &pim->CombineWithColor);
}

/* <dict> .image1 - */
private int
zimage1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image_t image;
    image_params ip;
    int code;

    gs_image_t_init(&image, gs_currentcolorspace(igs));
    code = pixel_image_params(i_ctx_p, op, (gs_pixel_image_t *)&image, &ip,
			      12);
    if (code < 0)
	return code;
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image, &ip.DataSource[0],
			image.CombineWithColor, 1);
}

/* <dict> .imagemask1 - */
private int
zimagemask1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image_t image;
    image_params ip;
    int code;

    gs_image_t_init_mask_adjust(&image, false,
				gs_incachedevice(igs) != CACHE_DEVICE_NONE);
    code = data_image_params(op, (gs_data_image_t *) & image, &ip, true, 1, 1);
    if (code < 0)
	return code;
    if (ip.MultipleDataSources)
	return_error(e_rangecheck);
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image, &ip.DataSource[0],
			true, 1);
}

/*
 * Process an image that has no explicit source data.  This isn't used by
 * standard Level 2, but it's a very small procedure and is needed by
 * both zdps.c and zdpnext.c.
 */
int
process_non_source_image(i_ctx_t *i_ctx_p, const gs_image_common_t * pic,
			 client_name_t cname)
{
    gx_image_enum_common_t *pie;
    int code = gs_image_begin_typed(pic, igs, false /****** WRONG ******/ ,
				    &pie);

    /* We didn't pass any data, so there's nothing to clean up. */
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zimage2_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"1.image1", zimage1},
    {"1.imagemask1", zimagemask1},
    op_def_end(0)
};
