/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevcmap.c,v 1.6 2004/05/26 04:10:58 dan Exp $ */
/* Special color mapping device */
#include "gx.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gxlum.h"
#include "gxfrac.h"
#include "gxdcconv.h"
#include "gdevcmap.h"

/*
 * The devices in this file exist only to implement the PCL5 special
 * color mapping algorithms.  They are not useful for PostScript.
 */

/* GC descriptor */
public_st_device_cmap();

/* Device procedures */
private dev_proc_get_params(cmap_get_params);
private dev_proc_put_params(cmap_put_params);
private dev_proc_begin_typed_image(cmap_begin_typed_image);
private dev_proc_get_color_mapping_procs(cmap_get_color_mapping_procs);

/*
 * NB: all of the device color model information will be replaced by
 * the target's color model information. Only the
 * get_color_mapping_procs method is modified (aside from
 * get_params/put_params).
 *
 * The begin_typed_image method is used only to force use of the default
 * image rendering routines if a special mapping_method (anything other
 * than device_cmap_identity) is requested.
 */

private const gx_device_cmap gs_cmap_device = {
    std_device_dci_body(gx_device_cmap, 0, "special color mapper",
                        0, 0, 1, 1,
                        3, 24, 255, 255, 256, 256),
    {
        0, 0, 0, 0, 0, 0, 0,
        gx_forward_fill_rectangle,
        gx_forward_tile_rectangle,
        gx_forward_copy_mono,
        gx_forward_copy_color,
        0, 0,
        cmap_get_params,
        cmap_put_params,
        0, 0, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        gx_default_begin_image,
        0, 0, 0, 0, 0,
        cmap_begin_typed_image,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        cmap_get_color_mapping_procs,
        0, 0, 0
    },
    0,                          /* target */
    device_cmap_identity
};

/* Set the color mapping method. */
private int
gdev_cmap_set_method(gx_device_cmap * cmdev,
		     gx_device_color_mapping_method_t method)
{
    gx_device *target = cmdev->target;

    /*
     * If we're transforming the color, we may need to fool the graphics
     * core into not halftoning.
     */
    set_dev_proc(cmdev, map_cmyk_color, gx_default_map_cmyk_color);
    set_dev_proc(cmdev, map_color_rgb, gx_forward_map_color_rgb);

    switch (method) {

	case device_cmap_identity:
	    /*
	     * In this case, and only this case, we can allow the target's
	     * color model to propagate here.
	     */
	    set_dev_proc(cmdev, map_cmyk_color, gx_forward_map_cmyk_color);
	    cmdev->color_info.max_gray = target->color_info.max_gray;
	    cmdev->color_info.max_color = target->color_info.max_color;
	    cmdev->color_info.max_components =
	        target->color_info.max_components;
	    cmdev->color_info.num_components =
		target->color_info.num_components;
	    cmdev->color_info.polarity = target->color_info.polarity;
	    cmdev->color_info.gray_index = target->color_info.gray_index;
	    cmdev->color_info.cm_name = target->color_info.cm_name;
	    gx_device_copy_color_procs((gx_device *)cmdev, target);
	    break;

	case device_cmap_monochrome:
	    cmdev->color_info.max_gray = target->color_info.max_gray;
	    cmdev->color_info.max_color = target->color_info.max_color;
	    cmdev->color_info.max_components = 
	        cmdev->color_info.num_components = 1;
	    cmdev->color_info.cm_name = "DeviceGray";
	    break;

	case device_cmap_snap_to_primaries:
	case device_cmap_color_to_black_over_white:
	    cmdev->color_info.max_gray = cmdev->color_info.max_color = 4095;
	    /*
	     * We have to be an RGB device, otherwise "primaries" doesn't
	     * have the proper meaning.
	     */
	    cmdev->color_info.max_components = 
	        cmdev->color_info.num_components = 3;
	    cmdev->color_info.cm_name = "DeviceRGB";
	    break;

	default:
	    return_error(gs_error_rangecheck);
    }
    cmdev->mapping_method = method;
    return 0;
}

/* Initialize the device. */
int
gdev_cmap_init(gx_device_cmap * dev, gx_device * target,
	       gx_device_color_mapping_method_t method)
{
    int code;

    gx_device_init((gx_device *) dev, (const gx_device *)&gs_cmap_device,
		   target->memory, true);
    gx_device_set_target((gx_device_forward *)dev, target);
    gx_device_copy_params((gx_device *)dev, target);
    check_device_separable((gx_device *)dev);
    gx_device_forward_fill_in_procs((gx_device_forward *) dev);
    code = gdev_cmap_set_method(dev, method);
    if (code < 0)
	return code;
    return 0;
}

/* Get parameters. */
private int
cmap_get_params(gx_device * dev, gs_param_list * plist)
{
    int code = gx_forward_get_params(dev, plist);
    int ecode = code;
    gx_device_cmap * const cmdev = (gx_device_cmap *)dev;
    int cmm = cmdev->mapping_method;

    if ((code = param_write_int(plist, "ColorMappingMethod", &cmm)) < 0)
	ecode = code;
    return ecode;
}

