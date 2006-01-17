/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevtrac.c,v 1.5 2003/06/06 17:50:27 igor Exp $ */
/* Tracing device (including sample high-level implementation) */
#include "gx.h"
#include "gserrors.h"
#include "gscspace.h"
#include "gxdevice.h"
#include "gxtmap.h"		/* for gxdht.h */
#include "gxdht.h"
#include "gxfont.h"
#include "gxiparam.h"
#include "gxistate.h"
#include "gxpaint.h"
#include "gzpath.h"
#include "gzcpath.h"

/* Define the default resolution. */
#ifndef X_DPI
#  define X_DPI 720
#endif
#ifndef Y_DPI
#  define Y_DPI 720
#endif

extern_st(st_gs_text_enum);
extern_st(st_gx_image_enum_common);

/* ---------------- Internal utilities ---------------- */

private void
trace_drawing_color(const char *prefix, const gx_drawing_color *pdcolor)
{
    dprintf1("%scolor=", prefix);
    if (pdcolor->type == gx_dc_type_none)
	dputs("none");
    else if (pdcolor->type == gx_dc_type_null)
	dputs("null");
    else if (pdcolor->type == gx_dc_type_pure)
	dprintf1("0x%lx", (ulong)pdcolor->colors.pure);
    else if (pdcolor->type == gx_dc_type_ht_binary) {
	int ci = pdcolor->colors.binary.b_index;

	dprintf5("binary(0x%lx, 0x%lx, %d/%d, index=%d)",
		 (ulong)pdcolor->colors.binary.color[0],
		 (ulong)pdcolor->colors.binary.color[1],
		 pdcolor->colors.binary.b_level,
		 (ci < 0 ? pdcolor->colors.binary.b_ht->order.num_bits :
		  pdcolor->colors.binary.b_ht->components[ci].corder.num_bits),
		 ci);
    } else if (pdcolor->type == gx_dc_type_ht_colored) {
	ulong plane_mask = pdcolor->colors.colored.plane_mask;
	int ci;

	dprintf1("colored(mask=%lu", plane_mask);
	for (ci = 0; plane_mask != 0; ++ci, plane_mask >>= 1)
	    if (plane_mask & 1) {
		dprintf2(", (base=%u, level=%u)",
			 pdcolor->colors.colored.c_base[ci],
			 pdcolor->colors.colored.c_level[ci]);
	    } else
		dputs(", -");
    } else {
	dputs("**unknown**");
    }
}

private void
trace_lop(gs_logical_operation_t lop)
{
    dprintf1(", lop=0x%x", (uint)lop);
}

private void
trace_path(const gx_path *path)
{
    gs_path_enum penum;

    gx_path_enum_init(&penum, path);
    for (;;) {
	gs_fixed_point pts[3];

	switch (gx_path_enum_next(&penum, pts)) {
	case gs_pe_moveto:
	    dprintf2("    %g %g moveto\n", fixed2float(pts[0].x),
		     fixed2float(pts[0].y));
	    continue;
	case gs_pe_lineto:
	    dprintf2("    %g %g lineto\n", fixed2float(pts[0].x),
		     fixed2float(pts[0].y));
	    continue;
	case gs_pe_curveto:
	    dprintf6("    %g %g %g %g %g %g curveto\n", fixed2float(pts[0].x),
		     fixed2float(pts[0].y), fixed2float(pts[1].x),
		     fixed2float(pts[1].y), fixed2float(pts[2].x),
		     fixed2float(pts[2].y));
	    continue;
	case gs_pe_closepath:
	    dputs("    closepath\n");
	    continue;
	default:
	    break;
	}
	break;
    }
}

private void
trace_clip(gx_device *dev, const gx_clip_path *pcpath)
{
    if (pcpath == 0)
	return;
    if (gx_cpath_includes_rectangle(pcpath, fixed_0, fixed_0,
				    int2fixed(dev->width),
				    int2fixed(dev->height))
	)
	return;
    dputs(", clip={");
    if (pcpath->path_valid)
	trace_path(&pcpath->path);
    else
	dputs("NO PATH");
    dputs("}");
}

/* ---------------- Low-level driver procedures ---------------- */

