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

/* $Id: gsovrc.c,v 1.10 2005/06/29 23:46:24 dan Exp $ */
/* overprint/overprint mode compositor implementation */

#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"             /* for gs_next_ids */
#include "gxcomp.h"
#include "gxdevice.h"
#include "gsdevice.h"
#include "gxgetbit.h"
#include "gsovrc.h"
#include "gxdcolor.h"
#include "gxoprect.h"
#include "gsbitops.h"
#include "gxistate.h"


/* GC descriptor for gs_overprint_t */
private_st_gs_overprint_t();

/*
 * Utility routine for encoding or decoding a color index. We cannot use
 * the general integer encoding routins for these, as they may be 64 bits
 * in length (the general routines are only designed for 32 bits). We also
 * cannot use the color-specific routines, as we do not have the required
 * device color information available.
 *
 * The scheme employed is the potentially 64-bit analog of the 32-bit
 * routines: the low order seven bits of each bytes represents a base-128
 * digit, and the high order bit is set if there is another digit. The
 * encoding order is little-endian.
 *
 * The write routine returns 0 on success, with *psize set to the number
 * of bytes used. Alternatively, the return value will be gs_error_rangecheck,
 * with *psize set to the number of bytes required, if there was insufficient
 * space.
 *
 * The read routine returns the number of bytes read on success, or < 0 in
 * the event of an error.
 */
private int
write_color_index(gx_color_index cindex, byte * data, uint * psize)
{
    int             num_bytes = 0;
    gx_color_index  ctmp = cindex;

    for (num_bytes = 1; (ctmp >>= 7) != 0; ++num_bytes)
        ;
    if (num_bytes > *psize) {
        *psize = num_bytes;
        return gs_error_rangecheck;
    }
    ctmp = cindex;
    *psize = num_bytes;
    for (; num_bytes > 1; ctmp >>= 7, --num_bytes)
        *data++ = 0x80 | (ctmp & 0x7f);
    *data = ctmp & 0x7f;
    return 0;
}

private int
read_color_index(gx_color_index * pcindex, const byte * data, uint size)
{
    gx_color_index  cindex = 0;
    int             nbytes = 0, shift = 0;

    for (;; shift += 7, data++) {
        if (++nbytes > size)
            return_error(gs_error_rangecheck);
        else {
            int     c = *data;

            cindex += (c & 0x7f) << shift;
            if ((c & 0x80) == 0)
                break;
        }
    }
    *pcindex = cindex;
    return nbytes;
}

/*
 * Check for equality of two overprint compositor objects.
 *
 * This is fairly simple.
 */
private bool
c_overprint_equal(const gs_composite_t * pct0, const gs_composite_t * pct1)
{
    if (pct0->type == pct1->type) {
        const gs_overprint_params_t *    pparams0;
        const gs_overprint_params_t *    pparams1;

        pparams0 = &((const gs_overprint_t *)(pct0))->params;
        pparams1 = &((const gs_overprint_t *)(pct1))->params;
        if (!pparams0->retain_any_comps)
            return !pparams1->retain_any_comps;
        else if (pparams0->retain_spot_comps)
            return pparams1->retain_spot_comps;
        else
            return pparams0->drawn_comps == pparams1->drawn_comps;
    } else
        return false;
}

/*
 * Bits corresponding to boolean values in the first byte of the string
 * representation of an overprint compositor.
 */
#define OVERPRINT_ANY_COMPS     1
#define OVERPRINT_SPOT_COMPS    2

/*
 * Convert an overprint compositor to string form for use by the command
 * list device.
 */
private int
c_overprint_write(const gs_composite_t * pct, byte * data, uint * psize)
{
    const gs_overprint_params_t *   pparams = &((const gs_overprint_t *)pct)->params;
    byte                            flags = 0;
    int                             used = 1, avail = *psize;

    /* encoded the booleans in a single byte */
    if (pparams->retain_any_comps) {
        flags |= OVERPRINT_ANY_COMPS;

        /* write out the component bits only if necessary (and possible) */
        if (pparams->retain_spot_comps)
            flags |= OVERPRINT_SPOT_COMPS;
        else {
            uint    tmp_size = (avail > 0 ? avail - 1 : 0);
            int     code = write_color_index( pparams->drawn_comps,
                                              data + 1,
                                              &tmp_size );

            if (code < 0 && code != gs_error_rangecheck)
                return code;
            used += tmp_size;
        }            
    }

    /* check for overflow */
    *psize = used;
    if (used > avail)
        return_error(gs_error_rangecheck);
    data[0] = flags;
    return 0;
}

