/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpx.c,v 1.16 2005/07/07 16:44:17 stefan Exp $ */
/* H-P PCL XL driver */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsccolor.h"
#include "gsdcolor.h"
#include "gxcspace.h"		/* for color mapping for images */
#include "gxdevice.h"
#include "gxpath.h"
#include "gdevvec.h"
#include "strimpl.h"
#include "srlx.h"
#include "gdevpxat.h"
#include "gdevpxen.h"
#include "gdevpxop.h"
#include "gdevpxut.h"

/* ---------------- Device definition ---------------- */

/* Define the default resolution. */
#ifndef X_DPI
#  define X_DPI 600
#endif
#ifndef Y_DPI
#  define Y_DPI 600
#endif

/* Structure definition */
#define NUM_POINTS 40		/* must be >= 3 and <= 255 */
typedef enum {
    POINTS_NONE,
    POINTS_LINES,
    POINTS_CURVES
} point_type_t;
typedef struct gx_device_pclxl_s {
    gx_device_vector_common;
    /* Additional state information */
    pxeMediaSize_t media_size;
    bool ManualFeed;            /* map ps setpage commands to pxl */
    bool ManualFeed_set;         
    int  MediaPosition;         
    int  MediaPosition_set;
    gx_path_type_t fill_rule;	/* ...winding_number or ...even_odd  */
    gx_path_type_t clip_rule;	/* ditto */
    pxeColorSpace_t color_space;
    struct pal_ {
	int size;		/* # of bytes */
	byte data[256 * 3];	/* up to 8-bit samples */
    } palette;
    struct pts_ {		/* buffer for accumulating path points */
	gs_int_point current;	/* current point as of start of data */
	point_type_t type;
	int count;
	gs_int_point data[NUM_POINTS];
    } points;
    struct ch_ {		/* cache for downloaded characters */
#define MAX_CACHED_CHARS 400
#define MAX_CHAR_DATA 500000
#define MAX_CHAR_SIZE 5000
#define CHAR_HASH_FACTOR 247
	ushort table[MAX_CACHED_CHARS * 3 / 2];
	struct cd_ {
	    gs_id id;		/* key */
	    uint size;
	} data[MAX_CACHED_CHARS];
	int next_in;		/* next data element to fill in */
	int next_out;		/* next data element to discard */
	int count;		/* of occupied data elements */
	ulong used;
    } chars;
    bool font_set;
} gx_device_pclxl;

gs_public_st_suffix_add0_final(st_device_pclxl, gx_device_pclxl,
			       "gx_device_pclxl",
			       device_pclxl_enum_ptrs, device_pclxl_reloc_ptrs,
			       gx_device_finalize, st_device_vector);

#define pclxl_device_body(dname, depth)\
  std_device_dci_type_body(gx_device_pclxl, 0, dname, &st_device_pclxl,\
			   DEFAULT_WIDTH_10THS * X_DPI / 10,\
			   DEFAULT_HEIGHT_10THS * Y_DPI / 10,\
			   X_DPI, Y_DPI,\
			   (depth > 8 ? 3 : 1), depth,\
			   (depth > 1 ? 255 : 1), (depth > 8 ? 255 : 0),\
			   (depth > 1 ? 256 : 2), (depth > 8 ? 256 : 1))

/* Driver procedures */
private dev_proc_open_device(pclxl_open_device);
private dev_proc_output_page(pclxl_output_page);
private dev_proc_close_device(pclxl_close_device);
private dev_proc_copy_mono(pclxl_copy_mono);
private dev_proc_copy_color(pclxl_copy_color);
private dev_proc_fill_mask(pclxl_fill_mask);

private dev_proc_get_params(pclxl_get_params);
private dev_proc_put_params(pclxl_put_params);

/*private dev_proc_draw_thin_line(pclxl_draw_thin_line); */
private dev_proc_begin_image(pclxl_begin_image);
private dev_proc_strip_copy_rop(pclxl_strip_copy_rop);

#define pclxl_device_procs(map_rgb_color, map_color_rgb)\
{\
	pclxl_open_device,\
	NULL,			/* get_initial_matrix */\
	NULL,			/* sync_output */\
	pclxl_output_page,\
	pclxl_close_device,\
	map_rgb_color,		/* differs */\
	map_color_rgb,		/* differs */\
	gdev_vector_fill_rectangle,\
	NULL,			/* tile_rectangle */\
	pclxl_copy_mono,\
	pclxl_copy_color,\
	NULL,			/* draw_line */\
	NULL,			/* get_bits */\
	pclxl_get_params,\
	pclxl_put_params,\
	NULL,			/* map_cmyk_color */\
	NULL,			/* get_xfont_procs */\
	NULL,			/* get_xfont_device */\
	NULL,			/* map_rgb_alpha_color */\
	gx_page_device_get_page_device,\
	NULL,			/* get_alpha_bits */\
	NULL,			/* copy_alpha */\
	NULL,			/* get_band */\
	NULL,			/* copy_rop */\
	gdev_vector_fill_path,\
	gdev_vector_stroke_path,\
	pclxl_fill_mask,\
	gdev_vector_fill_trapezoid,\
	gdev_vector_fill_parallelogram,\
	gdev_vector_fill_triangle,\
	NULL /****** WRONG ******/,	/* draw_thin_line */\
	pclxl_begin_image,\
	NULL,			/* image_data */\
	NULL,			/* end_image */\
	NULL,			/* strip_tile_rectangle */\
	pclxl_strip_copy_rop\
}

const gx_device_pclxl gs_pxlmono_device = {
    pclxl_device_body("pxlmono", 8),
    pclxl_device_procs(gx_default_gray_map_rgb_color, gx_default_gray_map_color_rgb)
};

const gx_device_pclxl gs_pxlcolor_device = {
    pclxl_device_body("pxlcolor", 24),
    pclxl_device_procs(gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb)
};

/* ---------------- Other utilities ---------------- */

inline private stream *
pclxl_stream(gx_device_pclxl *xdev)
{
    return gdev_vector_stream((gx_device_vector *)xdev);
}

/* Initialize for a page. */
private void
pclxl_page_init(gx_device_pclxl * xdev)
{
    gdev_vector_init((gx_device_vector *)xdev);
    xdev->in_page = false;
    xdev->fill_rule = gx_path_type_winding_number;
    xdev->clip_rule = gx_path_type_winding_number;
    xdev->color_space = eNoColorSpace;
    xdev->palette.size = 0;
    xdev->font_set = false;
}

/* Test whether a RGB color is actually a gray shade. */
#define RGB_IS_GRAY(ci) ((ci) >> 8 == ((ci) & 0xffff))

