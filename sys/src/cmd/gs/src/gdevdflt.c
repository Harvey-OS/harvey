/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevdflt.c,v 1.25 2005/03/14 18:08:36 dan Exp $ */
/* Default device implementation */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsropt.h"
#include "gxcomp.h"
#include "gxdevice.h"

/* ---------------- Default device procedures ---------------- */

/*
 * Set a color model polarity to be additive or subtractive. In either
 * case, indicate an error (and don't modify the polarity) if the current
 * setting differs from the desired and is not GX_CINFO_POLARITY_UNKNOWN.
 */
static void
set_cinfo_polarity(gx_device * dev, gx_color_polarity_t new_polarity)
{
#ifdef DEBUG
    /* sanity check */
    if (new_polarity == GX_CINFO_POLARITY_UNKNOWN) {
        dprintf("set_cinfo_polarity: illegal operand");
        return;
    }
#endif
    /*
     * The meory devices assume that single color devices are gray.
     * This may not be true if SeparationOrder is specified.  Thus only
     * change the value if the current value is unknown.
     */
    if (dev->color_info.polarity == GX_CINFO_POLARITY_UNKNOWN)
        dev->color_info.polarity = new_polarity;
}


private gx_color_index
(*get_encode_color(gx_device *dev))(gx_device *, const gx_color_value *)
{
    dev_proc_encode_color(*encode_proc);

    /* use encode_color if it has been provided */
    if ((encode_proc = dev_proc(dev, encode_color)) == 0) {
	if (dev->color_info.num_components == 1                          &&
	    dev_proc(dev, map_rgb_color) != 0) {
	    set_cinfo_polarity(dev, GX_CINFO_POLARITY_ADDITIVE);
	    encode_proc = gx_backwards_compatible_gray_encode;
	} else  if ( (dev->color_info.num_components == 3    )           &&
             (encode_proc = dev_proc(dev, map_rgb_color)) != 0  )
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_ADDITIVE);
        else if ( dev->color_info.num_components == 4                    &&
                 (encode_proc = dev_proc(dev, map_cmyk_color)) != 0   )
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_SUBTRACTIVE);
    }

    /*
     * If no encode_color procedure at this point, the color model had
     * better be monochrome (though not necessarily bi-level). In this
     * case, it is assumed to be additive, as that is consistent with
     * the pre-DeviceN code.
     *
     * If this is not the case, then the color model had better be known
     * to be separable and linear, for there is no other way to derive
     * an encoding. This is the case even for weakly linear and separable
     * color models with a known polarity.
     */
    if (encode_proc == 0) {
        if (dev->color_info.num_components == 1 && dev->color_info.depth != 0) {
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_ADDITIVE);
            if (dev->color_info.max_gray == (1 << dev->color_info.depth) - 1)
                encode_proc = gx_default_gray_fast_encode;
            else
                encode_proc = gx_default_gray_encode;
            dev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
        } else if (dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN) {
            gx_color_value  max_gray = dev->color_info.max_gray;
            gx_color_value  max_color = dev->color_info.max_color;

            if ( (max_gray & (max_gray + 1)) == 0  &&
                 (max_color & (max_color + 1)) == 0  )
                /* NB should be gx_default_fast_encode_color */
                encode_proc = gx_default_encode_color;
            else
                encode_proc = gx_default_encode_color;
        } else
            dprintf("get_encode_color: no color encoding information");
    }

    return encode_proc;
}

/*
 * Determine if a color model has the properties of a DeviceRGB
 * color model. This procedure is, in all likelihood, high-grade
 * overkill, but since this is not a performance sensitive area
 * no harm is done.
 *
 * Since there is little benefit to checking the values 0, 1, or
 * 1/2, we use the values 1/4, 1/3, and 3/4 in their place. We
 * compare the results to see if the intensities match to within
 * a tolerance of .01, which is arbitrarily selected.
 */

