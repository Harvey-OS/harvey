/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevdbit.c,v 1.11 2004/08/05 17:02:36 stefan Exp $ */
/* Default device bitmap copying implementation */
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsbittab.h"
#include "gsrect.h"
#include "gsropt.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gdevmem.h"
#undef mdev
#include "gxcpath.h"

/* By default, implement tile_rectangle using strip_tile_rectangle. */
int
gx_default_tile_rectangle(gx_device * dev, const gx_tile_bitmap * tile,
   int x, int y, int w, int h, gx_color_index color0, gx_color_index color1,
			  int px, int py)
{
    gx_strip_bitmap tiles;

    *(gx_tile_bitmap *) & tiles = *tile;
    tiles.shift = tiles.rep_shift = 0;
    return (*dev_proc(dev, strip_tile_rectangle))
	(dev, &tiles, x, y, w, h, color0, color1, px, py);
}

/* Implement copy_mono by filling lots of small rectangles. */
/* This is very inefficient, but it works as a default. */
int
gx_default_copy_mono(gx_device * dev, const byte * data,
	    int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h,
		     gx_color_index zero, gx_color_index one)
{
    bool invert;
    gx_color_index color;
    gx_device_color devc;

    fit_copy(dev, data, dx, raster, id, x, y, w, h);
    if (one != gx_no_color_index) {
	invert = false;
	color = one;
	if (zero != gx_no_color_index) {
	    int code = (*dev_proc(dev, fill_rectangle))
	    (dev, x, y, w, h, zero);

	    if (code < 0)
		return code;
	}
    } else {
	invert = true;
	color = zero;
    }
    set_nonclient_dev_color(&devc, color);
    return gx_dc_default_fill_masked
	(&devc, data, dx, raster, id, x, y, w, h, dev, rop3_T, invert);
}

/* Implement copy_color by filling lots of small rectangles. */
/* This is very inefficient, but it works as a default. */
int
gx_default_copy_color(gx_device * dev, const byte * data,
		      int dx, int raster, gx_bitmap_id id,
		      int x, int y, int w, int h)
{
    int depth = dev->color_info.depth;
    byte mask;

    dev_proc_fill_rectangle((*fill));
    const byte *row;
    int iy;

    if (depth == 1)
	return (*dev_proc(dev, copy_mono)) (dev, data, dx, raster, id,
					    x, y, w, h,
				    (gx_color_index) 0, (gx_color_index) 1);
    fit_copy(dev, data, dx, raster, id, x, y, w, h);
    fill = dev_proc(dev, fill_rectangle);
    mask = (byte) ((1 << depth) - 1);
    for (row = data, iy = 0; iy < h; row += raster, ++iy) {
	int ix;
	gx_color_index c0 = gx_no_color_index;
	const byte *ptr = row + ((dx * depth) >> 3);
	int i0;

	for (i0 = ix = 0; ix < w; ++ix) {
	    gx_color_index color;

	    if (depth >= 8) {
		color = *ptr++;
		switch (depth) {
		    case 64:
			color = (color << 8) + *ptr++;
		    case 56:
			color = (color << 8) + *ptr++;
		    case 48:
			color = (color << 8) + *ptr++;
		    case 40:
			color = (color << 8) + *ptr++;
		    case 32:
			color = (color << 8) + *ptr++;
		    case 24:
			color = (color << 8) + *ptr++;
		    case 16:
			color = (color << 8) + *ptr++;
		}
	    } else {
		uint dbit = (-(ix + dx + 1) * depth) & 7;

		color = (*ptr >> dbit) & mask;
		if (dbit == 0)
		    ptr++;
	    }
	    if (color != c0) {
		if (ix > i0) {
		    int code = (*fill)
		    (dev, i0 + x, iy + y, ix - i0, 1, c0);

		    if (code < 0)
			return code;
		}
		c0 = color;
		i0 = ix;
	    }
	}
	if (ix > i0) {
	    int code = (*fill) (dev, i0 + x, iy + y, ix - i0, 1, c0);

	    if (code < 0)
		return code;
	}
    }
    return 0;
}

int
gx_no_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
		 gx_color_index color, int depth)
{
    return_error(gs_error_unknownerror);
}

