/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gdevpsdp.c,v 1.2 2000/03/10 04:16:09 lpd Exp $ */
/* (Distiller) parameter handling for PostScript and PDF writers */
#include "string_.h"
#include "jpeglib_.h"		/* for sdct.h */
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"
#include "gxdevice.h"
#include "gsparamx.h"
#include "gdevpsdf.h"
#include "strimpl.h"		/* for short-sighted compilers */
#include "scfx.h"
#include "sdct.h"
#include "slzwx.h"
#include "spprint.h"
#include "srlx.h"
#include "szlibx.h"

/* Define a (bogus) GC descriptor for gs_param_string. */
/* The only ones we use are GC-able and not persistent. */
gs_private_st_composite(st_gs_param_string, gs_param_string, "gs_param_string",
			param_string_enum_ptrs, param_string_reloc_ptrs);
private
ENUM_PTRS_WITH(param_string_enum_ptrs, gs_param_string *pstr) return 0;
case 0: return ENUM_CONST_STRING(pstr);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(param_string_reloc_ptrs, gs_param_string *pstr)
{
    gs_const_string str;

    str.data = pstr->data, str.size = pstr->size;
    RELOC_CONST_STRING_VAR(str);
    pstr->data = str.data;
}
RELOC_PTRS_END
gs_private_st_element(st_param_string_element, gs_param_string,
		      "gs_param_string[]", param_string_elt_enum_ptrs,
		      param_string_elt_reloc_ptrs, st_gs_param_string);


/* ---------------- Get/put Distiller parameters ---------------- */

/*
 * This code handles all the Distiller parameters except the *ACSDict and
 * *ImageDict parameter dictionaries.  (It doesn't cause any of the
 * parameters actually to have any effect.)
 */

/*
 * ColorConversionStrategy is supposed to affect output color space
 * according to the following table.  ****** NOT IMPLEMENTED YET ******

PS Input:  LeaveCU UseDIC           UseDICFI         sRGB
Gray art   Gray    CalGray/ICCBased Gray             Gray
Gray image Gray    CalGray/ICCBased CalGray/ICCBased Gray
RGB art    RGB     CalGray/ICCBased RGB              CalRGB/sRGB
RGB image  RGB     CalGray/ICCBased CalRGB/ICCBased  CalRGB/sRGB
CMYK art   CMYK    LAB/ICCBased     CMYK             CalRGB/sRGB
CMYK image CMYK    LAB/ICCBased     LAB/ICCBased     CalRGB/sRGB
CIE art    Cal/ICC Cal/ICC          Cal/ICC          CalRGB/sRGB
CIE image  Cal/ICC Cal/ICC          Cal/ICC          CalRGB/sRGB

 */

/*
 * The Always/NeverEmbed parameters are defined as being incremental.  Since
 * this isn't compatible with the general property of page devices that if
 * you do a getpagedevice, doing a setpagedevice later will restore the same
 * state, we actually define the parameters in sets of 3:
 *	- AlwaysEmbed is used for incremental additions.
 *	- ~AlwaysEmbed is used for incremental deletions.
 *	- .AlwaysEmbed is used for the complete list.
 * and analogously for NeverEmbed.
 */

