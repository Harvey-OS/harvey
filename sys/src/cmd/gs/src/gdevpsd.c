/* Copyright (C) 2002 artofcode LLC.  All rights reserved.
  
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

/*$Id: gdevpsd.c,v 1.23 2005/08/30 06:38:44 igor Exp $ */
/* PhotoShop (PSD) export device, supporting DeviceN color models. */

#include "math_.h"
#include "gdevprn.h"
#include "gsparam.h"
#include "gscrd.h"
#include "gscrdp.h"
#include "gxlum.h"
#include "gdevdcrd.h"
#include "gstypes.h"
#include "icc.h"
#include "gxdcconv.h"
#include "gdevdevn.h"
#include "gsequivc.h"

/* Enable logic for a local ICC output profile. */
#define ENABLE_ICC_PROFILE 0

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* The device descriptor */
private dev_proc_open_device(psd_prn_open);
private dev_proc_get_params(psd_get_params);
private dev_proc_put_params(psd_put_params);
private dev_proc_print_page(psd_print_page);
private dev_proc_map_color_rgb(psd_map_color_rgb);
private dev_proc_get_color_mapping_procs(get_psdrgb_color_mapping_procs);
private dev_proc_get_color_mapping_procs(get_psd_color_mapping_procs);
private dev_proc_get_color_comp_index(psd_get_color_comp_index);
private dev_proc_encode_color(psd_encode_color);
private dev_proc_decode_color(psd_decode_color);
private dev_proc_update_spot_equivalent_colors(psd_update_spot_equivalent_colors);

/* This is redundant with color_info.cm_name. We may eliminate this
   typedef and use the latter string for everything. */
typedef enum {
    psd_DEVICE_GRAY,
    psd_DEVICE_RGB,
    psd_DEVICE_CMYK,
    psd_DEVICE_N
} psd_color_model;

/*
 * A structure definition for a DeviceN type device
 */
typedef struct psd_device_s {
    gx_device_common;
    gx_prn_device_common;

    /*        ... device-specific parameters ... */

    gs_devn_params devn_params;		/* DeviceN generated parameters */

    equivalent_cmyk_color_params equiv_cmyk_colors;

    psd_color_model color_model;

    /* ICC color profile objects, for color conversion. */
    char profile_rgb_fn[256];
    icmLuBase *lu_rgb;
    int lu_rgb_outn;

    char profile_cmyk_fn[256];
    icmLuBase *lu_cmyk;
    int lu_cmyk_outn;

    char profile_out_fn[256];
    icmLuBase *lu_out;
} psd_device;

/* GC procedures */
private 
ENUM_PTRS_WITH(psd_device_enum_ptrs, psd_device *pdev)
{
    if (index < pdev->devn_params.separations.num_separations)
	ENUM_RETURN(pdev->devn_params.separations.names[index].data);
    ENUM_PREFIX(st_device_printer,
		    pdev->devn_params.separations.num_separations);
}

ENUM_PTRS_END
private RELOC_PTRS_WITH(psd_device_reloc_ptrs, psd_device *pdev)
{
    RELOC_PREFIX(st_device_printer);
    {
	int i;

	for (i = 0; i < pdev->devn_params.separations.num_separations; ++i) {
	    RELOC_PTR(psd_device, devn_params.separations.names[i].data);
	}
    }
}
RELOC_PTRS_END

/* Even though psd_device_finalize is the same as gx_device_finalize, */
/* we need to implement it separately because st_composite_final */
/* declares all 3 procedures as private. */
private void
psd_device_finalize(void *vpdev)
{
    gx_device_finalize(vpdev);
}

gs_private_st_composite_final(st_psd_device, psd_device,
    "psd_device", psd_device_enum_ptrs, psd_device_reloc_ptrs,
    psd_device_finalize);

/*
 * Macro definition for psd device procedures
 */