/*
 * Convert the string representation of the overprint parameter into the
 * full compositor.
 */
private int
c_overprint_read(
    gs_composite_t **       ppct,
    const byte *            data,
    uint                    size,
    gs_memory_t *           mem )
{
    gs_overprint_params_t   params;
    byte                    flags = 0;
    int                     code = 0, nbytes = 1;

    if (size < 1)
        return_error(gs_error_rangecheck);
    flags = *data;
    params.retain_any_comps = (flags & OVERPRINT_ANY_COMPS) != 0;
    params.retain_spot_comps = (flags & OVERPRINT_SPOT_COMPS) != 0;

    /* check if the drawn_comps array is present */
    if (params.retain_any_comps && !params.retain_spot_comps) {
        code = read_color_index(&params.drawn_comps, data + 1, size - 1);
        if (code < 0)
            return code;
         nbytes += code;
    }

    code = gs_create_overprint(ppct, &params, mem);

    return code < 0 ? code : nbytes;
}


private composite_create_default_compositor_proc(c_overprint_create_default_compositor);

/* methods for the overprint compositor */
const gs_composite_type_t   gs_composite_overprint_type = {
    GX_COMPOSITOR_OVERPRINT,
    {
        c_overprint_create_default_compositor,  /* procs.create_default_compositor */
        c_overprint_equal,                      /* procs.equal */
        c_overprint_write,                      /* procs.write */
        c_overprint_read,                       /* procs.read */
	gx_default_composite_clist_write_update,/* procs.composite_clist_write_update */
	gx_default_composite_clist_read_update	/* procs.composite_clist_reade_update */
    }                                           /* procs */
};


/*
 * Create an overprint compositor data structure.
 *
 * Currently this just a stub.
 */
int
gs_create_overprint(
    gs_composite_t **               ppct,
    const gs_overprint_params_t *   pparams,
    gs_memory_t *                   mem )
{
    gs_overprint_t *                pct;

    rc_alloc_struct_0( pct,
                       gs_overprint_t,
                       &st_overprint,
                       mem,
                       return_error(gs_error_VMerror),
                       "gs_create_overprint" );
    pct->type = &gs_composite_overprint_type;
    pct->id = gs_next_ids(mem, 1);
    pct->params = *pparams;
    *ppct = (gs_composite_t *)pct;
    return 0;
}


/*
 * Verify that a compositor data structure is for the overprint compositor.
 * This is used by the gs_pdf1.4_device (and eventually the PDFWrite
 * device), which implements overprint and overprint mode directly.
 */
int
gs_is_overprint_compositor(const gs_composite_t * pct)
{
    return pct->type == &gs_composite_overprint_type;
}


/*
 * The overprint device.
 *
 * In principle there are two versions of this device: one if the traget
 * device is separable and linear, the other if it is not. The two have
 * slightly different data structures, but differ primarily in terms of
 * the standard set of methods. Because methods are non-static in
 * GhostScript, we make use of the same data structure and handle the
 * distinction during initialization.
 *
 * The data fields reflect entries in the gs_overprint_params_t
 * structure. There is no explicit retain_any_comps field, as the current
 * setting of this field can be determined by checking the fill_rectangle
 * method. There is also no retain_spot_comps field, as the code will
 * will determine explicitly which components are to be drawn.
 */
typedef struct overprint_device_s {
    gx_device_forward_common;

    /*
     * The set of components to be drawn. This field is used only if the
     * target color space is not separable and linear.
     */
    gx_color_index  drawn_comps;

    /*
     * The mask of gx_color_index bits to be retained during a drawing
     * operation. A bit in this mask is 1 if the corresponding bit or
     * the color index is to be retained; otherwise it is 0.
     *
     * The "non-drawn" region of the drawing gx_color_index is assumed
     * to have the value zero, so for a separable and linear color
     * encoding, the per-pixel drawing operation is:
     *
     *  output = (output & retain_mask) | src
     *
     * (For the fully general case, replace src by (src & ~retain_mask).)
     * Because of byte-alignment, byte-order and performance consideration,
     * the actually implement operation may be more complex, but this does
     * not change the overall effect.
     *
     * The actual value of retain_mask will be byte swap if this is
     * required. It will be required if depth > 8 and the host processor
     * is little-endian.
     */
    gx_color_index  retain_mask;

} overprint_device_t;

