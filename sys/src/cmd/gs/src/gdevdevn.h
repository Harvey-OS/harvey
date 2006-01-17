/* Copyright (C) 2003 Artifex Software Inc.  All rights reserved.
  
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

/*$Id: gdevdevn.h,v 1.12 2005/10/01 04:40:18 dan Exp $ */
/* Include file for common DeviceN process color model devices. */

#ifndef gdevdevn_INCLUDED
# define gdevdevn_INCLUDED

/*
 * Define the maximum number of spot colors supported by this device.
 * This value is arbitrary.  It is set simply to define a limit on
 * on the separation_name_array and separation_order map.
 */
#define GX_DEVICE_MAX_SEPARATIONS 16
/*
 * Define the maximum number of process model colorants.  Currently we only
 * have code for DeviceGray, DeviceRGB, and DeviceCMYK.  Thus this value
 * only needs to be 4.  However we are allowing for a future hexachrome
 * device.  (This value does not include spot colors.  See previous value.)
 */
#define MAX_DEVICE_PROCESS_COLORS 6

/*
 * Type definitions associated with the fixed color model names.
 */
typedef const char * fixed_colorant_name;
typedef fixed_colorant_name * fixed_colorant_names_list;

/*
 * Structure for holding SeparationNames elements.
 */
typedef struct devn_separation_name_s {
    int size;
    byte * data;
} devn_separation_name;

/*
 * Structure for holding SeparationNames elements.
 */
typedef struct gs_separations_s {
    int num_separations;
    devn_separation_name names[GX_DEVICE_MAX_SEPARATIONS];
} gs_separations;

/*
 * Type for holding a separation order map
 */
typedef int gs_separation_map[GX_DEVICE_MAX_SEPARATIONS];

typedef struct gs_devn_params_s {
    /*
     * Bits per component (device colorant).  Currently only 1 and 8 are
     * supported.
     */
    int bitspercomponent;

    /*
     * Pointer to the colorant names for the color model.  This will be
     * null if we have DeviceN type device.  The actual possible colorant
     * names are those in this list plus those in the separation[i].name
     * list (below).
     */
    fixed_colorant_names_list std_colorant_names;
    int num_std_colorant_names;	/* Number of names in list */
    int max_separations;	/* From MaxSeparation parameter */

    /*
    * Separation info (if any).
    */
    gs_separations separations;

    /*
     * Separation Order (if specified).
     */
    int num_separation_order_names;
    /*
     * The SeparationOrder parameter may change the logical order of
     * components.
     */
    gs_separation_map separation_order_map;
} gs_devn_params_t;

typedef gs_devn_params_t gs_devn_params;

extern fixed_colorant_name DeviceCMYKComponents[];

#include "gsequivc.h"

/*
 * Utility routines for common DeviceN related parameters:
 *   SeparationColorNames, SeparationOrder, and MaxSeparations
 */

/*
 * Convert standard color spaces into DeviceN colorants.
 * Note;  This routine require SeparationOrder map.
 */
void gray_cs_to_devn_cm(gx_device * dev, int * map, frac gray, frac out[]);

void rgb_cs_to_devn_cm(gx_device * dev, int * map,
		const gs_imager_state *pis, frac r, frac g, frac b, frac out[]);

void cmyk_cs_to_devn_cm(gx_device * dev, int * map,
		frac c, frac m, frac y, frac k, frac out[]);

/*
 * Possible values for the 'auto_spot_colors' parameter.
 */
/*
 * Do not automatically include spot colors
 */
#define NO_AUTO_SPOT_COLORS 0
/*
 * Automatically add spot colors up to the number that the device can image.
 * Spot colors over that limit will be handled by the alternate color space
 * for the Separation or DeviceN color space.
 */
#define ENABLE_AUTO_SPOT_COLORS	1
/*
 * Automatically add spot colors up to the GX_DEVICE_MAX_SEPARATIONS value.
 * Note;  Spot colors beyond the number that the device can image will be
 * ignored (i.e. treated like a colorant that is not specified by the
 * SeparationOrder device parameter.
 */
#define ALLOW_EXTRA_SPOT_COLORS 2

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
 *   auto_spot_colors - See comments above.
 *
 * This routine returns a positive value (0 to n) which is the device colorant
 * number if the name is found.  It returns GX_DEVICE_COLOR_MAX_COMPONENTS if
 * the color component is found but is not being used due to the
 * SeparationOrder parameter.  It returns a negative value if not found.
 *
 * This routine will also add separations to the device if space is
 * available.
 */
int devn_get_color_comp_index(const gx_device * dev,
    gs_devn_params * pdevn_params, equivalent_cmyk_color_params * pequiv_colors,
    const char * pname, int name_size, int component_type,
    int auto_spot_colors);

/* Utility routine for getting DeviceN parameters */
int devn_get_params(gx_device * pdev, gs_param_list * plist,
		    gs_devn_params * pdevn_params,
		    equivalent_cmyk_color_params * pequiv_colors);

/*
 * Utility routine for handling DeviceN related parameters.  This routine
 * assumes that the device is based upon a standard printer type device.
 * (See the next routine if not.)
 *
 * Note that this routine requires a pointer to the DeviceN parameters within
 * the device structure.  The pointer to the equivalent_cmyk_color_params is
 * optional (it should be NULL if this feature is not used by the device).
 */
int devn_printer_put_params(gx_device * pdev, gs_param_list * plist,
			gs_devn_params * pdevn_params,
			equivalent_cmyk_color_params * pequiv_colors);

/* 
 * Utility routine for handling DeviceN related parameters.  This routine
 * may modify the color_info, devn_params, and the * equiv_colors fields.
 * The pointer to the equivalent_cmyk_color_params is optional (it should be
 * NULL if this feature is not used by the device).
 *
 * Note:  This routine does not restore values in case of a problem.  This
 * is left to the caller.
 */
int devn_put_params(gx_device * pdev, gs_param_list * plist,
			gs_devn_params * pdevn_params,
			equivalent_cmyk_color_params * pequiv_colors);

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
int check_pcm_and_separation_names(const gx_device * dev,
		const gs_devn_params * pparams, const char * pname,
		int name_size, int component_type);

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
int repack_data(byte * source, byte * dest, int depth, int first_bit,
		int bit_width, int npixel);

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
int bpc_to_depth(int ncomp, int bpc);

#endif		/* ifndef gdevdevn_INCLUDED */