/* Set the color space and (optionally) palette. */
private void
pclxl_set_color_space(gx_device_pclxl * xdev, pxeColorSpace_t color_space)
{
    if (xdev->color_space != color_space) {
	stream *s = pclxl_stream(xdev);

	px_put_ub(s, (byte)color_space);
	px_put_ac(s, pxaColorSpace, pxtSetColorSpace);
	xdev->color_space = color_space;
    }
}
private void
pclxl_set_color_palette(gx_device_pclxl * xdev, pxeColorSpace_t color_space,
			const byte * palette, uint palette_size)
{
    if (xdev->color_space != color_space ||
	xdev->palette.size != palette_size ||
	memcmp(xdev->palette.data, palette, palette_size)
	) {
	stream *s = pclxl_stream(xdev);
	static const byte csp_[] = {
	    DA(pxaColorSpace),
	    DUB(e8Bit), DA(pxaPaletteDepth),
	    pxt_ubyte_array
	};

	px_put_ub(s, (byte)color_space);
	PX_PUT_LIT(s, csp_);
	px_put_u(s, palette_size);
	px_put_bytes(s, palette, palette_size);
	px_put_ac(s, pxaPaletteData, pxtSetColorSpace);
	xdev->color_space = color_space;
	xdev->palette.size = palette_size;
	memcpy(xdev->palette.data, palette, palette_size);
    }
}

/* Set a drawing RGB color. */
private int
pclxl_set_color(gx_device_pclxl * xdev, const gx_drawing_color * pdc,
		px_attribute_t null_source, px_tag_t op)
{
    stream *s = pclxl_stream(xdev);

    if (gx_dc_is_pure(pdc)) {
	gx_color_index color = gx_dc_pure_color(pdc);

	if (xdev->color_info.num_components == 1 || RGB_IS_GRAY(color)) {
	    pclxl_set_color_space(xdev, eGray);
	    px_put_uba(s, (byte) color, pxaGrayLevel);
	} else {
	    pclxl_set_color_space(xdev, eRGB);
	    spputc(s, pxt_ubyte_array);
	    px_put_ub(s, 3);
	    spputc(s, (byte) (color >> 16));
	    spputc(s, (byte) (color >> 8));
	    spputc(s, (byte) color);
	    px_put_a(s, pxaRGBColor);
	}
    } else if (gx_dc_is_null(pdc) || !color_is_set(pdc))
	px_put_uba(s, 0, null_source);
    else
	return_error(gs_error_rangecheck);
    spputc(s, (byte)op);
    return 0;
}

/* Test whether we can handle a given color space in an image. */
/* We cannot handle ICCBased color spaces. */
private bool
pclxl_can_handle_color_space(const gs_color_space * pcs)
{
    gs_color_space_index index = gs_color_space_get_index(pcs);

    if (index == gs_color_space_index_Indexed) {
	if (pcs->params.indexed.use_proc)
	    return false;
	index =
	    gs_color_space_get_index(gs_color_space_indexed_base_space(pcs));
    }
    return !(index == gs_color_space_index_Separation ||
	     index == gs_color_space_index_Pattern ||
             index == gs_color_space_index_CIEICC);
}

/* Set brush, pen, and mode for painting a path. */
private void
pclxl_set_paints(gx_device_pclxl * xdev, gx_path_type_t type)
{
    stream *s = pclxl_stream(xdev);
    gx_path_type_t rule = type & gx_path_type_rule;

    if (!(type & gx_path_type_fill) &&
	(color_is_set(&xdev->saved_fill_color.saved_dev_color) ||
	!gx_dc_is_null(&xdev->saved_fill_color.saved_dev_color) 
	)
	) {
	static const byte nac_[] = {
	    DUB(0), DA(pxaNullBrush), pxtSetBrushSource
	};

	PX_PUT_LIT(s, nac_);
	color_set_null(&xdev->saved_fill_color.saved_dev_color);
	if (rule != xdev->fill_rule) {
	    px_put_ub(s, (byte)(rule == gx_path_type_even_odd ? eEvenOdd :
		       eNonZeroWinding));
	    px_put_ac(s, pxaFillMode, pxtSetFillMode);
	    xdev->fill_rule = rule;
	}
    }
    if (!(type & gx_path_type_stroke) &&
	(color_is_set(&xdev->saved_stroke_color.saved_dev_color) ||
	!gx_dc_is_null(&xdev->saved_fill_color.saved_dev_color)
	 )
	) {
	static const byte nac_[] = {
	    DUB(0), DA(pxaNullPen), pxtSetPenSource
	};

	PX_PUT_LIT(s, nac_);
	color_set_null(&xdev->saved_stroke_color.saved_dev_color);
    }
}

/* Set the cursor. */
private int
pclxl_set_cursor(gx_device_pclxl * xdev, int x, int y)
{
    stream *s = pclxl_stream(xdev);

    px_put_ssp(s, x, y);
    px_put_ac(s, pxaPoint, pxtSetCursor);
    return 0;
}

/* ------ Paths ------ */

/* Flush any buffered path points. */
private void
px_put_np(stream * s, int count, pxeDataType_t dtype)
{
    px_put_uba(s, (byte)count, pxaNumberOfPoints);
    px_put_uba(s, (byte)dtype, pxaPointType);
}
private int
pclxl_flush_points(gx_device_pclxl * xdev)
{
    int count = xdev->points.count;

    if (count) {
	stream *s = pclxl_stream(xdev);
	px_tag_t op;
	int x = xdev->points.current.x, y = xdev->points.current.y;
	int uor = 0, sor = 0;
	pxeDataType_t data_type;
	int i, di;
	byte diffs[NUM_POINTS * 2];

	/*
	 * Writing N lines using a point list requires 11 + 4*N or 11 +
	 * 2*N bytes, as opposed to 8*N bytes using separate commands;
	 * writing N curves requires 11 + 12*N or 11 + 6*N bytes
	 * vs. 22*N.  So it's always shorter to write curves with a
	 * list (except for N = 1 with full-size coordinates, but since
	 * the difference is only 1 byte, we don't bother to ever use
	 * the non-list form), but lines are shorter only if N >= 3
	 * (again, with a 1-byte difference if N = 2 and byte
	 * coordinates).
	 */
	switch (xdev->points.type) {
	    case POINTS_NONE:
		return 0;
	    case POINTS_LINES:
		op = pxtLinePath;
		if (count < 3) {
		    for (i = 0; i < count; ++i) {
			px_put_ssp(s, xdev->points.data[i].x,
				xdev->points.data[i].y);
			px_put_a(s, pxaEndPoint);
			spputc(s, (byte)op);
		    }
		    goto zap;
		}
		/* See if we can use byte values. */
		for (i = di = 0; i < count; ++i, di += 2) {
		    int dx = xdev->points.data[i].x - x;
		    int dy = xdev->points.data[i].y - y;

		    diffs[di] = (byte) dx;
		    diffs[di + 1] = (byte) dy;
		    uor |= dx | dy;
		    sor |= (dx + 0x80) | (dy + 0x80);
		    x += dx, y += dy;
		}
		if (!(uor & ~0xff))
		    data_type = eUByte;
		else if (!(sor & ~0xff))
		    data_type = eSByte;
		else
		    break;
		op = pxtLineRelPath;
		/* Use byte values. */
	      useb:px_put_np(s, count, data_type);
		spputc(s, (byte)op);
		px_put_data_length(s, count * 2);	/* 2 bytes per point */
		px_put_bytes(s, diffs, count * 2);
		goto zap;
	    case POINTS_CURVES:
		op = pxtBezierPath;
		/* See if we can use byte values. */
		for (i = di = 0; i < count; i += 3, di += 6) {
		    int dx1 = xdev->points.data[i].x - x;
		    int dy1 = xdev->points.data[i].y - y;
		    int dx2 = xdev->points.data[i + 1].x - x;
		    int dy2 = xdev->points.data[i + 1].y - y;
		    int dx = xdev->points.data[i + 2].x - x;
		    int dy = xdev->points.data[i + 2].y - y;

		    diffs[di] = (byte) dx1;
		    diffs[di + 1] = (byte) dy1;
		    diffs[di + 2] = (byte) dx2;
		    diffs[di + 3] = (byte) dy2;
		    diffs[di + 4] = (byte) dx;
		    diffs[di + 5] = (byte) dy;
		    uor |= dx1 | dy1 | dx2 | dy2 | dx | dy;
		    sor |= (dx1 + 0x80) | (dy1 + 0x80) |
			(dx2 + 0x80) | (dy2 + 0x80) |
			(dx + 0x80) | (dy + 0x80);
		    x += dx, y += dy;
		}
		if (!(uor & ~0xff))
		    data_type = eUByte;
		else if (!(sor & ~0xff))
		    data_type = eSByte;
		else
		    break;
		op = pxtBezierRelPath;
		goto useb;
	    default:		/* can't happen */
		return_error(gs_error_unknownerror);
	}
	px_put_np(s, count, eSInt16);
	spputc(s, (byte)op);
	px_put_data_length(s, count * 4);	/* 2 UInt16s per point */
	for (i = 0; i < count; ++i) {
	    px_put_s(s, xdev->points.data[i].x);
	    px_put_s(s, xdev->points.data[i].y);
	}
      zap:xdev->points.type = POINTS_NONE;
	xdev->points.count = 0;
    }
    return 0;
}