gs_private_st_suffix_add0_final( st_overprint_device_t,
                                 overprint_device_t,
                                 "overprint_device_t",
                                 overprint_device_t_enum_ptrs,
                                 overprint_device_t_reloc_ptrs,
                                 gx_device_finalize,
                                 st_device_forward );


/*
 * In the default (overprint false) case, the overprint device is almost
 * a pure forwarding device: only the open_device and create_compositor
 * methods are not pure-forwarding methods. The
 * gx_device_foward_fill_in_procs procedure does not fill in all of the
 * necessary procedures, so some of them are provided explicitly below.
 * The put_params procedure also requires a small modification, so that
 * the open/close state of this device always reflects that of its
 * target.
 *
 * This and other method arrays are not declared const so that they may
 * be initialized once via gx_device_forward_fill_in_procs. They are
 * constant once this initialization is complete.
 */
private dev_proc_open_device(overprint_open_device);
private dev_proc_put_params(overprint_put_params);
private dev_proc_get_page_device(overprint_get_page_device);
private dev_proc_create_compositor(overprint_create_compositor);

private gx_device_procs no_overprint_procs = {
    overprint_open_device,              /* open_device */
    0,                                  /* get_initial_matrix */
    0,                                  /* sync_output */
    0,                                  /* output_page */
    0,                                  /* close_device */
    0,                                  /* map_rgb_color */
    0,                                  /* map_color_rgb */
    gx_forward_fill_rectangle,          /* fill_rectangle */
    gx_forward_tile_rectangle,          /* tile_rectangle */
    gx_forward_copy_mono,               /* copy_mono */
    gx_forward_copy_color,              /* copy_color */
    0,                                  /* draw_line (obsolete) */
    0,                                  /* get_bits */
    0,                                  /* get_params */
    overprint_put_params,               /* put_params */
    0,                                  /* map_cmyk_color */
    0,                                  /* get_xfont_procs */
    0,                                  /* get_xfont_device */
    0,                                  /* map_rgb_alpha_color */
    overprint_get_page_device,          /* get_page_device */
    0,                                  /* get_alpha_bits (obsolete) */
    0,                                  /* copy alpha */
    0,                                  /* get_band */
    0,                                  /* copy_rop */
    0,                                  /* fill_path */
    0,                                  /* stroke_path */
    0,                                  /* fill_mask */
    0,                                  /* fill_trapezoid */
    0,                                  /* fill_parallelogram */
    0,                                  /* fill_triangle */
    0,                                  /* draw_thin_line */
    0,                                  /* begin_image */
    0,                                  /* image_data (obsolete) */
    0,                                  /* end_image (obsolete) */
    gx_forward_strip_tile_rectangle,    /* strip_tile_rectangle */
    0,                                  /* strip_copy_rop */
    0,                                  /* get_clipping_box */
    0,                                  /* begin_typed_image */
    0,                                  /* get_bits_rectangle */
    0,                                  /* map_color_rgb_alpha */
    overprint_create_compositor,        /* create_compositor */
    0,                                  /* get_hardware_params */
    0,                                  /* text_begin */
    0,                                  /* gx_finish_copydevice */
    0,                                  /* begin_transparency_group */
    0,                                  /* end_transparency_group */
    0,                                  /* being_transparency_mask */
    0,                                  /* end_transparency_mask */
    0,                                  /* discard_transparency_layer */
    0,                                  /* get_color_mapping_procs */
    0,                                  /* get_color_comp_index */
    0,                                  /* encode_color */
    0                                   /* decode_color */
};

