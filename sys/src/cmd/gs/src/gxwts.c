/* Copyright (C) 2002 artofcode LLC.  All rights reserved.
  
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
/*$Id: gxwts.c,v 1.5 2002/11/02 07:31:45 raph Exp $ */
/* Rendering using Well Tempered Screening. */
#include "stdpre.h"
#include "memory_.h" /* for memcmp */
#include <stdlib.h> /* for malloc */
#include "gx.h"
#include "gxstate.h"
#include "gsht.h"
#include "math_.h"
#include "gserrors.h"
#include "gxdcolor.h"
#include "gxdevcli.h"
#include "gxdht.h"
#include "gxwts.h"

#define GXWTS_USE_DOUBLE

#ifndef UNIT_TEST
/* device color type for wts. */

/* todo: trace and relocate pointers */
gs_private_st_simple(st_dc_wts, gx_device_color, "dc_wts");
private dev_color_proc_save_dc(gx_dc_wts_save_dc);
private dev_color_proc_get_dev_halftone(gx_dc_wts_get_dev_halftone);
private dev_color_proc_load(gx_dc_wts_load);
private dev_color_proc_fill_rectangle(gx_dc_wts_fill_rectangle);
private dev_color_proc_equal(gx_dc_wts_equal);
private dev_color_proc_write(gx_dc_wts_write);
private dev_color_proc_read(gx_dc_wts_read);
private dev_color_proc_get_nonzero_comps(gx_dc_wts_get_nonzero_comps);
const gx_device_color_type_t gx_dc_type_data_wts = {
    &st_dc_wts,
    gx_dc_wts_save_dc, gx_dc_wts_get_dev_halftone,
    gx_dc_ht_get_phase,
    gx_dc_wts_load, gx_dc_wts_fill_rectangle,
    gx_dc_default_fill_masked, gx_dc_wts_equal,
    gx_dc_wts_write, gx_dc_wts_read,
    gx_dc_wts_get_nonzero_comps
};
#undef gx_dc_type_wts
const gx_device_color_type_t *const gx_dc_type_wts =
&gx_dc_type_data_wts;
#endif

/* Low-level implementation follows. */

/**
 * mul_shr_16: Multiply and shift right 16.
 * @a: 32-bit signed number.
 * @b: 32-bit signed number.
 *
 * Multiply @a and @b, then shift right 16 bits. Allow intermediate value
 * to overflow 32 bits.
 *
 * Return value: result.
 **/
#ifdef GXWTS_USE_DOUBLE
private int
mul_shr_16 (int a, int b)
{
  return (int)floor(((double) a) * ((double) b) * (1.0 / (1 << 16)));
}
#else
#error todo: supply mul_shr_16 based on 64 bit integer type
#endif

/* Implementation of wts_get_samples for rational cells. */
#if 0
private int
wts_get_samples_rat(const wts_screen_t *ws, int x, int y,
		    wts_screen_sample_t **samples, int *p_nsamples)
{
    int d = y / ws->cell_height;
    int r = y % ws->cell_height;
    int x_ix = ((d * ws->cell_shift) + x) % ws->cell_width;
    *p_nsamples = ws->cell_width - x_ix;
    *samples = ws->samples + x_ix + r * ws->cell_width;
    return 0;
}
#endif

/* Implementation of wts_get_samples for Screen J. */
private int
wts_get_samples_j(const wts_screen_t *ws, int x, int y,
		  wts_screen_sample_t **samples, int *p_nsamples)
{
    const wts_screen_j_t *wsj = (const wts_screen_j_t *)ws;
    /* int d = y / ws->cell_height; */
    int y_ix = y;
    int x_ix = x;
    double pad = (wsj->pa) * (1.0 / (1 << 16));
    double pbd = (wsj->pb) * (1.0 / (1 << 16));
    double afrac = x * pad;
    double bfrac = x * pbd;
    int acount = (int)floor(afrac);
    int bcount = (int)floor(bfrac);
    int ccount = mul_shr_16(y, wsj->pc);
    int dcount = mul_shr_16(y, wsj->pd);
    int nsamples;

    x_ix += acount * wsj->XA + bcount * wsj->XB +
	ccount * wsj->XC + dcount * wsj->XD;
    y_ix += acount * wsj->YA + bcount * wsj->YB +
	ccount * wsj->YC + dcount * wsj->YD;

    x_ix += (y_ix / ws->cell_height) * ws->cell_shift;
    x_ix %= ws->cell_width;
    y_ix %= ws->cell_height;

    nsamples = ws->cell_width - x_ix;
    if (floor (afrac + (nsamples - 1) * pad) > acount)
	nsamples = (int)ceil((acount + 1 - afrac) / pad);

    if (floor (bfrac + (nsamples - 1) * pbd) > bcount)
	nsamples = (int)ceil((bcount + 1 - bfrac) / pbd);
#if 0
    printf("get_samples: (%d, %d) -> (%d, %d) %d (cc=%d)\n",
	   x, y, x_ix, y_ix, nsamples, ccount);
#endif
    *p_nsamples = nsamples;
    *samples = ws->samples + x_ix + y_ix * ws->cell_width;
    return 0;
}

