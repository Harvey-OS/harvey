/* Copyright (C) 1997, 2000-2004 artofcode LLC. All rights reserved.
  
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

/* $Id: gdevps.c,v 1.40 2004/10/07 05:18:34 ray Exp $ */
/* PostScript-writing driver */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "time_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscdefs.h"
#include "gsmatrix.h"		/* for gsiparam.h */
#include "gsiparam.h"
#include "gsline.h"
#include "gsparam.h"
#include "gxdevice.h"
#include "gscspace.h"
#include "gxdcolor.h"
#include "gxpath.h"
#include "spprint.h"
#include "strimpl.h"
#include "sstring.h"
#include "sa85x.h"
#include "gdevpsdf.h"
#include "gdevpsu.h"

/* Current ProcSet version */
#define PSWRITE_PROCSET_VERSION 1

/* Guaranteed size of operand stack accoeding to PLRM */
#define MAXOPSTACK 500

private int psw_open_printer(gx_device * dev);
private int psw_close_printer(gx_device * dev);

/* ---------------- Device definition ---------------- */

/* Device procedures */
private dev_proc_open_device(psw_open);
private dev_proc_output_page(psw_output_page);
private dev_proc_close_device(psw_close);
private dev_proc_fill_rectangle(psw_fill_rectangle);
private dev_proc_copy_mono(psw_copy_mono);
private dev_proc_copy_color(psw_copy_color);
private dev_proc_put_params(psw_put_params);
private dev_proc_get_params(psw_get_params);
private dev_proc_fill_path(psw_fill_path);
private dev_proc_stroke_path(psw_stroke_path);
private dev_proc_fill_mask(psw_fill_mask);
private dev_proc_begin_image(psw_begin_image);

#define X_DPI 720
#define Y_DPI 720

typedef struct psw_path_state_s {
    int num_points;		/* # of points since last non-lineto */
    int move;			/* 1 iff last non-lineto was moveto, else 0 */
    gs_point dprev[2];		/* line deltas before previous point, */
				/* if num_points - move >= 2 */
} psw_path_state_t;

typedef struct psw_image_params_s {
    gx_bitmap_id id;
    ushort width, height;
} psw_image_params_t;

typedef struct gx_device_pswrite_s {
    gx_device_psdf_common;
    /* LanguageLevel in pswrite_common is settable. */
    gx_device_pswrite_common_t pswrite_common;
#define LanguageLevel_default 2.0
#define psdf_version_default psdf_version_level2
    /* End of parameters */
    bool first_page;
    psdf_binary_writer *image_writer;
#define image_stream image_writer->strm
#define image_cache_size 197
#define image_cache_reprobe_step 121
    psw_image_params_t image_cache[image_cache_size];
    bool cache_toggle;
    struct pf_ {
	gs_int_rect rect;
	gx_color_index color;
    } page_fill;
    /* Temporary state while writing a path */
    psw_path_state_t path_state;
} gx_device_pswrite;

/* GC descriptor and procedures */
gs_private_st_suffix_add1_final(st_device_pswrite, gx_device_pswrite,
				"gx_device_pswrite", device_pswrite_enum_ptrs,
				device_pswrite_reloc_ptrs, gx_device_finalize,
				st_device_psdf, image_writer);

#define psw_procs\
	{	psw_open,\
		gx_upright_get_initial_matrix,\
		NULL,			/* sync_output */\
		psw_output_page,\
		psw_close,\
		gx_default_rgb_map_rgb_color,\
		gx_default_rgb_map_color_rgb,\
		psw_fill_rectangle,\
		NULL,			/* tile_rectangle */\
		psw_copy_mono,\
		psw_copy_color,\
		NULL,			/* draw_line */\
		psdf_get_bits,\
		psw_get_params,\
		psw_put_params,\
		NULL,			/* map_cmyk_color */\
		NULL,			/* get_xfont_procs */\
		NULL,			/* get_xfont_device */\
		NULL,			/* map_rgb_alpha_color */\
		gx_page_device_get_page_device,\
		NULL,			/* get_alpha_bits */\
		NULL,			/* copy_alpha */\
		NULL,			/* get_band */\
		NULL,			/* copy_rop */\
		psw_fill_path,\
		psw_stroke_path,\
		psw_fill_mask,\
		gdev_vector_fill_trapezoid,\
		gdev_vector_fill_parallelogram,\
		gdev_vector_fill_triangle,\
		NULL /****** WRONG ******/,	/* draw_thin_line */\
		psw_begin_image,\
		NULL,			/* image_data */\
		NULL,			/* end_image */\
		NULL,			/* strip_tile_rectangle */\
		NULL,/******psw_strip_copy_rop******/\
		NULL,			/* get_clipping_box */\
		NULL,			/* begin_typed_image */\
		psdf_get_bits_rectangle,\
		NULL,			/* map_color_rgb_alpha */\
		psdf_create_compositor\
	}

const gx_device_pswrite gs_pswrite_device = {
    std_device_dci_type_body(gx_device_pswrite, 0, "pswrite",
			     &st_device_pswrite,
			     DEFAULT_WIDTH_10THS * X_DPI / 10,
			     DEFAULT_HEIGHT_10THS * Y_DPI / 10,
			     X_DPI, Y_DPI, 3, 24, 255, 255, 256, 256),
    psw_procs,
    psdf_initial_values(psdf_version_default, 1 /*true */ ),	/* (ASCII85EncodePages) */
    PSWRITE_COMMON_VALUES(LanguageLevel_default, /* LanguageLevel */
			  0 /*false*/, /* ProduceEPS */
			  PSWRITE_PROCSET_VERSION /* ProcSet_version */)
};

const gx_device_pswrite gs_epswrite_device = {
    std_device_dci_type_body(gx_device_pswrite, 0, "epswrite",
			     &st_device_pswrite,
			     DEFAULT_WIDTH_10THS * X_DPI / 10,
			     DEFAULT_HEIGHT_10THS * Y_DPI / 10,
			     X_DPI, Y_DPI, 3, 24, 255, 255, 256, 256),
    psw_procs,
    psdf_initial_values(psdf_version_default, 1 /*true */ ),	/* (ASCII85EncodePages) */
    PSWRITE_COMMON_VALUES(LanguageLevel_default, /* LanguageLevel */
			  1 /*true*/, /* ProduceEPS */
			  PSWRITE_PROCSET_VERSION /* ProcSet_version */)
};

/* Vector device implementation */
private int
    psw_beginpage(gx_device_vector * vdev),
    psw_can_handle_hl_color(gx_device_vector * vdev, const gs_imager_state * pis, 
                  const gx_drawing_color * pdc),
    psw_setcolors(gx_device_vector * vdev, const gs_imager_state * pis, 
                  const gx_drawing_color * pdc),
    psw_dorect(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1,
	       fixed y1, gx_path_type_t type),
    psw_beginpath(gx_device_vector * vdev, gx_path_type_t type),
    psw_moveto(gx_device_vector * vdev, floatp x0, floatp y0,
	       floatp x, floatp y, gx_path_type_t type),
    psw_lineto(gx_device_vector * vdev, floatp x0, floatp y0,
	       floatp x, floatp y, gx_path_type_t type),
    psw_curveto(gx_device_vector * vdev, floatp x0, floatp y0,
		floatp x1, floatp y1, floatp x2, floatp y2,
		floatp x3, floatp y3, gx_path_type_t type),
    psw_closepath(gx_device_vector * vdev, floatp x0, floatp y0,
		  floatp x_start, floatp y_start, gx_path_type_t type),
    psw_endpath(gx_device_vector * vdev, gx_path_type_t type);