/*
 * If overprint is set, the high and mid-level rendering methods are
 * replaced by the default routines. The low-level color rendering methods
 * are replaced with one of two sets of functions, depending on whether or
 * not the target device has a separable and linear color encoding.
 *
 *  1. If the target device does not have a separable and linear
 *     encoding, an overprint-specific fill_rectangle method is used,
 *     and the default methods are used for all other low-level rendering
 *     methods. There is no way to achieve good rendering performance
 *     when overprint is true and the color encoding is not separable
 *     and linear, so there is little reason to use more elaborate
 *     methods int this case.
 *
 *  2. If the target device does have a separable and linear color
 *     model, at least the fill_rectangle method and potentially other
 *     methods will be replaced by overprint-specific methods. Those
 *     methods not replaced will have their default values. The number
 *     of methods replaced is dependent on the desired level of
 *     performance: the more methods, the better the performance.
 *
 *     Note that certain procedures, such as copy_alpha and copy_rop,
 *     are likely to always be given their default values, as the concepts
 *     of alpha-compositing and raster operations are not compatible in
 *     a strict sense.
 */
private dev_proc_fill_rectangle(overprint_generic_fill_rectangle);
private dev_proc_fill_rectangle(overprint_sep_fill_rectangle);
/* other low-level overprint_sep_* rendering methods prototypes go here */

private gx_device_procs generic_overprint_procs = {
    overprint_open_device,              /* open_device */
    0,                                  /* get_initial_matrix */
    0,                                  /* sync_output */
    0,                                  /* output_page */
    0,                                  /* close_device */
    0,                                  /* map_rgb_color */
    0,                                  /* map_color_rgb */
    overprint_generic_fill_rectangle,   /* fill_rectangle */
    gx_default_tile_rectangle,          /* tile_rectangle */
    gx_default_copy_mono,               /* copy_mono */
    gx_default_copy_color,              /* copy_color */
    gx_default_draw_line,               /* draw_line (obsolete) */
    0,                                  /* get_bits */
    0,                                  /* get_params */
    overprint_put_params,               /* put_params */
    0,                                  /* map_cmyk_color */
    0,                                  /* get_xfont_procs */
    gx_default_get_xfont_device,        /* get_xfont_device */
    0,                                  /* map_rgb_alpha_color */
    overprint_get_page_device,          /* get_page_device */
    0,                                  /* get_alpha_bits (obsolete) */
    gx_default_copy_alpha,              /* copy alpha */
    0,                                  /* get_band */
    gx_default_copy_rop,                /* copy_rop */
    gx_default_fill_path,               /* fill_path */
    gx_default_stroke_path,             /* stroke_path */
    gx_default_fill_mask,               /* fill_mask */
    gx_default_fill_trapezoid,          /* fill_trapezoid */
    gx_default_fill_parallelogram,      /* fill_parallelogram */
    gx_default_fill_triangle,           /* fill_triangle */
    gx_default_draw_thin_line,          /* draw_thin_line */
    gx_default_begin_image,             /* begin_image */
    0,                                  /* image_data (obsolete) */
    0,                                  /* end_image (obsolete) */
    gx_default_strip_tile_rectangle,    /* strip_tile_rectangle */
    gx_default_strip_copy_rop,          /* strip_copy_rop */
    0,                                  /* get_clipping_box */
    gx_default_begin_typed_image,       /* begin_typed_image */
    0,                                  /* get_bits_rectangle */
    0,                                  /* map_color_rgb_alpha */
    overprint_create_compositor,        /* create_compositor */
    0,                                  /* get_hardware_params */
    gx_default_text_begin,              /* text_begin */
    0,                                  /* gx_finish_copydevice */
    0,                                  /* begin_transparency_group */
    0,                                  /* end_transparency_group */
    0,                                  /* begin_transparency_mask */
    0,                                  /* end_transparency_mask */
    0,                                  /* discard_transparency_layer */
    0,                                  /* get_color_mapping_procs */
    0,                                  /* get_color_comp_index */
    0,                                  /* encode_color */
    0                                   /* decode_color */
};

