/* Copyright (C) 1993, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxcht.c,v 1.17 2005/08/31 03:41:24 dan Exp $ */
/* Color halftone rendering for Ghostscript imaging library */
#include <assert.h>
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"		/* for id generation */
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxdevice.h"
#include "gxcmap.h"
#include "gxdcolor.h"
#include "gxistate.h"
#include "gzht.h"
#include "gsserial.h"

/* Define whether to force use of the slow code, for testing. */
#define USE_SLOW_CODE 0

/* Define the size of the tile buffer allocated on the stack. */
#define tile_longs_LARGE 256
#define tile_longs_SMALL 64
#if arch_small_memory
#  define tile_longs_allocated tile_longs_SMALL
#  define tile_longs tile_longs_SMALL
#else
#  define tile_longs_allocated tile_longs_LARGE
#  ifdef DEBUG
#    define tile_longs\
       (gs_debug_c('.') ? tile_longs_SMALL : tile_longs_LARGE)
#  else
#    define tile_longs tile_longs_LARGE
#  endif
#endif

/* Define the colored halftone device color type. */
gs_private_st_ptrs1(st_dc_ht_colored, gx_device_color, "dc_ht_colored",
    dc_ht_colored_enum_ptrs, dc_ht_colored_reloc_ptrs, colors.colored.c_ht);
private dev_color_proc_save_dc(gx_dc_ht_colored_save_dc);
private dev_color_proc_get_dev_halftone(gx_dc_ht_colored_get_dev_halftone);
private dev_color_proc_load(gx_dc_ht_colored_load);
private dev_color_proc_fill_rectangle(gx_dc_ht_colored_fill_rectangle);
private dev_color_proc_equal(gx_dc_ht_colored_equal);
private dev_color_proc_write(gx_dc_ht_colored_write);
private dev_color_proc_read(gx_dc_ht_colored_read);
const gx_device_color_type_t gx_dc_type_data_ht_colored = {
    &st_dc_ht_colored,
    gx_dc_ht_colored_save_dc, gx_dc_ht_colored_get_dev_halftone,
    gx_dc_ht_get_phase,
    gx_dc_ht_colored_load, gx_dc_ht_colored_fill_rectangle,
    gx_dc_default_fill_masked, gx_dc_ht_colored_equal,
    gx_dc_ht_colored_write, gx_dc_ht_colored_read,
    gx_dc_ht_colored_get_nonzero_comps
};
#undef gx_dc_type_ht_colored
const gx_device_color_type_t *const gx_dc_type_ht_colored =
    &gx_dc_type_data_ht_colored;
#define gx_dc_type_ht_colored (&gx_dc_type_data_ht_colored)

/* save information about the operand device color */
private void
gx_dc_ht_colored_save_dc(const gx_device_color * pdevc,
			 gx_device_color_saved * psdc)
{
    psdc->type = pdevc->type;
    memcpy( psdc->colors.colored.c_base,
            pdevc->colors.colored.c_base,
            sizeof(psdc->colors.colored.c_base) );
    memcpy( psdc->colors.colored.c_level,
            pdevc->colors.colored.c_level,
            sizeof(psdc->colors.colored.c_base) );
    psdc->colors.colored.alpha = pdevc->colors.colored.alpha;
    psdc->phase = pdevc->phase;
}

/* get the halftone used for the operand device color */
private const gx_device_halftone *
gx_dc_ht_colored_get_dev_halftone(const gx_device_color * pdevc)
{
    return pdevc->colors.colored.c_ht;
}

/* Compare two colored halftones for equality. */
private bool
gx_dc_ht_colored_equal(const gx_device_color * pdevc1,
		       const gx_device_color * pdevc2)
{
    uint num_comp = pdevc1->colors.colored.num_components;

    if (pdevc2->type != pdevc1->type ||
	pdevc1->colors.colored.c_ht != pdevc2->colors.colored.c_ht ||
	pdevc1->colors.colored.alpha != pdevc2->colors.colored.alpha ||
	pdevc1->phase.x != pdevc2->phase.x ||
	pdevc1->phase.y != pdevc2->phase.y ||
        num_comp != pdevc2->colors.colored.num_components
	)
	return false;
    return
	!memcmp(pdevc1->colors.colored.c_base,
		pdevc2->colors.colored.c_base,
		num_comp * sizeof(pdevc1->colors.colored.c_base[0])) &&
	!memcmp(pdevc1->colors.colored.c_level,
		pdevc2->colors.colored.c_level,
		num_comp * sizeof(pdevc1->colors.colored.c_level[0]));
}


/*
 * Flags to indicate the pieces of a colored halftone that are included
 * in its string representation. The first byte of the string holds this
 * set of flags.
 *
 * The case alpha = gx_max_color_value is by far the most common, so
 * special treatment is provided for this case.
 *
 * The halftone is never transmitted as part of a device color, so there
 * is no flag for it.
 */
private const int   dc_ht_colored_has_base = 0x01;
private const int   dc_ht_colored_has_level = 0x02;
private const int   dc_ht_colored_has_alpha = 0x04;
private const int   dc_ht_colored_alpha_is_max = 0x08;

/*
 * Serialize a device color that uses a traditional colored halftone.
 *
 * The number of components of a device color must match that of the
 * process color model, so it is not transmitted.
 *
 * The most common situation in which this routine is used is for 1-bit
 * per component devices. In that case, base[i] will always be 0 or 1,
 * and thus may be fit in a single bit.
 *
 * In many situations, one or more of the color component intensity
 * levels will be 0. The plane_mask field identifies those components
 * where this is not the case. By tansmitting the plane_mask, only those
 * components with non-zero levels need be transmitted.
 *
 * The case alpha = gx_max_color_value is by far the most common, so
 * special treatment is provided for this case.
 *
 *
 * Operands:
 *
 *  pdevc       pointer to device color to be serialized
 *
 *  psdc        pointer ot saved version of last serialized color (for
 *              this band)
 *  
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to buffer in which to write the data
 *
 *  psize       pointer to a location that, on entry, contains the size of
 *              the buffer pointed to by pdata; on return, the size of
 *              the data required or actually used will be written here.
 *
 * Returns:
 *  1, with *psize set to 0, if *psdc and *pdevc represent the same color
 *
 *  0, with *psize set to the amount of data written, if everything OK
 *
 *  gs_error_rangecheck, with *psize set to the size of buffer required,
 *  if *psize was not large enough
 *
 *  < 0, != gs_error_rangecheck, in the event of some other error
 *  (currently none); in this case *psize is not changed.
 */
