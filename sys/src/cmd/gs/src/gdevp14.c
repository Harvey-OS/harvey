/*
  Copyright (C) 2001-2004 artofcode LLC.
  
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

  Author: Raph Levien <raph@artofcode.com>
*/
/* $Id: gdevp14.c,v 1.35 2005/10/10 18:58:18 leonardo Exp $	*/
/* Compositing devices for implementing	PDF 1.4	imaging	model */

#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscdefs.h"
#include "gxdevice.h"
#include "gsdevice.h"
#include "gsstruct.h"
#include "gxistate.h"
#include "gxdcolor.h"
#include "gxiparam.h"
#include "gstparam.h"
#include "gxblend.h"
#include "gxtext.h"
#include "gsdfilt.h"
#include "gsimage.h"
#include "gsrect.h"
#include "gzstate.h"
#include "gdevp14.h"
#include "gsovrc.h"
#include "gxcmap.h"
#include "gscolor1.h"
#include "gstrans.h"
#include "gsutil.h"
#include "gxcldev.h"

/* #define DUMP_TO_PNG */

#ifdef DUMP_TO_PNG
#include "png_.h"
#endif

# define INCR(v) DO_NOTHING

/* Buffer stack	data structure */

#define	PDF14_MAX_PLANES 16

/* GC procedures for buffer stack */

private
ENUM_PTRS_WITH(pdf14_buf_enum_ptrs, pdf14_buf *buf)
    return 0;
    case 0: return ENUM_OBJ(buf->saved);
    case 1: return ENUM_OBJ(buf->data);
    case 2: return ENUM_OBJ(buf->transfer_fn);
ENUM_PTRS_END

private
RELOC_PTRS_WITH(pdf14_buf_reloc_ptrs, pdf14_buf	*buf)
{
    RELOC_VAR(buf->saved);
    RELOC_VAR(buf->data);
    RELOC_VAR(buf->transfer_fn);
}
RELOC_PTRS_END

gs_private_st_composite(st_pdf14_buf, pdf14_buf, "pdf14_buf",
			pdf14_buf_enum_ptrs, pdf14_buf_reloc_ptrs);

gs_private_st_ptrs2(st_pdf14_ctx, pdf14_ctx, "pdf14_ctx",
		    pdf14_ctx_enum_ptrs, pdf14_ctx_reloc_ptrs,
		    stack, maskbuf);

/* ------ The device descriptors ------	*/

/*
 * Default X and Y resolution.
 */
#define	X_DPI 72
#define	Y_DPI 72

private	int pdf14_open(gx_device * pdev);
private	dev_proc_close_device(pdf14_close);
private	int pdf14_output_page(gx_device	* pdev,	int num_copies,	int flush);
private	dev_proc_put_params(pdf14_put_params);
private	dev_proc_encode_color(pdf14_encode_color);
private	dev_proc_decode_color(pdf14_decode_color);
private	dev_proc_fill_rectangle(pdf14_fill_rectangle);
private	dev_proc_fill_rectangle(pdf14_mark_fill_rectangle);
private	dev_proc_fill_rectangle(pdf14_mark_fill_rectangle_ko_simple);
private	dev_proc_fill_path(pdf14_fill_path);
private	dev_proc_stroke_path(pdf14_stroke_path);
private	dev_proc_begin_typed_image(pdf14_begin_typed_image);
private	dev_proc_text_begin(pdf14_text_begin);
private	dev_proc_create_compositor(pdf14_create_compositor);
private	dev_proc_create_compositor(pdf14_forward_create_compositor);
private	dev_proc_begin_transparency_group(pdf14_begin_transparency_group);
private	dev_proc_end_transparency_group(pdf14_end_transparency_group);
private	dev_proc_begin_transparency_mask(pdf14_begin_transparency_mask);
private	dev_proc_end_transparency_mask(pdf14_end_transparency_mask);

private	const gx_color_map_procs *
    pdf14_get_cmap_procs(const gs_imager_state *, const gx_device *);

#define	XSIZE (int)(8.5	* X_DPI)	/* 8.5 x 11 inch page, by default */
#define	YSIZE (int)(11 * Y_DPI)

/* 24-bit color. */

#define	pdf14_procs(get_color_mapping_procs, get_color_comp_index) \
{\
	pdf14_open,			/* open */\
	NULL,				/* get_initial_matrix */\
	NULL,				/* sync_output */\
	pdf14_output_page,		/* output_page */\
	pdf14_close,			/* close */\
	pdf14_encode_color,		/* rgb_map_rgb_color */\
	pdf14_decode_color,		/* gx_default_rgb_map_color_rgb */\
	pdf14_fill_rectangle,		/* fill_rectangle */\
	NULL,				/* tile_rectangle */\
	NULL,				/* copy_mono */\
	NULL,				/* copy_color */\
	NULL,				/* draw_line */\
	NULL,				/* get_bits */\
	NULL,				/* get_params */\
	pdf14_put_params,		/* put_params */\
	NULL,				/* map_cmyk_color */\
	NULL,				/* get_xfont_procs */\
	NULL,				/* get_xfont_device */\
	NULL,				/* map_rgb_alpha_color */\
	NULL,				/* get_page_device */\
	NULL,				/* get_alpha_bits */\
	NULL,				/* copy_alpha */\
	NULL,				/* get_band */\
	NULL,				/* copy_rop */\
	pdf14_fill_path,		/* fill_path */\
	pdf14_stroke_path,		/* stroke_path */\
	NULL,				/* fill_mask */\
	NULL,				/* fill_trapezoid */\
	NULL,				/* fill_parallelogram */\
	NULL,				/* fill_triangle */\
	NULL,				/* draw_thin_line */\
	NULL,				/* begin_image */\
	NULL,				/* image_data */\
	NULL,				/* end_image */\
	NULL,				/* strip_tile_rectangle */\
	NULL,				/* strip_copy_rop, */\
	NULL,				/* get_clipping_box */\
	pdf14_begin_typed_image,	/* begin_typed_image */\
	NULL,				/* get_bits_rectangle */\
	NULL,				/* map_color_rgb_alpha */\
	pdf14_create_compositor,	/* create_compositor */\
	NULL,				/* get_hardware_params */\
	pdf14_text_begin,		/* text_begin */\
	NULL,				/* finish_copydevice */\
	pdf14_begin_transparency_group,\
	pdf14_end_transparency_group,\
	pdf14_begin_transparency_mask,\
	pdf14_end_transparency_mask,\
	NULL,				/* discard_transparency_layer */\
	get_color_mapping_procs,	/* get_color_mapping_procs */\
	get_color_comp_index,		/* get_color_comp_index */\
	pdf14_encode_color,		/* encode_color */\
	pdf14_decode_color		/* decode_color */\
}

private	const gx_device_procs pdf14_Gray_procs =
	pdf14_procs(gx_default_DevGray_get_color_mapping_procs,
			gx_default_DevGray_get_color_comp_index);

private	const gx_device_procs pdf14_RGB_procs =
	pdf14_procs(gx_default_DevRGB_get_color_mapping_procs,
			gx_default_DevRGB_get_color_comp_index);

private	const gx_device_procs pdf14_CMYK_procs =
	pdf14_procs(gx_default_DevCMYK_get_color_mapping_procs,
			gx_default_DevCMYK_get_color_comp_index);

gs_private_st_composite_use_final(st_pdf14_device, pdf14_device, "pdf14_device",
				  pdf14_device_enum_ptrs, pdf14_device_reloc_ptrs,
			  gx_device_finalize);

const pdf14_device gs_pdf14_Gray_device	= {
    std_device_color_stype_body(pdf14_device, &pdf14_Gray_procs, "pdf14gray",
				&st_pdf14_device,
				XSIZE, YSIZE, X_DPI, Y_DPI, 8, 255, 256),
    { 0 }
};

const pdf14_device gs_pdf14_RGB_device = {
    std_device_color_stype_body(pdf14_device, &pdf14_RGB_procs, "pdf14RGB",
				&st_pdf14_device,
				XSIZE, YSIZE, X_DPI, Y_DPI, 24, 255, 256),
    { 0 }
};

const pdf14_device gs_pdf14_CMYK_device	= {
    std_device_std_color_full_body_type(pdf14_device, &pdf14_CMYK_procs,
	"PDF14cmyk", &st_pdf14_device, XSIZE, YSIZE, X_DPI, Y_DPI, 32,
	0, 0, 0, 0, 0, 0),
    { 0 }
};

/* GC procedures */
private	
ENUM_PTRS_WITH(pdf14_device_enum_ptrs, pdf14_device *pdev) return 0;
case 0:	return ENUM_OBJ(pdev->ctx);
case 1:	ENUM_RETURN(gx_device_enum_ptr(pdev->target));
ENUM_PTRS_END
private	RELOC_PTRS_WITH(pdf14_device_reloc_ptrs, pdf14_device *pdev)
{
    RELOC_VAR(pdev->ctx);
    pdev->target = gx_device_reloc_ptr(pdev->target, gcst);
}
RELOC_PTRS_END

/* ------ Private definitions ------ */

/**
 * pdf14_buf_new: Allocate a new PDF 1.4 buffer.
 * @n_chan: Number of pixel channels including alpha.
 *
 * Return value: Newly allocated buffer, or NULL on failure.
 **/
private	pdf14_buf *
pdf14_buf_new(gs_int_rect *rect, bool has_alpha_g, bool	has_shape,
	       int n_chan,
	       gs_memory_t *memory)
{
    pdf14_buf *result;
    int rowstride = (rect->q.x - rect->p.x + 3) & -4;
    int height = (rect->q.y - rect->p.y);
    int n_planes = n_chan + (has_shape ? 1 : 0) + (has_alpha_g ? 1 : 0);
    int planestride;
    double dsize = (((double) rowstride) * height) * n_planes;

    if (dsize > (double)max_uint)
      return NULL;

    result = gs_alloc_struct(memory, pdf14_buf, &st_pdf14_buf,
			     "pdf14_buf_new");
    if (result == NULL)
	return result;

    result->isolated = false;
    result->knockout = false;
    result->has_alpha_g = has_alpha_g;
    result->has_shape = has_shape;
    result->rect = *rect;
    result->n_chan = n_chan;
    result->n_planes = n_planes;
    result->rowstride = rowstride;
    result->transfer_fn = NULL;
    
    if (height < 0) {
	/* Empty clipping - will skip all drawings. */
	result->planestride = 0;
	result->data = 0;
    } else {
	planestride = rowstride * height;
	result->planestride = planestride;
	result->data = gs_alloc_bytes(memory, planestride * n_planes,
					"pdf14_buf_new");
	if (result->data == NULL) {
	    gs_free_object(memory, result, "pdf_buf_new");
	    return NULL;
	}
	if (has_alpha_g) {
	    int alpha_g_plane = n_chan + (has_shape ? 1 : 0);
	    memset (result->data + alpha_g_plane * planestride, 0, planestride);
	}
    }
    result->bbox.p.x = max_int;
    result->bbox.p.y = max_int;
    result->bbox.q.x = min_int;
    result->bbox.q.y = min_int;
    return result;
}

private	void
pdf14_buf_free(pdf14_buf *buf, gs_memory_t *memory)
{
    gs_free_object(memory, buf->transfer_fn, "pdf14_buf_free");
    gs_free_object(memory, buf->data, "pdf14_buf_free");
    gs_free_object(memory, buf, "pdf14_buf_free");
}

private	pdf14_ctx *
pdf14_ctx_new(gs_int_rect *rect, int n_chan, bool additive, gs_memory_t	*memory)
{
    pdf14_ctx *result;
    pdf14_buf *buf;

    result = gs_alloc_struct(memory, pdf14_ctx, &st_pdf14_ctx,
			     "pdf14_ctx_new");
    if (result == NULL)
	return result;

    buf = pdf14_buf_new(rect, false, false, n_chan, memory);
    if (buf == NULL) {
	gs_free_object(memory, result, "pdf14_ctx_new");
	return NULL;
    }
    if_debug3('v', "[v]base buf: %d x %d, %d channels\n",
	      buf->rect.q.x, buf->rect.q.y, buf->n_chan);
    memset(buf->data, 0, buf->planestride * buf->n_planes);
    buf->saved = NULL;
    result->stack = buf;
    result->maskbuf = NULL;
    result->n_chan = n_chan;
    result->memory = memory;
    result->rect = *rect;
    result->additive = additive;
    return result;
}

private	void
pdf14_ctx_free(pdf14_ctx *ctx)
{
    pdf14_buf *buf, *next;

    for (buf = ctx->stack; buf != NULL; buf = next) {
	next = buf->saved;
	pdf14_buf_free(buf, ctx->memory);
    }
    gs_free_object (ctx->memory, ctx, "pdf14_ctx_free");
}

/**
 * pdf14_find_backdrop_buf: Find backdrop buffer.
 *
 * Return value: Backdrop buffer for current group operation, or NULL
 * if backdrop is fully transparent.
 **/
private	pdf14_buf *
pdf14_find_backdrop_buf(pdf14_ctx *ctx)
{
    pdf14_buf *buf = ctx->stack;

    while (buf != NULL) {
	if (buf->isolated) return NULL;
	if (!buf->knockout) return buf->saved;
	buf = buf->saved;
    }
    /* this really shouldn't happen, as bottom-most buf should be
       non-knockout */
    return NULL;
}

