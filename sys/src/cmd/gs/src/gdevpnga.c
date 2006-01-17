/*
  Copyright (C) 2001-2004 artofcode LLC. All rights reserved.
  
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

/* $Id: gdevpnga.c,v 1.9 2004/05/26 04:10:58 dan Exp $ */
/* Test driver for PDF 1.4 transparency stuff */

#include "gdevprn.h"
#include "gdevpccm.h"
#include "gscdefs.h"
#include "gsdevice.h"
#include "gdevmem.h"
#include "gxblend.h"
#include "gxtext.h"

# define INCR(v) DO_NOTHING

#define PNG_INTERNAL
/*
 * libpng versions 1.0.3 and later allow disabling access to the stdxxx
 * files while retaining support for FILE * I/O.
 */
#define PNG_NO_CONSOLE_IO
/*
 * Earlier libpng versions require disabling FILE * I/O altogether.
 * This produces a compiler warning about no prototype for png_init_io.
 * The right thing will happen at link time, since the library itself
 * is compiled with stdio support.  Unfortunately, we can't do this
 * conditionally depending on PNG_LIBPNG_VER, because this is defined
 * in png.h.
 */
/*#define PNG_NO_STDIO*/
#include "png_.h"

/* Buffer stack data structure */

#define PDF14_MAX_PLANES 16

typedef struct pdf14_buf_s pdf14_buf;
typedef struct pdf14_ctx_s pdf14_ctx;

struct pdf14_buf_s {
    pdf14_buf *saved;

    bool isolated;
    bool knockout;
    byte alpha;
    byte shape;
    gs_blend_mode_t blend_mode;

    bool has_alpha_g;
    bool has_shape;

    gs_int_rect rect;
    /* Note: the traditional GS name for rowstride is "raster" */

    /* Data is stored in planar format. Order of planes is: pixel values,
       alpha, shape if present, alpha_g if present. */

    int rowstride;
    int planestride;
    int n_chan; /* number of pixel planes including alpha */
    int n_planes; /* total number of planes including alpha, shape, alpha_g */
    byte *data;
};

struct pdf14_ctx_s {
    pdf14_buf *stack;
    gs_memory_t *memory;
    gs_int_rect rect;
    int n_chan;
};

/* GC procedures for buffer stack */

private
ENUM_PTRS_WITH(pdf14_buf_enum_ptrs, pdf14_buf *buf)
    return 0;
    case 0: return ENUM_OBJ(buf->saved);
    case 1: return ENUM_OBJ(buf->data);
ENUM_PTRS_END

private
RELOC_PTRS_WITH(pdf14_buf_reloc_ptrs, pdf14_buf *buf)
{
    RELOC_VAR(buf->saved);
    RELOC_VAR(buf->data);
}
RELOC_PTRS_END

gs_private_st_composite(st_pdf14_buf, pdf14_buf, "pdf14_buf",
			pdf14_buf_enum_ptrs, pdf14_buf_reloc_ptrs);

gs_private_st_ptrs1(st_pdf14_ctx, pdf14_ctx, "pdf14_ctx",
		    pdf14_ctx_enum_ptrs, pdf14_ctx_reloc_ptrs,
		    stack);

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

private int pnga_open(gx_device * pdev);
private dev_proc_close_device(pnga_close);
private int pnga_output_page(gx_device * pdev, int num_copies, int flush);
private dev_proc_fill_rectangle(pnga_fill_rectangle);
private dev_proc_fill_path(pnga_fill_path);
private dev_proc_stroke_path(pnga_stroke_path);
private dev_proc_begin_typed_image(pnga_begin_typed_image);
private dev_proc_text_begin(pnga_text_begin);
private dev_proc_begin_transparency_group(pnga_begin_transparency_group);
private dev_proc_end_transparency_group(pnga_end_transparency_group);

#define XSIZE (8.5 * X_DPI)	/* 8.5 x 11 inch page, by default */
#define YSIZE (11 * Y_DPI)

/* 24-bit color. */

