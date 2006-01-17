/* Copyright (C) 1996, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfi.c,v 1.73 2005/08/29 18:21:57 igor Exp $ */
/* Image handling for PDF-writing driver */
#include "memory_.h"
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsdevice.h"
#include "gsflip.h"
#include "gsstate.h"
#include "gscolor2.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"		/* for data stream */
#include "gxcspace.h"
#include "gximage3.h"
#include "gximag3x.h"
#include "gsiparm4.h"
#include "gxdcolor.h"
#include "gxpcolor.h"
#include "gxcolor2.h"
#include "gxhldevc.h"


/* Forward references */
private image_enum_proc_plane_data(pdf_image_plane_data);
private image_enum_proc_end_image(pdf_image_end_image);
private image_enum_proc_end_image(pdf_image_end_image_object);
private image_enum_proc_end_image(pdf_image_end_image_object2);
private image_enum_proc_end_image(pdf_image_end_image_cvd);
private IMAGE3_MAKE_MID_PROC(pdf_image3_make_mid);
private IMAGE3_MAKE_MCDE_PROC(pdf_image3_make_mcde);
private IMAGE3X_MAKE_MID_PROC(pdf_image3x_make_mid);
private IMAGE3X_MAKE_MCDE_PROC(pdf_image3x_make_mcde);

private const gx_image_enum_procs_t pdf_image_enum_procs = {
    pdf_image_plane_data,
    pdf_image_end_image
};
private const gx_image_enum_procs_t pdf_image_object_enum_procs = {
    pdf_image_plane_data,
    pdf_image_end_image_object
};
private const gx_image_enum_procs_t pdf_image_object_enum_procs2 = {
    pdf_image_plane_data,
    pdf_image_end_image_object2
};
private const gx_image_enum_procs_t pdf_image_cvd_enum_procs = {
    gx_image1_plane_data,
    pdf_image_end_image_cvd,
    gx_image1_flush
};

/* ---------------- Driver procedures ---------------- */

/* Define the structure for keeping track of progress through an image. */
typedef struct pdf_image_enum_s {
    gx_image_enum_common;
    gs_memory_t *memory;
    int width;
    int bits_per_pixel;		/* bits per pixel (per plane) */
    int rows_left;
    pdf_image_writer writer;
    gs_matrix mat;
} pdf_image_enum;
gs_private_st_composite(st_pdf_image_enum, pdf_image_enum, "pdf_image_enum",
  pdf_image_enum_enum_ptrs, pdf_image_enum_reloc_ptrs);
/* GC procedures */
private ENUM_PTRS_WITH(pdf_image_enum_enum_ptrs, pdf_image_enum *pie)
    if (index < pdf_image_writer_max_ptrs) {
	gs_ptr_type_t ret =
	    ENUM_USING(st_pdf_image_writer, &pie->writer, sizeof(pie->writer),
		       index);

	if (ret == 0)		/* don't stop early */
	    ENUM_RETURN(0);
	return ret;
    }
    return ENUM_USING_PREFIX(st_gx_image_enum_common,
			     pdf_image_writer_max_ptrs);
ENUM_PTRS_END
private RELOC_PTRS_WITH(pdf_image_enum_reloc_ptrs, pdf_image_enum *pie)
{
    RELOC_USING(st_pdf_image_writer, &pie->writer, sizeof(pie->writer));
    RELOC_USING(st_gx_image_enum_common, vptr, size);
}
RELOC_PTRS_END

/*
 * Test whether we can write an image in-line.  This is always true,
 * because we only support PDF 1.2 and later.
 */
private bool
can_write_image_in_line(const gx_device_pdf *pdev, const gs_image_t *pim)
{
    return true;
}

/*
 * Convert a Type 4 image to a Type 1 masked image if possible.
 * Type 1 masked images are more compact, and are supported in all PDF
 * versions, whereas general masked images require PDF 1.3 or higher.
 * Also, Acrobat 5 for Windows has a bug that causes an error for images
 * with a color-key mask, at least for 1-bit-deep images using an Indexed
 * color space.
 */
private int
color_is_black_or_white(gx_device *dev, const gx_drawing_color *pdcolor)
{
    return (!color_is_pure(pdcolor) ? -1 :
	    gx_dc_pure_color(pdcolor) == gx_device_black(dev) ? 0 :
	    gx_dc_pure_color(pdcolor) == gx_device_white(dev) ? 1 : -1);
}
private int
pdf_convert_image4_to_image1(gx_device_pdf *pdev,
			     const gs_imager_state *pis,
			     const gx_drawing_color *pbcolor,
			     const gs_image4_t *pim4, gs_image_t *pim1,
			     gx_drawing_color *pdcolor)
{
    if (pim4->BitsPerComponent == 1 &&
	(pim4->MaskColor_is_range ?
	 pim4->MaskColor[0] | pim4->MaskColor[1] :
	 pim4->MaskColor[0]) <= 1
	) {
	gx_device *const dev = (gx_device *)pdev;
	const gs_color_space *pcs = pim4->ColorSpace;
	bool write_1s = !pim4->MaskColor[0];
	gs_client_color cc;
	int code;

	/*
	 * Prepare the drawing color.  (pdf_prepare_imagemask will set it.)
	 * This is the other color in the image (the one that isn't the
	 * mask key), taking Decode into account.
	 */

	cc.paint.values[0] = pim4->Decode[(int)write_1s];
	cc.pattern = 0;
	code = pcs->type->remap_color(&cc, pcs, pdcolor, pis, dev,
				      gs_color_select_texture);
	if (code < 0)
	    return code;

	/*
	 * The PDF imaging model doesn't support RasterOp.  We can convert a
	 * Type 4 image to a Type 1 imagemask only if the effective RasterOp
	 * passes through the source color unchanged.  "Effective" means we
	 * take into account CombineWithColor, and whether the source and/or
	 * texture are black, white, or neither.
	 */
	{
	    gs_logical_operation_t lop = pis->log_op;
	    int black_or_white = color_is_black_or_white(dev, pdcolor);

	    switch (black_or_white) {
	    case 0: lop = lop_know_S_0(lop); break;
	    case 1: lop = lop_know_S_1(lop); break;
	    default: DO_NOTHING;
	    }
	    if (pim4->CombineWithColor)
		switch (color_is_black_or_white(dev, pbcolor)) {
		case 0: lop = lop_know_T_0(lop); break;
		case 1: lop = lop_know_T_1(lop); break;
		default: DO_NOTHING;
		}
	    else
		lop = lop_know_T_0(lop);
	    switch (lop_rop(lop)) {
	    case rop3_0:
		if (black_or_white != 0)
		    return -1;
		break;
	    case rop3_1:
		if (black_or_white != 1)
		    return -1;
		break;
	    case rop3_S:
		break;
	    default:
		return -1;
	    }
	    if ((lop & lop_S_transparent) && black_or_white == 1)
		return -1;
	}

	/* All conditions are met.  Convert to a masked image. */

	gs_image_t_init_mask_adjust(pim1, write_1s, false);
#define COPY_ELEMENT(e) pim1->e = pim4->e
	COPY_ELEMENT(ImageMatrix);
	COPY_ELEMENT(Width);
	COPY_ELEMENT(Height);
	pim1->BitsPerComponent = 1;
	/* not Decode */
	COPY_ELEMENT(Interpolate);
	pim1->format = gs_image_format_chunky; /* BPC = 1, doesn't matter */
#undef COPY_ELEMENT
	return 0;
    }
    return -1;			/* arbitrary <0 */
}

