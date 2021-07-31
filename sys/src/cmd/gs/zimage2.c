/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zimage2.c */
/* image operator extensions for Level 2 PostScript */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gscolor.h"
#include "gscspace.h"
#include "gscolor2.h"
#include "gsmatrix.h"
#include "idict.h"
#include "idparam.h"
#include "ilevel.h"
#include "igstate.h"		/* for igs */

/* Imported from zpaint.c */
extern int zimage_setup(P11(int width, int height, gs_matrix *pmat,
  ref *sources, int bits_per_component,
  bool multi, const gs_color_space *pcs, int masked,
  const float *decode, bool interpolate, int npop));
extern int zimage(P1(os_ptr));
extern int zimagemask(P1(os_ptr));

/* Define a structure for acquiring image parameters. */
typedef struct image_params_s {
	int Width;
	int Height;
	gs_matrix ImageMatrix;
	bool MultipleDataSources;
	ref DataSource[4];
	int BitsPerComponent;
	float Decode[2*4];
	const float *pDecode;
	bool Interpolate;
} image_params;

/* Common code for unpacking an image dictionary. */
/* Assume *op is a dictionary. */
private int
image_dict_unpack(os_ptr op, image_params *pip, int max_bits_per_component)
{	int code;
	int num_components;
	int decode_size;
	ref *pds;
	check_dict_read(*op);
	num_components = gs_currentcolorspace(igs)->type->num_components;
	if ( num_components < 1 )
		return_error(e_rangecheck);
	if ( max_bits_per_component == 1 )	/* imagemask */
		num_components = 1;		/* for Decode */
	if ( (code = dict_int_param(op, "ImageType", 1, 1, 1,
				    &code)) < 0 ||
	     (code = dict_int_param(op, "Width", 0, 0x7fff, -1,
				    &pip->Width)) < 0 ||
	     (code = dict_int_param(op, "Height", 0, 0x7fff, -1,
				    &pip->Height)) < 0 ||
	     (code = dict_matrix_param(op, "ImageMatrix",
				    &pip->ImageMatrix)) < 0 ||
	     (code = dict_bool_param(op, "MultipleDataSources", false,
				    &pip->MultipleDataSources)) < 0 ||
	     (code = dict_int_param(op, "BitsPerComponent", 0,
				    max_bits_per_component, -1,
				    &pip->BitsPerComponent)) < 0 ||
	     (code = decode_size = dict_float_array_param(op, "Decode",
				    num_components * 2,
				    &pip->Decode[0], NULL)) < 0 ||
	     (code = dict_bool_param(op, "Interpolate", false,
				    &pip->Interpolate)) < 0
	   )
		return code;
	if ( decode_size == 0 )
		pip->pDecode = 0;
	else if ( decode_size != num_components * 2 )
		return_error(e_rangecheck);
	else
		pip->pDecode = &pip->Decode[0];
	/* Extract and check the data sources. */
	if ( (code = dict_find_string(op, "DataSource", &pds)) < 0 )
		return code;
	if ( pip->MultipleDataSources )
	{	check_type_only(*pds, t_array);
		if ( r_size(pds) != num_components )
			return_error(e_rangecheck);
		memcpy(&pip->DataSource[0], pds->value.refs, sizeof(ref) * num_components);
	}
	else
		pip->DataSource[0] = *pds;
	return 0;
}

/* (<width> <height> <bits/sample> <matrix> <datasrc> image -) */
/* <dict> image - */
int
z2image(register os_ptr op)
{	if ( level2_enabled )
	{	check_op(1);
		if ( r_has_type(op, t_dictionary) )
		  {	image_params ip;
			int code = image_dict_unpack(op, &ip, 12);
			if ( code < 0 )
			  return code;
			return zimage_setup(ip.Width, ip.Height,
					    &ip.ImageMatrix,
					    &ip.DataSource[0],
					    ip.BitsPerComponent,
					    ip.MultipleDataSources,
					    gs_currentcolorspace(igs),
					    0, ip.pDecode, ip.Interpolate, 1);
		  }
	}
	/* Level 1 image operator */
	check_op(5);
	return zimage(op);
}

/* (<width> <height> <paint_1s> <matrix> <datasrc> imagemask -) */
/* <dict> imagemask - */
int
z2imagemask(register os_ptr op)
{	if ( level2_enabled )
	{	check_op(1);
		if ( r_has_type(op, t_dictionary) )
		  {	image_params ip;
			gs_color_space cs;
			int code = image_dict_unpack(op, &ip, 1);
			if ( code < 0 )
			  return code;
			if ( ip.MultipleDataSources )
			  return_error(e_rangecheck);
			cs.type = &gs_color_space_type_DeviceGray;
			return zimage_setup(ip.Width, ip.Height,
					    &ip.ImageMatrix,
					    &ip.DataSource[0], 1, false, &cs,
					    1, ip.pDecode, ip.Interpolate, 1);
		  }
	}
	/* Level 1 imagemask operator */
	check_op(5);
	return zimagemask(op);
}

/* ------ Experimental code ------ */

#include "gsiscale.h"
#include "ialloc.h"

/* <indata> <w> <h> <mat> <outdata> .scaleimage - */
/* Rescale an image represented as an array of bytes (representing values */
/* in the range [0..1]) using filtering. */
private int
zscaleimage(os_ptr op)
{	const_os_ptr rin = op - 4;
	const_os_ptr rout = op;
	gs_matrix mat;
	int code;
	gs_image_scale_state iss;

	check_write_type(*rout, t_string);
	if ( (code = read_matrix(op - 1, &mat)) < 0 )
	  return code;
	if ( mat.xx < 1 || mat.xy != 0 || mat.yx != 0 || mat.yy < 1 )
	  return_error(e_rangecheck);
	check_type(op[-2], t_integer);
	check_type(op[-3], t_integer);
	check_read_type(*rin, t_string);
	if ( op[-3].value.intval <= 0 || op[-2].value.intval <= 0 ||
	     r_size(rin) != op[-3].value.intval * op[-2].value.intval
	   )
	  return_error(e_rangecheck);
	iss.sizeofPixelIn = 1;
	iss.maxPixelIn = 0xff;
	iss.sizeofPixelOut = 1;
	iss.maxPixelOut = 0xff;
	iss.src_width = op[-3].value.intval;
	iss.src_height = op[-2].value.intval;
	iss.dst_width = (int)ceil(iss.src_width * mat.xx);
	iss.dst_height = (int)ceil(iss.src_height * mat.yy);
	if ( r_size(rout) != iss.dst_width * iss.dst_height )
	  return_error(e_rangecheck);
	iss.values_per_pixel = 1;
	iss.memory = imemory;
	code = gs_image_scale_init(&iss, false, false);
	if ( code < 0 )
	  return code;
	code = gs_image_scale(rout->value.bytes, rin->value.bytes, &iss);
	gs_image_scale_cleanup(&iss);
	pop(5);
	return 0;
}

/* ------ Initialization procedure ------ */

/* Note that these override the definitions in zpaint.c. */
BEGIN_OP_DEFS(zimage2_l2_op_defs) {
		op_def_begin_level2(),
	{"1image", z2image},
	{"1imagemask", z2imagemask},
	{"5.scaleimage", zscaleimage},
END_OP_DEFS(0) }