private int
trace_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		     gx_color_index color)
{
    dprintf5("fill_rectangle(%d, %d, %d, %d, 0x%lx)\n",
	     x, y, w, h, (ulong)color);
    return 0;
}

private int
trace_copy_mono(gx_device * dev, const byte * data,
		int dx, int raster, gx_bitmap_id id,
		int x, int y, int w, int h,
		gx_color_index zero, gx_color_index one)
{
    dprintf7("copy_mono(x=%d, y=%d, w=%d, h=%d, dx=%d, raster=%d, id=0x%lx,\n",
	     x, y, w, h, dx, raster, (ulong)id);
    dprintf2("    colors=(0x%lx,0x%lx))\n", (ulong)zero, (ulong)one);
    return 0;
}

private int
trace_copy_color(gx_device * dev, const byte * data,
		 int dx, int raster, gx_bitmap_id id,
		 int x, int y, int w, int h)
{
    dprintf7("copy_color(x=%d, y=%d, w=%d, h=%d, dx=%d, raster=%d, id=0x%lx)\n",
	     x, y, w, h, dx, raster, (ulong)id);
    return 0;
}

private int
trace_copy_alpha(gx_device * dev, const byte * data, int dx, int raster,
		 gx_bitmap_id id, int x, int y, int w, int h,
		 gx_color_index color, int depth)
{
    dprintf7("copy_alpha(x=%d, y=%d, w=%d, h=%d, dx=%d, raster=%d, id=0x%lx,\n",
	     x, y, w, h, dx, raster, (ulong)id);
    dprintf2("    color=0x%lx, depth=%d)\n", (ulong)color, depth);
    return 0;
}

private int
trace_fill_mask(gx_device * dev,
		const byte * data, int dx, int raster, gx_bitmap_id id,
		int x, int y, int w, int h,
		const gx_drawing_color * pdcolor, int depth,
		gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    dprintf7("fill_mask(x=%d, y=%d, w=%d, h=%d, dx=%d, raster=%d, id=0x%lx,\n",
	     x, y, w, h, dx, raster, (ulong)id);
    trace_drawing_color("    ", pdcolor);
    dprintf1(", depth=%d", depth);
    trace_lop(lop);
    trace_clip(dev, pcpath);
    dputs(")\n");
    return 0;
}

private int
trace_fill_trapezoid(gx_device * dev,
		     const gs_fixed_edge * left,
		     const gs_fixed_edge * right,
		     fixed ybot, fixed ytop, bool swap_axes,
		     const gx_drawing_color * pdcolor,
		     gs_logical_operation_t lop)
{
    dputs("**fill_trapezoid**\n");
    return 0;
}

private int
trace_fill_parallelogram(gx_device * dev,
			 fixed px, fixed py, fixed ax, fixed ay,
			 fixed bx, fixed by, const gx_drawing_color * pdcolor,
			 gs_logical_operation_t lop)
{
    dprintf6("fill_parallelogram((%g,%g), (%g,%g), (%g,%g)",
	     fixed2float(px), fixed2float(py), fixed2float(ax),
	     fixed2float(ay), fixed2float(bx), fixed2float(by));
    trace_drawing_color(", ", pdcolor);
    trace_lop(lop);
    dputs(")\n");
    return 0;
}

private int
trace_fill_triangle(gx_device * dev,
		    fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		    const gx_drawing_color * pdcolor,
		    gs_logical_operation_t lop)
{
    dprintf6("fill_triangle((%g,%g), (%g,%g), (%g,%g)",
	     fixed2float(px), fixed2float(py), fixed2float(ax),
	     fixed2float(ay), fixed2float(bx), fixed2float(by));
    trace_drawing_color(", ", pdcolor);
    trace_lop(lop);
    dputs(")\n");
    return 0;
}

private int
trace_draw_thin_line(gx_device * dev,
		     fixed fx0, fixed fy0, fixed fx1, fixed fy1,
		     const gx_drawing_color * pdcolor,
		     gs_logical_operation_t lop)
{
    dprintf4("draw_thin_line((%g,%g), (%g,%g)",
	     fixed2float(fx0), fixed2float(fy0), fixed2float(fx1),
	     fixed2float(fy1));
    trace_drawing_color(", ", pdcolor);
    trace_lop(lop);
    dputs(")\n");
    return 0;
}