/* ------ Images ------ */

private image_enum_proc_plane_data(pclxl_image_plane_data);
private image_enum_proc_end_image(pclxl_image_end_image);
private const gx_image_enum_procs_t pclxl_image_enum_procs = {
    pclxl_image_plane_data, pclxl_image_end_image
};

/* Begin an image. */
private void
pclxl_write_begin_image(gx_device_pclxl * xdev, uint width, uint height,
			uint dest_width, uint dest_height)
{
    stream *s = pclxl_stream(xdev);

    px_put_usa(s, width, pxaSourceWidth);
    px_put_usa(s, height, pxaSourceHeight);
    px_put_usp(s, dest_width, dest_height);
    px_put_ac(s, pxaDestinationSize, pxtBeginImage);
}

/* Write rows of an image. */
/****** IGNORES data_bit ******/
private void
pclxl_write_image_data(gx_device_pclxl * xdev, const byte * data, int data_bit,
		       uint raster, uint width_bits, int y, int height)
{
    stream *s = pclxl_stream(xdev);
    uint width_bytes = (width_bits + 7) >> 3;
    uint num_bytes = ROUND_UP(width_bytes, 4) * height;
    bool compress = num_bytes >= 8;
    int i;

    px_put_usa(s, y, pxaStartLine);
    px_put_usa(s, height, pxaBlockHeight);
    if (compress) {
	stream_RLE_state rlstate;
	stream_cursor_write w;
	stream_cursor_read r;

	/*
	 * H-P printers require that all the data for an operator be
	 * contained in a single data block.  Thus, we must allocate a
	 * temporary buffer for the compressed data.  Currently we don't go
	 * to the trouble of doing two passes if we can't allocate a buffer
	 * large enough for the entire transfer.
	 */
	byte *buf = gs_alloc_bytes(xdev->v_memory, num_bytes,
				   "pclxl_write_image_data");

	if (buf == 0)
	    goto nc;
	s_RLE_set_defaults_inline(&rlstate);
	rlstate.EndOfData = false;
	s_RLE_init_inline(&rlstate);
	w.ptr = buf - 1;
	w.limit = w.ptr + num_bytes;
	/*
	 * If we ever overrun the buffer, it means that the compressed
	 * data was larger than the uncompressed.  If this happens,
	 * write the data uncompressed.
	 */
	for (i = 0; i < height; ++i) {
	    r.ptr = data + i * raster - 1;
	    r.limit = r.ptr + width_bytes;
	    if ((*s_RLE_template.process)
		((stream_state *) & rlstate, &r, &w, false) != 0 ||
		r.ptr != r.limit
		)
		goto ncfree;
	    r.ptr = (const byte *)"\000\000\000\000\000";
	    r.limit = r.ptr + (-(int)width_bytes & 3);
	    if ((*s_RLE_template.process)
		((stream_state *) & rlstate, &r, &w, false) != 0 ||
		r.ptr != r.limit
		)
		goto ncfree;
	}
	r.ptr = r.limit;
	if ((*s_RLE_template.process)
	    ((stream_state *) & rlstate, &r, &w, true) != 0
	    )
	    goto ncfree;
	{
	    uint count = w.ptr + 1 - buf;

	    px_put_ub(s, eRLECompression);
	    px_put_ac(s, pxaCompressMode, pxtReadImage);
	    px_put_data_length(s, count);
	    px_put_bytes(s, buf, count);
	}
	gs_free_object(xdev->v_memory, buf, "pclxl_write_image_data");
	return;
      ncfree:gs_free_object(xdev->v_memory, buf, "pclxl_write_image_data");
    }
 nc:
    /* Write the data uncompressed. */
    px_put_ub(s, eNoCompression);
    px_put_ac(s, pxaCompressMode, pxtReadImage);
    px_put_data_length(s, num_bytes);
    for (i = 0; i < height; ++i) {
	px_put_bytes(s, data + i * raster, width_bytes);
	px_put_bytes(s, (const byte *)"\000\000\000\000", -(int)width_bytes & 3);
    }
}

/* End an image. */
private void
pclxl_write_end_image(gx_device_pclxl * xdev)
{
    spputc(xdev->strm, pxtEndImage);
}

/* ------ Fonts ------ */

/* Write a string (single- or double-byte). */
private void
px_put_string(stream * s, const byte * data, uint len, bool wide)
{
    if (wide) {
	spputc(s, pxt_uint16_array);
	px_put_u(s, len);
	px_put_bytes(s, data, len * 2);
    } else {
	spputc(s, pxt_ubyte_array);
	px_put_u(s, len);
	px_put_bytes(s, data, len);
    }
}

/* Write a 16-bit big-endian value. */
private void
px_put_us_be(stream * s, uint i)
{
    spputc(s, (byte) (i >> 8));
    spputc(s, (byte) i);
}

/* Define a bitmap font.  The client must call px_put_string */
/* with the font name immediately before calling this procedure. */
private void
pclxl_define_bitmap_font(gx_device_pclxl * xdev)
{
    stream *s = pclxl_stream(xdev);
    static const byte bfh_[] = {
	DA(pxaFontName), DUB(0), DA(pxaFontFormat),
	pxtBeginFontHeader,
	DUS(8 + 6 + 4 + 6), DA(pxaFontHeaderLength),
	pxtReadFontHeader,
	pxt_dataLengthByte, 8 + 6 + 4 + 6,
	0, 0, 0, 0,
	254, 0, (MAX_CACHED_CHARS + 255) >> 8, 0,
	'B', 'R', 0, 0, 0, 4
    };
    static const byte efh_[] = {
	0xff, 0xff, 0, 0, 0, 0,
	pxtEndFontHeader
    };

    PX_PUT_LIT(s, bfh_);
    px_put_us_be(s, (uint) (xdev->HWResolution[0] + 0.5));
    px_put_us_be(s, (uint) (xdev->HWResolution[1] + 0.5));
    PX_PUT_LIT(s, efh_);
}