#define device_procs(get_color_mapping_procs)\
{	psd_prn_open,\
	gx_default_get_initial_matrix,\
	NULL,				/* sync_output */\
	gdev_prn_output_page,		/* output_page */\
	gdev_prn_close,			/* close */\
	NULL,				/* map_rgb_color - not used */\
	psd_map_color_rgb,		/* map_color_rgb */\
	NULL,				/* fill_rectangle */\
	NULL,				/* tile_rectangle */\
	NULL,				/* copy_mono */\
	NULL,				/* copy_color */\
	NULL,				/* draw_line */\
	NULL,				/* get_bits */\
	psd_get_params,			/* get_params */\
	psd_put_params,			/* put_params */\
	NULL,				/* map_cmyk_color - not used */\
	NULL,				/* get_xfont_procs */\
	NULL,				/* get_xfont_device */\
	NULL,				/* map_rgb_alpha_color */\
	gx_page_device_get_page_device,	/* get_page_device */\
	NULL,				/* get_alpha_bits */\
	NULL,				/* copy_alpha */\
	NULL,				/* get_band */\
	NULL,				/* copy_rop */\
	NULL,				/* fill_path */\
	NULL,				/* stroke_path */\
	NULL,				/* fill_mask */\
	NULL,				/* fill_trapezoid */\
	NULL,				/* fill_parallelogram */\
	NULL,				/* fill_triangle */\
	NULL,				/* draw_thin_line */\
	NULL,				/* begin_image */\
	NULL,				/* image_data */\
	NULL,				/* end_image */\
	NULL,				/* strip_tile_rectangle */\
	NULL,				/* strip_copy_rop */\
	NULL,				/* get_clipping_box */\
	NULL,				/* begin_typed_image */\
	NULL,				/* get_bits_rectangle */\
	NULL,				/* map_color_rgb_alpha */\
	NULL,				/* create_compositor */\
	NULL,				/* get_hardware_params */\
	NULL,				/* text_begin */\
	NULL,				/* finish_copydevice */\
	NULL,				/* begin_transparency_group */\
	NULL,				/* end_transparency_group */\
	NULL,				/* begin_transparency_mask */\
	NULL,				/* end_transparency_mask */\
	NULL,				/* discard_transparency_layer */\
	get_color_mapping_procs,	/* get_color_mapping_procs */\
	psd_get_color_comp_index,	/* get_color_comp_index */\
	psd_encode_color,		/* encode_color */\
	psd_decode_color,		/* decode_color */\
	NULL,				/* pattern_manage */\
	NULL,				/* fill_rectangle_hl_color */\
	NULL,				/* include_color_space */\
	NULL,				/* fill_linear_color_scanline */\
	NULL,				/* fill_linear_color_trapezoid */\
	NULL,				/* fill_linear_color_triangle */\
	psd_update_spot_equivalent_colors /* update_spot_equivalent_colors */\
}


private fixed_colorant_name DeviceGrayComponents[] = {
	"Gray",
	0		/* List terminator */
};

private fixed_colorant_name DeviceRGBComponents[] = {
	"Red",
	"Green",
	"Blue",
	0		/* List terminator */
};

#define psd_device_body(procs, dname, ncomp, pol, depth, mg, mc, cn)\
    std_device_full_body_type_extended(psd_device, &procs, dname,\
	  &st_psd_device,\
	  (int)((long)(DEFAULT_WIDTH_10THS) * (X_DPI) / 10),\
	  (int)((long)(DEFAULT_HEIGHT_10THS) * (Y_DPI) / 10),\
	  X_DPI, Y_DPI,\
    	  GX_DEVICE_COLOR_MAX_COMPONENTS,	/* MaxComponents */\
	  ncomp,		/* NumComp */\
	  pol,			/* Polarity */\
	  depth, 0,		/* Depth, GrayIndex */\
	  mg, mc,		/* MaxGray, MaxColor */\
	  mg + 1, mc + 1,	/* DitherGray, DitherColor */\
	  GX_CINFO_SEP_LIN,	/* Linear & Separable */\
	  cn,			/* Process color model name */\
	  0, 0,			/* offsets */\
	  0, 0, 0, 0		/* margins */\
	),\
	prn_device_body_rest_(psd_print_page)

/*
 * PSD device with RGB process color model.
 */
private const gx_device_procs spot_rgb_procs = device_procs(get_psdrgb_color_mapping_procs);

const psd_device gs_psdrgb_device =
{   
    psd_device_body(spot_rgb_procs, "psdrgb", 3, GX_CINFO_POLARITY_ADDITIVE, 24, 255, 255, "DeviceRGB"),
    /* devn_params specific parameters */
    { 8,	/* Bits per color - must match ncomp, depth, etc. above */
      DeviceRGBComponents,	/* Names of color model colorants */
      3,			/* Number colorants for RGB */
      0,			/* MaxSeparations has not been specified */
      {0},			/* SeparationNames */
      0,			/* SeparationOrder names */
      {0, 1, 2, 3, 4, 5, 6, 7 }	/* Initial component SeparationOrder */
    },
    { true },			/* equivalent CMYK colors for spot colors */
    /* PSD device specific parameters */
    psd_DEVICE_RGB,		/* Color model */
};

/*
 * Select the default number of components based upon the number of bits
 * that we have in a gx_color_index
 */
#define NC ((arch_sizeof_color_index <= 8) ? arch_sizeof_color_index : 8)

/*
 * PSD device with CMYK process color model and spot color support.
 */
private const gx_device_procs spot_cmyk_procs
		= device_procs(get_psd_color_mapping_procs);

const psd_device gs_psdcmyk_device =
{   
    psd_device_body(spot_cmyk_procs, "psdcmyk", NC, GX_CINFO_POLARITY_SUBTRACTIVE, NC * 8, 255, 255, "DeviceCMYK"),
    /* devn_params specific parameters */
    { 8,	/* Bits per color - must match ncomp, depth, etc. above */
      DeviceCMYKComponents,	/* Names of color model colorants */
      4,			/* Number colorants for CMYK */
      NC,			/* MaxSeparations has not been specified */
      {0},			/* SeparationNames */
      0,			/* SeparationOrder names */
      {0, 1, 2, 3, 4, 5, 6, 7 }	/* Initial component SeparationOrder */
    },
    { true },			/* equivalent CMYK colors for spot colors */
    /* PSD device specific parameters */
    psd_DEVICE_CMYK,		/* Color model */
};

