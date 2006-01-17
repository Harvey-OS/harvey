/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: siinterp.c,v 1.7 2004/12/08 21:35:13 stefan Exp $ */
/* Image interpolation filter */
#include "memory_.h"
#include <assert.h>
#include "gxfixed.h"		/* for gxdda.h */
#include "gxdda.h"
#include "gxfrac.h"
#include "strimpl.h"
#include "siinterp.h"

/* ImageInterpolateEncode state */
typedef struct gx_dda_int_s {
    dda_state_struct(ia_, int, uint) state;
    dda_step_struct(ie_, int, uint) step;
} gx_dda_int_t;
typedef enum {
    SCALE_SAME = 0,
    SCALE_SAME_ALIGNED,
    SCALE_8_8,
    SCALE_8_8_ALIGNED,
    SCALE_8_16_BYTE2FRAC,
    SCALE_8_16_BYTE2FRAC_ALIGNED,
    SCALE_8_16_BYTE2FRAC_3,
    SCALE_8_16_BYTE2FRAC_3_ALIGNED,
    SCALE_8_16_GENERAL,
    SCALE_8_16_GENERAL_ALIGNED,
    SCALE_16_8,
    SCALE_16_8_ALIGNED,
    SCALE_16_16,
    SCALE_16_16_ALIGNED
} scale_case_t;
typedef struct stream_IIEncode_state_s {
    /* The client sets the params values before initialization. */
    stream_image_scale_state_common;  /* = state_common + params */
    /* The init procedure sets the following. */
    int sizeofPixelIn;		/* bytes per input pixel, 1 or 2 * Colors */
    int sizeofPixelOut;		/* bytes per output pixel, 1 or 2 * Colors */
    uint src_size;		/* bytes per row of input */
    uint dst_size;		/* bytes per row of output */
    void /*PixelOut */ *prev;	/* previous row of input data in output fmt, */
				/* [WidthIn * sizeofPixelOut] */
    void /*PixelOut */ *cur;	/* current row of input data in output fmt, */
				/* [WidthIn * sizeofPixelOut] */
    scale_case_t scale_case;
    /* The following are updated dynamically. */
    int dst_x;
    gx_dda_int_t dda_x;		/* DDA for dest X in current scan line */
    gx_dda_int_t dda_x_init;	/* initial setting of dda_x */
    int src_y, dst_y;
    gx_dda_int_t dda_y;		/* DDA for dest Y */
    int src_offset, dst_offset;
} stream_IIEncode_state;

gs_private_st_ptrs2(st_IIEncode_state, stream_IIEncode_state,
    "ImageInterpolateEncode state",
    iiencode_state_enum_ptrs, iiencode_state_reloc_ptrs,
    prev, cur);

/* Forward references */
private void s_IIEncode_release(stream_state * st);

/* Initialize the filter. */
private int
s_IIEncode_init(stream_state * st)
{
    stream_IIEncode_state *const ss = (stream_IIEncode_state *) st;
    gs_memory_t *mem = ss->memory;

    ss->sizeofPixelIn =
	ss->params.BitsPerComponentIn / 8 * ss->params.Colors;
    ss->sizeofPixelOut =
	ss->params.BitsPerComponentOut / 8 * ss->params.Colors;
    ss->src_size = ss->sizeofPixelIn * ss->params.WidthIn;
    ss->dst_size = ss->sizeofPixelOut * ss->params.WidthOut;

    /* Initialize destination DDAs. */
    ss->dst_x = 0;
    ss->src_offset = ss->dst_offset = 0;
    dda_init(ss->dda_x, 0, ss->params.WidthIn, ss->params.WidthOut);
    ss->dda_x_init = ss->dda_x;
    ss->src_y = ss->dst_y = 0;
    dda_init(ss->dda_y, 0, ss->params.HeightOut, ss->params.HeightIn);

    /* Allocate buffers for 2 rows of input data. */
    ss->prev = gs_alloc_byte_array(mem, ss->params.WidthIn,
				   ss->sizeofPixelOut, "IIEncode prev");
    ss->cur = gs_alloc_byte_array(mem, ss->params.WidthIn,
				  ss->sizeofPixelOut, "IIEncode cur");
    if (ss->prev == 0 || ss->cur == 0) {
	s_IIEncode_release(st);
	return ERRC;	/****** WRONG ******/
    }

    /* Determine the case for the inner loop. */
    ss->scale_case =
	(ss->params.BitsPerComponentIn == 8 ?
	 (ss->params.BitsPerComponentOut == 8 ?
	  (ss->params.MaxValueIn == ss->params.MaxValueOut ?
	   SCALE_SAME : SCALE_8_8) :
	  (ss->params.MaxValueIn == 255 && ss->params.MaxValueOut == frac_1 ?
	   (ss->params.Colors == 3 ? SCALE_8_16_BYTE2FRAC_3 :
	    SCALE_8_16_BYTE2FRAC) :
	   SCALE_8_16_GENERAL)) :
	 (ss->params.BitsPerComponentOut == 8 ? SCALE_16_8 :
	  ss->params.MaxValueIn == ss->params.MaxValueOut ?
	  SCALE_SAME : SCALE_16_16));

    return 0;
}

