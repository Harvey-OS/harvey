/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpsdi.c,v 1.12.2.1 2000/11/09 23:24:18 rayjj Exp $ */
/* Image compression for PostScript and PDF writers */
#include "stdio_.h"		/* for jpeglib.h */
#include "jpeglib_.h"		/* for sdct.h */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscspace.h"
#include "gdevpsdf.h"
#include "gdevpsds.h"
#include "strimpl.h"
#include "scfx.h"
#include "sdct.h"
#include "sjpeg.h"
#include "slzwx.h"
#include "spngpx.h"
#include "srlx.h"
#include "szlibx.h"

/* Define parameter-setting procedures. */
extern stream_state_proc_put_params(s_CF_put_params, stream_CF_state);

/* ---------------- Image compression ---------------- */

/*
 * Add a filter to expand or reduce the pixel width if needed.
 * At least one of bpc_in and bpc_out is 8; the other is 1, 2, 4, or 8,
 * except if bpc_out is 8, bpc_in may be 12.
 */
private int
pixel_resize(psdf_binary_writer * pbw, int width, int num_components,
	     int bpc_in, int bpc_out)
{
    gs_memory_t *mem = pbw->dev->v_memory;
    const stream_template *template;
    stream_1248_state *st;
    int code;

    if (bpc_out == bpc_in)
	return 0;
    if (bpc_in != 8) {
	static const stream_template *const exts[13] = {
	    0, &s_1_8_template, &s_2_8_template, 0, &s_4_8_template,
	    0, 0, 0, 0, 0, 0, 0, &s_12_8_template
	};

	template = exts[bpc_in];
    } else {
	static const stream_template *const rets[5] = {
	    0, &s_8_1_template, &s_8_2_template, 0, &s_8_4_template
	};

	template = rets[bpc_out];
    }
    st = (stream_1248_state *)
	s_alloc_state(mem, template->stype, "pixel_resize state");
    if (st == 0)
	return_error(gs_error_VMerror);
    code = psdf_encode_binary(pbw, template, (stream_state *) st);
    if (code < 0) {
	gs_free_object(mem, st, "pixel_resize state");
	return code;
    }
    s_1248_init(st, width, num_components);
    return 0;
}

/* Add the appropriate image compression filter, if any. */
private int
setup_image_compression(psdf_binary_writer *pbw, const psdf_image_params *pdip,
			const gs_pixel_image_t * pim)
{
    gx_device_psdf *pdev = pbw->dev;
    gs_memory_t *mem = pdev->v_memory;
    const stream_template *template = pdip->filter_template;
    const stream_template *orig_template = template;
    const stream_template *lossless_template =
	(pdev->params.UseFlateCompression &&
	 pdev->version >= psdf_version_ll3 ?
	 &s_zlibE_template : &s_LZWE_template);
    const gs_color_space *pcs = pim->ColorSpace; /* null if mask */
    int Colors = (pcs ? gs_color_space_num_components(pcs) : 1);
    bool Indexed =
	(pcs != 0 && 
	 gs_color_space_get_index(pcs) == gs_color_space_index_Indexed);
    gs_c_param_list *dict = pdip->Dict;
    stream_state *st;
    int code;

    if (!pdip->Encode)		/* no compression */
	return 0;
    if (pdip->AutoFilter) {
	/*
	 * Disregard the requested filter: use DCTEncode with ACSDict
	 * instead (or the lossless filter if the conditions for JPEG
	 * encoding aren't met).
	 *
	 * Even though this isn't obvious from the Adobe Tech Note,
	 * it appears that if UseFlateCompression is true, the default
	 * compressor for AutoFilter is FlateEncode, not LZWEncode.
	 */
	orig_template = template = &s_DCTE_template;
	dict = pdip->ACSDict;
    }
    if (template == 0)	/* no compression */
	return 0;
    if (pim->Width * pim->Height * Colors * pim->BitsPerComponent <= 160)	/* not worth compressing */
	return 0;
    /* Only use DCTE for 8-bit, non-Indexed data. */
    if (template == &s_DCTE_template) {
	if (Indexed ||
	    !(pdip->Downsample ?
	      pdip->Depth == 8 ||
	      (pdip->Depth == -1 && pim->BitsPerComponent == 8) :
	      pim->BitsPerComponent == 8)
	    ) {
	    /* Use LZW/Flate instead. */
	    template = lossless_template;
	}
    }
    st = s_alloc_state(mem, template->stype, "setup_image_compression");
    if (st == 0)
	return_error(gs_error_VMerror);
    if (template->set_defaults)
	(*template->set_defaults) (st);
    if (template == &s_CFE_template) {
	stream_CFE_state *const ss = (stream_CFE_state *) st;

	if (pdip->Dict != 0 && pdip->filter_template == template) {
	    s_CF_put_params((gs_param_list *)pdip->Dict,
			    (stream_CF_state *)ss); /* ignore errors */
	} else {
	    ss->K = -1;
	    ss->BlackIs1 = true;
	}
	ss->Columns = pim->Width;
	ss->Rows = (ss->EndOfBlock ? 0 : pim->Height);
    } else if ((template == &s_LZWE_template ||
		template == &s_zlibE_template) &&
	       pdev->version >= psdf_version_ll3) {
	/* If not Indexed, add a PNGPredictor filter. */
	if (!Indexed) {
	    code = psdf_encode_binary(pbw, template, st);
	    if (code < 0)
		goto fail;
	    template = &s_PNGPE_template;
	    st = s_alloc_state(mem, template->stype, "setup_image_compression");
	    if (st == 0) {
		code = gs_note_error(gs_error_VMerror);
		goto fail;
	    }
	    if (template->set_defaults)
		(*template->set_defaults) (st);
	    {
		stream_PNGP_state *const ss = (stream_PNGP_state *) st;

		ss->Colors = Colors;
		ss->Columns = pim->Width;
	    }
	}
    } else if (template == &s_DCTE_template) {
	code = psdf_DCT_filter((dict != 0 && orig_template == template ?
				(gs_param_list *)dict : NULL),
			       st, pim->Width, pim->Height, Colors, pbw);
	if (code < 0)
	    goto fail;
	/* psdf_DCT_filter already did the psdf_encode_binary. */
	return 0;
    }
    code = psdf_encode_binary(pbw, template, st);
    if (code >= 0)
	return 0;
 fail:
    gs_free_object(mem, st, "setup_image_compression");
    return code;
}