#undef NC

/* Open the psd devices */
int
psd_prn_open(gx_device * pdev)
{
    int code = gdev_prn_open(pdev);

    set_linear_color_bits_mask_shift(pdev);
    pdev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
    return code;
}

/*
 * The following procedures are used to map the standard color spaces into
 * the color components for the psdrgb device.
 */
private void
gray_cs_to_psdrgb_cm(gx_device * dev, frac gray, frac out[])
{
    int i = ((psd_device *)dev)->devn_params.separations.num_separations;

    out[0] = out[1] = out[2] = gray;
    for(; i>0; i--)			/* Clear spot colors */
        out[2 + i] = 0;
}

private void
rgb_cs_to_psdrgb_cm(gx_device * dev, const gs_imager_state *pis,
				  frac r, frac g, frac b, frac out[])
{
    int i = ((psd_device *)dev)->devn_params.separations.num_separations;

    out[0] = r;
    out[1] = g;
    out[2] = b;
    for(; i>0; i--)			/* Clear spot colors */
        out[2 + i] = 0;
}

private void
cmyk_cs_to_psdrgb_cm(gx_device * dev,
			frac c, frac m, frac y, frac k, frac out[])
{
    int i = ((psd_device *)dev)->devn_params.separations.num_separations;

    color_cmyk_to_rgb(c, m, y, k, NULL, out);
    for(; i>0; i--)			/* Clear spot colors */
        out[2 + i] = 0;
}

/* Color mapping routines for the psdcmyk device */

private void
gray_cs_to_psdcmyk_cm(gx_device * dev, frac gray, frac out[])
{
    int * map = ((psd_device *) dev)->devn_params.separation_order_map;

    gray_cs_to_devn_cm(dev, map, gray, out);
}

private void
rgb_cs_to_psdcmyk_cm(gx_device * dev, const gs_imager_state *pis,
			   frac r, frac g, frac b, frac out[])
{
    int * map = ((psd_device *) dev)->devn_params.separation_order_map;

    rgb_cs_to_devn_cm(dev, map, pis, r, g, b, out);
}

private void
cmyk_cs_to_psdcmyk_cm(gx_device * dev,
			frac c, frac m, frac y, frac k, frac out[])
{
    int * map = ((psd_device *) dev)->devn_params.separation_order_map;

    cmyk_cs_to_devn_cm(dev, map, c, m, y, k, out);
}

private void
cmyk_cs_to_spotn_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
    psd_device *xdev = (psd_device *)dev;
    int n = xdev->devn_params.separations.num_separations;
    icmLuBase *luo = xdev->lu_cmyk;
    int i;

    if (luo != NULL) {
	double in[4];
	double tmp[MAX_CHAN];
	int outn = xdev->lu_cmyk_outn;

	in[0] = frac2float(c);
	in[1] = frac2float(m);
	in[2] = frac2float(y);
	in[3] = frac2float(k);
	luo->lookup(luo, tmp, in);
	for (i = 0; i < outn; i++)
	    out[i] = float2frac(tmp[i]);
	for (; i < n + 4; i++)
	    out[i] = 0;
    } else {
	/* If no profile given, assume CMYK */
	out[0] = c;
	out[1] = m;
	out[2] = y;
	out[3] = k;
	for(i = 0; i < n; i++)			/* Clear spot colors */
	    out[4 + i] = 0;
    }
}

private void
gray_cs_to_spotn_cm(gx_device * dev, frac gray, frac out[])
{
    cmyk_cs_to_spotn_cm(dev, 0, 0, 0, (frac)(frac_1 - gray), out);
}

private void
rgb_cs_to_spotn_cm(gx_device * dev, const gs_imager_state *pis,
				   frac r, frac g, frac b, frac out[])
{
    psd_device *xdev = (psd_device *)dev;
    int n = xdev->devn_params.separations.num_separations;
    icmLuBase *luo = xdev->lu_rgb;
    int i;

    if (luo != NULL) {
	double in[3];
	double tmp[MAX_CHAN];
	int outn = xdev->lu_rgb_outn;

	in[0] = frac2float(r);
	in[1] = frac2float(g);
	in[2] = frac2float(b);
	luo->lookup(luo, tmp, in);
	for (i = 0; i < outn; i++)
	    out[i] = float2frac(tmp[i]);
	for (; i < n + 4; i++)
	    out[i] = 0;
    } else {
	frac cmyk[4];

	color_rgb_to_cmyk(r, g, b, pis, cmyk);
	cmyk_cs_to_spotn_cm(dev, cmyk[0], cmyk[1], cmyk[2], cmyk[3],
			    out);
    }
}

private const gx_cm_color_map_procs psdRGB_procs = {
    gray_cs_to_psdrgb_cm, rgb_cs_to_psdrgb_cm, cmyk_cs_to_psdrgb_cm
};

private const gx_cm_color_map_procs psdCMYK_procs = {
    gray_cs_to_psdcmyk_cm, rgb_cs_to_psdcmyk_cm, cmyk_cs_to_psdcmyk_cm
};