private int
gx_dc_ht_colored_write(
    const gx_device_color *         pdevc,
    const gx_device_color_saved *   psdc0,
    const gx_device *               dev,
    byte *                          pdata,
    uint *                          psize )
{
    int                             req_size = 1;   
    int                             flag_bits = 0;
    int                             num_comps = dev->color_info.num_components;
    int                             depth = dev->color_info.depth;
    gx_color_index                  plane_mask = pdevc->colors.colored.plane_mask;
    gx_color_value                  alpha = pdevc->colors.colored.alpha;
    const gx_device_color_saved *   psdc = psdc0;
    byte *                          pdata0 = pdata;

    /* sanity check */
    assert(pdevc->colors.colored.num_components == num_comps);

    /* check if saved color is of the same type */
    if (psdc != 0 && psdc->type != pdevc->type)
        psdc = 0;

    /* calculate the size required */
    if ( psdc == 0                                                         ||
         memcmp( pdevc->colors.colored.c_base,
                 psdc->colors.colored.c_base,
                 num_comps * sizeof(pdevc->colors.colored.c_base[0]) ) != 0  ) {
        flag_bits |= dc_ht_colored_has_base;
        if (num_comps == depth)     /* 1 bit / component */
            req_size += (num_comps + 7) >> 3;
        else
            req_size += num_comps * sizeof(pdevc->colors.colored.c_base[0]);
    }

    plane_mask = pdevc->colors.colored.plane_mask;
    if ( psdc == 0                                                          ||
         memcmp( pdevc->colors.colored.c_level,
                 psdc->colors.colored.c_level,
                 num_comps * sizeof(pdevc->colors.colored.c_level[0]) ) != 0  ) {
        gx_color_index  comp_bit;
        int             i;
        uint            tmp_mask;

        flag_bits |= dc_ht_colored_has_level;
        if (num_comps > 8 * sizeof(uint)) {
            tmp_mask = (uint)plane_mask;
            req_size += enc_u_sizew(tmp_mask);
            tmp_mask = (uint)(plane_mask >> (8 * sizeof(uint)));
            req_size += enc_u_sizew(tmp_mask);
        } else {
            tmp_mask = (uint)plane_mask;
            req_size += enc_u_sizew(tmp_mask);
        }
        for (i = 0, comp_bit = 0x1; i < num_comps; i++, comp_bit <<= 1) {
            if ((plane_mask & comp_bit) != 0)
                req_size += enc_u_sizew(pdevc->colors.colored.c_level[i]);
        }
    }

    if (psdc == 0 || alpha != psdc->colors.colored.alpha) {
        if (alpha == gx_max_color_value)
            flag_bits |= dc_ht_colored_alpha_is_max;
        else {
            flag_bits |= dc_ht_colored_has_alpha;
            req_size += enc_u_sizew(alpha);
        }
    }

    /* see if there is anything to do */
    if (flag_bits == 0) {
        *psize = 0;
        return 1;
    }

    /* see if enough space is available */
    if (req_size > *psize) {
        *psize = req_size;
        return gs_error_rangecheck;
    }

    /* write out the flag byte */
    *pdata++ = (byte)flag_bits;

    /* write out such other parts of the device color as required */
    if ((flag_bits & dc_ht_colored_has_base) != 0) {
        if (num_comps == depth) {
            gx_color_index  base_mask = 0;
            int             num_bytes = (num_comps + 7) >> 3;
            int             i;

            for (i = 0; i < num_comps; i++) {
                if (pdevc->colors.colored.c_base[i] != 0)
                    base_mask |= (gx_color_index)1 << i;
            }
            for (i = 0; i < num_bytes; i++, base_mask >>= 8)
                *pdata++ = (byte)base_mask;
        } else {
            memcpy( pdata,
                    pdevc->colors.colored.c_base,
                    num_comps * sizeof(pdevc->colors.colored.c_base[0]) );
            pdata += num_comps * sizeof(pdevc->colors.colored.c_base[0]);
        }
    }

    if ((flag_bits & dc_ht_colored_has_level) != 0) {
        gx_color_index  code_bit;
        int             i;
        uint            tmp_mask;

        if (num_comps > 8 * sizeof(uint)) {
            tmp_mask = (uint)plane_mask;
            enc_u_putw(tmp_mask, pdata);
            tmp_mask = (uint)(plane_mask >> (8 * sizeof(uint)));
            enc_u_putw(tmp_mask, pdata);
        } else {
            tmp_mask = (uint)plane_mask;
            enc_u_putw(tmp_mask, pdata);
        }
        for (i = 0, code_bit = 0x1; i < num_comps; i++, code_bit <<= 1) {
            if ((plane_mask & code_bit) != 0)
                enc_u_putw(pdevc->colors.colored.c_level[i], pdata);
        }
    }

    if ((flag_bits & dc_ht_colored_has_alpha) != 0)
        enc_u_putw(alpha, pdata);

    *psize = pdata - pdata0;
    return 0;
}

/*
 * Reconstruct a device color from its serial representation.
 *
 * Operands:
 *
 *  pdevc       pointer to the location in which to write the
 *              reconstructed device color
 *
 *  pis         pointer to the current imager state (to access the
 *              current halftone)
 *
 *  prior_devc  pointer to the current device color (this is provided
 *              separately because the device color is not part of the
 *              imager state)
 *
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to the buffer to be read
 *
 *  size        size of the buffer to be read; this should be large
 *              enough to hold the entire color description
 *
 *  mem         pointer to the memory to be used for allocations
 *              (ignored here)
 *
 * Returns:
 *
 *  # of bytes read if everthing OK, < 0 in the event of an error
 */