private	int
pdf14_push_transparency_group(pdf14_ctx	*ctx, gs_int_rect *rect,
			      bool isolated, bool knockout,
			      byte alpha, byte shape,
			      gs_blend_mode_t blend_mode)
{
    pdf14_buf *tos = ctx->stack;
    pdf14_buf *buf, *backdrop;
    bool has_shape;

    /* todo: fix this hack, which makes all knockout groups isolated.
       For the vast majority of files, there won't be any visible
       effects, but it still isn't correct. The pixel compositing code
       for non-isolated knockout groups gets pretty hairy, which is
       why this is here. */
    if (knockout) 
	isolated = true;

    has_shape = tos->has_shape || tos->knockout;

    buf = pdf14_buf_new(rect, !isolated, has_shape, ctx->n_chan, ctx->memory);
    if_debug3('v', "[v]push buf: %d x %d, %d channels\n", buf->rect.p.x, buf->rect.p.y, buf->n_chan);
    if (buf == NULL)
	return_error(gs_error_VMerror);
    buf->isolated = isolated;
    buf->knockout = knockout;
    buf->alpha = alpha;
    buf->shape = shape;
    buf->blend_mode = blend_mode;

    buf->saved = tos;
    ctx->stack = buf;

    backdrop = pdf14_find_backdrop_buf(ctx);
    if (backdrop == NULL) {
	memset(buf->data, 0, buf->planestride * (buf->n_chan +
						 (buf->has_shape ? 1 : 0)));
    } else {
	/* make copy of backdrop for compositing */
	byte *buf_plane = buf->data;
	byte *tos_plane = tos->data + buf->rect.p.x - tos->rect.p.x +
	    (buf->rect.p.y - tos->rect.p.y) * tos->rowstride;
	int width = buf->rect.q.x - buf->rect.p.x;
	int y0 = buf->rect.p.y;
	int y1 = buf->rect.q.y;
	int i;
	int n_chan_copy = buf->n_chan + (tos->has_shape ? 1 : 0);

	for (i = 0; i < n_chan_copy; i++) {
	    byte *buf_ptr = buf_plane;
	    byte *tos_ptr = tos_plane;
	    int y;

	    for (y = y0; y < y1; ++y) {
		memcpy (buf_ptr, tos_ptr, width); 
		buf_ptr += buf->rowstride;
		tos_ptr += tos->rowstride;
	    }
	    buf_plane += buf->planestride;
	    tos_plane += tos->planestride;
	}
	if (has_shape && !tos->has_shape)
	    memset (buf_plane, 0, buf->planestride);
    }

    return 0;
}

private	int
pdf14_pop_transparency_group(pdf14_ctx *ctx)
{
    pdf14_buf *tos = ctx->stack;
    pdf14_buf *nos = tos->saved;
    pdf14_buf *maskbuf = ctx->maskbuf;
    int y0 = tos->rect.p.y;
    int y1 = tos->rect.q.y;
    if (y0 < y1) {
	int x0 = tos->rect.p.x;
	int x1 = tos->rect.q.x;
	int n_chan = ctx->n_chan;
	int num_comp = n_chan - 1;
	byte alpha = tos->alpha;
	byte shape = tos->shape;
	byte blend_mode = tos->blend_mode;
	byte *tos_ptr = tos->data;
	byte *nos_ptr = nos->data + x0 - nos->rect.p.x +
	    (y0 - nos->rect.p.y) * nos->rowstride;
	byte *mask_ptr = NULL;
	int tos_planestride = tos->planestride;
	int nos_planestride = nos->planestride;
	int mask_planestride = 0x0badf00d; /* Quiet compiler. */
	byte mask_bg_alpha = 0; /* Quiet compiler. */
	int width = x1 - x0;
	int x, y;
	int i;
	byte tos_pixel[PDF14_MAX_PLANES];
	byte nos_pixel[PDF14_MAX_PLANES];
	bool tos_isolated = tos->isolated;
	bool nos_knockout = nos->knockout;
	byte *nos_alpha_g_ptr;
	int tos_shape_offset = n_chan * tos_planestride;
	int tos_alpha_g_offset = tos_shape_offset +
	(tos->has_shape ? tos_planestride : 0);
	int nos_shape_offset = n_chan * nos_planestride;
	bool nos_has_shape = nos->has_shape;
	byte *mask_tr_fn = NULL; /* Quiet compiler. */
	bool additive = ctx->additive;

	if (nos == NULL)
	    return_error(gs_error_rangecheck);

	rect_merge(nos->bbox, tos->bbox);

	if_debug6('v', "pdf14_pop_transparency_group y0 = %d, y1 = %d, w = %d, alpha = %d, shape = %d, bm = %d\n",
			    y0, y1, width, alpha, shape, blend_mode);
	if (nos->has_alpha_g)
	    nos_alpha_g_ptr = nos_ptr + n_chan * nos_planestride;
	else
	    nos_alpha_g_ptr = NULL;

	if (maskbuf != NULL) {
	    mask_ptr = maskbuf->data + x0 - maskbuf->rect.p.x +
		    (y0 - maskbuf->rect.p.y) * maskbuf->rowstride;
	    mask_planestride = maskbuf->planestride;
	    mask_bg_alpha = maskbuf->alpha;
	    mask_tr_fn = maskbuf->transfer_fn;
	}

	for (y = y0; y < y1; ++y) {
	    for (x = 0; x < width; ++x) {
		byte pix_alpha = alpha;

		/* Complement the components for subtractive color spaces */
		if (additive) {
		    for (i = 0; i < n_chan; ++i) {
			tos_pixel[i] = tos_ptr[x + i * tos_planestride];
			nos_pixel[i] = nos_ptr[x + i * nos_planestride];
		    }
		} else {
		    for (i = 0; i < num_comp; ++i) {
			tos_pixel[i] = 255 - tos_ptr[x + i * tos_planestride];
			nos_pixel[i] = 255 - nos_ptr[x + i * nos_planestride];
		    }
		    tos_pixel[num_comp] = tos_ptr[x + num_comp * tos_planestride];
		    nos_pixel[num_comp] = nos_ptr[x + num_comp * nos_planestride];
		}

		if (mask_ptr != NULL) {
		    int mask_alpha = mask_ptr[x + num_comp * mask_planestride];
		    int tmp;
		    byte mask;

			/*
			* The mask data is really monochrome.  Thus for additive (RGB)
			* we use the R channel for alpha since R = G = B.  For
			* subtractive (CMYK) we use the K channel.
			*/
		    if (mask_alpha == 255) {
			/* todo: rgba->mask */
			mask = additive ? mask_ptr[x]
					: 255 - mask_ptr[x + 3 * mask_planestride];
		    } else if (mask_alpha == 0)
			mask = mask_bg_alpha;
		    else {
			int t2 = additive ? mask_ptr[x]
					: 255 - mask_ptr[x + 3 * mask_planestride];

			t2 = (t2 - mask_bg_alpha) * mask_alpha + 0x80;
			mask = mask_bg_alpha + ((t2 + (t2 >> 8)) >> 8);
		    }
		    mask = mask_tr_fn[mask];
		    tmp = pix_alpha * mask + 0x80;
		    pix_alpha = (tmp + (tmp >> 8)) >> 8;
		}

		if (nos_knockout) {
		    byte *nos_shape_ptr = nos_has_shape ?
			&nos_ptr[x + nos_shape_offset] : NULL;
		    byte tos_shape = tos_ptr[x + tos_shape_offset];

		    art_pdf_composite_knockout_isolated_8(nos_pixel,
							nos_shape_ptr,
							tos_pixel,
							n_chan - 1,
							tos_shape,
							pix_alpha, shape);
		} else if (tos_isolated) {
		    art_pdf_composite_group_8(nos_pixel, nos_alpha_g_ptr,
						tos_pixel,
						n_chan - 1,
						pix_alpha, blend_mode);
		} else {
		    byte tos_alpha_g = tos_ptr[x + tos_alpha_g_offset];
		    art_pdf_recomposite_group_8(nos_pixel, nos_alpha_g_ptr,
						tos_pixel, tos_alpha_g,
						n_chan - 1,
						pix_alpha, blend_mode);
		}
		if (nos_has_shape) {
		    nos_ptr[x + nos_shape_offset] =
			art_pdf_union_mul_8 (nos_ptr[x + nos_shape_offset],
						tos_ptr[x + tos_shape_offset],
						shape);
		}
	    
		/* Complement the results for subtractive color spaces */
		if (additive) {
		    for (i = 0; i < n_chan; ++i) {
			nos_ptr[x + i * nos_planestride] = nos_pixel[i];
		    }
		} else {
		    for (i = 0; i < num_comp; ++i)
			nos_ptr[x + i * nos_planestride] = 255 - nos_pixel[i];
		    nos_ptr[x + num_comp * nos_planestride] = nos_pixel[num_comp];
		}
		if (nos_alpha_g_ptr != NULL)
		    ++nos_alpha_g_ptr;
	    }
	    tos_ptr += tos->rowstride;
	    nos_ptr += nos->rowstride;
	    if (nos_alpha_g_ptr != NULL)
		nos_alpha_g_ptr += nos->rowstride - width;
	    if (mask_ptr != NULL)
		mask_ptr += maskbuf->rowstride;
	}
    }

    ctx->stack = nos;
    if_debug0('v', "[v]pop buf\n");
    pdf14_buf_free(tos, ctx->memory);
    if (maskbuf != NULL) {
	pdf14_buf_free(maskbuf, ctx->memory);
	ctx->maskbuf = NULL;
    }
    return 0;
}

private	int
pdf14_push_transparency_mask(pdf14_ctx *ctx, gs_int_rect *rect,	byte bg_alpha,
			     byte *transfer_fn)
{
    pdf14_buf *buf;

    if_debug0('v', "[v]pdf_push_transparency_mask\n");
    buf = pdf14_buf_new(rect, false, false, ctx->n_chan, ctx->memory);
    if (buf == NULL)
	return_error(gs_error_VMerror);

    /* fill in, but these values aren't really used */
    buf->isolated = true;
    buf->knockout = false;
    buf->alpha = bg_alpha;
    buf->shape = 0xff;
    buf->blend_mode = BLEND_MODE_Normal;
    buf->transfer_fn = transfer_fn;

    buf->saved = ctx->stack;
    ctx->stack = buf;
    memset(buf->data, 0, buf->planestride * buf->n_chan);
    return 0;
}

private	int
pdf14_pop_transparency_mask(pdf14_ctx *ctx)
{
    pdf14_buf *tos = ctx->stack;

    ctx->stack = tos->saved;
    ctx->maskbuf = tos;
    return 0;
}

private	int
pdf14_open(gx_device *dev)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    gs_int_rect rect;

    if_debug2('v', "[v]pdf14_open: width = %d, height = %d\n",
	     dev->width, dev->height);

    rect.p.x = 0;
    rect.p.y = 0;
    rect.q.x = dev->width;
    rect.q.y = dev->height;
    pdev->ctx = pdf14_ctx_new(&rect, dev->color_info.num_components + 1,
	pdev->color_info.polarity != GX_CINFO_POLARITY_SUBTRACTIVE, dev->memory);
    if (pdev->ctx == NULL)
	return_error(gs_error_VMerror);
    return 0;
}

/*
 * Encode a list of colorant values into a gx_color_index_value.
 */
private	gx_color_index
pdf14_encode_color(gx_device *dev, const gx_color_value	colors[])
{
    int drop = sizeof(gx_color_value) * 8 - 8;
    gx_color_index color = 0;
    int i;
    int ncomp = dev->color_info.num_components;

    for (i = 0; i < ncomp; i++) {
	color <<= 8;
	color |= (colors[i] >> drop);
    }
    return (color == gx_no_color_index ? color ^ 1 : color);
}

/*
 * Decode a gx_color_index value back to a list of colorant values.
 */
private	int
pdf14_decode_color(gx_device * dev, gx_color_index color, gx_color_value * out)
{
    int i;
    int ncomp = dev->color_info.num_components;

    for (i = 0; i < ncomp; i++) {
	out[ncomp - i - 1] = (gx_color_value) ((color & 0xff) * 0x101);
	color >>= 8;
    }
    return 0;
}