private bool
is_like_DeviceRGB(gx_device * dev)
{
    const gx_cm_color_map_procs *   cm_procs;
    frac                            cm_comp_fracs[3];
    int                             i;

    if ( dev->color_info.num_components != 3                   ||
         dev->color_info.polarity != GX_CINFO_POLARITY_ADDITIVE  )
        return false;
    cm_procs = dev_proc(dev, get_color_mapping_procs)(dev);
    if (cm_procs == 0 || cm_procs->map_rgb == 0)
        return false;

    /* check the values 1/4, 1/3, and 3/4 */
    cm_procs->map_rgb( dev,
                       0,
                       frac_1 / 4,
                       frac_1 / 3,
                       3 * frac_1 / 4,
                       cm_comp_fracs );

    /* verify results to .01 */
    cm_comp_fracs[0] -= frac_1 / 4;
    cm_comp_fracs[1] -= frac_1 / 3;
    cm_comp_fracs[2] -= 3 * frac_1 / 4;
    for ( i = 0;
           i < 3                            &&
           -frac_1 / 100 < cm_comp_fracs[i] &&
           cm_comp_fracs[i] < frac_1 / 100;
          i++ )
        ;
    return i == 3;
}

/*
 * Similar to is_like_DeviceRGB, but for DeviceCMYK.
 */
private bool
is_like_DeviceCMYK(gx_device * dev)
{
    const gx_cm_color_map_procs *   cm_procs;
    frac                            cm_comp_fracs[4];
    int                             i;

    if ( dev->color_info.num_components != 4                      ||
         dev->color_info.polarity != GX_CINFO_POLARITY_SUBTRACTIVE  )
        return false;
    cm_procs = dev_proc(dev, get_color_mapping_procs)(dev);
    if (cm_procs == 0 || cm_procs->map_cmyk == 0)
        return false;

    /* check the values 1/4, 1/3, 3/4, and 1/8 */
    cm_procs->map_cmyk( dev,
                        frac_1 / 4,
                        frac_1 / 3,
                        3 * frac_1 / 4,
                        frac_1 / 8,
                        cm_comp_fracs );

    /* verify results to .01 */
    cm_comp_fracs[0] -= frac_1 / 4;
    cm_comp_fracs[1] -= frac_1 / 3;
    cm_comp_fracs[2] -= 3 * frac_1 / 4;
    cm_comp_fracs[3] -= frac_1 / 8;
    for ( i = 0;
           i < 4                            &&
           -frac_1 / 100 < cm_comp_fracs[i] &&
           cm_comp_fracs[i] < frac_1 / 100;
          i++ )
        ;
    return i == 4;
}

/*
 * Two default decode_color procedures to use for monochrome devices.
 * These will make use of the map_color_rgb routine, and use the first
 * component of the returned value or its inverse.
 */
private int
gx_default_1_add_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[1] )
{
    gx_color_value  rgb[3];
    int             code = dev_proc(dev, map_color_rgb)(dev, color, rgb);

    cv[0] = rgb[0];
    return code;
}

private int
gx_default_1_sub_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[1] )
{
    gx_color_value  rgb[3];
    int             code = dev_proc(dev, map_color_rgb)(dev, color, rgb);

    cv[0] = gx_max_color_value - rgb[0];
    return code;
}

/*
 * A default decode_color procedure for DeviceCMYK color models.
 *
 * There is no generally accurate way of decode a DeviceCMYK color using
 * the map_color_rgb method. Unfortunately, there are many older devices
 * employ the DeviceCMYK color model but don't provide a decode_color
 * method. The code below works on the assumption of full undercolor
 * removal and black generation. This may not be accurate, but is the
 * best that can be done in the general case without other information.
 */
private int
gx_default_cmyk_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[4] )
{
    /* The device may have been determined to be 'separable'. */
    if (dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN)
	return gx_default_decode_color(dev, color, cv);
    else {
        int i, code = dev_proc(dev, map_color_rgb)(dev, color, cv);
        gx_color_value min_val = gx_max_color_value;

        for (i = 0; i < 3; i++) {
            if ((cv[i] = gx_max_color_value - cv[i]) < min_val)
                min_val = cv[i];
        }
        for (i = 0; i < 3; i++)
            cv[i] -= min_val;
        cv[3] = min_val;

        return code;
    }
}