private int
gx_dc_ht_colored_read(
    gx_device_color *       pdevc,
    const gs_imager_state * pis,
    const gx_device_color * prior_devc,
    const gx_device *       dev,
    const byte *            pdata,
    uint                    size,
    gs_memory_t *           mem )       /* ignored */
{
    gx_device_color         devc;
    int                     num_comps = dev->color_info.num_components;
    int                     depth = dev->color_info.depth;
    const byte *            pdata0 = pdata;
    int                     flag_bits;

    /* if prior information is available, use it */
    if (prior_devc != 0 && prior_devc->type == gx_dc_type_ht_colored)
        devc = *prior_devc;
    else
        memset(&devc, 0, sizeof(devc));   /* clear pointers */
    devc.type = gx_dc_type_ht_colored;

    /* the number of components is determined by the color model */
    devc.colors.colored.num_components = num_comps;
    devc.colors.colored.c_ht = pis->dev_ht;

    /*
     * Verify that we have at least the flag bits. For performance
     * reasons, the routines that convert serialized representations
     * of integers do not check buffer size. Hence, in many cases below,
     * only a very rough check is made to verify that we have not
     * exhausted the buffer. This should not cause a problem in
     * practice.
     */
    if (size == 0)
        return_error(gs_error_rangecheck);
    size--;
    flag_bits = *pdata++;

    /* read the other components provided */
    if ((flag_bits & dc_ht_colored_has_base) != 0) {
        if (depth == num_comps) {
            gx_color_index  base_mask = 0;
            int             num_bytes = (num_comps + 7) >> 3;
            int             i, shift = 0;

            if (size < num_bytes)
                return_error(gs_error_rangecheck);
	    size -= num_bytes;
            for (i = 0; i < num_bytes; i++, shift += 8)
                base_mask |= (gx_color_index)(*pdata++) << shift;
            for (i = 0; i < num_comps; i++, base_mask >>= 1)
                devc.colors.colored.c_base[i] = base_mask & 0x1;
        } else {
            if (size < num_comps)
                return_error(gs_error_rangecheck);
	    size -= num_comps;
            memcpy(devc.colors.colored.c_base, pdata, num_comps);
            pdata += num_comps;
        }
    }

    if ((flag_bits & dc_ht_colored_has_level) != 0) {
        const byte *    pdata_start = pdata;
        gx_color_index  plane_mask;
        uint            tmp_mask;
        int             i;

        if (size < 1)
            return_error(gs_error_rangecheck);

        if (num_comps > 8 * sizeof(uint)) {
            enc_u_getw(tmp_mask, pdata);
            plane_mask = (gx_color_index)tmp_mask;
            enc_u_getw(tmp_mask, pdata);
            plane_mask = (gx_color_index)tmp_mask << (8 * sizeof(uint));
        } else {
            enc_u_getw(tmp_mask, pdata);
            plane_mask = (gx_color_index)tmp_mask;
        }
        devc.colors.colored.plane_mask = plane_mask;
        for (i = 0; i < num_comps; i++, plane_mask >>= 1) {
            if ((plane_mask & 0x1) != 0) {
                if (size - (pdata - pdata_start) < 1)
                    return_error(gs_error_rangecheck);
                enc_u_getw(devc.colors.colored.c_level[i], pdata);
            } else
                devc.colors.colored.c_level[i] = 0;
        }
        size -= pdata - pdata_start;
    }

    if ((flag_bits & dc_ht_colored_alpha_is_max) != 0)
        devc.colors.colored.alpha = gx_max_color_value;
    else if ((flag_bits & dc_ht_colored_has_alpha) != 0) {
        const byte *    pdata_start = pdata;

        if (size < 1)
            return_error(gs_error_rangecheck);
        enc_u_getw(devc.colors.colored.alpha, pdata);
        size -= pdata - pdata_start;
    }

    /* set the phase as required (select value is arbitrary) */
    color_set_phase_mod( &devc,
                         pis->screen_phase[0].x,
                         pis->screen_phase[0].y,
                         pis->dev_ht->lcm_width,
                         pis->dev_ht->lcm_height );

    /* everything looks OK */
    *pdevc = devc;
    return pdata - pdata0;
}


/*
 * Get the nonzero components of a coloredhalftone. This is used to
 * distinguish components that are given zero intensity due to halftoning
 * from those for which the original color intensity was in fact zero.
 *
 * An original component intensity of zero will yield a c_base value of
 * 0 and a c_level of 0. The plane_mask field already contains the latter
 * information, so we need only add those components for which c_base is
 * non-zero.
 */
int
gx_dc_ht_colored_get_nonzero_comps(
    const gx_device_color * pdevc,
    const gx_device *       dev_ignored,
    gx_color_index *        pcomp_bits )
{
    int                     i, ncomps =  pdevc->colors.colored.num_components;
    gx_color_index          comp_bits = pdevc->colors.colored.plane_mask;

    for (i = 0; i < ncomps; i++) {
        if (pdevc->colors.colored.c_base[i] != 0)
            comp_bits |= ((gx_color_index)1) << i;
    }
    *pcomp_bits = comp_bits;

    return 0;
}

/*
 * Define an abbreviation for a heavily used value: the maximum number of
 * of device colorants (device colors).
 */
#define MAX_DCC GX_DEVICE_COLOR_MAX_COMPONENTS
/*
 * Define a size for the "colors" array.  For less than 5 colors, there are
 * 2**n values stored (for a maximum of 16 values).  For 5 or more colors, we
 * only store 2 values per color so the array size can be 2 * MAX_DCC.  Use which
 * ever is larger for the array size.
 */
#define MAX_DCC_16 (2 * MAX_DCC < 16 ? 16 : 2 * MAX_DCC)

/* Forward references. */
/* Use a typedef to attempt to work around overly picky compilers. */
typedef gx_color_value gx_color_value_array[MAX_DCC];
typedef struct color_values_pair_s {
    gx_color_value_array values[2];
} color_values_pair_t;
#define SET_HT_COLORS_PROC(proc)\
  int proc(\
	      color_values_pair_t *pvp,\
	      gx_color_index colors[MAX_DCC_16],\
	      const gx_const_strip_bitmap *sbits[MAX_DCC],\
	      const gx_device_color *pdevc,\
	      gx_device *dev,\
	      gx_ht_cache *caches[MAX_DCC],\
	      int nplanes\
	      )

private SET_HT_COLORS_PROC(set_ht_colors_le_4);
private SET_HT_COLORS_PROC(set_cmyk_1bit_colors);
private SET_HT_COLORS_PROC(set_ht_colors_gt_4);

#define SET_COLOR_HT_PROC(proc)\
  void proc(\
	        byte *dest_data, /* the output tile */\
		uint dest_raster, /* ibid. */\
		int px,	/* the initial phase of the output tile */\
		int py,\
		int w,	/* how much of the tile to set */\
		int h,\
		int depth,	/* depth of tile (4, 8, 16, 24, 32) */\
		int special,	/* >0 means special 1-bit CMYK */\
		int nplanes,\
		gx_color_index plane_mask,	/* which planes are halftoned */\
		gx_device *dev,		/* in case we are mapping lazily */\
		const color_values_pair_t *pvp,	/* color values ditto */\
		gx_color_index colors[MAX_DCC],	/* the actual colors for the tile, */\
				/* actually [nplanes] */\
		const gx_const_strip_bitmap * sbits[MAX_DCC]	/* the bitmaps for the planes, */\
				/* actually [nplanes] */\
		)

private SET_COLOR_HT_PROC(set_color_ht_le_4);
private SET_COLOR_HT_PROC(set_color_ht_gt_4);