#ifdef DUMP_TO_PNG
/* Dumps a planar RGBA image to	a PNG file. */
private	int
dump_planar_rgba(gs_memory_t *mem, 
		 const byte *buf, int width, int height, int rowstride, int planestride)
{
    int rowbytes = width << 2;
    byte *row = gs_malloc(mem, rowbytes, 1, "png raster buffer");
    png_struct *png_ptr =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_info *info_ptr =
    png_create_info_struct(png_ptr);
    const char *software_key = "Software";
    char software_text[256];
    png_text text_png;
    const byte *buf_ptr = buf;
    FILE *file;
    int code;
    int y;

	file = fopen ("c:\\temp\\tmp.png", "wb");

    if_debug0('v', "[v]pnga_output_page\n");

    if (row == 0 || png_ptr == 0 || info_ptr == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    /* set error handling */
    if (setjmp(png_ptr->jmpbuf)) {
	/* If we get here, we had a problem reading the file */
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }

    code = 0;			/* for normal path */
    /* set up the output control */
    png_init_io(png_ptr, file);

    /* set the file information here */
    info_ptr->width = width;
    info_ptr->height = height;
    /* resolution is in pixels per meter vs. dpi */
    info_ptr->x_pixels_per_unit =
	(png_uint_32) (96.0 * (100.0 / 2.54));
    info_ptr->y_pixels_per_unit =
	(png_uint_32) (96.0 * (100.0 / 2.54));
    info_ptr->phys_unit_type = PNG_RESOLUTION_METER;
    info_ptr->valid |= PNG_INFO_pHYs;

    /* At present, only supporting 32-bit rgba */
    info_ptr->bit_depth = 8;
    info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;

    /* add comment */
    sprintf(software_text, "%s %d.%02d", gs_product,
	    (int)(gs_revision / 100), (int)(gs_revision % 100));
    text_png.compression = -1;	/* uncompressed */
    text_png.key = (char *)software_key;	/* not const, unfortunately */
    text_png.text = software_text;
    text_png.text_length = strlen(software_text);
    info_ptr->text = &text_png;
    info_ptr->num_text = 1;

    /* write the file information */
    png_write_info(png_ptr, info_ptr);

    /* don't write the comments twice */
    info_ptr->num_text = 0;
    info_ptr->text = NULL;

    /* Write the contents of the image. */
    for (y = 0; y < height; ++y) {
	int x;

	for (x = 0; x < width; ++x) {
	    row[(x << 2)] = buf_ptr[x];
	    row[(x << 2) + 1] = buf_ptr[x + planestride];
	    row[(x << 2) + 2] = buf_ptr[x + planestride * 2];
	    row[(x << 2) + 3] = buf_ptr[x + planestride * 3];
	}
	png_write_row(png_ptr, row);
	buf_ptr += rowstride;
    }

    /* write the rest of the file */
    png_write_end(png_ptr, info_ptr);

  done:
    /* free the structures */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    gs_free(mem, row, rowbytes, 1, "png raster buffer");

    fclose (file);
    return code;
}
#endif


/**
 * pdf14_put_image: Put rendered image to target device.
 * @pdev: The PDF 1.4 rendering device.
 * @pgs: State for image draw operation.
 * @target: The target device.
 *
 * Puts the rendered image in @pdev's buffer to @target. This is called
 * as part of the sequence of popping the PDF 1.4 device filter.
 *
 * Return code: negative on error.
 **/
private	int
pdf14_put_image(pdf14_device *pdev, gs_imager_state *pis, gx_device *target)
{
    int code;
    gs_image1_t image;
    gs_matrix pmat;
    gx_image_enum_common_t *info;
    int width = pdev->width;
    int height = pdev->height;
    int y;
    pdf14_buf *buf = pdev->ctx->stack;
    int planestride = buf->planestride;
    int num_comp = buf->n_chan - 1;
    byte *buf_ptr = buf->data;
    byte *linebuf;
    gs_color_space cs;
    const byte bg = pdev->ctx->additive ? 255 : 0;

#ifdef DUMP_TO_PNG
    dump_planar_rgba(pdev->memory, buf_ptr, width, height,
		     buf->rowstride, buf->planestride);
#endif

#if 0
    /* Set graphics state device to target, so that image can set up
       the color mapping properly. */
    rc_increment(pdev);
    gs_setdevice_no_init(pgs, target);
#endif

    if_debug0('v', "[v]pdf14_put_image\n");
    /*
     * Set color space to either Gray, RGB, or CMYK in preparation for sending
     * an image.
     */
    switch (num_comp) {
	case 1:				/* DeviceGray */
	    gs_cspace_init_DeviceGray(pis->memory, &cs);
	    break;
	case 3:				/* DeviceRGB */
	    gs_cspace_init_DeviceRGB(pis->memory, &cs);
	    break;
	case 4:				/* DeviceCMYK */
	    gs_cspace_init_DeviceCMYK(pis->memory, &cs);
	    break;
	default:			/* Should never occur */
	    return_error(gs_error_rangecheck);
	    break;
    }
    gs_image_t_init_adjust(&image, &cs, false);
    image.ImageMatrix.xx = (float)width;
    image.ImageMatrix.yy = (float)height;
    image.Width = width;
    image.Height = height;
    image.BitsPerComponent = 8;
    pmat.xx = (float)width;
    pmat.xy = 0;
    pmat.yx = 0;
    pmat.yy = (float)height;
    pmat.tx = 0;
    pmat.ty = 0;
    code = dev_proc(target, begin_typed_image) (target,
						pis, &pmat,
						(gs_image_common_t *)&image,
						NULL, NULL, NULL,
						pis->memory, &info);
    if (code < 0)
	return code;

    linebuf = gs_alloc_bytes(pdev->memory, width * num_comp, "pdf14_put_image");
    for (y = 0; y < height; y++) {
	gx_image_plane_t planes;
	int x;
	int rows_used;

	for (x = 0; x < width; x++) {
	    byte comp, a;
	    int tmp, comp_num;

	    /* composite RGBA (or CMYKA, etc.) pixel with over solid background */
	    a = buf_ptr[x + planestride * num_comp];

	    if ((a + 1) & 0xfe) {
		a ^= 0xff;
		for (comp_num = 0; comp_num < num_comp; comp_num++) {
		    comp  = buf_ptr[x + planestride * comp_num];
		    tmp = ((bg - comp) * a) + 0x80;
		    comp += (tmp + (tmp >> 8)) >> 8;
		    linebuf[x * num_comp + comp_num] = comp;
		}
	    } else if (a == 0) {
		for (comp_num = 0; comp_num < num_comp; comp_num++) {
		    linebuf[x * num_comp + comp_num] = bg;
		}
	    } else {
		for (comp_num = 0; comp_num < num_comp; comp_num++) {
		    comp = buf_ptr[x + planestride * comp_num];
		    linebuf[x * num_comp + comp_num] = comp;
		}
	    }
	}

	planes.data = linebuf;
	planes.data_x = 0;
	planes.raster = width * num_comp;
	info->procs->plane_data(info, &planes, 1, &rows_used);
	/* todo: check return value */

	buf_ptr += buf->rowstride;
    }
    gs_free_object(pdev->memory, linebuf, "pdf14_put_image");

    info->procs->end_image(info, true);

#if 0
    /* Restore device in graphics state.*/
    gs_setdevice_no_init(pgs, (gx_device*) pdev);
    rc_decrement_only(pdev, "pdf_14_put_image");
#endif

    return code;
}

private	int
pdf14_close(gx_device *dev)
{
    pdf14_device *pdev = (pdf14_device *)dev;

    if (pdev->ctx) {
	pdf14_ctx_free(pdev->ctx);
	pdev->ctx = NULL;
    }
    return 0;
}

private	int
pdf14_output_page(gx_device * dev, int num_copies, int flush)
{
    pdf14_device * pdev = (pdf14_device *)dev;

    if (pdev->target != NULL)
	return (*dev_proc(pdev->target, output_page)) (pdev->target, num_copies, flush);
    return 0;
}

#define	COPY_PARAM(p) dev->p = target->p
#define	COPY_ARRAY_PARAM(p) memcpy(dev->p, target->p, sizeof(dev->p))

/*
 * Copy device parameters back from a target.  This copies all standard
 * parameters related to page size and resolution, but not any of the
 * color-related parameters, as the pdf14 device retains its own color
 * handling. This routine is parallel to gx_device_copy_params().
 */
private	void
gs_pdf14_device_copy_params(gx_device *dev, const gx_device *target)
{
	COPY_PARAM(width);
	COPY_PARAM(height);
	COPY_ARRAY_PARAM(MediaSize);
	COPY_ARRAY_PARAM(ImagingBBox);
	COPY_PARAM(ImagingBBox_set);
	COPY_ARRAY_PARAM(HWResolution);
	COPY_ARRAY_PARAM(MarginsHWResolution);
	COPY_ARRAY_PARAM(Margins);
	COPY_ARRAY_PARAM(HWMargins);
	COPY_PARAM(PageCount);
#undef COPY_ARRAY_PARAM
#undef COPY_PARAM
}

/*
 * This is a forwarding version of the put_params device proc.  It is only
 * used when the PDF 1.4 compositor devices are closed.  The routine will
 * check if the target device has closed and, if so, close itself.  The routine
 * also sync the device parameters.
 */
private	int
pdf14_forward_put_params(gx_device * dev, gs_param_list	* plist)
{
    pdf14_device * pdev = (pdf14_device *)dev;
    gx_device * tdev = pdev->target;
    int code = 0;

    if (tdev != 0 && (code = dev_proc(tdev, put_params)(tdev, plist)) >= 0) {
	gx_device_decache_colors(dev);
	if (!tdev->is_open)
	    code = gs_closedevice(dev);
	gx_device_copy_params(dev, tdev);
    }
    return code;
}

/*
 * The put_params method for the PDF 1.4 device will check if the
 * target device has closed and, if so, close itself.  Note:  This routine is
 * currently being used by both the pdf14_clist_device and the pdf_device.
 * Please make sure that any changes are either applicable to both devices
 * or clone the routine for each device.
 */
private	int
pdf14_put_params(gx_device * dev, gs_param_list	* plist)
{
    pdf14_device * pdev = (pdf14_device *)dev;
    gx_device * tdev = pdev->target;
    int code = 0;

    if (tdev != 0 && (code = dev_proc(tdev, put_params)(tdev, plist)) >= 0) {
	gx_device_decache_colors(dev);
	if (!tdev->is_open)
	    code = gs_closedevice(dev);
	gs_pdf14_device_copy_params(dev, tdev);
    }
    return code;
}

/*
 * Copy marking related parameters into the PDF 1.4 device structure for use
 * by pdf14_fill_rrectangle.
 */
private	void
pdf14_set_marking_params(gx_device *dev, const gs_imager_state *pis)
{
    pdf14_device * pdev = (pdf14_device *)dev;

    pdev->opacity = pis->opacity.alpha;
    pdev->shape = pis->shape.alpha;
    pdev->alpha = pis->opacity.alpha * pis->shape.alpha;
    pdev->blend_mode = pis->blend_mode;
    if_debug3('v', "[v]set_marking_params, opacity = %g, shape = %g, bm = %d\n",
	      pdev->opacity, pdev->shape, pis->blend_mode);
}

private	int
pdf14_fill_path(gx_device *dev,	const gs_imager_state *pis,
			   gx_path *ppath, const gx_fill_params *params,
			   const gx_drawing_color *pdcolor,
			   const gx_clip_path *pcpath)
{
    gs_imager_state new_is = *pis;

    /*
     * The blend operations are not idempotent.  Force non-idempotent
     * filling and stroking operations.
     */
    new_is.log_op |= lop_pdf14;
    pdf14_set_marking_params(dev, pis);
    return gx_default_fill_path(dev, &new_is, ppath, params, pdcolor, pcpath);
}

private	int
pdf14_stroke_path(gx_device *dev, const	gs_imager_state	*pis,
			     gx_path *ppath, const gx_stroke_params *params,
			     const gx_drawing_color *pdcolor,
			     const gx_clip_path *pcpath)
{
    gs_imager_state new_is = *pis;

    /*
     * The blend operations are not idempotent.  Force non-idempotent
     * filling and stroking operations.
     */
    new_is.log_op |= lop_pdf14;
    pdf14_set_marking_params(dev, pis);
    return gx_default_stroke_path(dev, &new_is, ppath, params, pdcolor,
				  pcpath);
}

private	int
pdf14_begin_typed_image(gx_device * dev, const gs_imager_state * pis,
			   const gs_matrix *pmat, const gs_image_common_t *pic,
			   const gs_int_rect * prect,
			   const gx_drawing_color * pdcolor,
			   const gx_clip_path * pcpath, gs_memory_t * mem,
			   gx_image_enum_common_t ** pinfo)
{
    pdf14_set_marking_params(dev, pis);
    return gx_default_begin_typed_image(dev, pis, pmat, pic, prect, pdcolor,
					pcpath, mem, pinfo);
}

private	void
pdf14_set_params(gs_imager_state * pis,	gx_device * dev,
				const gs_pdf14trans_params_t * pparams)
{
    if (pparams->changed & PDF14_SET_BLEND_MODE)
	pis->blend_mode = pparams->blend_mode;
    if (pparams->changed & PDF14_SET_TEXT_KNOCKOUT)
	pis->text_knockout = pparams->text_knockout;
    if (pparams->changed & PDF14_SET_SHAPE_ALPHA)
	pis->shape.alpha = pparams->shape.alpha;
    if (pparams->changed & PDF14_SET_OPACITY_ALPHA)
	pis->opacity.alpha = pparams->opacity.alpha;
    pdf14_set_marking_params(dev, pis);
}

/*
 * This open_device method for the PDF 1.4 compositor devices is only used
 * when these devices are disabled.  This routine is about as close to
 * a pure "forwarding" open_device operation as is possible. Its only
 * significant function is to ensure that the is_open field of the
 * PDF 1.4 compositor devices matches that of the target device.
 *
 * We assume this procedure is called only if the device is not already
 * open, and that gs_opendevice will take care of the is_open flag.
 */
private	int
pdf14_forward_open_device(gx_device * dev)
{
    gx_device_forward * pdev = (gx_device_forward *)dev;
    gx_device * tdev = pdev->target;
    int code = 0;

    /* The PDF 1.4 compositing devices must have a target */
    if (tdev == 0)
	return_error(gs_error_unknownerror);
    if ((code = gs_opendevice(tdev)) >= 0)
	gx_device_copy_params(dev, tdev);
    return code;
}

/*
 * Convert all device procs to be 'forwarding'.  The caller is responsible
 * for setting any device procs that should not be forwarded.
 */
private	void
pdf14_forward_device_procs(gx_device * dev)
{
    gx_device_forward * pdev = (gx_device_forward *)dev;

    /*
     * We are using gx_device_forward_fill_in_procs to set the various procs.
     * This will ensure that any new device procs are also set.  However that
     * routine only changes procs which are NULL.  Thus we start by setting all
     * procs to NULL.
     */
    memset(&(pdev->procs), 0, size_of(pdev->procs));
    gx_device_forward_fill_in_procs(pdev);
    /*
     * gx_device_forward_fill_in_procs does not forward all procs.
     * Set the remainding procs to also forward.
     */
    set_dev_proc(dev, close_device, gx_forward_close_device);
    set_dev_proc(dev, fill_rectangle, gx_forward_fill_rectangle);
    set_dev_proc(dev, tile_rectangle, gx_forward_tile_rectangle);
    set_dev_proc(dev, copy_mono, gx_forward_copy_mono);
    set_dev_proc(dev, copy_color, gx_forward_copy_color);
    set_dev_proc(dev, get_page_device, gx_forward_get_page_device);
    set_dev_proc(dev, strip_tile_rectangle, gx_forward_strip_tile_rectangle);
    set_dev_proc(dev, copy_alpha, gx_forward_copy_alpha);
    /* These are forwarding devices with minor tweaks. */
    set_dev_proc(dev, open_device, pdf14_forward_open_device);
    set_dev_proc(dev, put_params, pdf14_forward_put_params);
}

/*
 * Disable the PDF 1.4 compositor device.  Once created, the PDF 1.4
 * compositor device is never removed.  (We do not have a remove compositor
 * method.)  However it is no-op'ed when the PDF 1.4 device is popped.  This
 * routine implements that action.
 */
private	int
pdf14_disable_device(gx_device * dev)
{
    gx_device_forward * pdev = (gx_device_forward *)dev;

    if_debug0('v', "[v]pdf14_disable_device\n");
    dev->color_info = pdev->target->color_info;
    pdf14_forward_device_procs(dev);
    set_dev_proc(dev, create_compositor, pdf14_forward_create_compositor);
    return 0;
}

/*
 * The default color space for PDF 1.4 blend modes is based upon the process
 * color model of the output device.
 */
private	pdf14_default_colorspace_t
pdf14_determine_default_blend_cs(gx_device * pdev)
{
    if (pdev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE)
	/* Use DeviceCMYK for all subrtactive process color models. */
	return DeviceCMYK;
    else {
	/*
	 * Note:  We do not allow the SeparationOrder device parameter for
	 * additive devices.  Thus we always have 1 colorant for DeviceGray
	 * and 3 colorants for DeviceRGB.  We do not currently support
	 * blending in a DeviceGray color space.  Thus we oniy use DeviceRGB.
	 */
	return DeviceRGB;
    }
}

/*
 * the PDF 1.4 transparency spec says that color space for blending
 * operations can be based upon either a color space specified in the
 * group or a default value based upon the output device.  We are
 * currently only using a color space based upon the device.
 */
private	int
get_pdf14_device_proto(gx_device * dev,
		const pdf14_device ** pdevproto)
{
    pdf14_default_colorspace_t dev_cs =
		pdf14_determine_default_blend_cs(dev);

    switch (dev_cs) {
	case DeviceGray:
	    *pdevproto = &gs_pdf14_Gray_device;
	    break;
	case DeviceRGB:
	    *pdevproto = &gs_pdf14_RGB_device;
	    break;
	case DeviceCMYK:
	    *pdevproto = &gs_pdf14_CMYK_device;
	    break;
	default:			/* Should not occur */
	    return_error(gs_error_rangecheck);
    }
    return 0;
}

/*
 * Recreate the PDF 1.4 compositor device.  Once created, the PDF 1.4
 * compositor device is never removed.  (We do not have a remove compositor
 * method.)  However it is no-op'ed when the PDF 1.4 device is popped.  This
 * routine will re-enable the compositor if the PDF 1.4 device is pushed
 * again.
 */
private	int
pdf14_recreate_device(gs_memory_t *mem,	gs_imager_state	* pis,
				gx_device * dev)
{
    pdf14_device * pdev = (pdf14_device *)dev;
    gx_device * target = pdev->target;
    const pdf14_device * dev_proto;
    int code;

    if_debug0('v', "[v]pdf14_recreate_device\n");

    /*
     * We will not use the entire prototype device but we will set the
     * color related info and the device procs to match the prototype.
     */
    code = get_pdf14_device_proto(target, &dev_proto);
    if (code < 0)
	return code;
    pdev->color_info = dev_proto->color_info;
    pdev->procs = dev_proto->procs;
    gx_device_fill_in_procs(dev);
    check_device_separable((gx_device *)pdev);

    return code;
}

/*
 * Implement the various operations that can be specified via the PDF 1.4
 * create compositor request.
 */
private	int
gx_update_pdf14_compositor(gx_device * pdev, gs_imager_state * pis,
    const gs_pdf14trans_t * pdf14pct, gs_memory_t * mem )
{
    pdf14_device *p14dev = (pdf14_device *)pdev;
    int code = 0;

    switch (pdf14pct->params.pdf14_op) {
	default:			/* Should not occur. */
	    break;
	case PDF14_PUSH_DEVICE:
	    p14dev->blend_mode = 0;
	    p14dev->opacity = p14dev->shape = 0.0;
	    pdf14_recreate_device(mem, pis, pdev);
	    break;
	case PDF14_POP_DEVICE:
	    pis->get_cmap_procs = p14dev->save_get_cmap_procs;
	    gx_set_cmap_procs(pis, p14dev->target);
	    code = pdf14_put_image(p14dev, pis, p14dev->target);
	    pdf14_disable_device(pdev);
	    pdf14_close(pdev);
	    break;
	case PDF14_BEGIN_TRANS_GROUP:
	    code = gx_begin_transparency_group(pis, pdev, &pdf14pct->params);
	    break;
	case PDF14_END_TRANS_GROUP:
	    code = gx_end_transparency_group(pis, pdev);
	    break;
	case PDF14_INIT_TRANS_MASK:
	    code = gx_init_transparency_mask(pis, &pdf14pct->params);
	    break;
	case PDF14_BEGIN_TRANS_MASK:
	    code = gx_begin_transparency_mask(pis, pdev, &pdf14pct->params);
	    break;
	case PDF14_END_TRANS_MASK:
	    code = gx_end_transparency_mask(pis, pdev, &pdf14pct->params);
	    break;
	case PDF14_SET_BLEND_PARAMS:
	    pdf14_set_params(pis, pdev, &pdf14pct->params);
	    break;
    }
    return code;
}

/*
 * The PDF 1.4 compositor is never removed.  (We do not have a 'remove
 * compositor' method.  However the compositor is disabled when we are not
 * doing a page which uses PDF 1.4 transparency.  This routine is only active
 * when the PDF 1.4 compositor is 'disabled'.  It checks for reenabling the
 * PDF 1.4 compositor.  Otherwise it simply passes create compositor requests
 * to the targer.
 */
private	int
pdf14_forward_create_compositor(gx_device * dev, gx_device * * pcdev,
	const gs_composite_t * pct, gs_imager_state * pis,
	gs_memory_t * mem)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    gx_device * tdev = pdev->target;
    gx_device * ndev;
    int code = 0;

    *pcdev = dev;
    if (gs_is_pdf14trans_compositor(pct)) {
	const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pct;

	if (pdf14pct->params.pdf14_op == PDF14_PUSH_DEVICE)
	    return gx_update_pdf14_compositor(dev, pis, pdf14pct, mem);
	return 0;
    }
    code = dev_proc(tdev, create_compositor)(tdev, &ndev, pct, pis, mem);
    if (code < 0)
	return code;
    pdev->target = ndev;
    return 0;
}