int
gx_default_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
		      gx_color_index color, int depth)
{				/* This might be called with depth = 1.... */
    if (depth == 1)
	return (*dev_proc(dev, copy_mono)) (dev, data, data_x, raster, id,
					    x, y, width, height,
					    gx_no_color_index, color);
    /*
     * Simulate alpha by weighted averaging of RGB values.
     * This is very slow, but functionally correct.
     */
    {
	const byte *row;
	gs_memory_t *mem = dev->memory;
	int bpp = dev->color_info.depth;
	int ncomps = dev->color_info.num_components;
	uint in_size = gx_device_raster(dev, false);
	byte *lin;
	uint out_size;
	byte *lout;
	int code = 0;
	gx_color_value color_cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
	int ry;

	fit_copy(dev, data, data_x, raster, id, x, y, width, height);
	row = data;
	out_size = bitmap_raster(width * bpp);
	lin = gs_alloc_bytes(mem, in_size, "copy_alpha(lin)");
	lout = gs_alloc_bytes(mem, out_size, "copy_alpha(lout)");
	if (lin == 0 || lout == 0) {
	    code = gs_note_error(gs_error_VMerror);
	    goto out;
	}
	(*dev_proc(dev, decode_color)) (dev, color, color_cv);
	for (ry = y; ry < y + height; row += raster, ++ry) {
	    byte *line;
	    int sx, rx;

	    DECLARE_LINE_ACCUM_COPY(lout, bpp, x);

	    code = (*dev_proc(dev, get_bits)) (dev, ry, lin, &line);
	    if (code < 0)
		break;
	    for (sx = data_x, rx = x; sx < data_x + width; ++sx, ++rx) {
		gx_color_index previous = gx_no_color_index;
		gx_color_index composite;
		int alpha2, alpha;

		if (depth == 2)	/* map 0 - 3 to 0 - 15 */
		    alpha = ((row[sx >> 2] >> ((3 - (sx & 3)) << 1)) & 3) * 5;
		else
		    alpha2 = row[sx >> 1],
			alpha = (sx & 1 ? alpha2 & 0xf : alpha2 >> 4);
	      blend:if (alpha == 15) {	/* Just write the new color. */
		    composite = color;
		} else {
		    if (previous == gx_no_color_index) {	/* Extract the old color. */
			if (bpp < 8) {
			    const uint bit = rx * bpp;
			    const byte *src = line + (bit >> 3);

			    previous =
				(*src >> (8 - ((bit & 7) + bpp))) &
				((1 << bpp) - 1);
			} else {
			    const byte *src = line + (rx * (bpp >> 3));

			    previous = 0;
			    switch (bpp >> 3) {
				case 8:
				    previous += (gx_color_index) * src++ 
					<< sample_bound_shift(previous, 56);
				case 7:
				    previous += (gx_color_index) * src++
					<< sample_bound_shift(previous, 48);
				case 6:
				    previous += (gx_color_index) * src++
					<< sample_bound_shift(previous, 40);
				case 5:
				    previous += (gx_color_index) * src++
					<< sample_bound_shift(previous, 32);
				case 4:
				    previous += (gx_color_index) * src++ << 24;
				case 3:
				    previous += (gx_color_index) * src++ << 16;
				case 2:
				    previous += (gx_color_index) * src++ << 8;
				case 1:
				    previous += *src++;
			    }
			}
		    }
		    if (alpha == 0) {	/* Just write the old color. */
			composite = previous;
		    } else {	/* Blend values. */
			gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
			int i;

			(*dev_proc(dev, decode_color)) (dev, previous, cv);
#if arch_ints_are_short
#  define b_int long
#else
#  define b_int int
#endif
#define make_shade(old, clr, alpha, amax) \
  (old) + (((b_int)(clr) - (b_int)(old)) * (alpha) / (amax))
			for (i=0; i<ncomps; i++)
			    cv[i] = make_shade(cv[i], color_cv[i], alpha, 15);
#undef b_int
#undef make_shade
			composite =
			    (*dev_proc(dev, encode_color)) (dev, cv);
			if (composite == gx_no_color_index) {	/* The device can't represent this color. */
			    /* Move the alpha value towards 0 or 1. */
			    if (alpha == 7)	/* move 1/2 towards 1 */
				++alpha;
			    alpha = (alpha & 8) | (alpha >> 1);
			    goto blend;
			}
		    }
		}
		LINE_ACCUM(composite, bpp);
	    }
	    LINE_ACCUM_COPY(dev, lout, bpp, x, rx, raster, ry);
	}
      out:gs_free_object(mem, lout, "copy_alpha(lout)");
	gs_free_object(mem, lin, "copy_alpha(lin)");
	return code;
    }
}

