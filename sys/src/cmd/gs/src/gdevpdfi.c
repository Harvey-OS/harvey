/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfi.c,v 1.20 2000/09/19 19:00:17 lpd Exp $ */
/* Image handling for PDF-writing driver */
#include "gx.h"
#include "gserrors.h"
#include "gsdevice.h"
#include "gsflip.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"		/* for data stream */
#include "gxcspace.h"
#include "gximage3.h"
#include "gximag3x.h"
#include "gsiparm4.h"

/* Forward references */
private image_enum_proc_plane_data(pdf_image_plane_data);
private image_enum_proc_end_image(pdf_image_end_image);
private image_enum_proc_end_image(pdf_image_end_image_object);
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
 * Test whether we can write an image in-line.  Before PDF 1.2, this is only
 * allowed for masks, images in built-in color spaces, and images in Indexed
 * color spaces based on these with a string lookup table.
 */
private bool
can_write_image_in_line(const gx_device_pdf *pdev, const gs_image_t *pim)
{
    const gs_color_space *pcs;

    if (pim->ImageMask)
	return true;
    if (pdev->CompatibilityLevel >= 1.2)
	return true;
    pcs = pim->ColorSpace;
 cs:
    switch (gs_color_space_get_index(pcs)) {
    case gs_color_space_index_DeviceGray:
    case gs_color_space_index_DeviceRGB:
    case gs_color_space_index_DeviceCMYK:
	return true;
    case gs_color_space_index_Indexed:
	if (pcs->params.indexed.use_proc)
	    return false;
	pcs = (const gs_color_space *)&pcs->params.indexed.base_space;
	goto cs;
    default:
	return false;
    }
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
    const gs_pixel_image_t *pim;
    int code;
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
    } image;
    ulong nbytes;
    int width, height;

    /* Check for the image types we can handle. */
    switch (pic->type->index) {
    case 1: {
	const gs_image_t *pim1 = (const gs_image_t *)pic;

	if (pim1->Alpha != gs_image_alpha_none)
	    goto nyi;
	is_mask = pim1->ImageMask;
	in_line = context == PDF_IMAGE_DEFAULT &&
	    can_write_image_in_line(pdev, pim1);
	image.type1 = *pim1;
	break;
    }
    case 3: {
	const gs_image3_t *pim3 = (const gs_image3_t *)pic;

	if (pdev->CompatibilityLevel < 1.3)
	    goto nyi;
	if (prect && !(prect->p.x == 0 && prect->p.y == 0 &&
		       prect->q.x == pim3->Width &&
		       prect->q.y == pim3->Height))
	    goto nyi;
	/*
	 * We handle ImageType 3 images in a completely different way:
	 * the default implementation sets up the enumerator.
	 */
	return gx_begin_image3_generic((gx_device *)pdev, pis, pmat, pic,
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
	return gx_begin_image3x_generic((gx_device *)pdev, pis, pmat, pic,
					prect, pdcolor, pcpath, mem,
					pdf_image3x_make_mid,
					pdf_image3x_make_mcde, pinfo);
    }
    case 4:
	if (pdev->CompatibilityLevel < 1.3)
	    goto nyi;
	image.type4 = *(const gs_image4_t *)pic;
	break;
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
    pcs = pim->ColorSpace;
    num_components = (is_mask ? 1 : gs_color_space_num_components(pcs));

    code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    if (context == PDF_IMAGE_TYPE3_MASK) {
	/*
	 * The soft mask for an ImageType 3x image uses a DevicePixel
	 * color space, which pdf_color_space() can't handle.  Patch it
	 * to DeviceGray here.
	 */
	gs_cspace_init_DeviceGray(&cs_gray_temp);
	pcs = &cs_gray_temp;
    } else if (is_mask)
	code = pdf_prepare_imagemask(pdev, pis, pdcolor);
    else
	code = pdf_prepare_image(pdev, pis);
    if (code < 0)
	goto nyi;
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
    *pinfo = (gx_image_enum_common_t *) pie;
    gx_image_enum_common_init(*pinfo, (const gs_data_image_t *) pim,
			      (context == PDF_IMAGE_TYPE3_MASK ?
			       &pdf_image_object_enum_procs :
			       &pdf_image_enum_procs),
			      (gx_device *)pdev, num_components, format);
    pie->memory = mem;
    width = rect.q.x - rect.p.x;
    pie->width = width;
    height = rect.q.y - rect.p.y;
    pie->bits_per_pixel =
	pim->BitsPerComponent * num_components / pie->num_planes;
    pie->rows_left = height;
    nbytes = (((ulong) pie->width * pie->bits_per_pixel + 7) >> 3) *
	pie->num_planes * pie->rows_left;
    in_line &= nbytes <= MAX_INLINE_IMAGE_BYTES;
    if (rect.p.x != 0 || rect.p.y != 0 ||
	rect.q.x != pim->Width || rect.q.y != pim->Height ||
	(is_mask ? pim->CombineWithColor :
	 pdf_color_space(pdev, &cs_value, pcs,
			 (in_line ? &pdf_color_space_names_short :
			  &pdf_color_space_names), in_line) < 0)
	) {
	gs_free_object(mem, pie, "pdf_begin_image");
	goto nyi;
    }
    pdf_put_clip_path(pdev, pcpath);
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
    }
    if ((code = pdf_begin_write_image(pdev, &pie->writer, gs_no_id, width,
				      height, NULL, in_line)) < 0 ||
	/****** pctm IS WRONG ******/
	(code = psdf_setup_image_filters((gx_device_psdf *) pdev,
					 &pie->writer.binary, &image.pixel,
					 pmat, pis)) < 0 ||
	(code = pdf_begin_image_data(pdev, &pie->writer,
				     (const gs_pixel_image_t *)&image,
				     &cs_value)) < 0
	)
	return code;
    return 0;
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
pdf_image_plane_data(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used)
{
    gx_device_pdf *pdev = (gx_device_pdf *)info->dev;
    pdf_image_enum *pie = (pdf_image_enum *) info;
    int h = height;
    int y;
    /****** DOESN'T HANDLE IMAGES WITH VARYING WIDTH PER PLANE ******/
    uint width_bits = pie->width * pie->plane_depths[0];
    /****** DOESN'T HANDLE NON-ZERO data_x CORRECTLY ******/
    uint bcount = (width_bits + 7) >> 3;
    uint ignore;
    int nplanes = pie->num_planes;
    stream *s = pdev->streams.strm;
    long pos = stell(s);
    int code;
    int status = 0;

    if (h > pie->rows_left)
	h = pie->rows_left;
    pie->rows_left -= h;
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
		status = sputs(pie->writer.binary.strm, row, flipped_count,
			       &ignore);
		if (status < 0)
		    break;
		offset += flip_count;
		count -= flip_count;
	    }
	} else {
	    status = sputs(pie->writer.binary.strm,
			   planes[0].data + planes[0].raster * y, bcount,
			   &ignore);
	}
	if (status < 0)
	    break;
    }
    *rows_used = h;
    if (status < 0)
	return_error(gs_error_ioerror);
    code = cos_stream_add_since(pie->writer.data, pos);
    return (code < 0 ? code : !pie->rows_left);