/* Prepare to use a colored halftone, by loading the default cache. */
private int
gx_dc_ht_colored_load(gx_device_color * pdevc, const gs_imager_state * pis,
		      gx_device * ignore_dev, gs_color_select_t select)
{
    /* TO_DO_DEVICEN */
 
    return 0;
}

/* Fill a rectangle with a colored halftone. */
/* Note that we treat this as "texture" for RasterOp. */
private int
gx_dc_ht_colored_fill_rectangle(const gx_device_color * pdevc,
				int x, int y, int w, int h,
				gx_device * dev, gs_logical_operation_t lop,
				const gx_rop_source_t * source)
{
    ulong tbits[tile_longs_allocated];
    const uint tile_bytes = tile_longs * size_of(long);
    gx_strip_bitmap tiles;
    gx_rop_source_t no_source;
    const gx_device_halftone *pdht = pdevc->colors.colored.c_ht;
    int depth = dev->color_info.depth;
    int nplanes = dev->color_info.num_components;

    SET_HT_COLORS_PROC((*set_ht_colors)) =
	(
#if USE_SLOW_CODE
	 set_ht_colors_gt_4
#else
	 (dev_proc(dev, map_cmyk_color) == gx_default_encode_color &&
	  dev->color_info.depth == 4) ?
	    set_cmyk_1bit_colors :
	    nplanes <= 4 ? set_ht_colors_le_4 :
			  set_ht_colors_gt_4
#endif
	 );
    SET_COLOR_HT_PROC((*set_color_ht)) =
	(
#if !USE_SLOW_CODE
	 !(pdevc->colors.colored.plane_mask & ~(gx_color_index)15) &&
	  set_ht_colors != set_ht_colors_gt_4 ?
	   set_color_ht_le_4 :
#endif
	 set_color_ht_gt_4);
    color_values_pair_t vp;
    gx_color_index colors[MAX_DCC_16];
    const gx_const_strip_bitmap *sbits[MAX_DCC];
    gx_ht_cache *caches[MAX_DCC];
    int special;
    int code = 0;
    int raster;
    uint size_x;
    int dw, dh;
    int lw = pdht->lcm_width, lh = pdht->lcm_height;
    bool no_rop;
    int i;

    if (w <= 0 || h <= 0)
	return 0;
    if ((w | h) >= 16) {
	/* It's worth taking the trouble to check the clipping box. */
	gs_fixed_rect cbox;
	int t;

	dev_proc(dev, get_clipping_box)(dev, &cbox);
	if ((t = fixed2int(cbox.p.x)) > x) {
	    if ((w += x - t) <= 0)
		return 0;
	    x = t;
	}
	if ((t = fixed2int(cbox.p.y)) > y) {
	    if ((h += y - t) <= 0)
		return 0;
	    y = t;
	}
	if ((t = fixed2int(cbox.q.x)) < x + w)
	    if ((w = t - x) <= 0)
		return 0;
	if ((t = fixed2int(cbox.q.y)) < y + h)
	    if ((h = t - y) <= 0)
		return 0;
    }
    /* Colored halftone patterns are unconditionally opaque. */
    lop &= ~lop_T_transparent;
    if (pdht->components == 0) {
	caches[0] = caches[1] = caches[2] = caches[3] = pdht->order.cache;
	for (i = 4; i < nplanes; ++i)
	    caches[i] = pdht->order.cache;
    } else {
	gx_ht_order_component *pocs = pdht->components;


	for (i = 0; i < nplanes; ++i)
	    caches[i] = pocs[i].corder.cache;
    }
    special = set_ht_colors(&vp, colors, sbits, pdevc, dev, caches, nplanes);
    no_rop = source == NULL && lop_no_S_is_T(lop);
    /*
     * If the LCM of the plane cell sizes is smaller than the rectangle
     * being filled, compute a single tile and let tile_rectangle do the
     * replication.
     */
    if ((w > lw || h > lh) &&
	(raster = bitmap_raster(lw * depth)) <= tile_bytes / lh
	) {
	/*
	 * The only reason we need to do fit_fill here is that if the
	 * device is a clipper, the caller might be counting on it to do
	 * all necessary clipping.  Actually, we should clip against the
	 * device's clipping box, not the default....
	 */
	fit_fill(dev, x, y, w, h);
	/* Check to make sure we still have a big rectangle. */
	if (w > lw || h > lh) {
	    tiles.data = (byte *)tbits;
	    tiles.raster = raster;
	    tiles.rep_width = tiles.size.x = lw;
	    tiles.rep_height = tiles.size.y = lh;
	    tiles.id = gs_next_ids(dev->memory, 1);
	    tiles.rep_shift = tiles.shift = 0;
	    set_color_ht((byte *)tbits, raster, 0, 0, lw, lh, depth,
			 special, nplanes, pdevc->colors.colored.plane_mask,
			 dev, &vp, colors, sbits);
	    if (no_rop)
		return (*dev_proc(dev, strip_tile_rectangle)) (dev, &tiles,
							       x, y, w, h,
				       gx_no_color_index, gx_no_color_index,
					    pdevc->phase.x, pdevc->phase.y);
	    if (source == NULL)
		set_rop_no_source(source, no_source, dev);
	    return (*dev_proc(dev, strip_copy_rop)) (dev, source->sdata,
			       source->sourcex, source->sraster, source->id,
			     (source->use_scolors ? source->scolors : NULL),
						     &tiles, NULL,
						     x, y, w, h,
					     pdevc->phase.x, pdevc->phase.y,
                                                lop);
	}
    }
    size_x = w * depth;
    raster = bitmap_raster(size_x);
    if (raster > tile_bytes) {
	/*
	 * We can't even do an entire line at once.  See above for
	 * why we do the X equivalent of fit_fill here.
	 */
	if (x < 0)
	    w += x, x = 0;
	if (x > dev->width - w)
	    w = dev->width - x;
	if (w <= 0)
	    return 0;
	size_x = w * depth;
	raster = bitmap_raster(size_x);
	if (raster > tile_bytes) {
	    /* We'll have to do a partial line. */
	    dw = tile_bytes * 8 / depth;
	    size_x = dw * depth;
	    raster = bitmap_raster(size_x);
	    dh = 1;
	    goto fit;
	}
    }
    /* Do as many lines as will fit. */
    dw = w;
    dh = tile_bytes / raster;
    if (dh > h)
	dh = h;
fit:				/* Now the tile will definitely fit. */
    if (!no_rop) {
	tiles.data = (byte *)tbits;
	tiles.id = gx_no_bitmap_id;
	tiles.raster = raster;
	tiles.rep_width = tiles.size.x = size_x / depth;
	tiles.rep_shift = tiles.shift = 0;
    }
    while (w) {
	int cy = y, ch = dh, left = h;

	for (;;) {
	    set_color_ht((byte *)tbits, raster,
			 x + pdevc->phase.x, cy + pdevc->phase.y,
			 dw, ch, depth, special, nplanes,
			 pdevc->colors.colored.plane_mask,
			 dev, &vp, colors, sbits);
	    if (no_rop) {
		code = (*dev_proc(dev, copy_color))
		    (dev, (byte *)tbits, 0, raster, gx_no_bitmap_id,
		     x, cy, dw, ch);
	    } else {
		tiles.rep_height = tiles.size.y = ch;
                if (source == NULL)
                    set_rop_no_source(source, no_source, dev);
		/****** WRONG - MUST ADJUST source VALUES ******/
	        code = (*dev_proc(dev, strip_copy_rop))
		    (dev, source->sdata, source->sourcex, source->sraster,
		     source->id,
		     (source->use_scolors ? source->scolors : NULL),
		     &tiles, NULL, x, cy, dw, ch, 0, 0, lop);
	    }
	    if (code < 0)
		return code;
	    if (!(left -= ch))
		break;
	    cy += ch;
	    if (ch > left)
		ch = left;
	}
	if (!(w -= dw))
	    break;
	x += dw;
	if (dw > w)
	    dw = w;
    }
    return code;
}

