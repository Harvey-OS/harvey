/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsalphac.c,v 1.8 2005/03/14 18:08:36 dan Exp $ */
/* Alpha-compositing implementation */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsalphac.h"
#include "gsiparam.h"		/* for gs_image_alpha_t */
#include "gsutil.h"		/* for gs_next_ids */
#include "gxalpha.h"
#include "gxcomp.h"
#include "gxdevice.h"
#include "gxgetbit.h"
#include "gxlum.h"

/* ---------------- Internal definitions ---------------- */

/* Define the parameters for a compositing operation. */
typedef struct gs_composite_params_s {
    gs_composite_op_t cop;
    float delta;		/* only for dissolve */
    uint source_alpha;		/* only if !psource->alpha */
    uint source_values[4];	/* only if !psource->data */
} gs_composite_params_t;

/* Define the source or destination for a compositing operation. */
#define pixel_row_fields(elt_type)\
  elt_type *data;\
  int bits_per_value;	/* 1, 2, 4, 8, 12, 16 */\
  int initial_x;\
  gs_image_alpha_t alpha
typedef struct pixel_row_s {
    pixel_row_fields(byte);
} pixel_row_t;
typedef struct const_pixel_row_s {
    pixel_row_fields(const byte);
} const_pixel_row_t;

/*
 * Composite two arrays of (premultiplied) pixel values.  Legal values of
 * values_per_pixel are 1-4, not including alpha.  Note that if pdest->alpha
 * is "none", the alpha value for all destination pixels will be taken as
 * unity, and any operation that could generate alpha values other than
 * unity will return an error.  "Could generate" means that there are
 * possible values of the source and destination alpha values for which the
 * result has non-unity alpha: the error check does not scan the actual
 * alpha data to test whether there are any actual values that would
 * generate a non-unity alpha result.
 */
int composite_values(const pixel_row_t * pdest,
		     const const_pixel_row_t * psource,
		     int values_per_pixel, uint num_pixels,
		     const gs_composite_params_t * pcp);

/* ---------------- Alpha-compositing objects ---------------- */

/*
 * Define which operations can generate non-unity alpha values in 3 of the 4
 * cases of source and destination not having unity alphas.  (This is always
 * possible in the fourth case, both S & D non-unity, except for CLEAR.)  We
 * do this with a bit mask indexed by the operation, counting from the LSB.
 * The name indicates whether S and/or D has non-unity alphas.
 */
#define alpha_out_notS_notD\
  (1<<composite_Dissolve)
#define _alpha_out_either\
  (alpha_out_notS_notD|(1<<composite_Satop)|(1<<composite_Datop)|\
    (1<<composite_Xor)|(1<<composite_PlusD)|(1<<composite_PlusL))
#define alpha_out_S_notD\
  (_alpha_out_either|(1<<composite_Copy)|(1<<composite_Sover)|\
    (1<<composite_Din)|(1<<composite_Dout))
#define alpha_out_notS_D\
  (_alpha_out_either|(1<<composite_Sin)|(1<<composite_Sout)|\
    (1<<composite_Dover)|(1<<composite_Highlight))

/* ------ Object definition and creation ------ */

/* Define alpha-compositing objects. */
private composite_create_default_compositor_proc(c_alpha_create_default_compositor);
private composite_equal_proc(c_alpha_equal);
private composite_write_proc(c_alpha_write);
private composite_read_proc(c_alpha_read);
const gs_composite_type_t gs_composite_alpha_type =
{
    GX_COMPOSITOR_ALPHA,
    {
	c_alpha_create_default_compositor,
	c_alpha_equal,
	c_alpha_write,
	c_alpha_read,
	gx_default_composite_clist_write_update,
	gx_default_composite_clist_read_update
    }
};
typedef struct gs_composite_alpha_s {
    gs_composite_common;
    gs_composite_alpha_params_t params;
} gs_composite_alpha_t;

gs_private_st_simple(st_composite_alpha, gs_composite_alpha_t,
		     "gs_composite_alpha_t");

