/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gdevps.c,v 1.3 2000/03/10 07:45:50 lpd Exp $ */
/* PostScript-writing driver */
#include "math_.h"
#include "memory_.h"
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

/****************************************************************
 * Notes:
 *	Images are never compressed; in fact, none of the other
 *	  Distiller parameters do anything.
 ****************************************************************/

/* ---------------- Device definition ---------------- */

/* Device procedures */
private dev_proc_open_device(psw_open);
private dev_proc_output_page(psw_output_page);
private dev_proc_close_device(psw_close);
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
    /* Settable parameters */
#define LanguageLevel_default 2.0
#define psdf_version_default psdf_version_level2
    float LanguageLevel;
    /* End of parameters */
    bool ProduceEPS;
    bool first_page;
    long bbox_position;
    psdf_binary_writer *image_writer;
#define image_stream image_writer->strm
#define image_cache_size 197
#define image_cache_reprobe_step 121
    psw_image_params_t image_cache[image_cache_size];
    bool cache_toggle;
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
		gdev_vector_fill_rectangle,\
		NULL,			/* tile_rectangle */\
		psw_copy_mono,\
		psw_copy_color,\
		NULL,			/* draw_line */\
		NULL,			/* get_bits */\
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
		NULL/******psw_strip_copy_rop******/\
	}

const gx_device_pswrite gs_pswrite_device =
{std_device_dci_type_body(gx_device_pswrite, 0, "pswrite",
			  &st_device_pswrite,
	DEFAULT_WIDTH_10THS * X_DPI / 10, DEFAULT_HEIGHT_10THS * Y_DPI / 10,
			  X_DPI, Y_DPI, 3, 24, 255, 255, 256, 256),
 psw_procs,
 psdf_initial_values(psdf_version_default, 1 /*true */ ),	/* (ASCII85EncodePages) */
 LanguageLevel_default,		/* LanguageLevel */
 0				/*false *//* ProduceEPS */
};

const gx_device_pswrite gs_epswrite_device =
{std_device_dci_type_body(gx_device_pswrite, 0, "epswrite",
			  &st_device_pswrite,
	DEFAULT_WIDTH_10THS * X_DPI / 10, DEFAULT_HEIGHT_10THS * Y_DPI / 10,
			  X_DPI, Y_DPI, 3, 24, 255, 255, 256, 256),
 psw_procs,
 psdf_initial_values(psdf_version_default, 1 /*true */ ),	/* (ASCII85EncodePages) */
 LanguageLevel_default,		/* LanguageLevel */
 1				/*true *//* ProduceEPS */
};

/* Vector device implementation */
private int
    psw_beginpage(P1(gx_device_vector * vdev)),
    psw_setcolors(P2(gx_device_vector * vdev, const gx_drawing_color * pdc)),
    psw_dorect(P6(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1,
		  fixed y1, gx_path_type_t type)),
    psw_beginpath(P2(gx_device_vector * vdev, gx_path_type_t type)),
    psw_moveto(P6(gx_device_vector * vdev, floatp x0, floatp y0,
		  floatp x, floatp y, gx_path_type_t type)),
    psw_lineto(P6(gx_device_vector * vdev, floatp x0, floatp y0,
		  floatp x, floatp y, gx_path_type_t type)),
    psw_curveto(P10(gx_device_vector * vdev, floatp x0, floatp y0,
		    floatp x1, floatp y1, floatp x2, floatp y2,
		    floatp x3, floatp y3, gx_path_type_t type)),
    psw_closepath(P6(gx_device_vector * vdev, floatp x0, floatp y0,
		     floatp x_start, floatp y_start, gx_path_type_t type)),
    psw_endpath(P2(gx_device_vector * vdev, gx_path_type_t type));
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

private const char *const psw_ps_header[] =
{
    "%!PS-Adobe-3.0",
    "%%Pages: (atend)",
    0
};

private const char *const psw_eps_header[] =
{
    "%!PS-Adobe-3.0 EPSF-3.0",
    0
};

private const char *const psw_header[] =
{
    "%%EndComments",
    "%%BeginProlog",
 "% This copyright applies to everything between here and the %%EndProlog:",
    0
};