private int
trace_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
			   int x, int y, int w, int h,
			   gx_color_index color0, gx_color_index color1,
			   int px, int py)
{
    dprintf6("strip_tile_rectangle(x=%d, y=%d, w=%d, h=%d, colors=(0x%lx,0x%lx),\n",
	     x, y, w, h, (ulong)color0, (ulong)color1);
    dprintf8("    size=(%d,%d) shift %u, rep=(%u,%u) shift %u, phase=(%d,%d))\n",
	     tiles->size.x, tiles->size.y, tiles->shift,
	     tiles->rep_width, tiles->rep_height, tiles->rep_shift, px, py);
    return 0;
}

private int
trace_strip_copy_rop(gx_device * dev, const byte * sdata, int sourcex,
		     uint sraster, gx_bitmap_id id,
		     const gx_color_index * scolors,
		     const gx_strip_bitmap * textures,
		     const gx_color_index * tcolors,
		     int x, int y, int width, int height,
		     int phase_x, int phase_y, gs_logical_operation_t lop)
{
    dputs("**strip_copy_rop**\n");
    return 0;
}

/* ---------------- High-level driver procedures ---------------- */

private int
trace_fill_path(gx_device * dev, const gs_imager_state * pis,
		gx_path * ppath, const gx_fill_params * params,
		const gx_drawing_color * pdcolor,
		const gx_clip_path * pcpath)
{
    dputs("fill_path({\n");
    trace_path(ppath);
    trace_drawing_color("}, ", pdcolor);
    dprintf5(", rule=%d, adjust=(%g,%g), flatness=%g, fill_zero_width=%s",
	     params->rule, fixed2float(params->adjust.x),
	     fixed2float(params->adjust.y), params->flatness,
	     (params->fill_zero_width ? "true" : "false"));
    trace_clip(dev, pcpath);
    /****** pis ******/
    dputs(")\n");
    return 0;
}

private int
trace_stroke_path(gx_device * dev, const gs_imager_state * pis,
		  gx_path * ppath, const gx_stroke_params * params,
		  const gx_drawing_color * pdcolor,
		  const gx_clip_path * pcpath)
{
    dputs("stroke_path({\n");
    trace_path(ppath);
    trace_drawing_color("}, ", pdcolor);
    dprintf1(", flatness=%g", params->flatness);
    trace_clip(dev, pcpath);
    /****** pis ******/
    dputs(")\n");
    return 0;
}

typedef struct trace_image_enum_s {
    gx_image_enum_common;
    gs_memory_t *memory;
    int rows_left;
} trace_image_enum_t;
gs_private_st_suffix_add0(st_trace_image_enum, trace_image_enum_t,
			  "trace_image_enum_t", trace_image_enum_enum_ptrs,
			  trace_image_enum_reloc_ptrs,
			  st_gx_image_enum_common);