private int
pdf_begin_image_data_decoded(gx_device_pdf *pdev, int num_components, const gs_range_t *pranges, int i, 
			     gs_pixel_image_t *pi, cos_value_t *cs_value, pdf_image_enum *pie)
{

    if (pranges) {
	/* Rescale the Decode values for the image data. */
	const gs_range_t *pr = pranges;
	float *decode = pi->Decode;
	int j;

	for (j = 0; j < num_components; ++j, ++pr, decode += 2) {
	    double vmin = decode[0], vmax = decode[1];
	    double base = pr->rmin, factor = pr->rmax - base;

	    decode[1] = (vmax - vmin) / factor + (vmin - base);
	    decode[0] = vmin - base;
	}
    }
    return pdf_begin_image_data(pdev, &pie->writer, pi, cs_value, i);
}

/*
 * Start processing an image.  This procedure takes extra arguments because
 * it has to do something slightly different for the parts of an ImageType 3
 * image.
 */
typedef enum {
    PDF_IMAGE_DEFAULT,
    PDF_IMAGE_TYPE3_MASK,	/* no in-line, don't render */
    PDF_IMAGE_TYPE3_DATA	/* no in-line */
} pdf_typed_image_context_t;
private int
pdf_begin_typed_image(gx_device_pdf *pdev, const gs_imager_state * pis,
		      const gs_matrix *pmat, const gs_image_common_t *pic,
		      const gs_int_rect * prect,
		      const gx_drawing_color * pdcolor,
		      const gx_clip_path * pcpath, gs_memory_t * mem,
		      gx_image_enum_common_t ** pinfo,
		      pdf_typed_image_context_t context)
{
    cos_dict_t *pnamed = 0;
    const gs_pixel_image_t *pim;
    int code, i;
    pdf_image_enum *pie;
    gs_image_format_t format;
    const gs_color_space *pcs;
    gs_color_space cs_gray_temp;
    cos_value_t cs_value;
    int num_components;
    bool is_mask = false, in_line = false;
    gs_int_rect rect;
    /*
     * We define this union because psdf_setup_image_filters may alter the
     * gs_pixel_image_t part, but pdf_begin_image_data must also have access
     * to the type-specific parameters.
     */
    union iu_ {
	gs_pixel_image_t pixel;	/* we may change some components */
	gs_image1_t type1;
	gs_image3_t type3;
	gs_image3x_t type3x;
	gs_image4_t type4;
    } image[4];
    int width, height;
    const gs_range_t *pranges = 0;
    const pdf_color_space_names_t *names;
    bool convert_to_process_colors = false;
    pdf_lcvd_t *cvd = NULL;

    /*
     * Pop the image name from the NI stack.  We must do this, to keep the
     * stack in sync, even if it turns out we can't handle the image.
     */
    {
	cos_value_t ni_value;

	if (cos_array_unadd(pdev->NI_stack, &ni_value) >= 0)
	    pnamed = (cos_dict_t *)ni_value.contents.object;
    }

    /* An initialization for pdf_end_and_do_image :
       We need to delay adding the "Mask" entry into a type 3 image dictionary
       until the mask is completed due to equal image merging. */
    pdev->image_mask_id = gs_no_id;

    /* Check for the image types we can handle. */
    switch (pic->type->index) {
    case 1: {
	const gs_image_t *pim1 = (const gs_image_t *)pic;

	if (pim1->Alpha != gs_image_alpha_none)
	    goto nyi;
	is_mask = pim1->ImageMask;
	if (is_mask) {
	    /* If parameters are invalid, use the default implementation. */
	    if (pdcolor->type != &gx_dc_pattern)
		if (pim1->BitsPerComponent != 1 ||
		    !((pim1->Decode[0] == 0.0 && pim1->Decode[1] == 1.0) ||
		      (pim1->Decode[0] == 1.0 && pim1->Decode[1] == 0.0))
		    )
		    goto nyi;
	}
	in_line = context == PDF_IMAGE_DEFAULT &&
	    can_write_image_in_line(pdev, pim1);
	image[0].type1 = *pim1;
	break;
    }
    case 3: {
	const gs_image3_t *pim3 = (const gs_image3_t *)pic;
	gs_image3_t pim3a;
	const gs_image_common_t *pic1 = pic;
	gs_matrix m, mi;
	const gs_matrix *pmat1 = pmat;

	pdev->image_mask_is_SMask = false;
	if (pdev->CompatibilityLevel < 1.2)
	    goto nyi;
	if (prect && !(prect->p.x == 0 && prect->p.y == 0 &&
		       prect->q.x == pim3->Width &&
		       prect->q.y == pim3->Height))
	    goto nyi;
	if (pdev->CompatibilityLevel < 1.3 && !pdev->PatternImagemask) {
	    gs_make_identity(&m);
	    pmat1 = &m;
	    m.tx = floor(pis->ctm.tx + 0.5); /* Round the origin against the image size distorsions */
	    m.ty = floor(pis->ctm.ty + 0.5);
	    pim3a = *pim3;
	    gs_matrix_invert(&pim3a.ImageMatrix, &mi);
	    gs_make_identity(&pim3a.ImageMatrix);
	    if (pim3a.Width < pim3a.MaskDict.Width && pim3a.Width > 0) {
		int sx = (pim3a.MaskDict.Width + pim3a.Width - 1) / pim3a.Width;
		
		gs_matrix_scale(&mi, 1.0 / sx, 1, &mi);
		gs_matrix_scale(&pim3a.ImageMatrix, 1.0 / sx, 1, &pim3a.ImageMatrix);
	    }
	    if (pim3a.Height < pim3a.MaskDict.Height && pim3a.Height > 0) {
		int sy = (pim3a.MaskDict.Height + pim3a.Height - 1) / pim3a.Height;

		gs_matrix_scale(&mi, 1, 1.0 / sy, &mi);
		gs_matrix_scale(&pim3a.ImageMatrix, 1, 1.0 / sy, &pim3a.ImageMatrix);
	    }
	    gs_matrix_multiply(&mi, &pim3a.MaskDict.ImageMatrix, &pim3a.MaskDict.ImageMatrix);
	    pic1 = (gs_image_common_t *)&pim3a;
	    /* Setting pdev->converting_image_matrix to communicate with pdf_image3_make_mcde. */
	    gs_matrix_multiply(&mi, &ctm_only(pis), &pdev->converting_image_matrix);
	}
	/*
	 * We handle ImageType 3 images in a completely different way:
	 * the default implementation sets up the enumerator.
	 */
	return gx_begin_image3_generic((gx_device *)pdev, pis, pmat1, pic1,
				       prect, pdcolor, pcpath, mem,
				       pdf_image3_make_mid,
				       pdf_image3_make_mcde, pinfo);
    }
    case IMAGE3X_IMAGETYPE: {
	/* See ImageType3 above for more information. */
	const gs_image3x_t *pim3x = (const gs_image3x_t *)pic;

	if (pdev->CompatibilityLevel < 1.4)
	    goto nyi;
	if (prect && !(prect->p.x == 0 && prect->p.y == 0 &&
		       prect->q.x == pim3x->Width &&
		       prect->q.y == pim3x->Height))
	    goto nyi;
	pdev->image_mask_is_SMask = true;
	return gx_begin_image3x_generic((gx_device *)pdev, pis, pmat, pic,
					prect, pdcolor, pcpath, mem,
					pdf_image3x_make_mid,
					pdf_image3x_make_mcde, pinfo);
    }
    case 4: {
	/* Try to convert the image to a plain masked image. */
	gx_drawing_color icolor;

	pdev->image_mask_is_SMask = false;
	if (pdf_convert_image4_to_image1(pdev, pis, pdcolor,
					 (const gs_image4_t *)pic,
					 &image[0].type1, &icolor) >= 0) {
	    gs_state *pgs = (gs_state *)gx_hld_get_gstate_ptr(pis);

	    if (pgs == NULL)
		return_error(gs_error_unregistered); /* Must not happen. */

	    /* Undo the pop of the NI stack if necessary. */
	    if (pnamed)
		cos_array_add_object(pdev->NI_stack, COS_OBJECT(pnamed));
	    /* HACK: temporary patch the color space, to allow 
	       pdf_prepare_imagemask to write the right color for the imagemask. */
	    code = gs_gsave(pgs);
	    if (code < 0)
		return code;
	    code = gs_setcolorspace(pgs, ((const gs_image4_t *)pic)->ColorSpace);
	    if (code < 0)
		return code;
	    code = pdf_begin_typed_image(pdev, pis, pmat,
					 (gs_image_common_t *)&image[0].type1,
					 prect, &icolor, pcpath, mem,
					 pinfo, context);
	    if (code < 0)
		return code;
	    return gs_grestore(pgs);
	}
	/* No luck.  Masked images require PDF 1.3 or higher. */
	if (pdev->CompatibilityLevel < 1.2)
	    goto nyi;
	if (pdev->CompatibilityLevel < 1.3 && !pdev->PatternImagemask) {
	    gs_matrix m, m1, mi;
	    gs_image4_t pi4 = *(const gs_image4_t *)pic;

	    gs_make_identity(&m1);
	    gs_matrix_invert(&pic->ImageMatrix, &mi);
	    gs_matrix_multiply(&mi, &ctm_only(pis), &m);
	    code = pdf_setup_masked_image_converter(pdev, mem, &m, &cvd, 
				 true, 0, 0, pi4.Width, pi4.Height, false);
	    if (code < 0)
		return code;
	    cvd->mdev.is_open = true; /* fixme: same as above. */
	    cvd->mask->is_open = true; /* fixme: same as above. */
	    cvd->mask_is_empty = false;
	    code = (*dev_proc(cvd->mask, fill_rectangle))((gx_device *)cvd->mask, 
			0, 0, cvd->mask->width, cvd->mask->height, (gx_color_index)0);
	    if (code < 0)
		return code;
	    gx_device_retain((gx_device *)cvd, true);
	    gx_device_retain((gx_device *)cvd->mask, true);
	    gs_make_identity(&pi4.ImageMatrix);
	    code = gx_default_begin_typed_image((gx_device *)cvd, 
		pis, &m1, (gs_image_common_t *)&pi4, prect, pdcolor, NULL, mem, pinfo);
	    if (code < 0)
		return code;
	    (*pinfo)->procs = &pdf_image_cvd_enum_procs;
	    return 0;
	}
	image[0].type4 = *(const gs_image4_t *)pic;
	break;
    }
    default:
	goto nyi;
    }
    pim = (const gs_pixel_image_t *)pic;
    format = pim->format;
    switch (format) {
    case gs_image_format_chunky:
    case gs_image_format_component_planar:
	break;
    default:
	goto nyi;
    }
    /* AR5 on Windows doesn't support 0-size images. Skipping. */
    if (pim->Width == 0 || pim->Height == 0)
	goto nyi;
    /* PDF doesn't support images with more than 8 bits per component. */
    if (pim->BitsPerComponent > 8)
	goto nyi;
    pcs = pim->ColorSpace;
    num_components = (is_mask ? 1 : gs_color_space_num_components(pcs));

    if (pdf_must_put_clip_path(pdev, pcpath))
	code = pdf_unclip(pdev);
    else 
	code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    if (context == PDF_IMAGE_TYPE3_MASK) {
	/*
	 * The soft mask for an ImageType 3x image uses a DevicePixel
	 * color space, which pdf_color_space() can't handle.  Patch it
	 * to DeviceGray here.
	 */
	gs_cspace_init_DeviceGray(pdev->memory, &cs_gray_temp);
	pcs = &cs_gray_temp;
    } else if (is_mask)
	code = pdf_prepare_imagemask(pdev, pis, pdcolor);
    else
	code = pdf_prepare_image(pdev, pis);
    if (code < 0)
	goto nyi;
    code = pdf_put_clip_path(pdev, pcpath);
    if (code < 0)
	return code;
    if (prect)
	rect = *prect;
    else {
	rect.p.x = rect.p.y = 0;
	rect.q.x = pim->Width, rect.q.y = pim->Height;
    }
    pie = gs_alloc_struct(mem, pdf_image_enum, &st_pdf_image_enum,
			"pdf_begin_image");
    if (pie == 0)
	return_error(gs_error_VMerror);
    memset(pie, 0, sizeof(*pie)); /* cleanup entirely for GC to work in all cases. */
    *pinfo = (gx_image_enum_common_t *) pie;
    gx_image_enum_common_init(*pinfo, (const gs_data_image_t *) pim,
		    ((pdev->CompatibilityLevel >= 1.3) ? 
			    (context == PDF_IMAGE_TYPE3_MASK ?
			    &pdf_image_object_enum_procs :
			    &pdf_image_enum_procs) :
			    context == PDF_IMAGE_TYPE3_MASK ?
			    &pdf_image_object_enum_procs :
			    context == PDF_IMAGE_TYPE3_DATA ?
			    &pdf_image_object_enum_procs2 :
			    &pdf_image_enum_procs),
			(gx_device *)pdev, num_components, format);
    pie->memory = mem;
    width = rect.q.x - rect.p.x;
    pie->width = width;
    height = rect.q.y - rect.p.y;
    pie->bits_per_pixel =
	pim->BitsPerComponent * num_components / pie->num_planes;
    pie->rows_left = height;
    if (pnamed != 0) /* Don't in-line the image if it is named. */
	in_line = false;
    else {
        double nbytes = (double)(((ulong) pie->width * pie->bits_per_pixel + 7) >> 3) *
	    pie->num_planes * pie->rows_left;
	
	in_line &= (nbytes < pdev->MaxInlineImageSize);
    }
    if (rect.p.x != 0 || rect.p.y != 0 ||
	rect.q.x != pim->Width || rect.q.y != pim->Height ||
	(is_mask && pim->CombineWithColor)
	/* Color space setup used to be done here: see SRZB comment below. */
	) {
	gs_free_object(mem, pie, "pdf_begin_image");
	goto nyi;
    }
    if (pmat == 0)
	pmat = &ctm_only(pis);
    {
	gs_matrix mat;
	gs_matrix bmat;
	int code;

	pdf_make_bitmap_matrix(&bmat, -rect.p.x, -rect.p.y,
			       pim->Width, pim->Height, height);
	if ((code = gs_matrix_invert(&pim->ImageMatrix, &mat)) < 0 ||
	    (code = gs_matrix_multiply(&bmat, &mat, &mat)) < 0 ||
	    (code = gs_matrix_multiply(&mat, pmat, &pie->mat)) < 0
	    ) {
	    gs_free_object(mem, pie, "pdf_begin_image");
	    return code;
	}
	/* AR3,AR4 show no image when CTM is singular; AR5 reports an error */
	if (pie->mat.xx * pie->mat.yy == pie->mat.xy * pie->mat.yx) {
	    gs_free_object(mem, pie, "pdf_begin_image");
	    goto nyi;
	}
    }
    pdf_image_writer_init(&pie->writer);
    pie->writer.alt_writer_count = (in_line || 
				    (pim->Width <= 64 && pim->Height <= 64) ||
				    pdev->transfer_not_identity ? 1 : 2);
    image[1] = image[0];
    names = (in_line ? &pdf_color_space_names_short : &pdf_color_space_names);
    if (psdf_is_converting_image_to_RGB((gx_device_psdf *)pdev, pis, pim)) {
	/* psdf_setup_image_filters may change the color space
	 * (in case of pdev->params.ConvertCMYKImagesToRGB == true).
	 * Account it here.
	 */
	cos_c_string_value(&cs_value, names->DeviceRGB);
    } else if (!is_mask) {
	code = pdf_color_space(pdev, &cs_value, &pranges,
				 pcs,
				 names, in_line);
	if (code < 0) {
	    const char *sname;

	    convert_to_process_colors = true;
	    switch (pdev->pcm_color_info_index) {
		case gs_color_space_index_DeviceGray: sname = names->DeviceGray; break;
		case gs_color_space_index_DeviceRGB:  sname = names->DeviceRGB;  break;
		case gs_color_space_index_DeviceCMYK: sname = names->DeviceCMYK; break;
		default:
		    eprintf("Unsupported ProcessColorModel.");
		    return_error(gs_error_undefined);
	    }
	    cos_c_string_value(&cs_value, sname);
	}
    }
    if ((code = pdf_begin_write_image(pdev, &pie->writer, gs_no_id, width,
		    height, pnamed, in_line)) < 0 ||
	/*
	 * Some regrettable PostScript code (such as LanguageLevel 1 output
	 * from Microsoft's PSCRIPT.DLL driver) misuses the transfer
	 * function to accomplish the equivalent of indexed color.
	 * Downsampling (well, only averaging) or JPEG compression are not
	 * compatible with this.  Play it safe by using only lossless
	 * filters if the transfer function(s) is/are other than the
	 * identity.
	 */
	(code = (pie->writer.alt_writer_count == 1 ?
		 psdf_setup_lossless_filters((gx_device_psdf *) pdev,
					     &pie->writer.binary[0],
					     &image[0].pixel) :
		 psdf_setup_image_filters((gx_device_psdf *) pdev,
					  &pie->writer.binary[0], &image[0].pixel,
					  pmat, pis, true))) < 0
	)
	goto fail;
    if (convert_to_process_colors) {
	code = psdf_setup_image_colors_filter(&pie->writer.binary[0], 
		    (gx_device_psdf *)pdev, &image[0].pixel, pis,
		    pdev->pcm_color_info_index);
	if (code < 0)
  	    goto fail;
    }
    if (pie->writer.alt_writer_count > 1) {
        code = pdf_make_alt_stream(pdev, &pie->writer.binary[1]);
        if (code)
            goto fail;
	code = psdf_setup_image_filters((gx_device_psdf *) pdev,
				  &pie->writer.binary[1], &image[1].pixel,
				  pmat, pis, false);
	if (code == gs_error_rangecheck) {
	    /* setup_image_compression rejected the alternative compression. */
	    pie->writer.alt_writer_count = 1;
	    memset(pie->writer.binary + 1, 0, sizeof(pie->writer.binary[1]));
	    memset(pie->writer.binary + 2, 0, sizeof(pie->writer.binary[1]));
	} else if (code)
	    goto fail;
	else if (convert_to_process_colors) {
	    code = psdf_setup_image_colors_filter(&pie->writer.binary[1], 
		    (gx_device_psdf *)pdev, &image[1].pixel, pis,
		    pdev->pcm_color_info_index);
	    if (code < 0)
  		goto fail;
	}
    }
    for (i = 0; i < pie->writer.alt_writer_count; i++) {
	code = pdf_begin_image_data_decoded(pdev, num_components, pranges, i,
			     &image[i].pixel, &cs_value, pie);
	if (code < 0)
  	    goto fail;
    }
    if (pie->writer.alt_writer_count == 2) {
        psdf_setup_compression_chooser(&pie->writer.binary[2], 
	     (gx_device_psdf *)pdev, pim->Width, pim->Height, 
	     num_components, pim->BitsPerComponent);
	pie->writer.alt_writer_count = 3;
    }
    if (pic->type->index == 4 && pdev->CompatibilityLevel < 1.3) {
	int i;

	/* Create a stream for writing the mask. */
	i = pie->writer.alt_writer_count;
	gs_image_t_init_mask_adjust((gs_image_t *)&image[i].type1, true, false);
	image[i].type1.Width = image[0].pixel.Width;
	image[i].type1.Height = image[0].pixel.Height;
	/* Won't use image[2]. */
	code = pdf_begin_write_image(pdev, &pie->writer, gs_no_id, width,
		    height, NULL, false);
        if (code)
            goto fail;
	code = psdf_setup_image_filters((gx_device_psdf *) pdev,
				  &pie->writer.binary[i], &image[i].pixel,
				  pmat, pis, true);
	if (code < 0)
  	    goto fail;
        psdf_setup_image_to_mask_filter(&pie->writer.binary[i], 
	     (gx_device_psdf *)pdev, pim->Width, pim->Height, 
	     num_components, pim->BitsPerComponent, image[i].type4.MaskColor);
	code = pdf_begin_image_data_decoded(pdev, num_components, pranges, i, 
			     &image[i].pixel, &cs_value, pie);
	if (code < 0)
  	    goto fail;
	++pie->writer.alt_writer_count;
	/* Note : Possible values for alt_writer_count are 1,2,3, 4.
	   1 means no alternative streams.
	   2 means the main image stream and a mask stream while converting 
		   an Image Type 4.
	   3 means the main image steram, alternative image compression stream, 
		   and the compression chooser.
	   4 meams 3 and a mask stream while convertingh an Image Type 4.
         */
    }
    return 0;
 fail:
    /****** SHOULD FREE STRUCTURES AND CLEAN UP HERE ******/
    /* Fall back to the default implementation. */
 nyi:
    return gx_default_begin_typed_image
	((gx_device *)pdev, pis, pmat, pic, prect, pdcolor, pcpath, mem,
	 pinfo);
}
int
gdev_pdf_begin_typed_image(gx_device * dev, const gs_imager_state * pis,
			   const gs_matrix *pmat, const gs_image_common_t *pic,
			   const gs_int_rect * prect,
			   const gx_drawing_color * pdcolor,
			   const gx_clip_path * pcpath, gs_memory_t * mem,
			   gx_image_enum_common_t ** pinfo)
{
    return pdf_begin_typed_image((gx_device_pdf *)dev, pis, pmat, pic, prect,
				 pdcolor, pcpath, mem, pinfo,
				 PDF_IMAGE_DEFAULT);
}

