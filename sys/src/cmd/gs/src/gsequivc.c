/* Copyright (C) 2004 Artifex Software Inc., artofcode LLC.  All rights reserved.
  
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

/*$Id: gsequivc.c,v 1.6 2005/07/08 22:04:31 dan Exp $ */
/* Routines for determining equivalent color for spot colors */

#include "math_.h"
#include "gdevprn.h"
#include "gsparam.h"
#include "gstypes.h"
#include "gxdcconv.h"
#include "gdevdevn.h"
#include "gsequivc.h"
#include "gzstate.h"
#include "gsstate.h"
#include "gscspace.h"
#include "gxcspace.h"

/*
 * These routines are part of the logic for determining an equivalent
 * process color model color for a spot color.  The definition of a
 * Separation or DeviceN color space include a tint transform function.
 * This tint transform function is used if the specified spot or separation
 * colorant is not present.  We want to be able to display the spot colors
 * on our RGB or CMYK display.  We are using the tint transform function
 * to determine a CMYK equivalent to the spot color.  Current only CMYK
 * equivalent colors are supported.  This is because the CMYK is the only
 * standard subtractive color space.
 *
 * This process consists of the following steps:
 *
 * 1.  Whenever new spot colors are found, set status flags indicating
 * that we have one or more spot colors for which we need to determine an
 * equivalent color.  New spot colors can either be explicitly specified by
 * the SeparationColorNames device parameter or they may be detected by the
 * device's get_color_comp_index routine.
 *
 * 2.  Whenever a Separation or DeviceN color space is installed, the
 * update_spot_equivalent_colors device proc is called.  This allows the
 * device to check if the color space contains a spot color for which the
 * device does not know the equivalent CMYK color.  The routine
 * update_spot_equivalent_cmyk_colors is provided for this task (and the
 * next item).
 *
 * 3.  For each spot color for which an equivalent color is not known, we
 * do the following:
 *   a.  Create a copy of the color space and change the copy so that it
 *       uses its alternate colorspace.
 *   b.  Create a copy of the current imager state and modify its color
 *       mapping (cmap) procs to use a special set of 'capture' procs.
 *   c.  Based upon the type of color space (Separation or DeviceN) create
 *       a 'color' which consists of the desired spot color set to 100% and
 *       and other components set to zero.
 *   d.  Call the remap_color routine using our modified color space and
 *       state structures.  Since we have forced the use of the alternate
 *       color space, we will eventually execute one of the 'capture' color
 *       space mapping procs.  This will give us either a gray, RGB, or
 *       CMYK color which is equivalent to the original spot color.  If the
 *       color is gray or RGB we convert it to CMYK.
 *   e.  Save the equivalent CMYK color in the device structure.
 *
 * 4.  When a page is to be displayed or a file created, the saved equivalent
 * color is used as desired.  It can be written into the output file.  It
 * may also be used to provide color values which can be combined with the
 * process color model components for a pixel, to correctly display spot
 * colors on a monitor.  (Note:  Overprinting effects with spot colors are
 * not correctly displayed on an RGB monitor if the device simply uses an RGB
 * process color model.  Instead it is necessary to use a subtractive process
 * color model and save both process color and spot color data and then
 * convert the overall result to RGB for display.)
 *
 * For this process to work properly, the following changes need to made to
 * the device.
 *
 * 1.  The device source module needs to include gsequivc.c for a definition
 *     of the relevant structures and routines.  An equivalent_cmyk_color_params
 *     structure needs to be added to the device's structure definition and
 *     it needs to be initialized.  For examples see the definition of the
 *     psd_device structure in src/gdevpsd.c and the definitions of the
 *     gs_psdrgb_device and gs_psdcmyk_devices devices in the same module.
 * 2.  Logic needs to be added to the device's get_color_comp_index and
 *     put_param routines to check if any separations have been added to the
 *     device.  For examples see code fragments in psd_get_color_comp_index and
 *     psd_put_params in src/gdevpsd.c.
 * 3.  The device needs to have its own version of the
 *     update_spot_equivalent_colors routine.  For examples see the definition
 *     of the device_procs macro and the psd_update_spot_equivalent_colors
 *     routine in src/gdevpsd.c.
 * 4.  The device then uses the saved equivalent color values when its output
 *     is created.  For example see the psd_write_header routine in
 *     src/gdevpsd.c.
 */