private const gx_device_procs pnga_procs =
{
	pnga_open,			/* open */
	NULL,	/* get_initial_matrix */
	NULL,	/* sync_output */
	pnga_output_page,		/* output_page */
	pnga_close,			/* close */
	gx_default_rgb_map_rgb_color,
	gx_default_rgb_map_color_rgb,
	pnga_fill_rectangle,	/* fill_rectangle */
	NULL,	/* tile_rectangle */
	NULL,	/* copy_mono */
	NULL,	/* copy_color */
	NULL,	/* draw_line */
	NULL,	/* get_bits */
	NULL,   /* get_params */
	NULL,   /* put_params */
	NULL,	/* map_cmyk_color */
	NULL,	/* get_xfont_procs */
	NULL,	/* get_xfont_device */
	NULL,	/* map_rgb_alpha_color */
#if 0
	gx_page_device_get_page_device,	/* get_page_device */
#else
	NULL,   /* get_page_device */
#endif
	NULL,	/* get_alpha_bits */
	NULL,	/* copy_alpha */
	NULL,	/* get_band */
	NULL,	/* copy_rop */
	pnga_fill_path,		/* fill_path */
	pnga_stroke_path,	/* stroke_path */
	NULL,	/* fill_mask */
	NULL,	/* fill_trapezoid */
	NULL,	/* fill_parallelogram */
	NULL,	/* fill_triangle */
	NULL,	/* draw_thin_line */
	NULL,	/* begin_image */
	NULL,	/* image_data */
	NULL,	/* end_image */
	NULL,	/* strip_tile_rectangle */
	NULL,	/* strip_copy_rop, */
	NULL,	/* get_clipping_box */
	pnga_begin_typed_image,	/* begin_typed_image */
	NULL,	/* get_bits_rectangle */
	NULL,	/* map_color_rgb_alpha */
	NULL,	/* create_compositor */
	NULL,	/* get_hardware_params */
	pnga_text_begin,	/* text_begin */
	NULL,	/* finish_copydevice */
	pnga_begin_transparency_group,
	pnga_end_transparency_group
};

typedef struct pnga_device_s {
    gx_device_common;

    pdf14_ctx *ctx;

} pnga_device;

gs_private_st_composite_use_final(st_pnga_device, pnga_device, "pnga_device",
				  pnga_device_enum_ptrs, pnga_device_reloc_ptrs,
				  gx_device_finalize);

const gx_device_printer gs_pnga_device = {
    std_device_color_stype_body(pnga_device, &pnga_procs, "pnga",
				&st_pnga_device,
				XSIZE, YSIZE, X_DPI, Y_DPI, 24, 255, 0),
    { 0 }
};

#if 0
prn_device(pnga_procs, "pnga",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   24, pnga_print_page);
#endif

/* GC procedures */
private 
ENUM_PTRS_WITH(pnga_device_enum_ptrs, pnga_device *pdev) return 0;
case 0: return ENUM_OBJ(pdev->ctx);
ENUM_PTRS_END
private RELOC_PTRS_WITH(pnga_device_reloc_ptrs, pnga_device *pdev)
{
    RELOC_VAR(pdev->ctx);
}
RELOC_PTRS_END

/* ------ The device descriptors for the marking device ------ */

private dev_proc_fill_rectangle(pnga_mark_fill_rectangle);
private dev_proc_fill_rectangle(pnga_mark_fill_rectangle_ko_simple);