private gx_device_procs sep_overprint_procs = {
    overprint_open_device,              /* open_device */
    0,                                  /* get_initial_matrix */
    0,                                  /* sync_output */
    0,                                  /* output_page */
    0,                                  /* close_device */
    0,                                  /* map_rgb_color */
    0,                                  /* map_color_rgb */
    overprint_sep_fill_rectangle,       /* fill_rectangle */
    gx_default_tile_rectangle,          /* tile_rectangle */
    gx_default_copy_mono,               /* copy_mono */
    gx_default_copy_color,              /* copy_color */
    gx_default_draw_line,               /* draw_line (obsolete) */
    0,                                  /* get_bits */
    0,                                  /* get_params */
    overprint_put_params,               /* put_params */
    0,                                  /* map_cmyk_color */
    0,                                  /* get_xfont_procs */
    gx_default_get_xfont_device,        /* get_xfont_device */
    0,                                  /* map_rgb_alpha_color */
    overprint_get_page_device,          /* get_page_device */
    0,                                  /* get_alpha_bits (obsolete) */
    gx_default_copy_alpha,              /* copy alpha */
    0,                                  /* get_band */
    gx_default_copy_rop,                /* copy_rop */
    gx_default_fill_path,               /* fill_path */
    gx_default_stroke_path,             /* stroke_path */
    gx_default_fill_mask,               /* fill_mask */
    gx_default_fill_trapezoid,          /* fill_trapezoid */
    gx_default_fill_parallelogram,      /* fill_parallelogram */
    gx_default_fill_triangle,           /* fill_triangle */
    gx_default_draw_thin_line,          /* draw_thin_line */
    gx_default_begin_image,             /* begin_image */
    0,                                  /* image_data (obsolete) */
    0,                                  /* end_image (obsolete) */
    gx_default_strip_tile_rectangle,    /* strip_tile_rectangle */
    gx_default_strip_copy_rop,          /* strip_copy_rop */
    0,                                  /* get_clipping_box */
    gx_default_begin_typed_image,       /* begin_typed_image */
    0,                                  /* get_bits_rectangle */
    0,                                  /* map_color_rgb_alpha */
    overprint_create_compositor,        /* create_compositor */
    0,                                  /* get_hardware_params */
    gx_default_text_begin,              /* text_begin */
    0,                                  /* gx_finish_copydevice */
    0,                                  /* begin_transparency_group */
    0,                                  /* end_transparency_group */
    0,                                  /* begin_transparency_mask */
    0,                                  /* end_transparency_mask */
    0,                                  /* discard_transparency_layer */
    0,                                  /* get_color_mapping_procs */
    0,                                  /* get_color_comp_index */
    0,                                  /* encode_color */
    0                                   /* decode_color */
};

/*
 * The prototype for the overprint device does not provide much
 * information; it exists primarily to facilitate use gx_init_device
 * and sundry other device utility routines.
 */
const overprint_device_t    gs_overprint_device = {
    std_device_std_body_open( overprint_device_t,   /* device type */
                              0,                    /* static_procs */
                              "overprint_device",   /* dname */
                              0, 0,                 /* width, height */
                              1, 1 ),               /* HWResolution */
    { 0 }                                           /* procs */
};



/*
 * Utility to reorder bytes in a color or mask based on the endianness of
 * the current device. This is required on little-endian machines if the
 * depth is larger 8. The resulting value is also replicated to fill the
 * entire gx_color_index if the depth is a divisor of the color index
 * size. If this is not the case, the result will be in the low-order
 * bytes of the color index.
 *
 * Though this process can be handled in full generality, the code below
 * takes advantage of the fact that depths that are > 8 must be a multiple
 * of 8 and <= 64
 */
#if !arch_is_big_endian

private gx_color_index
swap_color_index(int depth, gx_color_index color)
{
    int             shift = depth - 8;
    gx_color_index  mask = 0xff;

    color =  ((color >> shift) & mask)
           | ((color & mask) << shift)
           | (color & ~((mask << shift) | mask));
    if (depth > 24) {
        shift -= 16;
        mask <<= 8;
        color =  ((color >> shift) & mask)
               | ((color & mask) << shift)
               | (color & ~((mask << shift) | mask));

        if (depth > 40) {
            shift -= 16;
            mask <<= 8;
            color =  ((color >> shift) & mask)
                   | ((color & mask) << shift)
                   | (color & ~((mask << shift) | mask));

            if (depth > 56) {
                shift -= 16;
                mask <<= 8;
                color =  ((color >> shift) & mask)
                       | ((color & mask) << shift)
                       | (color & ~((mask << shift) | mask));
            }
        }
    }

    return color;
}

#endif  /* !arch_is_big_endian */

