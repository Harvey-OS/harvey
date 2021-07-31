/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevddrw.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Default polygon and image drawing device procedures */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsrect.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxiparam.h"
#include "gxistate.h"

#define SWAP(a, b, t)\
  (t = a, a = b, b = t)

/* ---------------- Polygon and line drawing ---------------- */

/* Define the 'remainder' analogue of fixed_mult_quo. */
private fixed
fixed_mult_rem(fixed a, fixed b, fixed c)
{
    /* All kinds of truncation may happen here, but it's OK. */
    return a * b - fixed_mult_quo(a, b, c) * c;
}

/*
 * The trapezoid fill algorithm uses trap_line structures to keep track of
 * the left and right edges during the Bresenham loop.
 */
typedef struct trap_line_s {
	/*
	 * h is the y extent of the line (edge.end.y - edge.start.y).
	 * We know h > 0.
	 */
    fixed h;
	/*
	 * The dx/dy ratio for the line is di + df/h.
	 * (The quotient refers to the l.s.b. of di, not fixed_1.)
	 * We know 0 <= df < h.
	 */
    int di;
    fixed df;
	/*
	 * The current position within the scan line is x + xf/h + 1.
	 * (The 1 refers to the least significant bit of x, not fixed_1;
	 * similarly, the quotient refers to the l.s.b. of x.)
	 * We know -h <= xf < 0.
	 */
    fixed x, xf;
	/*
	 * We increment (x,xf) by (ldi,ldf) after each scan line.
	 * (ldi,ldf) is just (di,df) converted to fixed point.
	 * We know 0 <= ldf < h.
	 */
    fixed ldi, ldf;
} trap_line;

/*
 * Compute the di and df members of a trap_line structure.  The x extent
 * (edge.end.x - edge.start.x) is a parameter; the y extent (h member)
 * has already been set.  Also adjust x for the initial y.
 */
inline private void
compute_dx(trap_line *tl, fixed xd, fixed ys)
{
    fixed h = tl->h;
    int di;

    if (xd >= 0) {
	if (xd < h)
	    tl->di = 0, tl->df = xd;
	else {
	    tl->di = di = (int)(xd / h);
	    tl->df = xd - di * h;
	    tl->x += ys * di;
	}
    } else {
	if ((tl->df = xd + h) >= 0 /* xd >= -h */)
	    tl->di = -1, tl->x -= ys;
	else {
	    tl->di = di = (int)-((h - 1 - xd) / h);
	    tl->df = xd - di * h;
	    tl->x += ys * di;
	}
    }
}

#define YMULT_LIMIT (max_fixed / fixed_1)

/* Compute ldi, ldf, and xf similarly. */
inline private void
compute_ldx(trap_line *tl, fixed ys)
{
    int di = tl->di;
    fixed df = tl->df;
    fixed h = tl->h;

    if ( df < YMULT_LIMIT ) {
	 if ( df == 0 )		/* vertical edge, worth checking for */
	     tl->ldi = int2fixed(di), tl->ldf = 0, tl->xf = -h;
	 else {
	     tl->ldi = int2fixed(di) + int2fixed(df) / h;
	     tl->ldf = int2fixed(df) % h;
	     tl->xf =
		 (ys < fixed_1 ? ys * df % h : fixed_mult_rem(ys, df, h)) - h;
	 }
    }
    else {
	tl->ldi = int2fixed(di) + fixed_mult_quo(fixed_1, df, h);
	tl->ldf = fixed_mult_rem(fixed_1, df, h);
	tl->xf = fixed_mult_rem(ys, df, h) - h;
    }
}

/*
 * Fill a trapezoid.  left.start => left.end and right.start => right.end
 * define the sides; ybot and ytop define the top and bottom.  Requires:
 *      {left,right}->start.y <= ybot <= ytop <= {left,right}->end.y.
 * Lines where left.x >= right.x will not be drawn.  Thanks to Paul Haeberli
 * for an early floating point version of this algorithm.
 */
