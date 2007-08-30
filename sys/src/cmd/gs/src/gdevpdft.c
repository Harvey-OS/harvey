/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdft.c,v 1.52 2005/09/06 22:21:14 leonardo Exp $ */
/* transparency processing for PDF-writing driver */
#include "gx.h"
#include "string_.h"
#include "gserrors.h"
#include "gstrans.h"
#include "gscolor2.h"
#include "gzstate.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"

private int
pdf_make_soft_mask_dict(gx_device_pdf * pdev, const gs_pdf14trans_params_t * pparams)
{
    pdf_resource_t *pres_soft_mask_dict = 0;
    cos_dict_t *soft_mask_dict;
    int code;

    /* Fixme : merge redundant objects. */
    code = pdf_alloc_resource(pdev, resourceSoftMaskDict, gs_no_id, &pres_soft_mask_dict, -1);
    if (code < 0)
	return code;
    cos_become(pres_soft_mask_dict->object, cos_type_dict);
    pdev->pres_soft_mask_dict = pres_soft_mask_dict;
    soft_mask_dict = (cos_dict_t *)pres_soft_mask_dict->object;
    code = cos_dict_put_c_key_string(soft_mask_dict, "/S", 
	    pparams->subtype == TRANSPARENCY_MASK_Alpha ? (byte *)"/Alpha" : (byte *)"/Luminosity", 
	    pparams->subtype == TRANSPARENCY_MASK_Alpha ? 6 : 11);
    if (code < 0)
	return code;
    if (pparams->Background_components) {
	cos_array_t *Background;

	Background = cos_array_from_floats(pdev, pparams->Background, 
		    pparams->Background_components, "pdf_write_soft_mask_dict");
	if (Background == NULL)
	    return_error(gs_error_VMerror);
	code = cos_dict_put_c_key_object(soft_mask_dict, "/BC", (cos_object_t *)Background);
	if (code < 0)
	    return code;
    }
    if (pparams->transfer_function != NULL) {
	long id;
	char buf[20];
	
	code = pdf_write_function(pdev, pparams->transfer_function, &id);
	if (code < 0)
	    return code;
	sprintf(buf, " %ld 0 R", id);
	code = cos_dict_put_c_key_string(soft_mask_dict, "/TR", (const byte *)buf, strlen(buf));
	if (code < 0)
	    return code;
    }
    return 0;

}

private int
pdf_make_group_dict(gx_device_pdf * pdev, const gs_pdf14trans_params_t * pparams,
			    const gs_imager_state * pis, cos_dict_t **pdict)
{
    pdf_resource_t *pres_group;
    cos_dict_t *group_dict;
    int code;
    const gs_state *gstate = gx_hld_get_gstate_ptr(pis);
    cos_value_t cs_value;

    code = pdf_alloc_resource(pdev, resourceGroup, gs_no_id, &pres_group, -1);
    if (code < 0)
	return code;
    cos_become(pres_group->object, cos_type_dict);
    group_dict = (cos_dict_t *)pres_group->object;
    code = cos_dict_put_c_key_string(group_dict, "/Type", (const byte *)"/Group", 6);
    if (code < 0)
	return code;
    code = cos_dict_put_c_key_string(group_dict, "/S", (const byte *)"/Transparency", 13);
    if (code < 0)
	return code;
    if (pparams->Isolated) {
	code = cos_dict_put_c_key_bool(group_dict, "/I", true);
	if (code < 0)
	    return code;
    }
    if (pparams->Knockout) {
	code = cos_dict_put_c_key_bool(group_dict, "/K", true);
	if (code < 0)
	    return code;
    }
    if (gstate != NULL) {
	const gs_color_space *cs = gstate->color_space;

	code = pdf_color_space(pdev, &cs_value, NULL, cs,
		&pdf_color_space_names, false);
	if (code < 0)
	    return code;
	code = cos_dict_put_c_key(group_dict, "/CS", &cs_value);
	if (code < 0)
	    return code;
    }
    group_dict = NULL; /* The next line invalidates it. */
    code = pdf_substitute_resource(pdev, &pres_group, resourceGroup, NULL, false);
    if (code < 0)
	return code;
    *pdict = (cos_dict_t *)pres_group->object;
    return 0;
}