int
gx_no_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
	       const gx_color_index * scolors,
	     const gx_tile_bitmap * texture, const gx_color_index * tcolors,
	       int x, int y, int width, int height,
	       int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}

int
gx_default_fill_mask(gx_device * orig_dev,
		     const byte * data, int dx, int raster, gx_bitmap_id id,
		     int x, int y, int w, int h,
		     const gx_drawing_color * pdcolor, int depth,
		     gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device *dev;
    gx_device_clip cdev;

    if (pcpath != 0) {
	gx_make_clip_path_device(&cdev, pcpath);
	cdev.target = orig_dev;
	dev = (gx_device *) & cdev;
	(*dev_proc(dev, open_device)) (dev);
    } else
	dev = orig_dev;
    if (depth > 1) {
	/****** CAN'T DO ROP OR HALFTONE WITH ALPHA ******/
	return (*dev_proc(dev, copy_alpha))
	    (dev, data, dx, raster, id, x, y, w, h,
	     gx_dc_pure_color(pdcolor), depth);
    } else
        return pdcolor->type->fill_masked(pdcolor, data, dx, raster, id,
				          x, y, w, h, dev, lop, false);
}

/* Default implementation of strip_tile_rectangle */
int
gx_default_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
   int x, int y, int w, int h, gx_color_index color0, gx_color_index color1,
				int px, int py)
{				/* Fill the rectangle in chunks. */
    int width = tiles->size.x;
    int height = tiles->size.y;
    int raster = tiles->raster;
    int rwidth = tiles->rep_width;
    int rheight = tiles->rep_height;
    int shift = tiles->shift;
    gs_id tile_id = tiles->id;

    fit_fill_xy(dev, x, y, w, h);

#ifdef DEBUG
    if (gs_debug_c('t')) {
	int ptx, pty;
	const byte *ptp = tiles->data;

	dlprintf4("[t]tile %dx%d raster=%d id=%lu;",
		  tiles->size.x, tiles->size.y, tiles->raster, tiles->id);
	dlprintf6(" x,y=%d,%d w,h=%d,%d p=%d,%d\n",
		  x, y, w, h, px, py);
	dlputs("");
	for (pty = 0; pty < tiles->size.y; pty++) {
	    dprintf("   ");
	    for (ptx = 0; ptx < tiles->raster; ptx++)
		dprintf1("%3x", *ptp++);
	}
	dputc('\n');
    }
#endif

    if (dev_proc(dev, tile_rectangle) != gx_default_tile_rectangle) {
	if (shift == 0) {	/*
				 * Temporarily patch the tile_rectangle procedure in the
				 * device so we don't get into a recursion loop if the
				 * device has a tile_rectangle procedure that conditionally
				 * calls the strip_tile_rectangle procedure.
				 */
	    dev_proc_tile_rectangle((*tile_proc)) =
		dev_proc(dev, tile_rectangle);
	    int code;

	    set_dev_proc(dev, tile_rectangle, gx_default_tile_rectangle);
	    code = (*tile_proc)
		(dev, (const gx_tile_bitmap *)tiles, x, y, w, h,
		 color0, color1, px, py);
	    set_dev_proc(dev, tile_rectangle, tile_proc);
	    return code;
	}
	/* We should probably optimize this case too, for the benefit */
	/* of window systems, but we don't yet. */
    } {				/*
				 * Note: we can't do the following computations until after
				 * the fit_fill_xy.
				 */
	int xoff =
	(shift == 0 ? px :
	 px + (y + py) / rheight * tiles->rep_shift);
	int irx = ((rwidth & (rwidth - 1)) == 0 ?	/* power of 2 */
		   (x + xoff) & (rwidth - 1) :
		   (x + xoff) % rwidth);
	int ry = ((rheight & (rheight - 1)) == 0 ?	/* power of 2 */
		  (y + py) & (rheight - 1) :
		  (y + py) % rheight);
	int icw = width - irx;
	int ch = height - ry;
	byte *row = tiles->data + ry * raster;

	dev_proc_copy_mono((*proc_mono));
	dev_proc_copy_color((*proc_color));
	int code;

	if (color0 == gx_no_color_index && color1 == gx_no_color_index)
	    proc_color = dev_proc(dev, copy_color), proc_mono = 0;
	else
	    proc_color = 0, proc_mono = dev_proc(dev, copy_mono);

#define real_copy_tile(srcx, tx, ty, tw, th, id)\
  code =\
    (proc_color != 0 ?\
     (*proc_color)(dev, row, srcx, raster, id, tx, ty, tw, th) :\
     (*proc_mono)(dev, row, srcx, raster, id, tx, ty, tw, th, color0, color1));\
  if (code < 0) return_error(code);\
  return_if_interrupt(dev->memory)
#ifdef DEBUG
#define copy_tile(srcx, tx, ty, tw, th, tid)\
  if_debug6('t', "   copy id=%lu sx=%d => x=%d y=%d w=%d h=%d\n",\
	    tid, srcx, tx, ty, tw, th);\
  real_copy_tile(srcx, tx, ty, tw, th, tid)
#else
#define copy_tile(srcx, tx, ty, tw, th, id)\
  real_copy_tile(srcx, tx, ty, tw, th, id)
#endif
	if (ch >= h) {		/* Shallow operation */
	    if (icw >= w) {	/* Just one (partial) tile to transfer. */
		copy_tile(irx, x, y, w, h,
			  (w == width && h == height ? tile_id :
			   gs_no_bitmap_id));
	    } else {
		int ex = x + w;
		int fex = ex - width;
		int cx = x + icw;
		ulong id = (h == height ? tile_id : gs_no_bitmap_id);

		copy_tile(irx, x, y, icw, h, gs_no_bitmap_id);
		while (cx <= fex) {
		    copy_tile(0, cx, y, width, h, id);
		    cx += width;
		}
		if (cx < ex) {
		    copy_tile(0, cx, y, ex - cx, h, gs_no_bitmap_id);
		}
	    }
	} else if (icw >= w && shift == 0) {
	    /* Narrow operation, no shift */
	    int ey = y + h;
	    int fey = ey - height;
	    int cy = y + ch;
	    ulong id = (w == width ? tile_id : gs_no_bitmap_id);

	    copy_tile(irx, x, y, w, ch, (ch == height ? id : gs_no_bitmap_id));
	    row = tiles->data;
	    do {
		ch = (cy > fey ? ey - cy : height);
		copy_tile(irx, x, cy, w, ch,
			  (ch == height ? id : gs_no_bitmap_id));
	    }
	    while ((cy += ch) < ey);
	} else {
	    /* Full operation.  If shift != 0, some scan lines */
	    /* may be narrow.  We could test shift == 0 in advance */
	    /* and use a slightly faster loop, but right now */
	    /* we don't bother. */
	    int ex = x + w, ey = y + h;
	    int fex = ex - width, fey = ey - height;
	    int cx, cy;

	    for (cy = y;;) {
		ulong id = (ch == height ? tile_id : gs_no_bitmap_id);

		if (icw >= w) {
		    copy_tile(irx, x, cy, w, ch,
			      (w == width ? id : gs_no_bitmap_id));
		} else {
		    copy_tile(irx, x, cy, icw, ch, gs_no_bitmap_id);
		    cx = x + icw;
		    while (cx <= fex) {
			copy_tile(0, cx, cy, width, ch, id);
			cx += width;
		    }
		    if (cx < ex) {
			copy_tile(0, cx, cy, ex - cx, ch, gs_no_bitmap_id);
		    }
		}
		if ((cy += ch) >= ey)
		    break;
		ch = (cy > fey ? ey - cy : height);
		if ((irx += shift) >= rwidth)
		    irx -= rwidth;
		icw = width - irx;
		row = tiles->data;
	    }
	}
#undef copy_tile
#undef real_copy_tile
    }
    return 0;
}