private const gx_device_vector_procs psw_vector_procs = {
	/* Page management */
    psw_beginpage,
	/* Imager state */
    psdf_setlinewidth,
    psdf_setlinecap,
    psdf_setlinejoin,
    psdf_setmiterlimit,
    psdf_setdash,
    psdf_setflat,
    psdf_setlogop,
	/* Other state */
    psw_can_handle_hl_color,
    psw_setcolors,		/* fill & stroke colors are the same */
    psw_setcolors,
	/* Paths */
    psdf_dopath,
    psw_dorect,
    psw_beginpath,
    psw_moveto,
    psw_lineto,
    psw_curveto,
    psw_closepath,
    psw_endpath
};

/* ---------------- File header ---------------- */

/*
 * NOTE: Increment PSWRITE_PROCSET_VERSION (above) whenever the following
 * definitions change.
 */

private const char *const psw_procset[] = {
    "/!{bind def}bind def/#{load def}!/N/counttomark #",
	/* <rbyte> <gbyte> <bbyte> rG - */
	/* <graybyte> G - */
 "/rG{3{3 -1 roll 255 div}repeat setrgbcolor}!/G{255 div setgray}!/K{0 G}!",
	/* <bbyte> <rgbyte> r6 - */
	/* <gbyte> <rbbyte> r5 - */
	/* <rbyte> <gbbyte> r3 - */
    "/r6{dup 3 -1 roll rG}!/r5{dup 3 1 roll rG}!/r3{dup rG}!",
    "/w/setlinewidth #/J/setlinecap #",
    "/j/setlinejoin #/M/setmiterlimit #/d/setdash #/i/setflat #",
    "/m/moveto #/l/lineto #/c/rcurveto #",
	/* <dx1> <dy1> ... <dxn> <dyn> p - */
    "/p{N 2 idiv{N -2 roll rlineto}repeat}!",
	/* <x> <y> <dx1> <dy1> ... <dxn> <dyn> P - */
    "/P{N 0 gt{N -2 roll moveto p}if}!",
    "/h{p closepath}!/H{P closepath}!",
	/* <dx> lx - */
	/* <dy> ly - */
	/* <dx2> <dy2> <dx3> <dy3> v - */
	/* <dx1> <dy1> <dx2> <dy2> y - */
    "/lx{0 rlineto}!/ly{0 exch rlineto}!/v{0 0 6 2 roll c}!/y{2 copy c}!",
	/* <x> <y> <dx> <dy> re - */
    "/re{4 -2 roll m exch dup lx exch ly neg lx h}!",
	/* <x> <y> <a> <b> ^ <x> <y> <a> <b> <-x> <-y> */
    "/^{3 index neg 3 index neg}!",
    "/f{P fill}!/f*{P eofill}!/s{H stroke}!/S{P stroke}!",
    "/q/gsave #/Q/grestore #/rf{re fill}!",
    "/Y{P clip newpath}!/Y*{P eoclip newpath}!/rY{re Y}!",
	/* <w> <h> <name> <data> <?> |= <w> <h> <data> */
    "/|={pop exch 4 1 roll 1 array astore cvx 3 array astore cvx exch 1 index def exec}!",
	/* <w> <h> <name> <length> <src> | <w> <h> <data> */
    "/|{exch string readstring |=}!",
	/* <w> <?> <name> (<length>|) + <w> <?> <name> <length> */
    "/+{dup type/nametype eq{2 index 7 add -3 bitshift 2 index mul}if}!",
	/* <w> <h> <name> (<length>|) $ <w> <h> <data> */
    "/@/currentfile #/${+ @ |}!",
	/* <file> <nbytes> <ncomp> B <proc_1> ... <proc_ncomp> true */
    "/B{{2 copy string{readstring pop}aload pop 4 array astore cvx",
    "3 1 roll}repeat pop pop true}!",
	/* <x> <y> <w> <h> <bpc/inv> <src> Ix <w> <h> <bps/inv> <mtx> <src> */
    "/Ix{[1 0 0 1 11 -2 roll exch neg exch neg]exch}!",
	/* <x> <y> <h> <src> , - */
	/* <x> <y> <h> <src> If - */
	/* <x> <y> <h> <src> I - */
"/,{true exch Ix imagemask}!/If{false exch Ix imagemask}!/I{exch Ix image}!",
    0
};

private const char *const psw_1_procset[] = {
    0
};

private const char *const psw_1_x_procset[] = {
	/* <w> <h> <name> <length> <src> |X <w> <h> <data> */
	/* <w> <h> <name> (<length>|) $X <w> <h> <data> */
    "/|X{exch string readhexstring |=}!/$X{+ @ |X}!",
	/* - @X <hexsrc> */
    "/@X{{currentfile ( ) readhexstring pop}}!",
    0
};

private const char *const psw_1_5_procset[] = {
	/* <x> <y> <w> <h> <src> <bpc> Ic - */
    "/Ic{exch Ix false 3 colorimage}!",
    0
};

private const char *const psw_2_procset[] = {
	/* <src> <w> <h> -mark- ... F <g4src> */
	/* <src> <w> <h> FX <g4src> */
    "/F{/Columns counttomark 3 add -2 roll/Rows exch/K -1/BlackIs1 true>>",
    "/CCITTFaxDecode filter}!/FX{<</EndOfBlock false F}!",
	/* <src> X <a85src> */
	/* - @X <a85src> */
	/* <w> <h> <src> /&2 <w> <h> <src> <w> <h> */
    "/X{/ASCII85Decode filter}!/@X{@ X}!/&2{2 index 2 index}!",
	/* <w> <h> @F <w> <h> <g4src> */
	/* <w> <h> @C <w> <h> <g4a85src> */
    "/@F{@ &2<<F}!/@C{@X &2 FX}!",
	/* <w> <h> <name> (<length>|) $X <w> <h> <data> */
	/* <w> <h> <?> <?> <src> &4 <w> <h> <?> <?> <src> <w> <h> */
	/* <w> <h> <name> (<length>|) $F <w> <h> <data> */
	/* <w> <h> <name> (<length>|) $C <w> <h> <data> */
    "/$X{+ @X |}!/&4{4 index 4 index}!/$F{+ @ &4<<F |}!/$C{+ @X &4 FX |}!",
	/* <w> <h> <bpc> <matrix> <decode> <interpolate> <src>IC - */
    "/IC{3 1 roll 10 dict begin 1{/ImageType/Interpolate/Decode/DataSource",
    "/ImageMatrix/BitsPerComponent/Height/Width}{exch def}forall",
    "currentdict end image}!",
    /* A hack for compatibility with interpreters, which don't consume 
       ASCII85Decode EOD when reader stops immediately before it : */
    "/~{@ read {pop} if}!",
    0
};

/* ---------------- Utilities ---------------- */

/*
 * Output the file header.  This must write to a file, not a stream,
 * because it may be called during finalization.
 */
