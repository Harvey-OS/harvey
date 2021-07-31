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

/*$Id: gdevpdfv.c,v 1.3.2.2 2000/11/09 23:24:18 rayjj Exp $ */
/* Color value writing for pdfwrite driver */
#include "math_.h"
#include "string_.h"
#include "gx.h"
#include "gscindex.h"
#include "gserrors.h"
#include "gsiparm3.h"		/* for pattern colors */
#include "gsmatrix.h"		/* for gspcolor.h */
#include "gscoord.h"		/* for gs_currentmatrix, requires gsmatrix.h */
#include "gsptype2.h"
#include "gxcolor2.h"		/* for gxpcolor.h */
#include "gxdcolor.h"		/* for gxpcolor.h */
#include "gxpcolor.h"		/* for pattern device color types */
#include "gxshade.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"
#include "szlibx.h"

/* Import the PatternType 2 Pattern device color type. */
extern const gx_device_color_type_t gx_dc_pattern2;

/* ---------------- Utilities ---------------- */

/* Write a matrix parameter. */
private int
cos_dict_put_matrix(cos_dict_t *pscd, const char *key, const gs_matrix *pmat)
{
    float matrix[6];

    matrix[0] = pmat->xx;
    matrix[1] = pmat->xy;
    matrix[2] = pmat->yx;
    matrix[3] = pmat->yy;
    matrix[4] = pmat->tx;
    matrix[5] = pmat->ty;
    return cos_dict_put_c_key_floats(pscd, key, matrix, 6);
}

/* ---------------- PatternType 1 colors ---------------- */

/*
 * Create a Pattern resource referencing an image (currently only an XObject).
 * p_tile is NULL for uncolored patterns or the NULL pattern.
 * m_tile is NULL for colored patterns that fill their bounding box,
 * including the NULL pattern.
 ****** WE DON'T HANDLE NULL PATTERNS YET ******
 */