private int
trace_plane_data(gx_image_enum_common_t * info,
		 const gx_image_plane_t * planes, int height,
		 int *rows_used)
{
    trace_image_enum_t *pie = (trace_image_enum_t *)info;
    int i;

    dprintf1("image_plane_data(height=%d", height);
    for (i = 0; i < pie->num_planes; ++i) {
	if (planes[i].data)
	    dprintf4(", {depth=%d, width=%d, dx=%d, raster=%u}",
		     pie->plane_depths[i], pie->plane_widths[i],
		     planes[i].data_x, planes[i].raster);
	else
	    dputs(", -");
    }
    dputs(")\n");
    *rows_used = height;
    return (pie->rows_left -= height) <= 0;
}
private int
trace_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    trace_image_enum_t *pie = (trace_image_enum_t *)info;

    gs_free_object(pie->memory, pie, "trace_end_image");
    return 0;
}
private const gx_image_enum_procs_t trace_image_enum_procs = {
    trace_plane_data, trace_end_image
};
private int
trace_begin_typed_image(gx_device * dev, const gs_imager_state * pis,
			const gs_matrix * pmat,
			const gs_image_common_t * pim,
			const gs_int_rect * prect,
			const gx_drawing_color * pdcolor,
			const gx_clip_path * pcpath,
			gs_memory_t * memory,
			gx_image_enum_common_t ** pinfo)
{
    trace_image_enum_t *pie;
    const gs_pixel_image_t *ppi = (const gs_pixel_image_t *)pim;
    int ncomp;

    dprintf7("begin_typed_image(type=%d, ImageMatrix=[%g %g %g %g %g %g]",
	     pim->type->index, pim->ImageMatrix.xx, pim->ImageMatrix.xy,
	     pim->ImageMatrix.yx, pim->ImageMatrix.yy,
	     pim->ImageMatrix.tx, pim->ImageMatrix.ty);
    switch (pim->type->index) {
    case 1:
	if (((const gs_image1_t *)ppi)->ImageMask) {
	    ncomp = 1;
	    break;
	}
	/* falls through */
    case 4:
	ncomp = gs_color_space_num_components(ppi->ColorSpace);
	break;
    case 3:
	ncomp = gs_color_space_num_components(ppi->ColorSpace) + 1;
	break;
    case 2:			/* no data */
	dputs(")\n");
	return 1;
    default:
	goto dflt;
    }
    pie = gs_alloc_struct(memory, trace_image_enum_t, &st_trace_image_enum,
			  "trace_begin_typed_image");
    if (pie == 0)
	goto dflt;
    if (gx_image_enum_common_init((gx_image_enum_common_t *)pie,
				  (const gs_data_image_t *)pim,
				  &trace_image_enum_procs, dev, ncomp,
				  ppi->format) < 0
	)
	goto dflt;
    dprintf4("\n    Width=%d, Height=%d, BPC=%d, num_components=%d)\n",
	     ppi->Width, ppi->Height, ppi->BitsPerComponent, ncomp);
    pie->memory = memory;
    pie->rows_left = ppi->Height;
    *pinfo = (gx_image_enum_common_t *)pie;
    return 0;
 dflt:
    dputs(") DEFAULTED\n");
    return gx_default_begin_typed_image(dev, pis, pmat, pim, prect, pdcolor,
					pcpath, memory, pinfo);
}