/* ---------------- All images ---------------- */

/* Process the next piece of an image. */
private int
pdf_image_plane_data_alt(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used, int alt_writer_index)
{
    pdf_image_enum *pie = (pdf_image_enum *) info;
    int h = height;
    int y;
    /****** DOESN'T HANDLE IMAGES WITH VARYING WIDTH PER PLANE ******/
    uint width_bits = pie->width * pie->plane_depths[0];
    /****** DOESN'T HANDLE NON-ZERO data_x CORRECTLY ******/
    uint bcount = (width_bits + 7) >> 3;
    uint ignore;
    int nplanes = pie->num_planes;
    int status = 0;

    if (h > pie->rows_left)
	h = pie->rows_left;
    for (y = 0; y < h; ++y) {
	if (nplanes > 1) {
	    /*
	     * We flip images in blocks, and each block except the last one
	     * must contain an integral number of pixels.  The easiest way
	     * to meet this condition is for all blocks except the last to
	     * be a multiple of 3 source bytes (guaranteeing an integral
	     * number of 1/2/4/8/12-bit samples), i.e., 3*nplanes flipped
	     * bytes.  This requires a buffer of at least
	     * 3*GS_IMAGE_MAX_COMPONENTS bytes.
	     */
	    int pi;
	    uint count = bcount;
	    uint offset = 0;
#define ROW_BYTES max(200 /*arbitrary*/, 3 * GS_IMAGE_MAX_COMPONENTS)
	    const byte *bit_planes[GS_IMAGE_MAX_COMPONENTS];
	    int block_bytes = ROW_BYTES / (3 * nplanes) * 3;
	    byte row[ROW_BYTES];

	    for (pi = 0; pi < nplanes; ++pi)
		bit_planes[pi] = planes[pi].data + planes[pi].raster * y;
	    while (count) {
		uint flip_count;
		uint flipped_count;

		if (count >= block_bytes) {
		    flip_count = block_bytes;
		    flipped_count = block_bytes * nplanes;
		} else {
		    flip_count = count;
		    flipped_count =
			(width_bits % (block_bytes * 8) * nplanes + 7) >> 3;
		}
		image_flip_planes(row, bit_planes, offset, flip_count,
				  nplanes, pie->plane_depths[0]);
		status = sputs(pie->writer.binary[alt_writer_index].strm, row, 
			       flipped_count, &ignore);
		if (status < 0)
		    break;
		offset += flip_count;
		count -= flip_count;
	    }
	} else {
	    status = sputs(pie->writer.binary[alt_writer_index].strm,
			   planes[0].data + planes[0].raster * y, bcount,
			   &ignore);
	}
	if (status < 0)
	    break;
    }
    *rows_used = h;
    if (status < 0)
	return_error(gs_error_ioerror);
    return !pie->rows_left;
#undef ROW_BYTES
}

