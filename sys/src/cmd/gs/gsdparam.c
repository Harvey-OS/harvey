/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsdparam.c */
/* Default device parameters for Ghostscript library */
#include "memory_.h"			/* for memcpy */
#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gxdevice.h"
#include "gxfixed.h"

/* Get the device parameters. */
int
gs_getdeviceparams(gx_device *dev, gs_param_list *plist)
{	gx_device_set_procs(dev);
	fill_dev_proc(dev, get_params, gx_default_get_params);
	return (*dev_proc(dev, get_params))(dev, plist);
}

/* Standard ProcessColorModel values. */
static const char *pcmsa[] = {
  0, "DeviceGray", 0, "DeviceRGB", "DeviceCMYK"
};

/* Get standard parameters. */
int
gx_default_get_params(gx_device *dev, gs_param_list *plist)
{
	int code;

	/* Standard page device parameters: */

	gs_param_string dns, pcms;
	gs_param_float_array psa, ibba, hwra, ma;
#define set_param_array(a, d, s)\
  (a.data = d, a.size = s, a.persistent = false);

	/* Non-standard parameters: */

	int colors = dev->color_info.num_components;
	int depth = dev->color_info.depth;
	int GrayValues = dev->color_info.max_gray + 1;
	int HWSize[2];
	  gs_param_int_array hwsa;
	float InitialMatrix[6];
	  gs_param_float_array ima;
	gs_param_float_array hwma;

	/* Fill in page device parameters. */

	param_string_from_string(dns, dev->dname);
	{	const char *cms = pcmsa[dev->color_info.num_components];
		/* We might have an uninitialized device with */
		/* color_info.num_components = 0.... */
		if ( cms != 0 )
		  param_string_from_string(pcms, cms);
		else
		  pcms.data = 0;
	}
	set_param_array(hwra, dev->HWResolution, 2);
	set_param_array(psa, dev->PageSize, 2);
	set_param_array(ibba, dev->ImagingBBox, 4);
	set_param_array(ma, dev->Margins, 2);

	/* Fill in non-standard parameters. */

	HWSize[0] = dev->width;
	HWSize[1] = dev->height;
	set_param_array(hwsa, HWSize, 2);
	fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
	{	gs_matrix mat;
		(*dev_proc(dev, get_initial_matrix))(dev, &mat);
		InitialMatrix[0] = mat.xx;
		InitialMatrix[1] = mat.xy;
		InitialMatrix[2] = mat.yx;
		InitialMatrix[3] = mat.yy;
		InitialMatrix[4] = mat.tx;
		InitialMatrix[5] = mat.ty;
	}
	set_param_array(ima, InitialMatrix, 6);
	set_param_array(hwma, dev->HWMargins, 4);

	/* Transmit the values. */

	if (
			/* Standard parameters */

	     (code = param_write_name(plist, "OutputDevice", &dns)) < 0 ||
	     (code = (pcms.data == 0 ? 0 :
		      param_write_name(plist, "ProcessColorModel", &pcms))) < 0 ||
	     (code = param_write_float_array(plist, "HWResolution", &hwra)) < 0 ||
	     (code = param_write_float_array(plist, "PageSize", &psa)) < 0 ||
	     (code = (dev->ImagingBBox_set ?
		      param_write_float_array(plist, "ImagingBBox", &ibba) :
		      param_write_null(plist, "ImagingBBox"))) < 0 ||
	     (code = param_write_float_array(plist, "Margins", &ma)) < 0 ||
	     (code = (dev->Orientation_set ?
		      param_write_int(plist, "Orientation", &dev->Orientation) :
		      param_write_null(plist, "Orientation"))) < 0 ||

			/* Non-standard parameters */

	     (code = param_write_int_array(plist, "HWSize", &hwsa)) < 0 ||
	     (code = param_write_float_array(plist, "InitialMatrix", &ima)) < 0 ||
	     (code = param_write_float_array(plist, ".HWMargins", &hwma)) < 0 ||
	     (code = param_write_string(plist, "Name", &dns)) < 0 ||
	     (code = param_write_int(plist, "Colors", &colors)) < 0 ||
	     (code = param_write_int(plist, ".HWBitsPerPixel", &depth)) < 0 ||
	     (code = param_write_int(plist, "GrayValues", &GrayValues)) < 0 ||
	     (code = param_write_long(plist, ".PageCount", &dev->PageCount)) < 0

	   )
		return code;

	/* Fill in color information. */

	if ( colors > 1 )
	{	int RGBValues = dev->color_info.max_color + 1;
		long ColorValues = 1L << depth;
		if ( (code = param_write_int(plist, "RedValues", &RGBValues)) < 0 ||
		     (code = param_write_int(plist, "GreenValues", &RGBValues)) < 0 ||
		     (code = param_write_int(plist, "BlueValues", &RGBValues)) < 0 ||
		     (code = param_write_long(plist, "ColorValues", &ColorValues)) < 0
		   )
			return code;
	}

	if ( depth <= 8 && colors <= 3 )
	{	byte palette[3 << 8];
		byte *p = palette;
		gs_param_string hwcms;
		gx_color_value rgb[3];
		gx_color_index i;
		if ( palette == 0 )
			return_error(gs_error_VMerror);
		for ( i = 0; (i >> depth) == 0; i++ )
		{	int j;
			(*dev_proc(dev, map_color_rgb))(dev, i, rgb);
			for ( j = 0; j < colors; j++ )
				*p++ = gx_color_value_to_byte(rgb[j]);
		}
		hwcms.data = palette, hwcms.size = colors << depth, hwcms.persistent = false;
		if ( (code = param_write_string(plist, "HWColorMap", &hwcms)) < 0 )
			return code;
	}

	return 0;
}