/*
 * Special case default color decode routine for a canonical 1-bit per
 * component DeviceCMYK color model.
 */
private int
gx_1bit_cmyk_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[4] )
{
    cv[0] = ((color & 0x8) != 0 ? gx_max_color_value : 0);
    cv[1] = ((color & 0x4) != 0 ? gx_max_color_value : 0);
    cv[2] = ((color & 0x2) != 0 ? gx_max_color_value : 0);
    cv[3] = ((color & 0x1) != 0 ? gx_max_color_value : 0);
    return 0;
}

private int
(*get_decode_color(gx_device * dev))(gx_device *, gx_color_index, gx_color_value *)
{
    /* if a method has already been provided, use it */
    if (dev_proc(dev, decode_color) != 0)
        return dev_proc(dev, decode_color);

    /*
     * If a map_color_rgb method has been provided, we may be able to use it.
     * Currently this will always be the case, as a default value will be
     * provided this method. While this default may not be correct, we are not
     * introducing any new errors by using it.
     */
    if (dev_proc(dev, map_color_rgb) != 0) {

        /* if the device has a DeviceRGB color model, use map_color_rgb */
        if (is_like_DeviceRGB(dev))
            return dev_proc(dev, map_color_rgb);

	/* If separable ande linear then use default */
        if ( dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN )
            return &gx_default_decode_color;

        /* gray devices can be handled based on their polarity */
        if ( dev->color_info.num_components == 1 &&
             dev->color_info.gray_index == 0       )
            return dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE
                       ? &gx_default_1_add_decode_color
                       : &gx_default_1_sub_decode_color;

        /*
         * There is no accurate way to decode colors for cmyk devices
         * using the map_color_rgb procedure. Unfortunately, this cases
         * arises with some frequency, so it is useful not to generate an
         * error in this case. The mechanism below assumes full undercolor
         * removal and black generation, which may not be accurate but are
         * the  best that can be done in the general case in the absence of
         * other information.
         *
         * As a hack to handle certain common devices, if the map_rgb_color
         * routine is cmyk_1bit_map_color_rgb, we provide a direct one-bit
         * decoder.
         */
        if (is_like_DeviceCMYK(dev)) {
            if (dev_proc(dev, map_color_rgb) == cmyk_1bit_map_color_rgb)
                return &gx_1bit_cmyk_decode_color;
            else
                return &gx_default_cmyk_decode_color;
        }
    }

    /*
     * The separable and linear case will already have been handled by
     * code in gx_device_fill_in_procs, so at this point we can only hope
     * the device doesn't use the decode_color method.
     */
    if (dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN )
        return &gx_default_decode_color;
    else
        return &gx_error_decode_color;
}

/*
 * If a device has a linear and separable encode color function then
 * set up the comp_bits, comp_mask, and comp_shift fields.  Note:  This
 * routine assumes that the colorant shift factor decreases with the
 * component number.  See check_device_separable() for a general routine.
 */
void
set_linear_color_bits_mask_shift(gx_device * dev)
{
    int i;
    byte gray_index = dev->color_info.gray_index;
    gx_color_value max_gray = dev->color_info.max_gray;
    gx_color_value max_color = dev->color_info.max_color;
    int num_components = dev->color_info.num_components;

#define comp_bits (dev->color_info.comp_bits)
#define comp_mask (dev->color_info.comp_mask)
#define comp_shift (dev->color_info.comp_shift)
    comp_shift[num_components - 1] = 0;
    for ( i = num_components - 1 - 1; i >= 0; i-- ) {
        comp_shift[i] = comp_shift[i + 1] + 
            ( i == gray_index ? ilog2(max_gray + 1) : ilog2(max_color + 1) );
    }
    for ( i = 0; i < num_components; i++ ) {
        comp_bits[i] = ( i == gray_index ?
                         ilog2(max_gray + 1) :
                         ilog2(max_color + 1) );
        comp_mask[i] = (((gx_color_index)1 << comp_bits[i]) - 1)
                                               << comp_shift[i];
    }
#undef comp_bits
#undef comp_mask
#undef comp_shift
}