typedef struct psdf_image_filter_name_s {
    const char *pname;
    const stream_template *template;
    psdf_version min_version;
} psdf_image_filter_name;
typedef struct psdf_image_param_names_s {
    const char *ACSDict;	/* not used for mono */
    const char *AntiAlias;
    const char *AutoFilter;	/* not used for mono */
    const char *Depth;
    const char *Dict;
    const char *Downsample;
    const char *DownsampleType;
#ifdef POST60
    const char *DownsampleThreshold;
#endif
    const char *Encode;
    const char *Filter;
    const char *Resolution;
} psdf_image_param_names;
private const psdf_image_param_names Color_names = {
    "ColorACSImageDict", "AntiAliasColorImages", "AutoFilterColorImages",
    "ColorImageDepth", "ColorImageDict",
    "DownsampleColorImages", "ColorImageDownsampleType",
#ifdef POST60
    "ColorImageDownsampleThreshold",
#endif
    "EncodeColorImages", "ColorImageFilter", "ColorImageResolution"
};
private const psdf_image_filter_name Poly_filters[] = {
    {"DCTEncode", &s_DCTE_template},
    {"FlateEncode", &s_zlibE_template, psdf_version_ll3},
    {"LZWEncode", &s_LZWE_template},
    {0, 0}
};
private const psdf_image_param_names Gray_names = {
    "GrayACSImageDict", "AntiAliasGrayImages", "AutoFilterGrayImages",
    "GrayImageDepth", "GrayImageDict",
    "DownsampleGrayImages", "GrayImageDownsampleType",
#ifdef POST60
    "GrayImageDownsampleThreshold",
#endif
    "EncodeGrayImages", "GrayImageFilter", "GrayImageResolution"
};
private const psdf_image_param_names Mono_names = {
    0, "AntiAliasMonoImages", 0,
    "MonoImageDepth", "MonoImageDict",
    "DownsampleMonoImages", "MonoImageDownsampleType",
#ifdef POST60
    "MonoImageDownsampleThreshold",
#endif
    "EncodeMonoImages", "MonoImageFilter", "MonoImageResolution"
};
private const psdf_image_filter_name Mono_filters[] = {
    {"CCITTFaxEncode", &s_CFE_template},
    {"FlateEncode", &s_zlibE_template, psdf_version_ll3},
    {"LZWEncode", &s_LZWE_template},
    {"RunLengthEncode", &s_RLE_template},
    {0, 0}
};
private const char *const AutoRotatePages_names[] = {
    "None", "All", "PageByPage", 0
};
private const char *const ColorConversionStrategy_names[] = {
    "LeaveColorUnchanged",
#ifdef POST60
    "UseDeviceIndependentColor",
    "UseDeviceIndependentColorForImages",
    "sRGB",
#else
    "UseDeviceDependentColor",
    "UseDeviceIndependentColor",
#endif
    0
};
private const char *const DownsampleType_names[] = {
    "Average",
    "Bicubic",
    "Subsample", 0
};
#ifdef POST60
private const char *const Binding_names[] = {
    "Left", "Right", 0
};
private const char *const DefaultRenderingIntent_names[] = {
    "Default", "Perceptual", "Saturation", "RelativeColorimetric",
    "AbsoluteColorimetric", 0
};
#endif
private const char *const TransferFunctionInfo_names[] = {
    "Preserve", "Apply", "Remove", 0
};
private const char *const UCRandBGInfo_names[] = {
    "Preserve", "Remove", 0
};
#ifdef POST60
private const char *const CannotEmbedFontPolicy_names[] = {
    "OK", "Warning", "Error", 0
};
#endif

#ifdef POST60
private const gs_param_item_t psdf_param_items[] = {
#define pi(key, type, memb) { key, type, offset_of(gx_device_psdf, params.memb) }

    /* General parameters */

    pi("ASCII85EncodePages", gs_param_type_bool, ASCII85EncodePages),
    /* (AutoRotatePages) */
    /* (Binding) */
    pi("CompressPages", gs_param_type_bool, CompressPages),
    /* (DefaultRenderingIntent) */
    pi("DetectBlends", gs_param_type_bool, DetectBlends),
    pi("DoThumbnails", gs_param_type_bool, DoThumbnails),
    pi("EndPage", gs_param_type_int, EndPage),
    pi("ImageMemory", gs_param_type_long, ImageMemory),
    pi("LockDistillerParams", gs_param_type_bool, LockDistillerParams),
    pi("LZWEncodePages", gs_param_type_bool, LZWEncodePages),
    pi("OPM", gs_param_type_int, OPM),
    pi("PreserveHalftoneInfo", gs_param_type_bool, PreserveHalftoneInfo),
    pi("PreserveOPIComments", gs_param_type_bool, PreserveOPIComments),
    pi("PreserveOverprintSettings", gs_param_type_bool, PreserveOverprintSettings),
    pi("StartPage", gs_param_type_int, StartPage),
    /* (TransferFunctionInfo) */
    /* (UCRandBGInfo) */
    pi("UseFlateCompression", gs_param_type_bool, UseFlateCompression),

    /* Color image processing parameters */

    pi("ConvertCMYKImagesToRGB", gs_param_type_bool, ConvertCMYKImagesToRGB),
    pi("ConvertImagesToIndexed", gs_param_type_bool, ConvertImagesToIndexed),

    /* Font embedding parameters */

    pi("EmbedAllFonts", gs_param_type_bool, EmbedAllFonts),
    pi("MaxSubsetPct", gs_param_type_int, MaxSubsetPct),

#undef pi
    gs_param_item_end
};
#endif