/* Create an alpha-compositing object. */
int
gs_create_composite_alpha(gs_composite_t ** ppcte,
	      const gs_composite_alpha_params_t * params, gs_memory_t * mem)
{
    gs_composite_alpha_t *pcte;

    rc_alloc_struct_0(pcte, gs_composite_alpha_t, &st_composite_alpha,
		      mem, return_error(gs_error_VMerror),
		      "gs_create_composite_alpha");
    pcte->type = &gs_composite_alpha_type;
    pcte->id = gs_next_ids(mem, 1);
    pcte->params = *params;
    *ppcte = (gs_composite_t *) pcte;
    return 0;
}

/* ------ Object implementation ------ */

#define pacte ((const gs_composite_alpha_t *)pcte)

private bool
c_alpha_equal(const gs_composite_t * pcte, const gs_composite_t * pcte2)
{
    return (pcte2->type == pcte->type &&
#define pacte2 ((const gs_composite_alpha_t *)pcte2)
	    pacte2->params.op == pacte->params.op &&
	    (pacte->params.op != composite_Dissolve ||
	     pacte2->params.delta == pacte->params.delta));
#undef pacte2
}

private int
c_alpha_write(const gs_composite_t * pcte, byte * data, uint * psize)
{
    uint size = *psize;
    uint used;

    if (pacte->params.op == composite_Dissolve) {
	used = 1 + sizeof(pacte->params.delta);
	if (size < used) {
	    *psize = used;
	    return_error(gs_error_rangecheck);
	}
	memcpy(data + 1, &pacte->params.delta, sizeof(pacte->params.delta));
    } else {
	used = 1;
	if (size < used) {
	    *psize = used;
	    return_error(gs_error_rangecheck);
	}
    }
    *data = (byte) pacte->params.op;
    *psize = used;
    return 0;
}

private int
c_alpha_read(gs_composite_t ** ppcte, const byte * data, uint size,
	     gs_memory_t * mem)
{
    gs_composite_alpha_params_t params;
    int code, nbytes = 1;

    if (size < 1 || *data > composite_op_last)
	return_error(gs_error_rangecheck);
    params.op = *data;
    if (params.op == composite_Dissolve) {
	if (size < 1 + sizeof(params.delta))
	    return_error(gs_error_rangecheck);
	memcpy(&params.delta, data + 1, sizeof(params.delta));
	nbytes += sizeof(params.delta);
    }
    code = gs_create_composite_alpha(ppcte, &params, mem);
    return code < 0 ? code : nbytes;
}

/* ---------------- Alpha-compositing device ---------------- */

/* Define the default alpha-compositing device. */
typedef struct gx_device_composite_alpha_s {
    gx_device_forward_common;
    gs_composite_alpha_params_t params;
} gx_device_composite_alpha;

gs_private_st_suffix_add0_final(st_device_composite_alpha,
		     gx_device_composite_alpha, "gx_device_composite_alpha",
    device_c_alpha_enum_ptrs, device_c_alpha_reloc_ptrs, gx_device_finalize,
				st_device_forward);
/* The device descriptor. */
private dev_proc_close_device(dca_close);
private dev_proc_fill_rectangle(dca_fill_rectangle);
private dev_proc_map_rgb_color(dca_map_rgb_color);
private dev_proc_map_color_rgb(dca_map_color_rgb);
private dev_proc_copy_mono(dca_copy_mono);
private dev_proc_copy_color(dca_copy_color);
private dev_proc_map_rgb_alpha_color(dca_map_rgb_alpha_color);
private dev_proc_map_color_rgb_alpha(dca_map_color_rgb_alpha);
private dev_proc_copy_alpha(dca_copy_alpha);
private const gx_device_composite_alpha gs_composite_alpha_device =
{std_device_std_body_open(gx_device_composite_alpha, 0,
			  "alpha compositor", 0, 0, 1, 1),
 {gx_default_open_device,
  gx_forward_get_initial_matrix,
  gx_default_sync_output,
  gx_default_output_page,
  dca_close,
  dca_map_rgb_color,
  dca_map_color_rgb,
  dca_fill_rectangle,
  gx_default_tile_rectangle,
  dca_copy_mono,
  dca_copy_color,
  gx_default_draw_line,
  gx_default_get_bits,
  gx_forward_get_params,
  gx_forward_put_params,
  gx_default_cmyk_map_cmyk_color,	/* only called for CMYK */
  gx_forward_get_xfont_procs,
  gx_forward_get_xfont_device,
  dca_map_rgb_alpha_color,
  gx_forward_get_page_device,
  gx_forward_get_alpha_bits,
  dca_copy_alpha,
  gx_forward_get_band,
  gx_default_copy_rop,
  gx_default_fill_path,
  gx_default_stroke_path,
  gx_default_fill_mask,
  gx_default_fill_trapezoid,
  gx_default_fill_parallelogram,
  gx_default_fill_triangle,
  gx_default_draw_thin_line,
  gx_default_begin_image,
  gx_default_image_data,
  gx_default_end_image,
  gx_default_strip_tile_rectangle,
  gx_default_strip_copy_rop,
  gx_forward_get_clipping_box,
  gx_default_begin_typed_image,
  gx_forward_get_bits_rectangle,
  dca_map_color_rgb_alpha,
  gx_no_create_compositor
 }
};