private const gx_cm_color_map_procs psdN_procs = {
    gray_cs_to_spotn_cm, rgb_cs_to_spotn_cm, cmyk_cs_to_spotn_cm
};

/*
 * These are the handlers for returning the list of color space
 * to color model conversion routines.
 */
private const gx_cm_color_map_procs *
get_psdrgb_color_mapping_procs(const gx_device * dev)
{
    return &psdRGB_procs;
}

private const gx_cm_color_map_procs *
get_psd_color_mapping_procs(const gx_device * dev)
{
    const psd_device *xdev = (const psd_device *)dev;

    if (xdev->color_model == psd_DEVICE_RGB)
	return &psdRGB_procs;
    else if (xdev->color_model == psd_DEVICE_CMYK)
	return &psdCMYK_procs;
    else if (xdev->color_model == psd_DEVICE_N)
	return &psdN_procs;
    else
	return NULL;
}

/*
 * Encode a list of colorant values into a gx_color_index_value.
 */
private gx_color_index
psd_encode_color(gx_device *dev, const gx_color_value colors[])
{
    int bpc = ((psd_device *)dev)->devn_params.bitspercomponent;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_index color = 0;
    int i = 0;
    int ncomp = dev->color_info.num_components;

    for (; i<ncomp; i++) {
	color <<= bpc;
        color |= (colors[i] >> drop);
    }
    return (color == gx_no_color_index ? color ^ 1 : color);
}

/*
 * Decode a gx_color_index value back to a list of colorant values.
 */
private int
psd_decode_color(gx_device * dev, gx_color_index color, gx_color_value * out)
{
    int bpc = ((psd_device *)dev)->devn_params.bitspercomponent;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    int mask = (1 << bpc) - 1;
    int i = 0;
    int ncomp = dev->color_info.num_components;

    for (; i<ncomp; i++) {
        out[ncomp - i - 1] = (gx_color_value) ((color & mask) << drop);
	color >>= bpc;
    }
    return 0;
}

/*
 * Convert a gx_color_index to RGB.
 */
private int
psd_map_color_rgb(gx_device *dev, gx_color_index color, gx_color_value rgb[3])
{
    psd_device *xdev = (psd_device *)dev;

    if (xdev->color_model == psd_DEVICE_RGB)
	return psd_decode_color(dev, color, rgb);
    /* TODO: return reasonable values. */
    rgb[0] = 0;
    rgb[1] = 0;
    rgb[2] = 0;
    return 0;
}

/*
 *  Device proc for updating the equivalent CMYK color for spot colors.
 */
private int
psd_update_spot_equivalent_colors(gx_device *pdev, const gs_state * pgs)
{
    psd_device * psdev = (psd_device *)pdev;

    update_spot_equivalent_cmyk_colors(pdev, pgs,
		    &psdev->devn_params, &psdev->equiv_cmyk_colors);
    return 0;
}

#if ENABLE_ICC_PROFILE
private int
psd_open_profile(psd_device *xdev, char *profile_fn, icmLuBase **pluo,
		 int *poutn)
{
    icmFile *fp;
    icc *icco;
    icmLuBase *luo;

    dlprintf1("psd_open_profile %s\n", profile_fn);
    fp = new_icmFileStd_name(profile_fn, (char *)"rb");
    if (fp == NULL)
	return_error(gs_error_undefinedfilename);
    icco = new_icc();
    if (icco == NULL)
	return_error(gs_error_VMerror);
    if (icco->read(icco, fp, 0))
	return_error(gs_error_rangecheck);
    luo = icco->get_luobj(icco, icmFwd, icmDefaultIntent, icmSigDefaultData, icmLuOrdNorm);
    if (luo == NULL)
	return_error(gs_error_rangecheck);
    *pluo = luo;
    luo->spaces(luo, NULL, NULL, NULL, poutn, NULL, NULL, NULL, NULL);
    return 0;
}

private int
psd_open_profiles(psd_device *xdev)
{
    int code = 0;
    if (xdev->lu_out == NULL && xdev->profile_out_fn[0]) {
	code = psd_open_profile(xdev, xdev->profile_out_fn,
				    &xdev->lu_out, NULL);
    }
    if (code >= 0 && xdev->lu_rgb == NULL && xdev->profile_rgb_fn[0]) {
	code = psd_open_profile(xdev, xdev->profile_rgb_fn,
				&xdev->lu_rgb, &xdev->lu_rgb_outn);
    }
    if (code >= 0 && xdev->lu_cmyk == NULL && xdev->profile_cmyk_fn[0]) {
	code = psd_open_profile(xdev, xdev->profile_cmyk_fn,
				&xdev->lu_cmyk, &xdev->lu_cmyk_outn);
    }
    return code;
}
#endif

