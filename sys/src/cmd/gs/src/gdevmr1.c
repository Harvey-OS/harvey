/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevmr1.c,v 1.5 2002/08/22 07:12:28 henrys Exp $ */
/* RasterOp implementation for monobit memory devices */
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
#include "gdevmrop.h"

/* Calculate the X offset for a given Y value, */
/* taking shift into account if necessary. */
#define x_offset(px, ty, textures)\
  ((textures)->shift == 0 ? (px) :\
   (px) + (ty) / (textures)->rep_height * (textures)->rep_shift)

/* ---------------- Monobit RasterOp ---------------- */

int
mem_mono_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			int x, int y, int width, int height,
			int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_memory *mdev = (gx_device_memory *) dev;
    gs_rop3_t rop = gs_transparent_rop(lop);	/* handle transparency */
    gx_strip_bitmap no_texture;
    bool invert;
    uint draster = mdev->raster;
    uint traster;
    int line_count;
    byte *drow;
    const byte *srow;
    int ty;

    /* If map_rgb_color isn't the default one for monobit memory */
    /* devices, palette might not be set; set it now if needed. */
    if (mdev->palette.data == 0) {
        gx_color_value cv[3];
        cv[0] = cv[1] = cv[2] = 0;
	gdev_mem_mono_set_inverted(mdev,
				   (*dev_proc(dev, map_rgb_color))
				   (dev, cv) != 0);
    }
    invert = mdev->palette.data[0] != 0;

#ifdef DEBUG
    if (gs_debug_c('b'))
	trace_copy_rop("mem_mono_strip_copy_rop",
		       dev, sdata, sourcex, sraster,
		       id, scolors, textures, tcolors,
		       x, y, width, height, phase_x, phase_y, lop);
    if (gs_debug_c('B'))
	debug_dump_bitmap(scan_line_base(mdev, y), mdev->raster,
			  height, "initial dest bits");
#endif

    /*
     * RasterOp is defined as operating in RGB space; in the monobit
     * case, this means black = 0, white = 1.  However, most monobit
     * devices use the opposite convention.  To make this work,
     * we must precondition the Boolean operation by swapping the
     * order of bits end-for-end and then inverting.
     */

    if (invert)
	rop = byte_reverse_bits[rop] ^ 0xff;

    /*
     * From this point on, rop works in terms of device pixel values,
     * not RGB-space values.
     */

    /* Modify the raster operation according to the source palette. */
    if (scolors != 0) {		/* Source with palette. */
	switch ((int)((scolors[1] << 1) + scolors[0])) {
	    case 0:
		rop = rop3_know_S_0(rop);
		break;
	    case 1:
		rop = rop3_invert_S(rop);
		break;
	    case 2:
		break;
	    case 3:
		rop = rop3_know_S_1(rop);
		break;
	}
    }
    /* Modify the raster operation according to the texture palette. */
    if (tcolors != 0) {		/* Texture with palette. */
	switch ((int)((tcolors[1] << 1) + tcolors[0])) {
	    case 0:
		rop = rop3_know_T_0(rop);
		break;
	    case 1:
		rop = rop3_invert_T(rop);
		break;
	    case 2:
		break;
	    case 3:
		rop = rop3_know_T_1(rop);
		break;
	}
    }
    /* Handle constant source and/or texture, and other special cases. */
    {
	gx_color_index color0, color1;

	switch (rop_usage_table[rop]) {
	    case rop_usage_none:
		/* We're just filling with a constant. */
		return (*dev_proc(dev, fill_rectangle))
		    (dev, x, y, width, height, (gx_color_index) (rop & 1));
	    case rop_usage_D:
		/* This is either D (no-op) or ~D. */
		if (rop == rop3_D)
		    return 0;
		/* Code no_S inline, then finish with no_T. */
		fit_fill(dev, x, y, width, height);
		sdata = scan_line_base(mdev, 0);
		sourcex = x;
		sraster = 0;
		goto no_T;
	    case rop_usage_S:
		/* This is either S or ~S, which copy_mono can handle. */
		if (rop == rop3_S)
		    color0 = 0, color1 = 1;
		else
		    color0 = 1, color1 = 0;
	      do_copy:return (*dev_proc(dev, copy_mono))
		    (dev, sdata, sourcex, sraster, id, x, y, width, height,
		     color0, color1);
	    case rop_usage_DS:
		/* This might be a case that copy_mono can handle. */
#define copy_case(c0, c1) color0 = c0, color1 = c1; goto do_copy;
		switch ((uint) rop) {	/* cast shuts up picky compilers */
		    case rop3_D & rop3_not(rop3_S):
			copy_case(gx_no_color_index, 0);
		    case rop3_D | rop3_S:
			copy_case(gx_no_color_index, 1);
		    case rop3_D & rop3_S:
			copy_case(0, gx_no_color_index);
		    case rop3_D | rop3_not(rop3_S):
			copy_case(1, gx_no_color_index);
		    default:;
		}
#undef copy_case
		fit_copy(dev, sdata, sourcex, sraster, id, x, y, width, height);
	      no_T:		/* Texture is not used; textures may be garbage. */
		no_texture.data = scan_line_base(mdev, 0);  /* arbitrary */
		no_texture.raster = 0;
		no_texture.size.x = width;
		no_texture.size.y = height;
		no_texture.rep_width = no_texture.rep_height = 1;
		no_texture.rep_shift = no_texture.shift = 0;
		textures = &no_texture;
		break;
	    case rop_usage_T:
		/* This is either T or ~T, which tile_rectangle can handle. */
		if (rop == rop3_T)
		    color0 = 0, color1 = 1;
		else
		    color0 = 1, color1 = 0;
	      do_tile:return (*dev_proc(dev, strip_tile_rectangle))
		    (dev, textures, x, y, width, height, color0, color1,
		     phase_x, phase_y);
	    case rop_usage_DT:
		/* This might be a case that tile_rectangle can handle. */
#define tile_case(c0, c1) color0 = c0, color1 = c1; goto do_tile;
		switch ((uint) rop) {	/* cast shuts up picky compilers */
		    case rop3_D & rop3_not(rop3_T):
			tile_case(gx_no_color_index, 0);
		    case rop3_D | rop3_T:
			tile_case(gx_no_color_index, 1);
		    case rop3_D & rop3_T:
			tile_case(0, gx_no_color_index);
		    case rop3_D | rop3_not(rop3_T):
			tile_case(1, gx_no_color_index);
		    default:;
		}
#undef tile_case
		fit_fill(dev, x, y, width, height);
		/* Source is not used; sdata et al may be garbage. */
		sdata = mdev->base;	/* arbitrary, as long as all */
					/* accesses are valid */
		sourcex = x;	/* guarantee no source skew */
		sraster = 0;
		break;
	    default:		/* rop_usage_[D]ST */
		fit_copy(dev, sdata, sourcex, sraster, id, x, y, width, height);
	}
    }

