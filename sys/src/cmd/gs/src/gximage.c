/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gximage.c,v 1.7 2004/08/04 19:36:12 stefan Exp $ */
/* Generic image support */
#include "memory_.h"
#include "gx.h"
#include "gscspace.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gsutil.h"
#include "gxcolor2.h"		/* for lookup map */
#include "gxiparam.h"
#include "stream.h"

/* ---------------- Generic image support ---------------- */

/* Structure descriptors */
public_st_gs_image_common();
public_st_gs_data_image();
public_st_gs_pixel_image();

/* Initialize the common parts of image structures. */
void
gs_image_common_t_init(gs_image_common_t * pic)
{
    gs_make_identity(&pic->ImageMatrix);
}
void
gs_data_image_t_init(gs_data_image_t * pim, int num_components)
{
    int i;

    gs_image_common_t_init((gs_image_common_t *) pim);
    pim->Width = pim->Height = 0;
    pim->BitsPerComponent = 1;
    if (num_components >= 0) {
	for (i = 0; i < num_components * 2; i += 2)
	    pim->Decode[i] = 0, pim->Decode[i + 1] = 1;
    } else {
	for (i = 0; i < num_components * -2; i += 2)
	    pim->Decode[i] = 1, pim->Decode[i + 1] = 0;
    }
    pim->Interpolate = false;
}
void
gs_pixel_image_t_init(gs_pixel_image_t * pim,
		      const gs_color_space * color_space)
{
    int num_components;

    if (color_space == 0 ||
	(num_components =
	 gs_color_space_num_components(color_space)) < 0
	)
	num_components = 0;
    gs_data_image_t_init((gs_data_image_t *) pim, num_components);
    pim->format = gs_image_format_chunky;
    pim->ColorSpace = color_space;
    pim->CombineWithColor = false;
}

/* Initialize the common part of an image-processing enumerator. */
int
gx_image_enum_common_init(gx_image_enum_common_t * piec,
			  const gs_data_image_t * pic,
			  const gx_image_enum_procs_t * piep,
			  gx_device * dev, int num_components,
			  gs_image_format_t format)
{
    int bpc = pic->BitsPerComponent;
    int i;

    piec->image_type = pic->type;
    piec->procs = piep;
    piec->dev = dev;
    piec->id = gs_next_ids(dev->memory, 1);
    switch (format) {
	case gs_image_format_chunky:
	    piec->num_planes = 1;
	    piec->plane_depths[0] = bpc * num_components;
	    break;
	case gs_image_format_component_planar:
	    piec->num_planes = num_components;
	    for (i = 0; i < num_components; ++i)
		piec->plane_depths[i] = bpc;
	    break;
	case gs_image_format_bit_planar:
	    piec->num_planes = bpc * num_components;
	    for (i = 0; i < piec->num_planes; ++i)
		piec->plane_depths[i] = 1;
	    break;
	default:
	    return_error(gs_error_rangecheck);
    }
    for (i = 0; i < piec->num_planes; ++i)
	piec->plane_widths[i] = pic->Width;
    return 0;
}

/* Compute the source size of an ordinary image with explicit data. */
int
gx_data_image_source_size(const gs_imager_state * pis,
			  const gs_image_common_t * pim, gs_int_point * psize)
{
    const gs_data_image_t *pdi = (const gs_data_image_t *)pim;

    psize->x = pdi->Width;
    psize->y = pdi->Height;
    return 0;
}

/* Process the next piece of an image with no source data. */
/* This procedure should never be called. */
int
gx_no_plane_data(gx_image_enum_common_t * info,
		 const gx_image_plane_t * planes, int height,
		 int *height_used)
{
    return_error(gs_error_Fatal);
}

/* Clean up after processing an image with no source data. */
/* This procedure may be called, but should do nothing. */
int
gx_ignore_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    return 0;
}

/* ---------------- Client procedures ---------------- */

