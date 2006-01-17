/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsovrc.h,v 1.4 2004/09/22 00:37:12 dan Exp $ */
/* overprint/overprint mode compositor interface */

#ifndef gsovrc_INCLUDED
#  define gsovrc_INCLUDED

#include "gsstype.h"
#include "gxcomp.h"

/*
 * Overprint compositor.
 *
 * The overprint and overprint mode capability allow drawing operations
 * to affect a subset of the set of color model components, rather than
 * all components. The overprint feature allows the set of affected
 * components to be determined by the color space, but independently of
 * the drawing color. Overprint mode allows both the color space and
 * drawing color to determine which components are affected.
 *
 * The significance of the overprint feature for a device is dependent on
 * the extent of rendering that is to be performed for that device:
 *
 *  1. High level devices, such as the PDFWrite and PSWrite devices,
 *     do not, in principle, require the graphic library to convert
 *     between color spaces and color models. These devices can accept
 *     color space descriptions, so conversion to a color model can be
 *     left to whatever application uses the generated file.
 *
 *     For these devices, overprint and overprint mode are just
 *     additional state parameters that are incorporated into the output
 *     file, which may or may not be supported on the eventual target
 *     application.
 *
 *     Note that the current implementation of PDFWrite and PSWrite
 *     do require the graphic library to convert between color spaces
 *     and a color model, for all objects except images. To implement
 *     the overprint feature under this design, the devices would
 *     have to support the get_bits_rectangle method, which means they
 *     would need to function as low-level devices. Since that defeats
 *     the purpose of having a high-level device, for the time being
 *     overprint and overprint mode are not fully supported for the
 *     PDFWrite and PSWrite devices.
 *
 *  2. Low level devices require the graphic library to convert between
 *     color spaces and color models, and perform actual rendering. For
 *     these devices both overprint and overprint mode require a form
 *     of mixing of the drawing color with any existing output. This
 *     mixing is mechanically similar to that required for transparency
 *     or raster op support, but differs from these because it affects 
 *     different color components differently (transparency and raster
 *     operations are applied uniformly to all color components).
 *
 * Because the implementation of overprint for high level devices is
 * either trivial or essentially impossible (short of dealing with them
 * as low level devices), the discussion below is restricted to the
 * implementation for low level devices.
 * 
 * In principle, the effects of overprint and overprint mode are
 * modified by changes to the current color, the current color space,
 * and the process color model.
 *
 * In most situation, the process color model cannot be changed without
 * discarding any existing graphic output, so within a given page this
 * affect may be ignored. An exception arises when emulating features
 * such as the "overprint preview" mode of Acrobat 5.0. This mode uses
 * a "fully dynamic" color model: any named color component is consider
 * to exist in the process color model (its conversion to a "real" color
 * is determined by the tint transform function associated with its
 * initial invocation). In this situation, the process color model may
 * "monotonically increase" as a page is processed. Since any such
 * change is associated with a change of color space, we need not be
 * concerned with independent changes in the process color model.
 *
 * For typical graphic operations such as line drawing or area filling,
 * both the current color and the current color space remain constant.
 * Overprint and overprint mode are significant for these operations
 * only if the output page contained data prior to the operation. If
 * this is not the case, overprint is irrelevant, as the initial state
 * of any retained color components, and the state of these components
 * in the drawing color, will be the same.
 *
 * Aside from the devices that generate output, Ghostscript makes use
 * of two additional types of the devices for rendering. Forwarding
 * devices do not generate output of their own; they merely forward
 * rendering commands to a target device. Accumulating devices render
 * output to a special buffer (often part of clipping or a caching 
 * operation), which is subsequently sent to the primary output device 
 * in a lower-level form.
 *
 * It is conceivable that a forwarding device could be dependent on the
 * overprint feature (e.g.: an "inking" filter to determine how much
 * of each colorant is required in each area of the output), but this
 * is unlikely and, in practice, never seems to arise (in practice,
 * inking filters scan the fully generated output). Hence, we ignore
 * this possibility, and need not be concerned with overprint when
 * installing or removing forwarding devices.
 *
 * Overprint is of interest to accumulating devices only if these
 * devices either begin operation with a non-empty output buffer, or if
 * the current color or color space can change during the lifetime of
 * an instantiation of the device. Most instantiations of accumulating
 * devices live only for the rendering of a single graphic object, for
 * which neither the color or color space can change. There are a few
 * exceptions:
 *
 *  - Though images and shaded fills are single graphic objects, they
 *    involve (potentially) many colors. Conveniently, overprint mode
 *    does not apply to either images or shaded fills, so these cause
 *    no difficulty. For the same reason PatternType 2 patterns do
 *    not cause difficulty, though it is necessary to create/update
 *    the overprint compositor to reflect the colorspace included
 *    in the shading dictionary.
 *
 *  - The gs_pdf14_device, which implements the PDF 1.4 transparency
 *    facilities, is an accumulating device that supports only
 *    the high-level rendering methods (fill_path, etc.). Actual
 *    rendering is done with a separate marking device, an instance
 *    of which is created for each graphic object rendered. The
 *    marking device renders into the output buffer of the 
 *    gs_pdf14_device, which contains the results of prior rendering
 *    operations. Thus, overprint is significant to the marking
 *    device. The interaction of transparency and overprint are,
 *    however, more complex than can be captured by a simple
 *    compositor. The gs_pdf14_device and the corresponding device
 *    (will) implement overprint and overprint mode directly. These
 *    devices discard attempts to build an overprint compositor.
 *
 *  - The most essential exception involves colored tiling patterns.
 *    Both the color and color space may change multiple times
 *    during the rendering of a colored pattern. If a colored
 *    pattern is cached, an accurate implementation would require
 *    the cache to included compositor creation instructions.
 *    This is not possible in the current Ghostscript pattern
 *    cache (which has other problems independent of overprint).
 *    The best that can be done is to keep the overprint compositor
 *    current during the caching operation itself. For single level
 *    patterns, this will allow the tile instance to be rendered
 *    accurately, even if the tile does not properly overprint
 *    the output raster.
 *
 * Based on this reasoning, we can limit application of the overprint
 * compositor to a small numer of situations:
 *
 *  1. When the overprint or overprint mode parameter of the current
 *     graphic state is changed (the overprint mode parameter is
 *     ignored if overprint is set to false).
 *
 *  2. When the current color space changes, if overprint is set to true
 *     (the colorspace cannot be relevant if the overprint is false).
 *     There are a few exceptions to this rule for uncolored caches
 *     (user paths, FontType 3 fonts) and anti-alias super-sampling.
 *
 *  3. If the current device color changes, if overprint is true and
 *     overprint mode is 1.
 *
 *  4. If the device in the graphic state is changed, or if this device is
 *     reopened after having been closed (e.g.: due to the action of
 *     put_params), if the overprint parameter is set to true.
 *
 *  5. During grestore or setgstate (because the overprint/overprint
 *     mode parameters might change without the current device changing).
 *
 *  6. When creating the pattern accumulator device, if overprint is
 *     set to true.
 *
 * In PostScript and PDF, a change of the color space always has an
 * associated change of the current color, so it is possible to
 * exclude color space changes from the list above. We do not do this,
 * however, because changes in the color space are usually less frequent
 * than changes in the current color, and the latter are only significant
 * if overprint mode is 1.
 *
 * Unlike the other compositors used by Ghostscript (the alpha-channel
 * compositor and the implemented but not used raster operation
 * compositor), once created, the overprint compositor is made part of the
 * current graphic state, and assumes the lifetime and open/close
 * status of the device to which it is applied. In effect, the device to
 * which the compositor is applied will, for purposes of the graphic
 * state, cease to exist as as independent item. The only way to discard
 * the compositor is to change the current device.
 *
 * Subsequent invocations of create_compositor will not create a new
 * compositor. Rather, they change the parameters of the existing
 * compositor device. The compositor functions with only a small
 * performance penalty if overprint is disabled, so there is no reason
 * to discard the compositor if oveprint is turned off.
 *
 * The compositor device will emulate the open/close state of the device
 * to which it is applied. This is accomplished by forwarding all open
 * and close requests. In addition, the compositor device will check the
 * status of the device to which it is applied after each put_params call,
 * and will change its own status to reflect this status. The underlying
 * device is required to close itself if an invocation of put_params
 * changes in the process color model.
 *
 * NB: It is not possible, without an excessive performance delay, for
 *     the compositor to detect all cases in which the device to which
 *     it is applied may close itself due to an error condition. We
 *     believe this will never cause difficulty in practice, as the
 *     closing of a device is not itself used as an error indication.
 */