/*
 * The PDF 1.4 compositor can be handled directly, so just set *pcdev = dev
 * and return. Since the gs_pdf14_device only supports the high-level routines
 * of the interface, don't bother trying to handle any other compositor.
 */
private	int
pdf14_create_compositor(gx_device * dev, gx_device * * pcdev,
	const gs_composite_t * pct, gs_imager_state * pis,
	gs_memory_t * mem)
{
    if (gs_is_pdf14trans_compositor(pct)) {
	const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pct;

	*pcdev = dev;
	return gx_update_pdf14_compositor(dev, pis, pdf14pct, mem);
    } else if (gs_is_overprint_compositor(pct)) {
	*pcdev = dev;
	return 0;
    } else
	return gx_no_create_compositor(dev, pcdev, pct, pis, mem);
}

private	int
pdf14_text_begin(gx_device * dev, gs_imager_state * pis,
		 const gs_text_params_t * text, gs_font * font,
		 gx_path * path, const gx_device_color * pdcolor,
		 const gx_clip_path * pcpath, gs_memory_t * memory,
		 gs_text_enum_t ** ppenum)
{
    int code;
    gs_text_enum_t *penum;

    if_debug0('v', "[v]pdf14_text_begin\n");
    pdf14_set_marking_params(dev, pis);
    code = gx_default_text_begin(dev, pis, text, font, path, pdcolor, pcpath,
				 memory, &penum);
    if (code < 0)
	return code;
    *ppenum = (gs_text_enum_t *)penum;
    return code;
}

private	int
pdf14_fill_rectangle(gx_device * dev,
		    int x, int y, int w, int h, gx_color_index color)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    pdf14_buf *buf = pdev->ctx->stack;

    fit_fill_xywh(dev, x, y, w, h);
    if (w <= 0 || h <= 0)
	return 0;
    if (buf->knockout)
	return pdf14_mark_fill_rectangle_ko_simple(dev, x, y, w, h, color);
    else
	return pdf14_mark_fill_rectangle(dev, x, y, w, h, color);
}

private	int
pdf14_begin_transparency_group(gx_device *dev,
			      const gs_transparency_group_params_t *ptgp,
			      const gs_rect *pbbox,
			      gs_imager_state *pis,
			      gs_transparency_state_t **ppts,
			      gs_memory_t *mem)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    double alpha = pis->opacity.alpha * pis->shape.alpha;
    gs_rect dev_bbox;
    gs_int_rect rect;
    int code;

    code = gs_bbox_transform(pbbox, &ctm_only(pis), &dev_bbox);
    if (code < 0)
	return code;
    rect.p.x = (int)floor(dev_bbox.p.x);
    rect.p.y = (int)floor(dev_bbox.p.y);
    rect.q.x = (int)ceil(dev_bbox.q.x);
    rect.q.y = (int)ceil(dev_bbox.q.y);
    rect_intersect(rect, pdev->ctx->rect);
    if_debug4('v', "[v]begin_transparency_group, I = %d, K = %d, alpha = %g, bm = %d\n",
	      ptgp->Isolated, ptgp->Knockout, alpha, pis->blend_mode);
    code = pdf14_push_transparency_group(pdev->ctx, &rect,
					 ptgp->Isolated, ptgp->Knockout,
					 (byte)floor (255 * alpha + 0.5),
					 (byte)floor (255 * pis->shape.alpha + 0.5),
					 pis->blend_mode);
    return code;
}

private	int
pdf14_end_transparency_group(gx_device *dev,
			      gs_imager_state *pis,
			      gs_transparency_state_t **ppts)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    int code;

    if_debug0('v', "[v]end_transparency_group\n");
    code = pdf14_pop_transparency_group(pdev->ctx);
    return code;
}

private	int
pdf14_begin_transparency_mask(gx_device	*dev,
			      const gx_transparency_mask_params_t *ptmp,
			      const gs_rect *pbbox,
			      gs_imager_state *pis,
			      gs_transparency_state_t **ppts,
			      gs_memory_t *mem)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    byte bg_alpha = 0;
    byte *transfer_fn = (byte *)gs_alloc_bytes(pdev->ctx->memory, 256,
					       "pdf14_push_transparency_mask");

    if (ptmp->Background_components)
	bg_alpha = (int)(255 * ptmp->Background[0] + 0.5);
    if_debug1('v', "begin transparency mask, bg_alpha = %d\n", bg_alpha);
    memcpy(transfer_fn, ptmp->transfer_fn, size_of(ptmp->transfer_fn));
    return pdf14_push_transparency_mask(pdev->ctx, &pdev->ctx->rect, bg_alpha,
					transfer_fn);
}

private	int
pdf14_end_transparency_mask(gx_device *dev,
			  gs_transparency_mask_t **pptm)
{
    pdf14_device *pdev = (pdf14_device *)dev;

    if_debug0('v', "end transparency mask!\n");
    return pdf14_pop_transparency_mask(pdev->ctx);
}

