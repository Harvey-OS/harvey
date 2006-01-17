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

/* $Id: gdevmr2n.c,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* RasterOp implementation for 2- and 4-bit memory devices */
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

/* Calculate the X offset for a given Y value, */
/* taking shift into account if necessary. */
#define x_offset(px, ty, textures)\
  ((textures)->shift == 0 ? (px) :\
   (px) + (ty) / (textures)->rep_height * (textures)->rep_shift)

/* ---------------- Fake RasterOp for 2- and 4-bit devices ---------------- */

/*
 * Define patched versions of the driver procedures that may be called
 * by mem_mono_strip_copy_rop (see below).  Currently we just punt to
 * the slow, general case; we could do a lot better.
 */
private int
mem_gray_rop_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			    gx_color_index color)
{
    return -1;
}
private int
mem_gray_rop_copy_mono(gx_device * dev, const byte * data,
		       int dx, int raster, gx_bitmap_id id,
		       int x, int y, int w, int h,
		       gx_color_index zero, gx_color_index one)
{
    return -1;
}
private int
mem_gray_rop_strip_tile_rectangle(gx_device * dev,
				  const gx_strip_bitmap * tiles,
				  int x, int y, int w, int h,
				  gx_color_index color0, gx_color_index color1,
				  int px, int py)
{
    return -1;
}

int
mem_gray_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			int x, int y, int width, int height,
			int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_color_index scolors2[2];
    const gx_color_index *real_scolors = scolors;
    gx_color_index tcolors2[2];
    const gx_color_index *real_tcolors = tcolors;
    gx_strip_bitmap texture2;
    const gx_strip_bitmap *real_texture = textures;
    long tdata;
    int depth = dev->color_info.depth;
    int log2_depth = depth >> 1;	/* works for 2, 4 */
    gx_color_index max_pixel = (1 << depth) - 1;
    int code;

#ifdef DEBUG
    if (gs_debug_c('b'))
	trace_copy_rop("mem_gray_strip_copy_rop",
		       dev, sdata, sourcex, sraster,
		       id, scolors, textures, tcolors,
		       x, y, width, height, phase_x, phase_y, lop);
#endif
    if (gx_device_has_color(dev) ||
	(lop & (lop_S_transparent | lop_T_transparent)) ||
	(scolors &&		/* must be (0,0) or (max,max) */
	 ((scolors[0] | scolors[1]) != 0) &&
	 ((scolors[0] & scolors[1]) != max_pixel)) ||
	(tcolors && (tcolors[0] != tcolors[1]))
	) {
	/* We can't fake it: do it the slow, painful way. */
	return mem_default_strip_copy_rop(dev, sdata, sourcex, sraster, id,
					  scolors, textures, tcolors,
					  x, y, width, height,
					  phase_x, phase_y, lop);
    }
    if (scolors) {		/* Must be a solid color: see above. */
	scolors2[0] = scolors2[1] = scolors[0] & 1;
	real_scolors = scolors2;
    }
    if (textures) {
	texture2 = *textures;
	texture2.size.x <<= log2_depth;
	texture2.rep_width <<= log2_depth;
	texture2.shift <<= log2_depth;
	texture2.rep_shift <<= log2_depth;
	real_texture = &texture2;
    }
    if (tcolors) {
	/* For polybit textures with colors other than */
	/* all 0s or all 1s, fabricate the data. */
	if (tcolors[0] != 0 && tcolors[0] != max_pixel) {
	    real_tcolors = 0;
	    *(byte *) & tdata = (byte) tcolors[0] << (8 - depth);
	    texture2.data = (byte *) & tdata;
	    texture2.raster = align_bitmap_mod;
	    texture2.size.x = texture2.rep_width = depth;
	    texture2.size.y = texture2.rep_height = 1;
	    texture2.id = gx_no_bitmap_id;
	    texture2.shift = texture2.rep_shift = 0;
	    real_texture = &texture2;
	} else {
	    tcolors2[0] = tcolors2[1] = tcolors[0] & 1;
	    real_tcolors = tcolors2;
	}
    }
    /*
     * mem_mono_strip_copy_rop may call fill_rectangle, copy_mono, or
     * strip_tile_rectangle for special cases.  Patch those procedures
     * temporarily so they will either do the right thing or return
     * an error.
     */
    {
	dev_proc_fill_rectangle((*fill_rectangle)) =
	    dev_proc(dev, fill_rectangle);
	dev_proc_copy_mono((*copy_mono)) =
	    dev_proc(dev, copy_mono);
	dev_proc_strip_tile_rectangle((*strip_tile_rectangle)) =
	    dev_proc(dev, strip_tile_rectangle);

	set_dev_proc(dev, fill_rectangle, mem_gray_rop_fill_rectangle);
	set_dev_proc(dev, copy_mono, mem_gray_rop_copy_mono);
	set_dev_proc(dev, strip_tile_rectangle,
		     mem_gray_rop_strip_tile_rectangle);
	dev->width <<= log2_depth;
	code = mem_mono_strip_copy_rop(dev, sdata,
				       (real_scolors == NULL ?
					sourcex << log2_depth : sourcex),
				       sraster, id, real_scolors,
				       real_texture, real_tcolors,
				       x << log2_depth, y,
				       width << log2_depth, height,
				       phase_x << log2_depth, phase_y, lop);
	set_dev_proc(dev, fill_rectangle, fill_rectangle);
	set_dev_proc(dev, copy_mono, copy_mono);
	set_dev_proc(dev, strip_tile_rectangle, strip_tile_rectangle);
	dev->width >>= log2_depth;
    }
    /* If we punted, use the general procedure. */
    if (code < 0)
	return mem_default_strip_copy_rop(dev, sdata, sourcex, sraster, id,
					  scolors, textures, tcolors,
					  x, y, width, height,
					  phase_x, phase_y, lop);
    return code;
}
