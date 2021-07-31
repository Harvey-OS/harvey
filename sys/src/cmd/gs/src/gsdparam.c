/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsdparam.c,v 1.4 2000/09/19 19:00:27 lpd Exp $ */
/* Default device parameters for Ghostscript library */
#include "memory_.h"		/* for memcpy */
#include "string_.h"		/* for strlen */
#include "gx.h"
#include "gserrors.h"
#include "gsdevice.h"		/* for prototypes */
#include "gsparam.h"
#include "gxdevice.h"
#include "gxfixed.h"

/* Define whether we accept PageSize as a synonym for MediaSize. */
/* This is for backward compatibility only. */
#define PAGESIZE_IS_MEDIASIZE

/* ================ Getting parameters ================ */

/* Forward references */
private bool param_HWColorMap(P2(gx_device *, byte *));

/* Get the device parameters. */
int
gs_get_device_or_hw_params(gx_device * orig_dev, gs_param_list * plist,
			   bool is_hardware)
{
    /*
     * We must be prepared to copy the device if it is the read-only
     * prototype.
     */
    gx_device *dev;
    int code;

    if (orig_dev->memory)
	dev = orig_dev;
    else {
	code = gs_copydevice(&dev, orig_dev, plist->memory);
	if (code < 0)
	    return code;
    }
    gx_device_set_procs(dev);
    fill_dev_proc(dev, get_params, gx_default_get_params);
    fill_dev_proc(dev, get_page_device, gx_default_get_page_device);
    fill_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    code = (is_hardware ?
	    (*dev_proc(dev, get_hardware_params)) (dev, plist) :
	    (*dev_proc(dev, get_params)) (dev, plist));
    if (dev != orig_dev)
	gx_device_retain(dev, false);  /* frees the copy */
    return code;
}

/* Standard ProcessColorModel values. */
static const char *const pcmsa[] =
{
    "", "DeviceGray", "", "DeviceRGB", "DeviceCMYK"
};