private int
wts_screen_h_offset(int x, double p1, int m1, int m2)
{
    /* todo: this is a linear search; constant time should be feasible */
    double running_p = 0;
    int width_so_far;
    int this_width;

    for (width_so_far = 0;; width_so_far += this_width) {
	running_p += p1;
	if (running_p >= 0.5) {
	    this_width = m1;
	    running_p -= 1;
	} else {
	    this_width = m2;
	}
	if (width_so_far + this_width > x)
	    break;
    }
    return x - width_so_far + (this_width == m1 ? 0 : m1);
}

/* Implementation of wts_get_samples for Screen H. */
private int
wts_get_samples_h(const wts_screen_t *ws, int x, int y,
		  wts_screen_sample_t **samples, int *p_nsamples)
{
    const wts_screen_h_t *wsh = (const wts_screen_h_t *)ws;
    int x_ix = wts_screen_h_offset(x, wsh->px,
				   wsh->x1, ws->cell_width - wsh->x1);
    int y_ix = wts_screen_h_offset(y, wsh->py,
				   wsh->y1, ws->cell_height - wsh->y1);
    *p_nsamples = (x_ix >= wsh->x1 ? ws->cell_width : wsh->x1) - x_ix;
    *samples = ws->samples + x_ix + y_ix * ws->cell_width;
    return 0;
}

/**
 * wts_get_samples: Get samples from Well Tempered Screening cell.
 * @ws: Well Tempered Screening cell.
 * @x: X coordinate of starting point.
 * @y: Y coordinate of starting point.
 * @samples: Where to store pointer to samples.
 * @p_nsamples: Where to store number of valid samples.
 *
 * Finds samples from the cell for use in halftoning. On success,
 * @p_nsamples is set to the number of valid samples, ie for 0 <= i <
 * nsamples, samples[i] is a valid sample for coordinate (x + i, y).
 * p_nsamples is guaranteed to at least 1. The samples in @samples
 * are valid for the lifetime of the cell, or until the next garbage
 * collection, whichever comes first.
 *
 * Todo: describe meaning of wts_screen_sample_t (particularly edge
 * cases).
 *
 * Note: may want to add a "cursor" to the api as an optimization. It
 * can wait, though.
 *
 * Return value: 0 on success.
 **/
int
wts_get_samples(const wts_screen_t *ws, int x, int y,
		wts_screen_sample_t **samples, int *p_nsamples)
{
    if (ws->type == WTS_SCREEN_J)
	return wts_get_samples_j(ws, x, y, samples, p_nsamples);
    if (ws->type == WTS_SCREEN_H)
	return wts_get_samples_h(ws, x, y, samples, p_nsamples);
    else
	return -1;
}

/* Device color methods follow. */

private void
gx_dc_wts_save_dc(const gx_device_color * pdevc, gx_device_color_saved * psdc)
{
    psdc->type = pdevc->type;
    memcpy( psdc->colors.wts.levels,
            pdevc->colors.wts.levels,
            sizeof(psdc->colors.wts.levels) );
    psdc->phase = pdevc->phase;
}

private const gx_device_halftone *
gx_dc_wts_get_dev_halftone(const gx_device_color * pdevc)
{
    return pdevc->colors.wts.w_ht;
}

private int
gx_dc_wts_load(gx_device_color *pdevc, const gs_imager_state * pis,
	       gx_device *ignore_dev, gs_color_select_t select)
{
    return 0;
}

/**
 * wts_draw: Draw a halftoned shade into a 1 bit deep buffer.
 * @ws: WTS screen.
 * @shade: Gray shade to draw.
 * @data: Destination buffer.
 * @data_raster: Rowstride for destination buffer.
 * @x, @y, @w, @h: coordinates of rectangle to draw.
 *
 * This is close to an implementation of the "draw" method for the
 * gx_ht_order class. Currently, only WTS screens implement this
 * method, and only WTS device colors invoke it. However, implementing
 * this for legacy order objects is probably a good idea, to improve
 * halftoning performance as the cell size scales up.
 *
 * However, it's not exactly an implementation of the "draw" method
 * for the gx_ht_order class because the "self" type would need to be
 * gx_ht_order. Currently, however, device colors don't hold a pointer
 * to the order object. Some amount of refactoring seems to be in
 * order.
 *
 * Return value: 0 on success.
 **/
