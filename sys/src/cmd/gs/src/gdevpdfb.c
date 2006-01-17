/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfb.c,v 1.34 2005/10/10 19:09:30 leonardo Exp $ */
/* Low-level bitmap image handling for PDF-writing driver */
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"		/* for data stream */
#include "gxcspace.h"
#include "gxdcolor.h"
#include "gxpcolor.h"
#include "gxhldevc.h"

/* We need this color space type for constructing temporary color spaces. */
extern const gs_color_space_type gs_color_space_type_Indexed;

/* ---------------- Utilities ---------------- */

/* Fill in the image parameters for a bitmap image. */
private void
pdf_make_bitmap_image(gs_image_t * pim, int x, int y, int w, int h)
{
    pim->Width = w;
    pim->Height = h;
    pdf_make_bitmap_matrix(&pim->ImageMatrix, x, y, w, h, h);
}

/* ---------------- Driver procedures ---------------- */

/* Copy a mask bitmap.  for_pattern = -1 means put the image in-line, */
/* 1 means put the image in a resource. */
private int
pdf_copy_mask_data(gx_device_pdf * pdev, const byte * base, int sourcex,
		   int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gs_image_t *pim, pdf_image_writer *piw,
		   int for_pattern)
{
    ulong nbytes;
    int code;
    const byte *row_base;
    int row_step;
    bool in_line;

    gs_image_t_init_mask(pim, true);
    pdf_make_bitmap_image(pim, x, y, w, h);
    nbytes = ((ulong)w * h + 7) / 8;

    if (for_pattern) {
	/*
	 * Patterns must be emitted in order of increasing user Y, i.e.,
	 * the opposite of PDF's standard image order.
	 */
	row_base = base + (h - 1) * raster;
	row_step = -raster;
	in_line = for_pattern < 0;
    } else {
	row_base = base;
	row_step = raster;
	in_line = nbytes < pdev->MaxInlineImageSize;
	pdf_put_image_matrix(pdev, &pim->ImageMatrix, 1.0);
	/*
	 * Check whether we've already made an XObject resource for this
	 * image.
	 */
	if (id != gx_no_bitmap_id) {
	    piw->pres = pdf_find_resource_by_gs_id(pdev, resourceXObject, id);
	    if (piw->pres)
		return 0;
	}
    }
    /*
     * We have to be able to control whether to put Pattern images in line,
     * to avoid trying to create an XObject resource while we're in the
     * middle of writing a Pattern resource.
     */
    if (for_pattern < 0)
	stream_puts(pdev->strm, "q ");
    pdf_image_writer_init(piw);
    if ((code = pdf_begin_write_image(pdev, piw, id, w, h, NULL, in_line)) < 0 ||
	(code = psdf_setup_lossless_filters((gx_device_psdf *) pdev,
					    &piw->binary[0],
					    (gs_pixel_image_t *)pim)) < 0 ||
	(code = pdf_begin_image_data(pdev, piw, (const gs_pixel_image_t *)pim,
				     NULL, 0)) < 0
	)
	return code;
    pdf_copy_mask_bits(piw->binary[0].strm, row_base, sourcex, row_step, w, h, 0);
    pdf_end_image_binary(pdev, piw, piw->height);
    return pdf_end_write_image(pdev, piw);
}

private void 
set_image_color(gx_device_pdf *pdev, gx_color_index c)
{
    pdf_set_pure_color(pdev, c, &pdev->saved_fill_color,
			&pdev->fill_used_process_color,
			&psdf_set_fill_color_commands);
    if (!pdev->HaveStrokeColor)
	pdf_set_pure_color(pdev, c, &pdev->saved_stroke_color,
			    &pdev->stroke_used_process_color,
			    &psdf_set_stroke_color_commands);
}