/* -------- Get parameters -------- */

extern stream_state_proc_get_params(s_DCTE_get_params, stream_DCT_state);
extern stream_state_proc_get_params(s_CF_get_params, stream_CF_state);
typedef stream_state_proc_get_params((*ss_get_params_t), stream_state);

#ifdef POST60
private int
psdf_write_name(gs_param_list *plist, const char *key, const char *str)
{
    gs_param_string pstr;

    param_string_from_string(pstr, str);
    return param_write_name(plist, key, &pstr);
}
#endif

private int
psdf_CF_get_params(gs_param_list * plist, const stream_state * ss, bool all)
{
    return (ss == 0 ? 0 :
	    s_CF_get_params(plist, (const stream_CF_state *)ss, all));
}
private int
psdf_DCT_get_params(gs_param_list * plist, const stream_state * ss, bool all)
{
    int code = (ss == 0 ? 0 :
		s_DCTE_get_params(plist, (const stream_DCT_state *)ss, all));
    /*
     * Add dummy Columns, Rows, and Colors parameters so that put_params
     * won't complain.
     */
    int dummy_size = 8, dummy_colors = 3;

    if (code < 0 ||
	(code = param_write_int(plist, "Columns", &dummy_size)) < 0 ||
	(code = param_write_int(plist, "Rows", &dummy_size)) < 0 ||
	(code = param_write_int(plist, "Colors", &dummy_colors)) < 0
	)
	return code;
    return 0;
}

/*
 * Get an image Dict parameter.  Note that we return a default (usually
 * empty) dictionary if the parameter has never been set.
 */
private int
psdf_get_image_dict_param(gs_param_list * plist, const gs_param_name pname,
			  stream_state * ss, ss_get_params_t get_params)
{
    gs_param_dict dict;
    int code;

    if (pname == 0)
	return 0;
    dict.size = 12;		/* enough for all param dicts we know about */
    if ((code = param_begin_write_dict(plist, pname, &dict, false)) < 0)
	return code;
    code = (*get_params)(dict.list, ss, false);
    param_end_write_dict(plist, pname, &dict);
    return code;
}

/* Get a set of image-related parameters. */
private int
psdf_get_image_params(gs_param_list * plist,
	  const psdf_image_param_names * pnames, psdf_image_params * params)
{
    int code;
    gs_param_string dsts, fs;

    param_string_from_string(dsts,
			     DownsampleType_names[params->DownsampleType]);
    if (
	   (code = psdf_get_image_dict_param(plist, pnames->ACSDict,
					     params->ACSDict,
					     psdf_DCT_get_params)) < 0 ||
	   (code = param_write_bool(plist, pnames->AntiAlias,
				    &params->AntiAlias)) < 0 ||
	   (pnames->AutoFilter != 0 &&
	    (code = param_write_bool(plist, pnames->AutoFilter,
				     &params->AutoFilter)) < 0) ||
	   (code = param_write_int(plist, pnames->Depth,
				   &params->Depth)) < 0 ||
	   (code = psdf_get_image_dict_param(plist, pnames->Dict,
					     params->Dict,
					     (params->Dict == 0 ||
					      params->Dict->template ==
					      &s_CFE_template ?
					      psdf_CF_get_params :
					      psdf_DCT_get_params))) < 0 ||
	   (code = param_write_bool(plist, pnames->Downsample,
				    &params->Downsample)) < 0 ||
	   (code = param_write_name(plist, pnames->DownsampleType,
				    &dsts)) < 0 ||
#ifdef POST60
	   /****** RANGE = 1.0 TO 10.0, DEFAULT 1.5 FOR COLOR, ELSE 2.0 ******/
	   (code = param_write_float(plist, pnames->DownsampleThreshold,
				     &params->DownsampleThreshold)) < 0 ||
#endif
	   (code = param_write_bool(plist, pnames->Encode,
				    &params->Encode)) < 0 ||
	   (code = (params->Filter == 0 ? 0 :
		    (param_string_from_string(fs, params->Filter),
		     param_write_name(plist, pnames->Filter, &fs)))) < 0 ||
	   (code = param_write_int(plist, pnames->Resolution,
				   &params->Resolution)) < 0
	)
	DO_NOTHING;
    return code;
}