private const gx_device_procs pnga_mark_procs =
{
	NULL,	/* open */
	NULL,	/* get_initial_matrix */
	NULL,	/* sync_output */
	NULL,	/* output_page */
	NULL,	/* close */
	gx_default_rgb_map_rgb_color,
	gx_default_rgb_map_color_rgb,
	NULL,	/* fill_rectangle */
	NULL,	/* tile_rectangle */
	NULL,	/* copy_mono */
	NULL,	/* copy_color */
	NULL,	/* draw_line */
	NULL,	/* get_bits */
	NULL,   /* get_params */
	NULL,   /* put_params */
	NULL,	/* map_cmyk_color */
	NULL,	/* get_xfont_procs */
	NULL,	/* get_xfont_device */
	NULL,	/* map_rgb_alpha_color */
#if 0
	gx_page_device_get_page_device,	/* get_page_device */
#else
	NULL,   /* get_page_device */
#endif
	NULL,	/* get_alpha_bits */
	NULL,	/* copy_alpha */
	NULL,	/* get_band */
	NULL,	/* copy_rop */
	NULL,	/* fill_path */
	NULL,	/* stroke_path */
	NULL,	/* fill_mask */
	NULL,	/* fill_trapezoid */
	NULL,	/* fill_parallelogram */
	NULL,	/* fill_triangle */
	NULL,	/* draw_thin_line */
	NULL,	/* begin_image */
	NULL,	/* image_data */
	NULL,	/* end_image */
	NULL,	/* strip_tile_rectangle */
	NULL,	/* strip_copy_rop, */
	NULL,	/* get_clipping_box */
	NULL,	/* begin_typed_image */
	NULL,	/* get_bits_rectangle */
	NULL,	/* map_color_rgb_alpha */
	NULL,	/* create_compositor */
	NULL,	/* get_hardware_params */
	NULL,	/* text_begin */
	NULL	/* finish_copydevice */
};

typedef struct pnga_mark_device_s {
    gx_device_common;

    pnga_device *pnga_dev;
    float opacity;
    float shape;
    float alpha; /* alpha = opacity * shape */
    gs_blend_mode_t blend_mode;
} pnga_mark_device;

gs_private_st_simple_final(st_pnga_mark_device, pnga_mark_device,
			   "pnga_mark_device", gx_device_finalize);

const gx_device_printer gs_pnga_mark_device = {
    std_device_color_stype_body(pnga_mark_device, &pnga_mark_procs,
				"pnga_mark",
				&st_pnga_mark_device,
				XSIZE, YSIZE, X_DPI, Y_DPI, 24, 255, 0),
    { 0 }
};

typedef struct pnga_text_enum_s {
    gs_text_enum_common;
    gs_text_enum_t *target_enum;
} pnga_text_enum_t;
extern_st(st_gs_text_enum);
gs_private_st_suffix_add1(st_pnga_text_enum, pnga_text_enum_t,
			  "pnga_text_enum_t", pnga_text_enum_enum_ptrs,
			  pnga_text_enum_reloc_ptrs, st_gs_text_enum,
			  target_enum);

/* ------ Private definitions ------ */

/**
 * pdf14_buf_new: Allocate a new PDF 1.4 buffer.
 * @n_chan: Number of pixel channels including alpha.
 *
 * Return value: Newly allocated buffer, or NULL on failure.
 **/
private pdf14_buf *
pdf14_buf_new(gs_int_rect *rect, bool has_alpha_g, bool has_shape,
	       int n_chan,
	       gs_memory_t *memory)
{
    pdf14_buf *result;
    int rowstride = (rect->q.x - rect->p.x + 3) & -4;
    int planestride = rowstride * (rect->q.y - rect->p.y);
    int n_planes = n_chan + (has_shape ? 1 : 0) + (has_alpha_g ? 1 : 0);

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
    return result;
}

private void
pdf14_buf_free(pdf14_buf *buf, gs_memory_t *memory)
{
    gs_free_object(memory, buf->data, "pdf14_buf_free");
    gs_free_object(memory, buf, "pdf14_buf_free");
}

private pdf14_ctx *
pdf14_ctx_new(gs_int_rect *rect, int n_chan, gs_memory_t *memory)
{
    pdf14_ctx *result;
    pdf14_buf *buf;

    result = gs_alloc_struct(memory, pdf14_ctx, &st_pdf14_ctx,
			     "pdf14_ctx_new");

    buf = pdf14_buf_new(rect, false, false, n_chan, memory);
    if (buf == NULL) {
	gs_free_object(memory, result, "pdf14_ctx_new");
	return NULL;
    }
    result->stack = buf;
    result->n_chan = n_chan;
    result->memory = memory;
    result->rect = *rect;
    if (result == NULL)
	return result;
    return result;
}