/* Copy a monochrome bitmap or mask. */
private int
pdf_copy_mono(gx_device_pdf *pdev,
	      const byte *base, int sourcex, int raster, gx_bitmap_id id,
	      int x, int y, int w, int h, gx_color_index zero,
	      gx_color_index one, const gx_clip_path *pcpath)
{
    int code;
    gs_color_space cs;
    cos_value_t cs_value;
    cos_value_t *pcsvalue;
    byte palette[arch_sizeof_color_index * 2];
    gs_image_t image;
    pdf_image_writer writer;
    pdf_stream_position_t ipos;
    pdf_resource_t *pres = 0;
    byte invert = 0;
    bool in_line = false;

    /* Update clipping. */
    if (pdf_must_put_clip_path(pdev, pcpath)) {
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
	code = pdf_put_clip_path(pdev, pcpath);
	if (code < 0)
	    return code;
    }
    /* We have 3 cases: mask, inverse mask, and solid. */
    if (zero == gx_no_color_index) {
	if (one == gx_no_color_index)
	    return 0;
	/* If a mask has an id, assume it's a character. */
	if (id != gx_no_bitmap_id && sourcex == 0) {
	    pres = pdf_find_resource_by_gs_id(pdev, resourceCharProc, id);
	    if (pres == 0) {	/* Define the character in an embedded font. */
		pdf_char_proc_t *pcp;
		double x_offset;
		int y_offset;

		gs_image_t_init_mask(&image, false);
		invert = 0xff;
		pdf_make_bitmap_image(&image, x, y, w, h);
		y_offset = pdf_char_image_y_offset(pdev, x, y, h);
		/*
		 * The Y axis of the text matrix is inverted,
		 * so we need to negate the Y offset appropriately.
		 */
		code = pdf_begin_char_proc(pdev, w, h, 0, y_offset, id,
					   &pcp, &ipos);
		if (code < 0)
		    return code;
		y_offset = -y_offset;
		x_offset = psdf_round(pdev->char_width.x, 100, 10); /* See 
			pdf_write_Widths about rounding. We need to provide 
			a compatible data for Tj. */
		pprintg1(pdev->strm, "%g ", x_offset);
		pprintd3(pdev->strm, "0 0 %d %d %d d1\n", y_offset, w, h + y_offset);
		pprintd3(pdev->strm, "%d 0 0 %d 0 %d cm\n", w, h,
			 y_offset);
		pdf_image_writer_init(&writer);
		code = pdf_begin_write_image(pdev, &writer, gs_no_id, w, h, NULL, true);
		if (code < 0)
		    return code;
		pres = (pdf_resource_t *) pcp;
		goto wr;
	    } else if (pdev->pte) {
		/* We're under pdf_text_process. It set a high level color. */
	    } else
		set_image_color(pdev, one);
	    pdf_make_bitmap_matrix(&image.ImageMatrix, x, y, w, h, h);
	    goto rx;
	}
	set_image_color(pdev, one);
	gs_image_t_init_mask(&image, false);
	invert = 0xff;
    } else if (one == gx_no_color_index) {
	gs_image_t_init_mask(&image, false);
	set_image_color(pdev, zero);
    } else if (zero == pdev->black && one == pdev->white) {
	gs_cspace_init_DeviceGray(pdev->memory, &cs);
	gs_image_t_init(&image, &cs);
    } else if (zero == pdev->white && one == pdev->black) {
	gs_cspace_init_DeviceGray(pdev->memory, &cs);
	gs_image_t_init(&image, &cs);
	invert = 0xff;
    } else {
	/*
	 * We think this code is never executed when interpreting PostScript
	 * or PDF: the library never uses monobit non-mask images
	 * internally, and high-level images don't go through this code.
	 * However, we still want the code to work.
	 */
	gs_color_space cs_base;
	gx_color_index c[2];
	int i, j;
	int ncomp = pdev->color_info.num_components;
	byte *p;

	code = pdf_cspace_init_Device(pdev->memory, &cs_base, ncomp);
	if (code < 0)
	    return code;
	c[0] = psdf_adjust_color_index((gx_device_vector *)pdev, zero);
	c[1] = psdf_adjust_color_index((gx_device_vector *)pdev, one);
	gs_cspace_init(&cs, &gs_color_space_type_Indexed, pdev->memory, false);
	cs.params.indexed.base_space = *(gs_direct_color_space *)&cs_base;
	cs.params.indexed.hival = 1;
	p = palette;
	for (i = 0; i < 2; ++i)
	    for (j = ncomp - 1; j >= 0; --j)
		*p++ = (byte)(c[i] >> (j * 8));
	cs.params.indexed.lookup.table.data = palette;
	cs.params.indexed.lookup.table.size = p - palette;
	cs.params.indexed.use_proc = false;
	gs_image_t_init(&image, &cs);
	image.BitsPerComponent = 1;
    }
    pdf_make_bitmap_image(&image, x, y, w, h);
    {
	ulong nbytes = (ulong) ((w + 7) >> 3) * h;

	in_line = nbytes < pdev->MaxInlineImageSize;
	if (in_line)
	    pdf_put_image_matrix(pdev, &image.ImageMatrix, 1.0);
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
	pdf_image_writer_init(&writer);
	code = pdf_begin_write_image(pdev, &writer, gs_no_id, w, h, NULL, in_line);
	if (code < 0)
	    return code;
    }
  wr:
    if (image.ImageMask)
	pcsvalue = NULL;
    else {
	/*
	 * We don't have to worry about color space scaling: the color
	 * space is always a Device space.
	 */
	code = pdf_color_space(pdev, &cs_value, NULL, &cs,
			       &writer.pin->color_spaces, in_line);
	if (code < 0)
	    return code;
	pcsvalue = &cs_value;
    }
    /*
     * There are 3 different cases at this point:
     *      - Writing an in-line image (pres == 0, writer.pres == 0);
     *      - Writing an XObject image (pres == 0, writer.pres != 0);
     *      - Writing the image for a CharProc (pres != 0).
     * We handle them with in-line code followed by a switch,
     * rather than making the shared code into a procedure,
     * simply because there would be an awful lot of parameters
     * that would need to be passed.
     */
    if (pres) {
	/*
	 * Always use CCITTFax 2-D for character bitmaps.  It takes less
	 * space to invert the data with Decode than to set BlackIs1.
	 */
	float d0 = image.Decode[0];

	image.Decode[0] = image.Decode[1];
	image.Decode[1] = d0;
	psdf_CFE_binary(&writer.binary[0], image.Width, image.Height, true);
	invert ^= 0xff;
    } else {
	/* Use the Distiller compression parameters. */
	psdf_setup_image_filters((gx_device_psdf *) pdev, &writer.binary[0],
				 (gs_pixel_image_t *)&image, NULL, NULL, true);
    }
    pdf_begin_image_data(pdev, &writer, (const gs_pixel_image_t *)&image,
			 pcsvalue, 0);
    code = pdf_copy_mask_bits(writer.binary[0].strm, base, sourcex, raster,
			      w, h, invert);
    if (code < 0)
	return code;
    pdf_end_image_binary(pdev, &writer, writer.height);
    if (!pres) {
	switch ((code = pdf_end_write_image(pdev, &writer))) {
	    default:		/* error */
		return code;
	    case 1:
		return 0;
	    case 0:
		return pdf_do_image(pdev, writer.pres, &image.ImageMatrix,
				    true);
	}
    }
    writer.end_string = "";	/* no Q */
    switch ((code = pdf_end_write_image(pdev, &writer))) {
    default:		/* error */
	return code;
    case 0:			/* not possible */
	return_error(gs_error_Fatal);
    case 1:
	break;
    }
    code = pdf_end_char_proc(pdev, &ipos);
    if (code < 0)
	return code;
  rx:{
	gs_matrix imat;

	imat = image.ImageMatrix;
	imat.xx /= w;
	imat.xy /= h;
	imat.yx /= w;
	imat.yy /= h;
	return pdf_do_char_image(pdev, (const pdf_char_proc_t *)pres, &imat);
    }
}
int
gdev_pdf_copy_mono(gx_device * dev,
		   const byte * base, int sourcex, int raster, gx_bitmap_id id,
		   int x, int y, int w, int h, gx_color_index zero,
		   gx_color_index one)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;

    if (w <= 0 || h <= 0)
	return 0;
    return pdf_copy_mono(pdev, base, sourcex, raster, id, x, y, w, h,
			 zero, one, NULL);
}