private int
psw_begin_file(gx_device_pswrite *pdev, const gs_rect *pbbox)
{
    int code;
    FILE *f = pdev->file;
    char const * const *p1;
    char const * const *p2;

    if (pdev->pswrite_common.LanguageLevel < 1.5) {
	p1 = psw_1_x_procset;
	p2 = psw_1_procset;
    } else if (pdev->pswrite_common.LanguageLevel > 1.5) {
	p1 = psw_1_5_procset;
	p2 = psw_2_procset;
    } else {
	p1 = psw_1_x_procset;
	p2 = psw_1_5_procset;
    }

    if ((code = psw_begin_file_header(f, (gx_device *)pdev, pbbox,
			  &pdev->pswrite_common,
			  pdev->params.ASCII85EncodePages)) < 0 ||
        (code = psw_print_lines(f, psw_procset)) < 0 ||
        (code = psw_print_lines(f, p1)) < 0 ||
        (code = psw_print_lines(f, p2)) < 0 ||
        (code = psw_end_file_header(f)) < 0)
        return code;
    if(fflush(f) == EOF)
        return_error(gs_error_ioerror);
    return 0;
}

/* Reset the image cache. */
private void
image_cache_reset(gx_device_pswrite * pdev)
{
    int i;

    for (i = 0; i < image_cache_size; ++i)
	pdev->image_cache[i].id = gx_no_bitmap_id;
    pdev->cache_toggle = false;
}

/* Look up or enter image parameters in the cache. */
/* Return -1 if the key is not in the cache, or its index. */
/* If id is gx_no_bitmap_id or enter is false, do not enter it. */
private int
image_cache_lookup(gx_device_pswrite * pdev, gx_bitmap_id id,
		   int width, int height, bool enter)
{
    int i1, i2;
    psw_image_params_t *pip1;
    psw_image_params_t *pip2;

    if (id == gx_no_bitmap_id)
	return -1;
    i1 = id % image_cache_size;
    pip1 = &pdev->image_cache[i1];
    if (pip1->id == id && pip1->width == width && pip1->height == height) {
	return i1;
    }
    i2 = (i1 + image_cache_reprobe_step) % image_cache_size;
    pip2 = &pdev->image_cache[i2];
    if (pip2->id == id && pip2->width == width && pip2->height == height) {
	return i2;
    }
    if (enter) {
	int i = ((pdev->cache_toggle = !pdev->cache_toggle) ? i2 : i1);
	psw_image_params_t *pip = &pdev->image_cache[i];

	pip->id = id, pip->width = width, pip->height = height;
	return i;
    }
    return -1;
}

/*
 * Prepare the encoding stream for image data.
 * Return 1 if using ASCII (Hex or 85) encoding, 0 if binary.
 */
private int
psw_image_stream_setup(gx_device_pswrite * pdev, bool binary_ok)
{
    int code;
    bool save = pdev->binary_ok;

    if (pdev->pswrite_common.LanguageLevel >= 2 || binary_ok) {
	pdev->binary_ok = binary_ok;
	code = psdf_begin_binary((gx_device_psdf *)pdev, pdev->image_writer);
    } else {
	/* LanguageLevel 1, binary not OK.  Use ASCIIHex encoding. */
	pdev->binary_ok = true;
	code = psdf_begin_binary((gx_device_psdf *)pdev, pdev->image_writer);
	if (code >= 0) {
	    stream_state *st =
		s_alloc_state(pdev->v_memory, s_AXE_template.stype,
			      "psw_image_stream_setup");

	    if (st == 0)
		code = gs_note_error(gs_error_VMerror);
	    else {
		code = psdf_encode_binary(pdev->image_writer,
					  &s_AXE_template, st);
		if (code >= 0)
		    ((stream_AXE_state *)st)->EndOfData = false; /* no > */
	    }
	}
    }
    pdev->binary_ok = save;
    return (code < 0 ? code : !binary_ok);
}

/* Clean up after writing an image. */
private int
psw_image_cleanup(gx_device_pswrite * pdev)
{
    int code = 0;
    
    if (pdev->image_stream != 0) {
	code = psdf_end_binary(pdev->image_writer);
	memset(pdev->image_writer, 0, sizeof(*pdev->image_writer));
    }
    return code;
}

/* Write data for an image.  Assumes width > 0, height > 0. */
private int
psw_put_bits(stream * s, const byte * data, int data_x_bit, uint raster,
	     uint width_bits, int height)
{
    const byte *row = data + (data_x_bit >> 3);
    int shift = data_x_bit & 7;
    int y;

    for (y = 0; y < height; ++y, row += raster) {
	if (shift == 0)
	    stream_write(s, row, (width_bits + 7) >> 3);
	else {
	    const byte *src = row;
	    int wleft = width_bits;
	    int cshift = 8 - shift;

	    for (; wleft + shift > 8; ++src, wleft -= 8)
		stream_putc(s, (byte)((*src << shift) + (src[1] >> cshift)));
	    if (wleft > 0)
		stream_putc(s, (byte)((*src << shift) & (byte)(0xff00 >> wleft)));
	}
        if (s->end_status == ERRC)
            return_error(gs_error_ioerror);
    }
    return 0;
}
private int
psw_put_image_bits(gx_device_pswrite *pdev, const char *op,
		   const byte * data, int data_x, uint raster,
		   int width, int height, int depth)
{
    int code;
    
    pprints1(pdev->strm, "%s\n", op);
    code = psw_put_bits(pdev->image_stream, data, data_x * depth, raster,
		 width * depth, height);
    if (code < 0)
        return code;
    psw_image_cleanup(pdev);
    return 0;
}
private int
psw_put_image(gx_device_pswrite *pdev, const char *op, int encode,
	      const byte * data, int data_x, uint raster,
	      int width, int height, int depth)
{
    int code = psw_image_stream_setup(pdev, !(encode & 1));

    if (code < 0)
	return code;
    if (encode & 2) {
	code = psdf_CFE_binary(pdev->image_writer, width, height, false);
	if (code < 0)
	    return code;
    }
    return psw_put_image_bits(pdev, op, data, data_x, raster,
			      width, height, depth);
}
private int
psw_image_write(gx_device_pswrite * pdev, const char *imagestr,
		const byte * data, int data_x, uint raster, gx_bitmap_id id,
		int x, int y, int width, int height, int depth)
{
    stream *s = gdev_vector_stream((gx_device_vector *) pdev);
    uint width_bits = width * depth;
    int index = image_cache_lookup(pdev, id, width_bits, height, false);
    char str[40];
    char endstr[20];
    int code, encode;
    const char *op;

    if (index >= 0) {
	sprintf(str, "%d%c", index / 26, index % 26 + 'A');
	pprintd2(s, "%d %d ", x, y);
	pprints2(s, "%s %s\n", str, imagestr);
        if (s->end_status == ERRC)
            return_error(gs_error_ioerror);
	return 0;
    }
    pprintd4(s, "%d %d %d %d ", x, y, width, height);
    encode = !pdev->binary_ok;
    if (depth == 1 && width > 16 && pdev->pswrite_common.LanguageLevel >= 2) {
	/*
	 * We should really look at the statistics of the image before
	 * committing to using G4 encoding....
	 */
	encode += 2;
    }
    if (id == gx_no_bitmap_id || width_bits * (ulong) height > 8000) {
	static const char *const uncached[4] = {
	    "@", "@X", "@F", "@C"
	};

	stream_puts(s, uncached[encode]);
	op = imagestr;
	strcpy(endstr, "\n");
    } else {
	static const char *const cached[4] = {
	    "$", "$X", "$F", "$C"
	};

	index = image_cache_lookup(pdev, id, width_bits, height, true);
	sprintf(str, "/%d%c", index / 26, index % 26 + 'A');
	stream_puts(s, str);
	if (depth != 1)
	    pprintld1(s, " %ld", ((width_bits + 7) >> 3) * (ulong) height);
	op = cached[encode];
	sprintf(endstr, "\n%s\n", imagestr);
    }
    if (s->end_status == ERRC)
        return_error(gs_error_ioerror);
    /*
     * In principle, we should put %%BeginData: / %%EndData around all data
     * sections.  However, as long as the data are ASCII (not binary), they
     * can't cause problems for a DSC parser as long as all lines are
     * limited to 255 characters and there is no possibility that %% might
     * occur at the beginning of a line.  ASCIIHexEncoded data can't contain
     * % at all, and our implementation of ASCII85Encode also guarantees
     * the desired property.  Therefore, we only bracket binary data.
     */
    if (encode & 1) {
	/* We're using ASCII encoding. */
	stream_putc(s, '\n');
	code = psw_put_image(pdev, op, encode, data, data_x, raster,
			     width, height, depth);
	if (code < 0)
	    return code;
    } else {
	/*
	 * Do a pre-pass to compute the amount of binary data for the
	 * %%BeginData DSC comment.
	 */
	stream poss;

	s_init(s, pdev->memory);
	swrite_position_only(&poss);
	pdev->strm = &poss;
	code = psw_put_image(pdev, op, encode, data, data_x, raster,
			     width, height, depth);
	pdev->strm = s;
	if (code < 0)
	    return code;
	pprintld1(s, "\n%%%%BeginData: %ld\n", stell(&poss));
	code = psw_put_image(pdev, op, encode, data, data_x, raster,
			     width, height, depth);
	if (code < 0)
	    return code;
	stream_puts(s, "\n%%EndData");
    }
    stream_puts(s, endstr);
    if (s->end_status == ERRC)
        return_error(gs_error_ioerror);
    return 0;
}