/* Get standard parameters. */
int
gx_default_get_params(gx_device * dev, gs_param_list * plist)
{
    int code;

    /* Standard page device parameters: */

    int mns = 1;
    bool seprs = false;
    gs_param_string dns, pcms;
    gs_param_float_array msa, ibba, hwra, ma;
    gs_param_string_array scna;

#define set_param_array(a, d, s)\
  (a.data = d, a.size = s, a.persistent = false);

    /* Non-standard parameters: */

    int colors = dev->color_info.num_components;
    int depth = dev->color_info.depth;
    int GrayValues = dev->color_info.max_gray + 1;
    int HWSize[2];
    gs_param_int_array hwsa;
    gs_param_float_array hwma, mhwra;

    /* Fill in page device parameters. */

    param_string_from_string(dns, dev->dname);
    {
	const char *cms = pcmsa[colors];

	/* We might have an uninitialized device with */
	/* color_info.num_components = 0.... */
	if (*cms != 0)
	    param_string_from_string(pcms, cms);
	else
	    pcms.data = 0;
    }
    set_param_array(hwra, dev->HWResolution, 2);
    set_param_array(msa, dev->MediaSize, 2);
    set_param_array(ibba, dev->ImagingBBox, 4);
    set_param_array(ma, dev->Margins, 2);
    set_param_array(scna, NULL, 0);

    /* Fill in non-standard parameters. */

    HWSize[0] = dev->width;
    HWSize[1] = dev->height;
    set_param_array(hwsa, HWSize, 2);
    set_param_array(hwma, dev->HWMargins, 4);
    set_param_array(mhwra, dev->MarginsHWResolution, 2);

    /* Transmit the values. */

    if (

	/* Standard parameters */

	(code = param_write_name(plist, "OutputDevice", &dns)) < 0 ||
#ifdef PAGESIZE_IS_MEDIASIZE
	(code = param_write_float_array(plist, "PageSize", &msa)) < 0 ||
#endif
	(code = (pcms.data == 0 ? 0 :
		 param_write_name(plist, "ProcessColorModel", &pcms))) < 0 ||
	(code = param_write_float_array(plist, "HWResolution", &hwra)) < 0 ||
	(code = (dev->ImagingBBox_set ?
		 param_write_float_array(plist, "ImagingBBox", &ibba) :
		 param_write_null(plist, "ImagingBBox"))) < 0 ||
	(code = param_write_float_array(plist, "Margins", &ma)) < 0 ||
	(code = param_write_int(plist, "MaxSeparations", &mns)) < 0 ||
	(code = (dev->NumCopies_set < 0 ||
		 (*dev_proc(dev, get_page_device))(dev) == 0 ? 0:
		 dev->NumCopies_set ?
		 param_write_int(plist, "NumCopies", &dev->NumCopies) :
		 param_write_null(plist, "NumCopies"))) < 0 ||
	(code = param_write_name_array(plist, "SeparationColorNames", &scna)) < 0 ||
	(code = param_write_bool(plist, "Separations", &seprs)) < 0 ||
	(code = param_write_bool(plist, "UseCIEColor", &dev->UseCIEColor)) < 0 ||

	/* Non-standard parameters */

	(code = param_write_int_array(plist, "HWSize", &hwsa)) < 0 ||
	(code = param_write_float_array(plist, ".HWMargins", &hwma)) < 0 ||
	(code = param_write_float_array(plist, ".MarginsHWResolution", &mhwra)) < 0 ||
	(code = param_write_float_array(plist, ".MediaSize", &msa)) < 0 ||
	(code = param_write_string(plist, "Name", &dns)) < 0 ||
	(code = param_write_int(plist, "Colors", &colors)) < 0 ||
	(code = param_write_int(plist, "BitsPerPixel", &depth)) < 0 ||
	(code = param_write_int(plist, "GrayValues", &GrayValues)) < 0 ||
	(code = param_write_long(plist, "PageCount", &dev->PageCount)) < 0 ||
	(code = param_write_bool(plist, ".IgnoreNumCopies", &dev->IgnoreNumCopies)) < 0 ||
	(code = param_write_int(plist, "TextAlphaBits",
				&dev->color_info.anti_alias.text_bits)) < 0 ||
	(code = param_write_int(plist, "GraphicsAlphaBits",
				&dev->color_info.anti_alias.graphics_bits)) < 0

	)
	return code;

    /* Fill in color information. */

    if (colors > 1) {
	int RGBValues = dev->color_info.max_color + 1;
	long ColorValues = 1L << depth;

	if ((code = param_write_int(plist, "RedValues", &RGBValues)) < 0 ||
	    (code = param_write_int(plist, "GreenValues", &RGBValues)) < 0 ||
	    (code = param_write_int(plist, "BlueValues", &RGBValues)) < 0 ||
	    (code = param_write_long(plist, "ColorValues", &ColorValues)) < 0
	    )
	    return code;
    }
    if (param_requested(plist, "HWColorMap")) {
	byte palette[3 << 8];

	if (param_HWColorMap(dev, palette)) {
	    gs_param_string hwcms;

	    hwcms.data = palette, hwcms.size = colors << depth,
		hwcms.persistent = false;
	    if ((code = param_write_string(plist, "HWColorMap", &hwcms)) < 0)
		return code;
	}
    }

    return 0;
}

/* Get the color map for a device.  Return true if there is one. */
private bool
param_HWColorMap(gx_device * dev, byte * palette /* 3 << 8 */ )
{
    int depth = dev->color_info.depth;
    int colors = dev->color_info.num_components;

    if (depth <= 8 && colors <= 3) {
	byte *p = palette;
	gx_color_value rgb[3];
	gx_color_index i;

	fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);
	for (i = 0; (i >> depth) == 0; i++) {
	    int j;

	    if ((*dev_proc(dev, map_color_rgb)) (dev, i, rgb) < 0)
		return false;
	    for (j = 0; j < colors; j++)
		*p++ = gx_color_value_to_byte(rgb[j]);
	}
	return true;
    }
    return false;
}