/*
 * Update the retain_mask field to reflect the information in the
 * drawn_comps field. This is useful only if the device color model
 * is separable.
 */
private void
set_retain_mask(overprint_device_t * opdev)
{
    int             i, ncomps = opdev->color_info.num_components;
    gx_color_index  drawn_comps = opdev->drawn_comps, retain_mask = 0;
#if !arch_is_big_endian
    int             depth = opdev->color_info.depth;
#endif

    for (i = 0; i < ncomps; i++, drawn_comps >>= 1) {
        if ((drawn_comps & 0x1) == 0)
            retain_mask |= opdev->color_info.comp_mask[i];
    }
#if !arch_is_big_endian
    if (depth > 8)
        retain_mask = swap_color_index(depth, retain_mask);
#endif
    opdev->retain_mask = retain_mask;
}

/* enlarge mask of non-zero components */
private gx_color_index
check_drawn_comps(int ncomps, frac cvals[GX_DEVICE_COLOR_MAX_COMPONENTS])
{
    int              i;
    gx_color_index   mask = 0x1, drawn_comps = 0;

    for (i = 0; i < ncomps; i++, mask <<= 1) {
        if (cvals[i] != frac_0)
            drawn_comps |= mask;
    }
    return drawn_comps;
}

/*
 * Update the overprint-specific device parameters.
 *
 * If spot colors are to be retain, the set of process (non-spot) colors is
 * determined by mapping through the standard color spaces and check which
 * components assume non-zero values.
 */
private int
update_overprint_params(
    overprint_device_t *            opdev,
    const gs_overprint_params_t *   pparams )
{
    int                             ncomps = opdev->color_info.num_components;

    /* check if overprint is to be turned off */
    if (!pparams->retain_any_comps) {
        /* if fill_rectangle forwards, overprint is already off */
        if (dev_proc(opdev, fill_rectangle) != gx_forward_fill_rectangle)
            memcpy( &opdev->procs,
                    &no_overprint_procs,
                    sizeof(no_overprint_procs) );
        return 0;
    }

    /* set the procedures according to the color model */
    if (opdev->color_info.separable_and_linear == GX_CINFO_SEP_LIN)
        memcpy( &opdev->procs,
                &sep_overprint_procs,
                sizeof(sep_overprint_procs) );
    else
        memcpy( &opdev->procs,
                &generic_overprint_procs,
                sizeof(generic_overprint_procs) );

    /* see if we need to determine the spot color components */
    if (!pparams->retain_spot_comps)
        opdev->drawn_comps = pparams->drawn_comps;
    else {
        gx_device *                     dev = (gx_device *)opdev;
        const gx_cm_color_map_procs *   pprocs;
        frac                            cvals[GX_DEVICE_COLOR_MAX_COMPONENTS];
        gx_color_index                  drawn_comps = 0;
        static const frac               frac_13 = float2frac(1.0 / 3.0);

        if ((pprocs = dev_proc(opdev, get_color_mapping_procs)(dev)) == 0 ||
            pprocs->map_gray == 0                                         ||
            pprocs->map_rgb == 0                                          ||
            pprocs->map_cmyk == 0                                           )
            return_error(gs_error_unknownerror);

        pprocs->map_gray(dev, frac_13, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);

        pprocs->map_rgb(dev, 0, frac_13, frac_0, frac_0, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);
        pprocs->map_rgb(dev, 0, frac_0, frac_13, frac_0, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);
        pprocs->map_rgb(dev, 0, frac_0, frac_0, frac_13, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);

        pprocs->map_cmyk(dev, frac_13, frac_0, frac_0, frac_0, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);
        pprocs->map_cmyk(dev, frac_0, frac_13, frac_0, frac_0, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);
        pprocs->map_cmyk(dev, frac_0, frac_0, frac_13, frac_0, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);
        pprocs->map_cmyk(dev, frac_0, frac_0, frac_0, frac_13, cvals);
        drawn_comps |= check_drawn_comps(ncomps, cvals);

        opdev->drawn_comps = drawn_comps;
    }

    /* check for degenerate case */
    if (opdev->drawn_comps == ((gx_color_index)1 << ncomps) - 1) {
        memcpy( &opdev->procs,
                &no_overprint_procs,
                sizeof(no_overprint_procs) );
        return 0;
    }

    /* if appropriate, update the retain_mask field */
    if (opdev->color_info.separable_and_linear == GX_CINFO_SEP_LIN)
        set_retain_mask(opdev);

    return 0;
}