/* Copy a color bitmap.  for_pattern = -1 means put the image in-line, */
/* 1 means put the image in a resource, 2 means image is a rasterized shading. */
int
pdf_copy_color_data(gx_device_pdf * pdev, const byte * base, int sourcex,
		    int raster, gx_bitmap_id id, int x, int y, int w, int h,
		    gs_image_t *pim, pdf_image_writer *piw,
		    int for_pattern)
{
    int depth = pdev->color_info.depth;
    int bytes_per_pixel = depth >> 3;
    gs_color_space cs;
    cos_value_t cs_value;
    ulong nbytes;
    int code = pdf_cspace_init_Device(pdev->memory, &cs, bytes_per_pixel);
    const byte *row_base;
    int row_step;
    bool in_line;

    if (code < 0)
	return code;		/* can't happen */
    gs_image_t_init(pim, &cs);
    pdf_make_bitmap_image(pim, x, y, w, h);
    pim->BitsPerComponent = 8;
    nbytes = (ulong)w * bytes_per_pixel * h;

    if (for_pattern == 1) {
	/*
	 * Patterns must be emitted in order of increasing user Y, i.e.,
	 * the opposite of PDF's standard image order.
	 */
	row_base = base + (h - 1) * raster;
	row_step = -raster;
	in_line = for_pattern < 0;
    } else {
	row_base = base;
	row_step = raster;
	in_line = nbytes < pdev->MaxInlineImageSize;
	pdf_put_image_matrix(pdev, &pim->ImageMatrix, 1.0);
	/*
	 * Check whether we've already made an XObject resource for this
	 * image.
	 */
	if (id != gx_no_bitmap_id) {
	    piw->pres = pdf_find_resource_by_gs_id(pdev, resourceXObject, id);
	    if (piw->pres)
		return 0;
	}
    }
    /*
     * We have to be able to control whether to put Pattern images in line,
     * to avoid trying to create an XObject resource while we're in the
     * middle of writing a Pattern resource.
     */
    if (for_pattern < 0)
	stream_puts(pdev->strm, "q ");
    /*
     * We don't have to worry about color space scaling: the color
     * space is always a Device space.
     */
    pdf_image_writer_init(piw);
    if ((code = pdf_begin_write_image(pdev, piw, id, w, h, NULL, in_line)) < 0 ||
	(code = pdf_color_space(pdev, &cs_value, NULL, &cs,
				&piw->pin->color_spaces, in_line)) < 0 ||
	(for_pattern < 2 || nbytes < 512000 ?
	    (code = psdf_setup_lossless_filters((gx_device_psdf *) pdev,
			&piw->binary[0], (gs_pixel_image_t *)pim)) :
	    (code = psdf_setup_image_filters((gx_device_psdf *) pdev,
			&piw->binary[0], (gs_pixel_image_t *)pim, NULL, NULL, false))
	) < 0 ||
	(code = pdf_begin_image_data(pdev, piw, (const gs_pixel_image_t *)pim,
				     &cs_value, 0)) < 0
	)
	return code;
    pdf_copy_color_bits(piw->binary[0].strm, row_base, sourcex, row_step, w, h,
			bytes_per_pixel);
    pdf_end_image_binary(pdev, piw, piw->height);
    return pdf_end_write_image(pdev, piw);
}