int
gx_image_data(gx_image_enum_common_t * info, const byte ** plane_data,
	      int data_x, uint raster, int height)
{
    int num_planes = info->num_planes;
    gx_image_plane_t planes[gs_image_max_planes];
    int i;

#ifdef DEBUG
    if (num_planes > gs_image_max_planes) {
	lprintf2("num_planes=%d > gs_image_max_planes=%d!\n",
		 num_planes, gs_image_max_planes);
	return_error(gs_error_Fatal);
    }
#endif
    for (i = 0; i < num_planes; ++i) {
	planes[i].data = plane_data[i];
	planes[i].data_x = data_x;
	planes[i].raster = raster;
    }
    return gx_image_plane_data(info, planes, height);
}

int
gx_image_plane_data(gx_image_enum_common_t * info,
		    const gx_image_plane_t * planes, int height)
{
    int ignore_rows_used;

    return gx_image_plane_data_rows(info, planes, height, &ignore_rows_used);
}

int
gx_image_plane_data_rows(gx_image_enum_common_t * info,
			 const gx_image_plane_t * planes, int height,
			 int *rows_used)
{
    return info->procs->plane_data(info, planes, height, rows_used);
}

int
gx_image_flush(gx_image_enum_common_t * info)
{
    int (*flush)(gx_image_enum_common_t *) = info->procs->flush;

    return (flush ? flush(info) : 0);
}

bool
gx_image_planes_wanted(const gx_image_enum_common_t *info, byte *wanted)
{
    bool (*planes_wanted)(const gx_image_enum_common_t *, byte *) =
	info->procs->planes_wanted;

    if (planes_wanted)
	return planes_wanted(info, wanted);
    else {
	memset(wanted, 0xff, info->num_planes);
	return true;
    }
}

int
gx_image_end(gx_image_enum_common_t * info, bool draw_last)
{
    return info->procs->end_image(info, draw_last);
}

/* ---------------- Serialization ---------------- */

/*
 * Define dummy sput/sget/release procedures for image types that don't
 * implement these functions.
 */

int
gx_image_no_sput(const gs_image_common_t *pic, stream *s,
		 const gs_color_space **ppcs)
{
    return_error(gs_error_rangecheck);
}

int
gx_image_no_sget(gs_image_common_t *pic, stream *s,
		 const gs_color_space *pcs)
{
    return_error(gs_error_rangecheck);
}

void
gx_image_default_release(gs_image_common_t *pic, gs_memory_t *mem)
{
    gs_free_object(mem, pic, "gx_image_default_release");
}

#ifdef DEBUG
private void
debug_b_print_matrix(const gs_pixel_image_t *pim)
{
    if_debug6('b', "      ImageMatrix=[%g %g %g %g %g %g]\n",
	      pim->ImageMatrix.xx, pim->ImageMatrix.xy,
	      pim->ImageMatrix.yx, pim->ImageMatrix.yy,
	      pim->ImageMatrix.tx, pim->ImageMatrix.ty);
}
private void
debug_b_print_decode(const gs_pixel_image_t *pim, int num_decode)
{
    if (gs_debug_c('b')) {
	const char *str = "      Decode=[";
	int i;

	for (i = 0; i < num_decode; str = " ", ++i)
	    dprintf2("%s%g", str, pim->Decode[i]);
	dputs("]\n");
    }
}
#else
#  define debug_b_print_matrix(pim) DO_NOTHING
#  define debug_b_print_decode(pim, num_decode) DO_NOTHING
#endif

/* Test whether an image has a default ImageMatrix. */
bool
gx_image_matrix_is_default(const gs_data_image_t *pid)
{
    return (is_xxyy(&pid->ImageMatrix) &&
	    pid->ImageMatrix.xx == pid->Width &&
	    pid->ImageMatrix.yy == -pid->Height &&
	    is_fzero(pid->ImageMatrix.tx) &&
	    pid->ImageMatrix.ty == pid->Height);
}

/* Put a variable-length uint on a stream. */
void
sput_variable_uint(stream *s, uint w)
{
    for (; w > 0x7f; w >>= 7)
	sputc(s, (byte)(w | 0x80));
    sputc(s, (byte)w);
}

/*
 * Write generic pixel image parameters.  The format is the following,
 * encoded as a variable-length uint in the usual way:
 *	xxxFEDCCBBBBA
 *	    A = 0 if standard ImageMatrix, 1 if explicit ImageMatrix
 *	    BBBB = BitsPerComponent - 1
 *	    CC = format
 *	    D = 0 if standard (0..1) Decode, 1 if explicit Decode
 *	    E = Interpolate
 *	    F = CombineWithColor
 *	    xxx = extra information from caller
 */
