/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxclip.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
    clip_call_fill_rectangle(P5(clip_callback_data_t * pccd,
				int xc, int yc, int xec, int yec)),
    clip_call_copy_mono(P5(clip_callback_data_t * pccd,
			   int xc, int yc, int xec, int yec)),
    clip_call_copy_color(P5(clip_callback_data_t * pccd,
			    int xc, int yc, int xec, int yec)),
    clip_call_copy_alpha(P5(clip_callback_data_t * pccd,
			    int xc, int yc, int xec, int yec)),
    clip_call_fill_mask(P5(clip_callback_data_t * pccd,
			   int xc, int yc, int xec, int yec)),
    clip_call_strip_tile_rectangle(P5(clip_callback_data_t * pccd,
				      int xc, int yc, int xec, int yec)),
    clip_call_strip_copy_rop(P5(clip_callback_data_t * pccd,
				int xc, int yc, int xec, int yec));

#endif /* gxclip_INCLUDED */