/* ---------------- Color table setup ---------------- */

/*
 * We could cache this if we had a place to store it.  Even a 1-element
 * cache would help performance substantially.
 *   Key: device + c_base/c_level of device color
 *   Value: colors table
 */

/*
 * We construct color halftone tiles out of multiple "planes".
 * Each plane specifies halftoning for one component (R/G/B, C/M/Y/K,
 * or DeviceN components).
 */

private const struct {
    ulong pad;			/* to get bytes aligned properly */
    byte bytes[sizeof(ulong) * 8];	/* 8 is arbitrary */
} ht_no_bitmap_data = { 0 };
private const gx_const_strip_bitmap ht_no_bitmap = {
    &ht_no_bitmap_data.bytes[0], sizeof(ulong),
    {sizeof(ulong) * 8, sizeof(ht_no_bitmap_data.bytes) / sizeof(ulong)},
    gx_no_bitmap_id, 1, 1, 0, 0
};

/* Set the color value(s) and halftone mask for one plane. */

/* Free variables: pvp, pdc, sbits, max_color */
#define SET_PLANE_COLOR_CONSTANT(i)\
  BEGIN\
    pvp->values[1][i] = pvp->values[0][i] = \
	fractional_color(pdc->colors.colored.c_base[i], max_color);\
    sbits[i] = &ht_no_bitmap;\
  END

/* Free variables: pvp, pdc, sbits, caches, invert, max_color */
#define SET_PLANE_COLOR(i)\
  BEGIN\
    uint q = pdc->colors.colored.c_base[i];\
    uint r = pdc->colors.colored.c_level[i];\
\
    pvp->values[0][i] = fractional_color(q, max_color);\
    if (r == 0)\
	pvp->values[1][i] = pvp->values[0][i], sbits[i] = &ht_no_bitmap;\
    else if (!invert) {\
	pvp->values[1][i] = fractional_color(q + 1, max_color);\
	sbits[i] = (const gx_const_strip_bitmap *)\
	    &gx_render_ht(caches[i], r)->tiles;\
    } else {                                                        \
	const gx_device_halftone *pdht = pdc->colors.colored.c_ht;  \
	int nlevels =\
	    (pdht->components ?\
	     pdht->components[i].corder.num_levels :\
	     pdht->order.num_levels);\
\
	pvp->values[1][i] = pvp->values[0][i];                   \
	pvp->values[0][i] = fractional_color(q + 1, max_color);   \
	sbits[i] = (const gx_const_strip_bitmap *)\
	    &gx_render_ht(caches[i], nlevels - r)->tiles;    \
    }\
  END

/* Set up the colors and the individual plane halftone bitmaps. */
private int
set_ht_colors_le_4(color_values_pair_t *pvp /* only used internally */,
		   gx_color_index colors[MAX_DCC_16] /* 16 used */,
		   const gx_const_strip_bitmap * sbits[MAX_DCC],
		   const gx_device_color * pdc, gx_device * dev,
		   gx_ht_cache * caches[MAX_DCC], int nplanes)
{
    gx_color_value max_color = dev->color_info.dither_colors - 1;
    gx_color_value cvalues[4];
    /*
     * NB: the halftone orders are all set up for an additive color space.
     *     To make these work with a subtractive device space such as CMYK,
     *     it is necessary to invert both the color level and the color
     *     pair. Note that if the original color was provided an additive
     *     space, this will reverse (in an approximate sense) the color
     *     conversion performed to express the color in the device space.
     */
    bool invert = dev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE;

    SET_PLANE_COLOR(0);
    if (nplanes >= 2) {
        SET_PLANE_COLOR(1);
    }
    if (nplanes >= 3) {
        SET_PLANE_COLOR(2);
    }
    if (nplanes == 3) {
	gx_color_value alpha = pdc->colors.colored.alpha;

	if (alpha == gx_max_color_value) {
#define M(i)\
  cvalues[0] = pvp->values[(i) & 1][0];\
  cvalues[1] = pvp->values[((i) & 2) >> 1][1];\
  cvalues[2] = pvp->values[(i) >> 2][2];\
  colors[i] = dev_proc(dev, encode_color)(dev, cvalues);

	    M(0); M(1); M(2); M(3); M(4); M(5); M(6); M(7);
#undef M
	} else {
#define M(i)\
  colors[i] = dev_proc(dev, map_rgb_alpha_color)(dev, pvp->values[(i) & 1][0],\
				     pvp->values[((i) & 2) >> 1][1],\
				     pvp->values[(i) >> 2][2], alpha)
	    M(0); M(1); M(2); M(3); M(4); M(5); M(6); M(7);
#undef M
	}
    } else if (nplanes > 3){
	SET_PLANE_COLOR(3);
	if (nplanes > 4) {
	    /*
	     * Set colors for any planes beyond the 4th.  Since this code
	     * only handles the case of at most 4 active planes, we know
	     * that any further planes are constant.
	     */
	    /****** DOESN'T MAP COLORS RIGHT, DOESN'T HANDLE ALPHA ******/
	    int pi;

	    for (pi = 4; pi < nplanes; ++pi)
		SET_PLANE_COLOR_CONSTANT(pi);
	}
	/*
	 * For CMYK output, especially if the input was RGB, it's
	 * common for one or more of the components to be zero.
	 * Each zero component can cut the cost of color mapping in
	 * half, so it's worth doing a little checking here.
	 */
#define M(i)\
    cvalues[0] = pvp->values[(i) & 1][0];\
    cvalues[1] = pvp->values[((i) & 2) >> 1][1];\
    cvalues[2] = pvp->values[((i) & 4) >> 2][2];\
    cvalues[3] = pvp->values[(i) >> 3][3];\
    colors[i] = dev_proc(dev, encode_color)(dev, cvalues)

      /* We know that plane_mask <= 15. */
	switch ((int)pdc->colors.colored.plane_mask) {
	    case 15:
		M(15); M(14); M(13); M(12);
		M(11); M(10); M(9); M(8);
	    case 7:
		M(7); M(6); M(5); M(4);
c3:	    case 3:
		M(3); M(2);
c1:	    case 1:
		M(1);
		break;
	    case 14:
		M(14); M(12); M(10); M(8);
	    case 6:
		M(6); M(4);
c2:	    case 2:
		M(2);
		break;
	    case 13:
		M(13); M(12); M(9); M(8);
	    case 5:
		M(5); M(4);
		goto c1;
	    case 12:
		M(12); M(8);
	    case 4:
		M(4);
		break;
	    case 11:
		M(11); M(10); M(9); M(8);
		goto c3;
	    case 10:
		M(10); M(8);
		goto c2;
	    case 9:
		M(9); M(8);
		goto c1;
	    case 8:
		M(8);
		break;
	    case 0:;
	}
	M(0);

#undef M
    }
    return 0;
}