/* Determine if a number is a power of two.  Works only for integers. */
#define is_power_of_two(x) ((((x) - 1) & (x)) == 0)

/*
 * This routine attempts to determine if a device's encode_color procedure
 * produces gx_color_index values which are 'separable'.  A 'separable' value
 * means two things.  Each colorant has a group of bits in the gx_color_index
 * value which is associated with the colorant.  These bits are separate.
 * I.e. no bit is associated with more than one colorant.  If a colorant has
 * a value of zero then the bits associated with that colorant are zero.
 * These criteria allows the graphics library to build gx_color_index values
 * from the colorant values and not using the encode_color routine. This is
 * useful and necessary for overprinting, the WTS screeening, halftoning more
 * than four colorants, and the fast shading logic.  However this information
 * is not setup by the default device macros.  Thus we attempt to derive this
 * information.
 *
 * This routine can be fooled.  However it usually errors on the side of
 * assuing that a device is not separable.  In this case it does not create
 * any new problems.  In theory it can be fooled into believing that a device
 * is separable when it is not.  However we do not know of any real cases that
 * will fool it.
 */
void
check_device_separable(gx_device * dev)
{
    int i, j;
    gx_device_color_info * pinfo = &(dev->color_info);
    int num_components = pinfo->num_components;
    byte comp_shift[GX_DEVICE_COLOR_MAX_COMPONENTS];
    byte comp_bits[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index comp_mask[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color_index;
    gx_color_index current_bits = 0;
    gx_color_value colorants[GX_DEVICE_COLOR_MAX_COMPONENTS] = { 0 };

    /* If this is already known then we do not need to do anything. */
    if (pinfo->separable_and_linear != GX_CINFO_UNKNOWN_SEP_LIN)
	return;
    /* If there is not an encode_color_routine then we cannot proceed. */
    if (dev_proc(dev, encode_color) == NULL)
	return;
    /*
     * If these values do not check then we should have an error.  However
     * we do not know what to do so we are simply exitting and hoping that
     * the device will clean up its values.
     */
    if (pinfo->gray_index < num_components &&
        (!pinfo->dither_grays || pinfo->dither_grays != (pinfo->max_gray + 1)))
	    return;
    if ((num_components > 1 || pinfo->gray_index != 0) &&
	(!pinfo->dither_colors || pinfo->dither_colors != (pinfo->max_color + 1)))
	return;
    /*
     * If dither_grays or dither_colors is not a power of two then we assume
     * that the device is not separable.  In theory this not a requirement
     * but it has been true for all of the devices that we have seen so far.
     * This assumption also makes the logic in the next section easier.
     */
    if (!is_power_of_two(pinfo->dither_grays)
		    || !is_power_of_two(pinfo->dither_colors))
	return;
    /*
     * Use the encode_color routine to try to verify that the device is
     * separable and to determine the shift count, etc. for each colorant.
     */ 
    color_index = dev_proc(dev, encode_color)(dev, colorants);
    if (color_index != 0)
	return;		/* Exit if zero colorants produce a non zero index */
    for (i = 0; i < num_components; i++) {
	/* Check this colorant = max with all others = 0 */
        for (j = 0; j < num_components; j++)
	    colorants[j] = 0;
	colorants[i] = gx_max_color_value;
	color_index = dev_proc(dev, encode_color)(dev, colorants);
	if (color_index == 0)	/* If no bits then we have a problem */
	    return;
	if (color_index & current_bits)	/* Check for overlapping bits */
	    return;
	current_bits |= color_index;
	comp_mask[i] = color_index;
	/* Determine the shift count for the colorant */
	for (j = 0; (color_index & 1) == 0 && color_index != 0; j++)
	    color_index >>= 1;
	comp_shift[i] = j;
	/* Determine the bit count for the colorant */
	for (j = 0; color_index != 0; j++) {
	    if ((color_index & 1) == 0) /* check for non-consecutive bits */
		return;
	    color_index >>= 1;
	}
	comp_bits[i] = j;
	/*
	 * We could verify that the bit count matches the dither_grays or
	 * dither_colors values, but this is not really required unless we
	 * are halftoning.  Thus we are allowing for non equal colorant sizes.
	 */
	/* Check for overlap with other colorant if they are all maxed */
        for (j = 0; j < num_components; j++)
	    colorants[j] = gx_max_color_value;
	colorants[i] = 0;
	color_index = dev_proc(dev, encode_color)(dev, colorants);
	if (color_index & comp_mask[i])	/* Check for overlapping bits */
	    return;
    }
    /* If we get to here then the device is very likely to be separable. */
    pinfo->separable_and_linear = GX_CINFO_SEP_LIN;
    for (i = 0; i < num_components; i++) {
	pinfo->comp_shift[i] = comp_shift[i];
	pinfo->comp_bits[i] = comp_bits[i];
	pinfo->comp_mask[i] = comp_mask[i];
    }
    /*
     * The 'gray_index' value allows one colorant to have a different number
     * of shades from the remainder.  Since the default macros only guess at
     * an appropriate value, we are setting its value based upon the data that
     * we just determined.  Note:  In some cases the macros set max_gray to 0
     * and dither_grays to 1.  This is not valid so ignore this case.
     */
    for (i = 0; i < num_components; i++) {
	int dither = 1 << comp_bits[i];

	if (pinfo->dither_grays != 1 && dither == pinfo->dither_grays) {
	    pinfo->gray_index = i;
	    break;
        }
    }
}
#undef is_power_of_two

/* Fill in NULL procedures in a device procedure record. */
void
gx_device_fill_in_procs(register gx_device * dev)
{
    gx_device_set_procs(dev);
    fill_dev_proc(dev, open_device, gx_default_open_device);
    fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
    fill_dev_proc(dev, sync_output, gx_default_sync_output);
    fill_dev_proc(dev, output_page, gx_default_output_page);
    fill_dev_proc(dev, close_device, gx_default_close_device);
    /* see below for map_rgb_color */
    fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);
    /* NOT fill_rectangle */
    fill_dev_proc(dev, tile_rectangle, gx_default_tile_rectangle);
    fill_dev_proc(dev, copy_mono, gx_default_copy_mono);
    fill_dev_proc(dev, copy_color, gx_default_copy_color);
    fill_dev_proc(dev, obsolete_draw_line, gx_default_draw_line);
    fill_dev_proc(dev, get_bits, gx_default_get_bits);
    fill_dev_proc(dev, get_params, gx_default_get_params);
    fill_dev_proc(dev, put_params, gx_default_put_params);
    /* see below for map_cmyk_color */
    fill_dev_proc(dev, get_xfont_procs, gx_default_get_xfont_procs);
    fill_dev_proc(dev, get_xfont_device, gx_default_get_xfont_device);
    fill_dev_proc(dev, map_rgb_alpha_color, gx_default_map_rgb_alpha_color);
    fill_dev_proc(dev, get_page_device, gx_default_get_page_device);
    fill_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    fill_dev_proc(dev, copy_alpha, gx_default_copy_alpha);
    fill_dev_proc(dev, get_band, gx_default_get_band);
    fill_dev_proc(dev, copy_rop, gx_default_copy_rop);
    fill_dev_proc(dev, fill_path, gx_default_fill_path);
    fill_dev_proc(dev, stroke_path, gx_default_stroke_path);
    fill_dev_proc(dev, fill_mask, gx_default_fill_mask);
    fill_dev_proc(dev, fill_trapezoid, gx_default_fill_trapezoid);
    fill_dev_proc(dev, fill_parallelogram, gx_default_fill_parallelogram);
    fill_dev_proc(dev, fill_triangle, gx_default_fill_triangle);
    fill_dev_proc(dev, draw_thin_line, gx_default_draw_thin_line);
    fill_dev_proc(dev, begin_image, gx_default_begin_image);
    /*
     * We always replace get_alpha_bits, image_data, and end_image with the
     * new procedures, and, if in a DEBUG configuration, print a warning if
     * the definitions aren't the default ones.
     */
#ifdef DEBUG
#  define CHECK_NON_DEFAULT(proc, default, procname)\
    BEGIN\
	if ( dev_proc(dev, proc) != NULL && dev_proc(dev, proc) != default )\
	    dprintf2("**** Warning: device %s implements obsolete procedure %s\n",\
		     dev->dname, procname);\
    END
#else
#  define CHECK_NON_DEFAULT(proc, default, procname)\
    DO_NOTHING
#endif
    CHECK_NON_DEFAULT(get_alpha_bits, gx_default_get_alpha_bits,
		      "get_alpha_bits");
    set_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    CHECK_NON_DEFAULT(image_data, gx_default_image_data, "image_data");
    set_dev_proc(dev, image_data, gx_default_image_data);
    CHECK_NON_DEFAULT(end_image, gx_default_end_image, "end_image");
    set_dev_proc(dev, end_image, gx_default_end_image);
#undef CHECK_NON_DEFAULT
    fill_dev_proc(dev, strip_tile_rectangle, gx_default_strip_tile_rectangle);
    fill_dev_proc(dev, strip_copy_rop, gx_default_strip_copy_rop);
    fill_dev_proc(dev, get_clipping_box, gx_default_get_clipping_box);
    fill_dev_proc(dev, begin_typed_image, gx_default_begin_typed_image);
    fill_dev_proc(dev, get_bits_rectangle, gx_default_get_bits_rectangle);
    fill_dev_proc(dev, map_color_rgb_alpha, gx_default_map_color_rgb_alpha);
    fill_dev_proc(dev, create_compositor, gx_default_create_compositor);
    fill_dev_proc(dev, get_hardware_params, gx_default_get_hardware_params);
    fill_dev_proc(dev, text_begin, gx_default_text_begin);
    fill_dev_proc(dev, finish_copydevice, gx_default_finish_copydevice);

    set_dev_proc(dev, encode_color, get_encode_color(dev));
    if (dev->color_info.num_components == 3)
	set_dev_proc(dev, map_rgb_color, dev_proc(dev, encode_color));
    if (dev->color_info.num_components == 4)
	set_dev_proc(dev, map_cmyk_color, dev_proc(dev, encode_color));

    if ( dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN ) {
        fill_dev_proc(dev, encode_color, gx_default_encode_color);
        fill_dev_proc(dev, map_cmyk_color, gx_default_encode_color);
        fill_dev_proc(dev, map_rgb_color, gx_default_encode_color);
    } else {
        /* if it isn't set now punt */
        fill_dev_proc(dev, encode_color, gx_error_encode_color);
        fill_dev_proc(dev, map_cmyk_color, gx_error_encode_color);
        fill_dev_proc(dev, map_rgb_color, gx_error_encode_color);
    }

    /*
     * Fill in the color mapping procedures and the component index
     * assignment procedure if they have not been provided by the client.
     *
     * Because it is difficult to provide default encoding procedures
     * that handle level inversion, this code needs to check both
     * the number of components and the polarity of color model.
     */
    switch (dev->color_info.num_components) {
    case 1:     /* DeviceGray or DeviceInvertGray */
	if (dev_proc(dev, get_color_mapping_procs) == NULL) {
	    /*
	     * If not gray then the device must provide the color
	     * mapping procs.
	     */
	    if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE) {
		fill_dev_proc( dev,
                           get_color_mapping_procs,
                           gx_default_DevGray_get_color_mapping_procs );
            }
	}
        fill_dev_proc( dev,
                       get_color_comp_index,
                       gx_default_DevGray_get_color_comp_index );
        break;

    case 3:
	if (dev_proc(dev, get_color_mapping_procs) == NULL) {
            if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE) {
                fill_dev_proc( dev,
                           get_color_mapping_procs,
                           gx_default_DevRGB_get_color_mapping_procs );
                fill_dev_proc( dev,
                           get_color_comp_index,
                           gx_default_DevRGB_get_color_comp_index );
            } else {
#if 0
                fill_dev_proc( dev,
                           get_color_mapping_procs,
                           gx_default_DevCMY_get_color_mapping_procs );
                fill_dev_proc( dev,
                           get_color_comp_index,
                           gx_default_DevCMY_get_color_comp_index );
#endif
	    }
        }
        break;

    case 4:
        fill_dev_proc(dev, get_color_mapping_procs, gx_default_DevCMYK_get_color_mapping_procs);
        fill_dev_proc(dev, get_color_comp_index, gx_default_DevCMYK_get_color_comp_index);
        break;
    default:		/* Unknown color model - set error handlers */
        fill_dev_proc(dev, get_color_mapping_procs, gx_error_get_color_mapping_procs);
        fill_dev_proc(dev, get_color_comp_index, gx_error_get_color_comp_index);
    }

    set_dev_proc(dev, decode_color, get_decode_color(dev));
    fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);

    /*
     * If the device is known not to support overprint mode, indicate this now.
     * Note that we do not insist that a device be use a strict DeviceCMYK
     * encoding; any color model that is subtractive and supports the cyan,
     * magenta, yellow, and black color components will do. We defer a more
     * explicit check until this information is explicitly required.
     */
    if ( dev->color_info.opmode == GX_CINFO_OPMODE_UNKNOWN          &&
         (dev->color_info.num_components < 4                     || 
          dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE ||
          dev->color_info.gray_index == GX_CINFO_COMP_NO_INDEX     )  )
	dev->color_info.opmode = GX_CINFO_OPMODE_NOT;

    fill_dev_proc(dev, pattern_manage, gx_default_pattern_manage);
    fill_dev_proc(dev, fill_rectangle_hl_color, gx_default_fill_rectangle_hl_color);
    fill_dev_proc(dev, include_color_space, gx_default_include_color_space);
    fill_dev_proc(dev, fill_linear_color_scanline, gx_default_fill_linear_color_scanline);
    fill_dev_proc(dev, fill_linear_color_trapezoid, gx_default_fill_linear_color_trapezoid);
    fill_dev_proc(dev, fill_linear_color_triangle, gx_default_fill_linear_color_triangle);
    fill_dev_proc(dev, update_spot_equivalent_colors, gx_default_update_spot_equivalent_colors);
}