private int
wts_draw(wts_screen_t *ws, wts_screen_sample_t shade,
	 byte *data, int data_raster,
	 int x, int y, int w, int h)
{
    int xo, yo;
    unsigned char *line_start = data;

    for (yo = 0; yo < h; yo++) {
	unsigned char *line_ptr = line_start;
	int mask = 0x80;
	unsigned char b = 0;
	int imax;

	for (xo = 0; xo < w; xo += imax) {
	    wts_screen_sample_t *samples;
	    int n_samples, i;

	    wts_get_samples(ws, x + xo, y + yo, &samples, &n_samples);
	    imax = min(w - xo, n_samples);
	    for (i = 0; i < imax; i++) {
		if (shade > samples[i])
		    b |= mask;
		mask >>= 1;
		if (mask == 0) {
		    *line_ptr++ = b;
		    b = 0;
		    mask = 0x80;
		}
	    }
	}
	if (mask != 0x80)
	    *line_ptr = b;
	line_start += data_raster;
    }
    return 0;
}

/**
 * Special case implementation for one component. When we do plane_mask,
 * we'll want to generalize this to handle any single-bit plane_mask.
 **/
private int
gx_dc_wts_fill_rectangle_1(const gx_device_color *pdevc,
			   int x, int y, int w, int h,
			   gx_device *dev, gs_logical_operation_t lop,
			   const gx_rop_source_t *source)
{
    /* gx_rop_source_t no_source; */
    int tile_raster = ((w + 31) & -32) >> 3;
    int tile_size = tile_raster * h;
    unsigned char *tile_data;
    int code = 0;
    gx_ht_order_component *components = pdevc->colors.wts.w_ht->components;
    wts_screen_t *ws = components[0].corder.wts;
    wts_screen_sample_t shade = pdevc->colors.wts.levels[0];
    gx_color_index color0, color1;

    color0 = dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN ? 0 :
	pdevc->colors.wts.plane_vector[1];
    color1 = pdevc->colors.wts.plane_vector[0];

    tile_data = malloc(tile_size);

    wts_draw(ws, shade, tile_data, tile_raster, x, y, w, h);
 
    /* See gx_dc_ht_binary_fill_rectangle() for explanation. */
    if (dev->color_info.depth > 1)
	lop &= ~lop_T_transparent;

    /* Interesting question: should data_x be (x & 7), rather than 0,
       to improve alignment? */
    if (source == NULL && lop_no_S_is_T(lop))
	code = (*dev_proc(dev, copy_mono))
	    (dev, tile_data, 0, tile_raster, gx_no_bitmap_id,
	     x, y, w, h, color0, color1);

    free(tile_data);
    return code;
}

private int
gx_dc_wts_write(
    const gx_device_color *         pdevc,
    const gx_device_color_saved *   psdc,
    const gx_device *               dev,
    byte *                          pdata,
    uint *                          psize )
{
    /* not yet implemented */
    return_error(gs_error_unknownerror);
}

private int
gx_dc_wts_read(
    gx_device_color *       pdevc,
    const gs_imager_state * pis,
    const gx_device_color * prior_devc,
    const gx_device *       dev,
    const byte *            pdata,
    uint                    size,
    gs_memory_t *           mem )
{
    /* not yet implemented */
    return_error(gs_error_unknownerror);
}


/**
 * wts_repack_tile_4: Repack four 1-bit tiles into chunky nibbles.
 * Note: argument list will change. plane_mask and base_color will
 * probably get added as an optimization.
 *
 * Note: we round w up to an even value. We're counting on the
 * subsequent copy_color to ignore any extra bits.
 **/
private void
wts_repack_tile_4(unsigned char *ctile_data, int ctile_raster,
		  const unsigned char **tile_data, int tile_raster,
		  const gx_color_index *plane_vector, bool invert,
		  int w, int h)
{
    int y;
    int tile_idx_start = 0;
    unsigned char *ctile_start = ctile_data;
    byte inv_byte = invert ? 0xff : 0;

    for (y = 0; y < h; y++) {
	int x;
	int tile_idx = tile_idx_start;

	for (x = 0; x < w; x += 2) {
	    byte b = 0;
	    byte m0 = 0x80 >> (x & 6);
	    byte m1 = m0 >> 1;
	    byte td;

	    td = tile_data[0][tile_idx] ^ inv_byte;
	    if (td & m0) b |= plane_vector[0] << 4;
	    if (td & m1) b |= plane_vector[0];

	    td = tile_data[1][tile_idx] ^ inv_byte;
	    if (td & m0) b |= plane_vector[1] << 4;
	    if (td & m1) b |= plane_vector[1];

	    td = tile_data[2][tile_idx] ^ inv_byte;
	    if (td & m0) b |= plane_vector[2] << 4;
	    if (td & m1) b |= plane_vector[2];

	    td = tile_data[3][tile_idx] ^ inv_byte;
	    if (td & m0) b |= plane_vector[3] << 4;
	    if (td & m1) b |= plane_vector[3];

	    if ((x & 6) == 6)
		tile_idx++;
	    ctile_start[x >> 1] = b;
	}
	tile_idx_start += tile_raster;
	ctile_start += ctile_raster;
    }
}

