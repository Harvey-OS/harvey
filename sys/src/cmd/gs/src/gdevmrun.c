/* Copyright (C) 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevmrun.c,v 1.5 2003/08/21 14:55:14 igor Exp $ */
/* Run-length encoded memory device */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gdevmrun.h"

/*
 * NOTE: THIS CODE HAS NOT BEEN TESTED.  IF YOU WANT TO USE IT, PLEASE
 * TEST IT CAREFULLY AND REPORT ANY PROBLEMS.
 */

/*
 * Define the representation of each run.  We store runs in a doubly-linked
 * list.  Run 0 is a dummy end-of-line run; run 1 is a dummy start-of-line
 * run.  The dummy runs have length MAX_RUN_LENGTH to prevent merging.
 *
 * We limit the number of runs per line for two reasons: if there are many
 * runs, the run-length representation probably isn't buying us much; and
 * we need to allocate temporary space on the stack for the runs when we
 * expand a line to uncompressed form.
 */
typedef gx_color_index run_value;
typedef uint run_index;
#define RUN_INDEX_BITS 10	/* see above */
#define MAX_RUNS (1 << RUN_INDEX_BITS)
#define MAX_RUN_INDEX (MAX_RUNS - 1)
typedef uint run_length;
#define RUN_LENGTH_BITS (32 - 2 * RUN_INDEX_BITS)
#define MAX_RUN_LENGTH ((1 << RUN_LENGTH_BITS) - 1)
typedef struct run_s {
    run_value value;
    run_length length : RUN_LENGTH_BITS;
    run_index next : RUN_INDEX_BITS;
    run_index prev : RUN_INDEX_BITS; /* 0 iff free run */
} run;

/*
 * Define a pointer into a run list.
 * For speed, we keep both the index of and the pointer to the current run.
 */
typedef struct run_ptr_s {
    run *ptr;
    run_index index;		/* index of current run */
} run_ptr;
typedef struct const_run_ptr_s {
    const run *ptr;
    run_index index;		/* index of current run */
} const_run_ptr;

/* Accessors */
#define RP_LENGTH(rp) ((rp).ptr->length)
#define RP_VALUE(rp) ((rp).ptr->value)
#define RP_NEXT(rp) ((rp).ptr->next)
#define RP_PREV(rp) ((rp).ptr->prev)
#define RL_DATA(line) ((run *)((line) + 1))
#define CONST_RL_DATA(line) ((const run *)((line) + 1))
#define RDEV_LINE(rdev, y) ((run_line *)scan_line_base(&(rdev)->md, y))
/* Traversers */
#define RP_AT_START(rp) ((rp).index == 1)
#define RP_AT_END(rp) ((rp).index == 0)
#define RP_TO_START(rp, data)\
  ((rp).index = (data)[1].next,\
   (rp).ptr = (data) + (rp).index)
/* Note that RP_TO_NEXT and RP_TO_PREV allow rpn == rpc. */
#define RP_TO_NEXT(rpc, data, rpn)\
  ((rpn).ptr = (data) + ((rpn).index = RP_NEXT(rpc)))
#define RP_TO_PREV(rpc, data, rpp)\
  ((rpp).ptr = (data) + ((rpp).index = RP_PREV(rpc)))

/*
 * Define the state of a single scan line.
 *
 * We maintain the following invariant: if two adjacent runs have the
 * same value, the sum of their lengths is greater than MAX_RUN_LENGTH.
 * This may miss optimality by nearly a factor of 2, but it's far easier
 * to maintain than a true optimal representation.
 *
 * For speed in the common case where nothing other than white is ever stored,
 * we initially don't bother to construct the runs (or the free run list)
 * for a line at all.
 */
typedef struct run_line_s {
    gx_color_index zero;	/* device white if line not initialized, */
				/* gx_no_color_index if initialized */
    uint xcur;			/* x value at cursor position */
    run_ptr rpcur;		/* cursor */
    run_index free;		/* head of free list */
} run_line;

