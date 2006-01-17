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

/*$Id: gdevxcf.c,v 1.10 2005/06/20 08:59:23 igor Exp $ */
/* Gimp (XCF) export device, supporting DeviceN color models. */

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

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* The device descriptor */
private dev_proc_get_params(xcf_get_params);
private dev_proc_put_params(xcf_put_params);
private dev_proc_print_page(xcf_print_page);
private dev_proc_map_color_rgb(xcf_map_color_rgb);
private dev_proc_get_color_mapping_procs(get_spotrgb_color_mapping_procs);
#if 0
private dev_proc_get_color_mapping_procs(get_spotcmyk_color_mapping_procs);
#endif
private dev_proc_get_color_mapping_procs(get_xcf_color_mapping_procs);
private dev_proc_get_color_comp_index(xcf_get_color_comp_index);
private dev_proc_encode_color(xcf_encode_color);
private dev_proc_decode_color(xcf_decode_color);

/*
 * Type definitions associated with the fixed color model names.
 */
typedef const char * fixed_colorant_name;
typedef fixed_colorant_name fixed_colorant_names_list[];

/*
 * Structure for holding SeparationNames and SeparationOrder elements.
 */
typedef struct gs_separation_names_s {
    int num_names;
    const gs_param_string * names[GX_DEVICE_COLOR_MAX_COMPONENTS];
} gs_separation_names;

/* This is redundant with color_info.cm_name. We may eliminate this
   typedef and use the latter string for everything. */
typedef enum {
    XCF_DEVICE_GRAY,
    XCF_DEVICE_RGB,
    XCF_DEVICE_CMYK,
    XCF_DEVICE_N
} xcf_color_model;

/*
 * A structure definition for a DeviceN type device
 */
typedef struct xcf_device_s {
    gx_device_common;
    gx_prn_device_common;

    /*        ... device-specific parameters ... */

    xcf_color_model color_model;

    /*
     * Bits per component (device colorant).  Currently only 1 and 8 are
     * supported.
     */
    int bitspercomponent;

    /*
     * Pointer to the colorant names for the color model.  This will be
     * null if we have DeviceN type device.  The actual possible colorant
     * names are those in this list plus those in the separation_names
     * list (below).
     */
    const fixed_colorant_names_list * std_colorant_names;
    int num_std_colorant_names;	/* Number of names in list */

    /*
    * Separation names (if any).
    */
    gs_separation_names separation_names;

    /*
     * Separation Order (if specified).
     */
    gs_separation_names separation_order;

    /* ICC color profile objects, for color conversion. */
    char profile_rgb_fn[256];
    icmLuBase *lu_rgb;
    int lu_rgb_outn;

    char profile_cmyk_fn[256];
    icmLuBase *lu_cmyk;
    int lu_cmyk_outn;

    char profile_out_fn[256];
    icmLuBase *lu_out;
} xcf_device;

/*
 * Macro definition for DeviceN procedures
 */
#define device_procs(get_color_mapping_procs)\
{	gdev_prn_open,\
	gx_default_get_initial_matrix,\
	NULL,				/* sync_output */\
	gdev_prn_output_page,		/* output_page */\
	gdev_prn_close,			/* close */\
	NULL,				/* map_rgb_color - not used */\
	xcf_map_color_rgb,		/* map_color_rgb */\
	NULL,				/* fill_rectangle */\
	NULL,				/* tile_rectangle */\
	NULL,				/* copy_mono */\
	NULL,				/* copy_color */\
	NULL,				/* draw_line */\
	NULL,				/* get_bits */\
	xcf_get_params,		/* get_params */\
	xcf_put_params,		/* put_params */\
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
	xcf_get_color_comp_index,	/* get_color_comp_index */\
	xcf_encode_color,		/* encode_color */\
	xcf_decode_color		/* decode_color */\
}


private const fixed_colorant_names_list DeviceGrayComponents = {
	"Gray",
	0		/* List terminator */
};

private const fixed_colorant_names_list DeviceRGBComponents = {
	"Red",
	"Green",
	"Blue",
	0		/* List terminator */
};

private const fixed_colorant_names_list DeviceCMYKComponents = {
	"Cyan",
	"Magenta",
	"Yellow",
	"Black",
	0		/* List terminator */
};


/*
 * Example device with RGB and spot color support
 */
private const gx_device_procs spot_rgb_procs = device_procs(get_spotrgb_color_mapping_procs);

const xcf_device gs_xcf_device =
{   
    prn_device_body_extended(xcf_device, spot_rgb_procs, "xcf",
	 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	 X_DPI, Y_DPI,		/* X and Y hardware resolution */
	 0, 0, 0, 0,		/* margins */
    	 GX_DEVICE_COLOR_MAX_COMPONENTS, 3,	/* MaxComponents, NumComp */
	 GX_CINFO_POLARITY_ADDITIVE,		/* Polarity */
	 24, 0,			/* Depth, Gray_index, */
	 255, 255, 256, 256,	/* MaxGray, MaxColor, DitherGray, DitherColor */
	 GX_CINFO_UNKNOWN_SEP_LIN, /* Let check_device_separable set up values */
	 "DeviceN",		/* Process color model name */
	 xcf_print_page),	/* Printer page print routine */
    /* DeviceN device specific parameters */
    XCF_DEVICE_RGB,		/* Color model */
    8,				/* Bits per color - must match ncomp, depth, etc. above */
    (&DeviceRGBComponents),	/* Names of color model colorants */
    3,				/* Number colorants for RGB */
    {0},			/* SeparationNames */
    {0}				/* SeparationOrder names */
};