int
gx_no_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
		     const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
		     int x, int y, int width, int height,
		     int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}

/* ---------------- Unaligned copy operations ---------------- */

/*
 * Implementing unaligned operations in terms of the standard aligned
 * operations requires adjusting the bitmap origin and/or the raster to be
 * aligned.  Adjusting the origin is simple; adjusting the raster requires
 * doing the operation one scan line at a time.
 */
int
gx_copy_mono_unaligned(gx_device * dev, const byte * data,
	    int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h,
		       gx_color_index zero, gx_color_index one)
{
    dev_proc_copy_mono((*copy_mono)) = dev_proc(dev, copy_mono);
    uint offset = ALIGNMENT_MOD(data, align_bitmap_mod);
    int step = raster & (align_bitmap_mod - 1);

    /* Adjust the origin. */
    data -= offset;
    dx += offset << 3;

    /* Adjust the raster. */
    if (!step) {		/* No adjustment needed. */
	return (*copy_mono) (dev, data, dx, raster, id,
			     x, y, w, h, zero, one);
    }
    /* Do the transfer one scan line at a time. */
    {
	const byte *p = data;
	int d = dx;
	int code = 0;
	int i;

	for (i = 0; i < h && code >= 0;
	     ++i, p += raster - step, d += step << 3
	    )
	    code = (*copy_mono) (dev, p, d, raster, gx_no_bitmap_id,
				 x, y + i, w, 1, zero, one);
	return code;
    }
}