#ifdef DEBUG
    if_debug1('b', "final rop=0x%x\n", rop);
#endif

    /* Set up transfer parameters. */
    line_count = height;
    srow = sdata;
    drow = scan_line_base(mdev, y);
    traster = textures->raster;
    ty = y + phase_y;

    /* Loop over scan lines. */
    for (; line_count-- > 0; drow += draster, srow += sraster, ++ty) {
	int sx = sourcex;
	int dx = x;
	int w = width;
	const byte *trow =
	textures->data + (ty % textures->rep_height) * traster;
	int xoff = x_offset(phase_x, ty, textures);
	int nw;

	/* Loop over (horizontal) copies of the tile. */
	for (; w > 0; sx += nw, dx += nw, w -= nw) {
	    int dbit = dx & 7;
	    int sbit = sx & 7;
	    int sskew = sbit - dbit;
	    int tx = (dx + xoff) % textures->rep_width;
	    int tbit = tx & 7;
	    int tskew = tbit - dbit;
	    int left = nw = min(w, textures->size.x - tx);
	    byte lmask = 0xff >> dbit;
	    byte rmask = 0xff << (~(dbit + nw - 1) & 7);
	    byte mask = lmask;
	    int nx = 8 - dbit;
	    byte *dptr = drow + (dx >> 3);
	    const byte *sptr = srow + (sx >> 3);
	    const byte *tptr = trow + (tx >> 3);

	    if (sskew < 0)
		--sptr, sskew += 8;
	    if (tskew < 0)
		--tptr, tskew += 8;
	    for (; left > 0;
		 left -= nx, mask = 0xff, nx = 8,
		 ++dptr, ++sptr, ++tptr
		) {
		byte dbyte = *dptr;

#define fetch1(ptr, skew)\
  (skew ? (ptr[0] << skew) + (ptr[1] >> (8 - skew)) : *ptr)
		byte sbyte = fetch1(sptr, sskew);
		byte tbyte = fetch1(tptr, tskew);

#undef fetch1
		byte result =
		(*rop_proc_table[rop]) (dbyte, sbyte, tbyte);

		if (left <= nx)
		    mask &= rmask;
		*dptr = (mask == 0xff ? result :
			 (result & mask) | (dbyte & ~mask));
	    }
	}
    }
#ifdef DEBUG
    if (gs_debug_c('B'))
	debug_dump_bitmap(scan_line_base(mdev, y), mdev->raster,
			  height, "final dest bits");
#endif
    return 0;
}