/* Set up colors using the standard 1-bit CMYK mapping. */
private int
set_cmyk_1bit_colors(color_values_pair_t *ignore_pvp,
		     gx_color_index colors[MAX_DCC_16] /*2 used*/,
		     const gx_const_strip_bitmap * sbits[MAX_DCC /*4 used*/],
		     const gx_device_color * pdc, gx_device * dev,
		     gx_ht_cache * caches[MAX_DCC /*4 used*/],
		     int nplanes /*4*/)
{
    const gx_device_halftone *pdht = pdc->colors.colored.c_ht;
    /*
     * By reversing the order of the planes, we make the pixel values
     * line up with the color indices.  Then instead of a lookup, we
     * can compute the pixels directly using a Boolean function.
     *
     * We compute each output bit
     *   out[i] = (in[i] & mask1) | (~in[i] & mask0)
     * We store the two masks in colors[0] and colors[1], since the
     * colors array is otherwise unused in this case.  We duplicate
     * the values in all the nibbles so we can do several pixels at a time.
     */
    bits32 mask0 = 0, mask1 = 0;
#define SET_PLANE_COLOR_CMYK(i, mask)\
  BEGIN\
    uint r = pdc->colors.colored.c_level[i];\
\
    if (r == 0) {\
	if (pdc->colors.colored.c_base[i])\
	    mask0 |= mask, mask1 |= mask;\
	sbits[3 - i] = &ht_no_bitmap;\
    } else {\
	int nlevels =\
	    (pdht->components ?\
	     pdht->components[i].corder.num_levels :\
	     pdht->order.num_levels);\
\
	mask0 |= mask;\
	sbits[3 - i] = (const gx_const_strip_bitmap *)\
	    &gx_render_ht(caches[i], nlevels - r)->tiles;\
    }\
  END
	 /* Suppress a compiler warning about signed/unsigned constants. */
    SET_PLANE_COLOR_CMYK(0, /*0x88888888*/ (bits32)~0x77777777);
    SET_PLANE_COLOR_CMYK(1, 0x44444444);
    SET_PLANE_COLOR_CMYK(2, 0x22222222);
    SET_PLANE_COLOR_CMYK(3, 0x11111111);

#undef SET_PLANE_COLOR_CMYK
    {
	gx_ht_cache *ctemp;

	ctemp = caches[0], caches[0] = caches[3], caches[3] = ctemp;
	ctemp = caches[1], caches[1] = caches[2], caches[2] = ctemp;
    }
    colors[0] = mask0;
    colors[1] = mask1;
    return 1;
}

/*
 * Set up colors for >4 planes.  In this case, we assume that the color
 * component values are "separable".  (That we can form a gx_color_index value
 * for a color by a bit wise or of the gx_color_index values of the individual
 * components.)
 */
private int
set_ht_colors_gt_4(color_values_pair_t *pvp,
		   gx_color_index colors[MAX_DCC_16 /* 2 * nplanes */],
		   const gx_const_strip_bitmap * sbits[MAX_DCC],
		   const gx_device_color * pdc, gx_device * dev,
		   gx_ht_cache * caches[MAX_DCC], int nplanes)
{
    gx_color_value max_color = dev->color_info.dither_colors - 1;
    bool invert = dev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE;
    gx_color_index plane_mask = pdc->colors.colored.plane_mask;
    int i;
    gx_color_value cv[MAX_DCC] = {0};

    /* Set the color values and halftone caches. */
    for (i = 0; i < nplanes; ++i)
	if ((plane_mask >> i) & 1)
	    SET_PLANE_COLOR(i);
	else
	    SET_PLANE_COLOR_CONSTANT(i);

    /*
     * Determine a gs_color_index value for each pair of component values.
     * We assume that an overall index value can be formed from the
     * bitwise or of each component.  We calculate a value for both
     * the high and low value of each component.  These are stored
     * in adjacent locations in 'colors'.
     */
    for (i = 0; i < nplanes; i++ ) {
	cv[i] = pvp->values[0][i];
	colors[2 * i] = dev_proc(dev, encode_color)(dev, cv);
	/* We only need both values for components being halftoned */
	if ((plane_mask >> i) & 1) {
	    cv[i] = pvp->values[1][i];
	    colors[2 * i + 1] = dev_proc(dev, encode_color)(dev, cv);
	}
	cv[i] = 0;
    }

    return 0;
}

/* ---------------- Color rendering ---------------- */

/* Define the bookkeeping structure for each plane of halftone rendering. */
typedef struct tile_cursor_s {
    int tile_shift;		/* X shift per copy of tile */
    int xoffset;
    int xshift;
    uint xbytes;
    int xbits;
    const byte *row;
    const byte *tdata;
    uint raster;
    const byte *data;
    int bit_shift;
} tile_cursor_t;

/*
 * Initialize one plane cursor, including setting up for the first row
 * (data and bit_shift).
 */