/* Get hardware-detected parameters. Default action is no hardware params. */
int
gx_default_get_hardware_params(gx_device * dev, gs_param_list * plist)
{
    return 0;
}

/* ---------------- Input and output media ---------------- */

/* Finish defining input or output media. */
private int
finish_media(gs_param_list * mlist, gs_param_name key, const char *media_type)
{
    int code = 0;

    if (media_type != 0) {
	gs_param_string as;

	param_string_from_string(as, media_type);
	code = param_write_string(mlist, key, &as);
    }
    return code;
}

/* Define input media. */

const gdev_input_media_t gdev_input_media_default =
{
    gdev_input_media_default_values
};

int 
gdev_begin_input_media(gs_param_list * mlist, gs_param_dict * pdict,
		       int count)
{
    pdict->size = count;
    return param_begin_write_dict(mlist, "InputAttributes", pdict, true);
}

int
gdev_write_input_media(int index, gs_param_dict * pdict,
		       const gdev_input_media_t * pim)
{
    char key[25];
    gs_param_dict mdict;
    int code;
    gs_param_string as;

    sprintf(key, "%d", index);
    mdict.size = 4;
    code = param_begin_write_dict(pdict->list, key, &mdict, false);
    if (code < 0)
	return code;
    if ((pim->PageSize[0] != 0 && pim->PageSize[1] != 0) ||
	(pim->PageSize[2] != 0 && pim->PageSize[3] != 0)
	) {
	gs_param_float_array psa;

	psa.data = pim->PageSize;
	psa.size =
	    (pim->PageSize[0] == pim->PageSize[2] &&
	     pim->PageSize[1] == pim->PageSize[3] ? 2 : 4);
	psa.persistent = false;
	code = param_write_float_array(mdict.list, "PageSize",
				       &psa);
	if (code < 0)
	    return code;
    }
    if (pim->MediaColor != 0) {
	param_string_from_string(as, pim->MediaColor);
	code = param_write_string(mdict.list, "MediaColor",
				  &as);
	if (code < 0)
	    return code;
    }
    if (pim->MediaWeight != 0) {
	/*
	 * We do the following silly thing in order to avoid
	 * having to work around the 'const' in the arg list.
	 */
	float weight = pim->MediaWeight;

	code = param_write_float(mdict.list, "MediaWeight",
				 &weight);
	if (code < 0)
	    return code;
    }
    code = finish_media(mdict.list, "MediaType", pim->MediaType);
    if (code < 0)
	return code;
    return param_end_write_dict(pdict->list, key, &mdict);
}

int
gdev_write_input_page_size(int index, gs_param_dict * pdict,
			   floatp width_points, floatp height_points)
{
    gdev_input_media_t media;

    media.PageSize[0] = media.PageSize[2] = width_points;
    media.PageSize[1] = media.PageSize[3] = height_points;
    media.MediaColor = 0;
    media.MediaWeight = 0;
    media.MediaType = 0;
    return gdev_write_input_media(index, pdict, &media);
}

int 
gdev_end_input_media(gs_param_list * mlist, gs_param_dict * pdict)
{
    return param_end_write_dict(mlist, "InputAttributes", pdict);
}

/* Define output media. */

const gdev_output_media_t gdev_output_media_default =
{
    gdev_output_media_default_values
};

int 
gdev_begin_output_media(gs_param_list * mlist, gs_param_dict * pdict,
			int count)
{
    pdict->size = count;
    return param_begin_write_dict(mlist, "OutputAttributes", pdict, true);
}

int
gdev_write_output_media(int index, gs_param_dict * pdict,
			const gdev_output_media_t * pom)
{
    char key[25];
    gs_param_dict mdict;
    int code;

    sprintf(key, "%d", index);
    mdict.size = 4;
    code = param_begin_write_dict(pdict->list, key, &mdict, false);
    if (code < 0)
	return code;
    code = finish_media(mdict.list, "OutputType", pom->OutputType);
    if (code < 0)
	return code;
    return param_end_write_dict(pdict->list, key, &mdict);
}

