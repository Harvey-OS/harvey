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

/*$Id: gdevpdfj.c,v 1.5 2000/09/19 19:00:17 lpd Exp $ */
/* Image-writing utilities for pdfwrite driver */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"
#include "gxcspace.h"
#include "gsiparm4.h"

#define CHECK(expr)\
  BEGIN if ((code = (expr)) < 0) return code; END

/* GC descriptors */
public_st_pdf_image_writer();

/* ---------------- Image stream dictionaries ---------------- */

const pdf_image_names_t pdf_image_names_full = {
    { PDF_COLOR_SPACE_NAMES },
    { PDF_FILTER_NAMES },
    PDF_IMAGE_PARAM_NAMES
};
const pdf_image_names_t pdf_image_names_short = {
    { PDF_COLOR_SPACE_NAMES_SHORT },
    { PDF_FILTER_NAMES_SHORT },
    PDF_IMAGE_PARAM_NAMES_SHORT
};

/* Store the values of image parameters other than filters. */
/* pdev is used only for updating procsets. */
/* pcsvalue is not used for masks. */
private int
pdf_put_pixel_image_values(cos_dict_t *pcd, gx_device_pdf *pdev,
			   const gs_pixel_image_t *pim,
			   const gs_color_space *pcs,
			   const pdf_image_names_t *pin,
			   const cos_value_t *pcsvalue)
{
    int num_components;
    float indexed_decode[2];
    const float *default_decode = NULL;
    int code;

    if (pcs) {
	CHECK(cos_dict_put_c_key(pcd, pin->ColorSpace, pcsvalue));
	pdf_color_space_procsets(pdev, pcs);
	num_components = gs_color_space_num_components(pcs);
	if (gs_color_space_get_index(pcs) == gs_color_space_index_Indexed) {
	    indexed_decode[0] = 0;
	    indexed_decode[1] = (1 << pim->BitsPerComponent) - 1;
	    default_decode = indexed_decode;
	}
    } else
	num_components = 1;
    CHECK(cos_dict_put_c_key_int(pcd, pin->Width, pim->Width));
    CHECK(cos_dict_put_c_key_int(pcd, pin->Height, pim->Height));
    CHECK(cos_dict_put_c_key_int(pcd, pin->BitsPerComponent,
				 pim->BitsPerComponent));
    {
	int i;

	for (i = 0; i < num_components * 2; ++i)
	    if (pim->Decode[i] !=
		(default_decode ? default_decode[i] : i & 1)
		)
		break;
	if (i < num_components * 2) {
	    cos_array_t *pca =
		cos_array_alloc(pdev, "pdf_put_pixel_image_values(decode)");

	    if (pca == 0)
		return_error(gs_error_VMerror);
	    for (i = 0; i < num_components * 2; ++i)
		CHECK(cos_array_add_real(pca, pim->Decode[i]));
	    CHECK(cos_dict_put_c_key_object(pcd, pin->Decode,
					    COS_OBJECT(pca)));
	}
    }
    if (pim->Interpolate)
	CHECK(cos_dict_put_c_strings(pcd, pin->Interpolate, "true"));
    return 0;
}
int
pdf_put_image_values(cos_dict_t *pcd, gx_device_pdf *pdev,
		     const gs_pixel_image_t *pic,
		     const pdf_image_names_t *pin,
		     const cos_value_t *pcsvalue)
{
    const gs_color_space *pcs = pic->ColorSpace;
    int code;

    switch (pic->type->index) {
    case 1: {
	const gs_image1_t *pim = (const gs_image1_t *)pic;

	if (pim->ImageMask) {
	    CHECK(cos_dict_put_c_strings(pcd, pin->ImageMask, "true"));
	    pdev->procsets |= ImageB;
	    pcs = NULL;
	}
    }
	break;
    case 3: {
	/*
	 * Clients must treat this as a special case: they must call
	 * pdf_put_image_values for the MaskDict separately, and must
	 * add the Mask entry to the main image stream (dictionary).
	 */
	/*const gs_image3_t *pim = (const gs_image3_t *)pic;*/
    }
	break;
    case 4: {
	const gs_image4_t *pim = (const gs_image4_t *)pic;
	int num_components = gs_color_space_num_components(pcs);
	cos_array_t *pca =
	    cos_array_alloc(pdev, "pdf_put_image_values(mask)");
	int i;
    
	if (pca == 0)
	    return_error(gs_error_VMerror);
	for (i = 0; i < num_components; ++i) {
	    int lo, hi;

	    if (pim->MaskColor_is_range)
		lo = pim->MaskColor[i * 2], hi = pim->MaskColor[i * 2 + 1];
	    else
		lo = hi = pim->MaskColor[i];
	    CHECK(cos_array_add_int(pca, lo));
	    CHECK(cos_array_add_int(pca, hi));
	}
	CHECK(cos_dict_put_c_key_object(pcd, "/Mask", COS_OBJECT(pca)));
    }
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    return pdf_put_pixel_image_values(pcd, pdev, pic, pcs, pin, pcsvalue);
}

/* Store filters for an image. */
/* Currently this only saves parameters for CCITTFaxDecode. */
int
pdf_put_image_filters(cos_dict_t *pcd, gx_device_pdf *pdev,
		      const psdf_binary_writer * pbw,
		      const pdf_image_names_t *pin)
{
    return pdf_put_filters(pcd, pdev, pbw->strm, &pin->filter_names);
}

/* ---------------- Image writing ---------------- */

/*
 * Fill in the image parameters for a device space bitmap.
 * PDF images are always specified top-to-bottom.
 * data_h is the actual number of data rows, which may be less than h.
 */
void
pdf_make_bitmap_matrix(gs_matrix * pmat, int x, int y, int w, int h,
		       int h_actual)
{
    pmat->xx = w;
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = -h_actual;
    pmat->tx = x;
    pmat->ty = y + h;
}

/*
 * Put out the gsave and matrix for an image.  y_scale adjusts the matrix
 * for images that end prematurely.
 */
void
pdf_put_image_matrix(gx_device_pdf * pdev, const gs_matrix * pmat,
		     floatp y_scale)
{
    gs_matrix imat;

    gs_matrix_translate(pmat, 0.0, 1.0 - y_scale, &imat);
    gs_matrix_scale(&imat, 1.0, y_scale, &imat);
    pdf_put_matrix(pdev, "q ", &imat, "cm\n");
}

/* Put out a reference to an image resource. */
int
pdf_do_image(gx_device_pdf * pdev, const pdf_resource_t * pres,
	     const gs_matrix * pimat, bool in_contents)
{
    if (in_contents) {
	int code = pdf_open_contents(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
    }
    if (pimat) {
	/* Adjust the matrix to account for short images. */
	const pdf_x_object_t *const pxo = (const pdf_x_object_t *)pres;
	double scale = (double)pxo->data_height / pxo->height;

	pdf_put_image_matrix(pdev, pimat, scale);
    }
    pprintld1(pdev->strm, "/R%ld Do\nQ\n", pdf_resource_id(pres));
    return 0;
}

/* ------ Begin / finish ------ */

/*
 * Begin writing an image, creating the resource if not in-line and
 * pres == 0, and setting up the binary writer.
 */
int
pdf_begin_write_image(gx_device_pdf * pdev, pdf_image_writer * piw,
		      gx_bitmap_id id, int w, int h, pdf_resource_t *pres,
		      bool in_line)
{
    /* Patch pdev->strm so the right stream gets into the writer. */
    stream *save_strm = pdev->strm;
    int code;

    if (in_line) {
	piw->pres = 0;
	piw->pin = &pdf_image_names_short;
	piw->data = cos_stream_alloc(pdev, "pdf_begin_image_data");
	if (piw->data == 0)
	    return_error(gs_error_VMerror);
	piw->end_string = " Q";
    } else {
	pdf_x_object_t *pxo;
	cos_stream_t *pcos;

	if (pres == 0) {
	    code = pdf_alloc_resource(pdev, resourceXObject, id, &piw->pres,
				      0L);
	    if (code < 0)
		return code;
	    cos_become(piw->pres->object, cos_type_stream);
	} else {
	    /* Resource already allocated (Mask for ImageType 3 image). */
	    piw->pres = pres;
	}
	piw->pres->rid = id;
	piw->pin = &pdf_image_names_full;
	pxo = (pdf_x_object_t *)piw->pres;
	pcos = (cos_stream_t *)pxo->object;
	CHECK(cos_dict_put_c_strings(cos_stream_dict(pcos), "/Subtype",
				     "/Image"));
	pxo->width = w;
	pxo->height = h;
	/* Initialize data_height for the benefit of copy_{mono,color}. */
	pxo->data_height = h;
	piw->data = pcos;
    }
    piw->height = h;
    pdev->strm = pdev->streams.strm;
    code = psdf_begin_binary((gx_device_psdf *) pdev, &piw->binary);
    pdev->strm = save_strm;
    return code;
}

/* Begin writing the image data, setting up the dictionary and filters. */
int
pdf_begin_image_data(gx_device_pdf * pdev, pdf_image_writer * piw,
		     const gs_pixel_image_t * pim, const cos_value_t *pcsvalue)
{
    cos_dict_t *pcd = cos_stream_dict(piw->data);
    int code = pdf_put_image_values(pcd, pdev, pim, piw->pin, pcsvalue);

    if (code >= 0)
	code = pdf_put_image_filters(pcd, pdev, &piw->binary, piw->pin);
    if (code < 0) {
	if (!piw->pres)
	    COS_FREE(piw->data, "pdf_begin_image_data");
	piw->data = 0;
    }
    return code;
}

/* Finish writing the binary image data. */
int
pdf_end_image_binary(gx_device_pdf *pdev, pdf_image_writer *piw, int data_h)
{
    long pos = stell(pdev->streams.strm);  /* piw->binary.target */
    int code;

    psdf_end_binary(&piw->binary);
    code = cos_stream_add_since(piw->data, pos);
    if (code < 0)
	return code;
    /* If the image ended prematurely, update the Height. */
    if (data_h != piw->height)
	code = cos_dict_put_c_key_int(cos_stream_dict(piw->data),
				      piw->pin->Height, data_h);
    return code;
}

/*
 * Finish writing an image.  If in-line, write the BI/dict/ID/data/EI and
 * return 1; if a resource, write the resource definition and return 0.
 */
int
pdf_end_write_image(gx_device_pdf * pdev, pdf_image_writer * piw)
{
    pdf_resource_t *pres = piw->pres;

    if (pres) {			/* image resource */
	if (!pres->named) {	/* named objects are written at the end */
	    cos_write_object(pres->object, pdev);
	    cos_release(pres->object, "pdf_end_write_image");
	}
	return 0;
    } else {			/* in-line image */
	stream *s = pdev->strm;

	pputs(s, "BI\n");
	cos_stream_elements_write(piw->data, pdev);
	pputs(s, (pdev->binary_ok ? "ID " : "ID\n"));
	cos_stream_contents_write(piw->data, pdev);
	pprints1(s, "\nEI%s\n", piw->end_string);
	COS_FREE(piw->data, "pdf_end_write_image");
	return 1;
    }
}

/* ------ Copy data ------ */

/* Copy the data for a mask or monobit bitmap. */
int
pdf_copy_mask_bits(stream *s, const byte *base, int sourcex, int raster,
		   int w, int h, byte invert)
{
    int yi;

    for (yi = 0; yi < h; ++yi) {
	const byte *data = base + yi * raster + (sourcex >> 3);
	int sbit = sourcex & 7;

	if (sbit == 0) {
	    int nbytes = (w + 7) >> 3;
	    int i;

	    for (i = 0; i < nbytes; ++data, ++i)
		sputc(s, *data ^ invert);
	} else {
	    int wleft = w;
	    int rbit = 8 - sbit;

	    for (; wleft + sbit > 8; ++data, wleft -= 8)
		sputc(s, ((*data << sbit) + (data[1] >> rbit)) ^ invert);
	    if (wleft > 0)
		sputc(s, ((*data << sbit) ^ invert) &
		      (byte) (0xff00 >> wleft));
	}
    }
    return 0;
}

/* Copy the data for a colored image (device pixels). */
int
pdf_copy_color_bits(stream *s, const byte *base, int sourcex, int raster,
		    int w, int h, int bytes_per_pixel)
{
    int yi;

    for (yi = 0; yi < h; ++yi) {
	uint ignore;

	sputs(s, base + sourcex * bytes_per_pixel + yi * raster,
	      w * bytes_per_pixel, &ignore);
    }
    return 0;
}