/* Set the font.  The client must call px_put_string */
/* with the font name immediately before calling this procedure. */
private void
pclxl_set_font(gx_device_pclxl * xdev)
{
    stream *s = pclxl_stream(xdev);
    static const byte sf_[] = {
	DA(pxaFontName), DUB(1), DA(pxaCharSize), DUS(0), DA(pxaSymbolSet),
	pxtSetFont
    };

    PX_PUT_LIT(s, sf_);
}

/* Define a character in a bitmap font.  The client must call px_put_string */
/* with the font name immediately before calling this procedure. */
private void
pclxl_define_bitmap_char(gx_device_pclxl * xdev, uint ccode,
	       const byte * data, uint raster, uint width_bits, uint height)
{
    stream *s = pclxl_stream(xdev);
    uint width_bytes = (width_bits + 7) >> 3;
    uint size = 10 + width_bytes * height;
    uint i;

    px_put_ac(s, pxaFontName, pxtBeginChar);
    px_put_u(s, ccode);
    px_put_a(s, pxaCharCode);
    if (size > 0xffff) {
	spputc(s, pxt_uint32);
	px_put_l(s, (ulong) size);
    } else
	px_put_us(s, size);
    px_put_ac(s, pxaCharDataSize, pxtReadChar);
    px_put_data_length(s, size);
    px_put_bytes(s, (const byte *)"\000\000\000\000\000\000", 6);
    px_put_us_be(s, width_bits);
    px_put_us_be(s, height);
    for (i = 0; i < height; ++i)
	px_put_bytes(s, data + i * raster, width_bytes);
    spputc(s, pxtEndChar);
}

/* Write the name of the only font we define. */
private void
pclxl_write_font_name(gx_device_pclxl * xdev)
{
    stream *s = pclxl_stream(xdev);

    px_put_string(s, (const byte *)"@", 1, false);
}

/* Look up a bitmap id, return the index in the character table. */
/* If the id is missing, return an index for inserting. */
private int
pclxl_char_index(gx_device_pclxl * xdev, gs_id id)
{
    int i, i_empty = -1;
    uint ccode;

    for (i = (id * CHAR_HASH_FACTOR) % countof(xdev->chars.table);;
	 i = (i == 0 ? countof(xdev->chars.table) : i) - 1
	) {
	ccode = xdev->chars.table[i];
	if (ccode == 0)
	    return (i_empty >= 0 ? i_empty : i);
	else if (ccode == 1) {
	    if (i_empty < 0)
		i_empty = i;
	    else if (i == i_empty)	/* full table */
		return i;
	} else if (xdev->chars.data[ccode].id == id)
	    return i;
    }
}

/* Remove the character table entry at a given index. */
private void
pclxl_remove_char(gx_device_pclxl * xdev, int index)
{
    uint ccode = xdev->chars.table[index];
    int i;

    if (ccode < 2)
	return;
    xdev->chars.count--;
    xdev->chars.used -= xdev->chars.data[ccode].size;
    xdev->chars.table[index] = 1;	/* mark as deleted */
    i = (index == 0 ? countof(xdev->chars.table) : index) - 1;
    if (xdev->chars.table[i] == 0) {
	/* The next slot in probe order is empty. */
	/* Mark this slot and any deleted predecessors as empty. */
	for (i = index; xdev->chars.table[i] == 1;
	     i = (i == countof(xdev->chars.table) - 1 ? 0 : i + 1)
	    )
	    xdev->chars.table[i] = 0;
    }
}

/* Write a bitmap as a text character if possible. */
/* The caller must set the color, cursor, and RasterOp. */
/* We know id != gs_no_id. */
private int
pclxl_copy_text_char(gx_device_pclxl * xdev, const byte * data,
		     int raster, gx_bitmap_id id, int w, int h)
{
    uint width_bytes = (w + 7) >> 3;
    uint size = width_bytes * h;
    int index;
    uint ccode;
    stream *s = pclxl_stream(xdev);

    if (size > MAX_CHAR_SIZE)
	return -1;
    index = pclxl_char_index(xdev, id);
    if ((ccode = xdev->chars.table[index]) < 2) {
	/* Enter the character in the table. */
	while (xdev->chars.used + size > MAX_CHAR_DATA ||
	       xdev->chars.count >= MAX_CACHED_CHARS - 2
	    ) {
	    ccode = xdev->chars.next_out;
	    index = pclxl_char_index(xdev, xdev->chars.data[ccode].id);
	    pclxl_remove_char(xdev, index);
	    xdev->chars.next_out =
		(ccode == MAX_CACHED_CHARS - 1 ? 2 : ccode + 1);
	}
	index = pclxl_char_index(xdev, id);
	ccode = xdev->chars.next_in;
	xdev->chars.data[ccode].id = id;
	xdev->chars.data[ccode].size = size;
	xdev->chars.table[index] = ccode;
	xdev->chars.next_in =
	    (ccode == MAX_CACHED_CHARS - 1 ? 2 : ccode + 1);
	if (!xdev->chars.count++) {
	    /* This is the very first character. */
	    pclxl_write_font_name(xdev);
	    pclxl_define_bitmap_font(xdev);
	}
	xdev->chars.used += size;
	pclxl_write_font_name(xdev);
	pclxl_define_bitmap_char(xdev, ccode, data, raster, w, h);
    }
    if (!xdev->font_set) {
	pclxl_write_font_name(xdev);
	pclxl_set_font(xdev);
	xdev->font_set = true;
    } {
	byte cc_bytes[2];

	cc_bytes[0] = (byte) ccode;
	cc_bytes[1] = ccode >> 8;
	px_put_string(s, cc_bytes, 1, cc_bytes[1] != 0);
    }
    px_put_ac(s, pxaTextData, pxtText);
    return 0;
}

/* ---------------- Vector implementation procedures ---------------- */

private int
pclxl_beginpage(gx_device_vector * vdev)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;
    /*
     * We can't use gdev_vector_stream here, because this may be called
     * from there before in_page is set.
     */
    stream *s = vdev->strm;
    byte media_source = eAutoSelect; /* default */

    px_write_page_header(s, (const gx_device *)vdev);

    if (xdev->ManualFeed_set && xdev->ManualFeed) 
	media_source = 2;
    else if (xdev->MediaPosition_set && xdev->MediaPosition >= 0 )
	media_source = xdev->MediaPosition;
 
    px_write_select_media(s, (const gx_device *)vdev, &xdev->media_size, &media_source );

    spputc(s, pxtBeginPage);
    return 0;
}

private int
pclxl_setlinewidth(gx_device_vector * vdev, floatp width)
{
    stream *s = gdev_vector_stream(vdev);

    px_put_us(s, (uint) width);
    px_put_ac(s, pxaPenWidth, pxtSetPenWidth);
    return 0;
}