private void
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
private pdf14_buf *
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

private int
pdf14_push_transparency_group(pdf14_ctx *ctx, gs_int_rect *rect,
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
    if (knockout) isolated = true;

    has_shape = tos->has_shape || tos->knockout;

    buf = pdf14_buf_new(rect, !isolated, has_shape, ctx->n_chan, ctx->memory);
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

private int
pdf14_pop_transparency_group(pdf14_ctx *ctx)
{
    pdf14_buf *tos = ctx->stack;
    pdf14_buf *nos = tos->saved;
    int n_chan = ctx->n_chan;
    byte alpha = tos->alpha;
    byte shape = tos->shape;
    byte blend_mode = tos->blend_mode;
    int x0 = tos->rect.p.x;
    int y0 = tos->rect.p.y;
    int x1 = tos->rect.q.x;
    int y1 = tos->rect.q.y;
    byte *tos_ptr = tos->data;
    byte *nos_ptr = nos->data + x0 - nos->rect.p.x +
	(y0 - nos->rect.p.y) * nos->rowstride;
    int tos_planestride = tos->planestride;
    int nos_planestride = nos->planestride;
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

    if (nos == NULL)
	return_error(gs_error_rangecheck);

    /* for now, only simple non-knockout */

    if (nos->has_alpha_g)
	nos_alpha_g_ptr = nos_ptr + n_chan * nos_planestride;
    else
	nos_alpha_g_ptr = NULL;

    for (y = y0; y < y1; ++y) {
	for (x = 0; x < width; ++x) {
	    for (i = 0; i < n_chan; ++i) {
		tos_pixel[i] = tos_ptr[x + i * tos_planestride];
		nos_pixel[i] = nos_ptr[x + i * nos_planestride];
	    }
	    
	    if (nos_knockout) {
		byte *nos_shape_ptr = nos_has_shape ?
		    &nos_ptr[x + nos_shape_offset] : NULL;
		byte tos_shape = tos_ptr[x + tos_shape_offset];

#if 1
		art_pdf_composite_knockout_isolated_8 (nos_pixel,
						       nos_shape_ptr,
						       tos_pixel,
						       n_chan - 1,
						       tos_shape,
						       alpha, shape);
#else
		tos_pixel[3] = tos_ptr[x + tos_shape_offset];
		art_pdf_composite_group_8(nos_pixel, nos_alpha_g_ptr,
					  tos_pixel,
					  n_chan - 1,
					  alpha, blend_mode);
#endif
	    } else if (tos_isolated) {
		art_pdf_composite_group_8(nos_pixel, nos_alpha_g_ptr,
					  tos_pixel,
					  n_chan - 1,
					  alpha, blend_mode);
	    } else {
		byte tos_alpha_g = tos_ptr[x + tos_alpha_g_offset];
		art_pdf_recomposite_group_8(nos_pixel, nos_alpha_g_ptr,
					    tos_pixel, tos_alpha_g,
					    n_chan - 1,
					    alpha, blend_mode);
	    }
	    if (nos_has_shape) {
		nos_ptr[x + nos_shape_offset] =
		    art_pdf_union_mul_8 (nos_ptr[x + nos_shape_offset],
					 tos_ptr[x + tos_shape_offset],
					 shape);
	    }
	    /* todo: knockout cases */
	    
	    for (i = 0; i < n_chan; ++i) {
		nos_ptr[x + i * nos_planestride] = nos_pixel[i];
	    }
	    if (nos_alpha_g_ptr != NULL)
		++nos_alpha_g_ptr;
	}
	tos_ptr += tos->rowstride;
	nos_ptr += nos->rowstride;
	if (nos_alpha_g_ptr != NULL)
	    nos_alpha_g_ptr += nos->rowstride - width;
    }

    ctx->stack = nos;
    pdf14_buf_free(tos, ctx->memory);
    return 0;
}

private int
pnga_open(gx_device *dev)
{
    pnga_device *pdev = (pnga_device *)dev;
    gs_int_rect rect;

    if_debug2('v', "[v]pnga_open: width = %d, height = %d\n",
	     dev->width, dev->height);

    rect.p.x = 0;
    rect.p.y = 0;
    rect.q.x = dev->width;
    rect.q.y = dev->height;
    pdev->ctx = pdf14_ctx_new(&rect, 4, dev->memory);
    if (pdev->ctx == NULL)
	return_error(gs_error_VMerror);

    return 0;
}

private int
pnga_close(gx_device *dev)
{
    pnga_device *pdev = (pnga_device *)dev;

    if (pdev->ctx)
	pdf14_ctx_free(pdev->ctx);
    return 0;
}

private int
pnga_output_page(gx_device *dev, int num_copies, int flush)
{
    pnga_device *pdev = (pnga_device *)dev;
    gs_memory_t *mem = dev->memory;
    int width = dev->width;
    int height = dev->height;
    int rowbytes = width << 2;

    /* PNG structures */
    byte *row = gs_alloc_bytes(mem, rowbytes, "png raster buffer");
    png_struct *png_ptr =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_info *info_ptr =
    png_create_info_struct(png_ptr);
    int depth = dev->color_info.depth;
    int y;
    int code;			/* return code */
    const char *software_key = "Software";
    char software_text[256];
    png_text text_png;
    char prefix[] = "pnga_png";
    char fname[gp_file_name_sizeof];
    FILE *file;

    pdf14_buf *buf = pdev->ctx->stack;
    int planestride = buf->planestride;
    byte *buf_ptr = buf->data;

    file = gp_open_scratch_file(prefix, fname, "wb");
    if (file == NULL) {
	code = gs_note_error(gs_error_invalidfileaccess);
	goto done;
    }
    /* todo: suck from OutputFile instead */

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
    info_ptr->width = dev->width;
    info_ptr->height = dev->height;
    /* resolution is in pixels per meter vs. dpi */
    info_ptr->x_pixels_per_unit =
	(png_uint_32) (dev->HWResolution[0] * (100.0 / 2.54));
    info_ptr->y_pixels_per_unit =
	(png_uint_32) (dev->HWResolution[1] * (100.0 / 2.54));
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
	buf_ptr += buf->rowstride;
    }

    /* write the rest of the file */
    png_write_end(png_ptr, info_ptr);

  done:
    /* free the structures */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    gs_free_object(mem, row, "png raster buffer");

    fclose (file);

    return code;
}