int
gx_default_open_device(gx_device * dev)
{
    /* Initialize the separable status if not known. */
    check_device_separable(dev);
    return 0;
}

/* Get the initial matrix for a device with inverted Y. */
/* This includes essentially all printers and displays. */
void
gx_default_get_initial_matrix(gx_device * dev, register gs_matrix * pmat)
{
    pmat->xx = dev->HWResolution[0] / 72.0;	/* x_pixels_per_inch */
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = dev->HWResolution[1] / -72.0;	/* y_pixels_per_inch */
    /****** tx/y is WRONG for devices with ******/
    /****** arbitrary initial matrix ******/
    pmat->tx = 0;
    pmat->ty = (float)dev->height;
}
/* Get the initial matrix for a device with upright Y. */
/* This includes just a few printers and window systems. */
void
gx_upright_get_initial_matrix(gx_device * dev, register gs_matrix * pmat)
{
    pmat->xx = dev->HWResolution[0] / 72.0;	/* x_pixels_per_inch */
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = dev->HWResolution[1] / 72.0;	/* y_pixels_per_inch */
    /****** tx/y is WRONG for devices with ******/
    /****** arbitrary initial matrix ******/
    pmat->tx = 0;
    pmat->ty = 0;
}

int
gx_default_sync_output(gx_device * dev)
{
    return 0;
}