private int
pclxl_setlinecap(gx_device_vector * vdev, gs_line_cap cap)
{
    stream *s = gdev_vector_stream(vdev);

    /* The PCL XL cap styles just happen to be identical to PostScript. */
    px_put_ub(s, (byte) cap);
    px_put_ac(s, pxaLineCapStyle, pxtSetLineCap);
    return 0;
}

private int
pclxl_setlinejoin(gx_device_vector * vdev, gs_line_join join)
{
    stream *s = gdev_vector_stream(vdev);

    /* The PCL XL join styles just happen to be identical to PostScript. */
    px_put_ub(s, (byte) join);
    px_put_ac(s, pxaLineJoinStyle, pxtSetLineJoin);
    return 0;
}

private int
pclxl_setmiterlimit(gx_device_vector * vdev, floatp limit)
{
    stream *s = gdev_vector_stream(vdev);
    /*
     * Amazingly enough, the PCL XL specification doesn't allow real
     * numbers for the miter limit.
     */
    int i_limit = (int)(limit + 0.5);

    px_put_u(s, max(i_limit, 1));
    px_put_ac(s, pxaMiterLength, pxtSetMiterLimit);
    return 0;
}

private int
pclxl_setdash(gx_device_vector * vdev, const float *pattern, uint count,
	      floatp offset)
{
    stream *s = gdev_vector_stream(vdev);

    if (count == 0) {
	static const byte nac_[] = {
	    DUB(0), DA(pxaSolidLine)
	};

	PX_PUT_LIT(s, nac_);
    } else if (count > 255)
	return_error(gs_error_limitcheck);
    else {
	uint i;

	/*
	 * Astoundingly, PCL XL doesn't allow real numbers here.
	 * Do the best we can.
	 */
	spputc(s, pxt_uint16_array);
	px_put_ub(s, (byte)count);
	for (i = 0; i < count; ++i)
	    px_put_s(s, (uint)pattern[i]);
	px_put_a(s, pxaLineDashStyle);
	if (offset != 0)
	    px_put_usa(s, (uint)offset, pxaDashOffset);
    }
    spputc(s, pxtSetLineDash);
    return 0;
}

private int
pclxl_setlogop(gx_device_vector * vdev, gs_logical_operation_t lop,
	       gs_logical_operation_t diff)
{
    stream *s = gdev_vector_stream(vdev);

    if (diff & lop_S_transparent) {
	px_put_ub(s, (byte)(lop & lop_S_transparent ? 1 : 0));
	px_put_ac(s, pxaTxMode, pxtSetSourceTxMode);
    }
    if (diff & lop_T_transparent) {
	px_put_ub(s, (byte)(lop & lop_T_transparent ? 1 : 0));
	px_put_ac(s, pxaTxMode, pxtSetPaintTxMode);
    }
    if (lop_rop(diff)) {
	px_put_ub(s, (byte)lop_rop(lop));
	px_put_ac(s, pxaROP3, pxtSetROP);
    }
    return 0;
}

private int
pclxl_can_handle_hl_color(gx_device_vector * vdev, const gs_imager_state * pis, 
                   const gx_drawing_color * pdc)
{
    return false;
}

private int
pclxl_setfillcolor(gx_device_vector * vdev, const gs_imager_state * pis, 
                   const gx_drawing_color * pdc)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;

    return pclxl_set_color(xdev, pdc, pxaNullBrush, pxtSetBrushSource);
}

private int
pclxl_setstrokecolor(gx_device_vector * vdev, const gs_imager_state * pis, 
                     const gx_drawing_color * pdc)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;

    return pclxl_set_color(xdev, pdc, pxaNullPen, pxtSetPenSource);
}

private int
pclxl_dorect(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1,
	     fixed y1, gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;
    stream *s = gdev_vector_stream(vdev);

    /* Check for out-of-range points. */
#define OUT_OF_RANGE(v) (v < 0 || v >= int2fixed(0x10000))
    if (OUT_OF_RANGE(x0) || OUT_OF_RANGE(y0) ||
	OUT_OF_RANGE(x1) || OUT_OF_RANGE(y1)
	)
	return_error(gs_error_rangecheck);
#undef OUT_OF_RANGE
    if (type & (gx_path_type_fill | gx_path_type_stroke)) {
	pclxl_set_paints(xdev, type);
	px_put_usq_fixed(s, x0, y0, x1, y1);
	px_put_ac(s, pxaBoundingBox, pxtRectangle);
    }
    if (type & gx_path_type_clip) {
	static const byte cr_[] = {
	    DA(pxaBoundingBox),
	    DUB(eInterior), DA(pxaClipRegion),
	    pxtSetClipRectangle
	};

	px_put_usq_fixed(s, x0, y0, x1, y1);
	PX_PUT_LIT(s, cr_);
    }
    return 0;
}

private int
pclxl_beginpath(gx_device_vector * vdev, gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;
    stream *s = gdev_vector_stream(vdev);

    spputc(s, pxtNewPath);
    xdev->points.type = POINTS_NONE;
    xdev->points.count = 0;
    return 0;
}

private int
pclxl_moveto(gx_device_vector * vdev, floatp x0, floatp y0, floatp x, floatp y,
	     gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;
    int code = pclxl_flush_points(xdev);

    if (code < 0)
	return code;
    return pclxl_set_cursor(xdev,
			    xdev->points.current.x = (int)x,
			    xdev->points.current.y = (int)y);
}

private int
pclxl_lineto(gx_device_vector * vdev, floatp x0, floatp y0, floatp x, floatp y,
	     gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;

    if (xdev->points.type != POINTS_LINES ||
	xdev->points.count >= NUM_POINTS
	) {
	if (xdev->points.type != POINTS_NONE) {
	    int code = pclxl_flush_points(xdev);

	    if (code < 0)
		return code;
	}
	xdev->points.current.x = (int)x0;
	xdev->points.current.y = (int)y0;
	xdev->points.type = POINTS_LINES;
    } {
	gs_int_point *ppt = &xdev->points.data[xdev->points.count++];

	ppt->x = (int)x, ppt->y = (int)y;
    }
    return 0;
}

private int
pclxl_curveto(gx_device_vector * vdev, floatp x0, floatp y0,
	   floatp x1, floatp y1, floatp x2, floatp y2, floatp x3, floatp y3,
	      gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;

    if (xdev->points.type != POINTS_CURVES ||
	xdev->points.count >= NUM_POINTS - 2
	) {
	if (xdev->points.type != POINTS_NONE) {
	    int code = pclxl_flush_points(xdev);

	    if (code < 0)
		return code;
	}
	xdev->points.current.x = (int)x0;
	xdev->points.current.y = (int)y0;
	xdev->points.type = POINTS_CURVES;
    }
    {
	gs_int_point *ppt = &xdev->points.data[xdev->points.count];

	ppt->x = (int)x1, ppt->y = (int)y1, ++ppt;
	ppt->x = (int)x2, ppt->y = (int)y2, ++ppt;
	ppt->x = (int)x3, ppt->y = (int)y3;
    }
    xdev->points.count += 3;
    return 0;
}

private int
pclxl_closepath(gx_device_vector * vdev, floatp x, floatp y,
		floatp x_start, floatp y_start, gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;
    stream *s = gdev_vector_stream(vdev);
    int code = pclxl_flush_points(xdev);

    if (code < 0)
	return code;
    spputc(s, pxtCloseSubPath);
    xdev->points.current.x = (int)x_start;
    xdev->points.current.y = (int)y_start;
    return 0;
}