private void
pnga_finalize(gx_device *dev)
{
    if_debug1('v', "[v]finalizing %lx\n", dev);
}

/**
 * pnga_get_marking_device: Obtain a marking device.
 * @dev: Original device.
 * @pis: Imager state.
 *
 * The current implementation creates a marking device each time this
 * routine is called. A potential optimization is to cache a single
 * instance in the original device.
 *
 * Return value: Marking device, or NULL on error.
 **/
private gx_device *
pnga_get_marking_device(gx_device *dev, const gs_imager_state *pis)
{
    pnga_device *pdev = (pnga_device *)dev;
    pdf14_buf *buf = pdev->ctx->stack;
    pnga_mark_device *mdev;
    int code = gs_copydevice((gx_device **)&mdev,
			     (const gx_device *)&gs_pnga_mark_device,
			     dev->memory);

    if (code < 0)
	return NULL;

    check_device_separable((gx_device *)mdev);
    gx_device_fill_in_procs((gx_device *)mdev);
    mdev->pnga_dev = pdev;
    mdev->opacity = pis->opacity.alpha;
    mdev->shape = pis->shape.alpha;
    mdev->alpha = pis->opacity.alpha * pis->shape.alpha;
    mdev->blend_mode = pis->blend_mode;

    if (buf->knockout) {
	fill_dev_proc((gx_device *)mdev, fill_rectangle,
		      pnga_mark_fill_rectangle_ko_simple);
    } else {
	fill_dev_proc((gx_device *)mdev, fill_rectangle,
		      pnga_mark_fill_rectangle);
    }

    if_debug1('v', "[v]creating %lx\n", mdev);
    mdev->finalize = pnga_finalize;
    return (gx_device *)mdev;
}