private int
pdf_image_plane_data(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used)
{
    pdf_image_enum *pie = (pdf_image_enum *) info;
    int i;
    for (i = 0; i < pie->writer.alt_writer_count; i++) {
        int code = pdf_image_plane_data_alt(info, planes, height, rows_used, i);
        if (code)
            return code;
    }
    pie->rows_left -= *rows_used;
    if (pie->writer.alt_writer_count > 2)
        pdf_choose_compression(&pie->writer, false);
    return !pie->rows_left;
}

private int
use_image_as_pattern(gx_device_pdf *pdev, pdf_resource_t *pres1, 
		     const gs_matrix *pmat, gs_id id)
{   /* See also dump_image in gdevpdfd.c . */
    gs_imager_state s;
    gs_pattern1_instance_t inst;
    cos_value_t v;
    const pdf_resource_t *pres;
    int code;

    memset(&s, 0, sizeof(s));
    s.ctm.xx = pmat->xx;
    s.ctm.xy = pmat->xy;
    s.ctm.yx = pmat->yx;
    s.ctm.yy = pmat->yy;
    s.ctm.tx = pmat->tx;
    s.ctm.ty = pmat->ty;
    memset(&inst, 0, sizeof(inst));
    inst.saved = (gs_state *)&s; /* HACK : will use s.ctm only. */
    inst.template.PaintType = 1;
    inst.template.TilingType = 1;
    inst.template.BBox.p.x = inst.template.BBox.p.y = 0;
    inst.template.BBox.q.x = 1;
    inst.template.BBox.q.y = 1;
    inst.template.XStep = 2; /* Set 2 times bigger step against artifacts. */
    inst.template.YStep = 2;
    code = (*dev_proc(pdev, pattern_manage))((gx_device *)pdev, 
	id, &inst, pattern_manage__start_accum);
    if (code >= 0)
	pprintld1(pdev->strm, "/R%ld Do\n", pdf_resource_id(pres1));
    pres = pdev->accumulating_substream_resource;
    if (code >= 0)
	code = pdf_add_resource(pdev, pdev->substream_Resources, "/XObject", pres1);
    if (code >= 0)
	code = (*dev_proc(pdev, pattern_manage))((gx_device *)pdev, 
	    id, &inst, pattern_manage__finish_accum);
    if (code >= 0)
	code = (*dev_proc(pdev, pattern_manage))((gx_device *)pdev, 
	    id, &inst, pattern_manage__load);
    if (code >= 0) {
	stream_puts(pdev->strm, "q ");
	code = pdf_cs_Pattern_colored(pdev, &v);
    }
    if (code >= 0) {
	cos_value_write(&v, pdev);
	pprintld1(pdev->strm, " cs /R%ld scn ", pdf_resource_id(pres));
    }
    if (code >= 0) {
	/* The image offset weas broken in gx_begin_image3_generic, 
	   (see 'origin' in there). 
	   As a temporary hack use the offset of the image. 
	   fixme : This isn't generally correct, 
	   because the mask may be "transpozed" against the image. */
	gs_matrix m = pdev->converting_image_matrix;

	m.tx = pmat->tx;
	m.ty = pmat->ty;
	code = pdf_do_image_by_id(pdev, pdev->image_mask_scale,
	     &m, true, pdev->image_mask_id);
	stream_puts(pdev->strm, "Q\n");
    }
    return code;
}