/* Determine whether an image should be downsampled. */
private bool
do_downsample(const psdf_image_params *pdip, const gs_pixel_image_t *pim,
	      floatp resolution)
{
    floatp factor = (int)(resolution / pdip->Resolution);

    return (pdip->Downsample && factor >= pdip->DownsampleThreshold &&
	    factor <= pim->Width && factor <= pim->Height);
}

/* Add downsampling, antialiasing, and compression filters. */
/* Uses AntiAlias, Depth, DownsampleThreshold, DownsampleType, Resolution. */
/* Assumes do_downsampling() is true. */
private int
setup_downsampling(psdf_binary_writer * pbw, const psdf_image_params * pdip,
		   gs_pixel_image_t * pim, floatp resolution)
{
    gx_device_psdf *pdev = pbw->dev;
    /* Note: Bicubic is currently interpreted as Average. */
    const stream_template *template =
	(pdip->DownsampleType == ds_Subsample ?
	 &s_Subsample_template : &s_Average_template);
    int factor = (int)(resolution / pdip->Resolution);
    int orig_bpc = pim->BitsPerComponent;
    int orig_width = pim->Width;
    int orig_height = pim->Height;
    stream_state *st;
    int code;

    st = s_alloc_state(pdev->v_memory, template->stype,
		       "setup_downsampling");
    if (st == 0)
	return_error(gs_error_VMerror);
    if (template->set_defaults)
	template->set_defaults(st);
    {
	stream_Downsample_state *const ss = (stream_Downsample_state *) st;

	ss->Colors =
	    (pim->ColorSpace == 0 ? 1 /*mask*/ :
	     gs_color_space_num_components(pim->ColorSpace));
	ss->WidthIn = pim->Width;
	ss->HeightIn = pim->Height;
	ss->XFactor = ss->YFactor = factor;
	ss->AntiAlias = pdip->AntiAlias;
	ss->padX = ss->padY = false; /* should be true */
	if (template->init)
	    template->init(st);
	pim->Width = s_Downsample_size_out(pim->Width, factor, ss->padX);
	pim->Height = s_Downsample_size_out(pim->Height, factor, ss->padY);
	pim->BitsPerComponent = pdip->Depth;
	gs_matrix_scale(&pim->ImageMatrix, (double)pim->Width / orig_width,
			(double)pim->Height / orig_height,
			&pim->ImageMatrix);
	/****** NO ANTI-ALIASING YET ******/
	if ((code = setup_image_compression(pbw, pdip, pim)) < 0 ||
	    (code = pixel_resize(pbw, pim->Width, ss->Colors,
				 8, pdip->Depth)) < 0 ||
	    (code = psdf_encode_binary(pbw, template, st)) < 0 ||
	    (code = pixel_resize(pbw, orig_width, ss->Colors,
				 orig_bpc, 8)) < 0
	    ) {
	    gs_free_object(pdev->v_memory, st, "setup_image_compression");
	    return code;
	}
    }
    return 0;
}

