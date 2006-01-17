/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevmr8n.c,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* RasterOp implementation for 8N-bit memory devices */
#include "memory_.h"
#include "gx.h"
#include "gsbittab.h"
#include "gserrors.h"
#include "gsropt.h"
#include "gxcindex.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxdevrop.h"
#include "gdevmem.h"
#include "gdevmrop.h"

/*
 * NOTE: The 16- and 32-bit cases aren't implemented: they just fall back to
 * the default implementation.  This is very slow and will be fixed someday.
 */

#define chunk byte

/* Calculate the X offset for a given Y value, */
/* taking shift into account if necessary. */
#define x_offset(px, ty, textures)\
  ((textures)->shift == 0 ? (px) :\
   (px) + (ty) / (textures)->rep_height * (textures)->rep_shift)

/* ---------------- RasterOp with 8-bit gray / 24-bit RGB ---------------- */

int
mem_gray8_rgb24_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			       const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			       int x, int y, int width, int height,
		       int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_memory *mdev = (gx_device_memory *) dev;
    gs_rop3_t rop = lop_rop(lop);
    gx_color_index const_source = gx_no_color_index;
    gx_color_index const_texture = gx_no_color_index;
    uint draster = mdev->raster;
    int line_count;
    byte *drow;
    int depth = dev->color_info.depth;
    int bpp = depth >> 3;	/* bytes per pixel, 1 or 3 */
    gx_color_index all_ones = ((gx_color_index) 1 << depth) - 1;
    gx_color_index strans =
	(lop & lop_S_transparent ? all_ones : gx_no_color_index);
    gx_color_index ttrans =
	(lop & lop_T_transparent ? all_ones : gx_no_color_index);

    /* Check for constant source. */
    if (!rop3_uses_S(rop))
	const_source = 0;	/* arbitrary */
    else if (scolors != 0 && scolors[0] == scolors[1]) {
	/* Constant source */
	const_source = scolors[0];
	if (const_source == gx_device_black(dev))
	    rop = rop3_know_S_0(rop);
	else if (const_source == gx_device_white(dev))
	    rop = rop3_know_S_1(rop);
    }

    /* Check for constant texture. */
    if (!rop3_uses_T(rop))
	const_texture = 0;	/* arbitrary */
    else if (tcolors != 0 && tcolors[0] == tcolors[1]) {
	/* Constant texture */
	const_texture = tcolors[0];
	if (const_texture == gx_device_black(dev))
	    rop = rop3_know_T_0(rop);
	else if (const_texture == gx_device_white(dev))
	    rop = rop3_know_T_1(rop);
    }

    if (bpp == 1 &&
	(gx_device_has_color(dev) ||
	 (gx_device_black(dev) != 0 || gx_device_white(dev) != all_ones))
	) {
	/*
	 * This is an 8-bit device but not gray-scale.  Except in a few
	 * simple cases, we have to use the slow algorithm that converts
	 * values to and from RGB.
	 */
	gx_color_index bw_pixel;

	switch (rop) {
	case rop3_0:
	    bw_pixel = gx_device_black(dev);
	    goto bw;
	case rop3_1:
	    bw_pixel = gx_device_white(dev);
bw:	    if (bw_pixel == 0x00)
		rop = rop3_0;
	    else if (bw_pixel == 0xff)
		rop = rop3_1;
	    else
		goto df;
	    break;
	case rop3_D:
	    break;
	case rop3_S:
	    if (lop & lop_S_transparent)
		goto df;
	    break;
	case rop3_T:
	    if (lop & lop_T_transparent)
		goto df;
	    break;
	default:
df:	    return mem_default_strip_copy_rop(dev,
					      sdata, sourcex, sraster, id,
					      scolors, textures, tcolors,
					      x, y, width, height,
					      phase_x, phase_y, lop);
	}
    }

    /* Adjust coordinates to be in bounds. */
    if (const_source == gx_no_color_index) {
	fit_copy(dev, sdata, sourcex, sraster, id,
		 x, y, width, height);
    } else {
	fit_fill(dev, x, y, width, height);
    }

    /* Set up transfer parameters. */
    line_count = height;
    drow = scan_line_base(mdev, y) + x * bpp;

    /*
     * There are 18 cases depending on whether each of the source and
     * texture is constant, 1-bit, or multi-bit, and on whether the
     * depth is 8 or 24 bits.  We divide first according to constant
     * vs. non-constant, and then according to 1- vs. multi-bit, and
     * finally according to pixel depth.  This minimizes source code,
     * but not necessarily time, since we do some of the divisions
     * within 1 or 2 levels of loop.
     */