private	int
pdf14_mark_fill_rectangle(gx_device * dev,
			 int x, int y, int w, int h, gx_color_index color)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    pdf14_buf *buf = pdev->ctx->stack;
    int i, j, k;
    byte *line, *dst_ptr;
    byte src[PDF14_MAX_PLANES];
    byte dst[PDF14_MAX_PLANES];
    gs_blend_mode_t blend_mode = pdev->blend_mode;
    bool additive = pdev->ctx->additive;
    int rowstride = buf->rowstride;
    int planestride = buf->planestride;
    bool has_alpha_g = buf->has_alpha_g;
    bool has_shape = buf->has_shape;
    int num_chan = buf->n_chan;
    int num_comp = num_chan - 1;
    int shape_off = num_chan * planestride;
    int alpha_g_off = shape_off + (has_shape ? planestride : 0);
    byte shape = 0; /* Quiet compiler. */
    byte src_alpha;

    if_debug7('v', "[v]pdf14_mark_fill_rectangle, (%d, %d), %d x %d color = %lx  bm %d, nc %d,\n", x, y, w, h, color, blend_mode, num_chan);

    /* Complement the components for subtractive color spaces */
    if (additive) {
	for (i = num_comp - 1; i >= 0; i--) {
	    src[i] = (byte)(color & 0xff);
	    color >>= 8;
	}
    }
    else {
	for (i = num_comp - 1; i >= 0; i--) {
	    src[i] = (byte)(0xff - (color & 0xff));
	    color >>= 8;
	}
    }
    src_alpha = src[num_comp] = (byte)floor (255 * pdev->alpha + 0.5);
    if (has_shape)
	shape = (byte)floor (255 * pdev->shape + 0.5);

    if (x < buf->rect.p.x) x = buf->rect.p.x;
    if (y < buf->rect.p.y) y = buf->rect.p.y;
    if (x + w > buf->rect.q.x) w = buf->rect.q.x - x;
    if (y + h > buf->rect.q.y) h = buf->rect.q.y - y;

    if (x < buf->bbox.p.x) buf->bbox.p.x = x;
    if (y < buf->bbox.p.y) buf->bbox.p.y = y;
    if (x + w > buf->bbox.q.x) buf->bbox.q.x = x + w;
    if (y + h > buf->bbox.q.y) buf->bbox.q.y = y + h;

    line = buf->data + (x - buf->rect.p.x) + (y - buf->rect.p.y) * rowstride;

    for (j = 0; j < h; ++j) {
	dst_ptr = line;
	for (i = 0; i < w; ++i) {
	    /* Complement the components for subtractive color spaces */
	    if (additive) {
		for (k = 0; k < num_chan; ++k)
		    dst[k] = dst_ptr[k * planestride];
	    }
	    else { /* Complement the components for subtractive color spaces */
		for (k = 0; k < num_comp; ++k)
		    dst[k] = 255 - dst_ptr[k * planestride];
		dst[num_comp] = dst_ptr[num_comp * planestride];
	    }
	    art_pdf_composite_pixel_alpha_8(dst, src, num_comp, blend_mode);
	    /* Complement the results for subtractive color spaces */
	    if (additive) {
		for (k = 0; k < num_chan; ++k)
		    dst_ptr[k * planestride] = dst[k];
	    }
	    else {
		for (k = 0; k < num_comp; ++k)
		    dst_ptr[k * planestride] = 255 - dst[k];
		dst_ptr[num_comp * planestride] = dst[num_comp];
	    }
	    if (has_alpha_g) {
		int tmp = (255 - dst_ptr[alpha_g_off]) * (255 - src_alpha) + 0x80;
		dst_ptr[alpha_g_off] = 255 - ((tmp + (tmp >> 8)) >> 8);
	    }
	    if (has_shape) {
		int tmp = (255 - dst_ptr[shape_off]) * (255 - shape) + 0x80;
		dst_ptr[shape_off] = 255 - ((tmp + (tmp >> 8)) >> 8);
	    }
	    ++dst_ptr;
	}
	line += rowstride;
    }
    return 0;
}

private	int
pdf14_mark_fill_rectangle_ko_simple(gx_device *	dev,
				   int x, int y, int w, int h, gx_color_index color)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    pdf14_buf *buf = pdev->ctx->stack;
    int i, j, k;
    byte *line, *dst_ptr;
    byte src[PDF14_MAX_PLANES];
    byte dst[PDF14_MAX_PLANES];
    int rowstride = buf->rowstride;
    int planestride = buf->planestride;
    int num_chan = buf->n_chan;
    int num_comp = num_chan - 1;
    int shape_off = num_chan * planestride;
    bool has_shape = buf->has_shape;
    byte opacity;
    bool additive = pdev->ctx->additive;

    if_debug6('v', "[v]pdf14_mark_fill_rectangle_ko_simple, (%d, %d), %d x %d color = %lx  bm %d, nc %d,\n", x, y, w, h, color, num_chan);

    /* Complement the components for subtractive color spaces */
    if (additive) {
	for (i = num_comp - 1; i >= 0; i--) {
	    src[i] = (byte)(color & 0xff);
	    color >>= 8;
	}
    }
    else {
	for (i = num_comp - 1; i >= 0; i--) {
	    src[i] = (byte)(0xff - (color & 0xff));
	    color >>= 8;
	}
    }
    src[num_comp] = (byte)floor (255 * pdev->alpha + 0.5);
    opacity = (byte)floor (255 * pdev->opacity + 0.5);

    if (x < buf->rect.p.x) x = buf->rect.p.x;
    if (y < buf->rect.p.y) y = buf->rect.p.y;
    if (x + w > buf->rect.q.x) w = buf->rect.q.x - x;
    if (y + h > buf->rect.q.y) h = buf->rect.q.y - y;

    if (x < buf->bbox.p.x) buf->bbox.p.x = x;
    if (y < buf->bbox.p.y) buf->bbox.p.y = y;
    if (x + w > buf->bbox.q.x) buf->bbox.q.x = x + w;
    if (y + h > buf->bbox.q.y) buf->bbox.q.y = y + h;

    line = buf->data + (x - buf->rect.p.x) + (y - buf->rect.p.y) * rowstride;

    for (j = 0; j < h; ++j) {
	dst_ptr = line;
	for (i = 0; i < w; ++i) {
	    /* Complement the components for subtractive color spaces */
	    if (additive) {
		for (k = 0; k < num_chan; ++k)
		    dst[k] = dst_ptr[k * planestride];
	    }
	    else {
		for (k = 0; k < num_comp; ++k)
		    dst[k] = 255 - dst_ptr[k * planestride];
		dst[num_comp] = dst_ptr[num_comp * planestride];
	    }
	    art_pdf_composite_knockout_simple_8(dst,
		has_shape ? dst_ptr + shape_off : NULL, src, num_comp, opacity);
	    /* Complement the results for subtractive color spaces */
	    if (additive) {
		for (k = 0; k < num_chan; ++k)
		    dst_ptr[k * planestride] = dst[k];
	    }
	    else {
		for (k = 0; k < num_comp; ++k)
		    dst_ptr[k * planestride] = 255 - dst[k];
		dst_ptr[num_comp * planestride] = dst[num_comp];
	    }
	    ++dst_ptr;
	}
	line += rowstride;
    }
    return 0;
}

/**
 * Here we have logic to override the cmap_procs with versions that
 * do not apply the transfer function. These copies should track the
 * versions in gxcmap.c.
 **/

private	cmap_proc_gray(pdf14_cmap_gray_direct);
private	cmap_proc_rgb(pdf14_cmap_rgb_direct);
private	cmap_proc_cmyk(pdf14_cmap_cmyk_direct);
private	cmap_proc_rgb_alpha(pdf14_cmap_rgb_alpha_direct);
private	cmap_proc_separation(pdf14_cmap_separation_direct);
private	cmap_proc_devicen(pdf14_cmap_devicen_direct);
private	cmap_proc_is_halftoned(pdf14_cmap_is_halftoned);

private	const gx_color_map_procs pdf14_cmap_many = {
     pdf14_cmap_gray_direct,
     pdf14_cmap_rgb_direct,
     pdf14_cmap_cmyk_direct,
     pdf14_cmap_rgb_alpha_direct,
     pdf14_cmap_separation_direct,
     pdf14_cmap_devicen_direct,
     pdf14_cmap_is_halftoned
    };

/**
 * Note: copied from gxcmap.c because it's inlined.
 **/
private	inline void
map_components_to_colorants(const frac * pcc,
	const gs_devicen_color_map * pcolor_component_map, frac * plist)
{
    int i = pcolor_component_map->num_colorants - 1;
    int pos;

    /* Clear all output colorants first */
    for (; i >= 0; i--) {
	plist[i] = frac_0;
    }

    /* Map color components into output list */
    for (i = pcolor_component_map->num_components - 1; i >= 0; i--) {
	pos = pcolor_component_map->color_map[i];
	if (pos >= 0)
	    plist[pos] = pcc[i];
    }
}

private	void
pdf14_cmap_gray_direct(frac gray, gx_device_color * pdc, const gs_imager_state * pis,
		 gx_device * dev, gs_color_select_t select)
{
    int i, ncomps = dev->color_info.num_components;
    frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color;

    /* map to the color model */
    dev_proc(dev, get_color_mapping_procs)(dev)->map_gray(dev, gray, cm_comps);

    for (i = 0; i < ncomps; i++)
	cv[i] = frac2cv(cm_comps[i]);

    /* encode as a color index */
    color = dev_proc(dev, encode_color)(dev, cv);

    /* check if the encoding was successful; we presume failure is rare */
    if (color != gx_no_color_index)
	color_set_pure(pdc, color);
}


private	void
pdf14_cmap_rgb_direct(frac r, frac g, frac b, gx_device_color *	pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    int i, ncomps = dev->color_info.num_components;
    frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color;

    /* map to the color model */
    dev_proc(dev, get_color_mapping_procs)(dev)->map_rgb(dev, pis, r, g, b, cm_comps);

    for (i = 0; i < ncomps; i++)
	cv[i] = frac2cv(cm_comps[i]);

    /* encode as a color index */
    color = dev_proc(dev, encode_color)(dev, cv);

    /* check if the encoding was successful; we presume failure is rare */
    if (color != gx_no_color_index)
	color_set_pure(pdc, color);
}

private	void
pdf14_cmap_cmyk_direct(frac c, frac m, frac y, frac k, gx_device_color * pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    int i, ncomps = dev->color_info.num_components;
    frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color;

    /* map to the color model */
    dev_proc(dev, get_color_mapping_procs)(dev)->map_cmyk(dev, c, m, y, k, cm_comps);

    for (i = 0; i < ncomps; i++)
	cv[i] = frac2cv(cm_comps[i]);

    color = dev_proc(dev, encode_color)(dev, cv);
    if (color != gx_no_color_index) 
	color_set_pure(pdc, color);
}

private	void
pdf14_cmap_rgb_alpha_direct(frac r, frac g, frac b, frac alpha,	gx_device_color	* pdc,
     const gs_imager_state * pis, gx_device * dev, gs_color_select_t select)
{
    int i, ncomps = dev->color_info.num_components;
    frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value cv_alpha, cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color;

    /* map to the color model */
    dev_proc(dev, get_color_mapping_procs)(dev)->map_rgb(dev, pis, r, g, b, cm_comps);

    /* pre-multiply to account for the alpha weighting */
    if (alpha != frac_1) {
#ifdef PREMULTIPLY_TOWARDS_WHITE
	frac alpha_bias = frac_1 - alpha;
#else
	frac alpha_bias = 0;
#endif

	for (i = 0; i < ncomps; i++)
	    cm_comps[i] = (frac)((long)cm_comps[i] * alpha) / frac_1 + alpha_bias;
    }

    for (i = 0; i < ncomps; i++)
	cv[i] = frac2cv(cm_comps[i]);

    /* encode as a color index */
    if (dev_proc(dev, map_rgb_alpha_color) != gx_default_map_rgb_alpha_color &&
	 (cv_alpha = frac2cv(alpha)) != gx_max_color_value)
	color = dev_proc(dev, map_rgb_alpha_color)(dev, cv[0], cv[1], cv[2], cv_alpha);
    else
	color = dev_proc(dev, encode_color)(dev, cv);

    /* check if the encoding was successful; we presume failure is rare */
    if (color != gx_no_color_index)
	color_set_pure(pdc, color);
}


private	void
pdf14_cmap_separation_direct(frac all, gx_device_color * pdc, const gs_imager_state * pis,
		 gx_device * dev, gs_color_select_t select)
{
    int i, ncomps = dev->color_info.num_components;
    bool additive = dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE;
    frac comp_value = all;
    frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color;

    if (pis->color_component_map.sep_type == SEP_ALL) {
	/*
	 * Invert the photometric interpretation for additive
	 * color spaces because separations are always subtractive.
	 */
	if (additive)
	    comp_value = frac_1 - comp_value;

	/* Use the "all" value for all components */
	i = pis->color_component_map.num_colorants - 1;
	for (; i >= 0; i--)
	    cm_comps[i] = comp_value;
    }
    else {
	/* map to the color model */
	map_components_to_colorants(&comp_value, &(pis->color_component_map), cm_comps);
    }

    /* apply the transfer function(s); convert to color values */
    if (additive)
	for (i = 0; i < ncomps; i++)
	    cv[i] = frac2cv(gx_map_color_frac(pis,
				cm_comps[i], effective_transfer[i]));
    else
	for (i = 0; i < ncomps; i++)
	    cv[i] = frac2cv(frac_1 - gx_map_color_frac(pis,
			(frac)(frac_1 - cm_comps[i]), effective_transfer[i]));

    /* encode as a color index */
    color = dev_proc(dev, encode_color)(dev, cv);

    /* check if the encoding was successful; we presume failure is rare */
    if (color != gx_no_color_index)
	color_set_pure(pdc, color);
}