/*
 * The open_device method for the overprint device is about as close to
 * a pure "forwarding" open_device operation as is possible. Its only
 * significant function is to ensure that the is_open field of the
 * overprint device matches that of the target device.
 *
 * We assume this procedure is called only if the device is not already
 * open, and that gs_opendevice will take care of the is_open flag.
 */
private int
overprint_open_device(gx_device * dev)
{
    overprint_device_t *    opdev = (overprint_device_t *)dev;
    gx_device *             tdev = opdev->target;
    int                     code = 0;

    /* the overprint device must have a target */
    if (tdev == 0)
        return_error(gs_error_unknownerror);
    if ((code = gs_opendevice(tdev)) >= 0)
        gx_device_copy_params(dev, tdev);
    return code;
}

/*
 * The put_params method for the overprint device will check if the
 * target device has closed and, if so, close itself.
 */
private int
overprint_put_params(gx_device * dev, gs_param_list * plist)
{
    overprint_device_t *    opdev = (overprint_device_t *)dev;
    gx_device *             tdev = opdev->target;
    int                     code = 0;


    if (tdev != 0 && (code = dev_proc(tdev, put_params)(tdev, plist)) >= 0) {
        gx_device_decache_colors(dev);
        if (!tdev->is_open)
            code = gs_closedevice(dev);
    }
    return code;
}

/*
 * The overprint device must never be confused with a page device.
 * Thus, we always forward the request for the page device to the
 * target, as should all forwarding devices.
 */
private gx_device *
overprint_get_page_device(gx_device * dev)
{
    overprint_device_t *    opdev = (overprint_device_t *)dev;
    gx_device *             tdev = opdev->target;

    return tdev == 0 ? 0 : dev_proc(tdev, get_page_device)(tdev);
}

/*
 * Calling create_compositor on the overprint device just updates the
 * overprint parameters; no new device is created.
 */
private int
overprint_create_compositor(
    gx_device *             dev,
    gx_device **            pcdev,
    const gs_composite_t *  pct,
    gs_imager_state *	    pis,
    gs_memory_t *           memory )
{
    if (pct->type != &gs_composite_overprint_type)
        return gx_default_create_compositor(dev, pcdev, pct, pis, memory);
    else {
        int     code;

        /* device must already exist, so just update the parameters */
        code = update_overprint_params(
                       (overprint_device_t *)dev,
                       &((const gs_overprint_t *)pct)->params );
        if (code >= 0)
            *pcdev = dev;
        return code;
    }
}


/*
 * The two rectangle-filling routines (which do the actual work) are just
 * stubbs for the time being. The actual routines would allocate a buffer,
 * use get_bits_rectangle to build a buffer of the existing data, modify
 * the appropriate components, then invoke the copy_color procedure on the
 * target device.
 */
private int
overprint_generic_fill_rectangle(
    gx_device *     dev,
    int             x,
    int             y,
    int             width,
    int             height,
    gx_color_index  color )
{
    overprint_device_t *    opdev = (overprint_device_t *)dev;
    gx_device *             tdev = opdev->target;

    if (tdev == 0)
        return 0;
    else
        return gx_overprint_generic_fill_rectangle( tdev,
                                                    opdev->drawn_comps,
                                                    x, y, width, height,
                                                    color,
                                                    dev->memory );
}

