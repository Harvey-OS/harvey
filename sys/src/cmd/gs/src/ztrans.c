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

/*$Id: ztrans.c,v 1.12 2000/09/19 19:00:55 lpd Exp $ */
/* Transparency operators */
#include "string_.h"
#include "ghost.h"
#include "oper.h"
#include "gscspace.h"		/* for gscolor2.h */
#include "gscolor2.h"
#include "gsipar3x.h"
#include "gstrans.h"
#include "gxiparam.h"		/* for image enumerator */
#include "idict.h"
#include "idparam.h"
#include "ifunc.h"
#include "igstate.h"
#include "iimage.h"
#include "iimage2.h"
#include "iname.h"
#include "store.h"

/* ------ Utilities ------ */

private int
set_float_value(i_ctx_t *i_ctx_p, int (*set_value)(P2(gs_state *, floatp)))
{
    os_ptr op = osp;
    double value;
    int code;

    if (real_param(op, &value) < 0)
	return_op_typecheck(op);
    if ((code = set_value(igs, value)) < 0)
	return code;
    pop(1);
    return 0;
}

private int
current_float_value(i_ctx_t *i_ctx_p,
		    float (*current_value)(P1(const gs_state *)))
{
    os_ptr op = osp;

    push(1);
    make_real(op, current_value(igs));
    return 0;
}

private int
enum_param(const ref *pnref, const char *const names[])
{
    const char *const *p;
    ref nsref;

    name_string_ref(pnref, &nsref);
    for (p = names; *p; ++p)
	if (r_size(&nsref) == strlen(*p) &&
	    !memcmp(*p, nsref.value.const_bytes, r_size(&nsref))
	    )
	    return p - names;
    return_error(e_rangecheck);
}

/* ------ Graphics state operators ------ */

private const char *const blend_mode_names[] = {
    GS_BLEND_MODE_NAMES, 0
};

/* <modename> .setblendmode - */
private int
zsetblendmode(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_type(*op, t_name);
    if ((code = enum_param(op, blend_mode_names)) < 0 ||
	(code = gs_setblendmode(igs, code)) < 0
	)
	return code;
    pop(1);
    return 0;
}

/* - .currentblendmode <modename> */
private int
zcurrentblendmode(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const char *mode_name = blend_mode_names[gs_currentblendmode(igs)];
    ref nref;
    int code = name_enter_string(mode_name, &nref);

    if (code < 0)
	return code;
    push(1);
    *op = nref;
    return 0;
}

/* <0..1> .setopacityalpha - */
private int
zsetopacityalpha(i_ctx_t *i_ctx_p)
{
    return set_float_value(i_ctx_p, gs_setopacityalpha);
}

/* - .currentopacityalpha <0..1> */
private int
zcurrentopacityalpha(i_ctx_t *i_ctx_p)
{
    return current_float_value(i_ctx_p, gs_currentopacityalpha);
}

/* <0..1> .setshapealpha - */
private int
zsetshapealpha(i_ctx_t *i_ctx_p)
{
    return set_float_value(i_ctx_p, gs_setshapealpha);
}

/* - .currentshapealpha <0..1> */
private int
zcurrentshapealpha(i_ctx_t *i_ctx_p)
{
    return current_float_value(i_ctx_p, gs_currentshapealpha);
}

/* <bool> .settextknockout - */
private int
zsettextknockout(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_boolean);
    gs_settextknockout(igs, op->value.boolval);
    pop(1);
    return 0;
}

/* - .currenttextknockout <bool> */
private int
zcurrenttextknockout(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_bool(op, gs_currenttextknockout(igs));
    return 0;
}

/* ------ Rendering stack operators ------ */

private int
rect_param(gs_rect *prect, os_ptr op)
{
    double coords[4];
    int code = num_params(op, 4, coords);

    if (code < 0)
	return code;
    prect->p.x = coords[0], prect->p.y = coords[1];
    prect->q.x = coords[2], prect->q.y = coords[3];
    return 0;
}