/* Get a font embedding parameter. */
private int
psdf_get_embed_param(gs_param_list *plist, gs_param_name allpname,
		     const gs_param_string_array *psa)
{
    int code = param_write_name_array(plist, allpname, psa);

    if (code >= 0)
	code = param_write_name_array(plist, allpname + 1, psa);
    return code;
}

/* Get parameters. */
int
gdev_psdf_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_psdf *pdev = (gx_device_psdf *) dev;
    int code = gdev_vector_get_params(dev, plist);
    gs_param_string arps, ccss, tfis, ucrbgis;
#ifdef POST60
    gs_param_string ccpros, cgpros, crpros, srpros;
#endif

    if (code < 0)
	return code;
    param_string_from_string(arps,
	AutoRotatePages_names[(int)pdev->params.AutoRotatePages]);
    param_string_from_string(ccss,
	ColorConversionStrategy_names[(int)pdev->params.ColorConversionStrategy]);
    param_string_from_string(tfis,
	TransferFunctionInfo_names[(int)pdev->params.TransferFunctionInfo]);
    param_string_from_string(ucrbgis,
	UCRandBGInfo_names[(int)pdev->params.UCRandBGInfo]);
#ifdef POST60
#define SET_PARAM_STRING(ps, e)\
  (ps.data = pdev->params.e.data, ps.size = pdev->params.e.size,\
   ps.persistent = false)
    SET_PARAM_STRING(ccpros, CalCMYKProfile);
    SET_PARAM_STRING(cgpros, CalGrayProfile);
    SET_PARAM_STRING(crpros, CalRGBProfile);
    SET_PARAM_STRING(srpros, sRGBProfile);
#undef SET_PARAM_STRING
#endif

    if (
#ifdef POST60
	(code = gs_param_write_items(plist, pdev, NULL, psdf_param_items)) < 0 ||
#endif

    /* General parameters */

#ifndef POST60
	(code = param_write_bool(plist, "ASCII85EncodePages",
				 &pdev->params.ASCII85EncodePages)) < 0 ||
#endif
	(code = param_write_name(plist, "AutoRotatePages",
				 &arps)) < 0 ||
#ifdef POST60
	(code = psdf_write_name(plist, "Binding",
		Binding_names[(int)pdev->params.Binding])) < 0 ||
	(code = psdf_write_name(plist, "DefaultRenderingIntent",
		DefaultRenderingIntent_names[(int)pdev->params.DefaultRenderingIntent])) < 0 ||
#else
	(code = param_write_bool(plist, "CompressPages",
				 &pdev->params.CompressPages)) < 0 ||
	(code = param_write_long(plist, "ImageMemory",
				 &pdev->params.ImageMemory)) < 0 ||
	(code = param_write_bool(plist, "LZWEncodePages",
				 &pdev->params.LZWEncodePages)) < 0 ||
	(code = param_write_bool(plist, "PreserveHalftoneInfo",
				 &pdev->params.PreserveHalftoneInfo)) < 0 ||
	(code = param_write_bool(plist, "PreserveOPIComments",
				 &pdev->params.PreserveOPIComments)) < 0 ||
	(code = param_write_bool(plist, "PreserveOverprintSettings",
				 &pdev->params.PreserveOverprintSettings)) < 0 ||
#endif
	(code = param_write_name(plist, "TransferFunctionInfo", &tfis)) < 0 ||
	(code = param_write_name(plist, "UCRandBGInfo", &ucrbgis)) < 0 ||
#ifndef POST60
	(code = param_write_bool(plist, "UseFlateCompression",
				 &pdev->params.UseFlateCompression)) < 0 ||
#endif

    /* Color sampled image parameters */

	(code = psdf_get_image_params(plist, &Color_names, &pdev->params.ColorImage)) < 0 ||
	(code = param_write_name(plist, "ColorConversionStrategy",
				 &ccss)) < 0 ||
#ifdef POST60
	(code = param_write_string(plist, "CalCMYKProfile", &ccpros)) < 0 ||
	(code = param_write_string(plist, "CalGrayProfile", &cgpros)) < 0 ||
	(code = param_write_string(plist, "CalRGBProfile", &crpros)) < 0 ||
	(code = param_write_string(plist, "sRGBProfile", &srpros)) < 0 ||
#else
	(code = param_write_bool(plist, "ConvertCMYKImagesToRGB",
				 &pdev->params.ConvertCMYKImagesToRGB)) < 0 ||
	(code = param_write_bool(plist, "ConvertImagesToIndexed",
				 &pdev->params.ConvertImagesToIndexed)) < 0 ||
#endif

    /* Gray sampled image parameters */

	(code = psdf_get_image_params(plist, &Gray_names, &pdev->params.GrayImage)) < 0 ||

    /* Mono sampled image parameters */

	(code = psdf_get_image_params(plist, &Mono_names, &pdev->params.MonoImage)) < 0 ||

    /* Font embedding parameters */

	(code = psdf_get_embed_param(plist, ".AlwaysEmbed", &pdev->params.AlwaysEmbed)) < 0 ||
	(code = psdf_get_embed_param(plist, ".NeverEmbed", &pdev->params.NeverEmbed)) < 0 ||
#ifdef POST60
	(code = psdf_write_name(plist, "CannotEmbedFontPolicy",
		CannotEmbedFontPolicy_names[(int)pdev->params.CannotEmbedFontPolicy])) < 0
#else
	(code = param_write_bool(plist, "EmbedAllFonts", &pdev->params.EmbedAllFonts)) < 0 ||
	(code = param_write_int(plist, "MaxSubsetPct", &pdev->params.MaxSubsetPct)) < 0 ||
	(code = param_write_bool(plist, "SubsetFonts", &pdev->params.SubsetFonts)) < 0
#endif
	);
    return code;
}