private int
overprint_sep_fill_rectangle(
    gx_device *     dev,
    int             x,
    int             y,
    int             width,
    int             height,
    gx_color_index  color )
{
    overprint_device_t *    opdev = (overprint_device_t *)dev;
    gx_device *             tdev = opdev->target;

    if (tdev == 0)
        return 0;
    else {
        int     depth = tdev->color_info.depth;

        /*
         * Swap the color index into the order required by a byte-oriented
         * bitmap. This is required only for littl-endian processors, and
         * then only if the depth > 8.
         */
#if !arch_is_big_endian
        if (depth > 8)
            color = swap_color_index(depth, color);
#endif

        /*
         * We can handle rectangle filling via bits_fill_rectangle_masked
         * if the depth is a divisor of 8 * sizeof(mono_fill_chunk). The
         * non-masked fill_rectangle code uses a byte-oriented routine
         * if depth > 8, but there is not much advantage to doing so if
         * masking is required.
         *
         * Directly testing (8 * sizeof(mono_fill_chunk)) % depth is
         * potentially expensive, since many rectangles are small. We
         * can avoid the modulus operation by noting that
         * 8 * sizeof(mono_fill_chunk) will be a power of 2, and so
         * we need only check that depth is a power of 2 and
         * depth < 8 * sizeof(mono_fill_chunk).
         */
        if ( depth <= 8 * sizeof(mono_fill_chunk) &&
             (depth & (depth - 1)) == 0             )
            return gx_overprint_sep_fill_rectangle_1( tdev,
                                                      opdev->retain_mask,
                                                      x, y, width, height,
                                                      color,
                                                      dev->memory );
        else
            return gx_overprint_sep_fill_rectangle_2( tdev,
                                                      opdev->retain_mask,
                                                      x, y, width, height,
                                                      color,
                                                      dev->memory );
    }
}


/* complete a porcedure set */
private void
fill_in_procs(gx_device_procs * pprocs)
{
    gx_device_forward   tmpdev;

    /*
     * gx_device_forward_fill_in_procs calls gx_device_fill_in_procs, which
     * requires the color_info field of the device be set to "reasonable"
     * values. Which values is irrelevant in this case, but they must not
     * contain dangling pointers, excessive numbers of components, etc.
     */
    memcpy( &tmpdev.color_info,
            &gs_overprint_device.color_info,
            sizeof(tmpdev.color_info) );
    /*
     * Prevent the check_device_separable routine from executing while we
     * fill in the procs.  Our tmpdev is not complete enough for it.
     */
    tmpdev.color_info.separable_and_linear = GX_CINFO_SEP_LIN_NONE;
    tmpdev.static_procs = 0;
    memcpy(&tmpdev.procs, pprocs, sizeof(tmpdev.procs));
    gx_device_forward_fill_in_procs(&tmpdev);
    memcpy(pprocs, &tmpdev.procs, sizeof(tmpdev.procs));
}

/*
 * Create an overprint compositor.
 *
 * Note that this routine will be called only if the device is not already
 * an overprint compositor. Hence, if pct->params.retain_any_comps is
 * false, we can just return.
 *
 * We also suppress use of overprint if the current device color model has only
 * a single component. In this case overprint mode is inapplicable (it applies
 * only to CMYK devices), and nothing can possibly be gained by using overprint.
 * More significantly, this cause avoids erroneous use of overprint when a
 * mask caching device is the current device, which would otherwise require
 * elaborate special handling in the caching device create_compositor
 * procedure.
 */
private int
c_overprint_create_default_compositor(
    const gs_composite_t *  pct,
    gx_device **            popdev,
    gx_device *             tdev,
    gs_imager_state *	    pis,
    gs_memory_t *           mem )
{
    const gs_overprint_t *  ovrpct = (const gs_overprint_t *)pct;
    overprint_device_t *    opdev = 0;

    /* see if there is anything to do */
    if ( !ovrpct->params.retain_any_comps) {
        *popdev = tdev;
        return 0;
    }

    /* check if the procedure arrays have been initialized */
    if (no_overprint_procs.get_xfont_procs == 0) {
        fill_in_procs(&no_overprint_procs);
        fill_in_procs(&generic_overprint_procs);
        fill_in_procs(&sep_overprint_procs);
    }

    /* build the overprint device */
    opdev = gs_alloc_struct_immovable( mem,
                                       overprint_device_t,
                                       &st_overprint_device_t,
                                       "create overprint compositor" );
    if ((*popdev = (gx_device *)opdev) == 0)
        return_error(gs_error_VMerror);
    gx_device_init( (gx_device *)opdev, 
                    (const gx_device *)&gs_overprint_device,
                    mem,
                    true );
    gx_device_copy_params((gx_device *)opdev, tdev);
    gx_device_set_target((gx_device_forward *)opdev, tdev);

    /* set up the overprint parameters */
    return update_overprint_params( opdev,
                                    &((const gs_overprint_t *)pct)->params );
}