private void
pnga_release_marking_device(gx_device *marking_dev)
{
    rc_decrement_only(marking_dev, "pnga_release_marking_device");
}

private int
pnga_fill_path(gx_device *dev, const gs_imager_state *pis,
			   gx_path *ppath, const gx_fill_params *params,
			   const gx_drawing_color *pdcolor,
			   const gx_clip_path *pcpath)
{
    int code;
    gx_device *mdev = pnga_get_marking_device(dev, pis);

    if (mdev == 0)
	return_error(gs_error_VMerror);
    code = gx_default_fill_path(mdev, pis, ppath, params, pdcolor, pcpath);
    pnga_release_marking_device(mdev);
    return code;
}

private int
pnga_stroke_path(gx_device *dev, const gs_imager_state *pis,
			     gx_path *ppath, const gx_stroke_params *params,
			     const gx_drawing_color *pdcolor,
			     const gx_clip_path *pcpath)
{
    int code;
    gx_device *mdev = pnga_get_marking_device(dev, pis);

    if (mdev == 0)
	return_error(gs_error_VMerror);
    code = gx_default_stroke_path(mdev, pis, ppath, params, pdcolor, pcpath);
    pnga_release_marking_device(mdev);
    return code;
}

private int
pnga_begin_typed_image(gx_device * dev, const gs_imager_state * pis,
			   const gs_matrix *pmat, const gs_image_common_t *pic,
			   const gs_int_rect * prect,
			   const gx_drawing_color * pdcolor,
			   const gx_clip_path * pcpath, gs_memory_t * mem,
			   gx_image_enum_common_t ** pinfo)
{
    gx_device *mdev = pnga_get_marking_device(dev, pis);
    int code;

    if (mdev == 0)
	return_error(gs_error_VMerror);

    code = gx_default_begin_typed_image(mdev, pis, pmat, pic, prect, pdcolor,
					pcpath, mem, pinfo);

    rc_decrement_only(mdev, "pnga_begin_typed_image");

    return code;
}

private int
pnga_text_resync(gs_text_enum_t *pte, const gs_text_enum_t *pfrom)
{
    pnga_text_enum_t *const penum = (pnga_text_enum_t *)pte;

    if ((pte->text.operation ^ pfrom->text.operation) & ~TEXT_FROM_ANY)
	return_error(gs_error_rangecheck);
    if (penum->target_enum) {
	int code = gs_text_resync(penum->target_enum, pfrom);

	if (code < 0)
	    return code;
    }
    pte->text = pfrom->text;
    gs_text_enum_copy_dynamic(pte, pfrom, false);
    return 0;
}

private int
pnga_text_process(gs_text_enum_t *pte)
{
    pnga_text_enum_t *const penum = (pnga_text_enum_t *)pte;
    int code;

    code = gs_text_process(penum->target_enum);
    gs_text_enum_copy_dynamic(pte, penum->target_enum, true);
    return code;
}

private bool
pnga_text_is_width_only(const gs_text_enum_t *pte)
{
    const pnga_text_enum_t *const penum = (const pnga_text_enum_t *)pte;

    if (penum->target_enum)
	return gs_text_is_width_only(penum->target_enum);
    return false;
}

private int
pnga_text_current_width(const gs_text_enum_t *pte, gs_point *pwidth)
{
    const pnga_text_enum_t *const penum = (const pnga_text_enum_t *)pte;

    if (penum->target_enum)
	return gs_text_current_width(penum->target_enum, pwidth);
    return_error(gs_error_rangecheck); /* can't happen */
}

private int
pnga_text_set_cache(gs_text_enum_t *pte, const double *pw,
		   gs_text_cache_control_t control)
{
    pnga_text_enum_t *const penum = (pnga_text_enum_t *)pte;

    if (penum->target_enum)
	return gs_text_set_cache(penum->target_enum, pw, control);
    return_error(gs_error_rangecheck); /* can't happen */
}

