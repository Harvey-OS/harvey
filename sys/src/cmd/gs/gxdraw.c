/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxdraw.c */
/* Primitive drawing routines for Ghostscript imaging library */
#include "math_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gxdraw.h"
#include "gzht.h"

/* Define the standard device color types. */
private struct_proc_enum_ptrs(dc_ht_binary_enum_ptrs);
private struct_proc_reloc_ptrs(dc_ht_binary_reloc_ptrs);
const gx_device_color_procs
  gx_dc_none =
    { gx_dc_no_load, gx_dc_no_fill_rectangle, 0, 0 },
  gx_dc_pure =
    { gx_dc_pure_load, gx_dc_pure_fill_rectangle, 0, 0 },
  gx_dc_ht_binary =
    { gx_dc_ht_binary_load, gx_dc_ht_binary_fill_rectangle,
      dc_ht_binary_enum_ptrs, dc_ht_binary_reloc_ptrs
    },
  gx_dc_ht_colored =
    { gx_dc_ht_colored_load, gx_dc_ht_colored_fill_rectangle, 0, 0 };
/* GC procedures */
#define cptr ((gx_device_color *)vptr)
private ENUM_PTRS_BEGIN(dc_ht_binary_enum_ptrs) return 0;
	case 0:
	{	gx_ht_tile *tile = cptr->colors.binary.b_tile;
		ENUM_RETURN(tile - tile->index);
	}
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(dc_ht_binary_reloc_ptrs) {
	uint index = cptr->colors.binary.b_tile->index;
	RELOC_TYPED_OFFSET_PTR(gx_device_color, colors.binary.b_tile, index);
} RELOC_PTRS_END
#undef cptr

/* Null color */
int
gx_dc_no_load(gx_device_color *pdevc, const gs_state *pgs)
{	return 0;
}
int
gx_dc_no_fill_rectangle(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	return 0;
}

/* Pure color */
int
gx_dc_pure_load(gx_device_color *pdevc, const gs_state *pgs)
{	return 0;
}
int
gx_dc_pure_fill_rectangle(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	return (*dev_proc(dev, fill_rectangle))(dev, x, y, w, h,
						pdevc->colors.pure);
}

/* Binary halftone */
int
gx_dc_ht_binary_fill_rectangle(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	const gx_ht_order *porder = &pgs->dev_ht->order;
	return (*dev_proc(dev, tile_rectangle))(dev,
				&pdevc->colors.binary.b_tile->tile,
				x, y, w, h, pdevc->colors.binary.color[0],
				pdevc->colors.binary.color[1],
				porder->phase.x, porder->phase.y);
}

/*
 * Auxiliary procedures for computing a * b / c and a * b % c
 * when a, b, and c are all non-negative,
 * b < c, and a * b exceeds (or might exceed) the capacity of a long.
 * It's really annoying that C doesn't provide any way to get at
 * the double-length multiply/divide instructions that
 * the machine undoubtedly provides....
 *
 * Note that these routines are exported for the benefit of gxfill.c.
 */

fixed
fixed_mult_quo(fixed a, fixed b, fixed c)
{	return (fixed)floor((double)a * b / c);
}
fixed
fixed_mult_rem(fixed a, fixed b, fixed c)
{	double prod = (double)a * b;
	return (fixed)(prod - floor(prod / c) * c);
}