/* Insert/delete */
private void
rp_delete_next(run_ptr *prpc, run *data, run_line *line)
{
    run_ptr rpn, rpn2;

    RP_TO_NEXT(*prpc, data, rpn);
    RP_TO_NEXT(rpn, data, rpn2);
    RP_NEXT(*prpc) = rpn2.index;
    RP_PREV(rpn2) = prpc->index;
    RP_NEXT(rpn) = line->free;
    RP_PREV(rpn) = 0;
    line->free = rpn.index;
}
private int
rp_insert_next(run_ptr *prpc, run *data, run_line *line, run_ptr *prpn)
{
    run_index new = line->free;
    run *prnew = data + new;

    if (new == 0)
	return -1;
    RP_TO_NEXT(*prpc, data, *prpn);
    RP_NEXT(*prpc) = new;
    RP_PREV(*prpn) = new;
    line->free = prnew->next;
    prnew->prev = prpc->index;
    prnew->next = prpn->index;
    prpn->index = new;
    prpn->ptr = prnew;
    return 0;
}
private int
rp_insert_prev(run_ptr *prpc, run *data, run_line *line, run_ptr *prpp)
{
    run_index new = line->free;
    run *prnew = data + new;

    if (new == 0)
	return -1;
    RP_TO_PREV(*prpc, data, *prpp);
    RP_NEXT(*prpp) = new;
    RP_PREV(*prpc) = new;
    line->free = prnew->next;
    prnew->prev = prpp->index;
    prnew->next = prpc->index;
    prpp->index = new;
    prpp->ptr = prnew;
    return 0;
}

/* Define the run-oriented device procedures. */
private dev_proc_copy_mono(run_copy_mono);
private dev_proc_copy_color(run_copy_color);
private dev_proc_fill_rectangle(run_fill_rectangle);
private dev_proc_copy_alpha(run_copy_alpha);
private dev_proc_strip_tile_rectangle(run_strip_tile_rectangle);
private dev_proc_strip_copy_rop(run_strip_copy_rop);
private dev_proc_get_bits_rectangle(run_get_bits_rectangle);

/*
 * Convert a memory device to run-length form.  The mdev argument should be
 * const, but it isn't because we need to call gx_device_white.
 */
int
gdev_run_from_mem(gx_device_run *rdev, gx_device_memory *mdev)
{
    int runs_per_line =
	(bitmap_raster(mdev->width * mdev->color_info.depth) -
	 sizeof(run_line)) / sizeof(run);
    /*
     * We use the scan lines of the memory device for storing runs.  We need
     * ceil(width / MAX_RUN_LENGTH) runs to represent a line where all
     * elements have the same value, +2 for the start and end runs.
     */
    int min_runs = (mdev->width + (MAX_RUN_LENGTH - 1)) / MAX_RUN_LENGTH + 2;
    int i;
    gx_color_index white = gx_device_white((gx_device *)mdev);

    rdev->md = *mdev;
    if (runs_per_line > MAX_RUNS)
	runs_per_line = MAX_RUNS;
    if (runs_per_line < min_runs)
	return 0;		/* just use the memory device as-is */
    for (i = 0; i < mdev->height; ++i) {
	run_line *line = RDEV_LINE(rdev, i);

	line->zero = white;
    }
    rdev->runs_per_line = runs_per_line;
    rdev->umin = 0;
    rdev->umax1 = mdev->height;
    rdev->smin = mdev->height;
    rdev->smax1 = 0;
    /* Save and replace the representation-aware rendering procedures. */
#define REPLACE(proc, rproc)\
  (rdev->save_procs.proc = dev_proc(&rdev->md, proc),\
   set_dev_proc(&rdev->md, proc, rproc))
    REPLACE(copy_mono, run_copy_mono);
    REPLACE(copy_color, run_copy_color);
    REPLACE(fill_rectangle, run_fill_rectangle);
    REPLACE(copy_alpha, run_copy_alpha);
    REPLACE(strip_tile_rectangle, run_strip_tile_rectangle);
    REPLACE(strip_copy_rop, run_strip_copy_rop);
    REPLACE(get_bits_rectangle, run_get_bits_rectangle);
#undef REPLACE
    return 0;
}