/* Get parameters.  We provide a default CRD. */
private int
psd_get_params(gx_device * pdev, gs_param_list * plist)
{
    psd_device *xdev = (psd_device *)pdev;
    int code;
#if ENABLE_ICC_PROFILE
    gs_param_string pos;
    gs_param_string prgbs;
    gs_param_string pcmyks;
#endif

    code = gdev_prn_get_params(pdev, plist);
    if (code < 0)
	return code;
    code = devn_get_params(pdev, plist,
    	&(xdev->devn_params), &(xdev->equiv_cmyk_colors));
    if (code < 0)
	return code;

#if ENABLE_ICC_PROFILE
    pos.data = (const byte *)xdev->profile_out_fn,
	pos.size = strlen(xdev->profile_out_fn),
	pos.persistent = false;
    code = param_write_string(plist, "ProfileOut", &pos);
    if (code < 0)
	return code;

    prgbs.data = (const byte *)xdev->profile_rgb_fn,
	prgbs.size = strlen(xdev->profile_rgb_fn),
	prgbs.persistent = false;
    code = param_write_string(plist, "ProfileRgb", &prgbs);
    if (code < 0)
	return code;

    pcmyks.data = (const byte *)xdev->profile_cmyk_fn,
	pcmyks.size = strlen(xdev->profile_cmyk_fn),
	pcmyks.persistent = false;
    code = param_write_string(plist, "ProfileCmyk", &prgbs);
#endif

    return code;
}

#if ENABLE_ICC_PROFILE
private int
psd_param_read_fn(gs_param_list *plist, const char *name,
		  gs_param_string *pstr, uint max_len)
{
    int code = param_read_string(plist, name, pstr);

    if (code == 0) {
	if (pstr->size >= max_len)
	    param_signal_error(plist, name, code = gs_error_rangecheck);
    } else {
	pstr->data = 0;
    }
    return code;
}
#endif

/* Compare a C string and a gs_param_string. */
static bool
param_string_eq(const gs_param_string *pcs, const char *str)
{
    return (strlen(str) == pcs->size &&
	    !strncmp(str, (const char *)pcs->data, pcs->size));
}

private int
psd_set_color_model(psd_device *xdev, psd_color_model color_model)
{
    xdev->color_model = color_model;
    if (color_model == psd_DEVICE_GRAY) {
	xdev->devn_params.std_colorant_names = DeviceGrayComponents;
	xdev->devn_params.num_std_colorant_names = 1;
	xdev->color_info.cm_name = "DeviceGray";
	xdev->color_info.polarity = GX_CINFO_POLARITY_ADDITIVE;
    } else if (color_model == psd_DEVICE_RGB) {
	xdev->devn_params.std_colorant_names = DeviceRGBComponents;
	xdev->devn_params.num_std_colorant_names = 3;
	xdev->color_info.cm_name = "DeviceRGB";
	xdev->color_info.polarity = GX_CINFO_POLARITY_ADDITIVE;
    } else if (color_model == psd_DEVICE_CMYK) {
	xdev->devn_params.std_colorant_names = DeviceCMYKComponents;
	xdev->devn_params.num_std_colorant_names = 4;
	xdev->color_info.cm_name = "DeviceCMYK";
	xdev->color_info.polarity = GX_CINFO_POLARITY_SUBTRACTIVE;
    } else if (color_model == psd_DEVICE_N) {
	xdev->devn_params.std_colorant_names = DeviceCMYKComponents;
	xdev->devn_params.num_std_colorant_names = 4;
	xdev->color_info.cm_name = "DeviceN";
	xdev->color_info.polarity = GX_CINFO_POLARITY_SUBTRACTIVE;
    } else {
	return -1;
    }

    return 0;
}

/* Set parameters.  We allow setting the number of bits per component. */
private int
psd_put_params(gx_device * pdev, gs_param_list * plist)
{
    psd_device * const pdevn = (psd_device *) pdev;
    int code = 0;
#if ENABLE_ICC_PROFILE
    gs_param_string po;
    gs_param_string prgb;
    gs_param_string pcmyk;
#endif
    gs_param_string pcm;
    psd_color_model color_model = pdevn->color_model;
    gx_device_color_info save_info = pdevn->color_info;

#if ENABLE_ICC_PROFILE
    code = psd_param_read_fn(plist, "ProfileOut", &po,
				 sizeof(pdevn->profile_out_fn));
    if (code >= 0)
	code = psd_param_read_fn(plist, "ProfileRgb", &prgb,
				 sizeof(pdevn->profile_rgb_fn));
    if (code >= 0)
	code = psd_param_read_fn(plist, "ProfileCmyk", &pcmyk,
				 sizeof(pdevn->profile_cmyk_fn));
#endif

    if (code >= 0)
	code = param_read_name(plist, "ProcessColorModel", &pcm);
    if (code == 0) {
	if (param_string_eq (&pcm, "DeviceGray"))
	    color_model = psd_DEVICE_GRAY;
	else if (param_string_eq (&pcm, "DeviceRGB"))
	    color_model = psd_DEVICE_RGB;
	else if (param_string_eq (&pcm, "DeviceCMYK"))
	    color_model = psd_DEVICE_CMYK;
	else if (param_string_eq (&pcm, "DeviceN"))
	    color_model = psd_DEVICE_N;
	else {
	    param_signal_error(plist, "ProcessColorModel",
			       code = gs_error_rangecheck);
	}
    }

    if (code >= 0)
        code = psd_set_color_model(pdevn, color_model);

    /* handle the standard DeviceN related parameters */
    if (code == 0)
        code = devn_printer_put_params(pdev, plist,
		&(pdevn->devn_params), &(pdevn->equiv_cmyk_colors));

    if (code < 0) {
	pdev->color_info = save_info;
	return code;
    }

#if ENABLE_ICC_PROFILE
    /* Open any ICC profiles that have been specified. */
    if (po.data != 0) {
	memcpy(pdevn->profile_out_fn, po.data, po.size);
	pdevn->profile_out_fn[po.size] = 0;
    }
    if (prgb.data != 0) {
	memcpy(pdevn->profile_rgb_fn, prgb.data, prgb.size);
	pdevn->profile_rgb_fn[prgb.size] = 0;
    }
    if (pcmyk.data != 0) {
	memcpy(pdevn->profile_cmyk_fn, pcmyk.data, pcmyk.size);
	pdevn->profile_cmyk_fn[pcmyk.size] = 0;
    }
    if (memcmp(&pdevn->color_info, &save_info,
			    size_of(gx_device_color_info)) != 0)
        code = psd_open_profiles(pdevn);
#endif

    return code;
}


