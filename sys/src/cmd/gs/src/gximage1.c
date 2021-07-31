/* Copyright (C) 1989, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gximage1.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* ImageType 1 initialization */
#include "gx.h"
#include "gserrors.h"
#include "gxiparam.h"
#include "gximage.h"
#include "stream.h"

/* Structure descriptor */
private_st_gs_image1();

/*
 * Define the image types for ImageType 1 images.
 * Note that opaque images and masks have different types, although
 * they have the same ImageType.
 */
private image_proc_sput(gx_image1_sput);
private image_proc_sget(gx_image1_sget);
private image_proc_release(gx_image1_release);
const gx_image_type_t gs_image_type_1 = {
    &st_gs_image1, gx_begin_image1, gx_data_image_source_size,
    gx_image1_sput, gx_image1_sget, gx_image1_release, 1
};
private image_proc_sput(gx_image1_mask_sput);
private image_proc_sget(gx_image1_mask_sget);
/*
 * Image masks are not actually pixel images, so they don't need to
 * worry about releasing the color space.
 */
const gx_image_type_t gs_image_type_mask1 = {
    &st_gs_image1, gx_begin_image1, gx_data_image_source_size,
    gx_image1_mask_sput, gx_image1_mask_sget, gx_image_default_release, 1
};

/* Define the procedures for initializing gs_image_ts to default values. */
void
gs_image_t_init_adjust(gs_image_t * pim, const gs_color_space * color_space,
		       bool adjust)
{
    gs_pixel_image_t_init((gs_pixel_image_t *) pim, color_space);
    pim->ImageMask = (color_space == NULL);
    pim->adjust = adjust;
    pim->type = (pim->ImageMask ? &gs_image_type_mask1 : &gs_image_type_1);
    pim->Alpha = gs_image_alpha_none;
}
void
gs_image_t_init_mask_adjust(gs_image_t * pim, bool write_1s, bool adjust)
{
    gs_image_t_init(pim, NULL);
    if (write_1s)
	pim->Decode[0] = 1, pim->Decode[1] = 0;
    else
	pim->Decode[0] = 0, pim->Decode[1] = 1;
    pim->adjust = adjust;
}

/* Start processing an ImageType 1 image. */
int
gx_begin_image1(gx_device * dev,
		const gs_imager_state * pis, const gs_matrix * pmat,
		const gs_image_common_t * pic, const gs_int_rect * prect,
		const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    gx_image_enum *penum;
    const gs_image_t *pim = (const gs_image_t *)pic;
    int code = gx_image_enum_alloc(pic, prect, mem, &penum);

    if (code < 0)
	return code;
    penum->alpha = pim->Alpha;
    penum->use_mask_color = false;
    penum->masked = pim->ImageMask;
    penum->adjust =
	(pim->ImageMask && pim->adjust ? float2fixed(0.25) : fixed_0);
    code = gx_image_enum_begin(dev, pis, pmat, pic, pdcolor, pcpath, mem,
			       penum);
    if (code >= 0)
	*pinfo = (gx_image_enum_common_t *)penum;
    return code;
}

/* Serialization */

private int
gx_image1_sput(const gs_image_common_t *pic, stream *s,
	       const gs_color_space **ppcs)
{
    const gs_image_t *const pim = (const gs_image_t *)pic;

    return gx_pixel_image_sput((const gs_pixel_image_t *)pic, s, ppcs,
			       (int)pim->Alpha);
}

private int
gx_image1_sget(gs_image_common_t *pic, stream *s,
	       const gs_color_space *pcs)
{
    gs_image1_t *const pim = (gs_image1_t *)pic;
    int code = gx_pixel_image_sget((gs_pixel_image_t *)pim, s, pcs);

    if (code < 0)
	return code;
    pim->type = &gs_image_type_1;
    pim->ImageMask = false;
    pim->Alpha = code;
    return 0;
}

/*
 * Masks have different parameters, so we use a different encoding:
 *	ABCDEE00
 *	    A = 0 if standard ImageMatrix, 1 if explicit ImageMatrix
 *	    B = 0 if Decode=[0 1], 1 if Decode=[1 0]
 *	    C = Interpolate
 *	    D = adjust
 *	    EE = Alpha
 *	Width, 7 bits per byte, low-order bits first, 0x80 = more bytes
 *	Height, encoded like Width
 *	ImageMatrix (if A = 1), per gs_matrix_store/fetch
 */

private int
gx_image1_mask_sput(const gs_image_common_t *pic, stream *s,
		    const gs_color_space **ignore_ppcs)
{
    const gs_image_t *pim = (const gs_image_t *)pic;
    byte b =
	(gx_image_matrix_is_default((const gs_data_image_t *)pim) ? 0 : 0x80) |
	(pim->Decode[0] != 0 ? 0x40 : 0) |
	(pim->Interpolate ? 0x20 : 0) |
	(pim->adjust ? 0x10 : 0) |
	(pim->Alpha << 2);

    sputc(s, b);
    sput_variable_uint(s, (uint)pim->Width);
    sput_variable_uint(s, (uint)pim->Height);
    if (b & 0x80)
	sput_matrix(s, &pim->ImageMatrix);
    return 0;
}

private int
gx_image1_mask_sget(gs_image_common_t *pic, stream *s,
		    const gs_color_space *ignore_pcs)
{
    gs_image1_t *const pim = (gs_image1_t *)pic;
    byte b = sgetc(s);
    int code;

    gs_image_t_init_mask(pim, (b & 0x40) != 0);
    if ((code = sget_variable_uint(s, (uint *)&pim->Width)) < 0 ||
	(code = sget_variable_uint(s, (uint *)&pim->Height)) < 0
	)
	return code;
    if (b & 0x80) {
	if ((code = sget_matrix(s, &pim->ImageMatrix)) < 0)
	    return code;
    } else
	gx_image_matrix_set_default((gs_data_image_t *)pim);
    pim->Interpolate = (b & 0x20) != 0;
    pim->adjust = (b & 0x10) != 0;
    pim->Alpha = (b >> 2) & 3;
    return 0;
}

private void
gx_image1_release(gs_image_common_t *pic, gs_memory_t *mem)
{
    gx_pixel_image_release((gs_pixel_image_t *)pic, mem);
}