/* Convert a scan line to expanded form in place. */
private int
run_expand(gx_device_run *rdev, int y)
{
    const run_line *line = RDEV_LINE(rdev, y);
    const run *const data = CONST_RL_DATA(line);
    const_run_ptr rp;
    int n, x, w;
#if RUN_LENGTH_BITS <= 8
    byte length[MAX_RUNS];
#else
# if RUN_LENGTH_BITS <= 16
    ushort length[MAX_RUNS];
# else
    uint length[MAX_RUNS];
# endif
#endif
    gx_color_index value[MAX_RUNS];

    if (line->zero != gx_no_color_index) {
	rdev->save_procs.fill_rectangle((gx_device *)&rdev->md,
					0, y, rdev->md.width, 1, line->zero);
	return 0;
    }
    /* Copy the runs into local storage to avoid stepping on our own toes. */
    for (n = 0, RP_TO_START(rp, data); !RP_AT_END(rp);
	 ++n, RP_TO_NEXT(rp, data, rp)
	) {
	length[n] = RP_LENGTH(rp);
	value[n] = RP_VALUE(rp);
    }
    for (x = 0, n = 0; x < rdev->md.width; x += w, ++n) {
	w = length[n];
	rdev->save_procs.fill_rectangle((gx_device *)&rdev->md,
					x, y, w, 1, value[n]);
    }
    return 0;
}

/*
 * Convert a range of scan lines to standard form.
 */
private int
run_standardize(gx_device_run *rdev, int y, int h)
{
    int ye, iy;

    fit_fill_y(&rdev->md, y, h);
    fit_fill_h(&rdev->md, y, h);
    ye = y + h;
    if (y < rdev->smin) {
	if (ye > rdev->smax1)
	    run_standardize(rdev, rdev->smax1, ye - rdev->smax1);
	if (ye < rdev->smin)
	    ye = rdev->smin;
	rdev->smin = y;
    } else if (ye > rdev->smax1) {
	if (y > rdev->smax1)
	    y = rdev->smax1;
	rdev->smax1 = ye;
    } else
	return 0;
    for (iy = y; iy < ye; ++iy)
	run_expand(rdev, iy);
    return 0;
}

/* Trampoline rendering procedures */
private int
run_copy_mono(gx_device * dev, const byte * data, int dx, int raster,
	      gx_bitmap_id id, int x, int y, int w, int h,
	      gx_color_index zero, gx_color_index one)
{
    gx_device_run *const rdev = (gx_device_run *)dev;

    run_standardize(rdev, y, h);
    return rdev->save_procs.copy_mono((gx_device *)&rdev->md,
				      data, dx, raster, id,
				      x, y, w, h, zero, one);
}
private int
run_copy_color(gx_device * dev, const byte * data,
	       int data_x, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h)
{
    gx_device_run *const rdev = (gx_device_run *)dev;

    run_standardize(rdev, y, h);
    return rdev->save_procs.copy_color((gx_device *)&rdev->md,
				       data, data_x, raster, id,
				       x, y, w, h);
}
private int
run_copy_alpha(gx_device * dev, const byte * data, int data_x, int raster,
	       gx_bitmap_id id, int x, int y, int w, int h,
	       gx_color_index color, int depth)
{
    gx_device_run *const rdev = (gx_device_run *)dev;

    run_standardize(rdev, y, h);
    return rdev->save_procs.copy_alpha((gx_device *)&rdev->md,
				       data, data_x, raster, id,
				       x, y, w, h, color, depth);
}
private int
run_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
   int x, int y, int w, int h, gx_color_index color0, gx_color_index color1,
				int px, int py)
{
    gx_device_run *const rdev = (gx_device_run *)dev;

    run_standardize(rdev, y, h);
    return rdev->save_procs.strip_tile_rectangle((gx_device *)&rdev->md,
						 tiles, x, y, w, h,
						 color0, color1, px, py);
}
private int
run_strip_copy_rop(gx_device * dev, const byte * sdata, int sourcex,
		   uint sraster, gx_bitmap_id id,
		   const gx_color_index * scolors,
		   const gx_strip_bitmap * textures,
		   const gx_color_index * tcolors,
		   int x, int y, int w, int h, int px, int py,
		   gs_logical_operation_t lop)
{
    gx_device_run *const rdev = (gx_device_run *)dev;

    run_standardize(rdev, y, h);
    return rdev->save_procs.strip_copy_rop((gx_device *)&rdev->md,
					   sdata, sourcex, sraster,
					   id, scolors, textures, tcolors,
					   x, y, w, h, px, py, lop);
}
private int
run_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
		       gs_get_bits_params_t * params, gs_int_rect **unread)
{
    gx_device_run *const rdev = (gx_device_run *)dev;

    run_standardize(rdev, prect->p.y, prect->q.y - prect->p.y);
    return rdev->save_procs.get_bits_rectangle((gx_device *)&rdev->md,
					       prect, params, unread);
}