private const char *const psw_prolog[] =
{
    "%%BeginResource: procset GS_pswrite_ProcSet",
    "/GS_pswrite_ProcSet 80 dict dup begin",
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
    "/m/moveto #/l/lineto #/c/rcurveto #/h{p closepath}!/H{P closepath}!",
	/* <dx> lx - */
	/* <dy> ly - */
	/* <dx2> <dy2> <dx3> <dy3> v - */
	/* <dx1> <dy1> <dx2> <dy2> y - */
    "/lx{0 rlineto}!/ly{0 exch rlineto}!/v{0 0 6 2 roll c}!/y{2 copy c}!",
	/* <x> <y> <dx> <dy> re - */
    "/re{4 -2 roll m exch dup lx exch ly neg lx h}!",
	/* <x> <y> <a> <b> ^ <x> <y> <a> <b> <-x> <-y> */
    "/^{3 index neg 3 index neg}!",
	/* <x> <y> <dx1> <dy1> ... <dxn> <dyn> P - */
    "/P{N 0 gt{N -2 roll moveto p}if}!",
	/* <dx1> <dy1> ... <dxn> <dyn> p - */
    "/p{N 2 idiv{N -2 roll rlineto}repeat}!",
    "/f{P fill}!/f*{P eofill}!/s{H stroke}!/S{P stroke}!",
    "/q/gsave #/Q/grestore #/rf{re fill}!",
    "/Y{initclip P clip newpath}!/Y*{initclip P eoclip newpath}!/rY{re Y}!",
	/* <w> <h> <name> <data> <?> |= <w> <h> <data> */
    "/|={pop exch 4 1 roll 3 array astore cvx exch 1 index def exec}!",
	/* <w> <h> <name> <length> <src> | <w> <h> <data> */
    "/|{exch string readstring |=}!",
	/* <w> <?> <name> (<length>|) + <w> <?> <name> <length> */
    "/+{dup type/nametype eq{2 index 7 add -3 bitshift 2 index mul}if}!",
	/* <w> <h> <name> (<length>|) $ <w> <h> <data> */
    "/@/currentfile #/${+ @ |}!",
	/* <x> <y> <w> <h> <bpc/inv> <src> Ix <w> <h> <bps/inv> <mtx> <src> */
    "/Ix{[1 0 0 1 11 -2 roll exch neg exch neg]exch}!",
	/* <x> <y> <h> <src> , - */
	/* <x> <y> <h> <src> If - */
	/* <x> <y> <h> <src> I - */
"/,{true exch Ix imagemask}!/If{false exch Ix imagemask}!/I{exch Ix image}!",
    0
};

private const char *const psw_1_prolog[] =
{
    0
};

private const char *const psw_1_x_prolog[] =
{
	/* <w> <h> <name> <length> <src> |X <w> <h> <data> */
	/* <w> <h> <name> (<length>|) $X <w> <h> <data> */
    "/|X{exch string readhexstring |=}!/$X{+ @ |X}!",
	/* - @X <hexsrc> */
    "/@X{{currentfile ( ) readhexstring pop}}!",
	/* <w> <h> <sizename> PS - */
    "/PS{1 index where{pop cvx exec pop pop}{pop/setpage where",
    "{pop pageparams 3{exch pop}repeat setpage}{pop pop}ifelse}ifelse}!",
    0
};

private const char *const psw_1_5_prolog[] =
{
	/* <x> <y> <w> <h> <src> <bpc> Ic - */
    "/Ic{exch Ix false 3 colorimage}!",
    0
};

private const char *const psw_2_prolog[] =
{
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
    0
};

private const char *const psw_end_prolog[] =
{
    "end def",
    "%%EndResource",
    "%%EndProlog",
    0
};

private void
psw_put_lines(stream * s, const char *const lines[])
{
    int i;

    for (i = 0; lines[i] != 0; ++i)
	pprints1(s, "%s\n", lines[i]);
}

/* ---------------- Utilities ---------------- */

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

/* Prepare the encoding stream for image data. */
/* Return 1 if we are using ASCII85 (or, for Level 1, ASCIIHex) encoding. */
private int
psw_image_stream_setup(gx_device_pswrite * pdev)
{
    int code, encode;

    if (pdev->LanguageLevel >= 2 || pdev->binary_ok) {
	code = psdf_begin_binary((gx_device_psdf *)pdev, pdev->image_writer);
	encode =
	    (pdev->image_stream->state->template == &s_A85E_template ? 1 : 0);
    } else {
	pdev->binary_ok = true;
	code = psdf_begin_binary((gx_device_psdf *)pdev, pdev->image_writer);
	pdev->binary_ok = false;
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
	encode = 1;
    }
    return (code < 0 ? code : encode);
}

