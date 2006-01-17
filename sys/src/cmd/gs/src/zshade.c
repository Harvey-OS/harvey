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

/* $Id: zshade.c,v 1.15 2004/08/04 19:36:13 stefan Exp $ */
/* PostScript language interface to shading */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gscolor3.h"
#include "gscspace.h"
#include "gscolor2.h"		/* requires gscspace.h */
#include "gsfunc3.h"
#include "gsptype2.h"
#include "gsstruct.h"		/* must precede gsshade.h */
#include "gsshade.h"
#include "gsuid.h"
#include "stream.h"		/* for files.h */
#include "files.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "ifunc.h"
#include "igstate.h"
#include "ipcolor.h"
#include "store.h"

/* Forward references */
private int shading_param(const_os_ptr op, const gs_shading_t ** ppsh);

/* ---------------- Standard operators ---------------- */

/* - currentsmoothness <smoothness> */
private int
zcurrentsmoothness(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_real(op, gs_currentsmoothness(igs));
    return 0;
}

/* <smoothness> setsmoothness - */
private int
zsetsmoothness(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double smoothness;
    int code;

    if (real_param(op, &smoothness) < 0)
	return_op_typecheck(op);
    if ((code = gs_setsmoothness(igs, smoothness)) < 0)
	return code;
    pop(1);
    return 0;
}

/* <shading> .shfill - */
private int
zshfill(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const gs_shading_t *psh;
    int code = shading_param(op, &psh);

    if (code < 0 || (code = gs_shfill(igs, psh)) < 0)
	return code;
    pop(1);
    return 0;
}

/* ------ Non-standard operators ------ */

/* <pattern> <matrix> <shading> .buildshadingpattern <pattern> <instance> */
private int
zbuildshadingpattern(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op2 = op - 2;
    gs_matrix mat;
    gs_pattern2_template_t template;
    int_pattern *pdata;
    gs_client_color cc_instance;
    int code;

    check_type(*op2, t_dictionary);
    check_dict_read(*op2);
    gs_pattern2_init(&template);
    if ((code = read_matrix(imemory, op - 1, &mat)) < 0 ||
	(code = dict_uid_param(op2, &template.uid, 1, imemory, i_ctx_p)) != 1 ||
	(code = shading_param(op, &template.Shading)) < 0 ||
	(code = int_pattern_alloc(&pdata, op2, imemory)) < 0
	)
	return_error((code < 0 ? code : e_rangecheck));
    template.client_data = pdata;
    code = gs_make_pattern(&cc_instance,
			   (const gs_pattern_template_t *)&template,
			   &mat, igs, imemory);
    if (code < 0) {
	ifree_object(pdata, "int_pattern");
	return code;
    }
    make_istruct(op - 1, a_readonly, cc_instance.pattern);
    pop(1);
    return code;
}

/* ------ Internal procedures ------ */

/* Get a shading parameter. */
private int
shading_param(const_os_ptr op, const gs_shading_t ** ppsh)
{	/*
	 * Since shadings form a subclass hierarchy, we currently have
	 * no way to check whether a structure is actually a shading.
	 */
    if (!r_is_struct(op) ||
	r_has_masked_attrs(op, a_executable | a_execute, a_all)
	)
	return_error(e_typecheck);
    *ppsh = (gs_shading_t *) op->value.pstruct;
    return 0;
}

/* ---------------- Shading dictionaries ---------------- */

/* ------ Common code ------ */

extern_st(st_color_space);

typedef int (*build_shading_proc_t)
     (i_ctx_t *i_ctx_p, const ref *op, const gs_shading_params_t *params,
      gs_shading_t **ppsh, gs_memory_t *mem);

/* Operators */