/* Finish initializing a line.  This is a separate procedure only */
/* for readability. */
private void
run_line_initialize(gx_device_run *rdev, int y)
{
    run_line *line = RDEV_LINE(rdev, y);
    run *data = RL_DATA(line);
    int left = rdev->md.width;
    run_index index = 2;
    run *rcur;

    line->zero = gx_no_color_index;
    data[0].length = MAX_RUN_LENGTH;	/* see above */
    data[0].value = gx_no_color_index;	/* shouldn't matter */
    data[1].length = MAX_RUN_LENGTH;
    data[1].value = gx_no_color_index;
    data[1].next = 2;
    rcur = data + index;
    for (; left > 0; index++, rcur++, left -= MAX_RUN_LENGTH) {
	rcur->length = min(left, MAX_RUN_LENGTH);
	rcur->value = 0;
	rcur->prev = index - 1;
	rcur->next = index + 1;
    }
    rcur->next = 0;
    data[0].prev = index - 1;
    line->xcur = 0;
    line->rpcur.ptr = data + 2;
    line->rpcur.index = 2;
    line->free = index;
    for (; index < rdev->runs_per_line; ++index)
	data[index].next = index + 1;
    data[index - 1].next = 0;
    if (y >= rdev->umin && y < rdev->umax1) {
	if (y > (rdev->umin + rdev->umax1) >> 1)
	    rdev->umax1 = y;
	else
	    rdev->umin = y + 1;
    }
}

/*
 * Replace an interval of a line with a new value.  This is the procedure
 * that does all the interesting work.  We assume the line has been
 * initialized, and that 0 <= xo < xe <= dev->width.
 */
private int
run_fill_interval(run_line *line, int xo, int xe, run_value new)
{
    run *data = RL_DATA(line);
    int xc = line->xcur;
    run_ptr rpc;
    int x0, x1;
    run_ptr rp0;
    int code;

    rpc = line->rpcur;

    /* Find the run that contains xo. */

    if (xo < xc) {
	while (xo < xc)
	    RP_TO_PREV(rpc, data, rpc), xc -= RP_LENGTH(rpc);
    } else {
	while (xo >= xc + RP_LENGTH(rpc))
	    xc += RP_LENGTH(rpc), RP_TO_NEXT(rpc, data, rpc);
    }

    /*
     * Skip runs above xo that already contain the new value.
     * If the entire interval already has the correct value, exit.
     * If we skip any such runs, set xo to just above them.
     */

    for (; !RP_AT_END(rpc) && RP_VALUE(rpc) == new;
	 RP_TO_NEXT(rpc, data, rpc)
	)
	if ((xo = xc += RP_LENGTH(rpc)) >= xe)
	    return 0;
    x0 = xc, rp0 = rpc;

    /* Find the run that contains xe-1. */

    while (xe > xc + RP_LENGTH(rpc))
	xc += RP_LENGTH(rpc), RP_TO_NEXT(rpc, data, rpc);

    /*
     * Skip runs below xe that already contain the new value.
     * (We know that some run between xo and xe doesn't.)
     * If we skip any such runs, set xe to just below them.
     */

    while (RP_TO_PREV(rpc, data, rpc), RP_VALUE(rpc) == new)
	xe = xc -= RP_LENGTH(rpc);
    RP_TO_NEXT(rpc, data, rpc);

    /*
     * At this point, we know the following:
     *      x0 <= xo < x0 + RP_LENGTH(rp0).
     *      RP_VALUE(rp0) != new.
     *      xc <= xe-1 < xc + RP_LENGTH(rpc).
     *      RP_VALUE(rpc) != new.
     * Note that rp0 and rpc may point to the same run.
     */

    /* Split off any unaffected prefix of the run at rp0. */

    if (x0 < xo) {
	uint diff = xo - x0;
	run_value v0 = RP_VALUE(rp0);
	run_ptr rpp;

	RP_TO_PREV(rp0, data, rpp);
	if (RP_VALUE(rpp) == v0 && RP_LENGTH(rpp) + diff <= MAX_RUN_LENGTH)
	    RP_LENGTH(rpp) += diff;
	else {
	    code = rp_insert_prev(&rp0, data, line, &rpp);
	    if (code < 0)
		return code;
	    RP_LENGTH(rpp) = diff;
	    RP_VALUE(rpp) = v0;
	}
	RP_LENGTH(rp0) -= diff;
    }

    /* Split off any unaffected suffix of the run at rpc. */

    x1 = xc + RP_LENGTH(rpc);
    if (x1 > xe) {
	uint diff = x1 - xe;
	run_value vc = RP_VALUE(rpc);
	run_ptr rpn;

	RP_TO_NEXT(rpc, data, rpn);
	if (RP_VALUE(rpn) == vc && RP_LENGTH(rpn) + diff <= MAX_RUN_LENGTH)
	    RP_LENGTH(rpn) += diff;
	else {
	    code = rp_insert_next(&rpc, data, line, &rpn);
	    if (code < 0)
		return code;
	    RP_LENGTH(rpn) = diff;
	    RP_VALUE(rpn) = vc;
	}
	RP_LENGTH(rpc) -= diff;
    }

    /* Delete all runs from rp0 through rpc. */

    RP_TO_PREV(rp0, data, rp0);
    while (RP_NEXT(rp0) != RP_NEXT(rpc))
	rp_delete_next(&rp0, data, line);

    /*
     * Finally, insert new runs with the new value.
     * We need to check for one boundary case, namely,
     * xo == x0 and the next lower run has the new value.
     * (There's probably a way to structure the code just slightly
     * differently to avoid this test.)
     */

    {
	uint left = xe - xo;

	if (xo == x0 && RP_VALUE(rp0) == new &&
	    RP_LENGTH(rp0) + left <= MAX_RUN_LENGTH
	    )
	    RP_LENGTH(rp0) += left;
	else {
	    /*
	     * If we need more than one run, we divide up the length to
	     * create more runs with length less than MAX_RUN_LENGTH in
	     * order to improve the chances of a later merge.  However,
	     * we still guarantee that we won't create more runs than
	     * the minimum number required to represent the length.
	     */
	    run_length len;

	    if (left <= MAX_RUN_LENGTH)
		len = left;
	    else {
		/*len = ceil(left / ceil(left / MAX_RUN_LENGTH))*/
		int pieces = left + (MAX_RUN_LENGTH - 1) / MAX_RUN_LENGTH;

		len = (left + pieces - 1) / pieces;
	    }
	    do {
		run_ptr rpn;

		/*
		 * The allocation in rp_insert_next can't fail, because
		 * we just deleted at least as many runs as we're going
		 * to insert.
		 */
		rp_insert_next(&rp0, data, line, &rpn);
		RP_LENGTH(rpn) = min(left, len);
		RP_VALUE(rpn) = new;
	    }
	    while ((left -= len) > 0);
	}
    }

    return 0;
}