/* Print a matrix. */
private void
psw_put_matrix(stream * s, const gs_matrix * pmat)
{
    pprintg6(s, "[%g %g %g %g %g %g]",
	     pmat->xx, pmat->xy, pmat->yx, pmat->yy, pmat->tx, pmat->ty);
}

/* Check for a deferred erasepage. */
private int
psw_check_erasepage(gx_device_pswrite *pdev)
{
    int code = 0;

    if (pdev->page_fill.color != gx_no_color_index) {
	if (pdev->page_fill.rect.p.x != pdev->page_fill.rect.q.x
	    && pdev->page_fill.rect.p.y != pdev->page_fill.rect.q.y)
	{
	    code = gdev_vector_fill_rectangle((gx_device *)pdev,
					      pdev->page_fill.rect.p.x,
					      pdev->page_fill.rect.p.y,
					      pdev->page_fill.rect.q.x -
						pdev->page_fill.rect.p.x,
					      pdev->page_fill.rect.q.y -
						pdev->page_fill.rect.p.y,
					      pdev->page_fill.color);
	}
	pdev->page_fill.color = gx_no_color_index;
    }
    return code;
}
#define CHECK_BEGIN_PAGE(pdev)\
  BEGIN\
    int code_ = psw_check_erasepage(pdev);\
\
    if (code_ < 0)\
      return code_;\
  END

/* Check if we write each page into separate file. */
private bool 
psw_is_separate_pages(gx_device_vector *const vdev)
{
    const char *fmt;
    gs_parsed_file_name_t parsed;
    int code = gx_parse_output_file_name(&parsed, &fmt, vdev->fname, strlen(vdev->fname));
    
    return (code >= 0 && fmt != 0);
}

/* ---------------- Vector device implementation ---------------- */

private int
psw_beginpage(gx_device_vector * vdev)
{
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
    int code = psw_open_printer((gx_device *)vdev);

    stream *s = vdev->strm;

    if (code < 0)
	 return code;

    if (pdev->first_page) {
	code = psw_begin_file(pdev, NULL);
        if (code < 0)
	     return code;
    }

    code = psw_write_page_header(s, (gx_device *)vdev, &pdev->pswrite_common, true, 
                          (psw_is_separate_pages(vdev) ? 1 : vdev->PageCount + 1),
                          image_cache_size);
    if (code < 0)
	 return code;

    pdev->page_fill.color = gx_no_color_index;
    return 0;
}


private int
psw_can_handle_hl_color(gx_device_vector * vdev, const gs_imager_state * pis1, 
              const gx_drawing_color * pdc)
{
    return false; /* High level color is not implemented yet. */
}

private int
psw_setcolors(gx_device_vector * vdev, const gs_imager_state * pis1, 
              const gx_drawing_color * pdc)
{
    const gs_imager_state * pis = NULL; /* High level color is not implemented yet. */
    if (!gx_dc_is_pure(pdc))
	return_error(gs_error_rangecheck);
    /* PostScript only keeps track of a single color. */
    gx_hld_save_color(pis, pdc, &vdev->saved_fill_color);
    gx_hld_save_color(pis, pdc, &vdev->saved_stroke_color);
    {
	stream *s = gdev_vector_stream(vdev);
	gx_color_index color = gx_dc_pure_color(pdc);
	int r = color >> 16;
	int g = (color >> 8) & 0xff;
	int b = color & 0xff;

	if (r == g && g == b) {
	    if (r == 0)
		stream_puts(s, "K\n");
	    else
		pprintd1(s, "%d G\n", r);
	} else if (r == g)
	    pprintd2(s, "%d %d r6\n", b, r);
	else if (g == b)
	    pprintd2(s, "%d %d r3\n", r, g);
	else if (r == b)
	    pprintd2(s, "%d %d r5\n", g, b);
	else
	    pprintd3(s, "%d %d %d rG\n", r, g, b);
        if (s->end_status == ERRC)
            return_error(gs_error_ioerror);
    }
    return 0;
}

/* Redefine dorect to recognize rectangle fills. */
private int
psw_dorect(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1, fixed y1,
	   gx_path_type_t type)
{
    if ((type & ~gx_path_type_rule) != gx_path_type_fill)
	return psdf_dorect(vdev, x0, y0, x1, y1, type);
    pprintg4(gdev_vector_stream(vdev), "%g %g %g %g rf\n",
	     fixed2float(x0), fixed2float(y0),
	     fixed2float(x1 - x0), fixed2float(y1 - y0));
    if ((gdev_vector_stream(vdev))->end_status == ERRC)
        return_error(gs_error_ioerror);
    return 0;
}

/*
 * We redefine path tracing to use a compact form for polygons; also,
 * we only need to write coordinates with 2 decimals of precision,
 * since this is 10 times more precise than any existing output device.
 */
inline private double
round_coord2(floatp v)
{
    return floor(v * 100 + 0.5) / 100.0;
}
private void
print_coord2(stream * s, floatp x, floatp y, const char *str)
{
    pprintg2(s, "%g %g ", round_coord2(x), round_coord2(y));
    if (str != 0)
	stream_puts(s, str);
}

private int
psw_beginpath(gx_device_vector * vdev, gx_path_type_t type)
{
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    if (type & (gx_path_type_fill | gx_path_type_stroke)) {
	/*
	 * fill_path and stroke_path call CHECK_BEGIN_PAGE themselves:
	 * we do it here to handle polygons (trapezoid, parallelogram,
	 * triangle), which don't go through fill_path.
	 */
	CHECK_BEGIN_PAGE(pdev);
    }
    pdev->path_state.num_points = 0;
    pdev->path_state.move = 0;
    if (type & gx_path_type_clip) {
	/*
	 * This approach doesn't work for clip + fill or stroke, but that
	 * combination can't occur.
	 */
	stream *s = gdev_vector_stream(vdev);

	stream_puts(s, "Q q\n");
	gdev_vector_reset(vdev);
        if (s->end_status == ERRC)
            return_error(gs_error_ioerror);
    }
    return 0;
}