int
gx_default_fill_trapezoid(gx_device * dev, const gs_fixed_edge * left,
    const gs_fixed_edge * right, fixed ybot, fixed ytop, bool swap_axes,
    const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    const fixed ymin = fixed_pixround(ybot) + fixed_half;
    const fixed ymax = fixed_pixround(ytop);

    if (ymin >= ymax)
	return 0;		/* no scan lines to sample */
    {
	int iy = fixed2int_var(ymin);
	const int iy1 = fixed2int_var(ymax);
	trap_line l, r;
	int rxl, rxr, ry;
	const fixed
	    x0l = left->start.x, x1l = left->end.x, x0r = right->start.x,
	    x1r = right->end.x, dxl = x1l - x0l, dxr = x1r - x0r;
	const fixed	/* partial pixel offset to first line to sample */
	    ysl = ymin - left->start.y, ysr = ymin - right->start.y;
	fixed fxl;
	bool fill_direct = color_writes_pure(pdevc, lop);
	int max_rect_height = 1;  /* max height to do fill as rectangle */
	int code;

	if_debug2('z', "[z]y=[%d,%d]\n", iy, iy1);

	l.h = left->end.y - left->start.y;
	r.h = right->end.y - right->start.y;
	l.x = x0l + (fixed_half - fixed_epsilon);
	r.x = x0r + (fixed_half - fixed_epsilon);
	ry = iy;

/*
 * Free variables of FILL_TRAP_RECT:
 *	swap_axes, pdevc, dev, lop
 * Free variables of FILL_TRAP_RECT_DIRECT:
 *	swap_axes, fill_rect, dev, cindex
 */
#define FILL_TRAP_RECT(x,y,w,h)\
  (swap_axes ? gx_fill_rectangle_device_rop(y, x, h, w, pdevc, dev, lop) :\
   gx_fill_rectangle_device_rop(x, y, w, h, pdevc, dev, lop))
#define FILL_TRAP_RECT_DIRECT(x,y,w,h)\
  (swap_axes ? (*fill_rect)(dev, y, x, h, w, cindex) :\
   (*fill_rect)(dev, x, y, w, h, cindex))

	/* Compute the dx/dy ratios. */

	/*
	 * Compute the x offsets at the first scan line to sample.  We need
	 * to be careful in computing ys# * dx#f {/,%} h# because the
	 * multiplication may overflow.  We know that all the quantities
	 * involved are non-negative, and that ys# is usually less than 1 (as
	 * a fixed, of course); this gives us a cheap conservative check for
	 * overflow in the multiplication.
	 */
#define YMULT_QUO(ys, tl)\
  (ys < fixed_1 && tl.df < YMULT_LIMIT ? ys * tl.df / tl.h :\
   fixed_mult_quo(ys, tl.df, tl.h))

	/*
	 * It's worth checking for dxl == dxr, since this is the case
	 * for parallelograms (including stroked lines).
	 * Also check for left or right vertical edges.
	 */
	if (fixed_floor(l.x) == fixed_pixround(x1l)) {
	    /* Left edge is vertical, we don't need to increment. */
	    l.di = 0, l.df = 0;
	    fxl = 0;
	} else {
	    compute_dx(&l, dxl, ysl);
	    fxl = YMULT_QUO(ysl, l);
	    l.x += fxl;
	}
	if (fixed_floor(r.x) == fixed_pixround(x1r)) {
	    /* Right edge is vertical.  If both are vertical, */
	    /* we have a rectangle. */
	    if (l.di == 0 && l.df == 0)
		max_rect_height = max_int;
	    else
		r.di = 0, r.df = 0;
	}
	/*
	 * The test for fxl != 0 is required because the right edge might
	 * cross some pixel centers even if the left edge doesn't.
	 */
	else if (dxr == dxl && fxl != 0) {
	    if (l.di == 0)
		r.di = 0, r.df = l.df;
	    else		/* too hard to do adjustments right */
		compute_dx(&r, dxr, ysr);
	    if (ysr == ysl && r.h == l.h)
		r.x += fxl;
	    else
		r.x += YMULT_QUO(ysr, r);
	} else {
	    compute_dx(&r, dxr, ysr);
	    r.x += YMULT_QUO(ysr, r);
	}
	rxl = fixed2int_var(l.x);
	rxr = fixed2int_var(r.x);

	/*
	 * Take a shortcut if we're only sampling a single scan line,
	 * or if we have a rectangle.
	 */
	if (iy1 - iy <= max_rect_height) {
	    iy = iy1 - 1;
	    if_debug2('z', "[z]rectangle, x=[%d,%d]\n", rxl, rxr);
	} else {
	    /* Compute one line's worth of dx/dy. */
	    compute_ldx(&l, ysl);
	    if (dxr == dxl && ysr == ysl && r.h == l.h)
		r.ldi = l.ldi, r.ldf = l.ldf, r.xf = l.xf;
	    else
		compute_ldx(&r, ysr);
	}

#define STEP_LINE(ix, tl)\
  tl.x += tl.ldi;\
  if ( (tl.xf += tl.ldf) >= 0 ) tl.xf -= tl.h, tl.x++;\
  ix = fixed2int_var(tl.x)

	if (fill_direct) {
	    gx_color_index cindex = pdevc->colors.pure;
	    dev_proc_fill_rectangle((*fill_rect)) =
		dev_proc(dev, fill_rectangle);

	    while (++iy != iy1) {
		int ixl, ixr;

		STEP_LINE(ixl, l);
		STEP_LINE(ixr, r);
		if (ixl != rxl || ixr != rxr) {
		    code = FILL_TRAP_RECT_DIRECT(rxl, ry, rxr - rxl, iy - ry);
		    if (code < 0)
			goto xit;
		    rxl = ixl, rxr = ixr, ry = iy;
		}
	    }
	    code = FILL_TRAP_RECT_DIRECT(rxl, ry, rxr - rxl, iy - ry);
	} else {
	    while (++iy != iy1) {
		int ixl, ixr;

		STEP_LINE(ixl, l);
		STEP_LINE(ixr, r);
		if (ixl != rxl || ixr != rxr) {
		    code = FILL_TRAP_RECT(rxl, ry, rxr - rxl, iy - ry);
		    if (code < 0)
			goto xit;
		    rxl = ixl, rxr = ixr, ry = iy;
		}
	    }
	    code = FILL_TRAP_RECT(rxl, ry, rxr - rxl, iy - ry);
	}
#undef STEP_LINE
xit:	if (code < 0 && fill_direct)
	    return_error(code);
	return_if_interrupt();
	return code;
    }
}