int 
gdev_end_output_media(gs_param_list * mlist, gs_param_dict * pdict)
{
    return param_end_write_dict(mlist, "OutputAttributes", pdict);
}

/* ================ Putting parameters ================ */

/* Forward references */
private int param_anti_alias_bits(P3(gs_param_list *, gs_param_name, int *));
private int param_MediaSize(P4(gs_param_list *, gs_param_name,
			       const float *, gs_param_float_array *));

private int param_check_bool(P4(gs_param_list *, gs_param_name, bool, bool));
private int param_check_long(P4(gs_param_list *, gs_param_name, long, bool));

#define param_check_int(plist, pname, ival, defined)\
  param_check_long(plist, pname, (long)(ival), defined)
private int param_check_bytes(P5(gs_param_list *, gs_param_name, const byte *, uint, bool));

#define param_check_string(plist, pname, str, defined)\
  param_check_bytes(plist, pname, (const byte *)str, strlen(str), defined)

/* Set the device parameters. */
/* If the device was open and the put_params procedure closed it, */
/* return 1; otherwise, return 0 or an error code as usual. */
int
gs_putdeviceparams(gx_device * dev, gs_param_list * plist)
{
    bool was_open = dev->is_open;
    int code;

    gx_device_set_procs(dev);
    fill_dev_proc(dev, put_params, gx_default_put_params);
    fill_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    code = (*dev_proc(dev, put_params)) (dev, plist);
    return (code < 0 ? code : was_open && !dev->is_open ? 1 : code);
}