typedef enum {
    USE_AS_MASK,
    USE_AS_IMAGE,
    USE_AS_PATTERN
} pdf_image_usage_t;

/* Close PDF image and do it. */
private int
pdf_end_and_do_image(gx_device_pdf *pdev, pdf_image_writer *piw, 
		     const gs_matrix *mat, gs_id ps_bitmap_id, pdf_image_usage_t do_image)
{
    int code = pdf_end_write_image(pdev, piw);
    pdf_resource_t *pres = piw->pres;

    switch (code) {
    default:
	return code;	/* error */
    case 1:
	code = 0;
	break;
    case 0:
	if (do_image == USE_AS_IMAGE) {
	    if (pdev->image_mask_id != gs_no_id) {
		char buf[20];

		sprintf(buf, "%ld 0 R", pdev->image_mask_id);
		code = cos_dict_put_string_copy((cos_dict_t *)pres->object, 
			pdev->image_mask_is_SMask ? "/SMask" : "/Mask", buf);
		if (code < 0)
		    return code;
	    }
	    if (pdev->image_mask_skip)
		code = 0;
	    else
		code = pdf_do_image(pdev, pres, mat, true);
	} else if (do_image == USE_AS_MASK) {
	    /* Provide data for pdf_do_image_by_id, which will be called through 
		use_image_as_pattern during the next call to this function. 
		See pdf_do_image about the meaning of 'scale'. */
	    const pdf_x_object_t *const pxo = (const pdf_x_object_t *)pres;

	    pdev->image_mask_scale = (double)pxo->data_height / pxo->height;
	    pdev->image_mask_id = pdf_resource_id(pres);
	    pdev->converting_image_matrix = *mat;
	} else if (do_image == USE_AS_PATTERN)
	    code = use_image_as_pattern(pdev, pres, mat, ps_bitmap_id);
    }
    return code;
}