/* Fill a trapezoid.  Requires: wt >= 0, wb >= 0, h >= 0. */
/* Note that the arguments are fixeds, not ints! */
/* This is derived from Paul Haeberli's floating point algorithm. */
typedef struct trap_line_s {
	int di; fixed df;		/* dx/dy ratio */
	fixed ldi, ldf;			/* increment per scan line */
	fixed x, xf;			/* current value */
} trap_line;
int
gx_fill_trapezoid_fixed(fixed fx0, fixed fw0, fixed fy0,
  fixed fx1, fixed fw1, fixed fh, bool swap_axes,
  const gx_device_color *pdevc, const gs_state *pgs)
{	const fixed ymin = fixed_rounded(fy0) + fixed_half;
	const fixed ymax = fixed_rounded(fy0 + fh);
	int iy = fixed2int_var(ymin);
	const int iy1 = fixed2int_var(ymax);
	if ( iy >= iy1 ) return 0;	/* no scan lines to sample */
   {	trap_line l, r;
	int rxl, rxr, ry;
	const fixed dxl = fx1 - fx0;
	const fixed dxr = dxl + fw1 - fw0;
	const fixed yline = ymin - fy0;	/* partial pixel offset to */
					/* first line to sample */
	int fill_direct = color_is_pure(pdevc);
	gx_color_index cindex;
	gx_device *dev;
	dev_proc_fill_rectangle((*fill_rect));
	int code;
	
	if_debug2('z', "[z]y=[%d,%d]\n", iy, iy1);

	if ( fill_direct )
	  cindex = pdevc->colors.pure,
	  dev = gs_currentdevice_inline(pgs),
	  fill_rect = dev_proc(dev, fill_rectangle);
	return_if_interrupt();
	r.x = (l.x = fx0 + fixed_half) + fw0;
	ry = iy;

	/* If the rounded X values are the same on both sides, */
	/* we can save ourselves a *lot* of work. */
	if ( fixed_floor(l.x) == fixed_rounded(fx1) &&
	     fixed_floor(r.x) == fixed_rounded(fx1 + fw1)
	   )
	{	rxl = fixed2int_var(l.x);
		rxr = fixed2int_var(r.x);
		iy = iy1;
		if_debug2('z', "[z]rectangle, x=[%d,%d]\n", rxl, rxr);
		goto last;
	}

#define fill_trap_rect(x,y,w,h)\
  (fill_direct ?\
    (swap_axes ? (*fill_rect)(dev, y, x, h, w, cindex) :\
     (*fill_rect)(dev, x, y, w, h, cindex)) :\
   swap_axes ? gx_fill_rectangle(y, x, h, w, pdevc, pgs) :\
   gx_fill_rectangle(x, y, w, h, pdevc, pgs))

	/* Compute the dx/dy ratios. */
	/* dx# = dx#i + (dx#f / fh). */
#define compute_dx(tl, d)\
  if ( d >= 0 )\
   { if ( d < fh ) tl.di = 0, tl.df = d;\
     else tl.di = (int)(d / fh), tl.df = d - tl.di * fh, tl.x += yline * tl.di;\
   }\
  else\
   { if ( (tl.df = d + fh) >= 0	/* d >= -fh */ ) tl.di = -1, tl.x -= yline;\
     else tl.di = (int)-((fh - 1 - d) / fh), tl.df = d - tl.di * fh, tl.x += yline * tl.di;\
   }

	/* Compute the x offsets at the first scan line to sample. */
	/* We need to be careful in computing yline * dx#f {/,%} fh */
	/* because the multiplication may overflow.  We know that */
	/* all the quantities involved are non-negative, and that */
	/* yline is less than 1 (as a fixed, of course); this gives us */
	/* a cheap conservative check for overflow in the multiplication. */
#define ymult_limit (max_fixed / fixed_1)
#define ymult_quo(yl, dxxf)\
  (dxxf < ymult_limit ? yl * dxxf / fh : fixed_mult_quo(yl, dxxf, fh))
#define ymult_rem(yl, dxxf)\
  (dxxf < ymult_limit ? yl * dxxf % fh : fixed_mult_rem(yl, dxxf, fh))

	/* It's worth checking for dxl == dxr, since this is the case */
	/* for parallelograms (including stroked lines). */
	compute_dx(l, dxl);
	if ( dxr == dxl )
	   {	fixed fx = ymult_quo(yline, l.df);
		l.x += fx;
		if ( l.di == 0 )
			r.di = 0, r.df = l.df;
		else			/* too hard to do adjustments right */
			compute_dx(r, dxr);
		r.x += fx;
	   }
	else
	   {	l.x += ymult_quo(yline, l.df);
		compute_dx(r, dxr);
		r.x += ymult_quo(yline, r.df);
	   }
	rxl = fixed2int_var(l.x);
	rxr = fixed2int_var(r.x);

	/* Compute one line's worth of dx/dy. */
	/* dx# * fixed_1 = ld#i + (ld#f / fh). */
	/* We don't have to bother with this if */
	/* we're only sampling a single scan line. */
	if ( iy1 - iy == 1 )
	   {	iy++;
		goto last;
	   }
#define compute_ldx(tl)\
  if ( tl.df < ymult_limit )\
    tl.ldi = int2fixed(tl.di) + int2fixed(tl.df) / fh,\
    tl.ldf = int2fixed(tl.df) % fh,\
    tl.xf = yline * tl.df % fh - fh;\
  else\
    tl.ldi = int2fixed(tl.di) + fixed_mult_quo(fixed_1, tl.df, fh),\
    tl.ldf = fixed_mult_rem(fixed_1, tl.df, fh),\
    tl.xf = fixed_mult_rem(yline, tl.df, fh) - fh
	compute_ldx(l);
	if ( dxr == dxl )
		r.ldi = l.ldi, r.ldf = l.ldf, r.xf = l.xf;
	else
	   {	compute_ldx(r);
	   }
#undef compute_ldx

	while ( ++iy != iy1 )
	   {	int ixl, ixr;
#define step_line(tl)\
  tl.x += tl.ldi;\
  if ( (tl.xf += tl.ldf) >= 0 ) tl.xf -= fh, tl.x++;
		step_line(l);
		step_line(r);
#undef step_line
		ixl = fixed2int_var(l.x);
		ixr = fixed2int_var(r.x);
		if ( ixl != rxl || ixr != rxr )
		   {	code = fill_trap_rect(rxl, ry, rxr - rxl, iy - ry);
			if ( code < 0 ) goto xit;
			rxl = ixl, rxr = ixr, ry = iy;
		   }	
	   }
last:	code = fill_trap_rect(rxl, ry, rxr - rxl, iy - ry);
xit:	if ( code < 0 && fill_direct )
		return_error(code);
	return_if_interrupt();
	return code;
   }
}