private int
psw_moveto(gx_device_vector * vdev, floatp x0, floatp y0, floatp x, floatp y,
	   gx_path_type_t type)
{
    stream *s = gdev_vector_stream(vdev);
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    if (pdev->path_state.num_points > pdev->path_state.move)
	stream_puts(s, (pdev->path_state.move ? "P\n" : "p\n"));
    else if (pdev->path_state.move) {
	/*
	 * Two consecutive movetos -- possible only if a zero-length line
	 * was discarded.
	 */
	stream_puts(s, "pop pop\n");
    }
    print_coord2(s, x, y, NULL);
    pdev->path_state.num_points = 1;
    pdev->path_state.move = 1;
    if (s->end_status == ERRC)
        return_error(gs_error_ioerror);
    return 0;
}

private int
psw_lineto(gx_device_vector * vdev, floatp x0, floatp y0, floatp x, floatp y,
	   gx_path_type_t type)
{
    double dx = x - x0, dy = y - y0;

    /*
     * Omit null lines except when stroking (so that linecap works).
     ****** MAYBE WRONG IF PATH CONSISTS ONLY OF NULL LINES. ******
     */
    if ((type & gx_path_type_stroke) || dx != 0 || dy != 0) {
	stream *s = gdev_vector_stream(vdev);
	gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

	if (pdev->path_state.num_points > MAXOPSTACK / 2 - 10) {
 	    stream_puts(s, (pdev->path_state.move ? "P\n" : "p\n"));
            pdev->path_state.num_points = 0;
            pdev->path_state.move = 0;
        }
        else if (pdev->path_state.num_points > 0 &&
	    !(pdev->path_state.num_points & 7)
	    )
	    stream_putc(s, '\n');	/* limit line length for DSC compliance */
	if (pdev->path_state.num_points - pdev->path_state.move >= 2 &&
	    dx == -pdev->path_state.dprev[1].x &&
	    dy == -pdev->path_state.dprev[1].y
	    )
	    stream_puts(s, "^ ");
	else
	    print_coord2(s, dx, dy, NULL);
	pdev->path_state.num_points++;
	pdev->path_state.dprev[1] = pdev->path_state.dprev[0];
	pdev->path_state.dprev[0].x = dx;
	pdev->path_state.dprev[0].y = dy;
        if (s->end_status == ERRC)
            return_error(gs_error_ioerror);
    }
    return 0;
}

private int
psw_curveto(gx_device_vector * vdev, floatp x0, floatp y0,
	    floatp x1, floatp y1, floatp x2, floatp y2, floatp x3, floatp y3,
	    gx_path_type_t type)
{
    stream *s = gdev_vector_stream(vdev);
    double dx1 = x1 - x0, dy1 = y1 - y0;
    double dx2 = x2 - x0, dy2 = y2 - y0;
    double dx3 = x3 - x0, dy3 = y3 - y0;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    if (pdev->path_state.num_points > 0)
	stream_puts(s, (pdev->path_state.move ?
		  (pdev->path_state.num_points == 1 ? "m\n" : "P\n") :
		  "p\n"));
    if (dx1 == 0 && dy1 == 0) {
	print_coord2(s, dx2, dy2, NULL);
	print_coord2(s, dx3, dy3, "v\n");
    } else if (x3 == x2 && y3 == y2) {
	print_coord2(s, dx1, dy1, NULL);
	print_coord2(s, dx2, dy2, "y\n");
    } else {
	print_coord2(s, dx1, dy1, NULL);
	print_coord2(s, dx2, dy2, NULL);
	print_coord2(s, dx3, dy3, "c\n");
    }
    pdev->path_state.num_points = 0;
    pdev->path_state.move = 0;
    if (s->end_status == ERRC)
        return_error(gs_error_ioerror);
    return 0;
}

private int
psw_closepath(gx_device_vector * vdev, floatp x0, floatp y0,
	      floatp x_start, floatp y_start, gx_path_type_t type)
{
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    stream_puts(gdev_vector_stream(vdev),
	  (pdev->path_state.num_points > 0 && pdev->path_state.move ?
	   "H\n" : "h\n"));
    pdev->path_state.num_points = 0;
    pdev->path_state.move = 0;
    if ((gdev_vector_stream(vdev))->end_status == ERRC)
        return_error(gs_error_ioerror);
    return 0;
}

private int
psw_endpath(gx_device_vector * vdev, gx_path_type_t type)
{
    stream *s = vdev->strm;
    const char *star = (type & gx_path_type_even_odd ? "*" : "");
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    if (pdev->path_state.num_points > 0 && !pdev->path_state.move)
	stream_puts(s, "p ");
    if (type & gx_path_type_fill) {
	if (type & (gx_path_type_stroke | gx_path_type_clip))
	    pprints1(s, "q f%s Q ", star);
	else
	    pprints1(s, "f%s\n", star);
    }
    if (type & gx_path_type_stroke) {
	if (type & gx_path_type_clip)
	    stream_puts(s, "q S Q ");
	else
	    stream_puts(s, "S\n");
    }
    if (type & gx_path_type_clip)
	pprints1(s, "Y%s\n", star);
    if (s->end_status == ERRC)
        return_error(gs_error_ioerror);
    return 0;
}

/* ---------------- Driver procedures ---------------- */

/* ------ Open/close/page ------ */

/* Open the device. */
private int
psw_open(gx_device * dev)
{
    gs_memory_t *mem = gs_memory_stable(dev->memory);
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    vdev->v_memory = mem;
    vdev->vec_procs = &psw_vector_procs;

    gdev_vector_init(vdev);
    vdev->fill_options = vdev->stroke_options = gx_path_type_optimize;
    pdev->binary_ok = !pdev->params.ASCII85EncodePages;
    pdev->image_writer = gs_alloc_struct(mem, psdf_binary_writer,
					 &st_psdf_binary_writer,
					 "psw_open(image_writer)");
    memset(pdev->image_writer, 0, sizeof(*pdev->image_writer));  /* for GC */
    image_cache_reset(pdev);

    pdev->strm = NULL;

    /* if (ppdev->OpenOutputFile) */
    {
	int code = psw_open_printer(dev);

	if (code < 0)
	    return code;
    }

    return 0;
}

/* Wrap up ("output") a page. */
private int
psw_output_page(gx_device * dev, int num_copies, int flush)
{
    int code;
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
    stream *s = gdev_vector_stream(vdev);

    /* Check for a legitimate empty page. */
    CHECK_BEGIN_PAGE(pdev);
    sflush(s);			/* sync stream and file */
    code = psw_write_page_trailer(vdev->file, num_copies, flush);
    if(code < 0)
        return code;
    vdev->in_page = false;
    pdev->first_page = false;
    gdev_vector_reset(vdev);
    image_cache_reset(pdev);
    if (ferror(vdev->file))
	return_error(gs_error_ioerror);

    dev->PageCount ++;
    if (psw_is_separate_pages(vdev)) {
	code = psw_close_printer(dev);

	if (code < 0) 
	    return code;
    }
    return 0;
}