/* -------- Put parameters -------- */

extern stream_state_proc_put_params(s_DCTE_put_params, stream_DCT_state);
extern stream_state_proc_put_params(s_CF_put_params, stream_CF_state);
typedef stream_state_proc_put_params((*ss_put_params_t), stream_state);

private int
psdf_CF_put_params(gs_param_list * plist, stream_state * st)
{
    stream_CFE_state *const ss = (stream_CFE_state *) st;

    (*s_CFE_template.set_defaults) (st);
    ss->K = -1;
    ss->BlackIs1 = true;
    return s_CF_put_params(plist, (stream_CF_state *) ss);
}
private int
psdf_DCT_put_params(gs_param_list * plist, stream_state * ss)
{
    return s_DCTE_put_params(plist, (stream_DCT_state *) ss);
}

/* Put [~](Always|Never)Embed parameters. */
private int
param_read_embed_array(gs_param_list * plist, gs_param_name pname,
		       gs_param_string_array * psa, int ecode)
{
    int code;

    psa->data = 0, psa->size = 0;
    switch (code = param_read_name_array(plist, pname, psa)) {
	default:
	    ecode = code;
	    param_signal_error(plist, pname, ecode);
	case 0:
	case 1:
	    break;
    }
    return ecode;
}
private bool
param_string_eq(const gs_param_string *ps1, const gs_param_string *ps2)
{
    return !bytes_compare(ps1->data, ps1->size, ps2->data, ps2->size);
}
private int
add_embed(gs_param_string_array *prsa, const gs_param_string_array *psa,
	  gs_memory_t *mem)
{
    uint i;
    gs_param_string *const rdata =
	(gs_param_string *)prsa->data; /* break const */
    uint count = prsa->size;

    for (i = 0; i < psa->size; ++i) {
	uint j;

	for (j = 0; j < count; ++j)
	    if (param_string_eq(&psa->data[i], &rdata[j]))
		    break;
	if (j == count) {
	    uint size = psa->data[i].size;
	    byte *data = gs_alloc_string(mem, size, "add_embed");

	    if (data == 0)
		return_error(gs_error_VMerror);
	    memcpy(data, psa->data[i].data, size);
	    rdata[count].data = data;
	    rdata[count].size = size;
	    rdata[count].persistent = false;
	    count++;
	}
    }
    prsa->size = count;
    return 0;
}
private void
delete_embed(gs_param_string_array *prsa, const gs_param_string_array *pnsa,
	     gs_memory_t *mem)
{
    uint i;
    gs_param_string *const rdata =
	(gs_param_string *)prsa->data; /* break const */
    uint count = prsa->size;

    for (i = pnsa->size; i-- > 0;) {
	uint j;

	for (j = count; j-- > 0;)
	    if (param_string_eq(&pnsa->data[i], &rdata[j]))
		break;
	if (j + 1 != 0) {
	    gs_free_const_string(mem, rdata[j].data, rdata[j].size,
				 "delete_embed");
	    rdata[j] = rdata[--count];
	}
    }
    prsa->size = count;
}
private int
psdf_put_embed_param(gs_param_list * plist, gs_param_name notpname,
		     gs_param_name allpname, gs_param_string_array * psa,
		     gs_memory_t *mem, int ecode)
{
    gs_param_name pname = notpname + 1;
    gs_param_string_array sa, nsa, asa;
    bool replace;
    gs_param_string *rdata;
    gs_param_string_array rsa;
    int code = 0;

    mem = gs_memory_stable(mem);
    ecode = param_read_embed_array(plist, pname, &sa, ecode);
    ecode = param_read_embed_array(plist, notpname, &nsa, ecode);
    ecode = param_read_embed_array(plist, allpname, &asa, ecode);
    if (ecode < 0)
	return ecode;
    /*
     * Figure out whether we're replacing (sa == asa or asa and no sa,
     * no nsa) or updating (all other cases).
     */
    if (asa.data == 0 || nsa.data != 0)
	replace = false;
    else if (sa.data == 0)
	replace = true;
    else if (sa.size != asa.size)
	replace = false;
    else {
	/* Test whether sa == asa. */
	uint i;

	replace = true;
	for (i = 0; i < sa.size; ++i)
	    if (!param_string_eq(&sa.data[i], &asa.data[i])) {
		replace = false;
		break;
	    }
	if (replace)
	    return 0;		/* no-op */
    }
    if (replace) {
	/* Wholesale replacement, only asa is relevant. */
	rdata = gs_alloc_struct_array(mem, asa.size, gs_param_string,
				      &st_param_string_element,
				      "psdf_put_embed_param(replace)");
	if (rdata == 0)
	    return_error(gs_error_VMerror);
	rsa.data = rdata;
	rsa.size = 0;
	if ((code = add_embed(&rsa, &asa, mem)) < 0) {
	    gs_free_object(mem, rdata, "psdf_put_embed_param(replace)");
	    ecode = code;
	} else
	    delete_embed(psa, psa, mem);
    } else if (sa.data || nsa.data) {
	/* Incremental update, sa and nsa are relevant, asa is not. */
	rdata = gs_alloc_struct_array(mem, psa->size + sa.size,
				      gs_param_string,
				      &st_param_string_element,
				      "psdf_put_embed_param(update)");
	if (rdata == 0)
	    return_error(gs_error_VMerror);
	memcpy(rdata, psa->data, psa->size * sizeof(*psa->data));
	rsa.data = rdata;
	rsa.size = psa->size;
	if ((code = add_embed(&rsa, &sa, mem)) < 0) {
	    gs_free_object(mem, rdata, "psdf_put_embed_param(update)");
	    ecode = code;
	} else {
	    delete_embed(&rsa, &nsa, mem);
	    rsa.data = gs_resize_object(mem, rdata, rsa.size,
					"psdf_put_embed_param(resize)");
	}
    } else
	return 0;		/* no-op */
    if (code >= 0) {
	gs_free_const_object(mem, psa->data, "psdf_put_embed_param(free)");
	rsa.persistent = false;
	*psa = rsa;
    }
    return ecode;
}