/* Function protypes */
private void capture_spot_equivalent_cmyk_colors(gx_device * pdev,
		    const gs_state * pgs, const gs_client_color * pcc,
		    const gs_color_space * pcs, int sep_num,
		    equivalent_cmyk_color_params * pparams);

#define compare_color_names(name, name_size, str, str_size) \
    (name_size == str_size && \
	(strncmp((const char *)name, (const char *)str, name_size) == 0))

private void
update_Separation_spot_equivalent_cmyk_colors(gx_device * pdev,
		    const gs_state * pgs, const gs_color_space * pcs,
		    gs_devn_params * pdevn_params,
		    equivalent_cmyk_color_params * pparams)
{
    int i;

    /* 
     * Check if the color space's separation name matches any of the
     * separations for which we need an equivalent CMYK color.
     */
    for (i = 0; i < pdevn_params->separations.num_separations; i++) {
	if (pparams->color[i].color_info_valid == false) {
	    const devn_separation_name * dev_sep_name =
			    &(pdevn_params->separations.names[i]);
	    unsigned int cs_sep_name_size;
	    unsigned char * pcs_sep_name;

	    pcs->params.separation.get_colorname_string
		(pdev->memory, pcs->params.separation.sep_name, &pcs_sep_name,
		 &cs_sep_name_size);
	    if (compare_color_names(dev_sep_name->data, dev_sep_name->size,
			    pcs_sep_name, cs_sep_name_size)) {
		gs_color_space temp_cs = *pcs;
		gs_client_color client_color;

		/*
	         * Create a copy of the color space and then modify it
		 * to force the use of the alternate color space.
	         */
		temp_cs.params.separation.use_alt_cspace = true;
		client_color.paint.values[0] = 1.0;
		capture_spot_equivalent_cmyk_colors(pdev, pgs, &client_color,
					    &temp_cs, i, pparams);
		break;
	    }
	}
    }
}

private void
update_DeviceN_spot_equivalent_cmyk_colors(gx_device * pdev,
		    const gs_state * pgs, const gs_color_space * pcs,
		    gs_devn_params * pdevn_params,
		    equivalent_cmyk_color_params * pparams)
{
    int i;
    unsigned int j;
    unsigned int cs_sep_name_size;
    unsigned char * pcs_sep_name;

    /*
     * Check if the color space contains components named 'None'.  If so then
     * our capture logic does not work properly.  When present, the 'None'
     * components contain alternate color information.  However this info is
     * specified as part of the 'color' and not part of the color space.  Thus
     * we do not have this data when this routine is called.  See the 
     * description of DeviceN color spaces in section 4.5 of the PDF spec.
     * In this situation we exit rather than produce invalid values.
     */
     for (j = 0; j < pcs->params.device_n.num_components; j++) {
	pcs->params.device_n.get_colorname_string
	    (pdev->memory, pcs->params.device_n.names[j],  
	     &pcs_sep_name, &cs_sep_name_size);
	if (compare_color_names("None", 4, pcs_sep_name, cs_sep_name_size))
	    return;
    }

    /* 
     * Check if the color space's separation names matches any of the
     * separations for which we need an equivalent CMYK color.
     */
    for (i = 0; i < pdevn_params->separations.num_separations; i++) {
	if (pparams->color[i].color_info_valid == false) {
	    const devn_separation_name * dev_sep_name =
			    &(pdevn_params->separations.names[i]);

	    for (j = 0; j < pcs->params.device_n.num_components; j++) {
	        pcs->params.device_n.get_colorname_string
		    (pdev->memory, pcs->params.device_n.names[j], &pcs_sep_name,
		     &cs_sep_name_size);
	        if (compare_color_names(dev_sep_name->data, dev_sep_name->size,
			    pcs_sep_name, cs_sep_name_size)) {
		    gs_color_space temp_cs = *pcs;
		    gs_client_color client_color;

		    /*
	             * Create a copy of the color space and then modify it
		     * to force the use of the alternate color space.
	             */
		    memset(&client_color, 0, sizeof(client_color));
		    temp_cs.params.device_n.use_alt_cspace = true;
		    client_color.paint.values[j] = 1.0;
		    capture_spot_equivalent_cmyk_colors(pdev, pgs, &client_color,
					    &temp_cs, i, pparams);
		    break;
	        }
	    }
	}
    }
}