/* Set up compression and downsampling filters for an image. */
/* Note that this may modify the image parameters. */
int
psdf_setup_image_filters(gx_device_psdf * pdev, psdf_binary_writer * pbw,
			 gs_pixel_image_t * pim, const gs_matrix * pctm,
			 const gs_imager_state * pis)
{
    /*
     * The following algorithms are per Adobe Tech Note # 5151,
     * "Acrobat Distiller Parameters", revised 16 September 1996
     * for Acrobat(TM) Distiller(TM) 3.0.
     *
     * The control structure is a little tricky, because filter
     * pipelines must be constructed back-to-front.
     */
    int code = 0;
    psdf_image_params params;
    int bpc = pim->BitsPerComponent;
    int bpc_out = pim->BitsPerComponent = min(bpc, 8);
    int ncomp;
    double resolution;

    /*
     * The Adobe documentation doesn't say this, but mask images are
     * compressed on the same basis as 1-bit-deep monochrome images,
     * except that anti-aliasing (resolution/depth tradeoff) is not
     * allowed.
     */
    if (pim->ColorSpace == NULL) { /* mask image */
	params = pdev->params.MonoImage;
	params.Depth = 1;
	ncomp = 1;
    } else {
	ncomp = gs_color_space_num_components(pim->ColorSpace);
	if (ncomp == 1) {
	    if (bpc == 1)
		params = pdev->params.MonoImage;
	    else
		params = pdev->params.GrayImage;
	    if (params.Depth == -1)
		params.Depth = bpc;
	} else {
	    params = pdev->params.ColorImage;
	    /* params.Depth is reset below */
	}
    }

    /*
     * We can compute the image resolution by:
     *    W / (W * ImageMatrix^-1 * CTM / HWResolution).
     * We can replace W by 1 to simplify the computation.
     */
    if (pctm == 0)
	resolution = -1;
    else {
	gs_point pt;

	/* We could do both X and Y, but why bother? */
	gs_distance_transform_inverse(1.0, 0.0, &pim->ImageMatrix, &pt);
	gs_distance_transform(pt.x, pt.y, pctm, &pt);
	resolution = 1.0 / hypot(pt.x / pdev->HWResolution[0],
				 pt.y / pdev->HWResolution[1]);
    }
    if (ncomp == 1) {
	/* Monochrome, gray, or mask */
	/* Check for downsampling. */
	if (do_downsample(&params, pim, resolution)) {
	    /* Use the downsampled depth, not the original data depth. */
	    if (params.Depth == 1) {
		params.Filter = pdev->params.MonoImage.Filter;
		params.filter_template = pdev->params.MonoImage.filter_template;
		params.Dict = pdev->params.MonoImage.Dict;
	    } else {
		params.Filter = pdev->params.GrayImage.Filter;
		params.filter_template = pdev->params.GrayImage.filter_template;
		params.Dict = pdev->params.GrayImage.Dict;
	    }
	    code = setup_downsampling(pbw, &params, pim, resolution);
	} else {
	    code = setup_image_compression(pbw, &params, pim);
	}
	if (code < 0)
	    return code;
	code = pixel_resize(pbw, pim->Width, ncomp, bpc, bpc_out);
    } else {
	/* Color */
	bool cmyk_to_rgb =
	    pdev->params.ConvertCMYKImagesToRGB &&
	    pis != 0 &&
	    gs_color_space_get_index(pim->ColorSpace) ==
	    gs_color_space_index_DeviceCMYK;

	if (cmyk_to_rgb)
	    pim->ColorSpace = gs_cspace_DeviceRGB(pis);
	if (params.Depth == -1)
	    params.Depth = (cmyk_to_rgb ? 8 : bpc_out);
	if (do_downsample(&params, pim, resolution)) {
	    code = setup_downsampling(pbw, &params, pim, resolution);
	} else {
	    code = setup_image_compression(pbw, &params, pim);
	}
	if (cmyk_to_rgb) {
	    gs_memory_t *mem = pdev->v_memory;
	    stream_C2R_state *ss = (stream_C2R_state *)
		s_alloc_state(mem, s_C2R_template.stype, "C2R state");
	    int code = pixel_resize(pbw, pim->Width, 3, 8, bpc_out);
		    
	    if (code < 0 ||
		(code = psdf_encode_binary(pbw, &s_C2R_template,
					   (stream_state *) ss)) < 0 ||
		(code = pixel_resize(pbw, pim->Width, 4, bpc, 8)) < 0
		)
		return code;
	    s_C2R_init(ss, pis);
	} else {
	    code = pixel_resize(pbw, pim->Width, ncomp, bpc, bpc_out);
	    if (code < 0)
		return code;
	}
    }
    return code;
}

/* Set up compression filters for a lossless image, with no downsampling, */
/* no color space conversion, and only lossless filters. */
/* Note that this may modify the image parameters. */
int
psdf_setup_lossless_filters(gx_device_psdf *pdev, psdf_binary_writer *pbw,
			    gs_pixel_image_t *pim)
{
    /*
     * Set up a device with modified parameters for computing the image
     * compression filters.  Don't allow downsampling or lossy compression.
     */
    gx_device_psdf ipdev;

    ipdev = *pdev;
    ipdev.params.ColorImage.AutoFilter = false;
    ipdev.params.ColorImage.Downsample = false;
    ipdev.params.ColorImage.Filter = "FlateEncode";
    ipdev.params.ColorImage.filter_template = &s_zlibE_template;
    ipdev.params.ConvertCMYKImagesToRGB = false;
    ipdev.params.GrayImage.AutoFilter = false;
    ipdev.params.GrayImage.Downsample = false;
    ipdev.params.GrayImage.Filter = "FlateEncode";
    ipdev.params.GrayImage.filter_template = &s_zlibE_template;
    return psdf_setup_image_filters(&ipdev, pbw, pim, NULL, NULL);
}