/* Fill a parallelogram whose points are p, p+a, p+b, and p+a+b. */
/* We should swap axes to get best accuracy, but we don't. */
int
gx_fill_pgram_fixed(fixed px, fixed py, fixed ax, fixed ay,
  fixed bx, fixed by, const gx_device_color *pdevc,
  const gs_state *pgs)
{	fixed t;
	fixed dy, qx, dx, wx, pax, qax;
	int code;
	/* Ensure ay >= 0, by >= 0. */
	if ( ay < 0 ) px += ax, py += ay, ax = -ax, ay = -ay;
	if ( by < 0 ) px += bx, py += by, bx = -bx, by = -by;
	qx = px + ax + bx;
	/* Make a special fast check for rectangles. */
	if ( (ay | bx) == 0 || (by | ax) == 0 )
	{	int rx = fixed2int_var_rounded(px);
		int ry = fixed2int_var_rounded(py);
		/* Exactly one of (ax,bx) and one of (ay,by) is non-zero. */
		int w = fixed2int_var_rounded(qx) - rx;
		if ( w < 0 ) rx += w, w = -w;
		return gx_fill_rectangle(rx, ry, w,
				fixed2int_rounded(py + ay + by) - ry,
				pdevc, pgs);
	}
	/* Not a rectangle.  Ensure ay <= by. */
#define swap(r, s) (t = r, r = s, s = t)
	if ( ay > by ) swap(ax, bx), swap(ay, by);
	/* Compute the distance from p to the point on the line (p, p+b) */
	/* whose Y coordinate is equal to ay. */
	dx = ( ((bx < 0 ? -bx : bx) | ay) < 1L << (size_of(fixed) * 4 - 1) ?
		bx * ay / by :
		(fixed)((double)bx * ay / by)
	     );
	if ( dx < ax ) pax = px + dx, qax = qx - ax, wx = ax - dx;
	else pax = px + ax, qax = qx - dx, wx = dx - ax;
#define rounded_same(p, a) /* fixed_rounded(p) == fixed_rounded(p + a) */\
  (fixed_fraction((p) + fixed_half) + (a) < fixed_1) /* know a >= 0 */
	if ( !rounded_same(py, ay) )
	   {	code = gx_fill_trapezoid_fixed(px, fixed_0, py, pax, wx, ay,
					       false, pdevc, pgs);
		if ( code < 0 ) return code;
	   }
	py += ay;
	dy = by - ay;
	if ( !rounded_same(py, dy) )
	   {	code = gx_fill_trapezoid_fixed(pax, wx, py, qax, wx, dy,
					       false, pdevc, pgs);
		if ( code < 0 ) return code;
	   }
	py += dy;
	if ( !rounded_same(py, ay) )
		return gx_fill_trapezoid_fixed(qax, wx, py, qx, fixed_0, ay,
					       false, pdevc, pgs);
#undef rounded_same
#undef swap
	return 0;
}