private uint
tile_size(const gx_strip_bitmap *tile, int depth)
{
    return (tile->rep_width * depth + 7) / 8 * tile->rep_height;
}
private int
pdf_pattern(gx_device_pdf *pdev, const gx_drawing_color *pdc,
	    const gx_color_tile *p_tile, const gx_color_tile *m_tile,
	    cos_stream_t *pcs_image, pdf_resource_t **ppres)
{
    pdf_resource_t *pres;
    int code = pdf_alloc_resource(pdev, resourcePattern, pdc->mask.id, ppres,
				  0L);
    cos_stream_t *pcos;
    cos_dict_t *pcd;
    cos_dict_t *pcd_Resources = cos_dict_alloc(pdev, "pdf_pattern(Resources)");
    const gx_color_tile *tile = (p_tile ? p_tile : m_tile);
    const gx_strip_bitmap *btile = (p_tile ? &p_tile->tbits : &m_tile->tmask);
    uint p_size =
	(p_tile == 0 ? 0 : tile_size(&p_tile->tbits, pdev->color_info.depth));
    uint m_size =
	(m_tile == 0 ? 0 : tile_size(&m_tile->tmask, 1));
    bool mask = p_tile == 0;
    gs_point step;
    gs_matrix smat;

    if (code < 0)
	return code;
    /*
     * Acrobat Reader can't handle image Patterns with more than
     * 64K of data.  :-(
     */
    if (max(p_size, m_size) > 65500)
	return_error(gs_error_limitcheck);
    /*
     * We currently can't handle Patterns whose X/Y step isn't parallel
     * to the coordinate axes.
     */
    if (is_xxyy(&tile->step_matrix))
	step.x = tile->step_matrix.xx, step.y = tile->step_matrix.yy;
    else if (is_xyyx(&tile->step_matrix))
	step.x = tile->step_matrix.yx, step.y = tile->step_matrix.xy;
    else
	return_error(gs_error_rangecheck);
    if (pcd_Resources == 0)
	return_error(gs_error_VMerror);
    gs_make_identity(&smat);
    smat.xx = btile->rep_width / (pdev->HWResolution[0] / 72.0);
    smat.yy = btile->rep_height / (pdev->HWResolution[1] / 72.0);
    pres = *ppres;
    {
	cos_dict_t *pcd_XObject = cos_dict_alloc(pdev, "pdf_pattern(XObject)");
	char key[MAX_REF_CHARS + 3];
	cos_value_t v;

	if (pcd_XObject == 0)
	    return_error(gs_error_VMerror);
	sprintf(key, "/R%ld", pcs_image->id);
	COS_OBJECT_VALUE(&v, pcs_image);
	if ((code = cos_dict_put(pcd_XObject, (byte *)key, strlen(key), &v)) < 0 ||
	    (code = cos_dict_put_c_key_object(pcd_Resources, "/XObject",
					      COS_OBJECT(pcd_XObject))) < 0
	    )
	    return code;
    }
    if ((code = cos_dict_put_c_strings(pcd_Resources, "/ProcSet",
				       (mask ? "[/PDF/ImageB]" :
					"[/PDF/ImageC]"))) < 0)
	return code;
    cos_become(pres->object, cos_type_stream);
    pcos = (cos_stream_t *)pres->object;
    pcd = cos_stream_dict(pcos);
    if ((code = cos_dict_put_c_key_int(pcd, "/PatternType", 1)) < 0 ||
	(code = cos_dict_put_c_key_int(pcd, "/PaintType",
				       (mask ? 2 : 1))) < 0 ||
	(code = cos_dict_put_c_key_int(pcd, "/TilingType",
				       tile->tiling_type)) < 0 ||
	(code = cos_dict_put_c_key_object(pcd, "/Resources",
					  COS_OBJECT(pcd_Resources))) < 0 ||
	(code = cos_dict_put_c_strings(pcd, "/BBox", "[0 0 1 1]")) < 0 ||
	(code = cos_dict_put_matrix(pcd, "/Matrix", &smat)) < 0 ||
	(code = cos_dict_put_c_key_real(pcd, "/XStep", step.x / btile->rep_width)) < 0 ||
	(code = cos_dict_put_c_key_real(pcd, "/YStep", step.y / btile->rep_height)) < 0
	) {
	return code;
    }

    {
	char buf[MAX_REF_CHARS + 6 + 1]; /* +6 for /R# Do\n */

	sprintf(buf, "/R%ld Do\n", pcs_image->id);
	cos_stream_add_bytes(pcos, (const byte *)buf, strlen(buf));
    }

    return 0;
}

/* Set the ImageMatrix, Width, and Height for a Pattern image. */
private void
pdf_set_pattern_image(gs_data_image_t *pic, const gx_strip_bitmap *tile)
{
    pic->ImageMatrix.xx = pic->Width = tile->rep_width;
    pic->ImageMatrix.yy = pic->Height = tile->rep_height;
}

/* Write the mask for a Pattern. */
private int
pdf_put_pattern_mask(gx_device_pdf *pdev, const gx_color_tile *m_tile,
		     cos_stream_t **ppcs_mask)
{
    int w = m_tile->tmask.rep_width, h = m_tile->tmask.rep_height;
    gs_image1_t image;
    pdf_image_writer writer;
    cos_stream_t *pcs_image;
    long pos;
    int code;

    gs_image_t_init_mask_adjust(&image, true, false);
    pdf_set_pattern_image((gs_data_image_t *)&image, &m_tile->tmask);
    if ((code = pdf_begin_write_image(pdev, &writer, gs_no_id, w, h, NULL, false)) < 0 ||
	(pdev->params.MonoImage.Encode &&
	 (code = psdf_CFE_binary(&writer.binary, w, h, true)) < 0) ||
	(code = pdf_begin_image_data(pdev, &writer, (const gs_pixel_image_t *)&image, NULL)) < 0
	)
	return code;
    pcs_image = (cos_stream_t *)writer.pres->object;
    pos = stell(pdev->streams.strm);
    /* Pattern masks are specified in device coordinates, so invert Y. */
    if ((code = pdf_copy_mask_bits(writer.binary.strm, m_tile->tmask.data + (h - 1) * m_tile->tmask.raster, 0, -m_tile->tmask.raster, w, h, 0)) < 0 ||
	(code = cos_stream_add_since(pcs_image, pos)) < 0 ||
	(code = pdf_end_image_binary(pdev, &writer, h)) < 0 ||
	(code = pdf_end_write_image(pdev, &writer)) < 0
	)
	return code;
    *ppcs_mask = pcs_image;
    return 0;
}