int
gdev_pdf_copy_color(gx_device * dev, const byte * base, int sourcex,
		    int raster, gx_bitmap_id id, int x, int y, int w, int h)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    gs_image_t image;
    pdf_image_writer writer;
    int code;
    
    if (w <= 0 || h <= 0)
	return 0;
    code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    /* Make sure we aren't being clipped. */
    code = pdf_put_clip_path(pdev, NULL);
    if (code < 0)
	return code;
    code = pdf_copy_color_data(pdev, base, sourcex, raster, id, x, y, w, h,
			       &image, &writer, 0);
    switch (code) {
	default:
	    return code;	/* error */
	case 1:
	    return 0;
	case 0:
	    return pdf_do_image(pdev, writer.pres, NULL, true);
    }
}

/* Fill a mask. */
int
gdev_pdf_fill_mask(gx_device * dev,
		 const byte * data, int data_x, int raster, gx_bitmap_id id,
		   int x, int y, int width, int height,
		   const gx_drawing_color * pdcolor, int depth,
		   gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;

    if (width <= 0 || height <= 0)
	return 0;
    if (depth > 1 || (!gx_dc_is_pure(pdcolor) != 0 && pdcolor->type != &gx_dc_pattern))
	return gx_default_fill_mask(dev, data, data_x, raster, id,
				    x, y, width, height, pdcolor, depth, lop,
				    pcpath);
    return pdf_copy_mono(pdev, data, data_x, raster, id, x, y, width, height,
			 gx_no_color_index, gx_dc_pure_color(pdcolor),
			 pcpath);
}