/* Put an image Dict parameter. */
private int
psdf_put_image_dict_param(gs_param_list * plist, const gs_param_name pname,
		      stream_state ** pss, const stream_template * template,
			  ss_put_params_t put_params, gs_memory_t * mem)
{
    gs_param_dict dict;
    stream_state *ss = *pss;
    int code;

    switch (code = param_begin_read_dict(plist, pname, &dict, false)) {
	default:
	    param_signal_error(plist, pname, code);
	    return code;
	case 1:
	    ss = 0;
	    break;
	case 0:{
	    /******
	     ****** THIS CAUSES A SEGV FOR DCT FILTERS, BECAUSE
	     ****** THEY DON'T INTIALIZE PROPERLY.
	     ******/
	    if (template != &s_DCTE_template) {
		stream_state *ss_new =
		    s_alloc_state(mem, template->stype, pname);

		if (ss_new == 0)
		    return_error(gs_error_VMerror);
		ss_new->template = template;
		if (template->set_defaults)
		    (*template->set_defaults)(ss_new);
		code = (*put_params)(dict.list, ss_new);
		if (code < 0) {
		    param_signal_error(plist, pname, code);
		    /* Make sure we free the new state. */
		    *pss = ss_new;
		} else
		    ss = ss_new;
	    }
	    }
	    param_end_read_dict(plist, pname, &dict);
    }
    if (*pss != ss) {
	if (ss) {
	    /****** FREE SUBSIDIARY OBJECTS -- HOW? ******/
	    gs_free_object(mem, *pss, pname);
	}
	*pss = ss;
    }
    return code;
}