private int
pclxl_endpath(gx_device_vector * vdev, gx_path_type_t type)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)vdev;
    stream *s = gdev_vector_stream(vdev);
    int code = pclxl_flush_points(xdev);
    gx_path_type_t rule = type & gx_path_type_rule;

    if (code < 0)
	return code;
    if (type & (gx_path_type_fill | gx_path_type_stroke)) {
	pclxl_set_paints(xdev, type);
	spputc(s, pxtPaintPath);
    }
    if (type & gx_path_type_clip) {
	static const byte scr_[] = {
	    DUB(eInterior), DA(pxaClipRegion), pxtSetClipReplace
	};

	if (rule != xdev->clip_rule) {
	    px_put_ub(s, (byte)(rule == gx_path_type_even_odd ? eEvenOdd :
		       eNonZeroWinding));
	    px_put_ac(s, pxaClipMode, pxtSetClipMode);
	    xdev->clip_rule = rule;
	}
	PX_PUT_LIT(s, scr_);
    }
    return 0;
}

/* Vector implementation procedures */

private const gx_device_vector_procs pclxl_vector_procs = {
	/* Page management */
    pclxl_beginpage,
	/* Imager state */
    pclxl_setlinewidth,
    pclxl_setlinecap,
    pclxl_setlinejoin,
    pclxl_setmiterlimit,
    pclxl_setdash,
    gdev_vector_setflat,
    pclxl_setlogop,
	/* Other state */
    pclxl_can_handle_hl_color,
    pclxl_setfillcolor,
    pclxl_setstrokecolor,
	/* Paths */
    gdev_vector_dopath,
    pclxl_dorect,
    pclxl_beginpath,
    pclxl_moveto,
    pclxl_lineto,
    pclxl_curveto,
    pclxl_closepath,
    pclxl_endpath
};

/* ---------------- Driver procedures ---------------- */

/* ------ Open/close/page ------ */

/* Open the device. */
private int
pclxl_open_device(gx_device * dev)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    int code;

    vdev->v_memory = dev->memory;	/****** WRONG ******/
    vdev->vec_procs = &pclxl_vector_procs;
    code = gdev_vector_open_file_options(vdev, 512,
					 VECTOR_OPEN_FILE_SEQUENTIAL);
    if (code < 0)
	return code;
    pclxl_page_init(xdev);
    px_write_file_header(vdev->strm, dev);
    xdev->media_size = pxeMediaSize_next;	/* no size selected */
    memset(&xdev->chars, 0, sizeof(xdev->chars));
    xdev->chars.next_in = xdev->chars.next_out = 2;
    return 0;
}

/* Wrap up ("output") a page. */
/* We only support flush = true, and we don't support num_copies != 1. */
private int
pclxl_output_page(gx_device * dev, int num_copies, int flush)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    stream *s;

    /* Note that unlike close_device, end_page must not omit blank pages. */
    if (!xdev->in_page)
	pclxl_beginpage((gx_device_vector *)dev);
    s = xdev->strm;
    spputc(s, pxtEndPage);
    sflush(s);
    pclxl_page_init(xdev);
    if (ferror(xdev->file))
	return_error(gs_error_ioerror);
    return gx_finish_output_page(dev, num_copies, flush);
}

/* Close the device. */
/* Note that if this is being called as a result of finalization, */
/* the stream may no longer exist. */
private int
pclxl_close_device(gx_device * dev)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    FILE *file = xdev->file;

    if (xdev->in_page)
	fputc(pxtEndPage, file);
    px_write_file_trailer(file);
    return gdev_vector_close_file((gx_device_vector *)dev);
}

/* ------ One-for-one images ------ */

private const byte eBit_values[] = {
    0, e1Bit, 0, 0, e4Bit, 0, 0, 0, e8Bit
};

/* Copy a monochrome bitmap. */
private int
pclxl_copy_mono(gx_device * dev, const byte * data, int data_x, int raster,
		gx_bitmap_id id, int x, int y, int w, int h,
		gx_color_index zero, gx_color_index one)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    int code;
    stream *s;
    gx_color_index color0 = zero, color1 = one;
    gs_logical_operation_t lop;
    byte palette[2 * 3];
    int palette_size;
    pxeColorSpace_t color_space;

    fit_copy(dev, data, data_x, raster, id, x, y, w, h);
    code = gdev_vector_update_clip_path(vdev, NULL);
    if (code < 0)
	return code;
    pclxl_set_cursor(xdev, x, y);
    if (id != gs_no_id && zero == gx_no_color_index &&
	one != gx_no_color_index && data_x == 0
	) {
	gx_drawing_color dcolor;

	set_nonclient_dev_color(&dcolor, one);
	pclxl_setfillcolor(vdev, NULL, &dcolor);
	if (pclxl_copy_text_char(xdev, data, raster, id, w, h) >= 0)
	    return 0;
    }
    /*
     * The following doesn't work if we're writing white with a mask.
     * We'll fix it eventually.
     */
    if (zero == gx_no_color_index) {
	if (one == gx_no_color_index)
	    return 0;
	lop = rop3_S | lop_S_transparent;
	color0 = (1 << dev->color_info.depth) - 1;
    } else if (one == gx_no_color_index) {
	lop = rop3_S | lop_S_transparent;
	color1 = (1 << dev->color_info.depth) - 1;
    } else {
	lop = rop3_S;
    }
    if (dev->color_info.num_components == 1 ||
	(RGB_IS_GRAY(color0) && RGB_IS_GRAY(color1))
	) {
	palette[0] = (byte) color0;
	palette[1] = (byte) color1;
	palette_size = 2;
	color_space = eGray;
    } else {
	palette[0] = (byte) (color0 >> 16);
	palette[1] = (byte) (color0 >> 8);
	palette[2] = (byte) color0;
	palette[3] = (byte) (color1 >> 16);
	palette[4] = (byte) (color1 >> 8);
	palette[5] = (byte) color1;
	palette_size = 6;
	color_space = eRGB;
    }
    code = gdev_vector_update_log_op(vdev, lop);
    if (code < 0)
	return 0;
    pclxl_set_color_palette(xdev, color_space, palette, palette_size);
    s = pclxl_stream(xdev);
    {
	static const byte mi_[] = {
	    DUB(e1Bit), DA(pxaColorDepth),
	    DUB(eIndexedPixel), DA(pxaColorMapping)
	};

	PX_PUT_LIT(s, mi_);
    }
    pclxl_write_begin_image(xdev, w, h, w, h);
    pclxl_write_image_data(xdev, data, data_x, raster, w, 0, h);
    pclxl_write_end_image(xdev);
    return 0;
}