private	void
pdf14_cmap_devicen_direct(const	frac * pcc, 
    gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
    gs_color_select_t select)
{
    int i, ncomps = dev->color_info.num_components;
    frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color;

    /* map to the color model */
    map_components_to_colorants(pcc, &(pis->color_component_map), cm_comps);;

    /* apply the transfer function(s); convert to color values */
    if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE)
	for (i = 0; i < ncomps; i++)
	    cv[i] = frac2cv(gx_map_color_frac(pis,
				cm_comps[i], effective_transfer[i]));
    else
	for (i = 0; i < ncomps; i++)
	    cv[i] = frac2cv(frac_1 - gx_map_color_frac(pis,
			(frac)(frac_1 - cm_comps[i]), effective_transfer[i]));

    /* encode as a color index */
    color = dev_proc(dev, encode_color)(dev, cv);

    /* check if the encoding was successful; we presume failure is rare */
    if (color != gx_no_color_index)
	color_set_pure(pdc, color);
}

private	bool
pdf14_cmap_is_halftoned(const gs_imager_state *	pis, gx_device * dev)
{
    return false;
}

private	const gx_color_map_procs *
pdf14_get_cmap_procs(const gs_imager_state *pis, const gx_device * dev)
{
    /* The pdf14 marking device itself is always continuous tone. */
    return &pdf14_cmap_many;
}


int
gs_pdf14_device_push(gs_memory_t *mem, gs_imager_state * pis,
				gx_device ** pdev, gx_device * target)
{
    const pdf14_device * dev_proto;
    pdf14_device *p14dev;
    int code;

    if_debug0('v', "[v]gs_pdf14_device_push\n");

    code = get_pdf14_device_proto(target, &dev_proto);
    if (code < 0)
	return code;
    code = gs_copydevice((gx_device **) &p14dev,
			 (const gx_device *) dev_proto, mem);
    if (code < 0)
	return code;

    check_device_separable((gx_device *)p14dev);
    gx_device_fill_in_procs((gx_device *)p14dev);

    gs_pdf14_device_copy_params((gx_device *)p14dev, target);

    rc_assign(p14dev->target, target, "gs_pdf14_device_push");

    p14dev->save_get_cmap_procs = pis->get_cmap_procs;
    pis->get_cmap_procs = pdf14_get_cmap_procs;
    gx_set_cmap_procs(pis, (gx_device *)p14dev);

    code = dev_proc((gx_device *) p14dev, open_device) ((gx_device *) p14dev);
    *pdev = (gx_device *) p14dev;
    pdf14_set_marking_params((gx_device *)p14dev, pis);
    return code;
}

/*
 * In a modest violation of good coding practice, the gs_composite_common
 * fields are "known" to be simple (contain no pointers to garbage
 * collected memory), and we also know the gs_pdf14trans_params_t structure
 * to be simple, so we just create a trivial structure descriptor for the
 * entire gs_pdf14trans_s structure.
 */
#define	private_st_gs_pdf14trans_t()\
  gs_private_st_ptrs1(st_pdf14trans, gs_pdf14trans_t, "gs_pdf14trans_t",\
      st_pdf14trans_enum_ptrs, st_pdf14trans_reloc_ptrs, params.transfer_function)

/* GC descriptor for gs_pdf14trans_t */
private_st_gs_pdf14trans_t();

/*
 * Check for equality of two PDF 1.4 transparency compositor objects.
 *
 * We are currently always indicating that PDF 1.4 transparency compositors are
 * equal.  Two transparency compositors may have teh same data but still
 * represent separate actions.  (E.g. two PDF14_BEGIN_TRANS_GROUP compositor
 * operations in a row mean that we are creating a group inside of a group.
 */
private	bool
c_pdf14trans_equal(const gs_composite_t	* pct0,	const gs_composite_t * pct1)
{
    return false;
}

#ifdef DEBUG
static char * pdf14_opcode_names[] = PDF14_OPCODE_NAMES;
#endif

#define	put_value(dp, value)\
    memcpy(dp, &value, sizeof(value));\
    dp += sizeof(value)

/*
 * Convert a PDF 1.4 transparency compositor to string form for use by the command
 * list device.
 */
private	int
c_pdf14trans_write(const gs_composite_t	* pct, byte * data, uint * psize)
{
    const gs_pdf14trans_params_t * pparams = &((const gs_pdf14trans_t *)pct)->params;
    int need, avail = *psize;
	/* Must be large enough for largest data struct */
    byte buf[21 + sizeof(pparams->Background)
		+ sizeof(pparams->GrayBackground) + sizeof(pparams->bbox)];
    byte * pbuf = buf;
    int opcode = pparams->pdf14_op;
    int mask_size = 0;

    /* Write PDF 1.4 compositor data into the clist */

    *pbuf++ = opcode;			/* 1 byte */
    switch (opcode) {
	default:			/* Should not occur. */
	    break;
	case PDF14_PUSH_DEVICE:
	case PDF14_POP_DEVICE:
	case PDF14_END_TRANS_GROUP:
	case PDF14_END_TRANS_MASK:
	    break;			/* No data */
	case PDF14_BEGIN_TRANS_GROUP:
	    /*
	     * The bbox data is floating point.  We are not currently using it.
	     * So we are not currently putting it into the clist.  We are also
	     * not using the color space.
	     */
	    *pbuf++ = (pparams->Isolated & 1) + ((pparams->Knockout & 1) << 1);
	    *pbuf++ = pparams->blend_mode;
	    put_value(pbuf, pparams->opacity.alpha);
	    put_value(pbuf, pparams->shape.alpha);
	    put_value(pbuf, pparams->bbox);	    
	    break;
	case PDF14_INIT_TRANS_MASK:
	    *pbuf++ = pparams->csel;
	    break;
	case PDF14_BEGIN_TRANS_MASK:
	    put_value(pbuf, pparams->subtype);
	    *pbuf++ = pparams->function_is_identity;
	    *pbuf++ = pparams->Background_components;
	    if (pparams->Background_components) {
		const int l = sizeof(pparams->Background[0]) * pparams->Background_components;

		memcpy(pbuf, pparams->Background, l);
		pbuf += l;
		memcpy(pbuf, &pparams->GrayBackground, sizeof(pparams->GrayBackground));
		pbuf += sizeof(pparams->GrayBackground);
	    }
	    if (!pparams->function_is_identity)
		mask_size = sizeof(pparams->transfer_fn);
	    break;
	case PDF14_SET_BLEND_PARAMS:
	    *pbuf++ = pparams->changed;
	    if (pparams->changed & PDF14_SET_BLEND_MODE)
		*pbuf++ = pparams->blend_mode;
	    if (pparams->changed & PDF14_SET_TEXT_KNOCKOUT)
		*pbuf++ = pparams->text_knockout;
	    if (pparams->changed & PDF14_SET_OPACITY_ALPHA)
		put_value(pbuf, pparams->opacity.alpha);
	    if (pparams->changed & PDF14_SET_SHAPE_ALPHA)
		put_value(pbuf, pparams->shape.alpha);
	    break;
    }
#undef put_value

    /* check for fit */
    need = (pbuf - buf) + mask_size;
    *psize = need;
    if (need > avail)
	return_error(gs_error_rangecheck);
    /* Copy our serialzed data into the output buffer */
    memcpy(data, buf, need - mask_size);
    if (mask_size)	/* Include the transfer mask data if present */
	memcpy(data + need - mask_size, pparams->transfer_fn, mask_size);
    if_debug2('v', "[v] c_pdf14trans_write: opcode = %s need = %d\n",
				pdf14_opcode_names[opcode], need);
    return 0;
}

/* Function prototypes */
int gs_create_pdf14trans( gs_composite_t ** ppct,
		const gs_pdf14trans_params_t * pparams,
		gs_memory_t * mem );

#define	read_value(dp, value)\
    memcpy(&value, dp, sizeof(value));\
    dp += sizeof(value)

/*
 * Convert the string representation of the PDF 1.4 tranparency  parameter
 * into the full compositor.
 */
private	int
c_pdf14trans_read(gs_composite_t * * ppct, const byte *	data,
				uint size, gs_memory_t * mem )
{
    gs_pdf14trans_params_t params = {0};
    const byte * start = data;
    int used, code = 0;

    if (size < 1)
	return_error(gs_error_rangecheck);

    /* Read PDF 1.4 compositor data from the clist */
    params.pdf14_op = *data++;
    if_debug2('v', "[v] c_pdf14trans_read: opcode = %s  avail = %d",
				pdf14_opcode_names[params.pdf14_op], size);
    switch (params.pdf14_op) {
	default:			/* Should not occur. */
	    break;
	case PDF14_PUSH_DEVICE:
	case PDF14_POP_DEVICE:
	case PDF14_END_TRANS_GROUP:
	    break;			/* No data */
	case PDF14_BEGIN_TRANS_GROUP:
	    /*
	     * We are currently not using the bbox or the colorspace so they were
	     * not placed in the clist
	     */
	    params.Isolated = (*data) & 1;
	    params.Knockout = (*data++ >> 1) & 1;
	    params.blend_mode = *data++;
	    read_value(data, params.opacity.alpha);
	    read_value(data, params.shape.alpha);
	    read_value(data, params.bbox);
	    break;
	case PDF14_INIT_TRANS_MASK:
	    params.csel = *data++;
	    break;
	case PDF14_BEGIN_TRANS_MASK:
	    read_value(data, params.subtype);
	    params.function_is_identity = *data++;
	    params.Background_components = *data++;
	    if (params.Background_components) {
		const int l = sizeof(params.Background[0]) * params.Background_components;

		memcpy(params.Background, data, l);
		data += l;
		memcpy(&params.GrayBackground, data, sizeof(params.GrayBackground));
		data += sizeof(params.GrayBackground);
	    }
	    if (params.function_is_identity) {
		int i;

		for (i = 0; i < MASK_TRANSFER_FUNCTION_SIZE; i++) {
		    params.transfer_fn[i] = (byte)floor(i *
			(255.0 / (MASK_TRANSFER_FUNCTION_SIZE - 1)) + 0.5);
		}
	    } else {
		read_value(data, params.transfer_fn);
	    }
	    break;
	case PDF14_END_TRANS_MASK:
	    break;			/* No data */
	case PDF14_SET_BLEND_PARAMS:
	    params.changed = *data++;
	    if (params.changed & PDF14_SET_BLEND_MODE)
		params.blend_mode = *data++;
	    if (params.changed & PDF14_SET_TEXT_KNOCKOUT)
		params.text_knockout = *data++;
	    if (params.changed & PDF14_SET_OPACITY_ALPHA)
		read_value(data, params.opacity.alpha);
	    if (params.changed & PDF14_SET_SHAPE_ALPHA)
		read_value(data, params.shape.alpha);
	    break;
    }
    code = gs_create_pdf14trans(ppct, &params, mem);
    if (code < 0)
	return code;
    used = data - start;
    if_debug1('v', "  used = %d\n", used);
    return used;
}

/*
 * Create a PDF 1.4 transparency compositor.
 *
 * Note that this routine will be called only if the device is not already
 * a PDF 1.4 transparency compositor.
 */
private	int
c_pdf14trans_create_default_compositor(const gs_composite_t * pct,
    gx_device ** pp14dev, gx_device * tdev, gs_imager_state * pis,
    gs_memory_t * mem)
{
    const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pct;
    gx_device * p14dev = NULL;
    int code = 0;

    /*
     * We only handle the push operation.  All other operations are ignored.
     */
    switch (pdf14pct->params.pdf14_op) {
	case PDF14_PUSH_DEVICE:
	    code = gs_pdf14_device_push(mem, pis, &p14dev, tdev);
	    *pp14dev = p14dev;
	    break;
	default:
	    *pp14dev = tdev;
	    break;
    }
    return code;
}

private	composite_clist_write_update(c_pdf14trans_clist_write_update);
private	composite_clist_read_update(c_pdf14trans_clist_read_update);

/*
 * Methods for the PDF 1.4 transparency compositor
 *
 * Note:  We have two set of methods.  They are the same except for the
 * composite_clist_write_update method.  Once the clist write device is created,
 * we use the second set of procedures.  This prevents the creation of multiple
 * PDF 1.4 clist write compositor devices being chained together.
 */
const gs_composite_type_t   gs_composite_pdf14trans_type = {
    GX_COMPOSITOR_PDF14_TRANS,
    {
	c_pdf14trans_create_default_compositor, /* procs.create_default_compositor */
	c_pdf14trans_equal,                      /* procs.equal */
	c_pdf14trans_write,                      /* procs.write */
	c_pdf14trans_read,                       /* procs.read */
		/* Create a PDF 1.4 clist write device */
	c_pdf14trans_clist_write_update,   /* procs.composite_clist_write_update */
	c_pdf14trans_clist_read_update	   /* procs.composite_clist_reade_update */
    }                                            /* procs */
};

const gs_composite_type_t   gs_composite_pdf14trans_no_clist_writer_type = {
    GX_COMPOSITOR_PDF14_TRANS,
    {
	c_pdf14trans_create_default_compositor, /* procs.create_default_compositor */
	c_pdf14trans_equal,                      /* procs.equal */
	c_pdf14trans_write,                      /* procs.write */
	c_pdf14trans_read,                       /* procs.read */
		/* The PDF 1.4 clist writer already exists, Do not create it. */
	gx_default_composite_clist_write_update, /* procs.composite_clist_write_update */
	c_pdf14trans_clist_read_update	   /* procs.composite_clist_reade_update */
    }                                            /* procs */
};