/* Create an alpha compositor. */
private int
c_alpha_create_default_compositor(const gs_composite_t * pcte,
	   gx_device ** pcdev, gx_device * dev, gs_imager_state * pis,
	   gs_memory_t * mem)
{
    gx_device_composite_alpha *cdev;

    if (pacte->params.op == composite_Copy) {
	/* Just use the original device. */
	*pcdev = dev;
	return 0;
    }
    cdev =
	gs_alloc_struct_immovable(mem, gx_device_composite_alpha,
				  &st_device_composite_alpha,
				  "create default alpha compositor");
    *pcdev = (gx_device *)cdev;
    if (cdev == 0)
	return_error(gs_error_VMerror);
    gx_device_init((gx_device *)cdev,
		   (const gx_device *)&gs_composite_alpha_device, mem, true);
    gx_device_copy_params((gx_device *)cdev, dev);
    /*
     * Set the color_info and depth to be compatible with the target,
     * but using standard chunky color storage, including alpha.
     ****** CURRENTLY ALWAYS USE 8-BIT COLOR ******
     */
    cdev->color_info.depth =
	(dev->color_info.num_components == 4 ? 32 /* CMYK, no alpha */ :
	 (dev->color_info.num_components + 1) * 8);
    cdev->color_info.max_gray = cdev->color_info.max_color = 255;
    /* No halftoning will occur, but we fill these in anyway.... */
    cdev->color_info.dither_grays = cdev->color_info.dither_colors = 256;
    /*
     * We could speed things up a little by tailoring the procedures in
     * the device to the specific num_components, but for simplicity,
     * we'll defer considering that until there is a demonstrated need.
     */
    gx_device_set_target((gx_device_forward *)cdev, dev);
    cdev->params = pacte->params;
    return 0;
}

/* Close the device and free its storage. */
private int
dca_close(gx_device * dev)
{				/*
				 * Finalization will call close again: avoid a recursion loop.
				 */
    set_dev_proc(dev, close_device, gx_default_close_device);
    gs_free_object(dev->memory, dev, "dca_close");
    return 0;
}

/* ------ (RGB) color mapping ------ */