/* Replace a rectangle with a new value. */
private int
run_fill_rectangle(gx_device *dev, int x, int y, int w, int h,
		   gx_color_index color)
{
    gx_device_run *const rdev = (gx_device_run *)dev;
    int xe, ye;
    int iy;

    fit_fill(dev, x, y, w, h);
    ye = y + h;
    /*
     * If the new value is white and the rectangle falls entirely within
     * the uninitialized region that we're keeping track of,
     * we can skip the entire operation.
     */
    if (y >= rdev->umin && ye <= rdev->umax1 &&
	color == RDEV_LINE(rdev, y)->zero
	)
	return 0;

    /*
     * Hand off any parts of the operation that fall within the area
     * already in standard form.
     */
    if (y < rdev->smax1 && ye > rdev->smin) {
	/* Some part of the operation must be handed off. */
	if (y < rdev->smin) {
	    run_fill_rectangle(dev, x, y, w, rdev->smin - y, color);
	    y = rdev->smin;
	}
	/* Now rdev->smin <= y < ye. */
	rdev->save_procs.fill_rectangle((gx_device *)&rdev->md,
					x, y, w, min(ye, rdev->smax1) - y,
					color);
	if (ye <= rdev->smax1)
	    return 0;
	y = rdev->smax1;
    }
    xe = x + w;
    for (iy = y; iy < ye; ++iy) {
	run_line *line = RDEV_LINE(rdev, iy);

	if (color != line->zero) {
	    if (line->zero != gx_no_color_index)
		run_line_initialize(rdev, iy);
	    if (run_fill_interval(line, x, xe, color) < 0) {
		/* We ran out of runs.  Convert to expanded form. */
		run_standardize(rdev, iy, 1);
		rdev->save_procs.fill_rectangle((gx_device *)&rdev->md,
						x, iy, w, 1, color);
	    }
	}
    }
    return 0;
}