/* Common framework for building shadings. */
private int
build_shading(i_ctx_t *i_ctx_p, build_shading_proc_t proc)
{
    os_ptr op = osp;
    int code;
    float box[4];
    gs_shading_params_t params;
    gs_shading_t *psh;
    ref *pvalue;

    check_type(*op, t_dictionary);
    params.ColorSpace = 0;
    params.Background = 0;
    /* Collect parameters common to all shading types. */
    {
	const gs_color_space *pcs_orig = gs_currentcolorspace(igs);
	int num_comp = gs_color_space_num_components(pcs_orig);
	gs_color_space *pcs;

	if (num_comp < 0)	/* Pattern color space */
	    return_error(e_rangecheck);
	pcs = ialloc_struct(gs_color_space, &st_color_space,
			    "build_shading");
	if (pcs == 0)
	    return_error(e_VMerror);
	gs_cspace_init_from(pcs, pcs_orig);
	params.ColorSpace = pcs;
	if (dict_find_string(op, "Background", &pvalue) > 0) {
	    gs_client_color *pcc =
		ialloc_struct(gs_client_color, &st_client_color,
			      "build_shading");

	    if (pcc == 0) {
		code = gs_note_error(e_VMerror);
		goto fail;
	    }
	    pcc->pattern = 0;
	    params.Background = pcc;
	    code = dict_floats_param(imemory, op, "Background",
				     gs_color_space_num_components(pcs),
				     pcc->paint.values, NULL);
	    if (code < 0)
		goto fail;
	}
    }
    if (dict_find_string(op, "BBox", &pvalue) <= 0)
	params.have_BBox = false;
    else if ((code = dict_floats_param(imemory, op, "BBox", 
				       4, box, NULL)) == 4) {
	params.BBox.p.x = box[0];
	params.BBox.p.y = box[1];
	params.BBox.q.x = box[2];
	params.BBox.q.y = box[3];
	params.have_BBox = true;
    } else
	goto fail;
    code = dict_bool_param(op, "AntiAlias", false, &params.AntiAlias);
    if (code < 0)
	goto fail;
    /* Finish building the shading. */
    code = (*proc)(i_ctx_p, op, &params, &psh, imemory);
    if (code < 0)
	goto fail;
    make_istruct_new(op, 0, psh);
    return code;
fail:
    gs_free_object(imemory, params.Background, "Background");
    if (params.ColorSpace) {
	gs_cspace_release(params.ColorSpace);
	gs_free_object(imemory, params.ColorSpace, "ColorSpace");
    }
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}

/* Collect a Function value. */
private int
build_shading_function(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn,
		       int num_inputs, gs_memory_t *mem)
{
    ref *pFunction;
    int code;

    *ppfn = 0;
    if (dict_find_string(op, "Function", &pFunction) <= 0)
	return 0;
    if (r_is_array(pFunction)) {
	uint size = r_size(pFunction);
	gs_function_t **Functions;
	uint i;
	gs_function_AdOt_params_t params;

	check_read(*pFunction);
	if (size == 0)
	    return_error(e_rangecheck);
	code = alloc_function_array(size, &Functions, mem);
	if (code < 0)
	    return code;
	for (i = 0; i < size; ++i) {
	    ref rsubfn;

	    array_get(imemory, pFunction, (long)i, &rsubfn);
	    code = fn_build_function(i_ctx_p, &rsubfn, &Functions[i], mem);
	    if (code < 0)
		break;
	}
	params.m = num_inputs;
	params.Domain = 0;
	params.n = size;
	params.Range = 0;
	params.Functions = (const gs_function_t * const *)Functions;
	if (code >= 0)
	    code = gs_function_AdOt_init(ppfn, &params, mem);
	if (code < 0)
	    gs_function_AdOt_free_params(&params, mem);
    } else {
	code = fn_build_function(i_ctx_p, pFunction, ppfn, mem);
	if (code < 0)
	    return code;
	if ((*ppfn)->params.m != num_inputs) {
	    gs_function_free(*ppfn, true, mem);
	    return_error(e_rangecheck);
	}
    }
    return code;
}

/* According to PLRM 3rd ed, p. 264  "indexed color space is not
 * allowed in any shading whose color values are generated by a function;
 * this applies to any shading dictionary that contains a Function entry."
 * Adobe interpreters follow PLRM in this respect and we follow them.
 */
private int
check_indexed_vs_function(const gs_color_space *pcs, const gs_function_t *foo)
{ if (foo && gs_color_space_get_index(pcs) == gs_color_space_index_Indexed)
    return_error(e_rangecheck);
  return 0;
}

/* ------ Build shadings ------ */

