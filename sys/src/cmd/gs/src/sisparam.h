/* Copyright (C) 1995, 1996, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sisparam.h,v 1.4 2002/02/21 22:24:54 giles Exp $ */
/* Generic image scaling stream definitions */
/* Requires strimpl.h */

#ifndef sisparam_INCLUDED
#  define sisparam_INCLUDED

/*
 * Image scaling streams all use a common set of parameters to define the
 * input and output data.  That is what we define here.
 */

/* Input values */
/*typedef byte PixelIn; */  /* per BitsPerComponentIn */
/*#define MaxValueIn 255 */  /* per MaxValueIn */

/* Output values */
/*typedef byte PixelOut; */  /* per BitsPerComponentOut */
/*#define MaxValueOut 255 */  /* per MaxValueOut */

/*
 * The 'support' S of a digital filter is the value such that the filter is
 * guaranteed to be zero for all arguments outside the range [-S..S].  We
 * limit the support so that we can put an upper bound on the time required
 * to compute an output value and on the amount of storage required for
 * X-filtered input data; this also allows us to use pre-scaled fixed-point
 * values for the weights if we wish.
 *
 * 8x8 pixels should be enough for any reasonable application....
 */
#define LOG2_MAX_ISCALE_SUPPORT 3
#define MAX_ISCALE_SUPPORT (1 << LOG2_MAX_ISCALE_SUPPORT)

/* Define image scaling stream parameters. */
typedef struct stream_image_scale_params_s {
    int Colors;			/* >= 1 */
    int BitsPerComponentIn;	/* bits per input value, 8 or 16 */
    uint MaxValueIn;		/* max value of input component, */
				/* 0 < MaxValueIn < 1 << BitsPerComponentIn */
    int WidthIn, HeightIn;	/* > 0 */
    int BitsPerComponentOut;	/* bits per output value, 8 or 16 */
    uint MaxValueOut;		/* max value of output component, */
				/* 0 < MaxValueOut < 1 << BitsPerComponentOut*/
    int WidthOut, HeightOut;	/* > 0 */
} stream_image_scale_params_t;

/* Define a generic image scaling stream state. */

#define stream_image_scale_state_common\
    stream_state_common;\
    stream_image_scale_params_t params

typedef struct stream_image_scale_state_s {
    stream_image_scale_state_common;
} stream_image_scale_state;

#endif /* sisparam_INCLUDED */