private gx_color_index
dca_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    return dca_map_rgb_alpha_color(dev, cv[0], cv[1], cv[2], gx_max_color_value);
}
private gx_color_index
dca_map_rgb_alpha_color(gx_device * dev,
	      gx_color_value red, gx_color_value green, gx_color_value blue,
			gx_color_value alpha)
{				/*
				 * We work exclusively with premultiplied color values, so we
				 * have to premultiply the color components by alpha here.
				 */
    byte a = gx_color_value_to_byte(alpha);

#define premult_(c)\
  (((c) * a + gx_max_color_value / 2) / gx_max_color_value)
#ifdef PREMULTIPLY_TOWARDS_WHITE
    byte bias = ~a;

#  define premult(c) (premult_(c) + bias)
#else
#  define premult(c) premult_(c)
#endif
    gx_color_index color;

    if (dev->color_info.num_components == 1) {
	uint lum =
	(red * lum_red_weight + green * lum_green_weight +
	 blue * lum_blue_weight + lum_all_weights / 2) /
	lum_all_weights;

	if (a == 0xff)
	    color = gx_color_value_to_byte(lum);
	else			/* Premultiplication is necessary. */
	    color = premult(lum);
    } else {
	if (a == 0xff)
	    color =
		((uint) gx_color_value_to_byte(red) << 16) +
		((uint) gx_color_value_to_byte(green) << 8) +
		gx_color_value_to_byte(blue);
	else			/* Premultiplication is necessary. */
	    color =
		(premult(red) << 16) + (premult(green) << 8) + premult(blue);
    }
#undef premult
    return (color << 8) + a;
}
private int
dca_map_color_rgb(gx_device * dev, gx_color_index color,
		  gx_color_value prgb[3])
{
    gx_color_value red = gx_color_value_from_byte((byte) (color >> 24));
    byte a = (byte) color;

#define postdiv_(c)\
  (((c) * 0xff + a / 2) / a)
#ifdef PREMULTIPLY_TOWARDS_WHITE
    byte bias = ~a;

#  define postdiv(c) postdiv_(c - bias)
#else
#  define postdiv(c) postdiv_(c)
#endif

    if (dev->color_info.num_components == 1) {
	if (a != 0xff) {
	    /* Undo premultiplication. */
	    if (a == 0)
		red = 0;
	    else
		red = postdiv(red);
	}
	prgb[0] = prgb[1] = prgb[2] = red;
    } else {
	gx_color_value
	    green = gx_color_value_from_byte((byte) (color >> 16)),
	    blue = gx_color_value_from_byte((byte) (color >> 8));

	if (a != 0xff) {
	    /* Undo premultiplication. */
/****** WHAT TO DO ABOUT BIG LOSS OF PRECISION? ******/
	    if (a == 0)
		red = green = blue = 0;
	    else {
		red = postdiv(red);
		green = postdiv(green);
		blue = postdiv(blue);
	    }
	}
	prgb[0] = red, prgb[1] = green, prgb[2] = blue;
    }
#undef postdiv
    return 0;
}
private int
dca_map_color_rgb_alpha(gx_device * dev, gx_color_index color,
			gx_color_value prgba[4])
{
    prgba[3] = gx_color_value_from_byte((byte) color);
    return dca_map_color_rgb(dev, color, prgba);
}

/* ------ Imaging ------ */