/* Put a set of image-related parameters. */
private int
psdf_put_image_params(const gx_device_psdf * pdev, gs_param_list * plist,
 const psdf_image_param_names * pnames, const psdf_image_filter_name * pifn,
		      psdf_image_params * params, int ecode)
{
    gs_param_string fs;
    /*
     * Since this procedure can be called before the device is open,
     * we must use pdev->memory rather than pdev->v_memory.
     */
    gs_memory_t *mem = pdev->memory;
    gs_param_name pname;
    int dsti = params->DownsampleType;
    int code;

    if ((pname = pnames->ACSDict) != 0) {
	code = psdf_put_image_dict_param(plist, pname, &params->ACSDict,
					 &s_DCTE_template,
					 psdf_DCT_put_params, mem);
	if (code < 0)
	    ecode = code;
    }
    ecode = param_put_bool(plist, pnames->AntiAlias,
				&params->AntiAlias, ecode);
    if (pnames->AutoFilter)
	ecode = param_put_bool(plist, pnames->AutoFilter,
				    &params->AutoFilter, ecode);
    ecode = param_put_int(plist, pnames->Depth,
			       &params->Depth, ecode);
    if ((pname = pnames->Dict) != 0) {
	const stream_template *template;
	ss_put_params_t put_params;

	/* Hack to determine what kind of a Dict we want: */
	if (pnames->Dict[0] == 'M')
	    template = &s_CFE_template,
		put_params = psdf_CF_put_params;
	else
	    template = &s_DCTE_template,
		put_params = psdf_DCT_put_params;
	code = psdf_put_image_dict_param(plist, pname, &params->Dict,
					 template, put_params, mem);
	if (code < 0)
	    ecode = code;
    }
    ecode = param_put_bool(plist, pnames->Downsample,
				&params->Downsample, ecode);
    if ((ecode = param_put_enum(plist, pnames->DownsampleType,
				     &dsti, DownsampleType_names,
				     ecode)) >= 0
	)
	params->DownsampleType = (enum psdf_downsample_type)dsti;
    ecode = param_put_bool(plist, pnames->Encode,
				&params->Encode, ecode);
    switch (code = param_read_string(plist, pnames->Filter, &fs)) {
	case 0:
	    {
		const psdf_image_filter_name *pn = pifn;

		while (pn->pname != 0 && !gs_param_string_eq(&fs, pn->pname))
		    pn++;
		if (pn->pname == 0 || pn->min_version > pdev->version) {
		    ecode = gs_error_rangecheck;
		    goto ipe;
		}
		params->Filter = pn->pname;
		params->filter_template = pn->template;
		break;
	    }
	default:
	    ecode = code;
	  ipe:param_signal_error(plist, pnames->Filter, ecode);
	case 1:
	    break;
    }
    ecode = param_put_int(plist, pnames->Resolution,
			       &params->Resolution, ecode);
    if (ecode >= 0) {		/* Force parameters to acceptable values. */
	if (params->Resolution < 1)
	    params->Resolution = 1;
	switch (params->Depth) {
	    default:
		params->Depth = -1;
	    case 1:
	    case 2:
	    case 4:
	    case 8:
	    case -1:
		break;
	}
    }
    return ecode;
}