private int
trace_text_process(gs_text_enum_t *pte)
{
    return 0;
}
private const gs_text_enum_procs_t trace_text_procs = {
    NULL, trace_text_process, NULL, NULL, NULL, NULL,
    gx_default_text_release
};
private int
trace_text_begin(gx_device * dev, gs_imager_state * pis,
		 const gs_text_params_t * text, gs_font * font,
		 gx_path * path, const gx_device_color * pdcolor,
		 const gx_clip_path * pcpath, gs_memory_t * memory,
		 gs_text_enum_t ** ppenum)
{
    static const char *const tags[sizeof(text->operation) * 8] = {
	"FROM_STRING", "FROM_BYTES", "FROM_CHARS", "FROM_GLYPHS",
	"FROM_SINGLE_CHAR", "FROM_SINGLE_GLYPH",
	"ADD_TO_ALL_WIDTHS", "ADD_TO_SPACE_WIDTH",
	"REPLACE_WIDTHS", "DO_NONE", "DO_DRAW", "DO_CHARWIDTH",
	"DO_FALSE_CHARPATH", "DO_TRUE_CHARPATH",
	"DO_FALSE_CHARBOXPATH", "DO_TRUE_CHARBOXPATH",
	"INTERVENE", "RETURN_WIDTH"
    };
    int i;
    gs_text_enum_t *pte;
    int code;

    dputs("text_begin(");
    for (i = 0; i < countof(tags); ++i)
	if (text->operation & (1 << i)) {
	    if (tags[i])
		dprintf1("%s ", tags[i]);
	    else
		dprintf1("%d? ", i);
	}
    dprintf1("font=%s\n           text=(", font->font_name.chars);
    if (text->operation & TEXT_FROM_SINGLE_CHAR)
	dprintf1("0x%lx", (ulong)text->data.d_char);
    else if (text->operation & TEXT_FROM_SINGLE_GLYPH)
	dprintf1("0x%lx", (ulong)text->data.d_glyph);
    else
	for (i = 0; i < text->size; ++i) {
	    if (text->operation & TEXT_FROM_STRING)
		dputc(text->data.bytes[i]);
	    else
		dprintf1("0x%lx ",
			 (text->operation & TEXT_FROM_GLYPHS ?
			  (ulong)text->data.glyphs[i] :
			  (ulong)text->data.chars[i]));
    }
    dprintf1(")\n           size=%u", text->size);
    if (text->operation & TEXT_ADD_TO_ALL_WIDTHS)
	dprintf2(", delta_all=(%g,%g)", text->delta_all.x, text->delta_all.y);
    if (text->operation & TEXT_ADD_TO_SPACE_WIDTH) {
	dprintf3(", space=0x%lx, delta_space=(%g,%g)",
		 (text->operation & TEXT_FROM_GLYPHS ?
		  (ulong)text->space.s_glyph : (ulong)text->space.s_char),
		 text->delta_space.x, text->delta_space.y);
    }
    if (text->operation & TEXT_REPLACE_WIDTHS) {
	dputs("\n           widths=");
	for (i = 0; i < text->widths_size; ++i) {
	    if (text->x_widths)
		dprintf1("(%g,", text->x_widths[i]);
	    else
		dputs("(,");
	    if (text->y_widths)
		dprintf1("%g)",
			 (text->y_widths == text->x_widths ?
			  text->y_widths[++i] : text->y_widths[i]));
	    else
		dputs(")");
	}
    }
    if (text->operation & TEXT_DO_DRAW)
	trace_drawing_color(", ", pdcolor);
    /*
     * We can't do it if CHAR*PATH or INTERVENE, or if (RETURN_WIDTH and not
     * REPLACE_WIDTHS and we can't get the widths from the font).
     */
    if (text->operation &
	(TEXT_DO_FALSE_CHARPATH | TEXT_DO_TRUE_CHARPATH |
	 TEXT_DO_FALSE_CHARBOXPATH | TEXT_DO_TRUE_CHARBOXPATH |
	 TEXT_INTERVENE)
	)
	goto dflt;
    rc_alloc_struct_1(pte, gs_text_enum_t, &st_gs_text_enum, memory,
		      goto dflt, "trace_text_begin");
    code = gs_text_enum_init(pte, &trace_text_procs, dev, pis, text, font,
			     path, pdcolor, pcpath, memory);
    if (code < 0)
	goto dfree;
    if ((text->operation & (TEXT_DO_CHARWIDTH | TEXT_RETURN_WIDTH)) &&
	!(text->operation & TEXT_REPLACE_WIDTHS)
	) {
	/*
	 * Get the widths from the font.  This code is mostly copied from
	 * the pdfwrite driver, and should probably be shared with it.
	 * ****** WRONG IF Type 0 FONT ******
	 */
	int i;
	gs_point w;
	double scale = (font->FontType == ft_TrueType ? 0.001 : 1.0);
	gs_fixed_point origin;
	gs_point dpt;
	int num_spaces = 0;

	if (!(text->operation & TEXT_FROM_STRING))
	    goto dfree;		/* can't handle yet */
	if (gx_path_current_point(path, &origin) < 0)
	    goto dfree;
	w.x = 0, w.y = 0;
	for (i = 0; i < text->size; ++i) {
	    gs_char ch = text->data.bytes[i];
	    int wmode = font->WMode;
	    gs_glyph glyph =
		((gs_font_base *)font)->procs.encode_char(font, ch,
							  GLYPH_SPACE_NAME);
	    gs_glyph_info_t info;

	    if (glyph != gs_no_glyph &&
		(code = font->procs.glyph_info(font, glyph, NULL,
					       GLYPH_INFO_WIDTH0 << wmode,
					       &info)) >= 0
	    ) {
		w.x += info.width[wmode].x;
		w.y += info.width[wmode].y;
	    } else
		goto dfree;
	    if (ch == text->space.s_char)
		++num_spaces;
	}
	gs_distance_transform(w.x * scale, w.y * scale,
			      &font->FontMatrix, &dpt);
	if (text->operation & TEXT_ADD_TO_ALL_WIDTHS) {
	    int num_chars = text->size;

	    dpt.x += text->delta_all.x * num_chars;
	    dpt.y += text->delta_all.y * num_chars;
	}
	if (text->operation & TEXT_ADD_TO_SPACE_WIDTH) {
	    dpt.x += text->delta_space.x * num_spaces;
	    dpt.y += text->delta_space.y * num_spaces;
	}
	pte->returned.total_width = dpt;
	gs_distance_transform(dpt.x, dpt.y, &ctm_only(pis), &dpt);
	code = gx_path_add_point(path,
				 origin.x + float2fixed(dpt.x),
				 origin.y + float2fixed(dpt.y));
	if (code < 0)
	    goto dfree;
    }
    dputs(")\n");
    *ppenum = pte;
    return 0;
 dfree:
    gs_free_object(memory, pte, "trace_text_begin");
 dflt:
    dputs(") DEFAULTED\n");
    return gx_default_text_begin(dev, pis, text, font, path, pdcolor,
				 pcpath, memory, ppenum);
}