#define dbit(base, i) ((base)[(i) >> 3] & (0x80 >> ((i) & 7)))
/* 8-bit */
#define cbit8(base, i, colors)\
  (dbit(base, i) ? (byte)colors[1] : (byte)colors[0])
#define rop_body_8(s_pixel, t_pixel)\
  if ( (s_pixel) == strans ||	/* So = 0, s_tr = 1 */\
       (t_pixel) == ttrans	/* Po = 0, p_tr = 1 */\
     )\
    continue;\
  *dptr = (*rop_proc_table[rop])(*dptr, s_pixel, t_pixel)
/* 24-bit */
#define get24(ptr)\
  (((gx_color_index)(ptr)[0] << 16) | ((gx_color_index)(ptr)[1] << 8) | (ptr)[2])
#define put24(ptr, pixel)\
  (ptr)[0] = (byte)((pixel) >> 16),\
  (ptr)[1] = (byte)((uint)(pixel) >> 8),\
  (ptr)[2] = (byte)(pixel)
#define cbit24(base, i, colors)\
  (dbit(base, i) ? colors[1] : colors[0])
#define rop_body_24(s_pixel, t_pixel)\
  if ( (s_pixel) == strans ||	/* So = 0, s_tr = 1 */\
       (t_pixel) == ttrans	/* Po = 0, p_tr = 1 */\
     )\
    continue;\
  { gx_color_index d_pixel = get24(dptr);\
    d_pixel = (*rop_proc_table[rop])(d_pixel, s_pixel, t_pixel);\
    put24(dptr, d_pixel);\
  }

    if (const_texture != gx_no_color_index) {
/**** Constant texture ****/
	if (const_source != gx_no_color_index) {
/**** Constant source & texture ****/
	    for (; line_count-- > 0; drow += draster) {
		byte *dptr = drow;
		int left = width;

		if (bpp == 1)
/**** 8-bit destination ****/
		    for (; left > 0; ++dptr, --left) {
			rop_body_8((byte)const_source, (byte)const_texture);
		    }
		else
/**** 24-bit destination ****/
		    for (; left > 0; dptr += 3, --left) {
			rop_body_24(const_source, const_texture);
		    }
	    }
	} else {
/**** Data source, const texture ****/
	    const byte *srow = sdata;

	    for (; line_count-- > 0; drow += draster, srow += sraster) {
		byte *dptr = drow;
		int left = width;

		if (scolors) {
/**** 1-bit source ****/
		    int sx = sourcex;

		    if (bpp == 1)
/**** 8-bit destination ****/
			for (; left > 0; ++dptr, ++sx, --left) {
			    byte s_pixel = cbit8(srow, sx, scolors);

			    rop_body_8(s_pixel, (byte)const_texture);
			}
		    else
/**** 24-bit destination ****/
			for (; left > 0; dptr += 3, ++sx, --left) {
			    bits32 s_pixel = cbit24(srow, sx, scolors);

			    rop_body_24(s_pixel, const_texture);
			}
		} else if (bpp == 1) {
/**** 8-bit source & dest ****/
		    const byte *sptr = srow + sourcex;

		    for (; left > 0; ++dptr, ++sptr, --left) {
			byte s_pixel = *sptr;

			rop_body_8(s_pixel, (byte)const_texture);
		    }
		} else {
/**** 24-bit source & dest ****/
		    const byte *sptr = srow + sourcex * 3;

		    for (; left > 0; dptr += 3, sptr += 3, --left) {
			bits32 s_pixel = get24(sptr);

			rop_body_24(s_pixel, const_texture);
		    }
		}
	    }
	}
    } else if (const_source != gx_no_color_index) {
/**** Const source, data texture ****/
	uint traster = textures->raster;
	int ty = y + phase_y;

	for (; line_count-- > 0; drow += draster, ++ty) {	/* Loop over copies of the tile. */
	    int dx = x, w = width, nw;
	    byte *dptr = drow;
	    const byte *trow =
	    textures->data + (ty % textures->size.y) * traster;
	    int xoff = x_offset(phase_x, ty, textures);

	    for (; w > 0; dx += nw, w -= nw) {
		int tx = (dx + xoff) % textures->rep_width;
		int left = nw = min(w, textures->size.x - tx);
		const byte *tptr = trow;

		if (tcolors) {
/**** 1-bit texture ****/
		    if (bpp == 1)
/**** 8-bit dest ****/
			for (; left > 0; ++dptr, ++tx, --left) {
			    byte t_pixel = cbit8(tptr, tx, tcolors);

			    rop_body_8((byte)const_source, t_pixel);
			}
		    else
/**** 24-bit dest ****/
			for (; left > 0; dptr += 3, ++tx, --left) {
			    bits32 t_pixel = cbit24(tptr, tx, tcolors);

			    rop_body_24(const_source, t_pixel);
			}
		} else if (bpp == 1) {
/**** 8-bit T & D ****/
		    tptr += tx;
		    for (; left > 0; ++dptr, ++tptr, --left) {
			byte t_pixel = *tptr;

			rop_body_8((byte)const_source, t_pixel);
		    }
		} else {
/**** 24-bit T & D ****/
		    tptr += tx * 3;
		    for (; left > 0; dptr += 3, tptr += 3, --left) {
			bits32 t_pixel = get24(tptr);

			rop_body_24(const_source, t_pixel);
		    }
		}
	    }
	}
    } else {
/**** Data source & texture ****/
	uint traster = textures->raster;
	int ty = y + phase_y;
	const byte *srow = sdata;

	/* Loop over scan lines. */
	for (; line_count-- > 0; drow += draster, srow += sraster, ++ty) {	/* Loop over copies of the tile. */
	    int sx = sourcex;
	    int dx = x;
	    int w = width;
	    int nw;
	    byte *dptr = drow;
	    const byte *trow =
	    textures->data + (ty % textures->size.y) * traster;
	    int xoff = x_offset(phase_x, ty, textures);

	    for (; w > 0; dx += nw, w -= nw) {	/* Loop over individual pixels. */
		int tx = (dx + xoff) % textures->rep_width;
		int left = nw = min(w, textures->size.x - tx);
		const byte *tptr = trow;

		/*
		 * For maximum speed, we should split this loop
		 * into 7 cases depending on source & texture
		 * depth: (1,1), (1,8), (1,24), (8,1), (8,8),
		 * (24,1), (24,24).  But since we expect these
		 * cases to be relatively uncommon, we just
		 * divide on the destination depth.
		 */
		if (bpp == 1) {
/**** 8-bit destination ****/
		    const byte *sptr = srow + sx;

		    tptr += tx;
		    for (; left > 0; ++dptr, ++sptr, ++tptr, ++sx, ++tx, --left) {
			byte s_pixel =
			    (scolors ? cbit8(srow, sx, scolors) : *sptr);
			byte t_pixel =
			    (tcolors ? cbit8(tptr, tx, tcolors) : *tptr);

			rop_body_8(s_pixel, t_pixel);
		    }
		} else {
/**** 24-bit destination ****/
		    const byte *sptr = srow + sx * 3;

		    tptr += tx * 3;
		    for (; left > 0; dptr += 3, sptr += 3, tptr += 3, ++sx, ++tx, --left) {
			bits32 s_pixel =
			    (scolors ? cbit24(srow, sx, scolors) :
			     get24(sptr));
			bits32 t_pixel =
			    (tcolors ? cbit24(tptr, tx, tcolors) :
			     get24(tptr));

			rop_body_24(s_pixel, t_pixel);
		    }
		}
	    }
	}
    }
#undef rop_body_8
#undef rop_body_24
#undef dbit
#undef cbit8
#undef cbit24
    return 0;
}
