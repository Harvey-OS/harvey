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

/* $Id: gxoprect.c,v 1.6 2005/06/20 08:59:23 igor Exp $ */
/* generic (very slow) overprint fill rectangle implementation */

#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"             /* for gs_next_ids */
#include "gxdevice.h"
#include "gsdevice.h"
#include "gxgetbit.h"
#include "gxoprect.h"
#include "gsbitops.h"


/*
 * Unpack a scanline for a depth < 8. In this case we know the depth is
 * divisor of 8 and thus a power of 2, which implies that 8 / depth is
 * also a power of 2.
 */
private void
unpack_scanline_lt8(
    gx_color_index *    destp,
    const byte *        srcp,
    int                 src_offset,
    int                 width,
    int                 depth )
{
    byte                buff = 0;
    int                 i = 0, shift = 8 - depth, p_per_byte = 8 / depth;

    /* exit early if nothing to do */
    if (width == 0)
        return;

    /* skip over src_offset */
    if (src_offset >= p_per_byte) {
        srcp += src_offset / p_per_byte;
        src_offset &= (p_per_byte - 1);
    }
    if (src_offset > 0) {
        buff = *srcp++ << (src_offset * depth);
        i = src_offset;
        width += src_offset;
    }

    /* process the interesting part of the scanline */
    for (; i < width; i++, buff <<= depth) {
        if ((i & (p_per_byte - 1)) == 0)
            buff = *srcp++;
        *destp++ = buff >> shift;
    }
}

/*
 * Pack a scanline for a depth of < 8. Note that data prior to dest_offset
 * and any data beyond the width must be left undisturbed.
 */
private void
pack_scanline_lt8(
    const gx_color_index *  srcp,
    byte *                  destp,
    int                     dest_offset,
    int                     width,
    int                     depth )
{
    byte                    buff = 0;
    int                     i = 0, p_per_byte = 8 / depth;

    /* exit early if nothing to do */
    if (width == 0)
        return;

    /* skip over dest_offset */
    if (dest_offset >= p_per_byte) {
        destp += dest_offset / p_per_byte;
        dest_offset &= (p_per_byte - 1);
    }
    if (dest_offset > 0) {
        buff = *destp++ >> (8 - dest_offset * depth);
        i = dest_offset;
        width += dest_offset;
    }

    /* process the interesting part of the scanline */
    for (; i < width; i++) {
        buff = (buff << depth) | *srcp++;
        if ((i & (p_per_byte - 1)) == p_per_byte - 1)
            *destp++ = buff;
    }
    if ((i &= (p_per_byte - 1)) != 0) {
        int     shift = depth * (p_per_byte - i);
        int     mask = (1 << shift) - 1;

        *destp = (*destp & mask) | (buff << shift);
    }
}

/*
 * Unpack a scanline for a depth >= 8. In this case, the depth must be
 * a multiple of 8.
 */
private void
unpack_scanline_ge8(
    gx_color_index *    destp,
    const byte *        srcp,
    int                 src_offset,
    int                 width,
    int                 depth )
{
    gx_color_index      buff = 0;
    int                 i, j, bytes_per_p = depth >> 3;

    /* skip over src_offset */
    srcp += src_offset * bytes_per_p;

    /* process the interesting part of the scanline */
    width *= bytes_per_p;
    for (i = 0, j = 0; i < width; i++) {
        buff = (buff << 8) | *srcp++;
        if (++j == bytes_per_p) {
            *destp++ = buff;
            buff = 0;
            j = 0;
        }
    }
}

/*
 * Pack a scanline for depth >= 8.
 */
private void
pack_scanline_ge8(
    const gx_color_index *  srcp,
    byte *                  destp,
    int                     dest_offset,
    int                     width,
    int                     depth )
{
    gx_color_index          buff = 0;
    int                     i, j, bytes_per_p = depth >> 3;
    int                     shift = depth - 8;

    /* skip over dest_offset */
    destp += dest_offset;

    /* process the interesting part of the scanline */
    width *= bytes_per_p;
    for (i = 0, j = bytes_per_p - 1; i < width; i++, buff <<= 8) {
        if (++j == bytes_per_p) {
            buff = *srcp++;
            j = 0;
        }
        *destp++ = buff >> shift;
    }
}