/* Clean up by releasing the buffers. */
private int
pdf_image_end_image_data(gx_image_enum_common_t * info, bool draw_last,
			 pdf_image_usage_t do_image)
{
    gx_device_pdf *pdev = (gx_device_pdf *)info->dev;
    pdf_image_enum *pie = (pdf_image_enum *)info;
    int height = pie->writer.height;
    int data_height = height - pie->rows_left;
    int code = 0;

    if (pie->writer.pres)
	((pdf_x_object_t *)pie->writer.pres)->data_height = data_height;
    else if (data_height > 0)
	pdf_put_image_matrix(pdev, &pie->mat, (double)data_height / height);
    if (data_height > 0) {
	code = pdf_complete_image_data(pdev, &pie->writer, data_height,
			pie->width, pie->bits_per_pixel);
	if (code < 0)
	    return code;
	code = pdf_end_image_binary(pdev, &pie->writer, data_height);
	/* The call above possibly decreases pie->writer.alt_writer_count in 2. */
	if (code < 0)
 	    return code;
	if (pie->writer.alt_writer_count == 2) {
	    /* We're converting a type 4 image into an imagemask with a pattern color. */
	    /* Since the type 3 image writes the mask first, do so here. */
	    pdf_image_writer writer = pie->writer;

	    writer.binary[0] = pie->writer.binary[1];
	    writer.pres = pie->writer.pres_mask;
	    writer.alt_writer_count = 1;
	    memset(&pie->writer.binary[1], 0, sizeof(pie->writer.binary[1]));
	    pie->writer.alt_writer_count--; /* For GC. */
	    pie->writer.pres_mask = 0; /* For GC. */
	    code = pdf_end_image_binary(pdev, &writer, data_height);
	    if (code < 0)
 		return code;
	    code = pdf_end_and_do_image(pdev, &writer, &pie->mat, info->id, USE_AS_MASK);
	    if (code < 0)
 		return code;
	    code = pdf_end_and_do_image(pdev, &pie->writer, &pie->mat, info->id, USE_AS_PATTERN);
	} else
	    code = pdf_end_and_do_image(pdev, &pie->writer, &pie->mat, info->id, do_image);
	pie->writer.alt_writer_count--; /* For GC. */
    }
    gs_free_object(pie->memory, pie, "pdf_end_image");
    return code;
}