#define PI_ImageMatrix 0x001
#define PI_BPC_SHIFT 1
#define PI_BPC_MASK 0xf
#define PI_FORMAT_SHIFT 5
#define PI_FORMAT_MASK 0x3
#define PI_Decode 0x080
#define PI_Interpolate 0x100
#define PI_CombineWithColor 0x200
#define PI_BITS 10
/*
 *	Width, encoded as a variable-length uint
 *	Height, encoded ditto
 *	ImageMatrix (if A = 1), per gs_matrix_store/fetch
 *	Decode (if D = 1): blocks of up to 4 components
 *	    aabbccdd, where each xx is decoded as:
 *		00 = default, 01 = swapped default,
 *		10 = (0,V), 11 = (U,V)
 *	    non-defaulted components (up to 8 floats)
 */
int
gx_pixel_image_sput(const gs_pixel_image_t *pim, stream *s,
		    const gs_color_space **ppcs, int extra)
{
    const gs_color_space *pcs = pim->ColorSpace;
    int bpc = pim->BitsPerComponent;
    int num_components = gs_color_space_num_components(pcs);
    int num_decode;
    uint control = extra << PI_BITS;
    float decode_default_1 = 1;
    int i;
    uint ignore;

    /* Construct the control word. */

    if (!gx_image_matrix_is_default((const gs_data_image_t *)pim))
	control |= PI_ImageMatrix;
    switch (pim->format) {
    case gs_image_format_chunky:
    case gs_image_format_component_planar:
	switch (bpc) {
	case 1: case 2: case 4: case 8: case 12: break;
	default: return_error(gs_error_rangecheck);
	}
	break;
    case gs_image_format_bit_planar:
	if (bpc < 1 || bpc > 8)
	    return_error(gs_error_rangecheck);
    }
    control |= (bpc - 1) << PI_BPC_SHIFT;
    control |= pim->format << PI_FORMAT_SHIFT;
    num_decode = num_components * 2;
    if (gs_color_space_get_index(pcs) == gs_color_space_index_Indexed)
	decode_default_1 = (float)pcs->params.indexed.hival;
    for (i = 0; i < num_decode; ++i)
	if (pim->Decode[i] != DECODE_DEFAULT(i, decode_default_1)) {
	    control |= PI_Decode;
	    break;
	}
    if (pim->Interpolate)
	control |= PI_Interpolate;
    if (pim->CombineWithColor)
	control |= PI_CombineWithColor;

    /* Write the encoding on the stream. */

    if_debug3('b', "[b]put control=0x%x, Width=%d, Height=%d\n",
	      control, pim->Width, pim->Height);
    sput_variable_uint(s, control);
    sput_variable_uint(s, (uint)pim->Width);
    sput_variable_uint(s, (uint)pim->Height);
    if (control & PI_ImageMatrix) {
	debug_b_print_matrix(pim);
	sput_matrix(s, &pim->ImageMatrix);
    }
    if (control & PI_Decode) {
	int i;
	uint dflags = 1;
	float decode[8];
	int di = 0;

	debug_b_print_decode(pim, num_decode);
	for (i = 0; i < num_decode; i += 2) {
	    float u = pim->Decode[i], v = pim->Decode[i + 1];
	    float dv = DECODE_DEFAULT(i + 1, decode_default_1);

	    if (dflags >= 0x100) {
		sputc(s, (byte)(dflags & 0xff));
		sputs(s, (const byte *)decode, di * sizeof(float), &ignore);
		dflags = 1;
		di = 0;
	    }
	    dflags <<= 2;
	    if (u == 0 && v == dv)
		DO_NOTHING;
	    else if (u == dv && v == 0)
		dflags += 1;
	    else {
		if (u != 0) {
		    dflags++;
		    decode[di++] = u;
		}
		dflags += 2;
		decode[di++] = v;
	    }
	}
	sputc(s, (byte)((dflags << (8 - num_decode)) & 0xff));
	sputs(s, (const byte *)decode, di * sizeof(float), &ignore);
    }
    *ppcs = pcs;
    return 0;
}