#ifndef gs_overprint_params_t_DEFINED
#  define gs_overprint_params_t_DEFINED
typedef struct gs_overprint_params_s    gs_overprint_params_t;
#endif

struct gs_overprint_params_s {

    /*
     * Are any component values to be retained?
     *
     * If this is false (overprint off), all other fields in the compositor
     * are ignored, and the compositor does nothing with respect to rendering
     * (it doesn't even impose a performance penalty).
     *
     * If this field is true, the retain_spot_comps and potentially the
     * retained_comps fields should be initialized.
     *
     * Note that this field may be false even if overprint is true. This
     * would be the case if the current color space was a Separation color
     * space with the component "All".
     */
    bool            retain_any_comps;

    /*
     * Are spot (non-process) color component values retained?
     *
     * If overprint is true, this field will be true for all color spaces
     * other than Separation/DeviceN color spaces.
     *
     * The overprint compositor will itself determine what constitutes a
     * process color. This is done by using the color space mapping
     * routines for the target device for all three standard device
     * color spaces (DeviceGray, DeviceRGB, and DeviceCMYK) and the
     * set of all possible colors with individual components either 0
     * or 1. Any color model component which is mapped to 0 for all of
     * these cases is considered a spot color.
     *
     * If this field is true, the drawn_comps field (see below) is ignored.
     *
     * NB: This field should not be used if the DeviceCMYK color space
     *     is being used with a DeviceCMYK color model (which may have
     *     additional spot colors). Such a color model must explicitly
     *     list the set of drawn components, so as to support overprint
     *     mode.
     */
    bool            retain_spot_comps;