private void
init_tile_cursor(int i, tile_cursor_t *ptc, const gx_const_strip_bitmap *btile,
		 int endx, int lasty)
{
    int tw = btile->size.x;
    int bx = ((ptc->tile_shift = btile->shift) == 0 ? endx :
	      endx + lasty / btile->size.y * ptc->tile_shift) % tw;
    int by = lasty % btile->size.y;

    ptc->xoffset = bx >> 3;
    ptc->xshift = 8 - (bx & 7);
    ptc->xbytes = (tw - 1) >> 3;
    ptc->xbits = ((tw - 1) & 7) + 1;
    ptc->tdata = btile->data;
    ptc->raster = btile->raster;
    ptc->row = ptc->tdata + by * (int)ptc->raster;
    ptc->data = ptc->row + ptc->xoffset;
    ptc->bit_shift = ptc->xshift;
    if_debug6('h', "[h]plane %d: size=%d,%d shift=%d bx=%d by=%d\n",
	      i, tw, btile->size.y, btile->shift, bx, by);
}

/* Step a cursor to the next row. */
private void
wrap_shifted_cursor(tile_cursor_t *ptc, const gx_const_strip_bitmap *psbit)
{
    ptc->row += ptc->raster * (psbit->size.y - 1);
    if (ptc->tile_shift) {
	if ((ptc->xshift += ptc->tile_shift) >= 8) {
	    if ((ptc->xoffset -= ptc->xshift >> 3) < 0) {
		/* wrap around in X */
		int bx = (ptc->xoffset << 3) + 8 - (ptc->xshift & 7) +
		    psbit->size.x;

		ptc->xoffset = bx >> 3;
		ptc->xshift = 8 - (bx & 7);
	    } else
		ptc->xshift &= 7;
	}
    }
}
#define STEP_ROW(c, i)\
  BEGIN\
    if (c.row > c.tdata)\
      c.row -= c.raster;\
    else {	/* wrap around to end of tile */\
	wrap_shifted_cursor(&c, sbits[i]);\
    }\
    c.data = c.row + c.xoffset;\
    c.bit_shift = c.xshift;\
  END

/* Define a table for expanding 8x1 bits to 8x4. */
private const bits32 expand_8x1_to_8x4[256] = {
#define X16(c)\
  c+0, c+1, c+0x10, c+0x11, c+0x100, c+0x101, c+0x110, c+0x111,\
  c+0x1000, c+0x1001, c+0x1010, c+0x1011, c+0x1100, c+0x1101, c+0x1110, c+0x1111
    X16(0x00000000), X16(0x00010000), X16(0x00100000), X16(0x00110000),
    X16(0x01000000), X16(0x01010000), X16(0x01100000), X16(0x01110000),
    X16(0x10000000), X16(0x10010000), X16(0x10100000), X16(0x10110000),
    X16(0x11000000), X16(0x11010000), X16(0x11100000), X16(0x11110000)
#undef X16
};

/*
 * Render the combined halftone for nplanes <= 4.
 */
private void
set_color_ht_le_4(byte *dest_data, uint dest_raster, int px, int py,
		  int w, int h, int depth, int special, int nplanes,
		  gx_color_index plane_mask, gx_device *ignore_dev,
		  const color_values_pair_t *ignore_pvp,
		  gx_color_index colors[MAX_DCC_16],
		  const gx_const_strip_bitmap * sbits[MAX_DCC])
{
    /*
     * Note that the planes are specified in the order RGB or CMYK, but
     * the indices used for the internal colors array are BGR or KYMC,
     * except for the special 1-bit CMYK case.
     */
    int x, y;
    tile_cursor_t cursor[MAX_DCC];
    int dbytes = depth >> 3;
    byte *dest_row =
	dest_data + dest_raster * (h - 1) + (w * depth) / 8;

    if (special > 0) {
	/* Planes are in reverse order. */
	plane_mask =
	    "\000\010\004\014\002\012\006\016\001\011\005\015\003\013\007\017"[plane_mask];
    }
    if_debug6('h',
	      "[h]color_ht_le_4: x=%d y=%d w=%d h=%d plane_mask=0x%lu depth=%d\n",
	      px, py, w, h, (ulong)plane_mask, depth);

    /* Do one-time cursor initialization. */
    {
	int endx = w + px;
	int lasty = h - 1 + py;

	if (plane_mask & 1)
	    init_tile_cursor(0, &cursor[0], sbits[0], endx, lasty);
	if (plane_mask & 2)
	    init_tile_cursor(1, &cursor[1], sbits[1], endx, lasty);
	if (plane_mask & 4)
	    init_tile_cursor(2, &cursor[2], sbits[2], endx, lasty);
	if (plane_mask & 8)
	    init_tile_cursor(3, &cursor[3], sbits[3], endx, lasty);
    }

    /* Now compute the actual tile. */
    for (y = h; ; dest_row -= dest_raster) {
	byte *dest = dest_row;

	--y;
	for (x = w; x > 0;) {
	    bits32 indices;
	    int nx, i;
	    register uint bits;

/* Get the next byte's worth of bits.  Note that there may be */
/* excess bits set beyond the 8th. */
#define NEXT_BITS(c)\
  BEGIN\
    if (c.data > c.row) {\
      bits = ((c.data[-1] << 8) | *c.data) >> c.bit_shift;\
      c.data--;\
    } else {\
      bits = *c.data >> c.bit_shift;\
      c.data += c.xbytes;\
      if ((c.bit_shift -= c.xbits) < 0) {\
	bits |= *c.data << -c.bit_shift;\
	c.bit_shift += 8;\
      } else {\
	bits |= ((c.data[-1] << 8) | *c.data) >> c.bit_shift;\
	c.data--;\
      }\
    }\
  END
	    if (plane_mask & 1) {
		NEXT_BITS(cursor[0]);
		indices = expand_8x1_to_8x4[bits & 0xff];
	    } else
		indices = 0;
	    if (plane_mask & 2) {
		NEXT_BITS(cursor[1]);
		indices |= expand_8x1_to_8x4[bits & 0xff] << 1;
	    }
	    if (plane_mask & 4) {
		NEXT_BITS(cursor[2]);
		indices |= expand_8x1_to_8x4[bits & 0xff] << 2;
	    }
	    if (plane_mask & 8) {
		NEXT_BITS(cursor[3]);
		indices |= expand_8x1_to_8x4[bits & 0xff] << 3;
	    }
#undef NEXT_BITS
	    nx = min(x, 8);	/* 1 <= nx <= 8 */
	    x -= nx;
	    switch (dbytes) {
		case 0:	/* 4 */
		    if (special > 0) {
			/* Special 1-bit CMYK. */
			/* Compute all the pixels at once! */
			indices =
			    (indices & colors[1]) | (~indices & colors[0]);
			i = nx;
			if ((x + nx) & 1) {
			    /* First pixel is even nibble. */
			    *dest = (*dest & 0xf) +
				((indices & 0xf) << 4);
			    indices >>= 4;
			    --i;
			}
			/* Now 0 <= i <= 8. */
			for (; (i -= 2) >= 0; indices >>= 8)
			    *--dest = (byte)indices;
			/* Check for final odd nibble. */
			if (i & 1)
			    *--dest = indices & 0xf;
		    } else {
			/* Other 4-bit pixel */
			i = nx;
			if ((x + nx) & 1) {
			    /* First pixel is even nibble. */
			    *dest = (*dest & 0xf) +
				((byte)colors[indices & 0xf] << 4);
			    indices >>= 4;
			    --i;
			}
			/* Now 0 <= i <= 8. */
			for (; (i -= 2) >= 0; indices >>= 8)
			    *--dest =
				(byte)colors[indices & 0xf] +
				((byte)colors[(indices >> 4) & 0xf]
				 << 4);
			/* Check for final odd nibble. */
			if (i & 1)
			    *--dest = (byte)colors[indices & 0xf];
		    }
		    break;
		case 4:	/* 32 */
		    for (i = nx; --i >= 0; indices >>= 4) {
			bits32 tcolor = (bits32)colors[indices & 0xf];

			dest -= 4;
			dest[3] = (byte)tcolor;
			dest[2] = (byte)(tcolor >> 8);
			tcolor >>= 16;
			dest[1] = (byte)tcolor;
			dest[0] = (byte)(tcolor >> 8);
		    }
		    break;
		case 3:	/* 24 */
		    for (i = nx; --i >= 0; indices >>= 4) {
			bits32 tcolor = (bits32)colors[indices & 0xf];

			dest -= 3;
			dest[2] = (byte) tcolor;
			dest[1] = (byte)(tcolor >> 8);
			dest[0] = (byte)(tcolor >> 16);
		    }
		    break;
		case 2:	/* 16 */
		    for (i = nx; --i >= 0; indices >>= 4) {
			uint tcolor =
			    (uint)colors[indices & 0xf];

			dest -= 2;
			dest[1] = (byte)tcolor;
			dest[0] = (byte)(tcolor >> 8);
		    }
		    break;
		case 1:	/* 8 */
		    for (i = nx; --i >= 0; indices >>= 4)
			*--dest = (byte)colors[indices & 0xf];
		    break;
	    }
	}
	if (y == 0)
	    break;

	if (plane_mask & 1)
	    STEP_ROW(cursor[0], 0);
	if (plane_mask & 2)
	    STEP_ROW(cursor[1], 1);
	if (plane_mask & 4)
	    STEP_ROW(cursor[2], 2);
	if (plane_mask & 8)
	    STEP_ROW(cursor[3], 3);
    }
}