private int
mask_op(i_ctx_t *i_ctx_p,
	int (*mask_proc)(P2(gs_state *, gs_transparency_channel_selector_t)))
{
    int csel;
    int code = int_param(osp, 1, &csel);

    if (code < 0)
	return code;
    code = mask_proc(igs, csel);
    if (code >= 0)
	pop(1);
    return code;

}

/* <paramdict> <llx> <lly> <urx> <ury> .begintransparencygroup - */
private int
zbegintransparencygroup(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr dop = op - 4;
    gs_transparency_group_params_t params;
    gs_rect bbox;
    int code;

    check_type(*dop, t_dictionary);
    check_dict_read(*dop);
    gs_trans_group_params_init(&params);
    if ((code = dict_bool_param(dop, "Isolated", false, &params.Isolated)) < 0 ||
	(code = dict_bool_param(dop, "Knockout", false, &params.Knockout)) < 0
	)
	return code;
    code = rect_param(&bbox, op);
    if (code < 0)
	return code;
    params.ColorSpace = gs_currentcolorspace(igs);
    code = gs_begin_transparency_group(igs, &params, &bbox);
    if (code < 0)
	return code;
    pop(5);
    return code;
}

/* - .discardtransparencygroup - */
private int
zdiscardtransparencygroup(i_ctx_t *i_ctx_p)
{
    if (gs_current_transparency_type(igs) != TRANSPARENCY_STATE_Group)
	return_error(e_rangecheck);
    return gs_discard_transparency_layer(igs);
}

/* - .endtransparencygroup - */
private int
zendtransparencygroup(i_ctx_t *i_ctx_p)
{
    return gs_end_transparency_group(igs);
}

/* <paramdict> <llx> <lly> <urx> <ury> .begintransparencymask - */
private int tf_using_function(P3(floatp, float *, void *));
private int
zbegintransparencymask(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr dop = op - 4;
    gs_transparency_mask_params_t params;
    ref *pparam;
    gs_rect bbox;
    int num_components =
	gs_color_space_num_components(gs_currentcolorspace(igs));
    int code;
    static const char *const subtype_names[] = {
	GS_TRANSPARENCY_MASK_SUBTYPE_NAMES, 0
    };

    check_type(*dop, t_dictionary);
    check_dict_read(*dop);
    if (dict_find_string(dop, "Subtype", &pparam) <= 0)
	return_error(e_rangecheck);
    if ((code = enum_param(pparam, subtype_names)) < 0)
	return code;
    gs_trans_mask_params_init(&params, code);
    if ((code = dict_floats_param(dop, "Background", num_components,
				  params.Background, NULL)) < 0
	)
	return code;
    else if (code > 0)
	params.has_Background = true;
    if (dict_find_string(dop, "TransferFunction", &pparam) >0) {
	gs_function_t *pfn = ref_function(pparam);

	if (pfn == 0 || pfn->params.m != 1 || pfn->params.n != 1)
	    return_error(e_rangecheck);
	params.TransferFunction = tf_using_function;
	params.TransferFunction_data = pfn;
    }
    code = rect_param(&bbox, op);
    if (code < 0)
	return code;
    code = gs_begin_transparency_mask(igs, &params, &bbox);
    if (code < 0)
	return code;
    pop(5);
    return code;
}
/* Implement the TransferFunction using a Function. */
private int
tf_using_function(floatp in_val, float *out, void *proc_data)
{
    float in = in_val;
    gs_function_t *const pfn = proc_data;

    return gs_function_evaluate(pfn, &in, out);
}

/* - .discardtransparencymask - */
private int
zdiscardtransparencymask(i_ctx_t *i_ctx_p)
{
    if (gs_current_transparency_type(igs) != TRANSPARENCY_STATE_Mask)
	return_error(e_rangecheck);
    return gs_discard_transparency_layer(igs);
}

/* <mask#> .endtransparencymask - */
private int
zendtransparencymask(i_ctx_t *i_ctx_p)
{
    return mask_op(i_ctx_p, gs_end_transparency_mask);
}

/* <mask#> .inittransparencymask - */
private int
zinittransparencymask(i_ctx_t *i_ctx_p)
{
    return mask_op(i_ctx_p, gs_init_transparency_mask);
}