/* Update parameters; copy the device information back afterwards. */
private int
cmap_put_params(gx_device * dev, gs_param_list * plist)
{
    int code = gx_forward_put_params(dev, plist);
    int ecode = code;
    gx_device_cmap * const cmdev = (gx_device_cmap *)dev;
    int cmm = cmdev->mapping_method;
    const char *param_name;

    switch (code = param_read_int(plist, param_name = "ColorMappingMethod",
				  &cmm)) {
    case 0:
	if (cmm < 0 || cmm > device_cmap_max_method) {
	    code = gs_note_error(gs_error_rangecheck);
	} else
	    break;
    default:
	ecode = code;
	param_signal_error(plist, param_name, ecode);
	break;
    case 1:
	break;
    }
    if (code >= 0) {
	gx_device_copy_params(dev, cmdev->target);
	gdev_cmap_set_method(cmdev, cmm);
    }
    return ecode;
}

/*
 * Handle high-level images.  The only reason we do this, rather than simply
 * pass the operation to the target, is that the image still has to call the
 * cmap device to do its color mapping.  As presently implemented, this
 * disables any high-level implementation that the target may provide.
 */
private int
cmap_begin_typed_image(gx_device * dev,
		       const gs_imager_state * pis, const gs_matrix * pmat,
		   const gs_image_common_t * pic, const gs_int_rect * prect,
		       const gx_drawing_color * pdcolor,
		       const gx_clip_path * pcpath,
		       gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    const gx_device_cmap *const cmdev = (const gx_device_cmap *)dev;
    gx_device *target = cmdev->target;

    if (cmdev->mapping_method == device_cmap_identity)
	return (*dev_proc(target, begin_typed_image))
	    (target, pis, pmat, pic, prect, pdcolor, pcpath, memory, pinfo);
    return gx_default_begin_typed_image(dev, pis, pmat, pic, prect,
					pdcolor, pcpath, memory, pinfo);
}

private void
cmap_gray_cs_to_cm(gx_device * dev, frac gray, frac out[])
{
    gx_device_cmap *    cmdev = (gx_device_cmap *)dev;
    frac gx_max_color_frac   = cv2frac(gx_max_color_value);
    switch (cmdev->mapping_method) {
    case device_cmap_snap_to_primaries:
        gray = (gray <= gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        break;

    case device_cmap_color_to_black_over_white:
        gray = (gray == 0 ? gx_max_color_frac : 0);
        break;
    }
    {
        gx_device *target = cmdev->target ? cmdev->target : dev;
        gx_cm_color_map_procs *cm_procs =  (dev_proc( target, get_color_mapping_procs)(target));
        cm_procs->map_gray(target, gray, out );
    }
}

private void
cmap_rgb_cs_to_cm(gx_device * dev, const gs_imager_state * pis, frac r, frac g, frac b, frac out[])
{
    
    gx_device_cmap *    cmdev = (gx_device_cmap *)dev;
    frac gx_max_color_frac   = cv2frac(gx_max_color_value);
    switch (cmdev->mapping_method) {
        
    case device_cmap_snap_to_primaries:
        r = (r <= gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        g = (g <= gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        b = (b <= gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        break;

    case device_cmap_color_to_black_over_white:
        if (r == 0 && g == 0 && b == 0)
            r = g = b = gx_max_color_frac;
        else
            r = g = b = 0;
        break;

    case device_cmap_monochrome:
        r = g = b = color_rgb_to_gray(r, g, b, NULL);
        break;
    }
    {
        gx_device *target = cmdev->target ? cmdev->target : dev;
        gx_cm_color_map_procs *cm_procs =  (dev_proc( target, get_color_mapping_procs)(target));
        cm_procs->map_rgb(target, pis, r, g, b, out );
    }
}

private void
cmap_cmyk_cs_to_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
    gx_device_cmap *    cmdev = (gx_device_cmap *)dev;
    frac gx_max_color_frac   = cv2frac(gx_max_color_value);
    /* We aren't sure what to do with k so we leave it alone.  NB this
       routine is untested and does not appear to be called.  More
       testing needed. */
    switch (cmdev->mapping_method) {
    case device_cmap_snap_to_primaries:
        c = (c > gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        m = (m > gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        y = (y > gx_max_color_frac / 2 ? 0 : gx_max_color_frac);
        break;

    case device_cmap_color_to_black_over_white:
        if (c == gx_max_color_frac && m == gx_max_color_frac && y == gx_max_color_frac)
            c = m = y = 0;
        else
            c = m = y = gx_max_color_frac;
        break;

    case device_cmap_monochrome:
        c = m = y = color_cmyk_to_gray(c, m, y, k, NULL);
        break;
    }
    {
        gx_device *target = cmdev->target ? cmdev->target : dev;
        gx_cm_color_map_procs *cm_procs =  (dev_proc( target, get_color_mapping_procs)(target));
        cm_procs->map_cmyk(target, c, m, y, k, out );
    }
}

private const gx_cm_color_map_procs cmap_cm_procs = {
    cmap_gray_cs_to_cm, cmap_rgb_cs_to_cm, cmap_cmyk_cs_to_cm
};


private const gx_cm_color_map_procs *
cmap_get_color_mapping_procs(const gx_device * dev)
{
    return &cmap_cm_procs;
}