/*
 * This routine will check to see if the color component name  match those
 * that are available amoung the current device's color components.  
 *
 * Parameters:
 *   dev - pointer to device data structure.
 *   pname - pointer to name (zero termination not required)
 *   nlength - length of the name
 *
 * This routine returns a positive value (0 to n) which is the device colorant
 * number if the name is found.  It returns a negative value if not found.
 */
private int
psd_get_color_comp_index(gx_device * dev, const char * pname,
					int name_size, int component_type)
{
    return devn_get_color_comp_index(dev,
		&(((psd_device *)dev)->devn_params),
		&(((psd_device *)dev)->equiv_cmyk_colors),
		pname, name_size, component_type, ENABLE_AUTO_SPOT_COLORS);
}


/* ------ Private definitions ------ */

/* All two-byte quantities are stored MSB-first! */
#if arch_is_big_endian
#  define assign_u16(a,v) a = (v)
#  define assign_u32(a,v) a = (v)
#else
#  define assign_u16(a,v) a = ((v) >> 8) + ((v) << 8)
#  define assign_u32(a,v) a = (((v) >> 24) & 0xff) + (((v) >> 8) & 0xff00) + (((v) & 0xff00) << 8) + (((v) & 0xff) << 24)
#endif

typedef struct {
    FILE *f;

    int width;
    int height;
    int base_bytes_pp;	/* almost always 3 (rgb) or 4 (CMYK) */
    int n_extra_channels;
    int num_channels;	/* base_bytes_pp + any spot colors that are imaged */
    /* Map output channel number to original separation number. */
    int chnl_to_orig_sep[GX_DEVICE_COLOR_MAX_COMPONENTS];
    /* Map output channel number to gx_color_index position. */
    int chnl_to_position[GX_DEVICE_COLOR_MAX_COMPONENTS];

    /* byte offset of image data */
    int image_data_off;
} psd_write_ctx;

private int
psd_setup(psd_write_ctx *xc, psd_device *dev)
{
    int i;

#define NUM_CMYK_COMPONENTS 4
    xc->base_bytes_pp = dev->devn_params.num_std_colorant_names;
    xc->num_channels = xc->base_bytes_pp;
    xc->n_extra_channels = dev->devn_params.separations.num_separations;
    xc->width = dev->width;
    xc->height = dev->height;

    /*
     * Determine the order of the output components.  This is based upon
     * the SeparationOrder parameter.  This parameter can be used to select
     * which planes are actually imaged.  For the process color model channels
     * we image the channels which are requested.  Non requested process color
     * model channels are simply filled with white.  For spot colors we only
     * image the requested channels.  Note:  There are no spot colors with
     * the RGB color model.
     */
    for (i = 0; i < xc->base_bytes_pp + xc->n_extra_channels; i++)
	xc->chnl_to_position[i] = -1;
    for (i = 0; i < xc->base_bytes_pp + xc->n_extra_channels; i++) {
	int sep_order_num = dev->devn_params.separation_order_map[i];

	if (sep_order_num != GX_DEVICE_COLOR_MAX_COMPONENTS) {
	    if (i < NUM_CMYK_COMPONENTS)	/* Do not rearrange CMYK */
	        xc->chnl_to_position[i] = sep_order_num;
	    else {				/* Re arrange separations */
	        xc->chnl_to_position[xc->num_channels] = sep_order_num;
	        xc->chnl_to_orig_sep[xc->num_channels++] = i;
	    }
	}
    }

    return 0;
}

private int
psd_write(psd_write_ctx *xc, const byte *buf, int size) {
    int code;

    code = fwrite(buf, 1, size, xc->f);
    if (code < 0)
	return code;
    return 0;
}

