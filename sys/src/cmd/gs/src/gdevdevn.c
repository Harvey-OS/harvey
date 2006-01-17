/* Copyright (C) 1991, 1995 to 1999 Aladdin Enterprises. 2001 ArtifexSoftware Inc.  All rights reserved.
  
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

/*$Id: gdevdevn.c,v 1.28 2005/10/01 04:40:18 dan Exp $ */
/* Example DeviceN process color model devices. */

#include "math_.h"
#include "string_.h"
#include "gdevprn.h"
#include "gsparam.h"
#include "gscrd.h"
#include "gscrdp.h"
#include "gxlum.h"
#include "gdevdcrd.h"
#include "gstypes.h"
#include "gxdcconv.h"
#include "gdevdevn.h"
#include "gsequivc.h"

/*
 * Utility routines for common DeviceN related parameters:
 *   SeparationColorNames, SeparationOrder, and MaxSeparations
 */

/* Convert a gray color space to DeviceN colorants. */
void
gray_cs_to_devn_cm(gx_device * dev, int * map, frac gray, frac out[])
{
    int i = dev->color_info.num_components - 1;

    for(; i >= 0; i--)			/* Clear colors */
        out[i] = frac_0;
    if ((i = map[3]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = frac_1 - gray;
}

/* Convert an RGB color space to DeviceN colorants. */
void
rgb_cs_to_devn_cm(gx_device * dev, int * map,
		const gs_imager_state *pis, frac r, frac g, frac b, frac out[])
{
    int i = dev->color_info.num_components - 1;
    frac cmyk[4];

    for(; i >= 0; i--)			/* Clear colors */
        out[i] = frac_0;
    color_rgb_to_cmyk(r, g, b, pis, cmyk);
    if ((i = map[0]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = cmyk[0];
    if ((i = map[1]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = cmyk[1];
    if ((i = map[2]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = cmyk[2];
    if ((i = map[3]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = cmyk[3];
}

/* Convert a CMYK color space to DeviceN colorants. */
void
cmyk_cs_to_devn_cm(gx_device * dev, int * map,
		frac c, frac m, frac y, frac k, frac out[])
{
    int i = dev->color_info.num_components - 1;

    for(; i >= 0; i--)			/* Clear colors */
        out[i] = frac_0;
    if ((i = map[0]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = c;
    if ((i = map[1]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = m;
    if ((i = map[2]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = y;
    if ((i = map[3]) != GX_DEVICE_COLOR_MAX_COMPONENTS)
        out[i] = k;
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
int
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

#define compare_color_names(name, name_size, str, str_size) \
    (name_size == str_size && \
	(strncmp((const char *)name, (const char *)str, name_size) == 0))

/*
 * This routine will check if a name matches any item in a list of process
 * color model colorant names.
 */
private bool
check_process_color_names(fixed_colorant_names_list plist,
			  const gs_param_string * pstring)
{
    if (plist) {
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
 * This routine will check to see if the color component name  match those
 * of either the process color model colorants or the names on the
 * SeparationColorNames list.
 *
 * Parameters:
 *   dev - pointer to device data structure.
 *   pname - pointer to name (zero termination not required)
 *   nlength - length of the name
 *
 * This routine returns a positive value (0 to n) which is the device colorant
 * number if the name is found.  It returns a negative value if not found.
 */
int
check_pcm_and_separation_names(const gx_device * dev,
		const gs_devn_params * pparams, const char * pname,
		int name_size, int component_type)
{
    fixed_colorant_name * pcolor = pparams->std_colorant_names;
    int color_component_number = 0;
    int i;

    /* Check if the component is in the process color model list. */
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
	const gs_separations * separations = &pparams->separations;
	int num_spot = separations->num_separations;

	for (i=0; i<num_spot; i++) {
	    if (compare_color_names((const char *)separations->names[i].data,
		  separations->names[i].size, pname, name_size)) {
		return color_component_number;
	    }
	    color_component_number++;
	}
    }

    return -1;
}

/*
 * This routine will check to see if the color component name  match those
 * that are available amoung the current device's color components.  
 *
 * Parameters:
 *   dev - pointer to device data structure.
 *   pname - pointer to name (zero termination not required)
 *   nlength - length of the name
 *   component_type - separation name or not
 *   pdevn_params - pointer to device's DeviceN paramters
 *   pequiv_colors - pointer to equivalent color structure (may be NULL)
 *
 * This routine returns a positive value (0 to n) which is the device colorant
 * number if the name is found.  It returns GX_DEVICE_COLOR_MAX_COMPONENTS if
 * the color component is found but is not being used due to the
 * SeparationOrder device parameter.  It returns a negative value if not found.
 *
 * This routine will also add separations to the device if space is
 * available.
 */
int
devn_get_color_comp_index(const gx_device * dev, gs_devn_params * pdevn_params,
		    equivalent_cmyk_color_params * pequiv_colors,
		    const char * pname, int name_size, int component_type,
		    int auto_spot_colors)
{
    int num_order = pdevn_params->num_separation_order_names;
    int color_component_number = 0;
    int max_spot_colors = GX_DEVICE_MAX_SEPARATIONS;

    /*
     * Check if the component is in either the process color model list
     * or in the SeparationNames list.
     */
    color_component_number = check_pcm_and_separation_names(dev, pdevn_params,
					pname, name_size, component_type);

    /* If we have a valid component */
    if (color_component_number >= 0) {
        /* Check if the component is in the separation order map. */
        if (num_order)
	    color_component_number = 
		pdevn_params->separation_order_map[color_component_number];
	else
	    /*
	     * We can have more spot colors than we can image.  We simply
	     * ignore the component (i.e. treat it the same as we would
	     * treat a component that is not in the separation order map).
	     * Note:  Most device do not allow more spot colors than we can
	     * image.  (See the options for auto_spot_color in gdevdevn.h.)
	     */
	    if (color_component_number >= dev->color_info.num_components)
	        color_component_number = GX_DEVICE_COLOR_MAX_COMPONENTS;
	
        return color_component_number;
    }
    /*
     * The given name does not match any of our current components or
     * separations.  Check if we should add the spot color to our list.
     * If the SeparationOrder parameter has been specified then we should
     * already have our complete list of desired spot colorants.
     */
    if (component_type != SEPARATION_NAME ||
	    auto_spot_colors == NO_AUTO_SPOT_COLORS ||
	    pdevn_params->num_separation_order_names != 0)
	return -1;	/* Do not add --> indicate colorant unknown. */
    /*
     * Check if we have room for another spot colorant.
     */
    if (auto_spot_colors == ENABLE_AUTO_SPOT_COLORS)
	max_spot_colors = dev->color_info.num_components -
	    pdevn_params->num_std_colorant_names;
    if (pdevn_params->separations.num_separations < max_spot_colors) {
	byte * sep_name;
	gs_separations * separations = &pdevn_params->separations;
	int sep_num = separations->num_separations++;

	/* We have a new spot colorant */
	sep_name = gs_alloc_bytes(dev->memory,
			name_size, "devn_get_color_comp_index");
	memcpy(sep_name, pname, name_size);
	separations->names[sep_num].size = name_size;
	separations->names[sep_num].data = sep_name;
	color_component_number = sep_num + pdevn_params->num_std_colorant_names;
	pdevn_params->separation_order_map[color_component_number] =
					       color_component_number;

	if (pequiv_colors != NULL) {
    	    /* Indicate that we need to find equivalent CMYK color. */
	    pequiv_colors->color[sep_num].color_info_valid = false;
	    pequiv_colors->all_color_info_valid = false;
	}
    }

    return color_component_number;
}

#define set_param_array(a, d, s)\
  (a.data = d, a.size = s, a.persistent = false);

/* Get parameters.  We provide a default CRD. */
int
devn_get_params(gx_device * pdev, gs_param_list * plist,
    gs_devn_params * pdevn_params, equivalent_cmyk_color_params * pequiv_colors)
{
    int code;
    bool seprs = false;
    gs_param_string_array scna;
    gs_param_string_array sona;

    set_param_array(scna, NULL, 0);
    set_param_array(sona, NULL, 0);

    if ( (code = sample_device_crd_get_params(pdev, plist, "CRDDefault")) < 0 ||
	 (code =
	    param_write_name_array(plist, "SeparationColorNames", &scna)) < 0 ||
	 (code = param_write_name_array(plist, "SeparationOrder", &sona)) < 0 ||
	 (code = param_write_bool(plist, "Separations", &seprs)) < 0)
	return code;

    return 0;
}
#undef set_param_array

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
 * Utility routine for handling DeviceN related parameters.  This routine
 * may modify the color_info, devn_params, and the equiv_cmyk_colors fields.
 *
 * Note:  This routine does not restore values in case of a problem.  This
 * is left to the caller.
 */
int
devn_put_params(gx_device * pdev, gs_param_list * plist,
    gs_devn_params * pdevn_params, equivalent_cmyk_color_params * pequiv_colors)
{
    int code = 0, ecode;
    gs_param_name param_name;
    int npcmcolors = pdevn_params->num_std_colorant_names;
    int num_spot = pdevn_params->separations.num_separations;
    bool num_spot_changed = false;
    int num_order = pdevn_params->num_separation_order_names;
    int max_sep = pdevn_params->max_separations;
    gs_param_string_array scna;		/* SeparationColorNames array */
    gs_param_string_array sona;		/* SeparationOrder names array */

    /* Get the SeparationOrder names */
    BEGIN_ARRAY_PARAM(param_read_name_array, "SeparationOrder",
		    			sona, sona.size, sone)
    {
	break;
    } END_ARRAY_PARAM(sona, sone);
    if (sona.data != 0 && sona.size > GX_DEVICE_COLOR_MAX_COMPONENTS)
	return_error(gs_error_rangecheck);

    /* Get the SeparationColorNames */
    BEGIN_ARRAY_PARAM(param_read_name_array, "SeparationColorNames",
		    			scna, scna.size, scne)
    {
	break;
    } END_ARRAY_PARAM(scna, scne);
    if (scna.data != 0 && scna.size > GX_DEVICE_MAX_SEPARATIONS)
	return_error(gs_error_rangecheck);

    /* Separations are only valid with a subrtractive color model */
    if (pdev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE) {
        /*
         * Process the SeparationColorNames.  Remove any names that already
	 * match the process color model colorant names for the device.
         */
        if (scna.data != 0) {
	    int i;
	    int num_names = scna.size;
	    fixed_colorant_names_list pcomp_names = 
	        pdevn_params->std_colorant_names;

	    for (i = num_spot = 0; i < num_names; i++) {
		/* Verify that the name is not one of our process colorants */
	        if (!check_process_color_names(pcomp_names, &scna.data[i])) {
		    byte * sep_name;
		    int name_size = scna.data[i].size;

		    /* We have a new separation */
		    sep_name = (byte *)gs_alloc_bytes(pdev->memory,
			name_size, "devicen_put_params_no_sep_order");
		    memcpy(sep_name, scna.data[i].data, name_size);
	            pdevn_params->separations.names[num_spot].size = name_size;
	            pdevn_params->separations.names[num_spot].data = sep_name;
		    if (pequiv_colors != NULL) {
			/* Indicate that we need to find equivalent CMYK color. */
			pequiv_colors->color[num_spot].color_info_valid = false;
			pequiv_colors->all_color_info_valid = false;
		    }
		    num_spot++;
		}
	    }
	    pdevn_params->separations.num_separations = num_spot;
	    num_spot_changed = true;
	    for (i = 0; i < num_spot + npcmcolors; i++)
		pdevn_params->separation_order_map[i] = i;
        }
        /*
         * Process the SeparationOrder names.
         */
        if (sona.data != 0) {
	    int i, comp_num;

	    num_order = sona.size;
	    for (i = 0; i < num_spot + npcmcolors; i++)
		pdevn_params->separation_order_map[i] = GX_DEVICE_COLOR_MAX_COMPONENTS;
	    for (i = 0; i < num_order; i++) {
	        /*
	         * Check if names match either the process color model or
	         * SeparationColorNames.  If not then error.
	         */
	        if ((comp_num = check_pcm_and_separation_names(pdev, pdevn_params,
		    (const char *)sona.data[i].data, sona.data[i].size, 0)) < 0) {
		    return_error(gs_error_rangecheck);
		}
		pdevn_params->separation_order_map[comp_num] = i;
	    }
        }
        /*
         * Adobe says that MaxSeparations is supposed to be 'read only'
	 * however we use this to allow the specification of the maximum
	 * number of separations.  Memory is allocated for the specified
	 * number of separations.  This allows us to then accept separation
	 * colors in color spaces even if they we not specified at the start
	 * of the image file.
         */
        code = param_read_int(plist, param_name = "MaxSeparations", &max_sep);
        switch (code) {
            default:
	        param_signal_error(plist, param_name, code);
            case 1:
		break;
            case 0:
	        if (max_sep < 1 || max_sep > GX_DEVICE_COLOR_MAX_COMPONENTS)
		    return_error(gs_error_rangecheck);
	        {
		    int depth =
		        bpc_to_depth(max_sep, pdevn_params->bitspercomponent);

                    if (depth > 8 * size_of(gx_color_index))
		        return_error(gs_error_rangecheck);
    	            pdevn_params->max_separations =
		        pdev->color_info.max_components =
    	                pdev->color_info.num_components = max_sep;
                    pdev->color_info.depth = depth;
	        }
        }
        /* 
         * The DeviceN device can have zero components if nothing has been
	 * specified.  This causes some problems so force at least one
	 * component until something is specified.
         */
        if (!pdev->color_info.num_components)
	    pdev->color_info.num_components = 1;
	/*
	 * Update the number of device components if we have changes in
	 * SeparationColorNames, SeparationOrder, or MaxSeparations.
	 */
	if (num_spot_changed || pdevn_params->max_separations != max_sep ||
	    	    pdevn_params->num_separation_order_names != num_order) {
	    pdevn_params->separations.num_separations = num_spot;
	    pdevn_params->num_separation_order_names = num_order;
    	    pdevn_params->max_separations = max_sep;
	    /*
	     * If we have SeparationOrder specified then the number of
	     * components is given by the number of names in the list.
	     * Otherwise check if the MaxSeparations parameter has specified
	     * a value.  If so then use that value, otherwise use the number
	     * of ProcessColorModel components plus the number of
	     * SeparationColorNames is used.
	     */
            pdev->color_info.num_components = (num_order) ? num_order 
		: (pdevn_params->max_separations)
				? pdevn_params->max_separations
				: npcmcolors + num_spot;
            pdev->color_info.depth = bpc_to_depth(pdev->color_info.num_components, 
					pdevn_params->bitspercomponent);
	}
    }
    return code;
}

/*
 * Utility routine for handling DeviceN related parameters in a
 * standard raster printer type device.
 */
int
devn_printer_put_params(gx_device * pdev, gs_param_list * plist,
    gs_devn_params * pdevn_params, equivalent_cmyk_color_params * pequiv_colors)
{
    int code;
    /* Save current data in case we have a problem */
    gx_device_color_info save_info = pdev->color_info;
    gs_devn_params saved_devn_params = *pdevn_params;
    equivalent_cmyk_color_params saved_equiv_colors;

    if (pequiv_colors != NULL)
        saved_equiv_colors = *pequiv_colors;

    /* Use utility routine to handle parameters */
    code = devn_put_params(pdev, plist, pdevn_params, pequiv_colors);

    /* Check for default printer parameters */
    if (code >= 0)
        code = gdev_prn_put_params(pdev, plist);

    /* If we have an error then restore original data. */
    if (code < 0) {
	pdev->color_info = save_info;
	*pdevn_params = saved_devn_params;
	if (pequiv_colors != NULL)
	   *pequiv_colors = saved_equiv_colors;
	return code;
    }

    /* If anything changed, then close the device, etc. */
    if (memcmp(&pdev->color_info, &save_info, sizeof(gx_device_color_info)) ||
	memcmp(pdevn_params, &saved_devn_params,
					sizeof(gs_devn_params)) ||
	(pequiv_colors != NULL &&
	    memcmp(pequiv_colors, &saved_equiv_colors,
				sizeof(equivalent_cmyk_color_params)))) {
	gs_closedevice(pdev);
        /* Reset the sparable and linear shift, masks, bits. */
	set_linear_color_bits_mask_shift(pdev);
        pdev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
    }

    return code;
}

/* ***************** The spotcmyk and devicen devices ***************** */

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* The device descriptor */
private dev_proc_open_device(spotcmyk_prn_open);
private dev_proc_get_params(spotcmyk_get_params);
private dev_proc_put_params(spotcmyk_put_params);
private dev_proc_print_page(spotcmyk_print_page);
private dev_proc_get_color_mapping_procs(get_spotcmyk_color_mapping_procs);
private dev_proc_get_color_mapping_procs(get_devicen_color_mapping_procs);
private dev_proc_get_color_comp_index(spotcmyk_get_color_comp_index);
private dev_proc_encode_color(spotcmyk_encode_color);
private dev_proc_decode_color(spotcmyk_decode_color);

/*
 * A structure definition for a DeviceN type device
 */
typedef struct spotcmyk_device_s {
    gx_device_common;
    gx_prn_device_common;
    gs_devn_params devn_params;
} spotcmyk_device;

/* GC procedures */

private 
ENUM_PTRS_WITH(spotcmyk_device_enum_ptrs, spotcmyk_device *pdev)
{
    if (index < pdev->devn_params.separations.num_separations)
	ENUM_RETURN(pdev->devn_params.separations.names[index].data);
    ENUM_PREFIX(st_device_printer,
		    pdev->devn_params.separations.num_separations);
}

ENUM_PTRS_END
private RELOC_PTRS_WITH(spotcmyk_device_reloc_ptrs, spotcmyk_device *pdev)
{
    RELOC_PREFIX(st_device_printer);
    {
	int i;

	for (i = 0; i < pdev->devn_params.separations.num_separations; ++i) {
	    RELOC_PTR(spotcmyk_device, devn_params.separations.names[i].data);
	}
    }
}
RELOC_PTRS_END

/* Even though spotcmyk_device_finalize is the same as gx_device_finalize, */
/* we need to implement it separately because st_composite_final */
/* declares all 3 procedures as private. */
private void
spotcmyk_device_finalize(void *vpdev)
{
    gx_device_finalize(vpdev);
}

gs_private_st_composite_final(st_spotcmyk_device, spotcmyk_device,
    "spotcmyk_device", spotcmyk_device_enum_ptrs, spotcmyk_device_reloc_ptrs,
    spotcmyk_device_finalize);

/*
 * Macro definition for DeviceN procedures
 */
#define device_procs(get_color_mapping_procs)\
{	spotcmyk_prn_open,\
	gx_default_get_initial_matrix,\
	NULL,				/* sync_output */\
	gdev_prn_output_page,		/* output_page */\
	gdev_prn_close,			/* close */\
	NULL,				/* map_rgb_color - not used */\
	NULL,				/* map_color_rgb - not used */\
	NULL,				/* fill_rectangle */\
	NULL,				/* tile_rectangle */\
	NULL,				/* copy_mono */\
	NULL,				/* copy_color */\
	NULL,				/* draw_line */\
	NULL,				/* get_bits */\
	spotcmyk_get_params,		/* get_params */\
	spotcmyk_put_params,		/* put_params */\
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
	spotcmyk_get_color_comp_index,	/* get_color_comp_index */\
	spotcmyk_encode_color,		/* encode_color */\
	spotcmyk_decode_color,		/* decode_color */\
	NULL,				/* pattern_manage */\
	NULL				/* fill_rectangle_hl_color */\
}

fixed_colorant_name DeviceCMYKComponents[] = {
	"Cyan",
	"Magenta",
	"Yellow",
	"Black",
	0		/* List terminator */
};


#define spotcmyk_device_body(procs, dname, ncomp, pol, depth, mg, mc, cn)\
    std_device_full_body_type_extended(spotcmyk_device, &procs, dname,\
	  &st_spotcmyk_device,\
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
	prn_device_body_rest_(spotcmyk_print_page)

/*
 * Example device with CMYK and spot color support
 */
private const gx_device_procs spot_cmyk_procs = device_procs(get_spotcmyk_color_mapping_procs);

const spotcmyk_device gs_spotcmyk_device =
{   
    spotcmyk_device_body(spot_cmyk_procs, "spotcmyk", 4, GX_CINFO_POLARITY_SUBTRACTIVE, 4, 1, 1, "DeviceCMYK"),
    /* DeviceN device specific parameters */
    { 1,			/* Bits per color - must match ncomp, depth, etc. above */
      DeviceCMYKComponents,	/* Names of color model colorants */
      4,			/* Number colorants for CMYK */
      0,			/* MaxSeparations has not been specified */
      {0},			/* SeparationNames */
      0,			/* SeparationOrder names */
      {0, 1, 2, 3, 4, 5, 6, 7 }	/* Initial component SeparationOrder */
    }
};

/*
 * Example DeviceN color device
 */
private const gx_device_procs devicen_procs = device_procs(get_devicen_color_mapping_procs);

const spotcmyk_device gs_devicen_device =
{   
    spotcmyk_device_body(devicen_procs, "devicen", 4, GX_CINFO_POLARITY_SUBTRACTIVE, 32, 255, 255, "DeviceCMYK"),
    /* DeviceN device specific parameters */
    { 8,			/* Bits per color - must match ncomp, depth, etc. above */
      NULL,			/* No names for standard DeviceN color model */
      0,			/* No standard colorants for DeviceN */
      0,			/* MaxSeparations has not been specified */
      {0},			/* SeparationNames */
      0,			/* SeparationOrder names */
      {0, 1, 2, 3, 4, 5, 6, 7 }	/* Initial component SeparationOrder */
    }
};

/* Open the psd devices */
int
spotcmyk_prn_open(gx_device * pdev)
{
    int code = gdev_prn_open(pdev);

    set_linear_color_bits_mask_shift(pdev);
    pdev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
    return code;
}

/* Color mapping routines for the spotcmyk device */

private void
gray_cs_to_spotcmyk_cm(gx_device * dev, frac gray, frac out[])
{
    int * map = ((spotcmyk_device *) dev)->devn_params.separation_order_map;

    gray_cs_to_devn_cm(dev, map, gray, out);
}

private void
rgb_cs_to_spotcmyk_cm(gx_device * dev, const gs_imager_state *pis,
				   frac r, frac g, frac b, frac out[])
{
    int * map = ((spotcmyk_device *) dev)->devn_params.separation_order_map;

    rgb_cs_to_devn_cm(dev, map, pis, r, g, b, out);
}

private void
cmyk_cs_to_spotcmyk_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
    int * map = ((spotcmyk_device *) dev)->devn_params.separation_order_map;

    cmyk_cs_to_devn_cm(dev, map, c, m, y, k, out);
}

private const gx_cm_color_map_procs spotCMYK_procs = {
    gray_cs_to_spotcmyk_cm, rgb_cs_to_spotcmyk_cm, cmyk_cs_to_spotcmyk_cm
};

private const gx_cm_color_map_procs *
get_spotcmyk_color_mapping_procs(const gx_device * dev)
{
    return &spotCMYK_procs;
}

/* Also use the spotcmyk procs for the devicen device. */

private const gx_cm_color_map_procs *
get_devicen_color_mapping_procs(const gx_device * dev)
{
    return &spotCMYK_procs;
}


/*
 * Encode a list of colorant values into a gx_color_index_value.
 */
private gx_color_index
spotcmyk_encode_color(gx_device *dev, const gx_color_value colors[])
{
    int bpc = ((spotcmyk_device *)dev)->devn_params.bitspercomponent;
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
spotcmyk_decode_color(gx_device * dev, gx_color_index color, gx_color_value * out)
{
    int bpc = ((spotcmyk_device *)dev)->devn_params.bitspercomponent;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    int mask = (1 << bpc) - 1;
    int i = 0;
    int ncomp = dev->color_info.num_components;

    for (; i<ncomp; i++) {
        out[ncomp - i - 1] = (gx_color_value)((color & mask) << drop);
	color >>= bpc;
    }
    return 0;
}

/* Get parameters. */
private int
spotcmyk_get_params(gx_device * pdev, gs_param_list * plist)
{
    int code = gdev_prn_get_params(pdev, plist);

    if (code < 0)
	return code;
    return devn_get_params(pdev, plist,
    	&(((spotcmyk_device *)pdev)->devn_params), NULL);
}

/* Set parameters. */
private int
spotcmyk_put_params(gx_device * pdev, gs_param_list * plist)
{
    return devn_printer_put_params(pdev, plist,
	&(((spotcmyk_device *)pdev)->devn_params), NULL);
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
 * number if the name is found.  It returns GX_DEVICE_COLOR_MAX_COMPONENTS if
 * the colorant is not being used due to a SeparationOrder device parameter.
 * It returns a negative value if not found.
 */
private int
spotcmyk_get_color_comp_index(gx_device * dev, const char * pname,
					int name_size, int component_type)
{
    return devn_get_color_comp_index(dev,
		&(((spotcmyk_device *)dev)->devn_params), NULL,
		pname, name_size, component_type, ENABLE_AUTO_SPOT_COLORS);
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
int
repack_data(byte * source, byte * dest, int depth, int first_bit,
		int bit_width, int npixel)
{
    int in_nbyte = depth >> 3;		/* Number of bytes per input pixel */
    int out_nbyte = bit_width >> 3;	/* Number of bytes per output pixel */
    gx_color_index mask = 1;
    gx_color_index data;
    int i, j, length = 0;
    byte temp;
    byte * out = dest;
    int in_bit_start = 8 - depth;
    int out_bit_start = 8 - bit_width;
    int in_byte_loc = in_bit_start, out_byte_loc = out_bit_start;

    mask = (mask << bit_width) - 1;
    for (i=0; i<npixel; i++) {
        /* Get the pixel data */
	if (!in_nbyte) {		/* Multiple pixels per byte */
	    data = *source;
	    data >>= in_byte_loc;
	    in_byte_loc -= depth;
	    if (in_byte_loc < 0) {	/* If finished with byte */
	        in_byte_loc = in_bit_start;
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
	    temp = (byte)(*out & ~(mask << out_byte_loc));
	    *out = (byte)(temp | (data << out_byte_loc));
	    out_byte_loc -= bit_width;
	    if (out_byte_loc < 0) {	/* If finished with byte */
	        out_byte_loc = out_bit_start;
		out++;
	    }
	}
	else {				/* One or more bytes per pixel */
	    *out++ = (byte)(data >> ((out_nbyte - 1) * 8));
	    for (j=1; j<out_nbyte; j++) {
	        *out++ = (byte)(data >> ((out_nbyte - 1 - j) * 8));
	    }
	}
    }
    /* Return the number of bytes in the destination buffer. */
    if (out_byte_loc != out_bit_start) { 	/* If partially filled last byte */
	*out = *out & ((~0) << out_byte_loc);	/* Mask unused part of last byte */
	out++;
    }
    length = out - dest;
    return length;
}

private int write_pcx_file(gx_device_printer * pdev, char * filename, int ncomp,
			    int bpc, int pcmlinelength);
/* 
 * This is an example print page routine for a DeviceN device.  This routine
 * will handle a DeviceN, a CMYK with spot colors, or an RGB process color model.
 *
 * This routine creates several output files.  If the process color model is
 * RGB or CMYK then a bit image file is created which contains the data for the
 * process color model data.  This data is put into the given file stream.
 * I.e. into the output file specified by the user.  This file is not created
 * for the DeviceN process color model.  A separate bit image file is created 
 * is created for the data for each of the given spot colors.  The names for
 * these files are created by taking the given output file name and appending
 * "sn" (where n is the spot color number 0 to ...) to the output file name.
 * The results are unknown if the output file is stdout etc.
 *
 * After the bit image files are created, then a set of PCX format files are
 * created from the bit image files.  This files have a ".pcx" appended to the
 * end of the files.  Thus a CMYK process color model with two spot colors
 * would end up with a total of six files being created.  (xxx, xxxs0, xxxs1,
 * xxx.pcx, xxxs0.pcx, and xxxs1.pcx).
 *
 * I do not assume that any users will actually want to create all of these
 * different files.  However I wanted to show an example of how each of the
 * spot * colorants could be unpacked from the process color model colorants.
 * The bit images files are an easy way to show this without the complication
 * of trying to put the data into a specific format.  However I do not have a
 * tool which will display the bit image data directly so I needed to convert
 * it to a form which I can view.  Thus the PCX format files are being created.
 * Note:  The PCX implementation is not complete.  There are many (most)
 * combinations of bits per pixel and number of colorants that are not supported.
 */
private int
spotcmyk_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    byte *in = gs_alloc_bytes(pdev->memory, line_size, "spotcmyk_print_page(in)");
    byte *buf = gs_alloc_bytes(pdev->memory, line_size + 3, "spotcmyk_print_page(buf)");
    const spotcmyk_device * pdevn = (spotcmyk_device *) pdev;
    int npcmcolors = pdevn->devn_params.num_std_colorant_names;
    int ncomp = pdevn->color_info.num_components;
    int depth = pdevn->color_info.depth;
    int nspot = pdevn->devn_params.separations.num_separations;
    int bpc = pdevn->devn_params.bitspercomponent;
    int lnum = 0, bottom = pdev->height;
    int width = pdev->width;
    FILE * spot_file[GX_DEVICE_COLOR_MAX_COMPONENTS] = {0};
    int i, code = 0;
    int first_bit;
    int pcmlinelength = 0; /* Initialize against indeterminizm in case of pdev->height == 0. */
    int linelength[GX_DEVICE_COLOR_MAX_COMPONENTS];
    byte *data;
    char spotname[gp_file_name_sizeof];

    if (in == NULL || buf == NULL) {
	code = gs_error_VMerror;
	goto prn_done;
    }
    /*
     * Check if the SeparationOrder list has changed the order of the process
     * color model colorants. If so then we will treat all colorants as if they
     * are spot colors.
     */
    for (i = 0; i < npcmcolors; i++)
	if (pdevn->devn_params.separation_order_map[i] != i)
	    break;
    if (i < npcmcolors || ncomp < npcmcolors) {
	nspot = ncomp;
	npcmcolors = 0;
    }

    /* Open the output files for the spot colors */
    for(i = 0; i < nspot; i++) {
	sprintf(spotname, "%ss%d", pdevn->fname, i);
        spot_file[i] = fopen(spotname, "wb");
	if (spot_file[i] == NULL) {
	    code = gs_error_VMerror;
	    goto prn_done;
	}
    }


    /* Now create the output bit image files */
    for (; lnum < bottom; ++lnum) {
	gdev_prn_get_bits(pdev, lnum, in, &data);
        /* Now put the pcm data into the output file */
	if (npcmcolors) {
	    first_bit = bpc * (ncomp - npcmcolors);
	    pcmlinelength = repack_data(data, buf, depth, first_bit, bpc * npcmcolors, width);
	    fwrite(buf, 1, pcmlinelength, prn_stream);
	}
	/* Put spot color data into the output files */
        for (i = 0; i < nspot; i++) {
	    first_bit = bpc * (nspot - 1 - i);
	    linelength[i] = repack_data(data, buf, depth, first_bit, bpc, width);
	    fwrite(buf, 1, linelength[i], spot_file[i]);
        }
    }

    /* Close the bit image files */
    for(i = 0; i < nspot; i++) {
        fclose(spot_file[i]);
	spot_file[i] = NULL;
    }

    /* Now convert the bit image files into PCX files */
    if (npcmcolors) {
	code = write_pcx_file(pdev, (char *) &pdevn->fname,
				npcmcolors, bpc, pcmlinelength);
	if (code < 0)
	    return code;
    }
    for(i = 0; i < nspot; i++) {
	sprintf(spotname, "%ss%d", pdevn->fname, i);
	code = write_pcx_file(pdev, spotname, 1, bpc, linelength[i]);
	if (code < 0)
	    return code;
    }


    /* Clean up and exit */
  prn_done:
    for(i = 0; i < nspot; i++) {
	if (spot_file[i] != NULL)
            fclose(spot_file[i]);
    }
    if (in != NULL)
        gs_free_object(pdev->memory, in, "spotcmyk_print_page(in)");
    if (buf != NULL)
        gs_free_object(pdev->memory, buf, "spotcmyk_print_page(buf)");
    return code;
}

/*
 * We are using the PCX output format.  This is done for simplicity.
 * Much of the following code was copied from gdevpcx.c.
 */

/* ------ Private definitions ------ */

/* All two-byte quantities are stored LSB-first! */
#if arch_is_big_endian
#  define assign_ushort(a,v) a = ((v) >> 8) + ((v) << 8)
#else
#  define assign_ushort(a,v) a = (v)
#endif

typedef struct pcx_header_s {
    byte manuf;			/* always 0x0a */
    byte version;
#define version_2_5			0
#define version_2_8_with_palette	2
#define version_2_8_without_palette	3
#define version_3_0 /* with palette */	5
    byte encoding;		/* 1=RLE */
    byte bpp;			/* bits per pixel per plane */
    ushort x1;			/* X of upper left corner */
    ushort y1;			/* Y of upper left corner */
    ushort x2;			/* x1 + width - 1 */
    ushort y2;			/* y1 + height - 1 */
    ushort hres;		/* horz. resolution (dots per inch) */
    ushort vres;		/* vert. resolution (dots per inch) */
    byte palette[16 * 3];	/* color palette */
    byte reserved;
    byte nplanes;		/* number of color planes */
    ushort bpl;			/* number of bytes per line (uncompressed) */
    ushort palinfo;
#define palinfo_color	1
#define palinfo_gray	2
    byte xtra[58];		/* fill out header to 128 bytes */
} pcx_header;

/* Define the prototype header. */
private const pcx_header pcx_header_prototype =
{
    10,				/* manuf */
    0,				/* version (variable) */
    1,				/* encoding */
    0,				/* bpp (variable) */
    00, 00,			/* x1, y1 */
    00, 00,			/* x2, y2 (variable) */
    00, 00,			/* hres, vres (variable) */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* palette (variable) */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    0,				/* reserved */
    0,				/* nplanes (variable) */
    00,				/* bpl (variable) */
    00,				/* palinfo (variable) */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* xtra */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


/* Forward declarations */
private void pcx_write_rle(const byte *, const byte *, int, FILE *);
private int pcx_write_page(gx_device_printer * pdev, FILE * infile,
    int linesize, FILE * outfile, pcx_header * phdr, bool planar, int depth);

static const byte pcx_cmyk_palette[16 * 3] =
{
    0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x0f, 0x0f, 0x00,
    0xff, 0x00, 0xff, 0x0f, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0x00, 0x00,
    0x00, 0xff, 0xff, 0x00, 0x0f, 0x0f, 0x00, 0xff, 0x00, 0x00, 0x0f, 0x00,
    0x00, 0x00, 0xff, 0x00, 0x00, 0x0f, 0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x0f,
};

static const byte pcx_ega_palette[16 * 3] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0xaa,
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0xaa, 0xaa, 0xaa, 0x00, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0x55, 0xff, 0x55, 0xff, 0x55, 0x55, 0xff, 0xff,
    0xff, 0x55, 0x55, 0xff, 0x55, 0xff, 0xff, 0xff, 0x55, 0xff, 0xff, 0xff
};


/*
 * This routine will set up the revision and palatte for the output
 * file.
 *
 * Please note that this routine does not currently handle all possible
 * combinations of bits and planes.
 *
 * Input parameters:
 *   pdev - Pointer to device data structure
 *   file - output file
 *   header - The header structure to hold the data.
 *   bits_per_plane - The number of bits per plane.
 *   num_planes - The number of planes.
 */
private bool
setup_pcx_header(gx_device_printer * pdev, pcx_header * phdr, int num_planes, int bits_per_plane)
{
    bool planar = true; /* Invalid cases could cause an indeterminizm. */
 
    *phdr = pcx_header_prototype;
    phdr->bpp = bits_per_plane;
    phdr->nplanes = num_planes;

    switch (num_planes) {
        case 1:
	    switch (bits_per_plane) {
	        case 1:
    			phdr->version = version_2_8_with_palette;
    			assign_ushort(phdr->palinfo, palinfo_gray);
    			memcpy((byte *) phdr->palette, "\000\000\000\377\377\377", 6);
			planar = false;
			break;
		case 2:				/* Not defined */
			break;
		case 4:	
    			phdr->version = version_2_8_with_palette;
    			memcpy((byte *) phdr->palette, pcx_ega_palette, sizeof(pcx_ega_palette));
			planar = true;
			break;
		case 5:				/* Not defined */
			break;
		case 8:
    			phdr->version = version_3_0;
    			assign_ushort(phdr->palinfo, palinfo_gray);
			planar = false;
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
	case 2:
	    switch (bits_per_plane) {
	        case 1:				/* Not defined */
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Not defined */
			break;
		case 5:				/* Not defined */
			break;
		case 8:				/* Not defined */
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
	case 3:
	    switch (bits_per_plane) {
	        case 1:				/* Not defined */
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Not defined */
			break;
		case 5:				/* Not defined */
			break;
		case 8:
    			phdr->version = version_3_0;
    			assign_ushort(phdr->palinfo, palinfo_color);
			planar = true;
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
	case 4:
	    switch (bits_per_plane) {
	        case 1:
    			phdr->version = 2;
    			memcpy((byte *) phdr->palette, pcx_cmyk_palette,
	   			sizeof(pcx_cmyk_palette));
			planar = false;
			phdr->bpp = 4;
			phdr->nplanes = 1;
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Not defined */
			break;
		case 5:				/* Not defined */
			break;
		case 8:				/* Not defined */
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
    }
    return planar;
}

/* Write a palette on a file. */
private int
pc_write_mono_palette(gx_device * dev, uint max_index, FILE * file)
{
    uint i, c;
    gx_color_value rgb[3];

    for (i = 0; i < max_index; i++) {
	rgb[0] = rgb[1] = rgb[2] = i << 8;
	for (c = 0; c < 3; c++) {
	    byte b = gx_color_value_to_byte(rgb[c]);

	    fputc(b, file);
	}
    }
    return 0;
}
/*
 * This routine will send any output data required at the end of a file
 * for a particular combination of planes and bits per plane.
 *
 * Please note that most combinations do not require anything at the end
 * of a data file.
 *
 * Input parameters:
 *   pdev - Pointer to device data structure
 *   file - output file
 *   header - The header structure to hold the data.
 *   bits_per_plane - The number of bits per plane.
 *   num_planes - The number of planes.
 */
private int
finish_pcx_file(gx_device_printer * pdev, FILE * file, pcx_header * header, int num_planes, int bits_per_plane)
{
    switch (num_planes) {
        case 1:
	    switch (bits_per_plane) {
	        case 1:				/* Do nothing */
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Do nothing */
			break;
		case 5:				/* Not defined */
			break;
		case 8:
			fputc(0x0c, file);
			return pc_write_mono_palette((gx_device *) pdev, 256, file);
		case 16:			/* Not defined */
			break;
	    }
	    break;
	case 2:
	    switch (bits_per_plane) {
	        case 1:				/* Not defined */
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Not defined */
			break;
		case 5:				/* Not defined */
			break;
		case 8:				/* Not defined */
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
	case 3:
	    switch (bits_per_plane) {
	        case 1:				/* Not defined */
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Not defined */
			break;
		case 5:				/* Not defined */
			break;
		case 8:				/* Do nothing */
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
	case 4:
	    switch (bits_per_plane) {
	        case 1:				/* Do nothing */
			break;
		case 2:				/* Not defined */
			break;
		case 4:				/* Not defined */
			break;
		case 5:				/* Not defined */
			break;
		case 8:				/* Not defined */
			break;
		case 16:			/* Not defined */
			break;
	    }
	    break;
    }
    return 0;
}

/* Send the page to the printer. */
private int
write_pcx_file(gx_device_printer * pdev, char * filename, int ncomp,
			    int bpc, int linesize)
{
    pcx_header header;
    int code;
    bool planar;
    char outname[gp_file_name_sizeof];
    FILE * in;
    FILE * out;
    int depth = bpc_to_depth(ncomp, bpc);

    in = fopen(filename, "rb");
    if (!in)
	return_error(gs_error_invalidfileaccess);
    sprintf(outname, "%s.pcx", filename);
    out = fopen(outname, "wb");
    if (!out) {
	fclose(in);
	return_error(gs_error_invalidfileaccess);
    }

    planar = setup_pcx_header(pdev, &header, ncomp, bpc);
    code = pcx_write_page(pdev, in, linesize, out, &header, planar, depth);
    if (code >= 0)
        code = finish_pcx_file(pdev, out, &header, ncomp, bpc);

    fclose(in);
    fclose(out);
    return code;
}

/* Write out a page in PCX format. */
/* This routine is used for all formats. */
/* The caller has set header->bpp, nplanes, and palette. */
private int
pcx_write_page(gx_device_printer * pdev, FILE * infile, int linesize, FILE * outfile,
	       pcx_header * phdr, bool planar, int depth)
{
    int raster = linesize;
    uint rsize = ROUND_UP((pdev->width * phdr->bpp + 7) >> 3, 2);	/* PCX format requires even */
    int height = pdev->height;
    uint lsize = raster + rsize;
    byte *line = gs_alloc_bytes(pdev->memory, lsize, "pcx file buffer");
    byte *plane = line + raster;
    int y;
    int code = 0;		/* return code */

    if (line == 0)		/* can't allocate line buffer */
	return_error(gs_error_VMerror);

    /* Fill in the other variable entries in the header struct. */

    assign_ushort(phdr->x2, pdev->width - 1);
    assign_ushort(phdr->y2, height - 1);
    assign_ushort(phdr->hres, (int)pdev->x_pixels_per_inch);
    assign_ushort(phdr->vres, (int)pdev->y_pixels_per_inch);
    assign_ushort(phdr->bpl, (planar || depth == 1 ? rsize :
			      raster + (raster & 1)));

    /* Write the header. */

    if (fwrite((const char *)phdr, 1, 128, outfile) < 128) {
	code = gs_error_ioerror;
	goto pcx_done;
    }
    /* Write the contents of the image. */
    for (y = 0; y < height; y++) {
	byte *row = line;
	byte *end;

	code = fread(line, sizeof(byte), linesize, infile);
	if (code < 0)
	    break;
	end = row + raster;
	if (!planar) {		/* Just write the bits. */
	    if (raster & 1) {	/* Round to even, with predictable padding. */
		*end = end[-1];
		++end;
	    }
	    pcx_write_rle(row, end, 1, outfile);
	} else
	    switch (depth) {

		case 4:
		    {
			byte *pend = plane + rsize;
			int shift;

			for (shift = 0; shift < 4; shift++) {
			    register byte *from, *to;
			    register int bright = 1 << shift;
			    register int bleft = bright << 4;

			    for (from = row, to = plane;
				 from < end; from += 4
				) {
				*to++ =
				    (from[0] & bleft ? 0x80 : 0) |
				    (from[0] & bright ? 0x40 : 0) |
				    (from[1] & bleft ? 0x20 : 0) |
				    (from[1] & bright ? 0x10 : 0) |
				    (from[2] & bleft ? 0x08 : 0) |
				    (from[2] & bright ? 0x04 : 0) |
				    (from[3] & bleft ? 0x02 : 0) |
				    (from[3] & bright ? 0x01 : 0);
			    }
			    /* We might be one byte short of rsize. */
			    if (to < pend)
				*to = to[-1];
			    pcx_write_rle(plane, pend, 1, outfile);
			}
		    }
		    break;

		case 24:
		    {
			int pnum;

			for (pnum = 0; pnum < 3; ++pnum) {
			    pcx_write_rle(row + pnum, row + raster, 3, outfile);
			    if (pdev->width & 1)
				fputc(0, outfile);		/* pad to even */
			}
		    }
		    break;

		default:
		    code = gs_note_error(gs_error_rangecheck);
		    goto pcx_done;

	    }
	code = 0;
    }

  pcx_done:
    gs_free_object(pdev->memory, line, "pcx file buffer");

    return code;
}

/* ------ Internal routines ------ */

/* Write one line in PCX run-length-encoded format. */
private void
pcx_write_rle(const byte * from, const byte * end, int step, FILE * file)
{  /*
    * The PCX format theoretically allows encoding runs of 63
    * identical bytes, but some readers can't handle repetition
    * counts greater than 15.
    */
#define MAX_RUN_COUNT 15
    int max_run = step * MAX_RUN_COUNT;

    while (from < end) {
	byte data = *from;

	from += step;
	if (data != *from || from == end) {
	    if (data >= 0xc0)
		putc(0xc1, file);
	} else {
	    const byte *start = from;

	    while ((from < end) && (*from == data))
		from += step;
	    /* Now (from - start) / step + 1 is the run length. */
	    while (from - start >= max_run) {
		putc(0xc0 + MAX_RUN_COUNT, file);
		putc(data, file);
		start += max_run;
	    }
	    if (from > start || data >= 0xc0)
		putc((from - start) / step + 0xc1, file);
	}
	putc(data, file);
    }
#undef MAX_RUN_COUNT
}
