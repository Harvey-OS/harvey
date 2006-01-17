/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsdp.c,v 1.14 2004/06/30 14:35:37 igor Exp $ */
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
 * you do a currentpagedevice, doing a setpagedevice later will restore the
 * same state, we actually define the parameters in sets of 3:
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

private const psdf_image_filter_name Poly_filters[] = {
    {"DCTEncode", &s_DCTE_template},
    {"FlateEncode", &s_zlibE_template, psdf_version_ll3},
    {"LZWEncode", &s_LZWE_template},
    {0, 0}
};

private const psdf_image_filter_name Mono_filters[] = {
    {"CCITTFaxEncode", &s_CFE_template},
    {"FlateEncode", &s_zlibE_template, psdf_version_ll3},
    {"LZWEncode", &s_LZWE_template},
    {"RunLengthEncode", &s_RLE_template},
    {0, 0}
};

typedef struct psdf_image_param_names_s {
    const char *ACSDict;	/* not used for mono */
    const char *Dict;
    const char *DownsampleType;
    float DownsampleThreshold_default;
    const psdf_image_filter_name *filter_names;
    const char *Filter;
    gs_param_item_t items[8];	/* AutoFilter (not used for mono), */
				/* AntiAlias, */
				/* Depth, Downsample, DownsampleThreshold, */
				/* Encode, Resolution, end marker */
} psdf_image_param_names_t;
#define pi(key, type, memb) { key, type, offset_of(psdf_image_params, memb) }
#define psdf_image_param_names(acs, aa, af, de, di, ds, dt, dst, dstd, e, f, fns, r)\
    acs, di, dt, dstd, fns, f, {\
      pi(af, gs_param_type_bool, AutoFilter),\
      pi(aa, gs_param_type_bool, AntiAlias),\
      pi(de, gs_param_type_int, Depth),\
      pi(ds, gs_param_type_bool, Downsample),\
      pi(dst, gs_param_type_float, DownsampleThreshold),\
      pi(e, gs_param_type_bool, Encode),\
      pi(r, gs_param_type_int, Resolution),\
      gs_param_item_end\
    }

private const psdf_image_param_names_t Color_names = {
    psdf_image_param_names(
	"ColorACSImageDict", "AntiAliasColorImages", "AutoFilterColorImages",
	"ColorImageDepth", "ColorImageDict",
	"DownsampleColorImages", "ColorImageDownsampleType",
	"ColorImageDownsampleThreshold", 1.5,
	"EncodeColorImages", "ColorImageFilter", Poly_filters,
	"ColorImageResolution"
    )
};
private const psdf_image_param_names_t Gray_names = {
    psdf_image_param_names(
	"GrayACSImageDict", "AntiAliasGrayImages", "AutoFilterGrayImages",
	"GrayImageDepth", "GrayImageDict",
	"DownsampleGrayImages", "GrayImageDownsampleType",
	"GrayImageDownsampleThreshold", 2.0,
	"EncodeGrayImages", "GrayImageFilter", Poly_filters,
	"GrayImageResolution"
    )
};
private const psdf_image_param_names_t Mono_names = {
    psdf_image_param_names(
	0, "AntiAliasMonoImages", 0,
	"MonoImageDepth", "MonoImageDict",
	"DownsampleMonoImages", "MonoImageDownsampleType",
	"MonoImageDownsampleThreshold", 2.0,
	"EncodeMonoImages", "MonoImageFilter", Mono_filters,
	"MonoImageResolution"
    )
};
#undef pi
private const char *const AutoRotatePages_names[] = {
    psdf_arp_names, 0
};
private const char *const ColorConversionStrategy_names[] = {
    psdf_ccs_names, 0
};
private const char *const DownsampleType_names[] = {
    psdf_ds_names, 0
};
private const char *const Binding_names[] = {
    psdf_binding_names, 0
};
private const char *const DefaultRenderingIntent_names[] = {
    psdf_ri_names, 0
};
private const char *const TransferFunctionInfo_names[] = {
    psdf_tfi_names, 0
};
private const char *const UCRandBGInfo_names[] = {
    psdf_ucrbg_names, 0
};
private const char *const CannotEmbedFontPolicy_names[] = {
    psdf_cefp_names, 0
};