private int
psd_write_8(psd_write_ctx *xc, byte v)
{
    return psd_write(xc, (byte *)&v, 1);
}

private int
psd_write_16(psd_write_ctx *xc, bits16 v)
{
    bits16 buf;

    assign_u16(buf, v);
    return psd_write(xc, (byte *)&buf, 2);
}

private int
psd_write_32(psd_write_ctx *xc, bits32 v)
{
    bits32 buf;

    assign_u32(buf, v);
    return psd_write(xc, (byte *)&buf, 4);
}

private int
psd_write_header(psd_write_ctx *xc, psd_device *pdev)
{
    int code = 0;
    int bytes_pp = xc->num_channels;
    int chan_idx;
    int chan_names_len = 0;
    int sep_num;
    const devn_separation_name *separation_name;

    psd_write(xc, (const byte *)"8BPS", 4); /* Signature */
    psd_write_16(xc, 1); /* Version - Always equal to 1*/
    /* Reserved 6 Bytes - Must be zero */
    psd_write_32(xc, 0);
    psd_write_16(xc, 0);
    psd_write_16(xc, (bits16) bytes_pp); /* Channels (2 Bytes) - Supported range is 1 to 24 */
    psd_write_32(xc, xc->height); /* Rows */
    psd_write_32(xc, xc->width); /* Columns */
    psd_write_16(xc, 8); /* Depth - 1, 8 and 16 */
    psd_write_16(xc, (bits16) xc->base_bytes_pp); /* Mode - RGB=3, CMYK=4 */
    
    /* Color Mode Data */
    psd_write_32(xc, 0); 	/* No color mode data */

    /* Image Resources */

    /* Channel Names */
    for (chan_idx = NUM_CMYK_COMPONENTS; chan_idx < xc->num_channels; chan_idx++) {
	sep_num = xc->chnl_to_orig_sep[chan_idx] - NUM_CMYK_COMPONENTS;
	separation_name = &(pdev->devn_params.separations.names[sep_num]);
	chan_names_len += (separation_name->size + 1);
    }    
    psd_write_32(xc, 12 + (chan_names_len + (chan_names_len % 2))
			+ (12 + (14 * (xc->num_channels - xc->base_bytes_pp)))
			+ 28); 
    psd_write(xc, (const byte *)"8BIM", 4);
    psd_write_16(xc, 1006); /* 0x03EE */
    psd_write_16(xc, 0); /* PString */
    psd_write_32(xc, chan_names_len + (chan_names_len % 2));
    for (chan_idx = NUM_CMYK_COMPONENTS; chan_idx < xc->num_channels; chan_idx++) {
	sep_num = xc->chnl_to_orig_sep[chan_idx] - NUM_CMYK_COMPONENTS;
	separation_name = &(pdev->devn_params.separations.names[sep_num]);
	psd_write_8(xc, (byte) separation_name->size);
	psd_write(xc, separation_name->data, separation_name->size);
    }    
    if (chan_names_len % 2) 
	psd_write_8(xc, 0); /* pad */

    /* DisplayInfo - Colors for each spot channels */
    psd_write(xc, (const byte *)"8BIM", 4);
    psd_write_16(xc, 1007); /* 0x03EF */
    psd_write_16(xc, 0); /* PString */
    psd_write_32(xc, 14 * (xc->num_channels - xc->base_bytes_pp)); /* Length */
    for (chan_idx = NUM_CMYK_COMPONENTS; chan_idx < xc->num_channels; chan_idx++) {
	sep_num = xc->chnl_to_orig_sep[chan_idx] - NUM_CMYK_COMPONENTS;
	psd_write_16(xc, 02); /* CMYK */
	/* PhotoShop stores all component values as if they were additive. */
	if (pdev->equiv_cmyk_colors.color[sep_num].color_info_valid) {
#define convert_color(component) ((bits16)((65535 * ((double)\
    (frac_1 - pdev->equiv_cmyk_colors.color[sep_num].component)) / frac_1)))
	    psd_write_16(xc, convert_color(c)); /* Cyan */
	    psd_write_16(xc, convert_color(m)); /* Magenta */
	    psd_write_16(xc, convert_color(y)); /* Yellow */
	    psd_write_16(xc, convert_color(k)); /* Black */
#undef convert_color
	}
	else {	    /* Else set C = M = Y = 0, K = 1 */
	    psd_write_16(xc, 65535); /* Cyan */
	    psd_write_16(xc, 65535); /* Magenta */
	    psd_write_16(xc, 65535); /* Yellow */
	    psd_write_16(xc, 0); /* Black */
	}
	psd_write_16(xc, 0); /* Opacity 0 to 100 */
	psd_write_8(xc, 2); /* Don't know */
	psd_write_8(xc, 0); /* Padding - Always Zero */
    }

    /* Image resolution */
    psd_write(xc, (const byte *)"8BIM", 4);
    psd_write_16(xc, 1005); /* 0x03ED */
    psd_write_16(xc, 0); /* PString */
    psd_write_32(xc, 16); /* Length */
    		/* Resolution is specified as a fixed 16.16 bits */
    psd_write_32(xc, (int) (pdev->HWResolution[0] * 0x10000 + 0.5));
    psd_write_16(xc, 1);	/* width:  1 --> resolution is pixels per inch */
    psd_write_16(xc, 1);	/* width:  1 --> resolution is pixels per inch */
    psd_write_32(xc, (int) (pdev->HWResolution[1] * 0x10000 + 0.5));
    psd_write_16(xc, 1);	/* height:  1 --> resolution is pixels per inch */
    psd_write_16(xc, 1);	/* height:  1 --> resolution is pixels per inch */

    /* Layer and Mask information */
    psd_write_32(xc, 0); 	/* No layer or mask information */

    return code;
}