/* Close the device. */
/* Note that if this is being called as a result of finalization, */
/* the stream may have been finalized; but the file will still be open. */
private int
psw_close(gx_device * dev)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    gs_free_object(pdev->v_memory, pdev->image_writer,
		   "psw_close(image_writer)");
    pdev->image_writer = 0;

    if (vdev->strm != NULL) {
	int code = psw_close_printer(dev);

	if (code < 0)
	    return code;
    }

    return 0;
}

/* ---------------- Get/put parameters ---------------- */

/* Get parameters. */
private int
psw_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_pswrite *const pdev = (gx_device_pswrite *)dev;
    int code = gdev_psdf_get_params(dev, plist);
    int ecode;

    if (code < 0)
	return code;
    if ((ecode = param_write_float(plist, "LanguageLevel", &pdev->pswrite_common.LanguageLevel)) < 0)
	return ecode;
    return code;
}

/* Put parameters. */
private int
psw_put_params(gx_device * dev, gs_param_list * plist)
{
    int ecode = 0;
    int code;
    gs_param_name param_name;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)dev;
    float ll = pdev->pswrite_common.LanguageLevel;
    psdf_version save_version = pdev->version;

    switch (code = param_read_float(plist, (param_name = "LanguageLevel"), &ll)) {
	case 0:
	    if (ll == 1.0 || ll == 1.5 || ll == 2.0 || ll == 3.0)
		break;
	    code = gs_error_rangecheck;
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 1:
	    ;
    }

    if (ecode < 0)
	return ecode;
    /*
     * We have to set version to the new value, because the set of
     * legal parameter values for psdf_put_params varies according to
     * the version.
     */
    {
	static const psdf_version vv[3] =
	{
	    psdf_version_level1, psdf_version_level1_color,
	    psdf_version_level2
	};

	pdev->version = vv[(int)(ll * 2) - 2];
    }
    code = gdev_psdf_put_params(dev, plist);
    if (code < 0) {
	pdev->version = save_version;
	return code;
    }
    pdev->pswrite_common.LanguageLevel = ll;
    return code;
}

/* ---------------- Rectangles ---------------- */

private int
psw_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		   gx_color_index color)
{
    gx_device_pswrite *const pdev = (gx_device_pswrite *)dev;

    /*
     * If the transfer function doesn't map 1 setgray to device white,
     * the first rectangle fill on a page (from erasepage) will be with
     * some color other than white, which gdev_vector_fill_rectangle
     * won't handle correctly.  Note that this doesn't happen on the
     * very first page of a document.
     */
    if (!pdev->in_page && !pdev->first_page) {
	if (pdev->page_fill.color == gx_no_color_index) {
	    /* Save, but don't output, the erasepage. */
	    pdev->page_fill.rect.p.x = x;
	    pdev->page_fill.rect.p.y = y;
	    pdev->page_fill.rect.q.x = x + w;
	    pdev->page_fill.rect.q.y = y + h;
	    pdev->page_fill.color = color;
	    return 0;
	}
    }
    return gdev_vector_fill_rectangle(dev, x, y, w, h, color);
}

/*
 * Open the printer's output file if necessary.
 */

private int 
psw_open_printer(gx_device * dev)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    if (vdev->strm != 0) {
	return 0;
    }
    {
	int code = gdev_vector_open_file_options(vdev, 512,
					VECTOR_OPEN_FILE_SEQUENTIAL_OK |
					VECTOR_OPEN_FILE_BBOX);

	if (code < 0)
	    return code;
    }

    pdev->first_page = true;

    return 0;
}

/*
 * Close the printer's output file.
 */

private int 
psw_close_printer(gx_device * dev)
{
    int code;
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
    FILE *f = vdev->file;
    gs_rect bbox;

    gx_device_bbox_bbox(vdev->bbox_device, &bbox);
    if (pdev->first_page & !vdev->in_page) {
	/* Nothing has been written.  Write the file header now. */
	code = psw_begin_file(pdev, &bbox);
        if (code < 0)
            return code;
    } else {
	/* If there is an incomplete page, complete it now. */
	if (vdev->in_page) { 
	    /*
	     * Flush the stream after writing page trailer.
	     */

	    stream *s = vdev->strm;
	    code = psw_write_page_trailer(vdev->file, 1, 1);
            if(code < 0)
                return code;
	    sflush(s);

	    dev->PageCount++;
	}
    }
    code = psw_end_file(f, dev, &pdev->pswrite_common, &bbox, 
                 (psw_is_separate_pages(vdev) ? 1 : vdev->PageCount));
    if(code < 0)
        return code;

    return gdev_vector_close_file(vdev);
}

/* ---------------- Images ---------------- */

/* Copy a monochrome bitmap. */
private int
psw_copy_mono(gx_device * dev, const byte * data,
	int data_x, int raster, gx_bitmap_id id, int x, int y, int w, int h,
	      gx_color_index zero, gx_color_index one)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
    gx_drawing_color color;
    const char *op;
    int code = 0;

    CHECK_BEGIN_PAGE(pdev);
    if (w <= 0 || h <= 0)
	return 0;
    (*dev_proc(vdev->bbox_device, copy_mono))
	((gx_device *) vdev->bbox_device, data, data_x, raster, id,
	 x, y, w, h, zero, one);
    if (one == gx_no_color_index) {
	set_nonclient_dev_color(&color, zero);
	code = gdev_vector_update_fill_color((gx_device_vector *) pdev,
					     NULL, &color);
	op = "If";
    } else if (zero == vdev->black && one == vdev->white)
	op = "1 I";
    else {
	if (zero != gx_no_color_index) {
	    code = (*dev_proc(dev, fill_rectangle)) (dev, x, y, w, h, zero);
	    if (code < 0)
		return code;
	}
	set_nonclient_dev_color(&color, one);
	code = gdev_vector_update_fill_color((gx_device_vector *) pdev,
					     NULL, &color);
	op = ",";
    }
    if (code < 0)
	return 0;
    code = gdev_vector_update_clip_path(vdev, NULL);
    if (code < 0)
	return code;
    return psw_image_write(pdev, op, data, data_x, raster, id,
			   x, y, w, h, 1);
}

/* Copy a color bitmap. */
private int
psw_copy_color(gx_device * dev,
	       const byte * data, int data_x, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h)
{
    int depth = dev->color_info.depth;
    const byte *bits = data + data_x * 3;
    char op[6];
    int code;
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    CHECK_BEGIN_PAGE(pdev);
    if (w <= 0 || h <= 0)
	return 0;
    (*dev_proc(vdev->bbox_device, copy_color))
	((gx_device *) vdev->bbox_device, data, data_x, raster, id,
	 x, y, w, h);
    /*
     * If this is a 1-pixel-high image, check for it being all the
     * same color, and if so, fill it as a rectangle.
     */
    if (h == 1 && !memcmp(bits, bits + 3, (w - 1) * 3)) {
	return (*dev_proc(dev, fill_rectangle))
	    (dev, x, y, w, h, (bits[0] << 16) + (bits[1] << 8) + bits[2]);
    }
    sprintf(op, "%d Ic", depth / 3);	/* RGB */
    code = gdev_vector_update_clip_path(vdev, NULL);
    if (code < 0)
	return code;
    return psw_image_write(pdev, op, data, data_x, raster, id,
			   x, y, w, h, depth);
}