/* Set standard parameters. */
/* Note that setting the size or resolution closes the device. */
/* Window devices that don't want this to happen must temporarily */
/* set is_open to false before calling gx_default_put_params, */
/* and then taking appropriate action afterwards. */
int
gx_default_put_params(gx_device * dev, gs_param_list * plist)
{
    int ecode = 0;
    int code;
    gs_param_name param_name;
    gs_param_float_array hwra;
    gs_param_int_array hwsa;
    gs_param_float_array msa;
    gs_param_float_array ma;
    gs_param_float_array hwma;
    gs_param_float_array mhwra;
    gs_param_string_array scna;
    int nci = dev->NumCopies;
    int ncset = dev->NumCopies_set;
    bool ignc = dev->IgnoreNumCopies;
    bool ucc = dev->UseCIEColor;
    gs_param_float_array ibba;
    bool ibbnull = false;
    int colors = dev->color_info.num_components;
    int depth = dev->color_info.depth;
    int GrayValues = dev->color_info.max_gray + 1;
    int RGBValues = dev->color_info.max_color + 1;
    long ColorValues = 1L << depth;
    int tab = dev->color_info.anti_alias.text_bits;
    int gab = dev->color_info.anti_alias.graphics_bits;
    gs_param_string cms;

    /*
     * Template:
     *   BEGIN_ARRAY_PARAM(param_read_xxx_array, "pname", pxxa, size, pxxe) {
     *     ... check value if desired ...
     *     if (success)
     *       break;
     *     ... set ecode ...
     *   } END_ARRAY_PARAM(pxxa, pxxe);
     */

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

    /*
     * The HWResolution, HWSize, and MediaSize parameters interact in
     * the following way:
     *      1. Setting HWResolution recomputes HWSize from MediaSize.
     *      2. Setting HWSize recomputes MediaSize from HWResolution.
     *      3. Setting MediaSize recomputes HWSize from HWResolution.
     * If more than one parameter is being set, we apply these rules
     * in the order 1, 2, 3.  This does the right thing in the most
     * common case of setting more than one parameter, namely,
     * setting both HWResolution and HWSize.
     */

    BEGIN_ARRAY_PARAM(param_read_float_array, "HWResolution", hwra, 2, hwre) {
	if (hwra.data[0] <= 0 || hwra.data[1] <= 0)
	    ecode = gs_note_error(gs_error_rangecheck);
	else
	    break;
    } END_ARRAY_PARAM(hwra, hwre);
    BEGIN_ARRAY_PARAM(param_read_int_array, "HWSize", hwsa, 2, hwsa) {
	/* We need a special check to handle the nullpage device, */
	/* whose size is legitimately [0 0]. */
	if ((hwsa.data[0] <= 0 && hwsa.data[0] != dev->width) ||
	    (hwsa.data[1] <= 0 && hwsa.data[1] != dev->height)
	)
	    ecode = gs_note_error(gs_error_rangecheck);
#define max_coord (max_fixed / fixed_1)
#if max_coord < max_int
	else if (hwsa.data[0] > max_coord || hwsa.data[1] > max_coord)
	    ecode = gs_note_error(gs_error_limitcheck);
#endif
#undef max_coord
	else
	    break;
    } END_ARRAY_PARAM(hwsa, hwse);
    {
	const float *res = (hwra.data == 0 ? dev->HWResolution : hwra.data);

#ifdef PAGESIZE_IS_MEDIASIZE
	const float *data;

	/* .MediaSize takes precedence over PageSize, so */
	/* we read PageSize first. */
	code = param_MediaSize(plist, "PageSize", res, &msa);
	if (code < 0)
	    ecode = code;
	/* Prevent data from being set to 0 if PageSize is specified */
	/* but .MediaSize is not. */
	data = msa.data;
	code = param_MediaSize(plist, ".MediaSize", res, &msa);
	if (code < 0)
	    ecode = code;
	else if (msa.data == 0)
	    msa.data = data;
#else
	code = param_MediaSize(plist, ".MediaSize", res, &msa);
	if (code < 0)
	    ecode = code;
#endif
    }

    BEGIN_ARRAY_PARAM(param_read_float_array, "Margins", ma, 2, me) {
	break;
    } END_ARRAY_PARAM(ma, me);
    BEGIN_ARRAY_PARAM(param_read_float_array, ".HWMargins", hwma, 4, hwme) {
	break;
    } END_ARRAY_PARAM(hwma, hwme);
    /* MarginsHWResolution cannot be changed, only checked. */
    BEGIN_ARRAY_PARAM(param_read_float_array, ".MarginsHWResolution", mhwra, 2, mhwre) {
	if (mhwra.data[0] != dev->MarginsHWResolution[0] ||
	    mhwra.data[1] != dev->MarginsHWResolution[1]
	)
	    ecode = gs_note_error(gs_error_rangecheck);
	else
	    break;
    } END_ARRAY_PARAM(mhwra, mhwre);
    switch (code = param_read_bool(plist, (param_name = ".IgnoreNumCopies"), &ignc)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }
    if (dev->NumCopies_set >= 0 &&
	(*dev_proc(dev, get_page_device))(dev) != 0
	) {
	switch (code = param_read_int(plist, (param_name = "NumCopies"), &nci)) {
	case 0:
	    if (nci < 0)
		ecode = gs_error_rangecheck;
	    else {
		ncset = 1;
		break;
	    }
	    goto nce;
	default:
	    if ((code = param_read_null(plist, param_name)) == 0) {
		ncset = 0;
		break;
	    }
	    ecode = code;	/* can't be 1 */
nce:
	    param_signal_error(plist, param_name, ecode);
	case 1:
	    break;
    }
    }
    if ((code = param_read_bool(plist, (param_name = "UseCIEColor"), &ucc)) < 0) {
	ecode = code;
	param_signal_error(plist, param_name, ecode);
    }
    if ((code = param_anti_alias_bits(plist, "TextAlphaBits", &tab)) < 0)
	ecode = code;
    if ((code = param_anti_alias_bits(plist, "GraphicsAlphaBits", &gab)) < 0)
	ecode = code;

    /* Ignore parameters that only have meaning for printers. */
#define IGNORE_INT_PARAM(pname)\
  { int igni;\
    switch ( code = param_read_int(plist, (param_name = pname), &igni) )\
      { default:\
	  ecode = code;\
	  param_signal_error(plist, param_name, ecode);\
	case 0:\
	case 1:\
	  break;\
      }\
  }
    IGNORE_INT_PARAM("%MediaSource")
	IGNORE_INT_PARAM("%MediaDestination")
	switch (code = param_read_float_array(plist, (param_name = "ImagingBBox"), &ibba)) {
	case 0:
	    if (ibba.size != 4 ||
		ibba.data[2] < ibba.data[0] || ibba.data[3] < ibba.data[1]
		)
		ecode = gs_note_error(gs_error_rangecheck);
	    else
		break;
	    goto ibbe;
	default:
	    if ((code = param_read_null(plist, param_name)) == 0) {
		ibbnull = true;
		ibba.data = 0;
		break;
	    }
	    ecode = code;	/* can't be 1 */
	  ibbe:param_signal_error(plist, param_name, ecode);
	case 1:
	    ibba.data = 0;
	    break;
    }

    /* Now check nominally read-only parameters. */
    if ((code = param_check_string(plist, "OutputDevice", dev->dname, true)) < 0)
	ecode = code;
    if ((code = param_check_string(plist, "ProcessColorModel", pcmsa[colors], colors != 0)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "MaxSeparations", 1, true)) < 0)
	ecode = code;
    if ((code = param_check_bool(plist, "Separations", false, true)) < 0)
	ecode = code;
    BEGIN_ARRAY_PARAM(param_read_name_array, "SeparationColorNames", scna, 0, scne) {
	break;
    } END_ARRAY_PARAM(scna, scne);
    if ((code = param_check_string(plist, "Name", dev->dname, true)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "Colors", colors, true)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "BitsPerPixel", depth, true)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "GrayValues", GrayValues, true)) < 0)
	ecode = code;
    if ((code = param_check_long(plist, "PageCount", dev->PageCount, true)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "RedValues", RGBValues, colors > 1)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "GreenValues", RGBValues, colors > 1)) < 0)
	ecode = code;
    if ((code = param_check_int(plist, "BlueValues", RGBValues, colors > 1)) < 0)
	ecode = code;
    if ((code = param_check_long(plist, "ColorValues", ColorValues, colors > 1)) < 0)
	ecode = code;
    if (param_read_string(plist, "HWColorMap", &cms) != 1) {
	byte palette[3 << 8];

	if (param_HWColorMap(dev, palette))
	    code = param_check_bytes(plist, "HWColorMap", palette,
				     colors << depth, true);
	else
	    code = param_check_bytes(plist, "HWColorMap", 0, 0, false);
	if (code < 0)
	    ecode = code;
    }

    /* We must 'commit', in order to detect unknown parameters, */
    /* even if there were errors. */
    code = param_commit(plist);
    if (ecode < 0)
	return ecode;
    if (code < 0)
	return code;

    /* Now actually make the changes. */
    /* Changing resolution or page size requires closing the device, */
    /* but changing margins or ImagingBBox does not. */
    /* In order not to close and reopen the device unnecessarily, */
    /* we check for replacing the values with the same ones. */

    if (hwra.data != 0 &&
	(dev->HWResolution[0] != hwra.data[0] ||
	 dev->HWResolution[1] != hwra.data[1])
	) {
	if (dev->is_open)
	    gs_closedevice(dev);
	gx_device_set_resolution(dev, hwra.data[0], hwra.data[1]);
    }
    if (hwsa.data != 0 &&
	(dev->width != hwsa.data[0] ||
	 dev->height != hwsa.data[1])
	) {
	if (dev->is_open)
	    gs_closedevice(dev);
	gx_device_set_width_height(dev, hwsa.data[0], hwsa.data[1]);
    }
    if (msa.data != 0 &&
	(dev->MediaSize[0] != msa.data[0] ||
	 dev->MediaSize[1] != msa.data[1])
	) {
	if (dev->is_open)
	    gs_closedevice(dev);
	gx_device_set_page_size(dev, msa.data[0], msa.data[1]);
    }
    if (ma.data != 0) {
	dev->Margins[0] = ma.data[0];
	dev->Margins[1] = ma.data[1];
    }
    if (hwma.data != 0) {
	dev->HWMargins[0] = hwma.data[0];
	dev->HWMargins[1] = hwma.data[1];
	dev->HWMargins[2] = hwma.data[2];
	dev->HWMargins[3] = hwma.data[3];
    }
    dev->NumCopies = nci;
    dev->NumCopies_set = ncset;
    dev->IgnoreNumCopies = ignc;
    if (ibba.data != 0) {
	dev->ImagingBBox[0] = ibba.data[0];
	dev->ImagingBBox[1] = ibba.data[1];
	dev->ImagingBBox[2] = ibba.data[2];
	dev->ImagingBBox[3] = ibba.data[3];
	dev->ImagingBBox_set = true;
    } else if (ibbnull) {
	dev->ImagingBBox_set = false;
    }
    dev->UseCIEColor = ucc;
    dev->color_info.anti_alias.text_bits = tab;
    dev->color_info.anti_alias.graphics_bits = gab;
    gx_device_decache_colors(dev);
    return 0;
}

