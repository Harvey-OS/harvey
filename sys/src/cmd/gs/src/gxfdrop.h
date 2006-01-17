/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfdrop.h,v 1.4 2003/11/25 08:34:07 igor Exp $ */
/* Dropout prevention for a character rasterization. */

#ifndef gxfdrop_INCLUDED
#  define gxfdrop_INCLUDED


/*  The structure margin_set and related structures and functions are used for 
    preventing dropouts rasterizing a character with zero fill adjustment. The purpose
    is to paint something along thin quazi-horizontal stems,
    which are composed of multiple small segments (such as a result of flattenpath).
    We call it "pseudo-rasterization".
    When fill adjustment takes place, this stuff is not required and is being skipped.

    To prevent dropouts in thin quazi-horizontal stems we look at raster
    through 1xN pixels window, where N is the width of the path bounding box.
    This window moves from bottom to top synchronousely with the motion of
    the filling loop, but its Y coordinate is always an integer plus one half
    (actually it moves convulsively).
    Through this window we can see an upper half of a pixel row,
    and lower half of the next pixel row. Painted spots are visible through
    this window as a set of "margins". To handle them we maintain
    a list of margin_s structures (each of which describes an interval
    to be painted), and array of "sections" (i-th section corresponds to
    half-integer X-coordinate Xi = bbox_left + i + 0.5, and stores fraction
    part of y-coordinate of intersection of the line x == Xi with margin
    boudaries, being visible through window (only extremal coordinates are stored 
    into a section)).
 
    The structure margin_set snaps what has been painted inside window.
    We handle 2 instances of margin_set : margin_set0 is being prepared and margin_set1 is
    being refinished. When the filling loop steps down over a pixel center,
    the refinished window is closed and released, the prapared window becomes
    the refinished one, and a new one starts to prepare.

    fixme:
    The current implementation is not optimised for very bold characters.
    We could encrease performance for them with maintaining
    a separate list of "exclusive" margins, which correspond
    to intervals painted with regular trapezoids and made with
    complete_margin. Using them we could skip access to 'sect' array elements
    inside "exclusive" margins, so that the number of small steps
    sensibly decreeses.

    fixme:
    Another optimization could be applied to the regular(old) trapezoid algorithm.
    Currently it breaks stems at any step of the Y cycle,
    generating big number of trapezoids.
    Perhaps we could store trapezoid vertices to active_line,
    and delay rendering a trapezoid until stem changes boundary segments.
    This also would make calls to the margin staff less frequent.

*/

/*
 * Configuration flags for the dropout prevention code.
 */
#define ADJUST_SERIF 1 /* See comments near occurances. */
#define CHECK_SPOT_CONTIGUITY 1 /* See comments near occurances. */

#ifndef active_line_DEFINED
#  define active_line_DEFINED
typedef struct active_line_s active_line;
#endif

#ifndef line_list_DEFINED
#  define line_list_DEFINED
typedef struct line_list_s line_list;
#endif

typedef struct margin_s
{   int ibeg, iend; /* Pixel indices of an interval to paint. */
    struct margin_s *prev, *next;
} margin;

typedef struct section_s
{   short y0, y1; /* Fraction part of y coordinates of intersections of the margin with line x==i + bbox_left */
#if ADJUST_SERIF && CHECK_SPOT_CONTIGUITY
    short x0, x1; /* Pixel coverage by X for checking the contiguity. */
#endif
} section;

typedef struct margin_set_s
{   fixed y; 
    margin *margin_list, *margin_touched;
    section *sect;
} margin_set;

extern_st(st_section);

/* vd_trace helpers. */
#define VD_SCALE 0.03
#define VD_RECT(x, y, w, h, c) vd_rect(int2fixed(x), int2fixed(y), int2fixed(x + w), int2fixed(y + h), 1, c)
#define VD_TRAP_COLOR RGB(0, 255, 255)
#define VD_MARG_COLOR RGB(255, 0, 0)

void init_section(section *sect, int i0, int i1);
void free_all_margins(line_list * ll);
int close_margins(gx_device * dev, line_list * ll, margin_set *ms);
int process_h_lists(line_list * ll, active_line * plp, active_line * flp, active_line * alp, fixed y0, fixed y1);
int margin_interior(line_list * ll, active_line * flp, active_line * alp, fixed y0, fixed y1);
int start_margin_set(gx_device * dev, line_list * ll, fixed y0);
int continue_margin_common(line_list * ll, margin_set * set, active_line * flp, active_line * alp, fixed y0, fixed y1);

#endif /* gxfdrop_INCLUDED */