private const gs_param_item_t psdf_param_items[] = {
#define pi(key, type, memb) { key, type, offset_of(psdf_distiller_params, memb) }

    /* General parameters */

    pi("ASCII85EncodePages", gs_param_type_bool, ASCII85EncodePages),
    /* (AutoRotatePages) */
    /* (Binding) */
    pi("CompressPages", gs_param_type_bool, CompressPages),
    /* (DefaultRenderingIntent) */
    pi("DetectBlends", gs_param_type_bool, DetectBlends),
    pi("DoThumbnails", gs_param_type_bool, DoThumbnails),
    pi("ImageMemory", gs_param_type_long, ImageMemory),
    /* (LockDistillerParams) */
    pi("LZWEncodePages", gs_param_type_bool, LZWEncodePages),
    pi("OPM", gs_param_type_int, OPM),
    pi("PreserveHalftoneInfo", gs_param_type_bool, PreserveHalftoneInfo),
    pi("PreserveOPIComments", gs_param_type_bool, PreserveOPIComments),
    pi("PreserveOverprintSettings", gs_param_type_bool, PreserveOverprintSettings),
    /* (TransferFunctionInfo) */
    /* (UCRandBGInfo) */
    pi("UseFlateCompression", gs_param_type_bool, UseFlateCompression),

    /* Color image processing parameters */

    pi("ConvertCMYKImagesToRGB", gs_param_type_bool, ConvertCMYKImagesToRGB),
    pi("ConvertImagesToIndexed", gs_param_type_bool, ConvertImagesToIndexed),

    /* Font embedding parameters */

    /* (CannotEmbedFontPolicy) */
    pi("EmbedAllFonts", gs_param_type_bool, EmbedAllFonts),
    pi("MaxSubsetPct", gs_param_type_int, MaxSubsetPct),
    pi("SubsetFonts", gs_param_type_bool, SubsetFonts),

#undef pi
    gs_param_item_end
};

/* -------- Get parameters -------- */

private int
psdf_write_name(gs_param_list *plist, const char *key, const char *str)
{
    gs_param_string pstr;

    param_string_from_string(pstr, str);
    return param_write_name(plist, key, &pstr);
}

private int
psdf_write_string_param(gs_param_list *plist, const char *key,
			const gs_const_string *pstr)
{
    gs_param_string ps;

    ps.data = pstr->data;
    ps.size = pstr->size;
    ps.persistent = false;
    return param_write_string(plist, key, &ps);
}

/*
 * Get an image Dict parameter.  Note that we return a default (empty)
 * dictionary if the parameter has never been set.
 */
private int
psdf_get_image_dict_param(gs_param_list * plist, const gs_param_name pname,
			  gs_c_param_list *plvalue)
{
    gs_param_dict dict;
    int code;

    if (pname == 0)
	return 0;
    dict.size = 12;		/* enough for all param dicts we know about */
    if ((code = param_begin_write_dict(plist, pname, &dict, false)) < 0)
	return code;
    if (plvalue != 0) {
	gs_c_param_list_read(plvalue);
	code = param_list_copy(dict.list, (gs_param_list *)plvalue);
    }
    param_end_write_dict(plist, pname, &dict);
    return code;
}