private bool check_all_colors_known(int num_spot,
		equivalent_cmyk_color_params * pparams)
{
    for (num_spot--; num_spot >= 0; num_spot--)
	if (pparams->color[num_spot].color_info_valid == false)
	    return false;
    return true;
}

/* If possible, update the equivalent CMYK color for a spot color */
void
update_spot_equivalent_cmyk_colors(gx_device * pdev, const gs_state * pgs,
    gs_devn_params * pdevn_params, equivalent_cmyk_color_params * pparams)
{
    const gs_color_space * pcs;

    /* If all of the color_info is valid then there is nothing to do. */
    if (pparams->all_color_info_valid)
	return;

    /* Verify that we really have some separations. */
    if (pdevn_params->separations.num_separations == 0) {
	pparams->all_color_info_valid = true;
	return;
    }
    /*
     * Verify that the given color space is a Separation or a DeviceN color
     * space.  If so then when check if the color space contains a separation
     * color for which we need a CMYK equivalent.
     */
    pcs = pgs->color_space;
    if (pcs != NULL) {
	if (pcs->type->index == gs_color_space_index_Separation) {
	    update_Separation_spot_equivalent_cmyk_colors(pdev, pgs, pcs,
						pdevn_params, pparams);
	    pparams->all_color_info_valid = check_all_colors_known
		    (pdevn_params->separations.num_separations, pparams);
	}
	else if (pcs->type->index == gs_color_space_index_DeviceN) {
	    update_DeviceN_spot_equivalent_cmyk_colors(pdev, pgs, pcs,
						pdevn_params, pparams);
	    pparams->all_color_info_valid = check_all_colors_known
		    (pdevn_params->separations.num_separations, pparams);
	}
    }
}

private void
save_spot_equivalent_cmyk_color(int sep_num,
		equivalent_cmyk_color_params * pparams, frac cmyk[4])
{
    pparams->color[sep_num].c = cmyk[0];
    pparams->color[sep_num].m = cmyk[1];
    pparams->color[sep_num].y = cmyk[2];
    pparams->color[sep_num].k = cmyk[3];
    pparams->color[sep_num].color_info_valid = true;
}

/*
 * A structure definition for a device for capturing equivalent colors
 */
typedef struct color_capture_device_s {
    gx_device_common;
    gx_prn_device_common;
    /*        ... device-specific parameters ... */
    /* The following values are needed by the cmap procs for saving data */
    int sep_num;	/* Number of the separation being captured */
    /* Pointer to original device's equivalent CMYK colors */
    equivalent_cmyk_color_params * pequiv_cmyk_colors;
} color_capture_device;

/*
 * Replacement routines for the cmap procs.  These routines will capture the
 * equivalent color.
 */
private cmap_proc_gray(cmap_gray_capture_cmyk_color);
private cmap_proc_rgb(cmap_rgb_capture_cmyk_color);
private cmap_proc_cmyk(cmap_cmyk_capture_cmyk_color);
private cmap_proc_rgb_alpha(cmap_rgb_alpha_capture_cmyk_color);
private cmap_proc_separation(cmap_separation_capture_cmyk_color);
private cmap_proc_devicen(cmap_devicen_capture_cmyk_color);