/* Write a colored Pattern color. */
/* (Single-use procedure for readability.) */
private int
pdf_put_colored_pattern(gx_device_pdf *pdev, const gx_drawing_color *pdc,
			const psdf_set_color_commands_t *ppscc,
			pdf_resource_t **ppres)
{
    const gx_color_tile *m_tile = pdc->mask.m_tile;
    const gx_color_tile *p_tile = pdc->colors.pattern.p_tile;
    int w = p_tile->tbits.rep_width, h = p_tile->tbits.rep_height;
    gs_color_space cs_Device;
    cos_value_t cs_value;
    pdf_image_writer writer;
    gs_image1_t image;
    cos_stream_t *pcs_image;
    cos_stream_t *pcs_mask = 0;
    cos_value_t v;
    long pos;
    int code = pdf_cs_Pattern_colored(pdev, &v);

    if (code < 0)
	return code;
    pdf_cspace_init_Device(&cs_Device, pdev->color_info.num_components);
    code = pdf_color_space(pdev, &cs_value, &cs_Device,
			   &pdf_color_space_names, true);
    if (code < 0)
	return code;
    gs_image_t_init_adjust(&image, &cs_Device, false);
    image.BitsPerComponent = 8;
    pdf_set_pattern_image((gs_data_image_t *)&image, &p_tile->tbits);
    if (m_tile) {
	if ((code = pdf_put_pattern_mask(pdev, m_tile, &pcs_mask)) < 0)
	    return code;
    }
    if ((code = pdf_begin_write_image(pdev, &writer, gs_no_id, w, h, NULL, false)) < 0 ||
	(code = psdf_setup_lossless_filters((gx_device_psdf *)pdev,
					    &writer.binary,
					    (gs_pixel_image_t *)&image)) < 0 ||
	(code = pdf_begin_image_data(pdev, &writer, (const gs_pixel_image_t *)&image, &cs_value)) < 0
	)
	return code;
    pcs_image = (cos_stream_t *)writer.pres->object;
    pos = stell(pdev->streams.strm);
    /* Pattern masks are specified in device coordinates, so invert Y. */
    if ((code = pdf_copy_color_bits(writer.binary.strm, p_tile->tbits.data + (h - 1) * p_tile->tbits.raster, 0, -p_tile->tbits.raster, w, h, pdev->color_info.depth >> 3)) < 0 ||
	(code = cos_stream_add_since(pcs_image, pos)) < 0 ||
	(code = pdf_end_image_binary(pdev, &writer, h)) < 0
	)
	return code;
    pcs_image = (cos_stream_t *)writer.pres->object;
    if ((pcs_mask != 0 &&
	 (code = cos_dict_put_c_key_object(cos_stream_dict(pcs_image), "/Mask",
					   COS_OBJECT(pcs_mask))) < 0) ||
	(code = pdf_end_write_image(pdev, &writer)) < 0
	)
	return code;
    code = pdf_pattern(pdev, pdc, p_tile, m_tile, pcs_image, ppres);
    if (code < 0)
	return code;
    cos_value_write(&v, pdev);
    pprints1(pdev->strm, " %s", ppscc->setcolorspace);
    return 0;
}

/* Write an uncolored Pattern color. */
/* (Single-use procedure for readability.) */
private int
pdf_put_uncolored_pattern(gx_device_pdf *pdev, const gx_drawing_color *pdc,
			  const psdf_set_color_commands_t *ppscc,
			  pdf_resource_t **ppres)
{
    const gx_color_tile *m_tile = pdc->mask.m_tile;
    cos_value_t v;
    stream *s = pdev->strm;
    int code = pdf_cs_Pattern_uncolored(pdev, &v);
    cos_stream_t *pcs_image;
    gx_drawing_color dc_pure;
    static const psdf_set_color_commands_t no_scc = {0, 0, 0};

    if (code < 0 ||
	(code = pdf_put_pattern_mask(pdev, m_tile, &pcs_image)) < 0 ||
	(code = pdf_pattern(pdev, pdc, NULL, m_tile, pcs_image, ppres)) < 0
	)
	return code;
    cos_value_write(&v, pdev);
    pprints1(s, " %s ", ppscc->setcolorspace);
    color_set_pure(&dc_pure, gx_dc_pure_color(pdc));
    psdf_set_color((gx_device_vector *)pdev, &dc_pure, &no_scc);
    return 0;
}