/* ---------------- The device definition ---------------- */

#define TRACE_DEVICE_BODY(dname, ncomp, depth, map_rgb_color, map_color_rgb, map_cmyk_color, map_rgb_alpha_color)\
    std_device_dci_body(gx_device, 0, dname,\
			DEFAULT_WIDTH_10THS * X_DPI / 10,\
			DEFAULT_HEIGHT_10THS * Y_DPI / 10,\
			X_DPI, Y_DPI, ncomp, depth,\
			(1 << (depth / ncomp)) - 1,\
			(ncomp > 1 ? (1 << (depth / ncomp)) - 1 : 0),\
			1 << (depth / ncomp),\
			(ncomp > 1 ? 1 << (depth / ncomp) : 1)),\
{\
     NULL,			/* open_device */\
     NULL,			/* get_initial_matrix */\
     NULL,			/* sync_output */\
     NULL,			/* output_page */\
     NULL,			/* close_device */\
     map_rgb_color,		/* differs */\
     map_color_rgb,		/* differs */\
     trace_fill_rectangle,\
     NULL,			/* tile_rectangle */\
     trace_copy_mono,\
     trace_copy_color,\
     NULL,			/* draw_line */\
     NULL,			/* get_bits */\
     NULL,			/* get_params */\
     NULL,			/* put_params */\
     map_cmyk_color,		/* differs */\
     NULL,			/* get_xfont_procs */\
     NULL,			/* get_xfont_device */\
     map_rgb_alpha_color,	/* differs */\
     gx_page_device_get_page_device,\
     NULL,			/* get_alpha_bits */\
     trace_copy_alpha,\
     NULL,			/* get_band */\
     NULL,			/* copy_rop */\
     trace_fill_path,\
     trace_stroke_path,\
     trace_fill_mask,\
     trace_fill_trapezoid,\
     trace_fill_parallelogram,\
     trace_fill_triangle,\
     trace_draw_thin_line,\
     NULL,			/* begin_image */\
     NULL,			/* image_data */\
     NULL,			/* end_image */\
     trace_strip_tile_rectangle,\
     trace_strip_copy_rop,\
     NULL,			/* get_clipping_box */\
     trace_begin_typed_image,\
     NULL,			/* get_bits_rectangle */\
     NULL,			/* map_color_rgb_alpha */\
     NULL,			/* create_compositor */\
     NULL,			/* get_hardware_params */\
     trace_text_begin,\
     NULL			/* finish_copydevice */\
}    

const gx_device gs_tr_mono_device = {
    TRACE_DEVICE_BODY("tr_mono", 1, 1,
		      gx_default_b_w_map_rgb_color,
		      gx_default_b_w_map_color_rgb, NULL, NULL)
};

const gx_device gs_tr_rgb_device = {
    TRACE_DEVICE_BODY("tr_rgb", 3, 24,
		      gx_default_rgb_map_rgb_color,
		      gx_default_rgb_map_color_rgb, NULL, NULL)
};

const gx_device gs_tr_cmyk_device = {
    TRACE_DEVICE_BODY("tr_cmyk", 4, 4,
		      NULL, cmyk_1bit_map_color_rgb,
		      cmyk_1bit_map_cmyk_color, NULL)
};