    /*
     * The list of color model compoents to be retained (i.e.: that are
     * not affected by drawing operations). The field is bit-encoded;
     * the bit corresponding to component i is (1 << i).  This bit will be
     * 1 if the corresponding component is set from the drawing color, 0 if
     * it is to be left unaffected.
     */
    gx_color_index  drawn_comps;
};

/*
 * The overprint compositor structure. This is exactly analogous to other
 * compositor structures, consisting of a the compositor common elements
 * and the overprint-specific parameters.
 */
typedef struct gs_overprint_s {
    gs_composite_common;
    gs_overprint_params_t   params;
} gs_overprint_t;

/*
 * In a modest violation of good coding practice, the gs_composite_common
 * fields are "known" to be simple (contain no pointers to garbage
 * collected memory), and we also know the gs_overprint_params_t structure
 * to be simple, so we just create a trivial structure descriptor for the
 * entire gs_overprint_s structure.
 */
#define private_st_gs_overprint_t()	/* In gsovrc.c */\
  gs_private_st_simple(st_overprint, gs_overprint_t, "gs_overprint_t");



/* some elementary macros for manipulating drawn_comps */
#define gs_overprint_set_drawn_comp(drawn_comps, i) \
    ((drawn_comps) |= (gx_color_index)1 << (i))

#define gs_overprint_clear_drawn_comp(drawn_comps, i)   \
    ((drawn_comps) &= ~((gx_color_index)1 << 1))

#define gs_overprint_clear_all_drawn_comps(drawn_comps) \
    ((drawn_comps) = 0)

#define gs_overprint_get_drawn_comp(drawn_comps, i)     \
    (((drawn_comps) & ((gx_color_index)1 << (i))) != 0)


/*
 * In the unlikely event that the overprint parameters will ever be
 * allocated on the heap, we provide GC structure descriptors for
 * them.
 */
extern_st(st_overprint_params);

#define public_st_overprint_params_t    /* in gsovrc.c */   \
    gs_public_st_simple( st_overprint_params,               \
                         gs_overprint_params_t,             \
                         "gs_overprint_params_t" )


/* create an overprint composition object */
extern  int    gs_create_overprint(
    gs_composite_t **               ppct,
    const gs_overprint_params_t *   pparams,
    gs_memory_t *                   mem );

/* verify that a compositor is the overprint compositor */
extern bool    gs_is_overprint_compositor(const gs_composite_t * pct);

#endif  /* gsovrc_INCLUDED */