/* ---------------- PatternType 2 colors ---------------- */

/* Write parameters common to all Shadings. */
private int
pdf_put_shading_common(cos_dict_t *pscd, const gs_shading_t *psh)
{
    gs_shading_type_t type = ShadingType(psh);
    const gs_color_space *pcs = psh->params.ColorSpace;
    int code = cos_dict_put_c_key_int(pscd, "/ShadingType", (int)type);
    cos_value_t cs_value;

    if (code < 0 ||
	(psh->params.AntiAlias &&
	 (code = cos_dict_put_c_strings(pscd, "/AntiAlias", "true")) < 0) ||
	(code = pdf_color_space(pscd->pdev, &cs_value, pcs,
				&pdf_color_space_names, false)) < 0 ||
	(code = cos_dict_put_c_key(pscd, "/ColorSpace", &cs_value)) < 0
	)
	return code;
    if (psh->params.Background) {
	code = cos_dict_put_c_key_floats(pscd, "/Background",
				   psh->params.Background->paint.values,
				   gs_color_space_num_components(pcs));
	if (code < 0)
	    return code;
    }
    if (psh->params.have_BBox) {
	float bbox[4];

	bbox[0] = psh->params.BBox.p.x;
	bbox[1] = psh->params.BBox.p.y;
	bbox[2] = psh->params.BBox.q.x;
	bbox[3] = psh->params.BBox.q.y;
	code = cos_dict_put_c_key_floats(pscd, "/BBox", bbox, 4);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* Write a linear (Axial / Radial) Shading. */
private int
pdf_put_linear_shading(cos_dict_t *pscd, const float *Coords,
		       int num_coords, const float *Domain /*[2]*/,
		       const gs_function_t *Function,
		       const bool *Extend /*[2]*/)
{
    int code = cos_dict_put_c_key_floats(pscd, "/Coords", Coords, num_coords);

    if (code < 0 ||
	((Domain[0] != 0 || Domain[1] != 1) &&
	 (code = cos_dict_put_c_key_floats(pscd, "/Domain", Domain, 2)) < 0)
	)
	return code;
    if (Function) {
	cos_value_t fn_value;

	if ((code = pdf_function(pscd->pdev, Function, &fn_value)) < 0 ||
	    (code = cos_dict_put_c_key(pscd, "/Function", &fn_value)) < 0
	    )
	    return code;
    }
    if (Extend[0] | Extend[1]) {
	char extend_str[1 + 5 + 1 + 5 + 1 + 1]; /* [bool bool] */

	sprintf(extend_str, "[%s %s]",
		(Extend[0] ? "true" : "false"),
		(Extend[1] ? "true" : "false"));
	code = cos_dict_put_c_key_string(pscd, "/Extend",
					 (const byte *)extend_str,
					 strlen(extend_str));
    }
    return code;
}

/* Write a scalar (non-mesh) Shading. */
/* (Single-use procedure for readability.) */
private int
pdf_put_scalar_shading(cos_dict_t *pscd, const gs_shading_t *psh)
{
    int code = pdf_put_shading_common(pscd, psh);

    if (code < 0)
	return code;
    switch (ShadingType(psh)) {
    case shading_type_Function_based: {
	const gs_shading_Fb_params_t *const params =
	    (const gs_shading_Fb_params_t *)&psh->params;
	cos_value_t fn_value;

	if ((code = cos_dict_put_c_key_floats(pscd, "/Domain", params->Domain, 4)) < 0 ||
	    (code = pdf_function(pscd->pdev, params->Function, &fn_value)) < 0 ||
	    (code = cos_dict_put_c_key(pscd, "/Function", &fn_value)) < 0 ||
	    (code = cos_dict_put_matrix(pscd, "/Matrix", &params->Matrix)) < 0
	    )
	    return code;
	return 0;
    }
    case shading_type_Axial: {
	const gs_shading_A_params_t *const params =
	    (const gs_shading_A_params_t *)&psh->params;

	return pdf_put_linear_shading(pscd, params->Coords, 4,
				      params->Domain, params->Function,
				      params->Extend);
    }
    case shading_type_Radial: {
	const gs_shading_R_params_t *const params =
	    (const gs_shading_R_params_t *)&psh->params;

	return pdf_put_linear_shading(pscd, params->Coords, 6,
				      params->Domain, params->Function,
				      params->Extend);
    }
    default:
	return_error(gs_error_rangecheck);
    }
}

/* Add an integer range to an array. */
private int
pdf_array_add_int2(cos_array_t *pca, int lower, int upper)
{
    int code = cos_array_add_int(pca, lower);

    if (code >= 0)
	code = cos_array_add_int(pca, upper);
    return code;
}

/* Put a clamped value into a data stream.  num_bytes < sizeof(int). */
private void
put_clamped(byte *p, floatp v, int num_bytes)
{
    int limit = 1 << (num_bytes * 8);
    int i, shift;

    if (v <= -limit)
	i = -limit + 1;
    else if (v >= limit)
	i = limit - 1;
    else
	i = (int)v;
    for (shift = (num_bytes - 1) * 8; shift >= 0; shift -= 8)
	*p++ = (byte)(i >> shift);
}
inline private void
put_clamped_coord(byte *p, floatp v, int num_bytes)
{
    put_clamped(p, (v + 32768) * 0xffffff / 65535, num_bytes);
}

/* Convert floating-point mesh data to packed binary. */
/* BitsPerFlag = 8, BitsPerCoordinate = 24, BitsPerComponent = 16, */
/* scaling is as defined below. */
private int
put_float_mesh_data(cos_stream_t *pscs, shade_coord_stream_t *cs,
		    int flag, int num_pts, int num_components,
		    bool is_indexed)
{
    byte b[1 + (3 + 3) * 16];	/* flag + x + y or c */
    gs_fixed_point pts[16];
    int i;
    int code;

    b[0] = (byte)flag;		/* may be -1 */
    if ((code = shade_next_coords(cs, pts, num_pts)) < 0)
	return code;
    for (i = 0; i < num_pts; ++i) {
	put_clamped_coord(b + 1 + i * 6, fixed2float(pts[i].x), 3);
	put_clamped_coord(b + 4 + i * 6, fixed2float(pts[i].y), 3);
    }
    if ((code = cos_stream_add_bytes(pscs, b + (flag < 0),
				     (flag >= 0) + num_pts * 6)) < 0)
	return code;
    for (i = 0; i < num_components; ++i) {
	float c;

	cs->get_decoded(cs, 0, NULL, &c);
	put_clamped(b, (is_indexed ? c + 32768 : (c + 256) * 65535 / 511), 2);
	if ((code = cos_stream_add_bytes(pscs, b, 2)) < 0)
	    return code;
    }
    return 0;
}

/* Write a mesh Shading. */
/* (Single-use procedure for readability.) */
private int
pdf_put_mesh_shading(cos_stream_t *pscs, const gs_shading_t *psh)
{
    cos_dict_t *const pscd = cos_stream_dict(pscs);
    gs_color_space *pcs = psh->params.ColorSpace;
    const gs_shading_mesh_params_t *const pmp =
	(const gs_shading_mesh_params_t *)&psh->params;
    int code = pdf_put_shading_common(pscd, psh);
    int bits_per_coordinate, bits_per_component, bits_per_flag;
    int num_comp = (pmp->Function ? 1 : gs_color_space_num_components(pcs));
    bool from_array = data_source_is_array(pmp->DataSource);
    bool is_indexed;
    shade_coord_stream_t cs;
    gs_matrix_fixed ctm_ident;
    int flag;

    if (code < 0)
	return code;

    /* Write parameters common to all mesh Shadings. */
    shade_next_init(&cs, pmp, NULL);
    if (from_array) {
	cos_array_t *pca = cos_array_alloc(pscd->pdev, "pdf_put_mesh_shading");
	int i;

	if (pca == 0)
	    return_error(gs_error_VMerror);
	for (i = 0; i < 2; ++i)
	    if ((code = pdf_array_add_int2(pca, -32768, 32767)) < 0)
		return code;
	if (gs_color_space_get_index(pcs) == gs_color_space_index_Indexed) {
	    is_indexed = true;
	    if ((code = pdf_array_add_int2(pca, -32768, 32767)) < 0)
		return code;
	} else {
	    is_indexed = false;
	    for (i = 0; i < num_comp; ++i)
		if ((code = pdf_array_add_int2(pca, -256, 255)) < 0)
		    return code;
	}
	code = cos_dict_put_c_key_object(pscd, "/Decode", COS_OBJECT(pca));
	if (code < 0)
	    return code;
	bits_per_coordinate = 24;
	bits_per_component = 16;
	bits_per_flag = 8;
	gs_make_identity((gs_matrix *)&ctm_ident);
	ctm_ident.tx_fixed = ctm_ident.ty_fixed = 0;
	cs.pctm = &ctm_ident;
    } else {
	byte buf[100];		/* arbitrary */
	uint num_read;

	code = cos_dict_put_c_key_floats(pscd, "/Decode", pmp->Decode,
				4 + gs_color_space_num_components(pcs) * 2);
	while (sgets(cs.s, buf, sizeof(buf), &num_read), num_read > 0)
	    if ((code = cos_stream_add_bytes(pscs, buf, num_read)) < 0)
		return code;
	bits_per_coordinate = pmp->BitsPerCoordinate;
	bits_per_component = pmp->BitsPerComponent;
	bits_per_flag = -1;
    }
    if (code < 0 ||
	(code = cos_dict_put_c_key_int(pscd, "/BitsPerCoordinate",
				       bits_per_coordinate)) < 0 ||
	(code = cos_dict_put_c_key_int(pscd, "/BitsPerComponent",
				       bits_per_component)) < 0
	)
	return code;

    switch (ShadingType(psh)) {
    case shading_type_Free_form_Gouraud_triangle: {
	const gs_shading_FfGt_params_t *const params =
	    (const gs_shading_FfGt_params_t *)pmp;

	if (from_array)
	    while ((flag = shade_next_flag(&cs, 0)) >= 0)
		if ((code = put_float_mesh_data(pscs, &cs, flag, 1, num_comp,
						is_indexed)) < 0)
		    return code;
	if (bits_per_flag < 0)
	    bits_per_flag = params->BitsPerFlag;
	break;
    }
    case shading_type_Lattice_form_Gouraud_triangle: {
	const gs_shading_LfGt_params_t *const params =
	    (const gs_shading_LfGt_params_t *)pmp;

	if (from_array)
	    while (!seofp(cs.s))
		if ((code = put_float_mesh_data(pscs, &cs, -1, 1, num_comp,
						is_indexed)) < 0)
		    return code;
	return cos_dict_put_c_key_int(pscd, "/VerticesPerRow",
				      params->VerticesPerRow);
    }
    case shading_type_Coons_patch: {
	const gs_shading_Cp_params_t *const params =
	    (const gs_shading_Cp_params_t *)pmp;

	if (from_array)
	    while ((flag = shade_next_flag(&cs, 0)) >= 0) {
		int num_c = (flag == 0 ? 4 : 2);

		if ((code = put_float_mesh_data(pscs, &cs, flag, 4 + num_c * 2,
						num_comp * num_c,
						is_indexed)) < 0)
		    return code;
	    }
	if (bits_per_flag < 0)
	    bits_per_flag = params->BitsPerFlag;
	break;
    }
    case shading_type_Tensor_product_patch: {
	const gs_shading_Tpp_params_t *const params =
	    (const gs_shading_Tpp_params_t *)pmp;

	if (from_array)
	    while ((flag = shade_next_flag(&cs, 0)) >= 0) {
		int num_c = (flag == 0 ? 4 : 2);

		if ((code = put_float_mesh_data(pscs, &cs, flag, 8 + num_c * 2,
						num_comp * num_c,
						is_indexed)) < 0)
		    return code;
	    }
	if (bits_per_flag < 0)
	    bits_per_flag = params->BitsPerFlag;
	break;
    }
    default:
	return_error(gs_error_rangecheck);
    }
    return cos_dict_put_c_key_int(pscd, "/BitsPerFlag", bits_per_flag);
}

/* Write a PatternType 2 (shading pattern) color. */
/* (Single-use procedure for readability.) */
private int
pdf_put_pattern2(gx_device_pdf *pdev, const gx_drawing_color *pdc,
		 const psdf_set_color_commands_t *ppscc,
		 pdf_resource_t **ppres)
{
    const gs_pattern2_instance_t *pinst =
	(gs_pattern2_instance_t *)pdc->ccolor.pattern;
    const gs_shading_t *psh = pinst->template.Shading;
    cos_value_t v;
    pdf_resource_t *pres;
    pdf_resource_t *psres;
    cos_dict_t *pcd;
    cos_object_t *psco;
    int code = pdf_cs_Pattern_colored(pdev, &v);
    gs_matrix smat;

    if (code < 0)
	return code;
    code = pdf_alloc_resource(pdev, resourcePattern, gs_no_id, ppres, 0L);
    if (code < 0)
	return code;
    pres = *ppres;
    cos_become(pres->object, cos_type_dict);
    pcd = (cos_dict_t *)pres->object;
    code = pdf_alloc_resource(pdev, resourceShading, gs_no_id, &psres, 0L);
    if (code < 0)
	return code;
    psco = psres->object;
    if (ShadingType(psh) >= 4) {
	/* Shading has an associated data stream. */
	cos_become(psco, cos_type_stream);
	code = pdf_put_mesh_shading((cos_stream_t *)psco, psh);
    } else {
	cos_become(psco, cos_type_dict);
	code = pdf_put_scalar_shading((cos_dict_t *)psco, psh);
    }
    /*
     * In PDF, the Matrix is the transformation from the pattern space to
     * the *default* user coordinate space, not the current space.
     */
    gs_currentmatrix(pinst->saved, &smat);
    {
	double xscale = 72.0 / pdev->HWResolution[0],
	    yscale = 72.0 / pdev->HWResolution[1];

	smat.xx *= xscale, smat.yx *= xscale, smat.tx *= xscale;
	smat.xy *= yscale, smat.yy *= yscale, smat.ty *= yscale;
    }
    if (code < 0 ||
	(code = cos_dict_put_c_key_int(pcd, "/PatternType", 2)) < 0 ||
	(code = cos_dict_put_c_key_object(pcd, "/Shading", psco)) < 0 ||
	(code = cos_dict_put_matrix(pcd, "/Matrix", &smat)) < 0
	/****** ExtGState ******/
	)
	return code;
    cos_value_write(&v, pdev);
    pprints1(pdev->strm, " %s", ppscc->setcolorspace);
    return 0;
}

/* ---------------- Public procedure ---------------- */

/* Write a color value.  rgs is "rg" for fill, "RG" for stroke. */
int
pdf_put_drawing_color(gx_device_pdf *pdev, const gx_drawing_color *pdc,
		      const psdf_set_color_commands_t *ppscc)
{
    if (gx_dc_is_pure(pdc))
	return psdf_set_color((gx_device_vector *) pdev, pdc, ppscc);
    else {
	/* We never halftone, so this must be a Pattern. */
	int code;
	pdf_resource_t *pres;
	cos_value_t v;

	if (pdc->type == gx_dc_type_pattern)
	    code = pdf_put_colored_pattern(pdev, pdc, ppscc, &pres);
	else if (pdc->type == &gx_dc_pure_masked)
	    code = pdf_put_uncolored_pattern(pdev, pdc, ppscc, &pres);
	else if (pdc->type == &gx_dc_pattern2)
	    code = pdf_put_pattern2(pdev, pdc, ppscc, &pres);
	else
	    return_error(gs_error_rangecheck);
	if (code < 0)
	    return code;
	cos_value_write(cos_resource_value(&v, pres->object), pdev);
	pprints1(pdev->strm, " %s\n", ppscc->setcolorn);
	return 0;
    }
}