private int
pnga_text_retry(gs_text_enum_t *pte)
{
    pnga_text_enum_t *const penum = (pnga_text_enum_t *)pte;

    if (penum->target_enum)
	return gs_text_retry(penum->target_enum);
    return_error(gs_error_rangecheck); /* can't happen */
}

private void
pnga_text_release(gs_text_enum_t *pte, client_name_t cname)
{
    pnga_text_enum_t *const penum = (pnga_text_enum_t *)pte;

    if (penum->target_enum) {
	gs_text_release(penum->target_enum, cname);
	penum->target_enum = 0;
    }
    gx_default_text_release(pte, cname);
}

private const gs_text_enum_procs_t pnga_text_procs = {
    pnga_text_resync, pnga_text_process,
    pnga_text_is_width_only, pnga_text_current_width,
    pnga_text_set_cache, pnga_text_retry,
    pnga_text_release
};

private int
pnga_text_begin(gx_device * dev, gs_imager_state * pis,
		 const gs_text_params_t * text, gs_font * font,
		 gx_path * path, const gx_device_color * pdcolor,
		 const gx_clip_path * pcpath, gs_memory_t * memory,
		 gs_text_enum_t ** ppenum)
{
    int code;
    gx_device *mdev = pnga_get_marking_device(dev, pis);
    pnga_text_enum_t *penum;
    gs_text_enum_t *target_enum;

    if (mdev == 0)
	return_error(gs_error_VMerror);
    if_debug0('v', "[v]pnga_text_begin\n");
    code = gx_default_text_begin(mdev, pis, text, font, path, pdcolor, pcpath,
				 memory, &target_enum);

    rc_alloc_struct_1(penum, pnga_text_enum_t, &st_pnga_text_enum, memory,
		      return_error(gs_error_VMerror), "pnga_text_begin");
    penum->rc.free = rc_free_text_enum;
    penum->target_enum = target_enum;
    code = gs_text_enum_init((gs_text_enum_t *)penum, &pnga_text_procs,
			     dev, pis, text, font, path, pdcolor, pcpath,
			     memory);
    if (code < 0) {
	gs_free_object(memory, penum, "pnga_text_begin");
	return code;
    }
    *ppenum = (gs_text_enum_t *)penum;
    rc_decrement_only(mdev, "pnga_text_begin");
    return code;
}

private int
pnga_fill_rectangle(gx_device * dev,
		    int x, int y, int w, int h, gx_color_index color)
{
    if_debug4('v', "[v]pnga_fill_rectangle, (%d, %d), %d x %d\n", x, y, w, h);
    return 0;
}


private int
pnga_begin_transparency_group(gx_device *dev,
			      const gs_transparency_group_params_t *ptgp,
			      const gs_rect *pbbox,
			      gs_imager_state *pis,
			      gs_transparency_state_t **ppts,
			      gs_memory_t *mem)
{
    pnga_device *pdev = (pnga_device *)dev;
    double alpha = pis->opacity.alpha * pis->shape.alpha;
    int code;

    if_debug4('v', "[v]begin_transparency_group, I = %d, K = %d, alpha = %g, bm = %d\n",
	      ptgp->Isolated, ptgp->Knockout, alpha, pis->blend_mode);

    code = pdf14_push_transparency_group(pdev->ctx, &pdev->ctx->rect,
					 ptgp->Isolated, ptgp->Knockout,
					 floor (255 * alpha + 0.5),
					 floor (255 * pis->shape.alpha + 0.5),
					 pis->blend_mode);
    return code;
}

private int
pnga_end_transparency_group(gx_device *dev,
			      gs_imager_state *pis,
			      gs_transparency_state_t **ppts)
{
    pnga_device *pdev = (pnga_device *)dev;
    int code;

    if_debug0('v', "[v]end_transparency_group\n");
    code = pdf14_pop_transparency_group(pdev->ctx);
    return code;
}