/* Default implementation of tile_rectangle */
int
gx_default_tile_rectangle(gx_device *dev, register const gx_tile_bitmap *tile,
  int x, int y, int w, int h, gx_color_index color0, gx_color_index color1,
  int px, int py)
{	/* Fill the rectangle in chunks */
	int width = tile->size.x;
	int height = tile->size.y;
	int raster = tile->raster;
	int rwidth = tile->rep_width;
	int irx = ((rwidth & (rwidth - 1)) == 0 ?	/* power of 2 */
		(x + px) & (rwidth - 1) :
		(x + px) % rwidth);
	int ry = (y + py) % tile->rep_height;
	int icw = width - irx;
	int ch = height - ry;
	byte *row = tile->data + ry * raster;
#define d_proc_mono dev_proc(dev, copy_mono)
	dev_proc_copy_mono((*proc_mono));
#define d_proc_color dev_proc(dev, copy_color)
	dev_proc_copy_color((*proc_color));
#define d_color_halftone\
		(color0 == gx_no_color_index && color1 == gx_no_color_index)
	int color_halftone;
#define get_color_info()\
  if ( (color_halftone = d_color_halftone) ) proc_color = d_proc_color;\
  else proc_mono = d_proc_mono
	int code;

#ifdef DEBUG
if ( gs_debug_c('t') )
   {	int ptx, pty;
	const byte *ptp = tile->data;
	dprintf3("[t]tile %dx%d raster=%d;",
		tile->size.x, tile->size.y, tile->raster);
	dprintf6(" x,y=%d,%d w,h=%d,%d p=%d,%d\n",
		x, y, w, h, px, py);
	for ( pty = 0; pty < tile->size.y; pty++ )
	   {	dprintf("   ");
		for ( ptx = 0; ptx < tile->raster; ptx++ )
			dprintf1("%3x", *ptp++);
	   }
	dputc('\n');
   }
#endif
/****** SHOULD ALSO PASS id IF COPYING A FULL TILE ******/
#define real_copy_tile(srcx, tx, ty, tw, th)\
  code =\
    (color_halftone ?\
     (*proc_color)(dev, row, srcx, raster, gx_no_bitmap_id, tx, ty, tw, th) :\
     (*proc_mono)(dev, row, srcx, raster, gx_no_bitmap_id, tx, ty, tw, th, color0, color1));\
  if ( code < 0 ) return_error(code);\
  return_if_interrupt()
#ifdef DEBUG
#define copy_tile(sx, tx, ty, tw, th)\
  if ( gs_debug_c('t') )\
	dprintf5("   copy sx=%d x=%d y=%d w=%d h=%d\n",\
		 sx, tx, ty, tw, th);\
  real_copy_tile(sx, tx, ty, tw, th)
#else
#define copy_tile(sx, tx, ty, tw, th)\
  real_copy_tile(sx, tx, ty, tw, th)
#endif
	if ( icw >= w )
	   {	/* Narrow operation */
		int ey, fey, cy;
		if ( ch >= h )
		   {	/* Just one (partial) tile to transfer. */
#define color_halftone d_color_halftone
#define proc_color d_proc_color
#define proc_mono d_proc_mono
			copy_tile(irx, x, y, w, h);
#undef proc_mono
#undef proc_color
#undef color_halftone
			return 0;
		   }
		get_color_info();
		ey = y + h;
		fey = ey - height;
		copy_tile(irx, x, y, w, ch);
		cy = y + ch;
		row = tile->data;
		do
		   {	ch = (cy > fey ? ey - cy : height);
			copy_tile(irx, x, cy, w, ch);
		   }
		while ( (cy += ch) < ey );
		return 0;
	   }
	get_color_info();
	if ( ch >= h )
	   {	/* Shallow operation */
		int ex = x + w;
		int fex = ex - width;
		int cx = x + icw;
		copy_tile(irx, x, y, icw, h);
		while ( cx <= fex )
		   {	copy_tile(0, cx, y, width, h);
			cx += width;
		   }
		if ( cx < ex )
		   {	copy_tile(0, cx, y, ex - cx, h);
		   }
	   }
	else
	   {	/* Full operation */
		int ex = x + w, ey = y + h;
		int fex = ex - width, fey = ey - height;
		int cx, cy;
		for ( cy = y; ; )
		   {	copy_tile(irx, x, cy, icw, ch);
			cx = x + icw;
			while ( cx <= fex )
			   {	copy_tile(0, cx, cy, width, ch);
				cx += width;
			   }
			if ( cx < ex )
			   {	copy_tile(0, cx, cy, ex - cx, ch);
			   }
			if ( (cy += ch) >= ey ) break;
			ch = (cy > fey ? ey - cy : height);
			row = tile->data;
		   }
	   }
#undef copy_tile
#undef real_copy_tile
	return 0;
}