private const gx_color_map_procs cmap_capture_cmyk_color = {
    cmap_gray_capture_cmyk_color, 
    cmap_rgb_capture_cmyk_color, 
    cmap_cmyk_capture_cmyk_color,
    cmap_rgb_alpha_capture_cmyk_color,
    cmap_separation_capture_cmyk_color,
    cmap_devicen_capture_cmyk_color
};

private void
cmap_gray_capture_cmyk_color(frac gray, gx_device_color * pdc,
	const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    equivalent_cmyk_color_params * pparams =
	    ((color_capture_device *)dev)->pequiv_cmyk_colors;
    int sep_num = ((color_capture_device *)dev)->sep_num;
    frac cmyk[4];

    cmyk[0] = cmyk[1] = cmyk[2] = frac_0;
    cmyk[3] = frac_1 - gray;
    save_spot_equivalent_cmyk_color(sep_num, pparams, cmyk);
}

private void
cmap_rgb_capture_cmyk_color(frac r, frac g, frac b, gx_device_color * pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    equivalent_cmyk_color_params * pparams =
	    ((color_capture_device *)dev)->pequiv_cmyk_colors;
    int sep_num = ((color_capture_device *)dev)->sep_num;
    frac cmyk[4];

    color_rgb_to_cmyk(r, g, b, pis, cmyk);
    save_spot_equivalent_cmyk_color(sep_num, pparams, cmyk);
}

private void
cmap_cmyk_capture_cmyk_color(frac c, frac m, frac y, frac k, gx_device_color * pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    equivalent_cmyk_color_params * pparams =
	    ((color_capture_device *)dev)->pequiv_cmyk_colors;
    int sep_num = ((color_capture_device *)dev)->sep_num;
    frac cmyk[4];

    cmyk[0] = c;
    cmyk[1] = m;
    cmyk[2] = y;
    cmyk[3] = k;
    save_spot_equivalent_cmyk_color(sep_num, pparams, cmyk);
}

private void
cmap_rgb_alpha_capture_cmyk_color(frac r, frac g, frac b, frac alpha,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
			 gs_color_select_t select)
{
    cmap_rgb_capture_cmyk_color(r, g, b, pdc, pis, dev, select);
}

private void
cmap_separation_capture_cmyk_color(frac all, gx_device_color * pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    dprintf("cmap_separation_capture_cmyk_color - this routine should not be executed\n");
}

private void
cmap_devicen_capture_cmyk_color(const frac * pcc, gx_device_color * pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    dprintf("cmap_devicen_capture_cmyk_color - this routine should not be executed\n");
}

/*
 * Note:  The color space (pcs) has already been modified to use the
 * alternate color space.
 */
private void
capture_spot_equivalent_cmyk_colors(gx_device * pdev, const gs_state * pgs,
    const gs_client_color * pcc, const gs_color_space * pcs,
    int sep_num, equivalent_cmyk_color_params * pparams)
{
    gs_imager_state temp_state = *((const gs_imager_state *)pgs);
    color_capture_device temp_device = { 0 };
    gx_device_color dev_color;

    /*
     * Create a temp device.  The primary purpose of this device is pass the
     * separation number and a pointer to the original device's equivalent
     * color parameters.  Since we only using this device for a very specific
     * purpose, we only set up the color_info structure and and our data.
     */
    temp_device.color_info = pdev->color_info;
    temp_device.sep_num = sep_num;
    temp_device.pequiv_cmyk_colors = pparams;
    /*
     * Create a temp copy of the imager state.  We do this so that we
     * can modify the color space mapping (cmap) procs.  We use our
     * replacment procs to capture the color.  The installation of a
     * Separation or DeviceN color space also sets a use_alt_cspace flag
     * in the state.  We also need to set this to use the alternate space.
     */
    temp_state.cmap_procs = &cmap_capture_cmyk_color;
    temp_state.color_component_map.use_alt_cspace = true;

    /* Now capture the color */
    pcs->type->remap_color (pcc, pcs, &dev_color, &temp_state,
		    (gx_device *)&temp_device, gs_color_select_texture);
}