private int
pnga_mark_fill_rectangle(gx_device * dev,
			 int x, int y, int w, int h, gx_color_index color)
{
    pnga_mark_device *mdev = (pnga_mark_device *)dev;
    pnga_device *pdev = (pnga_device *)mdev->pnga_dev;
    pdf14_buf *buf = pdev->ctx->stack;
    int i, j, k;
    byte *line, *dst_ptr;
    byte src[PDF14_MAX_PLANES];
    byte dst[PDF14_MAX_PLANES];
    gs_blend_mode_t blend_mode = mdev->blend_mode;
    int rowstride = buf->rowstride;
    int planestride = buf->planestride;
    bool has_alpha_g = buf->has_alpha_g;
    bool has_shape = buf->has_shape;
    int shape_off = buf->n_chan * planestride;
    int alpha_g_off = shape_off + (has_shape ? planestride : 0);
    byte shape;

    src[0] = color >> 16;
    src[1] = (color >> 8) & 0xff;
    src[2] = color & 0xff;
    src[3] = floor (255 * mdev->alpha + 0.5);
    if (has_shape)
	shape = floor (255 * mdev->shape + 0.5);

    if (x < buf->rect.p.x) x = buf->rect.p.x;
    if (y < buf->rect.p.x) y = buf->rect.p.y;
    if (x + w > buf->rect.q.x) w = buf->rect.q.x - x;
    if (y + h > buf->rect.q.y) h = buf->rect.q.y - y;

    line = buf->data + (x - buf->rect.p.x) + (y - buf->rect.p.y) * rowstride;

    for (j = 0; j < h; ++j) {
	dst_ptr = line;
	for (i = 0; i < w; ++i) {
	    for (k = 0; k < 4; ++k)
		dst[k] = dst_ptr[k * planestride];
	    art_pdf_composite_pixel_alpha_8(dst, src, 3, blend_mode);
	    for (k = 0; k < 4; ++k)
		dst_ptr[k * planestride] = dst[k];
	    if (has_alpha_g) {
		int tmp = (255 - dst_ptr[alpha_g_off]) * (255 - src[3]) + 0x80;
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

private int
pnga_mark_fill_rectangle_ko_simple(gx_device * dev,
				   int x, int y, int w, int h, gx_color_index color)
{
    pnga_mark_device *mdev = (pnga_mark_device *)dev;
    pnga_device *pdev = (pnga_device *)mdev->pnga_dev;
    pdf14_buf *buf = pdev->ctx->stack;
    int i, j, k;
    byte *line, *dst_ptr;
    byte src[PDF14_MAX_PLANES];
    byte dst[PDF14_MAX_PLANES];
    int rowstride = buf->rowstride;
    int planestride = buf->planestride;
    int shape_off = buf->n_chan * planestride;
    bool has_shape = buf->has_shape;
    byte opacity;

    src[0] = color >> 16;
    src[1] = (color >> 8) & 0xff;
    src[2] = color & 0xff;
    src[3] = floor (255 * mdev->shape + 0.5);
    opacity = floor (255 * mdev->opacity + 0.5);

    if (x < buf->rect.p.x) x = buf->rect.p.x;
    if (y < buf->rect.p.x) y = buf->rect.p.y;
    if (x + w > buf->rect.q.x) w = buf->rect.q.x - x;
    if (y + h > buf->rect.q.y) h = buf->rect.q.y - y;

    line = buf->data + (x - buf->rect.p.x) + (y - buf->rect.p.y) * rowstride;

    for (j = 0; j < h; ++j) {
	dst_ptr = line;
	for (i = 0; i < w; ++i) {
	    for (k = 0; k < 4; ++k)
		dst[k] = dst_ptr[k * planestride];
	    art_pdf_composite_knockout_simple_8(dst, has_shape ? dst_ptr + shape_off : NULL,
						src, 3, opacity);
	    for (k = 0; k < 4; ++k)
		dst_ptr[k * planestride] = dst[k];
	    ++dst_ptr;
	}
	line += rowstride;
    }
    return 0;
}