/* Fill or stroke a path. */
/* We redefine these to skip empty paths, and to allow optimization. */
private int
psw_fill_path(gx_device * dev, const gs_imager_state * pis,
	      gx_path * ppath, const gx_fill_params * params,
	      const gx_device_color * pdevc, const gx_clip_path * pcpath)
{
    CHECK_BEGIN_PAGE((gx_device_pswrite *)dev);
    if (gx_path_is_void(ppath))
	return 0;
    /* Update the clipping path now. */
    gdev_vector_update_clip_path((gx_device_vector *)dev, pcpath);
    return gdev_vector_fill_path(dev, pis, ppath, params, pdevc, pcpath);
}
private int
psw_stroke_path(gx_device * dev, const gs_imager_state * pis,
		gx_path * ppath, const gx_stroke_params * params,
		const gx_device_color * pdcolor, const gx_clip_path * pcpath)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;

    CHECK_BEGIN_PAGE((gx_device_pswrite *)dev);
    if (gx_path_is_void(ppath) &&
	(gx_path_is_null(ppath) ||
	 gs_currentlinecap((const gs_state *)pis) != gs_cap_round)
	)
	return 0;
    /* Update the clipping path now. */
    gdev_vector_update_clip_path(vdev, pcpath);
    /* Do the right thing for oddly transformed coordinate systems.... */
    {
	gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
	stream *s;
	int code;
	double scale;
	bool set_ctm;
	gs_matrix mat;

	if (!gx_dc_is_pure(pdcolor))
	    return gx_default_stroke_path(dev, pis, ppath, params, pdcolor,
					  pcpath);
	set_ctm = (bool)gdev_vector_stroke_scaling(vdev, pis, &scale, &mat);
	gdev_vector_update_clip_path(vdev, pcpath);
	gdev_vector_prepare_stroke((gx_device_vector *)pdev, pis, params,
				   pdcolor, scale);
	s = pdev->strm;
	if (set_ctm) {
	    stream_puts(s, "q");
	    if (is_fzero2(mat.xy, mat.yx) && is_fzero2(mat.tx, mat.ty))
		pprintg2(s, " %g %g scale\n", mat.xx, mat.yy);
	    else {
		psw_put_matrix(s, &mat);
		stream_puts(s, "concat\n");
	    }
            if (s->end_status == ERRC)
                return_error(gs_error_ioerror);
        }
	code = gdev_vector_dopath(vdev, ppath, gx_path_type_stroke,
				  (set_ctm ? &mat : (const gs_matrix *)0));
	if (code < 0)
	    return code;
	if (set_ctm)
	    stream_puts(s, "Q\n");
    }
    /* We must merge in the bounding box explicitly. */
    return (vdev->bbox_device == 0 ? 0 :
	    dev_proc(vdev->bbox_device, stroke_path)
	      ((gx_device *)vdev->bbox_device, pis, ppath, params,
	       pdcolor, pcpath));
}

/* Fill a mask. */
private int
psw_fill_mask(gx_device * dev,
	      const byte * data, int data_x, int raster, gx_bitmap_id id,
	      int x, int y, int w, int h,
	      const gx_drawing_color * pdcolor, int depth,
	      gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;

    CHECK_BEGIN_PAGE(pdev);
    if (w <= 0 || h <= 0)
	return 0;

    /* gdev_vector_update_clip_path may output a grestore and gsave,
     * causing the setting of the color to be lost.  Therefore, we
     * must update the clip path first.
     */
    if (depth > 1 ||
	gdev_vector_update_clip_path(vdev, pcpath) < 0 ||
	gdev_vector_update_fill_color(vdev, NULL, pdcolor) < 0 ||
	gdev_vector_update_log_op(vdev, lop) < 0
	)
	return gx_default_fill_mask(dev, data, data_x, raster, id,
				    x, y, w, h, pdcolor, depth, lop, pcpath);
    (*dev_proc(vdev->bbox_device, fill_mask))
	((gx_device *) vdev->bbox_device, data, data_x, raster, id,
	 x, y, w, h, pdcolor, depth, lop, pcpath);
    /* Update the clipping path now. */
    gdev_vector_update_clip_path(vdev, pcpath);
    return psw_image_write(pdev, ",", data, data_x, raster, id,
			   x, y, w, h, 1);
}

/* ---------------- High-level images ---------------- */

private image_enum_proc_plane_data(psw_image_plane_data);
private image_enum_proc_end_image(psw_image_end_image);
private const gx_image_enum_procs_t psw_image_enum_procs = {
    psw_image_plane_data, psw_image_end_image
};