private const gx_device_procs spot_cmyk_procs = device_procs(get_xcf_color_mapping_procs);

const xcf_device gs_xcfcmyk_device =
{   
    prn_device_body_extended(xcf_device, spot_cmyk_procs, "xcfcmyk",
	 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	 X_DPI, Y_DPI,		/* X and Y hardware resolution */
	 0, 0, 0, 0,		/* margins */
    	 GX_DEVICE_COLOR_MAX_COMPONENTS, 4,	/* MaxComponents, NumComp */
	 GX_CINFO_POLARITY_SUBTRACTIVE,		/* Polarity */
	 32, 0,			/* Depth, Gray_index, */
	 255, 255, 256, 256,	/* MaxGray, MaxColor, DitherGray, DitherColor */
	 GX_CINFO_UNKNOWN_SEP_LIN, /* Let check_device_separable set up values */
	 "DeviceN",		/* Process color model name */
	 xcf_print_page),	/* Printer page print routine */
    /* DeviceN device specific parameters */
    XCF_DEVICE_CMYK,		/* Color model */
    8,				/* Bits per color - must match ncomp, depth, etc. above */
    (&DeviceCMYKComponents),	/* Names of color model colorants */
    4,				/* Number colorants for RGB */
    {0},			/* SeparationNames */
    {0}				/* SeparationOrder names */
};

/*
 * The following procedures are used to map the standard color spaces into
 * the color components for the spotrgb device.
 */
private void
gray_cs_to_spotrgb_cm(gx_device * dev, frac gray, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    int i = ((xcf_device *)dev)->separation_names.num_names;

    out[0] = out[1] = out[2] = gray;
    for(; i>0; i--)			/* Clear spot colors */
        out[2 + i] = 0;
}

private void
rgb_cs_to_spotrgb_cm(gx_device * dev, const gs_imager_state *pis,
				  frac r, frac g, frac b, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    int i = ((xcf_device *)dev)->separation_names.num_names;

    out[0] = r;
    out[1] = g;
    out[2] = b;
    for(; i>0; i--)			/* Clear spot colors */
        out[2 + i] = 0;
}

private void
cmyk_cs_to_spotrgb_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    int i = ((xcf_device *)dev)->separation_names.num_names;

    color_cmyk_to_rgb(c, m, y, k, NULL, out);
    for(; i>0; i--)			/* Clear spot colors */
        out[2 + i] = 0;
}

private void
gray_cs_to_spotcmyk_cm(gx_device * dev, frac gray, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    int i = ((xcf_device *)dev)->separation_names.num_names;

    out[0] = out[1] = out[2] = 0;
    out[3] = frac_1 - gray;
    for(; i>0; i--)			/* Clear spot colors */
        out[3 + i] = 0;
}

private void
rgb_cs_to_spotcmyk_cm(gx_device * dev, const gs_imager_state *pis,
				   frac r, frac g, frac b, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    xcf_device *xdev = (xcf_device *)dev;
    int n = xdev->separation_names.num_names;
    int i;

    color_rgb_to_cmyk(r, g, b, pis, out);
    for(i = 0; i < n; i++)			/* Clear spot colors */
	out[4 + i] = 0;
}

private void
cmyk_cs_to_spotcmyk_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    xcf_device *xdev = (xcf_device *)dev;
    int n = xdev->separation_names.num_names;
    int i;

    out[0] = c;
    out[1] = m;
    out[2] = y;
    out[3] = k;
    for(i = 0; i < n; i++)			/* Clear spot colors */
	out[4 + i] = 0;
}

private void
cmyk_cs_to_spotn_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    xcf_device *xdev = (xcf_device *)dev;
    int n = xdev->separation_names.num_names;
    icmLuBase *luo = xdev->lu_cmyk;
    int i;

    if (luo != NULL) {
	double in[3];
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
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */

    cmyk_cs_to_spotn_cm(dev, 0, 0, 0, frac_1 - gray, out);
}

private void
rgb_cs_to_spotn_cm(gx_device * dev, const gs_imager_state *pis,
				   frac r, frac g, frac b, frac out[])
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    xcf_device *xdev = (xcf_device *)dev;
    int n = xdev->separation_names.num_names;
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

private const gx_cm_color_map_procs spotRGB_procs = {
    gray_cs_to_spotrgb_cm, rgb_cs_to_spotrgb_cm, cmyk_cs_to_spotrgb_cm
};

private const gx_cm_color_map_procs spotCMYK_procs = {
    gray_cs_to_spotcmyk_cm, rgb_cs_to_spotcmyk_cm, cmyk_cs_to_spotcmyk_cm
};

private const gx_cm_color_map_procs spotN_procs = {
    gray_cs_to_spotn_cm, rgb_cs_to_spotn_cm, cmyk_cs_to_spotn_cm
};

/*
 * These are the handlers for returning the list of color space
 * to color model conversion routines.
 */