/* Get a set of image-related parameters. */
private int
psdf_get_image_params(gs_param_list * plist,
	  const psdf_image_param_names_t * pnames, psdf_image_params * params)
{
    /* Skip AutoFilter for mono images. */
    const gs_param_item_t *items =
	(pnames->items[0].key == 0 ? pnames->items + 1 : pnames->items);
    int code;

    /*
     * We must actually return a value for every parameter, so that
     * all parameter names will be recognized as settable by -d or -s
     * from the command line.
     */
    if (
	   (code = gs_param_write_items(plist, params, NULL, items)) < 0 ||
	   (code = psdf_get_image_dict_param(plist, pnames->ACSDict,
					     params->ACSDict)) < 0 ||
	   /* (AntiAlias) */
	   /* (AutoFilter) */
	   /* (Depth) */
	   (code = psdf_get_image_dict_param(plist, pnames->Dict,
					     params->Dict)) < 0 ||
	   /* (Downsample) */
	   (code = psdf_write_name(plist, pnames->DownsampleType,
		DownsampleType_names[params->DownsampleType])) < 0 ||
	   /* (DownsampleThreshold) */
	   /* (Encode) */
	   (code = psdf_write_name(plist, pnames->Filter,
				   (params->Filter == 0 ?
				    pnames->filter_names[0].pname :
				    params->Filter))) < 0
	   /* (Resolution) */
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

    if (
	code < 0 ||
	(code = gs_param_write_items(plist, &pdev->params, NULL, psdf_param_items)) < 0 ||

    /* General parameters */

	(code = psdf_write_name(plist, "AutoRotatePages",
		AutoRotatePages_names[(int)pdev->params.AutoRotatePages])) < 0 ||
	(code = psdf_write_name(plist, "Binding",
		Binding_names[(int)pdev->params.Binding])) < 0 ||
	(code = psdf_write_name(plist, "DefaultRenderingIntent",
		DefaultRenderingIntent_names[(int)pdev->params.DefaultRenderingIntent])) < 0 ||
	(code = psdf_write_name(plist, "TransferFunctionInfo",
		TransferFunctionInfo_names[(int)pdev->params.TransferFunctionInfo])) < 0 ||
	(code = psdf_write_name(plist, "UCRandBGInfo",
		UCRandBGInfo_names[(int)pdev->params.UCRandBGInfo])) < 0 ||

    /* Color sampled image parameters */

	(code = psdf_get_image_params(plist, &Color_names, &pdev->params.ColorImage)) < 0 ||
	(code = psdf_write_name(plist, "ColorConversionStrategy",
		ColorConversionStrategy_names[(int)pdev->params.ColorConversionStrategy])) < 0 ||
	(code = psdf_write_string_param(plist, "CalCMYKProfile",
					&pdev->params.CalCMYKProfile)) < 0 ||
	(code = psdf_write_string_param(plist, "CalGrayProfile",
					&pdev->params.CalGrayProfile)) < 0 ||
	(code = psdf_write_string_param(plist, "CalRGBProfile",
					&pdev->params.CalRGBProfile)) < 0 ||
	(code = psdf_write_string_param(plist, "sRGBProfile",
					&pdev->params.sRGBProfile)) < 0 ||

    /* Gray sampled image parameters */

	(code = psdf_get_image_params(plist, &Gray_names, &pdev->params.GrayImage)) < 0 ||

    /* Mono sampled image parameters */

	(code = psdf_get_image_params(plist, &Mono_names, &pdev->params.MonoImage)) < 0 ||

    /* Font embedding parameters */

	(code = psdf_get_embed_param(plist, ".AlwaysEmbed", &pdev->params.AlwaysEmbed)) < 0 ||
	(code = psdf_get_embed_param(plist, ".NeverEmbed", &pdev->params.NeverEmbed)) < 0 ||
	(code = psdf_write_name(plist, "CannotEmbedFontPolicy",
		CannotEmbedFontPolicy_names[(int)pdev->params.CannotEmbedFontPolicy])) < 0
	)
	DO_NOTHING;
    return code;
}

/* -------- Put parameters -------- */

extern stream_state_proc_put_params(s_CF_put_params, stream_CF_state);
extern stream_state_proc_put_params(s_DCTE_put_params, stream_DCT_state);
typedef stream_state_proc_put_params((*ss_put_params_t), stream_state);

private int
psdf_read_string_param(gs_param_list *plist, const char *key,
		       gs_const_string *pstr, gs_memory_t *mem, int ecode)
{
    gs_param_string ps;
    int code;

    switch (code = param_read_string(plist, key, &ps)) {
    case 0: {
	uint size = ps.size;
	byte *data = gs_alloc_string(mem, size, "psdf_read_string_param");

	if (data == 0)
	    return_error(gs_error_VMerror);
	memcpy(data, ps.data, size);
	pstr->data = data;
	pstr->size = size;
	break;
    }
    default:
	ecode = code;
    case 1:
	break;
    }
    return ecode;
}

/*
 * The arguments and return value for psdf_put_enum are different because
 * we must cast the value both going in and coming out.
 */
private int
psdf_put_enum(gs_param_list *plist, const char *key, int value,
	      const char *const pnames[], int *pecode)
{
    *pecode = param_put_enum(plist, key, &value, pnames, *pecode);
    return value;
}

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
psdf_DCT_put_params(gs_param_list * plist, stream_state * st)
{
    return psdf_DCT_filter(plist, st, 8 /*nominal*/, 8 /*ibid.*/, 3 /*ibid.*/,
			   NULL);
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
			  gs_c_param_list **pplvalue,
			  const stream_template * template,
			  ss_put_params_t put_params, gs_memory_t * mem)
{
    gs_param_dict dict;
    gs_c_param_list *plvalue = *pplvalue;
    int code;

    mem = gs_memory_stable(mem);
    switch (code = param_begin_read_dict(plist, pname, &dict, false)) {
	default:
	    param_signal_error(plist, pname, code);
	    return code;
	case 1:
	    return 0;
	case 0: {
	    /* Check the parameter values now. */
	    stream_state *ss = s_alloc_state(mem, template->stype, pname);

	    if (ss == 0)
		return_error(gs_error_VMerror);
	    ss->template = template;
	    if (template->set_defaults)
		template->set_defaults(ss);
	    code = put_params(dict.list, ss);
	    if (template->release)
		template->release(ss);
	    gs_free_object(mem, ss, pname);
	    if (code < 0) {
		param_signal_error(plist, pname, code);
	    } else {
		plvalue = gs_c_param_list_alloc(mem, pname);
		if (plvalue == 0)
		    return_error(gs_error_VMerror);
		gs_c_param_list_write(plvalue, mem);
		code = param_list_copy((gs_param_list *)plvalue,
				       dict.list);
		if (code < 0) {
		    gs_c_param_list_release(plvalue);
		    gs_free_object(mem, plvalue, pname);
		    plvalue = *pplvalue;
		}
	    }
	}
	param_end_read_dict(plist, pname, &dict);
	break;
    }
    if (plvalue != *pplvalue) {
	if (*pplvalue)
	    gs_c_param_list_release(*pplvalue);
	*pplvalue = plvalue;
    }
    return code;
}

/* Put a set of image-related parameters. */
private int
psdf_put_image_params(const gx_device_psdf * pdev, gs_param_list * plist,
		      const psdf_image_param_names_t * pnames,
		      psdf_image_params * params, int ecode)
{
    gs_param_string fs;
    /*
     * Since this procedure can be called before the device is open,
     * we must use pdev->memory rather than pdev->v_memory.
     */
    gs_memory_t *mem = pdev->memory;
    gs_param_name pname;
    /* Skip AutoFilter for mono images. */
    const gs_param_item_t *items =
	(pnames->items[0].key == 0 ? pnames->items + 1 : pnames->items);
    int code = gs_param_read_items(plist, params, items);

    if ((pname = pnames->ACSDict) != 0) {
	code = psdf_put_image_dict_param(plist, pname, &params->ACSDict,
					 &s_DCTE_template,
					 psdf_DCT_put_params, mem);
	if (code < 0)
	    ecode = code;
    }
    /* (AntiAlias) */
    /* (AutoFilter) */
    /* (Depth) */
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
    /* (Downsample) */
    params->DownsampleType = (enum psdf_downsample_type)
	psdf_put_enum(plist, pnames->DownsampleType,
		      (int)params->DownsampleType, DownsampleType_names,
		      &ecode);
    /* (DownsampleThreshold) */
    /* (Encode) */
    switch (code = param_read_string(plist, pnames->Filter, &fs)) {
	case 0:
	    {
		const psdf_image_filter_name *pn = pnames->filter_names;

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
    /* (Resolution) */
    if (ecode >= 0) {		/* Force parameters to acceptable values. */
	if (params->Resolution < 1)
	    params->Resolution = 1;
	if (params->DownsampleThreshold < 1 ||
	    params->DownsampleThreshold > 10)
	    params->DownsampleThreshold = pnames->DownsampleThreshold_default;
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
    int ecode, code;
    psdf_distiller_params params;

    params = pdev->params;

    /*
     * If LockDistillerParams was true and isn't being set to false,
     * ignore all other psdf parameters.  However, do not ignore the
     * standard device parameters.
     */
    ecode = code = param_read_bool(plist, "LockDistillerParams",
  				   &params.LockDistillerParams);
    if (!(pdev->params.LockDistillerParams && params.LockDistillerParams)) {
  
	/* General parameters. */

	code = gs_param_read_items(plist, &params, psdf_param_items);
	if (code < 0)
	    ecode = code;
	params.AutoRotatePages = (enum psdf_auto_rotate_pages)
	    psdf_put_enum(plist, "AutoRotatePages", (int)params.AutoRotatePages,
			  AutoRotatePages_names, &ecode);
	params.Binding = (enum psdf_binding)
	    psdf_put_enum(plist, "Binding", (int)params.Binding,
			  Binding_names, &ecode);
	params.DefaultRenderingIntent = (enum psdf_default_rendering_intent)
	    psdf_put_enum(plist, "DefaultRenderingIntent",
			  (int)params.DefaultRenderingIntent,
			  DefaultRenderingIntent_names, &ecode);
	params.TransferFunctionInfo = (enum psdf_transfer_function_info)
	    psdf_put_enum(plist, "TransferFunctionInfo",
			  (int)params.TransferFunctionInfo,
			  TransferFunctionInfo_names, &ecode);
	params.UCRandBGInfo = (enum psdf_ucr_and_bg_info)
	    psdf_put_enum(plist, "UCRandBGInfo", (int)params.UCRandBGInfo,
			  UCRandBGInfo_names, &ecode);
	ecode = param_put_bool(plist, "UseFlateCompression",
			       &params.UseFlateCompression, ecode);

	/* Color sampled image parameters */

	ecode = psdf_put_image_params(pdev, plist, &Color_names,
				      &params.ColorImage, ecode);
	params.ColorConversionStrategy = (enum psdf_color_conversion_strategy)
	    psdf_put_enum(plist, "ColorConversionStrategy",
			  (int)params.ColorConversionStrategy,
			  ColorConversionStrategy_names, &ecode);
	ecode = psdf_read_string_param(plist, "CalCMYKProfile",
				       &params.CalCMYKProfile, mem, ecode);
	ecode = psdf_read_string_param(plist, "CalGrayProfile",
				       &params.CalGrayProfile, mem, ecode);
	ecode = psdf_read_string_param(plist, "CalRGBProfile",
				       &params.CalRGBProfile, mem, ecode);
	ecode = psdf_read_string_param(plist, "sRGBProfile",
				       &params.sRGBProfile, mem, ecode);

	/* Gray sampled image parameters */

	ecode = psdf_put_image_params(pdev, plist, &Gray_names,
				      &params.GrayImage, ecode);

	/* Mono sampled image parameters */

	ecode = psdf_put_image_params(pdev, plist, &Mono_names,
				      &params.MonoImage, ecode);

	/* Font embedding parameters */

	ecode = psdf_put_embed_param(plist, "~AlwaysEmbed", ".AlwaysEmbed",
				     &params.AlwaysEmbed, mem, ecode);
	ecode = psdf_put_embed_param(plist, "~NeverEmbed", ".NeverEmbed",
				     &params.NeverEmbed, mem, ecode);
	params.CannotEmbedFontPolicy = (enum psdf_cannot_embed_font_policy)
	    psdf_put_enum(plist, "CannotEmbedFontPolicy",
			  (int)params.CannotEmbedFontPolicy,
			  CannotEmbedFontPolicy_names, &ecode);
    }
    if (ecode < 0)
	return ecode;
    code = gdev_vector_put_params(dev, plist);
    if (code < 0)
	return code;

    pdev->params = params;	/* OK to update now */
    return 0;
}