int
gx_default_output_page(gx_device * dev, int num_copies, int flush)
{
    int code = dev_proc(dev, sync_output)(dev);

    if (code >= 0)
	code = gx_finish_output_page(dev, num_copies, flush);
    return code;
}

int
gx_default_close_device(gx_device * dev)
{
    return 0;
}

const gx_xfont_procs *
gx_default_get_xfont_procs(gx_device * dev)
{
    return NULL;
}

gx_device *
gx_default_get_xfont_device(gx_device * dev)
{
    return dev;
}

gx_device *
gx_default_get_page_device(gx_device * dev)
{
    return NULL;
}
gx_device *
gx_page_device_get_page_device(gx_device * dev)
{
    return dev;
}

int
gx_default_get_alpha_bits(gx_device * dev, graphics_object_type type)
{
    return (type == go_text ? dev->color_info.anti_alias.text_bits :
	    dev->color_info.anti_alias.graphics_bits);
}

int
gx_default_get_band(gx_device * dev, int y, int *band_start)
{
    return 0;
}

void
gx_default_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = 0;
    pbox->p.y = 0;
    pbox->q.x = int2fixed(dev->width);
    pbox->q.y = int2fixed(dev->height);
}
void
gx_get_largest_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = min_fixed;
    pbox->p.y = min_fixed;
    pbox->q.x = max_fixed;
    pbox->q.y = max_fixed;
}