/* ------ Soft-mask images ------ */

/* <dict> .image3x - */
private int mask_dict_param(P5(os_ptr, image_params *, const char *, int,
			       gs_image3x_mask_t *));
private int
zimage3x(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image3x_t image;
    ref *pDataDict;
    image_params ip_data;
    int num_components =
	gs_color_space_num_components(gs_currentcolorspace(igs));
    int ignored;
    int code;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    gs_image3x_t_init(&image, NULL);
    if (dict_find_string(op, "DataDict", &pDataDict) <= 0)
	return_error(e_rangecheck);
    if ((code = pixel_image_params(i_ctx_p, pDataDict,
				   (gs_pixel_image_t *)&image, &ip_data,
				   12)) < 0 ||
	(code = dict_int_param(pDataDict, "ImageType", 1, 1, 0, &ignored)) < 0
	)
	return code;
    /*
     * We have to process the masks in the reverse order, because they
     * insert their DataSource before the one(s) for the DataDict.
     */
    if ((code = mask_dict_param(op, &ip_data, "ShapeMaskDict", num_components,
				&image.Shape)) < 0 ||
	(code = mask_dict_param(op, &ip_data, "OpacityMaskDict", num_components,
				&image.Opacity)) < 0
	)
	return code;
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *)&image,
			&ip_data.DataSource[0],
			image.CombineWithColor, 1);
}    

/* Get one soft-mask dictionary parameter. */
private int
mask_dict_param(os_ptr op, image_params *pip_data, const char *dict_name,
		int num_components, gs_image3x_mask_t *pixm)
{
    ref *pMaskDict;
    image_params ip_mask;
    int ignored;
    int code, mcode;

    if (dict_find_string(op, dict_name, &pMaskDict) <= 0)
	return 1;
    if ((mcode = code = data_image_params(pMaskDict, &pixm->MaskDict, &ip_mask, false, 1, 12)) < 0 ||
	(code = dict_int_param(pMaskDict, "ImageType", 1, 1, 0, &ignored)) < 0 ||
	(code = dict_int_param(pMaskDict, "InterleaveType", 1, 3, -1,
			       &pixm->InterleaveType)) < 0 ||
	(code = dict_floats_param(op, "Matte", num_components, pixm->Matte, NULL)) < 0
	)
	return code;
    pixm->has_Matte = code > 0;
    /*
     * The MaskDict must have a DataSource iff InterleaveType == 3.
     */
    if ((pip_data->MultipleDataSources && pixm->InterleaveType != 3) ||
	ip_mask.MultipleDataSources ||
	mcode != (pixm->InterleaveType != 3)
	)
	return_error(e_rangecheck);
    if (pixm->InterleaveType == 3) {
	/* Insert the mask DataSource before the data DataSources. */
	memmove(&pip_data->DataSource[1], &pip_data->DataSource[0],
		(countof(pip_data->DataSource) - 1) *
		sizeof(pip_data->DataSource[0]));
	pip_data->DataSource[0] = ip_mask.DataSource[0];
    }
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def ztrans_op_defs[] = {
    {"1.setblendmode", zsetblendmode},
    {"0.currentblendmode", zcurrentblendmode},
    {"1.setopacityalpha", zsetopacityalpha},
    {"0.currentopacityalpha", zcurrentopacityalpha},
    {"1.setshapealpha", zsetshapealpha},
    {"0.currentshapealpha", zcurrentshapealpha},
    {"1.settextknockout", zsettextknockout},
    {"0.currenttextknockout", zcurrenttextknockout},
    {"5.begintransparencygroup", zbegintransparencygroup},
    {"0.discardtransparencygroup", zdiscardtransparencygroup},
    {"0.endtransparencygroup", zendtransparencygroup},
    {"5.begintransparencymask", zbegintransparencymask},
    {"0.discardtransparencymask", zdiscardtransparencymask},
    {"1.endtransparencymask", zendtransparencymask},
    {"1.inittransparencymask", zinittransparencymask},
    {"1.image3x", zimage3x},
    op_def_end(0)
};