private int
dca_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		   gx_color_index color)
{				/* This is where all the real work gets done! */
    gx_device_composite_alpha *adev = (gx_device_composite_alpha *) dev;
    gx_device *target = adev->target;
    byte *std_row;
    byte *native_row;
    gs_int_rect rect;
    gs_get_bits_params_t std_params, native_params;
    int code = 0;
    int yi;
    gs_composite_params_t cp;
    const_pixel_row_t source;
    pixel_row_t dest;

    fit_fill(dev, x, y, w, h);
    std_row = gs_alloc_bytes(dev->memory,
			     (dev->color_info.depth * w + 7) >> 3,
			     "dca_fill_rectangle(std)");
    native_row = gs_alloc_bytes(dev->memory,
				(target->color_info.depth * w + 7) >> 3,
				"dca_fill_rectangle(native)");
    if (std_row == 0 || native_row == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto out;
    }
    rect.p.x = x, rect.q.x = x + w;
    std_params.options =
	GB_COLORS_NATIVE |
	(GB_ALPHA_LAST | GB_DEPTH_8 | GB_PACKING_CHUNKY |
	 GB_RETURN_COPY | GB_RETURN_POINTER | GB_ALIGN_ANY |
	 GB_OFFSET_0 | GB_OFFSET_ANY | GB_RASTER_STANDARD |
	 GB_RASTER_ANY);
    cp.cop = adev->params.op;
    if (cp.cop == composite_Dissolve)
	cp.delta = adev->params.delta;
    {
	gx_color_value rgba[4];

/****** DOESN'T HANDLE CMYK ******/
	(*dev_proc(dev, map_color_rgb_alpha)) (dev, color, rgba);
	cp.source_values[0] = gx_color_value_to_byte(rgba[0]);
	cp.source_values[1] = gx_color_value_to_byte(rgba[1]);
	cp.source_values[2] = gx_color_value_to_byte(rgba[2]);
	cp.source_alpha = gx_color_value_to_byte(rgba[3]);
    }
    source.data = 0;
    source.bits_per_value = 8;
    source.alpha = gs_image_alpha_none;
    for (yi = y; yi < y + h; ++yi) {
	/* Read a row in standard representation. */
	rect.p.y = yi, rect.q.y = yi + 1;
	std_params.data[0] = std_row;
	code = (*dev_proc(target, get_bits_rectangle))
	    (target, &rect, &std_params, NULL);
	if (code < 0)
	    break;
	/* Do the work. */
	dest.data = std_params.data[0];
	dest.bits_per_value = 8;
	dest.initial_x =
	    (std_params.options & GB_OFFSET_ANY ? std_params.x_offset : 0);
	dest.alpha =
	    (std_params.options & GB_ALPHA_FIRST ? gs_image_alpha_first :
	     std_params.options & GB_ALPHA_LAST ? gs_image_alpha_last :
	     gs_image_alpha_none);
	code = composite_values(&dest, &source,
				dev->color_info.num_components, w, &cp);
	if (code < 0)
	    break;
	if (std_params.data[0] == std_row) {
	    /* Convert the row back to native representation. */
	    /* (Otherwise, we had a direct pointer to device data.) */
	    native_params.options =
		(GB_COLORS_NATIVE | GB_PACKING_CHUNKY | GB_RETURN_COPY |
		 GB_OFFSET_0 | GB_RASTER_ALL | GB_ALIGN_STANDARD);
	    native_params.data[0] = native_row;
	    code = gx_get_bits_copy(target, 0, w, 1, &native_params,
				    &std_params, std_row,
				    0 /* raster is irrelevant */ );
	    if (code < 0)
		break;
	    code = (*dev_proc(target, copy_color))
		(target, native_row, 0, 0 /* raster is irrelevant */ ,
		 gx_no_bitmap_id, x, yi, w, 1);
	    if (code < 0)
		break;
	}
    }
  out:gs_free_object(dev->memory, native_row, "dca_fill_rectangle(native)");
    gs_free_object(dev->memory, std_row, "dca_fill_rectangle(std)");
    return code;
}

private int
dca_copy_mono(gx_device * dev, const byte * data,
	    int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h,
	      gx_color_index zero, gx_color_index one)
{
/****** TEMPORARY ******/
    return gx_default_copy_mono(dev, data, dx, raster, id, x, y, w, h,
				zero, one);
}

private int
dca_copy_color(gx_device * dev, const byte * data,
	       int dx, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h)
{
/****** TEMPORARY ******/
    return gx_default_copy_color(dev, data, dx, raster, id, x, y, w, h);
}

private int
dca_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
	       gx_color_index color, int depth)
{
/****** TEMPORARY ******/
    return gx_default_copy_alpha(dev, data, data_x, raster, id, x, y,
				 width, height, color, depth);
}

/*
 * Composite two arrays of (premultiplied) pixel values.
 * See gsdpnext.h for the specification.
 *
 * The current implementation is simple but inefficient.  We'll speed it up
 * later if necessary.
 */
