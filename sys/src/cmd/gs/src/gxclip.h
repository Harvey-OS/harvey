/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxclip.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Internal definitions for clipping */

#ifndef gxclip_INCLUDED
#  define gxclip_INCLUDED

/*
 * Both rectangle list and mask clipping use callback procedures to process
 * each rectangle selected by the clipping region.  They share both the
 * callback procedures themselves and the structure that provides closure
 * data for these procedures.  We define a single closure structure, rather
 * than one per client/callback, just to reduce source code clutter.  The
 * comments below show which clients use each member.
 */
typedef struct clip_callback_data_s {
    /*
     * The original driver procedure stores the following of its arguments
     * that the callback procedure or the clipping algorithm needs.
     */
    gx_device *tdev;		/* target device (always set) */
    int x, y, w, h;		/* (always set) */
    gx_color_index color[2];	/* (all but copy_color) */
    const byte *data;		/* copy_*, fill_mask */
    int sourcex;		/* ibid. */
    uint raster;		/* ibid. */
    int depth;			/* copy_alpha, fill_mask */
    const gx_drawing_color *pdcolor;	/* fill_mask */
    gs_logical_operation_t lop;	/* fill_mask, strip_copy_rop */
    const gx_clip_path *pcpath;	/* fill_mask */
    const gx_strip_bitmap *tiles;	/* strip_tile_rectangle */
    gs_int_point phase;		/* strip_* */
    const gx_color_index *scolors;	/* strip_copy_rop */
    const gx_strip_bitmap *textures;	/* ibid. */
    const gx_color_index *tcolors;	/* ibid. */
} clip_callback_data_t;

/* Declare the callback procedures. */
int
    clip_call_fill_rectangle(clip_callback_data_t * pccd,
			     int xc, int yc, int xec, int yec),
    clip_call_copy_mono(clip_callback_data_t * pccd,
			int xc, int yc, int xec, int yec),
    clip_call_copy_color(clip_callback_data_t * pccd,
			 int xc, int yc, int xec, int yec),
    clip_call_copy_alpha(clip_callback_data_t * pccd,
			 int xc, int yc, int xec, int yec),
    clip_call_fill_mask(clip_callback_data_t * pccd,
			int xc, int yc, int xec, int yec),
    clip_call_strip_tile_rectangle(clip_callback_data_t * pccd,
				   int xc, int yc, int xec, int yec),
    clip_call_strip_copy_rop(clip_callback_data_t * pccd,
			     int xc, int yc, int xec, int yec);

#endif /* gxclip_INCLUDED */