/* Read TextAlphaBits or GraphicsAlphaBits. */
private int
param_anti_alias_bits(gs_param_list * plist, gs_param_name param_name, int *pa)
{
    int code = param_read_int(plist, param_name, pa);

    switch (code) {
    case 0:
	switch (*pa) {
	case 1: case 2: case 4:
	    return 0;
	default:
	    code = gs_error_rangecheck;
	}
    default:
	param_signal_error(plist, param_name, code);
    case 1:
	;
    }
    return code;
}


/* Read .MediaSize or, if supported as a synonym, PageSize. */
private int
param_MediaSize(gs_param_list * plist, gs_param_name pname,
		const float *res, gs_param_float_array * pa)
{
    gs_param_name param_name;
    int ecode = 0;
    int code;

    BEGIN_ARRAY_PARAM(param_read_float_array, pname, *pa, 2, mse) {
	float width_new = pa->data[0] * res[0] / 72;
	float height_new = pa->data[1] * res[1] / 72;

	if (width_new < 0 || height_new < 0)
	    ecode = gs_note_error(gs_error_rangecheck);
#define max_coord (max_fixed / fixed_1)
#if max_coord < max_int
	else if (width_new > max_coord || height_new > max_coord)
	    ecode = gs_note_error(gs_error_limitcheck);
#endif
#undef max_coord
	else
	    break;
    } END_ARRAY_PARAM(*pa, mse);
    return ecode;
}