/* Tile with a bitmap.  This is important for pattern fills. */
int
gdev_pdf_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
			      int x, int y, int w, int h,
			      gx_color_index color0, gx_color_index color1,
			      int px, int py)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    int tw = tiles->rep_width, th = tiles->rep_height;
    double xscale = pdev->HWResolution[0] / 72.0,
	yscale = pdev->HWResolution[1] / 72.0;
    bool mask;
    int depth;
    int (*copy_data)(gx_device_pdf *, const byte *, int, int,
		     gx_bitmap_id, int, int, int, int,
		     gs_image_t *, pdf_image_writer *, int);
    pdf_resource_t *pres;
    cos_value_t cs_value;
    int code;

    if (tiles->id == gx_no_bitmap_id || tiles->shift != 0 ||
	(w < tw && h < th) ||
	color0 != gx_no_color_index
	)
	goto use_default;
    if (color1 != gx_no_color_index) {
	/* This is a mask pattern. */
	mask = true;
	depth = 1;
	copy_data = pdf_copy_mask_data;
	code = pdf_cs_Pattern_uncolored(pdev, &cs_value);
    } else {
	/* This is a colored pattern. */
	mask = false;
	depth = pdev->color_info.depth;
	copy_data = pdf_copy_color_data;
	code = pdf_cs_Pattern_colored(pdev, &cs_value);
    }
    if (code < 0)
	goto use_default;
    pres = pdf_find_resource_by_gs_id(pdev, resourcePattern, tiles->id);
    if (!pres) {
	/* Create the Pattern resource. */
	int code;
	long image_id, length_id, start, end;
	stream *s;
	gs_image_t image;
	pdf_image_writer writer;
	long image_bytes = ((long)tw * depth + 7) / 8 * th;
	bool in_line = image_bytes < pdev->MaxInlineImageSize;
	ulong tile_id =
	    (tw == tiles->size.x && th == tiles->size.y ? tiles->id :
	     gx_no_bitmap_id);

	if (in_line)
	    image_id = 0;
	else if (image_bytes > 65500) {
	    /*
	     * Acrobat Reader can't handle image Patterns with more than
	     * 64K of data.  :-(
	     */
	    goto use_default;
	} else {
	    /* Write the image as an XObject resource now. */
	    code = copy_data(pdev, tiles->data, 0, tiles->raster,
			     tile_id, 0, 0, tw, th, &image, &writer, 1);
	    if (code < 0)
		goto use_default;
	    image_id = pdf_resource_id(writer.pres);
	}
	code = pdf_begin_resource(pdev, resourcePattern, tiles->id, &pres);
	if (code < 0)
	    goto use_default;
	s = pdev->strm;
	pprintd1(s, "/PatternType 1/PaintType %d/TilingType 1/Resources<<\n",
		 (mask ? 2 : 1));
	if (image_id)
	    pprintld2(s, "/XObject<</R%ld %ld 0 R>>", image_id, image_id);
	pprints1(s, "/ProcSet[/PDF/Image%s]>>\n", (mask ? "B" : "C"));
	/*
	 * Because of bugs in Acrobat Reader's Print function, we can't use
	 * the natural BBox and Step here: they have to be 1.
	 */
	pprintg2(s, "/Matrix[%g 0 0 %g 0 0]", tw / xscale, th / yscale);
	stream_puts(s, "/BBox[0 0 1 1]/XStep 1/YStep 1/Length ");
	if (image_id) {
	    char buf[MAX_REF_CHARS + 6 + 1]; /* +6 for /R# Do\n */

	    sprintf(buf, "/R%ld Do\n", image_id);
	    pprintd1(s, "%d>>stream\n", strlen(buf));
	    pprints1(s, "%sendstream\n", buf);
	    pdf_end_resource(pdev);
	} else {
	    length_id = pdf_obj_ref(pdev);
	    pprintld1(s, "%ld 0 R>>stream\n", length_id);
	    start = pdf_stell(pdev);
	    code = copy_data(pdev, tiles->data, 0, tiles->raster,
			     tile_id, 0, 0, tw, th, &image, &writer, -1);
	    switch (code) {
	    default:
		return code;	/* error */
	    case 1:
		break;
	    case 0:			/* not possible */
		return_error(gs_error_Fatal);
	    }
	    end = pdf_stell(pdev);
	    stream_puts(s, "\nendstream\n");
	    pdf_end_resource(pdev);
	    pdf_open_separate(pdev, length_id);
	    pprintld1(pdev->strm, "%ld\n", end - start);
	    pdf_end_separate(pdev);
	}
	pres->object->written = true; /* don't write at end of page */
    }
    /* Fill the rectangle with the Pattern. */
    {
	int code = pdf_open_page(pdev, PDF_IN_STREAM);
	stream *s;

	if (code < 0)
	    goto use_default;
	/* Make sure we aren't being clipped. */
	code = pdf_put_clip_path(pdev, NULL);
	if (code < 0)
	    return code;
	s = pdev->strm;
	/*
	 * Because of bugs in Acrobat Reader's Print function, we can't
	 * leave the CTM alone here: we have to reset it to the default.
	 */
	pprintg2(s, "q %g 0 0 %g 0 0 cm\n", xscale, yscale);
	cos_value_write(&cs_value, pdev);
	stream_puts(s, " cs");
	if (mask)
	    pprintg3(s, " %g %g %g", (int)(color1 >> 16) / 255.0,
		     (int)((color1 >> 8) & 0xff) / 255.0,
		     (int)(color1 & 0xff) / 255.0);
	pprintld1(s, "/R%ld scn", pdf_resource_id(pres));
	pprintg4(s, " %g %g %g %g re f Q\n",
		 x / xscale, y / yscale, w / xscale, h / xscale);
    }
    return 0;
use_default:
    return gx_default_strip_tile_rectangle(dev, tiles, x, y, w, h,
					   color0, color1, px, py);
}