/* Fill a parallelogram whose points are p, p+a, p+b, and p+a+b. */
/* We should swap axes to get best accuracy, but we don't. */
/* We must be very careful to follow the center-of-pixel rule in all cases. */
int
gx_default_fill_parallelogram(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		  const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    fixed t;
    fixed qx, qy, ym;
    dev_proc_fill_trapezoid((*fill_trapezoid));
    gs_fixed_edge left, right;
    int code;

    /* Make a special fast check for rectangles. */
    if (PARALLELOGRAM_IS_RECT(ax, ay, bx, by)) {
	gs_int_rect r;

	INT_RECT_FROM_PARALLELOGRAM(&r, px, py, ax, ay, bx, by);
	return gx_fill_rectangle_device_rop(r.p.x, r.p.y, r.q.x - r.p.x,
					    r.q.y - r.p.y, pdevc, dev, lop);
    }
    /*
     * Not a rectangle.  Ensure that the 'a' line is to the left of
     * the 'b' line.  Testing ax <= bx is neither sufficient nor
     * necessary: in general, we need to compare the slopes.
     */
    /* Ensure ay >= 0, by >= 0. */
    if (ay < 0)
	px += ax, py += ay, ax = -ax, ay = -ay;
    if (by < 0)
	px += bx, py += by, bx = -bx, by = -by;
    qx = px + ax + bx;
    if ((ax ^ bx) < 0) {	/* In this case, the test ax <= bx is sufficient. */
	if (ax > bx)
	    SWAP(ax, bx, t), SWAP(ay, by, t);
    } else {			/*
				 * Compare the slopes.  We know that ay >= 0, by >= 0,
				 * and ax and bx have the same sign; the lines are in the
				 * correct order iff
				 *          ay/ax >= by/bx, or
				 *          ay*bx >= by*ax
				 * Eventually we can probably find a better way to test this,
				 * without using floating point.
				 */
	if ((double)ay * bx < (double)by * ax)
	    SWAP(ax, bx, t), SWAP(ay, by, t);
    }
    fill_trapezoid = dev_proc(dev, fill_trapezoid);
    qy = py + ay + by;
    left.start.x = right.start.x = px;
    left.start.y = right.start.y = py;
    left.end.x = px + ax;
    left.end.y = py + ay;
    right.end.x = px + bx;
    right.end.y = py + by;
#define ROUNDED_SAME(p1, p2)\
  (fixed_pixround(p1) == fixed_pixround(p2))
    if (ay < by) {
	if (!ROUNDED_SAME(py, left.end.y)) {
	    code = (*fill_trapezoid) (dev, &left, &right, py, left.end.y,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	left.start = left.end;
	left.end.x = qx, left.end.y = qy;
	ym = right.end.y;
	if (!ROUNDED_SAME(left.start.y, ym)) {
	    code = (*fill_trapezoid) (dev, &left, &right, left.start.y, ym,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	right.start = right.end;
	right.end.x = qx, right.end.y = qy;
    } else {
	if (!ROUNDED_SAME(py, right.end.y)) {
	    code = (*fill_trapezoid) (dev, &left, &right, py, right.end.y,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	right.start = right.end;
	right.end.x = qx, right.end.y = qy;
	ym = left.end.y;
	if (!ROUNDED_SAME(right.start.y, ym)) {
	    code = (*fill_trapezoid) (dev, &left, &right, right.start.y, ym,
				      false, pdevc, lop);
	    if (code < 0)
		return code;
	}
	left.start = left.end;
	left.end.x = qx, left.end.y = qy;
    }
    if (!ROUNDED_SAME(ym, qy))
	return (*fill_trapezoid) (dev, &left, &right, ym, qy,
				  false, pdevc, lop);
    else
	return 0;
#undef ROUNDED_SAME
}

/* Fill a triangle whose points are p, p+a, and p+b. */
/* We should swap axes to get best accuracy, but we don't. */
int
gx_default_fill_triangle(gx_device * dev,
		 fixed px, fixed py, fixed ax, fixed ay, fixed bx, fixed by,
		  const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    fixed t;
    fixed ym;

    dev_proc_fill_trapezoid((*fill_trapezoid)) =
	dev_proc(dev, fill_trapezoid);
    gs_fixed_edge left, right;
    int code;

    /* Ensure ay >= 0, by >= 0. */
    if (ay < 0)
	px += ax, py += ay, bx -= ax, by -= ay, ax = -ax, ay = -ay;
    if (by < 0)
	px += bx, py += by, ax -= bx, ay -= by, bx = -bx, by = -by;
    /* Ensure ay <= by. */
    if (ay > by)
	SWAP(ax, bx, t), SWAP(ay, by, t);
    /*
     * Make a special check for a flat bottom or top,
     * which we can handle with a single call on fill_trapezoid.
     */
    left.start.x = right.start.x = px;
    left.start.y = right.start.y = py;
    if (ay == 0) {
	/* Flat top */
	if (ax < 0)
	    left.start.x = px + ax;
	else
	    right.start.x = px + ax;
	left.end.x = right.end.x = px + bx;
	left.end.y = right.end.y = py + by;
	ym = py;
    } else if (ay == by) {
	/* Flat bottom */
	if (ax < bx)
	    left.end.x = px + ax, right.end.x = px + bx;
	else
	    left.end.x = px + bx, right.end.x = px + ax;
	left.end.y = right.end.y = py + by;
	ym = py;
    } else {
	ym = py + ay;
	if (fixed_mult_quo(bx, ay, by) < ax) {
	    /* The 'b' line is to the left of the 'a' line. */
	    left.end.x = px + bx, left.end.y = py + by;
	    right.end.x = px + ax, right.end.y = py + ay;
	    code = (*fill_trapezoid) (dev, &left, &right, py, ym,
				      false, pdevc, lop);
	    right.start = right.end;
	    right.end = left.end;
	} else {
	    /* The 'a' line is to the left of the 'b' line. */
	    left.end.x = px + ax, left.end.y = py + ay;
	    right.end.x = px + bx, right.end.y = py + by;
	    code = (*fill_trapezoid) (dev, &left, &right, py, ym,
				      false, pdevc, lop);
	    left.start = left.end;
	    left.end = right.end;
	}
	if (code < 0)
	    return code;
    }
    return (*fill_trapezoid) (dev, &left, &right, ym, right.end.y,
			      false, pdevc, lop);
}

/* Draw a one-pixel-wide line. */
int
gx_default_draw_thin_line(gx_device * dev,
			  fixed fx0, fixed fy0, fixed fx1, fixed fy1,
		  const gx_device_color * pdevc, gs_logical_operation_t lop)
{
    int ix = fixed2int_var(fx0);
    int iy = fixed2int_var(fy0);
    int itox = fixed2int_var(fx1);
    int itoy = fixed2int_var(fy1);

    return_if_interrupt();
    if (itoy == iy) {		/* horizontal line */
	return (ix <= itox ?
		gx_fill_rectangle_device_rop(ix, iy, itox - ix + 1, 1,
					     pdevc, dev, lop) :
		gx_fill_rectangle_device_rop(itox, iy, ix - itox + 1, 1,
					     pdevc, dev, lop)
	    );
    }
    if (itox == ix) {		/* vertical line */
	return (iy <= itoy ?
		gx_fill_rectangle_device_rop(ix, iy, 1, itoy - iy + 1,
					     pdevc, dev, lop) :
		gx_fill_rectangle_device_rop(ix, itoy, 1, iy - itoy + 1,
					     pdevc, dev, lop)
	    );
    } {
	fixed h = fy1 - fy0;
	fixed w = fx1 - fx0;
	fixed tf;
	bool swap_axes;
	gs_fixed_edge left, right;

	if ((w < 0 ? -w : w) <= (h < 0 ? -h : h)) {
	    if (h < 0)
		SWAP(fx0, fx1, tf), SWAP(fy0, fy1, tf),
		    h = -h;
	    right.start.x = (left.start.x = fx0 - fixed_half) + fixed_1;
	    right.end.x = (left.end.x = fx1 - fixed_half) + fixed_1;
	    left.start.y = right.start.y = fy0;
	    left.end.y = right.end.y = fy1;
	    swap_axes = false;
	} else {
	    if (w < 0)
		SWAP(fx0, fx1, tf), SWAP(fy0, fy1, tf),
		    w = -w;
	    right.start.x = (left.start.x = fy0 - fixed_half) + fixed_1;
	    right.end.x = (left.end.x = fy1 - fixed_half) + fixed_1;
	    left.start.y = right.start.y = fx0;
	    left.end.y = right.end.y = fx1;
	    swap_axes = true;
	}
	return (*dev_proc(dev, fill_trapezoid)) (dev, &left, &right,
						 left.start.y, left.end.y,
						 swap_axes, pdevc, lop);
    }
}

/* Stub out the obsolete procedure. */
int
gx_default_draw_line(gx_device * dev,
		     int x0, int y0, int x1, int y1, gx_color_index color)
{
    return -1;
}

/* ---------------- Image drawing ---------------- */

/* GC structures for image enumerator */
public_st_gx_image_enum_common();

private 
ENUM_PTRS_WITH(image_enum_common_enum_ptrs, gx_image_enum_common_t *eptr)
    return 0;
case 0: return ENUM_OBJ(gx_device_enum_ptr(eptr->dev));
ENUM_PTRS_END

private RELOC_PTRS_WITH(image_enum_common_reloc_ptrs, gx_image_enum_common_t *eptr)
{
    eptr->dev = gx_device_reloc_ptr(eptr->dev, gcst);
}
RELOC_PTRS_END

/*
 * gx_default_begin_image is only invoked for ImageType 1 images.  However,
 * the argument types are different, and if the device provides a
 * begin_typed_image procedure, we should use it.  See gxdevice.h.
 */
private int
gx_no_begin_image(gx_device * dev,
		  const gs_imager_state * pis, const gs_image_t * pim,
		  gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		  gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    return -1;
}
int
gx_default_begin_image(gx_device * dev,
		       const gs_imager_state * pis, const gs_image_t * pim,
		       gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		       gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{
    /*
     * Hand off to begin_typed_image, being careful to avoid a
     * possible recursion loop.
     */
    dev_proc_begin_image((*save_begin_image)) = dev_proc(dev, begin_image);
    gs_image_t image;
    const gs_image_t *ptim;
    int code;

    set_dev_proc(dev, begin_image, gx_no_begin_image);
    if (pim->format == format)
	ptim = pim;
    else {
	image = *pim;
	image.format = format;
	ptim = &image;
    }
    code = (*dev_proc(dev, begin_typed_image))
	(dev, pis, NULL, (const gs_image_common_t *)ptim, prect, pdcolor,
	 pcpath, memory, pinfo);
    set_dev_proc(dev, begin_image, save_begin_image);
    return code;
}

int
gx_default_begin_typed_image(gx_device * dev,
			const gs_imager_state * pis, const gs_matrix * pmat,
		   const gs_image_common_t * pic, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		      gs_memory_t * memory, gx_image_enum_common_t ** pinfo)
{	/*
	 * If this is an ImageType 1 image using the imager's CTM,
	 * defer to begin_image.
	 */
    if (pic->type->begin_typed_image == gx_begin_image1) {
	const gs_image_t *pim = (const gs_image_t *)pic;

	if (pmat == 0 ||
	    (pis != 0 && !memcmp(pmat, &ctm_only(pis), sizeof(*pmat)))
	    ) {
	    int code = (*dev_proc(dev, begin_image))
	    (dev, pis, pim, pim->format, prect, pdcolor,
	     pcpath, memory, pinfo);

	    if (code >= 0)
		return code;
	}
    }
    return (*pic->type->begin_typed_image)
	(dev, pis, pmat, pic, prect, pdcolor, pcpath, memory, pinfo);
}

/* Backward compatibility for obsolete driver procedures. */

int
gx_default_image_data(gx_device *dev, gx_image_enum_common_t * info,
		      const byte ** plane_data,
		      int data_x, uint raster, int height)
{
    return gx_image_data(info, plane_data, data_x, raster, height);
}

int
gx_default_end_image(gx_device *dev, gx_image_enum_common_t * info,
		     bool draw_last)
{
    return gx_image_end(info, draw_last);
}