/* End a normal image, drawing it. */
private int
pdf_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    return pdf_image_end_image_data(info, draw_last, USE_AS_IMAGE);
}

/* End an image converted with pdf_lcvd_t. */
private int
pdf_image_end_image_cvd(gx_image_enum_common_t * info, bool draw_last)
{   pdf_lcvd_t *cvd = (pdf_lcvd_t *)info->dev;
    int code = pdf_dump_converted_image(cvd->pdev, cvd);
    int code1 = gx_image1_end_image(info, draw_last);
    int code2 = gs_closedevice((gx_device *)cvd->mask);
    int code3 = gs_closedevice((gx_device *)cvd);

    gs_free_object(cvd->mask->memory, (gx_device *)cvd->mask, "pdf_image_end_image_cvd");
    gs_free_object(cvd->mdev.memory, (gx_device *)cvd, "pdf_image_end_image_cvd");
    return code < 0 ? code : code1 < 0 ? code1 : code2 < 0 ? code2 : code3;
}
/* ---------------- Type 3/3x images ---------------- */

/*
 * For both types of masked images, we create temporary dummy (null) devices
 * that forward the begin_typed_image call to the implementation above.
 */
private int
pdf_make_mxd(gx_device **pmxdev, gx_device *tdev, gs_memory_t *mem)
{
    gx_device *fdev;
    int code = gs_copydevice(&fdev, (const gx_device *)&gs_null_device, mem);

    if (code < 0)
	return code;
    gx_device_set_target((gx_device_forward *)fdev, tdev);
    *pmxdev = fdev;
    return 0;
}

/* End the mask of an ImageType 3 image, not drawing it. */
private int
pdf_image_end_image_object(gx_image_enum_common_t * info, bool draw_last)
{
    return pdf_image_end_image_data(info, draw_last, USE_AS_MASK);
}
/* End the data of an ImageType 3 image, converting it into pattern. */
private int
pdf_image_end_image_object2(gx_image_enum_common_t * info, bool draw_last)
{
    return pdf_image_end_image_data(info, draw_last, USE_AS_PATTERN);
}

/* ---------------- Type 3 images ---------------- */

/* Implement the mask image device. */
private dev_proc_begin_typed_image(pdf_mid_begin_typed_image);
private int
pdf_image3_make_mid(gx_device **pmidev, gx_device *dev, int width, int height,
		    gs_memory_t *mem)
{
    gx_device_pdf *pdev = (gx_device_pdf *)dev;

    if (pdev->CompatibilityLevel < 1.3 && !pdev->PatternImagemask) {
	gs_matrix m;
	pdf_lcvd_t *cvd = NULL;
	int code;
        
	gs_make_identity(&m);
	code = pdf_setup_masked_image_converter(pdev, mem, &m, &cvd, 
					true, 0, 0, width, height, true);
	if (code < 0)
	    return code;
	cvd->mask->target = (gx_device *)cvd; /* Temporary, just to communicate with 
					 pdf_image3_make_mcde. The latter will reset it. */
	cvd->mask_is_empty = false;
	*pmidev = (gx_device *)cvd->mask;
	return 0;
    } else {
	int code = pdf_make_mxd(pmidev, dev, mem);

	if (code < 0)
	    return code;
	set_dev_proc(*pmidev, begin_typed_image, pdf_mid_begin_typed_image);
	return 0;
    }
}
private int
pdf_mid_begin_typed_image(gx_device * dev, const gs_imager_state * pis,
			  const gs_matrix *pmat, const gs_image_common_t *pic,
			  const gs_int_rect * prect,
			  const gx_drawing_color * pdcolor,
			  const gx_clip_path * pcpath, gs_memory_t * mem,
			  gx_image_enum_common_t ** pinfo)
{
    /* The target of the null device is the pdfwrite device. */
    gx_device_pdf *const pdev = (gx_device_pdf *)
	((gx_device_null *)dev)->target;
    return pdf_begin_typed_image
	(pdev, pis, pmat, pic, prect, pdcolor, pcpath, mem, pinfo,
	 PDF_IMAGE_TYPE3_MASK);
}

/* Implement the mask clip device. */
private int
pdf_image3_make_mcde(gx_device *dev, const gs_imager_state *pis,
		     const gs_matrix *pmat, const gs_image_common_t *pic,
		     const gs_int_rect *prect, const gx_drawing_color *pdcolor,
		     const gx_clip_path *pcpath, gs_memory_t *mem,
		     gx_image_enum_common_t **pinfo,
		     gx_device **pmcdev, gx_device *midev,
		     gx_image_enum_common_t *pminfo,
		     const gs_int_point *origin)
{
    int code;
    gx_device_pdf *pdev = (gx_device_pdf *)dev;

    if (pdev->CompatibilityLevel < 1.3 && !pdev->PatternImagemask) {
	/* pdf_image3_make_mid must set midev with a pdf_lcvd_t instance.*/
	pdf_lcvd_t *cvd = (pdf_lcvd_t *)((gx_device_memory *)midev)->target; 

	((gx_device_memory *)midev)->target = NULL;
	cvd->m = pdev->converting_image_matrix;
	cvd->mdev.mapped_x = origin->x;
	cvd->mdev.mapped_y = origin->y;
	*pmcdev = (gx_device *)&cvd->mdev;
	code = gx_default_begin_typed_image
	    ((gx_device *)&cvd->mdev, pis, pmat, pic, prect, pdcolor, pcpath, mem,
	    pinfo);
    } else {
	code = pdf_make_mxd(pmcdev, midev, mem);
	if (code < 0)
	    return code;
	code = pdf_begin_typed_image
	    ((gx_device_pdf *)dev, pis, pmat, pic, prect, pdcolor, pcpath, mem,
	    pinfo, PDF_IMAGE_TYPE3_DATA);
    }
    /* Due to equal image merging, we delay the adding of the "Mask" entry into 
       a type 3 image dictionary until the mask is completed. 
       Will do in pdf_end_and_do_image.*/
    return 0;
}