int
composite_values(const pixel_row_t * pdest, const const_pixel_row_t * psource,
   int values_per_pixel, uint num_pixels, const gs_composite_params_t * pcp)
{
    int dest_bpv = pdest->bits_per_value;
    int source_bpv = psource->bits_per_value;

    /*
     * source_alpha_j gives the source component index for the alpha value,
     * if the source has alpha.
     */
    int source_alpha_j =
    (psource->alpha == gs_image_alpha_last ? values_per_pixel :
     psource->alpha == gs_image_alpha_first ? 0 : -1);

    /* dest_alpha_j does the same for the destination. */
    int dest_alpha_j =
    (pdest->alpha == gs_image_alpha_last ? values_per_pixel :
     pdest->alpha == gs_image_alpha_first ? 0 : -1);

    /* dest_vpp is the number of stored destination values. */
    int dest_vpp = values_per_pixel + (dest_alpha_j >= 0);

    /* source_vpp is the number of stored source values. */
    int source_vpp = values_per_pixel + (source_alpha_j >= 0);

    bool constant_colors = psource->data == 0;
    uint highlight_value = (1 << dest_bpv) - 1;

    sample_load_declare(sptr, sbit);
    sample_store_declare(dptr, dbit, dbyte);

    {
	uint xbit = pdest->initial_x * dest_bpv * dest_vpp;

	sample_store_setup(dbit, xbit & 7, dest_bpv);
	dptr = pdest->data + (xbit >> 3);
    }
    {
	uint xbit = psource->initial_x * source_bpv * source_vpp;

	sbit = xbit & 7;
	sptr = psource->data + (xbit >> 3);
    }
    {
	uint source_max = (1 << source_bpv) - 1;
	uint dest_max = (1 << dest_bpv) - 1;

	/*
	 * We could save a little work by only setting up source_delta
	 * and dest_delta if the operation is Dissolve.
	 */
	float source_delta = pcp->delta * dest_max / source_max;
	float dest_delta = 1.0 - pcp->delta;
	uint source_alpha = pcp->source_alpha;
	uint dest_alpha = dest_max;

#ifdef PREMULTIPLY_TOWARDS_WHITE
	uint source_bias = source_max - source_alpha;
	uint dest_bias = 0;
	uint result_bias = 0;

#endif
	uint x;

	if (!pdest->alpha) {
	    uint mask =
	    (psource->alpha || source_alpha != source_max ?
	     alpha_out_S_notD : alpha_out_notS_notD);

	    if ((mask >> pcp->cop) & 1) {
		/*
		 * The operation could produce non-unity alpha values, but
		 * the destination can't store them.  Return an error.
		 */
		return_error(gs_error_rangecheck);
	    }
	}
	/* Preload the output byte buffer if necessary. */
	sample_store_preload(dbyte, dptr, dbit, dest_bpv);

	for (x = 0; x < num_pixels; ++x) {
	    int j;
	    uint result_alpha = dest_alpha;

/* get_value does not increment the source pointer. */
#define get_value(v, ptr, bit, bpv, vmax)\
  sample_load16(v, ptr, bit, bpv)

/* put_value increments the destination pointer. */
#define put_value(v, ptr, bit, bpv, bbyte)\
  sample_store_next16(v, ptr, bit, bpv, bbyte)

#define advance(ptr, bit, bpv)\
  sample_next(ptr, bit, bpv)

	    /* Get destination alpha value. */
	    if (dest_alpha_j >= 0) {
		int dabit = dbit + dest_bpv * dest_alpha_j;
		const byte *daptr = dptr + (dabit >> 3);

		get_value(dest_alpha, daptr, dabit & 7, dest_bpv, dest_max);
#ifdef PREMULTIPLY_TOWARDS_WHITE
		dest_bias = dest_max - dest_alpha;
#endif
	    }
	    /* Get source alpha value. */
	    if (source_alpha_j >= 0) {
		int sabit = sbit;
		const byte *saptr = sptr;

		if (source_alpha_j == 0)
		    advance(sptr, sbit, source_bpv);
		else
		    advance(saptr, sabit, source_bpv * source_alpha_j);
		get_value(source_alpha, saptr, sabit, source_bpv, source_max);
#ifdef PREMULTIPLY_TOWARDS_WHITE
		source_bias = source_max - source_alpha;
#endif
	    }
/*
 * We are always multiplying a dest value by a source value to compute a
 * dest value, so the denominator is always source_max.  (Dissolve is the
 * one exception.)
 */
#define fr(v, a) ((v) * (a) / source_max)
#define nfr(v, a, maxv) ((v) * (maxv - (a)) / source_max)

	    /*
	     * Iterate over the components of a single pixel.
	     * j = 0 for alpha, 1 .. values_per_pixel for color
	     * components, regardless of the actual storage order;
	     * we arrange things so that sptr/sbit and dptr/dbit
	     * always point to the right place.
	     */
	    for (j = 0; j <= values_per_pixel; ++j) {
		uint dest_v, source_v, result;

#define set_clamped(r, v)\
  BEGIN if ( (r = (v)) > dest_max ) r = dest_max; END

		if (j == 0) {
		    source_v = source_alpha;
		    dest_v = dest_alpha;
		} else {
		    if (constant_colors)
			source_v = pcp->source_values[j - 1];
		    else {
			get_value(source_v, sptr, sbit, source_bpv, source_max);
			advance(sptr, sbit, source_bpv);
		    }
		    get_value(dest_v, dptr, dbit, dest_bpv, dest_max);
#ifdef PREMULTIPLY_TOWARDS_WHITE
		    source_v -= source_bias;
		    dest_v -= dest_bias;
#endif
		}

		switch (pcp->cop) {
		    case composite_Clear:
			/*
			 * The NeXT documentation doesn't say this, but the CLEAR
			 * operation sets not only alpha but also all the color
			 * values to 0.
			 */
			result = 0;
			break;
		    case composite_Copy:
			result = source_v;
			break;
		    case composite_PlusD:
			/*
			 * This is the only case where we have to worry about
			 * clamping a possibly negative result.
			 */
			result = source_v + dest_v;
			result = (result < dest_max ? 0 : result - dest_max);
			break;
		    case composite_PlusL:
			set_clamped(result, source_v + dest_v);
			break;
		    case composite_Sover:
			set_clamped(result, source_v + nfr(dest_v, source_alpha, source_max));
			break;
		    case composite_Dover:
			set_clamped(result, nfr(source_v, dest_alpha, dest_max) + dest_v);
			break;
		    case composite_Sin:
			result = fr(source_v, dest_alpha);
			break;
		    case composite_Din:
			result = fr(dest_v, source_alpha);
			break;
		    case composite_Sout:
			result = nfr(source_v, dest_alpha, dest_max);
			break;
		    case composite_Dout:
			result = nfr(dest_v, source_alpha, source_max);
			break;
		    case composite_Satop:
			set_clamped(result, fr(source_v, dest_alpha) +
				    nfr(dest_v, source_alpha, source_max));
			break;
		    case composite_Datop:
			set_clamped(result, nfr(source_v, dest_alpha, dest_max) +
				    fr(dest_v, source_alpha));
			break;
		    case composite_Xor:
			set_clamped(result, nfr(source_v, dest_alpha, dest_max) +
				    nfr(dest_v, source_alpha, source_max));
			break;
		    case composite_Highlight:
			/*
			 * Bizarre but true: this operation converts white and
			 * light gray into each other, and leaves all other values
			 * unchanged.  We only implement it properly for gray-scale
			 * devices.
			 */
			if (j != 0 && !((source_v ^ highlight_value) & ~1))
			    result = source_v ^ 1;
			else
			    result = source_v;
			break;
		    case composite_Dissolve:
			/*
			 * In this case, and only this case, we need to worry about
			 * source and dest having different bpv values.  For the
			 * moment, we wimp out and do everything in floating point.
			 */
			result = (uint) (source_v * source_delta + dest_v * dest_delta);
			break;
		    default:
			return_error(gs_error_rangecheck);
		}
		/*
		 * Store the result.  We don't have to worry about
		 * destinations that don't store alpha, because we don't
		 * even compute an alpha value in that case.
		 */
#ifdef PREMULTIPLY_TOWARDS_WHITE
		if (j == 0) {
		    result_alpha = result;
		    result_bias = dest_max - result_alpha;
		    if (dest_alpha_j != 0)
			continue;
		} else {
		    result += result_bias;
		}
#else
		if (j == 0 && dest_alpha_j != 0) {
		    result_alpha = result;
		    continue;
		}
#endif
		put_value(result, dptr, dbit, dest_bpv, dbyte);
	    }
	    /* Skip a trailing source alpha value. */
	    if (source_alpha_j > 0)
		advance(sptr, sbit, source_bpv);
	    /* Store a trailing destination alpha value. */
	    if (dest_alpha_j > 0)
		put_value(result_alpha, dptr, dbit, dest_bpv, dbyte);
#undef get_value
#undef put_value
#undef advance
	}
	/* Store any partial output byte. */
	sample_store_flush(dptr, dbit, dest_bpv, dbyte);
    }
    return 0;
}
