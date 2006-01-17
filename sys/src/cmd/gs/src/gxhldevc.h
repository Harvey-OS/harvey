/* Copyright (C) 2003 Artifex Software Inc, artofcode llc.  All rights reserved.
  
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
/* $Id: gxhldevc.h,v 1.4 2003/09/08 11:45:08 igor Exp $ */
/* High level device color save/compare procedures */

#ifndef gxhldevc_INCLUDED
#  define gxhldevc_INCLUDED

#include "gsdcolor.h"

/*
 * Most high level devices want more information about the color spaces
 * which were used to create color values.
 *
 * There are some added complications:
 *
 * 1. Ghostscript has many dozens, if not hundreds, of device drivers which
 * have been written for it.  Many of these devices are outside of our
 * control.  (However we do receive questions and complaints when they no
 * longer work.)  Thus we also want to avoid making changes in the device
 * interface which would require changes to the code in these devices.
 *
 * 2. We also desire to not save pointers to color space structures, etc.
 * within the high level device.  Many of these structures are temporary,
 * stack based, or are deleted outside of the control of the device.  Thus
 * it becomes almost impossible to prevent pointers to deleted objects.
 *
 * 3. Both color spaces and device colors are passed to devices via pointers
 * to objects.  These objects, in turn, often contain pointers to other
 * objects.
 *
 * These constraints imply the need within the device to save color spaces
 * and colors in some form which will allows us to detect color space or
 * color changes when a new color space and a color is compared to the old
 * saved color space and color.  These 'saved' forms should not include
 * pointers to objects outside of the control of the device.
 *
 * The functions below are desiged to assist the high level device in the
 * saving, comparing, and getting high level color information.
 */
 

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

/*
 * A structure for saving high level color information for high level devices.
 */
typedef struct gx_hl_saved_color_s {
    gs_id color_space_id;
    gs_id pattern_id;
    bool ccolor_valid;
    gs_client_color ccolor;
    gx_device_color_saved saved_dev_color;
} gx_hl_saved_color;

/*
 * Initiailze a high level saved color to null
 */
void gx_hld_saved_color_init(gx_hl_saved_color * psc);

/*
 * Get graphics state pointer (from imager state pointer)
 * Return NULL if the imager state is not also a graphics state.
 */
const gs_state * gx_hld_get_gstate_ptr(const gs_imager_state * pis);

/*
 * Save the device color information including the color space id and
 * client color data (if available).  The pattern id is also saved for
 * detection of changes in the pattern.
 *
 * This routine returns 'true' if sufficient information was provided
 * to completely describe a full high level (non process color model)
 * color.  Otherwise 'false' is returned.  Thus the return does both
 * a save and test on the given color.
 *
 * If the device can't handle high level colors, it must pass NULL to 
 * the 'pis' argument.
 */
bool gx_hld_save_color(const gs_imager_state * pis,
	const gx_device_color * pdevc, gx_hl_saved_color * psc);

/*
 * Compare two saved colors to check if match.  Note this routine assumes
 * unused parts of the saved color have been zeroed.  See gx_hld_save_color()
 * for what is actually being compared.
 */
bool gx_hld_saved_color_equal(const gx_hl_saved_color * psc1,
			   const gx_hl_saved_color * psc2);

/*
 * Check whether two saved colors have same color space.
 */
bool gx_hld_saved_color_same_cspace(const gx_hl_saved_color * psc1,
			   const gx_hl_saved_color * psc2);

/*
 * Check if a high level color is availavble.
 */
bool
gx_hld_is_hl_color_available(const gs_imager_state * pis,
		const gx_device_color * pdevc);

/*
 * Return status from get_color_space_and_ccolor.  See that routine for
 * more information.
 *
 * Hopefully I will be given more information to allow the choice of
 * better names.
 */
typedef enum {
        non_pattern_color_space,
        pattern_color_sapce,
	use_process_color
} gx_hld_get_color_space_and_ccolor_status;

/*
 * Get pointers to the current color space and client color.
 *
 * There are four possible cases:
 * 1.  Either the device color or imager state pointer is NULL.  If so then
 *     we do not have enough information.  Thus we need to fall back to the
 *     process color model color.  Return NULL for both pointers.
 * 2.  The device color was not created from a color space and a client color.
 *     (See the set_non_client_color() macro.)  In this case NULL is returned
 *     for both pointers.  (Use process color model color.)
 * 3.  The device color is a 'pattern'.  Return pointers to both the current
 *     color space and the ccolor (client color) field in the device color
 *     structure.  Note:  For the shfill opeartor, a pattern color space will
 *     be used to build the device color.  However the current color space
 *     will not be the pattern color space.
 * 4.  All other cases: the current color space is the color space used to
 *     build the device color.  A pointer to the current color space is
 *     returned.  The client color pointer will be NULL.
 *
 * The status returned indicates if the color space information is valid
 * (non valid --> use_process_color) and whether the color space is
 * a pattern or non pattern).
 */
gx_hld_get_color_space_and_ccolor_status gx_hld_get_color_space_and_ccolor(
		const gs_imager_state * pis, const gx_device_color * pdevc,
		const gs_color_space ** ppcs, const gs_client_color ** ppcc);

/*
 * This routine will return the number of components in the current color
 * space.
 *
 * The routine will return -1 if the imager state does not point to a
 * graphics state (and thus we cannot get the current color space).
 */
int gx_hld_get_number_color_components(const gs_imager_state * pis);

/*
 * This defines sthe possible status to be returned from get_color_component.
 */
typedef enum {
    valid_result = 1,
    invalid_color_info = 2,
    invalid_component_requested = 3
} gx_hld_get_color_component_status;

/*
 * Get the requested high level color value.
 *
 * This routine will get the specified high level color if it is available.
 * If the value is not available (status equal either invalid_color_info or
 * invalid_component_requested).  In this case, it is suggested that the
 * device fall back to using the process color model.
 */
gx_hld_get_color_component_status gx_hld_get_color_component(
		const gs_imager_state * pis, const gx_device_color * pdevc,
		int comp_numi, float * output);

#endif