/* Build a ShadingType 1 (Function-based) shading. */
private int
build_shading_1(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_Fb_params_t params;
    int code;
    static const float default_Domain[4] = {0, 1, 0, 1};
    ref *pmatrix;

    *(gs_shading_params_t *)&params = *pcommon;
    gs_make_identity(&params.Matrix);
    params.Function = 0;
    if ((code = dict_floats_param(imemory, op, "Domain", 
				  4, params.Domain,
				  default_Domain)) < 0 ||
	(dict_find_string(op, "Matrix", &pmatrix) > 0 &&
	 (code = read_matrix(imemory, pmatrix, &params.Matrix)) < 0) ||
	(code = build_shading_function(i_ctx_p, op, &params.Function, 2, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = gs_shading_Fb_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
	return code;
    }
    return 0;
}
/* <dict> .buildshading1 <shading_struct> */
private int
zbuildshading1(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_1);
}

/* Collect parameters for an Axial or Radial shading. */
private int
build_directional_shading(i_ctx_t *i_ctx_p, const ref * op, float *Coords, int num_Coords,
			  float Domain[2], gs_function_t ** pFunction,
			  bool Extend[2], gs_memory_t *mem)
{
    int code = dict_floats_param(imemory, op, "Coords", 
				 num_Coords, Coords, NULL);
    static const float default_Domain[2] = {0, 1};
    ref *pExtend;

    *pFunction = 0;
    if (code < 0 ||
	(code = dict_floats_param(imemory, op, "Domain", 2, Domain,
				  default_Domain)) < 0 ||
	(code = build_shading_function(i_ctx_p, op, pFunction, 1, mem)) < 0
	)
	return code;
    if (!*pFunction)
	    return_error(e_undefined);
    if (dict_find_string(op, "Extend", &pExtend) <= 0)
	Extend[0] = Extend[1] = false;
    else {
	ref E0, E1;

	if (!r_is_array(pExtend))
	    return_error(e_typecheck);
	else if (r_size(pExtend) != 2)
	    return_error(e_rangecheck);
	else if ((array_get(imemory, pExtend, 0L, &E0), 
		  !r_has_type(&E0, t_boolean)) ||
		 (array_get(imemory, pExtend, 1L, &E1), 
		  !r_has_type(&E1, t_boolean))
	    )
	    return_error(e_typecheck);
	Extend[0] = E0.value.boolval, Extend[1] = E1.value.boolval;
    }
    return 0;
}

/* Build a ShadingType 2 (Axial) shading. */
private int
build_shading_2(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_A_params_t params;
    int code;

    *(gs_shading_params_t *)&params = *pcommon;
    if ((code = build_directional_shading(i_ctx_p, op, params.Coords, 4,
					  params.Domain, &params.Function,
					  params.Extend, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = gs_shading_A_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
    }
    return code;
}
/* <dict> .buildshading2 <shading_struct> */
private int
zbuildshading2(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_2);
}

/* Build a ShadingType 3 (Radial) shading. */
private int
build_shading_3(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_R_params_t params;
    int code;

    *(gs_shading_params_t *)&params = *pcommon;
    if ((code = build_directional_shading(i_ctx_p, op, params.Coords, 6,
					  params.Domain, &params.Function,
					  params.Extend, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = gs_shading_R_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
    }
    return code;
}
/* <dict> .buildshading3 <shading_struct> */
private int
zbuildshading3(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_3);
}

/* Collect parameters for a mesh shading. */
private int
build_mesh_shading(i_ctx_t *i_ctx_p, const ref * op,
		   gs_shading_mesh_params_t * params,
		   float **pDecode, gs_function_t ** pFunction,
		   gs_memory_t *mem)
{
    int code;
    float *data = 0;
    ref *pDataSource;

    *pDecode = 0;
    *pFunction = 0;
    if (dict_find_string(op, "DataSource", &pDataSource) <= 0)
	return_error(e_rangecheck);
    if (r_is_array(pDataSource)) {
	uint size = r_size(pDataSource);

	data = (float *)gs_alloc_byte_array(mem, size, sizeof(float),
					    "build_mesh_shading");
	if (data == 0)
	    return_error(e_VMerror);
	code = process_float_array(mem, pDataSource, size, data);
	if (code < 0) {
	    gs_free_object(mem, data, "build_mesh_shading");
	    return code;
	}
	data_source_init_floats(&params->DataSource, data, size);
    } else
	switch (r_type(pDataSource)) {
	    case t_file: {
		stream *s;

		check_read_file(s, pDataSource);
		data_source_init_stream(&params->DataSource, s);
		break;
	    }
	    case t_string:
		check_read(*pDataSource);
		data_source_init_string2(&params->DataSource,
					 pDataSource->value.bytes,
					 r_size(pDataSource));
		break;
	    default:
		return_error(e_typecheck);
	}
    code = build_shading_function(i_ctx_p, op, pFunction, 1, mem);
    if (code < 0) {
	gs_free_object(mem, data, "build_mesh_shading");
	return code;
    }
    if (data_source_is_array(params->DataSource)) {
	params->BitsPerCoordinate = 0;
	params->BitsPerComponent = 0;
    } else {
	int num_decode = 4 +
	    (*pFunction != 0 ? 1 :
	     gs_color_space_num_components(params->ColorSpace)) * 2;

	if ((code = dict_int_param(op, "BitsPerCoordinate", 1, 32, 0,
				   &params->BitsPerCoordinate)) >= 0 &&
	    (code = dict_int_param(op, "BitsPerComponent", 1, 16, 0,
				   &params->BitsPerComponent)) >= 0
	    ) {
	    *pDecode = (float *)
		gs_alloc_byte_array(mem, num_decode, sizeof(float),
				    "build_mesh_shading");
	    if (*pDecode == 0)
		code = gs_note_error(e_VMerror);
	    else {
	        code = dict_floats_param(mem, op, "Decode", num_decode, *pDecode, NULL);
		if (code < 0) {
		    gs_free_object(mem, *pDecode, "build_mesh_shading");
		    *pDecode = 0;
		}
	    }
	}
    }
    if (code < 0) {
	if (*pFunction != 0) {
	    gs_function_free(*pFunction, true, mem);
	    *pFunction = 0;
	}
	gs_free_object(mem, data, "build_mesh_shading");
    }
    return code;
}

/* Collect the BitsPerFlag parameter, if relevant. */
private int
flag_bits_param(const ref * op, const gs_shading_mesh_params_t * params,
		int *pBitsPerFlag)
{
    if (data_source_is_array(params->DataSource)) {
	*pBitsPerFlag = 0;
	return 0;
    } else {
	return dict_int_param(op, "BitsPerFlag", 2, 8, 0, pBitsPerFlag);
    }
}

/* Build a ShadingType 4 (Free-form Gouraud triangle mesh) shading. */
private int
build_shading_4(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_FfGt_params_t params;
    int code;

    *(gs_shading_params_t *)&params = *pcommon;
    if ((code =
	 build_mesh_shading(i_ctx_p, op, (gs_shading_mesh_params_t *)&params,
			    &params.Decode, &params.Function, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = flag_bits_param(op, (gs_shading_mesh_params_t *)&params,
				&params.BitsPerFlag)) < 0 ||
	(code = gs_shading_FfGt_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
	gs_free_object(mem, params.Decode, "Decode");
    }
    return code;
}
/* <dict> .buildshading4 <shading_struct> */
private int
zbuildshading4(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_4);
}

/* Build a ShadingType 5 (Lattice-form Gouraud triangle mesh) shading. */
private int
build_shading_5(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_LfGt_params_t params;
    int code;

    *(gs_shading_params_t *)&params = *pcommon;
    if ((code =
	 build_mesh_shading(i_ctx_p, op, (gs_shading_mesh_params_t *)&params,
			    &params.Decode, &params.Function, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = dict_int_param(op, "VerticesPerRow", 2, max_int, 0,
			       &params.VerticesPerRow)) < 0 ||
	(code = gs_shading_LfGt_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
	gs_free_object(mem, params.Decode, "Decode");
    }
    return code;
}
/* <dict> .buildshading5 <shading_struct> */
private int
zbuildshading5(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_5);
}

/* Build a ShadingType 6 (Coons patch mesh) shading. */
private int
build_shading_6(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_Cp_params_t params;
    int code;

    *(gs_shading_params_t *)&params = *pcommon;
    if ((code =
	 build_mesh_shading(i_ctx_p, op, (gs_shading_mesh_params_t *)&params,
			    &params.Decode, &params.Function, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = flag_bits_param(op, (gs_shading_mesh_params_t *)&params,
				&params.BitsPerFlag)) < 0 ||
	(code = gs_shading_Cp_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
	gs_free_object(mem, params.Decode, "Decode");
    }
    return code;
}
/* <dict> .buildshading6 <shading_struct> */
private int
zbuildshading6(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_6);
}

/* Build a ShadingType 7 (Tensor product patch mesh) shading. */
private int
build_shading_7(i_ctx_t *i_ctx_p, const ref * op, const gs_shading_params_t * pcommon,
		gs_shading_t ** ppsh, gs_memory_t *mem)
{
    gs_shading_Tpp_params_t params;
    int code;

    *(gs_shading_params_t *)&params = *pcommon;
    if ((code =
	 build_mesh_shading(i_ctx_p, op, (gs_shading_mesh_params_t *)&params,
			    &params.Decode, &params.Function, mem)) < 0 ||
	(code = check_indexed_vs_function(params.ColorSpace, params.Function)) < 0 ||
	(code = flag_bits_param(op, (gs_shading_mesh_params_t *)&params,
				&params.BitsPerFlag)) < 0 ||
	(code = gs_shading_Tpp_init(ppsh, &params, mem)) < 0
	) {
	gs_free_object(mem, params.Function, "Function");
	gs_free_object(mem, params.Decode, "Decode");
    }
    return code;
}
/* <dict> .buildshading7 <shading_struct> */
private int
zbuildshading7(i_ctx_t *i_ctx_p)
{
    return build_shading(i_ctx_p, build_shading_7);
}

/* ------ Initialization procedure ------ */

const op_def zshade_op_defs[] =
{
    op_def_begin_ll3(),
    {"0currentsmoothness", zcurrentsmoothness},
    {"1setsmoothness", zsetsmoothness},
    {"1.shfill", zshfill},
    {"1.buildshading1", zbuildshading1},
    {"1.buildshading2", zbuildshading2},
    {"1.buildshading3", zbuildshading3},
    {"1.buildshading4", zbuildshading4},
    {"1.buildshading5", zbuildshading5},
    {"1.buildshading6", zbuildshading6},
    {"1.buildshading7", zbuildshading7},
    {"3.buildshadingpattern", zbuildshadingpattern},
    op_def_end(0)
};