int
gx_no_create_compositor(gx_device * dev, gx_device ** pcdev,
			const gs_composite_t * pcte,
			gs_imager_state * pis, gs_memory_t * memory)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}
int
gx_default_create_compositor(gx_device * dev, gx_device ** pcdev,
			     const gs_composite_t * pcte,
			     gs_imager_state * pis, gs_memory_t * memory)
{
    return pcte->type->procs.create_default_compositor
	(pcte, pcdev, dev, pis, memory);
}
int
gx_null_create_compositor(gx_device * dev, gx_device ** pcdev,
			  const gs_composite_t * pcte,
			  gs_imager_state * pis, gs_memory_t * memory)
{
    *pcdev = dev;
    return 0;
}

/*
 * Default handler for creating a compositor device when writing the clist. */
int
gx_default_composite_clist_write_update(const gs_composite_t *pcte, gx_device * dev,
		gx_device ** pcdev, gs_imager_state * pis, gs_memory_t * mem)
{
    *pcdev = dev;		/* Do nothing -> return the same device */
    return 0;
}

/*
 * Default handler for updating the clist device when reading a compositing
 * device.
 */
int
gx_default_composite_clist_read_update(gs_composite_t *pxcte, gx_device * cdev,
		gx_device * tdev, gs_imager_state * pis, gs_memory_t * mem)
{
    return 0;			/* Do nothing */
}