private void
psd_calib_row(psd_write_ctx *xc, byte **tile_data, const byte *row, 
		int channel, icmLuBase *luo)
{
    int base_bytes_pp = xc->base_bytes_pp;
    int n_extra_channels = xc->n_extra_channels;
    int channels = base_bytes_pp + n_extra_channels;
    int inn, outn;
    int x;
    double in[MAX_CHAN], out[MAX_CHAN];

    luo->spaces(luo, NULL, &inn, NULL, &outn, NULL, NULL, NULL, NULL);
	
    for (x = 0; x < xc->width; x++) {
	if (channel < outn) {
	    int plane_idx;

	    for (plane_idx = 0; plane_idx < inn; plane_idx++)
		in[plane_idx] = row[x*channels+plane_idx] * (1.0 / 255);	

	    (*tile_data)[x] = (int)(0.5 + 255 * out[channel]);
	    luo->lookup(luo, out, in);
	} else {
	    (*tile_data)[x] = 255 ^ row[x*channels+base_bytes_pp+channel];
	}
    }
}

/*
 * Output the image data for the PSD device.  The data for the PSD is
 * written in separate planes.  If the device is psdrgb then we simply
 * write three planes of RGB data.  The DeviceN parameters (SeparationOrder,
 * SeparationCOlorNames, and MaxSeparations) are not applied to the psdrgb
 * device.
 *
 * The DeviceN parameters are applied to the psdcmyk device.  If the
 * SeparationOrder parameter is not specified then first we write out the data
 * for the CMYK planes and then any separation planes.  If the SeparationOrder
 * parameter is specified, then things are more complicated.  Logically we
 * would simply write the planes specified by the SeparationOrder data.
 * However Photoshop expects there to be CMYK data.  First we will write out
 * four planes of data for CMYK.  If any of these colors are present in the
 * SeparationOrder data then the plane data will contain the color information.
 * If a color is not present then the plane data will be zero.  After the CMYK
 * data, we will write out any separation data which is specified in the
 * SeparationOrder data.
 */
private int
psd_write_image_data(psd_write_ctx *xc, gx_device_printer *pdev)
{
    int code = 0;
    int raster = gdev_prn_raster(pdev);
    int i, j;
    byte *line, *sep_line;
    int base_bytes_pp = xc->base_bytes_pp;
    int bytes_pp =pdev->color_info.num_components;
    int chan_idx;
    psd_device *xdev = (psd_device *)pdev;
    icmLuBase *luo = xdev->lu_out;
    byte *row;

    psd_write_16(xc, 0); /* Compression */

    line = gs_alloc_bytes(pdev->memory, raster, "psd_write_image_data");
    sep_line = gs_alloc_bytes(pdev->memory, xc->width, "psd_write_sep_line");

    /* Print the output planes */
    for (chan_idx = 0; chan_idx < xc->num_channels; chan_idx++) {
	for (j = 0; j < xc->height; ++j) {
	    int data_pos = xc->chnl_to_position[chan_idx];

	    /* Check if the separation is present in the SeparationOrder */
	    if (data_pos >= 0) {
	        code = gdev_prn_get_bits(pdev, j, line, &row);
	        if (luo == NULL) {
		    for (i = 0; i < xc->width; ++i) {
		        if (base_bytes_pp == 3) {
			    /* RGB */
			    sep_line[i] = row[i*bytes_pp + data_pos];
		        } else {
			    /* CMYK */
			    sep_line[i] = 255 - row[i*bytes_pp + data_pos];
		        }
		    }
	        } else {
		    psd_calib_row(xc, &sep_line, row, data_pos, luo);
	        }	
	        psd_write(xc, sep_line, xc->width);
	    } else {
		if (chan_idx < NUM_CMYK_COMPONENTS) {
		    /* Write empty process color */
		    for (i = 0; i < xc->width; ++i)
		        sep_line[i] = 255;
	            psd_write(xc, sep_line, xc->width);
		}
	    }	
	}
    }

    gs_free_object(pdev->memory, sep_line, "psd_write_sep_line");
    gs_free_object(pdev->memory, line, "psd_write_image_data");
    return code;
}

static int
psd_print_page(gx_device_printer *pdev, FILE *file)
{
    psd_write_ctx xc;

    xc.f = file;

    psd_setup(&xc, (psd_device *)pdev);
    psd_write_header(&xc, (psd_device *)pdev);
    psd_write_image_data(&xc, pdev);

    return 0;
}