private int
pdf_make_form_dict(gx_device_pdf * pdev, const gs_pdf14trans_params_t * pparams,
			    const gs_imager_state * pis, 
			    const cos_dict_t *group_dict, cos_dict_t *form_dict)
{
    cos_array_t *bbox_array;
    float bbox[4];
    gs_rect bbox_rect;
    int code;


    code = gs_bbox_transform(&pparams->bbox, &ctm_only(pis), &bbox_rect);
    if (code < 0)
	return code;
    bbox[0] = bbox_rect.p.x;
    bbox[1] = bbox_rect.p.y;
    bbox[2] = bbox_rect.q.x;
    bbox[3] = bbox_rect.q.y;
    code = cos_dict_put_c_key_string(form_dict, "/Type", (const byte *)"/XObject", 8);
    if (code < 0)
	return code;
    code = cos_dict_put_c_key_string(form_dict, "/Subtype", (const byte *)"/Form", 5);
    if (code < 0)
	return code;
    code = cos_dict_put_c_key_int(form_dict, "/FormType", 1);
    if (code < 0)
	return code;
    code = cos_dict_put_c_key_string(form_dict, "/Matrix", (const byte *)"[1 0 0 1 0 0]", 13);
    if (code < 0)
	return code;
    bbox_array = cos_array_from_floats(pdev, bbox, 4, "pdf_begin_transparency_group");
    if (bbox_array == NULL)
	return_error(gs_error_VMerror);
    code = cos_dict_put_c_key_object(form_dict, "/BBox", (cos_object_t *)bbox_array);
    if (code < 0)
	return code;
    return cos_dict_put_c_key_object(form_dict, "/Group", (cos_object_t *)group_dict);
}

private int 
pdf_begin_transparency_group(gs_imager_state * pis, gx_device_pdf * pdev,
				const gs_pdf14trans_params_t * pparams)
{
    cos_dict_t *group_dict;
    bool in_page = is_in_page(pdev);
    const gs_state *gstate = gx_hld_get_gstate_ptr(pis);
    int code;

    if (gstate == NULL)
	return_error(gs_error_unregistered); /* Must not happen. */
    code = pdf_make_group_dict(pdev, pparams, pis, &group_dict);
    if (code < 0)
	return code;
    code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    if (pdf_must_put_clip_path(pdev, gstate->clip_path)) {
	code = pdf_put_clip_path(pdev, gstate->clip_path);
	if (code < 0)
	    return code;
    }
    if (!in_page) 
	pdev->pages[pdev->next_page].group_id = group_dict->id;
    else {
	pdf_resource_t *pres, *pres_gstate = NULL;

	code = pdf_prepare_drawing(pdev, pis, &pres_gstate);
	if (code < 0)
	    return code;
	code = pdf_end_gstate(pdev, pres_gstate);
	if (code < 0)
	    return code;
	code = pdf_enter_substream(pdev, resourceXObject, 
		gs_no_id, &pres, false, pdev->params.CompressPages);
	if (code < 0)
	    return code;
	return pdf_make_form_dict(pdev, pparams, pis, group_dict, (cos_dict_t *)pres->object);
    }
    return 0;
}

private int 
pdf_end_transparency_group(gs_imager_state * pis, gx_device_pdf * pdev)
{
    int bottom = (pdev->ResourcesBeforeUsage ? 1 : 0);

    if (pdev->sbstack_depth == bottom) {
	/* We're closing the page group. */
	if (pdev->pages[pdev->next_page].group_id == 0)
	    return_error(gs_error_unregistered); /* Must not happen. */
	return 0;
    } else {
	pdf_resource_t *pres = pdev->accumulating_substream_resource;
	int code;
	uint ignore;

	code = pdf_exit_substream(pdev);
	if (code < 0)
	    return code;
	code = pdf_substitute_resource(pdev, &pres, resourceXObject, NULL, false);
	if (code < 0)
	    return code;
	sputc(pdev->strm,'/');
	sputs(pdev->strm, (const byte *)pres->rname, strlen(pres->rname), &ignore);
	sputs(pdev->strm, (const byte *)" Do\n", 4, &ignore);
	return 0;    
    }
}

private int 
pdf_begin_transparency_mask(gs_imager_state * pis, gx_device_pdf * pdev,
				const gs_pdf14trans_params_t * pparams)
{
    if (pparams->mask_is_image) {
	/* HACK : 
	    The control comes here when 
	    the PDF interpreter will make the PS interpreter 
	    to interprete the mask for filling the transparency buffer
    	    with an SMask image.
	    Since we handle Type 3 images as a high level objects, 
	    we don't install the transparency buffer here
	    and need to skip the image enumeration for the SMask.
	    However we have no right method for skipping
	    an image enumeration due to possible side effect 
	    of the image data proc in Postscript language.
	    Therefore we do enumerate the image mask and accumulate
	    it as a PDF stream, but don't create a reference to it.
	    Later it will be enumerated once again as a part of SMask-ed image,
	    and the pdfwrite image handler will recognize duplicated images 
	    and won't create the second stream for same image.

	    We could make a special workaround for 
	    skipping mask images either in the graphics library or 
	    in the PS code of the PDF interpreter,
	    but we don't want to complicate things now.
	    The performance leak for the second enumeration 
	    shouldn't be harmful.

	    So now just set a flag for pdf_end_and_do_image.
	*/
	pdev->image_mask_skip = true;
	return 0;
    } else {
	int code;
	
	code = pdf_make_soft_mask_dict(pdev, pparams);
	if (code < 0)
	    return code;
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
	return pdf_begin_transparency_group(pis, pdev, pparams);
    }
}