/* ---------------- Type 3x images ---------------- */

/* Implement the mask image device. */
private int
pdf_image3x_make_mid(gx_device **pmidev, gx_device *dev, int width, int height,
		     int depth, gs_memory_t *mem)
{
    int code = pdf_make_mxd(pmidev, dev, mem);

    if (code < 0)
	return code;
    set_dev_proc(*pmidev, begin_typed_image, pdf_mid_begin_typed_image);
    return 0;
}

/* Implement the mask clip device. */
private int
pdf_image3x_make_mcde(gx_device *dev, const gs_imager_state *pis,
		      const gs_matrix *pmat, const gs_image_common_t *pic,
		      const gs_int_rect *prect,
		      const gx_drawing_color *pdcolor,
		      const gx_clip_path *pcpath, gs_memory_t *mem,
		      gx_image_enum_common_t **pinfo,
		      gx_device **pmcdev, gx_device *midev[2],
		      gx_image_enum_common_t *pminfo[2],
		      const gs_int_point origin[2],
		      const gs_image3x_t *pim)
{
    int code;
    pdf_image_enum *pmie;
    pdf_image_enum *pmce;
    cos_stream_t *pmcs;
    int i;
    const gs_image3x_mask_t *pixm;

    if (midev[0]) {
	if (midev[1])
	    return_error(gs_error_rangecheck);
	i = 0, pixm = &pim->Opacity;
    } else if (midev[1])
	i = 1, pixm = &pim->Shape;
    else
	return_error(gs_error_rangecheck);
    code = pdf_make_mxd(pmcdev, midev[i], mem);
    if (code < 0)
	return code;
    code = pdf_begin_typed_image
	((gx_device_pdf *)dev, pis, pmat, pic, prect, pdcolor, pcpath, mem,
	 pinfo, PDF_IMAGE_TYPE3_DATA);
    if (code < 0)
	return code;
    if ((*pinfo)->procs != &pdf_image_enum_procs) {
	/* We couldn't handle the image.  Bail out. */
	gx_image_end(*pinfo, false);
	gs_free_object(mem, *pmcdev, "pdf_image3x_make_mcde");
	return_error(gs_error_rangecheck);
    }
    pmie = (pdf_image_enum *)pminfo[i];
    pmce = (pdf_image_enum *)(*pinfo);
    pmcs = (cos_stream_t *)pmce->writer.pres->object;
    /*
     * Add the SMask entry to the image dictionary, and, if needed,
     * the Matte entry to the mask dictionary.
     */
    if (pixm->has_Matte) {
	int num_components =
	    gs_color_space_num_components(pim->ColorSpace);

	code = cos_dict_put_c_key_floats(
				(cos_dict_t *)pmie->writer.pres->object,
				"/Matte", pixm->Matte,
				num_components);
	if (code < 0)
	    return code;
    }
    return cos_dict_put_c_key_object(cos_stream_dict(pmcs), "/SMask",
				     pmie->writer.pres->object);
}

pdf_resource_t *pdf_substitute_pattern(pdf_resource_t *pres)
{
    pdf_pattern_t *ppat = (pdf_pattern_t *)pres;
    
    return (pdf_resource_t *)(ppat->substitute != 0 ? ppat->substitute : ppat);
}


private int 
check_unsubstituted2(gx_device_pdf * pdev, pdf_resource_t *pres0, pdf_resource_t *pres1)
{
    pdf_pattern_t *ppat = (pdf_pattern_t *)pres0;

    return ppat->substitute == NULL;
}

private int 
check_unsubstituted1(gx_device_pdf * pdev, pdf_resource_t *pres0)
{
    pdf_pattern_t *ppat = (pdf_pattern_t *)pres0;

    return ppat->substitute != NULL;
}

/*
   The pattern management device method.
   See gxdevcli.h about return codes.
 */
int
gdev_pdf_pattern_manage(gx_device *pdev1, gx_bitmap_id id,
		gs_pattern1_instance_t *pinst, pattern_manage_t function)
{   
    gx_device_pdf *pdev = (gx_device_pdf *)pdev1;
    int code;
    pdf_resource_t *pres, *pres1;

    switch (function) {
	case pattern_manage__can_accum:
	    return 1;
	case pattern_manage__start_accum:
	    code = pdf_enter_substream(pdev, resourcePattern, id, &pres, false, 
		    pdev->CompressFonts/* Have no better switch.*/);
	    if (code < 0)
		return code;
	    pres->rid = id;
	    code = pdf_store_pattern1_params(pdev, pres, pinst);
	    if (code < 0)
		return code;
	    return 1;
	case pattern_manage__finish_accum:
	    code = pdf_add_procsets(pdev->substream_Resources, pdev->procsets);
	    if (code < 0)
		return code;
	    pres = pres1 = pdev->accumulating_substream_resource;
	    code = pdf_exit_substream(pdev);
	    if (code < 0)
		return code;
	    if (pdev->substituted_pattern_count > 300 && 
		    pdev->substituted_pattern_drop_page != pdev->next_page) { /* arbitrary */
		pdf_drop_resources(pdev, resourcePattern, check_unsubstituted1);
		pdev->substituted_pattern_count = 0;
		pdev->substituted_pattern_drop_page = pdev->next_page;
	    }
	    code = pdf_find_same_resource(pdev, resourcePattern, &pres, check_unsubstituted2);
	    if (code < 0)
		return code;
	    if (code > 0) {
		pdf_pattern_t *ppat = (pdf_pattern_t *)pres1;

		code = pdf_cancel_resource(pdev, pres1, resourcePattern);
		if (code < 0)
		    return code;
		/* Do not remove pres1, because it keeps the substitution. */
		ppat->substitute = (pdf_pattern_t *)pres;
		pres->where_used |= pdev->used_mask;
		pdev->substituted_pattern_count++;
	    } else if (pres->object->id < 0)
		pdf_reserve_object_id(pdev, pres, 0);
	    return 1;
	case pattern_manage__load:
	    pres = pdf_find_resource_by_gs_id(pdev, resourcePattern, id);
	    if (pres == 0)
		return gs_error_undefined;
	    pres = pdf_substitute_pattern(pres);
	    code = pdf_add_resource(pdev, pdev->substream_Resources, "/Pattern", pres);
	    if (code < 0)
		return code;
	    return 1;
	case pattern_manage__shading_area:
	    return 0;
    }
    return_error(gs_error_unregistered);
}