/*
 * Verify that a compositor data structure is for the PDF 1.4 compositor.
 */
int
gs_is_pdf14trans_compositor(const gs_composite_t * pct)
{
    return (pct->type == &gs_composite_pdf14trans_type
		|| pct->type == &gs_composite_pdf14trans_no_clist_writer_type);
}

/*
 * Create a PDF 1.4 transparency compositor data structure.
 */
int
gs_create_pdf14trans(
    gs_composite_t **               ppct,
    const gs_pdf14trans_params_t *  pparams,
    gs_memory_t *                   mem )
{
    gs_pdf14trans_t *                pct;

    rc_alloc_struct_0( pct,
		       gs_pdf14trans_t,
		       &st_pdf14trans,
		       mem,
		       return_error(gs_error_VMerror),
		       "gs_create_pdf14trans" );
    pct->type = &gs_composite_pdf14trans_type;
    pct->id = gs_next_ids(mem, 1);
    pct->params = *pparams;
    *ppct = (gs_composite_t *)pct;
    return 0;
}

/*
 * Send a PDF 1.4 transparency compositor action to the specified device.
 */
int
send_pdf14trans(gs_imager_state	* pis, gx_device * dev,
    gx_device * * pcdev, gs_pdf14trans_params_t * pparams, gs_memory_t * mem)
{
    gs_composite_t * pct = NULL;
    int code;

    code = gs_create_pdf14trans(&pct, pparams, mem);
    if (code < 0)
	return code;
    code = dev_proc(dev, create_compositor) (dev, pcdev, pct, pis, mem);

    gs_free_object(pis->memory, pct, "send_pdf14trans");

    return code;
}

/* ------------- PDF 1.4 transparency device for clist writing ------------- */

/*
 * The PDF 1.4 transparency compositor device may have a different process
 * color model than the output device.  If we are banding then we need to
 * create two compositor devices.  The output side (clist reader) needs a
 * compositor to actually composite the output.  We also need a compositor
 * device before the clist writer.  This is needed to provide a process color
 * model which matches the PDF 1.4 blending space.
 *
 * This section provides support for this device.
 */

/* Define the default alpha-compositing	device.	*/
typedef	struct pdf14_clist_device_s {
    gx_device_forward_common;
    const gx_color_map_procs *(*save_get_cmap_procs)(const gs_imager_state *,
						     const gx_device *);
    gx_device_color_info saved_target_color_info;
    float opacity;
    float shape;
    gs_blend_mode_t blend_mode;
    bool text_knockout;
} pdf14_clist_device;

gs_private_st_suffix_add0_final(st_pdf14_clist_device,
    pdf14_clist_device, "pdf14_clist_device",
    device_c_pdf14_clist_enum_ptrs, device_c_pdf14_clist_reloc_ptrs,
    gx_device_finalize, st_device_forward);

#define	pdf14_clist_procs(get_color_mapping_procs, get_color_comp_index,\
						encode_color, decode_color) \
{\
	NULL,				/* open */\
	gx_forward_get_initial_matrix,	/* get_initial_matrix */\
	gx_forward_sync_output,		/* sync_output */\
	gx_forward_output_page,		/* output_page */\
	gx_forward_close_device,	/* close_device */\
	encode_color,			/* rgb_map_rgb_color */\
	decode_color,			/* map_color_rgb */\
	gx_forward_fill_rectangle,	/* fill_rectangle */\
	gx_forward_tile_rectangle,	/* tile_rectangle */\
	gx_forward_copy_mono,		/* copy_mono */\
	gx_forward_copy_color,		/* copy_color */\
	NULL		,		/* draw_line - obsolete */\
	gx_forward_get_bits,		/* get_bits */\
	gx_forward_get_params,		/* get_params */\
	pdf14_put_params,		/* put_params */\
	encode_color,			/* map_cmyk_color */\
	gx_forward_get_xfont_procs,	/* get_xfont_procs */\
	gx_forward_get_xfont_device,	/* get_xfont_device */\
	NULL,				/* map_rgb_alpha_color */\
	gx_forward_get_page_device,	/* get_page_device */\
	gx_forward_get_alpha_bits,	/* get_alpha_bits */\
	NULL,				/* copy_alpha */\
	gx_forward_get_band,		/* get_band */\
	gx_forward_copy_rop,		/* copy_rop */\
	pdf14_clist_fill_path,		/* fill_path */\
	pdf14_clist_stroke_path,		/* stroke_path */\
	gx_forward_fill_mask,		/* fill_mask */\
	gx_forward_fill_trapezoid,	/* fill_trapezoid */\
	gx_forward_fill_parallelogram,	/* fill_parallelogram */\
	gx_forward_fill_triangle,	/* fill_triangle */\
	gx_forward_draw_thin_line,	/* draw_thin_line */\
	pdf14_clist_begin_image,	/* begin_image */\
	gx_forward_image_data,		/* image_data */\
	gx_forward_end_image,		/* end_image */\
	gx_forward_strip_tile_rectangle, /* strip_tile_rectangle */\
	gx_forward_strip_copy_rop,	/* strip_copy_rop, */\
	gx_forward_get_clipping_box,	/* get_clipping_box */\
	pdf14_clist_begin_typed_image,	/* begin_typed_image */\
	gx_forward_get_bits_rectangle,	/* get_bits_rectangle */\
	NULL,				/* map_color_rgb_alpha */\
	pdf14_clist_create_compositor,	/* create_compositor */\
	gx_forward_get_hardware_params,	/* get_hardware_params */\
	pdf14_clist_text_begin,		/* text_begin */\
	NULL,				/* finish_copydevice */\
	pdf14_begin_transparency_group,\
	pdf14_end_transparency_group,\
	pdf14_begin_transparency_mask,\
	pdf14_end_transparency_mask,\
	NULL,				/* discard_transparency_layer */\
	get_color_mapping_procs,	/* get_color_mapping_procs */\
	get_color_comp_index,		/* get_color_comp_index */\
	encode_color,			/* encode_color */\
	decode_color			/* decode_color */\
}

private	dev_proc_create_compositor(pdf14_clist_create_compositor);
private	dev_proc_create_compositor(pdf14_clist_forward_create_compositor);
private	dev_proc_fill_path(pdf14_clist_fill_path);
private	dev_proc_stroke_path(pdf14_clist_stroke_path);
private	dev_proc_text_begin(pdf14_clist_text_begin);
private	dev_proc_begin_image(pdf14_clist_begin_image);
private	dev_proc_begin_typed_image(pdf14_clist_begin_typed_image);

private	const gx_device_procs pdf14_clist_Gray_procs =
	pdf14_clist_procs(gx_default_DevGray_get_color_mapping_procs,
			gx_default_DevGray_get_color_comp_index,
			gx_default_8bit_map_gray_color,
			gx_default_8bit_map_color_gray);

private	const gx_device_procs pdf14_clist_RGB_procs =
	pdf14_clist_procs(gx_default_DevRGB_get_color_mapping_procs,
			gx_default_DevRGB_get_color_comp_index,
			gx_default_rgb_map_rgb_color,
			gx_default_rgb_map_color_rgb);

private	const gx_device_procs pdf14_clist_CMYK_procs =
	pdf14_clist_procs(gx_default_DevCMYK_get_color_mapping_procs,
			gx_default_DevCMYK_get_color_comp_index,
			cmyk_8bit_map_cmyk_color, cmyk_8bit_map_color_cmyk);

const pdf14_clist_device pdf14_clist_Gray_device = {
    std_device_color_stype_body(pdf14_clist_device, &pdf14_clist_Gray_procs,
			"pdf14clistgray", &st_pdf14_clist_device,
			XSIZE, YSIZE, X_DPI, Y_DPI, 8, 255, 256),
    { 0 }
};

const pdf14_clist_device pdf14_clist_RGB_device	= {
    std_device_color_stype_body(pdf14_clist_device, &pdf14_clist_RGB_procs,
			"pdf14clistRGB", &st_pdf14_clist_device,
			XSIZE, YSIZE, X_DPI, Y_DPI, 24, 255, 256),
    { 0 }
};

const pdf14_clist_device pdf14_clist_CMYK_device = {
    std_device_std_color_full_body_type(pdf14_clist_device,
			&pdf14_clist_CMYK_procs, "PDF14clistcmyk",
			&st_pdf14_clist_device, XSIZE, YSIZE, X_DPI, Y_DPI, 32,
			0, 0, 0, 0, 0, 0),
    { 0 }
};

/*
 * the PDF 1.4 transparency spec says that color space for blending
 * operations can be based upon either a color space specified in the
 * group or a default value based upon the output device.  We are
 * currently only using a color space based upon the device.
 */
private	int
get_pdf14_clist_device_proto(gx_device * dev,
		const pdf14_clist_device ** pdevproto)
{
    pdf14_default_colorspace_t dev_cs =
		pdf14_determine_default_blend_cs(dev);

    switch (dev_cs) {
	case DeviceGray:
	    *pdevproto = &pdf14_clist_Gray_device;
	    break;
	case DeviceRGB:
	    *pdevproto = &pdf14_clist_RGB_device;
	    break;
	case DeviceCMYK:
	    *pdevproto = &pdf14_clist_CMYK_device;
	    break;
	default:			/* Should not occur */
	    return_error(gs_error_rangecheck);
    }
    return 0;
}

private	int
pdf14_create_clist_device(gs_memory_t *mem, gs_imager_state * pis,
				gx_device ** ppdev, gx_device * target)
{
    const pdf14_clist_device * dev_proto;
    pdf14_clist_device *pdev;
    int code;

    if_debug0('v', "[v]pdf14_create_clist_device\n");

    code = get_pdf14_clist_device_proto(target, &dev_proto);
    if (code < 0)
	return code;

    code = gs_copydevice((gx_device **) &pdev,
			 (const gx_device *) dev_proto, mem);
    if (code < 0)
	return code;

    check_device_separable((gx_device *)pdev);
    gx_device_fill_in_procs((gx_device *)pdev);

    gs_pdf14_device_copy_params((gx_device *)pdev, target);

    rc_assign(pdev->target, target, "pdf14_create_clist_device");

    code = dev_proc((gx_device *) pdev, open_device) ((gx_device *) pdev);
    *ppdev = (gx_device *) pdev;
    return code;
}

/*
 * Disable the PDF 1.4 clist compositor device.  Once created, the PDF 1.4
 * compositor device is never removed.  (We do not have a remove compositor
 * method.)  However it is no-op'ed when the PDF 1.4 device is popped.  This
 * routine implements that action.
 */
private	int
pdf14_disable_clist_device(gs_memory_t *mem, gs_imager_state * pis,
				gx_device * dev)
{
    gx_device_forward * pdev = (gx_device_forward *)dev;
    gx_device * target = pdev->target;

    if_debug0('v', "[v]pdf14_disable_clist_device\n");

    /*
     * To disable the action of this device, we forward all device
     * procedures to the target except the create_compositor and copy
     * the target's color_info.
     */
    dev->color_info = target->color_info;
    pdf14_forward_device_procs(dev);
    set_dev_proc(dev, create_compositor, pdf14_clist_forward_create_compositor);
    return 0;
}

/*
 * Recreate the PDF 1.4 clist compositor device.  Once created, the PDF 1.4
 * compositor device is never removed.  (We do not have a remove compositor
 * method.)  However it is no-op'ed when the PDF 1.4 device is popped.  This
 * routine will re-enable the compositor if the PDF 1.4 device is pushed
 * again.
 */
private	int
pdf14_recreate_clist_device(gs_memory_t	*mem, gs_imager_state *	pis,
				gx_device * dev)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    gx_device * target = pdev->target;
    const pdf14_clist_device * dev_proto;
    int code;

    if_debug0('v', "[v]pdf14_recreate_clist_device\n");

    /*
     * We will not use the entire prototype device but we will set the
     * color related info to match the prototype.
     */
    code = get_pdf14_clist_device_proto(target, &dev_proto);
    if (code < 0)
	return code;
    pdev->color_info = dev_proto->color_info;
    pdev->procs = dev_proto->procs;
    gx_device_fill_in_procs(dev);
    check_device_separable((gx_device *)pdev);

    return code;
}

/*
 * When we are banding, we have two PDF 1.4 compositor devices.  One for
 * when we are creating the clist.  The second is for imaging the data from
 * the clist.  This routine is part of the clist writing PDF 1.4 device.
 * This routine is only called once the PDF 1.4 clist write compositor already
 * exists.
 */