/* Draw a one-pixel-wide line. */
int
gx_draw_line_fixed(fixed ixf, fixed iyf, fixed itoxf, fixed itoyf,
  const gx_device_color *pdevc, const gs_state *pgs)
{	int ix = fixed2int_var(ixf);
	int iy = fixed2int_var(iyf);
	int itox = fixed2int_var(itoxf);
	int itoy = fixed2int_var(itoyf);
	gx_device *dev;
	return_if_interrupt();
	if ( itoy == iy )		/* horizontal line */
	  { return (ix <= itox ?
		    gx_fill_rectangle(ix, iy, itox - ix + 1, 1, pdevc, pgs) :
		    gx_fill_rectangle(itox, iy, ix - itox + 1, 1, pdevc, pgs)
		    );
	  }
	if ( itox == ix )		/* vertical line */
	  { return (iy <= itoy ?
		    gx_fill_rectangle(ix, iy, 1, itoy - iy + 1, pdevc, pgs) :
		    gx_fill_rectangle(ix, itoy, 1, iy - itoy + 1, pdevc, pgs)
		    );
	  }
	if ( color_is_pure(pdevc) &&
	    (dev = gs_currentdevice_inline(pgs),
	     (*dev_proc(dev, draw_line))(dev, ix, iy, itox, itoy,
					 pdevc->colors.pure)) >= 0
	   )
	  return 0;
	{ fixed h = itoyf - iyf;
	  fixed w = itoxf - ixf;
	  fixed tf;
#define fswap(a, b) tf = a, a = b, b = tf
	  if ( (w < 0 ? -w : w) <= (h < 0 ? -h : h) )
	    { if ( h < 0 )
		fswap(ixf, itoxf), fswap(iyf, itoyf),
		h = -h;
	      return gx_fill_trapezoid_fixed(ixf - fixed_half, fixed_1, iyf,
					     itoxf - fixed_half, fixed_1, h,
					     false, pdevc, pgs);
	    }
	  else
	    { if ( w < 0 )
		fswap(ixf, itoxf), fswap(iyf, itoyf),
		w = -w;
	      return gx_fill_trapezoid_fixed(iyf - fixed_half, fixed_1, ixf,
					     itoyf - fixed_half, fixed_1, w,
					     true, pdevc, pgs);
	    }
#undef fswap
	}
}

/* A stub to force use of the standard procedure. */
int
gx_default_draw_line(gx_device *dev,
  int x0, int y0, int x1, int y1, gx_color_index color)
{	return -1;
}