/* Check that a nominally read-only parameter is being set to */
/* its existing value. */
private int
param_check_bool(gs_param_list * plist, gs_param_name pname, bool value,
		 bool defined)
{
    int code;
    bool new_value;

    switch (code = param_read_bool(plist, pname, &new_value)) {
	case 0:
	    if (defined && new_value == value)
		break;
	    code = gs_note_error(gs_error_rangecheck);
	    goto e;
	default:
	    if (param_read_null(plist, pname) == 0)
		return 1;
	  e:param_signal_error(plist, pname, code);
	case 1:
	    ;
    }
    return code;
}
private int
param_check_long(gs_param_list * plist, gs_param_name pname, long value,
		 bool defined)
{
    int code;
    long new_value;

    switch (code = param_read_long(plist, pname, &new_value)) {
	case 0:
	    if (defined && new_value == value)
		break;
	    code = gs_note_error(gs_error_rangecheck);
	    goto e;
	default:
	    if (param_read_null(plist, pname) == 0)
		return 1;
	  e:param_signal_error(plist, pname, code);
	case 1:
	    ;
    }
    return code;
}
private int
param_check_bytes(gs_param_list * plist, gs_param_name pname, const byte * str,
		  uint size, bool defined)
{
    int code;
    gs_param_string new_value;

    switch (code = param_read_string(plist, pname, &new_value)) {
	case 0:
	    if (defined && new_value.size == size &&
		!memcmp((const char *)str, (const char *)new_value.data,
			size)
		)
		break;
	    code = gs_note_error(gs_error_rangecheck);
	    goto e;
	default:
	    if (param_read_null(plist, pname) == 0)
		return 1;
	  e:param_signal_error(plist, pname, code);
	case 1:
	    ;
    }
    return code;
}