private	int
pdf14_clist_create_compositor(gx_device	* dev, gx_device ** pcdev,
    const gs_composite_t * pct, gs_imager_state * pis, gs_memory_t * mem)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    int code;

    /* We only handle a few PDF 1.4 transparency operations 4 */
    if (gs_is_pdf14trans_compositor(pct)) {
	const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pct;

	switch (pdf14pct->params.pdf14_op) {
	    case PDF14_PUSH_DEVICE:
		/* Re-activate the PDF 1.4 compositor */
		pdev->saved_target_color_info = pdev->target->color_info;
		pdev->target->color_info = pdev->color_info;
		pdev->save_get_cmap_procs = pis->get_cmap_procs;
		pis->get_cmap_procs = pdf14_get_cmap_procs;
		gx_set_cmap_procs(pis, dev);
		code = pdf14_recreate_clist_device(mem, pis, dev);
		pdev->blend_mode = pdev->text_knockout = 0;
		pdev->opacity = pdev->shape = 0.0;
		if (code < 0)
		    return code;
		/*
		 * This routine is part of the PDF 1.4 clist write device.
		 * Change the compositor procs to not create another since we
		 * do not need to create a chain of identical devices.
		 */
		{
		    gs_composite_t pctemp = *pct;

		    pctemp.type = &gs_composite_pdf14trans_no_clist_writer_type;
		    code = dev_proc(pdev->target, create_compositor)
				(pdev->target, pcdev, &pctemp, pis, mem);
		    *pcdev = dev;
		    return code;
		}
	    case PDF14_POP_DEVICE:
		/* Restore the color_info for the clist device */
		pdev->target->color_info = pdev->saved_target_color_info;
		pis->get_cmap_procs = pdev->save_get_cmap_procs;
		gx_set_cmap_procs(pis, pdev->target);
		/* Disable the PDF 1.4 compositor */
		pdf14_disable_clist_device(mem, pis, dev);
		/*
		 * Make sure that the transfer funtions, etc. are current.
		 */
		code = cmd_put_color_mapping(
			(gx_device_clist_writer *)(pdev->target), pis);
		if (code < 0)
		    return code;
		break;
	    case PDF14_BEGIN_TRANS_GROUP:
		/*
		 * Keep track of any changes made in the blending parameters.
		 */
		pdev->text_knockout = pdf14pct->params.Knockout;
		pdev->blend_mode = pdf14pct->params.blend_mode;
		pdev->opacity = pdf14pct->params.opacity.alpha;
		pdev->shape = pdf14pct->params.shape.alpha;
		{
		    const gs_pdf14trans_params_t * pparams = &((const gs_pdf14trans_t *)pct)->params;

		    if (pparams->Background_components != 0 && 
			pparams->Background_components != pdev->color_info.num_components)
			return_error(gs_error_rangecheck);
		}
		break;
	    default:
		break;		/* Pass remaining ops to target */
	}
    }
    code = dev_proc(pdev->target, create_compositor)
			(pdev->target, pcdev, pct, pis, mem);
    if (*pcdev != pdev->target)
	rc_assign(pdev->target, *pcdev, "pdf14_clist_create_compositor");
    *pcdev = dev;
    return code;
}

/*
 * The PDF 1.4 clist compositor is never removed.  (We do not have a 'remove
 * compositor' method.  However the compositor is disabled when we are not
 * doing a page which uses PDF 1.4 transparency.  This routine is only active
 * when the PDF 1.4 compositor is 'disabled'.  It checks for reenabling the
 * PDF 1.4 compositor.  Otherwise it simply passes create compositor requests
 * to the targer.
 */
private	int
pdf14_clist_forward_create_compositor(gx_device	* dev, gx_device * * pcdev,
	const gs_composite_t * pct, gs_imager_state * pis,
	gs_memory_t * mem)
{
    pdf14_device *pdev = (pdf14_device *)dev;
    gx_device * tdev = pdev->target;
    gx_device * ndev;
    int code = 0;

    *pcdev = dev;
    if (gs_is_pdf14trans_compositor(pct)) {
	const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pct;

	if (pdf14pct->params.pdf14_op == PDF14_PUSH_DEVICE)
	    return pdf14_clist_create_compositor(dev, &ndev, pct, pis, mem);
	return 0;
    }
    code = dev_proc(tdev, create_compositor)(tdev, &ndev, pct, pis, mem);
    if (code < 0)
	return code;
    pdev->target = ndev;
    return 0;
}

/*
 * If any of the PDF 1.4 transparency blending parameters have changed, we
 * need to send them to the PDF 1.4 compositor on the output side of the clist.
 */
private	int
pdf14_clist_update_params(pdf14_clist_device * pdev, const gs_imager_state * pis)
{
    gs_pdf14trans_params_t params = { 0 };
    gx_device * pcdev;
    int changed = 0;
    int code = 0;

    params.pdf14_op = PDF14_SET_BLEND_PARAMS;
    if (pis->blend_mode != pdev->blend_mode) {
	changed |= PDF14_SET_BLEND_MODE;
	params.blend_mode = pdev->blend_mode = pis->blend_mode;
    }
    if (pis->text_knockout != pdev->text_knockout) {
	changed |= PDF14_SET_TEXT_KNOCKOUT;
	params.text_knockout = pdev->text_knockout = pis->text_knockout;
    }
    if (pis->shape.alpha != pdev->shape) {
	changed |= PDF14_SET_SHAPE_ALPHA;
	params.shape.alpha = pdev->shape = pis->shape.alpha;
    }
    if (pis->opacity.alpha != pdev->opacity) {
	changed |= PDF14_SET_OPACITY_ALPHA;
	params.opacity.alpha = pdev->opacity = pis->opacity.alpha;
    }
    /*
     * Put parameters into a compositor parameter and then call the
     * create_compositor.  This will pass the data through the clist
     * to the PDF 1.4 transparency output device.  Note:  This action
     * never creates a new PDF 1.4 compositor and it does not change
     * the imager state.
     */
    if (changed != 0) {
	params.changed = changed;
	code = send_pdf14trans((gs_imager_state *)pis, (gx_device *)pdev,
					&pcdev, &params, pis->memory);
    }
    return code;
}

/*
 * fill_path routine for the PDF 1.4 transaprency compositor device for
 * writing the clist.
 */
private	int
pdf14_clist_fill_path(gx_device	*dev, const gs_imager_state *pis,
			   gx_path *ppath, const gx_fill_params *params,
			   const gx_drawing_color *pdcolor,
			   const gx_clip_path *pcpath)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    gs_imager_state new_is = *pis;
    int code;

    /*
     * Ensure that that the PDF 1.4 reading compositor will have the current
     * blending parameters.  This is needed since the fill_rectangle routines
     * do not have access to the imager state.  Thus we have to pass any
     * changes explictly.
     */
    code = pdf14_clist_update_params(pdev, pis);
    if (code < 0)
	return code;
    /*
     * The blend operations are not idempotent.  Force non-idempotent
     * filling and stroking operations.
     */
    new_is.log_op |= lop_pdf14;
    return gx_default_fill_path(dev, &new_is, ppath, params, pdcolor, pcpath);
}

/*
 * stroke_path routine for the PDF 1.4 transaprency compositor device for
 * writing the clist.
 */
private	int
pdf14_clist_stroke_path(gx_device *dev,	const gs_imager_state *pis,
			     gx_path *ppath, const gx_stroke_params *params,
			     const gx_drawing_color *pdcolor,
			     const gx_clip_path *pcpath)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    gs_imager_state new_is = *pis;
    int code;

    /*
     * Ensure that that the PDF 1.4 reading compositor will have the current
     * blending parameters.  This is needed since the fill_rectangle routines
     * do not have access to the imager state.  Thus we have to pass any
     * changes explictly.
     */
    code = pdf14_clist_update_params(pdev, pis);
    if (code < 0)
	return code;
    /*
     * The blend operations are not idempotent.  Force non-idempotent
     * filling and stroking operations.
     */
    new_is.log_op |= lop_pdf14;
    return gx_default_stroke_path(dev, &new_is, ppath, params, pdcolor, pcpath);
}

/*
 * text_begin routine for the PDF 1.4 transaprency compositor device for
 * writing the clist.
 */
private	int
pdf14_clist_text_begin(gx_device * dev,	gs_imager_state	* pis,
		 const gs_text_params_t * text, gs_font * font,
		 gx_path * path, const gx_device_color * pdcolor,
		 const gx_clip_path * pcpath, gs_memory_t * memory,
		 gs_text_enum_t ** ppenum)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    gs_text_enum_t *penum;
    int code;

    /*
     * Ensure that that the PDF 1.4 reading compositor will have the current
     * blending parameters.  This is needed since the fill_rectangle routines
     * do not have access to the imager state.  Thus we have to pass any
     * changes explictly.
     */
    code = pdf14_clist_update_params(pdev, pis);
    if (code < 0)
	return code;
    /* Pass text_begin to the target */
    code = gx_default_text_begin(dev, pis, text, font, path,
				pdcolor, pcpath, memory, &penum);
    if (code < 0)
	return code;
    *ppenum = (gs_text_enum_t *)penum;
    return code;
}

private	int
pdf14_clist_begin_image(gx_device * dev,
		       const gs_imager_state * pis, const gs_image_t * pim,
		       gs_image_format_t format, const gs_int_rect * prect,
		       const gx_drawing_color * pdcolor,
		       const gx_clip_path * pcpath,
		       gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    int code;

    /*
     * Ensure that that the PDF 1.4 reading compositor will have the current
     * blending parameters.  This is needed since the fill_rectangle routines
     * do not have access to the imager state.  Thus we have to pass any
     * changes explictly.
     */
    code = pdf14_clist_update_params(pdev, pis);
    if (code < 0)
	return code;
    /* Pass image to the target */
    return gx_default_begin_image(dev, pis, pim, format, prect,
					pdcolor, pcpath, memory, pinfo);
}

private	int
pdf14_clist_begin_typed_image(gx_device	* dev, const gs_imager_state * pis,
			   const gs_matrix *pmat, const gs_image_common_t *pic,
			   const gs_int_rect * prect,
			   const gx_drawing_color * pdcolor,
			   const gx_clip_path * pcpath, gs_memory_t * mem,
			   gx_image_enum_common_t ** pinfo)
{
    pdf14_clist_device * pdev = (pdf14_clist_device *)dev;
    int code;

    /*
     * Ensure that that the PDF 1.4 reading compositor will have the current
     * blending parameters.  This is needed since the fill_rectangle routines
     * do not have access to the imager state.  Thus we have to pass any
     * changes explictly.
     */
    code = pdf14_clist_update_params(pdev, pis);
    if (code < 0)
	return code;
    /* Pass image to the target */
    return gx_default_begin_typed_image(dev, pis, pmat,
			    pic, prect, pdcolor, pcpath, mem, pinfo);
}

/*
 * When we push a PDF 1.4 transparency compositor onto the clist, we also need
 * to create a compositing device for clist writing.  The primary purpose of
 * this device is to provide support for the process color model in which
 * the PDF 1.4 transparency is done.  (This may differ from the process color
 * model of the output device.)  The actual work of compositing the image is
 * done on the output (reader) side of the clist.
 */
private	int
c_pdf14trans_clist_write_update(const gs_composite_t * pcte, gx_device * dev,
		gx_device ** pcdev, gs_imager_state * pis, gs_memory_t * mem)
{
    const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pcte;
    pdf14_clist_device * p14dev;
    int code = 0;

    /* We only handle the push/pop operations */
    switch (pdf14pct->params.pdf14_op) {
	case PDF14_PUSH_DEVICE:
	    code = pdf14_create_clist_device(mem, pis, pcdev, dev);
	    /*
	     * Set the color_info of the clist device to match the compositing
	     * device.  We will restore it when the compositor is popped.
	     * See pdf14_clist_create_compositor for the restore.  Do the same
	     * with the imager state's get_cmap_procs.  We do not want the
	     * imager state to use transfer functions on our color values.  The
	     * transfer functions will be applied at the end after we have done
	     * our PDF 1.4 blend operations.
	     */
	    p14dev = (pdf14_clist_device *)(*pcdev);
	    p14dev->saved_target_color_info = dev->color_info;
	    dev->color_info = (*pcdev)->color_info;
	    p14dev->save_get_cmap_procs = pis->get_cmap_procs;
	    pis->get_cmap_procs = pdf14_get_cmap_procs;
	    gx_set_cmap_procs(pis, dev);
	    return code;
	case PDF14_POP_DEVICE:
	    /*
	     * Ensure that the tranfer functions, etc.  are current before we
	     * dump our transparency image to the output device.
	     */
	    code = cmd_put_halftone((gx_device_clist_writer *)
			(((pdf14_clist_device *)dev)->target), pis->dev_ht);
	    break;
	default:
	    break;		/* do nothing for remaining ops */
    }
    *pcdev = dev;
    return code;
}

/*
 * When we push a PDF 1.4 transparency compositor, we need to make the clist
 * device color_info data match the compositing device.  We need to do this
 * since the PDF 1.4 transparency compositing device may use a different
 * process color model than the output device.  We do not need to modify the
 * color related device procs since the compositing device has its own.  We
 * restore the color_info data when the transparency device is popped.
 */
private	int
c_pdf14trans_clist_read_update(gs_composite_t *	pcte, gx_device	* cdev,
		gx_device * tdev, gs_imager_state * pis, gs_memory_t * mem)
{
    pdf14_device * p14dev = (pdf14_device *)tdev;
    gs_pdf14trans_t * pdf14pct = (gs_pdf14trans_t *) pcte;

    /*
     * We only handle the push/pop operations. Save and restore the color_info
     * field for the clist device.  (This is needed since the process color
     * model of the clist device needs to match the PDF 1.4 compositing
     * device.
     */
    switch (pdf14pct->params.pdf14_op) {
	case PDF14_PUSH_DEVICE:
	    p14dev->saved_clist_color_info = cdev->color_info;
	    cdev->color_info = p14dev->color_info;
	    break;
	case PDF14_POP_DEVICE:
	    cdev->color_info = p14dev->saved_clist_color_info;
	    break;
	default:
	    break;		/* do nothing for remaining ops */
    }
    return 0;
}