/* Start processing an image. */
private int
psw_begin_image(gx_device * dev,
		const gs_imager_state * pis, const gs_image_t * pim,
		gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
    gdev_vector_image_enum_t *pie;
    const gs_color_space *pcs = pim->ColorSpace;
    const gs_color_space *pbcs = pcs;
    const char *base_name = NULL;
    gs_color_space_index index;
    int num_components;
    bool binary = pdev->binary_ok;
    byte *buffer = 0;		/* image buffer if needed */
    stream *bs = 0;		/* buffer stream if needed */
#define MAX_IMAGE_OP 10		/* imagemask\n */
    int code;

    CHECK_BEGIN_PAGE(pdev);
    pie = gs_alloc_struct(mem, gdev_vector_image_enum_t,
			  &st_vector_image_enum, "psw_begin_image");
    if (pie == 0)
	return_error(gs_error_VMerror);
    if (prect && !(prect->p.x == 0 && prect->p.y == 0 &&
		   prect->q.x == pim->Width && prect->q.y == pim->Height)
	)
	goto fail;
    switch (pim->format) {
    case gs_image_format_chunky:
    case gs_image_format_component_planar:
	break;
    default:
	goto fail;
    }
    pie->memory = mem;
    pie->default_info = 0;	/* not used */
    if (pim->ImageMask) {
	index = -1;		/* bogus, not used */
	num_components = 1;
    } else {
	index = gs_color_space_get_index(pcs);
	num_components = gs_color_space_num_components(pcs);
	if (pim->CombineWithColor)
	    goto fail;
	/*
	 * We can only handle Device color spaces right now, or Indexed
	 * color spaces over them, and only the default Decode [0 1 ...]
	 * or [0 2^BPC-1] respectively.
	 */
	switch (index) {
	case gs_color_space_index_Indexed: {
	    if (pdev->pswrite_common.LanguageLevel < 2 || pcs->params.indexed.use_proc ||
		pim->Decode[0] != 0 ||
		pim->Decode[1] != (1 << pim->BitsPerComponent) - 1
		) {
		goto fail;
	    }
	    pbcs = (const gs_color_space *)&pcs->params.indexed.base_space;
	    switch (gs_color_space_get_index(pbcs)) {
	    case gs_color_space_index_DeviceGray:
		base_name = "DeviceGray"; break;
	    case gs_color_space_index_DeviceRGB:
		base_name = "DeviceRGB"; break;
	    case gs_color_space_index_DeviceCMYK:
		base_name = "DeviceCMYK"; break;
	    default:
		goto fail;
	    }
	    break;
	}
	case gs_color_space_index_DeviceGray:
	case gs_color_space_index_DeviceRGB:
	case gs_color_space_index_DeviceCMYK: {
	    int i;

	    for (i = 0; i < num_components * 2; ++i)
		if (pim->Decode[i] != (i & 1))
		    goto fail;
	    break;
	}
	default:
	    goto fail;
	}
    }
    if (pdev->pswrite_common.LanguageLevel < 2 && !pim->ImageMask) {
	/*
	 * Restrict ourselves to Level 1 images: bits per component <= 8,
	 * not indexed.
	 */
	if (pim->BitsPerComponent > 8 || pbcs != pcs)
	    goto fail;
    }
    if (gdev_vector_begin_image(vdev, pis, pim, format, prect, pdcolor,
				pcpath, mem, &psw_image_enum_procs, pie) < 0)
	goto fail;
    if (binary) {
	/*
	 * We need to buffer the entire image in memory.  Currently, the
	 * only reason for this is the infamous "short image" problem: the
	 * image may actually have fewer rows than its height specifies.  If
	 * it weren't for that, we could know the size of the binary data in
	 * advance.  However, this will change if we compress images.
	 */
	uint bsize = MAX_IMAGE_OP +
	    ((pie->bits_per_row + 7) >> 3) * pie->height;

	buffer = gs_alloc_bytes(mem, bsize, "psw_begin_image(buffer)");
	bs = s_alloc(mem, "psw_begin_image(buffer stream)");
	if (buffer && bs) {
	    swrite_string(bs, buffer, bsize);
	} else {
	    /* An allocation failed. */
	    gs_free_object(mem, bs, "psw_begin_image(buffer stream)");
	    gs_free_object(mem, buffer, "psw_begin_image(buffer)");
	    /*
	     * Rather than returning VMerror, we fall back to an ASCII
	     * encoding, which doesn't require a buffer stream.
	     */
	    buffer = 0;
	    bs = 0;
	    binary = false;
	}
    }
    if (binary) {
	/* Set up the image stream to write into the buffer. */
	stream *save = pdev->strm;

	pdev->strm = bs;
	code = psw_image_stream_setup(pdev, true);
	pdev->strm = save;
    } else {
	code = psw_image_stream_setup(pdev, false);
    }
    if (code < 0)
	goto fail;
    /* Update the clipping path now. */
    gdev_vector_update_clip_path(vdev, pcpath);
    /* Write the image/colorimage/imagemask preamble. */
    {
	stream *s = gdev_vector_stream((gx_device_vector *) pdev);
	const char *source = (code ? "@X" : "@");
	gs_matrix imat;
	const char *op;

	stream_puts(s, "q");
	(*dev_proc(dev, get_initial_matrix)) (dev, &imat);
	gs_matrix_scale(&imat, 72.0 / dev->HWResolution[0],
			72.0 / dev->HWResolution[1], &imat);
	gs_matrix_invert(&imat, &imat);
	gs_matrix_multiply(&ctm_only(pis), &imat, &imat);
	psw_put_matrix(s, &imat);
	pprintd2(s, "concat\n%d %d ", pie->width, pie->height);
	if (pim->ImageMask) {
	    stream_puts(s, (pim->Decode[0] == 0 ? "false" : "true"));
	    psw_put_matrix(s, &pim->ImageMatrix);
	    stream_puts(s, source);
	    op = "imagemask";
	} else {
	    pprintd1(s, "%d", pim->BitsPerComponent);
	    psw_put_matrix(s, &pim->ImageMatrix);
	    if (pbcs != pcs) {
		/* This is an Indexed color space. */
		pprints1(s, "[/Indexed /%s ", base_name);
		pprintd1(s, "%d\n", pcs->params.indexed.hival);
		/*
		 * Don't write the table in binary: it might interfere
		 * with DSC parsing.
		 */
		s_write_ps_string(s, pcs->params.indexed.lookup.table.data,
				  pcs->params.indexed.lookup.table.size,
				  PRINT_ASCII85_OK);
		pprintd1(s, "\n]setcolorspace[0 %d]", (int)pim->Decode[1]);
		pprints2(s, "%s %s",
			 (pim->Interpolate ? "true" : "false"), source);
		op = "IC";
	    } else if (index == gs_color_space_index_DeviceGray) {
		stream_puts(s, source);
		op = "image";
	    } else {
		if (format == gs_image_format_chunky)
		    pprints1(s, "%s false", source);
		else {
		    /* We have to use procedures. */
		    stream_puts(s, source);
		    pprintd2(s, " %d %d B",
			     (pim->Width * pim->BitsPerComponent + 7) >> 3,
			     num_components);
		}
		pprintd1(s, " %d", num_components);
		op = "colorimage";
	    }
	}
	stream_putc(s, '\n');
	pprints1((bs ? bs : s), "%s\n", op);
        if (s->end_status == ERRC) {
            gs_free_object(mem, bs, "psw_begin_image(buffer stream)");
            gs_free_object(mem, buffer, "psw_begin_image(buffer)");
            gs_free_object(mem, pie, "psw_begin_image");
            return_error(gs_error_ioerror);
        }
    }
    *pinfo = (gx_image_enum_common_t *) pie;
    return 0;
 fail:
    gs_free_object(mem, bs, "psw_begin_image(buffer stream)");
    gs_free_object(mem, buffer, "psw_begin_image(buffer)");
    gs_free_object(mem, pie, "psw_begin_image");
    return gx_default_begin_image(dev, pis, pim, format, prect,
				  pdcolor, pcpath, mem, pinfo);
}

/* Process the next piece of an image. */
private int
psw_image_plane_data(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used)
{
    gx_device *dev = info->dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)dev;
    gdev_vector_image_enum_t *pie = (gdev_vector_image_enum_t *) info;
    int code =
	gx_image_plane_data_rows(pie->bbox_info, planes, height, rows_used);
    int pi;

    for (pi = 0; pi < pie->num_planes; ++pi) {
	if (pie->bits_per_row != pie->width * info->plane_depths[pi])
	    return_error(gs_error_rangecheck);
	psw_put_bits(pdev->image_stream, planes[pi].data,
		     planes[pi].data_x * info->plane_depths[pi],
		     planes[pi].raster, pie->bits_per_row,
		     *rows_used);
        if (pdev->image_stream->end_status == ERRC)
            return_error(gs_error_ioerror);
    }
    pie->y += *rows_used;
    return code;
}

/* Clean up by releasing the buffers. */
private int
psw_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    gx_device *dev = info->dev;
    gx_device_vector *const vdev = (gx_device_vector *)dev;
    gx_device_pswrite *const pdev = (gx_device_pswrite *)vdev;
    gdev_vector_image_enum_t *pie = (gdev_vector_image_enum_t *) info;
    int code;

    code = gdev_vector_end_image(vdev, pie, draw_last, pdev->white);
    if (code > 0) {
	stream *s = pdev->strm;
	stream *bs = pdev->image_stream;

	/* If we were buffering a binary image, write it now. */
	while (bs != s && bs->strm != 0)
	    bs = bs->strm;
	psw_image_cleanup(pdev);
	if (bs != s) {
	    /*
	     * We were buffering a binary image.  Write it now, with the
	     * DSC comments.
	     */
	    gs_memory_t *mem = bs->memory;
	    byte *buffer = bs->cbuf;
	    long len = stell(bs);
	    uint ignore;

	    pprintld1(s, "%%%%BeginData: %ld\n", len);
	    sputs(s, buffer, (uint)len, &ignore);
	    stream_puts(s, "\n%%EndData");
	    /* Free the buffer and its stream. */
	    gs_free_object(mem, bs, "psw_image_end_image(buffer stream)");
	    gs_free_object(mem, buffer, "psw_image_end_image(buffer)");
	}
	stream_puts(s, "\nQ\n");
        if (s->end_status == ERRC)
            return_error(gs_error_ioerror);
    }
    return code;
}
