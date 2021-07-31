/* Copyright (C) 1996, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: spngpx.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Definitions for PNGPredictor filters */
/* Requires strimpl.h */

#ifndef spngpx_INCLUDED
#  define spngpx_INCLUDED

/* PNGPredictorDecode / PNGPredictorEncode */
typedef struct stream_PNGP_state_s {
    stream_state_common;
    /* The client sets the following before initialization. */
    int Colors;			/* # of colors, 1..16 */
    int BitsPerComponent;	/* 1, 2, 4, 8, 16 */
    uint Columns;		/* >0 */
    int Predictor;		/* 10-15, only relevant for Encode */
    /* The init procedure computes the following. */
    uint row_count;		/* # of bytes per row */
    byte end_mask;		/* mask for left-over bits in last byte */
    int bpp;			/* bytes per pixel */
    byte *prev_row;		/* previous row */
    int case_index;		/* switch index for case dispatch, */
				/* set dynamically when decoding */
    /* The following are updated dynamically. */
    long row_left;		/* # of bytes left in row */
    byte prev[32];		/* previous samples */
} stream_PNGP_state;

#define private_st_PNGP_state()	/* in sPNGP.c */\
  gs_private_st_ptrs1(st_PNGP_state, stream_PNGP_state,\
    "PNGPredictorEncode/Decode state", pngp_enum_ptrs, pngp_reloc_ptrs,\
    prev_row)
#define s_PNGP_set_defaults_inline(ss)\
  ((ss)->Colors = 1, (ss)->BitsPerComponent = 8, (ss)->Columns = 1,\
   (ss)->Predictor = 15,\
		/* Clear pointers */\
   (ss)->prev_row = 0)
extern const stream_template s_PNGPD_template;
extern const stream_template s_PNGPE_template;

#endif /* spngpx_INCLUDED */