/* Clean up after writing an image. */
private void
psw_image_cleanup(gx_device_pswrite * pdev)
{
    if (pdev->image_stream != 0) {
	psdf_end_binary(pdev->image_writer);
	memset(pdev->image_writer, 0, sizeof(*pdev->image_writer));
    }
}

/* Write data for an image.  Assumes width > 0, height > 0. */
private void
psw_put_bits(stream * s, const byte * data, int data_x_bit, uint raster,
	     uint width_bits, int height)
{
    const byte *row = data + (data_x_bit >> 3);
    int shift = data_x_bit & 7;
    int y;

    for (y = 0; y < height; ++y, row += raster)
	if (shift == 0)
	    pwrite(s, row, (width_bits + 7) >> 3);
	else {
	    const byte *src = row;
	    int wleft = width_bits;
	    int cshift = 8 - shift;

	    for (; wleft + shift > 8; ++src, wleft -= 8)
		pputc(s, (*src << shift) + (src[1] >> cshift));
	    if (wleft > 0)
		pputc(s, (*src << shift) & (byte)(0xff00 >> wleft));
	}
}
private int
psw_image_write(gx_device_pswrite * pdev, const char *imagestr,
		const byte * data, int data_x, uint raster, gx_bitmap_id id,
		int x, int y, int width, int height, int depth)
{
    stream *s = gdev_vector_stream((gx_device_vector *) pdev);
    uint width_bits = width * depth;
    int data_x_bit = data_x * depth;
    int index = image_cache_lookup(pdev, id, width_bits, height, false);
    char str[40];
    int code, encode;

    if (index >= 0) {
	sprintf(str, "%d%c", index / 26, index % 26 + 'A');
	pprintd2(s, "%d %d ", x, y);
	pprints2(s, "%s %s\n", str, imagestr);
	return 0;
    }
    pprintd4(s, "%d %d %d %d ", x, y, width, height);
    encode = code = psw_image_stream_setup(pdev);
    if (code < 0)
	return code;
    if (depth == 1 && width > 16 && pdev->LanguageLevel >= 2) {
	/*
	 * We should really look at the statistics of the image before
	 * committing to using G4 encoding....
	 */
	code = psdf_CFE_binary(pdev->image_writer, width, height, false);
	if (code < 0)
	    return code;
	encode += 2;
    }
    if (id == gx_no_bitmap_id || width_bits * (ulong) height > 8000) {
	static const char *const uncached[4] = {
	    "@", "@X", "@F", "@C"
	};

	pprints2(s, "%s %s\n", uncached[encode], imagestr);
	psw_put_bits(pdev->image_stream, data, data_x_bit, raster,
		     width_bits, height);
	psw_image_cleanup(pdev);
	spputc(s, '\n');
    } else {
	static const char *const cached[4] = {
	    "$", "$X", "$F", "$C"
	};

	index = image_cache_lookup(pdev, id, width_bits, height, true);
	sprintf(str, "/%d%c ", index / 26, index % 26 + 'A');
	pputs(s, str);
	if (depth != 1)
	    pprintld1(s, "%ld ", ((width_bits + 7) >> 3) * (ulong) height);
	pprints1(s, "%s\n", cached[encode]);
	psw_put_bits(pdev->image_stream, data, data_x_bit, raster,
		     width_bits, height);
	psw_image_cleanup(pdev);
	pprints1(s, "\n%s\n", imagestr);
    }
    return 0;
}

/* Print a matrix. */
private void
psw_put_matrix(stream * s, const gs_matrix * pmat)
{
    pprintg6(s, "[%g %g %g %g %g %g]",
	     pmat->xx, pmat->xy, pmat->yx, pmat->yy, pmat->tx, pmat->ty);
}

/* ---------------- Vector device implementation ---------------- */

#define pdev ((gx_device_pswrite *)vdev)