#undef ROW_BYTES
}

/* Clean up by releasing the buffers. */
private int
pdf_image_end_image_data(gx_image_enum_common_t * info, bool draw_last,
			 bool do_image)
{
    gx_device_pdf *pdev = (gx_device_pdf *)info->dev;
    pdf_image_enum *pie = (pdf_image_enum *)info;
    int height = pie->writer.height;
    int data_height = height - pie->rows_left;
    int code;

    if (pie->writer.pres)
	((pdf_x_object_t *)pie->writer.pres)->data_height = data_height;
    else
	pdf_put_image_matrix(pdev, &pie->mat,
			     (height == 0 || data_height == 0 ? 1.0 :
			      (double)data_height / height));
    code = pdf_end_image_binary(pdev, &pie->writer, data_height);
    if (code < 0)
	return code;
    code = pdf_end_write_image(pdev, &pie->writer);
    switch (code) {
    default:
	return code;	/* error */
    case 1:
	code = 0;
	break;
    case 0:
	if (do_image)
	    code = pdf_do_image(pdev, pie->writer.pres, &pie->mat, true);
    }
    gs_free_object(pie->memory, pie, "pdf_end_image");
    return code;
}

/* End a normal image, drawing it. */
private int
pdf_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    return pdf_image_end_image_data(info, draw_last, true);
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
    return pdf_image_end_image_data(info, draw_last, false);
}

/* ---------------- Type 3 images ---------------- */

/* Implement the mask image device. */
private dev_proc_begin_typed_image(pdf_mid_begin_typed_image);
private int
pdf_image3_make_mid(gx_device **pmidev, gx_device *dev, int width, int height,
		    gs_memory_t *mem)
{
    int code = pdf_make_mxd(pmidev, dev, mem);

    if (code < 0)
	return code;
    set_dev_proc(*pmidev, begin_typed_image, pdf_mid_begin_typed_image);
    return 0;
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
    int code = pdf_begin_typed_image
	(pdev, pis, pmat, pic, prect, pdcolor, pcpath, mem, pinfo,
	 PDF_IMAGE_TYPE3_MASK);

    if (code < 0)
	return code;
    if ((*pinfo)->procs != &pdf_image_object_enum_procs) {
	/* We couldn't handle the mask image.  Bail out. */
	/* (This is never supposed to happen.) */
	return_error(gs_error_rangecheck);
    }
    return code;
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
    int code = pdf_make_mxd(pmcdev, midev, mem);
    pdf_image_enum *pmie;
    pdf_image_enum *pmce;
    cos_stream_t *pmcs;

    if (code < 0)
	return code;
    code = pdf_begin_typed_image
	((gx_device_pdf *)dev, pis, pmat, pic, prect, pdcolor, pcpath, mem,
	 pinfo, PDF_IMAGE_TYPE3_DATA);
    if (code < 0)
	return code;
    /* Add the /Mask entry to the image dictionary. */
    if ((*pinfo)->procs != &pdf_image_enum_procs) {
	/* We couldn't handle the image.  Bail out. */
	gx_image_end(*pinfo, false);
	gs_free_object(mem, *pmcdev, "pdf_image3_make_mcde");
	return_error(gs_error_rangecheck);
    }
    pmie = (pdf_image_enum *)pminfo;
    pmce = (pdf_image_enum *)(*pinfo);
    pmcs = (cos_stream_t *)pmce->writer.pres->object;
    return cos_dict_put_c_key_object(cos_stream_dict(pmcs), "/Mask",
				     pmie->writer.pres->object);
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
