/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1989, 1992, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsimpath.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
/* Image to outline conversion for Ghostscript library */
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gspaint.h" /* for gs_imagepath prototype */
#include "gsstate.h"
#include "gspath.h"

/* Define the state of the conversion process. */
typedef struct {
	/* The following are set at the beginning of the conversion. */
	gs_state *pgs;
	const byte *data; /* image data */
	int width, height, raster;
	/* The following are updated dynamically. */
	int dx, dy; /* X/Y increment of current run */
	int count;  /* # of steps in current run */
} status;

/* Define the scaling for the path tracer. */
/* It must be even. */
#define outline_scale 4
/* Define the length of the short strokes for turning corners. */
#define step 1

/* Forward declarations */
private
int get_pixel(const status *, int, int);
private
int trace_from(status *, int, int, int);
private
int add_dxdy(status *, int, int, int);

#define add_deltas(s, dx, dy, n)                \
	if((code = add_dxdy(s, dx, dy, n)) < 0) \
	return code
/* Append an outline derived from an image to the current path. */
int
gs_imagepath(gs_state *pgs, int width, int height, const byte *data)
{
	status stat;
	status *out = &stat;
	int code, x, y;

	/* Initialize the state. */
	stat.pgs = pgs;
	stat.data = data;
	stat.width = width;
	stat.height = height;
	stat.raster = (width + 7) / 8;
	/* Trace the cells to form an outline.  The trace goes in clockwise */
	/* order, always starting by going west along a bottom edge. */
	for(y = height - 1; y >= 0; y--)
		for(x = width - 1; x >= 0; x--) {
			if(get_pixel(out, x, y) && !get_pixel(out, x, y - 1) &&
			   (!get_pixel(out, x + 1, y) || get_pixel(out, x + 1, y - 1)) &&
			   !trace_from(out, x, y, 1)) { /* Found a starting point */
				stat.count = 0;
				stat.dx = stat.dy = 0;
				if((code = trace_from(out, x, y, 0)) < 0)
					return code;
				add_deltas(out, 0, 0, 1); /* force out last segment */
				if((code = gs_closepath(pgs)) < 0)
					return code;
			}
		}
	return 0;
}

/* Get a pixel from the data.  Return 0 if outside the image. */
private
int
get_pixel(register const status *out, int x, int y)
{
	if(x < 0 || x >= out->width || y < 0 || y >= out->height)
		return 0;
	return (out->data[y * out->raster + (x >> 3)] >> (~x & 7)) & 1;
}

/* Trace a path.  If detect is true, don't draw, just return 1 if we ever */
/* encounter a starting point whose x,y follows that of the initial point */
/* in x-then-y scan order; if detect is false, actually draw the outline. */
private
int
trace_from(register status *out, int x0, int y0, int detect)
{
	int x = x0, y = y0;
	int dx = -1, dy = 0; /* initially going west */
	int part = 0;	/* how far along edge we are; */

	/* initialized only to pacify gcc */
	int code;

	if(!detect) {
		part = (get_pixel(out, x + 1, y - 1) ? outline_scale - step : step);
		code = gs_moveto(out->pgs,
				 x + 1 - part / (float)outline_scale,
				 (float)y);
		if(code < 0)
			return code;
	}
	while(1) { /* Relative to the current direction, */
		/* -dy,dx is at +90 degrees (counter-clockwise); */
		/* tx,ty is at +45 degrees; */
		/* ty,-tx is at -45 degrees (clockwise); */
		/* dy,-dx is at -90 degrees. */
		int tx = dx - dy, ty = dy + dx;

		if(get_pixel(out, x + tx, y + ty)) { /* Cell at 45 degrees is full, */
			/* go counter-clockwise. */
			if(!detect) { /* If this is a 90 degree corner set at a */
				/* 45 degree angle, avoid backtracking. */
				if(out->dx == ty && out->dy == -tx) {
#define half_scale (outline_scale / 2 - step)
					out->count -= half_scale;
					add_deltas(out, tx, ty, outline_scale / 2);
#undef half_scale
				} else {
					add_deltas(out, dx, dy, step - part);
					add_deltas(out, tx, ty, outline_scale - step);
				}
				part = outline_scale - step;
			}
			x += tx, y += ty;
			dx = -dy, dy += tx;
		} else if(!get_pixel(out, x + dx, y + dy)) { /* Cell straight ahead is empty, go clockwise. */
			if(!detect) {
				add_deltas(out, dx, dy, outline_scale - step - part);
				add_deltas(out, ty, -tx, step);
				part = step;
			}
			dx = dy, dy -= ty;
		} else { /* Neither of the above, go in same direction. */
			if(!detect) {
				add_deltas(out, dx, dy, outline_scale);
			}
			x += dx, y += dy;
		}
		if(dx == -step && dy == 0 && !(tx == -step && ty == -step)) { /* We just turned a corner and are going west, */
			/* so the previous pixel is a starting point pixel. */
			if(x == x0 && y == y0)
				return 0;
			if(detect && (y > y0 || (y == y0 && x > x0)))
				return 1;
		}
	}
}

/* Add a (dx, dy) pair to the path being formed. */
/* Accumulate successive segments in the same direction. */
private
int
add_dxdy(register status *out, int dx, int dy, int count)
{
	if(count != 0) {
		if(dx == out->dx && dy == out->dy)
			out->count += count;
		else {
			if(out->count != 0) {
				int code = gs_rlineto(out->pgs,
						      out->dx * out->count / (float)outline_scale,
						      out->dy * out->count / (float)outline_scale);

				if(code < 0)
					return code;
			}
			out->dx = dx, out->dy = dy;
			out->count = count;
		}
	}
	return 0;
}