/* Copy a color bitmap. */
private int
pclxl_copy_color(gx_device * dev,
		 const byte * base, int sourcex, int raster, gx_bitmap_id id,
		 int x, int y, int w, int h)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    stream *s;
    uint source_bit;
    int code;

    fit_copy(dev, base, sourcex, raster, id, x, y, w, h);
    code = gdev_vector_update_clip_path(vdev, NULL);
    if (code < 0)
	return code;
    source_bit = sourcex * dev->color_info.depth;
    if ((source_bit & 7) != 0)
	return gx_default_copy_color(dev, base, sourcex, raster, id,
				     x, y, w, h);
    gdev_vector_update_log_op(vdev, rop3_S);
    pclxl_set_cursor(xdev, x, y);
    s = pclxl_stream(xdev);
    {
	static const byte ci_[] = {
	    DA(pxaColorDepth),
	    DUB(eDirectPixel), DA(pxaColorMapping)
	};

	px_put_ub(s, eBit_values[dev->color_info.depth /
				 dev->color_info.num_components]);
	PX_PUT_LIT(s, ci_);
    }
    pclxl_write_begin_image(xdev, w, h, w, h);
    pclxl_write_image_data(xdev, base, source_bit, raster,
			   w * dev->color_info.depth, 0, h);
    pclxl_write_end_image(xdev);
    return 0;
}

/* Fill a mask. */
private int
pclxl_fill_mask(gx_device * dev,
		const byte * data, int data_x, int raster, gx_bitmap_id id,
		int x, int y, int w, int h,
		const gx_drawing_color * pdcolor, int depth,
		gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    int code;
    stream *s;

    fit_copy(dev, data, data_x, raster, id, x, y, w, h);
    if ((data_x & 7) != 0 || !gx_dc_is_pure(pdcolor) || depth > 1)
	return gx_default_fill_mask(dev, data, data_x, raster, id,
				    x, y, w, h, pdcolor, depth,
				    lop, pcpath);
    code = gdev_vector_update_clip_path(vdev, pcpath);
    if (code < 0)
	return code;
    code = gdev_vector_update_fill_color(vdev, NULL, pdcolor);
    if (code < 0)
	return 0;
    pclxl_set_cursor(xdev, x, y);
    if (id != gs_no_id && data_x == 0) {
	code = gdev_vector_update_log_op(vdev, lop);
	if (code < 0)
	    return 0;
	if (pclxl_copy_text_char(xdev, data, raster, id, w, h) >= 0)
	    return 0;
    }
    code = gdev_vector_update_log_op(vdev,
				     lop | rop3_S | lop_S_transparent);
    if (code < 0)
	return 0;
    pclxl_set_color_palette(xdev, eGray, (const byte *)"\377\000", 2);
    s = pclxl_stream(xdev);
    {
	static const byte mi_[] = {
	    DUB(e1Bit), DA(pxaColorDepth),
	    DUB(eIndexedPixel), DA(pxaColorMapping)
	};

	PX_PUT_LIT(s, mi_);
    }
    pclxl_write_begin_image(xdev, w, h, w, h);
    pclxl_write_image_data(xdev, data, data_x, raster, w, 0, h);
    pclxl_write_end_image(xdev);
    return 0;
}

/* Do a RasterOp. */
private int
pclxl_strip_copy_rop(gx_device * dev, const byte * sdata, int sourcex,
		     uint sraster, gx_bitmap_id id,
		     const gx_color_index * scolors,
		     const gx_strip_bitmap * textures,
		     const gx_color_index * tcolors,
		     int x, int y, int width, int height,
		     int phase_x, int phase_y, gs_logical_operation_t lop)
{				/* We can't do general RasterOps yet. */
/****** WORK IN PROGRESS ******/
    return 0;
}

/* ------ High-level images ------ */

#define MAX_ROW_DATA 4000	/* arbitrary */
typedef struct pclxl_image_enum_s {
    gdev_vector_image_enum_common;
    gs_matrix mat;
    struct ir_ {
	byte *data;
	int num_rows;		/* # of allocated rows */
	int first_y;
	uint raster;
    } rows;
} pclxl_image_enum_t;
gs_private_st_suffix_add1(st_pclxl_image_enum, pclxl_image_enum_t,
			  "pclxl_image_enum_t", pclxl_image_enum_enum_ptrs,
			  pclxl_image_enum_reloc_ptrs, st_vector_image_enum,
			  rows.data);

/* Start processing an image. */
private int
pclxl_begin_image(gx_device * dev,
		  const gs_imager_state * pis, const gs_image_t * pim,
		  gs_image_format_t format, const gs_int_rect * prect,
		  const gx_drawing_color * pdcolor,
		  const gx_clip_path * pcpath, gs_memory_t * mem,
		  gx_image_enum_common_t ** pinfo)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pclxl *const xdev = (gx_device_pclxl *)dev;
    const gs_color_space *pcs = pim->ColorSpace;
    pclxl_image_enum_t *pie;
    byte *row_data;
    int num_rows;
    uint row_raster;
    /*
     * Following should divide by num_planes, but we only handle chunky
     * images, i.e., num_planes = 1.
     */
    int bits_per_pixel =
	(pim->ImageMask ? 1 :
	 pim->BitsPerComponent * gs_color_space_num_components(pcs));
    gs_matrix mat;
    int code;

    /*
     * Check whether we can handle this image.  PCL XL 1.0 and 2.0 only
     * handle orthogonal transformations.
     */
    gs_matrix_invert(&pim->ImageMatrix, &mat);
    gs_matrix_multiply(&mat, &ctm_only(pis), &mat);
    /* Currently we only handle portrait transformations. */
    if (mat.xx <= 0 || mat.xy != 0 || mat.yx != 0 || mat.yy <= 0 ||
	(pim->ImageMask ?
	 (!gx_dc_is_pure(pdcolor) || pim->CombineWithColor) :
	 (!pclxl_can_handle_color_space(pim->ColorSpace) ||
	  (bits_per_pixel != 1 && bits_per_pixel != 4 &&
	   bits_per_pixel != 8))) ||
	format != gs_image_format_chunky ||
	prect
	)
	goto use_default;
    row_raster = (bits_per_pixel * pim->Width + 7) >> 3;
    num_rows = MAX_ROW_DATA / row_raster;
    if (num_rows > pim->Height)
	num_rows = pim->Height;
    if (num_rows <= 0)
	num_rows = 1;
    pie = gs_alloc_struct(mem, pclxl_image_enum_t, &st_pclxl_image_enum,
			  "pclxl_begin_image");
    row_data = gs_alloc_bytes(mem, num_rows * row_raster,
			      "pclxl_begin_image(rows)");
    if (pie == 0 || row_data == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto fail;
    }
    code = gdev_vector_begin_image(vdev, pis, pim, format, prect,
				   pdcolor, pcpath, mem,
				   &pclxl_image_enum_procs,
				   (gdev_vector_image_enum_t *)pie);
    if (code < 0)
	return code;
    pie->mat = mat;
    pie->rows.data = row_data;
    pie->rows.num_rows = num_rows;
    pie->rows.first_y = 0;
    pie->rows.raster = row_raster;
    *pinfo = (gx_image_enum_common_t *) pie;
    {
	gs_logical_operation_t lop = pis->log_op;

	if (pim->ImageMask) {
	    const byte *palette = (const byte *)
		(pim->Decode[0] ? "\377\000" : "\000\377");

	    code = gdev_vector_update_fill_color(vdev, 
	                             NULL, /* use process color */
				     pdcolor);
	    if (code < 0)
		goto fail;
	    code = gdev_vector_update_log_op
		(vdev, lop | rop3_S | lop_S_transparent);
	    if (code < 0)
		goto fail;
	    pclxl_set_color_palette(xdev, eGray, palette, 2);
	} else {
	    int bpc = pim->BitsPerComponent;
	    int num_components = pie->plane_depths[0] * pie->num_planes / bpc;
	    int sample_max = (1 << bpc) - 1;
	    byte palette[256 * 3];
	    int i;

	    code = gdev_vector_update_log_op
		(vdev, (pim->CombineWithColor ? lop : rop3_know_T_0(lop)));
	    if (code < 0)
		goto fail;
	    for (i = 0; i < 1 << bits_per_pixel; ++i) {
		gs_client_color cc;
		gx_device_color devc;
		int cv = i, j;
		gx_color_index ci;

		for (j = num_components - 1; j >= 0; cv >>= bpc, --j)
		    cc.paint.values[j] = pim->Decode[j * 2] +
			(cv & sample_max) *
			(pim->Decode[j * 2 + 1] - pim->Decode[j * 2]) /
			sample_max;
		(*pcs->type->remap_color)
		    (&cc, pcs, &devc, pis, dev, gs_color_select_source);
		if (!gx_dc_is_pure(&devc))
		    return_error(gs_error_Fatal);
		ci = gx_dc_pure_color(&devc);
		if (dev->color_info.num_components == 1) {
		    palette[i] = (byte)ci;
		} else {
		    byte *ppal = &palette[i * 3];

		    ppal[0] = (byte) (ci >> 16);
		    ppal[1] = (byte) (ci >> 8);
		    ppal[2] = (byte) ci;
		}
	    }
	    if (dev->color_info.num_components == 1)
		pclxl_set_color_palette(xdev, eGray, palette,
					1 << bits_per_pixel);
	    else
		pclxl_set_color_palette(xdev, eRGB, palette,
					3 << bits_per_pixel);
	}
    }
    return 0;
 fail:
    gs_free_object(mem, row_data, "pclxl_begin_image(rows)");
    gs_free_object(mem, pie, "pclxl_begin_image");
 use_default:
    return gx_default_begin_image(dev, pis, pim, format, prect,
				  pdcolor, pcpath, mem, pinfo);
}