private int
psw_beginpage(gx_device_vector * vdev)
{
    stream *s = vdev->strm;
    long page = vdev->PageCount + 1;

    if (pdev->first_page) {
	psw_put_lines(s,
		      (pdev->ProduceEPS ? psw_eps_header : psw_ps_header));
	if (ftell(vdev->file) < 0) {	/* File is not seekable. */
	    pdev->bbox_position = -1;
	    pputs(s, "%%BoundingBox: (atend)\n");
	    pputs(s, "%%HiResBoundingBox: (atend)\n");
	} else {		/* File is seekable, leave room to rewrite bbox. */
	    pdev->bbox_position = stell(s);
	    pputs(s, "%...............................................................\n");
	    pputs(s, "%...............................................................\n");
	}
	pprints1(s, "%%%%Creator: %s ", gs_product);
	pprintld1(s, "%ld ", (long)gs_revision);
	pprints1(s, "(%s)\n", vdev->dname);
	{
	    struct tm tms;
	    time_t t;
	    char date_str[25];

	    time(&t);
	    tms = *localtime(&t);
	    sprintf(date_str, "%d/%02d/%02d %02d:%02d:%02d",
		    tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday,
		    tms.tm_hour, tms.tm_min, tms.tm_sec);
	    pprints1(s, "%%%%CreationDate: %s\n", date_str);
	}
	if (pdev->params.ASCII85EncodePages)
	    pputs(s, "%%DocumentData: Clean7Bit\n");
	if (pdev->LanguageLevel == 2.0)
	    pputs(s, "%%LanguageLevel: 2\n");
	else if (pdev->LanguageLevel == 1.5)
	    pputs(s, "%%Extensions: CMYK\n");
	psw_put_lines(s, psw_header);
	pprints1(s, "%% %s\n", gs_copyright);
	psw_put_lines(s, psw_prolog);
	if (pdev->LanguageLevel < 1.5) {
	    psw_put_lines(s, psw_1_x_prolog);
	    psw_put_lines(s, psw_1_prolog);
	} else if (pdev->LanguageLevel > 1.5) {
	    psw_put_lines(s, psw_1_5_prolog);
	    psw_put_lines(s, psw_2_prolog);
	} else {
	    psw_put_lines(s, psw_1_x_prolog);
	    psw_put_lines(s, psw_1_5_prolog);
	}
	psw_put_lines(s, psw_end_prolog);
    }
    pprintld2(s, "%%%%Page: %ld %ld\n%%%%BeginPageSetup\n", page, page);
    pputs(s, "/pagesave save def GS_pswrite_ProcSet begin\n");
    if (!pdev->ProduceEPS) {
	int width = (int)(vdev->width * 72.0 / vdev->HWResolution[0] + 0.5);
	int height = (int)(vdev->height * 72.0 / vdev->HWResolution[1] + 0.5);

	if (pdev->LanguageLevel > 1.5)
	    pprintd2(s, "<< /PageSize [%d %d] >> setpagedevice\n",
		     width, height);
	else {
	    typedef struct ps_ {
		const char *size_name;
		int width, height;
	    } page_size;
	    static const page_size sizes[] = {
		{"/11x17", 792, 1224},
		{"/a3", 842, 1190},
		{"/a4", 595, 842},
		{"/b5", 501, 709},
		{"/ledger", 1224, 792},
		{"/legal", 612, 1008},
		{"/letter", 612, 792},
		{"null", 0, 0}
	    };
	    const page_size *p = sizes;

	    while (p->size_name[0] == '/' &&
		   (p->width != width || p->height != height))
		++p;
	    pprintd2(s, "%d %d ", width, height);
	    pprints1(s, "%s PS\n", p->size_name);
	}
    }
    pprintg2(s, "%g %g scale\n%%%%EndPageSetup\nmark\n",
	     72.0 / vdev->HWResolution[0], 72.0 / vdev->HWResolution[1]);
    return 0;
}