private const gx_cm_color_map_procs *
get_spotrgb_color_mapping_procs(const gx_device * dev)
{
    return &spotRGB_procs;
}

#if 0
private const gx_cm_color_map_procs *
get_spotcmyk_color_mapping_procs(const gx_device * dev)
{
    return &spotCMYK_procs;
}
#endif


private const gx_cm_color_map_procs *
get_xcf_color_mapping_procs(const gx_device * dev)
{
    const xcf_device *xdev = (const xcf_device *)dev;

    if (xdev->color_model == XCF_DEVICE_RGB)
	return &spotRGB_procs;
    else if (xdev->color_model == XCF_DEVICE_CMYK)
	return &spotCMYK_procs;
    else if (xdev->color_model == XCF_DEVICE_N)
	return &spotN_procs;
    else
	return NULL;
}

/*
 * Encode a list of colorant values into a gx_color_index_value.
 */
private gx_color_index
xcf_encode_color(gx_device *dev, const gx_color_value colors[])
{
    int bpc = ((xcf_device *)dev)->bitspercomponent;
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
xcf_decode_color(gx_device * dev, gx_color_index color, gx_color_value * out)
{
    int bpc = ((xcf_device *)dev)->bitspercomponent;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    int mask = (1 << bpc) - 1;
    int i = 0;
    int ncomp = dev->color_info.num_components;

    for (; i<ncomp; i++) {
        out[ncomp - i - 1] = (color & mask) << drop;
	color >>= bpc;
    }
    return 0;
}

/*
 * Convert a gx_color_index to RGB.
 */
private int
xcf_map_color_rgb(gx_device *dev, gx_color_index color, gx_color_value rgb[3])
{
    xcf_device *xdev = (xcf_device *)dev;

    if (xdev->color_model == XCF_DEVICE_RGB)
	return xcf_decode_color(dev, color, rgb);
    /* TODO: return reasonable values. */
    rgb[0] = 0;
    rgb[1] = 0;
    rgb[2] = 0;
    return 0;
}

/*
 * This routine will extract a specified set of bits from a buffer and pack
 * them into a given buffer.
 *
 * Parameters:
 *   source - The source of the data
 *   dest - The destination for the data
 *   depth - The size of the bits per pixel - must be a multiple of 8
 *   first_bit - The location of the first data bit (LSB).
 *   bit_width - The number of bits to be extracted.
 *   npixel - The number of pixels.
 *
 * Returns:
 *   Length of the output line (in bytes)
 *   Data in dest.
 */
#if 0
private int
repack_data(byte * source, byte * dest, int depth, int first_bit,
		int bit_width, int npixel)
{
    int in_nbyte = depth >> 3;		/* Number of bytes per input pixel */
    int out_nbyte = bit_width >> 3;	/* Number of bytes per output pixel */
    gx_color_index mask = 1;
    gx_color_index data;
    int i, j, length = 0;
    int in_byte_loc = 0, out_byte_loc = 0;
    byte temp;
    byte * out = dest;
    int max_bit_byte = 8 - bit_width;

    mask = (mask << bit_width) - 1;
    for (i=0; i<npixel; i++) {
        /* Get the pixel data */
	if (!in_nbyte) {		/* Multiple pixels per byte */
	    data = *source;
	    data >>= in_byte_loc;
	    in_byte_loc += depth;
	    if (in_byte_loc >= 8) {	/* If finished with byte */
	        in_byte_loc = 0;
		source++;
	    }
	}
	else {				/* One or more bytes per pixel */
	    data = *source++;
	    for (j=1; j<in_nbyte; j++)
	        data = (data << 8) + *source++;
	}
	data >>= first_bit;
	data &= mask;

	/* Put the output data */
	if (!out_nbyte) {		/* Multiple pixels per byte */
	    temp = *out & ~(mask << out_byte_loc);
	    *out = temp | (data << out_byte_loc);
	    out_byte_loc += bit_width;
	    if (out_byte_loc > max_bit_byte) {	/* If finished with byte */
	        out_byte_loc = 0;
		out++;
	    }
	}
	else {				/* One or more bytes per pixel */
	    *out++ = data >> ((out_nbyte - 1) * 8);
	    for (j=1; j<out_nbyte; j++) {
	        *out++ = data >> ((out_nbyte - 1 - j) * 8);
	    }
	}
    }
    /* Return the number of bytes in the destination buffer. */
    length = out - dest;
    if (out_byte_loc)		 	/* If partially filled last byte */
	length++;
    return length;
}
#endif /* 0 */

private int
xcf_open_profile(xcf_device *xdev, char *profile_fn, icmLuBase **pluo,
		 int *poutn)
{
    icmFile *fp;
    icc *icco;
    icmLuBase *luo;

    dlprintf1("xcf_open_profile %s\n", profile_fn);
    fp = new_icmFileStd_name(profile_fn, (char *)"r");
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
xcf_open_profiles(xcf_device *xdev)
{
    int code = 0;
    if (xdev->lu_out == NULL && xdev->profile_out_fn[0]) {
	code = xcf_open_profile(xdev, xdev->profile_out_fn,
				    &xdev->lu_out, NULL);
    }
    if (code >= 0 && xdev->lu_rgb == NULL && xdev->profile_rgb_fn[0]) {
	code = xcf_open_profile(xdev, xdev->profile_rgb_fn,
				&xdev->lu_rgb, &xdev->lu_rgb_outn);
    }
    if (code >= 0 && xdev->lu_cmyk == NULL && xdev->profile_cmyk_fn[0]) {
	code = xcf_open_profile(xdev, xdev->profile_cmyk_fn,
				&xdev->lu_cmyk, &xdev->lu_cmyk_outn);
    }
    return code;
}

#define set_param_array(a, d, s)\
  (a.data = d, a.size = s, a.persistent = false);

/* Get parameters.  We provide a default CRD. */
private int
xcf_get_params(gx_device * pdev, gs_param_list * plist)
{
    xcf_device *xdev = (xcf_device *)pdev;
    int code;
    bool seprs = false;
    gs_param_string_array scna;
    gs_param_string pos;
    gs_param_string prgbs;
    gs_param_string pcmyks;

    set_param_array(scna, NULL, 0);

    if ( (code = gdev_prn_get_params(pdev, plist)) < 0 ||
         (code = sample_device_crd_get_params(pdev, plist, "CRDDefault")) < 0 ||
	 (code = param_write_name_array(plist, "SeparationColorNames", &scna)) < 0 ||
	 (code = param_write_bool(plist, "Separations", &seprs)) < 0)
	return code;

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

    pcmyks.data = (const byte *)xdev->profile_cmyk_fn,
	pcmyks.size = strlen(xdev->profile_cmyk_fn),
	pcmyks.persistent = false;
    code = param_write_string(plist, "ProfileCmyk", &prgbs);

    return code;
}
#undef set_param_array

#define compare_color_names(name, name_size, str, str_size) \
    (name_size == str_size && \
	(strncmp((const char *)name, (const char *)str, name_size) == 0))

/*
 * This routine will check if a name matches any item in a list of process model
 * color component names.
 */
private bool
check_process_color_names(const fixed_colorant_names_list * pcomp_list,
			  const gs_param_string * pstring)
{
    if (pcomp_list) {
        const fixed_colorant_name * plist = *pcomp_list;
        uint size = pstring->size;
    
	while( *plist) {
	    if (compare_color_names(*plist, strlen(*plist), pstring->data, size)) {
		return true;
	    }
	    plist++;
	}
    }
    return false;
}

/*
 * This utility routine calculates the number of bits required to store
 * color information.  In general the values are rounded up to an even
 * byte boundary except those cases in which mulitple pixels can evenly
 * into a single byte.
 *
 * The parameter are:
 *   ncomp - The number of components (colorants) for the device.  Valid
 * 	values are 1 to GX_DEVICE_COLOR_MAX_COMPONENTS
 *   bpc - The number of bits per component.  Valid values are 1, 2, 4, 5,
 *	and 8.
 * Input values are not tested for validity.
 */
static int
bpc_to_depth(int ncomp, int bpc)
{
    static const byte depths[4][8] = {
	{1, 2, 0, 4, 8, 0, 0, 8},
	{2, 4, 0, 8, 16, 0, 0, 16},
	{4, 8, 0, 16, 16, 0, 0, 24},
	{4, 8, 0, 16, 32, 0, 0, 32}
    };

    if (ncomp <=4 && bpc <= 8)
        return depths[ncomp -1][bpc-1];
    else
    	return (ncomp * bpc + 7) & 0xf8;
}

#define BEGIN_ARRAY_PARAM(pread, pname, pa, psize, e)\
    BEGIN\
    switch (code = pread(plist, (param_name = pname), &(pa))) {\
      case 0:\
	if ((pa).size != psize) {\
	  ecode = gs_note_error(gs_error_rangecheck);\
	  (pa).data = 0;	/* mark as not filled */\
	} else
#define END_ARRAY_PARAM(pa, e)\
	goto e;\
      default:\
	ecode = code;\
e:	param_signal_error(plist, param_name, ecode);\
      case 1:\
	(pa).data = 0;		/* mark as not filled */\
    }\
    END

private int
xcf_param_read_fn(gs_param_list *plist, const char *name,
		  gs_param_string *pstr, int max_len)
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

/* Compare a C string and a gs_param_string. */
static bool
param_string_eq(const gs_param_string *pcs, const char *str)
{
    return (strlen(str) == pcs->size &&
	    !strncmp(str, (const char *)pcs->data, pcs->size));
}

private int
xcf_set_color_model(xcf_device *xdev, xcf_color_model color_model)
{
    xdev->color_model = color_model;
    if (color_model == XCF_DEVICE_GRAY) {
	xdev->std_colorant_names = &DeviceGrayComponents;
	xdev->num_std_colorant_names = 1;
	xdev->color_info.cm_name = "DeviceGray";
	xdev->color_info.polarity = GX_CINFO_POLARITY_ADDITIVE;
    } else if (color_model == XCF_DEVICE_RGB) {
	xdev->std_colorant_names = &DeviceRGBComponents;
	xdev->num_std_colorant_names = 3;
	xdev->color_info.cm_name = "DeviceRGB";
	xdev->color_info.polarity = GX_CINFO_POLARITY_ADDITIVE;
    } else if (color_model == XCF_DEVICE_CMYK) {
	xdev->std_colorant_names = &DeviceCMYKComponents;
	xdev->num_std_colorant_names = 4;
	xdev->color_info.cm_name = "DeviceCMYK";
	xdev->color_info.polarity = GX_CINFO_POLARITY_SUBTRACTIVE;
    } else if (color_model == XCF_DEVICE_N) {
	xdev->std_colorant_names = &DeviceCMYKComponents;
	xdev->num_std_colorant_names = 4;
	xdev->color_info.cm_name = "DeviceN";
	xdev->color_info.polarity = GX_CINFO_POLARITY_SUBTRACTIVE;
    } else {
	return -1;
    }

    return 0;
}

/* Set parameters.  We allow setting the number of bits per component. */
private int
xcf_put_params(gx_device * pdev, gs_param_list * plist)
{
    xcf_device * const pdevn = (xcf_device *) pdev;
    gx_device_color_info save_info;
    gs_param_name param_name;
    int npcmcolors;
    int num_spot = pdevn->separation_names.num_names;
    int ecode = 0;
    int code;
    gs_param_string_array scna;
    gs_param_string po;
    gs_param_string prgb;
    gs_param_string pcmyk;
    gs_param_string pcm;
    xcf_color_model color_model = pdevn->color_model;

    BEGIN_ARRAY_PARAM(param_read_name_array, "SeparationColorNames", scna, scna.size, scne) {
	break;
    } END_ARRAY_PARAM(scna, scne);

    if (code >= 0)
	code = xcf_param_read_fn(plist, "ProfileOut", &po,
				 sizeof(pdevn->profile_out_fn));
    if (code >= 0)
	code = xcf_param_read_fn(plist, "ProfileRgb", &prgb,
				 sizeof(pdevn->profile_rgb_fn));
    if (code >= 0)
	code = xcf_param_read_fn(plist, "ProfileCmyk", &pcmyk,
				 sizeof(pdevn->profile_cmyk_fn));

    if (code >= 0)
	code = param_read_name(plist, "ProcessColorModel", &pcm);
    if (code == 0) {
	if (param_string_eq (&pcm, "DeviceGray"))
	    color_model = XCF_DEVICE_GRAY;
	else if (param_string_eq (&pcm, "DeviceRGB"))
	    color_model = XCF_DEVICE_RGB;
	else if (param_string_eq (&pcm, "DeviceCMYK"))
	    color_model = XCF_DEVICE_CMYK;
	else if (param_string_eq (&pcm, "DeviceN"))
	    color_model = XCF_DEVICE_N;
	else {
	    param_signal_error(plist, "ProcessColorModel",
			       code = gs_error_rangecheck);
	}
    }
    if (code < 0)
	ecode = code;

    /*
     * Save the color_info in case gdev_prn_put_params fails, and for
     * comparison.
     */
    save_info = pdevn->color_info;
    ecode = xcf_set_color_model(pdevn, color_model);
    if (ecode == 0)
	ecode = gdev_prn_put_params(pdev, plist);
    if (ecode < 0) {
	pdevn->color_info = save_info;
	return ecode;
    }

    /* Separations are only valid with a subrtractive color model */
    if (pdev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE) {
        /*
         * Process the separation color names.  Remove any names that already
	 * match the process color model colorant names for the device.
         */
        if (scna.data != 0) {
	    int i;
	    int num_names = scna.size;
	    const fixed_colorant_names_list * pcomp_names = 
	    			((xcf_device *)pdev)->std_colorant_names;

	    for (i = num_spot = 0; i < num_names; i++) {
	        if (!check_process_color_names(pcomp_names, &scna.data[i]))
	            pdevn->separation_names.names[num_spot++] = &scna.data[i];
	    }
	    pdevn->separation_names.num_names = num_spot;
	    if (pdevn->is_open)
	        gs_closedevice(pdev);
        }
        npcmcolors = pdevn->num_std_colorant_names;
        pdevn->color_info.num_components = npcmcolors + num_spot;
        /* 
         * The DeviceN device can have zero components if nothing has been
	 * specified.  This causes some problems so force at least one
	 * component until something is specified.
         */
        if (!pdevn->color_info.num_components)
	    pdevn->color_info.num_components = 1;
        pdevn->color_info.depth = bpc_to_depth(pdevn->color_info.num_components,
						pdevn->bitspercomponent);
        if (pdevn->color_info.depth != save_info.depth) {
	    gs_closedevice(pdev);
        }
    }

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
    code = xcf_open_profiles(pdevn);

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
xcf_get_color_comp_index(gx_device * dev, const char * pname, int name_size,
					int component_type)
{
/* TO_DO_DEVICEN  This routine needs to include the effects of the SeparationOrder array */
    const fixed_colorant_names_list * list = ((const xcf_device *)dev)->std_colorant_names;
    const fixed_colorant_name * pcolor = *list;
    int color_component_number = 0;
    int i;

    /* Check if the component is in the implied list. */
    if (pcolor) {
	while( *pcolor) {
	    if (compare_color_names(pname, name_size, *pcolor, strlen(*pcolor)))
		return color_component_number;
	    pcolor++;
	    color_component_number++;
	}
    }

    /* Check if the component is in the separation names list. */
    {
	const gs_separation_names * separations = &((const xcf_device *)dev)->separation_names;
	int num_spot = separations->num_names;

	for (i=0; i<num_spot; i++) {
	    if (compare_color_names((const char *)separations->names[i]->data,
		  separations->names[i]->size, pname, name_size)) {
		return color_component_number;
	    }
	    color_component_number++;
	}
    }

    return -1;
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
    int offset;

    int width;
    int height;
    int base_bytes_pp; /* almost always 3 (rgb) */
    int n_extra_channels;

    int n_tiles_x;
    int n_tiles_y;
    int n_tiles;
    int n_levels;

    /* byte offset of image data */
    int image_data_off;
} xcf_write_ctx;

#define TILE_WIDTH 64
#define TILE_HEIGHT 64

private int
xcf_calc_levels(int size, int tile_size)
{
    int levels = 1;
    while (size > tile_size) {
	size >>= 1;
	levels++;
    }
    return levels;
}

private int
xcf_setup_tiles(xcf_write_ctx *xc, xcf_device *dev)
{
    xc->base_bytes_pp = 3;
    xc->n_extra_channels = dev->separation_names.num_names;
    xc->width = dev->width;
    xc->height = dev->height;
    xc->n_tiles_x = (dev->width + TILE_WIDTH - 1) / TILE_WIDTH;
    xc->n_tiles_y = (dev->height + TILE_HEIGHT - 1) / TILE_HEIGHT;
    xc->n_tiles = xc->n_tiles_x * xc->n_tiles_y;
    xc->n_levels = max(xcf_calc_levels(dev->width, TILE_WIDTH),
		       xcf_calc_levels(dev->height, TILE_HEIGHT));

    return 0;
}

/* Return value: Size of tile in pixels. */
private int
xcf_tile_sizeof(xcf_write_ctx *xc, int tile_idx)
{
    int tile_i = tile_idx % xc->n_tiles_x;
    int tile_j = tile_idx / xc->n_tiles_x;
    int tile_size_x = min(TILE_WIDTH, xc->width - tile_i * TILE_WIDTH);
    int tile_size_y = min(TILE_HEIGHT, xc->height - tile_j * TILE_HEIGHT);
    return tile_size_x * tile_size_y;
}

private int
xcf_write(xcf_write_ctx *xc, const byte *buf, int size) {
    int code;

    code = fwrite(buf, 1, size, xc->f);
    if (code < 0)
	return code;
    xc->offset += code;
    return 0;
}

private int
xcf_write_32(xcf_write_ctx *xc, bits32 v)
{
    bits32 buf;

    assign_u32(buf, v);
    return xcf_write(xc, (byte *)&buf, 4);
}

private int
xcf_write_image_props(xcf_write_ctx *xc)
{
    int code = 0;

    xcf_write_32(xc, 0);
    xcf_write_32(xc, 0);

    return code;
}

/**
 * Return value: Number of bytes needed to write layer.
 **/
private int
xcf_base_size(xcf_write_ctx *xc, const char *layer_name)
{
    int bytes_pp = xc->base_bytes_pp + xc->n_extra_channels;

    return 17 + strlen (layer_name) +		/* header and name */
	8 +					/* layer props */
	12 + xc->n_levels * 16 +		/* layer tile hierarchy */
	12 + xc->n_tiles * 4 +			/* tile offsets */
	xc->width * xc->height * bytes_pp;	/* image data */
}


private int
xcf_channel_size(xcf_write_ctx *xc, int name_size)
{
    return 17 + name_size +			/* header and name */
	8 +					/* channel props */
	4 + xc->n_levels * 16 +			/* channel tile hiearchy */
	12 + xc->n_tiles * 4;			/* tile offsets */
}

private int
xcf_write_header(xcf_write_ctx *xc, xcf_device *pdev)
{
    int code = 0;
    const char *layer_name = "Background";
    int level;
    int tile_offset;
    int tile_idx;
    int n_extra_channels = xc->n_extra_channels;
    int bytes_pp = xc->base_bytes_pp + n_extra_channels;
    int channel_idx;

    xcf_write(xc, (const byte *)"gimp xcf file", 14);
    xcf_write_32(xc, xc->width);
    xcf_write_32(xc, xc->height);
    xcf_write_32(xc, 0);

    xcf_write_image_props(xc);

    /* layer offsets */
    xcf_write_32(xc, xc->offset + 12 + 4 * n_extra_channels);
    xcf_write_32(xc, 0);

    /* channel offsets */
    tile_offset = xc->offset + 4 + 4 * n_extra_channels +
	xcf_base_size(xc, layer_name);
    for (channel_idx = 0; channel_idx < n_extra_channels; channel_idx++) {
	const gs_param_string *separation_name =
	    pdev->separation_names.names[channel_idx];
	dlprintf1("tile offset: %d\n", tile_offset);
	xcf_write_32(xc, tile_offset);
	tile_offset += xcf_channel_size(xc, separation_name->size);
    }
    xcf_write_32(xc, 0);

    /* layer */
    xcf_write_32(xc, xc->width);
    xcf_write_32(xc, xc->height);
    xcf_write_32(xc, 0);
    xcf_write_32(xc, strlen(layer_name) + 1);
    xcf_write(xc, (const byte *)layer_name, strlen(layer_name) + 1);

    /* layer props */
    xcf_write_32(xc, 0);
    xcf_write_32(xc, 0);

    /* layer tile hierarchy */
    xcf_write_32(xc, xc->offset + 8);
    xcf_write_32(xc, 0);

    xcf_write_32(xc, xc->width);
    xcf_write_32(xc, xc->height);
    xcf_write_32(xc, xc->base_bytes_pp);
    xcf_write_32(xc, xc->offset + (1 + xc->n_levels) * 4);
    tile_offset = xc->offset + xc->width * xc->height * bytes_pp +
	xc->n_tiles * 4 + 12;
    for (level = 1; level < xc->n_levels; level++) {
	xcf_write_32(xc, tile_offset);
	tile_offset += 12;
    }
    xcf_write_32(xc, 0);

    /* layer tile offsets */
    xcf_write_32(xc, xc->width);
    xcf_write_32(xc, xc->height);
    tile_offset = xc->offset + (xc->n_tiles + 1) * 4;
    for (tile_idx = 0; tile_idx < xc->n_tiles; tile_idx++) {
	xcf_write_32(xc, tile_offset);
	tile_offset += xcf_tile_sizeof(xc, tile_idx) * bytes_pp;
    }
    xcf_write_32(xc, 0);

    xc->image_data_off = xc->offset;

    return code;
}

private void
xcf_shuffle_to_tile(xcf_write_ctx *xc, byte **tile_data, const byte *row,
		    int y)
{
    int tile_j = y / TILE_HEIGHT;
    int yrem = y % TILE_HEIGHT;
    int tile_i;
    int base_bytes_pp = xc->base_bytes_pp;
    int n_extra_channels = xc->n_extra_channels;
    int row_idx = 0;

    for (tile_i = 0; tile_i < xc->n_tiles_x; tile_i++) {
	int x;
	int tile_width = min(TILE_WIDTH, xc->width - tile_i * TILE_WIDTH);
	int tile_height = min(TILE_HEIGHT, xc->height - tile_j * TILE_HEIGHT);
	byte *base_ptr = tile_data[tile_i] +
	    yrem * tile_width * base_bytes_pp;
	int extra_stride = tile_width * tile_height;
	byte *extra_ptr = tile_data[tile_i] + extra_stride * base_bytes_pp +
	    yrem * tile_width;
	    
	int base_idx = 0;

	for (x = 0; x < tile_width; x++) {
	    int plane_idx;
	    for (plane_idx = 0; plane_idx < base_bytes_pp; plane_idx++)
		base_ptr[base_idx++] = row[row_idx++];
	    for (plane_idx = 0; plane_idx < n_extra_channels; plane_idx++)
		extra_ptr[plane_idx * extra_stride + x] = 255 ^ row[row_idx++];
	}
    }
}

private void
xcf_icc_to_tile(xcf_write_ctx *xc, byte **tile_data, const byte *row,
		    int y, icmLuBase *luo)
{
    int tile_j = y / TILE_HEIGHT;
    int yrem = y % TILE_HEIGHT;
    int tile_i;
    int base_bytes_pp = xc->base_bytes_pp;
    int n_extra_channels = xc->n_extra_channels;
    int row_idx = 0;
    int inn, outn;

    luo->spaces(luo, NULL, &inn, NULL, &outn, NULL, NULL, NULL, NULL);

    for (tile_i = 0; tile_i < xc->n_tiles_x; tile_i++) {
	int x;
	int tile_width = min(TILE_WIDTH, xc->width - tile_i * TILE_WIDTH);
	int tile_height = min(TILE_HEIGHT, xc->height - tile_j * TILE_HEIGHT);
	byte *base_ptr = tile_data[tile_i] +
	    yrem * tile_width * base_bytes_pp;
	int extra_stride = tile_width * tile_height;
	byte *extra_ptr = tile_data[tile_i] + extra_stride * base_bytes_pp +
	    yrem * tile_width;
	double in[MAX_CHAN], out[MAX_CHAN];
	    
	int base_idx = 0;

	for (x = 0; x < tile_width; x++) {
	    int plane_idx;

	    for (plane_idx = 0; plane_idx < inn; plane_idx++)
		in[plane_idx] = row[row_idx++] * (1.0 / 255);
	    luo->lookup(luo, out, in);
	    for (plane_idx = 0; plane_idx < outn; plane_idx++)
		base_ptr[base_idx++] = (int)(0.5 + 255 * out[plane_idx]);
	    for (plane_idx = 0; plane_idx < n_extra_channels; plane_idx++)
		extra_ptr[plane_idx * extra_stride + x] = 255 ^ row[row_idx++];
	}
    }
}

private int
xcf_write_image_data(xcf_write_ctx *xc, gx_device_printer *pdev)
{
    int code = 0;
    int raster = gdev_prn_raster(pdev);
    int tile_i, tile_j;
    byte **tile_data;
    byte *line;
    int base_bytes_pp = xc->base_bytes_pp;
    int n_extra_channels = xc->n_extra_channels;
    int bytes_pp = base_bytes_pp + n_extra_channels;
    int chan_idx;
    xcf_device *xdev = (xcf_device *)pdev;
    icmLuBase *luo = xdev->lu_out;

    line = gs_alloc_bytes(pdev->memory, raster, "xcf_write_image_data");
    tile_data = (byte **)gs_alloc_bytes(pdev->memory,
					xc->n_tiles_x * sizeof(byte *),
					"xcf_write_image_data");
    for (tile_i = 0; tile_i < xc->n_tiles_x; tile_i++) {
	int tile_bytes = xcf_tile_sizeof(xc, tile_i) * bytes_pp;

	tile_data[tile_i] = gs_alloc_bytes(pdev->memory, tile_bytes,
					   "xcf_write_image_data");
    }
    for (tile_j = 0; tile_j < xc->n_tiles_y; tile_j++) {
	int y0, y1;
	int y;
	byte *row;

	y0 = tile_j * TILE_HEIGHT;
	y1 = min(xc->height, y0 + TILE_HEIGHT);
	for (y = y0; y < y1; y++) {
	    code = gdev_prn_get_bits(pdev, y, line, &row);
	    if (luo == NULL)
		xcf_shuffle_to_tile(xc, tile_data, row, y);
	    else
		xcf_icc_to_tile(xc, tile_data, row, y, luo);
	}
	for (tile_i = 0; tile_i < xc->n_tiles_x; tile_i++) {
	    int tile_idx = tile_j * xc->n_tiles_x + tile_i;
	    int tile_size = xcf_tile_sizeof(xc, tile_idx);
	    int base_size = tile_size * base_bytes_pp;

	    xcf_write(xc, tile_data[tile_i], base_size);
	    for (chan_idx = 0; chan_idx < n_extra_channels; chan_idx++) {
		xcf_write(xc, tile_data[tile_i] + base_size +
			  tile_size * chan_idx, tile_size);
	    }
	}
    }

    for (tile_i = 0; tile_i < xc->n_tiles_x; tile_i++) {
	gs_free_object(pdev->memory, tile_data[tile_i],
		"xcf_write_image_data");
    }
    gs_free_object(pdev->memory, tile_data, "xcf_write_image_data");
    gs_free_object(pdev->memory, line, "xcf_write_image_data");
    return code;
}

private int
xcf_write_fake_hierarchy(xcf_write_ctx *xc)
{
    int widthf = xc->width, heightf = xc->height;
    int i;

    for (i = 1; i < xc->n_levels; i++) {
	widthf >>= 1;
	heightf >>= 1;
	xcf_write_32(xc, widthf);
	xcf_write_32(xc, heightf);
	xcf_write_32(xc, 0);
    }
    return 0;
}

private int
xcf_write_footer(xcf_write_ctx *xc, xcf_device *pdev)
{
    int code = 0;
    int base_bytes_pp = xc->base_bytes_pp;
    int n_extra_channels = xc->n_extra_channels;
    int bytes_pp = base_bytes_pp + n_extra_channels;
    int chan_idx;

    xcf_write_fake_hierarchy(xc);

    for (chan_idx = 0; chan_idx < xc->n_extra_channels; chan_idx++) {
	const gs_param_string *separation_name =
	    pdev->separation_names.names[chan_idx];
	byte nullbyte[] = { 0 };
	int level;
	int offset;
	int tile_idx;

	dlprintf2("actual tile offset: %d %d\n", xc->offset, (int)arch_sizeof_color_index);
	xcf_write_32(xc, xc->width);
	xcf_write_32(xc, xc->height);
	xcf_write_32(xc, separation_name->size + 1);
	xcf_write(xc, separation_name->data, separation_name->size);
	xcf_write(xc, nullbyte, 1);

	/* channel props */
	xcf_write_32(xc, 0);
	xcf_write_32(xc, 0);

	/* channel tile hierarchy */
	xcf_write_32(xc, xc->offset + 4);

	xcf_write_32(xc, xc->width);
	xcf_write_32(xc, xc->height);
	xcf_write_32(xc, 1);
	xcf_write_32(xc, xc->offset + xc->n_levels * 16 - 8);
	offset = xc->offset + xc->n_levels * 4;
	for (level = 1; level < xc->n_levels; level++) {
	    xcf_write_32(xc, offset);
	    offset += 12;
	}
	xcf_write_32(xc, 0);
	xcf_write_fake_hierarchy(xc);

	/* channel tile offsets */
	xcf_write_32(xc, xc->width);
	xcf_write_32(xc, xc->height);
	offset = xc->image_data_off;
	for (tile_idx = 0; tile_idx < xc->n_tiles; tile_idx++) {
	    int tile_size = xcf_tile_sizeof(xc, tile_idx);

	    xcf_write_32(xc, offset + (base_bytes_pp + chan_idx) * tile_size);
	    offset += bytes_pp * tile_size;
	}
	xcf_write_32(xc, 0);
	
    }
    return code;
}

static int
xcf_print_page(gx_device_printer *pdev, FILE *file)
{
    xcf_write_ctx xc;

    xc.f = file;
    xc.offset = 0;

    xcf_setup_tiles(&xc, (xcf_device *)pdev);
    xcf_write_header(&xc, (xcf_device *)pdev);
    xcf_write_image_data(&xc, pdev);
    xcf_write_footer(&xc, (xcf_device *)pdev);

    return 0;
}