/* Set an image's ImageMatrix to the default. */
void
gx_image_matrix_set_default(gs_data_image_t *pid)
{
    pid->ImageMatrix.xx = (float)pid->Width;
    pid->ImageMatrix.xy = 0;
    pid->ImageMatrix.yx = 0;
    pid->ImageMatrix.yy = (float)-pid->Height;
    pid->ImageMatrix.tx = 0;
    pid->ImageMatrix.ty = (float)pid->Height;
}

/* Get a variable-length uint from a stream. */
int
sget_variable_uint(stream *s, uint *pw)
{
    uint w = 0;
    int shift = 0;
    int ch;

    for (; (ch = sgetc(s)) >= 0x80; shift += 7)
	w += (ch & 0x7f) << shift;
    if (ch < 0)
	return_error(gs_error_ioerror);
    *pw = w + (ch << shift);
    return 0;
}

/*
 * Read generic pixel image parameters.
 */
int
gx_pixel_image_sget(gs_pixel_image_t *pim, stream *s,
		    const gs_color_space *pcs)
{
    uint control;
    float decode_default_1 = 1;
    int num_components, num_decode;
    int i;
    int code;
    uint ignore;

    if ((code = sget_variable_uint(s, &control)) < 0 ||
	(code = sget_variable_uint(s, (uint *)&pim->Width)) < 0 ||
	(code = sget_variable_uint(s, (uint *)&pim->Height)) < 0
	)
	return code;
    if_debug3('b', "[b]get control=0x%x, Width=%d, Height=%d\n",
	      control, pim->Width, pim->Height);
    if (control & PI_ImageMatrix) {
	if ((code = sget_matrix(s, &pim->ImageMatrix)) < 0)
	    return code;
	debug_b_print_matrix(pim);
    } else
	gx_image_matrix_set_default((gs_data_image_t *)pim);
    pim->BitsPerComponent = ((control >> PI_BPC_SHIFT) & PI_BPC_MASK) + 1;
    pim->format = (control >> PI_FORMAT_SHIFT) & PI_FORMAT_MASK;
    pim->ColorSpace = pcs;
    num_components = gs_color_space_num_components(pcs);
    num_decode = num_components * 2;
    if (gs_color_space_get_index(pcs) == gs_color_space_index_Indexed)
	decode_default_1 = (float)pcs->params.indexed.hival;
    if (control & PI_Decode) {
	uint dflags = 0x10000;
	float *dp = pim->Decode;

	for (i = 0; i < num_decode; i += 2, dp += 2, dflags <<= 2) {
	    if (dflags >= 0x10000) {
		dflags = sgetc(s) + 0x100;
		if (dflags < 0x100)
		    return_error(gs_error_ioerror);
	    }
	    switch (dflags & 0xc0) {
	    case 0x00:
		dp[0] = 0, dp[1] = DECODE_DEFAULT(i + 1, decode_default_1);
		break;
	    case 0x40:
		dp[0] = DECODE_DEFAULT(i + 1, decode_default_1), dp[1] = 0;
		break;
	    case 0x80:
		dp[0] = 0;
		if (sgets(s, (byte *)(dp + 1), sizeof(float), &ignore) < 0)
		    return_error(gs_error_ioerror);
		break;
	    case 0xc0:
		if (sgets(s, (byte *)dp, sizeof(float) * 2, &ignore) < 0)
		    return_error(gs_error_ioerror);
		break;
	    }
	}
	debug_b_print_decode(pim, num_decode);
    } else {
        for (i = 0; i < num_decode; ++i)
	    pim->Decode[i] = DECODE_DEFAULT(i, decode_default_1);
    }
    pim->Interpolate = (control & PI_Interpolate) != 0;
    pim->CombineWithColor = (control & PI_CombineWithColor) != 0;
    return control >> PI_BITS;
}

/*
 * Release a pixel image object.  Currently this just frees the object.
 */
void
gx_pixel_image_release(gs_pixel_image_t *pic, gs_memory_t *mem)
{
    gx_image_default_release((gs_image_common_t *)pic, mem);
}