private int 
pdf_end_transparency_mask(gs_imager_state * pis, gx_device_pdf * pdev,
				const gs_pdf14trans_params_t * pparams)
{
    if (pdev->image_mask_skip) 
	pdev->image_mask_skip = false;
    else {
	pdf_resource_t *pres = pdev->accumulating_substream_resource;
	int code;
	char buf[20];
	
	code = pdf_exit_substream(pdev);
	if (code < 0)
	    return code;
	code = pdf_substitute_resource(pdev, &pres, resourceXObject, NULL, false);
	if (code < 0)
	    return 0;
	sprintf(buf, "%ld 0 R", pdf_resource_id(pres));
	code = cos_dict_put_c_key_string((cos_dict_t *)pdev->pres_soft_mask_dict->object,
		"/G", (const byte *)buf, strlen(buf));
	if (code < 0)
	    return code;
	code = pdf_substitute_resource(pdev, &pdev->pres_soft_mask_dict, 
					resourceSoftMaskDict, NULL, false);
	if (code < 0)
	    return code;
	pis->soft_mask_id = pdev->pres_soft_mask_dict->object->id;
	pdev->pres_soft_mask_dict = NULL;
    }
    return 0;
}

private int
pdf_set_blend_params(gs_imager_state * pis, gx_device_pdf * dev,
				const gs_pdf14trans_params_t * pparams)
{
    return 0;
}

int 
gdev_pdf_create_compositor(gx_device *dev,
    gx_device **pcdev, const gs_composite_t *pct,
    gs_imager_state *pis, gs_memory_t *memory)
{
    gx_device_pdf *pdev = (gx_device_pdf *)dev;
    
    if (pdev->HaveTransparency && pdev->CompatibilityLevel >= 1.4 &&
	    pct->type->comp_id == GX_COMPOSITOR_PDF14_TRANS) {
	gs_pdf14trans_t *pcte = (gs_pdf14trans_t *)pct;
	gs_pdf14trans_params_t *params = &pcte->params;

	*pcdev = dev;
	switch(params->pdf14_op) {
	    case PDF14_PUSH_DEVICE:
		return 0;
	    case PDF14_POP_DEVICE:
		return 0;
	    case PDF14_BEGIN_TRANS_GROUP:
		return pdf_begin_transparency_group(pis, pdev, params);
	    case PDF14_END_TRANS_GROUP:
		return pdf_end_transparency_group(pis, pdev);
	    case PDF14_INIT_TRANS_MASK:
		return gx_init_transparency_mask(pis, params);
	    case PDF14_BEGIN_TRANS_MASK:
		return pdf_begin_transparency_mask(pis, pdev, params);
	    case PDF14_END_TRANS_MASK:
		return pdf_end_transparency_mask(pis, pdev, params);
	    case PDF14_SET_BLEND_PARAMS:
		return pdf_set_blend_params(pis, pdev, params);
	    default :
		return_error(gs_error_unregistered); /* Must not happen. */
	}
    }
    return psdf_create_compositor(dev, pcdev, pct, pis, memory);
}

/* We're not sure why the folllowing device methods are never called.
   Stub them for a while. */

int 
gdev_pdf_begin_transparency_group(gx_device *dev,
    const gs_transparency_group_params_t *ptgp,
    const gs_rect *pbbox,
    gs_imager_state *pis,
    gs_transparency_state_t **ppts,
    gs_memory_t *mem)
{
    return 0;
}

int
gdev_pdf_end_transparency_group(gx_device *dev,
    gs_imager_state *pis,
    gs_transparency_state_t **ppts)
{
    return 0;
}

int
gdev_pdf_begin_transparency_mask(gx_device *dev,
    const gx_transparency_mask_params_t *ptmp,
    const gs_rect *pbbox,
    gs_imager_state *pis,
    gs_transparency_state_t **ppts,
    gs_memory_t *mem)
{
    return 0;
}

int
gdev_pdf_end_transparency_mask(gx_device *dev,
    gs_transparency_mask_t **pptm)
{
    return 0;
}

int
gdev_pdf_discard_transparency_layer(gx_device *dev,
    gs_transparency_state_t **ppts)
{
    return 0;
}