/* Special case implementation for four components. Intermediate color
 * to the order objecttile (for copy_color) is packed 2 to a byte.
 *
 * Looking at this code, it should generalize to more than four
 * components. Probably the repack code should get factored out.
 */
private int
gx_dc_wts_fill_rectangle_4(const gx_device_color *pdevc,
			   int x, int y, int w, int h,
			   gx_device *dev, gs_logical_operation_t lop,
			   const gx_rop_source_t *source)
{
    int num_comp = pdevc->colors.wts.num_components;
    /* gx_rop_source_t no_source; */

    int tile_raster = ((w + 31) & -32) >> 3;
    int tile_size = tile_raster * h;
    unsigned char *tile_data[4];

    int ctile_raster = ((w + 7) & -8) >> 1;
    int ctile_size = ctile_raster * h;
    unsigned char *ctile_data;

    int code = 0;
    bool invert = 0 && dev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE;
    int i;

    for (i = 0; i < num_comp; i++) {
	wts_screen_sample_t shade = pdevc->colors.wts.levels[i];
	gx_ht_order_component *components = pdevc->colors.wts.w_ht->components;
	wts_screen_t *ws = components[i].corder.wts;

	tile_data[i] = malloc(tile_size);
	wts_draw(ws, shade, tile_data[i], tile_raster, x, y, w, h);
    }

    ctile_data = malloc(ctile_size);
    wts_repack_tile_4(ctile_data, ctile_raster, 
    		  (const unsigned char **)tile_data, tile_raster,
		      pdevc->colors.wts.plane_vector, invert, w, h);
 
    /* See gx_dc_ht_binary_fill_rectangle() for explanation. */
    if (dev->color_info.depth > 1)
	lop &= ~lop_T_transparent;

    if (source == NULL && lop_no_S_is_T(lop))
	code = (*dev_proc(dev, copy_color))
	    (dev, ctile_data, 0, ctile_raster, gx_no_bitmap_id,
	     x, y, w, h);

    free(ctile_data);
    for (i = 0; i < num_comp; i++) {
	free(tile_data[i]);
    }

    return code;
}

private int
gx_dc_wts_fill_rectangle(const gx_device_color *pdevc,
			 int x, int y, int w, int h,
			 gx_device *dev, gs_logical_operation_t lop,
			 const gx_rop_source_t *source)
{
    int num_comp = pdevc->colors.wts.num_components;

    if (num_comp == 1)
	return gx_dc_wts_fill_rectangle_1(pdevc, x, y, w, h, dev, lop, source);
    else if (num_comp <= 4)
	return gx_dc_wts_fill_rectangle_4(pdevc, x, y, w, h, dev, lop, source);
    else
	return -1;
}

/* Compare two wts colors for equality. */
private int
gx_dc_wts_equal(const gx_device_color *pdevc1,
		const gx_device_color *pdevc2)
{
    uint num_comp = pdevc1->colors.wts.num_components;

    if (pdevc2->type != pdevc1->type ||
	pdevc1->phase.x != pdevc2->phase.x ||
	pdevc1->phase.y != pdevc2->phase.y ||
	num_comp != pdevc2->colors.wts.num_components
	)
	return false;
    return
	!memcmp(pdevc1->colors.wts.levels,
		pdevc2->colors.wts.levels,
		num_comp * sizeof(pdevc1->colors.wts.levels[0]));
}

/*
 * Get the nonzero components of a wts halftone. This is used to
 * distinguish components that are given zero intensity due to halftoning
 * from those for which the original color intensity was in fact zero.
 */
int
gx_dc_wts_get_nonzero_comps(
    const gx_device_color * pdevc,
    const gx_device *       dev_ignored,
    gx_color_index *        pcomp_bits )
{
    int                     i, ncomps =  pdevc->colors.wts.num_components;
    gx_color_index comp_bits = 0; /* todo: plane_mask */

    for (i = 0; i < ncomps; i++) {
        if (pdevc->colors.wts.levels[i] != 0)
            comp_bits |= ((gx_color_index)1) << i;
    }
    *pcomp_bits = comp_bits;

    return 0;
}