/* Process a buffer. */
private int
s_IIEncode_process(stream_state * st, stream_cursor_read * pr,
		   stream_cursor_write * pw, bool last)
{
    stream_IIEncode_state *const ss = (stream_IIEncode_state *) st;
    const scale_case_t scale_case = ss->scale_case +
	ALIGNMENT_MOD(pw->ptr, 2); /* ptr odd => buffer is aligned */
    byte *out = pw->ptr + 1;
    /****** WRONG, requires an entire output pixel ******/
    byte *limit = pw->limit + 1 - ss->sizeofPixelOut;

    /* Check whether we need to deliver any output. */

top:
    if (dda_current(ss->dda_y) > ss->dst_y) {
	/* Deliver some or all of the current scaled row. */
	while (ss->dst_x < ss->params.WidthOut) {
	    uint sx = dda_current(ss->dda_x) * ss->sizeofPixelIn;
	    const byte *in = (const byte *)ss->cur + sx;
	    int c;

	    if (out > limit) {
		pw->ptr = out - 1;
		return 1;
	    }
	    switch (scale_case) {
	    case SCALE_SAME:
	    case SCALE_SAME_ALIGNED:
		memcpy(out, in, ss->sizeofPixelIn);
		out += ss->sizeofPixelIn;
		break;
	    case SCALE_8_8:
	    case SCALE_8_8_ALIGNED:
		for (c = ss->params.Colors; --c >= 0; ++in, ++out)
		    *out = (byte)(*in * ss->params.MaxValueOut /
				  ss->params.MaxValueIn);
		break;
	    case SCALE_8_16_BYTE2FRAC:
	    case SCALE_8_16_BYTE2FRAC_ALIGNED: /* could be optimized */
	    case SCALE_8_16_BYTE2FRAC_3: /* could be optimized */
		for (c = ss->params.Colors; --c >= 0; ++in, out += 2) {
		    uint b = *in;
		    uint value = byte2frac(b);

		    out[0] = (byte)(value >> 8), out[1] = (byte)value;
		}
		break;
	    case SCALE_8_16_BYTE2FRAC_3_ALIGNED:
		{
		    uint b = in[0];

		    ((bits16 *)out)[0] = byte2frac(b);
		    b = in[1];
		    ((bits16 *)out)[1] = byte2frac(b);
		    b = in[2];
		    ((bits16 *)out)[2] = byte2frac(b);
		}
		out += 6;
		break;
	    case SCALE_8_16_GENERAL:
	    case SCALE_8_16_GENERAL_ALIGNED: /* could be optimized */
		for (c = ss->params.Colors; --c >= 0; ++in, out += 2) {
		    uint value = *in * ss->params.MaxValueOut /
			ss->params.MaxValueIn;

		    out[0] = (byte)(value >> 8), out[1] = (byte)value;
		}
		break;
	    case SCALE_16_8:
	    case SCALE_16_8_ALIGNED:
		for (c = ss->params.Colors; --c >= 0; in += 2, ++out)
		    *out = (byte)(*(const bits16 *)in *
				  ss->params.MaxValueOut /
				  ss->params.MaxValueIn);
		break;
	    case SCALE_16_16:
	    case SCALE_16_16_ALIGNED: /* could be optimized */
		for (c = ss->params.Colors; --c >= 0; in += 2, out += 2) {
		    uint value = *(const bits16 *)in *
			ss->params.MaxValueOut / ss->params.MaxValueIn;

		    out[0] = (byte)(value >> 8), out[1] = (byte)value;
		}
	    }
	    dda_next(ss->dda_x);
	    ss->dst_x++;
	}
	ss->dst_x = 0;
	ss->dst_y++;
	ss->dda_x = ss->dda_x_init;
	goto top;
    }
    pw->ptr = out - 1;
    if (ss->dst_y >= ss->params.HeightOut)
	return EOFC;

    if (ss->src_offset < ss->src_size) {
	uint count = min(ss->src_size - ss->src_offset, pr->limit - pr->ptr);

	if (count == 0)
	    return 0;
	memcpy((byte *)ss->cur + ss->src_offset, pr->ptr + 1, count);
	ss->src_offset += count;
	pr->ptr += count;
	if (ss->src_offset < ss->src_size)
	    return 0;
    }
    ss->src_offset = 0;
    ss->dst_x = 0;
    ss->dda_x = ss->dda_x_init;
    dda_next(ss->dda_y);
    goto top;
}

/* Release the filter's storage. */
private void
s_IIEncode_release(stream_state * st)
{
    stream_IIEncode_state *const ss = (stream_IIEncode_state *) st;
    gs_memory_t *mem = ss->memory;

    gs_free_object(mem, ss->cur, "IIEncode cur");
    ss->cur = 0;
    gs_free_object(mem, ss->prev, "IIEncode prev");
    ss->prev = 0;
}

/* Stream template */
const stream_template s_IIEncode_template = {
    &st_IIEncode_state, s_IIEncode_init, s_IIEncode_process, 1, 1,
    s_IIEncode_release
};