int
gx_default_finish_copydevice(gx_device *dev, const gx_device *from_dev)
{
    /* Only allow copying the prototype. */
    return (from_dev->memory ? gs_note_error(gs_error_rangecheck) : 0);
}

int
gx_default_pattern_manage(gx_device *pdev, gx_bitmap_id id,
		gs_pattern1_instance_t *pinst, pattern_manage_t function)
{
    return 0;
}

int
gx_default_fill_rectangle_hl_color(gx_device *pdev, 
    const gs_fixed_rect *rect, 
    const gs_imager_state *pis, const gx_drawing_color *pdcolor,
    const gx_clip_path *pcpath)
{
    return_error(gs_error_rangecheck);
}

int
gx_default_include_color_space(gx_device *pdev, gs_color_space *cspace, 
	const byte *res_name, int name_length)
{
    return 0;
}

/*
 * If a device want to determine an equivalent color for its spot colors then
 * it needs to implement this method.  See comments at the start of
 * src/gsequivc.c.
 */
int
gx_default_update_spot_equivalent_colors(gx_device *pdev, const gs_state * pgs)
{
    return 0;
}

/* ---------------- Default per-instance procedures ---------------- */

int
gx_default_install(gx_device * dev, gs_state * pgs)
{
    return 0;
}

int
gx_default_begin_page(gx_device * dev, gs_state * pgs)
{
    return 0;
}

int
gx_default_end_page(gx_device * dev, int reason, gs_state * pgs)
{
    return (reason != 2 ? 1 : 0);
}