int
gx_copy_color_unaligned(gx_device * dev, const byte * data,
			int data_x, int raster, gx_bitmap_id id,
			int x, int y, int width, int height)
{
    dev_proc_copy_color((*copy_color)) = dev_proc(dev, copy_color);
    int depth = dev->color_info.depth;
    uint offset = (uint) (data - (const byte *)0) & (align_bitmap_mod - 1);
    int step = raster & (align_bitmap_mod - 1);

    /*
     * Adjust the origin.
     * We have to do something very special for 24-bit data,
     * because that is the only depth that doesn't divide
     * align_bitmap_mod exactly.  In particular, we need to find
     * M*B + R == 0 mod 3, where M is align_bitmap_mod, R is the
     * offset value just calculated, and B is an integer unknown;
     * the new value of offset will be M*B + R.
     */
    if (depth == 24)
	offset += (offset % 3) *
	    (align_bitmap_mod * (3 - (align_bitmap_mod % 3)));
    data -= offset;
    data_x += (offset << 3) / depth;

    /* Adjust the raster. */
    if (!step) {		/* No adjustment needed. */
	return (*copy_color) (dev, data, data_x, raster, id,
			      x, y, width, height);
    }
    /* Do the transfer one scan line at a time. */
    {
	const byte *p = data;
	int d = data_x;
	int dstep = (step << 3) / depth;
	int code = 0;
	int i;

	for (i = 0; i < height && code >= 0;
	     ++i, p += raster - step, d += dstep
	    )
	    code = (*copy_color) (dev, p, d, raster, gx_no_bitmap_id,
				  x, y + i, width, 1);
	return code;
    }
}

int
gx_copy_alpha_unaligned(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
			gx_color_index color, int depth)
{
    dev_proc_copy_alpha((*copy_alpha)) = dev_proc(dev, copy_alpha);
    uint offset = (uint) (data - (const byte *)0) & (align_bitmap_mod - 1);
    int step = raster & (align_bitmap_mod - 1);

    /* Adjust the origin. */
    data -= offset;
    data_x += (offset << 3) / depth;

    /* Adjust the raster. */
    if (!step) {		/* No adjustment needed. */
	return (*copy_alpha) (dev, data, data_x, raster, id,
			      x, y, width, height, color, depth);
    }
    /* Do the transfer one scan line at a time. */
    {
	const byte *p = data;
	int d = data_x;
	int dstep = (step << 3) / depth;
	int code = 0;
	int i;

	for (i = 0; i < height && code >= 0;
	     ++i, p += raster - step, d += dstep
	    )
	    code = (*copy_alpha) (dev, p, d, raster, gx_no_bitmap_id,
				  x, y + i, width, 1, color, depth);
	return code;
    }
}
