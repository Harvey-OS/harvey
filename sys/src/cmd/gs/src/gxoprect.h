/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxoprect.h,v 1.2 2004/01/29 18:19:41 ray Exp $ */
/* geneic overprint fill rectangle interface */

#ifndef gxoprect_INCLUDED
#define gxoprect_INCLUDED

/*
 * Perform the fill rectangle operation for a non-separable color encoding
 * that requires overprint support.
 *
 * Returns 0 on success, < 0 in the event of an error.
 */
extern  int     gx_overprint_generic_fill_rectangle(
    gx_device *             tdev,
    gx_color_index          drawn_comps,
    int                     x,
    int                     y,
    int                     w,
    int                     h,
    gx_color_index          color,
    gs_memory_t *           mem );

/*
 * Perform the fill rectangle operation of a separable color encoding.
 * There are two versions of this routine: ..._1 for cases in which the
 * color depth is a divisor of 8 * sizeof(mono_fill_chunk), and ..._2 if
 * this is not the case (most typically if the depth == 24).
 *
 * For both cases, the color and retain_mask values passed to this
 * procedure are expected to be already swapped as required for a byte-
 * oriented bitmap. This consideration affects only little-endian
 * machines. For those machines, if depth > 9 the color passed to these
 * two procedures will not be the same as that passed to
 * gx_overprint_generic_fill_rectangle.
 *
 * Returns 0 on success, < 0 in the event of an error.
 */
extern  int     gx_overprint_sep_fill_rectangle_1(
    gx_device *             tdev,
    gx_color_index          retain_mask,    /* already swapped */
    int                     x,
    int                     y,
    int                     w,
    int                     h,
    gx_color_index          color,          /* already swapped */
    gs_memory_t *           mem );

extern  int     gx_overprint_sep_fill_rectangle_2(
    gx_device *             tdev,
    gx_color_index          retain_mask,    /* already swapped */
    int                     x,
    int                     y,
    int                     w,
    int                     h,
    gx_color_index          color,          /* already swapped */
    gs_memory_t *           mem );

#endif  /* gxoprect_INCLUDED */