/*
 * Perform the fill rectangle operation for a non-separable color encoding
 * that requires overprint support. This situation requires that colors be
 * decoded, modified, and re-encoded. These steps must be performed per
 * output pixel, so there is no hope of achieving good performance.
 * Consequently, only minimal performance optimizations are applied below.
 *
 * The overprint device structure is known only in gsovr.c, and thus is not
 * available here. The required information from the overprint device is,
 * therefore, provided via explicit operands.  The device operand points to
 * the target of the overprint compositor device, not the compositor device
 * itself. The drawn_comps bit array and the memory descriptor pointer are
 * also provided explicitly as operands.
 *
 * Returns 0 on success, < 0 in the event of an error.
 */
int
gx_overprint_generic_fill_rectangle(
    gx_device *             tdev,
    gx_color_index          drawn_comps,
    int                     x,
    int                     y,
    int                     w,
    int                     h,
    gx_color_index          color,
    gs_memory_t *           mem )
{
    gx_color_value          src_cvals[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index *        pcolor_buff = 0;
    byte *                  gb_buff = 0;
    gs_get_bits_params_t    gb_params;
    gs_int_rect             gb_rect;
    int                     depth = tdev->color_info.depth;
    int                     bit_x, start_x, end_x, raster, code;
    void                    (*unpack_proc)( gx_color_index *,
                                            const byte *,
                                            int, int, int );
    void                    (*pack_proc)( const gx_color_index *,
                                          byte *,
                                          int, int, int );

    fit_fill(tdev, x, y, w, h);
    bit_x = x * depth;
    start_x = bit_x & ~(8 * align_bitmap_mod - 1);
    end_x = bit_x + w * depth;

    /* select the appropriate pack/unpack routines */
    if (depth >= 8) {
        unpack_proc = unpack_scanline_ge8;
        pack_proc = pack_scanline_ge8;
    } else {
        unpack_proc = unpack_scanline_lt8;
        pack_proc = pack_scanline_lt8;
    }

    /* decode the source color */
    if ((code = dev_proc(tdev, decode_color)(tdev, color, src_cvals)) < 0)
        return code;

    /* allocate space for a scanline of color indices */
    pcolor_buff = (gx_color_index *)
                      gs_alloc_bytes( mem,
                                      w *  arch_sizeof_color_index,
                                      "overprint generic fill rectangle" );
    if (pcolor_buff == 0)
        return gs_note_error(gs_error_VMerror);

    /* allocate a buffer for the returned data */
    raster = bitmap_raster(end_x - start_x);
    gb_buff = gs_alloc_bytes(mem, raster, "overprint generic fill rectangle");
    if (gb_buff == 0) {
        gs_free_object( mem,
                        pcolor_buff,
                        "overprint generic fill rectangle" );
        return gs_note_error(gs_error_VMerror);
    }

    /*
     * Initialize the get_bits parameters. The selection of options is
     * based on the following logic:
     *
     *  - Overprint is only defined with respect to components of the
     *    process color model, so the retrieved information must be kept
     *    in that color model. The gx_bitmap_format_t bitfield regards
     *    this as the native color space.
     *
     *  - Overprinting and alpha compositing don't mix, so there is no
     *    reason to retrieve the alpha information.
     *
     *  - Data should be returned in the depth of the process color
     *    model. Though this depth could be specified explicitly, there
     *    is little reason to do so. 
     *
     *  - Though overprint is much more easily implemented with planar
     *    data, there is no planar version of the copy_color method to
     *    send the modified data back to device. Hence, we must retrieve
     *    data in chunky form.
     *
     *  - It is not possible to modify the raster data "in place", as
     *    doing so would bypass any other forwarding devices currently
     *    in the device "stack" (e.g.: a bounding box device). Hence,
     *    we must work with a copy of the data, which is passed to the
     *    copy_color method at the end of fill_rectangle operation.
     *
     *  - Though we only require data for those planes that will not be
     *    modified, there is no benefit to returning less than the full
     *    data for each pixel if the color encoding is not separable.
     *    Since this routine will be used only for encodings that are
     *    not separable, we might as well ask for full information.
     *
     *  - Though no particular alignment and offset are required, it is
     *    useful to make the copy operation as fast as possible. Ideally
     *    we would calculate an offset so that the data achieves optimal
     *    alignment. Alas, some of the devices work much more slowly if
     *    anything but GB_OFFSET_0 is specified, so that is what we use.
     */
    gb_params.options =  GB_COLORS_NATIVE
                       | GB_ALPHA_NONE
                       | GB_DEPTH_ALL
                       | GB_PACKING_CHUNKY
                       | GB_RETURN_COPY
                       | GB_ALIGN_STANDARD
                       | GB_OFFSET_0
                       | GB_RASTER_STANDARD;
    gb_params.x_offset = 0;     /* for consistency */
    gb_params.data[0] = gb_buff;
    gb_params.raster = raster;

    gb_rect.p.x = x;
    gb_rect.q.x = x + w;

    /* process each scanline separately */
    while (h-- > 0 && code >= 0) {
        gx_color_index *    cp = pcolor_buff;
        int                 i;

        gb_rect.p.y = y++;
        gb_rect.q.y = y;
        code = dev_proc(tdev, get_bits_rectangle)( tdev,
                                                   &gb_rect,
                                                   &gb_params,
                                                   0 );
        if (code < 0)
            break;
        unpack_proc(pcolor_buff, gb_buff, 0, w, depth);
        for (i = 0; i < w; i++, cp++) {
            gx_color_index  comps;
            int             j;
            gx_color_value  dest_cvals[GX_DEVICE_COLOR_MAX_COMPONENTS];
        
            if ((code = dev_proc(tdev, decode_color)(tdev, *cp, dest_cvals)) < 0)
                break;
            for (j = 0, comps = drawn_comps; comps != 0; ++j, comps >>= 1) {
                if ((comps & 0x1) != 0)
                    dest_cvals[j] = src_cvals[j];
            }
            *cp = dev_proc(tdev, encode_color)(tdev, dest_cvals);
        }
        pack_proc(pcolor_buff, gb_buff, 0, w, depth);
        code = dev_proc(tdev, copy_color)( tdev,
                                           gb_buff,
                                           0,
                                           raster,
                                           gs_no_bitmap_id,
                                           x, y - 1, w, 1 );
    }

    gs_free_object( mem,
                    gb_buff,
                    "overprint generic fill rectangle" );
    gs_free_object( mem,
                    pcolor_buff,
                    "overprint generic fill rectangle" );

    return code;
}



/*
 * Replication of 2 and 4 bit patterns to fill a mem_mono_chunk.
 */
private mono_fill_chunk fill_pat_2[4] = {
    mono_fill_make_pattern(0x00), mono_fill_make_pattern(0x55),
    mono_fill_make_pattern(0xaa), mono_fill_make_pattern(0xff)
};

private mono_fill_chunk fill_pat_4[16] = {
    mono_fill_make_pattern(0x00), mono_fill_make_pattern(0x11),
    mono_fill_make_pattern(0x22), mono_fill_make_pattern(0x33),
    mono_fill_make_pattern(0x44), mono_fill_make_pattern(0x55),
    mono_fill_make_pattern(0x66), mono_fill_make_pattern(0x77),
    mono_fill_make_pattern(0x88), mono_fill_make_pattern(0x99),
    mono_fill_make_pattern(0xaa), mono_fill_make_pattern(0xbb),
    mono_fill_make_pattern(0xcc), mono_fill_make_pattern(0xdd),
    mono_fill_make_pattern(0xee), mono_fill_make_pattern(0xff)
};

/*
 * Replicate a color or mask as required to fill a mem_mono_fill_chunk.
 * This is possible if (8 * sizeof(mono_fill_chunk)) % depth == 0.
 * Since sizeof(mono_fill_chunk) is a power of 2, this will be the case
 * if depth is a power of 2 and depth <= 8 * sizeof(mono_fill_chunk).
 */
private mono_fill_chunk
replicate_color(int depth, mono_fill_chunk color)
{
    switch (depth) {

      case 1:
        color = (mono_fill_chunk)(-(int)color); break;

      case 2:
        color = fill_pat_2[color]; break;

      case 4:
        color = fill_pat_4[color]; break;

      case 8:
        color= mono_fill_make_pattern(color); break;

#if mono_fill_chunk_bytes > 2
      case 16:
        color = (color << 16) | color;
        /* fall through */
#endif
#if mono_fill_chunk_bytes > 4
      case 32:
        color = (color << 32) | color;
        break;
#endif
    }

    return color;
}


/*
 * Perform the fill rectangle operation for a separable color encoding
 * that requires overprint support.
 *
 * This is handled via two separate cases. If
 *
 *    (8 * sizeof(mono_fill_chunk)) % tdev->color_info.depth = 0,
 *
 * then is possible to work via the masked analog of the bits_fill_rectangle
 * procedure, bits_fill_rectangle_masked. This requires that both the
 * color and component mask be replicated sufficiently to fill the
 * mono_fill_chunk. The somewhat elaborate set-up aside, the resulting
 * algorithm is about as efficient as can be achieved when using
 * get_bits_rectangle. More efficient algorithms require overprint to be
 * implemented in the target device itself.
 *
 * If the condition is not satisfied, a simple byte-wise algorithm is
 * used. This requires minimal setup but is not efficient, as it works in
 * units that are too small. More efficient methods are possible in this
 * case, but the required setup for a general depth is excessive (even
 * with the restriction that depth % 8 = 0). Hence, efficiency for these
 * cases is better addressed by direct implementation of overprint for
 * memory devices.
 *
 * For both cases, the color and retain_mask values passed to this
 * procedure are expected to be already swapped as required for a byte-
 * oriented bitmap. This consideration affects only little-endian
 * machines. For those machines, if depth > 9 the color passed to these
 * two procedures will not be the same as that passed to
 * gx_overprint_generic_fill_rectangle.
 *
 * Returns 0 on success, < 0 in the event of an error.
 */
int
gx_overprint_sep_fill_rectangle_1(
    gx_device *             tdev,
    gx_color_index          retain_mask,    /* already swapped */
    int                     x,
    int                     y,
    int                     w,
    int                     h,
    gx_color_index          color,          /* already swapped */
    gs_memory_t *           mem )
{
    byte *                  gb_buff = 0;
    gs_get_bits_params_t    gb_params;
    gs_int_rect             gb_rect;
    int                     code = 0, bit_w, depth = tdev->color_info.depth;
    int                     raster;
    mono_fill_chunk         rep_color, rep_mask;

    fit_fill(tdev, x, y, w, h);
    bit_w = w * depth;

    /* set up replicated color and retain mask */
    if (depth < 8 * sizeof(mono_fill_chunk)) {
        rep_color = replicate_color(depth, (mono_fill_chunk)color);
        rep_mask = replicate_color(depth, (mono_fill_chunk)retain_mask);
    } else {
        rep_color = (mono_fill_chunk)color;
        rep_mask = (mono_fill_chunk)retain_mask;
    }

    /* allocate a buffer for the returned data */
    raster = bitmap_raster(w * depth);
    gb_buff = gs_alloc_bytes(mem, raster, "overprint sep fill rectangle 1");
    if (gb_buff == 0)
        return gs_note_error(gs_error_VMerror);

    /*
     * Initialize the get_bits parameters. The selection of options is
     * the same as that for gx_overprint_generic_fill_rectangle (above).
     */
    gb_params.options =  GB_COLORS_NATIVE
                       | GB_ALPHA_NONE
                       | GB_DEPTH_ALL
                       | GB_PACKING_CHUNKY
                       | GB_RETURN_COPY
                       | GB_ALIGN_STANDARD
                       | GB_OFFSET_0
                       | GB_RASTER_STANDARD;
    gb_params.x_offset = 0;     /* for consistency */
    gb_params.data[0] = gb_buff;
    gb_params.raster = raster;

    gb_rect.p.x = x;
    gb_rect.q.x = x + w;

    /* process each scanline separately */
    while (h-- > 0 && code >= 0) {
        gb_rect.p.y = y++;
        gb_rect.q.y = y;
        code = dev_proc(tdev, get_bits_rectangle)( tdev,
                                                   &gb_rect,
                                                   &gb_params,
                                                   0 );
        if (code < 0)
            break;
        bits_fill_rectangle_masked( gb_buff,
                                    0,
                                    raster,
                                    rep_color,
                                    rep_mask,
                                    bit_w,
                                    1 );
        code = dev_proc(tdev, copy_color)( tdev,
                                           gb_buff,
                                           0,
                                           raster,
                                           gs_no_bitmap_id,
                                           x, y - 1, w, 1 );
    }

    gs_free_object( mem,
                    gb_buff,
                    "overprint generic fill rectangle" );

    return code;
}


int
gx_overprint_sep_fill_rectangle_2(
    gx_device *             tdev,
    gx_color_index          retain_mask,    /* already swapped */
    int                     x,
    int                     y,
    int                     w,
    int                     h,
    gx_color_index          color,          /* already swapped */
    gs_memory_t *           mem )
{
    byte *                  gb_buff = 0;
    gs_get_bits_params_t    gb_params;
    gs_int_rect             gb_rect;
    int                     code = 0, byte_w, raster;
    int                     byte_depth = tdev->color_info.depth >> 3;
    byte *                  pcolor;
    byte *                  pmask;

    fit_fill(tdev, x, y, w, h);
    byte_w = w * byte_depth;

    /* set up color and retain mask pointers */
    pcolor = (byte *)&color;
    pmask = (byte *)&retain_mask;
#if arch_is_big_endian
    pcolor += arch_sizeof_color_index - byte_depth;
    pmask += arch_sizeof_color_index - byte_depth;
#endif

    /* allocate a buffer for the returned data */
    raster = bitmap_raster(w * (byte_depth << 3));
    gb_buff = gs_alloc_bytes(mem, raster, "overprint sep fill rectangle 2");
    if (gb_buff == 0)
        return gs_note_error(gs_error_VMerror);

    /*
     * Initialize the get_bits parameters. The selection of options is
     * the same as that for gx_overprint_generic_fill_rectangle (above).
     */
    gb_params.options =  GB_COLORS_NATIVE
                       | GB_ALPHA_NONE
                       | GB_DEPTH_ALL
                       | GB_PACKING_CHUNKY
                       | GB_RETURN_COPY
                       | GB_ALIGN_STANDARD
                       | GB_OFFSET_0
                       | GB_RASTER_STANDARD;
    gb_params.x_offset = 0;     /* for consistency */
    gb_params.data[0] = gb_buff;
    gb_params.raster = raster;

    gb_rect.p.x = x;
    gb_rect.q.x = x + w;

    /* process each scanline separately */
    while (h-- > 0 && code >= 0) {
        int     i, j;
        byte *  cp = gb_buff;

        gb_rect.p.y = y++;
        gb_rect.q.y = y;
        code = dev_proc(tdev, get_bits_rectangle)( tdev,
                                                   &gb_rect,
                                                   &gb_params,
                                                   0 );
        if (code < 0)
            break;
        for (i = 0, j = 0; i < byte_w; i++, cp++) {
            *cp = (*cp & pmask[j]) | pcolor[j];
            if (++j == byte_depth)
                j = 0;
        }
        code = dev_proc(tdev, copy_color)( tdev,
                                           gb_buff,
                                           0,
                                           raster,
                                           gs_no_bitmap_id,
                                           x, y - 1, w, 1 );
    }

    gs_free_object( mem,
                    gb_buff,
                    "overprint generic fill rectangle" );

    return code;
}