/*
 * Render the combined halftone for nplanes > 4.  This routine assumes
 * that we can form a gx_color_index value by the bitwise or index values
 * for each of the individual components.
 */
private void
set_color_ht_gt_4(byte *dest_data, uint dest_raster, int px, int py,
		  int w, int h, int depth, int special, int num_planes,
		  gx_color_index plane_mask, gx_device *dev,
		  const color_values_pair_t *pvp,
		  gx_color_index colors[MAX_DCC_16],
		  const gx_const_strip_bitmap * sbits[MAX_DCC])
{
    int x, y;
    tile_cursor_t cursor[MAX_DCC];
    int dbytes = depth >> 3;
    byte *dest_row =
	dest_data + dest_raster * (h - 1) + (w * depth) / 8;
    int pmin, pmax;
    gx_color_index base_color = 0;

    /* Compute the range of active planes. */
    if (plane_mask == 0)
	pmin = 0, pmax = -1;
    else {
	for (pmin = 0; !((plane_mask >> pmin) & 1); )
	    ++pmin;
	for (pmax = 0; (plane_mask >> pmax) > 1; )
	    ++pmax;
    }
    if_debug6('h',
	      "[h]color_ht_gt_4: x=%d y=%d w=%d h=%d plane_mask=0x%lu depth=%d\n",
	      px, py, w, h, (ulong)plane_mask, depth);

    /* Do one-time cursor initialization. */
    {
	int endx = w + px;
	int lasty = h - 1 + py;
	int i;

	for (i = pmin; i <= pmax; ++i)
	    if ((plane_mask >> i) & 1)
		init_tile_cursor(i, &cursor[i], sbits[i], endx, lasty);
    }

    /* Pre-load the color value for the non halftoning planes. */
    {
	int i;

	for (i = 0; i < num_planes; ++i)
	    if ((~plane_mask >> i) & 1)
		base_color |= colors[2 * i];
    }

    /* Now compute the actual tile. */
    for (y = h; ; dest_row -= dest_raster) {
	byte *dest = dest_row;
	int i;

	--y;
	for (x = w; x > 0;) {
	    gx_color_index tcolor = base_color;

	    for (i = pmin; i <= pmax; ++i)
		if ((plane_mask >> i) & 1) {
		    /* Get the next bit from an individual mask. */
		    tile_cursor_t *ptc = &cursor[i];
		    byte tile_bit;

b:		    if (ptc->bit_shift < 8)
			tile_bit = *ptc->data >> ptc->bit_shift++;
		    else if (ptc->data > ptc->row) {
			tile_bit = *--(ptc->data);
			ptc->bit_shift = 1;
		    } else {
			/* Wrap around. */
			ptc->data += ptc->xbytes;
			ptc->bit_shift = 8 - ptc->xbits;
			goto b;
		    }
		    tcolor |= colors[2 * i + (tile_bit & 1)];
		}
	    --x;
	    switch (dbytes) {
		case 0:	/* 4 -- might be 2, but we don't support this */
		    if (x & 1) { /* odd nibble */
			*--dest = (byte)tcolor;
		    } else {	/* even nibble */
			*dest = (*dest & 0xf) + ((byte)tcolor << 4);
		    }
		    break;
		case 4:	/* 32 */
		    dest[-4] = (byte)(tcolor >> 24);
		case 3:	/* 24 */
		    dest[-3] = (byte)(tcolor >> 16);
		case 2:	/* 16 */
		    dest[-2] = (byte)(tcolor >> 8);
		case 1:	/* 8 */
		    dest[-1] = (byte)tcolor;
		    dest -= dbytes;
		    break;
	    }
	}
	if (y == 0)
	    break;
	for (i = pmin; i <= pmax; ++i)
	    if ((plane_mask >> i) & 1)
		STEP_ROW(cursor[i], i);
    }
}