private int
psw_setcolors(gx_device_vector * vdev, const gx_drawing_color * pdc)
{
    if (!gx_dc_is_pure(pdc))
	return_error(gs_error_rangecheck);
    /* PostScript only keeps track of a single color. */
    vdev->fill_color = *pdc;
    vdev->stroke_color = *pdc;
    {
	stream *s = gdev_vector_stream(vdev);
	gx_color_index color = gx_dc_pure_color(pdc);
	int r = color >> 16;
	int g = (color >> 8) & 0xff;
	int b = color & 0xff;

	if (r == g && g == b) {
	    if (r == 0)
		pputs(s, "K\n");
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
    return 0;
}

/*
 * We redefine path tracing to use a compact form for polygons; also,
 * we only need to write coordinates with 2 decimals of precision,
 * since this is 10 times more precise than any existing output device.
 */
#define round_coord(v) (floor((v) * 100 + 0.5) / 100.0)
private void
print_coord2(stream * s, floatp x, floatp y, const char *str)
{
    pprintg2(s, "%g %g ", round_coord(x), round_coord(y));
    if (str != 0)
	pputs(s, str);
}
#undef round_coord

private int
psw_beginpath(gx_device_vector * vdev, gx_path_type_t type)
{
    pdev->path_state.num_points = 0;
    pdev->path_state.move = 0;
    return 0;
}

private int
psw_moveto(gx_device_vector * vdev, floatp x0, floatp y0, floatp x, floatp y,
	   gx_path_type_t type)
{
    stream *s = gdev_vector_stream(vdev);

    if (pdev->path_state.num_points > pdev->path_state.move)
	pputs(s, (pdev->path_state.move ? "P\n" : "p\n"));
    else if (pdev->path_state.move) {
	/*
	 * Two consecutive movetos -- possible only if a zero-length line
	 * was discarded.
	 */
	pputs(s, "pop pop\n");
    }
    print_coord2(s, x, y, NULL);
    pdev->path_state.num_points = 1;
    pdev->path_state.move = 1;
    return 0;
}

private int
psw_lineto(gx_device_vector * vdev, floatp x0, floatp y0, floatp x, floatp y,
	   gx_path_type_t type)
{
    double dx = x - x0, dy = y - y0;

    /*
     * Omit null lines when filling.
     ****** MAYBE WRONG IF PATH CONSISTS ONLY OF NULL LINES. ******
     */
    if (dx != 0 || dy != 0) {
	stream *s = gdev_vector_stream(vdev);

	if (pdev->path_state.num_points - pdev->path_state.move >= 2 &&
	    dx == -pdev->path_state.dprev[1].x &&
	    dy == -pdev->path_state.dprev[1].y
	    )
	    pputs(s, "^ ");
	else
	    print_coord2(s, dx, dy, NULL);
	pdev->path_state.num_points++;
	pdev->path_state.dprev[1] = pdev->path_state.dprev[0];
	pdev->path_state.dprev[0].x = dx;
	pdev->path_state.dprev[0].y = dy;
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

    if (pdev->path_state.num_points > 0)
	pputs(s, (pdev->path_state.move ?
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
    return 0;
}

private int
psw_closepath(gx_device_vector * vdev, floatp x0, floatp y0,
	      floatp x_start, floatp y_start, gx_path_type_t type)
{
    pputs(gdev_vector_stream(vdev),
	  (pdev->path_state.num_points > 0 && pdev->path_state.move ?
	   "H\n" : "h\n"));
    pdev->path_state.num_points = 0;
    pdev->path_state.move = 0;
    return 0;
}

private int
psw_endpath(gx_device_vector * vdev, gx_path_type_t type)
{
    stream *s = vdev->strm;
    const char *star = (type & gx_path_type_even_odd ? "*" : "");

    if (pdev->path_state.num_points > 0 && !pdev->path_state.move)
	pputs(s, "p ");
    if (type & gx_path_type_fill) {
	if (type & (gx_path_type_stroke | gx_path_type_clip))
	    pprints1(s, "q f%s Q ", star);
	else
	    pprints1(s, "f%s\n", star);
    }
    if (type & gx_path_type_stroke) {
	if (type & gx_path_type_clip)
	    pputs(s, "q S Q ");
	else
	    pputs(s, "S\n");
    }
    if (type & gx_path_type_clip)
	pprints1(s, "Y%s\n", star);
    return 0;
}

#undef pdev

/* ---------------- Driver procedures ---------------- */

#define vdev ((gx_device_vector *)dev)
#define pdev ((gx_device_pswrite *)dev)

/* ------ Open/close/page ------ */

/* Open the device. */
private int
psw_open(gx_device * dev)
{
    gs_memory_t *mem = gs_memory_stable(dev->memory);

    vdev->v_memory = mem;
    vdev->vec_procs = &psw_vector_procs;
    {
	int code = gdev_vector_open_file_bbox(vdev, 512, true);

	if (code < 0)
	    return code;
    }
    gdev_vector_init(vdev);
    pdev->first_page = true;
    pdev->binary_ok = !pdev->params.ASCII85EncodePages;
    pdev->image_writer = gs_alloc_struct(mem, psdf_binary_writer,
					 &st_psdf_binary_writer,
					 "psw_open(image_writer)");
    memset(pdev->image_writer, 0, sizeof(*pdev->image_writer));  /* for GC */
    image_cache_reset(pdev);
    return 0;
}

/*
 * Write the page trailer.  We do this directly to the file, rather than to
 * the stream, because we may have to do it during finalization.
 */
private void
psw_write_page_trailer(gx_device *dev, int num_copies, int flush)
{
    FILE *f = vdev->file;

    if (num_copies != 1)
	fprintf(f, "userdict /#copies %d put\n", num_copies);
    fprintf(f, "cleartomark end %s pagesave restore\n%%%%PageTrailer\n",
	    (flush ? "showpage" : "copypage"));
}

/* Wrap up ("output") a page. */
private int
psw_output_page(gx_device * dev, int num_copies, int flush)
{
    stream *s = gdev_vector_stream(vdev);

    sflush(s);			/* sync stream and file */
    psw_write_page_trailer(dev, num_copies, flush);
    vdev->in_page = false;
    pdev->first_page = false;
    gdev_vector_reset(vdev);
    image_cache_reset(pdev);
    if (ferror(vdev->file))
	return_error(gs_error_ioerror);
    return gx_finish_output_page(dev, num_copies, flush);
}

/* Close the device. */
/* Note that if this is being called as a result of finalization, */
/* the stream may have been finalized; but the file will still be open. */
private void psw_print_bbox(P2(FILE *, const gs_rect *));
private int
psw_close(gx_device * dev)
{
    FILE *f = vdev->file;

    /* If there is an incomplete page, complete it now. */
    if (vdev->in_page) {
	/*
	 * Flush the stream if it hasn't been flushed (and finalized)
	 * already.
	 */
	stream *s = vdev->strm;

	if (s->swptr != s->cbuf - 1)
	    sflush(s);
	psw_write_page_trailer(dev, 1, 1);
	dev->PageCount++;
    }
    fprintf(f, "%%%%Trailer\n%%%%Pages: %ld\n", dev->PageCount);
    {
	gs_rect bbox;

	gx_device_bbox_bbox(vdev->bbox_device, &bbox);
	if (pdev->bbox_position >= 0) {
	    long save_pos = ftell(f);

	    fseek(f, pdev->bbox_position, SEEK_SET);
	    psw_print_bbox(f, &bbox);
	    fputc('%', f);
	    fseek(f, save_pos, SEEK_SET);
	} else
	    psw_print_bbox(f, &bbox);
    }
    if (!pdev->ProduceEPS)
	fputs("%%EOF\n", f);
    gs_free_object(pdev->v_memory, pdev->image_writer,
		   "psw_close(image_writer)");
    pdev->image_writer = 0;
    return gdev_vector_close_file(vdev);
}
private void
psw_print_bbox(FILE *f, const gs_rect *pbbox)
{
    fprintf(f, "%%%%BoundingBox: %d %d %d %d\n",
	    (int)floor(pbbox->p.x), (int)floor(pbbox->p.y),
	    (int)ceil(pbbox->q.x), (int)ceil(pbbox->q.y));
    fprintf(f, "%%%%HiResBoundingBox: %f %f %f %f\n",
	    pbbox->p.x, pbbox->p.y, pbbox->q.x, pbbox->q.y);

}

/* ---------------- Get/put parameters ---------------- */

/* Get parameters. */
private int
psw_get_params(gx_device * dev, gs_param_list * plist)
{
    int code = gdev_psdf_get_params(dev, plist);
    int ecode;

    if (code < 0)
	return code;
    if ((ecode = param_write_float(plist, "LanguageLevel", &pdev->LanguageLevel)) < 0)
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
    float ll = pdev->LanguageLevel;
    psdf_version save_version = pdev->version;

    switch (code = param_read_float(plist, (param_name = "LanguageLevel"), &ll)) {
	case 0:
	    if (ll == 1.0 || ll == 1.5 || ll == 2.0)
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
    pdev->LanguageLevel = ll;
    return code;
}

/* ---------------- Images ---------------- */

/* Copy a monochrome bitmap. */
private int
psw_copy_mono(gx_device * dev, const byte * data,
	int data_x, int raster, gx_bitmap_id id, int x, int y, int w, int h,
	      gx_color_index zero, gx_color_index one)
{
    gx_drawing_color color;
    const char *op;
    int code = 0;

    if (w <= 0 || h <= 0)
	return 0;
    (*dev_proc(vdev->bbox_device, copy_mono))
	((gx_device *) vdev->bbox_device, data, data_x, raster, id,
	 x, y, w, h, zero, one);
    if (one == gx_no_color_index) {
	color_set_pure(&color, zero);
	code = gdev_vector_update_fill_color((gx_device_vector *) pdev,
					     &color);
	op = "If";
    } else if (zero == vdev->black && one == vdev->white)
	op = "1 I";
    else {
	if (zero != gx_no_color_index) {
	    code = (*dev_proc(dev, fill_rectangle)) (dev, x, y, w, h, zero);
	    if (code < 0)
		return code;
	}
	color_set_pure(&color, one);
	code = gdev_vector_update_fill_color((gx_device_vector *) pdev,
					     &color);
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
/* We redefine these to skip empty paths. */
private int
psw_fill_path(gx_device * dev, const gs_imager_state * pis,
	      gx_path * ppath, const gx_fill_params * params,
	      const gx_device_color * pdevc, const gx_clip_path * pcpath)
{
    if (gx_path_is_void(ppath))
	return 0;
    return gdev_vector_fill_path(dev, pis, ppath, params, pdevc, pcpath);
}
private int
psw_stroke_path(gx_device * dev, const gs_imager_state * pis,
		gx_path * ppath, const gx_stroke_params * params,
		const gx_device_color * pdcolor, const gx_clip_path * pcpath)
{
    if (gx_path_is_void(ppath) &&
	(gx_path_is_null(ppath) ||
	 gs_currentlinecap((const gs_state *)pis) != gs_cap_round)
	)
	return 0;
    /* Do the right thing for oddly transformed coordinate systems.... */
    {
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
	    pputs(s, "q");
	    if (is_fzero2(mat.xy, mat.yx) && is_fzero2(mat.tx, mat.ty))
		pprintg2(s, " %g %g scale\n", mat.xx, mat.yy);
	    else {
		psw_put_matrix(s, &mat);
		pputs(s, "concat\n");
	    }
	}
	code = gdev_vector_dopath(vdev, ppath, gx_path_type_stroke,
				  (set_ctm ? &mat : (const gs_matrix *)0));
	if (code < 0)
	    return code;
	if (set_ctm)
	    pputs(s, "Q\n");
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
    if (w <= 0 || h <= 0)
	return 0;
    if (depth > 1 ||
	gdev_vector_update_fill_color(vdev, pdcolor) < 0 ||
	gdev_vector_update_clip_path(vdev, pcpath) < 0 ||
	gdev_vector_update_log_op(vdev, lop) < 0
	)
	return gx_default_fill_mask(dev, data, data_x, raster, id,
				    x, y, w, h, pdcolor, depth, lop, pcpath);
    (*dev_proc(vdev->bbox_device, fill_mask))
	((gx_device *) vdev->bbox_device, data, data_x, raster, id,
	 x, y, w, h, pdcolor, depth, lop, pcpath);
    return psw_image_write(pdev, ",", data, data_x, raster, id,
			   x, y, w, h, 1);
}

/* ---------------- High-level images ---------------- */

private image_enum_proc_plane_data(psw_image_plane_data);
private image_enum_proc_end_image(psw_image_end_image);
private const gx_image_enum_procs_t psw_image_enum_procs =
{
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
    gdev_vector_image_enum_t *pie =
	gs_alloc_struct(mem, gdev_vector_image_enum_t,
			&st_vector_image_enum, "psw_begin_image");
    const gs_color_space *pcs = pim->ColorSpace;
    const gs_color_space *pbcs = pcs;
    const char *base_name;
    gs_color_space_index index;
    int num_components;
    bool can_do = prect == 0 &&
	(pim->format == gs_image_format_chunky ||
	 pim->format == gs_image_format_component_planar);
    int code;

    if (pie == 0)
	return_error(gs_error_VMerror);
    pie->memory = mem;
    pie->default_info = 0;	/* not used */
    if (pim->ImageMask) {
	index = -1;		/* bogus, not used */
	num_components = 1;
    } else {
	index = gs_color_space_get_index(pcs);
	num_components = gs_color_space_num_components(pcs);
	if (pim->CombineWithColor)
	    can_do = false;
	/*
	 * We can only handle Device color spaces right now, or Indexed
	 * color spaces over them, and only the default Decode [0 1 ...]
	 * or [0 2^BPC-1] respectively.
	 */
	switch (index) {
	case gs_color_space_index_Indexed: {
	    if (pdev->LanguageLevel < 2 || pcs->params.indexed.use_proc ||
		pim->Decode[0] != 0 ||
		pim->Decode[1] != (1 << pim->BitsPerComponent) - 1
		) {
		can_do = false;
		break;
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
		can_do = false;
	    }
	    break;
	}
	case gs_color_space_index_DeviceGray:
	case gs_color_space_index_DeviceRGB:
	case gs_color_space_index_DeviceCMYK: {
	    int i;

	    for (i = 0; i < num_components * 2; ++i)
		if (pim->Decode[i] != (i & 1))
		    can_do = false;
	    break;
	}
	default:
	    can_do = false;
	}
    }
    if (pdev->LanguageLevel < 2 && !pim->ImageMask) {
	/*
	 * Restrict ourselves to Level 1 images: bits per component <= 8,
	 * not indexed.
	 */
	if (pim->BitsPerComponent > 8 || pbcs != pcs)
	    can_do = false;
    }
    if (!can_do ||
	gdev_vector_begin_image(vdev, pis, pim, format, prect, pdcolor,
			     pcpath, mem, &psw_image_enum_procs, pie) < 0 ||
	(code = psw_image_stream_setup(pdev)) < 0
	) {
	gs_free_object(mem, pie, "psw_begin_image");
	return gx_default_begin_image(dev, pis, pim, format, prect,
				      pdcolor, pcpath, mem, pinfo);
    }
    /* Write the image/colorimage/imagemask preamble. */
    {
	stream *s = gdev_vector_stream((gx_device_vector *) pdev);
	const char *source = (code ? "@X" : "@");
	gs_matrix imat;

	pputs(s, "q");
	(*dev_proc(dev, get_initial_matrix)) (dev, &imat);
	gs_matrix_scale(&imat, 72.0 / dev->HWResolution[0],
			72.0 / dev->HWResolution[1], &imat);
	gs_matrix_invert(&imat, &imat);
	gs_matrix_multiply(&ctm_only(pis), &imat, &imat);
	psw_put_matrix(s, &imat);
	pprintd2(s, "concat\n%d %d ", pie->width, pie->height);
	if (pim->ImageMask) {
	    pputs(s, (pim->Decode[0] == 0 ? "false" : "true"));
	    psw_put_matrix(s, &pim->ImageMatrix);
	    pprints1(s, "%s imagemask\n", source);
	} else {
	    pprintd1(s, "%d", pim->BitsPerComponent);
	    psw_put_matrix(s, &pim->ImageMatrix);
	    if (pbcs != pcs) {
		/* This is an Indexed color space. */
		pprints1(s, "[/Indexed /%s ", base_name);
		pprintd1(s, "%d\n", pcs->params.indexed.hival);
		s_write_ps_string(s, pcs->params.indexed.lookup.table.data,
				  pcs->params.indexed.lookup.table.size,
				  (pdev->binary_ok ? PRINT_BINARY_OK : 0) |
				  PRINT_ASCII85_OK);
		pprintd1(s, "\n]setcolorspace[0 %d]", (int)pim->Decode[1]),
		pprints2(s, "%s %s IC\n",
			 (pim->Interpolate ? "true" : "false"), source);
	    } else if (index == gs_color_space_index_DeviceGray)
		pprints1(s, "%s image\n", source);
	    else {
		if (format == gs_image_format_chunky)
		    pprints1(s, "%s false", source);
		else
		    pprints2(s, "%s %strue", source,
			     "dup dup dup " + (16 - num_components * 4));
		pprintd1(s, " %d colorimage\n", num_components);
	    }
	}
    }
    *pinfo = (gx_image_enum_common_t *) pie;
    return 0;
}

/* Process the next piece of an image. */
private int
psw_image_plane_data(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used)
{
    gx_device *dev = info->dev;
    gdev_vector_image_enum_t *pie = (gdev_vector_image_enum_t *) info;
    int code =
	gx_image_plane_data_rows(pie->bbox_info, planes, height, rows_used);
    int pi;

    for (pi = 0; pi < pie->num_planes; ++pi)
	psw_put_bits(pdev->image_stream, planes[pi].data,
		     planes[pi].data_x * info->plane_depths[pi],
		     planes[pi].raster,
		     pie->width * info->plane_depths[pi],
		     *rows_used);
    pie->y += *rows_used;
    return code;
}

/* Clean up by releasing the buffers. */
private int
psw_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    gx_device *dev = info->dev;
    gdev_vector_image_enum_t *pie = (gdev_vector_image_enum_t *) info;
    int code;

    code = gdev_vector_end_image(vdev, pie, draw_last, pdev->white);
    if (code > 0) {
	psw_image_cleanup(pdev);
	pputs(pdev->strm, "\nQ\n");
    }
    return code;
}