/* Set the device parameters. */
/* If the device was open and the put_params procedure closed it, */
/* return 1; otherwise, return 0 or an error code as usual. */
int
gs_putdeviceparams(gx_device *dev, gs_param_list *plist)
{	bool was_open = dev->is_open;
	int code;
	fill_dev_proc(dev, put_params, gx_default_put_params);
	code = (*dev_proc(dev, put_params))(dev, plist);
	return (code < 0 ? code : was_open && !dev->is_open ? 1 : code);
}

/* Set standard parameters. */
/* Note that setting the size or resolution closes the device. */
/* Window devices that don't want this to happen must temporarily */
/* set is_open to false before calling gx_default_put_params, */
/* and then taking appropriate action afterwards. */
int
gx_default_put_params(gx_device *dev, gs_param_list *plist)
{	int ecode = 0;
	int code;
	const char _ds *param_name;
	gs_param_float_array hwra;
	gs_param_int_array hwsa;
	gs_param_float_array psa;
	float page_width, page_height;
	gs_param_float_array ma;
	gs_param_float_array hwma;
	gs_param_float_array ibba;
	bool ibbnull = false;

#define BEGIN_ARRAY_PARAM(pread, pname, pa, psize, e)\
  switch ( code = pread(plist, (param_name = pname), &pa) )\
  {\
  case 0:\
	if ( pa.size != psize )\
	  ecode = gs_error_rangecheck;\
	else { 
/* The body of the processing code goes here. */
/* If it succeeds, it should do a 'break'; */
/* if it fails, it should set ecode and fall through. */
#define END_PARAM(pa, e)\
	}\
	goto e;\
  default:\
	ecode = code;\
e:	param_signal_error(plist, param_name, ecode);\
  case 1:\
	pa.data = 0;		/* mark as not filled */\
  }

	/* We check the parameters in the order HWResolution, HWSize, */
	/* PageSize so that PageSize supersedes HWSize and so that */
	/* HWSize and PageSize each have the resolution available to */
	/* compute the other one. */

	BEGIN_ARRAY_PARAM(param_read_float_array, "HWResolution", hwra, 2, hwre)
		if ( hwra.data[0] <= 0 || hwra.data[1] <= 0 )
		  ecode = gs_error_rangecheck;
		else
		  break;
	END_PARAM(hwra, hwre)

	BEGIN_ARRAY_PARAM(param_read_int_array, "HWSize", hwsa, 2, hwsa)
		if ( hwsa.data[0] <= 0 || hwsa.data[1] <= 0 )
		  ecode = gs_error_rangecheck;
#define max_coord (max_fixed / fixed_1)
#if max_coord < max_int
		else if ( hwsa.data[0] > max_coord || hwsa.data[1] > max_coord )
		  ecode = gs_error_limitcheck;
#endif
#undef max_coord
		else
		  break;
	END_PARAM(hwsa, hwse)

	BEGIN_ARRAY_PARAM(param_read_float_array, "PageSize", psa, 2, pse)
		if ( (page_width = psa.data[0] * dev->x_pixels_per_inch / 72) <= 0 ||
		     (page_height = psa.data[1] * dev->y_pixels_per_inch / 72) <= 0
		   )
		  ecode = gs_error_rangecheck;
#define max_coord (max_fixed / fixed_1)
#if max_coord < max_int
		else if ( page_width > max_coord || page_height > max_coord )
		  ecode = gs_error_limitcheck;
#endif
#undef max_coord
		else
		  break;
	END_PARAM(psa, pse)

	BEGIN_ARRAY_PARAM(param_read_float_array, "Margins", ma, 2, me)
		break;
	END_PARAM(ma, me)

	BEGIN_ARRAY_PARAM(param_read_float_array, ".HWMargins", hwma, 4, hwme)
		break;
	END_PARAM(hwma, hwme)

	switch ( code = param_read_float_array(plist, (param_name = "ImagingBBox"), &ibba) )
	{
	case 0:
		if ( ibba.size != 4 || ibba.data[0] < 0 || ibba.data[1] < 0 ||
		     ibba.data[2] < ibba.data[0] || ibba.data[3] < ibba.data[1]
		   )
		  ecode = gs_error_rangecheck;
		else
		  break;
		goto ibbe;
	default:
		if ( (code = param_read_null(plist, param_name)) == 0 )
		{	ibbnull = true;
			ibba.data = 0;
			break;
		}
		ecode = code;	/* can't be 1 */
ibbe:		param_signal_error(plist, param_name, ecode);
	case 1:
		ibba.data = 0;
		break;
	}

	if ( ecode < 0 )
	  return ecode;
	code = param_commit(plist);
	if ( code < 0 )
	  return code;

	/* Now actually make the changes. */
	/* Changing resolution or page size requires closing the device, */
	/* but changing margins or ImagingBBox does not. */
	/* In order not to close and reopen the device unnecessarily, */
	/* we check for replacing the values with the same ones. */

	if ( hwra.data != 0 &&
	      (dev->HWResolution[0] != hwra.data[0] ||
	       dev->HWResolution[1] != hwra.data[1])
	   )
	  {	if ( dev->is_open )
		  gs_closedevice(dev);
		dev->HWResolution[0] = hwra.data[0];
		dev->HWResolution[1] = hwra.data[1];
	  }
	if ( hwsa.data != 0 &&
	      (dev->width != hwsa.data[0] ||
	       dev->height != hwsa.data[1])
	   )
	  {	if ( dev->is_open )
		  gs_closedevice(dev);
		dev->width = hwsa.data[0];
		dev->height = hwsa.data[1];
		dev->PageSize[0] =
		  hwsa.data[0] * 72.0 / dev->x_pixels_per_inch;
		dev->PageSize[1] =
		  hwsa.data[1] * 72.0 / dev->y_pixels_per_inch;
	  }
	if ( psa.data != 0 &&
	      (dev->PageSize[0] != psa.data[0] ||
	       dev->PageSize[1] != psa.data[1])
	   )
	  {	if ( dev->is_open )
		  gs_closedevice(dev);
		dev->width = page_width;
		dev->height = page_height;
		dev->PageSize[0] = psa.data[0];
		dev->PageSize[1] = psa.data[1];
	  }
	if ( ma.data != 0 )
	  {	dev->Margins[0] = ma.data[0];
		dev->Margins[1] = ma.data[1];
	  }
	if ( hwma.data != 0 )
	  {	dev->HWMargins[0] = hwma.data[0];
		dev->HWMargins[1] = hwma.data[1];
		dev->HWMargins[2] = hwma.data[2];
		dev->HWMargins[3] = hwma.data[3];
	  }
	if ( ibba.data != 0 )
	  {	dev->ImagingBBox[0] = ibba.data[0];
		dev->ImagingBBox[1] = ibba.data[1];
		dev->ImagingBBox[2] = ibba.data[2];
		dev->ImagingBBox[3] = ibba.data[3];
		dev->ImagingBBox_set = true;
	  }
	else if ( ibbnull )
	  {	dev->ImagingBBox_set = false;
	  }
	return 0;
}