/* Write one strip of an image, from pie->rows.first_y to pie->y. */
private int
image_transform_x(const pclxl_image_enum_t *pie, int sx)
{
    return (int)((pie->mat.tx + sx * pie->mat.xx + 0.5) /
		 ((const gx_device_pclxl *)pie->dev)->scale.x);
}
private int
image_transform_y(const pclxl_image_enum_t *pie, int sy)
{
    return (int)((pie->mat.ty + sy * pie->mat.yy + 0.5) /
		 ((const gx_device_pclxl *)pie->dev)->scale.y);
}
private int
pclxl_image_write_rows(pclxl_image_enum_t *pie)
{
    gx_device_pclxl *const xdev = (gx_device_pclxl *)pie->dev;
    stream *s = pclxl_stream(xdev);
    int y = pie->rows.first_y;
    int h = pie->y - y;
    int xo = image_transform_x(pie, 0);
    int yo = image_transform_y(pie, y);
    int dw = image_transform_x(pie, pie->width) - xo;
    int dh = image_transform_y(pie, y + h) - yo;
    static const byte ii_[] = {
	DA(pxaColorDepth),
	DUB(eIndexedPixel), DA(pxaColorMapping)
    };

    if (dw <= 0 || dh <= 0)
	return 0;
    pclxl_set_cursor(xdev, xo, yo);
    px_put_ub(s, eBit_values[pie->bits_per_pixel]);
    PX_PUT_LIT(s, ii_);
    pclxl_write_begin_image(xdev, pie->width, h, dw, dh);
    pclxl_write_image_data(xdev, pie->rows.data, 0, pie->rows.raster,
			   pie->rows.raster << 3, 0, h);
    pclxl_write_end_image(xdev);
    return 0;
}

/* Process the next piece of an image. */
private int
pclxl_image_plane_data(gx_image_enum_common_t * info,
		       const gx_image_plane_t * planes, int height,
		       int *rows_used)
{
    pclxl_image_enum_t *pie = (pclxl_image_enum_t *) info;
    int data_bit = planes[0].data_x * info->plane_depths[0];
    int width_bits = pie->width * info->plane_depths[0];
    int i;

    /****** SHOULD HANDLE NON-BYTE-ALIGNED DATA ******/
    if (width_bits != pie->bits_per_row || (data_bit & 7) != 0)
	return_error(gs_error_rangecheck);
    if (height > pie->height - pie->y)
	height = pie->height - pie->y;
    for (i = 0; i < height; pie->y++, ++i) {
	if (pie->y - pie->rows.first_y == pie->rows.num_rows) {
	    int code = pclxl_image_write_rows(pie);

	    if (code < 0)
		return code;
	    pie->rows.first_y = pie->y;
	}
	memcpy(pie->rows.data +
	         pie->rows.raster * (pie->y - pie->rows.first_y),
	       planes[0].data + planes[0].raster * i + (data_bit >> 3),
	       pie->rows.raster);
    }
    *rows_used = height;
    return pie->y >= pie->height;
}

/* Clean up by releasing the buffers. */
private int
pclxl_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    pclxl_image_enum_t *pie = (pclxl_image_enum_t *) info;
    int code = 0;

    /* Write the final strip, if any. */
    if (pie->y > pie->rows.first_y && draw_last)
	code = pclxl_image_write_rows(pie);
    gs_free_object(pie->memory, pie->rows.data, "pclxl_end_image(rows)");
    gs_free_object(pie->memory, pie, "pclxl_end_image");
    return code;
}

/* Get parameters. */
int
pclxl_get_params(gx_device *dev, gs_param_list *plist)
{
    gx_device_pclxl *pdev = (gx_device_pclxl *) dev;
    int code = gdev_vector_get_params(dev, plist);

    if (code < 0)
	return code;

     if (code >= 0)
	code = param_write_bool(plist, "ManualFeed", &pdev->ManualFeed);
    return code;
}

/* Put parameters. */
int
pclxl_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_pclxl *pdev = (gx_device_pclxl *) dev;
    int code = 0;
    bool ManualFeed;
    bool ManualFeed_set = false;
    int MediaPosition;
    bool MediaPosition_set = false;

    code = param_read_bool(plist, "ManualFeed", &ManualFeed);
    if (code == 0) 
	ManualFeed_set = true;
    if (code >= 0) {
	code = param_read_int(plist, "%MediaSource", &MediaPosition);
	if (code == 0) 
	    MediaPosition_set = true;
	else if (code < 0) {
	    if (param_read_null(plist, "%MediaSource") == 0) {
		code = 0;
	    }
	}
    }

    /* note this handles not opening/closing the device */
    code = gdev_vector_put_params(dev, plist);
    if (code < 0)
	return code;

    if (code >= 0) {
	if (ManualFeed_set) {
	    pdev->ManualFeed = ManualFeed;
	    pdev->ManualFeed_set = true;
	}
	if (MediaPosition_set) {
	    pdev->MediaPosition = MediaPosition;
	    pdev->MediaPosition_set = true;
	}
    }
    return code;
}