/* Put parameters. */
int
gdev_psdf_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_psdf *pdev = (gx_device_psdf *) dev;
    gs_memory_t *mem =
	(pdev->v_memory ? pdev->v_memory : dev->memory);
    int ecode = 0;
    int code;
    gs_param_name param_name;
    psdf_distiller_params params;

    /* General parameters. */

    params = pdev->params;

    ecode = param_put_bool(plist, "ASCII85EncodePages",
				&params.ASCII85EncodePages, ecode);
    {
	int arpi = params.AutoRotatePages;

	ecode = param_put_enum(plist, "AutoRotatePages", &arpi,
				    AutoRotatePages_names, ecode);
	params.AutoRotatePages = (enum psdf_auto_rotate_pages)arpi;
    }
    ecode = param_put_bool(plist, "CompressPages",
				&params.CompressPages, ecode);
    switch (code = param_read_long(plist, (param_name = "ImageMemory"), &params.ImageMemory)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }
    ecode = param_put_bool(plist, "LZWEncodePages",
				&params.LZWEncodePages, ecode);
    ecode = param_put_bool(plist, "PreserveHalftoneInfo",
				&params.PreserveHalftoneInfo, ecode);
    ecode = param_put_bool(plist, "PreserveOPIComments",
				&params.PreserveOPIComments, ecode);
    ecode = param_put_bool(plist, "PreserveOverprintSettings",
				&params.PreserveOverprintSettings, ecode);
    {
	int tfii = params.TransferFunctionInfo;

	ecode = param_put_enum(plist, "TransferFunctionInfo", &tfii,
				    TransferFunctionInfo_names, ecode);
	params.TransferFunctionInfo = (enum psdf_transfer_function_info)tfii;
    }
    {
	int ucrbgi = params.UCRandBGInfo;

	ecode = param_put_enum(plist, "UCRandBGInfo", &ucrbgi,
				    UCRandBGInfo_names, ecode);
	params.UCRandBGInfo = (enum psdf_ucr_and_bg_info)ucrbgi;
    }
    ecode = param_put_bool(plist, "UseFlateCompression",
				&params.UseFlateCompression, ecode);

    /* Color sampled image parameters */

    ecode = psdf_put_image_params(pdev, plist, &Color_names, Poly_filters,
				  &params.ColorImage, ecode);
    {
	int ccsi = params.ColorConversionStrategy;

	ecode = param_put_enum(plist, "ColorConversionStrategy", &ccsi,
				    ColorConversionStrategy_names, ecode);
	params.ColorConversionStrategy =
	    (enum psdf_color_conversion_strategy)ccsi;
    }
    ecode = param_put_bool(plist, "ConvertCMYKImagesToRGB",
				&params.ConvertCMYKImagesToRGB, ecode);
    ecode = param_put_bool(plist, "ConvertImagesToIndexed",
				&params.ConvertImagesToIndexed, ecode);

    /* Gray sampled image parameters */

    ecode = psdf_put_image_params(pdev, plist, &Gray_names, Poly_filters,
				  &params.GrayImage, ecode);

    /* Mono sampled image parameters */

    ecode = psdf_put_image_params(pdev, plist, &Mono_names, Mono_filters,
				  &params.MonoImage, ecode);

    /* Font embedding parameters */

    ecode = psdf_put_embed_param(plist, "~AlwaysEmbed", ".AlwaysEmbed",
				 &params.AlwaysEmbed, mem, ecode);
    ecode = psdf_put_embed_param(plist, "~NeverEmbed", ".NeverEmbed",
				 &params.NeverEmbed, mem, ecode);
    ecode = param_put_bool(plist, "EmbedAllFonts",
				&params.EmbedAllFonts, ecode);
    ecode = param_put_bool(plist, "SubsetFonts",
				&params.SubsetFonts, ecode);
    ecode = param_put_int(plist, "MaxSubsetPct",
			       &params.MaxSubsetPct, ecode);

    if (ecode < 0)
	return ecode;
    code = gdev_vector_put_params(dev, plist);
    if (code < 0)
	return code;

    pdev->params = params;	/* OK to update now */
    return 0;
}
